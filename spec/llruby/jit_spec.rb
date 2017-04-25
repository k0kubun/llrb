describe LLRuby::JIT do
  describe '.precompile' do
    it 'compiles class method' do
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

    it 'compiles instance method' do
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

    it 'is callable multiple times' do
      klass = Class.new
      def klass.hello
        100
      end
      expect(LLRuby::JIT.precompile(klass, :hello)).to eq(true)
      expect(LLRuby::JIT.precompile(klass, :hello)).to eq(false)
    end
  end

  describe '.preview' do
    it 'dumps LLVM IR and does not redefine method' do
      klass = Class.new
      def klass.hello
        100
      end

      expect {
        LLRuby::JIT.preview(klass, :hello)
      }.to_not change {
        klass.method(:hello).hash
      }
      klass.hello
    end
  end

  describe '.precompile_internal' do
    it 'rejects non-Array argument' do
      object = Object.new
      expect {
        LLRuby::JIT.send(:precompile_internal, object, Object, :hash, 0, false)
      }.to raise_error(TypeError)
    end
  end
end
