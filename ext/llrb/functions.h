static LLVMValueRef
llrb_get_function(LLVMModuleRef mod, const char *name)
{
  LLVMValueRef func = LLVMGetNamedFunction(mod, name);
  if (func) return func;

  if (!strcmp(name, "rb_hash_new")) {
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), 0, 0, false));
  } else if (!strcmp(name, "rb_ary_resurrect")
      || !strcmp(name, "rb_str_resurrect")
      || !strcmp(name, "rb_obj_as_string")
      || !strcmp(name, "rb_str_freeze")
      || !strcmp(name, "rb_gvar_get")
      || !strcmp(name, "rb_ary_clear")
      || !strcmp(name, "llrb_self_from_cfp")
      || !strcmp(name, "llrb_insn_putspecialobject")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));
  } else if (!strcmp(name, "rb_ary_new_from_args")
      || !strcmp(name, "llrb_insn_concatstrings")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, true));
  } else if (!strcmp(name, "llrb_insn_setspecial")
      || !strcmp(name, "llrb_push_result")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMVoidType(), arg_types, 2, false));
  } else if (!strcmp(name, "rb_ivar_get")
      || !strcmp(name, "rb_gvar_set")
      || !strcmp(name, "rb_reg_new_ary")
      || !strcmp(name, "llrb_insn_getlocal_level0")
      || !strcmp(name, "llrb_insn_splatarray")
      || !strcmp(name, "llrb_insn_opt_plus")
      || !strcmp(name, "llrb_insn_opt_minus")
      || !strcmp(name, "llrb_insn_opt_lt")
      || !strcmp(name, "llrb_insn_getclassvariable")
      || !strcmp(name, "llrb_insn_getspecial")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "rb_funcall")
      || !strcmp(name, "llrb_insn_toregexp")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, true));
  } else if (!strcmp(name, "rb_hash_aset")
      || !strcmp(name, "rb_range_new")
      || !strcmp(name, "rb_ivar_set")
      || !strcmp(name, "llrb_insn_checkmatch")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "llrb_insn_setlocal_level0")
      || !strcmp(name, "llrb_insn_setclassvariable")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMVoidType(), arg_types, 3, false));
  } else if (!strcmp(name, "llrb_insn_setconstant")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMVoidType(), arg_types, 4, false));
  } else if (!strcmp(name, "vm_get_ev_const")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 4, false));
  } else if (!strcmp(name, "llrb_insn_defined")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 4, false));
  } else {
    rb_raise(rb_eCompileError, "'%s' is not defined in llrb_get_function", name);
  }
}
