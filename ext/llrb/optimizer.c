#include <stdio.h>
#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"
#include "llvm-c/Linker.h"
#include "llvm-c/TargetMachine.h"

static LLVMModuleRef
llrb_create_insns_module()
{
  LLVMMemoryBufferRef buf;
  char *err;
  if (LLVMCreateMemoryBufferWithContentsOfFile("ext/insns.bc", &buf, &err)) {
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

static void
llrb_link_insns(LLVMModuleRef mod)
{
  LLVMModuleRef insns_mod = llrb_create_insns_module();
  if (insns_mod == 0) return;

  if (LLVMLinkModules2(mod, insns_mod)) {
    fprintf(stderr, "LLVMLinkModules2 Failed!\n");
  }
}

// Main function of this file. This function links LLRB's internal functions,
// inline them and optimize the whole code using LLVM Pass.
LLVMModuleRef
llrb_optimize_module(LLVMModuleRef mod)
{
  llrb_link_insns(mod);
  //LLVMDumpModule(mod);
  return mod;
}
