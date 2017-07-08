require 'llrb/llrb'

module LLRB
  module JIT
    # Compile method to native code
    #
    # @param [Object] recv - receiver of method to be compiled
    # @param [String,Symbol] method - precompiled method name
    # @return [Boolean] - return true if precompiled
    def self.compile(recv, name)
      compile_proc(recv.method(name))
    end

    def self.compile_proc(func)
      iseqw = RubyVM::InstructionSequence.of(func)
      return false if iseqw.nil? # method defined with C function can't be compiled

      #puts "#{iseqw.disasm}\n"
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

    def self.start
      hook_stop
      start_internal
    end

    # Hook JIT stop at exit to safely shutdown Ruby VM. JIT touches Ruby VM and
    # it should not run after Ruby VM destruction. Otherwise it will cause SEGV.
    def self.hook_stop
      return if defined?(@hook_stop)
      @hook_stop = true
      at_exit { LLRB::JIT.stop }
    end
    private_class_method :hook_stop

    # Followings are defined in ext/llrb/llrb.cc

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    private_class_method :compile_iseq

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    private_class_method :preview_iseq

    # @param  [RubyVM::InstructionSequence] iseqw - RubyVM::InstructionSequence instance
    # @return [Boolean] return true if compiled
    private_class_method :is_compiled

    # This does not hook stop, but it may cause SEGV if JIT runs after Ruby VM is shut down.
    # To ensure JIT will be stopped on exit, you should use .start instead.
    # @return [Boolean] return true if started JIT
    private_class_method :start_internal
  end
end
