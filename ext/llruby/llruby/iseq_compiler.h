#ifndef LLRUBY_ISEQ_COMPILER_H
#define LLRUBY_ISEQ_COMPILER_H

#include "llruby/iseq.h"
#include "llvm/IR/Function.h"

namespace llruby {
  class IseqCompiler {
   public:
    llvm::Function* Compile(const Iseq& iseq);
  };
}

#endif // LLRUBY_ISEQ_COMPILER_H
