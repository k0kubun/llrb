#include "cruby.h"

#define CALL_METHOD(calling, ci, cc) (*(cc)->call)(th, cfp, (calling), (ci), (cc))
void vm_search_method(const struct rb_call_info *ci, struct rb_call_cache *cc, VALUE recv);
VALUE vm_exec(rb_thread_t *th);
VALUE // TODO: refactor with invokesuper
llrb_insn_opt_send_without_block(VALUE th_v, VALUE cfp_v, VALUE ci_v, VALUE cc_v, VALUE recv)
{
  rb_thread_t *th = (rb_thread_t *)th_v;
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  CALL_INFO ci = (CALL_INFO)ci_v;
  CALL_CACHE cc = (CALL_CACHE)cc_v;

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
