!include ../../mkconfig.nmk
!include ../../$(SYSHOST)/mkhost

OBJ=\
	bind.dis\
	cat.dis\
	cd.dis\
	changelogin.dis\
#	changepass.dis\
	chmod.dis\
	cmp.dis\
	cp.dis\
	date.dis\
	diff.dis\
	du.dis\
	echo.dis\
	getauthinfo.dis\
	grep.dis\
#	init.dis\
	irtest.dis\
	kill.dis\
	ls.dis\
	manufacture.dis\
	mathcalc.dis\
	mkdir.dis\
	mount.dis\
	mv.dis\
	netstat.dis\
	nsbuild.dis\
	os.dis\
	p.dis\
	ps.dis\
	pwd.dis\
	rcmd.dis\
	rm.dis\
	sh.dis\
	sleep.dis\
	stack.dis\
	tclsh.dis\
	time.dis\
	unicode.dis\
	unmount.dis\
	wc.dis\
	wish.dis\

all:	$(OBJ)

MODULE= ..\..\module\sys.m\
	..\..\module\draw.m\
	..\..\module\lib.m\
	..\..\module\prefab.m\
	..\..\module\ir.m\
	..\..\module\srv.m\
	..\..\module\tk.m\

$(OBJ):	$(MODULE)

install: $(OBJ)
	copy *.dis ..\..\dis

uninstall:
	@-for %%j in ( $(OBJ) ) \
	do $(DEL) $(DELFLAGS) ..\..\dis\%%j

package:	$(OBJ)\
		$(PREFIX_PKG)\dis
	copy $(OBJ) $(PREFIX_PKG)\dis

clean nuke:
	-$(DEL) $(DELFLAGS) *.dis 
	-$(DEL) $(DELFLAGS) *.sbl

.SUFFIXES:	.b .dis .s

.b.dis:	
	limbo -I../../module -gw $*.b

.b.s:	
	limbo -w -G -S -I../../module $*.b

$(PREFIX_PKG)\dis:
	mkdir $(PREFIX_PKG)\dis
