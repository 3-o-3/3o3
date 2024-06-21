/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                         main include file
 *
 *             15 May MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

#ifndef OS3_H_
#define OS3_H_ 1

#ifndef NULL
#define NULL (void*)0
#endif

typedef void os_void;

typedef char os_bool;
typedef char os_utf8;
typedef unsigned short os_utf16;

typedef unsigned char os_uint8;
typedef signed char os_int8;
typedef unsigned short os_uint16;
typedef signed short os_int16;
typedef unsigned int os_uint32;
typedef signed int os_int32;

#ifdef __AMD64__

typedef unsigned long long os_uint64;
typedef signed long long os_int64;
typedef long long os_intn;
typedef unsigned long long os_size;

#elif defined(__RPI400__) || defined(__I386__) || defined(WIN32)
/*typedef struct u64 {
    os_uint32 low;
    os_uint32 high;
} os_uint64;
typedef struct i64 {
    i32 low;
    i32 high;
} os_int64;
*/
typedef unsigned long long os_uint64;
typedef signed long long os_int64;
typedef long os_intn;
typedef unsigned long os_size;
#else
typedef long os_intn;
typedef  unsigned long os_size;
typedef  long os_intn;
#endif /*__RPI_400__ || __I386__*/

#ifndef assert
#define assert(A)                                              \
    if ((os_intn)(A) == 0) {                                       \
        OS_Assert();                                           \
        k__printf("\r\nAssert %s:%d\r\n", __FILE__, __LINE__); \
    }
#endif


void mmu__init();
os_intn pci__init();
void xhci__init();

#endif /* OS3_H_ */
