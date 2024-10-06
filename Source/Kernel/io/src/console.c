/*******************************************************************************
 * @file console.c
 *
 * @see console.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.0
 *
 * @brief Console drivers abtraction.
 *
 * @details Console driver abtraction layer. The functions of this module allows
 * to abtract the use of any supported console driver and the selection of the
 * desired driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>       /* Virtual Filesystem */
#include <ioctl.h>     /* IOCTL commands */
#include <string.h>    /* String manipulation */
#include <stdint.h>    /* Generic int types */
#include <stddef.h>    /* Standard definition */
#include <syslog.h>    /* Syslog service */
#include <kerror.h>    /* Kernel error codes */
#include <devtree.h>   /* Device tree manager */

#if DEBUG_LOG_UART
#include <uart.h>   /* Include the UART for debug */
#endif

/* Configuration files */
#include <config.h>

/* Header file */
#include <console.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the current module name */
#define MODULE_NAME "CONS"

/** @brief FDT console node name */
#define FDT_CONSOLE_NODE_NAME "console"

/** @brief FDT property for the console input device property */
#define FDT_CONSOLE_INPUT_DEV_PROP "inputdev"
/** @brief FDT property for the console output device property */
#define FDT_CONSOLE_OUTPUT_DEV_PROP "outputdev"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Execute a function if it exists.
 *
 * @param[in] FUNC The function to execute.
 * @param[in, out] ... Function parameters.
 */
#define EXEC_IF_SET(DRIVER, FUNC, ...) { \
    if(DRIVER.FUNC != NULL)              \
    {                                    \
        DRIVER.FUNC(__VA_ARGS__);        \
    }                                    \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the stdin file descriptor */
static int32_t sStdinFd = -1;

/** @brief Stores the stdout file descriptor */
static int32_t sStdoutFd = -1;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void consoleInit(void)
{
    const fdt_node_t* kpConsoleNode;
    const char*       kpStrProp;
    size_t            propLen;

    /* Get the FDT console node */
    kpConsoleNode = fdtGetNodeByName(FDT_CONSOLE_NODE_NAME);
    if(kpConsoleNode == NULL)
    {
        return;
    }

    /* Get the input device */
    kpStrProp = fdtGetProp(kpConsoleNode,
                           FDT_CONSOLE_INPUT_DEV_PROP,
                           &propLen);
    if(kpStrProp != NULL && propLen > 0)
    {
        sStdinFd = vfsOpen(kpStrProp, O_RDONLY, 0);
        if(sStdinFd < 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to open stdin device");
        }
    }

    /* Get the output device */
    kpStrProp = fdtGetProp(kpConsoleNode,
                           FDT_CONSOLE_OUTPUT_DEV_PROP,
                           &propLen);
    if(kpStrProp != NULL && propLen > 0)
    {
        sStdoutFd = vfsOpen(kpStrProp, O_RDWR, 0);
        if(sStdoutFd < 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to open stdout device");
        }
    }
}

void consoleClear(void)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_CLEAR, NULL);
    }
}

void consolePutCursor(const uint32_t kLine, const uint32_t kColumn)
{
    cursor_t cursor;

    if(sStdoutFd >= 0)
    {
        cursor.x = kColumn;
        cursor.y = kLine;

        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_RESTORE_CURSOR, &cursor);
    }
}

void consoleSaveCursor(cursor_t* pBuffer)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_SAVE_CURSOR, pBuffer);
    }
}

void consoleRestoreCursor(const cursor_t* pkBuffer)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_RESTORE_CURSOR, (void*)&pkBuffer);
    }
}

void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines)
{
    cons_ioctl_args_scroll_t args;

    if(sStdoutFd >= 0)
    {
        args.direction = kDirection;
        args.lineCount = kLines;
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_SCROLL, &args);
    }
}

void consoleSetColorScheme(const colorscheme_t* pkColorScheme)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd,
                 VFS_IOCTL_CONS_SET_COLORSCHEME,
                 (void*)pkColorScheme);
    }
}

void consoleSaveColorScheme(colorscheme_t* pBuffer)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_SAVE_COLORSCHEME, pBuffer);
    }
}

void consolePutString(const char* pkString)
{

#if DEBUG_LOG_UART
    uartDebugPutString(pkString);
#endif
    if(sStdoutFd >= 0)
    {
        vfsWrite(sStdoutFd, pkString, strlen(pkString));
    }
}

void consolePutChar(const char kCharacter)
{

#if DEBUG_LOG_UART
    uartDebugPutChar(kCharacter);
#endif
    if(sStdoutFd >= 0)
    {
        vfsWrite(sStdoutFd, (void*)&kCharacter, 1);
    }
}

ssize_t consoleRead(char* pBuffer, size_t kBufferSize)
{
    ssize_t retVal;

    if(sStdinFd >= 0)
    {
        retVal = vfsRead(sStdinFd, pBuffer, kBufferSize);
    }
    else
    {
        retVal = -1;
    }

    return retVal;
}

void consoleFlush(void)
{

    if(sStdoutFd >= 0)
    {
        vfsIOCTL(sStdoutFd, VFS_IOCTL_CONS_FLUSH, NULL);
    }
}

/************************************ EOF *************************************/