/*
 * llrb.c: Has Ruby interface and native code generation.
 *
 * LLRB's internal design:
 *   parser.c:     llrb_parse_iseq()         # ISeq -> Control Flow Graph
 *   compiler.c:   llrb_compile_iseq()       # Control Flow Graph -> LLVM IR
 *   optimizer.cc: llrb_optimize_function()  # LLVM IR -> optimized LLVM IR
 *   llrb.c:       llrb_create_native_func() # optimized LLVM IR -> Native code
 */
#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "cruby.h"
#include "cruby_extra/insns.inc"

static const char *llrb_funcname = "llrb_exec";

LLVMModuleRef llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname);
const rb_iseq_t *rb_iseqw_to_iseq(VALUE iseqw);

static uint64_t
llrb_create_native_func(LLVMModuleRef mod, const char *funcname)
{
  LLVMExecutionEngineRef engine;
  char *error;
  if (LLVMCreateJITCompilerForModule(&engine, mod, LLVMCodeGenLevelAggressive, &error) != 0) {
    fprintf(stderr, "Failed to create JIT compiler...\n");

    if (error) {
      fprintf(stderr, "LLVMCreateJITCompilerForModule: %s\n", error);
      LLVMDisposeMessage(error);
      return 0;
    }
  }
  return LLVMGetFunctionAddress(engine, funcname);
}

static void
llrb_replace_iseq_with_cfunc(const rb_iseq_t *iseq, rb_insn_func_t funcptr)
{
  VALUE *new_iseq_encoded = ALLOC_N(VALUE, 3);
  new_iseq_encoded[0] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function];
  new_iseq_encoded[1] = (VALUE)funcptr;
  new_iseq_encoded[2] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave]; // There may be the case that last insn is not :leave.

  // Don't we need to prevent race condition by another thread? Will GVL protect us? Can we use tracepoint to hook it?
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
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  if (llrb_check_already_compiled(iseq)) return Qfalse;

  LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
  LLVMDumpModule(mod);
  LLVMDisposeModule(mod);
  return Qtrue;
}

// LLRB::JIT.compile_iseq
// @param  [Array]   iseqw  - RubyVM::InstructionSequence instance
// @return [Boolean] return true if compiled
static VALUE
rb_jit_compile_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  if (llrb_check_already_compiled(iseq)) return Qfalse;

  LLVMModuleRef mod = llrb_compile_iseq(iseq, llrb_funcname);
  uint64_t func = llrb_create_native_func(mod, llrb_funcname);
  LLVMDisposeModule(mod);
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }

  llrb_replace_iseq_with_cfunc(iseq, (rb_insn_func_t)func);
  return Qtrue;
}

static VALUE
rb_jit_is_compiled(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  return llrb_check_already_compiled(iseq) ? Qtrue : Qfalse;
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
  rb_define_singleton_method(rb_mJIT, "preview_iseq", RUBY_METHOD_FUNC(rb_jit_preview_iseq), 1);
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 1);
  rb_define_singleton_method(rb_mJIT, "is_compiled",  RUBY_METHOD_FUNC(rb_jit_is_compiled), 1);

  extern void Init_compiler(VALUE rb_mJIT);
  Init_compiler(rb_mJIT);
}
