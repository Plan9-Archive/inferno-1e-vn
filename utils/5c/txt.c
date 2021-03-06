#include "gc.h"

void
ginit(void)
{
	Type *t;

	thechar = '5';
	thestring = "arm";
	exregoffset = REGEXT;
	exfregoffset = FREGEXT;
	listinit();
	nstring = 0;
	mnstring = 0;
	nrathole = 0;
	pc = 0;
	breakpc = -1;
	continpc = -1;
	cases = C;
	firstp = P;
	lastp = P;
	tfield = types[TFIELD];
	if(TINT == TSHORT) {
		tint = types[TSHORT];
		tuint = types[TUSHORT];
		types[TFUNC]->link = tint;
	}
	ewidth[TENUM] = ewidth[TINT];
	types[TENUM]->width = ewidth[TENUM];
	suround = SU_ALLIGN;
	supad = SU_PAD;

	zprog.link = P;
	zprog.as = AGOK;
	zprog.reg = NREG;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.from.reg = NREG;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.u0.s2.nreg = REGTMP;
	regnode.complex = 0;
	regnode.addable = 11;
	regnode.type = types[TLONG];

	constnode.op = OCONST;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;
	constnode.type = types[TLONG];

	fconstnode.op = OCONST;
	fconstnode.class = CXXX;
	fconstnode.complex = 0;
	fconstnode.addable = 20;
	fconstnode.type = types[TDOUBLE];

	nodsafe = new(ONAME, Z, Z);
	nodsafe->sym = slookup(".safe");
	nodsafe->type = tint;
	nodsafe->etype = tint->etype;
	nodsafe->class = CAUTO;
	complex(nodsafe);

	t = typ(TARRAY, types[TCHAR]);
	symrathole = slookup(".rathole");
	symrathole->class = CGLOBL;
	symrathole->type = t;

	nodrat = new(ONAME, Z, Z);
	nodrat->sym = symrathole;
	nodrat->type = types[TIND];
	nodrat->etype = TVOID;
	nodrat->class = CGLOBL;
	complex(nodrat);
	nodrat->type = t;

	nodret = new(ONAME, Z, Z);
	nodret->sym = slookup(".ret");
	nodret->type = types[TIND];
	nodret->etype = TIND;
	nodret->class = CPARAM;
	nodret = new(OIND, nodret, Z);
	complex(nodret);

	com64init();

	memset(reg, 0, sizeof(reg));
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(reg[i])
			diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NFREG; i++)
		if(reg[i])
			diag(Z, "freg %d left allocated", i-NREG);
	while(mnstring)
		outstring("", 1L);
	symstring->type->width = nstring;
	symrathole->type->width = nrathole;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == T)
			continue;
		if(s->type->width == 0)
			continue;
		if(s->class != CGLOBL && s->class != CSTATIC)
			continue;
		if(s->type == types[TENUM])
			continue;
		gpseudo(AGLOBL, s, nodconst(s->type->width));
	}
	nextpc();
	p->as = AEND;
	outcode();
}

void
nextpc(void)
{

	ALLOC(p, Prog);
	*p = zprog;
	p->lineno = nearln;
	pc++;
	if(firstp == P) {
		firstp = p;
		lastp = p;
		return;
	}
	lastp->link = p;
	lastp = p;
}

void
gargs(Node *n, Node *tn1, Node *tn2)
{
	long regs;
	Node fnxargs[20], *fnxp;

	regs = cursafe;

	fnxp = fnxargs;
	garg1(n, tn1, tn2, 0, &fnxp);	/* compile fns to temps */

	curarg = 0;
	fnxp = fnxargs;
	garg1(n, tn1, tn2, 1, &fnxp);	/* compile normal args and temps */

	cursafe = regs;
}

void
garg1(Node *n, Node *tn1, Node *tn2, int f, Node **fnxp)
{
	Node nod;

	if(n == Z)
		return;
	if(n->op == OLIST) {
		garg1(n->u0.s0.nleft, tn1, tn2, f, fnxp);
		garg1(n->u0.s0.nright, tn1, tn2, f, fnxp);
		return;
	}
	if(f == 0) {
		if(n->complex >= FNX) {
			regsalloc(*fnxp, n);
			nod = znode;
			nod.op = OAS;
			nod.u0.s0.nleft = *fnxp;
			nod.u0.s0.nright = n;
			nod.type = n->type;
			cgen(&nod, Z);
			(*fnxp)++;
		}
		return;
	}
	if(typesuv[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		return;
	}
	if(REGARG >= 0 && curarg == 0 && typechlp[n->type->etype]) {
		regaalloc1(tn1, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn1);
			(*fnxp)++;
		} else
			cgen(n, tn1);
		return;
	}
	regalloc(tn1, n, Z);
	if(n->complex >= FNX) {
		cgen(*fnxp, tn1);
		(*fnxp)++;
	} else
		cgen(n, tn1);
	regaalloc(tn2, n);
	gopcode(OAS, tn1, Z, tn2);
	regfree(tn1);
}

