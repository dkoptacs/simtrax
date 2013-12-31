	.file	"memset.bc"
	.text
	.globl	_Z6memsetPvij
	.align	2
	.type	_Z6memsetPvij,@function
	.ent	_Z6memsetPvij           # @_Z6memsetPvij
memset:
	.frame	r1,16,r15
	.mask	0x0
# BB#0:                                 # %entry
	ADDI      r1, r1, -16
	SWI       r6, r1, 24
	SWI       r7, r1, 28
	SWI       r8, r1, 32
	brid      ($BB0_2)
	LWI       r3, r1, 24
$BB0_1:                                 # %bb
                                        #   in Loop: Header=BB0_2 Depth=1
	LWI       r3, r1, 28
	LWI       r4, r1, 12
	sbi       r3, r4, 0
	LWI       r3, r1, 12
	ADDI      r3, r3, 1
$BB0_2:                                 # %bb
                                        # =>This Inner Loop Header: Depth=1
	SWI       r3, r1, 12
	LWI       r3, r1, 32
	ADD       r4, r0, r0
	CMP       r4, r4, r3
	bneid     r4, ($BB0_4)
	ADDI      r5, r0, 1
# BB#3:                                 # %bb1
                                        #   in Loop: Header=BB0_2 Depth=1
	ADDI      r5, r0, 0
$BB0_4:                                 # %bb1
                                        #   in Loop: Header=BB0_2 Depth=1
	ADDI      r3, r3, -1
	SWI       r3, r1, 32
	bneid     r4, ($BB0_1)
	sbi       r5, r1, 8
# BB#5:                                 # %bb2
	LWI       r3, r1, 24
	SWI       r3, r1, 4
	SWI       r3, r1, 0
	LWI       r3, r1, 0
	rtsd      r15, 8
	ADDI      r1, r1, 16
	.end	_Z6memsetPvij
$tmp0:
	.size	_Z6memsetPvij, ($tmp0)-_Z6memsetPvij


