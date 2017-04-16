require 'spec_helper'

describe LLRuby::MethodCompiler do
  describe '.compile' do
    before do
      stub_const 'A', Class.new.tap { |klass|
        klass.class_eval do
          def a
          end
        end
      }
    end

    it 'can be called' do
      a = A.new
      LLRuby::MethodCompiler.compile(a.method(:a))
    end

    it 'rejects non-method argument' do
      [
        nil,
        "str",
        :sym,
        100,
        { foo: 'bar' },
        A,
        A.new,
        RubyVM::InstructionSequence.of(-> { puts 'hello' }),
      ].each do |obj|
        expect {
          LLRuby::MethodCompiler.compile(obj)
        }.to raise_error(ArgumentError)
      end
    end
  end
end
