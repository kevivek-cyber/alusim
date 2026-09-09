; ---------------------------------------------------------------------------
; multiply.asm -- multiplication two ways
;
; COAL Practical 5 asks for multiplication by successive addition and by the
; add-and-shift method.  MUL uses add-and-shift in hardware; the loop below
; does the same job by successive addition so the two can be compared.
; ---------------------------------------------------------------------------

; ---- method 1: hardware add-and-shift multiply ----------------------------
        LOAD  R0, #13
        LOAD  R1, #7
        MUL   R0, R1        ; R0 = 13 * 7 = 91, in one instruction
        OUT   R0

; ---- method 2: successive addition ----------------------------------------
        LOAD  R2, #0        ; product
        LOAD  R3, #13       ; multiplicand
        LOAD  R4, #7        ; multiplier (counts down)
        LOAD  R5, #0        ; constant zero, to compare against

again:  ADD   R2, R3        ; product += multiplicand
        DEC   R4            ; multiplier--
        MOV   R6, R4
        CMP   R6, R5        ; multiplier == 0 ?
        JNZ   again         ; no -- keep adding

        OUT   R2            ; should also print 91
        STORE R2, [0x50]
        HLT
