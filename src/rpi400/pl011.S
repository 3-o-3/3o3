
/******************************************************************************
 *                        OS-3o3 operating system
 * 
 *                  serial port for the Raspberry PI400
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/


#include "../../include/arch/rpi400/defs.h"

.global pl011__init
pl011__init:
pl011_init_ret:
	bx lr

.global pl011__write
pl011__write:
	push {r1, r11,lr}

.global pl011_not_ready
pl011_not_ready:
	ldr r1,=PL011_FR
	ldr r1,[r1]
	and r1,r1,#PL011_TXFF
	cmp r1,#0
	bne pl011_not_ready

	ldr r1,=PL011_DR
	str r0,[r1]	

pl011_write_ret:
	pop {r1,r11,pc}


.global pl011__read
pl011__read:
	push {r1,r11,lr}
	ldr r1,=PL011_FR
	ldr r1,[r1]
	and r1,r1,#PL011_RXFE
	cmp r1,#0
	bne pl011_no_data

	ldr r1,=PL011_DR
	ldr r0,[r1]
	and r0,r0,#0xFF	
	b pl011_ret

pl011_no_data:
	ldr r0,=-1
pl011_ret:
	pop {r1,r11,pc}


.ltorg
