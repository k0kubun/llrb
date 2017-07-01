#include <stdbool.h>
#include "ruby.h"

//static bool
//llrb_check_already_compiled(const rb_iseq_t *iseq)
//{
//  return iseq->body->iseq_size == 3
//    && iseq->body->iseq_encoded[0] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function]
//    && iseq->body->iseq_encoded[2] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave];
//}

// LLRB::JIT.preview_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  //const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  //if (llrb_check_already_compiled(iseq)) return Qfalse;

  //LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
  //LLVMDumpModule(mod);
  //LLVMDisposeModule(mod);
  return Qtrue;
}

// LLRB::JIT.compile_iseq
// @param  [Array]   iseqw  - RubyVM::InstructionSequence instance
// @return [Boolean] return true if compiled
static VALUE
rb_jit_compile_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  //const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  //if (llrb_check_already_compiled(iseq)) return Qfalse;

  //LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
  //uint64_t func = llrb_create_native_func(mod, llrb_funcname);
  //LLVMDisposeModule(mod);
  //if (!func) {
  //  fprintf(stderr, "Failed to create native function...\n");
  //  return Qfalse;
  //}

  //llrb_replace_iseq_with_cfunc(iseq, (rb_insn_func_t)func);
  return Qtrue;
}

static VALUE
rb_jit_is_compiled(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  //const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  //return llrb_check_already_compiled(iseq) ? Qtrue : Qfalse;
  return Qfalse;
}

void
Init_llrb(void)
{
  // Required to generate native code.
  //LLVMInitializeNativeTarget();
  //LLVMInitializeNativeAsmPrinter();
  //LLVMInitializeNativeAsmParser();
  //LLVMLinkInMCJIT();

  VALUE rb_mLLRB = rb_define_module("LLRB");
  VALUE rb_mJIT = rb_define_module_under(rb_mLLRB, "JIT");
  rb_define_singleton_method(rb_mJIT, "preview_iseq", RUBY_METHOD_FUNC(rb_jit_preview_iseq), 1);
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 1);
  rb_define_singleton_method(rb_mJIT, "is_compiled",  RUBY_METHOD_FUNC(rb_jit_is_compiled), 1);

  //Init_compiler(rb_mJIT);
}
