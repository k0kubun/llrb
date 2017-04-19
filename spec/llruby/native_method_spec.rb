describe LLRuby::NativeMethod do
  describe '#define' do
    it 'can be called' do
      LLRuby::NativeMethod.new.define(LLRuby, 'hello')
    end
  end
end
