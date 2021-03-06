#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"
#include	"../port/netif.h"

/*
 *  Driver for the ns16552.
 */
enum
{
	/*
	 *  register numbers
	 */
	Data=	0,		/* xmit/rcv buffer */
	Iena=	1,		/* interrupt enable */
	 Ircv=	(1<<0),		/*  for char rcv'd */
	 Ixmt=	(1<<1),		/*  for xmit buffer empty */
	 Irstat=(1<<2),		/*  for change in rcv'er status */
	 Imstat=(1<<3),		/*  for change in modem status */
	Istat=	2,		/* interrupt flag (read) */
	 Fenabd=(3<<6),		/*  on if fifo's enabled */
	Fifoctl=2,		/* fifo control (write) */
	 Fena=	(1<<0),		/*  enable xmit/rcv fifos */
	 Ftrig1=	0,			/* trigger after 1 input character */
	 Ftrig4=	(1<<6),		/*  trigger after 4 input characters */
	 Ftrig8=	(1<<7),		/*  trigger after 8 input characters */
	 Ftrig14=	(3<<6),		/*  trigger after 14 input characters */
	 Fclear=(3<<1),		/*  clear xmit & rcv fifos */
	Format=	3,		/* byte format */
	 Bits8=	(3<<0),		/*  8 bits/byte */
	 Stop2=	(1<<2),		/*  2 stop bits */
	 Pena=	(1<<3),		/*  generate parity */
	 Peven=	(1<<4),		/*  even parity */
	 Pforce=(1<<5),		/*  force parity */
	 Break=	(1<<6),		/*  generate a break */
	 Dra=	(1<<7),		/*  address the divisor */
	Mctl=	4,		/* modem control */
	 Dtr=	(1<<0),		/*  data terminal ready */
	 Rts=	(1<<1),		/*  request to send */
	 Ri=	(1<<2),		/*  ring */
	 Inton=	(1<<3),		/*  turn on interrupts */
	 Loop=	(1<<4),		/*  loop back */
	Lstat=	5,		/* line status */
	 Inready=(1<<0),	/*  receive buffer full */
	 Oerror=(1<<1),		/*  receiver overrun */
	 Perror=(1<<2),		/*  receiver parity error */
	 Ferror=(1<<3),		/*  rcv framing error */
	 Outready=(1<<5),	/*  output buffer full */
	Mstat=	6,		/* modem status */
	 Ctsc=	(1<<0),		/*  clear to send changed */
	 Dsrc=	(1<<1),		/*  data set ready changed */
	 Rire=	(1<<2),		/*  rising edge of ring indicator */
	 Dcdc=	(1<<3),		/*  data carrier detect changed */
	 Cts=	(1<<4),		/*  complement of clear to send line */
	 Dsr=	(1<<5),		/*  complement of data set ready line */
	 Ring=	(1<<6),		/*  complement of ring indicator line */
	 Dcd=	(1<<7),		/*  complement of data carrier detect line */
	Scratch=7,		/* scratchpad */
	Dlsb=	0,		/* divisor lsb */
	Dmsb=	1,		/* divisor msb */

	CTLS= 023,
	CTLQ= 021,

	Stagesize= 1024,
	Dmabufsize=Stagesize/2,
	Nuart=	32,		/* max per machine */
};

typedef struct Uart Uart;
struct Uart
{
	QLock;

	Uart	*elist;		/* next enabled interface */
	char	name[NAMELEN];

	uchar	sticky[8];	/* sticky write register values */
	ulong	port;
	Lock	plock;		/* for printing variable */
	int	printing;	/* true if printing */
	ulong	freq;		/* clock frequency */
	int	opens;
	uchar	mask;		/* bits/char */
	uchar	istat;		/* last istat read */
	uchar	fifoon;		/* fifo's enabled */
	uchar	nofifo;		/* earlier chip version with nofifo */
	uchar	dmaon;		/* receiver connected to DMA channel */
	int	nxmit;	/* quota of characters to transmit */
	short	dmachan;	
	uchar	sendhup;	/* send hangup on Dsrc */
	int	enabled;
	int	dev;

