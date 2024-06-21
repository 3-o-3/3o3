/**
 * Modified MMXXIV by Jean-Marc Lienher
 */
/*--------------------------------------------------------------------
 * TITLE: Plasma Real Time Operating System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: rtos.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real Time Operating System
 *    Fully pre-emptive RTOS with support for:
 *       Heaps, Threads, Semaphores, Mutexes, Message Queues, and Timers.
 *    This file tries to be hardware independent except for calls to:
 *       MemoryRead() and MemoryWrite() for interrupts.
 *    Support for multiple CPUs using symmetric multiprocessing.
 *--------------------------------------------------------------------*/

#include "../include/rtos.h"
#include "../include/klib.h"

#define HEAP_MAGIC 0x1234abcd
#define THREAD_MAGIC 0x4321abcd
#define SEM_RESERVED_COUNT 2
#define INFO_COUNT 4
#define HEAP_COUNT 8

#define PRINTF_DEBUG(STRING, A, B)
/*#define PRINTF_DEBUG(STRING, A, B) k__printf(STRING, A, B)*/

/*************** Structures ***************/

typedef struct HeapNode_s
{
    struct HeapNode_s *next;
    int size;
} HeapNode_t;

struct OS_Heap_s
{
    os_uint32 magic;
    const char *name;
    OS_Semaphore_t *semaphore;
    HeapNode_t *available;
    HeapNode_t base;
    int count;
    struct OS_Heap_s *alternate;
};
/*typedef struct OS_Heap_s OS_Heap_t;*/

typedef enum
{
    THREAD_PEND = 0,   /*Thread in semaphore's linked list*/
    THREAD_READY = 1,  /*Thread in ThreadHead linked list*/
    THREAD_RUNNING = 2 /*Thread == ThreadCurrent[cpu]*/
} OS_ThreadState_e;

struct OS_Thread_s
{

    const char *name;                 /*Name of thread*/
    OS_ThreadState_e state;           /*Pending, ready, or running*/
    int cpuIndex;                     /*Which CPU is running the thread*/
    int cpuLock;                      /*Lock the thread to a specific CPU*/
    k__jmp_buf env;                    /*Registers saved during context*/
    OS_FuncPtr_t funcPtr;             /*First function called*/
    void *arg;                        /*Argument to first function called*/
    os_uint32 priority;               /*Priority of thread (0=low, 255=high)*/
    os_uint32 ticksTimeout;           /*Tick value when semaphore pend times out*/
    void *info[INFO_COUNT];           /*User storage*/
    OS_Semaphore_t *semaphorePending; /*Semaphore thread is blocked on*/
    int returnCode;                   /*Return value from semaphore pend*/
    os_uint32 processId;              /*Process ID if using MMU*/
    OS_Heap_t *heap;                  /*Heap used if no heap specified*/
    struct OS_Thread_s *next;         /*Linked list of threads by priority*/
    struct OS_Thread_s *prev;
    struct OS_Thread_s *nextTimeout; /*Linked list of threads by timeout*/
    struct OS_Thread_s *prevTimeout;
    os_uint32 magic[1]; /*Bottom of stack to detect stack overflow*/
};
/*typedef struct OS_Thread_s OS_Thread_t;*/

struct OS_Semaphore_s
{
    const char *name;
    struct OS_Thread_s *threadHead; /*threads pending on semaphore*/
    int count;
};
/*typedef struct OS_Semaphore_s OS_Semaphore_t;*/

struct OS_Mutex_s
{
    OS_Semaphore_t *semaphore;
    OS_Thread_t *thread;
    os_uint32 priorityRestore;
    int count;
};
/*typedef struct OS_Mutex_s OS_Mutex_t;*/

struct OS_MQueue_s
{
    const char *name;
    OS_Semaphore_t *semaphore;
    int count, size, used, read, write;
};
/*typedef struct OS_MQueue_s OS_MQueue_t;*/

struct OS_Timer_s
{
    const char *name;
    struct OS_Timer_s *next, *prev;
    os_uint32 ticksTimeout;
    os_uint32 ticksRestart;
    int active;
    OS_TimerFuncPtr_t callback;
    OS_MQueue_t *mqueue;
    os_uint32 info;
};
/*typedef struct OS_Timer_s OS_Timer_t;*/

os_uint32 irq_mask_ = 0;
os_uint32 irq_status_ = IRQ_COUNTER18_NOT;
os_uint32 *irq_mask = &irq_mask_;
os_uint32 *irq_status = &irq_status_;

/*************** Globals ******************/
static OS_Heap_t *HeapArray[HEAP_COUNT];
static int InterruptInside[OS_CPU_COUNT];
static int ThreadNeedReschedule[OS_CPU_COUNT];
static OS_Thread_t *ThreadCurrent[OS_CPU_COUNT]; /*Currently running thread(s)*/
static OS_Thread_t *ThreadHead = NULL;           /*Linked list of threads sorted by priority*/
static OS_Thread_t *TimeoutHead = NULL;          /*Linked list of threads sorted by timeout*/
static int ThreadSwapEnabled;
static os_uint32 ThreadTime; /*Number of ~10ms ticks since reboot*/
static void *NeedToFree;     /*Closed but not yet freed thread*/
static OS_Semaphore_t SemaphoreReserved[SEM_RESERVED_COUNT];
static OS_Semaphore_t *SemaphoreSleep;
static OS_Semaphore_t *SemaphoreRelease; /*Protects NeedToFree*/
static OS_Semaphore_t *SemaphoreLock;
static OS_Semaphore_t *SemaphoreTimer;
static OS_Timer_t *TimerHead; /*Linked list of timers sorted by timeout*/
static OS_FuncPtr_t Isr[32];  /*Interrupt service routines*/
#if defined(WIN32) && OS_CPU_COUNT > 1
static unsigned int Registration[OS_CPU_COUNT];
#endif

