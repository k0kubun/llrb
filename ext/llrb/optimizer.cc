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
RunModulePasses(llvm::Module *mod)
{
  llvm::legacy::PassManager mpm;

  llvm::PassManagerBuilder builder;
  builder.OptLevel  = 3;
  builder.SizeLevel = 0;
  builder.Inliner = llvm::createFunctionInliningPass(412);
  builder.populateModulePassManager(mpm);

  mpm.add(llvm::createVerifierPass());
  mpm.run(*mod);
}

static void
OptimizeFunction(llvm::Module *mod, llvm::Function *func)
{
  SetFunctionAttributes(mod);
  RunFunctionPasses(mod, func);
  RunModulePasses(mod);
}

} // namespace llrb

extern "C" {
void
llrb_optimize_function(LLVMModuleRef cmod, LLVMValueRef cfunc)
{
  llvm::Module *mod = llvm::unwrap(cmod);
  llvm::Function *func = llvm::unwrap<llvm::Function>(cfunc);
  llrb::OptimizeFunction(mod, func);
}
} // extern "C"
