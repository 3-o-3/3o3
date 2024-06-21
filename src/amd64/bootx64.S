//                        OS-3o3 Operating System
//
//                      26 march MMXXII PUBLIC DOMAIN
//           The author disclaims copyright to this source code.
//
// https://developer.ibm.com/articles/l-gas-nasm/
// https://www.dreamincode.net/forums/topic/350898-converting-gas-to-nasm/
// https://docs.microsoft.com/en-us/windows/win32/debug/pe-format?redirectedfrom=MSDN#section-flags
// https://kirux-bootloaderpart3.blogspot.com/2014/02/writing-bootloader-in-gnu-gas-assembler.html
// https://dox.ipxe.org/UefiSpec_8h_source.html#l01953
// https://github.com/rhboot/shim
// https://stackoverflow.com/questions/52956762/assembly-how-to-set-graphics-mode-in-uefi-no-vga-no-bios-nothing-deprecated

 
#include "../../include/arch/i386/defs.h"

.code64
.text
.global efi_main
.code64
.text

dummy_ldt:
    .word 0
    .quad 0

dummy_tr:
    .word 0
    .quad 0


.globl load_gdt
load_gdt:
    cli
    
    lgdtq GDTR_ADDR
    #lidtq IDTR_ADDR
    #lldt dummy_ldt
    #ltr TSS_SEG
    
    movq $0x0, %rax; # clear task priority register
    movq %rax, %CR8

    #movq $0x277, %rcx # IA32_PAT
    #rdmsr
    #movq $~0x0700000000, %rbx
    #andq %rbx, %rax # clear PA4
    #movq $0x0100000000, %rbx
    #orq %rbx, %rax #  PA4 = 0x01 = Write-combine = WC
    #wrmsr
    
    #movq $0x2FF, %rcx # IA32_MTRR_DEF_TYPE
    #rdmsr 
    #movq $0x00, %rax
    #wrmsr

    #movq $0x1B, %rcx # IA32_APIC_BASE_MSR 
    #rdmsr
    #movq $0x00, %rdx
    #orq $0x800, %rax
    #wrmsr

	movw $0x10, %ax
	movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    movw %ax, %gs  

    movq $LDADDR, %rsp # set the kernel stack just below the load address
    movq $LDADDR, %rbp

jump_to_32bit:
    movq $0x8, %rax   # 32bit code segment is 0x8
    pushq %rax
    movq $KERNEL_ENTRY64, %rax 
    pushq %rax
    lretq

.globl paging_off
paging_off:
    cli
    movq %cr3, %rax
    ret
    movq %cr0, %rax
    movq $0xFFFFFFFF7FFFFFFF, %rbx # clear PG
    andq %rbx, %rax
    movq %rax, %cr0
    ret

    .globl crash
crash:
    pushq $0xFFFFF
    retq


efi_main:
	subq $0x28, %rsp  # 32 byte of shadow space + 8 byte to align stack
	movq %rcx, ImageHandle
	movq %rdx, SystemTable

go:
	movq SystemTable, %rdx	

     	# Parameters
	movq 64(%rdx), %rcx 	# EFI_SYSTEM_TABLE_CONOUT 
    movq %rcx, ConOut
     movq 8(%rcx),%rax
    movq %rax, echo

	leaq hello(%rip), %rdx
	movq $0, %r8
	movq $0, %r9
	movq %rax, 0x20(%rsp)

	movq $0, %rax		# EFI_SUCCESS
	call *8(%rcx)		# EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_OUTPUTSTRING  
  
    movq ImageHandle, %rcx
	movq SystemTable, %rdx
    call main

halt:
    jmp halt
os:
	addq $0x28, %rsp
	ret
.global k__printf
k__printf:
    jmp k__printf2
.globl k__memcpy
k__memcpy:
	movq %rdx,%rsi	# src
	movq %rcx,%rdi	# dest
    movq %r8,%rcx	# n
	rep movsb
	ret
# int in8(int port)
.globl in8
in8:
	movq %rcx,%rdx	# port
	xorq %rax,%rax
	inb %dx,%al
	ret

# int in16(int port)
.globl in16
in16:
	movq %rcx,%rdx	# port
	xorq %rax,%rax
	inw %dx,%ax
	ret

# int in32(int port)
.globl in32
in32:
	movq %rcx,%rdx	# port
	inl %dx,%eax
	ret

# void out8(int port, int data)
.globl out8
out8:
	movq %rdx,%rax	# data
	movq %rcx,%rdx	# port
	outb %al,%dx
	ret

# void out16(int port, int data)
.globl out16
out16:
	movq %rdx,%rax	# data
	movq %rcx,%rdx	# port
	outw %ax,%dx
	ret

# void out32(int port, int data)
.globl out32
out32:
	movq %rdx,%rax	# data
	movq %rcx,%rdx	# port
	outl %eax,%dx
	ret

	.p2align 12,0
endcodes:

.set codesize, endcodes - efi_main 

 
datastart:
hello:  	.byte 'U',0,'E',0,'F',0,'I',0,'!',0,13,0,10,0,0,0

dataend:
	.rept (4096 - (dataend - datastart) % 4096)
	.byte 0
	.endr
dataends:

.set datasize, dataends - datastart
