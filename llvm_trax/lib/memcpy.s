	.file	"rt.bc"
	.text
	.globl	_Z6memcpyPvPKvj
	.align	2
	.type	_Z6memcpyPvPKvj,@function
	.ent	_Z6memcpyPvPKvj         # @_Z6memcpyPvPKvj
memcpy:
	.frame	r1,0,r15
	.mask	0x0
# BB#0:
	beqid     r7, ($BB0_3)
	NOP    
# BB#1:
	ADD      r3, r0, r0
$BB0_2:                                 # %.lr.ph
                                        # =>This Inner Loop Header: Depth=1
	lbu       r4, r6, r3
	sb        r4, r5, r3
	ADDI      r3, r3, 1
	CMPU      r4, r7, r3
	bltid     r4, ($BB0_2)
	NOP    
$BB0_3:                                 # %.loopexit
	rtsd      r15, 8
	ADD      r3, r5, r0
	.end	_Z6memcpyPvPKvj
$tmp0:
	.size	_Z6memcpyPvPKvj, ($tmp0)-_Z6memcpyPvPKvj


