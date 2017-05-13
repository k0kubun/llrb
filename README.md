# LLRB [![wercker status](https://app.wercker.com/status/71d808ff9de7f4f411714d40f9e99127/s/master "wercker status")](https://app.wercker.com/project/byKey/71d808ff9de7f4f411714d40f9e99127)

LLRB is a LLVM-based JIT compiler for Ruby.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'llrb'
```

## Build dependency

- LLVM/clang 3.8+
  - `llvm-config` command needs to appear in `$PATH`

## Usage
### Compiling method to native code

```rb
require 'llrb'

class Hello
  def world
     puts "Hello world!"
  end
end

hello = Hello.new
LLRB::JIT.precompile(hello, :world)
hello.world # Executed in native code
```

### Dumping LLVM IR

```rb
LLRB::JIT.preview(Hello, :world)
# ; ModuleID = 'llrb'
#
# define i64 @precompiled_method(i64) {
#   ret i64 8
# }
```

## Supported Iseq instructions

21/88 (23.9%)

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
- [x] newarray
- [x] duparray
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
- [ ] opt\_str\_freeze
- [ ] opt\_str\_uminus
- [ ] opt\_newarray\_max
- [ ] opt\_newarray\_min
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
- [x] opt\_ltlt
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
