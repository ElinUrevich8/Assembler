; 1. data overflow (1230)
Next:		.data		6,-9 , 15 , 1230

; 2. data overflow (-513)
    .data  -513

; 3. invalid characters (#123)
        .data   #123

; 4. invalid characters (.4)
        .data   12.4

; 5. missing argument in directive
        .data

; 6. invalid first operand (128 too large to fit in 8 bits)
       cmp #128,DATA1

; 7. invalid second operand (-129 too large to fit in 8 bits)
       cmp r2,#-129

; 8. missing operand
        sub #5

; 9. missing operand
        sub #5,

; 10. missing operand
        red

; 11. invalid target operand (immediate)
        add  #5,#6

; 12. invalid operand (immediate)
        inc  #50

; 13. undefined instruction
        and  r1,r2

; 14. invalid characters (blabla)
        prn r1 blabla

; 15. invalid characters (xyz)
        cmp  #xyz,r1

; 16. label previously declared
Next:   clr  r2

; 17. invalid source operand (register)   
        lea  r3,r1

; 18. invalid source operand (immediate)   
        lea  #3,r5

; 19. invalid characters (Start)
        stop Start

; 20. invalid label(non-alphabetic first character)
1DATA:  .data   300

; 21. missing argument in directive
        .entry

; 22. undefined directive
        .invalid 85,90

; 23. invalid characters (Next)
        rts Next
       
; 24. invalid characters (xyz)
        .struct 4,xyz

; 25. invalid characters 
        .struct STRCT1,"qwerty"
				
; 26. invalid characters (r3,"qwerty")
        .struct r3,"qwerty"
		
; 27. invalid characters (r4)
        .struct 3,r4

; 28. invalid characters ("1234",2)
        .struct "1234",2

; 29. missing arguments in directive 
        .struct 

; 30. missing argiment in directive 
        .struct 4

; 31. missing agrument in directive 
        .struct 4,

; 32. invalid characters (3)
        cmp r1,STRCT1.3

; 33. invalid characters (.2)
        prn STRCT1.1.2

; 34. invalid characters(r2)
        prn STRCT1.r2