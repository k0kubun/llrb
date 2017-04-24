#ifndef LLRUBY_NATIVE_METHOD_H
#define LLRUBY_NATIVE_METHOD_H

#include "llruby/iseq.h"
#include "llvm/IR/Function.h"

namespace llruby {
  class NativeMethod {
   public:
    llvm::Function *llvmfunc;

    NativeMethod(llvm::Function *func) : llvmfunc(func) {}
    uint64_t CreateNativeFunction();
  };
}

#endif // LLRUBY_NATIVE_METHOD_H
