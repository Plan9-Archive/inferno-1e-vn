TEXT	umult(SB),$0
	MOVL	m1+0(FP), AX
	MOVL	m2+4(FP), CX
	MOVL	hi+8(FP), BX
	MULL	CX
	MOVL	DX, (BX)
	RET
