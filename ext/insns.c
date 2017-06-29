#include "cruby.h"
#include "cruby/probes_helper.h"

void rb_vm_env_write(const VALUE *ep, int index, VALUE v);
void
llrb_insn_setlocal_level0(rb_control_frame_t *cfp, lindex_t idx, VALUE val)
{
  rb_vm_env_write(cfp->ep, -(int)idx, val);
}

VALUE
llrb_insn_getlocal_level0(rb_control_frame_t *cfp, lindex_t idx)
{
  return *(cfp->ep - idx);
}

void
llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

extern void llrb_insn_trace(rb_thread_t *th, rb_control_frame_t *cfp, rb_event_flag_t flag, VALUE val);

// TODO: Use vm_dtrace after Ruby 2.5
//void
//llrb_insn_trace(rb_thread_t *th, rb_control_frame_t *cfp, rb_event_flag_t flag, VALUE val)
//{
//  if (RUBY_DTRACE_METHOD_ENTRY_ENABLED() ||
//      RUBY_DTRACE_METHOD_RETURN_ENABLED() ||
//      RUBY_DTRACE_CMETHOD_ENTRY_ENABLED() ||
//      RUBY_DTRACE_CMETHOD_RETURN_ENABLED()) {
//
//    switch (flag) {
//      case RUBY_EVENT_CALL:
//        RUBY_DTRACE_METHOD_ENTRY_HOOK(th, 0, 0);
//        break;
//      case RUBY_EVENT_C_CALL:
//        RUBY_DTRACE_CMETHOD_ENTRY_HOOK(th, 0, 0);
//        break;
//      case RUBY_EVENT_RETURN:
//        RUBY_DTRACE_METHOD_RETURN_HOOK(th, 0, 0);
//        break;
//      case RUBY_EVENT_C_RETURN:
//        RUBY_DTRACE_CMETHOD_RETURN_HOOK(th, 0, 0);
//        break;
//    }
//  }
//
//  // TODO: Confirm it works, especially for :line event. We may need to update program counter.
//  EXEC_EVENT_HOOK(th, flag, cfp->self, 0, 0, 0 /* id and klass are resolved at callee */, val);
//}

VALUE
llrb_insn_opt_plus(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_PLUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) + FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '+', 1, rhs);
}

VALUE
llrb_insn_opt_lt(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
    SIGNED_VALUE a = lhs, b = rhs;

    if (a < b) {
      return Qtrue;
    } else {
      return Qfalse;
    }
  }
  return rb_funcall(lhs, '<', 1, rhs);
}

//rb_control_frame_t *
//llrb_exec(rb_thread_t *th, rb_control_frame_t *cfp)
//{
//  llrb_insn_trace(th, cfp, 8, (VALUE)52);
//  llrb_insn_trace(th, cfp, 1, (VALUE)52);
//  llrb_insn_setlocal_level0(cfp, 3, (VALUE)1);
//  llrb_insn_trace(th, cfp, 1, (VALUE)52);
//
//  while (RTEST(llrb_insn_opt_lt(llrb_insn_getlocal_level0(cfp, 3), (VALUE)12000001))) {
//    llrb_insn_trace(th, cfp, 1, (VALUE)52);
//    llrb_insn_setlocal_level0(cfp, (lindex_t)3,
//        llrb_insn_opt_plus(
//          llrb_insn_getlocal_level0(cfp, (VALUE)3),
//          (VALUE)3));
//  }
//
//  llrb_insn_trace(th, cfp, 16, (VALUE)8);
//  llrb_push_result(cfp, (VALUE)8);
//  return cfp;
//}
