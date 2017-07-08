#include "cruby.h"

VALUE
llrb_insn_opt_aset(VALUE recv, VALUE obj, VALUE set)
{
  if (!SPECIAL_CONST_P(recv)) {
    if (RBASIC_CLASS(recv) == rb_cArray && BASIC_OP_UNREDEFINED_P(BOP_ASET, ARRAY_REDEFINED_OP_FLAG) && FIXNUM_P(obj)) {
      rb_ary_store(recv, FIX2LONG(obj), set);
      return set;
    }
    else if (RBASIC_CLASS(recv) == rb_cHash && BASIC_OP_UNREDEFINED_P(BOP_ASET, HASH_REDEFINED_OP_FLAG)) {
      rb_hash_aset(recv, obj, set);
      return set;
    }
    else {
      //goto INSN_LABEL(normal_dispatch);
      return rb_funcall(recv, '+', 2, obj, set);
    }
  }
  else {
    //INSN_LABEL(normal_dispatch):
    //PUSH(recv);
    //PUSH(obj);
    //PUSH(set);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, '+', 2, obj, set);
  }
}
