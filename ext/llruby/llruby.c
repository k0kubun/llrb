#include "llruby.h"

VALUE rb_mLlruby;

void
Init_llruby(void)
{
  rb_mLlruby = rb_define_module("Llruby");
}
