describe LLRuby::NativeMethod do
  describe '#define' do
    it 'defines callable native method' do
      klass = Class.new
      LLRuby::NativeMethod.new.define(klass, 'hello')
      klass.new.hello
    end
  end
end
