#include "ruby.h"
#include "ruby/debug.h"
#include "cruby.h"
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#define LLRB_BUF_SIZE 2048

static struct {
  int running;
  VALUE frames_buffer[LLRB_BUF_SIZE];
  int lines_buffer[LLRB_BUF_SIZE];
} llrb_profiler;

static void
llrb_dump_iseq(const rb_iseq_t *iseq)
{
	switch (iseq->body->type) {
	  case ISEQ_TYPE_METHOD:
      fprintf(stderr," ISEQ_TYPE_METHOD:");
	    break;
	  case ISEQ_TYPE_CLASS:
      fprintf(stderr," ISEQ_TYPE_CLASS:");
	    break;
	  case ISEQ_TYPE_BLOCK:
      fprintf(stderr," ISEQ_TYPE_BLOCK:");
	    break;
	  case ISEQ_TYPE_EVAL:
      fprintf(stderr," ISEQ_TYPE_EVAL:");
	    break;
	  case ISEQ_TYPE_MAIN:
      fprintf(stderr," ISEQ_TYPE_MAIN:");
	    break;
	  case ISEQ_TYPE_TOP:
      fprintf(stderr," ISEQ_TYPE_TOP:");
	    break;
	  case ISEQ_TYPE_RESCUE:
      fprintf(stderr," ISEQ_TYPE_RESCUE:");
	    break;
	  case ISEQ_TYPE_ENSURE:
      fprintf(stderr," ISEQ_TYPE_ENSURE:");
	    break;
	  case ISEQ_TYPE_DEFINED_GUARD:
      fprintf(stderr," ISEQ_TYPE_DEFINED_GUARD:");
	    break;
	  default:
      fprintf(stderr," default:");
      break;
	}
}

static void
llrb_profile_iseq(const rb_iseq_t *iseq, int line)
{
  VALUE name = rb_profile_frame_full_label((VALUE)iseq);
  fprintf(stderr, "iseq:");
  llrb_dump_iseq(iseq);
  fprintf(stderr, " %s (%d) %lx\n", RSTRING_PTR(name), line, (VALUE)iseq);
}

static void
llrb_profile_callable_method_entry(const rb_callable_method_entry_t *cme, int line)
{
  VALUE name = rb_profile_frame_full_label((VALUE)cme);
  const rb_iseq_t *iseq = cme->def->body.iseq.iseqptr;
  fprintf(stderr, "cme:");
  llrb_dump_iseq(iseq);
  fprintf(stderr, " %s (%d) %lx\n", RSTRING_PTR(name), line, (VALUE)cme);
}

static void
llrb_profile_frames()
{
  int num = rb_profile_frames(0, sizeof(llrb_profiler.frames_buffer) / sizeof(VALUE),
      llrb_profiler.frames_buffer, llrb_profiler.lines_buffer);

  fprintf(stderr, "---start---\n");
  for (int i = 0; i < num; i++) {
    int line = llrb_profiler.lines_buffer[i];
    VALUE frame = llrb_profiler.frames_buffer[i];

    switch (imemo_type(frame)) {
      case imemo_iseq:
        llrb_profile_iseq((const rb_iseq_t *)frame, line);
        break;
      case imemo_ment:
        llrb_profile_callable_method_entry((const rb_callable_method_entry_t *)frame, line);
        break;
      default:
        break;
    }
  }
  fprintf(stderr, "---end---\n");
}

static void
llrb_job_handler(void *data)
{
  static int in_job_handler = 0;
  if (in_job_handler) return;
  if (!llrb_profiler.running) return;

  in_job_handler++;
  llrb_profile_frames();
  in_job_handler--;
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

  if (llrb_profiler.running) return Qfalse;

  sa.sa_sigaction = llrb_signal_handler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPROF, &sa, NULL);

  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 1000;
  timer.it_value = timer.it_interval;
  setitimer(ITIMER_PROF, &timer, 0);

  llrb_profiler.running = 1;
  return Qtrue;
}

static VALUE
rb_jit_stop(RB_UNUSED_VAR(VALUE self))
{
  struct sigaction sa;
  struct itimerval timer;

  if (!llrb_profiler.running) return Qfalse;
  llrb_profiler.running = 0;

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
  if (llrb_profiler.running) {
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_PROF, &timer, 0);
  }
}

static void
llrb_atfork_parent(void)
{
  struct itimerval timer;
  if (llrb_profiler.running) {
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

  llrb_profiler.running = 0;
  pthread_atfork(llrb_atfork_prepare, llrb_atfork_parent, llrb_atfork_child);
}
