// trap.c 
#include "printk.h"
#include "clock.h"
#include "proc.h"
extern void do_timer();
extern uint64 clone(struct pt_regs *regs);
extern void sys_write(unsigned int fd, const char* buf, uint64 count);
extern int sys_getpid();

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) 
{
   	
	if (scause == 0x8000000000000005) //interrupt
	{	
	
			printk("[S] Supervisor Mode Timer Interrupt\n");
			clock_set_next_event();
			do_timer();
	}
	else 
	{
        if (scause == 8) 
		{
            if (regs->x[17] == 64) 
                sys_write(regs->x[10], (char*)regs->x[11], regs->x[12]); 
			else if (regs->x[17] == 172)
                regs->x[10] = sys_getpid();
            else if (regs->x[17] == 220)
		        regs->x[10] = clone(regs);
            regs->sepc =(unsigned long)regs->sepc + (unsigned long)0x4;
        }
        if (scause == 12 || scause == 15 || scause == 13)
        {
            do_page_fault(scause, regs, sepc);
        }
    }
	return ;
}
