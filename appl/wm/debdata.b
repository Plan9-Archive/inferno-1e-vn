implement DebData;

include "sys.m";
	sys: Sys;

include "draw.m";

include "string.m";
	str: String;

include "tk.m";
	tk: Tk;

include "tklib.m";
	tklib: Tklib;

include "wmlib.m";
	wmlib: Wmlib;

include "debug.m";
	debug: Debug;
	Sym, Src, Exp, Module: import debug;

include "wmdeb.m";
	debsrc: DebSrc;

DatumSize:	con 32;
WalkWidth:	con "20";

screen:		ref Draw->Screen;
tktop:		ref Tk->Toplevel;
var:		ref Vars;
vid:		int;
tkids :=	1;	# increasing id of tk pieces

icondir :	con "debug/";

tkconfig := array[] of {
	"frame .body -width 600 -height 400",
	"pack .Wm_t -side top -fill x",
	"pack .body -expand 1 -fill both",
	"pack propagate . 0",
	"update",
	"image create bitmap Itemopen -file "+icondir+"open.bit -maskfile "+icondir+"open.bit",
	"image create bitmap Itemclosed -file "+icondir+"closed.bit -maskfile "+icondir+"closed.bit",
};

init(ascreen: ref Draw->Screen,
	geom: string,
	adebsrc: DebSrc,
	asys: Sys,
	atk: Tk,
	atklib: Tklib,
	astr: String,
	adebug: Debug): (chan of string, chan of string)
{
	screen = ascreen;
	debsrc = adebsrc;
	sys = asys;
	tk = atk;
	tklib = atklib;
	str = astr;
	debug = adebug;

	wmlib = load Wmlib Wmlib->PATH;

	wmlib->init();
	tktop = tk->toplevel(screen, geom+" -relief raised -bd 2");
	titlebut := wmlib->titlebar(tktop, "Stack", Wmlib->Appl);
	buts := chan of string;
	tk->namechan(tktop, buts, "buts");

	tklib->tkcmds(tktop, tkconfig);

	tkcmd("update");
	return (buts, titlebut);
}

ctl(s: string)
{
	if(var == nil)
		return;
	arg := s[1:];
	case s[0]{
	'o' =>
		var.expand(arg);
		var.update();
	'c' =>
		var.contract(arg);
		var.update();
	'y' =>
		var.scrolly(arg);
	's' =>
		var.showsrc(arg);
	}
	tkcmd("update");
}

titlectl(s: string)
{
	if(s[0] == 'e'){
		tkcmd("destroy .");
		return;
	}
	wmlib->titlectl(tktop, s);
	tkcmd("update");
}

Vars.create(): ref Vars
{
	t := ".body.v"+string vid++;

	tkcmd("frame "+t);
	xpanes(t);
	tkcmd("canvas "+t+".cvar -width 2 -height 2 -yscrollcommand {"+t+".sy set} -xscrollcommand {"+t+".sxvar set}");
	tkcmd("canvas "+t+".cval -width 2 -height 2 -xscrollcommand {"+t+".sxval set}");
	tkcmd("frame "+t+".f0");
	tkcmd("frame "+t+".F0");

	tkcmd(t+".cvar create window 0 0 -window "+t+".f0 -anchor nw");
	tkcmd(t+".cval create window 0 0 -window "+t+".F0 -anchor nw");
	tkcmd("scrollbar "+t+".sxvar -orient horizontal -command {"+t+".cvar xview}");
	tkcmd("scrollbar "+t+".sxval -orient horizontal -command {"+t+".cval xview}");
	tkcmd("pack "+t+".sxvar -fill x -side bottom -in "+t+".var");
	tkcmd("pack "+t+".sxval -fill x -side bottom -in "+t+".val");
	tkcmd("pack "+t+".cvar -expand 1 -fill both -in "+t+".var");
	tkcmd("pack "+t+".cval -expand 1 -fill both -in "+t+".val");

	tkcmd("scrollbar "+t+".sy -command {send buts y}");
	tkcmd("pack "+t+".sy -side right -fill y -in "+t);
	tkcmd("pack "+t+".vars -expand 1 -fill both -in "+t);

	return ref Vars(t, 0, nil);
}

xpanes(t: string)
{
	tkcmd("frame "+t+".vars");
	tkcmd("frame "+t+".var");
	tkcmd("frame "+t+".val");
	tkcmd("frame "+t+".bar -width 2 -borderwidth 1 -relief raised");
	tkcmd("pack "+t+".var -expand 1 -fill both -side left -in "+t+".vars");
	tkcmd("pack "+t+".bar -fill y -side left -in "+t+".vars");
	tkcmd("pack "+t+".val -expand 1 -fill both -side left -in "+t+".vars");
}

