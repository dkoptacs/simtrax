	.file	"rt.bc"
	.text
	.globl	_Z6memsetPvij
	.align	2
	.type	_Z6memsetPvij,@function
	.ent	_Z6memsetPvij           # @_Z6memsetPvij
memset:
	.frame	r1,12,r15
	.mask	0x0
# BB#0:
	ADDI      r1, r1, -12
	SWI       r5, r1, 16
	SWI       r6, r1, 20
	SWI       r7, r1, 24
	beqid     r7, ($BB0_4)
	NOP    
# BB#1:
	LWI       r3, r1, 16
	SWI       r3, r1, 4
	brid      ($BB0_2)
	SWI       r0, r1, 8
$BB0_3:                                 #   in Loop: Header=BB0_2 Depth=1
	LWI       r3, r1, 4
	LWI       r4, r1, 8
	LWI       r5, r1, 20
	sb        r5, r3, r4
	LWI       r3, r1, 8
	ADDI      r3, r3, 1
	SWI       r3, r1, 8
$BB0_2:                                 # =>This Inner Loop Header: Depth=1
	LWI       r3, r1, 8
	LWI       r4, r1, 24
	CMPU      r3, r4, r3
	bltid     r3, ($BB0_3)
	NOP    
$BB0_4:
	LWI       r3, r1, 16
	SWI       r3, r1, 0
	LWI       r3, r1, 0
	rtsd      r15, 8
	ADDI      r1, r1, 12
	.end	_Z6memsetPvij
$tmp0:
	.size	_Z6memsetPvij, ($tmp0)-_Z6memsetPvij


