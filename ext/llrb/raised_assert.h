#ifndef LLRB_RAISED_ASSERT_H
#define LLRB_RAISED_ASSERT_H

#include "llrb.h"

void raised_assert(bool expected, const char* message) {
  if (!expected) {
    rb_raise(rb_eRuntimeError, "%s", message);
  }
}

#endif // LLRB_RAISED_ASSERT_H
