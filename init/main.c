#include "printk.h"
#include "sbi.h"
#include "print.h"
#include "proc.h"

extern void test();

int start_kernel() {
    printk("2022");
    printk("[S-MODE] Hello RISC-V \n");
    schedule_sjf();
    test(); // DO NOT DELETE !!!

	return 0;
}