Node*
nodconst(long v)
{
	constnode.u0.nvconst = v;
	return &constnode;
}

Node*
nod32const(vlong v)
{
	constnode.u0.nvconst = v & MASK(32);
	return &constnode;
}

Node*
nodfconst(double d)
{
	fconstnode.u0.nfconst = d;
	return &fconstnode;
}

void
nodreg(Node *n, Node *nn, int reg)
{
	*n = regnode;
	n->u0.s2.nreg = reg;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

void
regret(Node *n, Node *nn)
{
	int r;

	r = REGRET;
	if(typefd[nn->type->etype])
		r = FREGRET+NREG;
	nodreg(n, nn, r);
	reg[r]++;
}

int
tmpreg(void)
{
	int i;

	for(i=REGRET+1; i<NREG; i++)
		if(reg[i] == 0)
			return i;
	diag(Z, "out of fixed registers");
	return 0;
}

void
regalloc(Node *n, Node *tn, Node *o)
{
	int i, j;
	static lasti;

	switch(tn->type->etype) {
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TLONG:
	case TULONG:
	case TIND:
		if(o != Z && o->op == OREGISTER) {
			i = o->u0.s2.nreg;
			if(i > 0 && i < NREG)
				goto out;
		}
		j = lasti + REGRET+1;
		for(i=REGRET+1; i<NREG; i++) {
			if(j >= NREG)
				j = REGRET+1;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		if(o != Z && o->op == OREGISTER) {
			i = o->u0.s2.nreg;
			if(i >= NREG && i < NREG+NFREG)
				goto out;
		}
		j = 0*2 + NREG;
		for(i=NREG; i<NREG+NREG; i++) {
			if(j >= NREG+NREG)
				j = NREG;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of float registers");
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	i = 0;
out:
	if(i)
		reg[i]++;
	lasti++;
	if(lasti >= 5)
		lasti = 0;
	nodreg(n, tn, i);
}

void
regialloc(Node *n, Node *tn, Node *o)
{
	Node nod;

	nod = *tn;
	nod.type = types[TIND];
	regalloc(n, &nod, o);
}

void
regfree(Node *n)
{
	int i;

	i = 0;
	if(n->op != OREGISTER && n->op != OINDREG)
		goto err;
	i = n->u0.s2.nreg;
	if(i < 0 || i >= sizeof(reg))
		goto err;
	if(reg[i] <= 0)
		goto err;
	reg[i]--;
	return;
err:
	diag(n, "error in regfree: %d", i);
}

void
regsalloc(Node *n, Node *nn)
{
	long o;

	o = nn->type->width;
	o += round(o, tint->width);
	cursafe += o;
	if(cursafe+curarg > maxargsafe)
		maxargsafe = cursafe+curarg;
	*n = *nodsafe;
	n->u0.s2.noffset = -(stkoff + cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
	n->lineno = nn->lineno;
}

void
regaalloc1(Node *n, Node *nn)
{
	int r;

	r = REGARG;
	nodreg(n, nn, r);
	reg[r]++;
	curarg += 4;
	if(cursafe+curarg > maxargsafe)
		maxargsafe = cursafe+curarg;
}

void
regaalloc(Node *n, Node *nn)
{
	long o;

	*n = *nn;
	n->op = OINDREG;
	n->u0.s2.nreg = REGSP;
	n->u0.s2.noffset = curarg + 4;
	n->complex = 0;
	n->addable = 20;
	o = nn->type->width;
	o += round(o, tint->width);
	curarg += o;
	if(cursafe+curarg > maxargsafe)
		maxargsafe = cursafe+curarg;
}

void
regind(Node *n, Node *nn)
{

	if(n->op != OREGISTER) {
		diag(n, "regind not OREGISTER");
		return;
	}
	n->op = OINDREG;
	n->type = nn->type;
}

void
raddr(Node *n, Prog *p)
{
	Adr a;

	naddr(n, &a);
	if(a.type == D_CONST && a.u0.aoffset == 0) {
		a.type = D_REG;
		a.reg = 0;
	}
	if(a.type != D_REG && a.type != D_FREG) {
		if(n)
			diag(n, "bad in raddr: %O", n->op);
		else
			diag(n, "bad in raddr: <null>");
		p->reg = NREG;
	} else
		p->reg = a.reg;
}

void
naddr(Node *n, Adr *a)
{
	long v;

	a->type = D_NONE;
	if(n == Z)
		return;
	switch(n->op) {
	default:
	bad:
		diag(n, "bad in naddr: %O", n->op);
		break;

	case OREGISTER:
		a->type = D_REG;
		a->sym = S;
		a->reg = n->u0.s2.nreg;
		if(a->reg >= NREG) {
			a->type = D_FREG;
			a->reg -= NREG;
		}
		break;

	case OIND:
		naddr(n->u0.s0.nleft, a);
		if(a->type == D_REG) {
			a->type = D_OREG;
			break;
		}
		if(a->type == D_CONST) {
			a->type = D_OREG;
			break;
		}
		goto bad;

	case OINDREG:
		a->type = D_OREG;
		a->sym = S;
		a->u0.aoffset = n->u0.s2.noffset;
		a->reg = n->u0.s2.nreg;
		break;

	case ONAME:
		a->etype = n->etype;
		a->type = D_OREG;
		a->name = D_STATIC;
		a->sym = n->sym;
		a->u0.aoffset = n->u0.s2.noffset;
		if(n->class == CSTATIC)
			break;
		if(n->class == CEXTERN || n->class == CGLOBL) {
			a->name = D_EXTERN;
			break;
		}
		if(n->class == CAUTO) {
			a->name = D_AUTO;
			break;
		}
		if(n->class == CPARAM) {
			a->name = D_PARAM;
			break;
		}
		goto bad;

	case OCONST:
		a->sym = S;
		a->reg = NREG;
		if(typefd[n->type->etype]) {
			a->type = D_FCONST;
			a->u0.adval = n->u0.nfconst;
		} else {
			a->type = D_CONST;
			a->u0.aoffset = n->u0.nvconst;
		}
		break;

	case OADDR:
		naddr(n->u0.s0.nleft, a);
		if(a->type == D_OREG) {
			a->type = D_CONST;
			break;
		}
		goto bad;

	case OADD:
		if(n->u0.s0.nleft->op == OCONST) {
			naddr(n->u0.s0.nleft, a);
			v = a->u0.aoffset;
			naddr(n->u0.s0.nright, a);
		} else {
			naddr(n->u0.s0.nright, a);
			v = a->u0.aoffset;
			naddr(n->u0.s0.nleft, a);
		}
		a->u0.aoffset += v;
		break;

	}
}

void
fop(int as, int f1, int f2, Node *t)
{
	Node nod1, nod2, nod3;

	nodreg(&nod1, t, NREG+f1);
	nodreg(&nod2, t, NREG+f2);
	regalloc(&nod3, t, t);
	gopcode(as, &nod1, &nod2, &nod3);
	gmove(&nod3, t);
	regfree(&nod3);
}

void
gmove(Node *f, Node *t)
{
	int ft, tt, a;
	Node nod;

	ft = f->type->etype;
	tt = t->type->etype;

	if(ft == TDOUBLE && f->op == OCONST) {
	}
	if(ft == TFLOAT && f->op == OCONST) {
	}

	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		switch(ft) {
		default:
			a = AMOVW;
			break;
		case TFLOAT:
			a = AMOVF;
			break;
		case TDOUBLE:
			a = AMOVD;
			break;
		case TCHAR:
			a = AMOVB;
			break;
		case TUCHAR:
			a = AMOVBU;
			break;
		case TSHORT:
			a = AMOVH;
			break;
		case TUSHORT:
			a = AMOVHU;
			break;
		}
		if(typechlp[ft] && typelp[tt])
			regalloc(&nod, t, t);
		else
			regalloc(&nod, f, t);
		gins(a, f, &nod);
		gmove(&nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * a store --
	 * put it into a register then
	 * store it.
	 */
	if(t->op == ONAME || t->op == OINDREG || t->op == OIND) {
		switch(tt) {
		default:
			a = AMOVW;
			break;
		case TUCHAR:
		case TCHAR:
			a = AMOVB;
			break;
		case TUSHORT:
		case TSHORT:
			a = AMOVH;
			break;
		case TFLOAT:
			a = AMOVF;
			break;
		case TVLONG:
		case TDOUBLE:
			a = AMOVD;
			break;
		}
		if(ft == tt)
			regalloc(&nod, t, f);
		else
			regalloc(&nod, t, Z);
		gmove(f, &nod);
		gins(a, &nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * type x type cross table
	 */
	a = AGOK;
	switch(ft) {
	case TDOUBLE:
	case TVLONG:
	case TFLOAT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			a = AMOVD;
			if(ft == TFLOAT)
				a = AMOVFD;
			break;
		case TFLOAT:
			a = AMOVDF;
			if(ft == TFLOAT)
				a = AMOVF;
			break;
		case TLONG:
		case TULONG:
		case TIND:
			a = AMOVWD;
			if(ft == TFLOAT)
				a = AMOVWF;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVWD;
			if(ft == TFLOAT)
				a = AMOVWF;
			break;
		}
		break;
	case TULONG:
	case TLONG:
	case TIND:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			gins(AMOVWD, f, t);
			if(ft == TULONG) {
			}
			return;
		case TFLOAT:
			gins(AMOVWF, f, t);
			if(ft == TULONG) {
			}
			return;
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TSHORT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVH, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TULONG:
		case TLONG:
		case TIND:
			a = AMOVH;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TUSHORT:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			regalloc(&nod, f, Z);
			gins(AMOVHU, f, &nod);
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVHU, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TLONG:
		case TULONG:
		case TIND:
			a = AMOVHU;
			break;
		case TSHORT:
		case TUSHORT:
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TCHAR:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			regalloc(&nod, f, Z);
			gins(AMOVB, f, &nod);
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVB, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
			a = AMOVB;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	case TUCHAR:
		switch(tt) {
		case TDOUBLE:
		case TVLONG:
			regalloc(&nod, f, Z);
			gins(AMOVBU, f, &nod);
			gins(AMOVWD, &nod, t);
			regfree(&nod);
			return;
		case TFLOAT:
			regalloc(&nod, f, Z);
			gins(AMOVBU, f, &nod);
			gins(AMOVWF, &nod, t);
			regfree(&nod);
			return;
		case TLONG:
		case TULONG:
		case TIND:
		case TSHORT:
		case TUSHORT:
			a = AMOVBU;
			break;
		case TCHAR:
		case TUCHAR:
			a = AMOVW;
			break;
		}
		break;
	}
	if(a == AMOVW || a == AMOVF || a == AMOVD)
	if(samaddr(f, t))
		return;
	gins(a, f, t);
}

void
gins(int a, Node *f, Node *t)
{

	nextpc();
	p->as = a;
	if(f != Z)
		naddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

void
gopcode(int o, Node *f1, Node *f2, Node *t)
{
	int a, et;
	Adr ta;

	et = TLONG;
	if(f1 != Z && f1->type != T)
		et = f1->type->etype;
	a = AGOK;
	switch(o) {
	case OAS:
		gmove(f1, t);
		return;

	case OASADD:
	case OADD:
		a = AADD;
		if(et == TFLOAT)
			a = AADDF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = AADDD;
		break;

	case OASSUB:
	case OSUB:
		a = ASUB;
		if(et == TFLOAT)
			a = ASUBF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = ASUBD;
		break;

	case OASOR:
	case OOR:
		a = AORR;
		break;

	case OASAND:
	case OAND:
		a = AAND;
		break;

	case OASXOR:
	case OXOR:
		a = AEOR;
		break;

	case OASLSHR:
	case OLSHR:
		a = ASRL;
		break;

	case OASASHR:
	case OASHR:
		a = ASRA;
		break;

	case OASASHL:
	case OASHL:
		a = ASLL;
		break;

	case OFUNC:
		a = ABL;
		break;

	case OASMUL:
	case OMUL:
		a = AMUL;
		if(et == TFLOAT)
			a = AMULF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = AMULD;
		break;

	case OASDIV:
	case ODIV:
		a = ADIV;
		if(et == TFLOAT)
			a = ADIVF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = ADIVD;
		break;

	case OASMOD:
	case OMOD:
		a = AMOD;
		break;

	case OASLMUL:
	case OLMUL:
		a = AMULU;
		break;

	case OASLMOD:
	case OLMOD:
		a = AMODU;
		break;

	case OASLDIV:
	case OLDIV:
		a = ADIVU;
		break;

	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHS:
	case OHI:
		a = ACMP;
		if(et == TFLOAT)
			a = ACMPF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = ACMPD;
		nextpc();
		p->as = a;
		naddr(f1, &p->from);
		raddr(f2, p);
		switch(o) {
		case OEQ:
			a = ABEQ;
			break;
		case ONE:
			a = ABNE;
			break;
		case OLT:
			a = ABLT;
			break;
		case OLE:
			a = ABLE;
			break;
		case OGE:
			a = ABGE;
			break;
		case OGT:
			a = ABGT;
			break;
		case OLO:
			a = ABLO;
			break;
		case OLS:
			a = ABLS;
			break;
		case OHS:
			a = ABHS;
			break;
		case OHI:
			a = ABHI;
			break;
		}
		f1 = Z;
		f2 = Z;
		break;
	}
	if(a == AGOK)
		diag(Z, "bad in gopcode %O", o);
	nextpc();
	p->as = a;
	if(f1 != Z)
		naddr(f1, &p->from);
	if(f2 != Z) {
		naddr(f2, &ta);
		p->reg = ta.reg;
	}
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

samaddr(Node *f, Node *t)
{

	if(f->op != t->op)
		return 0;
	switch(f->op) {

	case OREGISTER:
		if(f->u0.s2.nreg != t->u0.s2.nreg)
			break;
		return 1;
	}
	return 0;
}

void
gbranch(int o)
{
	int a;

	a = AGOK;
	switch(o) {
	case ORETURN:
		a = ARET;
		break;
	case OGOTO:
		a = AB;
		break;
	}
	nextpc();
	if(a == AGOK) {
		diag(Z, "bad in gbranch %O",  o);
		nextpc();
	}
	p->as = a;
}

void
patch(Prog *op, long pc)
{

	op->to.u0.aoffset = pc;
	op->to.type = D_BRANCH;
}

void
gpseudo(int a, Sym *s, Node *n)
{

	nextpc();
	p->as = a;
	p->from.type = D_OREG;
	p->from.sym = s;
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
sconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->u0.nvconst;
			if(vv >= (vlong)(-32766) && vv < (vlong)32766)
				return 1;
		}
	}
	return 0;
}

int
sval(long v)
{
	if(v >= -32766L && v < 32766L)
		return 1;
	return 0;
}

long
exreg(Type *t)
{
	long o;

	if(typechlp[t->etype]) {
		if(exregoffset <= NREG-1)
			return 0;
		o = exregoffset;
		exregoffset--;
		return o;
	}
	if(typefd[t->etype]) {
		if(exfregoffset <= NFREG-1)
			return 0;
		o = exfregoffset + NREG;
		exfregoffset--;
		return o;
	}
	return 0;
}

schar	ewidth[XTYPE] =
{
	-1,				/* TXXX */
	SZ_CHAR,	SZ_CHAR,	/* TCHAR	TUCHAR */
	SZ_SHORT,	SZ_SHORT,	/* TSHORT	TUSHORT */
	SZ_LONG,	SZ_LONG,	/* TLONG	TULONG */
	SZ_VLONG,	SZ_VLONG,	/* TVLONG	TUVLONG */
	SZ_FLOAT,	SZ_DOUBLE,	/* TFLOAT	TDOUBLE */
	SZ_IND,		0,		/* TIND		TFUNC */
	-1,		0,		/* TARRAY	TVOID */
	-1,		-1,		/* TSTRUCT	TUNION */
	-1				/* TENUM */
};
long	ncast[XTYPE] =
{
	/* TXXX */	0,
	/* TCHAR */	BCHAR|BUCHAR,
	/* TUCHAR */	BCHAR|BUCHAR,
	/* TSHORT */	BSHORT|BUSHORT,
	/* TUSHORT */	BSHORT|BUSHORT,
	/* TLONG */	BLONG|BULONG|BIND,
	/* TULONG */	BLONG|BULONG|BIND,
	/* TVLONG */	BVLONG|BUVLONG,
	/* TUVLONG */	BVLONG|BUVLONG,
	/* TFLOAT */	BFLOAT,
	/* TDOUBLE */	BDOUBLE,
	/* TIND */	BLONG|BULONG|BIND,
	/* TFUNC */	0,
	/* TARRAY */	0,
	/* TVOID */	0,
	/* TSTRUCT */	BSTRUCT,
	/* TUNION */	BUNION,
	/* TENUM */	0,
};
