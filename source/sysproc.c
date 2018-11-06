#include "types.h"
#include "arm.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return curr_proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = curr_proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  u_int32 ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&ticks_lock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(curr_proc->killed){
      release(&ticks_lock);
      return -1;
    }
    sleep(&ticks, &ticks_lock);
  }
  release(&ticks_lock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  u_int32 xticks;
  
  acquire(&ticks_lock);
  xticks = ticks;
  release(&ticks_lock);
  return xticks;
}
