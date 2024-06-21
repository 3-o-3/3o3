
#                        OS-3o3 operating system
#
#                this is the 32 bit i386 kernel entry point
#
#         27 february MMXXIII PUBLIC DOMAIN by Jean-Marc Lienher
#
#           The author disclaims copyright to this source code.
#
#

#include "../../include/arch/i386/defs.h"

.code32
.text
.global _start
_start:			# here we are at 0x00010400
	jmp main

.org GFX_INFO - _start - LDADDR - CODESIZE
.globl gfx_info
gfx_info:
.rept 40
.byte 0
.endr
.long 0x0B0000
.rept 210
.byte 0
.endr

.org KERNEL_INFO - _start - LDADDR - CODESIZE
.globl kernel32 #  FIXME
kernel32:
.long 0	# version
.long 0 # pointer to interface
.org HPET_CONFIG - _start - LDADDR - CODESIZE
.long 0

.org IDT_ADDR - _start - LDADDR - CODESIZE
.globl idt
idt:
	.rept 256
	.quad 0
	.quad 0
	.endr

.globl kernel64
.org KERNEL_ENTRY64 - _start - LDADDR - CODESIZE
kernel64: # we jump here from UEFI bootx64.S jump_to_32bit
    movw $0x10, %ax
	movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    movw %ax, %gs  
  
    movl %cr0, %eax
    movl $~0x80000000, %ebx # clear PG
    andl %ebx, %eax
	andl $~0x40000000, %eax # clear CD
	andl $~0x20000000, %eax # clear NW
#   andl $~0x10000, %eax # clear WP
    movl %eax, %cr0
   

    movl $0xc0000080, %ecx
    rdmsr
    andl $~0x100, %eax # clear IA32_EFER.LME
#   andl $~0x800, %eax # clear IA32_EFER.NXE
    wrmsr

    movl %cr4, %eax
    andl $~0x20, %eax  # clear PAE
#   andl $~0x1000, %eax # clear LA57
#   andl $~0x20000, %eax # clear PCIDE
    movl %eax, %cr4


    movl %cr4, %eax
#   orl $0x20, %eax  # set PAE
    orl $0x10, %eax  # set PSE
	orl $0x80, %eax # set PGE
    andl $~0x1000, %eax # clear LA57
    andl $~0x20000, %eax # clear PCIDE
	andl $~0x400000, %eax # clear PKE
    andl $~0x1000000, %eax # clear PKS
    movl %eax, %cr4
    
    movl $0xc0000080, %ecx # EFER MSR
    rdmsr
#   orl $0x100, %eax # set IA32_EFER.LME
    wrmsr

    movl $LDADDR - 512, %esp # set the kernel stack just below the load address
    movl $LDADDR, %ebp
    
	movl $PDT_ADDR, %eax
    movl %eax, %cr3 
 
    movl %cr0, %eax
    movl $0x80000001, %ebx # set PG and PE
	#orl $0x40000000, %ebx # set CD
	#orl $0x20000000, %ebx # set NW
	
    orl %ebx, %eax
    movl %eax, %cr0    
    ljmpl $0x8, $KERNEL_ENTRY32 
	
.globl syscall
syscall:
		int $0x80
		ret
	
.org ISR_STUB - _start - LDADDR - CODESIZE
.globl isr_stub
isr_stub:
	cli
	sti
	iret
	
.org ISR_STUB_ERR - _start - LDADDR- CODESIZE
.globl isr_stub_err
isr_stub_err:
	cli
	pushl $1
	jmp isr_common

.org ISR_SYSCALL - _start - LDADDR- CODESIZE
.globl isr_syscall
isr_syscall:
	cli
	push %esp
    call isr_handler
    add $4, %esp
    sti
	iret

.org ISR_TIMER - _start - LDADDR- CODESIZE
.globl isr_timer
isr_timer:
	cli
	pushl $0
	pushl $8 
	jmp isr_common


.org ISR_KBD - _start - LDADDR- CODESIZE
.globl isr_kbd
isr_kbd:
	cli
	pushl $0
	pushl $9
	jmp isr_common

.org ISR_MOUSE - _start - LDADDR- CODESIZE
.globl isr_mouse
isr_mouse:
	cli
	pushl $0
	pushl $0x74
	jmp isr_common

.org ISR_USB - _start - LDADDR- CODESIZE
.globl isr_usb # FIXME
isr_usb:
	cli
	pushl $0
	pushl $0xFF
	jmp isr_common

isr_common:
    pusha
    push %ds
    push %es
    push %fs
    push %gs

	#mov %ax, %cs 
    #mov $0x10, %ax
    #mov %ax, %ds
    #mov %ax, %es
    #mov %ax, %fs
    #mov %ax, %gs
    #cld

.extern isr_handler
    push %esp
    call isr_handler
    add $4, %esp
	pop %gs
    pop %fs
    pop %es
    pop %ds

    popa

    addl $8, %esp
	
    sti
	iret

.text
.globl  k__longjmp
k__longjmp:
	pushl	%ebp;
	movl	%esp,%ebp
	movl	8(%ebp),%ecx
	movl	12(%ebp),%eax
	orl		%eax,%eax
	jne		vvok
	incl	%eax
vvok:
	# uint32 ebp, esp, ebx, eip;
	movl	4(%ecx),%esp
	movl	0(%ecx),%ebp
	movl	8(%ecx),%ecx
    jmp     *%ecx
	
	.globl  k__setjmp
