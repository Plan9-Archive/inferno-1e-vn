#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

/*
 *  extended 82C37 in 82378IB PCI/ISA bridge
 */
typedef struct DMAport	DMAport;
typedef struct DMA	DMA;
typedef struct DMAxfer	DMAxfer;

enum
{
	/*
	 *  the byte registers for DMA0 are all one byte apart
	 */
	Dma0=		0x00,
	Dma0status=	Dma0+0x8,	/* status port */
	Dma0reset=	Dma0+0xD,	/* reset port */

	/*
	 *  the byte registers for DMA1 are all two bytes apart (why?)
	 */
	Dma1=		0xC0,
	Dma1status=	Dma1+2*0x8,	/* status port */
	Dma1reset=	Dma1+2*0xD,	/* reset port */
};

/*
 *  state of a dma transfer
 */
struct DMAxfer
{
	ulong	bpa;		/* bounce buffer physical address */
	void*	bva;		/* bounce buffer virtual address */
	void	*va;		/* virtual address destination/src */
	long	len;		/* bytes to be transferred */
	int	isread;
};

/*
 *  the dma controllers.  the first half of this structure specifies
 *  the I/O ports used by the DMA controllers.
 */
struct DMAport
{
	uchar	addr[4];	/* current address (4 channels) */
	uchar	count[4];	/* current count (4 channels) */
	uchar	page[4];	/* page registers (4 channels) */
	ushort	hpage[4];	/* high byte of address (4 channels) */
	uchar	cmd;		/* command status register */
	uchar	req;		/* request registers */
	uchar	sbm;		/* single bit mask register */
	uchar	mode;		/* mode register */
	uchar	cbp;		/* clear byte pointer */
	uchar	mc;		/* master clear */
	uchar	cmask;		/* clear mask register */
	uchar	wam;		/* write all mask register bit */
	ushort	emode;	/* extended mode register */
};

struct DMA
{
	DMAport;
	int	shift;
	Lock;
	DMAxfer	x[4];
};

DMA dma[2] = {
	{ 0x00, 0x02, 0x04, 0x06,
	  0x01, 0x03, 0x05, 0x07,
	  0x87, 0x83, 0x81, 0x82,
	  0x487, 0x483, 0x481, 0x482,
	  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x40b,
	 0 },

	{ 0xc0, 0xc4, 0xc8, 0xcc,
	  0xc2, 0xc6, 0xca, 0xce,
	  0x8f, 0x8b, 0x89, 0x8a,
	  0x48f, 0x48b, 0x489, 0x48a,
	  0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0x4d6,
	 1 },
};

/*
 *  allocate buffers for DMA to addresses outside the kernel.
 */
void
dmainit(void)
{
	int i, chan;
	DMA *dp;
	DMAxfer *xp;

	for(i = 0; i < 2; i++){
		dp = &dma[i];
		for(chan = 0; chan < 4; chan++){
			xp = &dp->x[chan];
			xp->bva = xspanalloc(BY2PG, BY2PG, 0);
			xp->bpa = PADDR(xp->bva);
			xp->len = 0;
			xp->isread = 0;
		}
	}
}

/*
 *  setup a dma transfer.  if the destination is not in kernel
 *  memory, allocate a page for the transfer.
 *
 *  we assume Be's boot rom has set up the command register before we
 *  are booted.
 *
 *  return the updated transfer length (we can't transfer across 64k
 *  boundaries)
 */
long
dmasetup(int chan, void *va, long len, int isread)
{
	DMA *dp;
	DMAxfer *xp;
	ulong pa;
	uchar mode;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;
	xp = &dp->x[chan];

	if(len > 0x10000)
		len = 0x10000;

	/*
	 *  if this isn't kernel memory use the allocated buffer page.
	 */
	pa = PADDR(va);
	if((((ulong)va)&KSEGM) != KZERO){
		if(len > BY2PG)
			len = BY2PG;
		if(!isread)
			memmove((void*)(xp->bva), va, len);
		xp->va = va;
		xp->len = len;
		xp->isread = isread;
		pa = xp->bpa;
	} else
		xp->len = 0;
	pa = DMASEG(pa);
	/*
	 * this setup must be atomic
	 */
	ilock(dp);
	outb(dp->sbm, 4|chan);
	mode = (isread ? 0x44 : 0x48) | chan;
	outb(dp->mode, mode);		/* single mode dma (give CPU a chance at mem) */
	if(dp->shift)
		outb(dp->emode, (3<<2)|chan);	/* count by bytes */
	outb(dp->cbp, 0);		/* set count & address to their first byte */
	outb(dp->addr[chan], pa);		/* set address */
	outb(dp->addr[chan], pa>>8);
	outb(dp->page[chan], pa>>16);
	outb(dp->hpage[chan], pa>>24);
	outb(dp->count[chan], len-1);		/* set count */
	outb(dp->count[chan], (len-1)>>8);
	outb(dp->sbm, chan);		/* enable the channel */
	iunlock(dp);

	return len;
}

int
dmadone(int chan)
{
	DMA *dp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	return inb(dp->cmd) & (1<<chan);
}

/*
 *  this must be called after a dma has been completed.
 *
 *  if a page has been allocated for the dma,
 *  copy the data into the actual destination
 *  and free the page.
 */
void
dmaend(int chan)
{
	DMA *dp;
	DMAxfer *xp;

	dp = &dma[(chan>>2)&1];
	chan = chan & 3;

	/*
	 *  disable the channel
	 */
	ilock(dp);
	outb(dp->sbm, 4|chan);
	iunlock(dp);

	xp = &dp->x[chan];
	if(xp->len == 0 || !xp->isread)
		return;

	/*
	 *  copy out of temporary page
	 */
	memmove(xp->va, (void*)(xp->bva), xp->len);
	xp->len = 0;
}
 
int
dmacount(int chan)
{
	int     retval;
	DMA     *dp;
 
	dp = &dma[(chan>>2)&1];
	outb(dp->cbp, 0);
	retval = inb(dp->count[chan]);
	retval |= inb(dp->count[chan]) << 8;
	return((retval<<dp->shift)+1);
}