	int	frame;		/* framing errors */
	int	overrun;	/* rcvr overruns */
	int	baud;		/* baud rate */

	/* flow control */
	int	xonoff;		/* software flow control on */
	int	blocked;
	int	modem;		/* hardware flow control on */
	int	cts;		/* ... cts state */
	int	ctsbackoff;
	int	rts;		/* ... rts state */
	Rendez	r;

	/* buffers */
	int	(*putc)(Queue*, int);
	Queue	*iq;
	Queue	*oq;

	/* staging areas to avoid some of the per character costs */
	uchar	istage[Stagesize];
	uchar	*ip;
	uchar	*ie;

	uchar	ostage[Stagesize];
	uchar	*op;
	uchar	*oe;
};

static Uart *uart[Nuart];
static int nuart;

static int haveinput;
struct Uartalloc {
	Lock;
	Uart *elist;	/* list of enabled interfaces */
} uartalloc;

void ns16552intr(int);

Block*	qbread(Queue*, int);	/* missing from portdat.h */


/*
 *  pick up architecture specific routines and definitions
 */
#include "ns16552.h"

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
static void
ns16552setbaud(Uart *p, int rate)
{
	ulong brconst;
	int fifo;

	if(rate <= 0)
		return;

	brconst = (p->freq+8*rate-1)/(16*rate);

	uartwrreg(p, Format, Dra);
	outb(p->port + Dmsb, (brconst>>8) & 0xff);
	outb(p->port + Dlsb, brconst & 0xff);
	uartwrreg(p, Format, 0);

	p->baud = rate;

	if(p->nofifo == 0){
		p->fifoon = 1;
		if(rate <= 1200)
			fifo = Ftrig1;
		else
			fifo = Ftrig8;
		uartwrreg(p, Fifoctl, Fena|Fclear|fifo);
		p->nxmit = 8;
	}else
		p->nxmit = 1;
}

static void
ns16552parity(Uart *p, char type)
{
	switch(type){
	case 'e':
		p->sticky[Format] |= Pena|Peven;
		break;
	case 'o':
		p->sticky[Format] &= ~Peven;
		p->sticky[Format] |= Pena;
		break;
	default:
		p->sticky[Format] &= ~(Pena|Peven);
		break;
	}
	uartwrreg(p, Format, 0);
}

/*
 *  set bits/character, default 8
 */
void
ns16552bits(Uart *p, int bits)
{
	if(bits < 5 || bits > 8)
		error(Ebadarg);

	p->sticky[Format] &= ~3;
	p->sticky[Format] |= bits-5;

	uartwrreg(p, Format, 0);
}


/*
 *  toggle DTR
 */
void
ns16552dtr(Uart *p, int n)
{
	if(n)
		p->sticky[Mctl] |= Dtr;
	else
		p->sticky[Mctl] &= ~Dtr;

	uartwrreg(p, Mctl, 0);
}

/*
 *  toggle RTS
 */
void
ns16552rts(Uart *p, int n)
{
	int x;

	x = splhi();
	lock(&p->plock);

	if(n)
		p->sticky[Mctl] |= Rts;
	else
		p->sticky[Mctl] &= ~Rts;

	uartwrreg(p, Mctl, 0);
	p->rts = n;

	unlock(&p->plock);
	splx(x);
}

/*
 *  send break
 */
void
ns16552break(Uart *p, int ms)
{
	if(ms == 0)
		ms = 200;

	uartwrreg(p, Format, Break);
	tsleep(&up->sleep, return0, 0, ms);
	uartwrreg(p, Format, 0);
}

//void
//ns16552fifoon(Uart *p)
//{
//	ulong i, x;

//	if(p->nofifo)
//		return;

//	x = splhi();

