#ifndef LLRUBY_ISEQ_COMPILER_H
#define LLRUBY_ISEQ_COMPILER_H

#include "llruby/iseq.h"
#include "llvm/IR/Function.h"

namespace llruby {
  class IseqCompiler {
   private:
    const Iseq& iseq;

   public:
    IseqCompiler(const Iseq& value):iseq(value) {};
    llvm::Function* Compile();
  };
}

#endif // LLRUBY_ISEQ_COMPILER_H
