/*******************************************************************************
 * @file interrupts.h
 *
 * @see interrupts.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 3.0
 *
 * @brief Interrupt manager.
 *
 * @details Interrupt manager. Allows to attach ISR to interrupt lines and
 * manage IRQ used by the CPU. We also define the general interrupt handler
 * here.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_INTERRUPTS_H_
#define __CORE_INTERRUPTS_H_

#include <cpu.h>        /* CPU structures and settings */
#include <stdint.h>     /* Generic int types */
#include <stddef.h>     /* Standard definitions */
#include <kerror.h>     /* Kernel error codes */
#include <ctrl_block.h> /* Kernel control blocks */

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Interrupt types enumeration.
 */
typedef enum
{
    /** @brief Spurious interrupt type. */
    INTERRUPT_TYPE_SPURIOUS,
    /** @brief Regular interrupt type. */
    INTERRUPT_TYPE_REGULAR
} INTERRUPT_TYPE_E;

/**
 * @brief Custom interrupt handler structure.
 *
 * @details Custom interrupt handler structure. This is the function called on
 * an exception handling.
 *
 * @param[in, out] pCurrThread The current thread scheduled at the moment of the
 * exception.
 */
typedef void(*custom_handler_t)(kernel_thread_t* pCurrThread);

/** @brief Defines the basic interface for an interrupt management driver (let
 * it be PIC or IO APIC for instance).
 */
typedef struct
{
    /** @brief The function should enable or diable an IRQ given the IRQ number
     * used as parameter.
     *
     * @details The function should enable or diable an IRQ given the IRQ number
     * used as parameter.
     *
     * @param[in] kIrqNumber The number of the IRQ to enable/disable.
     * @param[in] kEnabled Must be set to TRUE to enable the IRQ and FALSE to
     * disable the IRQ.
     */
    void (*pSetIrqMask)(const uint32_t kIrqNumber, const bool_t kEnabled);

    /**
     * @brief The function should acknowleges an IRQ.
     *
     * @details The function should acknowleges an IRQ.
     *
     * @param[in] kIrqNumber The irq number to acknowledge.
     */
    void (*pSetIrqEOI)(const uint32_t kIrqNumber);

    /**
     * @brief The function should check if the serviced interrupt is a spurious
     * interrupt. It also should handle the spurious interrupt.
     *
     * @details The function should check if the serviced interrupt is a
     * spurious interrupt. It also should handle the spurious interrupt.
     *
     * @param[in] kIntNumber The interrupt number of the interrupt to test.
     *
     * @return The function will return the interrupt type.
     * - INTERRUPT_TYPE_SPURIOUS if the current interrupt is a spurious one.
     * - INTERRUPT_TYPE_REGULAR if the current interrupt is a regular one.
     */
    INTERRUPT_TYPE_E (*pHandleSpurious)(const uint32_t kIntNumber);

    /**
     * @brief Returns the interrupt line attached to an IRQ.
     *
     * @details Returns the interrupt line attached to an IRQ. -1 is returned
     * if the IRQ number is not supported by the driver.
     *
     * @param[in] kIrqNumber The IRQ line for the requested interrupt line.
     *
     * @return The interrupt line attached to an IRQ. -1 is returned if the IRQ
     * number is not supported by the driver.
     */
    int32_t (*pGetIrqInterruptLine)(const uint32_t kIrqNumber);
} interrupt_driver_t;

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
 * @brief Initializes the kernel's interrupt manager.
 *
 * @details Blanks the handlers memory, initializes panic and spurious interrupt
 * lines handlers.
 */
void interruptInit(void);

/**
 * @brief Kernel's main interrupt handler.
 *
 * @details Generic and global interrupt handler. This function should only be
 * called by an assembly interrupt handler. The function will dispatch the
 * interrupt to the desired function to handle the interrupt.
 */
void interruptMainHandler(void);

/**
 * @brief Set the driver to be used by the kernel to manage interrupts.
 *
 * @details Changes the current interrupt manager by the new driver given as
 * parameter. The old driver structure is removed from memory.
 *
 * @param[in] pDriver The driver structure to be used by the interrupt manager.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if one of the driver's function pointers
 * is NULL or the driver's pointer is NULL.
 */
OS_RETURN_E interruptSetDriver(const interrupt_driver_t* pDriver);

/**
 * @brief Registers a new interrupt handler for the desired IRQ number.
 *
 * @details Registers a custom interrupt handler to be executed. The IRQ
 * number must be greater or equal to the minimal authorized custom IRQ number
 * and less than the maximal one.
 *
 * @param[in] kIrqNumber The IRQ number to attach the handler to.
 * @param[in] handler The handler for the desired interrupt.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E interruptIRQRegister(const uint32_t   kIrqNumber,
                                 custom_handler_t handler);

/**
 * @brief Unregisters an interrupt handler for the desired IRQ number.
 *
 * @details Unregisters a custom interrupt handler to be executed. The IRQ
 * number must be greater or equal to the minimal authorized custom IRQ number
 * and less than the maximal one.
 *
 * @param[in] kIrqNumber The IRQ number to detach the handler from.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the IRQ attached to the
 * interrupt line is not allowed.
 * - OS_ERR_NO_SUCH_IRQ_LINE is returned if the IRQ number is not supported.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the IRQ has no handler
 * attached.
 */
OS_RETURN_E interruptIRQRemove(const uint32_t kIrqNumber);

/**
 * @brief Registers an interrupt handler for the desired interrupt line.
 *
 * @details Registers a custom interrupt handler to be executed. The interrupt
 * line must be greater or equal to the minimal authorized custom interrupt line
 * and less than the maximal one.
 *
 * @param[in] kIntLine The interrupt line to attach the handler to.
 * @param[in] handler The handler for the desired interrupt.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E interruptRegister(const uint32_t   kIntLine,
                              custom_handler_t handler);

/**
 * @brief Unregisters a new interrupt handler for the desired interrupt line.
 *
 * @details Unregisters a custom interrupt handler to be executed. The interrupt
 * line must be greater or equal to the minimal authorized custom interrupt line
 * and less than the maximal one.
 *
 * @param[in] kIntLine The interrupt line to deattach the handler from.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the desired
 * interrupt line is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the interrupt line has no
 * handler attached.
 */
OS_RETURN_E interruptRemove(const uint32_t kIntLine);

/**
 * @brief Restores the CPU interrupts state.
 *
 * @details Restores the CPU interrupts state by setting the EFLAGS accordingly.
 *
 * @param[in] kPreviousState The previous interrupts state that has to be
 * retored.
 */
void interruptRestore(const uint32_t kPreviousState);

/**
 * @brief Disables the CPU interrupts.
 *
 * @details Disables the CPU interrupts by setting the EFLAGS accordingly.
 *
 * @return The current interrupt state is returned to be restored latter in the
 * execution of the kernel.
 */
uint32_t interruptDisable(void);

/**
 * @brief Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @param[in] kIrqNumber The irq number to enable/disable.
 * @param[in] kEnabled Must be set to 1 to enable the IRQ or 0 to disable the
 * IRQ.
 */
void interruptIRQSetMask(const uint32_t kIrqNumber, const bool_t kEnabled);

/**
 * @brief Acknowleges an IRQ.
 *
 * @details Acknowleges an IRQ.
 *
 * @param[in] kIrqNumber The irq number to acknowledge.
 */
void interruptIRQSetEOI(const uint32_t kIrqNumber);

#endif /* #ifndef __CORE_INTERRUPTS_H_ */

/************************************ EOF *************************************/
