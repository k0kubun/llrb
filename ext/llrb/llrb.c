#include "llrb.h"

VALUE rb_mLlrb;

void
Init_llrb(void)
{
  rb_mLlrb = rb_define_module("Llrb");
}
