require 'llrb/llrb'

module LLRB
  module JIT
    # Compile method to native code
    #
    # @param [Object] recv - receiver of method to be compiled
    # @param [String,Symbol] method - precompiled method name
    # @return [Boolean] - return true if precompiled
    def self.compile(recv, name)
      method = recv.method(name)
      iseqw = RubyVM::InstructionSequence.of(method)
      return false if iseqw.nil?

      compile_iseq(iseqw, recv, method.owner, method.original_name, method.arity)
    end

    # Preview compiled method in LLVM IR
    #
    # @param [Object] recv - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    def self.preview(recv, name)
      method = recv.method(name)
      iseqw = RubyVM::InstructionSequence.of(method)
      return false if iseqw.nil?

      preview_iseq(iseqw, recv)
    end

    def self.compile_defined()
    end

    # Followings are defined in ext/llrb/llrb.cc

    # @param  [Array]   iseqw  - RubyVM::InstructionSequence instance
    # @param  [Object]  recv   - method receiver
    # @param  [Class]   klass  - method class
    # @param  [Symbol]  method - method name to define
    # @param  [Integer] arity  - method arity
    # @return [Boolean] return true if compiled
    private_class_method :compile_iseq

    # @param  [Array]   iseqw - RubyVM::InstructionSequence instance
    # @param  [Object]  recv  - method receiver
    # @return [Boolean] return true if compiled
    private_class_method :preview_iseq
  end
end
