/*******************************************************************************
 * @file core_mgt.h
 *
 * @see cpu.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 07/06/2024
 *
 * @version 1.0
 *
 * @brief Core manager services. Used to identify cores, manage multicore
 * features, etc.
 *
 * @details Core manager services. Used to identify cores, manage multicore
 * features and other features related to the cpu / bsp interface.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_CORE_MGT_
#define __X86_CORE_MGT_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <lapic.h>       /* LAPIC driver */
#include <lapic_timer.h> /* LAPIC timer driver */

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
 * @brief Registers the LAPIC driver.
 *
 * @details Registers the LAPIC driver used by the core manager to end IPI,
 * start other CPUs and get the LAPIC Ids. This function must be called
 * before any other in the core manager. The function should only be called
 * once.
 *
 * @param[in] kpLapicDriver The LAPIC driver to register.
 */
void coreMgtRegLapicDriver(const lapic_driver_t* kpLapicDriver);

/**
 * @brief Registers the LAPIC Timer driver.
 *
 * @details Registers the LAPIC Timer driver used by the core manager to
 * initialize the LAPIC Timer for secondary cores
 *
 * @param[in] kpLapicDriver The LAPIC driver to register.
 */
void coreMgtRegLapicTimerDriver(const lapic_timer_driver_t* kpLapicTimerDrv);

/**
 * @brief Initializes the core manager.
 *
 * @details Intializes the core manager. During initialization, secondary CPUs
 * detection and enabling is done if possible. After this call, it is possible
 * that more core execute in the system.
 */
void coreMgtInit(void);

/**
 * @brief Initializes the secondary CPU cores.
 *
 * @details Initializes the secondary CPU cores. This function will setup the
 * internal CPU facilities such as LAPIC, timers etc.
 *
 * @param[in] kCpuId The identifier of the CPU to initialize.
 *
 * @warning This function should only be called by initializing CPUs / cores.
 */
void coreMgtApInit(const uint8_t kCpuId);

#endif /* #ifndef __X86_CORE_MGT_ */

/************************************ EOF *************************************/