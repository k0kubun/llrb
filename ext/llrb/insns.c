#include "cruby.h"

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L180-L199
VALUE llrb_insn_getconstant(VALUE orig_klass, ID id) {
  // TODO: Handle Qnil and Qfalse properly
  if (orig_klass == Qnil || orig_klass == Qfalse) {
    orig_klass = rb_cObject; // force top level
  }
  return rb_const_get(orig_klass, id);
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L201-L223
void llrb_insn_setconstant(VALUE cbase, ID id, VALUE val) {
  // TODO: Handle Qnil and Qfalse properly
  if (cbase == Qnil || cbase == Qfalse) {
    cbase = rb_cObject; // force top level
  }
  rb_const_set(cbase, id, val);
}

// NOTE: There's optimization chance to check flag beforehand.
// TODO: Use `vm_check_match` after Ruby 2.5.
VALUE check_match(VALUE pattern, VALUE target, enum vm_check_match_type type);
VALUE
llrb_insn_checkmatch(VALUE target, VALUE pattern, rb_num_t flag)
{
  enum vm_check_match_type checkmatch_type =
    (enum vm_check_match_type)(flag & VM_CHECKMATCH_TYPE_MASK);
  VALUE result = Qfalse;

  if (flag & VM_CHECKMATCH_ARRAY) {
    long i;
    for (i = 0; i < RARRAY_LEN(pattern); i++) {
      if (RTEST(check_match(RARRAY_AREF(pattern, i), target, checkmatch_type))) {
        result = Qtrue;
        break;
      }
    }
  }
  else {
    if (RTEST(check_match(pattern, target, checkmatch_type))) {
      result = Qtrue;
    }
  }
  return result;
}
