#include "cruby.h"

static inline VALUE
vm_getivar(VALUE obj, ID id, IC ic, struct rb_call_cache *cc, int is_attr)
{
  if (LIKELY(RB_TYPE_P(obj, T_OBJECT))) {
    VALUE val = Qundef;
    if (LIKELY(is_attr ? cc->aux.index > 0 : ic->ic_serial == RCLASS_SERIAL(RBASIC(obj)->klass))) {
      st_index_t index = !is_attr ? ic->ic_value.index : (cc->aux.index - 1);
      if (LIKELY(index < ROBJECT_NUMIV(obj))) {
        val = ROBJECT_IVPTR(obj)[index];
      }
     undef_check:
      if (UNLIKELY(val == Qundef)) {
        if (!is_attr && RTEST(ruby_verbose))
          rb_warning("instance variable %"PRIsVALUE" not initialized", QUOTE_ID(id));
        val = Qnil;
      }
      return val;
    }
    else {
      st_data_t index;
      struct st_table *iv_index_tbl = ROBJECT_IV_INDEX_TBL(obj);

      if (iv_index_tbl) {
        if (st_lookup(iv_index_tbl, id, &index)) {
          if (index < ROBJECT_NUMIV(obj)) {
            val = ROBJECT_IVPTR(obj)[index];
          }
          if (!is_attr) {
            ic->ic_value.index = index;
            ic->ic_serial = RCLASS_SERIAL(RBASIC(obj)->klass);
          }
          else { /* call_info */
            cc->aux.index = (int)index + 1;
          }
        }
      }
      goto undef_check;
    }
  }
  if (is_attr)
    return rb_attr_get(obj, id);
  return rb_ivar_get(obj, id);
}

VALUE
llrb_insn_getinstancevariable(VALUE obj, ID id, VALUE ic_v)
{
  return vm_getivar(obj, id, (IC)ic_v, 0, 0);
}
