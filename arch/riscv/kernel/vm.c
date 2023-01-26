// arch/riscv/kernel/vm.c
#include "defs.h"
#include <string.h>
#include "printk.h"
#include "mm.h"


/* _stext, _etext 分别记录了text段的起始与结束地址 */
extern char _stext[];
extern char _etext[];

extern char _srodata[];
extern char _erodata[];

extern char _sdata[];
extern char _edata[];

extern char _sbss[];
extern char _ebss[];

extern void set_satp_2();

void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
/* 作为全局变量，会被初始化为0 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */

    /* 将0x80000000映射到0x80000000 */    
    early_pgtbl[2] = early_pgtbl[2] | 0x2000000f;
    /* 将0xffffffe000000000映射到0x80000000 */
    early_pgtbl[384] = early_pgtbl[384] | 0x2000000f;
    //early_pgtbl[384] = early_pgtbl[384] | 0x20080c0f;
    return ;
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

/*     swapper_pg_dir[384] = swapper_pg_dir[384] | 0x20000000;

    uint64* pgtbl = (uint64*)&swapper_pg_dir[384]; */

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_stext, (uint64)_stext - PA2VA_OFFSET, (uint64)_etext - (uint64)_stext, 0b1011);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_srodata, (uint64)_srodata - PA2VA_OFFSET, (uint64)_erodata - (uint64)_srodata, 0b0011);
    
    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64)_sdata, (uint64)_sdata - PA2VA_OFFSET, 0x7dfd000, 0b0111);
    
    //swapper_pg_dir[384] = swapper_pg_dir[384] | 0x2000000f;

    // set satp with swapper_pg_dir
    //set_satp_2();
    uint64 satp_value = (((uint64)swapper_pg_dir - PA2VA_OFFSET) >> 12) | 0x8000000000000000;
    csr_write(satp, satp_value);

    // flush TLB
    asm volatile("sfence.vma zero, zero");
    return;
}


void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm){
    uint64 i;
    uint64 *first,*second,*third;
    // first, second, third represents first, second and third level
    
    for(i=0; i<sz; i+=PGSIZE){
        //first level
        first = &pgtbl[((va+i)>>30)&0x1FF];
        if((*first)&0x1) // has been created
            second = (uint64*)((unsigned long)(((*first)>>10)<<12) + PA2VA_OFFSET);
        else
        {
            second = (uint64*)kalloc();
            memset(second, 0, PGSIZE);
            *first = (unsigned long)((*first)&0xffc0000000000000)|(((((unsigned long)second  -PA2VA_OFFSET)>>12)<<10)|(unsigned long)0x1);
        }
        second = &second[((va+i)>>21)&0x1FF];
        if((*second)&0x1)// has been created
            third=(unsigned long*)((unsigned long)(((*second)>>10)<<12)+PA2VA_OFFSET);
        else
        {
            third = (uint64*)kalloc();
            memset(third,0,PGSIZE);
            *second = (unsigned long)((*second)&0xffc0000000000000)|(((((unsigned long)third-PA2VA_OFFSET)>>12)<<10)|(unsigned long)0x1);
        }
        third = &third[((va+i)>>12)&0x1FF];
        *third = (unsigned long)(((*third)&0xffc0000000000000)|((((unsigned long)(pa+i)>>12)<<10)|0x1|perm));  
	}
    return;
}

/* 
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm)
{
    
    uint64 thisva;
    uint64 thispa;
    int level;
    uint64 thispte;
    uint64* thisbase = pgtbl;
    for (thispa = pa, thisva = va; thisva < va + sz; thisva = thisva + PGSIZE, thispa = thispa + PGSIZE)
    {
        // 这里是大于0，因为最底层的页表此时是不可能被写过的 
        for (level=2; level>0; level--)
        {
            if (level == 2)
                thispte = thisbase[(thisva>>30) & 0x1ff];
            else
                thispte = thisbase[(thisva>>21) & 0x1ff];
            
            // 判断一下有没有写过
            if (thispte & 0x1)
            // 注意，这里需要转换到虚拟地址
                thisbase = (uint64 *)(((thispte >> 10) << 12) + PA2VA_OFFSET);
            else // 说明需要分配一块新的空间
            {
                thisbase = (uint64 *)kalloc();
                memset(thisbase, 0, PGSIZE);
                WritePTE(&thispte, ((unsigned long)thisbase -  PA2VA_OFFSET) >> 12, 0, 1);
            }
        }
        WritePTE(&thisbase[(va>>12) & 0x1ff], ((unsigned long)thispa) >> 12, perm, 1);
    }
} */