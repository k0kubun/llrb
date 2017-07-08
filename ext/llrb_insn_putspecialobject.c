#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop

VALUE vm_get_cbase(const VALUE *ep);
VALUE vm_get_const_base(const VALUE *ep);
VALUE
llrb_insn_putspecialobject(rb_num_t value_type) {
  enum vm_special_object_type type = (enum vm_special_object_type)value_type;

  switch (type) {
    case VM_SPECIAL_OBJECT_VMCORE:
      return rb_mRubyVMFrozenCore;
    case VM_SPECIAL_OBJECT_CBASE: {
      rb_thread_t *th = GET_THREAD();
      return vm_get_cbase(th->cfp->ep);
    }
    case VM_SPECIAL_OBJECT_CONST_BASE: {
      rb_thread_t *th = GET_THREAD();
      return vm_get_const_base(th->cfp->ep);
    }
    default:
      rb_bug("putspecialobject insn: unknown value_type");
  }
}
