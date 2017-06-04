// Functions in this file will be called by compiled native code.

#include "llrb.h"

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2157-L2169
VALUE llrb_insn_bitblt() {
  return rb_str_new2("a bit of bacon, lettuce and tomato");
}