/***************** Heap *******************/
/******************************************/
OS_Heap_t *OS_HeapCreate(const char *name, void *memory, os_uint32 size)
{
    OS_Heap_t *heap;

    assert(((os_uint32)memory & 3) == 0);
    heap = (OS_Heap_t *)memory;
    heap->magic = HEAP_MAGIC;
    heap->name = name;
    heap->semaphore = OS_SemaphoreCreate(name, 1);
    heap->available = (HeapNode_t *)(heap + 1);
    heap->available->next = &heap->base;
    heap->available->size = (size - sizeof(OS_Heap_t)) / sizeof(HeapNode_t);
    heap->base.next = heap->available;
    heap->base.size = 0;
    heap->count = 0;
    heap->alternate = NULL;
    return heap;
}

/******************************************/
void OS_HeapDestroy(OS_Heap_t *heap)
{
    OS_SemaphoreDelete(heap->semaphore);
}

/******************************************/
/* Modified from Kernighan & Ritchie "The C Programming Language" */
void *rtos__heap_alloc(OS_Heap_t *heap, os_intn bytes)
{
    HeapNode_t *node, *prevp;
    int nunits;

    if (heap == NULL && OS_ThreadSelf())
        heap = OS_ThreadSelf()->heap;
    if ((os_uint32)heap < HEAP_COUNT)
        heap = HeapArray[(int)heap];
    nunits = (bytes + sizeof(HeapNode_t) - 1) / sizeof(HeapNode_t) + 1;
    OS_SemaphorePend(heap->semaphore, OS_WAIT_FOREVER);
    prevp = heap->available;
    for (node = prevp->next;; prevp = node, node = node->next)
    {
        if (node->size >= nunits) /*Big enough? */
        {
            if (node->size == nunits)
            { /* Exactly*/
                prevp->next = node->next;
            }
            else
            { /*Allocate tail end*/
                node->size -= nunits;
                node += node->size;
                node->size = nunits;
            }

            heap->available = prevp;
            node->next = (HeapNode_t *)heap;
            PRINTF_DEBUG("malloc(%d, %d)\n", node->size * sizeof(HeapNode_t), heap->count);
            ++heap->count;
            OS_SemaphorePost(heap->semaphore);
            /*k__printf("OS_HeapMalloc(%d)=0x%x\n", bytes, (int)(node+1));*/
            return (void *)(node + 1);
        }
        if (node == heap->available) /*Wrapped around free list*/
        {
            OS_SemaphorePost(heap->semaphore);
            if (heap->alternate)
                return rtos__heap_alloc(heap->alternate, bytes);
            return NULL;
        }
    }
}

/******************************************/
/*Modified from K&R*/
void rtos__heap_free(void *block)
{
    OS_Heap_t *heap;
    HeapNode_t *bp, *node;

    /*k__printf("OS_HeapFree(0x%x)\n", block);*/
    if (block == NULL)
        return;
    bp = (HeapNode_t *)block - 1; /*point to block header*/
    heap = (OS_Heap_t *)bp->next;
    assert(heap->magic == HEAP_MAGIC);
    if (heap->magic != HEAP_MAGIC)
        return;
    OS_SemaphorePend(heap->semaphore, OS_WAIT_FOREVER);
    --heap->count;
    PRINTF_DEBUG("free(%d, %d)\n", bp->size * sizeof(HeapNode_t), heap->count);
    for (node = heap->available; !(node < bp && bp < node->next); node = node->next)
    {
        if (node >= node->next && (bp > node || bp < node->next))
            break; /*freed block at start or end of area*/
    }

    if (bp + bp->size == node->next) /*join to upper*/
    {
        bp->size += node->next->size;
        bp->next = node->next->next;
    }
    else
    {
        bp->next = node->next;
    }

    if (node + node->size == bp) /*join to lower*/
    {
        node->size += bp->size;
        node->next = bp->next;
    }
    else
        node->next = bp;
    heap->available = node;
    OS_SemaphorePost(heap->semaphore);
}

/******************************************/
void OS_HeapAlternate(OS_Heap_t *heap, OS_Heap_t *alternate)
{
    heap->alternate = alternate;
}

/******************************************/
void OS_HeapRegister(void *index, OS_Heap_t *heap)
{
    if ((os_uint32)index < HEAP_COUNT)
        HeapArray[(int)index] = heap;
}

/***************** Thread *****************/
/******************************************/
/*Linked list of threads sorted by priority
 The linked list is either ThreadHead (ready to run threads not including
 the currently running thread) or a list of threads waiting on a semaphore.
 Must be called with interrupts disabled*/
static void OS_ThreadPriorityInsert(OS_Thread_t **head, OS_Thread_t *thread)
{
    OS_Thread_t *node, *prev;

    prev = NULL;
    for (node = *head; node; node = node->next)
    {
        if (node->priority < thread->priority)
            break;
        prev = node;
    }

    if (prev == NULL)
    {
        thread->next = *head;
        thread->prev = NULL;
        if (*head)
            (*head)->prev = thread;
        *head = thread;
    }
    else
    {
        if (prev->next)
            prev->next->prev = thread;
        thread->next = prev->next;
        thread->prev = prev;
        prev->next = thread;
    }

    assert(ThreadHead);
    thread->state = THREAD_READY;
}

/******************************************/
/* Must be called with interrupts disabled*/
static void OS_ThreadPriorityRemove(OS_Thread_t **head, OS_Thread_t *thread)
{
    assert(thread->magic[0] == THREAD_MAGIC); /*check stack overflow*/
    if (thread->prev == NULL)
        *head = thread->next;
    else
        thread->prev->next = thread->next;
    if (thread->next)
        thread->next->prev = thread->prev;
    thread->next = NULL;
    thread->prev = NULL;
}

/******************************************/
/*Linked list of threads sorted by timeout value
 Must be called with interrupts disabled*/
