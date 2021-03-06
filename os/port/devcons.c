#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"devtab.h"

Queue*	kbdq;		/* unprocessed console input */
Queue*	lineq;		/* processed console input */
Queue*	printq;		/* console output */
Pointer	mouse;

static struct
{
	QLock;

	int	raw;		/* true if we shouldn't process input */
	int	ctl;		/* number of opens to the control file */
	int	x;		/* index into line */
	char	line[1024];	/* current input line */

	char	c;
	int	count;
	int	repeat;
	int	ctlpoff;
} kbd;


	char	sysname[NAMELEN] = "inferno";
	char	eve[NAMELEN] = "inferno";
	ulong	randomread(uchar*, ulong);
static	void	randominit(void);
static	void	seednrand(void);

void
printinit(void)
{
	lineq = qopen(2*1024, 0, 0, 0);
	qnoblock(lineq, 1);
}

/*
 *  return true if current user is eve
 */
int
iseve(void)
{
	Osenv *o;

	o = up->env;
	return strcmp(eve, o->user) == 0;
}

static int
consactive(void)
{
	if(printq)
		return qlen(printq) > 0;
	return 0;
}

static void
prflush(void)
{
	while(consactive())
		;
}

/*
 *   Print a string on the console.  Convert \n to \r\n for serial
 *   line consoles.  Locking of the queues is left up to the screen
 *   or uart code.  Multi-line messages to serial consoles may get
 *   interspersed with other messages.
 */
static void
putstrn0(char *str, int n, int usewrite)
{
	int m;
	char *t;
	char buf[PRINTSIZE+2];

	/*
	 *  if there's an attached bit mapped display,
	 *  put the message there.  screenputs is defined
	 *  as a null macro for systems that have no such
	 *  display.
	 */
	screenputs(str, n);

	/*
	 *  if there's a serial line being used as a console,
	 *  put the message there.
	 */
	if(printq == 0)
		return;

	while(n > 0) {
		t = memchr(str, '\n', n);
		if(t) {
			m = t - str;
			memmove(buf, str, m);
			buf[m] = '\r';
			buf[m+1] = '\n';
			if(usewrite)
				qwrite(printq, buf, m+2);
			else
				qiwrite(printq, buf, m+2);
			str = t + 1;
			n -= m + 1;
		} else {
			if(usewrite)
				qwrite(printq, str, n);
			else 
				qiwrite(printq, str, n);
			break;
		}
	}
}

void
putstrn(char *str, int n)
{
	putstrn0(str, n, 0);
}

int
snprint(char *s, int n, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	n = doprint(s, s+n, fmt, arg) - s;
	va_end(arg);

	return n;
}

int
sprint(char *s, char *fmt, ...)
{
	int n;
	va_list arg;

	va_start(arg, fmt);
	n = doprint(s, s+PRINTSIZE, fmt, arg) - s;
	va_end(arg);

	return n;
}

int
print(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	putstrn(buf, n);

	return n;
}

int
fprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	USED(fd);
	va_start(arg, fmt);
	n = doprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	putstrn(buf, n);

	return n;
}

