#include "cruby.h"

// NOTE: There's optimization chance to check flag beforehand.
// TODO: Use `vm_check_match` after Ruby 2.5.
VALUE check_match(VALUE pattern, VALUE target, enum vm_check_match_type type);
VALUE
llrb_insn_checkmatch(VALUE target, VALUE pattern, rb_num_t flag)
{
  enum vm_check_match_type checkmatch_type =
    (enum vm_check_match_type)(flag & VM_CHECKMATCH_TYPE_MASK);
  VALUE result = Qfalse;

  if (flag & VM_CHECKMATCH_ARRAY) {
    long i;
    for (i = 0; i < RARRAY_LEN(pattern); i++) {
      if (RTEST(check_match(RARRAY_AREF(pattern, i), target, checkmatch_type))) {
        result = Qtrue;
        break;
      }
    }
  }
  else {
    if (RTEST(check_match(pattern, target, checkmatch_type))) {
      result = Qtrue;
    }
  }
  return result;
}
