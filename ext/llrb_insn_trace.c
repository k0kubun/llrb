#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop
#include "cruby/probes_helper.h"

// TODO: Use vm_dtrace after Ruby 2.5
static inline void
_llrb_insn_trace(rb_thread_t *th, rb_control_frame_t *cfp, rb_event_flag_t flag, VALUE val)
{
  if (RUBY_DTRACE_METHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_METHOD_RETURN_ENABLED() ||
      RUBY_DTRACE_CMETHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_CMETHOD_RETURN_ENABLED()) {

    switch (flag) {
      case RUBY_EVENT_CALL:
        RUBY_DTRACE_METHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_CALL:
        RUBY_DTRACE_CMETHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_RETURN:
        RUBY_DTRACE_METHOD_RETURN_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_RETURN:
        RUBY_DTRACE_CMETHOD_RETURN_HOOK(th, 0, 0);
        break;
    }
  }

  // TODO: Confirm it works!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // Especially for :line event. We may need to update program counter.
  EXEC_EVENT_HOOK(th, flag, cfp->self, 0, 0, 0 /* id and klass are resolved at callee */, val);
}
void
llrb_insn_trace(VALUE th, VALUE cfp, rb_event_flag_t flag, VALUE val)
{
  _llrb_insn_trace((rb_thread_t *)th, (rb_control_frame_t *)cfp, flag, val);
}
