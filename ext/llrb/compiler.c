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

// @return true if the IR compiled from given insn includes `ret` instruction. In that case, next block won't be compiled.
static bool
llrb_compile_insn(const struct llrb_compiler *c, struct llrb_stack *stack, const unsigned int pos, const int insn, const VALUE *operands)
{
  switch (insn) {
    // case YARVINSN_nop:
    //   break; // nop
    // //case YARVINSN_getlocal: {
    // //  ;
    // //  break;
    // //}
    // //case YARVINSN_setlocal: {
    // //  ;
    // //  break;
    // //}
    // case YARVINSN_getspecial: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]), llrb_value(operands[1]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_getspecial"), args, 2, "getspecial"));
    //   break;
    // }
    // case YARVINSN_setspecial: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_setspecial"), args, 2, "");
    //   break;
    // }
    // case YARVINSN_getinstancevariable: { // TODO: implement inline cache counterpart
    //   LLVMValueRef args[] = { llrb_get_self(c), llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ivar_get"), args, 2, "getinstancevariable"));
    //   break;
    // }
    // case YARVINSN_setinstancevariable: { // TODO: implement inline cache counterpart
    //   LLVMValueRef args[] = { llrb_get_self(c), llrb_value(operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ivar_set"), args, 3, "");
    //   break;
    // }
    // case YARVINSN_getclassvariable: {
    //   LLVMValueRef args[] = { llrb_get_cfp(c), llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_getclassvariable"), args, 2, "getclassvariable"));
    //   break;
    // }
    // case YARVINSN_setclassvariable: {
    //   LLVMValueRef args[] = { llrb_get_cfp(c), llrb_value(operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_setclassvariable"), args, 3, "");
    //   break;
    // }
    // case YARVINSN_getconstant: {
    //   LLVMValueRef args[] = { llrb_get_thread(c), llrb_stack_pop(stack), llrb_value(operands[0]), LLVMConstInt(LLVMInt32Type(), 0, true) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "vm_get_ev_const"), args, 4, "getconstant"));
    //   break;
    // }
    // case YARVINSN_setconstant: {
    //   LLVMValueRef cbase = llrb_stack_pop(stack);
    //   LLVMValueRef args[] = { llrb_get_self(c), cbase, llrb_value(operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_setconstant"), args, 4, "");
    //   break;
    // }
    // case YARVINSN_getglobal: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_gvar_get"), args, 1, "getglobal"));
    //   break;
    // }
    // case YARVINSN_setglobal: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_gvar_set"), args, 2, "setglobal");
    //   break;
    // }
    // case YARVINSN_putnil:
    //   llrb_stack_push(stack, llrb_value(Qnil));
    //   break;
    // case YARVINSN_putself: {
    //   llrb_stack_push(stack, llrb_get_self(c));
    //   break;
    // }
    case YARVINSN_putobject:
      llrb_stack_push(stack, llrb_value(operands[0]));
      break;
    // case YARVINSN_putspecialobject: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_putspecialobject"), args, 1, "putspecialobject"));
    //   break;
    // }
    // case YARVINSN_putiseq:
    //   llrb_stack_push(stack, llrb_value(operands[0]));
    //   break;
    // case YARVINSN_putstring: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_str_resurrect"), args, 1, "putstring"));
    //   break;
    // }
    // case YARVINSN_concatstrings: {
    //   LLVMValueRef *args = ALLOC_N(LLVMValueRef, operands[0] + 1);
    //   args[0] = llrb_value(operands[0]); // function is in size_t. correct?
    //   for (long i = (long)operands[0]-1; 0 <= i; i--) {
    //     args[1+i] = llrb_stack_pop(stack);
    //   }
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_concatstrings"), args, operands[0] + 1, "concatstrings"));
    //   break;
    // }
    // case YARVINSN_tostring: {
    //   LLVMValueRef args[] = { llrb_stack_pop(stack) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_obj_as_string"), args, 1, "tostring"));
    //   break;
    // }
    // case YARVINSN_freezestring: { // TODO: check debug info
    //   LLVMValueRef args[] = { llrb_stack_pop(stack) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_str_freeze"), args, 1, "freezestring"));
    //   break;
    // }
    // case YARVINSN_toregexp: {
    //   rb_num_t cnt = operands[1];
    //   LLVMValueRef *args1 = ALLOC_N(LLVMValueRef, cnt+1);
    //   args1[0] = LLVMConstInt(LLVMInt64Type(), (long)cnt, true);
    //   for (rb_num_t i = 0; i < cnt; i++) {
    //     args1[1+i] = llrb_stack_pop(stack);
    //   }
    //   LLVMValueRef ary = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ary_new_from_args"), args1, 1+cnt, "toregexp");

    //   LLVMValueRef args2[] = { ary, LLVMConstInt(LLVMInt32Type(), (int)operands[0], true) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_reg_new_ary"), args2, 2, "toregexp"));

    //   LLVMValueRef args3[] = { ary };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ary_clear"), args3, 1, "toregexp");
    //   break;
    // }
    // case YARVINSN_newarray:
    //   llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
    //   break;
    // case YARVINSN_duparray: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_ary_resurrect"), args, 1, "duparray"));
    //   break;
    // }
    // //case YARVINSN_expandarray: {
    // //  rb_num_t flag = (rb_num_t)operands[1];
    // //  if (flag & 0x02) { // for postarg
    // //  } else {
    // //  }
    // //  break;
    // //}
    // case YARVINSN_concatarray: {
    //   LLVMValueRef args[] = { 0, llrb_stack_pop(stack) };
    //   args[0] = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_concatarray"), args, 2, "concatarray"));
    //   break;
    // }
    // case YARVINSN_splatarray: {
    //   // Can we refactor code for this kind of insn implementation...?
    //   LLVMValueRef args[] = { llrb_stack_pop(stack), llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_splatarray"), args, 2, "splatarray"));
    //   break;
    // }
    // case YARVINSN_newhash: {
    //   LLVMValueRef *values = ALLOC_N(LLVMValueRef, operands[0] / 2);
    //   LLVMValueRef *keys   = ALLOC_N(LLVMValueRef, operands[0] / 2);
    //   for (int i = 0; i < (int)operands[0] / 2; i++) {
    //     values[i] = llrb_stack_pop(stack);
    //     keys[i]   = llrb_stack_pop(stack);
    //   }

    //   LLVMValueRef result = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_hash_new"), 0, 0, "newhash");
    //   // reverse set
    //   for (int i = (int)operands[0] / 2 - 1; 0 <= i; i--) {
    //     LLVMValueRef args[] = { result, keys[i], values[i] };
    //     LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_hash_aset"), args, 3, "newhash_aset");
    //   }
    //   llrb_stack_push(stack, result);
    //   break;
    // }
    // case YARVINSN_newrange: {
    //   LLVMValueRef high = llrb_stack_pop(stack);
    //   LLVMValueRef low  = llrb_stack_pop(stack);
    //   LLVMValueRef flag = LLVMConstInt(LLVMInt64Type(), operands[0], false);
    //   LLVMValueRef args[] = { low, high, flag };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_range_new"), args, 3, "newrange"));
    //   break;
    // }
    // case YARVINSN_pop:
    //   llrb_stack_pop(stack);
    //   break;
    // case YARVINSN_dup: {
    //   LLVMValueRef value = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, value);
    //   llrb_stack_push(stack, value);
    //   break;
    // }
    // case YARVINSN_dupn: {
    //   LLVMValueRef *values = ALLOC_N(LLVMValueRef, operands[0]);
    //   for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
    //     values[i] = llrb_stack_pop(stack); // FIXME: obviously no need to pop
    //   }

    //   for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
    //     llrb_stack_push(stack, values[operands[0] - 1 - i]);
    //   }
    //   for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
    //     llrb_stack_push(stack, values[operands[0] - 1 - i]);
    //   }
    //   break;
    // }
    // case YARVINSN_swap: {
    //   LLVMValueRef first  = llrb_stack_pop(stack);
    //   LLVMValueRef second = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, first);
    //   llrb_stack_push(stack, second);
    //   break;
    // }
    // //case YARVINSN_reverse: {
    // //  rb_num_t n = (rb_num_t)operands[0];
    // //  unsigned int last = stack->size - 1;
    // //  unsigned int top_i = last - n;

    // //  for (rb_num_t i = 0; i < n/2; i++) {
    // //    LLVMValueRef v0 = stack->body[top_i+i];
    // //    LLVMValueRef v1 = stack->body[last-i];
    // //    stack->body[top_i+i] = v1;
    // //    stack->body[last-i]  = v0;
    // //  }
    // //  break;
    // //}
    // //case YARVINSN_reput:
    // //  break; // none
    // case YARVINSN_topn: {
    //   llrb_stack_push(stack, llrb_topn(stack, (unsigned int)operands[0]));
    //   break;
    // }
    // case YARVINSN_setn: {
    //   rb_num_t last = (rb_num_t)stack->size - 1;
    //   stack->body[last - (rb_num_t)operands[0]] = stack->body[last];
    //   break;
    // }
    // case YARVINSN_adjuststack: {
    //   for (rb_num_t i = 0; i < (rb_num_t)operands[0]; i++) {
    //     llrb_stack_pop(stack);
    //   }
    //   break;
    // }
    // case YARVINSN_defined: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]), llrb_value(operands[1]), llrb_value(operands[2]), llrb_stack_pop(stack) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_defined"), args, 4, "defined"));
    //   break;
    // }
    // case YARVINSN_checkmatch: {
    //   LLVMValueRef args[] = { 0, llrb_stack_pop(stack), LLVMConstInt(LLVMInt64Type(), operands[0], false) };
    //   args[0] = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_checkmatch"), args, 3, "checkmatch"));
    //   break;
    // }
    // case YARVINSN_checkkeyword: {
    //   LLVMValueRef args[] = { llrb_get_cfp(c), llrb_value((lindex_t)operands[0]), llrb_value((rb_num_t)operands[1]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_checkkeyword"), args, 3, "checkkeyword"));
    //   break;
    // }
    case YARVINSN_trace: {
      rb_event_flag_t flag = (rb_event_flag_t)((rb_num_t)operands[0]);
      LLVMValueRef val = (flag & (RUBY_EVENT_RETURN | RUBY_EVENT_B_RETURN)) ? stack->body[stack->size-1] : llrb_value(Qundef);
      llrb_call_func(c, "llrb_insn_trace", 4, llrb_get_thread(c), llrb_get_cfp(c), LLVMConstInt(LLVMInt32Type(), flag, false), val);
      break;
    }
    // //case YARVINSN_defineclass: {
    // //  ;
    // //  break;
    // //}
    // case YARVINSN_send: {
    //   CALL_INFO ci = (CALL_INFO)operands[0];
    //   unsigned int stack_size = ci->orig_argc + 1;

    //   LLVMValueRef *args = ALLOC_N(LLVMValueRef, 5 + stack_size);
    //   args[0] = llrb_get_thread(c);
    //   args[1] = llrb_get_cfp(c);
    //   args[2] = llrb_value((VALUE)ci);
    //   args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
    //   args[4] = llrb_value((VALUE)((ISEQ)operands[2]));
    //   args[5] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
    //   for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
    //     args[6 + i] = llrb_stack_pop(stack);
    //   }
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_send"), args, 6 + stack_size, "send"));
    //   break;
    // }
    // case YARVINSN_opt_str_freeze: {
    //   LLVMValueRef args[] = { llrb_value(operands[0]), llrb_value(rb_intern("freeze")), LLVMConstInt(LLVMInt32Type(), 0, true) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_funcall"), args, 3, "opt_str_freeze"));
    //   break;
    // }
    // case YARVINSN_opt_newarray_max:
    //   llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("max"), 0));
    //   break;
    // case YARVINSN_opt_newarray_min:
    //   llrb_stack_push(stack, llrb_compile_newarray(c, stack, (long)operands[0]));
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("min"), 0));
    //   break;
    // case YARVINSN_opt_send_without_block: {
    //   CALL_INFO ci = (CALL_INFO)operands[0];
    //   unsigned int stack_size = ci->orig_argc + 1;

    //   LLVMValueRef *args = ALLOC_N(LLVMValueRef, 5 + stack_size);
    //   args[0] = llrb_get_thread(c);
    //   args[1] = llrb_get_cfp(c);
    //   args[2] = llrb_value((VALUE)ci);
    //   args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
    //   args[4] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
    //   for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
    //     args[5 + i] = llrb_stack_pop(stack);
    //   }
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_opt_send_without_block"), args, 5 + stack_size, "opt_send_without_block"));
    //   break;
    // }
    // case YARVINSN_invokesuper: { // TODO: refactor with opt_send_without_block
    //   CALL_INFO ci = (CALL_INFO)operands[0];
    //   unsigned int stack_size = ci->orig_argc + 1;

    //   LLVMValueRef *args = ALLOC_N(LLVMValueRef, 5 + stack_size);
    //   args[0] = llrb_get_thread(c);
    //   args[1] = llrb_get_cfp(c);
    //   args[2] = llrb_value((VALUE)ci);
    //   args[3] = llrb_value((VALUE)((CALL_CACHE)operands[1]));
    //   args[4] = llrb_value((VALUE)((ISEQ)operands[2]));
    //   args[5] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
    //   for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
    //     args[6 + i] = llrb_stack_pop(stack);
    //   }
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_invokesuper"), args, 6 + stack_size, "invokesuper"));
    //   break;
    // }
    // case YARVINSN_invokeblock: {
    //   CALL_INFO ci = (CALL_INFO)operands[0];
    //   unsigned int stack_size = ci->orig_argc;

    //   LLVMValueRef *args = ALLOC_N(LLVMValueRef, 4 + stack_size);
    //   args[0] = llrb_get_thread(c);
    //   args[1] = llrb_get_cfp(c);
    //   args[2] = llrb_value((VALUE)ci);
    //   args[3] = LLVMConstInt(LLVMInt32Type(), stack_size, false);
    //   for (int i = (int)stack_size - 1; 0 <= i; i--) { // recv + argc
    //     args[4 + i] = llrb_stack_pop(stack);
    //   }
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_invokeblock"), args, 4 + stack_size, "invokeblock"));
    //   break;
    // }
    case YARVINSN_leave:
      if (stack->size != 1) {
        llrb_dump_cfg(c->body, c->cfg);
        LLVMDumpModule(c->mod);
        rb_raise(rb_eCompileError, "unexpected stack size at leave: %d", stack->size);
      }

      llrb_call_func(c, "llrb_push_result", 2, llrb_get_cfp(c), llrb_stack_pop(stack));
      LLVMBuildRet(c->builder, llrb_get_cfp(c));
      return true;
    // case YARVINSN_throw: {
    //   LLVMValueRef args[] = { llrb_get_thread(c), llrb_get_cfp(c), llrb_value((rb_num_t)operands[0]), llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_throw"), args, 4, "");

    //   // In opt_call_c_function, if we return 0, we can throw error fron th->errinfo.
    //   // https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2147-L2151
    //   LLVMBuildRet(c->builder, llrb_value(0));
    //   return true;
    //   break;
    // }
    // case YARVINSN_jump: {
    //   unsigned dest = pos + (unsigned)insn_len(insn) + operands[0];
    //   LLVMBasicBlockRef next_block = llrb_build_block(c, dest);

    //   // If stack is empty, don't create phi.
    //   if (stack->size == 0) {
    //     LLVMBuildBr(c->builder, next_block);
    //     llrb_compile_basic_block(c, 0, dest);
    //     return true;
    //   }

    //   LLVMValueRef phi = c->blocks[dest].phi;
    //   if (phi == 0) {
    //     llrb_push_incoming_things(&c->blocks[dest], LLVMGetInsertBlock(c->builder), llrb_stack_pop(stack));
    //   } else {
    //     LLVMValueRef values[] = { llrb_stack_pop(stack) };
    //     LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
    //     LLVMAddIncoming(phi, values, blocks, 1);
    //   }

    //   LLVMBuildBr(c->builder, next_block);
    //   return true;
    // }
    // case YARVINSN_branchif: {
    //   unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
    //   unsigned fallthrough = pos + (unsigned)insn_len(insn);
    //   LLVMBasicBlockRef branch_dest_block = llrb_build_block(c, branch_dest);
    //   LLVMBasicBlockRef fallthrough_block = llrb_build_block(c, fallthrough);

    //   LLVMValueRef cond = llrb_stack_pop(stack);
    //   LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), branch_dest_block, fallthrough_block);

    //   struct llrb_stack copied_stack = (struct llrb_stack){ .size = stack->size, .max = stack->max };
    //   copied_stack.body = ALLOC_N(LLVMValueRef, copied_stack.max);
    //   for (unsigned int i = 0; i < stack->size; i++) copied_stack.body[i] = stack->body[i];

    //   if (copied_stack.size > 0) {
    //     LLVMValueRef phi = c->blocks[fallthrough].phi;
    //     if (phi == 0) {
    //       llrb_push_incoming_things(&c->blocks[fallthrough], LLVMGetInsertBlock(c->builder), llrb_stack_pop(&copied_stack));
    //     } else {
    //       LLVMValueRef values[] = { llrb_stack_pop(&copied_stack) };
    //       LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
    //       LLVMAddIncoming(phi, values, blocks, 1);
    //     }
    //   }

    //   // If jumping forward (branch_dest > pos), create phi. (If jumping back (branch_dest < pos), consider it as loop and don't create phi in that case.)
    //   if (branch_dest > pos && stack->size > 0) {
    //     LLVMValueRef phi = c->blocks[branch_dest].phi;
    //     if (phi == 0) {
    //       llrb_push_incoming_things(&c->blocks[branch_dest], LLVMGetInsertBlock(c->builder), llrb_stack_pop(stack));
    //     } else {
    //       LLVMValueRef *values = ALLOC_N(LLVMValueRef, 1);
    //       values[0] = llrb_stack_pop(stack);
    //       LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
    //       LLVMAddIncoming(phi, values, blocks, 1);
    //     }
    //   }

    //   llrb_compile_basic_block(c, &copied_stack, fallthrough);
    //   llrb_compile_basic_block(c, stack, branch_dest);
    //   return true;
    // }
    // case YARVINSN_branchunless: {
    //   unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
    //   unsigned fallthrough = pos + (unsigned)insn_len(insn);
    //   LLVMBasicBlockRef branch_dest_block = llrb_build_block(c, branch_dest);
    //   LLVMBasicBlockRef fallthrough_block = llrb_build_block(c, fallthrough);

    //   LLVMValueRef cond = llrb_stack_pop(stack);
    //   LLVMBuildCondBr(c->builder, llrb_build_rtest(c->builder, cond), fallthrough_block, branch_dest_block);

    //   struct llrb_stack copied_stack = (struct llrb_stack){ .size = stack->size, .max = stack->max };
    //   copied_stack.body = ALLOC_N(LLVMValueRef, copied_stack.max);
    //   for (unsigned int i = 0; i < stack->size; i++) copied_stack.body[i] = stack->body[i];

    //   llrb_compile_basic_block(c, &copied_stack, fallthrough); // COMPILE FALLTHROUGH FIRST!!!!
    //   llrb_compile_basic_block(c, stack, branch_dest); // because this line will continue to compile next block and it should wait the other branch.
    //   return true;
    // }
    // case YARVINSN_branchnil: {
    //   unsigned branch_dest = pos + (unsigned)insn_len(insn) + operands[0];
    //   unsigned fallthrough = pos + (unsigned)insn_len(insn);
    //   LLVMBasicBlockRef branch_dest_block = llrb_build_block(c, branch_dest);
    //   LLVMBasicBlockRef fallthrough_block = llrb_build_block(c, fallthrough);

    //   LLVMValueRef cond = llrb_stack_pop(stack);
    //   LLVMBuildCondBr(c->builder,
    //       LLVMBuildICmp(c->builder, LLVMIntNE, cond, llrb_value(Qnil), "NIL_P"),
    //       fallthrough_block, branch_dest_block);

    //   LLVMValueRef phi = c->blocks[branch_dest].phi;
    //   if (phi == 0) {
    //     llrb_push_incoming_things(&c->blocks[branch_dest], LLVMGetInsertBlock(c->builder), llrb_value(Qnil));
    //   } else {
    //     LLVMValueRef values[] = { llrb_value(Qnil) };
    //     LLVMBasicBlockRef blocks[] = { LLVMGetInsertBlock(c->builder) };
    //     LLVMAddIncoming(phi, values, blocks, 1);
    //   }

    //   llrb_compile_basic_block(c, stack, fallthrough);
    //   return true;
    // }
    // case YARVINSN_getinlinecache:
    //   llrb_stack_push(stack, llrb_value(Qnil)); // TODO: implement
    //   break;
    // case YARVINSN_setinlinecache:
    //   break; // TODO: implement
    // //case YARVINSN_once:
    // case YARVINSN_opt_case_dispatch: // Use `switch` instruction
    //   llrb_stack_pop(stack); // TODO: implement
    //   break;
    case YARVINSN_opt_plus: {
      LLVMValueRef rhs = llrb_stack_pop(stack);
      LLVMValueRef lhs = llrb_stack_pop(stack);
      llrb_stack_push(stack, llrb_call_func(c, "llrb_insn_opt_plus", 2, lhs, rhs));
      break;
    }
    // case YARVINSN_opt_minus: {
    //   //llrb_stack_push(stack, llrb_compile_funcall(c, stack, '-', 1));
    //   LLVMValueRef args[] = { 0, llrb_stack_pop(stack) };
    //   args[0] = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_opt_minus"), args, 2, "opt_minus"));
    //   break;
    // }
    // case YARVINSN_opt_mult:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, '*', 1));
    //   break;
    // case YARVINSN_opt_div:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, '/', 1));
    //   break;
    // case YARVINSN_opt_mod:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, '%', 1));
    //   break;
    // case YARVINSN_opt_eq:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("=="), 1));
    //   break;
    // case YARVINSN_opt_neq:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("!="), 1));
    //   break;
    // case YARVINSN_opt_lt: {
    //   //llrb_stack_push(stack, llrb_compile_funcall(c, stack, '<', 1));
    //   LLVMValueRef args[] = { 0, llrb_stack_pop(stack) };
    //   args[0] = llrb_stack_pop(stack);
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_opt_lt"), args, 2, "opt_lt"));
    //   break;
    // }
    // case YARVINSN_opt_le:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("<="), 1));
    //   break;
    // case YARVINSN_opt_gt:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, '>', 1));
    //   break;
    // case YARVINSN_opt_ge:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern(">="), 1));
    //   break;
    // case YARVINSN_opt_ltlt:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("<<"), 1));
    //   break;
    // case YARVINSN_opt_aref:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("[]"), 1));
    //   break;
    // case YARVINSN_opt_aset:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("[]="), 2));
    //   break;
    // case YARVINSN_opt_aset_with: {
    //   LLVMValueRef value = llrb_stack_pop(stack);
    //   LLVMValueRef recv  = llrb_stack_pop(stack);

    //   LLVMValueRef resurrect_args[] = { llrb_value(operands[2]) };
    //   LLVMValueRef str = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_str_resurrect"), resurrect_args, 1, "opt_aset_with_3");

    //   // Not using llrb_compile_funcall to prevent stack overflow
    //   LLVMValueRef args[] = { recv, llrb_value(rb_intern("[]=")), LLVMConstInt(LLVMInt32Type(), 2, true), str, value };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_funcall"), args, 5, "opt_aset_with"));
    //   break;
    // }
    // case YARVINSN_opt_aref_with: {
    //   LLVMValueRef resurrect_args[] = { llrb_value(operands[2]) };
    //   LLVMValueRef str = LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_str_resurrect"), resurrect_args, 1, "opt_aref_with_3");

    //   // Not using llrb_compile_funcall to prevent stack overflow
    //   LLVMValueRef args[] = { llrb_stack_pop(stack), llrb_value(rb_intern("[]")), LLVMConstInt(LLVMInt32Type(), 1, true), str };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_funcall"), args, 4, "opt_aref_with"));
    //   break;
    // }
    // case YARVINSN_opt_length:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("length"), 0));
    //   break;
    // case YARVINSN_opt_size:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("size"), 0));
    //   break;
    // case YARVINSN_opt_empty_p:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("empty?"), 0));
    //   break;
    // case YARVINSN_opt_succ:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("succ"), 0));
    //   break;
    // case YARVINSN_opt_not:
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, '!', 0));
    //   break;
    // case YARVINSN_opt_regexpmatch1: {
    //   // Not using llrb_compile_funcall to prevent stack overflow
    //   LLVMValueRef args[] = { llrb_stack_pop(stack), llrb_value(rb_intern("=~")), LLVMConstInt(LLVMInt32Type(), 1, true), llrb_value(operands[0]) };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "rb_funcall"), args, 4, "opt_regexpmatch1"));
    //   break;
    // }
    // case YARVINSN_opt_regexpmatch2: {
    //   llrb_stack_push(stack, llrb_compile_funcall(c, stack, rb_intern("=~"), 1));
    //   break;
    // }
    // //case YARVINSN_opt_call_c_function:
    // case YARVINSN_getlocal_OP__WC__0: {
    //   LLVMValueRef idx = llrb_value((lindex_t)operands[0]);
    //   LLVMValueRef args[] = { llrb_get_cfp(c), idx };
    //   llrb_stack_push(stack, LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_getlocal_level0"), args, 2, "getlocal"));
    //   break;
    // }
    // //case YARVINSN_getlocal_OP__WC__1: {
    // //  ;
    // //  break;
    // //}
    // case YARVINSN_setlocal_OP__WC__0: {
    //   LLVMValueRef idx = llrb_value((lindex_t)operands[0]);
    //   LLVMValueRef args[] = { llrb_get_cfp(c), idx, llrb_stack_pop(stack) };
    //   LLVMBuildCall(c->builder, llrb_get_function(c->mod, "llrb_insn_setlocal_level0"), args, 3, "");
    //   break;
    // }
    // //case YARVINSN_setlocal_OP__WC__1: {
    // //  ;
    // //  break;
    // //}
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
  return false;
}

