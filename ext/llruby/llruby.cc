#include "llruby/ruby.h"
#include "llruby/iseq.h"

static VALUE
rb_jit_precompile_internal(RB_UNUSED_VAR(VALUE self), VALUE iseq_array, VALUE klass, VALUE method_sym)
{
  llruby::Iseq iseq(iseq_array);
  return Qnil;
}

extern "C" {
  void
  Init_llruby(void)
  {
    VALUE rb_mLLRuby = rb_define_module("LLRuby");
    VALUE rb_mJIT = rb_define_module_under(rb_mLLRuby, "JIT");
    rb_define_singleton_method(rb_mJIT, "precompile_internal", RUBY_METHOD_FUNC(rb_jit_precompile_internal), 3);
  }
}
