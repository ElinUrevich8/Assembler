; 1. undefined label as first operand
        add  Q,r2

; 2. undefined label as second operand
        add  r3,Q

; 3. undefined label as first operand (label X is ignored)
        add  X,r4

; 4. undefined label as second operand (label Y is ignored)
        add  r5,Y

; 5. undefined label
       .entry Q

; 6. undefined label (label X is ignored)
       .entry X

; 7. undefined label (label Y is ignored)
       .entry Y
