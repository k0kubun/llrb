#include "cruby.h"

VALUE
llrb_insn_getlocal(VALUE cfp_v, lindex_t idx, rb_num_t level)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  int i, lev = (int)level;
  const VALUE *ep = cfp->ep;

  for (i = 0; i < lev; i++) {
    ep = GET_PREV_EP(ep);
  }
  return *(ep - idx);
}
