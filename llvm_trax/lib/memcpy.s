	.section .mdebug.abi32
	.previous
	.file	"memcpy.bc"
	.text
	.globl	_Z6memcpyPvPKvj
	.align	2
	.type	_Z6memcpyPvPKvj,@function
	.ent	_Z6memcpyPvPKvj         # @_Z6memcpyPvPKvj
memcpy:
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
# BB#0:                                 # %entry
	beq	$6, $zero, $BB0_3
	nop
# BB#1:
	addu	$2, $zero, $4
$BB0_2:                                 # %for.body
                                        # =>This Inner Loop Header: Depth=1
	lbu	$3, 0($5)
	sb	$3, 0($2)
	addiu	$5, $5, 1
	addiu	$2, $2, 1
	addiu	$6, $6, -1
	bne	$6, $zero, $BB0_2
	nop
$BB0_3:                                 # %return
	addu	$2, $zero, $4
	jr	$ra
	nop
	.set	macro
	.set	reorder
	.end	_Z6memcpyPvPKvj
$tmp0:
	.size	_Z6memcpyPvPKvj, ($tmp0)-_Z6memcpyPvPKvj


