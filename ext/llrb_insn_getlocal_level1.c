#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE
llrb_insn_getlocal_level1(VALUE cfp_v, lindex_t idx)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  return *(GET_PREV_EP(cfp->ep) - idx);
}