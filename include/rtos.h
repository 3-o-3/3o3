/**
 * Modified MMXXIV for OS-3o3 by Jean-Marc Lienher
 */
/*--------------------------------------------------------------------
 * TITLE: Plasma Real Time Operating System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: rtos.h
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real Time Operating System
 *    Fully pre-emptive RTOS with support for:
 *       Heaps, Threads, Semaphores, Mutexes, Message Queues, and Timers.
 *--------------------------------------------------------------------*/
#ifndef RTOS_H
#define RTOS_H

#include "../include/os3.h"
#include "../include/rtos.h"

#define GEARS32 1
#define NO_MAIN 1
#define DISABLE_IRQ_SIM 1

/*Symmetric Multi-Processing*/
#define OS_CPU_COUNT 1

#define MemoryRead(A) (*(volatile os_uint32 *)(A))
#define MemoryWrite(A, V) *(volatile os_uint32 *)(A) = (V)

/***************** Assembly **************/
extern os_uint32 interrupt__enable(os_uint32 state);
extern void interrupt__init(void);

/***************** Heap ******************/
typedef struct OS_Heap_s OS_Heap_t;
#define HEAP_USER (OS_Heap_t *)0
#define HEAP_SYSTEM (OS_Heap_t *)1
#define HEAP_SMALL (OS_Heap_t *)2
#define HEAP_UI (OS_Heap_t *)3
OS_Heap_t *OS_HeapCreate(const char *name, void *memory, os_uint32 size);
void OS_HeapDestroy(OS_Heap_t *heap);
void *rtos__heap_alloc(OS_Heap_t *heap, os_intn bytes);
void rtos__heap_free(void *block);
void OS_HeapAlternate(OS_Heap_t *heap, OS_Heap_t *alternate);
void OS_HeapRegister(void *index, OS_Heap_t *heap);

/***************** Critical Sections *****************/
#if OS_CPU_COUNT <= 1
/*Single CPU*/
#define OS_CpuIndex() 0
#define OS_CriticalBegin() interrupt__enable(0)
#define OS_CriticalEnd(S) interrupt__enable(S)
#define OS_SpinLock() 0
#define OS_SpinUnlock(S) ((void)S)
#else
/*Symmetric multiprocessing*/
os_uint32 OS_CpuIndex(void);
#define OS_CriticalBegin() OS_SpinLock()
#define OS_CriticalEnd(S) OS_SpinUnlock(S)
os_uint32 OS_SpinLock(void);
void OS_SpinUnlock(os_uint32 state);
#endif

/***************** Thread *****************/
#ifdef WIN32
#define STACK_SIZE_MINIMUM (1024 * 8)
#else
#define STACK_SIZE_MINIMUM (1024 * 1)
#endif
#define STACK_SIZE_DEFAULT 1024 * 2
#undef THREAD_PRIORITY_IDLE
#define THREAD_PRIORITY_IDLE 0
#define THREAD_PRIORITY_MAX 255

typedef void (*OS_FuncPtr_t)(void *arg);
typedef struct OS_Thread_s OS_Thread_t;
OS_Thread_t *rtos__thread_create(const char *name,
                                 OS_FuncPtr_t funcPtr,
                                 void *arg,
                                 os_uint32 priority,
                                 os_uint32 stackSize);
void OS_ThreadExit(void);
OS_Thread_t *OS_ThreadSelf(void);
void rtos__thread_sleep(int ticks);
os_uint32 OS_ThreadTime(void);
void OS_ThreadInfoSet(OS_Thread_t *thread, os_uint32 index, void *info);
void *OS_ThreadInfoGet(OS_Thread_t *thread, os_uint32 index);
os_uint32 OS_ThreadPriorityGet(OS_Thread_t *thread);
void OS_ThreadPrioritySet(OS_Thread_t *thread, os_uint32 priority);
void OS_ThreadProcessId(OS_Thread_t *thread, os_uint32 processId, OS_Heap_t *heap);
void OS_ThreadCpuLock(OS_Thread_t *thread, int cpuIndex);

