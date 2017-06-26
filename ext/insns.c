#include "cruby.h"

VALUE
llrb_insn_getlocal_level0(rb_control_frame_t *cfp, lindex_t idx)
{
  return *(cfp->ep - idx);
}