//	/* reset fifos */
//	uartwrreg(p, Fifoctl, Fclear);

//	/* empty buffer and interrupt conditions */
//	for(i = 0; i < 16; i++){
//		if(uartrdreg(p, Istat))
//			;
//		if(uartrdreg(p, Data))
//			;
//	}

//	/* turn on fifo */
//	p->fifoon = 1;
//	uartwrreg(p, Fifoctl, Fena|Ftrig);

//	if((p->istat & Fenabd) == 0){
//		/* didn't work, must be an earlier chip type */
//		p->nofifo = 1;
//	}
		
//	splx(x);
//}

/*
 *  modem flow control on/off (rts/cts)
 */
void
ns16552mflow(Uart *p, int n)
{
	if(n){
		p->sticky[Iena] |= Imstat;
		uartwrreg(p, Iena, 0);
		p->modem = 1;
		p->cts = uartrdreg(p, Mstat) & Cts;

		if(p->nofifo)
			if((p->dmachan=ns16552dmarcv(p->dev)) >= 0) {
				p->dmaon = 1;
				/*
				 * Turn off receive interrupts since
				 * the receive buffer full signal is the
				 * DMA request input.
				 */
				p->sticky[Iena] &= ~Ircv;
				uartwrreg(p, Iena, 0);
				dmasetup(p->dmachan, p->ip, Dmabufsize, 1);
			}
	} else {
		p->sticky[Iena] &= ~Imstat;
		uartwrreg(p, Iena, 0);
		p->modem = 0;
		p->cts = 1;

		if(p->dmaon) {
			p->dmaon = 0;
			p->sticky[Iena] |= Ircv;
			uartwrreg(p, Iena, 0);
			//uartwrreg(p, Fifoctl, 0);
			//p->nxmit = 1;
		}
	}
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
void
ns16552enable(Uart *p)
{
	Uart **l;

	if(p->enabled)
		return;

	uartpower(p->dev, 1);

	/*
 	 *  turn on interrupts
	 */
	p->sticky[Iena] = Ircv | Ixmt | Irstat;
	uartwrreg(p, Iena, 0);

	/*
	 *  turn on DTR and RTS
	 */
	ns16552dtr(p, 1);
	ns16552rts(p, 1);

	/*
	 *  assume we can send
	 */
	p->cts = 1;
	p->blocked = 0;

	/*
	 *  set baud rate to the last used
	 */
	ns16552setbaud(p, p->baud);

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p)
			break;
	}
	if(*l == 0){
		p->elist = uartalloc.elist;
		uartalloc.elist = p;
	}
	p->enabled = 1;
	unlock(&uartalloc);
}

/*
 *  turn off a port's interrupts.  reset DTR and RTS
 */
void
ns16552disable(Uart *p)
{
	Uart **l;

	/*
 	 *  turn off interrpts
	 */
	p->sticky[Iena] = 0;
	uartwrreg(p, Iena, 0);

	/*
	 *  revert to default settings
	 */
	p->sticky[Format] = Bits8;
	uartwrreg(p, Format, 0);

	/*
	 *  turn off DTR, RTS, hardware flow control & fifo's
	 */
	ns16552dtr(p, 0);
	ns16552rts(p, 0);
	ns16552mflow(p, 0);
	p->xonoff = p->blocked = 0;

	uartpower(p->dev, 0);

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p){
			*l = p->elist;
			break;
		}
	}
	p->enabled = 0;
	unlock(&uartalloc);
}

/*
 *  put some bytes into the local queue to avoid calling
 *  qconsume for every character
 */
static int
stageoutput(Uart *p)
{
	int n;

	n = qconsume(p->oq, p->ostage, Stagesize);
	if(n <= 0)
		return 0;
	p->op = p->ostage;
	p->oe = p->ostage + n;
	return n;
}

/*
 *  (re)start output
 */
