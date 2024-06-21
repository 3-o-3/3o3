/******************************************************************************
 *                        OS-3o3 operating system
 * 
 *                    64bit boot for the Raspberry PI400
 *                     it then switches to 32 bit mode
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

#include "../../include/arch/rpi400/defs.h"
#define RPI_BASE        0xFE000000 /* RPi 4 */
#define GPIO_BASE       (RPI_BASE | 0x200000)
#define GPFSEL2         (GPIO_BASE | 0x8) // GPIO select 2
#define   GPIO_ALT4          (3)      // GPIO is Alternate4 (011)



	.arch armv8.2-a
	.section .text
	.align	2
	.globl	_start64
	.globl	_start
_start:
_start64:

	MRS      x0, MPIDR_EL1		// Read Affinity register
	AND      x0, x0, #0xFFFF	// Mask off Aff0/Aff1 fields
	CBZ      x0, boot			// Branch to boot if Aff0/Aff1 are zero (Core 0 of Cluster 0)
sleep:							// Else put processor to sleep
	WFI
	B        sleep

boot:
//https://github.com/sean-lawless/computersystems/tree/master/Lab4%20LED
	// GPIO 22-27 alternate function 4 for JTAG
	ldr x0,=GPFSEL2
	ldr w1,[x0]
	and w1,w1,#~(7 << 6); //gpio22
	orr w1,w1,#GPIO_ALT4 << 6; //alt4 ARM_TRST
	and w1,w1,#~(7 << 9); //gpio23
	orr w1,w1,#GPIO_ALT4 << 9; //alt4 ARM_RTCK
	and w1,w1,#~(7 << 12); //gpio24
	orr w1,w1,#GPIO_ALT4 << 12; //alt4 ARM_TDO
	and w1,w1,#~(7 << 15); //gpio25
	orr w1,w1,#GPIO_ALT4 << 15; //alt4 ARM_TCK
	and w1,w1,#~(7 << 18); //gpio26
	orr w1,w1,#GPIO_ALT4 << 18; //alt4 ARM_TDI
	and w1,w1,#~(7 << 21); //gpio27
	orr w1,w1,#GPIO_ALT4 << 21; //alt4 ARM_TMS
	str w1,[x0]

	

	adrp x0,_start64
	mov sp,x0
	msr CPTR_EL2, xzr

	mrs x0, HCR_EL2
	and x0,x0,#~0x80000000ULL // set EL1 to aarch32
	/*orr x0,x0,#(1<<56)
	orr x0,x0,#(1<<53)
	orr x0,x0,#(1<<47)
	orr x0,x0,#(1<<41)
	orr x0,x0,#(1<<40)
	orr x0,x0,#(1<<39)
	orr x0,x0,#(1)*/
	msr HCR_EL2, x0
	b after
	.pool
	.text
after:
	msr SCTLR_EL1, xzr
	msr SCTLR_EL2, xzr

	mov x1, #(0x13 + 0x40 + 0x80) // Set return level to EL1
	msr SPSR_EL2,x1	
	isb
	adr x0,end
	msr ELR_EL2,x0
	isb
	eret
end: