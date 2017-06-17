#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "ruby.h"
#include "compiler.h"
#include "insns.inc"
#include "insns_info.inc"

static VALUE rb_eCompileError;

// Emulates rb_control_frame's sp, which is function local
struct llrb_stack {
  unsigned int size;
  unsigned int max;
  LLVMValueRef *body;
};

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

static LLVMValueRef
llvm_value(VALUE value)
{
  return LLVMConstInt(LLVMInt64Type(), value, false); // TODO: support 32bit for VALUE type
}

static void
llrb_stack_push(struct llrb_stack *stack, LLVMValueRef value)
{
  if (stack->size >= stack->max) {
    rb_raise(rb_eCompileError, "LLRB's internal stack overflow: max=%d, next size=%d", stack->max, stack->size+1);
  }
  stack->body[stack->size] = value;
  stack->size++;
}

static LLVMValueRef
llrb_stack_pop(struct llrb_stack *stack)
{
  if (stack->size <= 0) {
    rb_raise(rb_eCompileError, "LLRB's internal stack underflow: next size=%d", stack->size-1);
  }
  stack->size--;
  return stack->body[stack->size];
}

// Don't use `rb_iseq_original_iseq` to avoid unnecessary memory allocation.
int rb_vm_insn_addr2insn(const void *addr);

// Return sorted unique Ruby array of BasicBlock start positions like [0, 2, 8].
//
// It's constructed in the following rule.
//   Rule 1: 0 is always included
//   Rule 2: All TS_OFFSET numers are included
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
      case YARVINSN_jump:
      case YARVINSN_branchif:
      case YARVINSN_branchunless:
      case YARVINSN_branchnil:
      case YARVINSN_opt_case_dispatch:
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

static void
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

// Given llrb_basic_block_starts result like [0, 2, 8, 9], it returns a Hash
// whose value specifies basic block ends like { 0 => 1, 2 => 7, 8 => 8, 9 => 10 }.
//
// TODO: Note that sometimes "end" points to actual end's operands. This may hit a bug... So fix it later for safety.
static VALUE
llrb_basic_block_end_by_start(const struct rb_iseq_constant_body *body)
{
  VALUE end_by_start = rb_hash_new();
  VALUE starts = llrb_basic_block_starts(body); // TODO: free?

  for (long i = 0; i < RARRAY_LEN(starts); i++) {
    VALUE start = RARRAY_AREF(starts, i);

    if (i == RARRAY_LEN(starts)-1) {
      rb_hash_aset(end_by_start, start, INT2FIX(body->iseq_size-1)); // This assumes the end is always "leave". Is it true?
    } else {
      VALUE next_start = RARRAY_AREF(starts, i+1);
      rb_hash_aset(end_by_start, start, INT2FIX(FIX2INT(next_start)-1));
    }
  }
  return end_by_start;
}

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
        case TS_VALUE:
          if (BUILTIN_TYPE(op) == T_STRING) {
            op = rb_str_resurrect(op);
            fprintf(stderr, "%-4s ", RSTRING_PTR(rb_inspect(op)));
          } else if (BUILTIN_TYPE(op) == T_ARRAY) {
            op = rb_ary_resurrect(op);
            fprintf(stderr, "%-4s ", RSTRING_PTR(rb_inspect(op)));
          }
          break;
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
  VALUE end_by_start = llrb_basic_block_end_by_start(body); // TODO: free?
  fprintf(stderr, "\nbasic block starts: %s\n", RSTRING_PTR(rb_inspect(starts)));
  fprintf(stderr, "basic block ends by starts: %s\n\n", RSTRING_PTR(rb_inspect(end_by_start)));
}

LLVMValueRef
llrb_argument_at(struct llrb_compiler *c, unsigned index)
{
  LLVMValueRef func = LLVMGetNamedFunction(c->mod, c->funcname);
  return LLVMGetParam(func, index);
}

