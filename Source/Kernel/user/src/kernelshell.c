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
#include <vfs.h>          /* VFS service */
#include <cpu.h>          /* CPU API*/
#include <kheap.h>        /* Kernel Heap */
#include <panic.h>        /* Kernel panic */
#include <stdlib.h>       /* Standard lib */
#include <signal.h>       /* Signal manager */
#include <string.h>       /* String manipulation */
#include <signal.h>       /* Signals */
#include <syslog.h>       /* Syslog services */
#include <console.h>      /* Console driver */
#include <graphics.h>     /* Graphics driver */
#include <time_mgt.h>     /* Time manager */
#include <ksemaphore.h>   /* Semaphore service */
#include <scheduler.h>    /* Scheduler services */
#include <interrupts.h>   /* Interrupt manager */
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
    void (*pFunc)(const char*);
} command_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _shellSyslog(const char* args);
static void _shellTimeTest(const char* args);
static void _shellDummyDefered(void* args);
static void _shellDefer(const char* args);
static void _shellDisplayThreads(const char* args);
static void _shellSignalSelf(const char* args);
static void _shellSignalHandler(void);
static void _shellCtxSwitchTime(const char* args);
static void _shellHelp(const char* args);
static void _shellDrawTest(const char* args);
static void _shellList(const char* args);
static void _shellCat(const char* args);
static void _shellMount(const char* args);
static void _shellTest(const char* args);
static void _shellPanic(const char* args);
static void _shellSleep(const char* args);
static void _shellExit(const char* args);
static void _shellFork(const char* args);
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
    {"draw", "Draw test", _shellDrawTest},
    {"ls", "List files in a path", _shellList},
    {"mount", "Mount a device", _shellMount},
    {"cat", "Cat a file", _shellCat},
    {"test", "Current dev test for testing purpose", _shellTest},
    {"panic", "Generates a kernel panic", _shellPanic},
    {"sleep", "Sleeps for ns time", _shellSleep},
    {"fork", "Tests the fork features", _shellFork},
    {"exit", "Exit the shell", _shellExit},
    {"help", "Display this help", _shellHelp},
    {NULL, NULL, NULL}
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _shellFork(const char* args)
{
    (void)args;
    OS_RETURN_E error;
    int values[50];
    int32_t newPid;
    for(int i = 0; i < 50; ++i)
    {
        values[i] = i;
    }

    error = schedFork(&newPid);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to fork, error: %d\n", error);
        return;
    }
    if(newPid == 0)
    {
        for(int i = 0; i < 50; ++i)
        {
            values[i] = i * 2;
        }
        schedSleep(1000000000);
        kprintf("In children and values are:\n");
        for(int i = 0; i < 50; ++i)
        {
            kprintf("Value: %d\n", values[i]);
        }
        schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    0);
    }
    else
    {
        schedSleep(1000000000);
        kprintf("In parent (pid of child is %d) and values are:\n", newPid);
        for(int i = 0; i < 50; ++i)
        {
            kprintf("Value: %d\n", values[i]);
        }
    }
}

static void _shellExit(const char* args)
{
    int32_t retCode;

    retCode = strtol(args, NULL, 10);

    kprintf("Kernel shell exiting with code %i.\n", retCode);
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    (void*)(uintptr_t)retCode);
}

static void _shellSleep(const char* args)
{
    uint64_t time;
    time = strtoul(args, NULL, 10);
    schedSleep(time);
}

static void _shellPanic(const char* args)
{
    (void)args;
    PANIC(OS_NO_ERR, "KERNEL_SHELL", "Kernel Shell Panic Generator");
}

static void _shellTest(const char* args)
{
    (void)args;
    _shellList("/initrd");
    kprintf("-----\n");
    _shellList("/initrd/folder1");
    kprintf("-----\n");
    _shellList("/initrd/folder1/anotherfolder/");
}

static void _shellCat(const char* args)
{
    int32_t fd;
    int32_t ret;
    char buffer[100];
    fd = vfsOpen(args, O_RDONLY, 0);
    if(fd < 0)
    {
        kprintf("Failed to open %s\n", args);
        return;
    }

    while((ret = vfsRead(fd, buffer, 99)) > 0)
    {
        buffer[ret] = 0;
        kprintf("%s", buffer);
    }
    kprintf("\n");

    vfsClose(fd);
}