/***************** Semaphore **************/
#define OS_SUCCESS 0
#define OS_ERROR -1
#define OS_WAIT_FOREVER -1
#define OS_NO_WAIT 0
typedef struct OS_Semaphore_s OS_Semaphore_t;
OS_Semaphore_t *OS_SemaphoreCreate(const char *name, os_uint32 count);
void OS_SemaphoreDelete(OS_Semaphore_t *semaphore);
int OS_SemaphorePend(OS_Semaphore_t *semaphore, int ticks); /*tick ~= 10ms*/
void OS_SemaphorePost(OS_Semaphore_t *semaphore);

/***************** Mutex ******************/
typedef struct OS_Mutex_s OS_Mutex_t;
OS_Mutex_t *OS_MutexCreate(const char *name);
void OS_MutexDelete(OS_Mutex_t *semaphore);
void OS_MutexPend(OS_Mutex_t *semaphore);
void OS_MutexPost(OS_Mutex_t *semaphore);

/***************** MQueue *****************/
enum
{
    MESSAGE_TYPE_USER = 0,
    MESSAGE_TYPE_TIMER = 5
};
typedef struct OS_MQueue_s OS_MQueue_t;
OS_MQueue_t *OS_MQueueCreate(const char *name,
                             int messageCount,
                             int messageBytes);
void OS_MQueueDelete(OS_MQueue_t *mQueue);
int OS_MQueueSend(OS_MQueue_t *mQueue, void *message);
int OS_MQueueGet(OS_MQueue_t *mQueue, void *message, int ticks);

/***************** Job ********************/
typedef void (*JobFunc_t)(void *a0, void *a1, void *a2);
void OS_Job(JobFunc_t funcPtr, void *arg0, void *arg1, void *arg2);

/***************** Timer ******************/
typedef struct OS_Timer_s OS_Timer_t;
typedef void (*OS_TimerFuncPtr_t)(OS_Timer_t *timer, os_uint32 info);
OS_Timer_t *OS_TimerCreate(const char *name, OS_MQueue_t *mQueue, os_uint32 info);
void OS_TimerDelete(OS_Timer_t *timer);
void OS_TimerCallback(OS_Timer_t *timer, OS_TimerFuncPtr_t callback);
void OS_TimerStart(OS_Timer_t *timer, os_uint32 ticks, os_uint32 ticksRestart);
void OS_TimerStop(OS_Timer_t *timer);

/***************** ISR ********************/
/*#define STACK_EPC 88/4*/
struct reg_buf;
void rtos__interrupt_service_routine(os_uint32 status, struct reg_buf *stack);
void OS_InterruptRegister(os_uint32 mask, OS_FuncPtr_t funcPtr);
os_uint32 OS_InterruptStatus(void);
os_uint32 OS_InterruptMaskSet(os_uint32 mask);
os_uint32 OS_InterruptMaskClear(os_uint32 mask);

/***************** Init ******************/
void rtos__init(os_uint32 *heapStorage, os_uint32 bytes);
void OS_InitSimulation(void);
void rtos__start(void);
void OS_Assert(void);
void MainThread(void *Arg);

/***************** debug ******************/
void k__printf(const char *format, ...);

typedef struct OS_Api_s
{
    os_uint32 version;
    void *next_version;
    void *(*OS_HeapMalloc)(OS_Heap_t *heap, int bytes);
    void (*OS_HeapFree)(void *block);
    OS_Thread_t *(*rtos__thread_create)(const char *name, OS_FuncPtr_t funcPtr, void *arg, os_uint32 priority, os_uint32 stackSize);
    void (*OS_ThreadExit)(void);
    OS_Thread_t *(*OS_ThreadSelf)(void);
    void (*rtos__thread_sleep)(int ticks);
    OS_MQueue_t *(*OS_MQueueCreate)(const char *name, int messageCount, int messageBytes);
    void (*OS_MQueueDelete)(OS_MQueue_t *mQueue);
    int (*OS_MQueueSend)(OS_MQueue_t *mQueue, void *message);
    int (*OS_MQueueGet)(OS_MQueue_t *mQueue, void *message, int ticks);
    void (*k__printf)(const char *format, ...);
} OS_Api_t;

#endif /* RTOS_H */
