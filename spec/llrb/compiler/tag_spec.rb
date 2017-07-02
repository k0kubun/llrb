RSpec.describe 'long jump by tag' do
  module LLRB::LongjumpResult
    class << self
      def executed
        @executed ||= Hash.new { |h, k| h[k] = false }
      end

      def ensured
        @ensured ||= Hash.new { |h, k| h[k] = false }
      end

      attr_accessor :tag
    end
  end

  module LLRB::LongjumpTest
    def self.test1(tag)
      throw tag, 'hello'
      LLRB::LongjumpResult.executed[:test1] = true
    ensure
      LLRB::LongjumpResult.ensured[:test1] = true
      LLRB::LongjumpResult.tag = tag
    end

    def self.test2(tag)
      test1(tag)
      LLRB::LongjumpResult.executed[:test2] = true
    ensure
      LLRB::LongjumpResult.ensured[:test2] = true
    end

    def self.test3(tag)
      test2(tag)
      LLRB::LongjumpResult.executed[:test3] = true
    ensure
      LLRB::LongjumpResult.ensured[:test3] = true
    end

    def self.test4
      ret = catch(:k0kubun, &method(:test3))
      LLRB::LongjumpResult.executed[:test4] = true
      ret
    ensure
      LLRB::LongjumpResult.ensured[:test4] = true
    end
  end

  specify 'catch' do
    #expect(LLRB::JIT.compile(LLRB::LongjumpTest, :test1)).to eq(true)
    #expect(LLRB::JIT.compile(LLRB::LongjumpTest, :test2)).to eq(true)
    #expect(LLRB::JIT.compile(LLRB::LongjumpTest, :test3)).to eq(true)
    expect(LLRB::JIT.compile(LLRB::LongjumpTest, :test4)).to eq(true)

    expect(LLRB::LongjumpTest.test4).to eq('hello')
    expect(LLRB::LongjumpResult.tag).to eq(:k0kubun)

    expect(LLRB::LongjumpResult.executed[:test1]).to eq(false)
    expect(LLRB::LongjumpResult.executed[:test2]).to eq(false)
    expect(LLRB::LongjumpResult.executed[:test3]).to eq(false)
    expect(LLRB::LongjumpResult.executed[:test4]).to eq(true)

    expect(LLRB::LongjumpResult.ensured[:test1]).to eq(true)
    expect(LLRB::LongjumpResult.ensured[:test2]).to eq(true)
    expect(LLRB::LongjumpResult.ensured[:test3]).to eq(true)
    expect(LLRB::LongjumpResult.ensured[:test4]).to eq(true)
  end
end
