#include "ruby.h"
#include "ruby/debug.h"

static void
llrb_funcall_callback(VALUE val, void *ptr)
{
  ;
}

static void
rb_jit_start(RB_UNUSED_VAR(VALUE self))
{
  VALUE tpval = rb_tracepoint_new(Qnil, RUBY_EVENT_CALL | RUBY_EVENT_B_CALL, llrb_funcall_callback, 0);
  rb_tracepoint_enable(tpval);
  RB_GC_GUARD(tpval);
}

static void
rb_jit_stop(RB_UNUSED_VAR(VALUE self))
{
  ;
}

void
Init_profiler(VALUE rb_mJIT)
{
  rb_define_singleton_method(rb_mJIT, "start", RUBY_METHOD_FUNC(rb_jit_start), 0);
  rb_define_singleton_method(rb_mJIT, "stop", RUBY_METHOD_FUNC(rb_jit_stop), 0);
}