k__setjmp:
	pushl	%ebp;
	movl	%esp,%ebp
	movl	8(%ebp),%eax

	# uint32 ebp, esp, ebx, eip;
	movl	%esp,%ecx	# esp
	addl	$8,%ecx
	movl	%ecx,4(%eax)
	
	movl	0(%esp),%ecx
	movl	%ecx,0(%eax)	# ebp
	
	movl	4(%esp),%ecx	# pc (eip)
	movl	%ecx,8(%eax)
	
	xorl	%eax,%eax
	popl	%ebp
	ret


# void *memcpy2(void *dest, void *src, os_size n)	
.globl k__memcpy
k__memcpy:
	movl 12(%esp),%ecx	# n
	movl 8(%esp),%esi	# src
	movl 4(%esp),%edi	# dest
	rep movsb
	ret

.globl k__printf
k__printf:
	jmp k__printf2

.globl mmu__init
mmu__init:
	ret

# int in8(int port)
.globl in8
in8:
	movl 4(%esp),%edx	# port
	xorl %eax,%eax
	inb %dx,%al
	ret

# int in16(int port)
.globl in16
in16:
	movl 4(%esp),%edx	# port
	xorl %eax,%eax
	inw %dx,%ax
	ret

# int in32(int port)
.globl in32
in32:
	movl 4(%esp),%edx	# port
	inl %dx,%eax
	ret

# void out8(int port, int data)
.globl out8
out8:
	movl 8(%esp),%eax	# data
	movl 4(%esp),%edx	# port
	outb %al,%dx
	ret

# void out16(int port, int data)
.globl out16
out16:
	movl 8(%esp),%eax	# data
	movl 4(%esp),%edx	# port
	outw %ax,%dx
	ret

# void out32(int port, int data)
.globl out32
out32:
	movl 8(%esp),%eax	# data
	movl 4(%esp),%edx	# port
	outl %eax,%dx
	ret

#uint32 cpu_interrupt_enable(uint32 state)
.globl cpu_interrupt_enable
cpu_interrupt_enable:
	movl 4(%esp),%eax	# state
	cmpl $0, %eax
	je disablei
	sti
	ret
disablei:
	cli
	ret

.globl delay
delay:
	nop
	ret

.globl cpu_apic_init
cpu_apic_init:
	cli
	movl $0x1B, %ecx # IA32_APIC_BASE MSR
    rdmsr 
    orl $0x800, %eax # global enable
    wrmsr
	ret

.globl cpu_interrupt_init
cpu_interrupt_init:
	cli
	# https://wiki.osdev.org/8259_PIC
	inb $0x21, %al
	movb %al, %bl
	inb $0xA1, %al
	movb %al, %bh

	movb $0x11, %al     # ICW1_ICW4 | ICW1_INIT
	outb %al,$0x20		# command master PIC 
	call delay   
	outb %al,$0xA0		#  command slave PIC
	call delay

						# ICW2
	movb $0x20,%al		# offset PIC 1  
	outb %al,$0x21		# data master PIC
	call delay
	movb $0x28,%al		# offset PIC 2   
	outb %al, $0xA1		# data master PIC
	call delay
						
						# ICW3 
	movb $0x04, %al		# slave PIC at IRQ2 pof master PIC      
	outb %al, $0x21 
	call delay
	movb $0x02, %al     # slave PIC cascade identity 
	outb %al, $0xA1		
	call delay
				
						# ICW4
	movb $0x01, %al     # 8086 mode
	outb %al,$0x21
	call delay
	outb %al,$0xA1
	call delay

	movb %bl,%al       # mask interrupts
	outb %al,$0x21
	call delay
	movb %bh,%al
	outb %al,$0xA1
	call delay

	movb $0xFE,%al       # new mask interrupts
	outb %al,$0x21
	call delay
	movb $0xFF,%al
	outb %al,$0xA1
	call delay

# https://github.com/public-domain/tinyos/blob/master/sys/kern/pit.c
# https://wiki.osdev.org/Programmable_Interval_Timer
	call delay
	movb $0x36, %al	# which chanel of the PIT
	outb %al,$0x43
	call delay
	movb $0x9b, %al # set frequency low byte (100HZ)
	outb %al,$0x40 
	call delay
	movb $0x2e, %al # set frequency high byte
	outb %al,$0x40 
	call delay
	ret

.globl beep
beep:
    # Set the speaker frequency
    movb $0xB6, %al         # Channel 2 Command Byte
    outb %al, $0x43
    movw $0x02FA, %ax       # Frequency divisor
    outb %al, $0x42         # Low byte
    movb %ah, %al
    outb %al, $0x42         # High byte

    # Start the speaker
    inb $0x61, %al
    orb $0x03, %al
    outb %al, $0x61

    # Wait for a while to keep the beep
    movl $0x0FFFFFFF, %ecx
.wait:
    loop .wait

    # Stop the speaker
    inb $0x61, %al
    andb $0xFC, %al
    outb %al, $0x61
    ret

.globl cpu_lidt
cpu_lidt:
	movl 4(%esp),%eax	# idt
	lidt (%eax)
	ret

.section .text.theend
pend:
	retl
	.globl get_prog_end
get_prog_end:
	call pend
	movl -4(%esp),%eax
	retl
