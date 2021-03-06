#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"

typedef struct Iphdr	Iphdr;
typedef struct Fragment	Fragment;
typedef struct Ipfrag	Ipfrag;

enum
{
	IPHDR		= 20,		/* sizeof(Iphdr) */
	IP_VER		= 0x40,		/* Using IP version 4 */
	IP_HLEN		= 0x05,		/* Header length in characters */
	IP_DF		= 0x4000,	/* Don't fragment */
	IP_MF		= 0x2000,	/* More fragments */
	IP_MAX		= (32*1024),	/* Maximum Internet packet size */
};

struct Iphdr
{
	byte	vihl;		/* Version and header length */
	byte	tos;		/* Type of service */
	byte	length[2];	/* packet length */
	byte	id[2];		/* Identification */
	byte	frag[2];	/* Fragment information */
	byte	ttl;		/* Time to live */
	byte	proto;		/* Protocol */
	byte	cksum[2];	/* Header checksum */
	byte	src[4];		/* Ip source */
	byte	dst[4];		/* Ip destination */
};

struct Fragment
{
	QLock;
	Block*	blist;
	Fragment* next;
	Ipaddr 	src;
	Ipaddr 	dst;
	ushort	id;
	ulong 	age;
};

struct Ipfrag
{
	ushort	foff;
	ushort	flen;
};

Fragment*	flisthead;
Fragment*	fragfree;
QLock		fraglock;
ulong		Id;
int		iprouting;	/* true if we route like a gateway */
ulong		ipcsumerr;
ulong		ipin, ippin;		/* bytes, packets in */
ulong		ipout, ippout;		/* bytes, packets out */

#define BLKIP(xp)	((Iphdr*)((xp)->rp))
/*
 * This sleazy macro relies on the media header size being
 * as large as sizeof(Ipfrag). ipreassemble checks this is true
 */
#define BKFG(xp)	((Ipfrag*)((xp)->base))

ushort		ipcsum(byte*);
Block*		ipreassemble(int, Block*, Iphdr*);
void		ipfragfree(Fragment*, int);
Fragment*	ipfragallo(void);

void
ipoput(Block *bp, int gating)
{
	Media *m;
	byte gate[4];
	ushort fragoff;
	Block *xp, *nb;
	Iphdr *eh, *feh;
	int lid, len, seglen, chunk, dlen, blklen, offset, medialen;

	/* Fill out the ip header */
	eh = (Iphdr *)(bp->rp);

	/* Number of bytes in data and ip header to write */
	len = blocklen(bp);
	ipout += len;
	ippout++;
	if(gating){
		chunk = nhgets(eh->length);
		if(chunk > len){
			netlog(Logip, "short gated packet\n");
			goto raise;
		}
		if(chunk < len)
			len = chunk;
	}
	if(len >= IP_MAX) {
		netlog(Logip, "exceeded ip max size %I\n", eh->dst);
		goto raise;
	}

	m = Mediaroute(eh->dst, gate);
	if(m == nil){
		netlog(Logip, "no interface %I\n", eh->dst);
		goto raise;
	}

	eh->vihl = IP_VER|IP_HLEN;
	eh->tos = 0;
	if(!gating)
		eh->ttl = 255;

	/* If we dont need to fragment just send it */
	medialen = m->maxmtu-m->hsize;
	if(len <= medialen) {
		if(!gating)
			hnputs(eh->id, Id++);
		hnputs(eh->length, len);
		eh->frag[0] = 0;
		eh->frag[1] = 0;
		eh->cksum[0] = 0;
		eh->cksum[1] = 0;
		hnputs(eh->cksum, ipcsum(&eh->vihl));

		Mediawrite(m, bp, gate);
		return;
	}

	if(eh->frag[0] & (IP_DF>>8))
		goto raise;

	seglen = (medialen - IPHDR) & ~7;
	if(seglen < 8)
		goto raise;

	dlen = len - IPHDR;
	xp = bp;
	if(gating)
		lid = nhgets(eh->id);
	else
		lid = Id++;

	offset = IPHDR;
	while(xp != nil && offset && offset >= BLEN(xp)) {
		offset -= BLEN(xp);
		xp = xp->next;
	}
	xp->rp += offset;

	for(fragoff = 0; fragoff < dlen; fragoff += seglen) {
		nb = allocb(IPHDR+seglen);
		feh = (Iphdr*)(nb->rp);

		memmove(nb->wp, eh, IPHDR);
		nb->wp += IPHDR;

		if((fragoff + seglen) >= dlen) {
			seglen = dlen - fragoff;
			hnputs(feh->frag, fragoff>>3);
		}
		else	
			hnputs(feh->frag, (fragoff>>3)|IP_MF);

		hnputs(feh->length, seglen + IPHDR);
		hnputs(feh->id, lid);

		/* Copy up the data area */
		chunk = seglen;
		while(chunk) {
			if(!xp) {
				freeblist(nb);
				goto raise;
			}
			blklen = chunk;
			if(BLEN(xp) < chunk)
				blklen = BLEN(xp);
			memmove(nb->wp, xp->rp, blklen);
			nb->wp += blklen;
			xp->rp += blklen;
			chunk -= blklen;
			if(xp->rp == xp->wp)
				xp = xp->next;
		} 

		feh->cksum[0] = 0;
		feh->cksum[1] = 0;
		hnputs(feh->cksum, ipcsum(&feh->vihl));
		Mediawrite(m, nb, gate);
	}

raise:
	freeblist(bp);	
}

