#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

static inline VALUE
_llrb_self_from_cfp(rb_control_frame_t *cfp)
{
  return cfp->self;
}
VALUE
llrb_self_from_cfp(VALUE cfp)
{
  return _llrb_self_from_cfp((rb_control_frame_t *)cfp);
}
