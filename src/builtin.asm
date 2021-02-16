_PrintInt:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        li      $v0, 1
        lw      $a0, 4($fp)
        syscall
        move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_PrintString:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        li      $v0, 4
        lw      $a0, 4($fp)
        syscall
        move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_PrintBool:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        lw      $t1, 4($fp)
        blez    $t1, fbr
        li      $v0, 4
        la      $a0, TRUE
        syscall
        b end

fbr:    li      $v0, 4
        la      $a0, FALSE
        syscall

end:    move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_Alloc:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        li      $v0, 9
        lw      $a0, 4($fp)
        syscall
        move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_StringEqual:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        subu    $sp, $sp, 4

        li      $v0, 0


        lw      $t0, 4($fp)
        li      $t3, 0

bloop1: lb      $t5, ($t0)
        beqz    $t5, eloop1
        addi    $t0, 1
        addi    $t3, 1
        b       bloop1

eloop1:
        lw      $t1, 8($fp)
        li      $t4, 0

bloop2: lb      $t5, ($t1)
        beqz    $t5, eloop2
        addi    $t1, 1
        addi    $t4, 1
        b       bloop2

eloop2: bne     $t3,$t4,end1

        lw      $t0, 4($fp)
        lw      $t1, 8($fp)
        li      $t3, 0

bloop3: lb      $t5, ($t0)
        lb      $t6, ($t1)
        bne     $t5, $t6, end1
        beqz    $t5, eloop3
        addi    $t3, 1
        addi    $t0, 1
        addi    $t1, 1
        b       bloop3

eloop3: li      $v0, 1

end1:   move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_Halt:
        li      $v0, 10
        syscall


_ReadInteger:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        subu    $sp, $sp, 4
        li      $v0, 5
        syscall
        move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


_ReadLine:
        subu    $sp, $sp, 8
        sw      $fp, 8($sp)
        sw      $ra, 4($sp)
        addiu   $fp, $sp, 8
        subu    $sp, $sp, 4


        li      $a0, 128
        li      $v0, 9
        syscall


        li      $a1, 128
        move    $a0, $v0
        li      $v0, 8
        syscall

        move    $t1, $a0

bloop4: lb      $t5, ($t1)
        beqz    $t5, eloop4
        addi    $t1, 1
        b       bloop4

eloop4: addi    $t1, -1
        li      $t6, 0
        sb      $t6, ($t1)

        move    $v0, $a0
        move    $sp, $fp
        lw      $ra, -4($fp)
        lw      $fp, 0($fp)
        jr      $ra


.data
TRUE:.asciiz "true"
FALSE:.asciiz "false"
SPACE:.asciiz "Making Space For Inputed Values Is Fun."
NEWLINE:.asciiz "\n"
errorMsg: .asciiz "Semantic Error"

