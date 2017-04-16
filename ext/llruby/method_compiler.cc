#include "ruby.h"

static VALUE
rb_mcomp_compile(RB_UNUSED_VAR(VALUE self), VALUE method)
{
  return Qnil;
}

void
Init_llruby_method_compiler(VALUE rb_mLLRuby)
{
  VALUE rb_mMethodCompiler = rb_define_module_under(rb_mLLRuby, "MethodCompiler");
  rb_define_singleton_method(rb_mMethodCompiler, "compile", RUBY_METHOD_FUNC(rb_mcomp_compile), 1);
}
