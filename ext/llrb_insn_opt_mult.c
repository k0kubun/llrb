#include "cruby.h"

VALUE
llrb_insn_opt_mult(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_MULT, INTEGER_REDEFINED_OP_FLAG)) {
    return rb_fix_mul_fix(recv, obj);
  }
  //else if (FLONUM_2_P(recv, obj) &&
  //    BASIC_OP_UNREDEFINED_P(BOP_MULT, FLOAT_REDEFINED_OP_FLAG)) {
  //  return DBL2NUM(RFLOAT_VALUE(recv) * RFLOAT_VALUE(obj));
  //}
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat && RBASIC_CLASS(obj) == rb_cFloat  &&
  //      BASIC_OP_UNREDEFINED_P(BOP_MULT, FLOAT_REDEFINED_OP_FLAG)) {
  //    val = DBL2NUM(RFLOAT_VALUE(recv) * RFLOAT_VALUE(obj));
  //  }
  //  else {
  //    goto INSN_LABEL(normal_dispatch);
  //  }
  //}
  else {
    //INSN_LABEL(normal_dispatch):
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, '*', 1, obj);
  }
}
