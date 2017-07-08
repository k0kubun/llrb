#include <stdarg.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L359-L372
extern VALUE rb_str_concat_literals(size_t, const VALUE*);
VALUE
llrb_insn_concatstrings(size_t num, ...) {
  VALUE *args = 0;
  va_list ar;

  if (num > 0) {
    args = ALLOCA_N(VALUE, num);
    va_start(ar, num);
    for (size_t i = 0; i < num; i++) {
      args[i] = va_arg(ar, VALUE);
    }
    va_end(ar);
  }

  return rb_str_concat_literals(num, args);
}