// In base 2, RTEST is: (v != Qfalse && v != Qnil) -> (v != 0000 && v != 1000) -> (v & 0111) != 0000 -> (v & ~Qnil) != 0
static LLVMValueRef
llrb_build_rtest(LLVMBuilderRef builder, LLVMValueRef value)
{
  LLVMValueRef masked = LLVMBuildAnd(builder, value, llvm_value(~Qnil), "RTEST_mask");
  return LLVMBuildICmp(builder, LLVMIntNE, masked, llvm_value(0), "RTEST");
}

static LLVMValueRef
llrb_get_function(LLVMModuleRef mod, const char *name)
{
  LLVMValueRef func = LLVMGetNamedFunction(mod, name);
  if (func) return func;

  if (!strcmp(name, "rb_funcall")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, true));
  } else if (!strcmp(name, "llrb_insn_checkmatch")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "rb_ary_new_from_args")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, true));
  } else if (!strcmp(name, "rb_ary_resurrect")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));
  } else if (!strcmp(name, "rb_str_resurrect")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));
  } else if (!strcmp(name, "rb_obj_as_string")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));
  } else if (!strcmp(name, "rb_hash_new")) {
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), 0, 0, false));
  } else if (!strcmp(name, "rb_hash_aset")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "rb_str_freeze")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, false));
  } else if (!strcmp(name, "rb_range_new")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "rb_ivar_get")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "rb_ivar_set")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "llrb_insn_getconstant")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "llrb_insn_setconstant")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 3, false));
  } else if (!strcmp(name, "llrb_insn_concatstrings")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 1, true));
  } else if (!strcmp(name, "llrb_insn_splatarray")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "llrb_insn_opt_plus")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "llrb_insn_opt_minus")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else if (!strcmp(name, "llrb_insn_opt_lt")) {
    LLVMTypeRef arg_types[] = { LLVMInt64Type(), LLVMInt64Type() };
    return LLVMAddFunction(mod, name, LLVMFunctionType(LLVMInt64Type(), arg_types, 2, false));
  } else {
    rb_raise(rb_eCompileError, "'%s' is not defined in llrb_get_function", name);
  }
}

static LLVMValueRef
llrb_compile_funcall(struct llrb_compiler *c, struct llrb_stack *stack, ID mid, int argc)
{
  LLVMValueRef func = llrb_get_function(c->mod, "rb_funcall");
  LLVMValueRef *args = ALLOC_N(LLVMValueRef, 3+argc); // 3 is recv, mid, n

  for (int i = argc-1; 0 <= i; i--) {
    args[3+i] = llrb_stack_pop(stack); // 3 is recv, mid, n
  }
  args[0] = llrb_stack_pop(stack);
  args[1] = llvm_value(mid);
  args[2] = LLVMConstInt(LLVMInt32Type(), argc, false);

  return LLVMBuildCall(c->builder, func, args, 3+argc, "rb_funcall");
}

static LLVMValueRef
llrb_compile_newarray(struct llrb_compiler *c, struct llrb_stack *stack, long num)
{
  LLVMValueRef *args = ALLOC_N(LLVMValueRef, num+1);
  args[0] = LLVMConstInt(LLVMInt64Type(), num, true); // TODO: support 32bit
  for (long i = num; 1 <= i; i--) {
    args[i] = llrb_stack_pop(stack);
  }

  LLVMValueRef func = llrb_get_function(c->mod, "rb_ary_new_from_args");
  return LLVMBuildCall(c->builder, func, args, num+1, "newarray");
}

static void llrb_compile_basic_block(struct llrb_compiler *c, struct llrb_stack *stack, unsigned int start);

