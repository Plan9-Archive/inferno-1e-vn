#include "asm.h"
#include "y.tab.h"

List*
newi(int v, List *l)
{
	List *n, *t;

	n = malloc(sizeof(List));
	if(l == nil)
		l = n;
	else {
		for(t = l; t->link; t = t->link)
			;
		t->link = n;
	}
	n->link = nil;
	n->u.ival = v;
	n->addr = -1;

	return l;
}

List*
news(String *s, List *l)
{
	List *n;

	n = malloc(sizeof(List));
	n->link = l;
	l = n;
	n->u.str = s;
	n->addr = -1;
	return l;
}

int
digit(char x)
{
	if(x >= 'A' && x <= 'F')
		return x - 'A' + 10;
	if(x >= 'a' && x <= 'f')
		return x - 'a' + 10;
	if(x >= '0' && x <= '9')
		return x - '0';
	diag("bad hex value in pointers");
	return 0;
}

void
heap(int id, int size, String *ptr)
{
	Desc *d, *f;
	char *p;
	int k, i;
	
	d = malloc(sizeof(Desc));
	d->id = id;
	d->size = size;
	size /= IBY2WD;
	d->map = malloc(size);
	d->np = 0;
	if(dlist == nil)
		dlist = d;
	else {
		for(f = dlist; f->link != nil; f = f->link)
			;
		f->link = d;
	}
	d->link = nil;
	dcount++;

	if(ptr == 0)
		return;
	if(--ptr->len & 1) {
		diag("pointer descriptor has bad length");
		return;	
	}

	k = 0;
	p = ptr->string;
	for(i = 0; i < ptr->len; i += 2) {
		d->map[k++] = (digit(p[0])<<4)|digit(p[1]);
		if(k > size) {
			diag("pointer descriptor too long");
			break;
		}
		p += 2;	
	}
	d->np = k;
}

void
conout(int val)
{
	if(val >= -64 && val <= 63) {
		Bputc(bout, val & ~0x80);
		return;
	}
	if(val >= -8192 && val <= 8191) {
		Bputc(bout, ((val>>8) & ~0xC0) | 0x80);
		Bputc(bout, val);
		return;
	}
	if((val & ~0xC0000000) != val)
		diag("overflow in constant 0x%lux\n", val);
	Bputc(bout, (val>>24) | 0xC0);
	Bputc(bout, val>>16);
	Bputc(bout, val>>8);
	Bputc(bout, val);
}

void
aout(Addr *a)
{
	if(a == nil)
		return;
	if(a->mode & AIND)
		conout(a->off);
	conout(a->val);
}

void
lout(void)
{
	char *p;
	Link *l;

	if(module == nil)
		module = enter("main", 0);

	for(p = module->name; *p; p++)
		Bputc(bout, *p);
	Bputc(bout, '\0');

	for(l = links; l; l = l->link) {
		conout(l->addr);
		conout(l->desc);
		Bputc(bout, l->type>>24);
		Bputc(bout, l->type>>16);
		Bputc(bout, l->type>>8);
		Bputc(bout, l->type);
		for(p = l->name; *p; p++)
			Bputc(bout, *p);
		Bputc(bout, '\0');
	}
}

void
assem(Inst *i)
{
	Desc *d;
	Inst *f, *link;
	int pc, n;

	f = 0;
	while(i) {
		link = i->link;
		i->link = f;
		f = i;
		i = link;
	}
	i = f;

	pc = 0;
	for(f = i; f; f = f->link) {
		f->pc = pc++;
		if(f->sym != nil)
			f->sym->value = f->pc;
	}

	if(pcentry >= pc)
		diag("entry pc out of range");
	if(dentry >= dcount)
		diag("entry descriptor out of range");

	conout(XMAGIC);
	conout(0);		/* Runtime flags */
	conout(1024);		/* default stack size */
	conout(pc);
	conout(dseg);
	conout(dcount);
	conout(nlink);
	conout(pcentry);
	conout(dentry);

	for(f = i; f; f = f->link) {
		if(f->dst && f->dst->sym) {
			f->dst->mode = AIMM;
			f->dst->val = f->dst->sym->value;
		}
		Bputc(bout, opcode(f));
		n = 0;
		if(f->src)
			n |= SRC(f->src->mode);
		else
			n |= SRC(AXXX);
		if(f->dst)
			n |= DST(f->dst->mode);
		else
			n |= DST(AXXX);
		if(f->reg)
			n |= f->reg->mode;
		else
			n |= AXNON;
		Bputc(bout, n);
		aout(f->reg);
		aout(f->src);
		aout(f->dst);

		if(listing)
			print("%4d %i\n", f->pc, f);
	}

	for(d = dlist; d; d = d->link) {
		conout(d->id);
		conout(d->size);
		conout(d->np);
		for(n = 0; n < d->np; n++)
			Bputc(bout, d->map[n]);
	}

	dout();
	lout();
}

