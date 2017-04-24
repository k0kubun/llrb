require 'llruby/llruby'

module LLRuby
  class NativeMethod
    # @param [LLRuby::LLVMFunction] llvmfunc
    def initialize(llvmfunc)
      @llvmfunc = llvmfunc
    end

    # @param [Class,Module] klass
    # @param [String,Symbol] method_name
    def define(klass, method_name)
      define_internal(@llvmfunc, klass, method_name)
    end

    # defined in ext/llruby/llruby.cc
    private :define_internal
  end
end
