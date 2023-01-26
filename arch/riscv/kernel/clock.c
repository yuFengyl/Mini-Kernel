// clock.c
#include "sbi.h"
#include "clock.h"
unsigned long TIMECLOCK = 10000000;

unsigned long get_cycles() {

	unsigned long ret_val;
	__asm__ volatile (
        "rdtime t0\n"
	"mv %[ret_val], t0\n"
        : [ret_val] "=r" (ret_val)
        : 
        : "memory"
    );
	return ret_val;
}

void clock_set_next_event() {
    unsigned long next = get_cycles() + TIMECLOCK;
	sbi_ecall(0x0, 0x0, next, 0, 0, 0, 0, 0);
	return ;
   
} 
