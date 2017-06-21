RSpec.describe LLRB::JIT do
  describe '.compile' do
    #it 'compiles class method' do
    #  klass = Class.new
    #  def klass.hello
    #    100
    #  end

    #  expect {
    #    LLRB::JIT.compile(klass, :hello)
    #  }.to change {
    #    klass.method(:hello).hash
    #  }
    #  klass.hello
    #end

    #it 'compiles instance method' do
    #  klass = Class.new
    #  klass.class_eval do
    #    def hello; 100; end
    #  end
    #  object = klass.new

    #  expect {
    #    LLRB::JIT.compile(object, :hello)
    #  }.to change {
    #    object.method(:hello).hash
    #  }
    #  object.hello
    #end

    #it 'is callable multiple times' do
    #  klass = Class.new
    #  def klass.hello
    #    100
    #  end
    #  expect(LLRB::JIT.compile(klass, :hello)).to eq(true)
    #  expect(LLRB::JIT.compile(klass, :hello)).to eq(false)
    #end
  end
end