// @return true if jumped in this insn, and in that case br won't be created.
static bool
llrb_compile_insn(struct llrb_compiler *c, struct llrb_stack *stack, const unsigned int pos, const int insn, const VALUE *operands)
{
  //fprintf(stderr, "  [DEBUG] llrb_compile_insn: %04d before %-27s (stack size: %d)\n", pos, insn_name(insn), stack->size);
  switch (insn) {
    //case YARVINSN_nop:
    //case YARVINSN_getlocal:
    //case YARVINSN_setlocal:
    //case YARVINSN_getspecial:
    //case YARVINSN_setspecial:
    //case YARVINSN_getinstancevariable:
    //case YARVINSN_setinstancevariable:
    //case YARVINSN_getclassvariable:
    //case YARVINSN_setclassvariable:
    //case YARVINSN_getconstant:
    //case YARVINSN_setconstant:
    //case YARVINSN_getglobal:
    //case YARVINSN_setglobal:
    case YARVINSN_putnil:
      llrb_stack_push(stack, llvm_value(Qnil));
      break;
    //case YARVINSN_putself:
    case YARVINSN_putobject:
      llrb_stack_push(stack, llvm_value(operands[0]));
      break;
    //case YARVINSN_putspecialobject:
    //case YARVINSN_putiseq:
    //case YARVINSN_putstring:
    //case YARVINSN_concatstrings:
    //case YARVINSN_tostring:
    //case YARVINSN_freezestring:
    //case YARVINSN_toregexp:
    case YARVINSN_newarray:
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      break;
    case YARVINSN_duparray: {
      LLVMValueRef args[] = { llvm_value(operands[0]) };
      LLVMValueRef func = llrb_get_function(c->mod, "rb_ary_resurrect");
      llrb_stack_push(stack, LLVMBuildCall(c->builder, func, args, 1, "duparray"));
      break;
    }
    //case YARVINSN_expandarray:
    //case YARVINSN_concatarray:
    //case YARVINSN_splatarray:
    //case YARVINSN_newhash:
    //case YARVINSN_newrange:
    case YARVINSN_pop:
      llrb_stack_pop(stack);
      break;
    case YARVINSN_dup: {
      LLVMValueRef value = llrb_stack_pop(stack);
      llrb_stack_push(stack, value);
      llrb_stack_push(stack, value);
      break;
    }
    //case YARVINSN_dupn:
    //case YARVINSN_swap:
    //case YARVINSN_reverse:
    //case YARVINSN_reput:
    //case YARVINSN_topn:
    //case YARVINSN_setn:
    //case YARVINSN_adjuststack:
    //case YARVINSN_defined:
    case YARVINSN_checkmatch: {
      LLVMValueRef func = llrb_get_function(c->mod, "llrb_insn_checkmatch");
      LLVMValueRef *args = ALLOC_N(LLVMValueRef, 3);
      args[1] = llrb_stack_pop(stack);
      args[0] = llrb_stack_pop(stack);
      args[2] = LLVMConstInt(LLVMInt64Type(), operands[0], false);
      llrb_stack_push(stack, LLVMBuildCall(c->builder, func, args, 3, "checkmatch"));
      break;
    }
    //case YARVINSN_checkkeyword:
    case YARVINSN_trace:
      break; // TODO: implement
    //case YARVINSN_defineclass:
    //case YARVINSN_send:
    case YARVINSN_opt_str_freeze: {
      LLVMValueRef func = llrb_get_function(c->mod, "rb_funcall");
      LLVMValueRef *args = ALLOC_N(LLVMValueRef, 3);
      args[0] = llvm_value(operands[0]);
      args[1] = llvm_value(rb_intern("freeze"));
      args[2] = LLVMConstInt(LLVMInt32Type(), 0, true);
      llrb_stack_push(stack, LLVMBuildCall(c->builder, func, args, 3, "opt_str_freeze"));
      break;
    }
    case YARVINSN_opt_newarray_max:
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("max"), 0));
      break;
    case YARVINSN_opt_newarray_min:
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("min"), 0));
      break;
    case YARVINSN_opt_send_without_block: {
      CALL_INFO ci = (CALL_INFO)operands[0];
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, ci->mid, ci->orig_argc));
      break;
    }
    //case YARVINSN_invokesuper:
    //case YARVINSN_invokeblock:
    case YARVINSN_leave:
      if (stack->size != 1) {
        llrb_disasm_insns(c->body);
        LLVMDumpModule(c->mod);
        rb_raise(rb_eCompileError, "unexpected stack size at leave: %d", stack->size);
      }
      LLVMBuildRet(c->builder, llrb_stack_pop(stack));
      return true;
    //case YARVINSN_throw:
    case YARVINSN_jump: {
      unsigned dest = pos + (unsigned)insn_len(insn) + operands[0];
      LLVMBasicBlockRef next_block = c->blocks[dest].block;

      // If stack is empty, don't create phi.
      if (stack->size == 0) {
        LLVMBuildBr(c->builder, next_block);
        llrb_compile_basic_block(c, 0, dest);
        return true;
      }

      LLVMValueRef phi = c->blocks[dest].phi;
      if (phi == 0) {
        // Push block/value for phi
        rb_ary_push(c->blocks[dest].incoming_blocks, (VALUE)LLVMGetInsertBlock(c->builder));
        rb_ary_push(c->blocks[dest].incoming_values, (VALUE)llrb_stack_pop(stack));
      } else {

        LLVMValueRef *values = ALLOC_N(LLVMValueRef, 1);
        values[0] = llrb_stack_pop(stack);
        LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
        LLVMAddIncoming(phi, values, blocks, 1);
      }

      LLVMBuildBr(c->builder, next_block);
      return true;
    }
    case YARVINSN_branchif: {
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      LLVMBasicBlockRef branch_dest_block = c->blocks[branch_dest].block;
      LLVMBasicBlockRef fallthrough_block = c->blocks[fallthrough].block;

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), branch_dest_block, fallthrough_block);

      struct llrb_stack copied_stack = (struct llrb_stack){ .size = stack->size, .max = stack->max };
      copied_stack.body = ALLOC_N(LLVMValueRef, copied_stack.max);
      for (unsigned int i = 0; i < stack->size; i++) copied_stack.body[i] = stack->body[i];

      if (copied_stack.size > 0) {
        LLVMValueRef phi = c->blocks[fallthrough].phi;
        if (phi == 0) {
          // Push block/value for phi
          rb_ary_push(c->blocks[fallthrough].incoming_blocks, (VALUE)LLVMGetInsertBlock(c->builder));
          rb_ary_push(c->blocks[fallthrough].incoming_values, (VALUE)llrb_stack_pop(&copied_stack));
        } else {
          LLVMValueRef *values = ALLOC_N(LLVMValueRef, 1);
          values[0] = llrb_stack_pop(&copied_stack);
          LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
          LLVMAddIncoming(phi, values, blocks, 1);
        }
      }

      // No stack size check?
      // If jumping back (branch_dest < pos), consider it as loop and don't create phi in that case.
      if (branch_dest > pos) {
        LLVMValueRef phi = c->blocks[branch_dest].phi;
        if (phi == 0) {
          // Push block/value for phi
          rb_ary_push(c->blocks[branch_dest].incoming_blocks, (VALUE)LLVMGetInsertBlock(c->builder));
          rb_ary_push(c->blocks[branch_dest].incoming_values, (VALUE)llrb_stack_pop(stack));
        } else {
          LLVMValueRef *values = ALLOC_N(LLVMValueRef, 1);
          values[0] = llrb_stack_pop(stack);
          LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
          LLVMAddIncoming(phi, values, blocks, 1);
        }
      }

      llrb_compile_basic_block(c, &copied_stack, fallthrough);
      llrb_compile_basic_block(c, stack, branch_dest);
      return true;
    }
    case YARVINSN_branchunless: {
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      LLVMBasicBlockRef branch_dest_block = c->blocks[branch_dest].block;
      LLVMBasicBlockRef fallthrough_block = c->blocks[fallthrough].block;

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), fallthrough_block, branch_dest_block);

      struct llrb_stack copied_stack = (struct llrb_stack){ .size = stack->size, .max = stack->max };
      copied_stack.body = ALLOC_N(LLVMValueRef, copied_stack.max);
      for (unsigned int i = 0; i < stack->size; i++) copied_stack.body[i] = stack->body[i];

      llrb_compile_basic_block(c, &copied_stack, fallthrough); // COMPILE FALLTHROUGH FIRST!!!!
      llrb_compile_basic_block(c, stack, branch_dest); // because this line will continue to compile next block and it should wait the other branch.
      return true;
    }
    case YARVINSN_branchnil: {
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      LLVMBasicBlockRef branch_dest_block = c->blocks[branch_dest].block;
      LLVMBasicBlockRef fallthrough_block = c->blocks[fallthrough].block;

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder,
          LLVMBuildICmp(c->builder, LLVMIntNE, cond, llvm_value(Qnil), "NIL_P"),
          fallthrough_block, branch_dest_block);

      LLVMValueRef phi = c->blocks[branch_dest].phi;
      if (phi == 0) {
        // Push block/value for phi
        rb_ary_push(c->blocks[branch_dest].incoming_blocks, (VALUE)LLVMGetInsertBlock(c->builder));
        rb_ary_push(c->blocks[branch_dest].incoming_values, (VALUE)llvm_value(Qnil));
      } else {
        LLVMValueRef values[] = { llvm_value(Qnil) };
        LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
        LLVMAddIncoming(phi, values, blocks, 1);
      }

      //fprintf(stderr, "[DEBUG] branchnil llrb_compile_basic_block by %04d after %s (stack size: %d)\n", pos, insn_name(insn), stack->size);
      llrb_compile_basic_block(c, stack, fallthrough);
      return true;
    }
    //case YARVINSN_getinlinecache:
    //case YARVINSN_setinlinecache:
    //case YARVINSN_once:
    case YARVINSN_opt_case_dispatch: // Use `switch` instruction
      llrb_stack_pop(stack); // TODO: implement
      break;
    case YARVINSN_opt_plus:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '+', 1));
      break;
    case YARVINSN_opt_minus:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '-', 1));
      break;
    case YARVINSN_opt_mult:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '*', 1));
      break;
    case YARVINSN_opt_div:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '/', 1));
      break;
    case YARVINSN_opt_mod:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '%', 1));
      break;
    case YARVINSN_opt_eq:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("=="), 1));
      break;
    //case YARVINSN_opt_neq:
    //case YARVINSN_opt_lt:
    //case YARVINSN_opt_le:
    //case YARVINSN_opt_gt:
    //case YARVINSN_opt_ge:
    //case YARVINSN_opt_ltlt:
    //case YARVINSN_opt_aref:
    //case YARVINSN_opt_aset:
    //case YARVINSN_opt_aset_with:
    //case YARVINSN_opt_aref_with:
    //case YARVINSN_opt_length:
    //case YARVINSN_opt_size:
    //case YARVINSN_opt_empty_p:
    //case YARVINSN_opt_succ:
    //case YARVINSN_opt_not:
    //case YARVINSN_opt_regexpmatch1:
    //case YARVINSN_opt_regexpmatch2:
    //case YARVINSN_opt_call_c_function:
    case YARVINSN_getlocal_OP__WC__0: {
      unsigned local_size = c->body->local_table_size;
      unsigned arg_size   = c->body->param.size;
      llrb_stack_push(stack, llrb_argument_at(c, 3 - local_size + 2 * arg_size - (unsigned)operands[0])); // XXX: is this okay????
      break;
    }
    //case YARVINSN_getlocal_OP__WC__1:
    //case YARVINSN_setlocal_OP__WC__0:
    //case YARVINSN_setlocal_OP__WC__1:
    case YARVINSN_putobject_OP_INT2FIX_O_0_C_:
      llrb_stack_push(stack, llvm_value(INT2FIX(0)));
      break;
    case YARVINSN_putobject_OP_INT2FIX_O_1_C_:
      llrb_stack_push(stack, llvm_value(INT2FIX(1)));
      break;
    default:
      llrb_disasm_insns(c->body);
      rb_raise(rb_eCompileError, "Unhandled insn at llrb_compile_insn: %s", insn_name(insn));
      break;
  }
  //fprintf(stderr, "  [DEBUG] llrb_compile_insn: %04d after %-27s (stack size: %d)\n", pos, insn_name(insn), stack->size);
  return false;
}

