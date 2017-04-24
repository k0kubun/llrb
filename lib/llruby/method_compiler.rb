require 'llruby/iseq_compiler'
require 'llruby/native_method'

module LLRuby
  module MethodCompiler
    # @param  [Method] method
    # @return [LLRuby::NativeMethod]
    def self.compile(method)
      unless method.is_a?(Method)
        raise ArgumentError.new("wrong argument type #{method.class} is given! (expected Method)")
      end

      iseq = RubyVM::InstructionSequence.of(method)
      llvmir = IseqCompiler.compile(iseq)
      NativeMethod.new(llvmir)
    end
  end
end
