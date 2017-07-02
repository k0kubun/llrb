#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wunknown-attributes"
#include "cruby.h"
#pragma clang diagnostic pop
#include "cruby/probes_helper.h"

//VALUE
//llrb_insn_getlocal_level0(rb_control_frame_t *cfp, lindex_t idx)
//{
//  return *(cfp->ep - idx);
//}
//
//void rb_vm_env_write(const VALUE *ep, int index, VALUE v);
//void
//llrb_insn_setlocal_level0(rb_control_frame_t *cfp, lindex_t idx, VALUE val)
//{
//  rb_vm_env_write(cfp->ep, -(int)idx, val);
//}
//
//// TODO: This can be optimized on runtime...
//VALUE
//llrb_self_from_cfp(rb_control_frame_t *cfp)
//{
//  return cfp->self;
//}
//
//static inline void
//vm_check_if_namespace(VALUE klass)
//{
//  if (!RB_TYPE_P(klass, T_CLASS) && !RB_TYPE_P(klass, T_MODULE)) {
//    rb_raise(rb_eTypeError, "%+"PRIsVALUE" is not a class/module", klass);
//  }
//}
//
//static inline void
//vm_ensure_not_refinement_module(VALUE self)
//{
//  if (RB_TYPE_P(self, T_MODULE) && FL_TEST(self, RMODULE_IS_REFINEMENT)) {
//    rb_warn("not defined at the refinement, but at the outer class/module");
//  }
//}
//
//// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L201-L223
//void
//llrb_insn_setconstant(VALUE self, VALUE cbase, ID id, VALUE val) {
//  vm_check_if_namespace(cbase);
//  vm_ensure_not_refinement_module(self);
//  rb_const_set(cbase, id, val);
//}
//
//// NOTE: There's optimization chance to check flag beforehand.
//// TODO: Use `vm_check_match` after Ruby 2.5.
//VALUE check_match(VALUE pattern, VALUE target, enum vm_check_match_type type);
//VALUE
//llrb_insn_checkmatch(VALUE target, VALUE pattern, rb_num_t flag)
//{
//  enum vm_check_match_type checkmatch_type =
//    (enum vm_check_match_type)(flag & VM_CHECKMATCH_TYPE_MASK);
//  VALUE result = Qfalse;
//
//  if (flag & VM_CHECKMATCH_ARRAY) {
//    long i;
//    for (i = 0; i < RARRAY_LEN(pattern); i++) {
//      if (RTEST(check_match(RARRAY_AREF(pattern, i), target, checkmatch_type))) {
//        result = Qtrue;
//        break;
//      }
//    }
//  }
//  else {
//    if (RTEST(check_match(pattern, target, checkmatch_type))) {
//      result = Qtrue;
//    }
//  }
//  return result;
//}
//
//VALUE vm_get_cbase(const VALUE *ep);
//VALUE vm_get_const_base(const VALUE *ep);
//VALUE
//llrb_insn_putspecialobject(rb_num_t value_type) {
//  enum vm_special_object_type type = (enum vm_special_object_type)value_type;
//
//  switch (type) {
//    case VM_SPECIAL_OBJECT_VMCORE:
//      return rb_mRubyVMFrozenCore;
//    case VM_SPECIAL_OBJECT_CBASE: {
//      rb_thread_t *th = GET_THREAD();
//      return vm_get_cbase(th->cfp->ep);
//    }
//    case VM_SPECIAL_OBJECT_CONST_BASE: {
//      rb_thread_t *th = GET_THREAD();
//      return vm_get_const_base(th->cfp->ep);
//    }
//    default:
//      rb_bug("putspecialobject insn: unknown value_type");
//  }
//}
//
//// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L359-L372
//VALUE rb_str_concat_literals(size_t, const VALUE*);
//VALUE
//llrb_insn_concatstrings(size_t num, ...) {
//  VALUE *args = 0;
//  va_list ar;
//
//  if (num > 0) {
//    args = ALLOCA_N(VALUE, num);
//    va_start(ar, num);
//    for (size_t i = 0; i < num; i++) {
//      args[i] = va_arg(ar, VALUE);
//    }
//    va_end(ar);
//  }
//
//  return rb_str_concat_literals(num, args);
//}
//
//// https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L515-L534
//VALUE
//llrb_insn_splatarray(VALUE ary, VALUE flag) {
//  VALUE tmp = rb_check_convert_type(ary, T_ARRAY, "Array", "to_a");
//  if (NIL_P(tmp)) {
//    tmp = rb_ary_new3(1, ary);
//  } else if (RTEST(flag)) {
//    tmp = rb_ary_dup(tmp);
//  }
//  return tmp;
//}
//
//VALUE vm_defined(rb_thread_t *th, rb_control_frame_t *reg_cfp, rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v);
//VALUE
//llrb_insn_defined(rb_num_t op_type, VALUE obj, VALUE needstr, VALUE v)
//{
//  rb_thread_t *th = GET_THREAD();
//  return vm_defined(th, th->cfp, op_type, obj, needstr, v);
//}
//
//VALUE vm_getspecial(rb_thread_t *th, const VALUE *lep, rb_num_t key, rb_num_t type);
//VALUE
//llrb_insn_getspecial(rb_num_t key, rb_num_t type)
//{
//  rb_thread_t *th = GET_THREAD();
//
//  const VALUE *ep = th->cfp->ep;
//  while (!VM_ENV_LOCAL_P(ep)) {
//    ep = VM_ENV_PREV_EP(ep);
//  }
//  return vm_getspecial(th, ep, key, type);
//}
//
//void lep_svar_set(rb_thread_t *th, const VALUE *lep, rb_num_t key, VALUE val);
//void
//llrb_insn_setspecial(rb_num_t key, VALUE obj)
//{
//  rb_thread_t *th = GET_THREAD();
//
//  const VALUE *ep = th->cfp->ep;
//  while (!VM_ENV_LOCAL_P(ep)) {
//    ep = VM_ENV_PREV_EP(ep);
//  }
//
//  lep_svar_set(th, ep, key, obj);
//}