static LLVMBasicBlockRef
llrb_build_basic_block(const struct llrb_compiler *c, const struct llrb_basic_block *basic_block)
{
  VALUE label = rb_str_new_cstr("label_"); // `rb_str_free`d in the end of this function.
  rb_str_catf(label, "%d", basic_block->start);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(c->func, RSTRING_PTR(label));
  rb_str_free(label);
  return block;
}

static void
llrb_compile_basic_block(const struct llrb_compiler *c, struct llrb_basic_block *basic_block, struct llrb_stack *stack)
{
  if (basic_block->compiled) return;
  basic_block->compiled = true;

  LLVMBasicBlockRef block = llrb_build_basic_block(c, basic_block);
  LLVMPositionBuilderAtEnd(c->builder, block);

  for (unsigned int i = basic_block->start; i <= basic_block->end;) {
    int insn = rb_vm_insn_addr2insn((void *)c->body->iseq_encoded[i]);
    llrb_compile_insn(c, stack, i, insn, c->body->iseq_encoded + (i+1));
    i += insn_len(insn);
  }
}

static void
llrb_init_cfg_for_compile(struct llrb_cfg *cfg)
{
  for (unsigned int i = 0; i < cfg->size; i++) {
    struct llrb_basic_block* block = cfg->blocks + i;
    block->compiled = false;
  }
}

