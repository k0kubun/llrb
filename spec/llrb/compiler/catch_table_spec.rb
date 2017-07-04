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

  specify 'break' do
    klass = Class.new
    def klass.test1
      test2 do
        break 1
      end
    end
    def klass.test2(&block)
      LLRB::JIT.compile_proc(block)
      2.tap(&block)
    end

    expect(klass.test1).to eq(1)
  end

  #specify 'next' do
  #end

  #specify 'redo' do
  #end

  #specify 'retry' do
  #end

  specify 'return' do
    klass = Class.new
    def klass.test1
      test2 do
        return 1
      end
      2
    end
    def klass.test2(&block)
      LLRB::JIT.compile_proc(block)
      block.call
      return 3
    end

    expect(klass.test1).to eq(1)
  end

  specify 'rescue' do
    klass = Class.new
    def klass.test
      raise 'hello'
    rescue RuntimeError => e
      e
    end

    expect(LLRB::JIT.compile(klass, :test)).to eq(true)
    expect(klass.test).to be_a(RuntimeError)
  end

  specify 'ensure' do
    ensured = false
    klass = Class.new
    def klass.test
      @ensured = false
      raise 'hello'
    ensure
      @ensured = true
    end

    begin
      klass.test
    rescue RuntimeError
      expect(klass.instance_variable_get(:@ensured)).to eq(true)
    end
  end
end
