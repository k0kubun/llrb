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
  fprintf(stderr, "\n");
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

static void
llrb_compile_iseq_body(LLVMModuleRef mod, LLVMBuilderRef builder, const char *funcname, const rb_iseq_t *iseq)
{
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

  llrb_compile_iseq_body(mod, builder, funcname, iseq);
  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
