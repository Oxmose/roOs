/*******************************************************************************
 * @file core_mgt.c
 *
 * @see cpu.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <acpi.h>         /* ACPI structures */
#include <lapic.h>        /* LAPIC driver */
#include <x86cpu.h>       /* CPU management */
#include <stdint.h>       /* Generic int types */
#include <devtree.h>      /* Device tree library */
#include <console.h>      /* Console service */
#include <drivermgr.h>    /* Driver manager */
#include <ctrl_block.h>   /* Thread's control block */
#include <lapic_timer.h>  /* LAPIC timer driver */
#include <kerneloutput.h> /* Kernel output methods */

/* Configuration files */
#include <config.h>

/* Header file */
#include <core_mgt.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#if MAX_CPU_COUNT <= 0
#error "MAX_CPU_COUNT must be greater or equal to 1"
#endif

/** @brief Current module name */
#define MODULE_NAME "I386 CORE MGT"

/** @brief LAPIC flag: enabled (running) */
#define LAPIC_FLAG_ENABLED 0x1

/** @brief LAPIC flag: capable (available to be started) */
#define LAPIC_FLAG_CAPABLE 0x2

/** @brief IPI send flag CPU mask */
#define CORE_MGT_IPI_SEND_TO_CPU_MASK (CORE_MGT_IPI_SEND_TO(0xFFFFFFFF));

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Stores the number of enabled (running) cores in the system. */
extern volatile uint32_t _bootedCPUCount;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Stores the number of available cores in the system. */
static uint8_t sAvailabeCpuCount = 1;

/** @brief Stores the translated CPU identifiers */
static uint8_t sCoreIds[MAX_CPU_COUNT] = {0};

#if MAX_CPU_COUNT > 1

/** @brief Stores the LAPIC driver instance */
static const lapic_driver_t* kspLapicDriver = NULL;

/** @brief Stores the LAPIC timer driver instance */
static const lapic_timer_driver_t* kspLapicTimerDriver = NULL;

#endif

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

#if MAX_CPU_COUNT > 1
void coreMgtRegLapicDriver(const lapic_driver_t* kpLapicDriver)
{
    kspLapicDriver = kpLapicDriver;
}

void coreMgtRegLapicTimerDriver(const lapic_timer_driver_t* kpLapicTimerDriver)
{
    kspLapicTimerDriver = kpLapicTimerDriver;
}

void coreMgtInit(void)
{
    const lapic_node_t* kpLapicNode;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_INIT_ENTRY,
                       0);

    /* Check if the LAPIC driver was registered */
    if(kspLapicDriver == NULL)
    {
        KERNEL_ERROR("LAPIC driver was not registered to core manager.\n"
                     "Continuing with one core.\n");
        KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                           TRACE_X86_CPU_CORE_MGT_INIT_EXIT,
                           1,
                           -1);
        return;
    }

    if(_bootedCPUCount != 1)
    {
        KERNEL_ERROR("Multiple booted CPU count (%d). Core manager must be "
                     "initialized with only one core running.\n",
                     _bootedCPUCount);
        KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                           TRACE_X86_CPU_CORE_MGT_INIT_EXIT,
                           1,
                           -1);
        return;
    }

    /* Init the current core information */
    sCoreIds[0] = kspLapicDriver->pGetLAPICId();

    /* Check if we need to enable more cores */
    kpLapicNode = kspLapicDriver->pGetLAPICList();
    while(kpLapicNode != NULL)
    {
        /* If not self */
        if(sCoreIds[0] != kpLapicNode->lapic.lapicId)
        {
            /* Increase the available CPU count */
            ++sAvailabeCpuCount;

            KERNEL_DEBUG(CORE_MGT_DEBUG_ENABLED,
                         MODULE_NAME,
                         "CPU With LAPIC id %d flags: 0x%x",
                         kpLapicNode->lapic.lapicId,
                         kpLapicNode->lapic.flags);

            /* Check if core can be started */
            if((kpLapicNode->lapic.flags & LAPIC_FLAG_ENABLED) != 0)
            {
                /* Start the core */
                kspLapicDriver->pStartCpu(kpLapicNode->lapic.lapicId);
            }
        }

        /* Go to next */
        kpLapicNode = kpLapicNode->pNext;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_INIT_EXIT,
                       1,
                       0);
}

void coreMgtApInit(const uint8_t kCpuId)
{
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_AP_INIT_ENTRY,
                       0);

    /* Init our LAPIC ID */
    sCoreIds[kCpuId] = kspLapicDriver->pGetLAPICId();

    /* Init LAPIC for the calling CPU */
    kspLapicDriver->pInitApCore();

    /* Init LAPIC timer for the calling CPU is exists */
    if(kspLapicTimerDriver != NULL)
    {
        kspLapicTimerDriver->pInitApCore(kCpuId);
    }

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_AP_INIT_EXIT,
                       0);
}

void coreMgtSendIpi(const uint32_t kFlags, const uint8_t kVector)
{
    uint8_t i;
    uint8_t destCpuId;
    uint8_t srcCpuId;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_SEND_IPI_ENTRY,
                       2,
                       kFlags,
                       kVector);

    /* Check if we should only send to one CPU */
    if((kFlags & CORE_MGT_IPI_BROADCAST_TO_OTHER) == 0 &&
       (kFlags & CORE_MGT_IPI_BROADCAST_TO_ALL) == 0)
    {
        /* Get the core to send to */
        destCpuId = kFlags & CORE_MGT_IPI_SEND_TO_CPU_MASK;

        /* Check if in bounds */
        if(destCpuId < MAX_CPU_COUNT)
        {
            kspLapicDriver->pSendIPI(sCoreIds[destCpuId], kVector);
        }
    }
    else if((kFlags & CORE_MGT_IPI_BROADCAST_TO_ALL) ==
            CORE_MGT_IPI_BROADCAST_TO_ALL)
    {
        /* Send to all */
        for(i = 0; i < MAX_CPU_COUNT; ++i)
        {
            kspLapicDriver->pSendIPI(sCoreIds[i], kVector);
        }
    }
    else if((kFlags & CORE_MGT_IPI_BROADCAST_TO_OTHER) ==
            CORE_MGT_IPI_BROADCAST_TO_OTHER)
    {
        /* Send to all excepted the caller */
        srcCpuId = cpuGetId();
        for(i = 0; i < MAX_CPU_COUNT; ++i)
        {
            if(i != srcCpuId)
            {
                kspLapicDriver->pSendIPI(sCoreIds[i], kVector);
            }
        }
    }

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_SEND_IPI_EXIT,
                       2,
                       kFlags,
                       kVector);
}

uint8_t cpuGetId(void)
{
    uint32_t cpuId;
    /* On I386, GS stores the CPU Id assigned at boot */
    __asm__ __volatile__ ("mov %%gs, %0" : "=r"(cpuId));
    return cpuId & 0xFF;
}

#else /* MAX_CPU_COUNT > 1 */

void coreMgtRegLapicDriver(const lapic_driver_t* kpLapicDriver)
{
    (void)kpLapicDriver;
}

void coreMgtRegLapicTimerDriver(const lapic_timer_driver_t* kpLapicTimerDriver)
{
    (void)kpLapicTimerDriver;
}

void coreMgtInit(void)
{
    return;
}

void coreMgtApInit(void)
{
    return;
}

void coreMgtSendIpi(const uint32_t kFlags, const uint8_t kVector)
{
    (void)kFlags;
    (void)kVector;
}

uint8_t cpuGetId(void)
{
    return 0;
}
#endif /* MAX_CPU_COUNT > 1 */

/************************************ EOF *************************************/