#include "cruby.h"

VALUE
llrb_insn_opt_neq(VALUE lhs, VALUE rhs)
{
  return rb_funcall(lhs, rb_intern("!="), 1, rhs);
}
