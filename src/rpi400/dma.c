/******************************************************************************
 *                       OS-3o3 operating system
 *
 *                  move memory using raspberry PI DMA
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 *
 *****************************************************************************/

#include "../../include/klib.h"

#define DMA_CHANNEL 2
#define DATA_CACHE_LINE_LENGTH_MIN 32

#define ARM_DMA_BASE (PERIPHERAL_BASE + 0x7000)
#define ARM_DMA_INT_STA (ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE (ARM_DMA_BASE + 0xFF0)
#define CS 0x0
#define CONBLK_AD 0x04
#define TI 0x08
#define SOURCE_AD 0x0C
#define DEST_AD 0x10
#define TXFR_LEN 0x14
#define STRIDE 0x18
#define NEXTCONBK 0x1C
#define DEBUG_V 0x20

#define CS_RESET (1 << 31)
#define CS_ABORT (1 << 30)
#define CS_INT (1 << 2)
#define CS_ERROR (1 << 8)
#define CS_END (1 << 1)
#define CS_ACTIVE (1 << 0)

struct TDMAControlBlock
{
    int nTransferInf;
    int nSourceAddress;
    int nDestAdd;
    int nTransferLength;
    int n2DModeStride;
    int nNextCtlBA;
    int nReserved[2];
};

extern volatile struct TDMAControlBlock dma_ctrl;

void dma__init()
{
    int cs;
    int t = 0;
    poke(ARM_DMA_ENABLE, peek(ARM_DMA_ENABLE) | (1 << DMA_CHANNEL));
    while (t < 100000)
    {
        peek(ARM_DMA_ENABLE);
        t++;
    }
    cs = ARM_DMA_BASE + ((DMA_CHANNEL) * 0x100);
    poke(cs, CS_RESET);
    while ((peek(cs) & CS_RESET) != 0)
    {
    }
}

/* https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf */
int dma__move(char *dest, char *src, unsigned int len)
{
    int cs;
    unsigned int l = len;
    char *a;

    cs = ARM_DMA_BASE + ((DMA_CHANNEL) * 0x100);

    dma_ctrl.nReserved[0] = 0;
    dma_ctrl.nReserved[1] = 0;

    l = len;
    a = dest;
    invalid_cache(a);
    while (l >= DATA_CACHE_LINE_LENGTH_MIN)
    {
        l -= DATA_CACHE_LINE_LENGTH_MIN;
        a += DATA_CACHE_LINE_LENGTH_MIN;
        invalid_cache(a);
    }
    l = len;
    a = src;
    invalid_cache(a);
    while (l >= DATA_CACHE_LINE_LENGTH_MIN)
    {
        l -= DATA_CACHE_LINE_LENGTH_MIN;
        a += DATA_CACHE_LINE_LENGTH_MIN;
        invalid_cache(a);
    }
    sync_cache();

    dma_ctrl.nTransferInf = 0x330;
    dma_ctrl.nSourceAddress = BUS_ADDRESS(src);
    dma_ctrl.nDestAdd = BUS_ADDRESS(dest);
    dma_ctrl.nTransferLength = len;
    dma_ctrl.n2DModeStride = 0;
    dma_ctrl.nNextCtlBA = 0;
    invalid_cache((char *)&dma_ctrl);
    poke(cs + CONBLK_AD, BUS_ADDRESS((char *)&dma_ctrl));

    poke(cs, CS_ACTIVE);

    while ((peek(cs) & CS_ACTIVE) != 0)
    {
    }
    l = len;
    a = dest;
    invalid_cache(a);
    while (l >= DATA_CACHE_LINE_LENGTH_MIN)
    {
        l -= DATA_CACHE_LINE_LENGTH_MIN;
        a += DATA_CACHE_LINE_LENGTH_MIN;
        invalid_cache(a);
    }
    return 0;
}
