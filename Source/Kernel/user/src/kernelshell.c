/*******************************************************************************
 * @file kernelshell.c
 *
 * @see kernelshell.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's shell definition.
 *
 * @details Kernel's shell definition. This shell is the entry point of the
 * kernel for the user. It has kernel rights and can be extended by the user
 * for different purposes.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>          /* CPU API*/
#include <kheap.h>        /* Kernel Heap */
#include <signal.h>       /* Signal manager */
#include <string.h>       /* String manipulation */
#include <signal.h>       /* Signals */
#include <syslog.h>       /* Syslog services */
#include <console.h>      /* Console driver */
#include <graphics.h>     /* Graphics driver */
#include <time_mgt.h>     /* Time manager */
#include <semaphore.h>    /* Semaphore service */
#include <scheduler.h>    /* Scheduler services */
#include <kerneloutput.h> /* Kernel output */

/* Header file */
#include <kernelshell.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the kernel shell version */
#define SHELL_VERION "0.1"

#define SHELL_INPUT_BUFFER_SIZE 128

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

typedef struct
{
    const char* pCommandName;
    const char* pDescription;
    void (*pFunc)(void);
} command_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _shellSyslog(void);
static void _shellTimeTest(void);
static void _shellDummyDefered(void* args);
static void _shellDefer(void);
static void _shellDisplayThreads(void);
static void _shellSignalSelf(void);
static void _shellSignalHandler(void);
static void _shellCtxSwitchTime(void);
static void _shellHelp(void);
static void _shellExecuteCommand(void);
static void _shellGetCommand(void);
static void* _shellEntry(void* args);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
static char sInputBuffer[SHELL_INPUT_BUFFER_SIZE + 1];
static size_t sInputBufferCursor;
static spinlock_t sSignalLock = SPINLOCK_INIT_VALUE;
static uint64_t sTimeSwitchStart[2];
static uint64_t sTimeSwitchEnd[2];
static volatile uint32_t threadStarted = 0;
static spinlock_t threadStartedLock = SPINLOCK_INIT_VALUE;

