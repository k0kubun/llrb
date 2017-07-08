#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE vm_throw(rb_thread_t *th, rb_control_frame_t *reg_cfp, rb_num_t throw_state, VALUE throwobj);
void
llrb_insn_throw(VALUE th_value, VALUE cfp_value, rb_num_t throw_state, VALUE throwobj)
{
  rb_thread_t *th = (rb_thread_t *)th_value;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_value;
  RUBY_VM_CHECK_INTS(th);
  th->errinfo = vm_throw(th, cfp, throw_state, throwobj);
}