static void
ns16552kick(Uart *p)
{
	int n, nb;

	ilock(&p->plock);
	if(p->printing == 0 && p->cts && p->blocked == 0)
	if(p->op < p->oe || stageoutput(p)){
		n = 0;
		while((uartrdreg(p, Lstat) & Outready) == 0){
			if(++n > 100000){
				print("stuck serial line\n");
				break;
			}
		}
		for(nb = p->nxmit; --nb >= 0;){
			p->printing = 1;
			for(n=0; n<10000 && (uartrdreg(p, Lstat)&Outready) == 0; n++)
				;
			outb(p->port + Data, *(p->op++));
			if(p->op >= p->oe && stageoutput(p) == 0)
				break;
		}
	}
	iunlock(&p->plock);
}

/*
 *  restart input if its off
 */
static void
ns16552flow(Uart *p)
{
	if(p->modem){
		ns16552rts(p, 1);
		haveinput = 1;
	}
}

/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
static void
ns16552setup0(Uart *p)
{
	memset(p->sticky, 0, sizeof(p->sticky));
	/*
	 *  set rate to 9600 baud.
	 *  8 bits/character.
	 *  1 stop bit.
	 *  interrupts enabled.
	 */
	p->sticky[Format] = Bits8;
	uartwrreg(p, Format, 0);
	p->sticky[Mctl] |= Inton;
	uartwrreg(p, Mctl, 0x0);

	p->nofifo = 1;
	uartwrreg(p, Fifoctl, Fena|Fclear|Ftrig14);

	delay(100);

	print("uart%d: ", p->dev);
	if((uartrdreg(p, Istat) & 0xc0) == 0xc0){
		if((uartrdreg(p, Fifoctl) & Ftrig14) == Ftrig14){
			p->nofifo = 0;
			print("ns16550A fifo\n");
		} else
			print("ns82550/16550, broken fifo\n");
	} else
		print("ns82450/16450, no fifo\n");

	uartwrreg(p, Fifoctl, 0);

	ns16552setbaud(p, 9600);

	p->iq = qopen(4*1024, 0, ns16552flow, p);
	p->oq = qopen(4*1024, 0, ns16552kick, p);

	p->ip = p->istage;
	p->ie = &p->istage[Stagesize];
	p->op = p->ostage;
	p->oe = p->ostage;
}

/*
 *  called by main() to create a new duart
 */
void
ns16552setup(ulong port, ulong freq, char *name)
{
	Uart *p;

	if(nuart >= Nuart)
		return;

	p = xalloc(sizeof(Uart));
	uart[nuart] = p;
	strcpy(p->name, name);
	p->dev = nuart;
	nuart++;
	p->port = port;
	p->freq = freq;
	ns16552setup0(p);
}

/*
 *  called by main() to configure a duart port as a console or a mouse
 */
void
ns16552special(int port, int baud, Queue **in, Queue **out, int (*putc)(Queue*, int))
{
	Uart *p = uart[port];

	ns16552enable(p);
	if(baud)
		ns16552setbaud(p, baud);
	p->putc = putc;
	if(in)
		*in = p->iq;
	if(out)
		*out = p->oq;
	p->opens++;
}

/*
 *  reset the interface
 */
void
ns16552shake(Uart *p)
{
	int xonoff, modem;

	xonoff = p->xonoff;
	modem = p->modem;
	ns16552disable(p);
	ns16552enable(p);
	p->xonoff = xonoff;
	ns16552mflow(p, modem);
}

/*
 *  handle an interrupt to a single uart
 */