static void OS_ThreadTimeoutInsert(OS_Thread_t *thread)
{
    OS_Thread_t *node, *prev;
    int diff;
    prev = NULL;
    for (node = TimeoutHead; node; node = node->nextTimeout)
    {
        diff = thread->ticksTimeout - node->ticksTimeout;
        if (diff <= 0)
            break;
        prev = node;
    }

    if (prev == NULL)
    {
        thread->nextTimeout = TimeoutHead;
        thread->prevTimeout = NULL;
        if (TimeoutHead)
            TimeoutHead->prevTimeout = thread;
        TimeoutHead = thread;
    }
    else
    {
        if (prev->nextTimeout)
            prev->nextTimeout->prevTimeout = thread;
        thread->nextTimeout = prev->nextTimeout;
        thread->prevTimeout = prev;
        prev->nextTimeout = thread;
    }
}

/******************************************/
/*Must be called with interrupts disabled */
static void OS_ThreadTimeoutRemove(OS_Thread_t *thread)
{
    if (thread->prevTimeout == NULL && TimeoutHead != thread)
        return; /*not in list*/
    if (thread->prevTimeout == NULL)
        TimeoutHead = thread->nextTimeout;
    else
        thread->prevTimeout->nextTimeout = thread->nextTimeout;
    if (thread->nextTimeout)
        thread->nextTimeout->prevTimeout = thread->prevTimeout;
    thread->nextTimeout = NULL;
    thread->prevTimeout = NULL;
}

static void OS_ThreadInit(void *arg);

/******************************************/
/*Loads highest priority thread from the ThreadHead linked list
 The currently running thread isn't in the ThreadHead list
 Must be called with interrupts disabled*/
static void OS_ThreadReschedule(int roundRobin)
{
    OS_Thread_t *threadNext, *threadCurrent;
    int rc, cpuIndex = OS_CpuIndex();

    if (ThreadSwapEnabled == 0 || InterruptInside[cpuIndex])
    {
        ThreadNeedReschedule[cpuIndex] |= 2 + roundRobin; /*Reschedule later*/
        return;
    }
    ThreadNeedReschedule[cpuIndex] = 0;

    /* Determine which thread should run*/
    threadNext = ThreadHead;
    while (threadNext && threadNext->cpuLock != -1 && threadNext->cpuLock != cpuIndex)
        threadNext = threadNext->next; /*Skip CPU locked threads*/
    if (threadNext == NULL)
        return;
    threadCurrent = ThreadCurrent[cpuIndex];

    if (threadCurrent == NULL || threadCurrent->state == THREAD_PEND || threadCurrent->priority < threadNext->priority || (roundRobin && threadCurrent->priority == threadNext->priority))
    {

        /*Swap threads*/
        ThreadCurrent[cpuIndex] = threadNext;
        if (threadCurrent)
        {

            assert(threadCurrent->magic[0] == THREAD_MAGIC); /*check stack overflow*/
            if (threadCurrent->state == THREAD_RUNNING)
                OS_ThreadPriorityInsert(&ThreadHead, threadCurrent);
            PRINTF_DEBUG("Pause(%d,%s) ", OS_CpuIndex(), threadCurrent->name);
            PRINTF_DEBUG(ThreadCurrent[OS_CpuIndex()]->name, "", "");

            rc = k__setjmp(threadCurrent->env); /*ANSI C call to save registers*/

            if (rc == 1)
            {
                PRINTF_DEBUG("Resume(%d,%s) ", OS_CpuIndex(),
                             ThreadCurrent[OS_CpuIndex()]->name);
                PRINTF_DEBUG(ThreadCurrent[OS_CpuIndex()]->name, "", "");
                return; /* Returned from longjmp()*/
            }
        }

        /*Remove the new running thread from the ThreadHead linked list*/
        threadNext = ThreadCurrent[OS_CpuIndex()]; /*removed warning*/
        assert(threadNext->state == THREAD_READY);
        OS_ThreadPriorityRemove(&ThreadHead, threadNext);
        threadNext->state = THREAD_RUNNING;
        threadNext->cpuIndex = OS_CpuIndex();
        k__longjmp(threadNext->env, 1); /*ANSI C call to restore registers*/
    }
}

/******************************************/
void OS_ThreadCpuLock(OS_Thread_t *thread, int cpuIndex)
{
    thread->cpuLock = cpuIndex;
    if (thread == OS_ThreadSelf() && cpuIndex != (int)OS_CpuIndex())
        rtos__thread_sleep(1);
}

/******************************************/
static void OS_ThreadInit(void *arg)
{
    OS_FuncPtr_t f;
    os_uint32 cpuIndex = OS_CpuIndex();
    (void)arg;

    PRINTF_DEBUG("Starting(%d,%s) \n", cpuIndex, OS_ThreadSelf()->name);
    PRINTF_DEBUG(OS_ThreadSelf()->name, "", "");
    OS_CriticalEnd(1);
    f = ThreadCurrent[cpuIndex]->funcPtr;
    f(ThreadCurrent[cpuIndex]->arg);
    OS_ThreadExit();
}

/******************************************/
/* Stops warning "argument X might be clobbered by `longjmp'"*/
static void OS_ThreadRegsInit(k__jmp_buf env)
{

    k__setjmp(env); /* ANSI C call to save registers*/
}