static const command_t sCommands[] = {
    {"tasks", "Display the current threads", _shellDisplayThreads},
    {"deferIsr", "Defer a test ISR", _shellDefer},
    {"signalSelf", "Signal the Shell", _shellSignalSelf},
    {"timeCtxSw", "Get the average context switch time", _shellCtxSwitchTime},
    {"timePrec", "Timer precision test", _shellTimeTest},
    {"syslog", "Syslog test", _shellSyslog},
    {"help", "Display this help", _shellHelp},
    {NULL, NULL, NULL}
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _shellHelp(void)
{
    size_t i;

    for(i = 0; sCommands[i].pCommandName != NULL; ++i)
    {
        kprintf("%s - %s\n",
                sCommands[i].pCommandName,
                sCommands[i].pDescription);
    }
}

static void _shellSyslog(void)
{
    syslog(SYSLOG_LEVEL_CRITICAL,
           "SHELL",
           "This is a critical message from TID %d",
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_ERROR,
           "SHELL",
           "This is an error message from TID %d",
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_WARNING,
           "SHELL",
           "This is a warning message from TID %d",
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_INFO,
           "SHELL",
           "This is an info message from TID %d",
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_DEBUG,
           "SHELL",
           "This is a debug message from TID %d",
           schedGetCurrentThread()->tid);
}

static void _shellTimeTest(void)
{
    uint32_t i;
    uint64_t time;
    uint64_t s;
    uint64_t ms;
    uint64_t us;
    uint64_t ns;
    uint64_t oldTime = timeGetUptime();
    uint64_t tmp;
    for(i = 0; i < 1000; ++i)
    {
        cpuHalt();
        tmp = timeGetUptime();
        time = tmp - oldTime;
        oldTime = tmp;
        s = time / 1000000000;
        ms = (time % 1000000000) / 1000000;
        us = (time % 1000000) / 1000;
        ns = (time % 1000);
        kprintf("Time: %llu | %llu.%llu.%llu.%llu\n", time, s, ms, us, ns);
    }
}

static void* _shellCtxSwitchRoutineAlone(void* args)
{
    int32_t tid;
    uint64_t endTime;
    uint32_t i;

    tid = (int32_t)(uintptr_t)args;

    for(i = 0; i < 1000000; ++i)
    {
        sTimeSwitchStart[tid] = timeGetUptime();
        schedSchedule();
        endTime = timeGetUptime();
        sTimeSwitchEnd[tid] = (sTimeSwitchEnd[tid] * i +
                               (endTime - sTimeSwitchStart[tid])) / (i + 1);
    }

    kprintf("Switch time no resched: %llu\n", sTimeSwitchEnd[tid]);

    return NULL;
}

static void* _shellCtxSwitchRoutine(void* args)
{
    int32_t tid;
    uint64_t endTime;
    uint32_t i;

    tid = (int32_t)(uintptr_t)args;
    spinlockAcquire(&threadStartedLock);
    ++threadStarted;
    spinlockRelease(&threadStartedLock);
    while(threadStarted != 2) {}
    for(i = 0; i < 100000; ++i)
    {
        sTimeSwitchStart[tid] = timeGetUptime();
        schedSchedule();
        endTime = timeGetUptime();
        sTimeSwitchEnd[(tid + 1) % 2] = (sTimeSwitchEnd[(tid + 1) % 2] * i +
                               (endTime - sTimeSwitchStart[(tid + 1) % 2])) / (i + 1);
    }

    kprintf("Switch time resched: %llu\n", sTimeSwitchEnd[(tid + 1) % 2]);

    return NULL;
}

static void _shellCtxSwitchTime(void)
{
    kernel_thread_t* pShellThread[2];
    OS_RETURN_E      error;

    memset(sTimeSwitchStart, 0, sizeof(sTimeSwitchStart));
    memset(sTimeSwitchEnd, 0, sizeof(sTimeSwitchEnd));
    threadStarted = 0;

    error = schedCreateKernelThread(&pShellThread[0],
                                    KERNEL_LOWEST_PRIORITY - 1,
                                    "kernelShellTime",
                                    0x1000,
                                    0x8,
                                    _shellCtxSwitchRoutineAlone,
                                    (void*)0);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to start thread. Error %d\n", error);
        return;
    }

    error = schedJoinThread(pShellThread[0], NULL, NULL);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to join thread. Error %d\n", error);
        return;
    }

    memset(sTimeSwitchStart, 0, sizeof(sTimeSwitchStart));
    memset(sTimeSwitchEnd, 0, sizeof(sTimeSwitchEnd));
    threadStarted = 0;

    /* We don't keep the kernel shell thread handle, it the child of the main
     * kernel thread (IDLE) and will be fully destroyed on exit, without need
     * of join.
     */
    error = schedCreateKernelThread(&pShellThread[0],
                                    KERNEL_LOWEST_PRIORITY - 1,
                                    "kernelShellTime0",
                                    0x1000,
                                    0x8,
                                    _shellCtxSwitchRoutine,
                                    (void*)0);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to start thread. Error %d\n", error);
        return;
    }
    error = schedCreateKernelThread(&pShellThread[1],
                                    KERNEL_LOWEST_PRIORITY - 1,
                                    "kernelShellTime1",
                                    0x1000,
                                    0x8,
                                    _shellCtxSwitchRoutine,
                                    (void*)1);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to start thread. Error %d\n", error);
        return;
    }

    error = schedJoinThread(pShellThread[0], NULL, NULL);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to join thread. Error %d\n", error);
    }
    error = schedJoinThread(pShellThread[1], NULL, NULL);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to join thread. Error %d\n", error);
    }
}

static void _shellSignalHandler(void)
{
    kprintf("Hey! I'm the kernel shell signal handler (thread %d)\n",
            schedGetCurrentThread()->tid);

    spinlockRelease(&sSignalLock);
}

static void _shellSignalSelf(void)
{
    OS_RETURN_E error;

    spinlockAcquire(&sSignalLock);

    error = signalRegister(THREAD_SIGNAL_USR1, _shellSignalHandler);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to register signal handler with error %d\n", error);
        return;
    }

    error = signalThread(schedGetCurrentThread(), THREAD_SIGNAL_USR1);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to signal self %d\n", error);
        return;
    }

    spinlockAcquire(&sSignalLock);
    spinlockRelease(&sSignalLock);

    kprintf("Kernel shell is back from signaling\n");
}

static void _shellDummyDefered(void* args)
{
    kprintf("Defered from shell with argument: %d (Thread %d)\n",
            (uint32_t)(uintptr_t)args,
            schedGetCurrentThread()->tid);
}

static void _shellDefer(void)
{
    OS_RETURN_E error;

    error = interruptDeferIsr(_shellDummyDefered, (void*)42);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to defer with error %d\n", error);
    }
}