void
panic(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	strcpy(buf, "panic: ");
	va_start(arg, fmt);
	n = doprint(buf+strlen(buf), buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	buf[n] = '\n';
	putstrn(buf, n+1);
	spllo();
	prflush();
	exit(1);
}

int
pprint(char *fmt, ...)
{
	int n;
	Chan *c;
	Osenv *o;
	va_list arg;
	char buf[2*PRINTSIZE];

	n = sprint(buf, "%s %d: ", up->text, up->pid);
	va_start(arg, fmt);
	n = doprint(buf+n, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	o = up->env;
	if(o->fgrp == 0) {
		print("%s", buf);
		return 0;
	}
	c = o->fgrp->fd[2];
	if(c==0 || (c->mode!=OWRITE && c->mode!=ORDWR)) {
		print("%s", buf);
		return 0;
	}

	if(waserror()) {
		print("%s", buf);
		return 0;
	}
	(*devtab[c->type].write)(c, buf, n, c->offset);
	poperror();

	lock(c);
	c->offset += n;
	unlock(c);

	return n;
}

void
echo(Rune r, char *buf, int n)
{
	Proc *p;
	Osenv *o;
	int i;
	char ptb[128];
	static int ctrlt, pid;
	extern ulong etext;

	/*
	 * ^p hack
	 */
	if(r==0x10 && !kbd.ctlpoff)
		exit(0);

	/*
	 * ^t hack BUG
	 */
	if(ctrlt == 2){
		ctrlt = 0;
		switch(r){
		case 0x14:
			break;	/* pass it on */
		case 's':
			dumpstack();
			break;
		case 'x':
			xsummary();
			ixsummary();
			break;
		case 'd':
			consdebug();
			return;
		case 'p':
			procdump();
			return;
		case 'r':
			exit(0);
			break;
		case 'f':
			p = proctab(6);
			o = p->env;
			for(i = 0; i < o->fgrp->maxfd; i++) {
				if(o->fgrp->fd[i] == 0)
					continue; 
				ptpath(o->fgrp->fd[i]->path, ptb, sizeof(ptb));
				print("%d: %s\n", i, ptb);
			}
			return;
		}
	}
	else if(r == 0x14){
		ctrlt++;
		return;
	}
	ctrlt = 0;
	if(kbd.raw)
		return;

	/*
	 *  finally, the actual echoing
	 */
	if(r == '\n'){
		if(printq)
			qiwrite(printq, "\r", 1);
	} else if(r == 0x15){
		buf = "^U\n";
		n = 3;
	}
	screenputs(buf, n);
	if(printq)
		qiwrite(printq, buf, n);
}

/*
 *  Called by a uart interrupt for console input.
 *
 *  turn '\r' into '\n' before putting it into the queue.
 */
int
kbdcr2nl(Queue *q, int ch)
{
	if(ch == '\r')
		ch = '\n';
	return kbdputc(q, ch);
}

/*
 *  Put character, possibly a rune, into read queue at interrupt time.
 *  Called at interrupt time to process a character.
 */
int
kbdputc(Queue *q, int ch)
{
	int n;
	char buf[3];
	Rune r;

	USED(q);
	r = ch;
	n = runetochar(buf, &r);
	if(n == 0)
		return 0;
	echo(r, buf, n);
	qproduce(kbdq, buf, n);
	return 0;
}

void
kbdrepeat(int rep)
{
	kbd.repeat = rep;
	kbd.count = 0;
}

void
kbdclock(void)
{
	if(kbd.repeat == 0)
		return;
	if(kbd.repeat==1 && ++kbd.count>HZ){
		kbd.repeat = 2;
		kbd.count = 0;
		return;
	}
	if(++kbd.count&1)
		kbdputc(kbdq, kbd.c);
}

enum{
	Qdir,
	Qcons,
	Qconsctl,
	Qkeyboard,
	Qmemory,
	Qmsec,
	Qnull,
	Qpgrpid,
	Qpin,
	Qpointer,
	Qrandom,
	Qsysname,
	Qtime,
	Quser,
};

Dirtab consdir[]=
{
	"cons",		{Qcons},	0,		0660,
	"consctl",	{Qconsctl},	0,		0220,
	"keyboard",	{Qkeyboard},	0,		0666,
	"memory",	{Qmemory},	0,		0444,
	"msec",		{Qmsec},	NUMSIZE,	0444,
	"null",		{Qnull},	0,		0666,
	"pin",		{Qpin},		0,		0666,
	"pointer",	{Qpointer},	0,		0444,
	"pgrpid",	{Qpgrpid},	NUMSIZE,	0444,
	"random",	{Qrandom},	0,		0444,
	"sysname",	{Qsysname},	0,		0664,
	"time",		{Qtime},	0,		0664,
 	"user",		{Quser},	NAMELEN,	0666,
};

#define	NCONS	(sizeof consdir/sizeof(Dirtab))

ulong	boottime;		/* seconds since epoch at boot */

long
seconds(void)
{
	return boottime + TK2SEC(MACHP(0)->ticks);
}

int
readnum(ulong off, char *buf, ulong n, ulong val, int size)
{
	char tmp[64];

	snprint(tmp, sizeof(tmp), "%*.0ud", size-1, val);
	tmp[size-1] = ' ';
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, tmp+off, n);
	return n;
}

int
readstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

void
consreset(void)
{
}

void
consinit(void)
{
	randominit();
}

Chan*
consattach(char *spec)
{
	return devattach('c', spec);
}

Chan*
consclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
conswalk(Chan *c, char *name)
{
	return devwalk(c, name, consdir, NCONS, devgen);
}

void
consstat(Chan *c, char *dp)
{
	devstat(c, dp, consdir, NCONS, devgen);
}

Chan*
consopen(Chan *c, int omode)
{
	c->aux = 0;
	switch(c->qid.path){
	case Qconsctl:
		if(!iseve())
			error(Eperm);
		qlock(&kbd);
		kbd.ctl++;
		qunlock(&kbd);
		break;
	case Qkeyboard:
		qlock(&kbd);
		kbd.ctl++;
		kbd.raw = 1;
		kbd.x = 0;
		qunlock(&kbd);
		break;
	case Qpointer:
		if(incref(&mouse.ref) == 1)
			cursoron();
		break;
	}
	return devopen(c, omode, consdir, NCONS, devgen);
}

void
conscreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
consclose(Chan *c)
{
	if((c->flag&COPEN) == 0)
		return;

	switch(c->qid.path){
	case Qconsctl:
	case Qkeyboard:
		/* last close of control file turns off raw */
		qlock(&kbd);
		if(--kbd.ctl == 0)
			kbd.raw = 0;
		qunlock(&kbd);
		break;
	case Qpointer:
		if(decref(&mouse.ref) == 0)
			cursoroff();
		break;
	}
}

static int
changed(void *a)
{
	Pointer *p = a;
	return p->modify == 1;
}

long
consread(Chan *c, void *buf, long n, ulong offset)
{
	int l;
	Osenv *o;
	Pointer mt;
	int ch, eol;
	char *p, tmp[128];

	if(n <= 0)
		return n;
	o = up->env;
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, consdir, NCONS, devgen);
	case Qcons:
	case Qkeyboard:
		qlock(&kbd);
		if(waserror()) {
			qunlock(&kbd);
			nexterror();
		}
		while(!qcanread(lineq)) {
			qread(kbdq, &kbd.line[kbd.x], 1);
			ch = kbd.line[kbd.x];
			if(kbd.raw) {
				qiwrite(lineq, &kbd.line[kbd.x], 1);
				continue;
			}
			eol = 0;
			switch(ch){
			case '\b':
				if(kbd.x)
					kbd.x--;
				break;
			case 0x15:
				kbd.x = 0;
				break;
			case '\n':
			case 0x04:
				eol = 1;
			default:
				kbd.line[kbd.x++] = ch;
				break;
			}
			if(kbd.x == sizeof(kbd.line) || eol) {
				if(ch == 0x04)
					kbd.x--;
				qwrite(lineq, kbd.line, kbd.x);
				kbd.x = 0;
			}
		}
		n = qread(lineq, buf, n);
		qunlock(&kbd);
		poperror();
		return n;
	case Qpin:
		p = "pin set";
		if(up->env->pgrp->pin == Nopin)
			p = "no pin";
		return readstr(offset, buf, n, p);
	case Qpointer:
		qlock(&mouse.q);
		if(waserror()) {
			qunlock(&mouse.q);
			nexterror();
		}
		sleep(&mouse.r, changed, &mouse);
		mt = mouse;
		mouse.modify = 0;
		l = sprint(tmp, "m%11d %11d %11d ", mt.x, mt.y, mt.b);
		if(n < l)
			l = n;
		memmove(buf, tmp, l);
		qunlock(&mouse.q);
		poperror();
		return l;
	case Qpgrpid:
		return readnum(offset, buf, n, o->pgrp->pgrpid, NUMSIZE);
	case Qtime:
		snprint(tmp, sizeof(tmp), "%.lld", (vlong)seconds() * 1000000);
		return readstr(offset, buf, n, tmp);
	case Quser:
		return readstr(offset, buf, n, o->user);
	case Qnull:
		return 0;
	case Qmsec:
		return readnum(offset, buf, n, TK2MS(MACHP(0)->ticks), NUMSIZE);
	case Qsysname:
		return readstr(offset, buf, n, sysname);
	case Qrandom:
		return randomread(buf, n);
	case Qmemory:
		return poolread(buf, n, offset);
	default:
		print("consread %lux\n", c->qid);
		error(Egreg);
	}
	return -1;		/* never reached */
}

