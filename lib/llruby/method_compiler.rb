require 'llruby/iseq_compiler'

module LLRuby
  module MethodCompiler
    # @param  [Method] method
    # @return [LLRuby::LLVMIR]
    def self.compile(method)
      unless method.is_a?(Method)
        raise ArgumentError.new("wrong argument type #{method.class} is given! (expected Method)")
      end

      iseq = RubyVM::InstructionSequence.of(method)
      IseqCompiler.compile(iseq)
    end
  end
end
