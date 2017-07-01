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
      return false if iseqw.nil? # method defined with C function can't be compiled

      compile_iseq(iseqw)
    end

    # Preview compiled method in LLVM IR
    #
    # @param [Object] recv - receiver of method to be compiled
    # @param [String,Symbol] method_name - precompiled method name
    def self.preview(recv, name)
      method = recv.method(name)
      iseqw = RubyVM::InstructionSequence.of(method)
      return false if iseqw.nil?

      puts "#{iseqw.disasm}\n"
      preview_iseq(iseqw)
    end

    def self.compiled?(recv, name)
      method = recv.method(name)
      iseqw = RubyVM::InstructionSequence.of(method)
      return false if iseqw.nil?

      is_compiled(iseqw)
    end

    # Followings are defined in ext/llrb/llrb.cc

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    # private_class_method :compile_iseq

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    # private_class_method :preview_iseq

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    # private_class_method :is_compiled
  end
end
