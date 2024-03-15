.word 0x100 20
.word 0x101 8

    sll $sp, $imm1, $imm2, $zero, 1, 11		     # set $sp = 1 << 11 = 2048 
    add $sp, $sp, $imm1, $zero, -2, 0            # Allocate space on the stack    
    sw $a0, $sp, $imm1, $zero,  0, 0             # Store Current a0
    sw $a1, $sp, $imm1, $zero,  1, 0             # Store Current a1 
    lw $a0, $imm1, $zero, $zero, 0x100, 0        # Load n into $a0 from address
    lw $a1, $imm1, $zero, $zero, 0x101, 0        # Load k into $a1 from address
    jal $ra, $zero, $zero, $imm1, binom, 0       # function call binom, save return addr in $ra
    lw $a0, $sp, $imm1, $zero, 0, 0              # Restore  $a0
    lw $a1, $sp, $imm1, $zero, 1, 0              # Restore $a1 
    add $sp, $sp, $imm1, $zero, 2, 0             # Free stack space
    sw $zero, $imm1, $zero, $v0, 0x102, 0        # Store result at address 0x102
    halt $zero, $zero, $zero, $zero, 0, 0	     # halt (exit)

binom:
    add $sp, $sp, $imm1, $zero, -4, 0            # Allocate space on the stack for 4    
    sw $a0, $sp, $imm1, $zero, 0, 0              # Store Current n
    sw $a1, $sp, $imm1, $zero, 1, 0              # Store Current k
    sw $s0, $sp, $imm1, $zero, 2, 0              # Store Current $s0
    sw $ra, $sp, $imm1, $zero, 3, 0              # Store Current $ra

     
    beq $zero, $a1, $zero, $imm1, base_case, 0   # Base cases: binom(n, 0)
    beq $zero, $a0, $a1, $imm1, base_case, 0     # Base cases: binom(n, n)
    blt $zero, $a0, $a1, $imm1,  smaller_case, 0 # Base cases: binom(n, k), n<k

    # Recursive calls: binom(n-1, k-1) + binom(n-1, k)
    add $a0, $a0, $imm1, $zero, -1, 0            # claculate n-1
    add $a1, $a1, $imm1, $zero, -1, 0            # calculate k-1
    jal $ra, $zero, $zero, $imm1, binom, 0       # Recursive call for binom(n-1, k-1)
    add $s0, $v0, $zero, $zero, 0, 0             # $s0 = binom(n-1, k-1)

    add $a1, $a1, $imm1, $zero, 1, 0             # calculate (k-1)+1
    jal $ra, $zero, $zero, $imm1, binom, 0       # Recursive call for binom(n-1, k)
    add $v0, $v0, $s0, $zero, 0, 0               # $v0 = binom(n-1, k-1) + binom(n-1, k)
    beq $zero, $zero, $zero, $imm1, end_binom, 0 #finish

base_case:
    add $v0, $imm1, $zero, $zero, 1, 0           # Set the result to 1
    beq $zero, $zero, $zero, $imm1, end_binom, 0 #finish


smaller_case:
    add $v0, $zero, $zero, $zero, 0, 0           # Set the result to 0


end_binom:
    lw $a0, $sp, $imm1, $zero, 0, 0              # restore $a0 from stack
    lw $a1, $sp, $imm1, $zero, 1, 0              # restore $a1 from stack
    lw $s0, $sp, $imm1, $zero, 2, 0              # restore $s0 from stack
    lw $ra, $sp, $imm1, $zero, 3, 0              # restore $ra from stack
    add $sp, $sp, $imm1, $zero, 4, 0             # Deallocate space on the stack

    beq $zero, $zero, $zero, $ra, 0, 0           # Return from function in address in $ra
