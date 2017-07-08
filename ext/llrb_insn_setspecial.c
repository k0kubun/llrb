#include "cruby.h"

void lep_svar_set(rb_thread_t *th, const VALUE *lep, rb_num_t key, VALUE val);
void
llrb_insn_setspecial(rb_num_t key, VALUE obj)
{
  rb_thread_t *th = GET_THREAD();

  const VALUE *ep = th->cfp->ep;
  while (!VM_ENV_LOCAL_P(ep)) {
    ep = VM_ENV_PREV_EP(ep);
  }

  lep_svar_set(th, ep, key, obj);
}
