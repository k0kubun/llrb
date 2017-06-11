#include "llrb.h"

VALUE rb_mLLRB;

void
Init_llrb(void)
{
  rb_mLLRB = rb_define_module("LLRB");
}
