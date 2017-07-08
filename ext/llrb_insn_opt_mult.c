#include "cruby.h"

VALUE
llrb_insn_opt_mult(VALUE lhs, VALUE rhs)
{
  return rb_funcall(lhs, '+', 1, rhs);
}
