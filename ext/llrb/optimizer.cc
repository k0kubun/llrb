#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"

namespace llrb {

static void
SetTargetMetadata(llvm::Module *mod)
{
  llvm::TargetMachine *targetMachine = llvm::EngineBuilder().selectTarget();
  mod->setDataLayout(targetMachine->createDataLayout());
  mod->setTargetTriple(targetMachine->getTargetTriple().getTriple());
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
  builder.Inliner = llvm::createFunctionInliningPass(builder.OptLevel, builder.SizeLevel);
  builder.populateModulePassManager(mpm);

  mpm.add(llvm::createVerifierPass());
  mpm.run(*mod);
}

static void
OptimizeFunction(llvm::Module *mod, llvm::Function *func)
{
  SetTargetMetadata(mod);
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
