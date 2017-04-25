#include "llruby/ruby.h"
#include "llruby/iseq.h"
#include "llruby/native_compiler.h"

static llruby::NativeCompiler native_compiler;

// LLRuby::JIT.precompile_internal
// @param [Array]  iseq_array - result of RubyVM::InstructionSequence#to_a
// @param [Class]  klass      - class to define method
// @param [Symbol] method_sym - method name to define
static VALUE
rb_jit_precompile_internal(RB_UNUSED_VAR(VALUE self), VALUE ruby_iseq, VALUE klass, VALUE method_sym)
{
  Check_Type(ruby_iseq, T_ARRAY);
  llruby::Iseq iseq(ruby_iseq);
  native_compiler.Compile(iseq);
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
