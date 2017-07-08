#include "cruby.h"

VALUE
llrb_insn_getlocal_level0(VALUE cfp_v, lindex_t idx)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  return *(cfp->ep - idx);
}
