#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "ruby.h"
#include "compiler.h"
#include "insns.inc"
#include "insns_info.inc"

static VALUE rb_eCompileError;

// https://github.com/ruby/ruby/blob/v2_4_1/compile.c#L695-L707
static int
rb_vm_insn_addr2insn(const void *addr)
{
  int insn;
  const void * const *table = rb_vm_get_insns_address_table();

  for (insn = 0; insn < VM_INSTRUCTION_SIZE; insn++) {
    if (table[insn] == addr) {
      return insn;
    }
  }
  rb_bug("rb_vm_insn_addr2insn (llrb): invalid insn address: %p", addr);
}

static void
dump_insns(const struct rb_iseq_constant_body *body)
{
  fprintf(stderr, "[insns dump]\n");
  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    fprintf(stderr, "%s: %d [%s]\n", insn_name(insn), insn_len(insn)-1, insn_op_types(insn));

    for (int j = 1; j < insn_len(insn); j++) {
      if (insn_op_type(insn, j-1) == 'V') {
        fprintf(stderr, "  %s\n", RSTRING_PTR(rb_inspect(body->iseq_encoded[i+j])));
      }
    }
    i += insn_len(insn);
  }
  fprintf(stderr, "\n");
}

static void
llrb_compile_insn(LLVMModuleRef mod, const struct rb_iseq_constant_body *body, const int insn, const VALUE *operands)
{
  switch (insn) {
    case YARVINSN_nop:
      break; // do nothing
    case YARVINSN_trace:
      break; // TODO: implement
    case YARVINSN_putobject:
      break; // TODO: implement
    case YARVINSN_leave:
      break; // TODO: implement
    default:
      dump_insns(body);
      rb_raise(rb_eCompileError, "Unhandled insn at llrb_compile_insn: %s", insn_name(insn));
      break;
  }
}

static void
llrb_compile_iseq_body(LLVMModuleRef mod, const struct rb_iseq_constant_body *body)
{
  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    llrb_compile_insn(mod, body, insn, body->iseq_encoded + (i+1));
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
  LLVMTypeRef arg_types[] = { LLVMInt64Type() };
  LLVMValueRef func = LLVMAddFunction(mod, funcname, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, block);

  llrb_compile_iseq_body(mod, iseq->body);

  LLVMBuildRet(builder, LLVMConstInt(LLVMInt64Type(), Qnil, false));
  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
