#include "llrb.h"
#include "iseq.h"
#include "compiler.h"
#include "llvm/Support/TargetSelect.h"

// LLRB::JIT.precompile_internal
// @param  [Array]   iseq_array - result of RubyVM::InstructionSequence#to_a
// @param  [Class]   klass      - class to define method
// @param  [Symbol]  method_sym - method name to define
// @param  [Integer] arity      - method arity
// @return [Boolean] return true if precompiled
static VALUE
rb_jit_precompile_internal(RB_UNUSED_VAR(VALUE self), VALUE ruby_iseq, VALUE klass, VALUE method_sym, VALUE arity, VALUE dry_run)
{
  Check_Type(ruby_iseq, T_ARRAY);
  llrb::Iseq iseq(ruby_iseq);
  uint64_t func = llrb::Compiler().Compile(iseq, RTEST(dry_run));
  if (RTEST(dry_run)) return Qfalse;
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }

  VALUE method_str = rb_convert_type(method_sym, T_STRING, "String", "to_s");
  rb_define_method(klass, RSTRING_PTR(method_str), RUBY_METHOD_FUNC(func), FIX2INT(arity));
  return Qtrue;
}

extern "C" {
  extern void Init_llrb_fcore(VALUE rb_mLLRB);

  void
  Init_llrb(void)
  {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    VALUE rb_mLLRB = rb_define_module("LLRB");
    VALUE rb_mJIT = rb_define_module_under(rb_mLLRB, "JIT");
    rb_define_singleton_method(rb_mJIT, "precompile_internal", RUBY_METHOD_FUNC(rb_jit_precompile_internal), 5);

    Init_llrb_fcore(rb_mLLRB);
  }
}
