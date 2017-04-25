#ifndef LLRUBY_ISEQ_COMPILER_H
#define LLRUBY_ISEQ_COMPILER_H

#include "llruby/iseq.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

namespace llruby {
  class IseqCompiler {
   private:
    llvm::LLVMContext context;
    const Iseq& iseq;

   public:
    IseqCompiler(const Iseq& value):iseq(value) {};
    llvm::Function* Compile();
  };
}

#endif // LLRUBY_ISEQ_COMPILER_H
