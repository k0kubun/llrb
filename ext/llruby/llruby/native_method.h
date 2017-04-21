#ifndef LLRUBY_NATIVE_METHOD_H
#define LLRUBY_NATIVE_METHOD_H

#include "llruby/iseq.h"

namespace llruby {
  class NativeMethod {
   public:
    uint64_t CreateFunction();
  };
}

#endif // LLRUBY_NATIVE_METHOD_H
