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
#include <string.h>       /* String manipulation */
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

static void* _shellEntry(void* args);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _shellPrintHeader(void)
{
    uint8_t       i;
    uint8_t       cpuId;
    cursor_t      savedCursor;
    colorscheme_t savedColorScheme;
    colorscheme_t colorScheme;
    time_t        currTime;

    /* Save cursor and scheme */
    consoleSaveCursor(&savedCursor);
    consoleSaveColorScheme(&savedColorScheme);

    /* Set header position and scheme */
    colorScheme.background = BG_CYAN;
    colorScheme.foreground = FG_BLACK;
    colorScheme.vgaColor = TRUE;
    consoleSetColorScheme(&colorScheme);
    consolePutCursor(0, 0);

    /* Print the shell version */
    kprintf("UTK Shell V" SHELL_VERION);
    for(i = 0; i < 36 - strlen("UTK Shell V" SHELL_VERION); ++i)
    {
        kprintf(" ");
    }

    /* Print time */
    currTime = timeGetDayTime();
    kprintf("%02d:%02d:%02d",
            currTime.hours,
            currTime.minutes,
            currTime.seconds);

    /* Print current CPU */
    cpuId = cpuGetId();
    for(i = 0; i < 22; ++i)
    {
        kprintf(" ");
    }
    kprintf("CPU: %02d (%03d%%)", cpuId, (uint32_t)schedGetCpuLoad(cpuId));


    /* Flush buffer */
    kprintfFlush();

    consolePutCursor(savedCursor.y, savedCursor.y);
    consoleSetColorScheme(&savedColorScheme);
}

static void* _shellEntry(void* args)
{
    (void)args;

    /* Clear the console */
    consoleClear();

    while(TRUE)
    {
        _shellPrintHeader();

        schedSleep(1000000000);
    }

    return (void*)0;
}

unsigned long next=1;
int rand(void) ;
void* testThread(void* args);
int rand(void)
{
    next = next*1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void* testThread(void* args)
{
    (void)args;

    while(1)
    {
        for(volatile int i = 0; i < 100 * (rand() % 1000000000); ++i)
        {

        }
        schedSleep(1000000);
    }

    return NULL;
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
                                    "K_SHELL",
                                    0x1000,
                                    0,
                                    _shellEntry,
                                    NULL);
    error = schedCreateKernelThread(&pShellThread,
                                    1,
                                    "K_SHELLTEST",
                                    0x1000,
                                    0,
                                    testThread,
                                    NULL);

    if(error != OS_NO_ERR)
    {
        KERNEL_ERROR("Failed to start the kernel shell. Error %d\n", error);
    }
}

/************************************ EOF *************************************/