VALUE
llrb_insn_opt_plus(VALUE lhs, VALUE rhs)
{
  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_PLUS, INTEGER_REDEFINED_OP_FLAG)) {
    return LONG2NUM(FIX2LONG(lhs) + FIX2LONG(rhs));
  }
  return rb_funcall(lhs, '+', 1, rhs);
}

//VALUE
//llrb_insn_opt_minus(VALUE lhs, VALUE rhs)
//{
//  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
//    return LONG2NUM(FIX2LONG(lhs) - FIX2LONG(rhs));
//  }
//  return rb_funcall(lhs, '-', 1, rhs);
//}
//
//VALUE
//llrb_insn_opt_lt(VALUE lhs, VALUE rhs)
//{
//  if (FIXNUM_2_P(lhs, rhs) && BASIC_OP_UNREDEFINED_P(BOP_MINUS, INTEGER_REDEFINED_OP_FLAG)) {
//    SIGNED_VALUE a = lhs, b = rhs;
//
//    if (a < b) {
//      return Qtrue;
//    } else {
//      return Qfalse;
//    }
//  }
//  return rb_funcall(lhs, '<', 1, rhs);
//}
//
//VALUE vm_get_cvar_base(const rb_cref_t *cref, rb_control_frame_t *cfp);
//rb_cref_t * rb_vm_get_cref(const VALUE *ep);
//VALUE
//llrb_insn_getclassvariable(rb_control_frame_t *cfp, ID id)
//{
//  return rb_cvar_get(vm_get_cvar_base(rb_vm_get_cref(cfp->ep), cfp), id);
//}
//
//VALUE vm_get_cvar_base(const rb_cref_t *cref, rb_control_frame_t *cfp);
//void
//llrb_insn_setclassvariable(rb_control_frame_t *cfp, ID id, VALUE val)
//{
//  vm_ensure_not_refinement_module(cfp->self);
//  rb_cvar_set(vm_get_cvar_base(rb_vm_get_cref(cfp->ep), cfp), id, val);
//}

static inline void
_llrb_push_result(rb_control_frame_t *cfp, VALUE result)
{
  // PUSH(result)
  *(cfp->sp) = result;
  cfp->sp += 1;
}

void
llrb_push_result(VALUE cfp, VALUE result)
{
  _llrb_push_result((rb_control_frame_t *)cfp, result);
}

