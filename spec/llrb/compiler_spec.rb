require 'pry'

RSpec.describe 'llrb_compile_iseq' do
  def test_compile(*args, &block)
    ruby = Class.new
    ruby.send(:define_singleton_method, :test, &block)

    native = Class.new
    native.send(:define_singleton_method, :test, &block)

    expect(LLRB::JIT.compile(native, :test)).to eq(true)
    expect(native.test(*args.map(&:dup))).to eq(ruby.test(*args.map(&:dup)))
  end

  # specify 'nop' do
  # specify 'getlocal' do
  # specify 'setlocal' do
  # specify 'getspecial' do
  # specify 'setspecial' do
  # specify 'getinstancevariable' do
  # specify 'setinstancevariable' do
  # specify 'getclassvariable' do
  # specify 'setclassvariable' do
  # specify 'getconstant' do
  # specify 'setconstant' do
  # specify 'getglobal' do
  # specify 'setglobal' do

  specify 'putnil' do
    test_compile { nil }
  end

  # specify 'putself' do

  specify 'putobject' do
    test_compile { true }
    test_compile { false }
    test_compile { 100 }
    test_compile { :hello }
    test_compile { (1..2) }
  end

  # specify 'putspecialobject' do
  # specify 'putiseq' do
  # specify 'putstring' do
  # specify 'concatstrings' do
  # specify 'tostring' do
  # specify 'freezestring' do
  # specify 'toregexp' do
  # specify 'newarray' do
  # specify 'duparray' do
  # specify 'expandarray' do
  # specify 'concatarray' do
  # specify 'splatarray' do
  # specify 'newhash' do
  # specify 'newrange' do
  # specify 'pop' do
  # specify 'dup' do
  # specify 'dupn' do
  # specify 'swap' do
  # specify 'reverse' do
  # specify 'reput' do
  # specify 'topn' do
  # specify 'setn' do
  # specify 'adjuststack' do
  # specify 'defined' do
  # specify 'checkmatch' do
  # specify 'checkkeyword' do
  # specify 'trace' do
  # specify 'defineclass' do
  # specify 'send' do
  # specify 'opt_str_freeze' do
  # specify 'opt_newarray_max' do
  # specify 'opt_newarray_min' do
  # specify 'opt_send_without_block' do
  # specify 'invokesuper' do
  # specify 'invokeblock' do

  specify 'leave' do
    test_compile { nil }
  end

  # specify 'throw' do

  specify 'jump' do
    test_compile(true) { |a| 1 if a }
    #test_compile(true) { while 1+1; end }
  end

  # specify 'branchif' do

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

    #test_compile(true) do |a|
    #  raise 'unreachable' until a
    #end
  end

  # specify 'branchnil' do
  # specify 'getinlinecache' do
  # specify 'setinlinecache' do
  # specify 'once' do
  # specify 'opt_case_dispatch' do

  specify 'opt_plus' do
    test_compile(1, 2) { |a, b| a+b }
  end

  specify 'opt_minus' do
    test_compile(2, 1) { |a, b| a-b }
  end

  # specify 'opt_mult' do
  # specify 'opt_div' do
  # specify 'opt_mod' do
  # specify 'opt_eq' do
  # specify 'opt_neq' do
  # specify 'opt_lt' do
  # specify 'opt_le' do
  # specify 'opt_gt' do
  # specify 'opt_ge' do
  # specify 'opt_ltlt' do
  # specify 'opt_aref' do
  # specify 'opt_aset' do
  # specify 'opt_aset_with' do
  # specify 'opt_aref_with' do
  # specify 'opt_length' do
  # specify 'opt_size' do
  # specify 'opt_empty_p' do
  # specify 'opt_succ' do
  # specify 'opt_not' do
  # specify 'opt_regexpmatch1' do
  # specify 'opt_regexpmatch2' do
  # specify 'opt_call_c_function' do
  # specify 'bitblt' do
  # specify 'answer' do

  specify 'getlocal_OP__WC__0' do
    test_compile(1) { |a| a }
  end

  # specify 'getlocal_OP__WC__1' do
  # specify 'setlocal_OP__WC__0' do
  # specify 'setlocal_OP__WC__1' do

  specify 'putobject_OP_INT2FIX_O_0_C_' do
    test_compile { 0 }
  end

  specify 'putobject_OP_INT2FIX_O_1_C_' do
    test_compile { 1 }
  end
end
