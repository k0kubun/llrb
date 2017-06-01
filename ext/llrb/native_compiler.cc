#include <memory>
#include <algorithm>
#include "iseq.h"
#include "llrb.h"
#include "native_compiler.h"
#include "parser.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"

extern VALUE rb_mLLRBFrozenCore;

namespace llrb {

uint64_t NativeCompiler::Compile(const Iseq& iseq) {
  return Compile(iseq, false);
}

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
  std::vector<Entry> parsed = Parser(iseq.bytecode).Parse();
  if (parsed.empty()) return nullptr;
  //Entry::Dump(parsed); // for debug

  DeclareCRubyAPIs(mod);
  std::vector<llvm::Type*> arg_types = { llvm::IntegerType::get(context, 64) };
  for (int i = 0; i < iseq.arg_size; i++) {
    arg_types.push_back(llvm::IntegerType::get(context, 64));
  }

  llvm::Function *func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), arg_types, false),
      llvm::Function::ExternalLinkage, "precompiled_method", mod);
  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", func));

  llvm::Value *result = CompileParsedBytecode(mod, parsed, iseq.arg_size, iseq.local_size);
  if (result == nullptr) {
    mod->dump();
    return nullptr;
  }
  builder.CreateRet(result);
  return func;
}

llvm::Value* NativeCompiler::CompileParsedBytecode(llvm::Module *mod, const std::vector<Entry>& parsed, int arg_size, int local_size) {
  std::vector<llvm::Value*> stack;
  for (const Entry& entry : parsed) {
    switch (entry.type) {
      case ENTRY_INSN: {
        bool succeeded = CompileInstruction(mod, stack, entry, arg_size, local_size);
        if (!succeeded) return nullptr;
        break;
      }
      case ENTRY_LABEL:
      case ENTRY_LINENO:
        // ignored for now
        break;
      default:
        fprintf(stderr, "unexpected entry.type at CompileParsedBytecode: %d\n", entry.type);
        return nullptr;
    }
  }

  if (stack.size() == 1) {
    return stack.back();
  } else {
    fprintf(stderr, "unexpected stack size at CompileParsedBytecode: %d\n", (int)stack.size());
    return nullptr;
  }
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
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
      llvm::Function::ExternalLinkage, "rb_str_resurrect", mod);
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
      llvm::Function::ExternalLinkage, "rb_obj_as_string", mod);
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
      llvm::Function::ExternalLinkage, "rb_str_freeze", mod);
  llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 64),
        llvm::IntegerType::get(context, 64)},
        true),
      llvm::Function::ExternalLinkage, "rb_ivar_set", mod);
}

