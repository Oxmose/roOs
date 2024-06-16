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

/* Tracing feature */
#include <tracing.h>

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

OS_RETURN_E consoleSetDriver(const console_driver_t* pkDriver)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_DRIVER_ENTRY,
                       2,
                       0,
                       (uint32_t)pkDriver);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_DRIVER_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pkDriver >> 32),
                       (uint32_t)(uintptr_t)pkDriver);
#endif

    sConsoleDriver = *pkDriver;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_DRIVER_EXIT,
                       2,
                       0,
                       (uint32_t)pkDriver);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_DRIVER_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pkDriver >> 32),
                       (uint32_t)(uintptr_t)pkDriver);
#endif
    return OS_NO_ERR;
}

void consoleClear(void)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED, TRACE_CONS_CLEAR_ENTRY, 0);

    EXEC_IF_SET(sConsoleDriver, pClear, sConsoleDriver.pDriverCtrl);

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
                pPutCursor,
                sConsoleDriver.pDriverCtrl,
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
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_ENTRY,
                       2,
                       0,
                       (uint32_t)pBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pBuffer >> 32),
                       (uint32_t)(uintptr_t)pBuffer);
#endif

    EXEC_IF_SET(sConsoleDriver,
                pSaveCursor,
                sConsoleDriver.pDriverCtrl,
                pBuffer);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_EXIT,
                       2,
                       0,
                       (uint32_t)pBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_CURSOR_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pBuffer >> 32),
                       (uint32_t)(uintptr_t)pBuffer);
#endif
}

void consoleRestoreCursor(const cursor_t* pkBuffer)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_ENTRY,
                       2,
                       0,
                       (uint32_t)pkBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pkBuffer >> 32),
                       (uint32_t)(uintptr_t)pkBuffer);
#endif

    EXEC_IF_SET(sConsoleDriver,
                pRestoreCursor,
                sConsoleDriver.pDriverCtrl,
                pkBuffer);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_EXIT,
                       2,
                       0,
                       (uint32_t)pkBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_RESTORE_CURSOR_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pkBuffer >> 32),
                       (uint32_t)(uintptr_t)pkBuffer);
#endif
}

void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines)
{
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SCROLL_ENTRY,
                       2,
                       kDirection,
                       kLines);

    EXEC_IF_SET(sConsoleDriver,
                pScroll,
                sConsoleDriver.pDriverCtrl,
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
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_ENTRY,
                       2,
                       0,
                       (uint32_t)pkColorScheme);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pkColorScheme >> 32),
                       (uint32_t)(uintptr_t)pkColorScheme);
#endif

    EXEC_IF_SET(sConsoleDriver,
                pSetColorScheme,
                sConsoleDriver.pDriverCtrl,
                pkColorScheme);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_EXIT,
                       2,
                       0,
                       (uint32_t)pkColorScheme);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SET_COLORSCHEME_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pkColorScheme >> 32),
                       (uint32_t)(uintptr_t)pkColorScheme);
#endif
}

void consoleSaveColorScheme(colorscheme_t* pBuffer)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_ENTRY,
                       2,
                       0,
                       (uint32_t)pBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pBuffer >> 32),
                       (uint32_t)(uintptr_t)pBuffer);
#endif

    EXEC_IF_SET(sConsoleDriver,
                pSaveColorScheme,
                sConsoleDriver.pDriverCtrl,
                pBuffer);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_EXIT,
                       2,
                       0,
                       (uint32_t)pBuffer);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_SAVE_COLORSCHEME_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pBuffer >> 32),
                       (uint32_t)(uintptr_t)pBuffer);
#endif
}

void consolePutString(const char* pkString)
{

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_ENTRY,
                       2,
                       0,
                       (uint32_t)pkString);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pkString >> 32),
                       (uint32_t)(uintptr_t)pkString);
#endif

#if DEBUG_LOG_UART
    uartDebugPutString(pkString);
#endif
    EXEC_IF_SET(sConsoleDriver,
                pPutString,
                sConsoleDriver.pDriverCtrl,
                pkString);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_EXIT,
                       2,
                       0,
                       (uint32_t)pkString);
#else
    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_STRING_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pkString >> 32),
                       (uint32_t)(uintptr_t)pkString);
#endif
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
                pPutChar,
                sConsoleDriver.pDriverCtrl,
                kCharacter);

    KERNEL_TRACE_EVENT(TRACE_CONS_ENABLED,
                       TRACE_CONS_PUT_CHAR_EXIT,
                       1,
                       kCharacter);
}

/************************************ EOF *************************************/