void
initfrag(int size)
{
	Fragment *fq, *eq;

	fragfree = (Fragment*)malloc(sizeof(Fragment) * size);
	if(fragfree == nil)
		panic("initfrag");

	eq = &fragfree[size];
	for(fq = fragfree; fq < eq; fq++)
		fq->next = fq+1;

	fragfree[size-1].next = nil;
}

void
ipiput(Block *bp)
{
	Iphdr *h;
	Proto *p;
	ushort frag;
	int notforme;

	h = (Iphdr *)(bp->rp);
	netlog(Logip, "ipiput %I %I len %d proto %d\n", h->src, h->dst, BLEN(bp), h->proto);

	/* Ensure we have enough data to process */
	if(BLEN(bp) < IPHDR) {
		bp = pullupblock(bp, IPHDR);
		if(bp == nil)
			return;
	}
	h = (Iphdr *)(bp->rp);

	/* Look to see if its for me before we waste time checksuming it */
	notforme = Mediaforme(h->dst) == 0;
	if(notforme && !iprouting) {
		netlog(Logip, "ip: pkt not for me\n");
		freeblist(bp);
		return;
	}

	if(ipcsum(&h->vihl)) {
		ipcsumerr++;
		netlog(Logip, "ip: checksum error %I\n", h->src);
		freeblist(bp);
		return;
	}

	/* Check header length and version */
	if(h->vihl != (IP_VER|IP_HLEN)) {
		netlog(Logip, "ip: %I bad hivl %ux\n", h->src, h->vihl);
		freeblist(bp);
		return;
	}
	frag = nhgets(h->frag);
	if(frag) {
		h->tos = 0;
		if(frag & IP_MF)
			h->tos = 1;
		bp = ipreassemble(frag, bp, h);
		if(bp == nil)
			return;
	}

	ipin += blocklen(bp);
	ippin++;

	if(iprouting) {
		/* gate */
		if(notforme){
			if(h->ttl == 0)
				freeblist(bp);
			else {
				h->ttl--;
				ipoput(bp, 1);
			}
			return;
		}
	}

	p = Fsrcvpcol(h->proto);
	if(p != nil) {
		(*p->rcv)(bp);
		return;
	}
	freeblist(bp);
}

int
ipstats(char *buf, int len)
{
	int n;

	n = snprint(buf, len, "ip: csum %d inb %d outb %d inp %d outp %d\n",
		ipcsumerr, ipin, ipout, ippin, ippout);
	return n;
}

QLock iplock;

