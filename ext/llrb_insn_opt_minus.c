#include "cruby.h"

VALUE
llrb_insn_opt_minus(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) - FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '-', 1, rhs);
}
