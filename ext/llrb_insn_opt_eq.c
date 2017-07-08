#include "cruby.h"

static inline VALUE
opt_eq_func(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
    BASIC_OP_UNREDEFINED_P(BOP_EQ, INTEGER_REDEFINED_OP_FLAG)) {
    return (recv == obj) ? Qtrue : Qfalse;
  }
  else if (FLONUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_EQ, FLOAT_REDEFINED_OP_FLAG)) {
    return (recv == obj) ? Qtrue : Qfalse;
  }
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat &&
  //    RBASIC_CLASS(obj) == rb_cFloat &&
  //    BASIC_OP_UNREDEFINED_P(BOP_EQ, FLOAT_REDEFINED_OP_FLAG)) {
  //    double a = RFLOAT_VALUE(recv);
  //    double b = RFLOAT_VALUE(obj);

  //    if (isnan(a) || isnan(b)) {
  //      return Qfalse;
  //    }
  //    return  (a == b) ? Qtrue : Qfalse;
  //  }
  //  else if (RBASIC_CLASS(recv) == rb_cString &&
  //      RBASIC_CLASS(obj) == rb_cString &&
  //      BASIC_OP_UNREDEFINED_P(BOP_EQ, STRING_REDEFINED_OP_FLAG)) {
  //    return rb_str_equal(recv, obj);
  //  }
  //}

  //{
  //  vm_search_method(ci, cc, recv);

  //  if (check_cfunc(cc->me, rb_obj_equal)) {
  //    return recv == obj ? Qtrue : Qfalse;
  //  }
  //}

  return Qundef;
}

VALUE
llrb_insn_opt_eq(VALUE recv, VALUE obj)
{
  VALUE val = opt_eq_func(recv, obj);

  if (val == Qundef) {
    /* other */
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, rb_intern("=="), 1, obj);
  }
  return val;
}
