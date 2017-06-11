#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "ruby.h"
#include "compiler.h"

const rb_iseq_t *rb_iseqw_to_iseq(VALUE iseqw);
static const char *llrb_funcname = "llrb_exec";

// LLRB::JIT.preview_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @param  [Object]  recv  - method receiver (not used for now)
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, RB_UNUSED_VAR(VALUE recv))
{
  LLVMModuleRef mod = llrb_compile_iseq(rb_iseqw_to_iseq(iseqw), llrb_funcname);
  LLVMDumpModule(mod);
  return Qtrue;
}

// LLRB::JIT.compile_iseq
// @param  [Array]   iseqw  - RubyVM::InstructionSequence instance
// @param  [Object]  recv   - method receiver (not used for now)
// @param  [Class]   klass  - method class
// @param  [Symbol]  name   - method name to define
// @param  [Integer] arity  - method arity
// @return [Boolean] return true if compiled
static VALUE
rb_jit_compile_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, RB_UNUSED_VAR(VALUE recv), VALUE klass, VALUE name, VALUE arity)
{
  LLVMModuleRef mod = llrb_compile_iseq(rb_iseqw_to_iseq(iseqw), llrb_funcname);
  uint64_t func = llrb_create_native_func(mod, llrb_funcname);
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }

  VALUE name_str = rb_convert_type(name, T_STRING, "String", "to_s");
  rb_define_method(klass, RSTRING_PTR(name_str), RUBY_METHOD_FUNC(func), FIX2INT(arity));
  return Qtrue;
}

void
Init_llrb(void)
{
  // Required to generate native code.
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMLinkInMCJIT();

  VALUE rb_mLLRB = rb_define_module("LLRB");
  VALUE rb_mJIT = rb_define_module_under(rb_mLLRB, "JIT");
  rb_define_singleton_method(rb_mJIT, "preview_iseq", RUBY_METHOD_FUNC(rb_jit_preview_iseq), 2);
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 5);
}
