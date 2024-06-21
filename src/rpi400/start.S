/******************************************************************************
 *                        OS-3o3 operating system
 * 
 *                    32bit boot for the Raspberry PI400
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

#
#	NMH's Simple C Compiler, 2011--2014
#	C runtime module for FreeBSD/armv6
#

# Calling conventions: r0,r1,r2,stack, return in r0
#                      64-bit values in r0/r1, r2/r3, never in r1/r2
#                      (observe register alignment!)
# System call: r7=call#, arguments as above,
#              carry indicates error,
#              return/error value in r0

// http://hehezhou.cn/arm/AArch32-vbar.html
// https://github.com/LdB-ECM/Raspberry-Pi/blob/master/SD_FAT32/SmartStart32.S

#include "../../include/arch/rpi400/defs.h"
#define LOCAL_CONTROL   0xff800000
#define LOCAL_PRESCALER 0xff800008
#define OSC_FREQ        54000000

.equ F, 0x40
.equ I, 0x80
.equ A, 0x100
.equ Mode_FIQ, 0x11 + F + I
.equ Mode_IRQ, 0x12 + F + I
.equ Mode_SVC, 0x13 + F + I
.equ Mode_HYP, 0x1A + F + I 


#define RPI_BASE        0xFE000000 /* RPi 4 */
#define GPIO_BASE       (RPI_BASE | 0x200000)
#define GPFSEL2         (GPIO_BASE | 0x8) // GPIO select 2
#define   GPIO_ALT4          (3)      // GPIO is Alternate4 (011)


.arch armv8.2-a
	.text
	.align	2
	.globl	_start
_start:

#include "boot64.inc"
mode_svc:
	nop
	nop
	nop
	nop
	mrc p15,0,r6,c0,c0,5 	// Read MPIDR into r6
	lsls r6,r6,#30 		// Extract processor number field
step1:
    ldr r0,=0
init_vect:
	bl set_vector
	add r0,r0,#4
	cmp r0,#0x20
	bne init_vect
	b step2
set_vector:
	ldr r2,=irq_reset // table of vector routines
	add r2,r2,r0	  // add offset  
	ldr r2,[r2]	  // read routine address
	sub r2,r2,r0	  // subtract offset 
	lsr r2,r2,#2	  // convert to word count
	sub r2,r2,#2      // correct branching offset
	orr r2,r2,#0xEA000000 // branch op code
	str r2,[r0]	  // store "b _rountineX" in real vector
	mov pc,lr

step2: 				
	// setup stacks
	mov r0,#Mode_IRQ 
	msr cpsr_c,r0
	ldr	sp, =START_ADDRESS - 256
	mov r0,#Mode_FIQ 
	msr cpsr_c,r0
	ldr	sp, =START_ADDRESS - 512
	mov r0,#Mode_SVC
	msr cpsr_c,r0
	ldr	sp, =START_ADDRESS - 768
#define SVC_STACK (START_ADDRESS - 1024)
step3:
	cmp r6,#0
	beq core0

second_core:
	wfe
	b second_core
core0:
	bl pl011__init

	mov r0,#'3'
	bl pl011__write
	
	mov r0,#'2'
	bl pl011__write

wa:
	bl pl011__read
	cmp r0,#'b'
	//bne wa
	
	movw r1,#:lower16:LOCAL_PRESCALER
	movt r1,#:upper16:LOCAL_PRESCALER
	movw r0,#:lower16:(39768216)
	movt r0,#:upper16:(39768216)
	str r0,[r1]

	mov r0,#'G'
	bl pl011__write
	
	bl k__fb_init
	mov r0,r2

	mov r0,#'r'
	bl pl011__write
 

	mov r0,#'s'
	bl pl011__write
	mov r0,#' '
	bl pl011__write

	bl main
hang:
	bl pl011__read
	cmp r0,#-1
	beq hang

	push {r0}
	bl fb_swap
	pop {r0}

	mov r2,r0
	bl pl011__write
	b hang

