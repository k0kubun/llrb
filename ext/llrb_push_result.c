#include "cruby.h"

void
llrb_push_result(VALUE cfp_v, VALUE result)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;

  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}