void
ns16552intr(int dev)
{
	uchar ch;
	int s, l, loops, nb;
	Uart *p = uart[dev];

	for(loops = 0;; loops++){
		p->istat = s = uartrdreg(p, Istat);
		if(loops > 10000000){
			print("lstat %ux mstat %ux istat %ux iena %ux ferr %d oerr %d",
				uartrdreg(p, Lstat), uartrdreg(p, Mstat),
				s, uartrdreg(p, Iena), p->frame, p->overrun);
			panic("ns16552intr");
		}
		switch(s&0x3f){
		case 6:	/* receiver line status */
			l = uartrdreg(p, Lstat);
			if(l & Ferror)
				p->frame++;
			if(l & Oerror)
				p->overrun++;
			break;
	
		case 4:	/* received data available */
		case 12:
			ch = uartrdreg(p, Data) & 0xff;
			if(p->xonoff){
				if(ch == CTLS){
					p->blocked = 1;
				}else if (ch == CTLQ){
					p->blocked = 0;
					 /* clock gets output going again */
				}
			}
			if(p->putc)
				(*p->putc)(p->iq, ch);
			else {
				if(p->ip < p->ie)
					*p->ip++ = ch;
				haveinput = 1;
			}
			break;
	
		case 2:	/* transmitter not full */
			lock(&p->plock);
			l = uartrdreg(p, Lstat) & Outready;
			if(l)
				p->printing = 0;
			if(p->printing == 0 && p->cts && p->blocked == 0)
			if(p->op < p->oe || stageoutput(p)){
				for(nb = p->nxmit; --nb >= 0;){
					p->printing = 1;
					outb(p->port + Data, *(p->op++));
					if(p->op >= p->oe && stageoutput(p) == 0)
						break;
				}
			}
			unlock(&p->plock);
			break;
	
		case 0:	/* modem status */
			ch = uartrdreg(p, Mstat);
			if(ch & Ctsc){
				p->cts = ch & Cts; /* clock gets output going again */
				p->ctsbackoff += 1;
			}
			if(((ch & Dsr) == 0 || (ch&Dcd) == 0) && p->sendhup && p->iq){
				qhangup(p->iq, 0);
				qhangup(p->oq, 0);
			}
			break;
	
		default:
			if(s&1)
				return;
			print("weird modem interrupt #%2.2ux\n", s);
			break;
		}
	}
}

/*
 *  we save up input characters till clock time
 *
 *  There's also a bit of code to get a stalled print going.
 *  It shouldn't happen, but it does.  Obviously I don't
 *  understand something.  Since it was there, I bundled a
 *  restart after flow control with it to give some histeresis
 *  to the hardware flow control.  This makes compressing
 *  modems happier but will probably bother something else.
 *	 -- presotto
 */
void
uartclock(void)
{
	int n;
	Uart *p;
	static uchar bidx;

	for(p = uartalloc.elist; p; p = p->elist){
		ns16552intr(p->dev);
		if(p->dmaon) {
			/*
			 * Stop DMA, read count, change buffers,
			 * and restart. This minimizes window of
			 * opportunity for data loss.
			 * Dmabufsize must be large enough not to
			 * overflow between calls to uartclock().
			 */
			dmaend(p->dmachan);
			/* DMA counts down */
			n = Dmabufsize - dmacount(p->dmachan);
			if(n > 0 && p->iq){
				/* alternate buffers to guarantee no data loss */
				bidx ^= 1;
				dmasetup(p->dmachan,
					&p->istage[bidx*Dmabufsize],
					Dmabufsize, 1);
				if(qproduce(p->iq, p->ip, n) < 0)
					ns16552rts(p, 0);
				else
					p->ip = &p->istage[bidx*Dmabufsize];
			}
			else
				/* restart DMA */
				dmasetup(p->dmachan, p->ip, Dmabufsize, 1);
		}
		else if(haveinput){
			n = p->ip - p->istage;
			if(n > 0 && p->iq){
				if(n > Stagesize)
					panic("uartclock");
				if(qproduce(p->iq, p->istage, n) < 0)
					ns16552rts(p, 0);
				else
					p->ip = p->istage;
			}
		}
		if(p->ctsbackoff-- < 0){
			p->ctsbackoff = 0;
			ns16552kick(p);
		}
	}
	haveinput = 0;
}

Dirtab *ns16552dir;
int ndir;

