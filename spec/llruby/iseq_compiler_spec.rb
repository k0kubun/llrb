require 'spec_helper'

describe LLRuby::IseqCompiler do
  describe '.compile' do
    it 'can be called' do
      func = Proc.new do
        puts 'hello'
      end
      iseq = RubyVM::InstructionSequence.of(func)
      LLRuby::IseqCompiler.compile(iseq)
    end

    it 'rejects non-iseq argument' do
      [
        nil,
        "str",
        :sym,
        100,
        { foo: 'bar' },
        -> { puts 'hello' },
        RubyVM::InstructionSequence.of(-> { puts 'hello' }).to_a,
      ].each do |obj|
        expect {
          LLRuby::IseqCompiler.compile(obj)
        }.to raise_error(ArgumentError)
      end
    end
  end
end
