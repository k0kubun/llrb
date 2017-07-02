RSpec.describe 'llrb_compile_iseq' do
  def test_compile(*args, &block)
    klass = Class.new
    klass.send(:define_singleton_method, :test, &block)
    result = klass.test(*args.map(&:dup))

    already_compiled = LLRB::JIT.compiled?(klass, :test)
    #LLRB::JIT.preview(klass, :test)
    compiled = LLRB::JIT.compile(klass, :test)
    expect(compiled).to eq(true) unless already_compiled
    expect(klass.test(*args.map(&:dup))).to eq(result)
    GC.start # to increase possibility to hit a bug caused by GC.
  end

  #specify 'break' do
  #end

  #specify 'next' do
  #end

  #specify 'redo' do
  #end

  #specify 'retry' do
  #end

  #specify 'return' do
  #end

  #specify 'rescue' do
  #end

  #specify 'ensure' do
  #end
end
