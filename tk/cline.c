#include "lib9.h"
#include "image.h"
#include "tk.h"

#define	O(t, e)		((long)(&((t*)0)->e))

/* Line Options (+ means implemented)
	+arrow
	+arrowshape
	+capstyle
	+fill
	 joinstyle
	+smooth
	+splinesteps
	+stipple
	+tags
	+width
*/

static
TkStab tkcapstyle[] =
{
	"bevel",	Endsquare,
	"miter",	Endsquare,
	"round",	Enddisc,
	nil
};

static
TkOption lineopts[] =
{
	"arrow",	OPTstab,	O(TkCline, arrow),	tklines,
	"arrowshape",	OPTfrac,	O(TkCline, shape[0]),	IAUX(3),
	"width",	OPTfrac,	O(TkCline, width),	nil,
	"stipple",	OPTbmap,	O(TkCline, stipple),	nil,
	"smooth",	OPTstab,	O(TkCline, smooth),	tkbool,
	"splinesteps",	OPTdist,	O(TkCline, steps),	nil,
	"capstyle",	OPTstab,	O(TkCline, capstyle),	tkcapstyle,
	nil
};

static
TkOption itemopts[] =
{
	"tags",		OPTctag,	O(TkCitem, tags),	nil,
	"fill",		OPTcolr,	O(TkCitem, env),	IAUX(TkCforegnd),
	nil
};

void
tkcvslinesize(TkCitem *i)
{
	TkCline *l;
	int j, w, as, shape[3], arrow;

	l = TKobj(TkCline, i);
	w = TKF2I(l->width);

	i->p.bb = bbnil;
	tkpolybound(i->p.drawpt, i->p.npoint, &i->p.bb);

	l->arrowf = l->capstyle;
	l->arrowl = l->capstyle;
	if(l->arrow != 0) {
		as = w/3;
		if(as < 1)
			as = 1;
		for(j = 0; j < 3; j++) {
			shape[j] = l->shape[j];
			if(shape[j] == 0)
				shape[j] = as * cvslshape[j];
		}
		arrow = ARROW(TKF2I(shape[0]), TKF2I(shape[1]), TKF2I(shape[2]));
		if(l->arrow & TkCarrowf)
			l->arrowf = arrow;
		if(l->arrow & TkCarrowl)
			l->arrowl = arrow;
		w += shape[2];
	}

	i->p.bb = insetrect(i->p.bb, -w);
}

char*
tkcvslinecreat(Tk* tk, char *arg, char **val)
{
	char *e;
	TkCline *l;
	TkCitem *i;
	TkCanvas *c;
	TkOptab tko[3];

	c = TKobj(TkCanvas, tk);

	i = tkcnewitem(tk, TkCVline, sizeof(TkCitem)+sizeof(TkCline));
	if(i == nil)
		return TkNomem;

	l = TKobj(TkCline, i);
	l->width = TKI2F(1);

	e = tkparsepts(tk->env->top, &i->p, &arg);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}

	tko[0].ptr = l;
	tko[0].optab = lineopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;
	e = tkparse(tk->env->top, arg, tko, nil);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}
	tkmkpen(&l->pen, i->env, l->stipple);

	e = tkcaddtag(tk, i, 1);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}

	tkcvslinesize(i);
	tkcvsappend(c, i);

	if(tk->master || tk->parent) {
		tkbbmax(&c->update, &i->p.bb);
		tk->flag |= Tkdirty;
	}
	return tkvalue(val, "%d", i->id);
}

char*
tkcvslinecget(TkCitem *i, char *arg, char **val)
{
	TkOptab tko[3];
	TkCline *l = TKobj(TkCline, i);

	tko[0].ptr = l;
	tko[0].optab = lineopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;

	return tkgencget(tko, arg, val);
}

char*
tkcvslineconf(Tk *tk, TkCitem *i, char *arg)
{
	char *e;
	TkOptab tko[3];
	TkCline *l = TKobj(TkCline, i);

	tko[0].ptr = l;
	tko[0].optab = lineopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;

	e = tkparse(tk->env->top, arg, tko, nil);

	tkmkpen(&l->pen, i->env, l->stipple);
	tkcvslinesize(i);

	return e;
}

void
tkcvslinefree(TkCitem *i)
{
	TkCline *l;

	l = TKobj(TkCline, i);
	if(l->stipple)
		freeimage(l->stipple);
	if(l->pen)
		freeimage(l->pen);
}

void
tkcvslinedraw(Image *img, TkCitem *i)
{
	int w;
	Point *p;
	TkCline *l;
	Image *pen;

	l = TKobj(TkCline, i);

	pen = l->pen;
	if(pen == nil)
		pen = tkgc(i->env, TkCforegnd);

	w = TKF2I(l->width)/2;
	if(w < 0)
		return;

	p = i->p.drawpt;
	if(l->smooth == BoolT)
		bezspline(img, p, i->p.npoint, l->arrowf, l->arrowl, w, pen, p[0]);
	else
		poly(img, p, i->p.npoint, l->arrowf, l->arrowl, w, pen, p[0]);
}

char*
tkcvslinecoord(TkCitem *i, char *arg, int x, int y)
{
	char *e;
	TkCpoints p;

	if(arg == nil) {
		tkxlatepts(i->p.parampt, i->p.npoint, x, y);
		tkxlatepts(i->p.drawpt, i->p.npoint, TKF2I(x), TKF2I(y));
	}
	else {
		p.parampt = nil;
		p.drawpt = nil;
		e = tkparsepts(i->env->top, &p, &arg);
		if(e != nil) {
			tkfreepoint(&p);
			return e;
		}
		if(p.npoint < 2) {
			tkfreepoint(&p);
			return TkFewpt;
		}
		tkfreepoint(&i->p);
		i->p = p;
	}
	tkcvslinesize(i);
	return nil;
}
