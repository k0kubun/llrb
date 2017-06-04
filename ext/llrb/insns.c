// Functions in this file will be called by compiled native code.

#include "llrb.h"

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L515-L534
VALUE llrb_insn_splatarray(VALUE ary, VALUE flag) {
  VALUE tmp = rb_check_convert_type(ary, T_ARRAY, "Array", "to_a");
  if (NIL_P(tmp)) {
    tmp = rb_ary_new3(1, ary);
  } else if (RTEST(flag)) {
    tmp = rb_ary_dup(tmp);
  }
  return tmp;
}

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2157-L2169
VALUE llrb_insn_bitblt() {
  return rb_str_new2("a bit of bacon, lettuce and tomato");
}
