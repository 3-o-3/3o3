
/*
 *                         OS-3o3 Operating System
 *
 *                      12 may MMXXIV PUBLIC DOMAIN
 *           The authors disclaim copyright to this source code.
 *
 *
 */
#include "os3.h"

#define K__ALIGNED_HEAP_SIZE 0x10000

void* k__alloc_aligned(os_size size, os_size align, os_size boundary);
void k__printf(const os_utf8* str, ...);
void* k__memcpy(void* dest, void* src, os_size n);
os_intn k__memcmp(void* str1, void* str2, os_size n);
void* k__memset(void* str, os_intn c, os_size n);
void k__usleep(os_intn us);
void k__init();
void k__draw_char(os_intn x, os_intn y, os_uint32 c);
void k__puts(os_utf8* s);
void k__scroll(os_intn a);


#define K__r8(a) (*((volatile os_uint8*)(a)))
#define K__w8(a, b) ((*((volatile os_uint8*)(a)) = b))
#define K__r16(a) (*((volatile os_uint16*)(a)))
#define K__w16(a, b) ((*((volatile os_uint16*)(a)) = b))
#define K__r32(a) (*((volatile os_uint32*)(a)))
#define K__w32(a, b) ((*((volatile os_uint32*)(a)) = b))

void k__fb_init();
void fb_swap(void);
os_uint32* get_prog_end();
void syscall(os_uint32);

#ifdef __I386__
#include "arch/i386/defs.h"
#define k__heap_alloc(a, b) rtos__heap_alloc(a, b)
#endif
#ifdef __AMD64__
#include "arch/i386/defs.h"
void acpi__init(void*);
void load_gdt();
void paging_on();
os_uint64 paging_off();
#define k__heap_alloc(a, b) NULL
os_uint8 in8(os_intn port);
os_uint16 in16(os_intn port);
os_uint8 in8(os_intn port);
os_uint16 in16(os_intn port);
os_uint32 in32(os_intn port);
void out8(os_intn port, os_intn data);
void out16(os_intn port, os_intn data);
void out32(os_intn port, os_intn data);
#include "arch/amd64/internal.h"
#endif
#ifdef __RPI400__
#include "arch/rpi400/defs.h"
#define k__heap_alloc(a, b) rtos__heap_alloc(a, b)
void os__data_sync_barrier();
void os__data_mem_barrier();
#endif

#ifdef __I386__
os_uint8 in8(os_intn port);
os_uint16 in16(os_intn port);
os_uint8 in8(os_intn port);
os_uint16 in16(os_intn port);
os_uint32 in32(os_intn port);
void out8(os_intn port, os_intn data);
void out16(os_intn port, os_intn data);
void out32(os_intn port, os_intn data);
void beep();
#include "arch/i386/internal.h"
#endif /* __I386__ */

#ifdef __RPI400__
#include "arch/rpi400/internal.h"
os_intn kbhit();
os_intn getch();

#endif /*__RPI400__ */