static void _shellDisplayThreads(void)
{
    size_t         threadCount;
    size_t         i;
    uint32_t       j;
    size_t         prio;
    thread_info_t* pThreadTable;

    threadCount = schedGetThreadCount();
    pThreadTable = kmalloc(sizeof(thread_info_t) * threadCount);
    if(pThreadTable == NULL)
    {
        kprintf("Unable to allocate thread table memory\n");
        return;
    }

    threadCount = schedGetThreads(pThreadTable, threadCount);
    kprintf("#------------------------------------------------------------------------#\n");
    kprintf("| PID  | NAME                           | TYPE   | PRIO | STATE    | CPU |\n");
    kprintf("#------------------------------------------------------------------------#\n");
    for(prio = KERNEL_HIGHEST_PRIORITY; prio <= KERNEL_LOWEST_PRIORITY; ++prio)
    {
        for(i = 0; i < threadCount; ++i)
        {
            if(pThreadTable[i].priority != prio)
            {
                continue;
            }
            kprintf("| % 4d | %s", pThreadTable[i].tid, pThreadTable[i].pName);
            for(j = 0;
                j < THREAD_NAME_MAX_LENGTH - strlen(pThreadTable[i].pName) - 1;
                ++j)
            {
                kprintf(" ");
            }
            switch(pThreadTable[i].type)
            {
                case THREAD_TYPE_KERNEL:
                    kprintf("| KERNEL |");
                    break;
                case THREAD_TYPE_USER:
                    kprintf("| USER   |");
                    break;
                default:
                    kprintf("| NONE   |");
                    break;
            }
            kprintf("  % 3d |", pThreadTable[i].priority);
            switch(pThreadTable[i].currentState)
            {
                case THREAD_STATE_RUNNING:
                    kprintf(" RUNNING  |");
                    break;
                case THREAD_STATE_READY:
                    kprintf(" READY    |");
                    break;
                case THREAD_STATE_SLEEPING:
                    kprintf(" SLEEPING |");
                    break;
                case THREAD_STATE_ZOMBIE:
                    kprintf(" ZOMBIE   |");
                    break;
                case THREAD_STATE_JOINING:
                    kprintf(" JOINING  |");
                    break;
                case THREAD_STATE_WAITING:
                    kprintf(" WAITING  |");
                    break;
                default:
                    kprintf(" UNKNOWN  |");
                    break;
            }
            if(pThreadTable[i].currentState == THREAD_STATE_RUNNING)
            {
                kprintf(" % 3d |\n", pThreadTable[i].schedCpu);
            }
            else
            {
                kprintf("   * |\n", pThreadTable[i].schedCpu);
            }
        }
    }
    kprintf("#------------------------------------------------------------------------#\n");
}

static void _shellExecuteCommand(void)
{
    size_t cursor;
    char   command[SHELL_INPUT_BUFFER_SIZE + 1];

    if(sInputBufferCursor == 0)
    {
        return;
    }

    for(cursor = 0; cursor < sInputBufferCursor; ++cursor)
    {
        if(sInputBuffer[cursor] == ' ')
        {
            break;
        }
        else
        {
            command[cursor] = sInputBuffer[cursor];
        }
    }
    command[cursor] = 0;

    /* Remove the space */
    if(cursor < sInputBufferCursor)
    {
        ++cursor;
    }

    size_t i;

    for(i = 0; sCommands[i].pCommandName != NULL; ++i)
    {
        if(strcmp(command, sCommands[i].pCommandName) == 0)
        {
            sCommands[i].pFunc();
            break;
        }
    }

    if(sCommands[i].pCommandName == NULL)
    {
        kprintf("Unknown command: %s\n", command);
    }
}

static void _shellGetCommand(void)
{
    ssize_t readCount;
    char    readChar;

    sInputBufferCursor = 0;
    kprintf("> ");
    kprintfFlush();
    while(TRUE)
    {
        readCount = consoleRead(&readChar, 1);
        if(readCount > 0)
        {
            if(readChar == 0xD || readChar == 0xA)
            {
                kprintf("\n");
                break;
            }
            else if(readChar == 0x7F || readChar == '\b')
            {
                if(sInputBufferCursor > 0)
                {
                    --sInputBufferCursor;
                    kprintf("\b \b");
                    kprintfFlush();
                }
            }
            else if(sInputBufferCursor < SHELL_INPUT_BUFFER_SIZE)
            {
                sInputBuffer[sInputBufferCursor] = readChar;
                ++sInputBufferCursor;
                kprintf("%c", readChar);
                kprintfFlush();
            }
        }
    }
    sInputBuffer[sInputBufferCursor] = 0;
}

#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"

static void* _shellEntry(void* args)
{
    (void)args;

    /* Wait for all the system to be up */
    schedSleep(2000000000);

    kprintf("\n");


    while(TRUE)
    {
        _shellGetCommand();
        _shellExecuteCommand();
    }

    return (void*)0;
}

void kernelShellInit(void)
{
    OS_RETURN_E      error;
    kernel_thread_t* pShellThread;

    /* We don't keep the kernel shell thread handle, it the child of the main
     * kernel thread (IDLE) and will be fully destroyed on exit, without need
     * of join.
     */
    error = schedCreateKernelThread(&pShellThread,
                                    0,
                                    "kernelShell",
                                    0x1000,
                                    0x2,
                                    _shellEntry,
                                    NULL);
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               "SHELL",
               "Failed to start the kernel shell. Error %d",
               error);
    }
}

/************************************ EOF *************************************/