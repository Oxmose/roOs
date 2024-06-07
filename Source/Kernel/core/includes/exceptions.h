/*******************************************************************************
 * @file exceptions.h
 *
 * @see exceptions.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/05/2023
 *
 * @version 2.0
 *
 * @brief Exceptions manager.
 *
 * @warning These functions must be called during or after the interrupts are
 * set.
 *
 * @details Exception manager. Allows to attach ISR to exceptions lines.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_EXCEPTIONS_H_
#define __CORE_EXCEPTIONS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cpu.h>        /* CPU structures and settings */
#include <stdint.h>     /* Generic int types */
#include <stddef.h>     /* Standard definitions */
#include <kerror.h>     /* Kernel error codes */
#include <ctrl_block.h> /* Kernel control blocks */
#include <interrupts.h> /* Interrupts blocks */

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

/**
 * @brief Initializes the exception manager.
 *
 * @details Blanks the handlers memory and initialize the first 32 exceptions to
 * catch exceptions.
 */
void exceptionInit(void);

/**
 * @brief Registers a new exception handler for the desired exception line.
 *
 * @details Registers a custom exception handler to be executed. The exception
 * line must be greater or equal to the minimal authorized custom exception line
 * and less than the maximal one.
 *
 * @param[in] kExceptionLine The exception line to attach the handler to.
 * @param[in] handler The handler for the desired exception.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the desired
 * exception line is not allowed.
 * - OS_ERR_NULL_POINTER is returned if the pointer
 * to the handler is NULL.
 * - OS_ERR_INTERRUPT_ALREADY_REGISTERED is returned if a
 * handler is already registered for this exception line.
 */
OS_RETURN_E exceptionRegister(const uint32_t   kExceptionLine,
                              custom_handler_t handler);

/**
 * @brief Unregisters a new exception handler for the desired exception line.
 *
 * @details Unregisters a custom exception handler to be executed. The exception
 * line must be greater or equal to the minimal authorized custom exception line
 * and less than the maximal one.
 *
 * @param[in] kExceptionLine The exception line to deattach the handler from.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the desired
 * exception line is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the exception line has no
 * handler attached.
 */
OS_RETURN_E exceptionRemove(const uint32_t kExceptionLine);

#endif /* #ifndef __CORE_EXCEPTIONS_H_ */

/************************************ EOF *************************************/