#include "ruby.h"
#include "llruby/iseq.h"

// NOTE: We may be able to instantiate faster using the definition of "struct rb_iseq_struct" for specific Ruby versions.
static void
instantiate_cpp_iseq(VALUE ruby_iseq, llruby::Iseq *cpp_iseq)
{
}

// @param  [Array] ruby_iseq - Return value of RubyVM::InstructionSequence#to_a
// @return [LLRuby::LLVMIR]
static VALUE
rb_iseqcomp_compile_internal(RB_UNUSED_VAR(VALUE self), VALUE ruby_iseq)
{
  Check_Type(ruby_iseq, T_ARRAY);

  llruby::Iseq cpp_iseq;
  instantiate_cpp_iseq(ruby_iseq, &cpp_iseq);

  return Qnil;
}

void
Init_llruby_iseq_compiler(VALUE rb_mLLRuby)
{
  VALUE rb_mIseqCompiler = rb_define_module_under(rb_mLLRuby, "IseqCompiler");
  rb_define_singleton_method(rb_mIseqCompiler, "compile_internal", RUBY_METHOD_FUNC(rb_iseqcomp_compile_internal), 1);
}
