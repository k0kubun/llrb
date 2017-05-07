#ifndef LLRUBY_NATIVE_COMPILER_H
#define LLRUBY_NATIVE_COMPILER_H

#include <vector>
#include "iseq.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#pragma clang diagnostic pop

namespace llruby {
  class NativeCompiler {
   private:
    std::vector<llvm::Value *> stack;
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;

   public:
    NativeCompiler() : builder(context) {}
    uint64_t Compile(const Iseq& iseq, bool dry_run);

   private:
    uint64_t CreateNativeFunction(llvm::Function *func, std::unique_ptr<llvm::Module> mod);
    llvm::Function* CompileIseq(const Iseq& iseq, llvm::Module* mod);
    void CompileInstruction(const std::vector<Object>& instruction, llvm::Module* mod, llvm::Function* rb_funcallf);
    llvm::Value* CompileObject(const Object& object);
  };
}

#endif // LLRUBY_NATIVE_COMPILER_H
