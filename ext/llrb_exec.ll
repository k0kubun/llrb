; ModuleID = 'llrb'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.rb_vm_struct = type { i64, %struct.rb_global_vm_lock_struct, %union.pthread_mutex_t, %struct.rb_thread_struct*, %struct.rb_thread_struct*, %struct.list_head, i64, i64, i8, i32, i32, i64, [4 x i64], i64, i64, i64, i64, i64, i64, i64, %struct.st_table*, %struct.st_table*, [65 x %struct.anon.3], %struct.rb_hook_list_struct, %struct.st_table*, %struct.rb_postponed_job_struct*, i32, i32, i64, i64, i64, i64, i64, i64, %struct.rb_objspace*, %struct.rb_at_exit_list*, i64*, %struct.st_table*, %struct.anon.4, [24 x i16] }
%struct.rb_thread_struct = type { %struct.list_node, i64, %struct.rb_vm_struct*, i64*, i64, %struct.rb_control_frame_struct*, i32, i32, i64, i32, i32, i64, %struct.rb_callable_method_entry_struct*, %struct.rb_calling_info*, i64, i64, i64*, i64, i64, i32, i32, i32, %struct.native_thread_data_struct, i8*, i64, i64, i64, i64, i64, i32, i32, i64, %union.pthread_mutex_t, %struct.rb_thread_cond_struct, %struct.rb_unblock_callback, i64, %struct.rb_mutex_struct*, %struct.rb_vm_tag*, %struct.rb_vm_protect_tag*, %struct.st_table*, i64, i64, %struct.rb_thread_list_struct*, i64, i64, i64 (...)*, %struct.anon.5, i64, %struct.rb_hook_list_struct, %struct.rb_trace_arg_struct*, %struct.rb_fiber_struct*, %struct.rb_fiber_struct*, [1 x %struct.__jmp_buf_tag], %struct.rb_ensure_list*, i16, i8*, i64, i64 }
%struct.rb_unblock_callback = type { void (i8*)*, i8* }
%struct.st_table = type { i8, i8, i8, i32, %struct.st_hash_type*, i64, i64*, i64, i64, %struct.st_table_entry* }
%struct.anon.5 = type { i64*, i64*, i64, [1 x %struct.__jmp_buf_tag] }
%union.pthread_mutex_t = type { %struct.__pthread_mutex_s }
%struct.rb_thread_cond_struct = type { %union.pthread_cond_t, i32 }
%union.pthread_cond_t = type { %struct.anon.2 }
%struct.rb_vm_tag = type { i64, i64, [1 x %struct.__jmp_buf_tag], %struct.rb_vm_tag* }
%struct.list_head = type { %struct.list_node }
%struct.list_node = type { %struct.list_node*, %struct.list_node* }
%struct.rb_hook_list_struct = type { %struct.rb_event_hook_struct*, i32, i32 }
%struct.rb_event_hook_struct = type opaque
%struct.anon.3 = type { i64, i32 }
%struct.anon.4 = type { i64, i64, i64, i64 }
%struct.rb_objspace = type opaque
%struct.native_thread_data_struct = type { %struct.list_node, %struct.rb_thread_cond_struct }
%struct.__jmp_buf_tag = type { [8 x i64], i32, %struct.__sigset_t }
%struct.__sigset_t = type { [16 x i64] }
%struct.rb_fiber_struct = type opaque
%struct.rb_ensure_list = type { %struct.rb_ensure_list*, %struct.rb_ensure_entry }
%struct.rb_ensure_entry = type { i64, i64 (...)*, i64 }
%struct.rb_control_frame_struct = type { i64*, i64*, %struct.rb_iseq_struct*, i64, i64*, i8* }
%struct.st_table_entry = type opaque
%struct.rb_mutex_struct = type opaque
%struct.st_hash_type = type { i32 (...)*, i64 (...)* }
%struct.rb_postponed_job_struct = type opaque
%struct.rb_global_vm_lock_struct = type { i64, %union.pthread_mutex_t, i64, %struct.rb_thread_cond_struct, %struct.rb_thread_cond_struct, %struct.rb_thread_cond_struct, i32, i32 }
%struct.rb_vm_protect_tag = type { %struct.rb_vm_protect_tag* }
%struct.rb_callable_method_entry_struct = type { i64, i64, %struct.rb_method_definition_struct*, i64, i64 }
%struct.rb_method_definition_struct = type { i8, i32, i32, %union.anon, i64 }
%union.anon = type { %struct.rb_method_cfunc_struct }
%struct.rb_method_cfunc_struct = type { i64 (...)*, i64 (i64 (...)*, i64, i32, i64*)*, i32 }
%struct.rb_calling_info = type { i64, i64, i32 }
%struct.rb_at_exit_list = type { void (%struct.rb_vm_struct*)*, %struct.rb_at_exit_list* }
%struct.rb_trace_arg_struct = type { i32, %struct.rb_thread_struct*, %struct.rb_control_frame_struct*, i64, i64, i64, i64, i64, i32, i32, i64 }
%struct.__pthread_mutex_s = type { i32, i32, i32, i32, i32, i16, i16, %struct.__pthread_internal_list }
%struct.__pthread_internal_list = type { %struct.__pthread_internal_list*, %struct.__pthread_internal_list* }
%struct.rb_iseq_struct = type { i64, i64, %struct.rb_iseq_constant_body*, %union.anon.8 }
%struct.rb_iseq_constant_body = type { i32, i32, i64*, %struct.anon, %struct.rb_iseq_location_struct, %struct.iseq_line_info_entry*, i64*, %struct.iseq_catch_table*, %struct.rb_iseq_struct*, %struct.rb_iseq_struct*, %union.iseq_inline_storage_entry*, %struct.rb_call_info*, %struct.rb_call_cache*, i64, i32, i32, i32, i32, i32, i32 }
%struct.rb_call_cache = type { i64, i64, %struct.rb_callable_method_entry_struct*, i64 (%struct.rb_thread_struct*, %struct.rb_control_frame_struct*, %struct.rb_calling_info*, %struct.rb_call_info*, %struct.rb_call_cache*)*, %union.anon.7 }
%struct.anon = type { %struct.anon.0, i32, i32, i32, i32, i32, i32, i32, i64*, %struct.rb_iseq_param_keyword* }
%struct.rb_iseq_param_keyword = type { i32, i32, i32, i32, i64*, i64* }
%struct.iseq_line_info_entry = type opaque
%union.anon.8 = type { %struct.anon.9 }
%struct.anon.9 = type { i64, i32 }
%union.iseq_inline_storage_entry = type { %struct.iseq_inline_cache_entry }
%struct.iseq_inline_cache_entry = type { i64, %struct.rb_cref_struct*, %union.anon.6 }
%union.anon.6 = type { i64 }
%struct.rb_call_info = type { i64, i32, i32 }
%union.anon.7 = type { i32 }
%struct.rb_iseq_location_struct = type { i64, i64, i64, i64, i64 }
%struct.rb_thread_list_struct = type { %struct.rb_thread_list_struct*, %struct.rb_thread_struct* }
%struct.iseq_catch_table = type opaque
%struct.rb_cref_struct = type { i64, i64, i64, %struct.rb_cref_struct*, %struct.rb_scope_visi_struct }
%struct.rb_scope_visi_struct = type { i8, [3 x i8] }
%struct.anon.0 = type { i8, [3 x i8] }
%struct.anon.2 = type { i32, i32, i64, i64, i64, i8*, i32, i32 }

