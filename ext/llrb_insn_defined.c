#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE vm_defined(rb_thread_t *th, rb_control_frame_t *reg_cfp, rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v);
VALUE
llrb_insn_defined(rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v)
{
  rb_thread_t *th = GET_THREAD();
  return vm_defined(th, th->cfp, op_type, obj, needstr, v);
}
