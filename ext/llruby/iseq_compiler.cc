#include "llruby/iseq_compiler.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

namespace llruby {

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> mod;

// IseqCompiler#compile_internal
// @param  [Array] ruby_iseq - Return value of RubyVM::InstructionSequence#to_a
// @return [LLRuby::LLVMFunction]
llvm::Function* IseqCompiler::Compile() {
  mod = llvm::make_unique<llvm::Module>("top", context);

  std::vector<llvm::Type*> args = { llvm::IntegerType::get(mod->getContext(), 64) };
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), args, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "rb_hello_func", mod.get());

  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt64(Qnil));
  return func;
}

} // namespace llruby
