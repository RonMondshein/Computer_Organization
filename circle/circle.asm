.word 0x100 10

    sll $sp, $imm1, $imm2, $zero, 1, 11     # set $sp = 1 << 11 = 2048
    add $sp, $sp, $imm1, $zero, -3, 0            # adjust stack for 3 items
    sw $zero, $sp, $imm1, $s2, 2, 0      
    sw $zero, $sp, $imm1, $s1, 1, 0
    sw $zero, $sp, $imm1, $s0, 0, 0
    lw $s2, $imm1, $zero, $zero, 0x100, 0        # Load the radius value
    mac $s2, $s2, $s2, $zero, 0, 0               # calculate r^2
   
    # center coordinates

    add $s1, $imm1, $zero, $zero, 128, 0                     # Initialize x center
    add $s0, $imm1, $zero, $zero, 128, 0                     # Initialize y center

    add $t0, $zero, $zero, $zero, 0, 0                       # Initialize counter for x    
   

    loop_x:
        add $t1, $zero, $zero, $zero, 0, 0                   # Initialize counter for y

    loop_y:
        add $t2, $imm1, $zero, $zero, 256, 0              # loop limit
        beq $zero, $t0, $t2, $imm2, 0, end_function               # exit condition
        sub $t2, $s1, $t0, $zero, 0, 0                    # (pixelX - screenCenter)
        sub $a0, $s0, $t1, $zero, 0, 0                    # (pixelY - screenCenter)
        mac $t2, $t2, $t2, $zero, 0, 0                    # Square the result
        mac $a0, $a0, $a0, $zero, 0, 0                    # Square the result
        add $a0, $t2, $a0, $zero, 0, 0                    # Sum of squared values
        ble $zero, $a0, $s2, $imm1, pixel_on, 0           # color if within the radius

    adding_y:
        add $t1, $t1, $imm1, $zero, 1, 0                  # y += 1
        add $a0, $zero, $imm1, $zero, 256, 0
        bne $zero, $t1, $a0, $imm1, loop_y, 0             # check if should do another y iteration
        add $t0, $t0, $imm1, $zero, 1, 0                  # or start a new x
        jal  $ra, $zero, $zero, $imm1, loop_x, 0      

    pixel_on:
        mac $a1, $t0, $imm1, $zero, 256, 0
        add $a1, $a1, $t1, $zero, 0, 0                    # pixel = 256*x + y
        out $zero, $zero, $imm1, $a1, 20, 0               # set pixel address            
        out $zero, $zero, $imm2, $imm1, 255, 21      # set pixel color to white
        out $zero, $zero, $imm2, $imm1, 1, 22      # draw pixel
        jal  $ra, $zero, $zero, $imm1, adding_y, 0

   
    end_function:
        lw $s0, $sp, $imm1, $zero, 0, 0
        lw $s1, $sp, $imm1, $zero, 1, 0
        lw $s2, $sp, $imm1, $zero, 2, 0
        add $sp, $sp, $imm1, $zero, 3, 0                     # pop 3 items from stack
        halt $zero, $zero, $zero, $zero, 0, 0