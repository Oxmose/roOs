/*******************************************************************************
 * @file lapic-timer.h
 *
 * @see lapic-timer.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 1.0
 *
 * @brief Local APIC (Advanced programmable interrupt controler) timer driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) timer driver.
 * Manages  the X86 LAPIC timer using the LAPIC driver. The LAPIC timer can be
 * used for scheduling purpose, limetime purpose or auxiliary timer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_LAPIC_TIMER_H_
#define __X86_LAPIC_TIMER_H_

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

/** @brief x86 LAPIC Timer driver. */
typedef struct
{
    /**
     * @brief Initializes a secondary core LAPIC Timer.
     *
     * @details Initializes a secondary core LAPIC Timer. This function
     * initializes the secondary core LAPIC timer interrupts and settings.
     *
     * @param[in] kCpuId The CPU identifier for which we should enable the LAPIC
     * timer.
     */
    void (*pInitApCore)(const uint8_t kCpuId);
} lapic_timer_driver_t;

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

#endif /* #ifndef __X86_LAPIC_TIMER_H_ */

/************************************ EOF *************************************/