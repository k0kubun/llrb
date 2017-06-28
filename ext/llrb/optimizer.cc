#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CBindingWrapping.h"
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
ApplyFunctionPasses(llvm::Module *mod, llvm::Function *func)
{
  auto fpm = llvm::make_unique<llvm::legacy::FunctionPassManager>(mod);

  //fpm->add(llvm::createInstructionCombiningPass()); // SEGV with opt_newarray_min/max, throw, jump, branchif/unless
  fpm->add(llvm::createReassociatePass());
  fpm->add(llvm::createGVNPass());
  fpm->add(llvm::createCFGSimplificationPass());

  fpm->doInitialization();
  fpm->run(*func);
}

static void
OptimizeFunction(llvm::Module *mod, llvm::Function *func)
{
  SetTargetMetadata(mod);
  ApplyFunctionPasses(mod, func);
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
