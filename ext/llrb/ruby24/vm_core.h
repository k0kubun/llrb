#ifndef LLRB_RUBY24_VM_CORE_H
#define LLRB_RUBY24_VM_CORE_H

#include "llrb.h"
#include "ruby/defines.h"

/* llrb: vm_insnhelper.h */
#define FIXNUM_2_P(a, b) ((a) & (b) & 1)

/* llrb: ccan/list/list.h */
/* Licensed under BSD-MIT - see ext/llrb/ruby24/BSD-MIT file for details */

struct list_node
{
        struct list_node *next, *prev;
};

struct list_head
{
        struct list_node n;
};

/* llrb: internal.h */
/* Licensed under Ruby License - see COPYING for details */

#define LIKELY(x) RB_LIKELY(x)
#define UNLIKELY(x) RB_UNLIKELY(x)

/* llrb: vm_core.h */
/* Licensed under Ruby License - see COPYING for details */

#include "ruby/thread_native.h"
#if   defined(_WIN32)

typedef struct rb_global_vm_lock_struct {
    HANDLE lock;
} rb_global_vm_lock_t;

#elif defined(HAVE_PTHREAD_H)

#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif

typedef struct rb_thread_cond_struct {
    pthread_cond_t cond;
#ifdef HAVE_CLOCKID_T
    clockid_t clockid;
#endif
} rb_nativethread_cond_t;

typedef struct rb_global_vm_lock_struct {
    /* fast path */
    unsigned long acquired;
    rb_nativethread_lock_t lock;

    /* slow path */
    volatile unsigned long waiting;
    rb_nativethread_cond_t cond;

    /* yield */
    rb_nativethread_cond_t switch_cond;
    rb_nativethread_cond_t switch_wait_cond;
    //int need_yield;
    //int wait_yield;
} rb_global_vm_lock_t;

#endif

#include <signal.h>

#ifndef NSIG
# define NSIG (_SIGMAX + 1)      /* For QNX */
#endif

#define RUBY_NSIG NSIG

enum ruby_special_exceptions {
    ruby_error_reenter,
    ruby_error_nomemory,
    ruby_error_sysstack,
    ruby_error_closed_stream,
    ruby_special_error_count
};

enum ruby_basic_operators {
    BOP_PLUS,
    BOP_MINUS,
    BOP_MULT,
    BOP_DIV,
    BOP_MOD,
    BOP_EQ,
    BOP_EQQ,
    BOP_LT,
    BOP_LE,
    BOP_LTLT,
    BOP_AREF,
    BOP_ASET,
    BOP_LENGTH,
    BOP_SIZE,
    BOP_EMPTY_P,
    BOP_SUCC,
    BOP_GT,
    BOP_GE,
    BOP_NOT,
    BOP_NEQ,
    BOP_MATCH,
    BOP_FREEZE,
    BOP_MAX,
    BOP_MIN,

    BOP_LAST_
};

struct rb_vm_struct;
typedef void rb_vm_at_exit_func(struct rb_vm_struct*);

typedef struct rb_at_exit_list {
    rb_vm_at_exit_func *func;
    struct rb_at_exit_list *next;
} rb_at_exit_list;

typedef struct rb_hook_list_struct {
    struct rb_event_hook_struct *hooks;
    rb_event_flag_t events;
    int need_clean;
} rb_hook_list_t;

typedef struct rb_vm_struct {
    VALUE self;

    rb_global_vm_lock_t gvl;
    rb_nativethread_lock_t    thread_destruct_lock;

    struct rb_thread_struct *main_thread;
    struct rb_thread_struct *running_thread;

    struct list_head living_threads;
    size_t living_thread_num;
    VALUE thgroup_default;

    unsigned int running: 1;
    unsigned int thread_abort_on_exception: 1;
    unsigned int thread_report_on_exception: 1;
    int trace_running;
    volatile int sleeper;

    /* object management */
    VALUE mark_object_ary;
    const VALUE special_exceptions[ruby_special_error_count];

    /* load */
    VALUE top_self;
    VALUE load_path;
    VALUE load_path_snapshot;
    VALUE load_path_check_cache;
    VALUE expanded_load_path;
    VALUE loaded_features;
    VALUE loaded_features_snapshot;
    struct st_table *loaded_features_index;
    struct st_table *loading_table;

    /* signal */
    struct {
        VALUE cmd;
        int safe;
    } trap_list[RUBY_NSIG];

    /* hook */
    rb_hook_list_t event_hooks;

    /* relation table of ensure - rollback for callcc */
    struct st_table *ensure_rollback_table;

    /* postponed_job */
    struct rb_postponed_job_struct *postponed_job_buffer;
    int postponed_job_index;

    int src_encoding_index;

    VALUE verbose, debug, orig_progname, progname;
    VALUE coverages;

    VALUE defined_module_hash;

    struct rb_objspace *objspace;

    rb_at_exit_list *at_exit;

    VALUE *defined_strings;
    st_table *frozen_strings;

    /* params */
    struct { /* size in byte */
        size_t thread_vm_stack_size;
        size_t thread_machine_stack_size;
        size_t fiber_vm_stack_size;
        size_t fiber_machine_stack_size;
    } default_params;

    short redefined_flag[BOP_LAST_];
} rb_vm_t;

/* optimize insn */
#define INTEGER_REDEFINED_OP_FLAG (1 << 0)
#define FLOAT_REDEFINED_OP_FLAG  (1 << 1)
#define STRING_REDEFINED_OP_FLAG (1 << 2)
#define ARRAY_REDEFINED_OP_FLAG  (1 << 3)
#define HASH_REDEFINED_OP_FLAG   (1 << 4)
/* #define BIGNUM_REDEFINED_OP_FLAG (1 << 5) */
#define SYMBOL_REDEFINED_OP_FLAG (1 << 6)
#define TIME_REDEFINED_OP_FLAG   (1 << 7)
#define REGEXP_REDEFINED_OP_FLAG (1 << 8)
#define NIL_REDEFINED_OP_FLAG    (1 << 9)
#define TRUE_REDEFINED_OP_FLAG   (1 << 10)
#define FALSE_REDEFINED_OP_FLAG  (1 << 11)

#define BASIC_OP_UNREDEFINED_P(op, klass) (LIKELY((GET_VM()->redefined_flag[(op)]&(klass)) == 0))

extern rb_vm_t *ruby_current_vm;
#define GET_VM() ruby_current_vm

#endif // LLRB_RUBY24_VM_CORE_H
