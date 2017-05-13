#include <memory>
#include <algorithm>
#include "iseq.h"
#include "llrb.h"
#include "native_compiler.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#pragma clang diagnostic pop

namespace llrb {

uint64_t NativeCompiler::Compile(const Iseq& iseq, bool dry_run) {
  std::unique_ptr<llvm::Module> mod = llvm::make_unique<llvm::Module>("llrb", context);
  llvm::Function *func = CompileIseq(mod.get(), iseq);
  if (!func) return 0;

  if (dry_run) {
    mod->dump();
    return 0;
  } else {
    return CreateNativeFunction(std::move(mod), func);
  }
}

uint64_t NativeCompiler::CreateNativeFunction(std::unique_ptr<llvm::Module> mod, llvm::Function *func) {
  llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::move(mod)).create();
  if (engine == NULL) {
    fprintf(stderr, "Failed to create ExecutionEngine...\n");
    return 0;
  }
  return engine->getFunctionAddress(func->getName());
}

llvm::Function* NativeCompiler::CompileIseq(llvm::Module *mod, const Iseq& iseq) {
  DeclareCRubyAPIs(mod);
  llvm::Function *func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64) }, false),
      llvm::Function::ExternalLinkage, "precompiled_method", mod);

  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", func));
  stack.clear();

  for (const Object& insn : iseq.bytecode) {
    if (insn.klass == "Array") {
      bool compiled = CompileInstruction(mod, insn.array);
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

void NativeCompiler::DeclareCRubyAPIs(llvm::Module *mod) {
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 32)},
        true),
      llvm::Function::ExternalLinkage, "rb_funcall", mod);
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
      llvm::Function::ExternalLinkage, "rb_ary_new_from_args", mod);
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
      llvm::Function::ExternalLinkage, "rb_ary_resurrect", mod);
}

bool NativeCompiler::CompileInstruction(llvm::Module *mod, const std::vector<Object>& instruction) {
  const std::string& name = instruction[0].symbol;
  if (name == "putnil") {
    llvm::Value *result = CompileObject(Object(Qnil));
    if (!result) return false;
    stack.push_back(result);
  } else if (name == "putobject") {
    llvm::Value *result = CompileObject(instruction[1]);
    if (!result) return false;
    stack.push_back(result);
  } else if (name == "putobject_OP_INT2FIX_O_0_C_") {
    stack.push_back(builder.getInt64(INT2FIX(0)));
  } else if (name == "putobject_OP_INT2FIX_O_1_C_") {
    stack.push_back(builder.getInt64(INT2FIX(1)));
  } else if (name == "opt_send_without_block") {
    std::map<std::string, Object> options = instruction[1].hash;
    Object mid = options["mid"];
    Object orig_argc = options["orig_argc"];
    CompileFuncall(mod, builder.getInt64(rb_intern(mid.symbol.c_str())), orig_argc.integer);
  } else if (name == "newarray") {
    CompileNewArray(mod, instruction[1].integer);
  } else if (name == "duparray") {
    CompileDupArray(mod, instruction);
  } else if (name == "pop") {
    stack.pop_back();
  } else if (name == "setn") {
    int last = (int)stack.size() - 1;
    stack[last - instruction[1].integer] = stack.back();
  } else if (name == "opt_plus") {
    CompileFuncall(mod, builder.getInt64('+'), 1);
  } else if (name == "opt_minus") {
    CompileFuncall(mod, builder.getInt64('-'), 1);
  } else if (name == "opt_mult") {
    CompileFuncall(mod, builder.getInt64('*'), 1);
  } else if (name == "opt_div") {
    CompileFuncall(mod, builder.getInt64('/'), 1);
  } else if (name == "opt_mod") {
    CompileFuncall(mod, builder.getInt64('%'), 1);
  } else if (name == "opt_eq") {
    CompileFuncall(mod, builder.getInt64(rb_intern("==")), 1);
  } else if (name == "opt_neq") {
    CompileFuncall(mod, builder.getInt64(rb_intern("!=")), 1);
  } else if (name == "opt_lt") {
    CompileFuncall(mod, builder.getInt64('<'), 1);
  } else if (name == "opt_le") {
    CompileFuncall(mod, builder.getInt64(rb_intern("<=")), 1);
  } else if (name == "opt_gt") {
    CompileFuncall(mod, builder.getInt64('>'), 1);
  } else if (name == "opt_ge") {
    CompileFuncall(mod, builder.getInt64(rb_intern(">=")), 1);
  } else if (name == "opt_ltlt") {
    CompileFuncall(mod, builder.getInt64(rb_intern("<<")), 1);
  } else if (name == "opt_aref") {
    CompileFuncall(mod, builder.getInt64(rb_intern("[]")), 1);
  } else if (name == "opt_aset") {
    CompileFuncall(mod, builder.getInt64(rb_intern("[]=")), 2);
  } else if (name == "opt_succ") {
    CompileFuncall(mod, builder.getInt64(rb_intern("succ")), 0);
  } else if (name == "opt_not") {
    CompileFuncall(mod, builder.getInt64('!'), 0);
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

void NativeCompiler::CompileNewArray(llvm::Module* mod, int num) {
  std::vector<llvm::Value*> args = { builder.getInt64(num) };
  std::vector<llvm::Value*> rest = PopLast(num);
  args.insert(args.end(), rest.begin(), rest.end());

  llvm::Value* result = builder.CreateCall(mod->getFunction("rb_ary_new_from_args"), args, "newarray");
  stack.push_back(result);
}

// NOTE: Can we use external pointer from compiled function?
void NativeCompiler::CompileDupArray(llvm::Module* mod, const std::vector<Object>& instruction) {
  std::vector<llvm::Value*> args = { builder.getInt64(instruction[1].raw) };
  llvm::Value* result = builder.CreateCall(mod->getFunction("rb_ary_resurrect"), args, "duparray");
  stack.push_back(result);
}

void NativeCompiler::CompileFuncall(llvm::Module *mod, llvm::Value *op_sym, int argc) {
  std::vector<llvm::Value*> rest = PopLast(argc);
  std::vector<llvm::Value*> args = { PopBack(), op_sym, builder.getInt32(argc) };
  args.insert(args.end(), rest.begin(), rest.end());

  llvm::Value* result = builder.CreateCall(mod->getFunction("rb_funcall"), args, "funcall");
  stack.push_back(result);
}

llvm::Value* NativeCompiler::CompileObject(const Object& object) {
  if (object.klass == "NilClass") {
    return builder.getInt64(Qnil);
  } else if (object.klass == "Fixnum") {
    return builder.getInt64(INT2FIX(object.integer));
  } else if (object.klass == "Symbol") {
    return builder.getInt64(object.raw);
  } else if (object.klass == "TrueClass") {
    return builder.getInt64(Qtrue);
  } else if (object.klass == "FalseClass") {
    return builder.getInt64(Qfalse);
  } else {
    fprintf(stderr, "unexpected object.klass at CompileObject: %s\n", object.klass.c_str());
    return nullptr;
  }
}

llvm::Value* NativeCompiler::PopBack() {
  llvm::Value* ret = stack.back();
  stack.pop_back();
  return ret;
}

std::vector<llvm::Value*> NativeCompiler::PopLast(int num) {
  std::vector<llvm::Value*> ret;
  for (int i = 0; i < num; i++) {
    ret.push_back(PopBack());
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

} // namespace llrb
