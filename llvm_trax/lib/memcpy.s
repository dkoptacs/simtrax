	.file	"rt.bc"
	.text
	.globl	_Z6memcpyPvPKvj
	.align	2
	.type	_Z6memcpyPvPKvj,@function
	.ent	_Z6memcpyPvPKvj         # @_Z6memcpyPvPKvj
memcpy:
	.frame	r1,0,r15
	.mask	0x0
# BB#0:                                 # %entry
	ADD       r3, r0, r0
	CMP       r3, r3, r8
	beqid     r3, ($BB0_3)
	NOP    
# BB#1:                                 # %bb.lr.ph
	ADD       r3, r0, r0
	ADD       r4, r6, r0
	ADD       r10, r0, r0
$BB0_2:                                 # %bb
                                        # =>This Inner Loop Header: Depth=1
	ADDI      r5, r0, -1
	ADDK      r8, r8, r5
	ADDKC      r3, r3, r5
	lbui      r5, r7, 0
	ADDI      r9, r4, 1
	ADDI      r7, r7, 1
	BITOR     r11, r8, r3
	sbi       r5, r4, 0
	CMP       r5, r10, r11
	bneid     r5, ($BB0_2)
	ADD       r4, r9, r0
$BB0_3:                                 # %return
	rtsd      r15, 8
	ADD       r3, r6, r0
	.end	_Z6memcpyPvPKvj
$tmp0:
	.size	_Z6memcpyPvPKvj, ($tmp0)-_Z6memcpyPvPKvj


