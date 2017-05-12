describe 'llruby::NativeCompiler' do
  def test_compile(&block)
    klass = Class.new
    klass.send(:define_singleton_method, :test, &block)
    result = klass.test
    expect(LLRuby::JIT.precompile(klass, :test)).to eq(true)
    expect(klass.test).to eq(result)
  end

  specify 'putnil' do
    test_compile { nil }
  end

  specify 'putobject' do
    test_compile { true }
    test_compile { false }
    test_compile { 100 }
    test_compile { 0 }
    test_compile { 1 }
  end

  specify 'newarray' do
    test_compile { [] }
    test_compile { [[], nil, 1] }
  end

  specify 'opt_plus' do
    test_compile { 1 + 2 + 3 }
    test_compile { [nil] + [nil] }
  end

  specify 'opt_minus' do
    test_compile { 1 - 3 - 2 }
  end

  specify 'opt_mult' do
    test_compile { 3 * 2 * 1 }
  end

  specify 'opt_div' do
    test_compile { 3 / 2 / 1 }
  end

  specify 'opt_mod' do
    test_compile { 3 % 2 % 1 }
  end

  specify 'opt_eq' do
    test_compile { 2 == 2 }
    test_compile { 3 == 2 }
  end

  specify 'opt_neq' do
    test_compile { 2 != 2 }
    test_compile { 3 != 2 }
  end

  specify 'opt_lt' do
    test_compile { 2 < 3 }
    test_compile { 2 < 2 }
    test_compile { 3 < 2 }
  end

  specify 'opt_le' do
    test_compile { 2 <= 3 }
    test_compile { 2 <= 2 }
    test_compile { 3 <= 2 }
  end

  specify 'opt_gt' do
    test_compile { 2 > 3 }
    test_compile { 2 > 2 }
    test_compile { 3 > 2 }
  end

  specify 'opt_ge' do
    test_compile { 2 >= 3 }
    test_compile { 2 >= 2 }
    test_compile { 3 >= 2 }
  end

  specify 'opt_succ' do
    test_compile { 2.succ }
  end

  specify 'opt_not' do
    test_compile { !nil }
    test_compile { !true }
    test_compile { !false }
    test_compile { !100 }
    test_compile { nil.! }
    test_compile { true.! }
    test_compile { false.! }
    test_compile { 100.! }
  end

  specify 'opt_send_without_block' do
    test_compile { 2 ** 3 }
    test_compile { 2.even? }
    test_compile { [nil].push(3) }
  end

  describe 'integration' do
    specify 'arithmetic' do
      test_compile { 1 + (3 - 4) * 5 / 2 }
    end
  end
end
