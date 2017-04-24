#include "llruby/ruby.h"
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

// @param  [Class,Module] klass
// @param  [String,Symbol] method_name
// @return [nil]
static VALUE
rb_nativem_define_internal(RB_UNUSED_VAR(VALUE self), VALUE klass, VALUE method_name)
{
  VALUE name_str = rb_convert_type(method_name, T_STRING, "String", "to_s");
  uint64_t func = llruby::NativeMethod().CreateFunction();
  if (func == 0) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qnil;
  }
  rb_define_method(klass, RSTRING_PTR(name_str), RUBY_METHOD_FUNC(func), 0);
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
} // extern "C"
