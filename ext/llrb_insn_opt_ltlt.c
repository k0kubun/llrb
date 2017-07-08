#include "cruby.h"

VALUE
llrb_insn_opt_ltlt(VALUE recv, VALUE obj)
{
  if (!SPECIAL_CONST_P(recv)) {
    if (RBASIC_CLASS(recv) == rb_cString &&
      BASIC_OP_UNREDEFINED_P(BOP_LTLT, STRING_REDEFINED_OP_FLAG)) {
      return rb_str_concat(recv, obj);
    }
    else if (RBASIC_CLASS(recv) == rb_cArray &&
        BASIC_OP_UNREDEFINED_P(BOP_LTLT, ARRAY_REDEFINED_OP_FLAG)) {
      return rb_ary_push(recv, obj);
    }
    else {
      //goto INSN_LABEL(normal_dispatch);
      return rb_funcall(recv, rb_intern("<<"), 1, obj);
    }
  }
  else {
    //INSN_LABEL(normal_dispatch):
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, rb_intern("<<"), 1, obj);
  }
}
