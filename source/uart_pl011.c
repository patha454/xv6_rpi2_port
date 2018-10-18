/*
 * pl011.c
 *
 *  Created on: Nov 20, 2016
 *      Author: Mahdi Amiri
 */
#include <fvp.h>
#include <types.h>
#include <defs.h>

u_int32 uart_lock; //Mutex lock

void uartinit_fvp(){
	/* Enable pl011 interrupts */
	*(volatile u_int32*) FVP_PL011_UARTIMSC = FVP_PL011_UARTIMSC_RXIM;

	/* Enable pl011 controller */
	*(volatile u_int32*) FVP_PL011_UARTCR =
			*(volatile u_int32*) FVP_PL011_UARTCR | FVP_PL011_UARTCR_UARTEN | FVP_PL011_UARTCR_TXE | FVP_PL011_UARTCR_RXE;
	//outw(UARTCR,inw(UARTCR) | UARTCR_UARTEN | UARTCR_TXE | UARTCR_RXE);
	//enable_irq(37,1);
	uart_lock=0;						// Open Mutex lock
}

void uartputc_fvp(u_int32 c)
{
	if(c=='\n') {
		/* Wait until the buffer is empty */
		while (*(volatile u_int32*)(FVP_PL011_UARTFR) & (FVP_PL011_UARTFR_TXFF));
		//while (inw(UARTFR) & UARTFR_TXFF);
		/* Put the character into the register */
		*(volatile u_int32*) FVP_PL011_UARTDR = 0x0d;
		//outw(UARTDR , c);
	}
	while (*(volatile u_int32*)(FVP_PL011_UARTFR) & (FVP_PL011_UARTFR_TXFF));
	*(volatile u_int32*) FVP_PL011_UARTDR = c;


}

u_int32 uartgetc_fvp()
{
    /* Wait until the buffer is empty */
	while (*(volatile u_int32*)(FVP_PL011_UARTFR) & (FVP_PL011_UARTFR_RXFE));
	//while (inw(UARTFR) & UARTFR_RXFE);
    /* Put the character into the register */
    return *(volatile u_int32*) FVP_PL011_UARTDR;
    //outw(UARTDR , c);
}
