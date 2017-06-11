#include "llrb.h"
#include "compiler.h"

const rb_iseq_t *rb_iseqw_to_iseq(VALUE iseqw);

// LLRB::JIT.preview_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @param  [Object]  recv  - method receiver
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, VALUE recv)
{
  return Qtrue;
}

// LLRB::JIT.compile_iseq
// @param  [Array]   iseqw  - RubyVM::InstructionSequence instance
// @param  [Object]  recv   - method receiver
// @param  [Class]   klass  - method class
// @param  [Symbol]  name   - method name to define
// @param  [Integer] arity  - method arity
// @return [Boolean] return true if compiled
static VALUE
rb_jit_compile_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, VALUE recv, VALUE klass, VALUE name, VALUE arity)
{
  uint64_t func = llrb_compile_iseq(rb_iseqw_to_iseq(iseqw));
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }
  return Qtrue;
}

void
Init_llrb(void)
{
  VALUE rb_mLLRB = rb_define_module("LLRB");
  VALUE rb_mJIT = rb_define_module_under(rb_mLLRB, "JIT");
  rb_define_singleton_method(rb_mJIT, "preview_iseq", RUBY_METHOD_FUNC(rb_jit_preview_iseq), 2);
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 5);
}
