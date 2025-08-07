mcro LOOP
    inc r1
    dec r2
mcroend

mcro PRINT
    prn r3
mcroend

START:  mov r4, r5
        LOOP
        PRINT
        stop