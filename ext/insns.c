#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

void
llrb_push_result(VALUE cfp, VALUE result)
{
  _llrb_push_result((rb_control_frame_t *)cfp, result);
}

VALUE
llrb_insn_opt_plus(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_PLUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) + FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '+', 1, rhs);
}

VALUE
llrb_exec(VALUE th, VALUE cfp)
{
  llrb_push_result(cfp,
      llrb_insn_opt_plus(
        llrb_insn_opt_plus(
          llrb_insn_opt_plus(
            llrb_insn_opt_plus((VALUE)3, (VALUE)5),
            (VALUE)7),
          (VALUE)9),
        (VALUE)11)
      );
  return cfp;
}
