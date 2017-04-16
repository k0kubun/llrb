#include "ruby.h"

void Init_llruby_iseq_compiler(VALUE rb_mLLRuby);

extern "C" {
void
Init_llruby(void)
{
  VALUE rb_mLLRuby = rb_define_module("LLRuby");
  Init_llruby_iseq_compiler(rb_mLLRuby);
}
}; // extern "C"
