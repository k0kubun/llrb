#include "cruby.h"

// TODO: Use vm_check_keyword after Ruby 2.5
VALUE
llrb_insn_checkkeyword(VALUE cfp_v, lindex_t kw_bits_index, rb_num_t keyword_index)
{
  rb_control_frame_t *cfp = (rb_control_frame_t *)cfp_v;

  const VALUE *ep = cfp->ep;
  const VALUE kw_bits = *(ep - kw_bits_index);

  if (FIXNUM_P(kw_bits)) {
    int bits = FIX2INT(kw_bits);
    return (bits & (0x01 << keyword_index)) ? Qfalse : Qtrue;
  }
  else {
    VM_ASSERT(RB_TYPE_P(kw_bits, T_HASH));
    return rb_hash_has_key(kw_bits, INT2FIX(keyword_index)) ? Qfalse : Qtrue;
  }
}
