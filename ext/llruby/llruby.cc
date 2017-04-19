#include "ruby.h"
#include "llruby/iseq.h"
#include "llruby/iseq_compiler.h"
#include "llruby/native_method.h"

// @param  [Array] ruby_iseq - Return value of RubyVM::InstructionSequence#to_a
// @return [LLRuby::LLVMIR]
static VALUE
rb_iseqcomp_compile_internal(RB_UNUSED_VAR(VALUE self), VALUE ruby_iseq)
{
  Check_Type(ruby_iseq, T_ARRAY);

  llruby::Iseq iseq(ruby_iseq);
  llruby::IseqCompiler compiler(iseq);
  compiler.Compile();

  return Qnil;
}

static VALUE
rb_nativem_define_internal(RB_UNUSED_VAR(VALUE self), VALUE klass, VALUE method_name)
{
  llruby::NativeMethod method;
  method.Define();
  return Qnil;
}

extern "C" {
void
Init_llruby(void)
{
  VALUE rb_mLLRuby = rb_define_module("LLRuby");

  VALUE rb_mIseqCompiler = rb_define_module_under(rb_mLLRuby, "IseqCompiler");
  rb_define_singleton_method(rb_mIseqCompiler, "compile_internal", RUBY_METHOD_FUNC(rb_iseqcomp_compile_internal), 1);

  VALUE rb_cNativeMethod = rb_define_class_under(rb_mLLRuby, "NativeMethod", rb_cObject);
  rb_define_method(rb_cNativeMethod, "define_internal", RUBY_METHOD_FUNC(rb_nativem_define_internal), 2);
}
}; // extern "C"
