void	atareset(void);
void	atainit(void);
Chan*	ataattach(char*);
Chan*	ataclone(Chan*, Chan*);
int	atawalk(Chan*, char*);
void	atastat(Chan*, char*);
Chan*	ataopen(Chan*, int);
void	atacreate(Chan*, char*, int, ulong);
void	ataclose(Chan*);
long	ataread(Chan*, void*, long, ulong);
Block*	atabread(Chan*, long, ulong);
long	atawrite(Chan*, void*, long, ulong);
long	atabwrite(Chan*, Block*, ulong);
void	ataremove(Chan*);
void	atawstat(Chan*, char*);
void	audioreset(void);
void	audioinit(void);
Chan*	audioattach(char*);
Chan*	audioclone(Chan*, Chan*);
int	audiowalk(Chan*, char*);
void	audiostat(Chan*, char*);
Chan*	audioopen(Chan*, int);
void	audiocreate(Chan*, char*, int, ulong);
void	audioclose(Chan*);
long	audioread(Chan*, void*, long, ulong);
Block*	audiobread(Chan*, long, ulong);
long	audiowrite(Chan*, void*, long, ulong);
long	audiobwrite(Chan*, Block*, ulong);
void	audioremove(Chan*);
void	audiowstat(Chan*, char*);
void	consreset(void);
void	consinit(void);
Chan*	consattach(char*);
Chan*	consclone(Chan*, Chan*);
int	conswalk(Chan*, char*);
void	consstat(Chan*, char*);
Chan*	consopen(Chan*, int);
void	conscreate(Chan*, char*, int, ulong);
void	consclose(Chan*);
long	consread(Chan*, void*, long, ulong);
Block*	consbread(Chan*, long, ulong);
long	conswrite(Chan*, void*, long, ulong);
long	consbwrite(Chan*, Block*, ulong);
void	consremove(Chan*);
void	conswstat(Chan*, char*);
void	drawreset(void);
void	drawinit(void);
Chan*	drawattach(char*);
Chan*	drawclone(Chan*, Chan*);
int	drawwalk(Chan*, char*);
void	drawstat(Chan*, char*);
Chan*	drawopen(Chan*, int);
void	drawcreate(Chan*, char*, int, ulong);
void	drawclose(Chan*);
long	drawread(Chan*, void*, long, ulong);
Block*	drawbread(Chan*, long, ulong);
long	drawwrite(Chan*, void*, long, ulong);
long	drawbwrite(Chan*, Block*, ulong);
void	drawremove(Chan*);
void	drawwstat(Chan*, char*);
void	etherreset(void);
void	etherinit(void);
Chan*	etherattach(char*);
Chan*	etherclone(Chan*, Chan*);
int	etherwalk(Chan*, char*);
void	etherstat(Chan*, char*);
Chan*	etheropen(Chan*, int);
void	ethercreate(Chan*, char*, int, ulong);
void	etherclose(Chan*);
long	etherread(Chan*, void*, long, ulong);
Block*	etherbread(Chan*, long, ulong);
long	etherwrite(Chan*, void*, long, ulong);
long	etherbwrite(Chan*, Block*, ulong);
void	etherremove(Chan*);
void	etherwstat(Chan*, char*);
void	ipreset(void);
void	ipinit(void);
Chan*	ipattach(char*);
Chan*	ipclone(Chan*, Chan*);
int	ipwalk(Chan*, char*);
void	ipstat(Chan*, char*);
Chan*	ipopen(Chan*, int);
void	ipcreate(Chan*, char*, int, ulong);
void	ipclose(Chan*);
long	ipread(Chan*, void*, long, ulong);
Block*	ipbread(Chan*, long, ulong);
long	ipwrite(Chan*, void*, long, ulong);
long	ipbwrite(Chan*, Block*, ulong);
void	ipremove(Chan*);
void	ipwstat(Chan*, char*);
void	mntreset(void);
void	mntinit(void);
Chan*	mntattach(char*);
Chan*	mntclone(Chan*, Chan*);
int	mntwalk(Chan*, char*);
void	mntstat(Chan*, char*);
Chan*	mntopen(Chan*, int);
void	mntcreate(Chan*, char*, int, ulong);
void	mntclose(Chan*);
long	mntread(Chan*, void*, long, ulong);
Block*	mntbread(Chan*, long, ulong);
long	mntwrite(Chan*, void*, long, ulong);
long	mntbwrite(Chan*, Block*, ulong);
void	mntremove(Chan*);
void	mntwstat(Chan*, char*);
void	mpegreset(void);
void	mpeginit(void);
Chan*	mpegattach(char*);
Chan*	mpegclone(Chan*, Chan*);
int	mpegwalk(Chan*, char*);
void	mpegstat(Chan*, char*);
Chan*	mpegopen(Chan*, int);
void	mpegcreate(Chan*, char*, int, ulong);
void	mpegclose(Chan*);
long	mpegread(Chan*, void*, long, ulong);
Block*	mpegbread(Chan*, long, ulong);
long	mpegwrite(Chan*, void*, long, ulong);
long	mpegbwrite(Chan*, Block*, ulong);
void	mpegremove(Chan*);
void	mpegwstat(Chan*, char*);
void	ns16552reset(void);
void	ns16552init(void);
Chan*	ns16552attach(char*);
Chan*	ns16552clone(Chan*, Chan*);
int	ns16552walk(Chan*, char*);
void	ns16552stat(Chan*, char*);
Chan*	ns16552open(Chan*, int);
void	ns16552create(Chan*, char*, int, ulong);
void	ns16552close(Chan*);
long	ns16552read(Chan*, void*, long, ulong);
Block*	ns16552bread(Chan*, long, ulong);
long	ns16552write(Chan*, void*, long, ulong);
long	ns16552bwrite(Chan*, Block*, ulong);
void	ns16552remove(Chan*);
void	ns16552wstat(Chan*, char*);
void	progreset(void);
void	proginit(void);
Chan*	progattach(char*);
Chan*	progclone(Chan*, Chan*);
int	progwalk(Chan*, char*);
void	progstat(Chan*, char*);
Chan*	progopen(Chan*, int);
void	progcreate(Chan*, char*, int, ulong);
void	progclose(Chan*);
long	progread(Chan*, void*, long, ulong);
Block*	progbread(Chan*, long, ulong);
long	progwrite(Chan*, void*, long, ulong);
long	progbwrite(Chan*, Block*, ulong);
void	progremove(Chan*);
void	progwstat(Chan*, char*);
void	rootreset(void);
void	rootinit(void);
Chan*	rootattach(char*);
Chan*	rootclone(Chan*, Chan*);
int	rootwalk(Chan*, char*);
void	rootstat(Chan*, char*);
Chan*	rootopen(Chan*, int);
void	rootcreate(Chan*, char*, int, ulong);
void	rootclose(Chan*);
long	rootread(Chan*, void*, long, ulong);
Block*	rootbread(Chan*, long, ulong);
long	rootwrite(Chan*, void*, long, ulong);
long	rootbwrite(Chan*, Block*, ulong);
void	rootremove(Chan*);
void	rootwstat(Chan*, char*);
void	rtcreset(void);
void	rtcinit(void);
Chan*	rtcattach(char*);
Chan*	rtcclone(Chan*, Chan*);
int	rtcwalk(Chan*, char*);
void	rtcstat(Chan*, char*);
Chan*	rtcopen(Chan*, int);
void	rtccreate(Chan*, char*, int, ulong);
void	rtcclose(Chan*);
long	rtcread(Chan*, void*, long, ulong);
Block*	rtcbread(Chan*, long, ulong);
long	rtcwrite(Chan*, void*, long, ulong);
long	rtcbwrite(Chan*, Block*, ulong);
void	rtcremove(Chan*);
void	rtcwstat(Chan*, char*);
void	srvreset(void);
void	srvinit(void);
Chan*	srvattach(char*);
Chan*	srvclone(Chan*, Chan*);
int	srvwalk(Chan*, char*);
void	srvstat(Chan*, char*);
Chan*	srvopen(Chan*, int);
void	srvcreate(Chan*, char*, int, ulong);
void	srvclose(Chan*);
long	srvread(Chan*, void*, long, ulong);
Block*	srvbread(Chan*, long, ulong);
long	srvwrite(Chan*, void*, long, ulong);
long	srvbwrite(Chan*, Block*, ulong);
void	srvremove(Chan*);
void	srvwstat(Chan*, char*);
void	sslreset(void);
void	sslinit(void);
Chan*	sslattach(char*);
Chan*	sslclone(Chan*, Chan*);
int	sslwalk(Chan*, char*);
void	sslstat(Chan*, char*);
Chan*	sslopen(Chan*, int);
void	sslcreate(Chan*, char*, int, ulong);
void	sslclose(Chan*);
long	sslread(Chan*, void*, long, ulong);
Block*	sslbread(Chan*, long, ulong);
long	sslwrite(Chan*, void*, long, ulong);
long	sslbwrite(Chan*, Block*, ulong);
void	sslremove(Chan*);
void	sslwstat(Chan*, char*);
void	tinyfsreset(void);
void	tinyfsinit(void);
Chan*	tinyfsattach(char*);
Chan*	tinyfsclone(Chan*, Chan*);
int	tinyfswalk(Chan*, char*);
void	tinyfsstat(Chan*, char*);
Chan*	tinyfsopen(Chan*, int);
void	tinyfscreate(Chan*, char*, int, ulong);
void	tinyfsclose(Chan*);
long	tinyfsread(Chan*, void*, long, ulong);
Block*	tinyfsbread(Chan*, long, ulong);
long	tinyfswrite(Chan*, void*, long, ulong);
long	tinyfsbwrite(Chan*, Block*, ulong);
void	tinyfsremove(Chan*);
void	tinyfswstat(Chan*, char*);
void	tvreset(void);
void	tvinit(void);
Chan*	tvattach(char*);
Chan*	tvclone(Chan*, Chan*);
int	tvwalk(Chan*, char*);
void	tvstat(Chan*, char*);
Chan*	tvopen(Chan*, int);
void	tvcreate(Chan*, char*, int, ulong);
void	tvclose(Chan*);
long	tvread(Chan*, void*, long, ulong);
Block*	tvbread(Chan*, long, ulong);
long	tvwrite(Chan*, void*, long, ulong);
long	tvbwrite(Chan*, Block*, ulong);
void	tvremove(Chan*);
void	tvwstat(Chan*, char*);
