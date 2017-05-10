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

## Supported Iseq instructions

18/84 (21.4%)

### nop
- [x] nop

### variable
- [ ] getlocal
- [ ] setlocal
- [ ] getspecial
- [ ] setspecial
- [ ] getinstancevariable
- [ ] setinstancevariable
- [ ] getclassvariable
- [ ] setclassvariable
- [ ] getconstant
- [ ] setconstant
- [ ] getglobal
- [ ] setglobal

### put
- [x] putnil
- [ ] putself
- [x] putobject (true, false, Fixnum, 0, 1)
- [ ] putspecialobject
- [ ] putiseq
- [ ] putstring
- [ ] concatstrings
- [ ] tostring
- [ ] freezestring
- [ ] toregexp
- [ ] newarray
- [ ] duparray
- [ ] expandarray
- [ ] concatarray
- [ ] splatarray
- [ ] newhash
- [ ] newrange

### stack
- [ ] pop
- [ ] dup
- [ ] dupn
- [ ] swap
- [ ] reverse
- [ ] reput
- [ ] topn
- [ ] setn
- [ ] adjuststack

### setting
- [ ] defined
- [ ] checkmatch
- [ ] checkkeyword
- [ ] trace

### class/module
- [ ] defineclass

### method/iterator
- [ ] send
- [ ] invokesuper
- [ ] invokeblock
- [ ] leave

### optimize
- [x] opt\_send\_without\_block
- [ ] getinlinecache
- [ ] setinlinecache
- [ ] once
- [ ] opt\_case\_dispatch
- [x] opt\_plus
- [x] opt\_minus
- [x] opt\_mult
- [x] opt\_div
- [x] opt\_mod
- [x] opt\_eq
- [x] opt\_neq
- [x] opt\_lt
- [x] opt\_le
- [x] opt\_gt
- [x] opt\_ge
- [ ] opt\_ltlt
- [ ] opt\_aref
- [ ] opt\_aset
- [ ] opt\_aset\_with
- [ ] opt\_aref\_with
- [ ] opt\_length
- [ ] opt\_size
- [ ] opt\_empty\_p
- [x] opt\_succ
- [x] opt\_not
- [ ] opt\_regexpmatch1
- [ ] opt\_regexpmatch2
- [ ] opt\_call\_c\_function

### exception
- [ ] throw

### jump
- [ ] jump
- [ ] branchif
- [ ] branchunless
- [ ] branchnil

### joke
- [ ] bitblt
- [x] answer

## License

The gem is available as open source under the terms of the [MIT License](http://opensource.org/licenses/MIT).
