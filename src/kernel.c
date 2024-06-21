
/******************************************************************************
 *                       OS-3o3 operating system
 *
 *                       Main kernel entry point
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

/*
 Size of io.sys is 384kB max due to limitation in io16.S

 */

#include "../include/klib.h"
#include "../include/rtos.h"


void knl__thread_main(void *arg)
{
    while (1)
    {
        k__printf("main thread\n");
        rtos__thread_sleep(100);
    }
}

void knl__thread_usb(void *arg)
{
    while (1)
    {
        k__printf("thread USB\n");
        xhci__init();
        rtos__thread_sleep(1000);
    }
}


int main(void)
{
    /* FIXME must find the end of kernel */
    os_uint32 programEnd = 0x01000000; 

    k__init();
    k__printf("\r\nStarting OS-3o3...");

    mmu__init();
    pci__init();

    k__printf(" @ 0x%x\r\n", k__fb);

    /*  https://wiki.osdev.org/Memory_Map_(x86) */
    if (programEnd < 0x00100000)
    {
        programEnd = 0x00100000; /*1MB*/
    }

    rtos__init((os_uint32 *)programEnd,
               RAM_EXTERNAL_SIZE - programEnd);

    rtos__thread_create("Main", knl__thread_main, NULL, 100, 4000);
    rtos__thread_create("USB", knl__thread_usb, NULL, 100, 40000);

    rtos__start();
    k__printf("End 32gears...\n");
    while (1)
    {
        k__printf(".");
    }
    return 0;
}
