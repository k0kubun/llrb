// codegen.cc is separated from compiler.c because some CRuby's internal headers can't be compiled with C++ compiler.
// And llvm-c headers don't provide all APIs. I use codegen.cc to link them. But some can be replaced with C API and we should do it later.

#include "llvm/Support/TargetSelect.h"

extern "C" {
void
Init_codegen(void)
{
  // They are required to be called to generate native function.
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
}
} // extern "C"
