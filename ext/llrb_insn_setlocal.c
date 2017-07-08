#include "cruby.h"

void rb_vm_env_write(const VALUE *ep, int index, VALUE v);

void
llrb_insn_setlocal(VALUE cfp_v, lindex_t idx, rb_num_t level, VALUE val)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  int i, lev = (int)level;
  const VALUE *ep = cfp->ep;

  for (i = 0; i < lev; i++) {
    ep = GET_PREV_EP(ep);
  }
  rb_vm_env_write(ep, -(int)idx, val);
}
