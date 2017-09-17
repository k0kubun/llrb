/*
 * optimizer.cc: Optimizes LLVM IR using LLVM Passes.
 *
 * This file is created as C++ file for following 2 reasons:
 *
 *   1. LLVM needs to be linked by C++ linker, even if we use LLVM-C API.
 *      C extension is linked using C++ linker if we have *.cc file.
 *      We should find a better way to do that.
 *
 *   2. Just to make it easy to port opt(1)'s code. Sometimes LLVM-C APIs
 *      don't include C++'s ones. But it looks current implementation
 *      doesn't depend on such APIs. So this reason can be fixed.
 *
 */
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Support/Host.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/InferFunctionAttrs.h"
#include "llvm/LinkAllPasses.h"

#include "llvm/ADT/Statistic.h"

namespace llrb {

static inline std::string GetFeaturesStr()
{
  llvm::SubtargetFeatures ret;
  llvm::StringMap<bool> features;

  if (llvm::sys::getHostCPUFeatures(features)) {
    for (auto &feature : features) {
      ret.AddFeature(feature.first(), feature.second);
    }
  }

  return ret.getString();
}

// Setting function attributes is required for function inlining.
static void
SetFunctionAttributes(llvm::Module *mod)
{
  std::string cpu = llvm::sys::getHostCPUName();
  std::string features = GetFeaturesStr();

  for (auto &func : *mod) {
    auto &ctx = func.getContext();
    llvm::AttributeSet attrs = func.getAttributes(), newAttrs;

    if (!cpu.empty()) {
      newAttrs = newAttrs.addAttribute(ctx, llvm::AttributeSet::FunctionIndex, "target-cpu", cpu);
    }
    if (!features.empty()) {
      newAttrs = newAttrs.addAttribute(ctx, llvm::AttributeSet::FunctionIndex, "target-features", features);
    }

    newAttrs = attrs.addAttributes(ctx, llvm::AttributeSet::FunctionIndex, newAttrs);
    func.setAttributes(newAttrs);
  }
}

static void
RunFunctionPasses(llvm::Module *mod, llvm::Function *func)
{
  std::unique_ptr<llvm::legacy::FunctionPassManager> fpm;
  fpm.reset(new llvm::legacy::FunctionPassManager(mod));

  fpm->add(llvm::createVerifierPass());

  llvm::PassManagerBuilder builder;
  builder.OptLevel  = 3;
  builder.SizeLevel = 0;
  builder.populateFunctionPassManager(*fpm);

  fpm->doInitialization();
  for (llvm::Function &f : *mod) {
    fpm->run(f);
  }
  fpm->doFinalization();
}

static void
PopulateModulePassManager(llvm::legacy::PassManager& mpm)
{
  //mpm.add(llvm::createForceFunctionAttrsLegacyPass());

  // addInitialAliasAnalysisPasses(MPM);
  mpm.add(llvm::createTypeBasedAAWrapperPass());
  //mpm.add(llvm::createScopedNoAliasAAWrapperPass());

  // if (!DisableUnitAtATime)
  //mpm.add(llvm::createInferFunctionAttrsLegacyPass());
  //mpm.add(llvm::createIPSCCPPass());              // IP SCCP
  //mpm.add(llvm::createGlobalOptimizerPass());     // Optimize out global vars
  //mpm.add(llvm::createPromoteMemoryToRegisterPass()); // Promote any localized global vars
  //mpm.add(llvm::createDeadArgEliminationPass());  // Dead argument elimination
  //mpm.add(llvm::createInstructionCombiningPass());// Clean up after IPCP & DAE
  //mpm.add(llvm::createCFGSimplificationPass());   // Clean up after IPCP & DAE

  // if (EnableNonLTOGlobalsModRef)
  mpm.add(llvm::createGlobalsAAWrapperPass());

  // Start of CallGraph SCC passes.
  //mpm.add(llvm::createPruneEHPass());
  mpm.add(llvm::createFunctionInliningPass(412));
  //mpm.add(llvm::createPostOrderFunctionAttrsPass());
  //mpm.add(llvm::createArgumentPromotionPass());   // Scalarize uninlined fn args

  // Start of function pass.
  //mpm.add(llvm::createSROAPass()); // Break up aggregate allocas, using SSAUpdater.
  mpm.add(llvm::createEarlyCSEPass());              // Catch trivial redundancies
  mpm.add(llvm::createJumpThreadingPass());         // Thread jumps.
  //mpm.add(llvm::createCorrelatedValuePropagationPass()); // Propagate conditionals
  mpm.add(llvm::createCFGSimplificationPass());     // Merge & remove BBs
  mpm.add(llvm::createInstructionCombiningPass());  // Combine silly seq's

  //mpm.add(llvm::createTailCallEliminationPass()); // Eliminate tail calls
  //mpm.add(llvm::createCFGSimplificationPass());     // Merge & remove BBs
  mpm.add(llvm::createReassociatePass());           // Reassociate expressions
  //mpm.add(llvm::createLoopRotatePass(-1)); // Rotate Loop - disable header duplication at -Oz
  mpm.add(llvm::createLICMPass());                  // Hoist loop invariants
  mpm.add(llvm::createLoopUnswitchPass(false));
  //mpm.add(llvm::createCFGSimplificationPass());
  mpm.add(llvm::createInstructionCombiningPass());
  mpm.add(llvm::createIndVarSimplifyPass());        // Canonicalize indvars
  //mpm.add(llvm::createLoopIdiomPass());             // Recognize idioms like memset.
  //mpm.add(llvm::createLoopDeletionPass());          // Delete dead loops
  //mpm.add(llvm::createSimpleLoopUnrollPass()); // Unroll small loops

  // if (OptLevel > 1)
  //mpm.add(llvm::createMergedLoadStoreMotionPass()); // Merge ld/st in diamonds
  mpm.add(llvm::createGVNPass(false));  // Remove redundancies
  mpm.add(llvm::createMemCpyOptPass());             // Remove memcpy / form memset
  //mpm.add(llvm::createSCCPPass());                  // Constant prop with SCCP

  // Delete dead bit computations (instcombine runs after to fold away the dead
  // computations, and then ADCE will run later to exploit any new DCE
  // opportunities that creates).
  //mpm.add(llvm::createBitTrackingDCEPass());        // Delete dead bit computations

  // Run instcombine after redundancy elimination to exploit opportunities
  // opened up by them.
  //mpm.add(llvm::createInstructionCombiningPass());
  //mpm.add(llvm::createJumpThreadingPass());         // Thread jumps
  //mpm.add(llvm::createCorrelatedValuePropagationPass());
  mpm.add(llvm::createDeadStoreEliminationPass());  // Delete dead stores
  mpm.add(llvm::createLICMPass());

  //mpm.add(llvm::createAggressiveDCEPass());         // Delete dead instructions
  mpm.add(llvm::createCFGSimplificationPass()); // Merge & remove BBs
  mpm.add(llvm::createInstructionCombiningPass());  // Clean up after everything.

  // FIXME: This is a HACK! The inliner pass above implicitly creates a CGSCC
  // pass manager that we are specifically trying to avoid. To prevent this
  // we must insert a no-op module pass to reset the pass manager.
  //mpm.add(llvm::createBarrierNoopPass());

  mpm.add(llvm::createReversePostOrderFunctionAttrsPass());

  // Remove avail extern fns and globals definitions if we aren't
  // compiling an object file for later LTO. For LTO we want to preserve
  // these so they are eligible for inlining at link-time. Note if they
  // are unreferenced they will be removed by GlobalDCE later, so
  // this only impacts referenced available externally globals.
  // Eventually they will be suppressed during codegen, but eliminating
  // here enables more opportunity for GlobalDCE as it may make
  // globals referenced by available external functions dead
  // and saves running remaining passes on the eliminated functions.
  //mpm.add(llvm::createEliminateAvailableExternallyPass());

  // We add a fresh GlobalsModRef run at this point. This is particularly
  // useful as the above will have inlined, DCE'ed, and function-attr
  // propagated everything. We should at this point have a reasonably minimal
  // and richly annotated call graph. By computing aliasing and mod/ref
  // information for all local globals here, the late loop passes and notably
  // the vectorizer will be able to use them to help recognize vectorizable
  // memory operations.
  //
  // Note that this relies on a bug in the pass manager which preserves
  // a module analysis into a function pass pipeline (and throughout it) so
  // long as the first function pass doesn't invalidate the module analysis.
  // Thus both Float2Int and LoopRotate have to preserve AliasAnalysis for
  // this to work. Fortunately, it is trivial to preserve AliasAnalysis
  // (doing nothing preserves it as it is required to be conservatively
  // correct in the face of IR changes).
  mpm.add(llvm::createGlobalsAAWrapperPass());

  // if (RunFloat2Int)
  //mpm.add(llvm::createFloat2IntPass());

  // Re-rotate loops in all our loop nests. These may have fallout out of
  // rotated form due to GVN or other transformations, and the vectorizer relies
  // on the rotated form. Disable header duplication at -Oz.
  //mpm.add(llvm::createLoopRotatePass(-1));

  mpm.add(llvm::createLoopVectorizePass(false, false));

  // FIXME: Because of #pragma vectorize enable, the passes below are always
  // inserted in the pipeline, even when the vectorizer doesn't run (ex. when
  // on -O1 and no #pragma is found). Would be good to have these two passes
  // as function calls, so that we can only pass them when the vectorizer
  // changed the code.
  mpm.add(llvm::createInstructionCombiningPass());

  //mpm.add(llvm::createCFGSimplificationPass());
  //mpm.add(llvm::createInstructionCombiningPass());

  mpm.add(llvm::createLoopUnrollPass());    // Unroll small loops

  // LoopUnroll may generate some redundency to cleanup.
  mpm.add(llvm::createInstructionCombiningPass());

  // Runtime unrolling will introduce runtime check in loop prologue. If the
  // unrolled loop is a inner loop, then the prologue will be inside the
  // outer loop. LICM pass can help to promote the runtime check out if the
  // checked value is loop invariant.
  mpm.add(llvm::createLICMPass());

  // After vectorization and unrolling, assume intrinsics may tell us more
  // about pointer alignments.
  //mpm.add(llvm::createAlignmentFromAssumptionsPass());

  // FIXME: We shouldn't bother with this anymore.
  //mpm.add(llvm::createStripDeadPrototypesPass()); // Get rid of dead prototypes

  // GlobalOpt already deletes dead functions and globals, at -O2 try a
  // late pass of GlobalDCE.  It is capable of deleting dead cycles.
  //mpm.add(llvm::createGlobalDCEPass());         // Remove dead fns and globals.
  //mpm.add(llvm::createConstantMergePass());     // Merge dup global constants
}

static void
RunModulePasses(llvm::Module *mod)
{
  llvm::legacy::PassManager mpm;

  llvm::PassManagerBuilder builder;
  builder.OptLevel  = 3;
  builder.SizeLevel = 0;
  builder.Inliner = llvm::createFunctionInliningPass(100);
  builder.populateModulePassManager(mpm);
  if (0) PopulateModulePassManager(mpm);

  mpm.add(llvm::createVerifierPass());
  mpm.run(*mod);
}

static void
OptimizeFunction(llvm::Module *mod, llvm::Function *func, bool enable_stats)
{
  SetFunctionAttributes(mod);
  RunFunctionPasses(mod, func);
  if (enable_stats) llvm::EnableStatistics();
  RunModulePasses(mod);
  if (enable_stats) llvm::PrintStatistics();
}

} // namespace llrb

extern "C" {
void
llrb_optimize_function(LLVMModuleRef cmod, LLVMValueRef cfunc, bool enable_stats)
{
  llvm::Module *mod = llvm::unwrap(cmod);
  llvm::Function *func = llvm::unwrap<llvm::Function>(cfunc);
  llrb::OptimizeFunction(mod, func, enable_stats);
}
} // extern "C"
