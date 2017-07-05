#include "ruby.h"
#include "ruby/debug.h"
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

static int running = 0;

static void
llrb_job_handler(void *data)
{
  fprintf(stderr, ".");
}

static void
llrb_signal_handler(int sig, siginfo_t *sinfo, void *ucontext)
{
  if (!rb_during_gc()) {
    rb_postponed_job_register_one(0, llrb_job_handler, 0);
  }
}

static VALUE
rb_jit_start(RB_UNUSED_VAR(VALUE self))
{
  struct sigaction sa;
  struct itimerval timer;

  if (running) return Qfalse;

  sa.sa_sigaction = llrb_signal_handler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPROF, &sa, NULL);

  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 1000;
  timer.it_value = timer.it_interval;
  setitimer(ITIMER_PROF, &timer, 0);

  running = 1;
  return Qtrue;
}

static VALUE
rb_jit_stop(RB_UNUSED_VAR(VALUE self))
{
  struct sigaction sa;
  struct itimerval timer;

  if (!running) return Qfalse;
  running = 0;

	memset(&timer, 0, sizeof(timer));
	setitimer(ITIMER_PROF, &timer, 0);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPROF, &sa, NULL);

  return Qtrue;
}

static void
llrb_atfork_prepare(void)
{
  struct itimerval timer;
  if (running) {
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_PROF, &timer, 0);
  }
}

static void
llrb_atfork_parent(void)
{
  struct itimerval timer;
  if (running) {
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_value = timer.it_interval;
    setitimer(ITIMER_PROF, &timer, 0);
  }
}

static void
llrb_atfork_child(void)
{
  rb_jit_stop(Qnil);
}

void
Init_profiler(VALUE rb_mJIT)
{
  rb_define_singleton_method(rb_mJIT, "start", RUBY_METHOD_FUNC(rb_jit_start), 0);
  rb_define_singleton_method(rb_mJIT, "stop", RUBY_METHOD_FUNC(rb_jit_stop), 0);

  pthread_atfork(llrb_atfork_prepare, llrb_atfork_parent, llrb_atfork_child);
}
