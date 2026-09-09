; ---------------------------------------------------------------------------
; subroutine.asm -- CALL / RET and the hardware stack
;
; The call stack is our own Stack<T> class, so watch the "stack" view while
; stepping through this: CALL pushes the return address, RET pops it back.
; ---------------------------------------------------------------------------

        LOAD  R0, #6
        LOAD  R1, #4
        CALL  addthem       ; R2 = R0 + R1
        OUT   R2            ; prints 10

        LOAD  R0, #20
        LOAD  R1, #22
        CALL  addthem       ; same subroutine, called again
        OUT   R2            ; prints 42

        PUSH  R2            ; the stack also works for plain data
        LOAD  R2, #0
        POP   R2            ; ... and restores it
        OUT   R2            ; prints 42 again
        HLT

; --- subroutine: R2 = R0 + R1 ----------------------------------------------
addthem:
        MOV   R2, R0
        ADD   R2, R1
        RET
