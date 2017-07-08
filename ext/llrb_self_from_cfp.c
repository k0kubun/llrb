#include "cruby.h"

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
