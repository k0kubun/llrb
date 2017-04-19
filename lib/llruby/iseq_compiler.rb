require 'llruby/llruby'

module LLRuby
  module IseqCompiler
    # @param  [RubyVM::InstructionSequence] iseq
    # @return [LLRuby::LLVMIR]
    def self.compile(iseq)
      unless iseq.is_a?(RubyVM::InstructionSequence)
        raise ArgumentError.new("wrong argument type #{iseq.class} is given! (expected RubyVM::InstructionSequence)")
      end
      compile_internal(iseq.to_a)
    end

    # defined in ext/llruby/llruby.cc
    private_class_method :compile_internal
  end
end