Block*
consbread(Chan *c, long n, ulong offset)
{
	return devbread(c, n, offset);
}

long
conswrite(Chan *c, void *va, long n, ulong offset)
{
	vlong t;
	Osenv *o;
	long l, bp;
	char *a = va;
	char buf[256];

	switch(c->qid.path){
	case Qcons:
		/*
		 * Can't page fault in putstrn, so copy the data locally.
		 */
		l = n;
		while(l > 0){
			bp = l;
			if(bp > sizeof buf)
				bp = sizeof buf;
			memmove(buf, a, bp);
			putstrn0(a, bp, 1);
			a += bp;
			l -= bp;
		}
		break;
	case Qpin:
		if(up->env->pgrp->pin != Nopin)
			error("pin already set");
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, va, n);
		buf[n] = '\0';
		up->env->pgrp->pin = atoi(buf);
		return n;
	case Qconsctl:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		for(a = buf; a;){
			if(strncmp(a, "rawon", 5) == 0){
				qlock(&kbd);
				if(kbd.x){
					qwrite(kbdq, kbd.line, kbd.x);
					kbd.x = 0;
				}
				kbd.raw = 1;
				qunlock(&kbd);
			} else if(strncmp(a, "rawoff", 6) == 0){
				kbd.raw = 0;
				kbd.x = 0;
			} else if(strncmp(a, "ctlpon", 6) == 0){
				kbd.ctlpoff = 0;
			} else if(strncmp(a, "ctlpoff", 7) == 0){
				kbd.ctlpoff = 1;
			}
			if(a = strchr(a, ' '))
				a++;
		}
		break;

	case Qtime:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, a, n);
		buf[n] = 0;
		t = strtoll(buf, 0, 0)/1000000;
		boottime = t - TK2SEC(MACHP(0)->ticks);
		break;

	case Quser:
		if(!iseve())
			error(Eperm);
		if(offset != 0)
			error(Ebadarg);
		if(n <= 0 || n >= NAMELEN)
			error(Ebadarg);
		o = up->env;
		strncpy(o->user, a, n);
		o->user[n] = 0;
		if(o->user[n-1] == '\n')
			o->user[n-1] = 0;
		break;

	case Qnull:
		break;

	case Qsysname:
		if(offset != 0)
			error(Ebadarg);
		if(n <= 0 || n >= NAMELEN)
			error(Ebadarg);
		strncpy(sysname, a, n);
		sysname[n] = 0;
		if(sysname[n-1] == '\n')
			sysname[n-1] = 0;
		break;

	default:
		print("conswrite: %d\n", c->qid.path);
		error(Egreg);
	}
	return n;
}

