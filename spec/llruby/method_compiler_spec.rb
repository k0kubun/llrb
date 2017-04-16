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
  end
end
