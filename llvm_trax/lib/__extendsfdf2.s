	.file	"extend2.bc"
	.text
	.globl	__extendsfdf2
	.align	2
	.type	__extendsfdf2,@function
	.ent	__extendsfdf2      # @__extendsfdf2
__extendsfdf2:
	.frame	r1,0,r15
	.mask	0x0
# BB#0:
	rtsd      r15, 8
	ADD      r3, r5, r0
	.end	__extendsfdf2
$tmp0:
	.size	__extendsfdf2, ($tmp0)-__extendsfdf2


