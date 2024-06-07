/*******************************************************************************
 * @file cpu.h
 *
 * @see cpu.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Generic CPU management functions
 *
 * @details Generic CPU manipulation functions. The underlying platform should
 * implement those calls.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_H_
#define __CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Structure that contains the CPU interrupts fonfiguration. */
typedef struct
{
    /** @brief Minimal exception line */
    uint32_t minExceptionLine;
    /** @brief Maximal exception line */
    uint32_t maxExceptionLine;
    /** @brief Minimal interrupt line */
    uint32_t minInterruptLine;
    /** @brief Maximal interrupt line */
    uint32_t maxInterruptLine;
    /** @brief Total interrupt lines, including interrupts and exceptions */
    uint32_t totalInterruptLineCount;
    /** @brief Kernel panic interrupt line id */
    uint32_t panicInterruptLine;
    /** @brief Spurious interrupts line id */
    uint32_t spuriousInterruptLine;
} cpu_interrupt_config_t;

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
 * @brief Raises CPU interrupt.
 *
 * @details Raises a software CPU interrupt on the desired line.
 *
 * @param[in] kInterruptLine The line on which the interrupt should be raised.
 *
 * @return OS_NO_ERR shoudl be return in case of success.
 * - OS_ERR_UNAUTHORIZED_ACTION Is returned if the interrupt line is not
 * correct.
 */
OS_RETURN_E cpuRaiseInterrupt(const uint32_t kInterruptLine);

/**
 * @brief Returns the saved interrupt state.
 *
 * @details Returns the saved interrupt state based on the virtual CPU.
 *
 * @param[in] kpVCpu The virtual CPU to extract the information from.
 *
 * @return The current saved interrupt state: 1 if enabled, 0 otherwise.
 */
uint32_t cpuGetContextIntState(const void* kpVCpu);

/**
 * @brief Returns the last interrupt registered for the virtual CPU.
 *
 * @details Returns the last interrupt registered for the virtual CPU.
 *
 * @param[in] kpVCpu The virtual CPU to extract the information from.
 *
 * @return The current saved interrupt number.
 */
uint32_t cpuGetContextIntNumber(const void* kpVCpu);

/**
 * @brief Returns the CPU's interrupt configuration.
 *
 * @details Returns the CPU's interrupt configuration. The configuration is
 * initialized during the CPU initialization.
 *
 * @return The CPU's interrupt configuration is returned.
 */
const cpu_interrupt_config_t* cpuGetInterruptConfig(void);

/**
 * @brief Initializes the CPU.
 *
 * @details Initializes the CPU registers and relevant structures.
 */
void cpuInit(void);

/**
 * @brief Checks the architecture's feature and requirements for UTK.
 *
 * @details Checks the architecture's feature and requirements for UTK. If a
 * requirement is not met, a kernel panic is raised.
 */
void cpuValidateArchitecture(void);

/**
 * @brief Returns the CPU current interrupt state.
 *
 * @details Returns the current CPU eflags interrupt enable value.
 *
 * @return The CPU current interrupt state: 1 if enabled, 0 otherwise.
 */
uint32_t cpuGeIntState(void);

/** @brief Clears interupt bit which results in disabling interrupts. */
void cpuClearInterrupt(void);

/** @brief Sets interrupt bit which results in enabling interupts. */
void cpuSetInterrupt(void);

/** @brief Halts the CPU for lower energy consuption. */
void cpuHalt(void);


#endif /* #ifndef __CPU_H_ */

/************************************ EOF *************************************/