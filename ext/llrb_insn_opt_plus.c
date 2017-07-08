#include "cruby.h"

VALUE
llrb_insn_opt_plus(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_PLUS,INTEGER_REDEFINED_OP_FLAG)) {
    /* fixnum + fixnum */
#ifndef LONG_LONG_VALUE
    VALUE msb = (VALUE)1 << ((sizeof(VALUE) * CHAR_BIT) - 1);
    VALUE val = recv - 1 + obj;
    if ((~(recv ^ obj) & (recv ^ val)) & msb) {
      return rb_int2big((SIGNED_VALUE)((val>>1) | (recv & msb)));
    }
    return val;
#else
    return LONG2NUM(FIX2LONG(recv) + FIX2LONG(obj));
#endif
  }
  //else if (FLONUM_2_P(recv, obj) &&
  //    BASIC_OP_UNREDEFINED_P(BOP_PLUS, FLOAT_REDEFINED_OP_FLAG)) {
  //  return DBL2NUM(RFLOAT_VALUE(recv) + RFLOAT_VALUE(obj));
  //}
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat && RBASIC_CLASS(obj) == rb_cFloat &&
  //      BASIC_OP_UNREDEFINED_P(BOP_PLUS, FLOAT_REDEFINED_OP_FLAG)) {
  //    val = DBL2NUM(RFLOAT_VALUE(recv) + RFLOAT_VALUE(obj));
  //  }
  //  else if (RBASIC_CLASS(recv) == rb_cString && RBASIC_CLASS(obj) == rb_cString &&
  //      BASIC_OP_UNREDEFINED_P(BOP_PLUS, STRING_REDEFINED_OP_FLAG)) {
  //    val = rb_str_plus(recv, obj);
  //  }
  //  else if (RBASIC_CLASS(recv) == rb_cArray &&
  //    BASIC_OP_UNREDEFINED_P(BOP_PLUS, ARRAY_REDEFINED_OP_FLAG)) {
  //    val = rb_ary_plus(recv, obj);
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
    return rb_funcall(recv, '+', 1, obj);
  }
}
