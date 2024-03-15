main:
    add $t0, $imm1, $zero, $zero, 7, 0                     # set source sector to 7 (start from the last sector)
    add $t1, $imm1, $zero, $zero, 8, 0                     # set destination sector to 8
    sll $sp, $imm1, $imm2, $zero, 1, 11		               # set $sp = 1 << 11 = 2048 

move_sectors:
    # Read sector content from the current sector
    out $zero, $zero, $imm1, $t0, 15, 0                    # set sector
    out $zero, $zero, $imm1, $imm2, 16, 0                  # set buffer
    out $zero, $zero, $imm1, $imm2, 14, 1                  # set diskcmd to read

check_status:
    in $t2, $zero, $imm1, $zero, 17, 0                     # check the disk status
    bne $zero, $t2, $zero, $imm1, check_status, 0          # check until status is 0

    # Write sector content to the destination sector
    out $zero, $zero, $imm1, $t1, 15, 0                    # set sector
    out $zero, $zero, $imm1, $imm2, 14, 2                  # set diskcmd to write

check_status2:
    in $t2, $zero, $imm1, $zero, 17, 0                     # check the disk status
    bne $zero, $t2, $zero, $imm1, check_status2, 0          # check until status is 0

    # Move to the previous sectors
    add $t0, $imm1, $t0, $zero, -1, 0                      # decrement source sector
    add $t1, $imm1, $t1, $zero, -1, 0                      # decrement destination sector


    # Check if we have moved all 8 sectors
    bge $zero, $t0, $zero, $imm1, move_sectors, 0          # branch if source sector is greater than or equal to zero

    Halt $zero, $zero, $zero, $zero, 0, 0                   # Halt the system
