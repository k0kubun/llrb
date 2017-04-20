#include "llruby/ruby.h"
#include "llruby/native_method.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

namespace llruby {

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> mod;

void* NativeMethod::CreateFunction() {
  mod = llvm::make_unique<llvm::Module>("top", context);

  std::vector<llvm::Type*> args;
  args.push_back(llvm::IntegerType::get(mod->getContext(), 64));
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), args, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "rb_hello_func", mod.get());

  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt64(Qnil));

  return (void *)0;
}

} // namespace llruby
