describe LLRuby::JIT do
  describe '.precompile' do
    it 'compiles instance method' do
      klass = Class.new
      def klass.hello
        100
      end
      LLRuby::JIT.precompile(klass, :hello)
      klass.hello
    end

    it 'compiles class method' do
      klass = Class.new
      klass.class_eval do
        def hello; 100; end
      end
      object = klass.new
      LLRuby::JIT.precompile(object, :hello)
      object.hello
    end
  end

  describe '.precompile_internal' do
    it 'rejects non-Array argument' do
      object = Object.new
      expect {
        LLRuby::JIT.send(:precompile_internal, object, Object, :hash)
      }.to raise_error(TypeError)
    end
  end
end