Block*
ipreassemble(int offset, Block *bp, Iphdr *ip)
{
	int fend;
	ushort id;
	Fragment *f;
	Ipaddr src, dst;
	Block *bl, **l, *last, *prev;
	int ovlap, len, fragsize, pktposn;

	src = nhgetl(ip->src);
	dst = nhgetl(ip->dst);
	id = nhgets(ip->id);

	/*
	 *  block lists are too hard, pullupblock into a single block
	 */
	if(bp->next){
		bp = pullupblock(bp, blocklen(bp));
		ip = (Iphdr *)(bp->rp);
	}

	qlock(&iplock);

	/*
	 *  find a reassembly queue for this fragment
	 */
	qlock(&fraglock);
	for(f = flisthead; f; f = f->next)
		if(f->src == src && f->dst == dst && f->id == id)
			break;
	qunlock(&fraglock);

	/*
	 *  if this isn't a fragmented packet, accept it
	 *  and get rid of any fragments that might go
	 *  with it.
	 */
	if(!ip->tos && (offset & ~(IP_MF|IP_DF)) == 0) {
		if(f != nil) {
			qlock(f);
			ipfragfree(f, 1);
		}
		qunlock(&iplock);
		return bp;
	}

	if(bp->base+sizeof(Ipfrag) > bp->rp)
		bp = padblock(bp, sizeof(Ipfrag));

	BKFG(bp)->foff = offset<<3;
	BKFG(bp)->flen = nhgets(ip->length)-IPHDR;

	/* First fragment allocates a reassembly queue */
	if(f == nil) {
		f = ipfragallo();
		qlock(f);
		f->id = id;
		f->src = src;
		f->dst = dst;

		f->blist = bp;

		qunlock(f);
		qunlock(&iplock);
		return nil;
	}
	qlock(f);

	/*
	 *  find the new fragment's position in the queue
	 */
	prev = nil;
	l = &f->blist;
	bl = f->blist;
	while(bl != nil && BKFG(bp)->foff > BKFG(bl)->foff) {
		prev = bl;
		l = &bl->next;
		bl = bl->next;
	}

	/* Check overlap of a previous fragment - trim away as necessary */
	if(prev) {
		ovlap = BKFG(prev)->foff + BKFG(prev)->flen - BKFG(bp)->foff;
		if(ovlap > 0) {
			if(ovlap >= BKFG(bp)->flen) {
				freeblist(bp);
				qunlock(f);
				qunlock(&iplock);
				return nil;
			}
			BKFG(prev)->flen -= ovlap;
		}
	}

	/* Link onto assembly queue */
	bp->next = *l;
	*l = bp;

	/* Check to see if succeeding segments overlap */
	if(bp->next) {
		l = &bp->next;
		fend = BKFG(bp)->foff + BKFG(bp)->flen;
		/* Take completely covered segements out */
		while(*l) {
			ovlap = fend - BKFG(*l)->foff;
			if(ovlap <= 0)
				break;
			if(ovlap < BKFG(*l)->flen) {
				BKFG(*l)->flen -= ovlap;
				BKFG(*l)->foff += ovlap;
				/* move up ip hdrs */
				memmove((*l)->rp + ovlap, (*l)->rp, IPHDR);
				(*l)->rp += ovlap;
				break;
			}
			last = (*l)->next;
			(*l)->next = nil;
			freeblist(*l);
			*l = last;
		}
	}

	/*
	 *  look for a complete packet.  if we get to a fragment
	 *  without IP_MF set, we're done.
	 */
	pktposn = 0;
	for(bl = f->blist; bl; bl = bl->next) {
		if(BKFG(bl)->foff != pktposn)
			break;
		if((BLKIP(bl)->frag[0]&(IP_MF>>8)) == 0) {
			bl = f->blist;
			len = nhgets(BLKIP(bl)->length);
			bl->wp = bl->rp + len;

			/* Pullup all the fragment headers and
			 * return a complete packet
			 */
			for(bl = bl->next; bl; bl = bl->next) {
				fragsize = BKFG(bl)->flen;
				len += fragsize;
				bl->rp += IPHDR;
				bl->wp = bl->rp + fragsize;
			}

			bl = f->blist;
			f->blist = nil;
			ipfragfree(f, 1);
			ip = BLKIP(bl);
			hnputs(ip->length, len);
			qunlock(&iplock);
			return bl;		
		}
		pktposn += BKFG(bl)->flen;
	}
	qunlock(f);
	qunlock(&iplock);
	return nil;
}

/*
 * ipfragfree - Free a list of fragments, fragment list must be locked
 */
void
ipfragfree(Fragment *frag, int lockq)
{
	Fragment *fl, **l;

	if(frag->blist)
		freeblist(frag->blist);

	frag->src = 0;
	frag->id = 0;
	frag->blist = nil;
	qunlock(frag);

	if(lockq)
		qlock(&fraglock);

	l = &flisthead;
	for(fl = *l; fl; fl = fl->next) {
		if(fl == frag) {
			*l = frag->next;
			break;
		}
		l = &fl->next;
	}

	frag->next = fragfree;
	fragfree = frag;

	if(lockq)
		qunlock(&fraglock);
}

/*
 * ipfragallo - allocate a reassembly queue
 */
Fragment *
ipfragallo(void)
{
	Fragment *f;

	qlock(&fraglock);
	while(fragfree == nil) {
		for(f = flisthead; f; f = f->next)
			if(canqlock(f)) {
				ipfragfree(f, 0);
				break;
			}
	}
	f = fragfree;
	fragfree = f->next;
	f->next = flisthead;
	flisthead = f;
	f->age = msec + 30000;

	qunlock(&fraglock);
	return f;
}

ushort
ipcsum(byte *addr)
{
	int len;
	ulong sum;

	sum = 0;
	len = (addr[0]&0xf)<<2;

	while(len > 0) {
		sum += addr[0]<<8 | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return (sum^0xffff);
}