// Compiles Control Flow Graph having encoded YARV instructions to LLVM IR.
static LLVMValueRef
llrb_compile_cfg(LLVMModuleRef mod, const struct rb_iseq_constant_body *body, struct llrb_cfg *cfg, const char* funcname)
{
  LLVMTypeRef args[] = { LLVMInt64Type(), LLVMInt64Type() };
  LLVMValueRef func = LLVMAddFunction(mod, funcname,
      LLVMFunctionType(LLVMInt64Type(), args, 2, false));

  llrb_init_cfg_for_compile(cfg);
  const struct llrb_compiler compiler = (struct llrb_compiler){
    .body = body,
    .cfg = cfg,
    .func = func,
    .builder = LLVMCreateBuilder(),
    .mod = mod,
  };
  struct llrb_stack stack = (struct llrb_stack){
    .body = ALLOC_N(LLVMValueRef, body->stack_max), // `xfree`d in the end of this function.
    .size = 0,
    .max  = body->stack_max,
  };

  // To simulate YARV stack, we need to traverse CFG again here instead of loop from start to end.
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

// In this function, LLRB has following dependency tree without mutual dependencies:
// llrb.c -> compiler.c -> parser.c, optimizer.cc
//
// llrb_create_native_func() uses a LLVM function named as `funcname` defined in returned LLVM module.
LLVMModuleRef
llrb_compile_iseq(const struct rb_iseq_constant_body *body, const char* funcname)
{
  extern void llrb_parse_iseq(const struct rb_iseq_constant_body *body, struct llrb_cfg *result);
  struct llrb_cfg cfg;
  llrb_parse_iseq(body, &cfg);

  LLVMModuleRef mod = llrb_build_initial_module();
  LLVMValueRef func = llrb_compile_cfg(mod, body, &cfg, funcname);

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
