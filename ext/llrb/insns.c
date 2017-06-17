#include "cruby.h"

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L180-L199
VALUE
llrb_insn_getconstant(VALUE orig_klass, ID id) {
  // TODO: Handle Qnil and Qfalse properly
  if (orig_klass == Qnil || orig_klass == Qfalse) {
    orig_klass = rb_cObject; // force top level
  }
  return rb_const_get(orig_klass, id);
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L201-L223
void
llrb_insn_setconstant(VALUE cbase, ID id, VALUE val) {
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

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L359-L372
VALUE rb_str_concat_literals(size_t, const VALUE*);
VALUE
llrb_insn_concatstrings(size_t num, ...) {
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
VALUE
llrb_insn_splatarray(VALUE ary, VALUE flag) {
  VALUE tmp = rb_check_convert_type(ary, T_ARRAY, "Array", "to_a");
  if (NIL_P(tmp)) {
    tmp = rb_ary_new3(1, ary);
  } else if (RTEST(flag)) {
    tmp = rb_ary_dup(tmp);
  }
  return tmp;
}

VALUE vm_defined(rb_thread_t *th, rb_control_frame_t *reg_cfp, rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v);
VALUE
llrb_insn_defined(rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v)
{
  rb_thread_t *th = GET_THREAD();
  return vm_defined(th, th->cfp, op_type, obj, needstr, v);
}
