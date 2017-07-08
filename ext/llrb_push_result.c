#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}
void
llrb_push_result(VALUE cfp, VALUE result)
{
  _llrb_push_result((rb_control_frame_t *)cfp, result);
}
