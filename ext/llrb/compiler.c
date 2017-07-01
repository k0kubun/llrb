#include <stdbool.h>
#include <string.h>
#include "llvm-c/Core.h"
#include "cruby.h"
#include "funcs.h"

// Store compiler's internal state and shared variables
struct llrb_compiler {
  const struct rb_iseq_constant_body *body;
  LLVMValueRef func;
  LLVMBuilderRef builder;
  LLVMModuleRef mod;
};

static LLVMValueRef
llrb_value(VALUE value)
{
  return LLVMConstInt(LLVMInt64Type(), value, false); // TODO: support 32bit for VALUE type
}

static inline LLVMValueRef
llrb_get_cfp(struct llrb_compiler *c)
{
  return LLVMGetParam(c->func, 1);
}

static LLVMValueRef
llrb_plus(struct llrb_compiler *c, LLVMValueRef lhs, LLVMValueRef rhs)
{
  LLVMValueRef args[] = { lhs, rhs };
  return LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_opt_plus"), args, 2, "");
}

static void
llrb_compile_prototype(struct llrb_compiler *c, LLVMModuleRef mod)
{
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(c->func, "entry");
  LLVMPositionBuilderAtEnd(c->builder, block);

  LLVMValueRef v1 = llrb_plus(c, llrb_value(INT2FIX(1)), llrb_value(INT2FIX(2)));
  LLVMValueRef v2 = llrb_plus(c, v1, llrb_value(INT2FIX(3)));
  LLVMValueRef v3 = llrb_plus(c, v2, llrb_value(INT2FIX(4)));
  LLVMValueRef v4 = llrb_plus(c, v3, llrb_value(INT2FIX(5)));

  LLVMValueRef args[] = { llrb_get_cfp(c), v4 };
  LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_push_result"), args, 2, "");
  LLVMBuildRet(c->builder, llrb_get_cfp(c));
}

LLVMModuleRef
llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname)
{
  LLVMModuleRef mod = LLVMModuleCreateWithName("llrb");

  LLVMTypeRef args[] = { LLVMInt64Type(), LLVMInt64Type() };
  LLVMValueRef func = LLVMAddFunction(mod, funcname,
      LLVMFunctionType(LLVMInt64Type(), args, 2, false));

  struct llrb_compiler compiler = (struct llrb_compiler){
    .body = iseq->body,
    .func = func,
    .builder = LLVMCreateBuilder(),
    .mod = mod,
  };

  llrb_compile_prototype(&compiler, mod);

  extern void llrb_optimize_function(LLVMModuleRef cmod, LLVMValueRef cfunc);
  llrb_optimize_function(mod, func);

  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
