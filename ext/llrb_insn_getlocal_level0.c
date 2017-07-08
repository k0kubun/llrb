#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE
llrb_insn_getlocal_level0(VALUE cfp_v, lindex_t idx)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  return *(cfp->ep - idx);
}