static void
llrb_compile_basic_block(struct llrb_compiler *c, struct llrb_stack *stack, unsigned int start)
{
  // Avoid compiling multiple times
  if (c->blocks[start].compiled) return;
  c->blocks[start].compiled = true;

  LLVMBasicBlockRef block = c->blocks[start].block;
  LLVMPositionBuilderAtEnd(c->builder, block);

  // Use previous stack or create new one
  if (stack == 0) {
    stack = ALLOC_N(struct llrb_stack, 1);
    stack->body = ALLOC_N(LLVMValueRef, c->body->stack_max);
    stack->size = 0;
    stack->max  = c->body->stack_max;
  }

  // If incoming blocks and values exist, build phi node and push it to stack.
  VALUE incoming_values = c->blocks[start].incoming_values;
  VALUE incoming_blocks = c->blocks[start].incoming_blocks;
  long len;
  if ((len = RARRAY_LEN(incoming_values)) > 0 && len == RARRAY_LEN(incoming_blocks)) {
    LLVMValueRef *values = ALLOC_N(LLVMValueRef, len);
    for (long i = 0; i < len; i++) {
      values[i] = (LLVMValueRef)RARRAY_AREF(incoming_values, i);
    }

    LLVMBasicBlockRef *blocks = ALLOC_N(LLVMBasicBlockRef, len);
    for (long i = 0; i < len; i++) {
      blocks[i] = (LLVMBasicBlockRef)RARRAY_AREF(incoming_blocks, i);
    }

    LLVMValueRef phi = LLVMBuildPhi(c->builder, LLVMInt64Type(), "llrb_compile_basic_block");
    LLVMAddIncoming(phi, values, blocks, len);
    llrb_stack_push(stack, phi);
    c->blocks[start].phi = phi;
  }

  // Compile instructions in this basic block
  unsigned int block_end = (unsigned int)c->blocks[start].block_end;
  bool jumped;
  unsigned int pos;
  for (pos = start; pos <= block_end;) {
    int insn = rb_vm_insn_addr2insn((void *)c->body->iseq_encoded[pos]);
    jumped = llrb_compile_insn(c, stack, pos, insn, c->body->iseq_encoded + (pos+1));
    pos += insn_len(insn);
  }

  // After reached block end, if not jumped and next block exists, create br to next block.
  if (!jumped && pos < c->body->iseq_size) {
    LLVMBasicBlockRef next_block = c->blocks[pos].block;
    // Create phi only when stack has value.
    if (stack->size > 0) {

      LLVMValueRef phi = c->blocks[pos].phi;
      if (phi == 0) {
        // Push block/value for phi
        rb_ary_push(c->blocks[pos].incoming_blocks, (VALUE)block);
        rb_ary_push(c->blocks[pos].incoming_values, (VALUE)llrb_stack_pop(stack));
      } else {
        LLVMValueRef values[] = { llrb_stack_pop(stack) };
        LLVMBasicBlockRef blocks[] = { block };
        LLVMAddIncoming(phi, values, blocks, 1);
      }
    }

    LLVMBuildBr(c->builder, next_block);
    llrb_compile_basic_block(c, stack, pos);
  }
}

