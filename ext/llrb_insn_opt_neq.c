#include "cruby.h"

void vm_search_method(const struct rb_call_info *ci, struct rb_call_cache *cc, VALUE recv);

inline VALUE
rb_obj_equal(VALUE obj1, VALUE obj2)
{
    if (obj1 == obj2) return Qtrue;
    return Qfalse;
}

#define id_eq               idEq
inline VALUE
rb_obj_not_equal(VALUE obj1, VALUE obj2)
{
    VALUE result = rb_funcall(obj1, id_eq, 1, obj2);
    return RTEST(result) ? Qfalse : Qtrue;
}

static inline int
check_cfunc(const rb_callable_method_entry_t *me, VALUE (*func)())
{
  if (me && me->def->type == VM_METHOD_TYPE_CFUNC &&
      me->def->body.cfunc.func == func) {
    return 1;
  }
  else {
    return 0;
  }
}

static inline VALUE
opt_eq_func(VALUE recv, VALUE obj, CALL_INFO ci, CALL_CACHE cc)
{
  if (FIXNUM_2_P(recv, obj) &&
    BASIC_OP_UNREDEFINED_P(BOP_EQ, INTEGER_REDEFINED_OP_FLAG)) {
    return (recv == obj) ? Qtrue : Qfalse;
  }
  else if (FLONUM_2_P(recv, obj) &&
      BASIC_OP_UNREDEFINED_P(BOP_EQ, FLOAT_REDEFINED_OP_FLAG)) {
    return (recv == obj) ? Qtrue : Qfalse;
  }
  //else if (!SPECIAL_CONST_P(recv) && !SPECIAL_CONST_P(obj)) {
  //  if (RBASIC_CLASS(recv) == rb_cFloat &&
  //    RBASIC_CLASS(obj) == rb_cFloat &&
  //    BASIC_OP_UNREDEFINED_P(BOP_EQ, FLOAT_REDEFINED_OP_FLAG)) {
  //    double a = RFLOAT_VALUE(recv);
  //    double b = RFLOAT_VALUE(obj);

  //    if (isnan(a) || isnan(b)) {
  //      return Qfalse;
  //    }
  //    return  (a == b) ? Qtrue : Qfalse;
  //  }
  //  else if (RBASIC_CLASS(recv) == rb_cString &&
  //      RBASIC_CLASS(obj) == rb_cString &&
  //      BASIC_OP_UNREDEFINED_P(BOP_EQ, STRING_REDEFINED_OP_FLAG)) {
  //    return rb_str_equal(recv, obj);
  //  }
  //}

  {
    vm_search_method(ci, cc, recv);

    if (check_cfunc(cc->me, rb_obj_equal)) {
      return recv == obj ? Qtrue : Qfalse;
    }
  }

  return Qundef;
}

static inline VALUE
_llrb_insn_opt_neq(VALUE recv, VALUE obj, CALL_INFO ci, CALL_CACHE cc, CALL_INFO ci_eq, CALL_CACHE cc_eq)
{
  //extern VALUE rb_obj_not_equal(VALUE obj1, VALUE obj2);
  vm_search_method(ci, cc, recv);

  VALUE val = Qundef;

  if (check_cfunc(cc->me, rb_obj_not_equal)) {
    val = opt_eq_func(recv, obj, ci_eq, cc_eq);

    if (val != Qundef) {
      val = RTEST(val) ? Qfalse : Qtrue;
    }
  }

  if (val == Qundef) {
    /* other */
    //PUSH(recv);
    //PUSH(obj);
    //CALL_SIMPLE_METHOD(recv);
    return rb_funcall(recv, rb_intern("!="), 1, obj);
  }
  return val;
}

VALUE
llrb_insn_opt_neq(VALUE recv, VALUE obj, VALUE ci, VALUE cc, VALUE ci_eq, VALUE cc_eq)
{
  return _llrb_insn_opt_neq(recv, obj, (CALL_INFO)ci, (CALL_CACHE)cc, (CALL_INFO)ci_eq, (CALL_CACHE)cc_eq);
}
