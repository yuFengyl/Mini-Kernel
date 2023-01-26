// arch/riscv/include/proc.h

#include "types.h"


/* vm_area_struct vm_flags */
#define VM_READ     0x00000001
#define VM_WRITE    0x00000002
#define VM_EXEC     0x00000004

#define NR_TASKS  (1 + 1)

#define TASK_RUNNING    0 

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

struct thread_info {
    uint64 kernel_sp;
    uint64 user_sp;
};

typedef unsigned long* pagetable_t;

struct thread_struct {
    uint64 ra;
    uint64 sp;                     
    uint64 s[12];

    uint64 sepc, sstatus, sscratch; 
};

struct vm_area_struct {
    struct mm_struct *vm_mm;    /* The mm_struct we belong to. */
    uint64 vm_start;          /* Our start address within vm_mm. */
    uint64 vm_end;            /* The first byte after our end address 
                                    within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    struct vm_area_struct *vm_next, *vm_prev;

    uint64 vm_flags;      /* Flags as listed above. */
};

struct mm_struct {
    struct vm_area_struct *mmap;       /* list of VMAs */
};

struct task_struct {
    struct thread_info* thread_info;
    uint64 state;
    uint64 counter;
    uint64 priority;
    uint64 pid;

    struct thread_struct thread;

    pagetable_t pgd;

    struct mm_struct *mm;
    struct pt_regs *trapframe;
    uint64 user_stack;
};

void task_init(); 

void do_timer();

void schedule_sjf();

void schedule_p();

void switch_to(struct task_struct* next);

void dummy();

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr);

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot);

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length);

struct pt_regs {
    unsigned long x[32];
	unsigned long sepc;
    unsigned long sstatus;
};

void do_page_fault(unsigned long scause, struct pt_regs *regs, uint64 sepc);

extern uint64 this;