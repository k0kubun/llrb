#include "parser.h"

// Return sorted unique Ruby array of BasicBlock start positions like [0, 2, 8].
//
// It's constructed in the following rule.
//   Rule 1: 0 is always included
//   Rule 2: All TS_OFFSET numers are included
//   Rule 3: Positions immediately after jump instructions (jump, branchnil, branchif, branchunless, opt_case_dispatch, leave) are included
VALUE
llrb_basic_block_starts(const struct rb_iseq_constant_body *body)
{
  // For debug
  if (0) llrb_disasm_insns(body);

  // Rule 1
  VALUE starts = rb_ary_new_capa(1);
  rb_ary_push(starts, INT2FIX(0));

  for (unsigned int i = 0; i < body->iseq_size;) {
    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);

    // Rule 2
    for (int j = 1; j < insn_len(insn); j++) {
      VALUE op = body->iseq_encoded[i+j];
      switch (insn_op_type(insn, j-1)) {
        case TS_OFFSET:
          rb_ary_push(starts, INT2FIX((int)(i+insn_len(insn)+op)));
          break;
      }
    }

    // Rule 3
    switch (insn) {
      case YARVINSN_branchif:
      case YARVINSN_branchunless:
      case YARVINSN_branchnil:
      case YARVINSN_jump:
      case YARVINSN_opt_case_dispatch:
      case YARVINSN_throw:
        if (i+insn_len(insn) < body->iseq_size) {
          rb_ary_push(starts, INT2FIX(i+insn_len(insn)));
        }
        break;
    }

    i += insn_len(insn);
  }
  starts = rb_ary_sort_bang(starts);
  rb_funcall(starts, rb_intern("uniq!"), 0);
  return starts;
}

void
llrb_init_basic_blocks(struct llrb_compiler *c, const struct rb_iseq_constant_body *body, LLVMValueRef func)
{
  VALUE starts = llrb_basic_block_starts(body);

  for (long i = 0; i < RARRAY_LEN(starts); i++) {
    long start = FIX2INT(RARRAY_AREF(starts, i));
    unsigned int block_end;

    VALUE label = rb_str_new_cstr("label_"); // TODO: free?
    rb_str_catf(label, "%ld", start);
    LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, RSTRING_PTR(label));

    if (i == RARRAY_LEN(starts)-1) {
      block_end = (unsigned int)(body->iseq_size-1); // This assumes the end is always "leave". Is it true?
    } else {
      block_end = (unsigned int)FIX2INT(RARRAY_AREF(starts, i+1))-1;
    }

    c->blocks[start] = (struct llrb_block_info){
      .block = block,
      .phi = 0,
      .compiled = false,
      .block_end = block_end,
      .incoming_values = rb_ary_new(), // TODO: free?
      .incoming_blocks = rb_ary_new(), // TODO: free?
    };
  }
}
