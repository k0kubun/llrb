/*
 * parser.c: Constructs Control Flow Graph from encoded YARV instructions in ISeq.
 */

#include <stdio.h>
#include "cfg.h"
#include "cruby.h"
#include "cruby_extra/insns.inc"
#include "cruby_extra/insns_info.inc"

static VALUE rb_eParseError;

// Not using `rb_iseq_original_iseq` to avoid unnecessary memory allocation.
extern int rb_vm_insn_addr2insn(const void *addr);

// Return sorted unique Ruby array of Basic Block start positions. Example: [0, 2, 8].
//
// It's constructed in the following rule.
//   Rule 1: 0 is always included
//   Rule 2: TS_OFFSET numers for are branch and jump instructions are included
//   Rule 3: Positions immediately after jump instructions (jump, branchnil, branchif, branchunless, opt_case_dispatch, leave) are included
static VALUE
llrb_basic_block_starts(const struct rb_iseq_constant_body *body)
{
  // Rule 1
  VALUE starts = rb_ary_new_capa(1);
  rb_ary_push(starts, INT2FIX(0));

  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);

    // Rule 2
    switch (insn) {
      case YARVINSN_branchif:
      case YARVINSN_branchunless:
      case YARVINSN_branchnil:
      case YARVINSN_jump: {
        VALUE op = body->iseq_encoded[i+1];
        rb_ary_push(starts, INT2FIX(i+insn_len(insn)+op));
        break;
      }
      //case YARVINSN_opt_case_dispatch:
    }

    // Rule 3
    switch (insn) {
      case YARVINSN_branchif:
      case YARVINSN_branchunless:
      case YARVINSN_branchnil:
      case YARVINSN_jump:
      case YARVINSN_throw:
        if (i+insn_len(insn) < body->iseq_size) {
          rb_ary_push(starts, INT2FIX(i+insn_len(insn)));
        }
        break;
      //case YARVINSN_opt_case_dispatch:
    }

    i += insn_len(insn);
  }
  starts = rb_ary_sort_bang(starts);
  rb_funcall(starts, rb_intern("uniq!"), 0);
  return starts;
}

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
llrb_dump_cfg(const struct rb_iseq_constant_body *body, const struct llrb_cfg *cfg)
{
  fprintf(stderr, "\n== cfg: LLRB ================================\n");
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    fprintf(stderr, "BasicBlock[%d-%d]", block->start, block->end);

    if (block->incoming_size > 0) fprintf(stderr, " <- ");
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

static unsigned int
llrb_find_block_end(const struct rb_iseq_constant_body *body, unsigned int start, VALUE starts)
{
  unsigned int i = start;
  while (i < body->iseq_size) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    unsigned int next_i = i + insn_len(insn);

    // If next insn or end of iseq is found, it's not a block end.
    if (RTEST(rb_ary_includes(starts, INT2FIX(next_i)))
        || next_i == body->iseq_size) {
      break;
    }
    i = next_i;
  }
  return i;
}

static void
llrb_create_basic_blocks(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg)
{
  VALUE starts = llrb_basic_block_starts(body);
  cfg->size = RARRAY_LEN(starts);
  cfg->blocks = (struct llrb_basic_block *)xmalloc(cfg->size * sizeof(struct llrb_basic_block)); // freed in llrb_destruct_cfg

  for (unsigned int i = 0; i < RARRAY_LEN(starts); i++) {
    unsigned int start = (unsigned int)FIX2INT(RARRAY_AREF(starts, i));
    cfg->blocks[i] = (struct llrb_basic_block){
      .start = start,
      .end = llrb_find_block_end(body, start, starts),
      .incoming_size = 0,
      .incoming_starts = 0,
    };
  }
  RB_GC_GUARD(starts);
}

static void
llrb_push_incoming_start(struct llrb_basic_block *block, unsigned int start)
{
  block->incoming_size++;
  if (block->incoming_size == 1) {
    block->incoming_starts = (unsigned int *)xmalloc(sizeof(unsigned int));
  } else {
    block->incoming_starts = (unsigned int *)xrealloc(
        block->incoming_starts, block->incoming_size * sizeof(unsigned int));
  }
  block->incoming_starts[block->incoming_size-1] = start;
}

static struct llrb_basic_block *
llrb_find_block(struct llrb_cfg *cfg, unsigned int start)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    if (block->start == start) return block;
  }
  rb_raise(rb_eParseError, "BasicBlock (start = %d) was not found in llrb_find_block", start);
}

static void
llrb_set_incoming_blocks(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    struct llrb_basic_block *next_block = 0;
    if (i < cfg->size-1) next_block = cfg->blocks + (i+1);

    int end_insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[block->end]);
    switch (end_insn) {
      case YARVINSN_branchnil:
      case YARVINSN_branchif:
      case YARVINSN_branchunless: {
        VALUE offset = (rb_num_t)body->iseq_encoded[block->end+1];
        llrb_push_incoming_start(llrb_find_block(cfg, block->end + insn_len(end_insn) + offset), block->start);
        if (next_block) llrb_push_incoming_start(next_block, block->start);
        break;
      }
      case YARVINSN_jump: {
        VALUE offset = (rb_num_t)body->iseq_encoded[block->end+1];
        llrb_push_incoming_start(llrb_find_block(cfg, block->end + insn_len(end_insn) + offset), block->start);
        break;
      }
      case YARVINSN_throw: // TODO: should be modified when catch table is implemented
        break; // no next block
      //case YARVINSN_opt_case_dispatch: // TODO: MUST be modified when opt_case_dispatch is implemented
      default: {
        if (next_block) llrb_push_incoming_start(next_block, block->start);
        break;
      }
    }
  }
}

void
llrb_parse_iseq(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg)
{
  llrb_create_basic_blocks(body, cfg);
  llrb_set_incoming_blocks(body, cfg);
  if (1) llrb_dump_cfg(body, cfg);
}

void
Init_parser(VALUE rb_mJIT)
{
  rb_eParseError = rb_define_class_under(rb_mJIT, "ParseError", rb_eStandardError);
}
