#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE vm_getspecial(rb_thread_t *th, const VALUE *lep, rb_num_t key, rb_num_t type);
VALUE
llrb_insn_getspecial(rb_num_t key, rb_num_t type)
{
  rb_thread_t *th = GET_THREAD();

  const VALUE *ep = th->cfp->ep;
  while (!VM_ENV_LOCAL_P(ep)) {
    ep = VM_ENV_PREV_EP(ep);
  }
  return vm_getspecial(th, ep, key, type);
}
