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
#include <stdint.h> /* Generic int types and bool */
#include <stddef.h> /* Standard definition */
#include <kerror.h> /* Kernel error codes */

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

/* None */

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
 * @param[in/out] ... Function parameters.
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

OS_RETURN_E consoleSetDriver(const console_driver_t* pkDriver)
{
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_START, 2,
                       (uintptr_t)pkDriver & 0xFFFFFFFF,
                       (uintptr_t)pkDriver >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_START, 2,
                       (uintptr_t)driver,
                       0);
#endif

    sConsoleDriver = *pkDriver;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_END, 1,
                       OS_NO_ERR);
    return OS_NO_ERR;
}

void consoleClear(void)
{
    EXEC_IF_SET(sConsoleDriver, pClear);
}

void consolePutCursor(const uint32_t kLine, const uint32_t kColumn)
{
    EXEC_IF_SET(sConsoleDriver, pPutCursor, kLine, kColumn);
}

void consoleSaveCursor(cursor_t* pBuffer)
{
    EXEC_IF_SET(sConsoleDriver, pSaveCursor, pBuffer);
}

void consoleRestoreCursor(const cursor_t* pkBuffer)
{
    EXEC_IF_SET(sConsoleDriver, pRestoreCursor, pkBuffer);
}

void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines)
{
    EXEC_IF_SET(sConsoleDriver, pScroll, kDirection, kLines);
}

void consoleSetColorScheme(const colorscheme_t* pkColorScheme)
{
    EXEC_IF_SET(sConsoleDriver, pSetColorScheme, pkColorScheme);
}

void consoleSaveColorScheme(colorscheme_t* pBuffer)
{
    EXEC_IF_SET(sConsoleDriver, pSaveColorScheme, pBuffer);
}

void consolePutString(const char* pkString)
{
#if DEBUG_LOG_UART
    uart_put_string(pkString);
#endif
    EXEC_IF_SET(sConsoleDriver, pPutString, pkString);
}

void consolePutChar(const char kCharacter)
{
#if DEBUG_LOG_UART
    uart_put_char(kCharacter);
#endif
    EXEC_IF_SET(sConsoleDriver, pPutChar, kCharacter);
}

/************************************ EOF *************************************/