#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE
llrb_insn_opt_lt(VALUE lhs, VALUE rhs)
{
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
