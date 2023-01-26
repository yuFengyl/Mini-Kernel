//arch/riscv/kernel/proc.c
#include "proc.h"
#include "rand.h"
#include "string.h"
#include "mm.h"
#include "printk.h"
#include "defs.h"
extern void __dummy(struct task_struct* this);
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

// lab4
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);
extern unsigned long  swapper_pg_dir[512];
extern char uapp_start[];
extern char uapp_end[];


struct task_struct* idle;           // idle process
struct task_struct* current;        
struct task_struct* task[10];
int flag[NR_TASKS];
uint64 this = NR_TASKS;

void task_init() 
{

	idle = (struct task_struct*)kalloc(); //1
	idle->state = TASK_RUNNING; // 2 
	idle->counter = idle->priority = 0; //3
	idle->pid = 0; //4
	current = idle;
	task[0] = idle;


    /* YOUR CODE HERE */

	int i;
	for (i=1; i<NR_TASKS; i++)
	{
		task[i] = (struct task_struct*)kalloc();
		task[i]->state = TASK_RUNNING;
		task[i]->counter = 0;
		task[i]->priority = rand();
		task[i]->pid = i;
		// left
		task[i]->thread.ra = (uint64)&(__dummy);
		task[i]->thread.sp = (uint64)task[i] + 0x1000;
		//uint64 a = (uint64)&(task[i]->thread.sp) - (uint64)task[i];

		uint64* user_stack = (uint64*)kalloc();
		uint64* user_pagetable = (uint64*)kalloc();
		task[i]->user_stack = (unsigned long)user_stack;

		task[i]->mm = (struct mm_struct*)kalloc();
		task[i]->mm->mmap = NULL;
		//task[i]->mm->mmap = (struct vm_area_struct*)kalloc();

		// 注意，这里用户进程应该存的是物理地址
		task[i]->pgd = (uint64*)((uint64)user_pagetable - PA2VA_OFFSET);
		//memcpy(user_pagetable, swapper_pg_dir, PGSIZE);
		for (int j = 0; j < 512; j++)
            user_pagetable[j] =  swapper_pg_dir[j];

		// creat_mapping for user program
		//create_mapping(user_pagetable, USER_START, (unsigned long)uapp_start-PA2VA_OFFSET, (unsigned long)uapp_end -  (unsigned long)uapp_start, 0b11111);
		do_mmap(task[i]->mm, USER_START, (unsigned long)uapp_end -  (unsigned long)uapp_start, 0b111);

		// 这里是要将user在虚拟地址空间下的stack映射到上面所分配的user_stack所指向的物理空间下
		//create_mapping(user_pagetable, USER_END - PGSIZE, (unsigned long )user_stack - PA2VA_OFFSET, PGSIZE, 0b10111);
		do_mmap(task[i]->mm, USER_END - PGSIZE, PGSIZE, 0b11);




		task[i]->thread.sepc = USER_START;
		csr_write(sepc, task[i]->thread.sepc);
		task[i]->thread.sscratch = USER_END;

		//对sstatus的设置，其中SPP(bit 8)应该被设置为0，SPIE(bit 5)应该被设置为1，SUM(bit 18)设置为1
		task[i]->thread.sstatus =  csr_read(sstatus);
		task[i]->thread.sstatus = task[i]->thread.sstatus | 0x00040020; 
		csr_write(sstatus, task[i]->thread.sstatus);
	}

    /* YOUR CODE HERE */

	printk("...proc_init done!\n");
	return ;

}

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running! thread space begin at %lx \n", current->pid, current);
        }
    }
}


void switch_to(struct task_struct* next) {
    struct task_struct *temp = current;
    if(current == next);
    else{
        //printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",next->pid,next->priority,next->counter);
        current = next;
        __switch_to(temp, next);
    }
    return;
}

void do_timer(void)
{
	if (current->pid==task[0]->pid || current->counter<=0)
		schedule_sjf();
		//schedule_p();
	else
	{
		current->counter--;
		return ;
	}
}

void schedule_sjf()
{
	struct task_struct* next = task[0];
	int i;
	for (i=1; i<this; i++)
	{
		if (next->pid == 0)
		{
			if (task[i]->counter > 0)
				next = task[i];
		}
		if (task[i]->counter<next->counter && task[i]->counter>0)
			next = task[i];
	}
	
	if (next->pid == task[0]->pid)
	{
		//current = task[0];
		//for (i=1; i<NR_TASKS; i++)
			//flag[i] = 0;
		for (i=1; i<this; i++)
		{
			task[i]->counter = rand();
			printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
		}
		for (i=1; i<this; i++)
	        {
        	        if (next->pid == 0)
               		{
                        	if (task[i]->counter > 0)
                                	next = task[i];
                	}
                	if (task[i]->counter < next->counter)
                        	next = task[i];
        	}
	}
	//printk("switch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
	switch_to(next);
	
}

