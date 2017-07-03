/*
 * cfg.h: Has a Control Flow Graph struct definition shared by parser.c and compiler.c.
 */

#ifndef LLRB_CFG_H
#define LLRB_CFG_H

#include <stdbool.h>
#include "llvm-c/Core.h"

struct llrb_basic_block {
  // Fields set by parser:
  unsigned int start;            // Start index of ISeq body's iseq_encoded.
  unsigned int end;              // End index of ISeq body's iseq_encoded.
  unsigned int incoming_size;    // Size of incoming_starts.
  unsigned int *incoming_starts; // Start indices of incoming basic blocks. This buffer is freed by llrb_destruct_cfg.
  bool traversed;                // Prevents infinite loop in `llrb_set_incoming_blocks_by` and used by compiler to judge reachable or not.

  // Fields set by compiler:
  LLVMBasicBlockRef ref;         // LLVM's actual BasicBlock reference. This value is always available after `llrb_init_cfg_for_compile` is called.
  LLVMValueRef phi;              // Phi node to collect incoming values. This will be created if incoming_size > 1 and compiled time's stack size > 0.
  bool compiled;                 // Prevents infinite loop in `llrb_compile_basic_block`.
};

// Holds Control-Flow-Graph-like data structure. Actually it's a buffer of graph nodes.
struct llrb_cfg {
  struct llrb_basic_block* blocks; // This buffer is freed by llrb_destruct_cfg.
  unsigned int size;
};

// Used by llrb_dump_cfg.
#include "cruby.h"
#include "cruby_extra/insns.inc"
#include "cruby_extra/insns_info.inc"

// Not using `rb_iseq_original_iseq` to avoid unnecessary memory allocation.
extern int rb_vm_insn_addr2insn(const void *addr);

static void
llrb_disasm_insns(const struct rb_iseq_constant_body *body, unsigned int start, unsigned int end)
{
  for (unsigned int i = start; i <= end;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    fprintf(stderr, "  %04d %-27s [%-4s] ", i, insn_name(insn), insn_op_types(insn));

    for (int j = 1; j < insn_len(insn); j++) {
      VALUE op = body->iseq_encoded[i+j];
      switch (insn_op_type(insn, j-1)) {
        case TS_NUM:
          fprintf(stderr, "%-4ld ", (rb_num_t)op);
          break;
        case TS_OFFSET:
          fprintf(stderr, "%"PRIdVALUE" ", (VALUE)(i + j + op + 1));
          break;
      }
    }
    fprintf(stderr, "\n");
    i += insn_len(insn);
  }
  fprintf(stderr, "\n");
}

static void
llrb_dump_catch_table(const struct iseq_catch_table *ct)
{
  if (!ct) return;

  fprintf(stderr, "-- LLRB: catch table (size=%d)----------------\n", ct->size);
  for (unsigned int i = 0; i < ct->size; i++) {
    const struct iseq_catch_table_entry *entry = &ct->entries[i];

    switch (entry->type) {
      case CATCH_TYPE_RESCUE:
        fprintf(stderr, "CATCH_TYPE_RESCUE");
        break;
      case CATCH_TYPE_ENSURE:
        fprintf(stderr, "CATCH_TYPE_ENSURE");
        break;
      case CATCH_TYPE_RETRY:
        fprintf(stderr, "CATCH_TYPE_RETRY");
        break;
      case CATCH_TYPE_BREAK:
        fprintf(stderr, "CATCH_TYPE_BREAK");
        break;
      case CATCH_TYPE_REDO:
        fprintf(stderr, "CATCH_TYPE_REDO");
        break;
      case CATCH_TYPE_NEXT:
        fprintf(stderr, "CATCH_TYPE_NEXT");
        break;
    }

    fprintf(stderr, ": start=%d, end=%d, cont=%d, sp=%d, iseq=%lx\n",
        entry->start, entry->end, entry->cont, entry->sp, (VALUE)entry->iseq);
  }
  fprintf(stderr, "\n");
}

// NOTE: insns(|_info).inc has some static functions, and llrb_dump_cfg uses all those functions.
// Thus we must call this function somewhere in all included files to pass compilation. `if (0)` is placed for such a purpose.
static void
llrb_dump_cfg(const struct rb_iseq_constant_body *body, const struct llrb_cfg *cfg)
{
  fprintf(stderr, "\n== LLRB: cfg ================================\n");
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    fprintf(stderr, "BasicBlock[%d-%d]", block->start, block->end);

    if (block->incoming_size > 0) fprintf(stderr, " <- ");
    if (!block->traversed) fprintf(stderr, " UNREACHABLE");
    for (unsigned int j = 0; j < block->incoming_size; j++) {
      fprintf(stderr, "%d", block->incoming_starts[j]);
      if (j != block->incoming_size-1) {
        fprintf(stderr, ", ");
      }
    }

    fprintf(stderr, "\n");
    llrb_disasm_insns(body, block->start, block->end);
  }
  llrb_dump_catch_table(body->catch_table);
}

#endif // LLRB_CFG_H
