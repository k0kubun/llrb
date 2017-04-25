#include "llruby/iseq_compiler.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

extern llvm::LLVMContext context;

namespace llruby {

// IseqCompiler#compile_internal
// @param  [Array] ruby_iseq - Return value of RubyVM::InstructionSequence#to_a
// @return [LLRuby::LLVMFunction]
llvm::Function* IseqCompiler::Compile(const Iseq& iseq) {
  std::vector<llvm::Type*> args = { llvm::IntegerType::get(context, 64) };
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), args, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "rb_hello_func");

  llvm::IRBuilder<> builder(context);
  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt64(Qnil));
  return func;
}

} // namespace llruby
