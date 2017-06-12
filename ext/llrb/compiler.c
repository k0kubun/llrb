#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "ruby.h"
#include "compiler.h"
#include "insns.inc"
#include "insns_info.inc"

static VALUE rb_eCompileError;

// Emulates rb_control_frame's sp, which is function local
struct llrb_cfstack {
  unsigned int size;
  unsigned int max;
  LLVMValueRef *body;
};

// Store compiler's internal state and shared variables
struct llrb_compiler {
  struct llrb_cfstack stack;
  const struct rb_iseq_constant_body *body;
  const char *funcname;
  LLVMBuilderRef builder;
  LLVMModuleRef mod;
};

static LLVMValueRef
llvm_value(VALUE value)
{
  return LLVMConstInt(LLVMInt64Type(), value, false);
}

static void
llrb_stack_push(struct llrb_cfstack *stack, LLVMValueRef value)
{
  if (stack->size >= stack->max) {
    rb_raise(rb_eCompileError, "LLRB's internal stack overflow: max=%d, next size=%d", stack->max, stack->size+1);
  }
  stack->body[stack->size] = value;
  stack->size++;
}

static LLVMValueRef
llrb_stack_pop(struct llrb_cfstack *stack)
{
  if (stack->size <= 0) {
    rb_raise(rb_eCompileError, "LLRB's internal stack underflow: next size=%d", stack->size-1);
  }
  stack->size--;
  return stack->body[stack->size];
}

// Don't use `rb_iseq_original_iseq` to avoid unnecessary memory allocation.
int rb_vm_insn_addr2insn(const void *addr);

// Return sorted unique Ruby array of BasicBlock start positions like [0, 2, 8].
//
// It's constructed in the following rule.
//   Rule 1: 0 is always included
//   Rule 2: All TS_OFFSET numers are included
//   Rule 3: Positions immediately after jump, branchif and branchunless are included
static VALUE
llrb_basic_block_starts(const struct rb_iseq_constant_body *body)
{
  // Rule 1
  VALUE positions = rb_ary_new_capa(1);
  rb_ary_push(positions, INT2FIX(0));

  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);

    // Rule 2
    for (int j = 1; j < insn_len(insn); j++) {
      VALUE op = body->iseq_encoded[i+j];
      switch (insn_op_type(insn, j-1)) {
        case TS_OFFSET:
          rb_ary_push(positions, INT2FIX((int)(i+insn_len(insn)+op)));
          break;
      }
    }

    // Rule 3
    switch (insn) {
      case YARVINSN_jump:
      case YARVINSN_branchif:
      case YARVINSN_branchunless:
        rb_ary_push(positions, INT2FIX(i+insn_len(insn)));
        break;
    }

    i += insn_len(insn);
  }
  return rb_funcall(rb_ary_sort_bang(positions), rb_intern("uniq!"), 0);
}

// Given llrb_basic_block_starts result like [0, 2, 8, 9], it returns a Hash
// whose value specifies basic block ends like { 0 => 1, 2 => 7, 8 => 8, 9 => 10 }.
static VALUE
llrb_basic_block_end_by_start(const struct rb_iseq_constant_body *body)
{
  VALUE end_by_start = rb_hash_new();
  VALUE starts = llrb_basic_block_starts(body); // TODO: free? And it's duplicated with llrb_basic_block_by_start.

  for (long i = 0; i < RARRAY_LEN(starts); i++) {
    VALUE start = RARRAY_AREF(starts, i);

    if (i == RARRAY_LEN(starts)-1) {
      rb_hash_aset(end_by_start, start, INT2FIX(body->iseq_size-1)); // This assumes the end is always "leave". Is it true?
    } else {
      VALUE next_start = RARRAY_AREF(starts, i+1);
      rb_hash_aset(end_by_start, start, INT2FIX(FIX2INT(next_start)-1));
    }
  }
  return end_by_start;
}

// Given basic block starts like [0, 2, 8], it returns a Hash like
// { 0 => label_0 BasicBlock, 2 => label_2 BasicBlock, 8 => label_8 BasicBlock }.
//
// Those blocks are appended to a given func.
// This hash and its blocks are necessary to be built first for forwared reference.
static VALUE
llrb_basic_block_by_start(const struct rb_iseq_constant_body *body, LLVMValueRef func)
{
  VALUE block_by_start = rb_hash_new();
  VALUE starts = llrb_basic_block_starts(body); // TODO: free?

  for (long i = 0; i < RARRAY_LEN(starts); i++) {
    VALUE start = RARRAY_AREF(starts, i);
    VALUE label = rb_str_new_cstr("label_"); // TODO: free?
    rb_str_catf(label, "%d", FIX2INT(start));
    rb_hash_aset(block_by_start, start, (VALUE)LLVMAppendBasicBlock(func, RSTRING_PTR(label)));
  }
  return block_by_start;
}

