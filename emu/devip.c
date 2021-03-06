#include	"lib9.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"ip.h"

enum
{
	Qtopdir		= 1,	/* top level directory */
	Qprotodir,		/* directory for a protocol */
	Qclonus,
	Qconvdir,		/* directory for a conversation */
	Qdata,
	Qctl,
	Qstatus,
	Qremote,
	Qlocal,
	Qlisten,

	MAXPROTO	= 4,
	MAXCONV		= 4096
};
#define TYPE(x) 	((x).path & 0xf)
#define CONV(x) 	(((x).path >> 4)&(MAXCONV-1))
#define PROTO(x) 	(((x).path >> 16)&0xff)
#define QID(p, c, y) 	(((p)<<16) | ((c)<<4) | (y))

typedef struct Proto	Proto;
typedef struct Conv	Conv;
struct Conv
{
	int	x;
	Ref	r;
	int	sfd;
	int	perm;
	char	owner[NAMELEN];
	char*	state;
	ulong	laddr;
	ushort	lport;
	ulong	raddr;
	ushort	rport;
	int	header;
	int	restricted;
	char	cerr[NAMELEN];
	Proto*	p;
};

struct Proto
{
	Lock	l;
	int	x;
	int	stype;
	char	name[NAMELEN];
	int	nc;
	int	maxconv;
	Conv**	conv;
	Qid	qid;
};

static	int	np;
static	Proto	proto[MAXPROTO];
static	int	eipconv(va_list*, Fconv*);
static	Conv*	protoclone(Proto*, char*, int);
static	Conv*	newconv(Proto*, Conv **);
static	void	setladdr(Conv*);


int
ipgen(Chan *c, Dirtab *d, int nd, int s, Dir *dp)
{
	Qid q;
	Conv *cv;
	char name[16], *p;

	USED(nd);
	USED(d);
	q.vers = 0;
	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s >= np)
			return -1;

		q.path = QID(s, 0, Qprotodir)|CHDIR;
		devdir(c, q, proto[s].name, 0, "network", CHDIR|0555, dp);
		return 1;
	case Qprotodir:
		if(s < proto[PROTO(c->qid)].nc) {
			cv = proto[PROTO(c->qid)].conv[s];
			sprint(name, "%d", s);
			q.path = QID(PROTO(c->qid), s, Qconvdir)|CHDIR;
			devdir(c, q, name, 0, cv->owner, CHDIR|0555, dp);
			return 1;
		}
		s -= proto[PROTO(c->qid)].nc;
		switch(s) {
		default:
			return -1;
		case 0:
			p = "clone";
			q.path = QID(PROTO(c->qid), 0, Qclonus);
			break;
		}
		devdir(c, q, p, 0, "network", 0555, dp);
		return 1;
	case Qconvdir:
		cv = proto[PROTO(c->qid)].conv[CONV(c->qid)];
		switch(s) {
		default:
			return -1;
		case 0:
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qdata);
			devdir(c, q, "data", 0, cv->owner, cv->perm, dp);
			return 1;
		case 1:
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qctl);
			devdir(c, q, "ctl", 0, cv->owner, cv->perm, dp);
			return 1;
		case 2:
			p = "status";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qstatus);
			break;
		case 3:
			p = "remote";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qremote);
			break;
		case 4:
			p = "local";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qlocal);
			break;
		case 5:
			p = "listen";
			q.path = QID(PROTO(c->qid), CONV(c->qid), Qlisten);
			break;
		}
		devdir(c, q, p, 0, cv->owner, 0444, dp);
		return 1;
	}
	return -1;
}

static void
newproto(char *name, int type, int maxconv)
{
	int l;
	Proto *p;

	if(np >= MAXPROTO) {
		print("no %s: increase MAXPROTO", name);
		return;
	}

	p = &proto[np];
	strcpy(p->name, name);
	p->stype = type;
	p->qid.path = CHDIR|QID(np, 0, Qprotodir);
	p->x = np++;
	p->maxconv = maxconv;
	l = sizeof(Conv*)*p->maxconv;
	p->conv = malloc(l);
	if(p->conv == 0)
		panic("no memory");
	memset(p->conv, 0, l);
}

