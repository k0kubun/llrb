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

### nop
- [x] nop

### variable
- [ ] getlocal
- [x] getlocal\_OP\_\_WC\_\_0
- [ ] getlocal\_OP\_\_WC\_\_1
- [ ] setlocal
- [x] setlocal\_OP\_\_WC\_\_0
- [ ] setlocal\_OP\_\_WC\_\_1
- [x] getspecial
- [x] setspecial
- [x] getinstancevariable
- [x] setinstancevariable
- [x] getclassvariable
- [x] setclassvariable
- [x] getconstant
- [x] setconstant
- [x] getglobal
- [x] setglobal

### put
- [x] putnil
- [x] putself
- [x] putobject
- [x] putobject\_OP\_INT2FIX\_O\_0\_C\_
- [x] putobject\_OP\_INT2FIX\_O\_1\_C\_
- [x] putspecialobject
- [x] putiseq
- [x] putstring
- [x] concatstrings
- [x] tostring
- [x] freezestring
- [x] toregexp
- [x] newarray
- [x] duparray
- [ ] expandarray
- [ ] concatarray
- [x] splatarray
- [x] newhash
- [x] newrange

### stack
- [x] pop
- [x] dup
- [x] dupn
- [x] swap
- [ ] reverse
- [!] reput
- [x] topn
- [x] setn
- [x] adjuststack

### setting
- [x] defined
- [x] checkmatch
- [ ] checkkeyword
- [ ] trace

### class/module
- [ ] defineclass (!)

### method/iterator
- [ ] send (!)
- [ ] invokesuper (!)
- [ ] invokeblock (!)
- [x] leave

### optimize
- [x] opt\_str\_freeze
- [x] opt\_newarray\_max
- [x] opt\_newarray\_min
- [x] opt\_send\_without\_block
- [!] getinlinecache
- [!] setinlinecache
- [!] once
- [!] opt\_case\_dispatch
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
- [!] opt\_call\_c\_function

### exception
- [ ] throw (!)

### jump
- [x] jump
- [x] branchif
- [x] branchunless
- [x] branchnil

### joke
- [!] bitblt
- [!] answer

## License

MIT License
