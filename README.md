# LLRuby [![wercker status](https://app.wercker.com/status/acd09c7ee0739521508fed6187758a53/s/master "wercker status")](https://app.wercker.com/project/byKey/acd09c7ee0739521508fed6187758a53)

LLRuby is LLVM-based JIT compiler for Ruby.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'llruby'
```

## Usage
### Compiling method to native code

```rb
require 'llruby'

class Hello
  def world
     puts "Hello world!"
  end
end

hello = Hello.new
LLRuby::JIT.precompile(hello, :world)
hello.world # Executed in native code
```

### Dumping LLVM IR

```rb
LLRuby::JIT.preview(Hello, :world)
# ; ModuleID = 'llruby'
#
# define i64 @precompiled_method(i64) {
#   ret i64 8
# }
```

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/k0kubun/llruby.

## License

The gem is available as open source under the terms of the [MIT License](http://opensource.org/licenses/MIT).
