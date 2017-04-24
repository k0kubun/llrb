describe LLRuby::NativeMethod do
  describe '#define' do
    it 'defines callable native method' do
      klass = Class.new
      klass.class_eval do
        def old_hello
        end
      end

      iseq = RubyVM::InstructionSequence.of(klass.new.method(:old_hello))
      func = LLRuby::IseqCompiler.compile(iseq)

      native_method = LLRuby::NativeMethod.new(func)
      native_method.define(klass, 'new_hello')
      klass.new.new_hello
    end
  end
end
