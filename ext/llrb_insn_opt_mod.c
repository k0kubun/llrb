#include "cruby.h"

VALUE
llrb_insn_opt_mod(VALUE recv, VALUE obj)
{
  if (FIXNUM_2_P(recv, obj) &&
    BASIC_OP_UNREDEFINED_P(BOP_MOD, INTEGER_REDEFINED_OP_FLAG )) {
    //if (FIX2LONG(obj) == 0) goto INSN_LABEL(normal_dispatch);
    if (FIX2LONG(obj) == 0) return rb_funcall(recv, '%', 1, obj);
    return rb_fix_mod_fix(recv, obj);
  }
  else if (FLONUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_MOD, FLOAT_REDEFINED_OP_FLAG)) {
    return DBL2NUM(ruby_float_mod(RFLOAT_VALUE(recv), RFLOAT_VALUE(obj)));
  }
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat && RBASIC_CLASS(obj) == rb_cFloat &&
  //    BASIC_OP_UNREDEFINED_P(BOP_MOD, FLOAT_REDEFINED_OP_FLAG)) {
  //    val = DBL2NUM(ruby_float_mod(RFLOAT_VALUE(recv), RFLOAT_VALUE(obj)));
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
  }
  return rb_funcall(recv, '%', 1, obj);
}
