#include <stdio.h>
#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"
#include "llvm-c/Linker.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Transforms/IPO.h"
#include "llvm-c/Transforms/Scalar.h"

static LLVMModuleRef
llrb_create_insns_module()
{
  LLVMMemoryBufferRef buf;
  char *err;
  if (LLVMCreateMemoryBufferWithContentsOfFile("ext/module.bc", &buf, &err)) {
    fprintf(stderr, "LLVMCreateMemoryBufferWithContentsOfFile Error: %s\n", err);
    return 0;
  }

  LLVMModuleRef ret;
  if (LLVMParseBitcode2(buf, &ret)) {
    fprintf(stderr, "LLVMParseBitcode2 Failed!\n");
    return 0;
  }
  LLVMDisposeMemoryBuffer(buf);
  return ret;
}

// Main function of this file. This function links LLRB's internal functions,
// inline them and optimize the whole code using LLVM Pass.
LLVMModuleRef
llrb_optimize_module()
{
  return llrb_create_insns_module();
}
