#include <memory>
#include <algorithm>
#include "iseq.h"
#include "llrb.h"
#include "compiler.h"
#include "parser.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"

extern VALUE rb_mLLRBFrozenCore;

namespace llrb {

uint64_t Compiler::Compile(const Iseq& iseq) {
  return Compile(iseq, false);
}

uint64_t Compiler::Compile(const Iseq& iseq, bool dry_run) {
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

uint64_t Compiler::CreateNativeFunction(std::unique_ptr<llvm::Module> mod, llvm::Function *func) {
  llvm::ExecutionEngine *engine = llvm::EngineBuilder(std::move(mod)).create();
  if (engine == NULL) {
    fprintf(stderr, "Failed to create ExecutionEngine...\n");
    return 0;
  }
  return engine->getFunctionAddress(func->getName());
}

llvm::Function* Compiler::CompileIseq(llvm::Module *mod, const Iseq& iseq) {
  std::vector<Entry> parsed = Parser(iseq.bytecode).Parse();
  if (parsed.empty()) return nullptr;
  //Entry::Dump(parsed); // for debug

  std::vector<llvm::Type*> arg_types = { llvm::IntegerType::get(context, 64) };
  for (int i = 0; i < iseq.arg_size; i++) {
    arg_types.push_back(llvm::IntegerType::get(context, 64));
  }

  llvm::Function *func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getInt64Ty(context), arg_types, false),
      llvm::Function::ExternalLinkage, "precompiled_method", mod);
  builder.SetInsertPoint(llvm::BasicBlock::Create(context, "entry", func));

  llvm::Value *result = CompileParsedBytecode(mod, parsed, iseq.arg_size, iseq.local_size);
  if (result == nullptr) return nullptr;
  builder.CreateRet(result);
  return func;
}

llvm::Value* Compiler::CompileParsedBytecode(llvm::Module *mod, const std::vector<Entry>& parsed, int arg_size, int local_size) {
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

// This function checks null and prevents SEGV. Normally we can't use mod->getFunction with CreateCall casually.
llvm::Function* Compiler::GetFunction(llvm::Module *mod, const std::string& name) {
  llvm::Function *func = mod->getFunction(name);
  if (func) return func;

  if (name == "rb_funcall") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 32)},
          true),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_ary_new_from_args") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, true),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_ary_resurrect") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_str_resurrect") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_obj_as_string") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_hash_new") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_hash_aset") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64)},
          false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_str_freeze") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), { llvm::IntegerType::get(context, 64)}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_range_new") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 32)},
          false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "rb_ivar_set") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64)},
          false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "llrb_insn_bitblt") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {}, false),
        llvm::Function::ExternalLinkage, name, mod);
  } else if (name == "llrb_insn_splatarray") {
    func = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt64Ty(context), {
          llvm::IntegerType::get(context, 64),
          llvm::IntegerType::get(context, 64)},
          false),
        llvm::Function::ExternalLinkage, name, mod);
  } else {
    rb_raise(rb_eRuntimeError, "'%s' is not defined in GetFunction", name.c_str());
  }
  return func;
}

// destructive for stack
// TODO: Change to take Environment for locals: arg_size, local_size
bool Compiler::CompileInstruction(llvm::Module *mod, std::vector<llvm::Value*>& stack, const Entry& insn_entry, int arg_size, int local_size) {
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
    stack.push_back(builder.CreateCall(GetFunction(mod, "rb_str_resurrect"), args, "putstring"));
  } else if (name == "tostring") {
    std::vector<llvm::Value*> args = { PopBack(stack) };
    stack.push_back(builder.CreateCall(GetFunction(mod, "rb_obj_as_string"), args, "tostring"));
  } else if (name == "freezestring") { // TODO: check debug info
    std::vector<llvm::Value*> args = { PopBack(stack) };
    stack.push_back(builder.CreateCall(GetFunction(mod, "rb_str_freeze"), args, "freezestring"));
  } else if (name == "opt_send_without_block") {
    std::map<std::string, Object> options = instruction[1].hash;
    Object mid = options["mid"];
    Object orig_argc = options["orig_argc"];
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern(mid.symbol.c_str())), orig_argc.integer));
  } else if (name == "newarray") {
    stack.push_back(CompileNewArray(mod, PopLast(stack, instruction[1].integer)));
  } else if (name == "duparray") {
    stack.push_back(CompileDupArray(mod, instruction));
  } else if (name == "splatarray") {
    std::vector<llvm::Value*> args = { PopBack(stack), builder.getInt64(instruction[1].raw) };
    stack.push_back(builder.CreateCall(GetFunction(mod, "llrb_insn_splatarray"), args, "splatarray"));
  } else if (name == "newhash") {
    stack.push_back(CompileNewHash(mod, PopLast(stack, instruction[1].integer)));
  } else if (name == "newrange") {
    stack.push_back(CompileNewRange(mod, instruction, PopLast(stack, 2)));
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
  } else if (name == "opt_regexpmatch1") {
    stack.push_back(builder.getInt64(instruction[1].raw));
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("=~")), 1));
  } else if (name == "opt_regexpmatch2") {
    stack.push_back(CompileFuncall(mod, stack, builder.getInt64(rb_intern("=~")), 1));
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
  } else if (name == "bitblt") {
    stack.push_back(builder.CreateCall(GetFunction(mod, "llrb_insn_bitblt"), {}, "bitblt"));
  } else if (name == "answer") {
    stack.push_back(builder.getInt64(INT2FIX(42)));
  } else {
    fprintf(stderr, "unexpected instruction at CompileInstruction: %s\n", name.c_str());
    return false;
  }
  return stack.size() == 0 || stack.back() != nullptr;
}

