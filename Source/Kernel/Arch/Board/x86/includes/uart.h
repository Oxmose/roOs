/*******************************************************************************
 * @file uart.h
 *
 * @see uart.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.0
 *
 * @brief UART communication driver.
 *
 * @details UART communication driver. Initializes the uart ports as in and
 * output. The uart can be used to output data or communicate with other
 * prepherals that support this communication method
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_UART_H_
#define __X86_UART_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <config.h>  /* Configuration */

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

/* None */

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

#if DEBUG_LOG_UART
/**
 * @brief Initializes the UART debugging driver.
 *
 * @details Initializes the UART debugging driver. This UART is used to output
 * debugging printfs used by the kernel an drivers.
 */
void uartDebugInit(void);

/**
 * @brief Write the string given as patameter on the debug port.
 *
 * @details The function will output the data given as parameter on the debug
 * port. This call is blocking until the data has been sent to the uart port
 * controler.
 *
 * @param[in, out] pDrvCtrl The UART driver controler to use.
 * @param[in] kpString The string to write to the uart port.
 *
 * @warning string must be NULL terminated.
 */
void uartDebugPutString(const char* kpString);

/**
 * @brief Write the character given as patameter on the debug port.
 *
 * @details The function will output the character given as parameter on the
 * debug port. This call is blocking until the data has been sent to the uart
 * port controler.
 *
 * @param[in, out] pDrvCtrl The UART driver controler to use.
 * @param[in] kCharacter The character to write to the uart port.
 */
void uartDebugPutChar(const char kCharacter);
#endif

#endif /* #ifndef __X86_UART_H_ */

/************************************ EOF *************************************/
