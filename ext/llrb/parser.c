/*
 * parser.c: Constructs Control Flow Graph from encoded YARV instructions in ISeq.
 */

#include <stdio.h>
#include "cfg.h"
#include "cruby.h"

static VALUE rb_eParseError;

// Return sorted unique Ruby array of Basic Block start positions. Example: [0, 2, 8].
//
// It's constructed in the following rule.
//   Rule 1: 0 is always included
//   Rule 2: TS_OFFSET numers for are branch and jump instructions are included
//   Rule 3: Positions immediately after jump instructions (jump, branchnil, branchif, branchunless, opt_case_dispatch, leave) are included
static VALUE
llrb_basic_block_starts(const struct rb_iseq_constant_body *body)
{
  // XXX: No need to check leave? leave is always in the end?

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
      .traversed = false,
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
llrb_set_incoming_blocks_by(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg, struct llrb_basic_block *block)
{
  if (block->traversed) return;
  block->traversed = true;

  struct llrb_basic_block *next_block = 0;
  struct llrb_basic_block *last_block = cfg->blocks + (cfg->size-1);
  if (block < last_block) next_block = block + 1;

  // XXX: No need to check leave? leave is always in the end?
  int end_insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[block->end]);
  switch (end_insn) {
    case YARVINSN_branchnil:
    case YARVINSN_branchif:
    case YARVINSN_branchunless: {
      VALUE offset = (rb_num_t)body->iseq_encoded[block->end+1];
      struct llrb_basic_block *dest_block = llrb_find_block(cfg, block->end + insn_len(end_insn) + offset);
      llrb_push_incoming_start(dest_block, block->start);
      llrb_set_incoming_blocks_by(body, cfg, dest_block);

      if (next_block) {
        llrb_push_incoming_start(next_block, block->start);
        llrb_set_incoming_blocks_by(body, cfg, next_block);
      }
      break;
    }
    case YARVINSN_jump: {
      VALUE offset = (rb_num_t)body->iseq_encoded[block->end+1];
      struct llrb_basic_block *dest_block = llrb_find_block(cfg, block->end + insn_len(end_insn) + offset);
      llrb_push_incoming_start(dest_block, block->start);
      llrb_set_incoming_blocks_by(body, cfg, dest_block);
      break;
    }
    case YARVINSN_throw: // TODO: should be modified when catch table is implemented
      break; // no next block
    //case YARVINSN_opt_case_dispatch: // TODO: MUST be modified when opt_case_dispatch is implemented
    default: {
      if (next_block) {
        llrb_push_incoming_start(next_block, block->start);
        llrb_set_incoming_blocks_by(body, cfg, next_block);
      }
      break;
    }
  }
}

// To prevent from creating incoming block from unreachable basic block, this does not loop cfg's basic blocks
// but traverse insns instead. This may be unnecessary after catch table is implemented...
static void
llrb_set_incoming_blocks(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg)
{
  llrb_set_incoming_blocks_by(body, cfg, cfg->blocks);
}

void
llrb_parse_iseq(const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg)
{
  llrb_create_basic_blocks(body, cfg);
  llrb_set_incoming_blocks(body, cfg);
  if (0) llrb_dump_cfg(body, cfg);
}

void
Init_parser(VALUE rb_mJIT)
{
  rb_eParseError = rb_define_class_under(rb_mJIT, "ParseError", rb_eStandardError);
}