void
data(int type, int addr, List *l)
{
	List *f;

	l->type = type;
	l->addr = addr;

	if(mdata == nil)
		mdata = l;
	else {
		for(f = mdata; f->link != nil; f = f->link)
			;
		f->link = l;
	}
}

void
ext(int addr, int type, String *s)
{
	int i;
	char *p;
	List *n;

	data(DEFW, addr, newi(type, nil));

	n = nil;
	p = s->string;
	for(i = 0; i < s->len; i++)
		n = newi(*p++, n);
	data(DEFB, addr+IBY2WD, n);

	if(addr+s->len > dseg)
		diag("ext beyond mp");
}

void
mklink(int desc, int addr, int type, String *s)
{
	Link *l;

	for(l = links; l; l = l->link)
		if(strcmp(l->name, s->string) == 0)
			diag("%s already defined", s->string);

	nlink++;
	l = malloc(sizeof(Link));
	l->desc = desc;
	l->addr = addr;
	l->type = type;
	l->name = s->string;
	l->link = nil;

	if(links == nil)
		links = l;
	else
		linkt->link = l;
	linkt = l;
}

void
dout(void)
{
	int n, i;
	List *l, *e;

	e = nil;
	for(l = mdata; l; l = e) {
		switch(l->type) {
		case DEFB:
			n = 1;
			for(e = l->link; e && e->addr == -1; e = e->link)
				n++;
			if(n < DMAX)
				Bputc(bout, DBYTE(DEFB, n));
			else {
				Bputc(bout, DBYTE(DEFB, 0));
				conout(n);
			}
			conout(l->addr);
			while(l != e) {
				Bputc(bout, l->u.ival);
				l = l->link;
			}
			break;
		case DEFW:
			n = 1;
			for(e = l->link; e && e->addr == -1; e = e->link)
				n++;
			if(n < DMAX)
				Bputc(bout, DBYTE(DEFW, n));
			else {
				Bputc(bout, DBYTE(DEFW, 0));
				conout(n);
			}
			conout(l->addr);
			while(l != e) {
				Bputc(bout, l->u.ival>>24);
				Bputc(bout, l->u.ival>>16);
				Bputc(bout, l->u.ival>>8);
				Bputc(bout, l->u.ival);
				l = l->link;
			}
			break;
		case DEFF:
			n = 1;
			for(e = l->link; e && e->addr == -1; e = e->link)
				n++;
			if(n < DMAX)
				Bputc(bout, DBYTE(DEFF, n));
			else {
				Bputc(bout, DBYTE(DEFF, 0));
				conout(n);
			}
			conout(l->addr);
			while(l != e) {
				Bputc(bout, l->u.ival>>24);
				Bputc(bout, l->u.ival>>16);
				Bputc(bout, l->u.ival>>8);
				Bputc(bout, l->u.ival);
				l = l->link;
			}
			break;
		case DEFS:
			n = l->u.str->len-1;
			if(n < DMAX && n != 0)
				Bputc(bout, DBYTE(DEFS, n));
			else {
				Bputc(bout, DBYTE(DEFS, 0));
				conout(n);
			}
			conout(l->addr);
			for(i = 0; i < n; i++)
				Bputc(bout, l->u.str->string[i]);

			e = l->link;
			break;
		}
	}

	Bputc(bout, DBYTE(DEFZ, 0));
}
