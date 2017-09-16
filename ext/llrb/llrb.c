/*
 * llrb.c: Has Ruby interface and native code generation.
 *
 * LLRB's internal design:
 *   parser.c:     llrb_parse_iseq()         # ISeq -> Control Flow Graph
 *   compiler.c:   llrb_compile_cfg()        # Control Flow Graph -> LLVM IR
 *   optimizer.cc: llrb_optimize_function()  # LLVM IR -> optimized LLVM IR
 *   llrb.c:       llrb_create_native_func() # optimized LLVM IR -> Native code
 */
#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "cruby.h"
#include "cruby_extra/insns.inc"

static const char *llrb_funcname = "llrb_exec";

LLVMModuleRef llrb_compile_iseq(const struct rb_iseq_constant_body *body, const VALUE *new_iseq_encoded, const char* funcname, bool enable_stats);
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
llrb_replace_iseq_with_cfunc(const rb_iseq_t *iseq, VALUE *new_iseq_encoded, rb_insn_func_t funcptr)
{
  new_iseq_encoded[0] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function];
  new_iseq_encoded[1] = (VALUE)funcptr;

  // JIT code may change program counter to address after `new_iseq_encoded[2]` to get `catch_table` work.
  // So filling "leave" insns is necessary here.
  for (unsigned int i = 2; i < iseq->body->iseq_size; i++) {
    // Is there a case that last insn is not "leave"?
    new_iseq_encoded[i] = (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave];
  }

  // Changing iseq->body->iseq_encoded will not break threads executing old iseq_encoded
  // because program counter will still point to old iseq's address. This operation is considered safe.
  // But in the same time it's hard to free memory of old iseq_encoded.
  iseq->body->iseq_encoded = new_iseq_encoded;
}

static bool
llrb_check_already_compiled(const rb_iseq_t *iseq)
{
  return iseq->body->iseq_size >= 3
    && iseq->body->iseq_encoded[0] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_opt_call_c_function]
    && iseq->body->iseq_encoded[2] == (VALUE)rb_vm_get_insns_address_table()[YARVINSN_leave];
}

static bool
llrb_should_not_compile(const rb_iseq_t *iseq)
{
  extern bool llrb_check_not_compilable(const rb_iseq_t *iseq);
  return llrb_check_already_compiled(iseq) || llrb_check_not_compilable(iseq);
}

// LLRB::JIT.preview_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @return [Boolean] return true if compiled
static VALUE
rb_jit_preview_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw)
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  if (llrb_should_not_compile(iseq)) return Qfalse;

  LLVMModuleRef mod = llrb_compile_iseq(iseq->body, iseq->body->iseq_encoded, llrb_funcname, false);
  LLVMDumpModule(mod);
  LLVMDisposeModule(mod);
  return Qtrue;
}

// Used by profiler.c too
VALUE
llrb_compile_iseq_to_method(const rb_iseq_t *iseq, bool enable_stats)
{
  if (llrb_should_not_compile(iseq)) return Qfalse;

  // Creating new_iseq_encoded before compilation to calculate program counter.
  VALUE *new_iseq_encoded = ALLOC_N(VALUE, iseq->body->iseq_size); // Never freed.
  LLVMModuleRef mod = llrb_compile_iseq(iseq->body, new_iseq_encoded, llrb_funcname, enable_stats);

  uint64_t func = llrb_create_native_func(mod, llrb_funcname);
  //LLVMDisposeModule(mod); // This causes SEGV: "corrupted double-linked list".
  if (!func) {
    fprintf(stderr, "Failed to create native function...\n");
    return Qfalse;
  }

  llrb_replace_iseq_with_cfunc(iseq, new_iseq_encoded, (rb_insn_func_t)func);
  return Qtrue;
}

// LLRB::JIT.compile_iseq
// @param  [Array]   iseqw - RubyVM::InstructionSequence instance
// @param  [Boolean] enable_stats - Enable LLVM Pass statistics
// @return [Boolean] return true if compiled
static VALUE
rb_jit_compile_iseq(RB_UNUSED_VAR(VALUE self), VALUE iseqw, VALUE enable_stats)
{
  const rb_iseq_t *iseq = rb_iseqw_to_iseq(iseqw);
  return llrb_compile_iseq_to_method(iseq, RTEST(enable_stats));
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
  rb_define_singleton_method(rb_mJIT, "compile_iseq", RUBY_METHOD_FUNC(rb_jit_compile_iseq), 2);
  rb_define_singleton_method(rb_mJIT, "is_compiled",  RUBY_METHOD_FUNC(rb_jit_is_compiled), 1);

  extern void Init_profiler(VALUE rb_mJIT);
  Init_profiler(rb_mJIT);

  extern void Init_parser(VALUE rb_mJIT);
  Init_parser(rb_mJIT);

  extern void Init_compiler(VALUE rb_mJIT);
  Init_compiler(rb_mJIT);
}
