void	addinclude(char*);
int	addrconv(va_list*, Fconv*);
Decl	*adtdecl(Type*);
Decl	*adtmeths(Type*);
void	adtstub(Decl*);
long	align(long, int);
void	*allocmem(ulong);
void	altcom(Node*);
Inst	*andand(Node*, int, Inst*);
Decl	*appdecls(Decl*, Decl*);
int	argcompat(Node*, Decl*, Node*);
void	arraycom(Node*, Node*);
void	arraydefault(Node*, Node*);
Type	*arrowtype(Type*);
void	asmdesc(Desc*);
void	asmentry(Decl*);
void	asminitializer(long, Node*);
void	asminst(Inst*);
void	asmmod(Decl*);
void	asmstring(long, Sym*);
void	asmvar(long, Decl*);
int	assignindices(Node*);
int	atcompat(Decl*, Decl*, int);
void	bccom(Node*, Inst**);
Inst	*bcom(Node*, int, Inst*);
void	bindnames(Node*);
Ok	callcast(Node*, int, int);
void	callcom(Src*, int, Node*, Node*);
int	casecheck(Node*, Node*, Node*);
void	casecom(Node*);
Node	*caselist(Node*, Node*);
int	checkadt(Type*, Type*);
int	checkarc(Type*, Type*);
void	checkrefs(Decl*);
Node	*checkused(Node*);
int	circlval(Node*, Node*);
int	conchk(Node*, int, int);
Node	*condecl(Decl*, Node*);
void	constub(Decl*);
Type	*copytype(Type*);
Type	*copytypeids(Type*);
char	*ctprint(char*, char*, Type*);
int	ctypeconv(va_list*, Fconv*);
Line	curline(void);
int	declare(int, Decl*);
void	declas(Node*);
void	declaserr(Node*);
int	declasinfer(Node*, Type*);
int	declconv(va_list*, Fconv*);
Decl	*declsort(Decl*);
void	declstart(void);
void	deldecl(Decl*);
uchar	*descmap(Decl*, uchar*, long);
void	disaddr(Addr*);
void	discon(long);
void	disdata(int, long);
void	disdesc(Desc*);
void	disentry(Decl*);
void	disinst(Inst*);
void	dismod(Decl*);
void	disvar(long, Decl*);
void	disword(long);
int	dotconv(va_list*, Fconv*);
char	*dotprint(char*, char*, Decl*, int);
void	dtocanon(double, ulong[]);
Decl	*dupdecl(Decl*);
Node	*dupn(Src*, Node*);
Node	*eacom(Node*, Node*, Node*);
Ok	echeck(Node*, int);
Node	*ecom(Src*, Node*, Node*);
Node	*elemsort(Node*);
void	emit(Decl*);
Sym	*enter(char*, int);
Desc	*enterdesc(uchar*, long, long);
Sym	*enterstring(char*, int);
char	*eprint(char*, char*, Node*);
char	*eprintlist(char*, char*, Node*, char*);
void	error(Line, char*, ...);
Node	*etolist(Node*);
int	expconv(va_list*, Fconv*);
void	fatal(char*, ...);
int	findadts(Decl *, Decl***);
void	findfns(Node*);
Fline	fline(int);
void	fmtcheck(Node*, Node*, Node*);
void	fncom(Decl*);
void	fndef(Decl*, Type*);
Node	*fnfinishdef(Decl*, Node*);
Node	*fold(Node*);
void	foldbranch(Inst*);
Node	*foldc(Node*);
Node	*foldcast(Node*, Node*);
Node	*foldcasti(Node*, Node*);
Node	*foldr(Node*);
Node	*foldvc(Node*);
Addr	genaddr(Node*);
Inst	*genbra(Src*, int, Node*, Node*);
Inst	*genchan(Src*, Type*, Node*);
Desc	*gendesc(Decl*, long, Decl*);
Inst	*genmove(Src*, int, Type*, Node*, Node*);
Inst	*genop(Src*, int, Node*, Node*, Node*);
Inst	*genrawop(Src*, int, Node*, Node*, Node*);
Addr	genreg(Node*);
void	genstart(void);
int	gfltconv(va_list*, Fconv*);
Decl	*globalBconst(Node*);
Node	*globalas(Node*, Node*, int);
Decl	*globalbconst(Node*);
Decl	*globalconst(Node*);
Decl	*globalfconst(Node*);
void	globalinit(Node*, int);
Decl	*globalsconst(Node*);
int	hasside(Node*);
int	idcompat(Decl*, Decl*, int);
long	idoffsets(Decl*, long, int);
Type	*idtype(Type*);
Node	*import(Node*, Decl*);
void	importchk(Node*);
void	includef(Sym*);
Node	*indsascom(Src*, Node*, Node*);
int	initable(Node*, Node*, int);
int	install(Sym*, Decl*);
int	installglobal(Sym*, Decl*);
Decl	*installids(int, Decl*);
int	instconv(va_list*, Fconv*);
int	islval(Node*);
void	joiniface(Decl*, Type*);
void	lexinit(void);
void	lexstart(char*);
int	lineconv(va_list*, Fconv*);
Decl	*lookdot(Decl*, Sym*);
int	mapconv(va_list*, Fconv*);
int	marklval(Node*);
int	mathchk(Node*, int);
Type	*mkadtcon(Type*);
Type	*mkarrowtype(Line*, Line*, Type*, Sym*);
Node	*mkbin(int, Node*, Node*);
Node	*mkconst(Src*, Long);
Decl	*mkdecl(Src*, int, Type*);
Node	*mkdeclname(Src*, Decl*);
Desc	*mkdesc(long, Decl*, long);
Node	*mkfield(Src*, Sym*);
Decl	*mkids(Src*, Sym*, Type*, Decl*);
Type	*mkidtype(Src*, Sym*);
Type	*mkiface(Decl*);
Inst	*mkinst(Src*);
Node	*mkname(Src*, Sym*);
Node	*mknil(Src*);
Node	*mkn(int, Node*, Node*);
Node	*mkrconst(Src*, Real);
Node	*mksconst(Src*, Sym*);
Node	*mkscope(Decl*, Node*);
Type	*mktalt(Case*);
Desc	*mktdesc(Type*);
Node	*mktn(Type*);
Type	*mktype(Line*, Line*, int, Type*, Decl*);
Node	*mkunary(int, Node*);
Type	*mkvarargs(Node*, Node*);
Teq	*modclass(void);
void	modcode(Decl*);
void	modcom(void);
void	moddataref(void);
void	moddecl(Decl*, Decl*);
Decl	*modglobals(Decl*, Decl*);
void	modrefable(Type*);
void	modstub(Decl*);
void	modtab(Decl*);
int	mpatof(char*, double*);
int	nameconv(va_list*, Fconv*);
Decl	*namedot(Decl*, Sym*);
Decl	*namesort(Decl*);
void	narrowmods(void);
void	nerror(Node*, char*, ...);
Inst	*nextinst(void);
int	nodeconv(va_list*, Fconv*);
char	*nprint(char*, char*, Node*, int);
int	ntconv(va_list*, Fconv*);
void	nwarn(Node*, char*, ...);
int	opconv(va_list*, Fconv*);
void	optabinit(void);
Inst	*oror(Node*, int, Inst*);
Node	*passimplicit(Node*, Node*);
void	patch(Inst*, Inst*);
void	popblock(void);
Decl	*popglscope(void);
void	popimports(Node*);
void	popscopes(void);
Decl	*popscope(void);
void	printdecls(Decl*);
int	pushblock(void);
void	pushglscope(void);
void	pushscope(void);
void	reach(Inst*);
void	*reallocmem(void*, ulong);
void	recvacom(Src*, Node*, Node*);
void	reftype(Type*);
void	repushblock(int);
long	resolvedesc(Decl*, long, Decl*);
int	resolvemod(Decl*);
long	resolvepcs(Inst*);
Node	*retalloc(Node*, Node*);
Decl	*revids(Decl*);
Inst	*rewritedestreg(Inst*, int, int);
Inst	*rewritesrcreg(Inst*, int, int, int);
Node	*rotater(Node*);
int	sameaddr(Node*, Node*);
void	sbladt(Decl**, int);
void	sblfiles(void);
void	sblfn(Decl**, int);
void	sblinst(Inst*, long);
void	sblmod(Decl*);
void	sblvar(Decl*);
Node*	scheck(Node*, Type*);
void	scom(Node*);
char	*secpy(char*, char*, char*);
char	*seprint(char*, char*, char*, ...);
int	shiftchk(Node*);
ulong	sign(Decl*);
long	sizetype(Type*);
Node	*slicelcom(Src*, Node*, Node*);
Type	*snaptypes(Type*);
int	specific(Type*);
int	srcconv(va_list*, Fconv*);
int	storeconv(va_list*, Fconv*);
char	*stprint(char*, char*, Type*);
Sym	*stringcat(Sym*, Sym*);
char	*stringprint(char*, char*, Sym*);
Long	strtoi(char*, int);
int	stypeconv(va_list*, Fconv*);
Node	*sumark(Node*);
int	symcmp(Sym*, Sym*);
Node	*tacquire(Node*);
Node	*talloc(Node*, Type*, Node*);
int	tcompat(Type*, Type*, int);
Decl	*tdecls(void);
uchar	*tdescmap(Type*, uchar*, long);
void	teqclass(Type*);
int	tequal(Type*, Type*);
void	tfree(Node*);
void	tinit(void);
char	*tprint(char*, char*, Type*);
void	translate(char*, char*, char*);
int	tupaliased(Node*, Node*);
void	tupcom(Node*, Node*);
void	tuplcom(Node*, Node*);
Decl	*tuplefields(Node*);
int	typeconv(va_list*, Fconv*);
void	typedecl(Decl*, Type*);
Decl	*typeids(Decl*, Type*);
void	typestart(void);
Desc	*usedesc(Desc*);
Type	*usetype(Type*);
int	valistype(Node*);
int	varchk(Node*);
int	varcom(Decl*);
Node	*vardecl(Decl*, Type*);
Node	*varinit(Decl*, Node*);
Decl	*varlistdecl(Decl*, Node*);
Decl	*vars(Decl*);
int	vcom(Decl*);
Type	*verifytypes(Type*, Decl*);
void	warn(Line, char*, ...);
void	yyerror(char*, ...);
int	yylex(void);
int	yyparse(void);
