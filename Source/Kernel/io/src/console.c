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
#include <stdint.h>    /* Generic int types and bool_t */
#include <stddef.h>    /* Standard definition */
#include <kerror.h>    /* Kernel error codes */
#include <devtree.h>   /* Device tree manager */
#include <drivermgr.h> /* Driver manager */

#if DEBUG_LOG_UART
#include <uart.h>   /* Include the UART for debug */
#endif

/* Configuration files */
#include <config.h>

/* Header file */
#include <console.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

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
/** @brief Stores the currently selected driver */
static console_driver_t sConsoleDriver = {NULL};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void consoleInit(void)
{
    const fdt_node_t* kpConsoleNode;
    const uint32_t*   kpUintProp;
    size_t            propLen;
    console_driver_t* pConsDriver;

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_INIT_ENTRY, 0);

    /* Get the FDT console node */
    kpConsoleNode = fdtGetNodeByName(FDT_CONSOLE_NODE_NAME);
    if(kpConsoleNode == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_INIT_EXIT, 1, -1);
        return;
    }

    /* Get the input driver */
    kpUintProp = fdtGetProp(kpConsoleNode,
                            FDT_CONSOLE_INPUT_DEV_PROP,
                            &propLen);
    if(kpUintProp != NULL && propLen == sizeof(uint32_t))
    {
        pConsDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
        if(pConsDriver != NULL && pConsDriver->inputDriver.pDriverCtrl != NULL)
        {
            sConsoleDriver.inputDriver = pConsDriver->inputDriver;
        }
    }

    /* Get the output driver */
    kpUintProp = fdtGetProp(kpConsoleNode,
                            FDT_CONSOLE_OUTPUT_DEV_PROP,
                            &propLen);
    if(kpUintProp != NULL && propLen == sizeof(uint32_t))
    {
        pConsDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
        if(pConsDriver != NULL && pConsDriver->outputDriver.pDriverCtrl != NULL)
        {
            sConsoleDriver.outputDriver = pConsDriver->outputDriver;
        }
    }

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_INIT_EXIT, 1, 0);
}

void consoleClear(void)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_CLEAR_ENTRY, 0);

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pClear,
                sConsoleDriver.outputDriver.pDriverCtrl);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_CLEAR_EXIT, 0);
}

void consolePutCursor(const uint32_t kLine, const uint32_t kColumn)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_CURSOR_ENTRY,
                       2,
                       kLine,
                       kColumn);

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pPutCursor,
                sConsoleDriver.outputDriver.pDriverCtrl,
                kLine,
                kColumn);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_CURSOR_EXIT,
                       2,
                       kLine,
                       kColumn);
}

void consoleSaveCursor(cursor_t* pBuffer)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer));

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pSaveCursor,
                sConsoleDriver.outputDriver.pDriverCtrl,
                pBuffer);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer));
}

void consoleRestoreCursor(const cursor_t* pkBuffer)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pkBuffer),
                       KERNEL_TRACE_LOW(pkBuffer));

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pRestoreCursor,
                sConsoleDriver.outputDriver.pDriverCtrl,
                pkBuffer);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pkBuffer),
                       KERNEL_TRACE_LOW(pkBuffer));
}

void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SCROLL_ENTRY,
                       2,
                       kDirection,
                       kLines);

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pScroll,
                sConsoleDriver.outputDriver.pDriverCtrl,
                kDirection,
                kLines);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SCROLL_EXIT,
                       2,
                       kDirection,
                       kLines);
}

void consoleSetColorScheme(const colorscheme_t* pkColorScheme)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pkColorScheme),
                       KERNEL_TRACE_LOW(pkColorScheme));

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pSetColorScheme,
                sConsoleDriver.outputDriver.pDriverCtrl,
                pkColorScheme);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pkColorScheme),
                       KERNEL_TRACE_LOW(pkColorScheme));
}

void consoleSaveColorScheme(colorscheme_t* pBuffer)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer));

    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pSaveColorScheme,
                sConsoleDriver.outputDriver.pDriverCtrl,
                pBuffer);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer));
}

void consolePutString(const char* pkString)
{

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pkString),
                       KERNEL_TRACE_LOW(pkString));

#if DEBUG_LOG_UART
    uartDebugPutString(pkString);
#endif
    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pPutString,
                sConsoleDriver.outputDriver.pDriverCtrl,
                pkString);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pkString),
                       KERNEL_TRACE_LOW(pkString));
}

void consolePutChar(const char kCharacter)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_CHAR_ENTRY,
                       1,
                       kCharacter);

#if DEBUG_LOG_UART
    uartDebugPutChar(kCharacter);
#endif
    EXEC_IF_SET(sConsoleDriver,
                outputDriver.pPutChar,
                sConsoleDriver.outputDriver.pDriverCtrl,
                kCharacter);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_CHAR_EXIT,
                       1,
                       kCharacter);
}

ssize_t consoleRead(char* pBuffer, size_t kBufferSize)
{
    ssize_t retVal;

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_READ_ENTRY,
                       4,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer),
                       KERNEL_TRACE_HIGH(kBufferSize),
                       KERNEL_TRACE_LOW(kBufferSize));

    if(sConsoleDriver.inputDriver.pRead != NULL)
    {
        retVal = sConsoleDriver.inputDriver.pRead(
                                         sConsoleDriver.inputDriver.pDriverCtrl,
                                         pBuffer,
                                         kBufferSize);
    }
    else
    {
        retVal = -1;
    }
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_READ_EXIT,
                       5,
                       KERNEL_TRACE_HIGH(pBuffer),
                       KERNEL_TRACE_LOW(pBuffer),
                       KERNEL_TRACE_HIGH(kBufferSize),
                       KERNEL_TRACE_LOW(kBufferSize),
                       retVal);

    return retVal;
}

void consoleEcho(const bool_t kEnable)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_ECHO_ENTRY,
                       1,
                       kEnable);

    EXEC_IF_SET(sConsoleDriver,
                inputDriver.pEcho,
                sConsoleDriver.inputDriver.pDriverCtrl,
                kEnable);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_ECHO_EXIT,
                       1,
                       kEnable);
}

/************************************ EOF *************************************/