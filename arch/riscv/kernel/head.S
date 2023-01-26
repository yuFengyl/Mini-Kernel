.extern start_kernel
.extern mm_init
.extern task_init
.extern setup_vm

    .section .text.init
    .globl _start
    .globl _traps
	.globl early_pgtbl
	.globl swapper_pg_dir
	.globl set_satp_2
_start:
    # ------------------
    # - your code here -
    # ------------------

	la sp,boot_stack_top

	call setup_vm
	call relocate
	

	call mm_init

	call setup_vm_final

    call task_init

	
	la t0, _traps
	csrw stvec, t0

	#---------------------------------------------

	li t0, 0x20
	csrs sie, t0 # set sie[STIE] = 1

	#----------------------------------------------

	rdtime t0
	li t1, 10000000
	add a0, t0, t1
	li a7, 0x00
	li a6, 0
	ecall          


	#----------------------------------------------

	#li t0, 0x2 
	#csrs sstatus, t0 #  sstatus[sie] = 1

	#----------------------------------------------


_lab4code:


	j start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
	li t1, 0xffffffdf80000000
	add ra, ra, t1
	add sp, sp, t1
   
    ###################### 
    #   YOUR CODE HERE   #
    ######################

    # set satp with early_pgtbl
	la t1, early_pgtbl
	srli t1, t1, 12
	csrw satp, t1
	li t1, 1
	slli t1, t1, 63
	csrs satp, t1

    ###################### 
    #   YOUR CODE HERE   #
    ######################
    
    # flush tlb
    sfence.vma zero, zero

    ret


    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
