/*
 * compiler/stack.h: Data structure to emulate YARV stack.
 * This header is used only by compiler.c.
 */

#ifndef LLRB_COMPILER_STACK_H
#define LLRB_COMPILER_STACK_H

// Emulates rb_control_frame's sp (stack pointer), which is function local
struct llrb_stack {
  unsigned int size;
  unsigned int max;
  LLVMValueRef *body;
};

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

#endif // LLRB_COMPILER_STACK_H
