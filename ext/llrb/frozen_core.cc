#include "iseq.h"
#include "llrb.h"
#include "native_compiler.h"

VALUE
rb_llrb_core_define_method(VALUE frozen_core, VALUE method_sym, VALUE iseq_ary)
{
  llrb::Iseq iseq(iseq_ary);
  uint64_t func = llrb::NativeCompiler().Compile(iseq);
  if (!func) {
    fprintf(stderr, "Failed to define native function...\n");
    return Qnil;
  }

  VALUE misc = RARRAY_AREF(iseq_ary, 4);
  VALUE arity = rb_hash_aref(misc, rb_id2sym(rb_intern("arg_size")));
  VALUE context = rb_ivar_get(frozen_core, rb_intern("__llrb_self__"));
  VALUE method_str = rb_sym_to_s(method_sym);

  rb_define_method(rb_obj_class(context), RSTRING_PTR(method_str), RUBY_METHOD_FUNC(func), FIX2INT(arity));
  return method_sym;
}

// Even with that name, actually not frozen to set/get ivar.
VALUE rb_mLLRBFrozenCore;

extern "C" {
  void
  Init_llrb_frozen_core(VALUE rb_mLLRB)
  {
    // The same as: https://github.com/ruby/ruby/blob/v2_4_1/vm.c#L2766-L2788
    rb_mLLRBFrozenCore = rb_class_new(rb_cBasicObject);
    RBASIC(rb_mLLRBFrozenCore)->flags = T_ICLASS;
    rb_define_singleton_method(rb_mLLRBFrozenCore, "core#define_method", RUBY_METHOD_FUNC(rb_llrb_core_define_method), 2);
    rb_gc_register_mark_object(rb_mLLRBFrozenCore); // not defined class will be GCed without this
  }
}