static void
llrb_disasm_insns(const struct rb_iseq_constant_body *body)
{
  fprintf(stderr, "\n== disasm: LLRB ================================\n");
  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    fprintf(stderr, "%04d %-27s [%-4s] ", i, insn_name(insn), insn_op_types(insn));

    for (int j = 1; j < insn_len(insn); j++) {
      VALUE op = body->iseq_encoded[i+j];
      switch (insn_op_type(insn, j-1)) {
        case TS_VALUE:
          fprintf(stderr, "%-4s ", RSTRING_PTR(rb_inspect(op)));
          break;
        case TS_NUM:
          fprintf(stderr, "%-4ld ", (rb_num_t)op);
          break;
        case TS_OFFSET:
          fprintf(stderr, "%"PRIdVALUE" ", (VALUE)(i + j + op + 1));
          break;
      }
    }
    fprintf(stderr, "\n");
    i += insn_len(insn);
  }
  VALUE starts = llrb_basic_block_starts(body); // TODO: free?
  VALUE end_by_start = llrb_basic_block_end_by_start(body); // TODO: free?
  fprintf(stderr, "\nbasic block starts: %s\n", RSTRING_PTR(rb_inspect(starts)));
  fprintf(stderr, "basic block ends by starts: %s\n\n", RSTRING_PTR(rb_inspect(end_by_start)));
}

LLVMValueRef
llrb_argument_at(struct llrb_compiler *c, unsigned index)
{
  LLVMValueRef func = LLVMGetNamedFunction(c->mod, c->funcname);
  return LLVMGetParam(func, index);
}

static void
llrb_compile_insn(struct llrb_compiler *c, const int insn, const VALUE *operands)
{
  switch (insn) {
    case YARVINSN_nop:
      break; // do nothing
    case YARVINSN_getlocal_OP__WC__0: {
      unsigned local_size = c->body->local_table_size;
      unsigned arg_size   = c->body->param.size;
      llrb_stack_push(&c->stack, llrb_argument_at(c, 3 - local_size + 2 * arg_size - (unsigned)operands[0])); // XXX: is this okay????
      break;
    }
    case YARVINSN_trace:
      break; // TODO: implement
    case YARVINSN_putobject:
      llrb_stack_push(&c->stack, llvm_value(operands[0]));
      break;
    case YARVINSN_putobject_OP_INT2FIX_O_0_C_:
      llrb_stack_push(&c->stack, llvm_value(INT2FIX(0)));
      break;
    case YARVINSN_putobject_OP_INT2FIX_O_1_C_:
      llrb_stack_push(&c->stack, llvm_value(INT2FIX(1)));
      break;
    case YARVINSN_leave:
      if (c->stack.size != 1) {
        rb_raise(rb_eCompileError, "unexpected stack size at leave: %d", c->stack.size);
      }
      LLVMBuildRet(c->builder, llrb_stack_pop(&c->stack));
      break;
    default:
      llrb_disasm_insns(c->body);
      rb_raise(rb_eCompileError, "Unhandled insn at llrb_compile_insn: %s", insn_name(insn));
      break;
  }
}

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

LLVMModuleRef
llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname)
{
  LLVMModuleRef mod = LLVMModuleCreateWithName("llrb");

  LLVMTypeRef *arg_types = ALLOC_N(LLVMTypeRef, iseq->body->param.size+1);
  for (unsigned i = 0; i <= iseq->body->param.size; i++) arg_types[i] = LLVMInt64Type();
  LLVMValueRef func = LLVMAddFunction(mod, funcname,
      LLVMFunctionType(LLVMInt64Type(), arg_types, iseq->body->param.size+1, false));

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, block);

  struct llrb_compiler compiler = (struct llrb_compiler){
    .stack = (struct llrb_cfstack){
      .body = ALLOC_N(LLVMValueRef, iseq->body->stack_max),
      .size = 0,
      .max  = iseq->body->stack_max
    },
    .body = iseq->body,
    .funcname = funcname,
    .builder = builder,
    .mod = mod
  };

  for (unsigned int i = 0; i < iseq->body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)iseq->body->iseq_encoded[i]);
    llrb_compile_insn(&compiler, insn, iseq->body->iseq_encoded + (i+1));
    i += insn_len(insn);
  }
  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
