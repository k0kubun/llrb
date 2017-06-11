#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "ruby.h"
#include "compiler.h"

uint64_t
llrb_compile_iseq(const rb_iseq_t *iseq)
{
  LLVMModuleRef mod = LLVMModuleCreateWithName("llrb");
  LLVMTypeRef arg_types[] = { LLVMInt64Type() };
  LLVMValueRef func = LLVMAddFunction(mod, "llrb_exec", LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMBuildRet(builder, LLVMConstInt(LLVMInt64Type(), Qnil, false));

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

  return LLVMGetFunctionAddress(engine, "llrb_exec");
}