Vars.show(v: self ref Vars)
{
	if(v == var)
		return;
	if(var != nil)
		tkcmd("pack forget "+var.tk);
	var = v;
	tkcmd("pack "+var.tk+" -expand 1 -fill both");
	v.update();
}

Vars.delete(v: self ref Vars)
{
	if(var == v)
		var = nil;
	tkcmd("destroy "+v.tk);
	tkcmd("update");
}

Vars.refresh(v: self ref Vars, ea: array of ref Exp)
{
	vtk := v.tk;
	nea := len ea;
	newd := array[nea] of ref Datum;
	da := v.d;
	nd := len da;
	n := nea;
	if(n > nd)
		n = nd;
	for(i := 0; i < n; i++){
		d := da[nd-i-1];
		if(!sameexp(ea[nea-i-1], d.e, 1))
			break;
		newd[nea-i-1] = d;
	}
	n = nea-i;
	for(; i < nd; i++)
		da[nd-i-1].destroy();
	v.d = nil;
	for(i = 0; i < n; i++){
		debsrc->findmod(ea[i].m);
		ea[i].findsym();
		newd[i] = mkkid(ea[i], v.tk, "0", string tkids++, nil, nil, -1, "");
	}
	for(; i < nea; i++){
		debsrc->findmod(ea[i].m);
		ea[i].findsym();
		d := newd[i];
		newd[i] = mkkid(ea[i], v.tk, "0", d.tkid, d.kids, d.val, d.canwalk, "");
	}
	v.d = newd;
	v.update();
}

Vars.update(v: self ref Vars)
{
	tkcmd("update");
	tkcmd(v.tk+".cvar configure -scrollregion {0 0 ["+v.tk+".f0 cget -width] ["+v.tk+".f0 cget -height]}");
	tkcmd(v.tk+".cval configure -scrollregion {0 0 ["+v.tk+".F0 cget -width] ["+v.tk+".F0 cget -height]}");
	tkcmd("update");
}

Vars.scrolly(v: self ref Vars, pos: string)
{
	tkcmd(v.tk+".cvar yview"+pos);
	tkcmd(v.tk+".cval yview"+pos);
}

Vars.showsrc(v: self ref Vars, who: string)
{
	(sid, kids) := str->splitl(who[1:], ".");
	showsrc(v.d, sid, kids);
}

showsrc(da: array of ref Datum, id, kids: string)
{
	if(da == nil)
		return;
	for(i := 0; i < len da; i++){
		d := da[i];
		if(d.tkid != id)
			continue;
		if(kids == "")
			d.showsrc();
		else{
			sid : string;
			(sid, kids) = str->splitl(kids[1:], ".");
			showsrc(d.kids, sid, kids);
		}
		break;
	}
}

Vars.expand(v: self ref Vars, who: string)
{
	(sid, kids) := str->splitl(who[1:], ".");
	v.d = expandkid(v.d, sid, kids, who);
}

expandkid(da: array of ref Datum, id, kids, who: string): array of ref Datum
{
	if(da == nil)
		return nil;
	for(i := 0; i < len da; i++){
		d := da[i];
		if(d.tkid != id)
			continue;
		if(kids == "")
			da[i] = d.expand(nil, who);
		else{
			sid : string;
			(sid, kids) = str->splitl(kids[1:], ".");
			d.kids = expandkid(d.kids, sid, kids, who);
		}
		break;
	}
	return da;
}

Vars.contract(v: self ref Vars, who: string)
{
	(sid, kids) := str->splitl(who[1:], ".");
	v.d = contractkid(v.d, sid, kids, who);
}

contractkid(da: array of ref Datum, id, kids, who: string): array of ref Datum
{
	if(da == nil)
		return nil;
	for(i := 0; i < len da; i++){
		d := da[i];
		if(d.tkid != id)
			continue;
		if(kids == "")
			da[i] = d.contract(who);
		else{
			sid : string;
			(sid, kids) = str->splitl(kids[1:], ".");
			d.kids = contractkid(d.kids, sid, kids, who);
		}
		break;
	}
	return da;
}

Datum.contract(d: self ref Datum, who: string): ref Datum
{
	vtk := d.vtk;
	tkid := d.tkid;
	if(tkid == "")
		return d;
	kids := d.kids;
	if(kids == nil){
		tkcmd(vtk+".v"+tkid+".b configure -image Itemclosed -command {send buts o"+who+"}");
		return d;
	}

	for(i := 0; i < len kids; i++)
		kids[i].destroy();
	d.kids = nil;
	tkcmd("destroy "+vtk+".f"+tkid);
	tkcmd("destroy "+vtk+".F"+tkid);
	tkcmd(vtk+".v"+tkid+".b configure -image Itemclosed -command {send buts o"+who+"}");

	return d;
}

