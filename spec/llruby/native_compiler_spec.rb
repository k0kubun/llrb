describe 'llruby::NativeCompiler' do
  def test_compile(&block)
    klass = Class.new
    klass.send(:define_singleton_method, :test, &block)
    result = klass.test
    expect(LLRuby::JIT.precompile(klass, :test)).to eq(true)
    expect(klass.test).to eq(result)
  end

  it 'compiles putnil' do
    test_compile { nil }
  end

  it 'compiles putobject' do
    test_compile { true }
    test_compile { false }
    test_compile { 100 }
  end

  it 'compiles opt_plus' do
    test_compile { 2 + 3 }
  end

  it 'compiles opt_minus' do
    test_compile { 3 - 2 }
    test_compile { 3 + 4 - 2 }
  end
end
