#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

void
llrb_set_pc(VALUE cfp_v, VALUE pc_v)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  cfp->pc = (VALUE *)pc_v;
}
