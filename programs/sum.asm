; ---------------------------------------------------------------------------
; sum.asm -- add the numbers 1..N and print the total
;
; Demonstrates: immediate load, a counted loop, CMP + conditional branch,
;               storing to memory, and program output.
; ---------------------------------------------------------------------------

        LOAD  R0, #0        ; R0 = running total
        LOAD  R1, #1        ; R1 = counter
        LOAD  R2, #11       ; R2 = limit (sum 1..10, stop when counter hits 11)

loop:   ADD   R0, R1        ; total += counter
        INC   R1            ; counter++
        MOV   R3, R1
        CMP   R3, R2        ; counter == limit ?
        JNZ   loop          ; no -- go round again

        STORE R0, [0x40]    ; keep the answer in memory
        OUT   R0            ; print it (1+2+...+10 = 55)
        HLT
