// Functions in this file will be called by compiled native code.

#include "stdarg.h"
#include "ruby24/vm_core.h"

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

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L359-L372
VALUE rb_str_concat_literals(size_t, const VALUE*);
VALUE llrb_insn_concatstrings(size_t num, ...) {
  VALUE *args = 0;
  va_list ar;

  if (num > 0) {
    args = ALLOCA_N(VALUE, num);
    va_start(ar, num);
    for (size_t i = 0; i < num; i++) {
      args[i] = va_arg(ar, VALUE);
    }
    va_end(ar);
  }

  return rb_str_concat_literals(num, args);
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

VALUE llrb_insn_opt_plus(VALUE lhs, VALUE rhs) {
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_PLUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) + FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '+', 1, rhs);
}

VALUE llrb_insn_opt_minus(VALUE lhs, VALUE rhs) {
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) - FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '-', 1, rhs);
}

VALUE llrb_insn_opt_lt(VALUE lhs, VALUE rhs) {
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    SIGNED_VALUE a = lhs, b = rhs;

    if (a < b) {
      return Qtrue;
    } else {
      return Qfalse;
    }
  }
  return rb_funcall(lhs, '<', 1, rhs);
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2157-L2169
VALUE llrb_insn_bitblt() {
  return rb_str_new2("a bit of bacon, lettuce and tomato");
}
