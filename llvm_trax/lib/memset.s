	.section .mdebug.abi32
	.previous
	.file	"memset.bc"
	.text
	.globl	_Z6memsetPvij
	.align	2
	.type	_Z6memsetPvij,@function
	.ent	_Z6memsetPvij           # @_Z6memsetPvij
memset:
	.frame	$sp,24,$ra
	.mask 	0x80010000,-4
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
# BB#0:                                 # %entry
	addiu	$sp, $sp, -24
	sw	$ra, 20($sp)            # 4-byte Folded Spill
	sw	$16, 16($sp)            # 4-byte Folded Spill
	addu	$16, $zero, $4
	beq	$6, $zero, $BB0_2
	nop
# BB#1:                                 # %return.loopexit
	andi	$5, $5, 255
	addu	$4, $zero, $16
	jal	memset
	nop
$BB0_2:                                 # %return
	addu	$2, $zero, $16
	lw	$16, 16($sp)            # 4-byte Folded Reload
	lw	$ra, 20($sp)            # 4-byte Folded Reload
	addiu	$sp, $sp, 24
	jr	$ra
	nop
	.set	macro
	.set	reorder
	.end	_Z6memsetPvij
$tmp2:
	.size	_Z6memsetPvij, ($tmp2)-_Z6memsetPvij


