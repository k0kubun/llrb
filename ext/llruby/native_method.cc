#include "llruby/native_method.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

namespace llruby {

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;

void* NativeMethod::CreateFunction() {
  module = llvm::make_unique<llvm::Module>("top", context);
  llvm::Function *func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false),
      llvm::Function::ExternalLinkage, "main", module.get());
  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt32(0));
  module->dump();
  return (void *)0;
}

} // namespace llruby
