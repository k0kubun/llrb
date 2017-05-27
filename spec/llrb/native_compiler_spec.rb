describe 'llrb::NativeCompiler' do
  def test_compile(&block)
    klass = Class.new
    klass.send(:define_singleton_method, :test, &block)
    result = klass.test
    expect(LLRB::JIT.precompile(klass, :test)).to eq(true)
    expect(klass.test).to eq(result)
  end

  specify 'putnil' do
    test_compile { nil }
  end

  specify 'putself' do
    test_compile { print }
  end

  specify 'putobject' do
    test_compile { true }
    test_compile { false }
    test_compile { 100 }
    test_compile { 0 }
    test_compile { 1 }
    test_compile { :hello }
  end

  specify 'putstring' do
    test_compile { "hello" }
  end

  # specify 'concatstrings' do
  #   test_compile { "h#{2}o" }
  # end

  # specify 'tostring' do
  #   test_compile { "h#{2}o" }
  # end

  specify 'newarray' do
    test_compile { [] }
    test_compile { [[], nil, 1] }
  end

  specify 'duparray' do
    test_compile { [1, 2] }
    test_compile { [:foo, :bar] }
  end

  specify 'pop' do
    test_compile { [nil][0] = 1 }
  end

  specify 'setn' do
    test_compile { [nil][0] = 1 }
  end

  specify 'opt_str_freeze' do
    test_compile { "foo".freeze }
    test_compile { "bar".freeze.frozen? }
  end

  if RUBY_VERSION >= "2.5.0"
    specify 'opt_str_uminus' do
      test_compile { -"foo" }
      test_compile { (-"bar").frozen? }
    end
  end

  if RUBY_VERSION >= "2.4.0"
    specify 'opt_newarray_max' do
      test_compile { [[], [0]].max }
    end

    specify 'opt_newarray_min' do
      test_compile { [[], [0]].min }
    end
  end

  specify 'opt_send_without_block' do
    test_compile { 2 ** 3 }
    test_compile { false || 2.even? }
    test_compile { [nil].push(3) }
    test_compile { [] + [nil].push(3) }
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

  specify 'opt_ltlt' do
    test_compile { [] << [] }
    test_compile { [[], [1]] << [3, :hello] }
    test_compile { [] << [nil, nil] }
  end

  specify 'opt_aref' do
    test_compile { [1, 2, 3][1] }
    test_compile { ([] << [[nil, false], [[], 3]][1][0])[0] }
  end

  specify 'opt_aset' do
    test_compile { [nil][0] = 1 }
  end

  specify 'opt_length' do
    test_compile { [1, nil, false].length }
  end

  specify 'opt_size' do
    test_compile { [1, nil, false].size }
  end

  specify 'opt_empty_p' do
    test_compile { [].empty? }
    test_compile { [1].empty? }
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

  describe 'integration' do
    specify 'arithmetic' do
      test_compile { 1 + (3 - 4) * 5 / 2 }
    end
  end
end
