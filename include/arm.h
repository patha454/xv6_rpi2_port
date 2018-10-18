/*****************************************************************
*       arm.h
*       by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/




#define PSR_MODE_USR		0x00000010 
#define PSR_MODE_FIQ		0x00000011
#define PSR_MODE_IRQ		0x00000012
#define PSR_MODE_SVC		0x00000013
#define PSR_MODE_MON		0x00000016
#define PSR_MODE_ABT		0x00000017
#define PSR_MODE_UND		0x0000001B
#define PSR_MODE_SYS		0x0000001F
#define PSR_MASK		0x0000001F
#define USER_MODE		0x0

#define PSR_DISABLE_IRQ		0x00000080
#define PSR_DISABLE_FIQ		0x00000040

#define PSR_V			0x10000000
#define PSR_C			0x20000000
#define PSR_Z			0x40000000
#define PSR_N			0x80000000


static inline u_int32
inw(u_int32 addr)
{
    u_int32 data;

    asm volatile("ldr %0,[%1]" : "=r"(data) : "r"(addr));
    return data;
}

static inline void
outw(u_int32 addr, u_int32 data)
{
    asm volatile("str %1,[%0]" : : "r"(addr), "r"(data));
}


// Layout of the trap frame built on the stack
// by exception.s, and passed to trap().
struct trapframe {
  u_int32 sp; // user mode sp
  u_int32 r0;
  u_int32 r1;
  u_int32 r2;
  u_int32 r3;
  u_int32 r4;
  u_int32 r5;
  u_int32 r6;
  u_int32 r7;
  u_int32 r8;
  u_int32 r9;
  u_int32 r10;
  u_int32 r11;
  u_int32 r12;
  u_int32 r13;
  u_int32 r14;
  u_int32 trapno;
  u_int32 ifar; // Instruction Fault Address Register (IFAR)
  u_int32 cpsr;
  u_int32 spsr; // saved cpsr from the trapped/interrupted mode
  u_int32 pc; // return address of the interrupted code
};

