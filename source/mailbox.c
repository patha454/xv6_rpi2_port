/*****************************************************************
*       mailbox.c
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/



#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"
#include "mailbox.h"

/* Note: for more tags refer to 
https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface */


void
create_request(volatile u_int32 *mbuf, u_int32 tag, u_int32 buflen, u_int32 len, u_int32 *data)
{
    int i;
    volatile u_int32 *tag_info;
    u_int32 nw, tag_len, total_len;

    tag_info = mbuf + POS_TAG;

    tag_info[POS_TAG_ID] = tag;
    tag_info[POS_TAG_BUFLEN] = buflen;
    tag_info[POS_TAG_DATALEN] = len & 0x7FFFFFFF;

    nw = buflen >> 2;

    if (!data)
        for (i = 0; i < nw; ++i) tag_info[POS_TAG_DATA + i] = 0;
    else
        for (i = 0; i < nw; ++i) tag_info[POS_TAG_DATA + i] = data[i];

    tag_info[POS_TAG_DATA+nw] = 0; // indicate kernel_bin_end of tag

    tag_len = mbuf[MB_HEADER_LENGTH + POS_TAG_BUFLEN];
    total_len = (MB_HEADER_LENGTH*4) + (TAG_HEADER_LENGTH*4) + tag_len + 4;

    mbuf[POS_OVERALL_LENGTH] = total_len;
    mbuf[POS_RV] = MPI_REQUEST;

}

volatile u_int32 *mail_buffer;

void mailboxinit()
{
mail_buffer = (u_int32 *)kalloc();
}

u_int32
readmailbox(u_char8 channel)
{
	u_int32 x, y, z;

again:
	while ((inw(MAILBOX_BASE+24) & 0x40000000) != 0);
	x = inw(MAILBOX_BASE);
	z = x & 0xf; y = (u_int32)(channel & 0xf);
	if(z != y) goto again;

	return x&0xfffffff0;
}

void
writemailbox(u_int32 *addr, u_char8 channel)
{
	u_int32 x, y, a;

	a = (u_int32)addr;
	a -= KERNBASE;   /* convert to ARM physical address */
	a += 0xc0000000; /* convert to VC address space */
	x = a & 0xfffffff0;
	y = x | (u_int32)(channel & 0xf);

	flush_dcache_all();

	while ((inw(MAILBOX_BASE+24) & 0x80000000) != 0);
	//while ((inw(MAILBOX_BASE+0x38) & 0x80000000) != 0);
	outw(MAILBOX_BASE+32, y);
}
