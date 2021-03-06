#include "lib9.h"
#include "image.h"
#include "tk.h"

#define	O(t, e)		((long)(&((t*)0)->e))

/* Bitmap Options (+ means implemented)
	+anchor
	+bitmap
*/

typedef struct TkCbits TkCbits;
struct TkCbits
{
	int	anchor;
	Point	anchorp;
	Image*	bitmap;
};

static
TkOption bitopts[] =
{
	"anchor",	OPTstab,	O(TkCbits, anchor),	tkanchor,
	"bitmap",	OPTbmap,	O(TkCbits, bitmap),	nil,
	nil
};

static
TkOption itemopts[] =
{
	"tags",		OPTctag,	O(TkCitem, tags),	nil,
	"background",	OPTcolr,	O(TkCitem, env),	IAUX(TkCbackgnd),
	"foreground",	OPTcolr,	O(TkCitem, env),	IAUX(TkCforegnd),
	nil
};

void
tkcvsbitsize(TkCitem *i)
{
	Point o;
	int dx, dy;
	TkCbits *b;

	b = TKobj(TkCbits, i);
	i->p.bb = bbnil;
	if(b->bitmap == nil)
		return;

	dx = Dx(b->bitmap->r);
	dy = Dy(b->bitmap->r);

	o = tkcvsanchor(i->p.drawpt[0], dx, dy, b->anchor);

	b->anchorp = o;
	i->p.bb.min.x = o.x;
	i->p.bb.min.y = o.y;
	i->p.bb.max.x = o.x + dx;
	i->p.bb.max.y = o.y + dy;
}

char*
tkcvsbitcreat(Tk* tk, char *arg, char **val)
{
	char *e;
	TkCbits *b;
	TkCitem *i;
	TkCanvas *c;
	TkOptab tko[3];

	c = TKobj(TkCanvas, tk);

	i = tkcnewitem(tk, TkCVbitmap, sizeof(TkCitem)+sizeof(TkCbits));
	if(i == nil)
		return TkNomem;

	b = TKobj(TkCbits, i);

	e = tkparsepts(tk->env->top, &i->p, &arg);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}
	if(i->p.npoint != 1) {
		tkcvsfreeitem(i);
		return TkFewpt;
	}

	tko[0].ptr = b;
	tko[0].optab = bitopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;
	e = tkparse(tk->env->top, arg, tko, nil);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}
	
	e = tkcaddtag(tk, i, 1);
	if(e != nil) {
		tkcvsfreeitem(i);
		return e;
	}

	tkcvsbitsize(i);
	tkcvsappend(c, i);

	if(tk->master || tk->parent) {
		tkbbmax(&c->update, &i->p.bb);
		tk->flag |= Tkdirty;
	}
	return tkvalue(val, "%d", i->id);
}

char*
tkcvsbitcget(TkCitem *i, char *arg, char **val)
{
	TkOptab tko[3];
	TkCbits *b = TKobj(TkCbits, i);

	tko[0].ptr = b;
	tko[0].optab = bitopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;

	return tkgencget(tko, arg, val);
}

char*
tkcvsbitconf(Tk *tk, TkCitem *i, char *arg)
{
	char *e;
	TkOptab tko[3];
	TkCbits *b = TKobj(TkCbits, i);

	tko[0].ptr = b;
	tko[0].optab = bitopts;
	tko[1].ptr = i;
	tko[1].optab = itemopts;
	tko[2].ptr = nil;

	e = tkparse(tk->env->top, arg, tko, nil);
	tkcvsbitsize(i);
	return e;
}

void
tkcvsbitfree(TkCitem *i)
{
	TkCbits *b;

	b = TKobj(TkCbits, i);
	if(b->bitmap)
		freeimage(b->bitmap);
}

void
tkcvsbitdraw(Image *img, TkCitem *i)
{
	TkEnv *e;
	TkCbits *b;
	Rectangle r;
	Image *ones, *bi;

	e = i->env;
	b = TKobj(TkCbits, i);

	ones = img->display->ones;
	bi = b->bitmap;
	if(bi == nil)
		return;

	r.min = b->anchorp;
	r.max = r.min;
	r.max.x += Dx(bi->r);
	r.max.y += Dy(bi->r);

	if(bi->ldepth != 0) {
		draw(img, r, bi, ones, tkzp);
		return;
	}
	draw(img, r, tkgc(e, TkCbackgnd), ones, tkzp);
	draw(img, r, tkgc(e, TkCforegnd), bi, tkzp);
}

char*
tkcvsbitcoord(TkCitem *i, char *arg, int x, int y)
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
		if(p.npoint != 1) {
			tkfreepoint(&p);
			return TkFewpt;
		}
		tkfreepoint(&i->p);
		i->p = p;
	}
	tkcvsbitsize(i);
	return nil;
}
