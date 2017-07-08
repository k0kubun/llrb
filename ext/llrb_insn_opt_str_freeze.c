#include "cruby.h"

VALUE
llrb_insn_opt_str_freeze(VALUE str)
{
  if (BASIC_OP_UNREDEFINED_P(BOP_FREEZE, STRING_REDEFINED_OP_FLAG)) {
    return str;
  }
  else {
    return rb_funcall(rb_str_resurrect(str), idFreeze, 0);
  }
}