// destructive for stack
// TODO: Change to take Environment for locals: arg_size, local_size
bool NativeCompiler::CompileInstruction(llvm::Module *mod, std::vector<llvm::Value*>& stack, const Entry& insn_entry, int arg_size, int local_size) {
  const std::vector<Object>& instruction = insn_entry.insn;
  const std::string& name = instruction[0].symbol;
  if (name == "getlocal_OP__WC__0") {
    stack.push_back(ArgumentAt(mod, 3 - local_size + 2 * arg_size - instruction[1].integer)); // XXX: is this okay????
  } else if (name == "putnil") {
    stack.push_back(builder.getInt64(Qnil));
  } else if (name == "putself") {
    stack.push_back(ArgumentAt(mod, 0));
  } else if (name == "putobject") {
    stack.push_back(builder.getInt64(instruction[1].raw));
  } else if (name == "putobject_OP_INT2FIX_O_0_C_") {
    stack.push_back(builder.getInt64(INT2FIX(0)));
  } else if (name == "putobject_OP_INT2FIX_O_1_C_") {
    stack.push_back(builder.getInt64(INT2FIX(1)));
  } else if (name == "putspecialobject") {
    stack.push_back(CompilePutSpecialObject(mod, instruction[1].integer));
  } else if (name == "putiseq") {
    stack.push_back(builder.getInt64(instruction[1].raw));
  } else if (name == "putstring") {
    std::vector<llvm::Value*> args = { builder.getInt64(instruction[1].raw) };
    llvm::Value* result = builder.CreateCall(mod->getFunction("rb_str_resurrect"), args, "putstring");
    stack.push_back(result);
  } else if (name == "tostring") {
    std::vector<llvm::Value*> args = { PopBack(stack) };
    llvm::Value* result = builder.CreateCall(mod->getFunction("rb_obj_as_string"), args, "tostring");
    stack.push_back(result);
  } else if (name == "freezestring") { // TODO: check debug info
    std::vector<llvm::Value*> args = { PopBack(stack) };
    llvm::Value* result = builder.CreateCall(mod->getFunction("rb_str_freeze"), args, "freezestring");
    stack.push_back(result);
  } else if (name == "opt_send_without_block") {
    std::map<std::string, Object> options = instruction[1].hash;
    Object mid = options["mid"];
    Object orig_argc = options["orig_argc"];
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern(mid.symbol.c_str())), orig_argc.integer));
  } else if (name == "newarray") {
    stack.push_back(CompileNewArray(mod, PopLast(stack, instruction[1].integer)));
  } else if (name == "duparray") {
    stack.push_back(CompileDupArray(mod, instruction));
  } else if (name == "splatarray") { // TODO: implement full vm_splat_array
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("to_a")), 0));
  } else if (name == "pop") {
    stack.pop_back();
  } else if (name == "setn") {
    int last = (int)stack.size() - 1;
    stack[last - instruction[1].integer] = stack.back();
  } else if (name == "opt_str_freeze" || name == "opt_str_uminus") {
    stack.push_back(builder.getInt64(instruction[1].raw)); // TODO: detect redefinition
  } else if (name == "opt_newarray_max") {
    stack.push_back(CompileNewArray(mod, PopLast(stack, instruction[1].integer)));
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("max")), 0));
  } else if (name == "opt_newarray_min") {
    stack.push_back(CompileNewArray(mod, PopLast(stack, instruction[1].integer)));
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("min")), 0));
  } else if (name == "opt_plus") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('+'), 1));
  } else if (name == "opt_minus") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('-'), 1));
  } else if (name == "opt_mult") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('*'), 1));
  } else if (name == "opt_div") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('/'), 1));
  } else if (name == "opt_mod") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('%'), 1));
  } else if (name == "opt_eq") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("==")), 1));
  } else if (name == "opt_neq") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("!=")), 1));
  } else if (name == "opt_lt") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('<'), 1));
  } else if (name == "opt_le") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("<=")), 1));
  } else if (name == "opt_gt") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('>'), 1));
  } else if (name == "opt_ge") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern(">=")), 1));
  } else if (name == "opt_ltlt") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("<<")), 1));
  } else if (name == "opt_aref") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("[]")), 1));
  } else if (name == "opt_aset") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("[]=")), 2));
  } else if (name == "opt_length") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("length")), 0));
  } else if (name == "opt_size") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("size")), 0));
  } else if (name == "opt_empty_p") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("empty?")), 0));
  } else if (name == "opt_succ") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("succ")), 0));
  } else if (name == "opt_not") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64('!'), 0));
  } else if (name == "trace") {
    // ignored for now
  } else if (name == "leave") {
    // ignored for now
  } else if (name == "nop") {
    // nop
  } else if (name == "jump") {
    fprintf(stderr, "parsed tree had jump instruction: %s\n", instruction[1].symbol.c_str());
    return false;
  } else if (name == "branchunless") {
    stack.push_back(CompileBranchUnless(mod, PopBack(stack), insn_entry.fallthrough, insn_entry.branched, arg_size, local_size));
  } else if (name == "answer") {
    stack.push_back(builder.getInt64(INT2FIX(42)));
  } else {
    fprintf(stderr, "unexpected instruction at CompileInstruction: %s\n", name.c_str());
    return false;
  }
  return stack.size() == 0 || stack.back() != nullptr;
}

llvm::Value* NativeCompiler::CompileNewArray(llvm::Module* mod, const std::vector<llvm::Value*>& objects) {
  std::vector<llvm::Value*> args = { builder.getInt64(objects.size()) };
  args.insert(args.end(), objects.begin(), objects.end());

  return builder.CreateCall(mod->getFunction("rb_ary_new_from_args"), args, "newarray");
}

