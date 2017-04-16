#include "llruby.h"

VALUE rb_mLLRuby;

void
Init_llruby(void)
{
  rb_mLLRuby = rb_define_module("LLRuby");
}