/******************************************/
OS_Thread_t *rtos__thread_create(const char *name,
                                 OS_FuncPtr_t funcPtr,
                                 void *arg,
                                 os_uint32 priority,
                                 os_uint32 stackSize)
{
    OS_Thread_t *thread = NULL;
    os_uint8 *stack;
    struct k__jmp_buf *env;
    os_uint32 state;
   
    OS_SemaphorePend(SemaphoreRelease, OS_WAIT_FOREVER);
    if (NeedToFree)
        rtos__heap_free(NeedToFree);
    NeedToFree = NULL;
    OS_SemaphorePost(SemaphoreRelease);

    if (stackSize == 0)
        stackSize = STACK_SIZE_DEFAULT;
    if (stackSize < STACK_SIZE_MINIMUM)
        stackSize = STACK_SIZE_MINIMUM;
    thread = (OS_Thread_t *)rtos__heap_alloc(NULL, sizeof(OS_Thread_t) + stackSize);

    assert(thread);
    if (thread == NULL)
        return NULL;

    k__memset(thread, 0, sizeof(OS_Thread_t));
    stack = (os_uint8 *)(thread + 1);
    k__memset(stack, 0xcd, stackSize);

    thread->name = name;
    thread->state = THREAD_READY;
    thread->cpuLock = -1;
    thread->funcPtr = funcPtr;
    thread->arg = arg;
    thread->priority = priority;
    thread->semaphorePending = NULL;
    thread->returnCode = 0;
    if (OS_ThreadSelf())
    {
        thread->processId = OS_ThreadSelf()->processId;
        thread->heap = OS_ThreadSelf()->heap;
    }
    else
    {
        thread->processId = 0;
        thread->heap = NULL;
    }
    thread->next = NULL;
    thread->prev = NULL;
    thread->nextTimeout = NULL;
    thread->prevTimeout = NULL;
    thread->magic[0] = THREAD_MAGIC;

    OS_ThreadRegsInit(thread->env);

    env = thread->env;
    env->pc = (os_uint32)OS_ThreadInit;

    env->bp = (os_uint32)stack + stackSize - 24; /*minimum stack frame size*/
    env->sp = env->bp;

    /*Add thread to linked list of ready to run threads*/

    state = OS_CriticalBegin();
    OS_ThreadPriorityInsert(&ThreadHead, thread);

    OS_ThreadReschedule(0); /*run highest priority thread*/

    OS_CriticalEnd(state);
    return thread;
}

/******************************************/
void OS_ThreadExit(void)
{
    os_uint32 state, cpuIndex = OS_CpuIndex();

    for (;;)
    {
        /*Free the memory for closed but not yet freed threads*/
        OS_SemaphorePend(SemaphoreRelease, OS_WAIT_FOREVER);
        if (NeedToFree)
            rtos__heap_free(NeedToFree);
        NeedToFree = NULL;
        OS_SemaphorePost(SemaphoreRelease);

        state = OS_CriticalBegin();
        if (NeedToFree)
        {
            OS_CriticalEnd(state);
            continue;
        }
        ThreadCurrent[cpuIndex]->state = THREAD_PEND;
        NeedToFree = ThreadCurrent[cpuIndex];
        OS_ThreadReschedule(0);
        OS_CriticalEnd(state);
    }
}

/******************************************/
/*Return currently running thread*/
OS_Thread_t *OS_ThreadSelf(void)
{
    return ThreadCurrent[OS_CpuIndex()];
}

/******************************************/
/*Sleep for ~10 msecs ticks*/
void rtos__thread_sleep(int ticks)
{
    OS_SemaphorePend(SemaphoreSleep, ticks);
}

/******************************************/
/*Return the number of ~10 msecs ticks since reboot*/
os_uint32 OS_ThreadTime(void)
{
    return ThreadTime;
}

/******************************************/
/* Save thread unique values */
void OS_ThreadInfoSet(OS_Thread_t *thread, os_uint32 index, void *Info)
{
    if (index < INFO_COUNT)
        thread->info[index] = Info;
}

/******************************************/
void *OS_ThreadInfoGet(OS_Thread_t *thread, os_uint32 index)
{
    if (index < INFO_COUNT)
        return thread->info[index];
    return NULL;
}

/******************************************/
os_uint32 OS_ThreadPriorityGet(OS_Thread_t *thread)
{
    return thread->priority;
}

/******************************************/
void OS_ThreadPrioritySet(OS_Thread_t *thread, os_uint32 priority)
{
    os_uint32 state;
    state = OS_CriticalBegin();
    thread->priority = priority;
    if (thread->state == THREAD_READY)
    {
        OS_ThreadPriorityRemove(&ThreadHead, thread);
        OS_ThreadPriorityInsert(&ThreadHead, thread);
        OS_ThreadReschedule(0);
    }
    OS_CriticalEnd(state);
}

/******************************************/
void OS_ThreadProcessId(OS_Thread_t *thread, os_uint32 processId, OS_Heap_t *heap)
{
    thread->processId = processId;
    thread->heap = heap;
}

/******************************************/
/*Must be called with interrupts disabled every ~10 msecs */
/*Will wake up threads that have timed out waiting on a semaphore*/
static void OS_ThreadTick(void *Arg)
{
    OS_Thread_t *thread;
    OS_Semaphore_t *semaphore;
    int diff;

    ++ThreadTime; /* Number of ~10 msec ticks since reboot*/
    while (TimeoutHead)
    {
        thread = TimeoutHead;
        diff = ThreadTime - thread->ticksTimeout;
        if (diff < 0)
            break;

        /*The thread has timed out waiting for a semaphore*/
        OS_ThreadTimeoutRemove(thread);
        semaphore = thread->semaphorePending;
        ++semaphore->count;
        thread->semaphorePending = NULL;
        thread->returnCode = -1;

        OS_ThreadPriorityRemove(&semaphore->threadHead, thread);
        OS_ThreadPriorityInsert(&ThreadHead, thread);
    }
    OS_ThreadReschedule(1); /*Run highest priority thread*/
}

/***************** Semaphore **************/
/******************************************/
/*Create a counting semaphore*/
OS_Semaphore_t *OS_SemaphoreCreate(const char *name, os_uint32 count)
{
    OS_Semaphore_t *semaphore;
    static int semCount = 0;

    if (semCount < SEM_RESERVED_COUNT)
        semaphore = &SemaphoreReserved[semCount++]; /*Heap not ready yet*/
    else
        semaphore = (OS_Semaphore_t *)rtos__heap_alloc(HEAP_SYSTEM, sizeof(OS_Semaphore_t));
    assert(semaphore);
    if (semaphore == NULL)
        return NULL;

    semaphore->name = name;
    semaphore->threadHead = NULL;
    semaphore->count = count;
    return semaphore;
}

/******************************************/
void OS_SemaphoreDelete(OS_Semaphore_t *semaphore)
{
    while (semaphore->threadHead)
        OS_SemaphorePost(semaphore);
    rtos__heap_free(semaphore);
}

