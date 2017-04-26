#ifndef LLRUBY_NATIVE_COMPILER_H
#define LLRUBY_NATIVE_COMPILER_H

#include "iseq.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

namespace llruby {
  class NativeCompiler {
   private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;

   public:
    NativeCompiler() : builder(context) {}
    uint64_t Compile(const Iseq& iseq, bool dry_run);

   private:
    uint64_t CreateNativeFunction(llvm::Function *func, std::unique_ptr<llvm::Module> mod);
    llvm::Function* CompileIseq(const Iseq& iseq, llvm::Module* mod);
  };
}

#endif // LLRUBY_NATIVE_COMPILER_H