LLVMModuleRef
llrb_compile_iseq(const rb_iseq_t *iseq, const char* funcname)
{
  LLVMModuleRef mod = LLVMModuleCreateWithName("llrb");

  LLVMTypeRef *arg_types = ALLOC_N(LLVMTypeRef, iseq->body->param.size+1);
  for (unsigned i = 0; i <= iseq->body->param.size; i++) arg_types[i] = LLVMInt64Type();
  LLVMValueRef func = LLVMAddFunction(mod, funcname,
      LLVMFunctionType(LLVMInt64Type(), arg_types, iseq->body->param.size+1, false));

  struct llrb_compiler compiler = (struct llrb_compiler){
    .body = iseq->body,
    .funcname = funcname,
    .builder = LLVMCreateBuilder(),
    .mod = mod,
    // In each iseq insn index, it stores `struct llrb_block_info`. For easy implementation (it stores llrb_block_info in the index of insn),
    // it allocates the number of `iseq->body->iseq_size`. So it uses unnecessary memory. Fix it later.
    .blocks = ALLOC_N(struct llrb_block_info, iseq->body->iseq_size)
  };
  llrb_init_basic_blocks(&compiler, iseq->body, func);

  //llrb_disasm_insns(iseq->body);
  llrb_compile_basic_block(&compiler, 0, 0);
  //LLVMDumpModule(mod);
  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
