#ifndef LLRB_NATIVE_COMPILER_H
#define LLRB_NATIVE_COMPILER_H

#include <vector>
#include "iseq.h"
#include "parser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

namespace llrb {
  class Compiler {
   private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;

   public:
    Compiler() : builder(context) {}
    uint64_t Compile(const Iseq& iseq);
    uint64_t Compile(const Iseq& iseq, bool dry_run);

   private:
    llvm::Function* CompileIseq(llvm::Module *mod, const Iseq& iseq);
    llvm::Value* CompileParsedBytecode(llvm::Module *mod, const std::vector<Entry>& parsed, int arg_size, int local_size);
    bool CompileInstruction(llvm::Module *mod, std::vector<llvm::Value*>& stack, const Entry& insn_entry, int arg_size, int local_size); // destructive for stack
    llvm::Value* CompileNewArray(llvm::Module *mod, const std::vector<llvm::Value*>& objects);
    llvm::Value* CompileDupArray(llvm::Module *mod, const std::vector<Object>& instruction);
    llvm::Value* CompileFuncall(llvm::Module *mod, std::vector<llvm::Value*>& stack, llvm::Value *op_sym, int argc); // destructive for stack
    llvm::Value* CompilePutSpecialObject(llvm::Module *mod, int type);
    llvm::Value* CompileBranchUnless(llvm::Module *mod, llvm::Value *cond, const std::vector<Entry>& fallthrough, const std::vector<Entry>& branched, int arg_size, int local_size);
    llvm::Value* BuildRTEST(llvm::Value *value);
    void DeclareCRubyAPIs(llvm::Module *mod);
    uint64_t CreateNativeFunction(std::unique_ptr<llvm::Module> mod, llvm::Function *func);
    llvm::Value* ArgumentAt(llvm::Module *mod, int index);
    llvm::Value* PopBack(std::vector<llvm::Value*>& stack); // destructive for stack
    std::vector<llvm::Value*> PopLast(std::vector<llvm::Value*>& stack, int num); // destructive for stack
  };
}

#endif // LLRB_NATIVE_COMPILER_H