llvm::Value* NativeCompiler::CompileDupArray(llvm::Module* mod, const std::vector<Object>& instruction) {
  std::vector<llvm::Value*> args = { builder.getInt64(instruction[1].raw) };
  return builder.CreateCall(mod->getFunction("rb_ary_resurrect"), args, "duparray");
}

// destructive for stack
llvm::Value* NativeCompiler::CompileFuncall(llvm::Module *mod, std::vector<llvm::Value*>& stack, llvm::Value *op_sym, int argc) {
  std::vector<llvm::Value*> rest = PopLast(stack, argc);
  std::vector<llvm::Value*> args = { PopBack(stack), op_sym, builder.getInt32(argc) };
  args.insert(args.end(), rest.begin(), rest.end());

  return builder.CreateCall(mod->getFunction("rb_funcall"), args, "funcall");
}

llvm::Value* NativeCompiler::CompilePutSpecialObject(llvm::Module *mod, int type) {
  if (type == 1) {
    // Workaround to set self for core#define_method
    std::vector<llvm::Value*> args = {
      builder.getInt64(rb_mLLRBFrozenCore),
      builder.getInt64(rb_intern("__llrb_self__")),
      ArgumentAt(mod, 0)
    };
    builder.CreateCall(mod->getFunction("rb_ivar_set"), args, "specialobject_self");

    return builder.getInt64(rb_mLLRBFrozenCore);
  } else {
    fprintf(stderr, "unhandled putspecialconst!: %d\n", type);
    return nullptr;
  }
}

// TODO: Change to take Environment for locals: arg_size, local_size
llvm::Value* NativeCompiler::CompileBranchUnless(llvm::Module *mod, llvm::Value *cond_obj, const std::vector<Entry>& fallthrough, const std::vector<Entry>& branched, int arg_size, int local_size) {
  llvm::Function* func = mod->getFunction("precompiled_method");

  llvm::BasicBlock *fallth_b = llvm::BasicBlock::Create(context, "fallthrough", func);
  llvm::BasicBlock *branch_b = llvm::BasicBlock::Create(context, "branched");
  llvm::BasicBlock *merged_b = llvm::BasicBlock::Create(context, "merged");

  llvm::Value *cond = builder.CreateICmpNE(cond_obj, builder.getInt64(Qfalse), "branchunless"); // TODO: Implement RTEST
  builder.CreateCondBr(cond, fallth_b, branch_b);

  builder.SetInsertPoint(fallth_b);
  llvm::Value *fallth_result = CompileParsedBytecode(mod, fallthrough, arg_size, local_size);
  builder.CreateBr(merged_b);
  fallth_b = builder.GetInsertBlock();

  func->getBasicBlockList().push_back(branch_b);
  builder.SetInsertPoint(branch_b);
  llvm::Value *branch_result = CompileParsedBytecode(mod, branched, arg_size, local_size);
  builder.CreateBr(merged_b);
  branch_b = builder.GetInsertBlock();

  func->getBasicBlockList().push_back(merged_b);
  builder.SetInsertPoint(merged_b);

  llvm::PHINode *phi = builder.CreatePHI(llvm::Type::getInt64Ty(context), 2, "branchunless_result");
  phi->addIncoming(fallth_result, fallth_b);
  phi->addIncoming(branch_result, branch_b);
  return phi;
}

llvm::Value* NativeCompiler::ArgumentAt(llvm::Module *mod, int index) {
  int i = 0;
  for (llvm::Value &arg : mod->getFunction("precompiled_method")->args()) {
    if (i == index) {
      return &arg;
    }
    i++;
  }
  return nullptr;
}

// destructive for stack
llvm::Value* NativeCompiler::PopBack(std::vector<llvm::Value*>& stack) {
  llvm::Value* ret = stack.back();
  stack.pop_back();
  return ret;
}

// destructive for stack
std::vector<llvm::Value*> NativeCompiler::PopLast(std::vector<llvm::Value*>& stack, int num) {
  std::vector<llvm::Value*> ret;
  for (int i = 0; i < num; i++) {
    ret.push_back(PopBack(stack));
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

} // namespace llrb
