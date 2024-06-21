
void dma__init();
void gic400__irq_init();
unsigned int peek(unsigned int a);

void poke(unsigned int a, unsigned int v);
void gic400__irq_connect(int irq, void *handler, void *param);
void os__disable_irqs(void);
void os__enable_irqs(void);

void invalid_cache(char *a);
void sync_cache();
void pl011__write(char s);

os_uint32 mbox_call(os_uint32 a);

#define BUS_ADDRESS(a) \
    (((os_uint32)a) & ~0xC0000000) | GPU_MEM_BASE

typedef struct k__jmp_buf
{
    os_uint32 bp, sp, pc, extra[10];
} k__jmp_buf[1];

os_intn k__longjmp(k__jmp_buf, os_intn);
os_intn k__setjmp(k__jmp_buf);

void debug_putchar(char s);

void gic400__irq_vector();
void k__fb_init();
int main();

extern volatile os_uint8 *k__fb_base0;
extern volatile os_uint8 *k__fb_base1;
extern volatile os_uint8 *k__fb;
extern os_uint16 k__fb_height;
extern os_uint16 k__fb_width;
extern os_uint16 k__fb_pitch;
extern os_uint16 k__fb_bpp;

void fb_swap();