; Function Attrs: nounwind uwtable
define %struct.rb_control_frame_struct* @llrb_exec(%struct.rb_thread_struct* %th, %struct.rb_control_frame_struct* %cfp) {
  %1 = alloca %struct.rb_thread_struct*, align 8
  %2 = alloca %struct.rb_control_frame_struct*, align 8
  store %struct.rb_thread_struct* %th, %struct.rb_thread_struct** %1, align 8
  store %struct.rb_control_frame_struct* %cfp, %struct.rb_control_frame_struct** %2, align 8
  %3 = load %struct.rb_thread_struct*, %struct.rb_thread_struct** %1, align 8
  %4 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_trace(%struct.rb_thread_struct* %3, %struct.rb_control_frame_struct* %4, i32 8, i64 52)
  %5 = load %struct.rb_thread_struct*, %struct.rb_thread_struct** %1, align 8
  %6 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_trace(%struct.rb_thread_struct* %5, %struct.rb_control_frame_struct* %6, i32 1, i64 52)
  %7 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_setlocal_level0(%struct.rb_control_frame_struct* %7, i64 3, i64 1)
  %8 = load %struct.rb_thread_struct*, %struct.rb_thread_struct** %1, align 8
  %9 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_trace(%struct.rb_thread_struct* %8, %struct.rb_control_frame_struct* %9, i32 1, i64 52)
  br label %10

; <label>:10                                      ; preds = %17, %0
  %11 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  %12 = call i64 @llrb_insn_getlocal_level0(%struct.rb_control_frame_struct* %11, i64 3)
  %13 = call i64 @llrb_insn_opt_lt(i64 %12, i64 12000001)
  %14 = and i64 %13, -9
  %15 = icmp eq i64 %14, 0
  %16 = xor i1 %15, true
  br i1 %16, label %17, label %24

; <label>:17                                      ; preds = %10
  %18 = load %struct.rb_thread_struct*, %struct.rb_thread_struct** %1, align 8
  %19 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_trace(%struct.rb_thread_struct* %18, %struct.rb_control_frame_struct* %19, i32 1, i64 52)
  %20 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  %21 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  %22 = call i64 @llrb_insn_getlocal_level0(%struct.rb_control_frame_struct* %21, i64 3)
  %23 = call i64 @llrb_insn_opt_plus(i64 %22, i64 3)
  call void @llrb_insn_setlocal_level0(%struct.rb_control_frame_struct* %20, i64 3, i64 %23)
  br label %10

; <label>:24                                      ; preds = %10
  %25 = load %struct.rb_thread_struct*, %struct.rb_thread_struct** %1, align 8
  %26 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_insn_trace(%struct.rb_thread_struct* %25, %struct.rb_control_frame_struct* %26, i32 16, i64 8)
  %27 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  call void @llrb_push_result(%struct.rb_control_frame_struct* %27, i64 8)
  %28 = load %struct.rb_control_frame_struct*, %struct.rb_control_frame_struct** %2, align 8
  ret %struct.rb_control_frame_struct* %28
}

declare void @llrb_insn_trace(%struct.rb_thread_struct*, %struct.rb_control_frame_struct*, i32, i64)

declare void @llrb_insn_setlocal_level0(%struct.rb_control_frame_struct* %cfp, i64 %idx, i64 %val)

declare i64 @llrb_insn_getlocal_level0(%struct.rb_control_frame_struct* %cfp, i64 %idx)

declare i64 @llrb_insn_opt_lt(i64 %lhs, i64 %rhs)

declare void @llrb_push_result(%struct.rb_control_frame_struct* %cfp, i64 %result)

declare i64 @llrb_insn_opt_plus(i64, i64)
