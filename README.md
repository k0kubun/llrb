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

10/94 (10.6%)

### nop
- [ ] nop

### variable
- [ ] getlocal (!)
- [x] getlocal\_OP\_\_WC\_\_0
- [ ] getlocal\_OP\_\_WC\_\_1 (!)
- [ ] setlocal (!)
- [ ] setlocal\_OP\_\_WC\_\_0 (!)
- [ ] setlocal\_OP\_\_WC\_\_1 (!)
- [ ] getspecial (!)
- [ ] setspecial (!)
- [ ] getinstancevariable
- [ ] setinstancevariable
- [ ] getclassvariable (!)
- [ ] setclassvariable (!)
- [ ] getconstant (!)
- [ ] setconstant (!)
- [ ] getglobal (!)
- [ ] setglobal (!)

### put
- [x] putnil
- [ ] putself
- [x] putobject
- [x] putobject\_OP\_INT2FIX\_O\_0\_C\_
- [x] putobject\_OP\_INT2FIX\_O\_1\_C\_
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
- [ ] defined (!)
- [ ] checkmatch (!)
- [ ] checkkeyword (!)
- [ ] trace

### class/module
- [ ] defineclass (!)

### method/iterator
- [ ] send (!)
- [ ] invokesuper (!)
- [ ] invokeblock (!)
- [x] leave

### optimize
- [ ] opt\_str\_freeze
- [ ] opt\_str\_uminus
- [ ] opt\_newarray\_max
- [ ] opt\_newarray\_min
- [ ] opt\_send\_without\_block
- [ ] getinlinecache
- [ ] setinlinecache (actually not set)
- [ ] once (!)
- [ ] opt\_case\_dispatch (!)
- [x] opt\_plus
- [x] opt\_minus
- [ ] opt\_mult
- [ ] opt\_div
- [ ] opt\_mod
- [ ] opt\_eq
- [ ] opt\_neq
- [ ] opt\_lt
- [ ] opt\_le
- [ ] opt\_gt
- [ ] opt\_ge
- [ ] opt\_ltlt
- [ ] opt\_aref
- [ ] opt\_aset
- [ ] opt\_aset\_with
- [ ] opt\_aref\_with
- [ ] opt\_length
- [ ] opt\_size
- [ ] opt\_empty\_p
- [ ] opt\_succ
- [ ] opt\_not
- [ ] opt\_regexpmatch1
- [ ] opt\_regexpmatch2
- [ ] opt\_call\_c\_function

### exception
- [ ] throw (!)

### jump
- [x] jump
- [ ] branchif (!)
- [x] branchunless
- [ ] branchnil (!)

### joke
- [ ] bitblt
- [ ] answer

## License

MIT License
