#include "cruby.h"

VALUE
llrb_insn_opt_lt(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_LT, INTEGER_REDEFINED_OP_FLAG)) {
    SIGNED_VALUE a = recv, b = obj;

    if (a < b) {
      return Qtrue;
    }
    else {
      return Qfalse;
    }
  }
  else if (FLONUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_LT, FLOAT_REDEFINED_OP_FLAG)) {
    /* flonum is not NaN */
    return RFLOAT_VALUE(recv) < RFLOAT_VALUE(obj) ? Qtrue : Qfalse;
  }
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat && RBASIC_CLASS(obj) == rb_cFloat  &&
  //      BASIC_OP_UNREDEFINED_P(BOP_LT, FLOAT_REDEFINED_OP_FLAG)) {
  //    val = double_cmp_lt(RFLOAT_VALUE(recv), RFLOAT_VALUE(obj));
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
    return rb_funcall(recv, '<', 1, obj);
  }
}
