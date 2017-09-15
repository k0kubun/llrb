/*
 * compiler/funcs.h: External function declaration called by JIT-ed code.
 * This header is used only by compiler.c.
 * TODO: Automatically generate this from something.
 */

#ifndef LLRB_COMPILER_FUNCS_H
#define LLRB_COMPILER_FUNCS_H

#include <string.h>
#include "ruby.h"
#include "llvm-c/BitReader.h"
#include "llvm-c/Linker.h"

#define LLRB_EXTERN_FUNC_MAX_ARGC 6
struct llrb_extern_func {
  unsigned int return_type; // 0 = void, 32 = 32bit int, 64 = 64bit int
  unsigned int argc;
  unsigned int argv[LLRB_EXTERN_FUNC_MAX_ARGC]; // 0 = void, 32 = 32bit int, 64 = 64bit int
  bool unlimited;
  const char *name;
  bool has_bc;
};

// TODO: support 32bit environment
static struct llrb_extern_func llrb_extern_funcs[] = {
  { 64, 0, { 0  }, false, "rb_hash_new", false },
  { 64, 1, { 64 }, false, "llrb_insn_opt_str_freeze", true },
  { 64, 1, { 64 }, false, "llrb_insn_putspecialobject", true },
  { 64, 1, { 64 }, false, "llrb_self_from_cfp", true },
  { 64, 1, { 64 }, false, "rb_ary_clear", false },
  { 64, 1, { 64 }, false, "rb_ary_resurrect", false },
  { 64, 1, { 64 }, false, "rb_gvar_get", false },
  { 64, 1, { 64 }, false, "rb_obj_as_string", false },
  { 64, 1, { 64 }, false, "rb_str_freeze", false },
  { 64, 1, { 64 }, false, "rb_str_resurrect", false },
  { 64, 1, { 64 }, true,  "llrb_insn_concatstrings", true },
  { 64, 1, { 64 }, true,  "rb_ary_new_from_args", false },
  { 0,  2, { 64, 64 }, false, "llrb_insn_setspecial", true },
  { 0,  2, { 64, 64 }, false, "llrb_push_result", true },
  { 0,  2, { 64, 64 }, false, "llrb_set_pc", true },
  { 64, 2, { 64, 32 }, false, "rb_reg_new_ary", false },
  { 64, 2, { 64, 64 }, false, "llrb_insn_concatarray", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getclassvariable", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getlocal_level0", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getlocal_level1", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getspecial", true },
  //{ 64, 2, { 64, 64 }, false, "llrb_insn_opt_newarray_max", true },
  //{ 64, 2, { 64, 64 }, false, "llrb_insn_opt_newarray_min", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_mult", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_div", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_mod", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_eq", true },
  { 64, 6, { 64, 64, 64, 64, 64, 64 }, false, "llrb_insn_opt_neq", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_le", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_gt", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_ge", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_ltlt", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_aref", true },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_opt_aset", true },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_getinstancevariable", true },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_setinstancevariable", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_lt", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_minus", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_plus", true },
  { 64, 2, { 64, 64 }, false, "llrb_insn_splatarray", true },
  { 64, 2, { 64, 64 }, false, "rb_gvar_set", false },
  { 64, 2, { 64, 64 }, false, "rb_ivar_get", false },
  { 64, 2, { 64, 64 }, true,  "llrb_insn_toregexp", false },
  { 64, 2, { 64, 64 }, true,  "rb_funcall", false },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setclassvariable", true },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setlocal_level0", true },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setlocal_level1", true },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_checkkeyword", true },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_checkmatch", true },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_getlocal", true },
  { 64, 3, { 64, 64, 64 }, false, "rb_hash_aset", false },
  { 64, 3, { 64, 64, 64 }, false, "rb_ivar_set", false },
  { 64, 3, { 64, 64, 64 }, false, "rb_range_new", false },
  { 0,  4, { 64, 64, 32, 64 }, false, "llrb_insn_trace", true },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_setconstant", true },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_setlocal", true },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_throw", true },
  { 64, 4, { 64, 64, 64, 32 }, false, "vm_get_ev_const", false },
  { 64, 4, { 64, 64, 64, 32 }, true,  "llrb_insn_invokeblock", true },
  { 64, 4, { 64, 64, 64, 64 }, false, "llrb_insn_defined", true },
  { 64, 5, { 64, 64, 64, 64, 64 }, false, "llrb_insn_opt_send_without_block", true },
  { 64, 6, { 64, 64, 64, 64, 64, 32 }, true, "llrb_insn_invokesuper", true },
  { 64, 6, { 64, 64, 64, 64, 64, 32 }, true, "llrb_insn_send", true },
};
static size_t llrb_extern_func_num = sizeof(llrb_extern_funcs) / sizeof(struct llrb_extern_func);

static inline LLVMTypeRef
llrb_num_to_type(unsigned int num)
{
  switch (num) {
    case 64:
      return LLVMInt64Type();
    case 32:
      return LLVMInt32Type();
    case 0:
      return LLVMVoidType();
    default:
      rb_raise(rb_eCompileError, "'%d' is unexpected for llrb_num_to_type", num);
  }
}

static void
llrb_link_module(LLVMModuleRef mod, const char *funcname)
{
  char *bc_name = ZALLOC_N(char, strlen(LLRB_BITCODE_DIR "/") + strlen(funcname) + strlen(".bc") + 1); // freed in this function
  strcat(bc_name, LLRB_BITCODE_DIR "/");
  strcat(bc_name, funcname);
  strcat(bc_name, ".bc");

  LLVMMemoryBufferRef buf;
  char *err;
  if (LLVMCreateMemoryBufferWithContentsOfFile(bc_name, &buf, &err)) {
    fprintf(stderr, "Failed to load func bitcode: '%s'\n", bc_name);
    rb_raise(rb_eCompileError, "LLVMCreateMemoryBufferWithContentsOfFile Error: %s", err);
  }
  xfree(bc_name);

  LLVMModuleRef func_mod;
  if (LLVMParseBitcode2(buf, &func_mod)) {
    rb_raise(rb_eCompileError, "LLVMParseBitcode2 Failed!");
  }
  LLVMDisposeMemoryBuffer(buf);

  LLVMLinkModules2(mod, func_mod);
}

static LLVMValueRef
llrb_get_function(LLVMModuleRef mod, const char *name)
{
  LLVMValueRef func = LLVMGetNamedFunction(mod, name);
  if (func) return func;

  for (size_t i = 0; i < llrb_extern_func_num; i++) {
    if (strcmp(name, llrb_extern_funcs[i].name)) continue;

    if (llrb_extern_funcs[i].has_bc) {
      llrb_link_module(mod, llrb_extern_funcs[i].name);
      func = LLVMGetNamedFunction(mod, name);
      if (func) {
        return func;
      } else {
        rb_raise(rb_eCompileError, "'%s' was not found in bitcode file", name);
      }
    }

    LLVMTypeRef arg_types[LLRB_EXTERN_FUNC_MAX_ARGC];
    for (unsigned int j = 0; j < llrb_extern_funcs[i].argc; j++) {
      arg_types[j] = llrb_num_to_type(llrb_extern_funcs[i].argv[j]);
    }
    return LLVMAddFunction(mod, llrb_extern_funcs[i].name, LLVMFunctionType(
          llrb_num_to_type(llrb_extern_funcs[i].return_type), arg_types,
          llrb_extern_funcs[i].argc, llrb_extern_funcs[i].unlimited));
  }

  rb_raise(rb_eCompileError, "'%s' is not defined in llrb_extern_funcs", name);
}

#endif // LLRB_COMPILER_FUNCS_H
