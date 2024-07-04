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

    if(strcmp(command, "hello") == 0)
    {
        kprintf("Hi! I am UTK :)\n");
        kprintf("Your arguments are: %s\n", sInputBuffer + cursor);
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
                                    "K_SHELL",
                                    0x1000,
                                    0,
                                    _shellEntry,
                                    NULL);
    if(error != OS_NO_ERR)
    {
        KERNEL_ERROR("Failed to start the kernel shell. Error %d\n", error);
    }
}

/************************************ EOF *************************************/