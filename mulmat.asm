.word 0x100 1
.word 0x101 2
.word 0x102 3
.word 0x103 4
.word 0x104 5
.word 0x105 6
.word 0x106 7
.word 0x107 8
.word 0x108 9
.word 0x109 10
.word 0x10A 11
.word 0x10B 12
.word 0x10C 13
.word 0x10D 14
.word 0x10E 15
.word 0x10F 16
.word 0x110 1
.word 0x111 2
.word 0x112 3
.word 0x113 4
.word 0x114 1
.word 0x115 2
.word 0x116 3
.word 0x117 4
.word 0x118 1
.word 0x119 2
.word 0x11A 3
.word 0x11B 4
.word 0x11C 1
.word 0x11d 2
.word 0x11e 3
.word 0x11f 4

#Matrix multiplication mat1*mat2 = result_mat
main:
    sll $sp, $imm1, $imm2, $zero, 1, 11 # set $sp = 1 << 11 = 2048 
    add $sp, $sp, $imm1, $zero, -3, 0 # allocate space in stack

    sw $s0, $sp, $imm1, $zero, 1, 0 #save s0
    add $s1, $sp, $imm1, $zero, 2, 0 #save s1
    add $s2, $sp, $imm1, $zero, 3, 0 #save s2
    
    add $s0, $zero, $imm1, $zero, -1, 0 # index i = -1

for1:
    add $s0, $s0, $imm1, $zero, 1, 0 #i++
    beq $zero, $imm2, $s0, $imm1, exit, 4 #if(4 =  i) exit for1
    add $s1, $zero, $imm1, $zero, -1, 0 # index j = -1

for2:

    add $s1, $s1, $imm1, $zero, 1, 0 #j++
    beq $zero, $imm2, $s1, $imm1, for1, 4 #if(4 =  j) exit for2
    add $t0, $zero, $zero, $zero, 0, 0 # temp_sum = 0
    add $s2, $zero, $zero, $zero, 0, 0 # index k = 0
  
for3:
    beq $zero, $imm2, $s2, $imm1, cal, 4 #if(4 =  k) exit for3 to cal

    mac $t1, $s0, $imm1, $zero, 4, 0 # t1 = i * 4
    add $t1, $imm1, $s2, $t1, 0x100, 0 #mat1[i][k]-> MEM[0x100 + i * 4 + k]
    lw $t1, $t1, $zero, $zero, 0, 0 # t1 <= mat1[i][k]

    mac $t2, $s2, $imm1, $zero, 4, 0 # t2 = k * 4
    add $t2, $imm1, $t2, $s1, 0x110, 0 #mat1[k][j]-> MEM[0x110 + k * 4 + j]
    lw $t2, $t2, $zero, $zero, 0, 0 # t2 <= mat2[k][j]

    mac $t0, $t1, $t2, $t0, 0, 0 # temp sum = mat1[i][k] * mat2[k][j] + temp_sum
    add $s2, $s2, $imm1, $zero, 1, 0 # k = k + 1
    beq $zero, $zero, $zero, $imm1, for3, 0 # jump back to for3

cal:
    mac $t2, $s0, $imm1, $zero, 4, 0 # t2 = i * 4
    add $t1, $imm1, $t2, $s1, 0x120, 0 # result_mat[i][j]-> MEM[0x120 + i * 4 + j]
    sw $t0, $t1, $zero, $zero, 0, 0 # result_mat[i][j] = temp_sum
    beq $zero, $zero, $zero, $imm1, for2, 0 #go to for2

exit:
    lw $s0, $sp, $imm1, $zero, 0, 0 # restore value of $s0
    lw $s0, $sp, $imm1, $zero, 1, 0 # restore value of $s1
    lw $s0, $sp, $imm1, $zero, 2, 0 # restore value of $s2
    add $sp, $sp, $imm1, $zero, 3, 0 # Restore stack
    halt $zero, $zero, $zero, $zero, 0, 0                   # Halt the system