/******************************************/
/*Sleep the number of ticks (~10ms) until the semaphore is acquired */
int OS_SemaphorePend(OS_Semaphore_t *semaphore, int ticks)
{
    os_uint32 state, cpuIndex;
    OS_Thread_t *thread;
    int returnCode = 0;

    PRINTF_DEBUG("SemPend(%d,%s) ", OS_CpuIndex(), semaphore->name);
    assert(semaphore);
    cpuIndex = OS_CpuIndex();
    assert(InterruptInside[cpuIndex] == 0);
    state = OS_CriticalBegin(); /*Disable interrupts*/
    if (--semaphore->count < 0)
    {
        /*Semaphore not available*/
        if (ticks == 0)
        {
            ++semaphore->count;
            OS_CriticalEnd(state);
            return -1;
        }

        /*Need to sleep until the semaphore is available*/
        cpuIndex = OS_CpuIndex();
        thread = ThreadCurrent[cpuIndex];
        assert(thread);
        thread->semaphorePending = semaphore;
        thread->ticksTimeout = ticks + OS_ThreadTime();

        /*FYI: The current thread isn't in the ThreadHead linked list
         Place the thread into a sorted linked list of pending threads*/
        OS_ThreadPriorityInsert(&semaphore->threadHead, thread);
        thread->state = THREAD_PEND;
        if (ticks != OS_WAIT_FOREVER)
            OS_ThreadTimeoutInsert(thread); /*Check every ~10ms for timeouts*/
        assert(ThreadHead);
        OS_ThreadReschedule(0);          /*Run highest priority thread*/
        returnCode = thread->returnCode; /* Will be -1 if timed out*/
    }
    OS_CriticalEnd(state); /*Re-enable interrupts*/
    return returnCode;
}

/******************************************/
/*Release a semaphore and possibly wake up a blocked thread*/
void OS_SemaphorePost(OS_Semaphore_t *semaphore)
{
    os_uint32 state;
    OS_Thread_t *thread;

    /*PRINTF_DEBUG("SemPost(%d,%s) ", OS_CpuIndex(), semaphore->name);*/
    assert(semaphore);
    state = OS_CriticalBegin();
    if (++semaphore->count <= 0)
    {
        /*Wake up a thread that was waiting for this semaphore*/
        thread = semaphore->threadHead;
        OS_ThreadTimeoutRemove(thread);
        OS_ThreadPriorityRemove(&semaphore->threadHead, thread);
        OS_ThreadPriorityInsert(&ThreadHead, thread);
        thread->semaphorePending = NULL;
        thread->returnCode = 0;
        OS_ThreadReschedule(0);
    }
    OS_CriticalEnd(state);
}

/***************** Mutex ******************/
/******************************************/
/*Create a mutex (a thread aware semaphore)*/
OS_Mutex_t *OS_MutexCreate(const char *name)
{
    OS_Mutex_t *mutex;

    mutex = (OS_Mutex_t *)rtos__heap_alloc(HEAP_SYSTEM, sizeof(OS_Mutex_t));
    if (mutex == NULL)
        return NULL;
    mutex->semaphore = OS_SemaphoreCreate(name, 1);
    if (mutex->semaphore == NULL)
        return NULL;
    mutex->thread = NULL;
    mutex->count = 0;
    return mutex;
}

/******************************************/
void OS_MutexDelete(OS_Mutex_t *mutex)
{
    OS_SemaphoreDelete(mutex->semaphore);
    rtos__heap_free(mutex);
}

/******************************************/
void OS_MutexPend(OS_Mutex_t *mutex)
{
    OS_Thread_t *thread;
    os_uint32 state;

    assert(mutex);
    thread = OS_ThreadSelf();
    if (thread == mutex->thread)
    {
        ++mutex->count;
        return;
    }

    state = OS_CriticalBegin();
    /*Priority inheritance to prevent priority inversion*/
    if (mutex->thread && mutex->thread->priority < thread->priority)
        OS_ThreadPrioritySet(mutex->thread, thread->priority);

    OS_SemaphorePend(mutex->semaphore, OS_WAIT_FOREVER);
    mutex->priorityRestore = thread->priority;
    mutex->thread = thread;
    mutex->count = 1;
    OS_CriticalEnd(state);
}

/******************************************/
void OS_MutexPost(OS_Mutex_t *mutex)
{
    OS_Thread_t *thread;
    os_uint32 state, priorityRestore;

    thread = OS_ThreadSelf();
    assert(mutex);
    assert(mutex->thread == thread);
    assert(mutex->count > 0);
    if (--mutex->count <= 0)
    {
        state = OS_CriticalBegin();
        mutex->thread = NULL;
        priorityRestore = mutex->priorityRestore;
        OS_SemaphorePost(mutex->semaphore);
        if (priorityRestore < thread->priority)
            OS_ThreadPrioritySet(thread, priorityRestore);
        OS_CriticalEnd(state);
    }
}

/***************** MQueue *****************/
/******************************************/
/*Create a message queue*/
OS_MQueue_t *OS_MQueueCreate(const char *name,
                             int messageCount,
                             int messageBytes)
{
    OS_MQueue_t *queue;
    int size;

    assert((messageBytes & 3) == 0);
    size = messageBytes / sizeof(os_uint32);
    queue = (OS_MQueue_t *)rtos__heap_alloc(HEAP_SYSTEM, sizeof(OS_MQueue_t) + messageCount * size * 4);
    if (queue == NULL)
        return queue;
    queue->name = name;
    queue->semaphore = OS_SemaphoreCreate(name, 0);
    if (queue->semaphore == NULL)
        return NULL;
    queue->count = messageCount;
    queue->size = size;
    queue->used = 0;
    queue->read = 0;
    queue->write = 0;
    return queue;
}

/******************************************/
void OS_MQueueDelete(OS_MQueue_t *mQueue)
{
    OS_SemaphoreDelete(mQueue->semaphore);
    rtos__heap_free(mQueue);
}

