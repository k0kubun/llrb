#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"

void
llrb_add_target_metadata(LLVMModuleRef mod)
{
  char *triple = LLVMGetDefaultTargetTriple();
  LLVMSetTarget(mod, triple);
  LLVMDisposeMessage(triple);
  LLVMSetDataLayout(mod, "e-m:e-i64:64-f80:128-n8:16:32:64-S128"); // TODO: Dynamically generate this
}

// Main function of this file. This function links LLRB's internal functions,
// inline them and optimize the whole code using LLVM Pass.
LLVMModuleRef
llrb_optimize_module(LLVMModuleRef mod)
{
  llrb_add_target_metadata(mod);
  //LLVMDumpModule(mod);
  return mod;
}
