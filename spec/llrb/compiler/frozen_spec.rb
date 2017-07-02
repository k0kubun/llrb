RSpec.describe 'llrb_compile_iseq' do
  def test_compile(*args, &block)
    ruby = Class.new
    ruby.send(:define_singleton_method, :test, &block)

    native = Class.new
    native.send(:define_singleton_method, :test, &block)

    expect(LLRB::JIT.compile(native, :test)).to eq(true)
    expect(native.test(*args.map(&:dup))).to eq(ruby.test(*args.map(&:dup)))
  end

  specify 'freezestring' do
    test_compile { "#{true}" }
    test_compile { "#{true}".frozen? }
  end
end
