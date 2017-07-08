#include "cruby.h"

VALUE
llrb_insn_opt_ge(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
    BASIC_OP_UNREDEFINED_P(BOP_GE, INTEGER_REDEFINED_OP_FLAG)) {
    SIGNED_VALUE a = recv, b = obj;

    if (a >= b) {
      return Qtrue;
    }
    else {
      return Qfalse;
    }
  }
  else if (FLONUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_GE, FLOAT_REDEFINED_OP_FLAG)) {
    /* flonum is not NaN */
    return RFLOAT_VALUE(recv) >= RFLOAT_VALUE(obj) ? Qtrue : Qfalse;
  }
  else {
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, rb_intern(">="), 1, obj);
  }
}
