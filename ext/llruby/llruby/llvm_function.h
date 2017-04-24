#ifndef LLRUBY_LLVM_FUNCTION_H
#define LLRUBY_LLVM_FUNCTION_H

#include "ruby.h"

namespace llruby {
  llvm::Function* GetLLVMFunction(VALUE llvm_function_obj);
  VALUE WrapLLVMFunction(llvm::Function *func);
}

#endif // LLRUBY_LLVM_FUNCTION_H
