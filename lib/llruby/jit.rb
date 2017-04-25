require 'llruby/llruby'

module LLRuby
  module JIT
    # @param [Object] receiver - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    def self.precompile(receiver, method_name)
      ruby_method = receiver.method(method_name)
      iseq = RubyVM::InstructionSequence.of(ruby_method)
      precompile_internal(iseq.to_a, ruby_method.owner, ruby_method.original_name)
    end

    # defined in ext/llruby/llruby.cc
    #
    # @param [Array]  iseq_array - result of RubyVM::InstructionSequence#to_a
    # @param [Class]  klass      - class to define method
    # @param [Symbol] method_sym - method name to define
    private_class_method :precompile_internal
  end
end
