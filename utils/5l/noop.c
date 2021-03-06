#include	"l.h"

void
noops(void)
{
	Prog *p, *q, *q1;
	int o, curframe, curbecome, maxbecome;

	/*
	 * find leaf subroutines
	 * become sizes
	 * frame sizes
	 * strip NOPs
	 * expand RET
	 * expand BECOME pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);

	curframe = 0;
	curbecome = 0;
	maxbecome = 0;
	curtext = 0;

	q = P;
	for(p = firstp; p != P; p = p->link) {

		/* find out how much arg space is used in this TEXT */
		if(p->to.type == D_OREG && p->to.reg == REGSP)
			if(p->to.u0.aoffset > curframe)
				curframe = p->to.u0.aoffset;

		switch(p->as) {
		case ATEXT:
			if(curtext && curtext->from.u1.asym) {
				curtext->from.u1.asym->frame = curframe;
				curtext->from.u1.asym->become = curbecome;
				if(curbecome > maxbecome)
					maxbecome = curbecome;
			}
			curframe = 0;
			curbecome = 0;

			p->mark |= LEAF;
			curtext = p;
			break;

		case ARET:
			/* special form of RET is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.u0.aoffset > curbecome)
					curbecome = p->from.u0.aoffset;
			break;

		case ADIV:
		case ADIVU:
		case AMOD:
		case AMODU:
			q = p;
			if(prog_div == P)
				initdiv();
			if(curtext != P)
				curtext->mark &= ~LEAF;
			continue;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case ABL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case ABCASE:
		case AB:

		case ABEQ:
		case ABNE:
		case ABCS:
		case ABHS:
		case ABCC:
		case ABLO:
		case ABMI:
		case ABPL:
		case ABVS:
		case ABVC:
		case ABHI:
		case ABLS:
		case ABGE:
		case ABLT:
		case ABGT:
		case ABLE:

		dstlab:
			q1 = p->cond;
			if(q1 != P) {
				while(q1->as == ANOP) {
					q1 = q1->link;
					p->cond = q1;
				}
			}
			break;
		}
		q = p;
	}

	if(curtext && curtext->from.u1.asym) {
		curtext->from.u1.asym->frame = curframe;
		curtext->from.u1.asym->become = curbecome;
		if(curbecome > maxbecome)
			maxbecome = curbecome;
	}

	if(debug['b'])
		print("max become = %d\n", maxbecome);
	xdefine("ALEFbecome", STEXT, maxbecome);

	curtext = 0;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		case ATEXT:
			curtext = p;
			break;
		case ABL:
			if(curtext != P && curtext->from.u1.asym != S && curtext->to.u0.aoffset >= 0) {
				o = maxbecome - curtext->from.u1.asym->frame;
				if(o <= 0)
					break;
				/* calling a become or calling a variable */
				if(p->to.u1.asym == S || p->to.u1.asym->become) {
					curtext->to.u0.aoffset += o;
					if(debug['b']) {
						curp = p;
						print("%D calling %D increase %d\n",
							&curtext->from, &p->to, o);
					}
				}
			}
			break;
		}
	}

	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			curtext = p;
			autosize = p->to.u0.aoffset + 4;
			if(autosize <= 4)
			if(curtext->mark & LEAF) {
				p->to.u0.aoffset = -4;
				autosize = 0;
			}

			q = p;
			if(autosize) {
				q = prg();
				q->as = ASUB;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.u0.aoffset = autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			} else
			if(!(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.u1.asym->name);
				Bflush(&bso);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.u1.asym)
					curtext->from.u1.asym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = AMOVW;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_OREG;
			q1->from.u0.aoffset = 0;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARET:
			nocache(p);
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AB;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.u0.aoffset = 0;
					p->to.reg = REGLINK;
					break;
				}

				p->as = AADD;
				p->from.type = D_CONST;
				p->from.u0.aoffset = autosize;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				q = prg();
				q->as = AB;
				q->line = p->line;
				q->to.type = D_OREG;
				q->to.u0.aoffset = 0;
				q->to.reg = REGLINK;

				q->link = p->link;
				p->link = q;
				break;
			}
			p->as = AMOVW;
			p->from.type = D_OREG;
			p->from.u0.aoffset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = 2;

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADD;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.u0.aoffset = autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			}

			q1 = prg();
			q1->as = AB;
			q1->line = p->line;
			q1->to.type = D_OREG;
			q1->to.u0.aoffset = 0;
			q1->to.reg = 2;

			q1->link = q->link;
			q->link = q1;
			break;

		become:
			if(curtext->mark & LEAF) {

				q = prg();
				q->line = p->line;
				q->as = AB;
				q->from = zprg.from;
				q->to = p->to;
				q->cond = p->cond;
				q->link = p->link;
				p->link = q;

				p->as = AADD;
				p->from = zprg.from;
				p->from.type = D_CONST;
				p->from.u0.aoffset = autosize;
				p->to = zprg.to;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				break;
			}
			q = prg();
			q->line = p->line;
			q->as = AB;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			p->link = q;

			q = prg();
			q->line = p->line;
			q->as = AADD;
			q->from.type = D_CONST;
			q->from.u0.aoffset = autosize;
			q->to.type = D_REG;
			q->to.reg = REGSP;
			q->link = p->link;
			p->link = q;

			p->as = AMOVW;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.u0.aoffset = 0;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;

		case ADIV:
		case ADIVU:
		case AMOD:
		case AMODU:
			if(debug['M'])
				break;
			if(p->from.type != D_REG)
				break;
			if(p->to.type != D_REG)
				break;
			q1 = p;

			/* MOV a,4(SP) */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = q1->from.reg;
			p->to.type = D_OREG;
			p->to.reg = REGSP;
			p->to.u0.aoffset = 4;

			/* MOV b,REGTMP */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = q1->reg;
			if(q1->reg == NREG)
				p->from.reg = q1->to.reg;
			p->to.type = D_REG;
			p->to.reg = REGTMP;
			p->to.u0.aoffset = 0;

			/* CALL appropriate */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = ABL;
			p->line = q1->line;
			p->to.type = D_BRANCH;
			p->cond = p;
			switch(o) {
			case ADIV:
				p->cond = prog_div;
				break;
			case ADIVU:
				p->cond = prog_divu;
				break;
			case AMOD:
				p->cond = prog_mod;
				break;
			case AMODU:
				p->cond = prog_modu;
				break;
			}

			/* MOV REGTMP, b */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AMOVW;
			p->line = q1->line;
			p->from.type = D_REG;
			p->from.reg = REGTMP;
			p->from.u0.aoffset = 0;
			p->to.type = D_REG;
			p->to.reg = q1->to.reg;

			/* ADD $8,SP */
			q = prg();
			q->link = p->link;
			p->link = q;
			p = q;

			p->as = AADD;
			p->from.type = D_CONST;
			p->from.reg = NREG;
			p->from.u0.aoffset = 8;
			p->reg = NREG;
			p->to.type = D_REG;
			p->to.reg = REGSP;

			/* SUB $8,SP */
			q1->as = ASUB;
			q1->from.type = D_CONST;
			q1->from.u0.aoffset = 8;
			q1->from.reg = NREG;
			q1->reg = NREG;
			q1->to.type = D_REG;
			q1->to.reg = REGSP;
			break;
		}
	}
}

void
initdiv(void)
{
	Sym *s2, *s3, *s4, *s5;
	Prog *p;

	s2 = lookup("_div", 0);
	s3 = lookup("_divu", 0);
	s4 = lookup("_mod", 0);
	s5 = lookup("_modu", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.u1.asym == s2)
				prog_div = p;
			if(p->from.u1.asym == s3)
				prog_divu = p;
			if(p->from.u1.asym == s4)
				prog_mod = p;
			if(p->from.u1.asym == s5)
				prog_modu = p;
		}
	if(prog_div == P) {
		diag("undefined: %s\n", s2->name);
		prog_div = curtext;
	}
	if(prog_divu == P) {
		diag("undefined: %s\n", s3->name);
		prog_divu = curtext;
	}
	if(prog_mod == P) {
		diag("undefined: %s\n", s4->name);
		prog_mod = curtext;
	}
	if(prog_modu == P) {
		diag("undefined: %s\n", s5->name);
		prog_modu = curtext;
	}
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}
