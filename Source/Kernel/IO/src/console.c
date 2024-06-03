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
    sConsoleDriver = *pkDriver;
    return OS_NO_ERR;
}

void consoleClear(void)
{
    EXEC_IF_SET(sConsoleDriver, pClear, sConsoleDriver.pDriverCtrl);
}

void consolePutCursor(const uint32_t kLine, const uint32_t kColumn)
{
    EXEC_IF_SET(sConsoleDriver,
                pPutCursor,
                sConsoleDriver.pDriverCtrl,
                kLine,
                kColumn);
}

void consoleSaveCursor(cursor_t* pBuffer)
{
    EXEC_IF_SET(sConsoleDriver,
                pSaveCursor,
                sConsoleDriver.pDriverCtrl,
                pBuffer);
}

void consoleRestoreCursor(const cursor_t* pkBuffer)
{
    EXEC_IF_SET(sConsoleDriver,
                pRestoreCursor,
                sConsoleDriver.pDriverCtrl,
                pkBuffer);
}

void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines)
{
    EXEC_IF_SET(sConsoleDriver,
                pScroll,
                sConsoleDriver.pDriverCtrl,
                kDirection,
                kLines);
}

void consoleSetColorScheme(const colorscheme_t* pkColorScheme)
{
    EXEC_IF_SET(sConsoleDriver,
                pSetColorScheme,
                sConsoleDriver.pDriverCtrl,
                pkColorScheme);
}

void consoleSaveColorScheme(colorscheme_t* pBuffer)
{
    EXEC_IF_SET(sConsoleDriver,
                pSaveColorScheme,
                sConsoleDriver.pDriverCtrl,
                pBuffer);
}

void consolePutString(const char* pkString)
{
#if DEBUG_LOG_UART
    uartDebugPutString(pkString);
#endif
    EXEC_IF_SET(sConsoleDriver,
                pPutString,
                sConsoleDriver.pDriverCtrl,
                pkString);
}

void consolePutChar(const char kCharacter)
{
#if DEBUG_LOG_UART
    uartDebugPutChar(kCharacter);
#endif
    EXEC_IF_SET(sConsoleDriver,
                pPutChar,
                sConsoleDriver.pDriverCtrl,
                kCharacter);
}

/************************************ EOF *************************************/