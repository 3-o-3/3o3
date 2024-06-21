
extern volatile os_uint8 *k__fb;
extern os_uint16 k__fb_height;
extern os_uint16 k__fb_width;
extern os_uint16 k__fb_pitch;
extern os_uint16 k__fb_bpp;

/*https://www.oreilly.com/library/view/understanding-the-linux/0596005652/ch04s02.html#understandlk-CHP-4-SECT-2.4*/

typedef struct reg_buf
{
    os_uint32 __ignored, fs, es, ds;
    os_uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    os_uint32 int_no, err_no;
    os_uint32 eip, cs, eflags, user_esp, user_ss;
} reg_buf[1];

typedef struct k__jmp_buf
{
    os_uint32 bp, sp, pc, extra[10];
} k__jmp_buf[1];

os_intn k__longjmp(k__jmp_buf, os_intn);
os_intn k__setjmp(k__jmp_buf);
