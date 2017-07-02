/*
 * compiler/funcs.h: External function declaration called by JIT-ed code.
 * This header is used only by compiler.c.
 * TODO: Automatically generate this from something.
 */

#ifndef LLRB_COMPILER_FUNCS_H
#define LLRB_COMPILER_FUNCS_H

#define LLRB_EXTERN_FUNC_MAX_ARGC 6
struct llrb_extern_func {
  unsigned int return_type; // 0 = void, 32 = 32bit int, 64 = 64bit int
  unsigned int argc;
  unsigned int argv[LLRB_EXTERN_FUNC_MAX_ARGC]; // 0 = void, 32 = 32bit int, 64 = 64bit int
  bool unlimited;
  const char *name;
};

// TODO: support 32bit environment
static struct llrb_extern_func llrb_extern_funcs[] = {
  { 64, 0, { 0  }, false, "rb_hash_new" },
  { 64, 1, { 64 }, false, "llrb_insn_putspecialobject" },
  { 64, 1, { 64 }, false, "llrb_self_from_cfp" },
  { 64, 1, { 64 }, false, "rb_ary_clear" },
  { 64, 1, { 64 }, false, "rb_ary_resurrect" },
  { 64, 1, { 64 }, false, "rb_gvar_get" },
  { 64, 1, { 64 }, false, "rb_obj_as_string" },
  { 64, 1, { 64 }, false, "rb_str_freeze" },
  { 64, 1, { 64 }, false, "rb_str_resurrect" },
  { 64, 1, { 64 }, true,  "llrb_insn_concatstrings" },
  { 64, 1, { 64 }, true,  "rb_ary_new_from_args" },
  { 0,  2, { 64, 64 }, false, "llrb_insn_setspecial" },
  { 0,  2, { 64, 64 }, false, "llrb_push_result" },
  { 64, 2, { 64, 32 }, false, "rb_reg_new_ary" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_concatarray" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getclassvariable" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getlocal_level0" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getlocal_level1" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_getspecial" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_lt" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_minus" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_opt_plus" },
  { 64, 2, { 64, 64 }, false, "llrb_insn_splatarray" },
  { 64, 2, { 64, 64 }, false, "rb_gvar_set" },
  { 64, 2, { 64, 64 }, false, "rb_ivar_get" },
  { 64, 2, { 64, 64 }, true,  "llrb_insn_toregexp" },
  { 64, 2, { 64, 64 }, true,  "rb_funcall" },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setclassvariable" },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setlocal_level0" },
  { 0,  3, { 64, 64, 64 }, false, "llrb_insn_setlocal_level1" },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_checkkeyword" },
  { 64, 3, { 64, 64, 64 }, false, "llrb_insn_checkmatch" },
  { 64, 3, { 64, 64, 64 }, false, "rb_hash_aset" },
  { 64, 3, { 64, 64, 64 }, false, "rb_ivar_set" },
  { 64, 3, { 64, 64, 64 }, false, "rb_range_new" },
  { 0,  4, { 64, 64, 32, 64 }, false, "llrb_insn_trace" },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_setconstant" },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_throw" },
  { 64, 4, { 64, 64, 64, 32 }, false, "vm_get_ev_const" },
  { 64, 4, { 64, 64, 64, 32 }, true,  "llrb_insn_invokeblock" },
  { 64, 4, { 64, 64, 64, 64 }, false, "llrb_insn_defined" },
  { 64, 5, { 64, 64, 64, 64, 32 }, true, "llrb_insn_opt_send_without_block" },
  { 64, 6, { 64, 64, 64, 64, 64, 32 }, true, "llrb_insn_invokesuper" },
  { 64, 6, { 64, 64, 64, 64, 64, 32 }, true, "llrb_insn_send" },
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

static LLVMValueRef
llrb_get_function(LLVMModuleRef mod, const char *name)
{
  LLVMValueRef func = LLVMGetNamedFunction(mod, name);
  if (func) return func;

  for (size_t i = 0; i < llrb_extern_func_num; i++) {
    if (strcmp(name, llrb_extern_funcs[i].name)) continue;

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
