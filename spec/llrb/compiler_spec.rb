RSpec.describe 'llrb_compile_iseq' do
  def test_compile(*args, &block)
    klass = Class.new
    klass.send(:define_singleton_method, :test, &block)
    result = klass.test(*args.map(&:dup))

    # Possibly the same block in a loop is compiled multiple times
    already_compiled = LLRB::JIT.compiled?(klass, :test)
    #LLRB::JIT.preview(klass, :test)
    compiled = LLRB::JIT.compile(klass, :test)
    expect(compiled).to eq(true) unless already_compiled
    expect(klass.test(*args.map(&:dup))).to eq(result)
    GC.start # to increase possibility to hit a bug caused by GC.
  end

  def test_error(error, *args, &block)
    native = Class.new
    native.send(:define_singleton_method, :test, &block)

    expect(LLRB::JIT.compile(native, :test)).to eq(true)
    expect { native.test(*args.map(&:dup)) }.to raise_error(error)
  end

  specify 'nop' do
    test_compile { nil rescue true }
  end

  specify 'getlocal' do
    compiled = false
    klass1 = Class.new
    klass1.send(:define_singleton_method, :test) do |&block|
      compiled = LLRB::JIT.compile_proc(block)
      block.call
    end

    klass2 = Class.new
    klass2.send(:define_singleton_method, :test) do |&block|
      block.call
    end

    klass3 = Class.new
    klass3.send(:define_singleton_method, :test) do
      a = 3
      klass2.test do
        b = 2
        klass1.test do
          a * b
        end
      end
    end

    expect(klass3.test).to eq(6)
    expect(compiled).to eq(true)
  end

  specify 'setlocal' do
    compiled = false
    klass1 = Class.new
    klass1.send(:define_singleton_method, :test) do |&block|
      compiled = LLRB::JIT.compile_proc(block)
      block.call
    end

    klass2 = Class.new
    klass2.send(:define_singleton_method, :test) do |&block|
      block.call
    end

    klass3 = Class.new
    klass3.send(:define_singleton_method, :test) do
      a = 3
      b = 1
      klass2.test do
        klass1.test do
          b = b * 2
          a * b
        end
      end
    end

    expect(klass3.test).to eq(6)
    expect(compiled).to eq(true)
  end

  specify 'getspecial' do
    test_compile { $1 }
  end

  specify 'setspecial' do
    test_compile { true if nil.nil?..nil.nil? }
  end

  specify 'getinstancevariable' do
    test_compile { @a = 1; @a }
  end

  specify 'setinstancevariable' do
    test_compile { @a = 2 }
  end

  # If we do this via Class.new, it prints warning...
  class LLRB::ClassVariableTest
    def self.test_getclassvariable
      @@a = 1
      @@a
    end

    def self.test_setclassvariable
      @@b = 2
    end
  end

  specify 'getclassvariable' do
    result = LLRB::ClassVariableTest.test_getclassvariable
    expect(LLRB::JIT.compile(LLRB::ClassVariableTest, :test_getclassvariable)).to eq(true)
    expect(LLRB::ClassVariableTest.test_getclassvariable).to eq(result)
  end

  specify 'setclassvariable' do
    result = LLRB::ClassVariableTest.test_setclassvariable
    expect(LLRB::JIT.compile(LLRB::ClassVariableTest, :test_setclassvariable)).to eq(true)
    expect(LLRB::ClassVariableTest.test_setclassvariable).to eq(result)
  end

  specify 'getconstant' do
    test_compile { RubyVM::InstructionSequence }
  end

  specify 'setconstant' do
    test_compile(Class.new) do |klass|
      klass::X = true
    end
  end

  specify 'getglobal' do
    $llrb_gvar_get = 1
    test_compile { $llrb_gvar_get }
  end

  specify 'setglobal' do
    test_compile { $llrb_gvar_set = 2 }
  end

  specify 'putnil' do
    test_compile { nil }
  end

  specify 'putself' do
    test_compile { self }
    test_compile { print }
  end

  specify 'putobject' do
    test_compile { true }
    test_compile { false }
    test_compile { 100 }
    test_compile { :hello }
    test_compile { (1..2) }
  end

  specify 'putspecialobject' do
    test_compile do
      def answer; 42; end
    end
  end

  specify 'putiseq' do
    test_compile do
      def self.concatenate(hello, world)
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

  specify 'toregexp' do
    test_compile { /#{true}/ =~ "true" }
  end

  specify 'newarray' do
    test_compile { [] }
    test_compile { [[], nil, 1] }
  end

  specify 'duparray' do
    test_compile { [1, 2] }
    test_compile { [:foo, :bar] }
  end

  #specify 'expandarray' do
  #  test_compile { y = [ true, false, nil ]; x, = y; x }
  #end

  specify 'concatarray' do
    test_compile { ["t", "r", *x = "u", "e"].join }
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
    test_compile(false, 1) { |a, b| a || b }
  end

  specify 'dup' do
    test_compile(1) { |a| a&.+(2) }
  end

  specify 'dupn' do
    test_compile(Class.new) do |klass|
      klass::X ||= true
    end
  end

  specify 'swap' do
    test_compile do
      {}['true'] = true
    end
  end

  #specify 'reverse' do
  #  test_compile do
  #    q, (w, e), r = 1, [2, 3], 4; e == 3
  #  end
  #end

  # specify 'reput' do

  specify 'topn' do
    test_compile { {}['true'] = true }
  end

  specify 'setn' do
    test_compile { [nil][0] = 1 }
  end

  specify 'adjuststack' do
    test_compile([true]) do |x|
      x[0] ||= nil
      x[0]
    end
  end

  specify 'defined' do
    test_compile { defined?(a) }
    test_compile(1) { |a| defined?(a) }
  end

  specify 'checkmatch' do
    [0, 1].each do |aa|
      test_compile(aa) do |a|
        case a
        when 1
          2
        else
          3
        end
      end
    end

    test_compile(1) do |a|
      1000 + case a
             when 1
               100
             end
    end

    test_compile(3) do |a|
      case a
      when 1
        100
      when 3
      end
    end

    test_compile(3, 2) do |a, b|
      1000 + case a
             when 1
               100
             when 2
               200
             when 3
               300 + case b
                     when 4
                       400
                     else
                       500
                     end + 600
             end + 700
    end
  end

  specify 'checkkeyword' do
    test_compile(x: true) do |x: rand|
      x
    end
  end

  specify 'trace' do
    test_compile { nil }
  end

  # specify 'defineclass' do

  specify 'send' do
    test_compile do
      1.times { 2 }
    end

    test_compile do
      [1, 2].inject([]) do |ary, i|
        ary << i
      end
    end

    klass = Class.new {
      def self.test(a)
        if a > 0
          test(a-1) { 1 } + yield
        else
          0
        end
      end
    }
    result = klass.test(1) { 2 }
    expect(LLRB::JIT.compile(klass, :test)).to eq(true)
    expect(klass.test(1) { 2 }).to eq(result)

    test_compile do
      klass.test(0, &nil)
    end

    klass2 = Class.new
    klass2.send(:define_singleton_method, :test) do |&block|
      klass.test(1, &block)
    end

    result = klass2.test { 2 }
    expect(LLRB::JIT.compile(klass2, :test)).to eq(true)
    expect(klass2.test { 2 }).to eq(result)
  end

  specify 'opt_str_freeze' do
    test_compile { "foo".freeze }
    test_compile { "bar".freeze.frozen? }
  end

  specify 'opt_newarray_max' do
    test_compile { [[], [0]].max }
  end

  specify 'opt_newarray_min' do
    test_compile { [[], [0]].min }
  end

  specify 'opt_send_without_block' do
    test_compile { [1,2].reverse }
    test_compile { 2 ** 3 }
    test_compile { false || 2.even? }
    test_compile { [nil].push(3) }
    test_compile { [] + [nil].push(3) }

    klass = Class.new {
      def self.test(a)
        if a > 0
          test(a-1)
        end
      end
    }
    result = klass.test(1)
    expect(LLRB::JIT.compile(klass, :test)).to eq(true)
    expect(klass.test(1)).to eq(result)
  end

  specify 'invokesuper' do
    mod = Module.new {
      def test
        super
      end
    }
    klass = Class.new {
      prepend mod
      def test
        1
      end
    }
    object = klass.new
    result = object.test
    expect(LLRB::JIT.compile(object, :test)).to eq(true)
    expect(object.test).to eq(result)
  end

  specify 'invokeblock' do
    klass = Class.new {
      def test
        1 + yield + 3
      end
    }
    object = klass.new
    result = object.test { 2 }
    expect(LLRB::JIT.compile(object, :test)).to eq(true)
    expect(object.test { 2 }).to eq(result)
  end

  specify 'leave' do
    test_compile { nil }
  end

  specify 'throw' do
    test_compile do
      if 1+1 == 1
        return 3
      else
        return 4
      end
      5
    end

    test_compile do
      if 1+1 == 2
        4
      else
        return 3
      end
      5
    end
  end

  specify 'jump' do
    test_compile(true) { |a| 1 if a }
    test_compile(nil) { |a| while a; end }
  end

  specify 'branchif' do
    test_compile(false, 1) { |a, b| a || b }
    test_compile(1, 2) { |a, b| a || b }
    test_compile(false) do |a|
      1+1 while a
    end
  end

  specify 'branchunless' do
    test_compile(true) do |a|
      if a
        1
      else
        2
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
          19
        elsif a == 2
          if b == 1
            21
          else
            29
          end
        elsif a == 3
          39
        elsif a == 4
          if b == 1
            41
          elsif b == 2
            42
          else
            49
          end
        else
          99
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

    test_compile(true) do |a|
      1 + if a
            2
          end
    end

    test_compile(true, false) do |a, b|
      if a
        2
      else
        3
      end + if b
              4
            else
              5
            end
    end

    test_compile(false) do |a|
      1 + if a
            2
          else
            3
          end + 4
    end

    test_compile(false, true) do |a, b|
      unless a
        if b
          7
        end + 9
      end
    end

    test_compile(false, true, false) do |a, b, c|
      1 + if a
            2 + if b
                  3
                else
                  4
                end + 5
          else
            6 + unless c
                  7
                else
                  8
                end + 9
          end + 10
    end

    test_compile(true) { |a| until a; end }
    test_compile(true) do |a|
      1+1 until a
    end
  end

  specify 'branchnil' do
    test_compile(1) { |a| a&.+(2) }
    test_compile(nil) { |a| a&.+(2) }
    test_compile(1) { |a| 2 + a&.+(3) + 4 }
    test_error(TypeError, nil) { |a| 1 + a&.+(3) + 2 }
  end

  specify 'getinlinecache' do
    test_compile { Struct }
  end

  specify 'setinlinecache' do
    test_compile { Struct }
  end

  # specify 'once' do

  specify 'opt_case_dispatch' do
    test_compile(1) do |a|
      case a
      when 1
        2
      end
    end
  end

  specify 'opt_plus' do
    test_compile do
      1 + 2 + 3 + 4 + 5
    end
  end

  specify 'opt_minus' do
    test_compile(2, 1) { |a, b| a-b }
    test_compile(2.1, 1.2) { |a, b| a-b }
  end

  specify 'opt_mult' do
    test_compile { 3 * 2 * 1 }
    test_compile { 3.1 * 2.1 * 1.1 }
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

  # specify 'opt_call_c_function' do

  specify 'getlocal_OP__WC__0' do
    test_compile(1) { |a| a }
  end

  specify 'getlocal_OP__WC__1' do
    compiled = false
    klass1 = Class.new
    klass1.send(:define_singleton_method, :test) do |&block|
      compiled = LLRB::JIT.compile_proc(block)
      block.call
    end

    klass2 = Class.new
    klass2.send(:define_singleton_method, :test) do
      a = 1
      klass1.test do
        a * 2
      end
    end

    expect(klass2.test).to eq(2)
    expect(compiled).to eq(true)
  end

  specify 'setlocal_OP__WC__0' do
    test_compile(1) do |a|
      a = 2
    end
    test_compile do
      a = 4
      b = 3
      a - b
    end
  end

  specify 'setlocal_OP__WC__1' do
    compiled = false
    klass1 = Class.new
    klass1.send(:define_singleton_method, :test) do |&block|
      compiled = LLRB::JIT.compile_proc(block)
      block.call
    end

    klass2 = Class.new
    klass2.send(:define_singleton_method, :test) do
      a = 2
      b = 1
      klass1.test do
        b = b + 2
        a * b
      end
    end

    expect(klass2.test).to eq(6)
    expect(compiled).to eq(true)
  end

  specify 'putobject_OP_INT2FIX_O_0_C_' do
    test_compile { 0 }
  end

  specify 'putobject_OP_INT2FIX_O_1_C_' do
    test_compile { 1 }
  end
end