Datum.showsrc(d: self ref Datum)
{
	debsrc->showmodsrc(debsrc->findmod(d.e.m), d.e.src());
}

Datum.destroy(d: self ref Datum)
{
	kids := d.kids;
	for(i := 0; i < len kids; i++)
		kids[i].destroy();
	vtk := d.vtk;
	tkid := string d.tkid;
	if(d.kids != nil){
		tkcmd("destroy "+vtk+".f"+tkid);
		tkcmd("destroy "+vtk+".F"+tkid);
	}
	d.kids = nil;
	tkcmd("destroy "+vtk+".v"+tkid);
	tkcmd("destroy "+vtk+".V"+tkid);
}

mkkid(e: ref Exp, vtk, parent, me: string, okids: array of ref Datum, oval:string, owalk: int, who: string): ref Datum
{
	(val, walk) := e.val();

	who = who+"."+me;

	# make the tk goo
	if(walk != owalk){
		if(owalk != -1)
			tkcmd("destroy "+vtk+".v"+me);
		tkcmd("frame "+vtk+".v"+me);
		if(walk)
			tkcmd("button "+vtk+".v"+me+".b -image Itemclosed -command 'send buts o"+who);
		else
			tkcmd("frame "+vtk+".v"+me+".b -width "+WalkWidth);
		tkcmd("label "+vtk+".v"+me+".l -text '"+e.name);
		tkcmd("bind "+vtk+".v"+me+".l <ButtonRelease-1> 'send buts s"+who);
		tkcmd("pack "+vtk+".v"+me+".b "+vtk+".v"+me+".l -side left");
	}
	tkcmd("pack "+vtk+".v"+me+" -side top -anchor w -in "+vtk+".f"+parent);

	# tk value goo
	if(val == "")
		val = " ";
	if(oval != ""){
		if(val != oval)
			tkcmd(vtk+".V"+me+" configure -text '"+val);
	}else
		tkcmd("label "+vtk+".V"+me+" -text '"+val);
	tkcmd("pack "+vtk+".V"+me+" -side top -anchor w -in "+vtk+".F"+parent);

	d := ref Datum(me, parent, vtk, e, val, walk, nil);
	if(okids != nil){
		if(walk)
			return d.expand(okids, who);
		for(i := 0; i < len okids; i++)
			okids[i].destroy();
	}
	return d;
}

Datum.expand(d: self ref Datum, okids: array of ref Datum, who: string): ref Datum
{
	e := d.e.expand();
	if(e == nil)
		return d;

	vtk := d.vtk;

	me := d.tkid;

	# make the tk goo for holding kids
	needtk := okids == nil;
	if(needtk){
		tkcmd("frame "+vtk+".f"+me);
		tkcmd("frame "+vtk+".f"+me+".x -width "+WalkWidth);
		tkcmd("frame "+vtk+".f"+me+".v");
		tkcmd("pack "+vtk+".f"+me+".x "+vtk+".f"+me+".v -side left -fill y -expand 1");
		tkcmd("frame "+vtk+".F"+me);
	}

	kids := array[len e] of ref Datum;
	for(i := 0; i < len e; i++){
		if(i >= len okids)
			break;
		ok := okids[i];
		if(!sameexp(e[i], ok.e, 0))
			break;
		kids[i] = mkkid(e[i], vtk, me, ok.tkid, ok.kids, ok.val, ok.canwalk, who);
	}
	for(oi := i; oi < len okids; oi++)
		okids[oi].destroy();
	for(; i < len e; i++)
		kids[i] = mkkid(e[i], vtk, me, string tkids++, nil, nil, -1, who);

	tkcmd("pack "+vtk+".f"+me+" -side top -anchor w -after "+vtk+".v"+me);
	tkcmd("pack "+vtk+".F"+me+" -side top -anchor w -after "+vtk+".V"+me);
	tkcmd(vtk+".v"+me+".b configure -image Itemopen -command {send buts c"+who+"}");

	d.kids = kids;
	return d;
}

sameexp(e, f: ref Exp, offmatch: int): int
{
	if(e.m != f.m || e.p != f.p || e.name != f.name)
		return 0;
	return !offmatch || e.offset == f.offset;
}

tkcmd(cmd: string): string
{
	s := tk->cmd(tktop, cmd);
	if(len s != 0 && s[0] == '!')
		sys->print("%s '%s'\n", s, cmd);
	return s;
}