llvm::Value* Compiler::CompileNewArray(llvm::Module* mod, const std::vector<llvm::Value*>& objects) {
  std::vector<llvm::Value*> args = { builder.getInt64(objects.size()) };
  args.insert(args.end(), objects.begin(), objects.end());

  return builder.CreateCall(GetFunction(mod, "rb_ary_new_from_args"), args, "newarray");
}

llvm::Value* Compiler::CompileDupArray(llvm::Module *mod, const std::vector<Object>& instruction) {
  std::vector<llvm::Value*> args = { builder.getInt64(instruction[1].raw) };
  return builder.CreateCall(GetFunction(mod, "rb_ary_resurrect"), args, "duparray");
}

llvm::Value* Compiler::CompileNewHash(llvm::Module *mod, const std::vector<llvm::Value*>& objects) {
  llvm::Value *result = builder.CreateCall(GetFunction(mod, "rb_hash_new"), {}, "newhash");
  for (size_t i = 0; i < objects.size() / 2; i++) {
    std::vector<llvm::Value*> args = { result, objects[i*2], objects[i*2+1] };
    builder.CreateCall(GetFunction(mod, "rb_hash_aset"), args, "newhash");
  }
  return result;
}

llvm::Value* Compiler::CompileNewRange(llvm::Module *mod, const std::vector<Object>& instruction, const std::vector<llvm::Value*>& objects) {
  llvm::Value *low  = objects[0];
  llvm::Value *high = objects[1];
  llvm::Value *flag = builder.getInt64(FIX2INT(instruction[1].raw));
  std::vector<llvm::Value*> args = { low, high, flag };
  return builder.CreateCall(GetFunction(mod, "rb_range_new"), args, "newrange");
}

// destructive for stack
llvm::Value* Compiler::CompileFuncall(llvm::Module *mod, std::vector<llvm::Value*>& stack, llvm::Value *op_sym, int argc) {
  std::vector<llvm::Value*> rest = PopLast(stack, argc);
  std::vector<llvm::Value*> args = { PopBack(stack), op_sym, builder.getInt32(argc) };
  args.insert(args.end(), rest.begin(), rest.end());

  return builder.CreateCall(GetFunction(mod, "rb_funcall"), args, "funcall");
}

llvm::Value* Compiler::CompilePutSpecialObject(llvm::Module *mod, int type) {
  if (type == 1) {
    // Workaround to set self for core#define_method.
    // FIXME: Obviously this isn't thread safe...
    std::vector<llvm::Value*> args = {
      builder.getInt64(rb_mLLRBFrozenCore),
      builder.getInt64(rb_intern("__llrb_self__")),
      ArgumentAt(mod, 0)
    };
    builder.CreateCall(GetFunction(mod, "rb_ivar_set"), args, "specialobject_self");

    return builder.getInt64(rb_mLLRBFrozenCore);
  } else {
    fprintf(stderr, "unhandled putspecialconst!: %d\n", type);
    return nullptr;
  }
}

// TODO: Change to take Environment for locals: arg_size, local_size
llvm::Value* Compiler::CompileBranchUnless(llvm::Module *mod, llvm::Value *cond, const std::vector<Entry>& fallthrough, const std::vector<Entry>& branched, int arg_size, int local_size) {
  llvm::Function* func = GetFunction(mod, "precompiled_method");

  llvm::BasicBlock *fallth_b = llvm::BasicBlock::Create(context, "fallthrough", func);
  llvm::BasicBlock *branch_b = llvm::BasicBlock::Create(context, "branched");
  llvm::BasicBlock *merged_b = llvm::BasicBlock::Create(context, "merged");
  builder.CreateCondBr(BuildRTEST(cond), fallth_b, branch_b);

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

// In base 2, RTEST is: (v != Qfalse && v != Qnil) -> (v != 0000 && v != 0100) -> (v & 1011) != 0000 -> (v & ~Qnil) != 0
llvm::Value* Compiler::BuildRTEST(llvm::Value *value) {
  llvm::Value *masked = builder.CreateAnd(value, builder.getInt64(~Qnil));
  return builder.CreateICmpNE(masked, builder.getInt64(0), "RTEST");
}

llvm::Value* Compiler::ArgumentAt(llvm::Module *mod, int index) {
  int i = 0;
  for (llvm::Value &arg : GetFunction(mod, "precompiled_method")->args()) {
    if (i == index) {
      return &arg;
    }
    i++;
  }
  return nullptr;
}

// destructive for stack
llvm::Value* Compiler::PopBack(std::vector<llvm::Value*>& stack) {
  if (stack.size() < 1) {
    rb_raise(rb_eRuntimeError, "stack size underflow");
  }
  llvm::Value* ret = stack.back();
  stack.pop_back();
  return ret;
}

// destructive for stack
std::vector<llvm::Value*> Compiler::PopLast(std::vector<llvm::Value*>& stack, int num) {
  std::vector<llvm::Value*> ret;
  for (int i = 0; i < num; i++) {
    ret.push_back(PopBack(stack));
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

} // namespace llrb
