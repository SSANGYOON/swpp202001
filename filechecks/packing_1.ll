define void @main() {
; CHECK:     start main 0:
; CHECK:          [[P_ZEXT:%[a-zA-Z0-9_]+]] = zext i32 15 to i64
; CHECK-NEXT:     [[Q_ZEXT:%[a-zA-Z0-9_]+]] = zext i32 16 to i64
; CHECK-NEXT:     [[P_SHL:%[a-zA-Z0-9_]+]] = shl i64 [[P_ZEXT]], 32
; CHECK-NEXT:     [[FIT:%[a-zA-Z0-9_]+]] = add i64 [[Q_ZEXT]], [[P_SHL]]
; CHECK-NEXT:     [[P_TMP1:%[a-zA-Z0-9_]+]] = lshr i64 [[FIT]], 32
; CHECK-NEXT:     [[P_TMP2:%[a-zA-Z0-9_]+]] = urem i64 [[P_TMP1]], 4294967296
; CHECK-NEXT:     [[_P:%[a-zA-Z0-9_]+]] = trunc i64 [[P_TMP2]] to 32
; CHECK-NEXT:     [[Q_TMP1:%[a-zA-Z0-9_]+]] = lshr i64 [[FIT]], 0
; CHECK-NEXT:     [[Q_TMP2:%[a-zA-Z0-9_]+]] = urem i64 [[Q_TMP1]], 4294967296
; CHECK-NEXT:     [[_Q:%[a-zA-Z0-9_]+]] = trunc i64 [[Q_TMP2]] to 32
; CHECK-NEXT:     [[R:%[a-zA-Z0-9_]+]] = add i32 [[_P]], [[_Q]]
; CHECK-NEXT:     ret void  
    %p = alloca i32
    %q = alloca i32

    store i32 15, i32* %p
    store i32 16, i32* %q

    %_P = load i32, i32* %p
    %_Q = load i32, i32* %q

    %r = add i32 %_P, %_Q
    ret void
}
; CHECK:     end main