static void _shellMount(const char* args)
{
    int32_t ret;
    uint32_t i;
    uint32_t lastCpy;
    size_t length;
    uint8_t copyIndex;
    char argsVal[3][128];

    lastCpy = 0;
    copyIndex = 0;
    length = strlen(args);
    for(i = 0; i < length; ++i)
    {
        if(args[i] == ' ')
        {
            memcpy(argsVal[copyIndex], args + lastCpy, i - lastCpy);
            argsVal[copyIndex][i - lastCpy] = 0;
            ++copyIndex;
            lastCpy = i + 1;
        }
        else if(i == length - 1)
        {
            memcpy(argsVal[copyIndex], args + lastCpy, i - lastCpy + 1);
            argsVal[copyIndex][i - lastCpy + 1] = 0;
            ++copyIndex;
            lastCpy = i + 1;
        }
    }


    if(copyIndex != 3)
    {
        kprintf("Error: mount <dev_path> <dir_path> <fs_name>\n");
        return;
    }
    kprintf("Mouting %s to %s (fs: %s)\n", argsVal[0], argsVal[1], argsVal[2]);

    ret = vfsMount(argsVal[1], argsVal[0], argsVal[2]);
    if(ret != 0)
    {
        kprintf("Failed to mount: %d\n", ret);
    }
}

static void _shellList(const char* args)
{
    int32_t  fd;
    dirent_t dirEnt;
    fd = vfsOpen(args, O_RDONLY, 0);
    if(fd < 0)
    {
        kprintf("Failed to open %s\n", args);
        return;
    }

    while(vfsReaddir(fd, &dirEnt) >= 0)
    {
        kprintf("%s\n", dirEnt.pName);
    }

    vfsClose(fd);
}

static void _shellDrawTest(const char* args)
{
    (void)args;
    uint32_t x;

    consoleClear();
    consolePutCursor(0, 0);

    graphicsDrawRectangle(0, 0, 2000, 2000, 0xFFFFFFFF);

    for(x = 1; x < 1022; ++x)
    {
        graphicsDrawLine(x, 1, 500, 500, 0xff33addf);
        schedSleep(333333);
    }
}

static void _shellHelp(const char* args)
{
    (void)args;
    size_t i;

    for(i = 0; sCommands[i].pCommandName != NULL; ++i)
    {
        kprintf("%s - %s\n",
                sCommands[i].pCommandName,
                sCommands[i].pDescription);
    }
}

static void _shellSyslog(const char* args)
{
    syslog(SYSLOG_LEVEL_CRITICAL,
           "SHELL",
           args,
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_ERROR,
           "SHELL",
           args,
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_WARNING,
           "SHELL",
           args,
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_INFO,
           "SHELL",
           args,
           schedGetCurrentThread()->tid);
    syslog(SYSLOG_LEVEL_DEBUG,
           "SHELL",
           args,
           schedGetCurrentThread()->tid);
}

static void _shellTimeTest(const char* args)
{
    (void)args;

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

static void* _shellScheduleRoutineAlone(void* args)
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

    kprintf("Schedule time alone: %llu\n", sTimeSwitchEnd[tid]);

    return NULL;
}

static void* _shellScheduleRoutine(void* args)
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

    kprintf("Schedule time multiple: %llu\n", sTimeSwitchEnd[(tid + 1) % 2]);

    return NULL;
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
        cpuRaiseInterrupt(0x22);
        endTime = timeGetUptime();
        sTimeSwitchEnd[tid] = (sTimeSwitchEnd[tid] * i +
                               (endTime - sTimeSwitchStart[tid])) / (i + 1);
    }

    kprintf("Context switch time alone: %llu\n", sTimeSwitchEnd[tid]);

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
        cpuRaiseInterrupt(0x22);
        endTime = timeGetUptime();
        sTimeSwitchEnd[(tid + 1) % 2] = (sTimeSwitchEnd[(tid + 1) % 2] * i +
                               (endTime - sTimeSwitchStart[(tid + 1) % 2])) / (i + 1);
    }

    kprintf("Context switch time multiple: %llu\n", sTimeSwitchEnd[(tid + 1) % 2]);

    return NULL;
}

