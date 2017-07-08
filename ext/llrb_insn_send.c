#include "cruby.h"

#define CALL_METHOD(calling, ci, cc) (*(cc)->call)(th, cfp, (calling), (ci), (cc))
void vm_search_method(const struct rb_call_info *ci, struct rb_call_cache *cc, VALUE recv);
VALUE vm_exec(rb_thread_t *th);
void vm_caller_setup_arg_block(const rb_thread_t *th, rb_control_frame_t *reg_cfp,
    struct rb_calling_info *calling, const struct rb_call_info *ci, rb_iseq_t *blockiseq, const int is_super);

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

VALUE
llrb_insn_send(VALUE th_v, VALUE cfp_v, VALUE ci_v, VALUE cc_v, VALUE blockiseq_v, unsigned int stack_size, ...)
{
  rb_thread_t *th = (rb_thread_t *)th_v;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  CALL_INFO ci = (CALL_INFO)ci_v;
  CALL_CACHE cc = (CALL_CACHE)cc_v;
  ISEQ blockiseq = (ISEQ)blockiseq_v;

  VALUE recv = Qnil;
  va_list ar;
  va_start(ar, stack_size);
  for (unsigned int i = 0; i < stack_size; i++) {
    VALUE val = va_arg(ar, VALUE);
    if (i == 0) recv = val;
    _llrb_push_result(cfp, val);
  }
  va_end(ar);

  struct rb_calling_info calling;

  vm_caller_setup_arg_block(th, cfp, &calling, ci, blockiseq, 0);
  calling.argc = ci->orig_argc;
  calling.recv = recv;
  vm_search_method(ci, cc, recv);

  VALUE result = CALL_METHOD(&calling, ci, cc);
  if (result == Qundef) {
    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
    return vm_exec(th);
  }
  return result;
}
