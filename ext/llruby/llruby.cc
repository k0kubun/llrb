#include "ruby.h"

VALUE rb_mLLRuby;

extern "C" {
void
Init_llruby(void)
{
  rb_mLLRuby = rb_define_module("LLRuby");
}
}; // extern "C"