static void _shellCtxSwitchTime(const char* args)
{
    (void)args;
    kernel_thread_t* pShellThread[2];
    OS_RETURN_E      error;

    memset(sTimeSwitchStart, 0, sizeof(sTimeSwitchStart));
    memset(sTimeSwitchEnd, 0, sizeof(sTimeSwitchEnd));
    threadStarted = 0;

    error = schedCreateKernelThread(&pShellThread[0],
                                    11,
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

    error = schedCreateKernelThread(&pShellThread[0],
                                    11,
                                    "kernelShellTime",
                                    0x1000,
                                    0x8,
                                    _shellScheduleRoutineAlone,
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
                                    11,
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
                                    11,
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

    memset(sTimeSwitchStart, 0, sizeof(sTimeSwitchStart));
    memset(sTimeSwitchEnd, 0, sizeof(sTimeSwitchEnd));
    threadStarted = 0;

    /* We don't keep the kernel shell thread handle, it the child of the main
     * kernel thread (IDLE) and will be fully destroyed on exit, without need
     * of join.
     */
    error = schedCreateKernelThread(&pShellThread[0],
                                    11,
                                    "kernelShellTime0",
                                    0x1000,
                                    0x8,
                                    _shellScheduleRoutine,
                                    (void*)0);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to start thread. Error %d\n", error);
        return;
    }
    error = schedCreateKernelThread(&pShellThread[1],
                                    11,
                                    "kernelShellTime1",
                                    0x1000,
                                    0x8,
                                    _shellScheduleRoutine,
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

static void _shellSignalSelf(const char* args)
{
    (void)args;
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

static void _shellDefer(const char* args)
{
    (void)args;
    OS_RETURN_E error;

    error = interruptDeferIsr(_shellDummyDefered, (void*)42);
    if(error != OS_NO_ERR)
    {
        kprintf("Failed to defer with error %d\n", error);
    }
}

static void _shellDisplayThreads(const char* args)
{
    (void)args;
    size_t         threadCount;
    size_t         i;
    uint32_t       j;
    size_t         prio;
    int32_t*       pThreadTable;
    thread_info_t  threadInfo;

    threadCount = schedGetThreadCount();
    pThreadTable = kmalloc(sizeof(int32_t) * threadCount);
    if(pThreadTable == NULL)
    {
        kprintf("Unable to allocate thread table memory\n");
        return;
    }

    threadCount = schedGetThreadsIds(pThreadTable, threadCount);
    kprintf("#---------------------------------------------------------------------------------------------------------#\n");
    kprintf("|  PID  |  TID  | NAME                           | TYPE   | PRIO | STATE    | CPU | STACKS                |\n");
    kprintf("#---------------------------------------------------------------------------------------------------------#\n");
    for(prio = KERNEL_HIGHEST_PRIORITY; prio <= KERNEL_LOWEST_PRIORITY; ++prio)
    {
        for(i = 0; i < threadCount; ++i)
        {
            if(schedGetThreadInfo(&threadInfo, pThreadTable[i]) != OS_NO_ERR)
            {
                continue;
            }
            if(threadInfo.priority != prio)
            {
                continue;
            }
            kprintf("| % 5d | % 5d | %s",
                    threadInfo.pid,
                    threadInfo.tid,
                    threadInfo.pName);
            for(j = 0;
                j < THREAD_NAME_MAX_LENGTH - strlen(threadInfo.pName) - 1;
                ++j)
            {
                kprintf(" ");
            }
            switch(threadInfo.type)
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
            kprintf("  % 3d |", threadInfo.priority);
            switch(threadInfo.currentState)
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
            if(threadInfo.currentState == THREAD_STATE_RUNNING)
            {
                kprintf(" % 3d |", threadInfo.schedCpu);
            }
            else
            {
                kprintf("   * |", threadInfo.schedCpu);
            }
            kprintf(" K: 0x%P |\n", threadInfo.kStack);
            kprintf("|       |       |                                |        |      |          |     | U: 0x%P |\n", threadInfo.uStack);
            kprintf("#---------------------------------------------------------------------------------------------------------#\n");
        }
    }

}

static void _shellExecuteCommand(void)
{
    size_t cursor;
    size_t i;
    char   command[SHELL_INPUT_BUFFER_SIZE + 1];
    char*  args;

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
    if(cursor == sInputBufferCursor)
    {
        args = "\0";
    }
    else
    {
        args = &sInputBuffer[cursor + 1];
    }

    for(i = 0; sCommands[i].pCommandName != NULL; ++i)
    {
        if(strcmp(command, sCommands[i].pCommandName) == 0)
        {
            sCommands[i].pFunc(args);
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
    colorscheme_t saveScheme;
    colorscheme_t promptScheme;

    sInputBufferCursor = 0;
    promptScheme.background = BG_BLACK;
    promptScheme.foreground = FG_CYAN;
    consoleSaveColorScheme(&saveScheme);
    consoleSetColorScheme(&promptScheme);
    kprintf(">");
    kprintfFlush();
    consoleSetColorScheme(&saveScheme);
    kprintf(" ");
    kprintfFlush();
    while(true)
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

static void* _shellEntry(void* args)
{
    (void)args;

    /* Wait for all the system to be up */
    schedSleep(100000000);

    kprintf("\n");


    while(true)
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
                                    10,
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