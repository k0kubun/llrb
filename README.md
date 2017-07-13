# LLRB [![wercker status](https://app.wercker.com/status/71d808ff9de7f4f411714d40f9e99127/s/master "wercker status")](https://app.wercker.com/project/byKey/71d808ff9de7f4f411714d40f9e99127)

LLRB is a LLVM-based JIT compiler for Ruby.

## What's LLRB?

This is an experimental project to implement an idea presented by [@evanphx](https://github.com/evanphx) at [RubyKaigi 2015 Keynote](http://rubykaigi.org/2015/presentations/evanphx):<br>
Method JIT compiler inlining CRuby core functions using LLVM.

### How does it work?

On build time, some core functions are compiled to LLVM bitcode (binary form of LLVM IR) files via LLVM IR.

```
 ________     _________     ______________
|        |   |         |   |              |
| CRuby  |   | CRuby   |   | CRuby        |
| C code |-->| LLVM IR |-->| LLVM bitcode |
|________|   |_________|   |______________|
```

Those files are separated per function to load only necessary functions.
You can see how they are separated in [ext directory](./ext).

After profiler of LLRB JIT is started, when Ruby is running,
LLRB compiles Ruby method's YARV Instruction Sequence to native machine code.

```
 ______     ______________      __________      ___________      _________
|      |   |              |    |          |    |           |    |         |
| YARV |   | ISeq Control |    | LLVM IR  |    | Optimized |    | Machine |
| ISeq |-->| Flow Graph   |-*->| for ISeq |-*->| LLVM IR   |-*->| code    |
|______|   |______________| |  |__________| |  |___________| |  |_________|
                            |               |                |
                            | Link          | Optimize       | JIT compile
                      ______|_______     ___|____          __|____
                     |              |   |        |        |       |
                     | CRuby        |   | LLVM   |        | LLVM  |
                     | LLVM Bitcode |   | Passes |        | MCJIT |
                     |______________|   |________|        |_______|
```

### Does it improve performance?

Now basic instruction inlining is done. Let's see its effect.

Consider following Ruby method, which is the same as [ruby/benchmark/bm\_loop\_whileloop.rb](https://github.com/ruby/ruby/blob/v2_4_1/benchmark/bm_loop_whileloop.rb).

```rb
def while_loop
  i = 0
  while i<30_000_000
    i += 1
  end
end
```

The YARV ISeq, compilation target in LLRB, is this:

```
> puts RubyVM::InstructionSequence.of(method(:while_loop)).disasm
== disasm: #<ISeq:while_loop@(pry)>=====================================
== catch table
| catch type: break  st: 0015 ed: 0035 sp: 0000 cont: 0035
| catch type: next   st: 0015 ed: 0035 sp: 0000 cont: 0012
| catch type: redo   st: 0015 ed: 0035 sp: 0000 cont: 0015
|------------------------------------------------------------------------
local table (size: 1, argc: 0 [opts: 0, rest: -1, post: 0, block: -1, kw: -1@-1, kwrest: -1])
[ 1] i
0000 trace            8                                               (   1)
0002 trace            1                                               (   2)
0004 putobject_OP_INT2FIX_O_0_C_
0005 setlocal_OP__WC__0 3
0007 trace            1                                               (   3)
0009 jump             25
0011 putnil
0012 pop
0013 jump             25
0015 trace            1                                               (   4)
0017 getlocal_OP__WC__0 3
0019 putobject_OP_INT2FIX_O_1_C_
0020 opt_plus         <callinfo!mid:+, argc:1, ARGS_SIMPLE>, <callcache>
0023 setlocal_OP__WC__0 3
0025 getlocal_OP__WC__0 3                                             (   3)
0027 putobject        30000000
0029 opt_lt           <callinfo!mid:<, argc:1, ARGS_SIMPLE>, <callcache>
0032 branchif         15
0034 putnil
0035 trace            16                                              (   6)
0037 leave                                                            (   4)
=> nil
```

By LLRB compiler, such YARV ISeq is compiled to LLVM IR like:

```llvm
define i64 @llrb_exec(i64, i64) {
label_0:
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 8, i64 52)
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  call void @llrb_insn_setlocal_level0(i64 %1, i64 3, i64 1)
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  br label %label_25

label_15:                                         ; preds = %label_25
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  %2 = call i64 @llrb_insn_getlocal_level0(i64 %1, i64 3)
  call void @llrb_set_pc(i64 %1, i64 94225474387824)
  %opt_plus = call i64 @llrb_insn_opt_plus(i64 %2, i64 3)
  call void @llrb_insn_setlocal_level0(i64 %1, i64 3, i64 %opt_plus)
  br label %label_25

label_25:                                         ; preds = %label_15, %label_0
  %3 = call i64 @llrb_insn_getlocal_level0(i64 %1, i64 3)
  call void @llrb_set_pc(i64 %1, i64 94225474387896)
  %opt_lt = call i64 @llrb_insn_opt_lt(i64 %3, i64 60000001)
  %RTEST_mask = and i64 %opt_lt, -9
  %RTEST = icmp ne i64 %RTEST_mask, 0
  br i1 %RTEST, label %label_15, label %label_34

label_34:                                         ; preds = %label_25
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 16, i64 8)
  call void @llrb_set_pc(i64 %1, i64 94225474387960)
  call void @llrb_push_result(i64 %1, i64 8)
  ret i64 %1
}
```

As LLRB compiler links precompiled LLVM bitcode of CRuby functions,
using LLVM's FunctionInliningPass ("Pass" is LLVM's optimizer),
some C functions are inlined and inlined code will be well optimized by Passes like InstCombinePass.

```llvm
define i64 @llrb_exec(i64, i64) #0 {
  ...

land.lhs.true.i:                                  ; preds = %label_25
  %49 = load %struct.rb_vm_struct*, %struct.rb_vm_struct** @ruby_current_vm, align 8, !dbg !3471, !tbaa !3472
  %arrayidx.i = getelementptr inbounds %struct.rb_vm_struct, %struct.rb_vm_struct* %49, i64 0, i32 39, i64 7, !dbg !3471
  %50 = load i16, i16* %arrayidx.i, align 2, !dbg !3471, !tbaa !3473
  %and2.i = and i16 %50, 1, !dbg !3471
  %tobool6.i = icmp eq i16 %and2.i, 0, !dbg !3471
  br i1 %tobool6.i, label %if.then.i, label %if.else11.i, !dbg !3475, !prof !3380

if.then.i:                                        ; preds = %land.lhs.true.i
  call void @llvm.dbg.value(metadata i64 %48, i64 0, metadata !2680, metadata !3361) #7, !dbg !3477
  call void @llvm.dbg.value(metadata i64 60000001, i64 0, metadata !2683, metadata !3361) #7, !dbg !3478
  %cmp7.i = icmp slt i64 %48, 60000001, !dbg !3479
  %..i = select i1 %cmp7.i, i64 20, i64 0, !dbg !3481
  br label %llrb_insn_opt_lt.exit

if.else11.i:                                      ; preds = %land.lhs.true.i, %label_25
  %call35.i = call i64 (i64, i64, i32, ...) @rb_funcall(i64 %48, i64 60, i32 1, i64 60000001) #7, !dbg !3483
  br label %llrb_insn_opt_lt.exit, !dbg !3486

llrb_insn_opt_lt.exit:                            ; preds = %if.then.i, %if.else11.i
  %retval.1.i = phi i64 [ %..i, %if.then.i ], [ %call35.i, %if.else11.i ]
  %RTEST_mask = and i64 %retval.1.i, -9
  %RTEST = icmp eq i64 %RTEST_mask, 0

  ...
}
```

In this example, you can see many things are inlined.
LLRB's compiled code fetches RubyVM state and check whether `<` method is redefined or not,
and if `<` is not redefined, `if.then.i` block is used and in that block `icmp slt` is used instead of calling Ruby method `#<`.
Note that it's done by just inlining YARV's `opt_lt` instruction directly and it's not hard to implement.

Thus, following benchmark shows the performance is improved.

```rb
ruby = Class.new
def ruby.script
  i = 0
  while i< 30_000_000
    i += 1
  end
end

llrb = Class.new
def llrb.script
  i = 0
  while i< 30_000_000
    i += 1
  end
end

LLRB::JIT.compile(llrb, :script)

Benchmark.ips do |x|
  x.report('Ruby') { ruby.script }
  x.report('LLRB') { llrb.script }
  x.compare!
end
```

On [wercker](https://app.wercker.com/k0kubun/llrb/runs/build/5966cdab1ee2040001449915?step=5966cdded82c270001e8740):

```
Calculating -------------------------------------
                Ruby      7.449  (± 0.0%) i/s -     38.000  in   5.101634s
                LLRB     36.974  (± 0.0%) i/s -    186.000  in   5.030540s

Comparison:
                LLRB:       37.0 i/s
                Ruby:        7.4 i/s - 4.96x  slower
```

## How is the design?
### Built as C extension

Currently LLRB is in an experimental stage. For fast development and to stay up-to-date
for CRuby core changes, LLRB is built as C extension.
We can use bundler, benchmark-ips, pry, everything else in normal C extension repository. It's hard in just CRuby fork.

For optimization, unfortunately it needs to export some symbols from CRuby,
so it needs to compile [k0kubun/ruby's llrb branch](https://github.com/k0kubun/ruby/tree/llrb)
and install llrb.gem from that Ruby.

But I had the rule that I don't add modification except exporting symbols.
And LLRB code refers to such exported functions or variables in CRuby core by including CRuby header as possible.
I believe it contributes to maintainability of LLRB prototype.

### Conservative design for safety

YARV is already proven to be reliable. In LLRB, YARV is not modified at all.
Then, how is JIT achieved?

YARV has `opt_call_c_function` instruction, which is explained to ["call native compiled method"](https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2136).
Why not use that?

LLRB compiles any ISeq to following ISeq.

```
0000 opt_call_c_function
0002 leave
```

So simple. Note that `opt_call_c_function` [can handle YARV's internal exception](https://github.com/ruby/ruby/blob/v2_4_1/insns.def#L2147-L2151).
In that instruction, we can do everything.

One more conservative thing in LLRB is that it fills `leave` instructions to remaining places.
To let YARV catch table work, it needs to update program counter properly,
and then it requires an instruction to the place that program counter points to.

For safe exit when catch table is used, `leave` instructions are filled to the rest of first `opt_call_c_function`.

### Sampling-based lightweight profiler

Sampling profiler is promising approach to reduce the overhead of profiling without spoiling profiling efficiency.
LLRB's profiler to schedule JIT-ed ISeq is implemented in almost the same way as [stackprof](https://github.com/tmm1/stackprof).
It is widely-used on production and proven to be a reliable approach.

It kicks profiler in some CPU-time interval, and the parameter can be modified if necessary.
Using [optcarrot](https://github.com/mame/optcarrot) benchmark, I tested profiler overhead
and LLRB's profiler didn't reduce the fps of optcarrot with current parameter.

Also, it uses `rb_postponed_job_register_one` API, which is used by stackprof too, to do JIT compile.
So the compilation is done in very safe timing.

### Less compilation effort

CRuby's C functions to inline are precompiled as LLVM bitcode on LLRB build process.
LLRB's compiler builds LLVM IR using LLVM IRBuilder, so the bitcode files are directly linked to that.

It means that LLRB has no overhead of parsing and compiling C source code on runtime.
It has less compilation effort, right?

Currently the performance bottleneck is not in compiler, unfortunately!
So it doesn't use extra thread for JIT compilation for now.

## Project status

Experimental. Not matured at all.

Everything I said above is already implemented, but that's all!
Core function inlining is achieved but it's not completely applied to all instructions
and we need MUCH MORE effort to improve performance in real-world application.

## Build dependency

- **64bit CPU**
  - This should be fixed later
- LLVM/clang 3.8+
  - `llvm-config`, `clang` and `llvm-as` commands need to appear in `$PATH`
- [CRuby fork in k0kubun/ruby's llrb branch](https://github.com/k0kubun/ruby/tree/llrb)

## Usage

Once build dependency is met, execute `gem install llrb` and do:

```ruby
require 'llrb/start'
```

`llrb/start` file does `LLRB::JIT.start`. Note that you can also do that by `ruby -rllrb/start -e "..."`.

If you want to see which method is compiled, compile the gem with `#define LLRB_ENABLE_DEBUG 1`.
Again, it's in an experimental stage and currently it doesn't improve performance in real-world application.

## TODOs

- Improve performance...
- Implement ISeq method inlining
- Support all YARV instructions
  - `expandarray`, `reverse`, `reput`, `defineclass`, `once`, `opt_call_c_function` are not supported yet.
- Care about unexpectedly GCed object made during compilation

## License

The same as Ruby.
