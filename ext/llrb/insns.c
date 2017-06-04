// Functions in this file will be called by compiled native code.

#include "llrb.h"

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L180-L199
VALUE llrb_insn_getconstant(VALUE orig_klass, ID id) {
  // TODO: Handle Qnil and Qfalse properly
  if (orig_klass == Qnil || orig_klass == Qfalse) {
    orig_klass = rb_cObject; // force top level
  }
  return rb_const_get(orig_klass, id);
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L201-L223
VALUE llrb_insn_setconstant(VALUE cbase, ID id, VALUE val) {
  // TODO: Handle Qnil and Qfalse properly
  if (cbase == Qnil || cbase == Qfalse) {
    cbase = rb_cObject; // force top level
  }
  rb_const_set(cbase, id, val);
  return val;
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L515-L534
VALUE llrb_insn_splatarray(VALUE ary, VALUE flag) {
  VALUE tmp = rb_check_convert_type(ary, T_ARRAY, "Array", "to_a");
  if (NIL_P(tmp)) {
    tmp = rb_ary_new3(1, ary);
  } else if (RTEST(flag)) {
    tmp = rb_ary_dup(tmp);
  }
  return tmp;
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2157-L2169
VALUE llrb_insn_bitblt() {
  return rb_str_new2("a bit of bacon, lettuce and tomato");
}
