describe LLRuby::JIT do
  describe '.precompile' do
    it 'compiles instance method' do
      klass = Class.new
      def klass.hello
        100
      end

      expect {
        LLRuby::JIT.precompile(klass, :hello)
      }.to change {
        klass.method(:hello).hash
      }
      klass.hello
    end

    it 'compiles class method' do
      klass = Class.new
      klass.class_eval do
        def hello; 100; end
      end
      object = klass.new

      expect {
        LLRuby::JIT.precompile(object, :hello)
      }.to change {
        object.method(:hello).hash
      }
      object.hello
    end
  end

  describe '.precompile_internal' do
    it 'rejects non-Array argument' do
      object = Object.new
      expect {
        LLRuby::JIT.send(:precompile_internal, object, Object, :hash, 0)
      }.to raise_error(TypeError)
    end
  end
end
