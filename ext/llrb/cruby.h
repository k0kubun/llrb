/*
 * cruby.h: Has CRuby's internal data structure definitions including ISeq struct.
 */

#ifndef LLRB_CRUBY_H
#define LLRB_CRUBY_H

#include "ruby.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "cruby/internal.h"
#include "cruby/vm_core.h"
#pragma GCC diagnostic pop
#include "cruby/method.h"
#include "cruby/vm_exec.h"

/* start vm_insnhelper.h (which can't be compiled without calling static function) */

/* optimize insn */
#define FIXNUM_2_P(a, b) ((a) & (b) & 1)
#if USE_FLONUM
#define FLONUM_2_P(a, b) (((((a)^2) | ((b)^2)) & 3) == 0) /* (FLONUM_P(a) && FLONUM_P(b)) */
#else
#define FLONUM_2_P(a, b) 0
#endif

#define GET_PREV_EP(ep)                ((VALUE *)((ep)[VM_ENV_DATA_INDEX_SPECVAL] & ~0x03))

/* end vm_insnhelper.h */

#endif // LLRB_CRUBY_H
