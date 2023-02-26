#include "proc.h"
#include "sbi.h"
#include "printk.h"
#include "defs.h"

extern struct task_struct* current;
extern struct task_struct* task[10];
extern unsigned long  swapper_pg_dir[512];
extern void ret_from_fork(struct pt_regs *trapframe);
extern uint64 kalloc();
extern uint64 rand();

int sys_getpid() 
{
    return current->pid;
}

void sys_write(unsigned int fd, const char* buf, uint64 count) 
{
    for(int i = 0; i < count; i++) 
        sbi_ecall(0x1, fd, (uint64)buf[i], 0, 0, 0, 0, 0);
}

void forkret() {
    ret_from_fork(current->trapframe);
}

uint64 do_fork(struct pt_regs *regs) {
    // 设置好子进程的 state, counter, priority, pid 等，并将该子进程正确添加至到全局变量 task 数组中
    task[this] = (struct task_struct*)kalloc();
    task[this]->state = TASK_RUNNING;
    task[this]->counter = 0;
    task[this]->pid = this;
    task[this]->priority = rand();

    unsigned long * user_stack = kalloc();
    unsigned long sscratch = csr_read(sscratch);
    // 并将父进程用户栈的内容拷贝到子进程的用户栈中。
    for(int i=0; i<512; i++)
	    user_stack[i]=((unsigned long*)(USER_END-PGSIZE))[i];

    task[this]->user_stack = (unsigned long)user_stack+PGSIZE;

    // TODO ?
    task[this]->thread.ra = (uint64)forkret;
    task[this]->thread.sp = (uint64)task[this]+PGSIZE;
    task[this]->thread.sscratch = task[this]->thread.sp;
    task[this]->thread.sepc = regs->sepc;

    // 正确设置子进程的 pgd 成员变量，为子进程分配根页表，并将内核根页表 swapper_pg_dir 的内容复制到子进程的根页表中，从而对于子进程来说只建立了内核的页表映射。
    unsigned long * newpagetable=kalloc();
    task[this]->pgd = (unsigned long)newpagetable-PA2VA_OFFSET;
    for(int i=0; i<512; i++)
        newpagetable[i] = swapper_pg_dir[i];

    task[this]->mm = kalloc();
    task[this]->mm->mmap = NULL;
    task[this]->trapframe = kalloc();

    struct vm_area_struct* thisvm = current->mm->mmap;
    struct vm_area_struct* firstvm = thisvm;
    struct vm_area_struct* new;
    struct vm_area_struct* prev = task[this]->mm->mmap;
    struct vm_area_struct* firstcreate;
    uint64 vmflag = 0;
    // 复制父进程的 vma 链表。
    while(thisvm != firstvm || (thisvm == firstvm && vmflag == 0)){
	    new = (struct vm_area_struct*)kalloc();
	    new->vm_mm = thisvm->vm_mm;
	    new->vm_start = thisvm->vm_start;
	    new->vm_end = thisvm->vm_end;
	    new->vm_flags = thisvm->vm_flags;
        if(vmflag == 0){
            task[this]->mm->mmap = new;
            firstcreate = new;
        }
        else{
            prev->vm_next = new;
            firstcreate->vm_prev = new;
        }
        new->vm_prev = prev;
        new->vm_next = firstcreate;
        
        prev = new;
        thisvm = thisvm->vm_next;
        vmflag++;
    }
    // 正确设置子进程的 trapframe 成员变量。将父进程的上下文环境（即传入的 regs）保存到子进程的 trapframe 中。
    task[this]->trapframe->sepc = regs->sepc; 
    task[this]->trapframe->sstatus = regs->sstatus;
    for (int i=0; i<31; i++)
        task[this]->trapframe->x[i] = regs->x[i];
    
    task[this]->trapframe->x[2] = csr_read(sscratch);
    task[this]->trapframe->x[10] = 0;
    return this++;
}


uint64 clone(struct pt_regs *regs) {
    return do_fork(regs);
}
