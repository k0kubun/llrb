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
LLRB::JIT.preview(hello, :world)
; ModuleID = 'llrb'

define i64 @precompiled_method(i64) {
entry:
  %putstring = call i64 @rb_str_resurrect(i64 94336633338360)
  %funcall = call i64 (i64, i64, i32, ...) @rb_funcall(i64 %0, i64 15329, i32 1, i64 %putstring)
  ret i64 %funcall
}

declare i64 @rb_str_resurrect(i64)

declare i64 @rb_funcall(i64, i64, i32, ...)
```

## Supported Iseq instructions

56/94 (59.6%)

### nop
- [x] nop

### variable
- [ ] getlocal
- [x] getlocal\_OP\_\_WC\_\_0
- [ ] getlocal\_OP\_\_WC\_\_1
- [ ] setlocal
- [ ] setlocal\_OP\_\_WC\_\_0
- [ ] setlocal\_OP\_\_WC\_\_1
- [ ] getspecial
- [ ] setspecial
- [ ] getinstancevariable
- [ ] setinstancevariable
- [ ] getclassvariable
- [ ] setclassvariable
- [x] getconstant (current scope not handled)
- [ ] setconstant
- [ ] getglobal
- [ ] setglobal

### put
- [x] putnil
- [x] putself
- [x] putobject
- [x] putobject\_OP\_INT2FIX\_O\_0\_C\_
- [x] putobject\_OP\_INT2FIX\_O\_1\_C\_
- [x] putspecialobject (vmcore)
- [x] putiseq
- [x] putstring
- [ ] concatstrings
- [x] tostring
- [x] freezestring
- [ ] toregexp
- [x] newarray
- [x] duparray
- [ ] expandarray
- [ ] concatarray
- [x] splatarray
- [x] newhash
- [x] newrange

### stack
- [x] pop
- [ ] dup
- [ ] dupn
- [x] swap
- [ ] reverse
- [ ] reput
- [x] topn
- [x] setn
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
- [x] opt\_str\_freeze
- [x] opt\_str\_uminus
- [x] opt\_newarray\_max
- [x] opt\_newarray\_min
- [x] opt\_send\_without\_block
- [x] getinlinecache
- [x] setinlinecache (actually not set)
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
- [x] opt\_aref
- [x] opt\_aset
- [x] opt\_aset\_with
- [x] opt\_aref\_with
- [x] opt\_length
- [x] opt\_size
- [x] opt\_empty\_p
- [x] opt\_succ
- [x] opt\_not
- [x] opt\_regexpmatch1
- [x] opt\_regexpmatch2
- [ ] opt\_call\_c\_function

### exception
- [ ] throw

### jump
- [x] jump
- [ ] branchif
- [x] branchunless
- [ ] branchnil

### joke
- [x] bitblt
- [x] answer

## License

The gem is available as open source under the terms of the [MIT License](http://opensource.org/licenses/MIT).
