#include "cruby.h"

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

#define CALL_METHOD(calling, ci, cc) (*(cc)->call)(th, cfp, (calling), (ci), (cc))
void vm_search_method(const struct rb_call_info *ci, struct rb_call_cache *cc, VALUE recv);
VALUE vm_exec(rb_thread_t *th);
VALUE // TODO: refactor with invokesuper
llrb_insn_opt_send_without_block(VALUE th_value, VALUE cfp_value, VALUE ci_value, VALUE cc_value, unsigned int stack_size, ...)
{
  rb_thread_t *th = (rb_thread_t *)th_value;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_value;
  CALL_INFO ci = (CALL_INFO)ci_value;
  CALL_CACHE cc = (CALL_CACHE)cc_value;

  VALUE recv = Qnil;
  va_list ar;
  va_start(ar, stack_size);
  for (unsigned int i = 0; i < stack_size; i++) {
    VALUE val = va_arg(ar, VALUE);
    if (i == 0) recv = val;
    _llrb_push_result(cfp, val);
  }
  va_end(ar);

  vm_search_method(ci, cc, recv);

  struct rb_calling_info calling;
  calling.block_handler = VM_BLOCK_HANDLER_NONE;
  calling.argc = ci->orig_argc;
  calling.recv = recv;

  VALUE result = CALL_METHOD(&calling, ci, cc);
  if (result == Qundef) {
    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
    return vm_exec(th);
  }
  return result;
}
