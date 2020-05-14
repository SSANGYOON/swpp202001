define i32 @src() {
; CHECK:     start main 0:
; CHECK:          [[A_ZEXT:%[a-zA-Z0-9_]+]] = zext i32 15 to i64
; CHECK-NEXT:     [[B_ZEXT:%[a-zA-Z0-9_]+]] = zext i32 16 to i64
; CHECK-NEXT:     [[A_SHL:%[a-zA-Z0-9_]+]] = shl i64 [[A_ZEXT]], 32
; CHECK-NEXT:     [[FIT:%[a-zA-Z0-9_]+]] = add i64 [[B_ZEXT]], [[A_SHL]]

; CHECK-NEXT:     [[A_TMP1:%[a-zA-Z0-9_]+]] = lshr i64 [[FIT]], 32
; CHECK-NEXT:     [[A_TMP2:%[a-zA-Z0-9_]+]] = urem i64 [[A_TMP1]], 4294967296
; CHECK-NEXT:     [[_A:%[a-zA-Z0-9_]+]] = trunc i64 [[A_TMP2]] to 32
; CHECK-NEXT:     [[B_TMP1:%[a-zA-Z0-9_]+]] = lshr i64 [[FIT]], 0
; CHECK-NEXT:     [[B_TMP2:%[a-zA-Z0-9_]+]] = urem i64 [[B_TMP1]], 4294967296
; CHECK-NEXT:     [[_B:%[a-zA-Z0-9_]+]] = trunc i64 [[B_TMP2]] to 32

; CHECK-NEXT:     [[A_OR_B:%[a-zA-Z0-9_]+]] = icmp sge i32 %_A, %_B

; CHECK:     GA:
; CHECK-NEXT:  ret i32 [[_A]]
; CHECK:     GB:
; CHECK-NEXT:  ret i32 [[_B]]

    %a = alloca i32
    %b = alloca i32

    store i32 15, i32* %a
    store i32 16, i32* %b

    %_A = load i32, i32* %a
    %_B = load i32, i32* %b

    %a_or_b=icmp sge i32 %_A, %_B
    br i1 %a_or_b , label %GA, label %GB
    GA:
    ret i32 %_A
    GB:
    ret i32 %_B
}
; CHECK:     end main
