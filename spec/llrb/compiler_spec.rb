require 'pry'

RSpec.describe 'llrb::Compiler' do
  def test_compile(*args, &block)
    ruby = Class.new
    ruby.send(:define_singleton_method, :test, &block)

    native = Class.new
    native.send(:define_singleton_method, :test, &block)

    expect(LLRB::JIT.compile(native, :test)).to eq(true)
    expect(native.test(*args.map(&:dup))).to eq(ruby.test(*args.map(&:dup)))
  end

  specify 'getlocal_OP__WC__0' do
    test_compile(1) { |a| a }
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
end
