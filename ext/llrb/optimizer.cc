#include "llvm/Support/CBindingWrapping.h"

extern "C" {
void
llrb_optimize_function(LLVMModuleRef cmod, LLVMValueRef cfunc)
{
  //llvm::Module *mod = llvm::unwrap(cmod);
  //llvm::Function *func = llvm::unwrap<llvm::Function>(cfunc);
  //llrb::OptimizeFunction(mod, func);
}
} // extern "C"
