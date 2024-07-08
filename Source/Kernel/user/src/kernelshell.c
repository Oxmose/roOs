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
#include <string.h>       /* String manipulation */
#include <signal.h>       /* Signals */
#include <console.h>      /* Console driver */
#include <time_mgt.h>     /* Time manager */
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

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _shellDummyDefered(void* args);
static void _shellDisplayThreads(void);
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
/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _shellDummyDefered(void* args)
{
    kprintf("Defered from shell with argument: %d (Thread %d)\n",
            (uint32_t)(uintptr_t)args,
            schedGetCurrentThread()->tid);
}

static void _shellDisplayThreads(void)
{
    size_t         threadCount;
    size_t         i;
    uint32_t       j;
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
    for(i = 0; i < threadCount; ++i)
    {
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
    kprintf("#------------------------------------------------------------------------#\n");
}

static void _shellExecuteCommand(void)
{
    OS_RETURN_E error;
    size_t      cursor;
    char        command[SHELL_INPUT_BUFFER_SIZE + 1];

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

    if(strcmp(command, "hello") == 0)
    {
        kprintf("Hi! I am roOs :)\n");
        kprintf("Your arguments are: %s\n", sInputBuffer + cursor);
    }
    else if(strcmp(command, "tasks") == 0)
    {
        _shellDisplayThreads();
    }
    else if(strcmp(command, "deferIsr") == 0)
    {
        error = interruptDeferIsr(_shellDummyDefered, (void*)42);
        if(error != OS_NO_ERR)
        {
            kprintf("Failed to defer with error %d\n", error);
        }
    }
    else
    {
        kprintf("Unknown command: %s\n", command);
    }
}

static void _shellGetCommand(void)
{

    char readChar;

    sInputBufferCursor = 0;
    kprintf("> ");
    kprintfFlush();
    consoleEcho(FALSE);
    while(TRUE)
    {
        consoleRead(&readChar, 1);

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
    sInputBuffer[sInputBufferCursor] = 0;
}

static void* _shellEntry(void* args)
{
    (void)args;

    /* Clear the console */
    consoleClear();
    consolePutCursor(0, 0);

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
                                    1,
                                    "kernelShell",
                                    0x1000,
                                    2,
                                    _shellEntry,
                                    NULL);
    if(error != OS_NO_ERR)
    {
        KERNEL_ERROR("Failed to start the kernel shell. Error %d\n", error);
    }
}

/************************************ EOF *************************************/