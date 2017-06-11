# frozen_string_literal: true
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

  specify 'freezestring' do
    test_compile { "#{true}" }
  end
end
