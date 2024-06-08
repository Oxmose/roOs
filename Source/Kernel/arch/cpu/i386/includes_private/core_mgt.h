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

#ifndef __I386_CORE_MGT_
#define __I386_CORE_MGT_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <lapic.h>       /* LAPIC driver */
#include <lapic_timer.h> /* LAPIC timer driver */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief CPU IPI send flag: send to all CPUs but the calling one. */
#define CORE_MGT_IPI_BROADCAST_TO_OTHER 0x100

/** @brief CPU IPI send flag: send to all CPUs including the calling one. */
#define CORE_MGT_IPI_BROADCAST_TO_ALL 0x300

/** @brief CPU IPI send flag: send to a specific CPU using its id. */
#define CORE_MGT_IPI_SEND_TO(X) ((X) & 0xFF)


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

/**
 * @brief Sends an IPI to the cores.
 *
 * @details Sends an IPI to the cores. The flags define the nature of the
 * IPI, if it should be broadcasted, including the calling core, etc.
 *
 * @param[in] kFlags The flags to use, see IPI flags definition.
 * @param[in] kVector The IPI vector to use.
 */
void coreMgtSendIpi(const uint32_t kFlags, const uint8_t kVector);


#endif /* #ifndef __I386_CORE_MGT_ */

/************************************ EOF *************************************/