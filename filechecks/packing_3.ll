define i32 @f(i32* %i){
; CHECK:     start main 0:
; CHECK:     [[P:%[a-zA-Z0-9_]+]] = alloc i32
; CHECK-NEXT:     [[Q:%[a-zA-Z0-9_]+]] = alloc i32
; CHECK-NEXT:     [[B:%[a-zA-Z0-9_]+]] = load i32, i32* [[I:%[a-zA-Z0-9_]+]]
; CHECK-NEXT:     store i32 [[B]], i32* [[P]]
; CHECK-NEXT:     [[C:%[a-zA-Z0-9_]+]] = load i32, i32* [[I]]
; CHECK-NEXT:     store i32 [[C]], i32* [[Q]]
; CHECK-NEXT:     [[E:%[a-zA-Z0-9_]+]] = load i32, i32* [[P]]
; CHECK-NEXT:     [[F:%[a-zA-Z0-9_]+]] = load i32, i32* [[Q]] 
; CHECK-NEXT:     [[RESULT:%[a-zA-Z0-9_]+]] = add i32 [[E]], [[F]]
; CHECK-NEXT:     ret [[RESULT]]

    %p = alloc i32
    %q = alloc i32

    %b = load i32, i32* i
    store i32 %b, i32* %p

    %c = load i32, i32* i
    store i32 %c, i32* %q

    %e = load i32, i32* %p
    %f = load i32, i32* %q
    
    %result = add i32 %e, %f
    ret result
}
; CHECK:     end main