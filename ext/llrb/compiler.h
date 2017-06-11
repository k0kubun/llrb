#ifndef LLRB_COMPILER_H
#define LLRB_COMPILER_H

// For rb_iseq_t and others
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "cruby/internal.h"
#include "cruby/vm_core.h"
#pragma GCC diagnostic pop
#include "cruby/method.h"

uint64_t llrb_create_native_func(LLVMModuleRef mod, const char *funcname);
LLVMModuleRef llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname);

#endif // LLRB_COMPILER_H
