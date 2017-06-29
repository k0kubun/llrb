; ModuleID = 'llrb'

define i64 @llrb_exec(i64, i64) {
label_0:
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 8, i64 52)
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  call void @llrb_insn_setlocal_level0(i64 %1, i64 3, i64 1)
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  br label %label_25

label_15:                                         ; preds = %label_25
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 1, i64 52)
  %getlocal1 = call i64 @llrb_insn_getlocal_level0(i64 %1, i64 3)
  %opt_plus = call i64 @llrb_insn_opt_plus(i64 %getlocal1, i64 3)
  call void @llrb_insn_setlocal_level0(i64 %1, i64 3, i64 %opt_plus)
  br label %label_25

label_25:                                         ; preds = %label_15, %label_0
  %getlocal = call i64 @llrb_insn_getlocal_level0(i64 %1, i64 3)
  %opt_lt = call i64 @llrb_insn_opt_lt(i64 %getlocal, i64 12000001)
  %RTEST_mask = and i64 %opt_lt, -9
  %RTEST = icmp ne i64 %RTEST_mask, 0
  br i1 %RTEST, label %label_15, label %label_34

label_34:                                         ; preds = %label_25
  call void @llrb_insn_trace(i64 %0, i64 %1, i32 16, i64 8)
  call void @llrb_push_result(i64 %1, i64 8)
  ret i64 %1
}

declare void @llrb_insn_trace(i64, i64, i32, i64)

declare void @llrb_insn_setlocal_level0(i64, i64, i64)

declare i64 @llrb_insn_getlocal_level0(i64, i64)

declare i64 @llrb_insn_opt_lt(i64, i64)

declare void @llrb_push_result(i64, i64)

declare i64 @llrb_insn_opt_plus(i64, i64)
