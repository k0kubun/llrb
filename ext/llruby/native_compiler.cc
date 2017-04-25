#include "llruby/iseq.h"
#include "llruby/native_compiler.h"

namespace llruby {

NativeCompiler::NativeCompiler() : builder(context) {
  mod = llvm::make_unique<llvm::Module>("llruby", context);
}

uint64_t NativeCompiler::Compile(const Iseq& iseq) {
  return 0;
}

} // namespace llruby