//VALUE vm_throw(rb_thread_t *th, rb_control_frame_t *reg_cfp, rb_num_t throw_state, VALUE throwobj);
//void
//llrb_insn_throw(rb_thread_t *th, rb_control_frame_t *cfp, rb_num_t throw_state, VALUE throwobj)
//{
//  RUBY_VM_CHECK_INTS(th);
//  th->errinfo = vm_throw(th, cfp, throw_state, throwobj);
//}
//
//// TODO: Use vm_check_keyword after Ruby 2.5
//VALUE
//llrb_insn_checkkeyword(rb_control_frame_t *cfp, lindex_t kw_bits_index, rb_num_t keyword_index)
//{
//  const VALUE *ep = cfp->ep;
//  const VALUE kw_bits = *(ep - kw_bits_index);
//
//  if (FIXNUM_P(kw_bits)) {
//    int bits = FIX2INT(kw_bits);
//    return (bits & (0x01 << keyword_index)) ? Qfalse : Qtrue;
//  }
//  else {
//    VM_ASSERT(RB_TYPE_P(kw_bits, T_HASH));
//    return rb_hash_has_key(kw_bits, INT2FIX(keyword_index)) ? Qfalse : Qtrue;
//  }
//}
//
//// TODO: Use vm_concat_array after Ruby 2.5
//VALUE
//llrb_insn_concatarray(VALUE ary1, VALUE ary2st)
//{
//  const VALUE ary2 = ary2st;
//  VALUE tmp1 = rb_check_convert_type(ary1, T_ARRAY, "Array", "to_a");
//  VALUE tmp2 = rb_check_convert_type(ary2, T_ARRAY, "Array", "to_a");
//
//  if (NIL_P(tmp1)) {
//    tmp1 = rb_ary_new3(1, ary1);
//  }
//
//  if (NIL_P(tmp2)) {
//    tmp2 = rb_ary_new3(1, ary2);
//  }
//
//  if (tmp1 == ary1) {
//    tmp1 = rb_ary_dup(ary1);
//  }
//  return rb_ary_concat(tmp1, tmp2);
//}

// TODO: Use vm_dtrace after Ruby 2.5
static inline void
_llrb_insn_trace(rb_thread_t *th, rb_control_frame_t *cfp, rb_event_flag_t flag, VALUE val)
{
  if (RUBY_DTRACE_METHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_METHOD_RETURN_ENABLED() ||
      RUBY_DTRACE_CMETHOD_ENTRY_ENABLED() ||
      RUBY_DTRACE_CMETHOD_RETURN_ENABLED()) {

    switch (flag) {
      case RUBY_EVENT_CALL:
        RUBY_DTRACE_METHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_CALL:
        RUBY_DTRACE_CMETHOD_ENTRY_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_RETURN:
        RUBY_DTRACE_METHOD_RETURN_HOOK(th, 0, 0);
        break;
      case RUBY_EVENT_C_RETURN:
        RUBY_DTRACE_CMETHOD_RETURN_HOOK(th, 0, 0);
        break;
    }
  }

  // TODO: Confirm it works!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // Especially for :line event. We may need to update program counter.
  EXEC_EVENT_HOOK(th, flag, cfp->self, 0, 0, 0 /* id and klass are resolved at callee */, val);
}

void
llrb_insn_trace(VALUE th, VALUE cfp, rb_event_flag_t flag, VALUE val)
{
  _llrb_insn_trace((rb_thread_t *)th, (rb_control_frame_t *)cfp, flag, val);
}

