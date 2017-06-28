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

#include "cruby/probes_helper.h"
static inline void
llrb_insn_trace2(rb_thread_t *th, rb_control_frame_t *cfp, rb_event_flag_t flag, VALUE val)
{
  if (RUBY_DTRACE_METHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_METHOD_RETURN_ENABLED() ||
      RUBY_DTRACE_CMETHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_CMETHOD_RETURN_ENABLED()) {

    switch (flag) {
      case RUBY_EVENT_CALL:
        RUBY_DTRACE_METHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_CALL:
        RUBY_DTRACE_CMETHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_RETURN:
        RUBY_DTRACE_METHOD_RETURN_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_RETURN:
        RUBY_DTRACE_CMETHOD_RETURN_HOOK(th, 0, 0);
        break;
    }
  }

  // TODO: Confirm it works, especially for :line event. We may need to update program counter.
  EXEC_EVENT_HOOK(th, flag, cfp->self, 0, 0, 0 /* id and klass are resolved at callee */, val);
}

void rb_vm_env_write(const VALUE *ep, int index, VALUE v);
static inline void
llrb_insn_setlocal_level0(rb_control_frame_t *cfp, lindex_t idx, VALUE val)
{
  rb_vm_env_write(cfp->ep, -(int)idx, val);
}

static inline VALUE
llrb_insn_getlocal_level0(rb_control_frame_t *cfp, lindex_t idx)
{
  return *(cfp->ep - idx);
}

static inline VALUE
llrb_insn_opt_lt(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    SIGNED_VALUE a = lhs, b = rhs;

    if (a < b) {
      return Qtrue;
    } else {
      return Qfalse;
    }
  }
  return rb_funcall(lhs, '<', 1, rhs);
}

rb_control_frame_t *
llrb_exec2(rb_thread_t *th, rb_control_frame_t *cfp)
{
  llrb_insn_trace2(th, cfp, 8, (VALUE)52);
  llrb_insn_trace2(th, cfp, 1, (VALUE)52);
  llrb_insn_trace2(th, cfp, 1, (VALUE)52);

  llrb_insn_setlocal_level0(cfp, 3, INT2FIX(0));
  while (RTEST(llrb_insn_opt_lt(llrb_insn_getlocal_level0(cfp, 3), INT2FIX(6000000)))) {
    llrb_insn_trace2(th, cfp, 1, (VALUE)52);
    llrb_insn_setlocal_level0(cfp, 3, INT2FIX(FIX2INT(llrb_insn_getlocal_level0(cfp, 3)) + 1));
  }

  llrb_insn_trace2(th, cfp, 16, (VALUE)8);
  *(cfp->sp) = llrb_insn_getlocal_level0(cfp, 3);
  cfp->sp += 1;
  return cfp;
}

void
llrb_replace_iseq_with_cfunc(const rb_iseq_t *iseq, rb_insn_func_t funcptr)
{
  VALUE *new_iseq_encoded = ALLOC_N(VALUE, 3);
  new_iseq_encoded[0] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function];
  new_iseq_encoded[1] = (VALUE)llrb_exec2;
  new_iseq_encoded[2] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave]; // There may be the case that last insn is not :leave.

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
  LLVMDisposeModule(mod);
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
  rb_define_singleton_method(rb_mJIT, "preview_iseq", RUBY_METHOD_FUNC(rb_jit_preview_iseq), 2);
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 5);
  rb_define_singleton_method(rb_mJIT, "is_compiled",  RUBY_METHOD_FUNC(rb_jit_is_compiled), 1);

  Init_compiler(rb_mJIT);
}
