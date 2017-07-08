#include "cruby.h"

VALUE
llrb_insn_opt_aref(VALUE recv, VALUE obj)
{
  if (!SPECIAL_CONST_P(recv)) {
    if (RBASIC_CLASS(recv) == rb_cArray && BASIC_OP_UNREDEFINED_P(BOP_AREF, ARRAY_REDEFINED_OP_FLAG) && FIXNUM_P(obj)) {
      return rb_ary_entry(recv, FIX2LONG(obj));
    }
    else if (RBASIC_CLASS(recv) == rb_cHash && BASIC_OP_UNREDEFINED_P(BOP_AREF, HASH_REDEFINED_OP_FLAG)) {
      return rb_hash_aref(recv, obj);
    }
    else {
      //goto INSN_LABEL(normal_dispatch);
      return rb_funcall(recv, rb_intern("[]"), 1, obj);
    }
  }
  else {
    //INSN_LABEL(normal_dispatch):
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, rb_intern("[]"), 1, obj);
  }
}