//#define CALL_METHOD(calling, ci, cc) (*(cc)->call)(th, cfp, (calling), (ci), (cc))
//void vm_search_method(const struct rb_call_info *ci, struct rb_call_cache *cc, VALUE recv);
//VALUE vm_exec(rb_thread_t *th);
//VALUE // TODO: refactor with invokesuper
//llrb_insn_opt_send_without_block(rb_thread_t *th, rb_control_frame_t *cfp, CALL_INFO ci, CALL_CACHE cc, unsigned int stack_size, ...)
//{
//  VALUE recv = Qnil;
//  va_list ar;
//  va_start(ar, stack_size);
//  for (unsigned int i = 0; i < stack_size; i++) {
//    VALUE val = va_arg(ar, VALUE);
//    if (i == 0) recv = val;
//    llrb_push_result(cfp, val);
//  }
//  va_end(ar);
//
//  vm_search_method(ci, cc, recv);
//
//  struct rb_calling_info calling;
//  calling.block_handler = VM_BLOCK_HANDLER_NONE;
//  calling.argc = ci->orig_argc;
//  calling.recv = recv;
//
//  VALUE result = CALL_METHOD(&calling, ci, cc);
//  if (result == Qundef) {
//    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
//    return vm_exec(th);
//  }
//  return result;
//}
//
//void vm_caller_setup_arg_block(const rb_thread_t *th, rb_control_frame_t *reg_cfp,
//    struct rb_calling_info *calling, const struct rb_call_info *ci, rb_iseq_t *blockiseq, const int is_super);
//void vm_search_super_method(rb_thread_t *th, rb_control_frame_t *reg_cfp,
//    struct rb_calling_info *calling, struct rb_call_info *ci, struct rb_call_cache *cc);
//VALUE
//llrb_insn_invokesuper(rb_thread_t *th, rb_control_frame_t *cfp, CALL_INFO ci, CALL_CACHE cc, ISEQ blockiseq, unsigned int stack_size, ...)
//{
//  va_list ar;
//  va_start(ar, stack_size);
//  for (unsigned int i = 0; i < stack_size; i++) {
//    llrb_push_result(cfp, va_arg(ar, VALUE));
//  }
//  va_end(ar);
//
//  struct rb_calling_info calling;
//  calling.argc = ci->orig_argc;
//
//  vm_caller_setup_arg_block(th, cfp, &calling, ci, blockiseq, 1);
//  calling.recv = th->cfp->self;
//  vm_search_super_method(th, th->cfp, &calling, ci, cc);
//
//  VALUE result = CALL_METHOD(&calling, ci, cc);
//  if (result == Qundef) {
//    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
//    return vm_exec(th);
//  }
//  return result;
//}
//
//VALUE vm_invoke_block(rb_thread_t *th, rb_control_frame_t *reg_cfp, struct rb_calling_info *calling, const struct rb_call_info *ci);
//VALUE
//llrb_insn_invokeblock(rb_thread_t *th, rb_control_frame_t *cfp, CALL_INFO ci, unsigned int stack_size, ...)
//{
//  va_list ar;
//  va_start(ar, stack_size);
//  for (unsigned int i = 0; i < stack_size; i++) {
//    llrb_push_result(cfp, va_arg(ar, VALUE));
//  }
//  va_end(ar);
//
//  struct rb_calling_info calling;
//  calling.argc = ci->orig_argc;
//  calling.block_handler = VM_BLOCK_HANDLER_NONE;
//  calling.recv = cfp->self;
//
//  VALUE val = vm_invoke_block(th, cfp, &calling, ci);
//  if (val == Qundef) {
//    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
//    return vm_exec(th);
//  }
//  return val;
//}
//
//VALUE
//llrb_insn_send(rb_thread_t *th, rb_control_frame_t *cfp, CALL_INFO ci, CALL_CACHE cc, ISEQ blockiseq, unsigned int stack_size, ...)
//{
//  VALUE recv = Qnil;
//  va_list ar;
//  va_start(ar, stack_size);
//  for (unsigned int i = 0; i < stack_size; i++) {
//    VALUE val = va_arg(ar, VALUE);
//    if (i == 0) recv = val;
//    llrb_push_result(cfp, val);
//  }
//  va_end(ar);
//
//  struct rb_calling_info calling;
//
//  vm_caller_setup_arg_block(th, cfp, &calling, ci, blockiseq, 0);
//  calling.argc = ci->orig_argc;
//  calling.recv = recv;
//  vm_search_method(ci, cc, recv);
//
//  VALUE result = CALL_METHOD(&calling, ci, cc);
//  if (result == Qundef) {
//    VM_ENV_FLAGS_SET(th->cfp->ep, VM_FRAME_FLAG_FINISH);
//    return vm_exec(th);
//  }
//  return result;
//}
