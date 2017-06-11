require 'llrb/llrb'

module LLRB
  module JIT
    # Iseq array will be used on runtime but it can be GCed. This ivar prevents it.
    # TODO: Find a way to do it from C++.
    @gc_guarded = []

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
      iseq_array = iseq.to_a
      @gc_guarded.concat(iseq_array.last.select { |insn, _| insn == :putiseq }.map(&:last)) unless dry_run
      precompile_internal(iseq_array, original.owner, original.original_name, original.arity, dry_run).tap do |succeeded|
        unless succeeded
          $stderr.puts "  => #{original.owner}##{method_name}"
          require 'pry'
          Pry::ColorPrinter.pp(iseq_array)
        end
      end
    end

    def self.precompile_all(recv)
      owned_methods = recv.methods.select do |m|
        recv.method(m).owner == recv.class
      end
      owned_methods.each do |m|
        return recv unless precompile(recv, m)
      end
      recv
    end

    # Preview compiled method in LLVM IR
    #
    # @param [Object] receiver - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    def self.preview(receiver, method_name)
      precompile(receiver, method_name, dry_run: true)
    end

    # defined in ext/llrb/llrb.cc
    # If dry_run is true, iseq_array is only used.
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
