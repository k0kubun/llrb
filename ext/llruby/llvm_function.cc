#include "ruby.h"
#include "llvm/IR/Function.h"

static size_t
llruby_llvm_function_dsize(RB_UNUSED_VAR(const void *arg))
{
  return sizeof(llvm::Function);
}

const rb_data_type_t llruby_llvm_func_t = {
  /* .wrap_struct_name = */ "llvm::Function",
  /* .function = */ {
    /* .dmark = */ NULL, // llvm::Function doesn't have Ruby object. No mark is necessary.
    /* .dfree = */ NULL, // Even if LLRuby::LLVMFunction is freed, its function may be executed. So we need to manually remove function in another place.
    /* .dsize = */ llruby_llvm_function_dsize,
    /* .reserved = */ { 0, 0 },
  },
  /* .parent = */ 0,
  /* .data = */ NULL,
  /* .flags = */ RUBY_TYPED_FREE_IMMEDIATELY, // I'm not sure about how we should handle this in llvm::Function. Just conservative choice.
};

extern VALUE rb_cLLVMFunction;

namespace llruby {

llvm::Function* GetLLVMFunction(VALUE llvm_function_obj) {
  llvm::Function *func;
  TypedData_Get_Struct(llvm_function_obj, llvm::Function, &llruby_llvm_func_t, func);
  return func;
}

VALUE WrapLLVMFunction(llvm::Function *func) {
  return TypedData_Wrap_Struct(rb_cLLVMFunction, &llruby_llvm_func_t, func);
}

} // namespace llruby
