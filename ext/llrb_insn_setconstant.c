#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

static inline void
vm_check_if_namespace(VALUE klass)
{
  if (!RB_TYPE_P(klass, T_CLASS) && !RB_TYPE_P(klass, T_MODULE)) {
    rb_raise(rb_eTypeError, "%+"PRIsVALUE" is not a class/module", klass);
  }
}
static inline void
vm_ensure_not_refinement_module(VALUE self)
{
  if (RB_TYPE_P(self, T_MODULE) && FL_TEST(self, RMODULE_IS_REFINEMENT)) {
    rb_warn("not defined at the refinement, but at the outer class/module");
  }
}
// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L201-L223
void
llrb_insn_setconstant(VALUE self, VALUE cbase, ID id, VALUE val) {
  vm_check_if_namespace(cbase);
  vm_ensure_not_refinement_module(self);
  rb_const_set(cbase, id, val);
}
