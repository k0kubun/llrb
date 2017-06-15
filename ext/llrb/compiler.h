#ifndef LLRB_COMPILER_H
#define LLRB_COMPILER_H

// For rb_iseq_t and others
#include "cruby.h"

uint64_t llrb_create_native_func(LLVMModuleRef mod, const char *funcname);
LLVMModuleRef llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname);
void Init_compiler(VALUE rb_mJIT);

#endif // LLRB_COMPILER_H
