#include "cruby.h"

static inline VALUE
vm_setivar(VALUE obj, ID id, VALUE val, IC ic, struct rb_call_cache *cc, int is_attr)
{
  rb_check_frozen(obj);

  if (LIKELY(RB_TYPE_P(obj, T_OBJECT))) {
    VALUE klass = RBASIC(obj)->klass;
    st_data_t index;

    if (LIKELY(
      (!is_attr && ic->ic_serial == RCLASS_SERIAL(klass)) ||
      (is_attr && cc->aux.index > 0))) {
      VALUE *ptr = ROBJECT_IVPTR(obj);
      index = !is_attr ? ic->ic_value.index : cc->aux.index-1;

      if (index < ROBJECT_NUMIV(obj)) {
        RB_OBJ_WRITE(obj, &ptr[index], val);
        return val; /* inline cache hit */
      }
    }
    else {
      struct st_table *iv_index_tbl = ROBJECT_IV_INDEX_TBL(obj);

      if (iv_index_tbl && st_lookup(iv_index_tbl, (st_data_t)id, &index)) {
        if (!is_attr) {
          ic->ic_value.index = index;
          ic->ic_serial = RCLASS_SERIAL(klass);
        }
        else if (index >= INT_MAX) {
          rb_raise(rb_eArgError, "too many instance variables");
        }
        else {
          cc->aux.index = (int)(index + 1);
        }
      }
      /* fall through */
    }
  }
  return rb_ivar_set(obj, id, val);
}

void
llrb_insn_setinstancevariable(VALUE obj, ID id, VALUE val, VALUE ic_v)
{
  vm_setivar(obj, id, val, (IC)ic_v, 0, 0);
}