long
consbwrite(Chan *c, Block *bp, ulong offset)
{
	return devbwrite(c, bp, offset);
}

void
consremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
conswstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

static struct
{
	QLock;
	Rendez	producer;
	Rendez	consumer;
	ulong	randomcount;
	uchar	buf[1024];
	uchar	*ep;
	uchar	*rp;
	uchar	*wp;
	uchar	next;
	uchar	bits;
	uchar	wakeme;
	uchar	filled;
	int	target;
	int	kprocstarted;
	ulong	randn;
} rb;

static void
seednrand(void)
{
	randomread((void*)&rb.randn, sizeof(rb.randn));
}

int
nrand(int n)
{
	rb.randn = rb.randn*1103515245 + 12345 + MACHP(0)->ticks;
	return (rb.randn>>16) % n;
}

static int
rbnotfull(void*)
{
	int i;

	i = rb.wp - rb.rp;
	if(i < 0)
		i += sizeof(rb.buf);
	return i < rb.target;
}

static int
rbnotempty(void*)
{
	return rb.wp != rb.rp;
}

void
genrandom(void*)
{
	setpri(PriBackground);

	for(;;) {
		if(!rbnotfull(0))
			sleep(&rb.producer, rbnotfull, 0);
		rb.randomcount++;
	}
}

/*
 *  produce random bits in a circular buffer
 */
void
randomclock(void)
{
	uchar *p;

	if(rb.randomcount == 0)
		return;

	if(!rbnotfull(0)) {
		rb.filled = 1;
		return;
	}

	rb.bits = (rb.bits<<2) ^ (rb.randomcount&3);
	rb.randomcount = 0;

	rb.next += 2;
	if(rb.next != 8)
		return;

	rb.next = 0;
	*rb.wp ^= rb.bits ^ *rb.rp;
	p = rb.wp+1;
	if(p == rb.ep)
		p = rb.buf;
	rb.wp = p;

	if(rb.wakeme)
		wakeup(&rb.consumer);
}

static void
randominit(void)
{
	rb.target = 16;
	rb.ep = rb.buf + sizeof(rb.buf);
	rb.rp = rb.wp = rb.buf;
}

/*
 *  consume random bytes from a circular buffer
 */
ulong
randomread(void *xp, ulong n)
{
	int i, sofar;
	uchar *e, *p;

	p = xp;

	if(waserror()){
		qunlock(&rb);
		nexterror();
	}

	qlock(&rb);
	if(!rb.kprocstarted){
		rb.kprocstarted = 1;
		kproc("genrand", genrandom, 0);
	}

	for(sofar = 0; sofar < n; sofar += i){
		i = rb.wp - rb.rp;
		if(i == 0){
			rb.wakeme = 1;
			wakeup(&rb.producer);
			sleep(&rb.consumer, rbnotempty, 0);
			rb.wakeme = 0;
			continue;
		}
		if(i < 0)
			i = rb.ep - rb.rp;
		if((i+sofar) > n)
			i = n - sofar;
		memmove(p + sofar, rb.rp, i);
		e = rb.rp + i;
		if(e == rb.ep)
			e = rb.buf;
		rb.rp = e;
	}
	if(rb.filled && rb.wp == rb.rp){
		i = 2*rb.target;
		if(i > sizeof(rb.buf) - 1)
			i = sizeof(rb.buf) - 1;
		rb.target = i;
		rb.filled = 0;
	}
	qunlock(&rb);
	poperror();

	wakeup(&rb.producer);

	return n;
}
