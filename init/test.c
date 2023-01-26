#include "printk.h"
#include "defs.h"

// Please do not modify

void test() {
	printk("\n");
    while (1)
    {
	    
	    //printk("kernel is running!\n");
	    unsigned int cnt = 0;
	    while (cnt < 0x1FFFFFF0)
		    cnt++;
    }
}