/******************************************/
/* Send a message that is messageBytes long (defined during create)*/
int OS_MQueueSend(OS_MQueue_t *mQueue, void *message)
{
    os_uint32 state, *dst, *src;
    int i;

    assert(mQueue);
    src = (os_uint32 *)message;
    state = OS_CriticalBegin(); /*Disable interrupts */
    if (++mQueue->used > mQueue->count)
    {
        /*The message queue is full so discard the message*/
        --mQueue->used;
        OS_CriticalEnd(state);
        return -1;
    }
    dst = (os_uint32 *)(mQueue + 1) + mQueue->write * mQueue->size;
    for (i = 0; i < mQueue->size; ++i) /* Copy the message into the queue */
        dst[i] = src[i];
    if (++mQueue->write >= mQueue->count)
        mQueue->write = 0;
    OS_CriticalEnd(state);               /*Re-enable interrupts*/
    OS_SemaphorePost(mQueue->semaphore); /*Wakeup the receiving thread*/
    return 0;
}

/******************************************/
/*Receive a message that is messageBytes long (defined during create)
 Wait at most ~10 msec ticks*/
int OS_MQueueGet(OS_MQueue_t *mQueue, void *message, int ticks)
{
    os_uint32 state, *dst, *src;
    int i, rc;

    assert(mQueue);
    rc = OS_SemaphorePend(mQueue->semaphore, ticks); /*Wait for message*/
    if (rc)
        return rc;              /*Request timed out so rc = -1 */
    state = OS_CriticalBegin(); /*Disable interrupts*/
    --mQueue->used;
    dst = (os_uint32 *)message;
    src = (os_uint32 *)(mQueue + 1) + mQueue->read * mQueue->size;
    for (i = 0; i < mQueue->size; ++i) /*Copy message from the queue*/
        dst[i] = src[i];
    if (++mQueue->read >= mQueue->count)
        mQueue->read = 0;
    OS_CriticalEnd(state); /*Re-enable interrupts*/
    return 0;
}

/***************** Jobs *******************/
/******************************************/
static OS_MQueue_t *jobQueue;
static OS_Thread_t *jobThread;

/*This thread waits for jobs that request a function to be called*/
static void JobThread(void *arg)
{
    os_uint32 message[4];
    JobFunc_t funcPtr;
    (void)arg;
    for (;;)
    {
        OS_MQueueGet(jobQueue, message, OS_WAIT_FOREVER);
        funcPtr = (JobFunc_t)message[0];
        funcPtr((void *)message[1], (void *)message[2], (void *)message[3]);
    }
}

/******************************************/
/*Call a function using the job thread so the caller won't be blocked*/
void OS_Job(JobFunc_t funcPtr, void *arg0, void *arg1, void *arg2)
{
    os_uint32 message[4];

    if (jobQueue == NULL)
    {
        OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
        if (jobQueue == NULL)
        {
            jobQueue = OS_MQueueCreate("job", 100, 16);
            jobThread = rtos__thread_create("job", JobThread, NULL, 150, 4000);
        }
        OS_SemaphorePost(SemaphoreLock);
        if (jobQueue == NULL)
            return;
    }

    message[0] = (os_uint32)funcPtr;
    message[1] = (os_uint32)arg0;
    message[2] = (os_uint32)arg1;
    message[3] = (os_uint32)arg2;
    OS_MQueueSend(jobQueue, message);
}

/***************** Timer ******************/
/******************************************/
/*This thread polls the list of timers to see if any have timed out*/
static void OS_TimerThread(void *arg)
{
    os_uint32 timeNow;
    int diff, ticks;
    os_uint32 message[8];
    OS_Timer_t *timer;
    (void)arg;

    timeNow = OS_ThreadTime(); /*Number of ~10 msec ticks since reboot*/
    for (;;)
    {
        /* Determine how long to sleep*/
        OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
        if (TimerHead)
            ticks = TimerHead->ticksTimeout - timeNow;
        else
            ticks = OS_WAIT_FOREVER;
        OS_SemaphorePost(SemaphoreLock);
        OS_SemaphorePend(SemaphoreTimer, ticks);

        /*Send messages for all timed out timers*/
        timeNow = OS_ThreadTime();
        for (;;)
        {
            timer = NULL;
            OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
            if (TimerHead)
            {
                diff = timeNow - TimerHead->ticksTimeout;
                if (diff >= 0)
                    timer = TimerHead;
            }
            OS_SemaphorePost(SemaphoreLock);
            if (timer == NULL)
                break;
            if (timer->ticksRestart)
                OS_TimerStart(timer, timer->ticksRestart, timer->ticksRestart);
            else
                OS_TimerStop(timer);

            if (timer->callback)
                timer->callback(timer, timer->info);
            else
            {
                /*Send message*/
                message[0] = MESSAGE_TYPE_TIMER;
                message[1] = (os_uint32)timer;
                message[2] = timer->info;
                OS_MQueueSend(timer->mqueue, message);
            }
        }
    }
}

/******************************************/
/*Create a timer that will send a message upon timeout*/
OS_Timer_t *OS_TimerCreate(const char *name, OS_MQueue_t *mQueue, os_uint32 info)
{
    OS_Timer_t *timer;

    OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
    if (SemaphoreTimer == NULL)
    {
        SemaphoreTimer = OS_SemaphoreCreate("Timer", 0);
        rtos__thread_create("Timer", OS_TimerThread, NULL, 250, 2000);
    }
    OS_SemaphorePost(SemaphoreLock);

    timer = (OS_Timer_t *)rtos__heap_alloc(HEAP_SYSTEM, sizeof(OS_Timer_t));
    if (timer == NULL)
        return NULL;
    timer->name = name;
    timer->callback = NULL;
    timer->mqueue = mQueue;
    timer->next = NULL;
    timer->prev = NULL;
    timer->info = info;
    timer->active = 0;
    return timer;
}

/******************************************/
void OS_TimerDelete(OS_Timer_t *timer)
{
    OS_TimerStop(timer);
    rtos__heap_free(timer);
}

