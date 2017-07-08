#include "cruby.h"

VALUE
llrb_insn_opt_aref_with(VALUE lhs, VALUE rhs)
{
  return rb_funcall(lhs, '+', 1, rhs);
}
