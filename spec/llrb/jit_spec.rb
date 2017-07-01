RSpec.describe LLRB::JIT do
  #describe '.compile' do
  #  it 'compiles class method' do
  #    klass = Class.new
  #    def klass.hello
  #      100
  #    end

  #    expect {
  #      LLRB::JIT.compile(klass, :hello)
  #    }.to change {
  #      RubyVM::InstructionSequence.of(klass.method(:hello)).to_a
  #    }
  #    klass.hello
  #  end

  #  it 'compiles instance method' do
  #    klass = Class.new
  #    klass.class_eval do
  #      def hello; 100; end
  #    end
  #    object = klass.new

  #    expect {
  #      LLRB::JIT.compile(object, :hello)
  #    }.to change {
  #      RubyVM::InstructionSequence.of(object.method(:hello)).to_a
  #    }
  #    object.hello
  #  end

  #  it 'is callable multiple times' do
  #    klass = Class.new
  #    def klass.hello
  #      100
  #    end
  #    expect(LLRB::JIT.compile(klass, :hello)).to eq(true)
  #    expect(LLRB::JIT.compile(klass, :hello)).to eq(false)
  #  end

  #  it 'rejects to compile method defined by C' do
  #    expect(LLRB::JIT.compile('', :prepend)).to eq(false)
  #  end
  #end

  #describe '.compiled?' do
  #  it 'returns true if the method is already compiled' do
  #    klass = Class.new
  #    def klass.hello
  #      100
  #    end
  #    expect(LLRB::JIT.compiled?(klass, :hello)).to eq(false)
  #    expect(LLRB::JIT.compile(klass, :hello)).to eq(true)
  #    expect(LLRB::JIT.compiled?(klass, :hello)).to eq(true)
  #  end
  #end
end
