/*******************************************************************************
 * @file lapic.h
 *
 * @see lapic.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 2.0
 *
 * @brief Local APIC (Advanced programmable interrupt controler) driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) driver.
 * Manages x86 IRQs from the IO-APIC. IPI (inter processor interrupt) are also
 * possible thanks to the driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_LAPIC_H_
#define __X86_LAPIC_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Standard int definition */
#include <stddef.h> /* Standard definitions */
/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 LAPIC driver. */
typedef struct
{
    /**
     * @brief Sets END OF INTERRUPT for the current CPU Local APIC.
     *
     * @details Sets END OF INTERRUPT for the current CPU Local APIC.
     *
     * @param[in] kInterruptLine The interrupt line for which the EOI should be
     * set.
     */
    void (*pSetIrqEOI)(const uint32_t kInterruptLine);

    /**
     * @brief Returns the base address of the local APIC.
     *
     * @details Returns the base address of the local APIC.
     *
     * @return The base address of the LAPIC is returned.
     */
    uintptr_t (*pGetBaseAddress)(void);
} lapic_driver_t;

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

/* None */

#endif /* #ifndef __X86_LAPIC_H_ */

/************************************ EOF *************************************/