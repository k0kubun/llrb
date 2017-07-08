#include "cruby.h"

VALUE vm_exec(rb_thread_t *th);

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

VALUE vm_invoke_block(rb_thread_t *th, rb_control_frame_t *reg_cfp, struct rb_calling_info *calling, const struct rb_call_info *ci);
VALUE
llrb_insn_invokeblock(VALUE th_v, VALUE cfp_v, VALUE ci_v, unsigned int stack_size, ...)
{
  rb_thread_t *th = (rb_thread_t *)th_v;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  CALL_INFO ci = (CALL_INFO)ci_v;

  va_list ar;
  va_start(ar, stack_size);
  for (unsigned int i = 0; i < stack_size; i++) {
    _llrb_push_result(cfp, va_arg(ar, VALUE));
  }
  va_end(ar);

  struct rb_calling_info calling;
  calling.argc = ci->orig_argc;
  calling.block_handler = VM_BLOCK_HANDLER_NONE;
  calling.recv = cfp->self;

  VALUE val = vm_invoke_block(th, cfp, &calling, ci);
  if (val == Qundef) {
    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
    return vm_exec(th);
  }
  return val;
}
