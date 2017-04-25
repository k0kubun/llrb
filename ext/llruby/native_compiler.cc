#include <memory>
#include "llruby/iseq.h"
#include "llruby/native_compiler.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"

namespace llruby {

uint64_t NativeCompiler::Compile(const Iseq& iseq, bool dry_run) {
  std::unique_ptr<llvm::Module> mod = llvm::make_unique<llvm::Module>("llruby", context);
  llvm::Function *func = CompileIseq(iseq, mod.get());

  if (dry_run) {
    mod->dump();
    return 0;
  } else {
    return CreateNativeFunction(func, std::move(mod));
  }
}

uint64_t NativeCompiler::CreateNativeFunction(llvm::Function *func, std::unique_ptr<llvm::Module> mod) {
  llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::move(mod)).create();
  if (engine == NULL) {
    fprintf(stderr, "Failed to create ExecutionEngine...\n");
    return 0;
  }
  return engine->getFunctionAddress(func->getName());
}

llvm::Function* NativeCompiler::CompileIseq(const Iseq& iseq, llvm::Module* mod) {
  std::vector<llvm::Type*> args = { llvm::IntegerType::get(context, 64) };
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt64Ty(context), args, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "precompiled_method", mod);

  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "", func));
  builder.CreateRet(builder.getInt64(Qnil));
  return func;
}

} // namespace llruby
