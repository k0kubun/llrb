#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

// TODO: Use vm_concat_array after Ruby 2.5
VALUE
llrb_insn_concatarray(VALUE ary1, VALUE ary2st)
{
  const VALUE ary2 = ary2st;
  VALUE tmp1 = rb_check_convert_type(ary1, T_ARRAY, "Array", "to_a");
  VALUE tmp2 = rb_check_convert_type(ary2, T_ARRAY, "Array", "to_a");

  if (NIL_P(tmp1)) {
    tmp1 = rb_ary_new3(1, ary1);
  }

  if (NIL_P(tmp2)) {
    tmp2 = rb_ary_new3(1, ary2);
  }

  if (tmp1 == ary1) {
    tmp1 = rb_ary_dup(ary1);
  }
  return rb_ary_concat(tmp1, tmp2);
}
