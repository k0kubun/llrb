require 'pry'

describe 'llrb::Compiler' do
  def test_compile(*args, &block)
    ruby = Class.new
    ruby.send(:define_singleton_method, :test, &block)

    native = Class.new
    native.send(:define_singleton_method, :test, &block)

    begin
      expect(LLRB::JIT.precompile(native, :test)).to eq(true)
    rescue RSpec::Expectations::ExpectationNotMetError
      iseq_array = RubyVM::InstructionSequence.of(ruby.method(:test)).to_a
      Pry::ColorPrinter.pp(iseq_array)
      raise
    end

    begin
      expect(native.test(*args.map(&:dup))).to eq(ruby.test(*args.map(&:dup)))
    rescue RSpec::Expectations::ExpectationNotMetError
      LLRB::JIT.preview(ruby, :test)
      raise
    end
  end

  # for debug
  def preview_compile(*args, &block)
    native = Class.new
    native.send(:define_singleton_method, :test, &block)
    iseq_array = RubyVM::InstructionSequence.of(native.method(:test)).to_a
    Pry::ColorPrinter.pp(iseq_array)
    LLRB::JIT.preview(native, :test)
  end

  # Don't use this method for iseq that can be generated from compiler
  def eval_iseq(*iseq_bytecode)
    klass = Class.new
    klass.send(:define_singleton_method, :native) { nil }
    original = klass.method(:native)

    # Create valid iseq using actual iseq
    iseq_array = RubyVM::InstructionSequence.of(original).to_a
    index = iseq_array.last.index([:putnil])
    iseq_array.last.delete([:putnil])
    iseq_array.last.insert(index, *iseq_bytecode)

    expect(
      LLRB::JIT.send(:precompile_internal, iseq_array, original.owner, original.original_name, original.arity, false)
    ).to eq(true)
    klass.native
  end

  specify 'nop' do
    result = eval_iseq(
      [:putstring, "stack wasn't modified"],
      [:nop],
    )
    expect(result).to eq("stack wasn't modified")
  end

  specify 'getlocal_OP__WC__0' do
    test_compile(1) { |a| a }
  end

  specify 'getinstancevariable' do
    test_compile { @a = 1; @a }
  end

  specify 'setinstancevariable' do
    test_compile { @a = 2 }
  end

  specify 'getconstant' do
    test_compile { RubyVM::InstructionSequence }
  end

  specify 'setconstant' do
    test_compile(Class.new) do |klass|
      klass::X = true
    end
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
    test_compile { :hello }
    test_compile { (1..2) }
  end

  specify 'putobject_OP_INT2FIX_O_0_C_' do
    test_compile { 0 }
  end

  specify 'putobject_OP_INT2FIX_O_1_C_' do
    test_compile { 1 }
  end

  specify 'putspecialobject' do
    test_compile do
      def answer; 42; end
      answer
    end
  end

  specify 'putiseq' do
    test_compile do
      def answer; 42; end
    end

    test_compile do
      def concatenate(hello, world)
        hello + world
      end
      concatenate("hello", "world")
    end
  end

  specify 'putstring' do
    test_compile { "hello" }
  end

  specify 'concatstrings' do
    test_compile { "a#{}b" }
  end

  specify 'tostring' do
    test_compile { "h#{2}o" }
  end

  specify 'newarray' do
    test_compile { [] }
    test_compile { [[], nil, 1] }
  end

  specify 'duparray' do
    test_compile { [1, 2] }
    test_compile { [:foo, :bar] }
  end

  specify 'splatarray' do
    test_compile { [*(1..2)] }
    test_compile { [*''] }
  end

  specify 'newhash' do
    test_compile(1) do |a|
      { a: a }
    end

    test_compile(1, 2) do |a, b|
      { a: a, b: b }.to_a
    end
  end

  specify 'newrange' do
    test_compile(1) do |x|
      (0..x)
    end

    test_compile(1) do |x|
      (0...x)
    end
  end

  specify 'pop' do
    test_compile { [nil][0] = 1 }
    test_compile { def x; true; end; x }
  end

  specify 'dup' do
    test_compile(Class.new) do |klass|
      klass::X = true
    end
  end

  specify 'swap' do
    test_compile { {}['true'] = true }
  end

  specify 'topn' do
    test_compile { {}['true'] = true }
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

  specify 'getinlinecache' do
    test_compile { Struct }
  end

  specify 'setinlinecache' do
    test_compile { Struct }
  end

  specify 'opt_plus' do
    test_compile { 1 + 2 + 3 }
    test_compile { [nil] + [nil] }

    test_compile(1, 2) do |a, b|
      a + b
    end
  end

  specify 'opt_minus' do
    test_compile { 1 - 3 - 2 }
  end

  specify 'opt_mult' do
    test_compile { 3 * 2 * 1 }
  end

  specify 'opt_div' do
    test_compile { 3 / 2 / 1 }
    test_compile { 1 + (3 - 4) * 5 / 2 }
    test_compile(2, 3, 5, 7, 11) do |a, b, c, d, e|
      (a + b) * c ** d * e / b
    end
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

  specify 'opt_aset_with' do
    test_compile { {}['true'] = true }
  end

  specify 'opt_aref_with' do
    test_compile(100) do |x|
      { 'true' => x }['true']
    end
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

  specify 'opt_regexpmatch1' do
    test_compile { /true/ =~ 'true' }
  end

  specify 'opt_regexpmatch2' do
    test_compile { 'true' =~ /true/ }
  end

  specify 'jump' do
    test_compile(false) do |a|
      if a
        true
      else
        false
      end
    end
  end

  specify 'branchunless' do
    test_compile(3) do |a|
      if a > 2
        true
      else
        false
      end
    end

    [
      [true, false, false],
      [false, true, false],
      [false, false, true],
      [false, false, false],
    ].each do |args|
      test_compile(*args) do |a, b, c|
        if a
          1
        elsif b
          2
        elsif c
          3
        else
          4
        end
      end
    end

    [
      true,
      false,
      nil,
    ].each do |arg|
      test_compile(arg) do |a|
        unless a
          1
        else
          2
        end
      end

      test_compile(arg) do |a|
        1 unless a
      end

      test_compile(arg) do |a|
        1 if a
      end
    end

    [
      [1, nil],
      [2, 1],
      [2, nil],
      [3, nil],
      [4, 1],
      [4, 2],
      [4, nil],
      [nil, nil],
    ].each do |args|
      test_compile(*args) do |a, b|
        if a == 1
          [1, nil]
        elsif a == 2
          if b == 1
            [2, 1]
          else
            [2, nil]
          end
        elsif a == 3
          [3, nil]
        elsif a == 4
          if b == 1
            [4, 1]
          elsif b == 2
            [4, 2]
          else
            [4, nil]
          end
        else
          [nil, nil]
        end
      end
    end

    test_compile(true, true, true, false) do |a, b, c, d|
      if a
        if b
          if c
            if d
              1
            else
              2
            end
          end
        end
      end
    end
  end

  specify 'bitblt' do
    result = eval_iseq(
      [:bitblt],
    )
    expect(result).to eq("a bit of bacon, lettuce and tomato")
  end

  specify 'answer' do
    result = eval_iseq(
      [:answer],
    )
    expect(result).to eq(42)
  end

  it 'compiles arguments' do
    klass = Class.new
    klass.send(:define_singleton_method, :test) do |a, b, c|
      (a + b) * c + a
    end
    expect(LLRB::JIT.precompile(klass, :test)).to eq(true)

    expect(klass.test(2, 3, 7)).to eq((2 + 3) * 7 + 2)
  end

  it 'compiles instance method definition' do
    klass = Class.new
    klass.class_eval do
      def test
        def hello
          'world'
        end
      end
    end

    object = klass.new
    expect(LLRB::JIT.precompile(object, :test)).to eq(true)

    expect { object.hello }.to raise_error(NoMethodError)
    expect(object.test).to eq(:hello)
    expect(object.hello).to eq('world')
  end

  it 'compiles singleton method definition' do
    klass = Class.new
    klass.class_eval do
      def self.test
        def hello
          'world'
        end
      end
    end

    expect(LLRB::JIT.precompile(klass, :test)).to eq(true)

    expect { klass.hello }.to raise_error(NoMethodError)
    expect(klass.test).to eq(:hello)
    expect(klass.hello).to eq('world')
  end

  it 'compiles fibonacci' do
    test_compile do
      def fib n
        if n < 3
          1
        else
          fib(n-1) + fib(n-2)
        end
      end

      fib(5)
    end
  end
end
