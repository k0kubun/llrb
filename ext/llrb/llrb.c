#include <stdbool.h>
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "ruby.h"
#include "compiler.h"
#include "insns.inc"

const rb_iseq_t *rb_iseqw_to_iseq(VALUE iseqw);
static const char *llrb_funcname = "llrb_exec";

uint64_t
llrb_create_native_func(LLVMModuleRef mod, const char *funcname)
{
  LLVMExecutionEngineRef engine;
  char *error;
  if (LLVMCreateJITCompilerForModule(&engine, mod, 0, &error) != 0) {
    fprintf(stderr, "Failed to create JIT compiler...\n");

    if (error) {
      fprintf(stderr, "LLVMCreateJITCompilerForModule: %s\n", error);
      LLVMDisposeMessage(error);
      return 0;
    }
  }
  return LLVMGetFunctionAddress(engine, funcname);
}

void
llrb_replace_iseq_with_cfunc(const rb_iseq_t *iseq, rb_insn_func_t funcptr)
{
  VALUE *new_iseq_encoded = ALLOC_N(VALUE, 3);
  new_iseq_encoded[0] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function];
  new_iseq_encoded[1] = (VALUE)funcptr;
  new_iseq_encoded[2] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave];

  // Don't we need to prevent race condition by another thread? Will GVL protect us?
  iseq->body->iseq_encoded = new_iseq_encoded;
  iseq->body->iseq_size = 3;
}

static bool
llrb_check_already_compiled(const rb_iseq_t *iseq)
{
  return iseq->body->iseq_size == 3
    && iseq->body->iseq_encoded[0] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function]
    && iseq->body->iseq_encoded[2] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave];
}

// LLRB::JIT.preview_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @param  [Object]  recv  - method receiver (not used for now)
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, RB_UNUSED_VAR(VALUE recv))
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  if (llrb_check_already_compiled(iseq)) return Qfalse;

  LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
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
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  if (llrb_check_already_compiled(iseq)) return Qfalse;

  LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
  uint64_t func = llrb_create_native_func(mod, llrb_funcname);
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }

  //VALUE name_str = rb_convert_type(name, T_STRING, "String", "to_s");
  //rb_define_method(klass, RSTRING_PTR(name_str), RUBY_METHOD_FUNC(func), FIX2INT(arity));
  llrb_replace_iseq_with_cfunc(iseq, (rb_insn_func_t)func);
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

  Init_compiler(rb_mJIT);
}
