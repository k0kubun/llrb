#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

rb_cref_t * rb_vm_get_cref(const VALUE *ep);
static inline void
vm_ensure_not_refinement_module(VALUE self)
{
  if (RB_TYPE_P(self, T_MODULE) && FL_TEST(self, RMODULE_IS_REFINEMENT)) {
    rb_warn("not defined at the refinement, but at the outer class/module");
  }
}

VALUE vm_get_cvar_base(const rb_cref_t *cref, rb_control_frame_t *cfp);
void
llrb_insn_setclassvariable(VALUE cfp_v, ID id, VALUE val)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;
  vm_ensure_not_refinement_module(cfp->self);
  rb_cvar_set(vm_get_cvar_base(rb_vm_get_cref(cfp->ep), cfp), id, val);
}