void
ipinit(void)
{
	newproto("udp", S_UDP, 10);
	newproto("tcp", S_TCP, 20);

	fmtinstall('i', eipconv);
	fmtinstall('I', eipconv);
	fmtinstall('E', eipconv);
}

Chan *
ipattach(void *spec)
{
	Chan *c;

	c = devattach('I', spec);
	c->qid.path = QID(0, 0, Qtopdir)|CHDIR;
	c->qid.vers = 0;
	return c;
}

Chan *
ipclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
ipwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, ipgen);
}

void
ipstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, ipgen);
}

Chan *
ipopen(Chan *c, int omode)
{
	Proto *p;
	ulong raddr;
	ushort rport;
	int perm, sfd;
	Conv *cv, *lcv;

	perm = 0;
	omode &= 3;
	switch(omode) {
	case OREAD:
		perm = 4;
		break;
	case OWRITE:
		perm = 2;
		break;
	case ORDWR:
		perm = 6;
		break;
	}

	switch(TYPE(c->qid)) {
	default:
		break;
	case Qtopdir:
	case Qprotodir:
	case Qconvdir:
	case Qstatus:
	case Qremote:
	case Qlocal:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qclonus:
		p = &proto[PROTO(c->qid)];
		cv = protoclone(p, up->env->user, -1);
		if(cv == 0)
			error(Enodev);
		c->qid.path = QID(p->x, cv->x, Qctl);
		c->qid.vers = 0;
		break;
	case Qdata:
	case Qctl:
		p = &proto[PROTO(c->qid)];
		lock(&p->l);
		cv = p->conv[CONV(c->qid)];
		lock(&cv->r.l);
		if((perm & (cv->perm>>6)) != perm) {
			if(strcmp(up->env->user, cv->owner) != 0 ||
		 	  (perm & cv->perm) != perm) {
				unlock(&cv->r.l);
				unlock(&p->l);
				error(Eperm);
			}
		}
		cv->r.ref++;
		if(cv->r.ref == 1) {
			memmove(cv->owner, up->env->user, NAMELEN);
			cv->perm = 0660;
		}
		unlock(&cv->r.l);
		unlock(&p->l);
		break;
	case Qlisten:
		p = &proto[PROTO(c->qid)];
		lcv = p->conv[CONV(c->qid)];
		sfd = so_accept(lcv->sfd, &raddr, &rport);
		cv = protoclone(p, up->env->user, sfd);
		if(cv == 0) {
			so_close(sfd);
			error(Enodev);
		}
		cv->raddr = raddr;
		cv->rport = rport;
		setladdr(cv);
		cv->state = "Established";
		c->qid.path = QID(p->x, cv->x, Qctl);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
ipcreate(Chan *c, char *name, int mode, ulong perm)
{
	USED(c);
	USED(name);
	USED(mode);
	USED(perm);
	error(Eperm);
}

void
ipremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
ipwstat(Chan *c, char *buf)
{
	USED(c);
	USED(buf);
	error(Eperm);
}

void
ipclose(Chan *c)
{
	Conv *cc;

	switch(TYPE(c->qid)) {
	case Qdata:
	case Qctl:
		if((c->flag & COPEN) == 0)
			break;
		cc = proto[PROTO(c->qid)].conv[CONV(c->qid)];
		if(decref(&cc->r) != 0)
			break;
		strcpy(cc->owner, "network");
		cc->perm = 0666;
		cc->state = "Closed";
		cc->laddr = 0;
		cc->raddr = 0;
		cc->lport = 0;
		cc->rport = 0;
		if(cc->sfd != -1)
			so_close(cc->sfd);
		break;
	}
}

long
ipread(Chan *ch, void *a, long n, ulong offset)
{
	int r;
	Conv *c;
	Proto *x;
	uchar ip[4];
	char buf[128], *p, ebuf[ERRLEN];

	p = a;
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qprotodir:
	case Qtopdir:
	case Qconvdir:
		return devdirread(ch, a, n, 0, 0, ipgen);
	case Qctl:
		sprint(buf, "%d", CONV(ch->qid));
		return readstr(offset, p, n, buf);
	case Qremote:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		hnputl(ip, c->raddr);
		sprint(buf, "%I!%d\n", ip, c->rport);
		return readstr(offset, p, n, buf);
	case Qlocal:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		hnputl(ip, c->laddr);
		sprint(buf, "%I!%d\n", ip, c->lport);
		return readstr(offset, p, n, buf);
	case Qstatus:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		sprint(buf, "%s/%d %d %s \n",
			c->p->name, c->x, c->r.ref, c->state);
		return readstr(offset, p, n, buf);
	case Qdata:
		c = proto[PROTO(ch->qid)].conv[CONV(ch->qid)];
		if(c->sfd < 0)
			error(Ehungup);
		if(c->header) {
			p = a;
			p += SINADDRSZ;
			n -= SINADDRSZ;
			r = so_recv(c->sfd, p, n, a, SINADDRSZ);
			if(r > 0)
				r += SINADDRSZ;
		}
		else
			r = so_recv(c->sfd, a, n, nil, 0);
		if(r < 0) {
			oserrstr(ebuf);
			error(ebuf);
		}
		return r;
	}
}

static void
setladdr(Conv *c)
{
	so_getsockname(c->sfd, &c->laddr, &c->lport);
}

static void
setlport(Conv *c)
{
	if(c->restricted == 0 && c->lport == 0)
		return;

	so_bind(c->sfd, c->restricted, c->lport);
}

static void
setladdrport(Conv *c, char *str)
{
	char *p, addr[4];

	p = strchr(str, '!');
	if(p == 0) {
		p = str;
		c->laddr = 0;
	}
	else {
		*p++ = 0;
		parseip(addr, str);
		c->laddr = nhgetl((uchar*)addr);
	}
	if(*p == '*')
		c->lport = 0;
	else
		c->lport = atoi(p);

	setlport(c);
}

static char*
setraddrport(Conv *c, char *str)
{
	char *p, addr[4];

	p = strchr(str, '!');
	if(p == 0)
		return "malformed address";
	*p++ = 0;
	parseip(addr, str);
	c->raddr = nhgetl((uchar*)addr);
	c->rport = atoi(p);
	p = strchr(p, '!');
	if(p) {
		if(strcmp(p, "!r") == 0)
			c->restricted = 1;
	}
	return 0;
}

long
ipwrite(Chan *ch, void *a, long n, ulong offset)
{
	Conv *c;
	Proto *x;
	int r, nf;
	char *p, *fields[3], buf[128], ebuf[ERRLEN];

	USED(offset);
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qctl:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = '\0';

		nf = parsefields(buf, fields, 3, " ");
		if(strcmp(fields[0], "connect") == 0){
			switch(nf) {
			default:
				error("bad args to connect");
			case 2:
				p = setraddrport(c, fields[1]);
				if(p != 0)
					error(p);
				break;
			case 3:
				p = setraddrport(c, fields[1]);
				if(p != 0)
					error(p);
				c->lport = atoi(fields[2]);
				setlport(c);
				break;
			}
			so_connect(c->sfd, c->raddr, c->rport);
			setladdr(c);
			c->state = "Established";
			return n;
		}
		if(strcmp(fields[0], "announce") == 0) {
			switch(nf){
			default:
				error("bad args to announce");
			case 2:
				setladdrport(c, fields[1]);
				break;
			}
			if(c->p->stype != S_UDP)
				so_listen(c->sfd);
			c->state = "Announced";
			return n;
		}
		if(strcmp(fields[0], "bind") == 0){
			switch(nf){
			default:
				error("bad args to bind");
			case 2:
				c->lport = atoi(fields[1]);
				break;
			}
			setlport(c);
			return n;
		}
		if(strcmp(fields[0], "headers") == 0){
			if(c->p->stype != S_UDP)
				error(Enoproto);
			c->header = 1;
			return n;
		}
		if(strcmp(fields[0], "hangup") == 0){
			if(c->p->stype == S_UDP)
				so_close(c->sfd);
			else if(so_hangup(c->sfd) < 0) {
				oserrstr(ebuf);
				error(ebuf);
			}
			c->sfd = -1;
			c->state = "Hungup";
			return n;
		}
		error("bad control message");
	case Qdata:
		x = &proto[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(c->sfd < 0)
			error(Ehungup);
		if(c->header) {
			if(n < SINADDRSZ)
				error(Eshort);
			p = a;
			p += SINADDRSZ;
			n -= SINADDRSZ;
			r = so_send(c->sfd, p, n, a, SINADDRSZ);
			if(r >= 0)
				r += SINADDRSZ;
		}
		else
			r = so_send(c->sfd, a, n, nil, 0);

		if(r < 0) {
			oserrstr(ebuf);
			error(ebuf);
		}
		return r;
	}
	return n;
}

static Conv*
protoclone(Proto *p, char *user, int nfd)
{
	Conv *c, **pp, **ep, **np;
	int maxconv;

	c = 0;
	lock(&p->l);
	if(waserror()) {
		unlock(&p->l);
		nexterror();
	}
	ep = &p->conv[p->maxconv];
	for(pp = p->conv; pp < ep; pp++) {
		c = *pp;
		if(c == 0) {
			c = newconv(p, pp);
			break;
		}
		lock(&c->r.l);
		if(c->r.ref == 0) {
			c->r.ref++;
			break;
		}
		unlock(&c->r.l);
	}
	if(pp >= ep) {
		if(p->maxconv >= MAXCONV) {
			unlock(&p->l);
			poperror();
			return 0;
		}
		maxconv = 2 * p->maxconv;
		if(maxconv > MAXCONV)
			maxconv = MAXCONV;
		np = realloc(p->conv, sizeof(Conv*) * maxconv);
		if(np == 0)
			error(Enomem);
		p->conv = np;
		pp = &p->conv[p->maxconv];
		memset(pp, 0, sizeof(Conv*)*(maxconv - p->maxconv));
		p->maxconv = maxconv;
		c = newconv(p, pp);
	}

	strcpy(c->owner, user);
	c->perm = 0660;
	c->state = "Closed";
	c->restricted = 0;
	c->laddr = 0;
	c->raddr = 0;
	c->lport = 0;
	c->rport = 0;
	c->sfd = nfd;
	if(nfd == -1)
		c->sfd = so_socket(p->stype);

	unlock(&c->r.l);
	unlock(&p->l);
	poperror();
	return c;
}

static Conv*
newconv(Proto *p, Conv **pp)
{
	Conv *c;

	*pp = c = malloc(sizeof(Conv));
	if(c == 0)
		error(Enomem);
	lock(&c->r.l);
	c->r.ref = 1;
	c->p = p;
	c->x = pp - p->conv;
	p->nc++;
	return c;
}

static int
eipconv(va_list *arg, Fconv *f)
{
	char buf[64];
	static char *efmt = "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux";
	static char *ifmt = "%d.%d.%d.%d";
	uchar *p, ip[4];

	switch(f->chr) {
	case 'E':		/* Ethernet address */
		p = va_arg(*arg, uchar*);
		sprint(buf, efmt, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	case 'I':		/* Ip address */
		p = va_arg(*arg, uchar*);
		sprint(buf, ifmt, p[0], p[1], p[2], p[3]);
		break;
	case 'i':
		hnputl(ip, va_arg(*arg, ulong));
		sprint(buf, ifmt, ip[0], ip[1], ip[2], ip[3]);
		break;
	default:
		strcpy(buf, "(eipconv)");
	}
	strconv(buf, f);
	return sizeof(uchar*);
}
