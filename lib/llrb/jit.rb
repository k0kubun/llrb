require 'llrb/llrb'

module LLRB
  module JIT
    # Precompile method to native code
    #
    # @param [Object] receiver - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    # @param [Boolean] dry_run - don't precompile and just dump LLVM IR if true
    # @return [Boolean] - return true if precompiled
    def self.precompile(receiver, method_name, dry_run: false)
      original = receiver.method(method_name)
      iseq = RubyVM::InstructionSequence.of(original)
      return false if iseq.nil?
      precompile_internal(iseq.to_a, original.owner, original.original_name, original.arity, dry_run)
    end

    # Preview compiled method in LLVM IR
    #
    # @param [Object] receiver - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    def self.preview(receiver, method_name)
      precompile(receiver, method_name, dry_run: true)
    end

    # defined in ext/llrb/llrb.cc
    #
    # @param  [Array]   iseq_array - result of RubyVM::InstructionSequence#to_a
    # @param  [Class]   klass      - class to define method
    # @param  [Symbol]  method_sym - method name to define
    # @param  [Integer] arity      - method arity
    # @param  [Boolean] dry_run    - don't precompile and just dump LLVM IR if true
    # @return [Boolean] return true if precompiled
    private_class_method :precompile_internal
  end
end