/******************************************/
void OS_TimerCallback(OS_Timer_t *timer, OS_TimerFuncPtr_t callback)
{
    timer->callback = callback;
}

/******************************************/
/*Must not be called from an ISR
 In ~10 msec ticks send a message (or callback)*/
void OS_TimerStart(OS_Timer_t *timer, os_uint32 ticks, os_uint32 ticksRestart)
{
    OS_Timer_t *node, *prev;
    int diff, check = 0;
    int cpuIndex;
    cpuIndex = OS_CpuIndex();
    assert(timer);
    assert(InterruptInside[cpuIndex] == 0);
    ticks += OS_ThreadTime();
    if (timer->active)
        OS_TimerStop(timer);
    OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
    if (timer->active)
    {
        /*Prevent race condition*/
        OS_SemaphorePost(SemaphoreLock);
        return;
    }
    timer->ticksTimeout = ticks;
    timer->ticksRestart = ticksRestart;
    timer->active = 1;
    prev = NULL;
    for (node = TimerHead; node; node = node->next)
    {
        diff = ticks - node->ticksTimeout;
        if (diff <= 0)
            break;
        prev = node;
    }
    timer->next = node;
    timer->prev = prev;
    if (node)
        node->prev = timer;
    if (prev == NULL)
    {
        TimerHead = timer;
        check = 1;
    }
    else
        prev->next = timer;
    OS_SemaphorePost(SemaphoreLock);
    if (check)
        OS_SemaphorePost(SemaphoreTimer); /*Wakeup OS_TimerThread*/
}

/******************************************/
/*Must not be called from an ISR*/
void OS_TimerStop(OS_Timer_t *timer)
{
    int cpuIndex;
    cpuIndex = OS_CpuIndex();
    assert(timer);
    assert(InterruptInside[cpuIndex] == 0);
    OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
    if (timer->active)
    {
        timer->active = 0;
        if (timer->prev == NULL)
            TimerHead = timer->next;
        else
            timer->prev->next = timer->next;
        if (timer->next)
            timer->next->prev = timer->prev;
    }
    OS_SemaphorePost(SemaphoreLock);
}

/***************** ISR ********************/
/******************************************/
void rtos__interrupt_service_routine(os_uint32 status, struct reg_buf *stack)
{
    int i;
    OS_FuncPtr_t f;
    os_uint32 state, cpuIndex = OS_CpuIndex();

    f = Isr[31];
    if (status == 0 && Isr[31])
        f(stack); /*SYSCALL or BREAK*/

    InterruptInside[cpuIndex] = 1;
    i = 0;
    do
    {
        if (status & 1)
        {
            f = Isr[i];
            if (f)
                f(stack);
            else
                OS_InterruptMaskClear(1 << i);
        }
        status >>= 1;
        ++i;
    } while (status);
    InterruptInside[cpuIndex] = 0;

    state = OS_SpinLock();
    if (ThreadNeedReschedule[cpuIndex])
        OS_ThreadReschedule(ThreadNeedReschedule[cpuIndex] & 1);
    OS_SpinUnlock(state);
}

/******************************************/
void OS_InterruptRegister(os_uint32 mask, OS_FuncPtr_t funcPtr)
{
    int i;

    for (i = 0; i < 32; ++i)
    {
        if (mask & (1 << i))
            Isr[i] = funcPtr;
    }
}

/******************************************/
/*Plasma hardware dependent*/
os_uint32 OS_InterruptStatus(void)
{
    return MemoryRead(IRQ_STATUS) & MemoryRead(IRQ_MASK);
}

/******************************************/
/*Plasma hardware dependent*/
os_uint32 OS_InterruptMaskSet(os_uint32 mask)
{
    os_uint32 state;
    state = OS_CriticalBegin();
    mask |= MemoryRead(IRQ_MASK);
    MemoryWrite(IRQ_MASK, mask);
    OS_CriticalEnd(state);
    return mask;
}

/******************************************/
/*Plasma hardware dependent*/
os_uint32 OS_InterruptMaskClear(os_uint32 mask)
{
    os_uint32 state;
    state = OS_CriticalBegin();
    mask = MemoryRead(IRQ_MASK) & ~mask;
    MemoryWrite(IRQ_MASK, mask);
    OS_CriticalEnd(state);
    return mask;
}

/**************** Init ********************/
/******************************************/
/*If there aren't any other ready to run threads then spin here*/
#ifndef DISABLE_IRQ_SIM
static int SimulateIsr;
#endif
static void OS_IdleThread(void *arg)
{
    os_uint32 IdleCount = 0;
#ifndef DISABLE_IRQ_SIM
    unsigned int value; /*tick interrupt*/
#endif
    (void)arg;

    /*Don't block in the idle thread!*/
    for (;;)
    {
        ++IdleCount;
        k__draw_char(0, 0, (IdleCount + 20) & 0x7F);
#ifndef DISABLE_IRQ_SIM
        MemoryRead(IRQ_MASK + 4); /* will call Sleep*/
        if (SimulateIsr && (int)arg == OS_CPU_COUNT - 1)
        {
            value = IRQ_COUNTER18; /**/ tick interrupt*/
            for (;;)
            {
                value |= OS_InterruptStatus();
                if (value == 0)
                    break;
                rtos__interrupt_service_routine(value, (void *)0);
                value = 0;
            }
        }
#endif
#if OS_CPU_COUNT > 1
        if ((int)arg < OS_CPU_COUNT - 1)
        {
            unsigned int state;
#ifndef WIN32
            for (IdleCount = 0; IdleCount < 25000; ++IdleCount)
                ;
#endif
            state = OS_SpinLock();
            OS_ThreadReschedule(1);
            OS_SpinUnlock(state);
        }
#endif
    }
}

/******************************************/
/*Plasma hardware dependent
ISR called every ~10 msecs when bit 18 of the counter register toggles*/
static void OS_InterruptTick(void *arg)
{
    os_uint32 state, mask;

    /*Toggle looking for IRQ_COUNTER18 or IRQ_COUNTER18_NOT*/
    state = OS_SpinLock();
    mask = MemoryRead(IRQ_MASK);
    mask ^= IRQ_COUNTER18 | IRQ_COUNTER18_NOT;
    MemoryWrite(IRQ_MASK, mask);
    OS_ThreadTick(arg);
    OS_SpinUnlock(state);
}