irq_reset:    .long nothing // reset
irq_undef:    .long handler_undef /* 0x4  Undefined Instruction */
irq_swi:      .long handler_svc /* 0x8  Software Interrupt */
irq_prefetch: .long handler_prefetch /* 0xC  Prefetch Abort */
irq_data:     .long handler_data /* 0x10 Data Abort */
irq_reserve:  .long nothing /* 0x14 Reserved */
irq_irq:      .long handler_irq /* 0x18 IRQ*/
irq_fiq:      .long handler_fiq /* 0x1C FIQ */

handler_fiq:
	movs pc,lr

handler_undef:
	movs pc,lr


handler_prefetch:
	subs    pc,lr,#4

handler_data:
	sub lr,lr,#4
	stmfd sp!,{r0-r12,lr} 
	ldmfd sp!,{r0-r12,pc}^ 

handler_irq:
	sub lr,lr,#4
	stmfd sp!,{r0-r12,lr} 
	mov r0,#71
	bl gic400__irq_vector
	ldmfd sp!,{r0-r12,pc}^ 

handler_svc:
	#ldr sp,=SVC_STACK
	push {r0-r12,lr} 
	ldr r0,[lr,#-4]
	bic r0,r0,#0xFF000000 // get svc number in r0
	mov r1,sp
	bl pl011__write
	// call c_handler(intno, regs[])
	pop {r0-r12,lr}
	movs pc, lr

nothing:
	sub lr,lr,#4
	movs pc,lr



.global syscall
syscall:
	svc '%'
	movs pc,lr

.global dma_ctrl
.align 5
dma_ctrl:
.rept 8
.long 0
.endr

kbchar:
.long -1

.align 2

.global os__clear_timer
os__clear_timer:
	push {r11,lr}
	mrrc p15, 2, r0, r1, c14 // get  CNTP_CVAL
	bl cntp_cval			// add period for the next tick
	mcrr p15, 2, r0, r1, c14 // set CNTP_CVAL
	pop {r11,pc}


.global os__init_timer
os__init_timer:
	push {r11,lr}
	mrrc p15, 0, r0, r1, c14	// get CNTPCT
	bl cntp_cval
	mcrr p15, 2, r0, r1, c14	// set CNTP_CVAL
	mov r0,#1
	mcr p15, 0, r0, c14, c2, 1 //

	mrc p15, 0, r0, c14, c0, 0 // get CNTFRQ (must be 54000000)
	
	mov r1,#LOCAL_PRESCALER -LOCAL_CONTROL
	add r1,r1,#LOCAL_CONTROL
	ldr r1,[r1]		// get PRESCALER (must be 39768216 for 1MHz)
	pop {r11,pc}


.global kbhit
kbhit:
	push {r11,lr}
	bl pl011__read
	cmp r0,#-1
	beq kbhit_none
	ldr r1,=kbchar
	str r0,[r1]
	mov r0,#1
	b kbhit_ret
kbhit_none:
	mov r0,#0
kbhit_ret:
	pop {r11,pc}
	
.global getch
getch:
	ldr r0,=kbchar
	ldr r0,[r0]
	ldr r2,=kbchar
	ldr r1,=-1
	str r1,[r2]
	bx lr

.global os__enable_irqs
os__enable_irqs:
	mrs r0, cpsr
 	bic r0, r0, #I
   	msr cpsr_c, r0
	cpsie i
	mov pc,lr

.global os__disable_irqs
os__disable_irqs:
	cpsid i
	mov pc,lr

	nop
	nop
	nop
.global irq_stub
irq_stub:
	nop
	nop
	nop
	nop
	mov pc,lr
	push {r0-r12,lr}
	bl gic400__irq_vector
	pop {r0-r12,pc} 

.global k__memcpy
k__memcpy:
	b dma__move // this is faster

	push {r10,r11,lr}
	mov r11,sp
	// r0 dest
	// r1 src
	// r2 size
	mov r10,r2
	lsr r10,r10,#6
// 1 2 4 8 16 32 64
// 0 1 2 3  4  5  6	
move_loop64:
	cmp r10,#0
	beq move_4
	ldrd r2,r3,[r1,#0]
	strd r2,r3,[r0,#0]
	ldrd r4,r5,[r1,#8]
	strd r4,r5,[r0,#8]
	ldrd r6,r7,[r1,#16]
	strd r6,r7,[r0,#16]
	ldrd r8,r9,[r1,#24]
	strd r8,r9,[r0,#24]
	ldrd r2,r3,[r1,#32]
	strd r2,r3,[r0,#32]
	ldrd r4,r5,[r1,#40]
	strd r4,r5,[r0,#40]
	ldrd r6,r7,[r1,#48]
	strd r6,r7,[r0,#48]
	ldrd r8,r9,[r1,#56]
	strd r8,r9,[r0,#56]
	
	add r0,r0,#64
	add r1,r1,#64
	sub r10,r10,#1
	b move_loop64

move_4:
	ldr r10,[r11,#16] // size
	and r10,r10,#63
	lsr r10,r10,#2
move_loop4:
	cmp r10,#0
	beq move_1
	ldr r3,[r1]
	str r3,[r0]
	add r0,r0,#4
	add r1,r1,#4
	sub r10,r10,#1
	b move_loop4

move_1:
	ldr r10,[r11,#16] // size
	and r10,r10,#3
move_loop1:
	cmp r10,#0
	beq move_ret
	ldrb r3,[r1]
	strb r3,[r0]
	add r0,r0,#1
	add r1,r1,#1
	sub r10,r10,#1
	b move_loop1

move_ret:
	pop {r10,r11,pc}

.align 2

.global k__printf
k__printf:
	b k__printf2

.global peek
peek:
	push {r11,lr}
	mov r11,sp
//	ldr r0,[r11,#8] 
	ldr r0,[r0]
	pop {r11,pc}

.global invalid_cache
invalid_cache:
	push {r11,lr}
	mov r11,sp
	//ldr r0,[r11,#8] 
	mcr p15,0,r0,c7,c14,1
	pop {r11,pc}

.global sync_cache
sync_cache: //fixme
	push {r4-r11,lr}
	dmb
	isb
	dsb st
	isb
	mcr p15, 0, r0, c7, c5,  0
	isb
	mcr p15, 0, r0, c7, c5,  6
	dsb
	isb
	pop {r4-r11,pc}

.global os__data_sync_barrier
os__data_sync_barrier:
	push {r11,lr}
	eor r0,r0,r0
	dsb
	pop {r11,pc}

.global os__data_mem_barrier
os__data_mem_barrier:
	push {r11,lr}
	eor r0,r0,r0
	dmb
	pop {r11,pc}


	.global os__enable_mmu
os__enable_mmu:
	push {r4,r11,lr}
	
	mov r4,r0
	mrc p15, 0, r0, c1, c0,  0 // SCTLR
	and r0,r0,#~(ARM_CONTROL_MMU)
	mcr p15, 0, r0, c1, c0,  0 // SCTLR
	

	mov r0,#(LPAE_MAIR_NORMAL << (ATTRINDX_NORMAL*8))
	orr r0,r0,#(LPAE_MAIR_DEVICE  << (ATTRINDX_DEVICE*8))
	orr r0,r0,#(LPAE_MAIR_COHERENT << (ATTRINDX_COHERENT*8))
	mcr p15, 0, r0, c10, c2, 0 // MAIR0

	mov r0,#(LPAE_TTBCR_EAE)
	//orr r0,r0,#(LPAE_TTBCR_EPD1)
	orr r0,r0,#(ATTRIB_SH_INNER_SHAREABLE << LPAE_TTBCR_SH0__SHIFT)
	orr r0,r0,#(LPAE_TTBCR_ORGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_ORGN0__SHIFT)
	orr r0,r0,#(LPAE_TTBCR_IRGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_IRGN0__SHIFT)
	orr r0,r0,#(LPAE_TTBCR_T0SZ_4GB)
	mcr p15, 0, r0, c2, c0,  2 // TTBCR (TCR_EL1 for 64bit)

	//mcr p15, 0, r0, c2, c0,  3 // TTBCR2
	
	// https://git.sr.ht/~sircmpwn/helios/commit/aa5388d2582fb917647d9e5b4f96291cee590267

	mov r0,r4
	eor r1,r1,r1
	mcrr p15, 0, r0, r1, c2 // TTBR0
	mcrr p15, 1, r0, r1, c2 // TTBR1

	ldr r0,=0xFFFFFFFF
	ldr r0,=1
	//mcr p15, 0, r0, c3, c0,  0 //  DACR
	
	
	dsb ishst
	//tlbi vmalle1is
	//mcr p15, 0, r0, c8, c5,  0 // DTLBIALL
	mcr p15, 0, r0, c8, c6,  0 // ITLBIALL
	mcr p15, 0, r0, c7, c1,  0 // ICIALLU
	mcr p15, 0, r0, c8, c7, 0  // TLBIALL
	dsb ish
	isb

	mrc p15, 0, r0, c1, c0,  0 // SCTLR
	ldr r0,=0
	and r0,r0,#(~ARM_CONTROL_STRICT_ALIGNMENT)
	orr r0,r0,#(ARM_CONTROL_MMU)
	//orr r0,r0,#(ARM_CONTROL_L1_CACHE)
	//orr r0,r0,#(ARM_CONTROL_L1_INSTRUCTION_CACHE)
	//orr r0,r0,#(ARM_CONTROL_BRANCH_PREDICTION)
	
	mcr p15, 0, r0, c1, c0,  0 // SCTLR
	isb

	pop {r4,r11,pc}


.global poke
poke:
	push {r11,lr}
	mov r11,sp
//	ldr r0,[r11,#8] 
//	ldr r1,[r11,#12] 
	str r1,[r0]
	pop {r11,pc}

.global k__usleep
k__usleep:
	push {r1,r11,lr}
	mov r11,sp
	ldr r0,[r11,#8] 
usleep1:
	sub r0,r0,#1
	cmp r0,#0
	bne usleep1

	pop {r1,r11,pc}

	.text
	.align 2

# unsigned integer divide
# inner loop code taken from http://me.henri.net/fp-div.html
# in:  r0 = num,  r1 = den
# out: r0 = quot, r1 = rem

	.globl	udiv
	.align	2
udiv:	rsb     r2,r1,#0
	mov     r1,#0
	adds    r0,r0,r0
	.rept   32
	adcs    r1,r2,r1,lsl #1
	subcc   r1,r1,r2
	adcs    r0,r0,r0
	.endr
	mov     pc,lr

# signed integer divide
# in:  r0 = num,  r1 = den
# out: r0 = quot

	.globl	sdiv
	.align	2
sdiv:	push	{lr}
	eor	r3,r0,r1	@ r3 = sign
	asr	r3,r3,#31
	cmp	r1,#0
	beq	divz
	rsbmi	r1,r1,#0
	cmp	r0,#0
	rsbmi	r0,r0,#0
	bl	udiv
	cmp	r3,#0
	rsbne	r0,r0,#0
	pop	{pc}
divz:	
	mov	r0,#0		// return 0 if x/0
	pop	{pc}



# signed integer modulo
# in:  r0 = num,  r1 = den
# out: r0 = rem

	.globl	srem
	.align	2
srem:	push	{lr}
	asr	r4,r0,#31		@ r4 = sign
	bl	sdiv
	mov	r0,r1
	cmp	r4,#0
	rsbne	r0,r0,#0
	pop	{pc}


# int setjmp(jmp_buf env);

	.globl	k__setjmp
	.align	2
k__setjmp:
	str	r11,[r0]
	str	sp,[r0,#4]
	str	lr,[r0,#8]
	mov	r0,#0
	mov	pc,lr

# void longjmp(jmp_buf env, int v);

	.globl	k__longjmp
	.align	2
k__longjmp:
	ldr	r11,[r0]
	ldr	sp,[r0,#4]
	ldr	lr,[r0,#8]
	mov r0,r1
	cmp	r1,#0
	moveq r0,#1
	mov	pc,lr

