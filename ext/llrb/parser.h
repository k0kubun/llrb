#ifndef LLRB_PARSER_H
#define LLRB_PARSER_H

#include <stdbool.h>
#include "llvm-c/Core.h"
#include "cruby.h"
#include "insns.inc"
#include "insns_info.inc"

// Store metadata of compiled basic blocks
struct llrb_block_info {
  LLVMBasicBlockRef block;
  LLVMValueRef phi;
  unsigned int block_end;
  bool compiled;

  // TODO: use proper container for following fields
  VALUE incoming_values;
  VALUE incoming_blocks;
};

// Store compiler's internal state and shared variables
struct llrb_compiler {
  const struct rb_iseq_constant_body *body;
  const char *funcname;
  LLVMBuilderRef builder;
  LLVMModuleRef mod;
  struct llrb_block_info *blocks;
};

VALUE llrb_basic_block_starts(const struct rb_iseq_constant_body *body);

void llrb_init_basic_blocks(struct llrb_compiler *c, const struct rb_iseq_constant_body *body, LLVMValueRef func);

// Not using `rb_iseq_original_iseq` to avoid unnecessary memory allocation.
int rb_vm_insn_addr2insn(const void *addr);

// This is defined in header to suppress unused function error for insn_op_type, insn_op_types, insn_name.
static void
llrb_disasm_insns(const struct rb_iseq_constant_body *body)
{
  fprintf(stderr, "\n== disasm: LLRB ================================");
  VALUE starts = llrb_basic_block_starts(body); // TODO: free?
  for (unsigned int i = 0; i < body->iseq_size;) {
    if (RTEST(rb_ary_includes(starts, INT2FIX(i)))) {
      fprintf(stderr, "\n");
    }

    int insn = rb_vm_insn_addr2insn((void *)body->iseq_encoded[i]);
    fprintf(stderr, "%04d %-27s [%-4s] ", i, insn_name(insn), insn_op_types(insn));

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
  fprintf(stderr, "\nbasic block starts: %s\n", RSTRING_PTR(rb_inspect(starts)));
}

#endif // LLRB_PARSER_H