/******************************************/
/*Initialize the OS by setting up the system heap and the tick interrupt*/
void rtos__init(os_uint32 *heapStorage, os_uint32 bytes)
{
    int i;

    TimeoutHead = NULL;
    TimerHead = NULL;
    ThreadHead = NULL;
    SemaphoreTimer = NULL;

    interrupt__init();                 /*Patch interrupt vector*/
    OS_InterruptMaskClear(0xffffffff); /*Disable interrupts*/
    HeapArray[0] = OS_HeapCreate("Heap", heapStorage, bytes);
    HeapArray[1] = HeapArray[0];
#ifndef WIN32
    /* HeapArray[6] = OS_HeapCreate("2nd", (os_uint8*)RAM_EXTERNAL_BASE +
            0x180000, 1024*512);
     OS_HeapAlternate(HeapArray[0], HeapArray[6]);
     HeapArray[7] = OS_HeapCreate("Alt", (os_uint8*)RAM_EXTERNAL_BASE +
            RAM_EXTERNAL_SIZE*2, 1024*1024*60);
     OS_HeapAlternate(HeapArray[6], HeapArray[7]);
     */
#endif

    SemaphoreSleep = OS_SemaphoreCreate("Sleep", 0);
    SemaphoreRelease = OS_SemaphoreCreate("Release", 1);
    SemaphoreLock = OS_SemaphoreCreate("Lock", 1);
    if ((MemoryRead(IRQ_STATUS) & (IRQ_COUNTER18 | IRQ_COUNTER18_NOT)) == 0)
    {
        /*Detected running in simulator*/
        k__printf("SimIsr\n");
        /*SimulateIsr = 1;*/
    }

    for (i = 0; i < OS_CPU_COUNT; ++i)
        rtos__thread_create("Idle", OS_IdleThread, (void *)i, i, 256);

    /*Plasma hardware dependent (register for OS tick interrupt every ~10 msecs)*/
    OS_InterruptRegister(IRQ_COUNTER18 | IRQ_COUNTER18_NOT, OS_InterruptTick);
    OS_InterruptMaskSet(IRQ_COUNTER18);
}

#if OS_CPU_COUNT > 1
static volatile os_uint8 SpinLockArray[OS_CPU_COUNT];
/******************************************/
os_uint32 OS_CpuIndex(void)
{
#ifdef WIN32
    int i;
    unsigned int threadId = GetCurrentThreadId();
    for (i = 0; i < OS_CPU_COUNT; ++i)
    {
        if (threadId == ThreadId[i])
            return i;
    }
#endif
    /* return MemoryRead(GPIO_CPU_INDEX);*/
    return 0; // 0 to OS_CPU_COUNT-1
}

/******************************************/
/*Symmetric Multiprocessing Spin Lock Mutex*/
os_uint32 OS_SpinLock(void)
{
    os_uint32 state, cpuIndex, i, ok, delay;
    volatile os_uint32 keepVar;

    cpuIndex = OS_CpuIndex();
    state = rtos__asm_interrupt_enable(0); /*disable interrupts*/
    if (SpinLockArray[cpuIndex])
        return (os_uint32)-1; /*already locked*/
    delay = (4 + cpuIndex) << 2;

    /*Spin until only this CPU has the spin lock*/
    for (;;)
    {
        ok = 1;
        SpinLockArray[cpuIndex] = 1;
        for (i = 0; i < OS_CPU_COUNT; ++i)
        {
            if (i != cpuIndex && SpinLockArray[i])
                ok = 0; /*Another CPU has the spin lock*/
        }
        if (ok)
            return state;
        SpinLockArray[cpuIndex] = 0;
        rtos__asm_interrupt_enable(state); /*re-enable interrupts*/
        for (i = 0; i < delay; ++i)        /*wait a bit*/
            ++ok;
        keepVar = ok; /*don't optimize away the delay loop*/
        if (delay < 128)
            delay <<= 1;
        state = rtos__asm_interrupt_enable(0); /*disable interrupts*/
    }
}

/******************************************/
void OS_SpinUnlock(os_uint32 state)
{
    os_uint32 cpuIndex;
    if (state == (os_uint32)-1)
        return; /*nested lock call*/
    cpuIndex = OS_CpuIndex();
    SpinLockArray[cpuIndex] = 0;
}
#endif // OS_CPU_COUNT > 1

/******************************************/
/*Start thread swapping*/
void rtos__start(void)
{
    ThreadSwapEnabled = 1;
    (void)OS_SpinLock();
    OS_ThreadReschedule(1);
    while (ThreadHead)
    {
    }
}

/******************************************/
/*Place breakpoint here*/
void OS_Assert(void)
{
}

/**************** Example *****************/
#ifndef NO_MAIN
#ifdef WIN32
static os_uint8 HeapSpace[1024 * 512]; /*or simulation on a PC*/
#endif

int main(int programEnd, char *argv[])
{
    (void)programEnd; /*Pointer to end of used memory*/
    (void)argv;

    k__printf("Starting RTOS " __DATE__ " " __TIME__ "\n");
    MemoryWrite(IRQ_MASK, 0);
#ifdef WIN32
    rtos__init((os_uint32 *)HeapSpace, sizeof(HeapSpace)); /*For PC simulation*/
#else
    /*Create heap in remaining space after program in 1MB external RAM*/
    rtos__init((os_uint32 *)programEnd,
               RAM_EXTERNAL_BASE + RAM_EXTERNAL_SIZE - programEnd);
#endif
    UartInit();
    rtos__thread_create("Main", MainThread, NULL, 100, 4000);
#if defined(WIN32) && OS_CPU_COUNT > 1
    OS_InitSimulation();
#endif
    rtos__start();
    return 0;
}
#endif /*NO_MAIN*/
