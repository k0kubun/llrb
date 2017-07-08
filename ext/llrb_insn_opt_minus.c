#include "cruby.h"

VALUE
llrb_insn_opt_minus(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    long a, b, c;

    a = FIX2LONG(recv);
    b = FIX2LONG(obj);
    c = a - b;
    return LONG2NUM(c);
  }
  //else if (FLONUM_2_P(recv, obj) &&
  //    BASIC_OP_UNREDEFINED_P(BOP_MINUS, FLOAT_REDEFINED_OP_FLAG)) {
  //  return DBL2NUM(RFLOAT_VALUE(recv) - RFLOAT_VALUE(obj));
  //}
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat && RBASIC_CLASS(obj) == rb_cFloat  &&
  //      BASIC_OP_UNREDEFINED_P(BOP_MINUS, FLOAT_REDEFINED_OP_FLAG)) {
  //    val = DBL2NUM(RFLOAT_VALUE(recv) - RFLOAT_VALUE(obj));
  //  }
  //  else {
  //    goto INSN_LABEL(normal_dispatch);
  //  }
  //}
  else {
    /* other */
    //INSN_LABEL(normal_dispatch):
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, '-', 1, obj);
  }
}