void schedule_p()
{
	struct task_struct* next = task[0];
	int i;
	for (i=1; i<this; i++)
	{
		if (task[i]->priority>next->priority && task[i]->counter>0)
			next = task[i];

	}
	if (next->pid == task[0]->pid)
        {
		//current = task[0];
                //for (i=1; i<NR_TASKS; i++)
                        //flag[i] = 0;
                for (i=1; i<this; i++)
                {
                        task[i]->counter = rand();
                        printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
                }
                for (i=1; i<this; i++)
                {
                        if (task[i]->priority>next->priority && task[i]->counter>0)
                        	next = task[i];
                }
        }
	printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next->pid, next->priority, next->counter);
        switch_to(next);

}

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr)
{
	if (mm->mmap == NULL)
		return NULL;
	struct vm_area_struct *this;
	struct vm_area_struct *first = mm->mmap;
	int flag = 0;
	for (this = mm->mmap; !(this->vm_start <= addr && this->vm_end > addr); this = this->vm_next, flag++)
	{
		if (this->vm_next == first && flag>0)
			return NULL;
	}
	return this;
}

uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot)
{
	struct vm_area_struct *this = (struct vm_area_struct *)kalloc();
	this->vm_start = addr;
	this->vm_mm = mm;
	this->vm_end = addr+length;
	this->vm_flags = prot;
	if (mm->mmap == NULL)
	{
		mm->mmap = this;
		this->vm_next = this;
		this->vm_prev = this;
		return addr;
	}
	else
	{
		if ( !(find_vma(mm, addr) == NULL && find_vma(mm, addr+length) == NULL) ) //表明找到了
			addr = get_unmapped_area(mm, length);
		struct vm_area_struct *a, *b;
		a = mm->mmap;
		b = mm->mmap->vm_next;
		this->vm_next = b;
		this->vm_prev = a;
		a->vm_next = this;
		b->vm_prev = this;
		return addr;	
	}
}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length)
{
	uint64 i;
	uint64 j;
	uint64 addr;
	for (i=0; 1; i=i+PGSIZE)
	{
		addr = i;
		for (j=0; j<length; j=j+PGSIZE)
		{
			if (!find_vma(mm, addr+j))
				continue;
			else
				break;
		}
		if (i>=length)
			return addr;
	}
}

void do_page_fault(unsigned long scause, struct pt_regs *regs, uint64 sepc) {
	/*
	1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
	2. 通过 scause 获得当前的 Page Fault 类型
	3. 通过 find_vm() 找到对应的 vm_area_struct
	4. 通过 vm_area_struct 的 vm_flags 对当前的 Page Fault 类型进行检查
		4.1 Instruction Page Fault      -> VM_EXEC
		4.2 Load Page Fault             -> VM_READ
		4.3 Store Page Fault            -> VM_WRITE
	5. 最后调用 create_mapping 对页表进行映射
	*/
	uint64 stval;
	__asm__ __volatile__ ("csrr %0, stval\n\t" : "=r" (stval));

	int flag = 0;
	struct vm_area_struct *this = find_vma(current->mm, stval);
	if (scause == 12)
		if (this->vm_flags & VM_EXEC)
			flag = 1;
	if (scause == 13)
		if (this->vm_flags & VM_READ)
			flag = 1;
	if (scause == 15)
		if (this->vm_flags & VM_WRITE)
			flag = 1;
	if (flag == 1)
	{
		uint64 perm;
		perm = (this->vm_flags << 1) + 0b10001;
		if (this->vm_start == USER_START)
			create_mapping((uint64*)((unsigned long)current->pgd + PA2VA_OFFSET), this->vm_start, (uint64)uapp_start-PA2VA_OFFSET, this->vm_end-this->vm_start, (uint64)0b11111);
		else if (this->vm_start == USER_END-PGSIZE)
			create_mapping((uint64*)((unsigned long)current->pgd + PA2VA_OFFSET), this->vm_start , current->user_stack-PA2VA_OFFSET-PGSIZE, PGSIZE ,(uint64)0b10111);
		printk("[S] PAGE_FAULT: scause: %ld, sepc: 0x%lx, badaddr: 0x%lx \n",scause, sepc, stval);
	}
}