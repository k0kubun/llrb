#include "cruby.h"

void rb_vm_env_write(const VALUE *ep, int index, VALUE v);
void
llrb_insn_setlocal_level0(VALUE cfp_v, lindex_t idx, VALUE val)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  rb_vm_env_write(cfp->ep, -(int)idx, val);
}
