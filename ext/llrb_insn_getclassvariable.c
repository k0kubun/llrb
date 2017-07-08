#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE vm_get_cvar_base(const rb_cref_t *cref, rb_control_frame_t *cfp);
rb_cref_t * rb_vm_get_cref(const VALUE *ep);
VALUE
llrb_insn_getclassvariable(VALUE cfp_v, ID id)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  return rb_cvar_get(vm_get_cvar_base(rb_vm_get_cref(cfp->ep), cfp), id);
}
