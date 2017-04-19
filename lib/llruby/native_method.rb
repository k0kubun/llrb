module LLRuby
  class NativeMethod
    # @param [LLRuby::LLVMIR]
    def initialize(llvmir = nil) # drop nil once LLVMIR is created
      @llvmir = llvmir
    end

    # @param [Class,Module] klass
    # @param [String,Symbol] method_name
    def define(klass, method_name)
    end
  end
end
