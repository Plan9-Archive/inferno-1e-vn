TEXT	realcanlock(SB),$0
	MOVL	$1,AX
	MOVL	l+0(FP),BX
	XCHGL	AX,(BX)
	CMPL	AX, $0
	JEQ	ret1
	MOVL	$0, AX
	RET
ret1:
	MOVL	$1, AX
	RET

TEXT	lockinit(SB),$0
	MOVL	$realcanlock(SB),AX
	MOVL	l+0(FP),BX
	MOVL	AX,(BX)
	RET

TEXT	tramp(SB),$0
	MOVL	nsp+0(FP), BX		/* new stack */
	MOVL	fn+4(FP), CX		/* func to exec */
	MOVL	arg+8(FP),DX

	LEAL	-8(BX), SP		/* new stack */
	PUSHL	DX
	CALL	*CX
	POPL	AX

	PUSHL	$0
	CALL	_exits(SB)
	POPL	AX
	RET

TEXT	getcallerpc(SB),$0
	MOVL	l+0(FP),AX
	MOVL	-4(AX), AX
	RET

TEXT	vstack(SB),$0
	MOVL	arg+0(FP), AX
	MOVL	ustack(SB), SP
	PUSHL	AX
	CALL	exectramp(SB)
	POPL	AX			/* dammit ken! */
	RET

TEXT	FPsave(SB), 1, $0
	MOVL	fpu+0(FP), AX
	FSTENV	0(AX)
	RET

TEXT	FPrestore(SB), 1, $0
	MOVL	fpu+0(FP), AX
	FLDENV	0(AX)
	RET