void
uartflush(void)
{
	Uart *p;

	p = uart[0];
	while(p && p->oq && (p->op < p->oe || qlen(p->oq) > 0))
		ns16552kick(p);
}

static void
setlength(int i)
{
	Uart *p;

	if(i > 0){
		p = uart[i];
		if(p && p->opens && p->iq)
			ns16552dir[3*i].length = qlen(p->iq);
	} else for(i = 0; i < nuart; i++){
		p = uart[i];
		if(p && p->opens && p->iq)
			ns16552dir[3*i].length = qlen(p->iq);
	}
		
}

/*
 *  all uarts must be ns16552setup() by this point or inside of ns16552install()
 */
void
ns16552reset(void)
{
	int i;
	Dirtab *dp;

	ns16552install();	/* architecture specific */

	ndir = 3*nuart;
	ns16552dir = xalloc(ndir * sizeof(Dirtab));
	dp = ns16552dir;
	for(i = 0; i < nuart; i++){
		/* 3 directory entries per port */
		strcpy(dp->name, uart[i]->name);
		dp->qid.path = NETQID(i, Ndataqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sctl", uart[i]->name);
		dp->qid.path = NETQID(i, Nctlqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sstat", uart[i]->name);
		dp->qid.path = NETQID(i, Nstatqid);
		dp->perm = 0444;
		dp++;
	}
}

void
ns16552init(void)
{
}

Chan*
ns16552attach(char *spec)
{
	return devattach('t', spec);
}

Chan*
ns16552clone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
ns16552walk(Chan *c, char *name)
{
	return devwalk(c, name, ns16552dir, ndir, devgen);
}

void
ns16552stat(Chan *c, char *dp)
{
	if(NETTYPE(c->qid.path) == Ndataqid)
		setlength(NETID(c->qid.path));
	devstat(c, dp, ns16552dir, ndir, devgen);
}

Chan*
ns16552open(Chan *c, int omode)
{
	Uart *p;

	c = devopen(c, omode, ns16552dir, ndir, devgen);

	switch(NETTYPE(c->qid.path)){
	case Nctlqid:
	case Ndataqid:
		p = uart[NETID(c->qid.path)];
		qlock(p);
		if(p->opens++ == 0){
			ns16552enable(p);
			qreopen(p->iq);
			qreopen(p->oq);
		}
		qunlock(p);
		break;
	}

	return c;
}

void
ns16552create(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
ns16552close(Chan *c)
{
	Uart *p;

	if(c->qid.path & CHDIR)
		return;
	if((c->flag & COPEN) == 0)
		return;
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
	case Nctlqid:
		p = uart[NETID(c->qid.path)];
		qlock(p);
		if(--(p->opens) == 0){
			ns16552disable(p);
			qclose(p->iq);
			qclose(p->oq);
			p->ip = p->istage;
		}
		qunlock(p);
		break;
	}
}

static long
uartstatus(Chan *c, Uart *p, void *buf, long n, long offset)
{
	uchar mstat;
	uchar tstat;
	char str[256];

	USED(c);

	str[0] = 0;
	tstat = p->sticky[Mctl];
	mstat = uartrdreg(p, Mstat);
	sprint(str, "opens %d ferr %d oerr %d baud %d", p->opens,
		p->frame, p->overrun, p->baud);
	if(mstat & Cts)
		strcat(str, " cts");
	if(mstat & Dsr)
		strcat(str, " dsr");
	if(mstat & Ring)
		strcat(str, " ring");
	if(mstat & Dcd)
		strcat(str, " dcd");
	if(tstat & Dtr)
		strcat(str, " dtr");
	if(tstat & Rts)
		strcat(str, " rts");
{
int i, j;

i = strlen(str);
for(j = 0; j < 8; j++) i+=sprint(str+i, " %ux", uartrdreg(p, j));
}
	strcat(str, "\n");
	return readstr(offset, buf, n, str);
}

long
ns16552read(Chan *c, void *buf, long n, ulong offset)
{
	Uart *p;

	if(c->qid.path & CHDIR){
		setlength(-1);
		return devdirread(c, buf, n, ns16552dir, ndir, devgen);
	}

	p = uart[NETID(c->qid.path)];
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qread(p->iq, buf, n);
	case Nctlqid:
		return readnum(offset, buf, n, NETID(c->qid.path), NUMSIZE);
	case Nstatqid:
		return uartstatus(c, p, buf, n, offset);
	}

	return 0;
}

Block*
ns16552bread(Chan *c, long n, ulong offset)
{
	if(c->qid.path & CHDIR || NETTYPE(c->qid.path) != Ndataqid)
		return devbread(c, n, offset);
	return qbread(uart[NETID(c->qid.path)]->iq, n);
}

static void
ns16552ctl(Uart *p, char *cmd)
{
	int i, n;

	/* let output drain for a while */
	for(i = 0; i < 16 && qlen(p->oq); i++)
		tsleep(&p->r, qlen, p->oq, 125);

	if(strncmp(cmd, "break", 5) == 0){
		ns16552break(p, 0);
		return;
	}
		

	n = atoi(cmd+1);
	switch(*cmd){
	case 'B':
	case 'b':
		ns16552setbaud(p, n);
		break;
	case 'D':
	case 'd':
		ns16552dtr(p, n);
		break;
	case 'f':
	case 'F':
		qflush(p->oq);
		break;
	case 'H':
	case 'h':
		qhangup(p->iq, 0);
		qhangup(p->oq, 0);
		break;
	case 'L':
	case 'l':
		ns16552bits(p, n);
		break;
	case 'm':
	case 'M':
		ns16552mflow(p, n);
		break;
	case 'n':
	case 'N':
		qnoblock(p->oq, n);
		break;
	case 'P':
	case 'p':
		ns16552parity(p, *(cmd+1));
		break;
	case 'K':
	case 'k':
		ns16552break(p, n);
		break;
	case 'R':
	case 'r':
		ns16552rts(p, n);
		break;
	case 'Q':
	case 'q':
		qsetlimit(p->iq, n);
		qsetlimit(p->oq, n);
		break;
	case 'W':
	case 'w':
		/* obsolete */
		break;
	case 'X':
	case 'x':
		p->xonoff = n;
		break;
	case 'Y':
	case 'y':
		p->sendhup = n;
		break;
	}
}

long
ns16552write(Chan *c, void *buf, long n, ulong offset)
{
	Uart *p;
	char cmd[32];

	USED(offset);

	if(c->qid.path & CHDIR)
		error(Eperm);

	p = uart[NETID(c->qid.path)];

	/*
	 *  The fifo's turn themselves off sometimes.
	 *  It must be something I don't understand. -- presotto
	 */
//	if((p->istat & Fenabd) == 0 && p->fifoon && p->nofifo == 0)
//		ns16552fifoon(p);

	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qwrite(p->oq, buf, n);
	case Nctlqid:
		if(n >= sizeof(cmd))
			n = sizeof(cmd)-1;
		memmove(cmd, buf, n);
		cmd[n] = 0;
		ns16552ctl(p, cmd);
		return n;
	}
}

long
ns16552bwrite(Chan *c, Block *bp, ulong offset)
{
	return devbwrite(c, bp, offset);
}

void
ns16552remove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
ns16552wstat(Chan *c, char *dp)
{
	Dir d;
	Dirtab *dt;

	if(!iseve())
		error(Eperm);
	if(CHDIR & c->qid.path)
		error(Eperm);
	if(NETTYPE(c->qid.path) == Nstatqid)
		error(Eperm);

	dt = &ns16552dir[3 * NETID(c->qid.path)];
	convM2D(dp, &d);
	d.mode &= 0666;
	dt[0].perm = dt[1].perm = d.mode;
}
