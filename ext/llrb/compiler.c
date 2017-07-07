/*
 * compiler.c: Compiles encoded YARV instructions structured as Control Flow Graph to LLVM IR.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"
#include "cfg.h"
#include "cruby.h"

static VALUE rb_eCompileError;
#include "compiler/funcs.h"
#include "compiler/stack.h"

// Store compiler's internal state and shared variables
struct llrb_compiler {
  const struct rb_iseq_constant_body *body;
  const VALUE *new_iseq_encoded; // program counter's base address after this compilation.
  struct llrb_cfg *cfg;
  LLVMValueRef func;
  LLVMBuilderRef builder;
  LLVMModuleRef mod;
};

static inline LLVMValueRef
llrb_value(VALUE value)
{
  return LLVMConstInt(LLVMInt64Type(), value, false); // TODO: support 32bit for VALUE type
}

static inline LLVMValueRef
llrb_get_thread(const struct llrb_compiler *c)
{
  return LLVMGetParam(c->func, 0);
}

static inline LLVMValueRef
llrb_get_cfp(const struct llrb_compiler *c)
{
  return LLVMGetParam(c->func, 1);
}

// In base 2, RTEST is: (v != Qfalse && v != Qnil) -> (v != 0000 && v != 1000) -> (v & 0111) != 0000 -> (v & ~Qnil) != 0
static LLVMValueRef
llrb_build_rtest(LLVMBuilderRef builder, LLVMValueRef value)
{
  LLVMValueRef masked = LLVMBuildAnd(builder, value, llrb_value(~Qnil), "RTEST_mask");
  return LLVMBuildICmp(builder, LLVMIntNE, masked, llrb_value(0), "RTEST");
}

static LLVMValueRef
llrb_call_func(const struct llrb_compiler *c, const char *funcname, unsigned argc, ...)
{
  LLVMValueRef *args = ALLOC_N(LLVMValueRef, argc); // `xfree`d in the end of this function.

  va_list ar;
  va_start(ar, argc);
  for (unsigned i = 0; i < argc; i++) {
    args[i] = va_arg(ar, LLVMValueRef);
  }
  va_end(ar);

  LLVMValueRef ret = LLVMBuildCall(c->builder, llrb_get_function(c->mod, funcname), args, argc, "");
  xfree(args);
  return ret;
}

static inline LLVMValueRef
llrb_get_self(const struct llrb_compiler *c)
{
  return llrb_call_func(c, "llrb_self_from_cfp", 1, llrb_get_cfp(c));
}

static LLVMValueRef
llrb_compile_funcall(const struct llrb_compiler *c, struct llrb_stack *stack, ID mid, int argc)
{
  LLVMValueRef func = llrb_get_function(c->mod, "rb_funcall");
  LLVMValueRef *args = ALLOC_N(LLVMValueRef, 3+argc); // 3 is recv, mid, n

  for (int i = argc-1; 0 <= i; i--) {
    args[3+i] = llrb_stack_pop(stack); // 3 is recv, mid, n
  }
  args[0] = llrb_stack_pop(stack);
  args[1] = llrb_value(mid);
  args[2] = LLVMConstInt(LLVMInt32Type(), argc, false);

  return LLVMBuildCall(c->builder, func, args, 3+argc, "rb_funcall");
}

// Must call `llrb_destruct_stack` after usage of return value.
// TODO: Using `memcpy` would be faster.
static struct llrb_stack *
llrb_copy_stack(const struct llrb_stack *stack)
{
  struct llrb_stack *ret = ALLOC_N(struct llrb_stack, 1); // `xfree`d by `llrb_destruct_stack`.
  ret->size = stack->size;
  ret->max  = stack->max;
  ret->body = ALLOC_N(LLVMValueRef, ret->max); // `xfree`d by `llrb_destruct_stack`.
  for (unsigned int i = 0; i < stack->size; i++) {
    ret->body[i] = stack->body[i];
  }
  return ret;
}

static void
llrb_destruct_stack(struct llrb_stack *stack)
{
  xfree(stack->body);
  xfree(stack);
}

static struct llrb_basic_block *
llrb_find_block(struct llrb_cfg *cfg, unsigned int start)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    if (block->start == start) return block;
  }
  rb_raise(rb_eCompileError, "BasicBlock (start = %d) was not found in llrb_find_block", start);
}

static void
llrb_push_incoming_things(const struct llrb_compiler *c, struct llrb_basic_block *block, LLVMBasicBlockRef current_ref, LLVMValueRef incoming_value)
{
  // Ensure phi node existence here.
  if (block->phi == 0) {
    // Phi node should be created before its basic block's compilation...
    if (block->compiled) {
      rb_raise(rb_eCompileError, "Already compiled BasicBlock (start = %d) didn't have phi and was requested to push incoming value", block->start);
    }
    LLVMPositionBuilderAtEnd(c->builder, block->ref);
    block->phi = LLVMBuildPhi(c->builder, LLVMInt64Type(), ""); // TODO: Support 32bit
    LLVMPositionBuilderAtEnd(c->builder, current_ref);
  }

  LLVMValueRef values[] = { incoming_value };
  LLVMBasicBlockRef blocks[] = { current_ref };
  LLVMAddIncoming(block->phi, values, blocks, 1);
}

static LLVMValueRef
llrb_compile_newarray(const struct llrb_compiler *c, struct llrb_stack *stack, long num)
{
  LLVMValueRef *args = ALLOC_N(LLVMValueRef, num+1); // `xfree`d in the end of this function.
  args[0] = LLVMConstInt(LLVMInt64Type(), num, true); // TODO: support 32bit
  for (long i = num; 1 <= i; i--) {
    args[i] = llrb_stack_pop(stack);
  }

  LLVMValueRef func = llrb_get_function(c->mod, "rb_ary_new_from_args");
  LLVMValueRef ret = LLVMBuildCall(c->builder, func, args, num+1, "newarray");
  xfree(args);
  return ret;
}

// If insn can call any method, it is throwable and needs to change program counter. Or it may rb_raise.
static bool
llrb_pc_change_required(const int insn)
{
  switch (insn) {
    case YARVINSN_tostring:
    case YARVINSN_freezestring:
    case YARVINSN_checkmatch:
    case YARVINSN_send:
    case YARVINSN_opt_str_freeze:
    case YARVINSN_opt_newarray_max:
    case YARVINSN_opt_newarray_min:
    case YARVINSN_opt_send_without_block:
    case YARVINSN_invokesuper:
    case YARVINSN_invokeblock:
    case YARVINSN_leave:
    case YARVINSN_throw:
    case YARVINSN_opt_plus:
    case YARVINSN_opt_minus:
    case YARVINSN_opt_mult:
    case YARVINSN_opt_div:
    case YARVINSN_opt_mod:
    case YARVINSN_opt_eq:
    case YARVINSN_opt_neq:
    case YARVINSN_opt_lt:
    case YARVINSN_opt_le:
    case YARVINSN_opt_gt:
    case YARVINSN_opt_ge:
    case YARVINSN_opt_ltlt:
    case YARVINSN_opt_aref:
    case YARVINSN_opt_aset:
    case YARVINSN_opt_aset_with:
    case YARVINSN_opt_aref_with:
    case YARVINSN_opt_length:
    case YARVINSN_opt_size:
    case YARVINSN_opt_empty_p:
    case YARVINSN_opt_succ:
    case YARVINSN_opt_not:
    case YARVINSN_opt_regexpmatch1:
    case YARVINSN_opt_regexpmatch2:
    case YARVINSN_opt_call_c_function:
      return true;
    default:
      return false;
  }
}

// Catch table checks program counter to decide catch it or not. So we need to set program counter before method call or throw insn.
static void
llrb_increment_pc(const struct llrb_compiler *c, const unsigned int pos, const int insn)
{
  if (pos == 0) return; // Skip. 0 would be opt_call_c_function and there's no need to change.

  if (llrb_pc_change_required(insn)) {
    // This case should be rejected to compile by `llrb_check_not_compilable`.
    if (pos == 1) rb_raise(rb_eCompileError, "program counter is set to 1 from iseq_encoded");

    // `pos` MUST NOT be 0 (it causes stack level too deep) or 1 (funcptr is considered as insn and it's invalid as insn).
    // TODO: It should be checked before compilation!
    const VALUE *pc = c->new_iseq_encoded + pos; // This must be `new_iseq_encoded` to get proper `epc` in `vm_exec`.
    llrb_call_func(c, "llrb_set_pc", 2, llrb_get_cfp(c), llrb_value((VALUE)pc));
  }
}

static void llrb_compile_basic_block(const struct llrb_compiler *c, struct llrb_basic_block *block, struct llrb_stack *stack);

// @param created_br is set true if conditional branch is created. In that case, br for next block isn't created in `llrb_compile_basic_block`.
// @return true if the IR compiled from given insn includes `ret` instruction. In that case, next block won't be compiled in `llrb_compile_basic_block`.
static bool
llrb_compile_insn(const struct llrb_compiler *c, struct llrb_stack *stack, const unsigned int pos, const int insn, const VALUE *operands, bool *created_br)
{
  llrb_increment_pc(c, pos, insn);

  //fprintf(stderr, "  [DEBUG] llrb_compile_insn: %04d before %-27s (stack size: %d)\n", pos, insn_name(insn), stack->size);
  *created_br = false;
  switch (insn) {
    case YARVINSN_nop:
      break; // nop
    case YARVINSN_getlocal: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_getlocal", 3, llrb_get_cfp(c),
            llrb_value((lindex_t)operands[0]), llrb_value((rb_num_t)operands[1])));
      break;
    }
    case YARVINSN_setlocal: {
      llrb_call_func(c, "llrb_insn_setlocal", 4, llrb_get_cfp(c),
          llrb_value((lindex_t)operands[0]), llrb_value((rb_num_t)operands[1]), llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_getspecial: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_getspecial", 2, llrb_value(operands[0]), llrb_value(operands[1])));
      break;
    }
    case YARVINSN_setspecial: {
      llrb_call_func(c, "llrb_insn_setspecial", 2, llrb_value(operands[0]), llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_getinstancevariable: { // TODO: implement inline cache counterpart
      llrb_stack_push(stack, llrb_call_func(c, "rb_ivar_get", 2, llrb_get_self(c), llrb_value(operands[0])));
      break;
    }
    case YARVINSN_setinstancevariable: { // TODO: implement inline cache counterpart
      llrb_call_func(c, "rb_ivar_set", 3, llrb_get_self(c), llrb_value(operands[0]), llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_getclassvariable: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_getclassvariable", 2, llrb_get_cfp(c), llrb_value(operands[0])));
      break;
    }
    case YARVINSN_setclassvariable: {
      llrb_call_func(c, "llrb_insn_setclassvariable", 3, llrb_get_cfp(c), llrb_value(operands[0]), llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_getconstant: {
      llrb_stack_push(stack, llrb_call_func(c, "vm_get_ev_const", 4, llrb_get_thread(c),
            llrb_stack_pop(stack), llrb_value(operands[0]), LLVMConstInt(LLVMInt32Type(), 0, true)));
      break;
    }
    case YARVINSN_setconstant: {
      LLVMValueRef cbase = llrb_stack_pop(stack);
      LLVMValueRef args[] = { llrb_get_self(c), cbase, llrb_value(operands[0]), llrb_stack_pop(stack) };
      LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_setconstant"), args, 4, "");
      break;
    }
    case YARVINSN_getglobal: {
      llrb_stack_push(stack, llrb_call_func(c, "rb_gvar_get", 1, llrb_value(operands[0])));
      break;
    }
    case YARVINSN_setglobal: {
      llrb_call_func(c, "rb_gvar_set", 2, llrb_value(operands[0]), llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_putnil:
      llrb_stack_push(stack, llrb_value(Qnil));
      break;
    case YARVINSN_putself: {
      llrb_stack_push(stack, llrb_get_self(c));
      break;
    }
    case YARVINSN_putobject:
      llrb_stack_push(stack, llrb_value(operands[0]));
      break;
    case YARVINSN_putspecialobject: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_putspecialobject", 1, llrb_value(operands[0])));
      break;
    }
    case YARVINSN_putiseq:
      llrb_stack_push(stack, llrb_value(operands[0]));
      break;
    case YARVINSN_putstring: {
      llrb_stack_push(stack, llrb_call_func(c, "rb_str_resurrect", 1, llrb_value(operands[0])));
      break;
    }
    case YARVINSN_concatstrings: {
      LLVMValueRef *args = ALLOC_N(LLVMValueRef, operands[0] + 1); // `xfree`d in this block.
      args[0] = llrb_value(operands[0]); // function is in size_t. correct?
      for (long i = (long)operands[0]-1; 0 <= i; i--) {
        args[1+i] = llrb_stack_pop(stack);
      }
      llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_concatstrings"), args, operands[0] + 1, "concatstrings"));
      xfree(args);
      break;
    }
    case YARVINSN_tostring: {
      llrb_stack_push(stack, llrb_call_func(c, "rb_obj_as_string", 1, llrb_stack_pop(stack)));
      break;
    }
    case YARVINSN_freezestring: { // TODO: check debug info
      llrb_stack_push(stack, llrb_call_func(c, "rb_str_freeze", 1, llrb_stack_pop(stack))); // TODO: inline
      break;
    }
    case YARVINSN_toregexp: {
      rb_num_t cnt = operands[1];
      LLVMValueRef *args1 = ALLOC_N(LLVMValueRef, cnt+1); // `xfree`d in this block.
      args1[0] = LLVMConstInt(LLVMInt64Type(), (long)cnt, true);
      for (rb_num_t i = 0; i < cnt; i++) {
        args1[1+i] = llrb_stack_pop(stack);
      }
      LLVMValueRef ary = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ary_new_from_args"), args1, 1+cnt, "toregexp");
      xfree(args1);

      llrb_stack_push(stack, llrb_call_func(c, "rb_reg_new_ary", 2, ary, LLVMConstInt(LLVMInt32Type(), (int)operands[0], true)));

      llrb_call_func(c, "rb_ary_clear", 1, ary);
      break;
    }
    case YARVINSN_newarray:
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      break;
    case YARVINSN_duparray: {
      llrb_stack_push(stack, llrb_call_func(c, "rb_ary_resurrect", 1, llrb_value(operands[0]))); // TODO: inline rb_ary_resurrect?
      break;
    }
    //case YARVINSN_expandarray: {
    //  rb_num_t flag = (rb_num_t)operands[1];
    //  if (flag & 0x02) { // for postarg
    //  } else {
    //  }
    //  break;
    //}
    case YARVINSN_concatarray: {
      LLVMValueRef ary2st = llrb_stack_pop(stack);
      LLVMValueRef ary1   = llrb_stack_pop(stack);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_concatarray", 2, ary1, ary2st));
      break;
    }
    case YARVINSN_splatarray: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_splatarray", 2, llrb_stack_pop(stack), llrb_value(operands[0])));
      break;
    }
    case YARVINSN_newhash: {
      LLVMValueRef *values = ALLOC_N(LLVMValueRef, operands[0] / 2);
      LLVMValueRef *keys   = ALLOC_N(LLVMValueRef, operands[0] / 2);
      for (int i = 0; i < (int)operands[0] / 2; i++) {
        values[i] = llrb_stack_pop(stack);
        keys[i]   = llrb_stack_pop(stack);
      }

      LLVMValueRef result = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_hash_new"), 0, 0, "newhash");
      // reverse set
      for (int i = (int)operands[0] / 2 - 1; 0 <= i; i--) {
        LLVMValueRef args[] = { result, keys[i], values[i] };
        LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_hash_aset"), args, 3, "newhash_aset");
      }
      llrb_stack_push(stack, result);
      break;
    }
    case YARVINSN_newrange: {
      LLVMValueRef high = llrb_stack_pop(stack);
      LLVMValueRef low  = llrb_stack_pop(stack);
      LLVMValueRef flag = LLVMConstInt(LLVMInt64Type(), operands[0], false);
      llrb_stack_push(stack, llrb_call_func(c, "rb_range_new", 3, low, high, flag));
      break;
    }
    case YARVINSN_pop:
      llrb_stack_pop(stack);
      break;
    case YARVINSN_dup: {
      LLVMValueRef value = llrb_stack_pop(stack);
      llrb_stack_push(stack, value);
      llrb_stack_push(stack, value);
      break;
    }
    case YARVINSN_dupn: {
      LLVMValueRef *values = ALLOC_N(LLVMValueRef, operands[0]); // `xfree`d in this block.
      for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
        values[i] = llrb_stack_pop(stack); // FIXME: obviously no need to pop
      }

      for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
        llrb_stack_push(stack, values[operands[0] - 1 - i]);
      }
      for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
        llrb_stack_push(stack, values[operands[0] - 1 - i]);
      }
      xfree(values);
      break;
    }
    case YARVINSN_swap: {
      LLVMValueRef first  = llrb_stack_pop(stack);
      LLVMValueRef second = llrb_stack_pop(stack);
      llrb_stack_push(stack, first);
      llrb_stack_push(stack, second);
      break;
    }
    //case YARVINSN_reverse: {
    //  rb_num_t n = (rb_num_t)operands[0];
    //  unsigned int last = stack->size - 1;
    //  unsigned int top_i = last - n;

    //  for (rb_num_t i = 0; i < n/2; i++) {
    //    LLVMValueRef v0 = stack->body[top_i+i];
    //    LLVMValueRef v1 = stack->body[last-i];
    //    stack->body[top_i+i] = v1;
    //    stack->body[last-i]  = v0;
    //  }
    //  break;
    //}
    //case YARVINSN_reput:
    //  break; // none
    case YARVINSN_topn: {
      llrb_stack_push(stack, llrb_stack_topn(stack, (unsigned int)operands[0]));
      break;
    }
    case YARVINSN_setn: {
      rb_num_t last = (rb_num_t)stack->size - 1;
      stack->body[last - (rb_num_t)operands[0]] = stack->body[last];
      break;
    }
    case YARVINSN_adjuststack: {
      for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
        llrb_stack_pop(stack);
      }
      break;
    }
    case YARVINSN_defined: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_defined", 4, llrb_value(operands[0]),
            llrb_value(operands[1]), llrb_value(operands[2]), llrb_stack_pop(stack)));
      break;
    }
    case YARVINSN_checkmatch: {
      LLVMValueRef pattern = llrb_stack_pop(stack);
      LLVMValueRef target =llrb_stack_pop(stack);
      LLVMValueRef flag = LLVMConstInt(LLVMInt64Type(), operands[0], false);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_checkmatch", 3, target, pattern, flag));
      break;
    }
    case YARVINSN_checkkeyword: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_checkkeyword", 3, llrb_get_cfp(c),
            llrb_value((lindex_t)operands[0]), llrb_value((rb_num_t)operands[1])));
      break;
    }
    case YARVINSN_trace: {
      rb_event_flag_t flag = (rb_event_flag_t)((rb_num_t)operands[0]);
      LLVMValueRef val = (flag & (RUBY_EVENT_RETURN | RUBY_EVENT_B_RETURN)) ? stack->body[stack->size-1] : llrb_value(Qundef);
      llrb_call_func(c, "llrb_insn_trace", 4, llrb_get_thread(c), llrb_get_cfp(c), LLVMConstInt(LLVMInt32Type(), flag, false), val);
      break;
    }
    //case YARVINSN_defineclass: {
    //  ;
    //  break;
    //}
    case YARVINSN_send: {
      CALL_INFO ci = (CALL_INFO)operands[0];
      unsigned int stack_size = ci->orig_argc + 1;
      if (ci->flag & VM_CALL_ARGS_BLOCKARG) stack_size++; // push `&block`

      unsigned int arg_size = 6 + stack_size;
      LLVMValueRef *args = ALLOC_N(LLVMValueRef, arg_size);
      args[0] = llrb_get_thread(c);
      args[1] = llrb_get_cfp(c);
      args[2] = llrb_value((VALUE)ci);
      args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
      args[4] = llrb_value((VALUE)((ISEQ)operands[2]));
      args[5] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
      for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
        args[6 + i] = llrb_stack_pop(stack);
      }

      llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_send"), args, arg_size, "send"));
      break;
    }
    case YARVINSN_opt_str_freeze: { // TODO: optimize
      llrb_stack_push(stack, llrb_call_func(c, "rb_funcall", 3, llrb_value(operands[0]),
            llrb_value(rb_intern("freeze")), LLVMConstInt(LLVMInt32Type(), 0, true)));
      break;
    }
    case YARVINSN_opt_newarray_max: // TODO: optimize
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("max"), 0));
      break;
    case YARVINSN_opt_newarray_min: // TODO: optimize
      llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("min"), 0));
      break;
    case YARVINSN_opt_send_without_block: {
      CALL_INFO ci = (CALL_INFO)operands[0];
      unsigned int stack_size = ci->orig_argc + 1;

      LLVMValueRef *args = ALLOC_N(LLVMValueRef, 5 + stack_size); // `xfree`d in this block.
      args[0] = llrb_get_thread(c);
      args[1] = llrb_get_cfp(c);
      args[2] = llrb_value((VALUE)ci);
      args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
      args[4] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
      for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
        args[5 + i] = llrb_stack_pop(stack);
      }

      LLVMValueRef func = llrb_get_function(c->mod, "llrb_insn_opt_send_without_block");
      llrb_stack_push(stack, LLVMBuildCall(c->builder, func, args, 5 + stack_size, "opt_send_without_block"));
      xfree(args);
      break;
    }
    case YARVINSN_invokesuper: { // TODO: refactor with opt_send_without_block
      CALL_INFO ci = (CALL_INFO)operands[0];
      unsigned int stack_size = ci->orig_argc + 1;

      LLVMValueRef *args = ALLOC_N(LLVMValueRef, 5 + stack_size);
      args[0] = llrb_get_thread(c);
      args[1] = llrb_get_cfp(c);
      args[2] = llrb_value((VALUE)ci);
      args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
      args[4] = llrb_value((VALUE)((ISEQ)operands[2]));
      args[5] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
      for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
        args[6 + i] = llrb_stack_pop(stack);
      }
      llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_invokesuper"), args, 6 + stack_size, "invokesuper"));
      break;
    }
    case YARVINSN_invokeblock: {
      CALL_INFO ci = (CALL_INFO)operands[0];
      unsigned int stack_size = ci->orig_argc;

      LLVMValueRef *args = ALLOC_N(LLVMValueRef, 4 + stack_size);
      args[0] = llrb_get_thread(c);
      args[1] = llrb_get_cfp(c);
      args[2] = llrb_value((VALUE)ci);
      args[3] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
      for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
        args[4 + i] = llrb_stack_pop(stack);
      }
      llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_invokeblock"), args, 4 + stack_size, "invokeblock"));
      break;
    }
    case YARVINSN_leave:
      if (stack->size != 1) {
        llrb_dump_cfg(c->body, c->cfg);
        rb_raise(rb_eCompileError, "unexpected stack size at leave: %d", stack->size);
      }

      llrb_call_func(c, "llrb_push_result", 2, llrb_get_cfp(c), llrb_stack_pop(stack));
      LLVMBuildRet(c->builder, llrb_get_cfp(c));
      return true;
    case YARVINSN_throw: {
      llrb_call_func(c, "llrb_insn_throw", 4, llrb_get_thread(c), llrb_get_cfp(c),
          llrb_value((rb_num_t)operands[0]), llrb_stack_pop(stack));

      // In opt_call_c_function, if we return 0, we can throw error fron th->errinfo.
      // https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2147-L2151
      LLVMBuildRet(c->builder, llrb_value(0));
      return true;
    }
    case YARVINSN_jump: {
      unsigned dest = pos + (unsigned)insn_len(insn) + operands[0];
      struct llrb_basic_block *next_block = llrb_find_block(c->cfg, dest);

      LLVMBuildBr(c->builder, next_block->ref);
      *created_br = true;

      if (next_block->incoming_size > 1 && stack->size > 0) {
        llrb_push_incoming_things(c, next_block, LLVMGetInsertBlock(c->builder), llrb_stack_pop(stack));
      }
      llrb_compile_basic_block(c, next_block, stack);
      return true;
    }
    case YARVINSN_branchif: { // TODO: refactor with other branch insns
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      struct llrb_basic_block *branch_dest_block = llrb_find_block(c->cfg, branch_dest);
      struct llrb_basic_block *fallthrough_block = llrb_find_block(c->cfg, fallthrough);

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), branch_dest_block->ref, fallthrough_block->ref);
      *created_br = true;

      struct llrb_stack *branch_dest_stack = llrb_copy_stack(stack); // `llrb_destruct_stack`ed in this block.
      if (branch_dest_block->incoming_size > 1 && branch_dest_stack->size > 0) {
        llrb_push_incoming_things(c, branch_dest_block,
            LLVMGetInsertBlock(c->builder), llrb_stack_pop(branch_dest_stack));
      }
      llrb_compile_basic_block(c, branch_dest_block, branch_dest_stack);
      llrb_destruct_stack(branch_dest_stack);
      break; // caller `compile_basic_block` compiles fallthrough_block and pushes incoming things to its phi node.
    }
    case YARVINSN_branchunless: { // TODO: refactor with other branch insns
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      struct llrb_basic_block *branch_dest_block = llrb_find_block(c->cfg, branch_dest);
      struct llrb_basic_block *fallthrough_block = llrb_find_block(c->cfg, fallthrough);

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), fallthrough_block->ref, branch_dest_block->ref);
      *created_br = true;

      struct llrb_stack *branch_dest_stack = llrb_copy_stack(stack); // `llrb_destruct_stack`ed in this block.
      if (branch_dest_block->incoming_size > 1 && branch_dest_stack->size > 0) {
        llrb_push_incoming_things(c, branch_dest_block,
            LLVMGetInsertBlock(c->builder), llrb_stack_pop(branch_dest_stack));
      }
      llrb_compile_basic_block(c, branch_dest_block, branch_dest_stack);
      llrb_destruct_stack(branch_dest_stack);
      break; // caller `compile_basic_block` compiles fallthrough_block and pushes incoming things to its phi node.
    }
    case YARVINSN_branchnil: { // TODO: refactor with other branch insns
      unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
      unsigned fallthrough = pos + (unsigned)insn_len(insn);
      struct llrb_basic_block *branch_dest_block = llrb_find_block(c->cfg, branch_dest);
      struct llrb_basic_block *fallthrough_block = llrb_find_block(c->cfg, fallthrough);

      LLVMValueRef cond = llrb_stack_pop(stack);
      LLVMBuildCondBr(c->builder,
          LLVMBuildICmp(c->builder, LLVMIntNE, cond, llrb_value(Qnil), "NIL_P"),
          fallthrough_block->ref, branch_dest_block->ref);
      *created_br = true;

      struct llrb_stack *branch_dest_stack = llrb_copy_stack(stack); // `llrb_destruct_stack`ed in this block.
      if (branch_dest_block->incoming_size > 1 && branch_dest_stack->size > 0) {
        llrb_push_incoming_things(c, branch_dest_block,
            LLVMGetInsertBlock(c->builder), llrb_stack_pop(branch_dest_stack));
      }
      llrb_compile_basic_block(c, branch_dest_block, branch_dest_stack);
      llrb_destruct_stack(branch_dest_stack);
      break; // caller `compile_basic_block` compiles fallthrough_block and pushes incoming things to its phi node.
    }
    case YARVINSN_getinlinecache:
      llrb_stack_push(stack, llrb_value(Qnil)); // TODO: implement
      break;
    case YARVINSN_setinlinecache:
      break; // TODO: implement
    //case YARVINSN_once:
    case YARVINSN_opt_case_dispatch: // Use `switch` instruction
      llrb_stack_pop(stack); // TODO: implement
      break;
    case YARVINSN_opt_plus: {
      LLVMValueRef rhs = llrb_stack_pop(stack);
      LLVMValueRef lhs = llrb_stack_pop(stack);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_opt_plus", 2, lhs, rhs));
      break;
    }
    case YARVINSN_opt_minus: {
      LLVMValueRef rhs = llrb_stack_pop(stack);
      LLVMValueRef lhs = llrb_stack_pop(stack);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_opt_minus", 2, lhs, rhs));
      break;
    }
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
    case YARVINSN_opt_neq:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("!="), 1));
      break;
    case YARVINSN_opt_lt: {
      LLVMValueRef rhs = llrb_stack_pop(stack);
      LLVMValueRef lhs = llrb_stack_pop(stack);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_opt_lt", 2, lhs, rhs));
      break;
    }
    case YARVINSN_opt_le:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("<="), 1));
      break;
    case YARVINSN_opt_gt:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '>', 1));
      break;
    case YARVINSN_opt_ge:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern(">="), 1));
      break;
    case YARVINSN_opt_ltlt:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("<<"), 1));
      break;
    case YARVINSN_opt_aref:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("[]"), 1));
      break;
    case YARVINSN_opt_aset:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("[]="), 2));
      break;
    case YARVINSN_opt_aset_with: {
      LLVMValueRef value = llrb_stack_pop(stack);
      LLVMValueRef recv  = llrb_stack_pop(stack);
      LLVMValueRef str   = llrb_call_func(c, "rb_str_resurrect", 1, llrb_value(operands[2]));

      // Not using llrb_compile_funcall to prevent stack overflow
      llrb_stack_push(stack, llrb_call_func(c, "rb_funcall", 5, recv,
            llrb_value(rb_intern("[]=")), LLVMConstInt(LLVMInt32Type(), 2, true), str, value));
      break;
    }
    case YARVINSN_opt_aref_with: {
      LLVMValueRef str = llrb_call_func(c, "rb_str_resurrect", 1, llrb_value(operands[2]));

      // Not using llrb_compile_funcall to prevent stack overflow
      llrb_stack_push(stack, llrb_call_func(c, "rb_funcall", 4, llrb_stack_pop(stack),
            llrb_value(rb_intern("[]")), LLVMConstInt(LLVMInt32Type(), 1, true), str));
      break;
    }
    case YARVINSN_opt_length:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("length"), 0));
      break;
    case YARVINSN_opt_size:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("size"), 0));
      break;
    case YARVINSN_opt_empty_p:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("empty?"), 0));
      break;
    case YARVINSN_opt_succ:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("succ"), 0));
      break;
    case YARVINSN_opt_not:
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, '!', 0));
      break;
    case YARVINSN_opt_regexpmatch1: {
      // Not using llrb_compile_funcall to prevent stack overflow
      llrb_stack_push(stack, llrb_call_func(c, "rb_funcall", 4, llrb_stack_pop(stack),
            llrb_value(rb_intern("=~")), LLVMConstInt(LLVMInt32Type(), 1, true), llrb_value(operands[0])));
      break;
    }
    case YARVINSN_opt_regexpmatch2: {
      llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("=~"), 1));
      break;
    }
    //case YARVINSN_opt_call_c_function:
    case YARVINSN_getlocal_OP__WC__0: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_getlocal_level0", 2, llrb_get_cfp(c), llrb_value((lindex_t)operands[0])));
      break;
    }
    case YARVINSN_getlocal_OP__WC__1: {
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_getlocal_level1", 2, llrb_get_cfp(c), llrb_value((lindex_t)operands[0])));
      break;
    }
    case YARVINSN_setlocal_OP__WC__0: {
      LLVMValueRef idx = llrb_value((lindex_t)operands[0]);
      llrb_call_func(c, "llrb_insn_setlocal_level0", 3, llrb_get_cfp(c), idx, llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_setlocal_OP__WC__1: {
      LLVMValueRef idx = llrb_value((lindex_t)operands[0]);
      llrb_call_func(c, "llrb_insn_setlocal_level1", 3, llrb_get_cfp(c), idx, llrb_stack_pop(stack));
      break;
    }
    case YARVINSN_putobject_OP_INT2FIX_O_0_C_:
      llrb_stack_push(stack, llrb_value(INT2FIX(0)));
      break;
    case YARVINSN_putobject_OP_INT2FIX_O_1_C_:
      llrb_stack_push(stack, llrb_value(INT2FIX(1)));
      break;
    default:
      llrb_dump_cfg(c->body, c->cfg);
      rb_raise(rb_eCompileError, "Unhandled insn at llrb_compile_insn: %s", insn_name(insn));
      break;
  }
  //fprintf(stderr, "  [DEBUG] llrb_compile_insn: %04d after %-27s (stack size: %d)\n", pos, insn_name(insn), stack->size);
  return false;
}

static void
llrb_compile_basic_block(const struct llrb_compiler *c, struct llrb_basic_block *block, struct llrb_stack *stack)
{
  if (block->compiled) return;
  block->compiled = true;

  // If phi node is created for this block, push it to stack.
  if (block->phi) {
    llrb_stack_push(stack, block->phi);
  }

  // Here is the actual compilation of block specified in arguments.
  bool returned = false, created_br = false;
  unsigned int pos = block->start;
  while (pos <= block->end) {
    LLVMPositionBuilderAtEnd(c->builder, block->ref); // Reset everytime to allow recursive compilation.
    int insn = rb_vm_insn_addr2insn((void *)c->body->iseq_encoded[pos]);
    returned = llrb_compile_insn(c, stack, pos, insn, c->body->iseq_encoded + (pos+1), &created_br);
    pos += insn_len(insn);
  }

  // If the function is not returned yet, compiles next block.
  if (!returned) {
    // In the end of the function, it must be returned...
    if (pos >= c->body->iseq_size) {
      rb_raise(rb_eCompileError, "Compiler compiled the end of function but the function was not returned");
    }

    struct llrb_basic_block *next_block = llrb_find_block(c->cfg, pos);
    LLVMPositionBuilderAtEnd(c->builder, block->ref); // Reset to allow recursive compilation.
    if (!created_br) LLVMBuildBr(c->builder, next_block->ref);

    if (next_block->incoming_size > 1 && stack->size > 0) {
      llrb_push_incoming_things(c, next_block, block->ref, llrb_stack_pop(stack));
    }
    llrb_compile_basic_block(c, next_block, stack);
  }
}

static LLVMBasicBlockRef
llrb_build_basic_block_ref(const struct llrb_compiler *c, const struct llrb_basic_block *block)
{
  VALUE label = rb_str_new_cstr("label_"); // `rb_str_free`d in the end of this function.
  rb_str_catf(label, "%d", block->start);

  LLVMBasicBlockRef ref = LLVMAppendBasicBlock(c->func, RSTRING_PTR(label));
  rb_str_free(label);
  return ref;
}

static void
llrb_init_cfg_for_compile(const struct llrb_compiler *c, struct llrb_cfg *cfg)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block* block = cfg->blocks + i;
    block->compiled = false;
    if (!block->traversed) continue;

    block->ref = llrb_build_basic_block_ref(c, block);
    block->phi = 0;
  }
}

// Compiles Control Flow Graph having encoded YARV instructions to LLVM IR.
static LLVMValueRef
llrb_compile_cfg(LLVMModuleRef mod, const struct rb_iseq_constant_body *body, const VALUE *new_iseq_encoded, struct llrb_cfg *cfg, const char* funcname)
{
  LLVMTypeRef args[] = { LLVMInt64Type(), LLVMInt64Type() };
  LLVMValueRef func = LLVMAddFunction(mod, funcname,
      LLVMFunctionType(LLVMInt64Type(), args, 2, false));

  const struct llrb_compiler compiler = (struct llrb_compiler){
    .body = body,
    .new_iseq_encoded = new_iseq_encoded,
    .cfg = cfg,
    .func = func,
    .builder = LLVMCreateBuilder(),
    .mod = mod,
  };
  llrb_init_cfg_for_compile(&compiler, cfg);

  // To simulate YARV stack, we need to traverse CFG again here instead of loop from start to end.
  struct llrb_stack stack = (struct llrb_stack){
    .body = ALLOC_N(LLVMValueRef, body->stack_max), // `xfree`d in the end of this function.
    .size = 0,
    .max  = body->stack_max,
  };
  llrb_compile_basic_block(&compiler, cfg->blocks, &stack);

  xfree(stack.body);
  return func;
}

// This sweaps memory `xmalloc`ed by llrb_create_basic_blocks.
static void
llrb_destruct_cfg(struct llrb_cfg *cfg)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block *block = cfg->blocks + i;
    xfree(block->incoming_starts);
  }
  xfree(cfg->blocks);
}

static LLVMModuleRef
llrb_build_initial_module()
{
  LLVMMemoryBufferRef buf;
  char *err;
  if (LLVMCreateMemoryBufferWithContentsOfFile("ext/insns.bc", &buf, &err)) {
    rb_raise(rb_eCompileError, "LLVMCreateMemoryBufferWithContentsOfFile Error: %s", err);
  }

  LLVMModuleRef ret;
  if (LLVMParseBitcode2(buf, &ret)) {
    rb_raise(rb_eCompileError, "LLVMParseBitcode2 Failed!");
  }
  LLVMDisposeMemoryBuffer(buf);
  return ret; // LLVMModuleCreateWithName("llrb");
}

static bool
llrb_includes_unsupported_insn(const rb_iseq_t *iseq)
{
  unsigned int i = 0;
  while (i < iseq->body->iseq_size) {
    int insn = rb_vm_insn_addr2insn((void *)iseq->body->iseq_encoded[i]);
    switch (insn) {
      case YARVINSN_expandarray:
        return true;
      default:
        break;
    }
    i += insn_len(insn);
  }
  return false;
}

bool
llrb_check_not_compilable(const rb_iseq_t *iseq)
{
  // At least 3 is needed: opt_call_c_function + funcptr + leave
  return iseq->body->iseq_size < 3
    // We don't want to set pc to index 1. It will be funcptr. So we don't compile for such case.
    || (insn_len(rb_vm_insn_addr2insn((void *)iseq->body->iseq_encoded[0])) == 1 &&
        llrb_pc_change_required(rb_vm_insn_addr2insn((void *)iseq->body->iseq_encoded[1]))
    || llrb_includes_unsupported_insn(iseq));
}

// llrb_create_native_func() uses a LLVM function named as `funcname` defined in returned LLVM module.
LLVMModuleRef
llrb_compile_iseq(const struct rb_iseq_constant_body *body, const VALUE *new_iseq_encoded, const char* funcname)
{
  extern void llrb_parse_iseq(const struct rb_iseq_constant_body *body, struct llrb_cfg *result);
  struct llrb_cfg cfg;
  llrb_parse_iseq(body, &cfg);

  LLVMModuleRef mod = llrb_build_initial_module();
  LLVMValueRef func = llrb_compile_cfg(mod, body, new_iseq_encoded, &cfg, funcname);

  extern void llrb_optimize_function(LLVMModuleRef cmod, LLVMValueRef cfunc);
  if (1) llrb_optimize_function(mod, func);

  if (0) llrb_dump_cfg(body, &cfg);
  if (0) LLVMDumpModule(mod);

  llrb_destruct_cfg(&cfg);
  return mod;
}

void
Init_compiler(VALUE rb_mJIT)
{
  rb_eCompileError = rb_define_class_under(rb_mJIT, "CompileError", rb_eStandardError);
}
