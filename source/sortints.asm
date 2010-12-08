data:
        mov r0, ds
        mov r1, 5 
        mov r2, 2
        mov r3, 8
        mov r4, 9
        mov r5, 6
        stw r0, r1
        add r0, r0, 4
        stw r0, r2
        add r0, r0, 4
        stw r0, r3
        add r0, r0, 4
        stw r0, r4
        add r0, r0, 4
        stw r0, r5
        add r0, r0, 4
        mov r0, ds
        mov r14, ds
main:
        ldw r1, r0
        mov r10, 0
        cmp r1, r10
    eq  b end
        add r0, r0, 4
        ldw r2, r0
        cmp r1, r2
    lt  b main
        stw r0, r1 
        sub r0, r0, 4
        stw r0, r2
    al  b main
end:
add r14, r14, 1
