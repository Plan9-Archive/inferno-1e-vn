/*
 * VM instruction set
 */
enum
{
	INOP,
	IALT,
	INBALT,
	IGOTO,
	ICALL,
	IFRAME,
	ISPAWN,
	IRUNT,
	ILOAD,
	IMCALL,
	IMSPAWN,
	IMFRAME,
	IRET,
	IJMP,
	ICASE,
	IEXIT,
	INEW,
	INEWA,
	INEWCB,
	INEWCW,
	INEWCF,
	INEWCP,
	INEWCM,
	INEWCMP,
	ISEND,
	IRECV,
	ICONSB,
	ICONSW,
	ICONSP,
	ICONSF,
	ICONSM,
	ICONSMP,
	IHEADB,
	IHEADW,
	IHEADP,
	IHEADF,
	IHEADM,
	IHEADMP,
	ITAIL,
	ILEA,
	IINDX,
	IMOVP,
	IMOVM,
	IMOVMP,
	IMOVB,
	IMOVW,
	IMOVF,
	ICVTBW,
	ICVTWB,
	ICVTFW,
	ICVTWF,
	ICVTCA,
	ICVTAC,
	ICVTWC,
	ICVTCW,
	ICVTFC,
	ICVTCF,
	IADDB,
	IADDW,
	IADDF,
	ISUBB,
	ISUBW,
	ISUBF,
	IMULB,
	IMULW,
	IMULF,
	IDIVB,
	IDIVW,
	IDIVF,
	IMODW,
	IMODB,
	IANDB,
	IANDW,
	IORB,
	IORW,
	IXORB,
	IXORW,
	ISHLB,
	ISHLW,
	ISHRB,
	ISHRW,
	IINSC,
	IINDC,
	IADDC,
	ILENC,
	ILENA,
	ILENL,
	IBEQB,
	IBNEB,
	IBLTB,
	IBLEB,
	IBGTB,
	IBGEB,
	IBEQW,
	IBNEW,
	IBLTW,
	IBLEW,
	IBGTW,
	IBGEW,
	IBEQF,
	IBNEF,
	IBLTF,
	IBLEF,
	IBGTF,
	IBGEF,
	IBEQC,
	IBNEC,
	IBLTC,
	IBLEC,
	IBGTC,
	IBGEC,
	ISLICEA,
	ISLICELA,
	ISLICEC,
	IINDW,
	IINDF,
	IINDB,
	INEGF,
	IMOVL,
	IADDL,
	ISUBL,
	IDIVL,
	IMODL,
	IMULL,
	IANDL,
	IORL,
	IXORL,
	ISHLL,
	ISHRL,
	IBNEL,
	IBLTL,
	IBLEL,
	IBGTL,
	IBGEL,
	IBEQL,
	ICVTLF,
	ICVTFL,
	ICVTLW,
	ICVTWL,
	ICVTLC,
	ICVTCL,
	IHEADL,
	ICONSL,
	INEWCL,
	ICASEC,
	IINDL,
	IMOVPC,
	ITCMP			/* Fix MAXDIS if you add opcodes */
};

enum
{
	MAXDIS	= ITCMP+1,

	XMAGIC	= 819248,	/* Normal magic */
	SMAGIC	= 923426,	/* Signed module */

	AMP	= 0x00,		/* Src/Dst op addressing */
	AFP	= 0x01,
	AIMM	= 0x02,
	AXXX	= 0x03,
	AIND	= 0x04,
	AMASK	= 0x07,
	AOFF	= 0x08,
	AVAL	= 0x10,

	ARM	= 0xC0,		/* Middle op addressing */
	AXNON	= 0x00,
	AXIMM	= 0x40,
	AXINF	= 0x80,
	AXINM	= 0xC0,

	DEFZ	= 0,
	DEFB	= 1,		/* Byte */
	DEFW	= 2,		/* Word */
	DEFS	= 3,		/* Utf-string */
	DEFF	= 4,		/* Real value */
	DEFA	= 5,		/* Array */
	DIND	= 6,		/* Set index */
	DAPOP	= 7,		/* Restore address register */
	DEFL	= 8,		/* BIG */

	DADEPTH = 4,		/* Array address stack size */

	REGLINK	= 0,
	REGFRAME= 1,
	REGMOD	= 2,
	REGTYP	= 3,
	REGRET	= 4,
	NREG	= 5,

	IBY2WD	= 4,
	IBY2FT	= 8,
	IBY2LG	= 8,

	MUSTCOMPILE	= (1<<0),
	DONTCOMPILE	= (1<<1),
	SHAREMP		= (1<<2)
};

#define DTYPE(x)	(x>>4)
#define DBYTE(x, l)	((x<<4)|l)
#define DMAX		(1<<4)
#define DLEN(x)		(x& (DMAX-1))

#define SRC(x)		((x)<<3)
#define DST(x)		((x)<<0)
#define USRC(x)		(((x)>>3)&AMASK)
#define UDST(x)		((x)&AMASK)
#define UXSRC(x)	((x)&(AMASK<<3))
#define UXDST(x)	((x)&(AMASK<<0))
