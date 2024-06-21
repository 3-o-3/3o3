/******************************************************************************
 *                        OS-3o3 operating system
 * 
 *                 Message box for the Raspberry PI400
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

#include "../../include/arch/rpi400/defs.h"

.global mbox
.align 4
mbox:
.rept 36
.long 0
.endr

.global mbox_call
mbox_call:
	push {r1,lr}
	ldr r1,=mbox
	and r1,r1,#0xFFFFFFF0
	and r0,r0,#0x0F
	orr r1,r1,r0

mbox_full:
	ldr r0,=MBOX_STATUS
	ldr r0,[r0]
	and r0,r0,#MBOX_FULL
	cmp r0,#0
	bne mbox_full	


	ldr r0,=MBOX_WRITE
	str r1,[r0]

	and r1,r1,#0xF
	ldr r0,=10
	push {r0}
	bl k__usleep
	pop {r0}

mbox_reply:
	ldr r0,=MBOX_STATUS
	ldr r0,[r0]
	and r0,r0,#MBOX_EMPTY
	cmp r0,#0
	bne mbox_reply	

	ldr r0,=MBOX_READ
	ldr r0,[r0]
	and r0,r0,#0xF
	cmp r0,r1
	bne mbox_reply	

	ldr r0,=mbox
	ldr r0,[r0,#4]
	cmp r0,#MBOX_RESPONSE
	beq mbox_success

	ldr r0,=0
	b mbox_ret

mbox_success: 
	ldr r0,=1
mbox_ret:
	pop {r1,lr}
	bx lr

.ltorg

