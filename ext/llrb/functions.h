#ifndef LLRB_FUNCTIONS_H
#define LLRB_FUNCTIONS_H

#define LLRB_EXTERN_FUNC_MAX_ARGC 4
struct llrb_extern_func {
  unsigned int return_type; // 0 = void, 32 = 32bit int, 64 = 64bit int
  unsigned int argc;
  unsigned int argv[LLRB_EXTERN_FUNC_MAX_ARGC]; // 0 = void, 32 = 32bit int, 64 = 64bit int
  bool unlimited;
  const char *name;
};

static struct llrb_extern_func llrb_extern_funcs[] = {
  { 64, 0, { 0  },             false, "rb_hash_new" },
  { 64, 1, { 64 },             false, "llrb_insn_putspecialobject" },
  { 64, 1, { 64 },             false, "llrb_self_from_cfp" },
  { 64, 1, { 64 },             false, "rb_ary_clear" },
  { 64, 1, { 64 },             false, "rb_ary_resurrect" },
  { 64, 1, { 64 },             false, "rb_gvar_get" },
  { 64, 1, { 64 },             false, "rb_obj_as_string" },
  { 64, 1, { 64 },             false, "rb_str_freeze" },
  { 64, 1, { 64 },             false, "rb_str_resurrect" },
  { 64, 1, { 64 },             true,  "llrb_insn_concatstrings" },
  { 64, 1, { 64 },             true,  "rb_ary_new_from_args" },
  { 0,  2, { 64, 64 },         false, "llrb_insn_setspecial" },
  { 0,  2, { 64, 64 },         false, "llrb_push_result" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_concatarray" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_getclassvariable" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_getlocal_level0" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_getspecial" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_opt_lt" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_opt_minus" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_opt_plus" },
  { 64, 2, { 64, 64 },         false, "llrb_insn_splatarray" },
  { 64, 2, { 64, 64 },         false, "rb_gvar_set" },
  { 64, 2, { 64, 64 },         false, "rb_ivar_get" },
  { 64, 2, { 64, 64 },         false, "rb_reg_new_ary" },
  { 64, 2, { 64, 64 },         true,  "llrb_insn_toregexp" },
  { 64, 2, { 64, 64 },         true,  "rb_funcall" },
  { 0,  3, { 64, 64, 64 },     false, "llrb_insn_setclassvariable" },
  { 0,  3, { 64, 64, 64 },     false, "llrb_insn_setlocal_level0" },
  { 64, 3, { 64, 64, 64 },     false, "llrb_insn_checkkeyword" },
  { 64, 3, { 64, 64, 64 },     false, "llrb_insn_checkmatch" },
  { 64, 3, { 64, 64, 64 },     false, "rb_hash_aset" },
  { 64, 3, { 64, 64, 64 },     false, "rb_ivar_set" },
  { 64, 3, { 64, 64, 64 },     false, "rb_range_new" },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_setconstant" },
  { 0,  4, { 64, 64, 64, 64 }, false, "llrb_insn_throw" },
  { 64, 4, { 64, 64, 64, 32 }, false, "vm_get_ev_const" },
  { 64, 4, { 64, 64, 64, 64 }, false, "llrb_insn_defined" },
};
static size_t llrb_extern_func_num = sizeof(llrb_extern_funcs) / sizeof(struct llrb_extern_func);

#endif // LLRB_FUNCTIONS_H
