/*
 * cfg.h: Has a Control Flow Graph struct definition shared by parser.c and compiler.c.
 */

#ifndef LLRB_CFG_H
#define LLRB_CFG_H

#include <stdbool.h>

struct llrb_basic_block {
  // Fields set by parser:
  unsigned int start;            // Start index of ISeq body's iseq_encoded.
  unsigned int end;              // End index of ISeq body's iseq_encoded.
  unsigned int incoming_size;    // Size of incoming_starts.
  unsigned int *incoming_starts; // Start indices of incoming basic blocks.
  bool traversed; // Used by `llrb_set_incoming_blocks_by` to prevent infinite recursion by circular reference.

  // Fields set by compiler:
  bool compiled;
};

// Holds Control-Flow-Graph-like data structure. Actually it's a buffer of graph nodes.
struct llrb_cfg {
  struct llrb_basic_block* blocks;
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
}

#endif // LLRB_CFG_H
