#ifndef LLRUBY_NATIVE_COMPILER_H
#define LLRUBY_NATIVE_COMPILER_H

#include "llruby/iseq.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

namespace llruby {
  class NativeCompiler {
   private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> mod;

   public:
    NativeCompiler();
    uint64_t Compile(const Iseq& iseq);
  };
}

#endif // LLRUBY_NATIVE_COMPILER_H
