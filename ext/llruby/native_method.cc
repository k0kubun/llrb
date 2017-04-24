#include <memory>
#include <cstdio>
#include "llvm/Support/TargetSelect.h"
#include "llruby/ruby.h"
#include "llruby/native_method.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"

namespace llruby {

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> mod;

// NativeMethod#define_internal
// @param  [Class,Module] klass
// @param  [String,Symbol] method_name
// @return [nil]
uint64_t NativeMethod::CreateFunction() {
  mod = llvm::make_unique<llvm::Module>("top", context);

  std::vector<llvm::Type*> args;
  args.push_back(llvm::IntegerType::get(mod->getContext(), 64));
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), args, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "rb_hello_func", mod.get());

  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt64(Qnil));

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::move(mod)).create();
  if (engine == NULL) {
    fprintf(stderr, "Failed to create ExecutionEngine...\n");
    return 0;
  }
  return engine->getFunctionAddress(func->getName());
}

} // namespace llruby
