#include <memory>
#include "iseq.h"
#include "llruby.h"
#include "native_compiler.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#pragma clang diagnostic pop

namespace llruby {

uint64_t NativeCompiler::Compile(const Iseq& iseq, bool dry_run) {
  std::unique_ptr<llvm::Module> mod = llvm::make_unique<llvm::Module>("llruby", context);
  llvm::Function *func = CompileIseq(iseq, mod.get());
  if (!func) return 0;

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

llvm::Function* NativeCompiler::CompileIseq(const Iseq& iseq, llvm::Module *mod) {
  llvm::Function *rb_funcallf = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 32)},
        true),
      llvm::Function::ExternalLinkage, "rb_funcall", mod);

  llvm::Function *func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64) }, false),
      llvm::Function::ExternalLinkage, "precompiled_method", mod);
  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", func));
  stack.clear();

  for (const Object& insn : iseq.bytecode) {
    if (insn.klass == "Array") {
      bool compiled = CompileInstruction(insn.array, mod, rb_funcallf);
      if (!compiled) return nullptr;
    } else if (insn.klass == "Symbol") {
      // label. ignored for now
    } else if (insn.klass == "Fixnum" || insn.klass == "Integer") {
      // lineno. ignored for now
    } else {
      fprintf(stderr, "unexpected insn.klass at CompileIseq: %s\n", insn.klass.c_str());
      return nullptr;
    }
  }

  if (stack.size() == 1) {
    builder.CreateRet(stack.back());
  } else {
    fprintf(stderr, "unexpected stack size at CompileIseq: %d\n", (int)stack.size());
    mod->dump();
    return nullptr;
  }
  return func;
}

bool NativeCompiler::CompileInstruction(const std::vector<Object>& instruction, llvm::Module *mod, llvm::Function *rb_funcallf) {
  const std::string& name = instruction[0].symbol;
  if (name == "putnil") {
    stack.push_back(CompileObject(Object(Qnil)));
  } else if (name == "putobject") {
    stack.push_back(CompileObject(instruction[1]));
  } else if (name == "opt_send_without_block") {
    std::map<std::string, Object> options = instruction[1].hash;
    Object mid = options["mid"];
    Object orig_argc = options["orig_argc"];
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern(mid.symbol.c_str())), orig_argc.integer);
  } else if (name == "opt_plus") {
    CompileFuncall(rb_funcallf, builder.getInt64('+'), 1);
  } else if (name == "opt_minus") {
    CompileFuncall(rb_funcallf, builder.getInt64('-'), 1);
  } else if (name == "opt_mult") {
    CompileFuncall(rb_funcallf, builder.getInt64('*'), 1);
  } else if (name == "opt_div") {
    CompileFuncall(rb_funcallf, builder.getInt64('/'), 1);
  } else if (name == "opt_mod") {
    CompileFuncall(rb_funcallf, builder.getInt64('%'), 1);
  } else if (name == "opt_eq") {
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern("==")), 1);
  } else if (name == "opt_neq") {
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern("!=")), 1);
  } else if (name == "opt_lt") {
    CompileFuncall(rb_funcallf, builder.getInt64('<'), 1);
  } else if (name == "opt_le") {
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern("<=")), 1);
  } else if (name == "opt_gt") {
    CompileFuncall(rb_funcallf, builder.getInt64('>'), 1);
  } else if (name == "opt_ge") {
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern(">=")), 1);
  } else if (name == "opt_succ") {
    CompileFuncall(rb_funcallf, builder.getInt64(rb_intern("succ")), 0);
  } else if (name == "opt_not") {
    CompileFuncall(rb_funcallf, builder.getInt64('!'), 0);
  } else if (name == "trace") {
    // ignored for now
  } else if (name == "leave") {
    // ignored for now
  } else if (name == "nop") {
    // nop
  } else if (name == "answer") {
    stack.push_back(builder.getInt64(INT2FIX(42)));
  } else {
    fprintf(stderr, "unexpected instruction at CompileInstruction: %s\n", name.c_str());
    return false;
  }
  return true;
}

void NativeCompiler::CompileFuncall(llvm::Function *rb_funcallf, llvm::Value *op_sym, int argc) {
  size_t last = stack.size() - 1;
  std::vector<llvm::Value*> args = { stack[last-argc], op_sym, builder.getInt32(argc) };
  for (int i = 0; i < argc; i++) {
    args.push_back(stack[last-i]);
  }
  for (int i = 0; i <= argc; i++) stack.pop_back();

  llvm::Value* result = builder.CreateCall(rb_funcallf, args, "rb_funcall");
  stack.push_back(result);
}

llvm::Value* NativeCompiler::CompileObject(const Object& object) {
  if (object.klass == "NilClass") {
    return builder.getInt64(Qnil);
  } else if (object.klass == "Fixnum") {
    return builder.getInt64(INT2FIX(object.integer));
  } else if (object.klass == "TrueClass") {
    return builder.getInt64(Qtrue);
  } else if (object.klass == "FalseClass") {
    return builder.getInt64(Qfalse);
  } else {
    fprintf(stderr, "unexpected object.klass at CompileObject: %s\n", object.klass.c_str());
    return nullptr;
  }
}

} // namespace llruby
