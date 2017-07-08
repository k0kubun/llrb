#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

#define CALL_METHOD(calling, ci, cc) (*(cc)->call)(th, cfp, (calling), (ci), (cc))
VALUE vm_exec(rb_thread_t *th);

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

void vm_caller_setup_arg_block(const rb_thread_t *th, rb_control_frame_t *reg_cfp,
    struct rb_calling_info *calling, const struct rb_call_info *ci, rb_iseq_t *blockiseq, const int is_super);
void vm_search_super_method(rb_thread_t *th, rb_control_frame_t *reg_cfp,
    struct rb_calling_info *calling, struct rb_call_info *ci, struct rb_call_cache *cc);
VALUE
llrb_insn_invokesuper(VALUE th_v, VALUE cfp_v, VALUE ci_v, VALUE cc_v, VALUE blockiseq_v, unsigned int stack_size, ...)
{
  rb_thread_t *th = (rb_thread_t *)th_v;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  CALL_INFO ci = (CALL_INFO)ci_v;
  CALL_CACHE cc = (CALL_CACHE)cc_v;
  ISEQ blockiseq = (ISEQ)blockiseq_v;

  va_list ar;
  va_start(ar, stack_size);
  for (unsigned int i = 0; i < stack_size; i++) {
    _llrb_push_result(cfp, va_arg(ar, VALUE));
  }
  va_end(ar);

  struct rb_calling_info calling;
  calling.argc = ci->orig_argc;

  vm_caller_setup_arg_block(th, cfp, &calling, ci, blockiseq, 1);
  calling.recv = th->cfp->self;
  vm_search_super_method(th, th->cfp, &calling, ci, cc);

  VALUE result = CALL_METHOD(&calling, ci, cc);
  if (result == Qundef) {
    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
    return vm_exec(th);
  }
  return result;
}
