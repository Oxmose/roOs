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
#include <panic.h>        /* Kernel panic */
#include <lapic.h>        /* LAPIC driver */
#include <x86cpu.h>       /* CPU management */
#include <stdint.h>       /* Generic int types */
#include <devtree.h>      /* Device tree library */
#include <console.h>      /* Console service */
#include <drivermgr.h>    /* Driver manager */
#include <ctrl_block.h>   /* Thread's control block */
#include <interrupts.h>   /* Interrupt manager */
#include <lapic_timer.h>  /* LAPIC timer driver */
#include <kerneloutput.h> /* Kernel output methods */

/* Configuration files */
#include <config.h>

/* Header file */
#include <core_mgt.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#if MAX_CPU_COUNT <= 0
#error "MAX_CPU_COUNT must be greater or equal to 1"
#endif

/** @brief Current module name */
#define MODULE_NAME "CORE MGT"

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

/**
 * @brief Assert macro used by the core manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the core manager to ensure correctness of
 * execution. Due to the critical nature of the core manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define CORE_MGT_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == FALSE)                                      \
    {                                                        \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);                \
    }                                                        \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

#if MAX_CPU_COUNT > 1
/**
 * @brief IPI interrupt handler.
 *
 * @details IPI interrupt handler. Based on the IPI parameters, the handler
 * dispatches the IPI request.
 *
 * @param[in] pCurrThread The executing thread at the moment of the IPI.
 */
static void _ipiInterruptHandler(kernel_thread_t* pCurrThread);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Stores the number of enabled (running) cores in the system. */
extern volatile uint32_t _bootedCPUCount;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Stores the translated CPU identifiers */
static uint8_t sCoreIds[MAX_CPU_COUNT] = {0};

/** @brief Stores the LAPIC driver instance */
static const lapic_driver_t* kspLapicDriver = NULL;

/** @brief Stores the LAPIC timer driver instance */
static const lapic_timer_driver_t* kspLapicTimerDriver = NULL;

/** @brief Stores the IPI interrupt line. */
static uint32_t sIpiInterruptLine;

/** @brief Stores the IPI parameters */
static ipi_params_t sIpiParameters[MAX_CPU_COUNT];

/** @brief Stores the IPI parameters locks */
static spinlock_t sIpiParametersLocks[MAX_CPU_COUNT];

#endif /* #if MAX_CPU_COUNT > 1 */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

#if MAX_CPU_COUNT > 1
static void _ipiInterruptHandler(kernel_thread_t* pCurrThread)
{
    ipi_params_t params;
    uint8_t      cpuId;

    cpuId = cpuGetId();

    /* Get the parameter and unlock the ipi parameters lock */
    params = sIpiParameters[cpuId];
    spinlockRelease(&sIpiParametersLocks[cpuId]);

    /* Dispatch */
    switch(params.function)
    {
        case IPI_FUNC_PANIC:
            kernelPanicHandler(pCurrThread);
            break;
        case IPI_FUNC_TLB_INVAL:
            cpuInvalidateTlbEntry((uintptr_t)params.pData);
            break;
        default:
            PANIC(OS_ERR_INCORRECT_VALUE,
                  MODULE_NAME,
                  "Unknown IPI function",
                  TRUE);
    }

    interruptIRQSetEOI(sIpiInterruptLine);
}

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
    uint32_t                      i;
    OS_RETURN_E                   error;
    const lapic_node_t*           kpLapicNode;
    const cpu_interrupt_config_t* kpCpuIntConfig;

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

    /* Get the CPU interrupt configuration */
    kpCpuIntConfig = cpuGetInterruptConfig();
    sIpiInterruptLine = kpCpuIntConfig->ipiInterruptLine;

    /* Register the IPI handler */
    error = interruptRegister(sIpiInterruptLine, _ipiInterruptHandler);
    CORE_MGT_ASSERT(error == OS_NO_ERR,
                    "Failed to register IPI interrupt",
                    error);

    /* Initializes the IPI parameters locks */
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        SPINLOCK_INIT(sIpiParametersLocks[i]);
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

void coreMgtSendIpi(const uint32_t kFlags, const ipi_params_t* kpParams)
{
    uint8_t i;
    uint8_t destCpuId;
    uint8_t srcCpuId;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_SEND_IPI_ENTRY,
                       2,
                       kFlags,
                       kpParams->function);

    if(kspLapicDriver == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                           TRACE_X86_CPU_CORE_MGT_SEND_IPI_EXIT,
                           2,
                           kFlags,
                           kpParams->function);
        return;
    }

    /* Check if we should only send to one CPU */
    if((kFlags & CORE_MGT_IPI_BROADCAST_TO_OTHER) == 0 &&
       (kFlags & CORE_MGT_IPI_BROADCAST_TO_ALL) == 0)
    {
        /* Get the core to send to */
        destCpuId = kFlags & CORE_MGT_IPI_SEND_TO_CPU_MASK;

        /* Check if in bounds */
        if(destCpuId < MAX_CPU_COUNT)
        {
            spinlockAcquire(&sIpiParametersLocks[destCpuId]);
            sIpiParameters[destCpuId] = *kpParams;
            kspLapicDriver->pSendIPI(sCoreIds[destCpuId], sIpiInterruptLine);
        }
    }
    else if((kFlags & CORE_MGT_IPI_BROADCAST_TO_ALL) ==
            CORE_MGT_IPI_BROADCAST_TO_ALL)
    {
        /* Send to all */
        for(i = 0; i < MAX_CPU_COUNT; ++i)
        {
            spinlockAcquire(&sIpiParametersLocks[i]);
            sIpiParameters[i] = *kpParams;
            kspLapicDriver->pSendIPI(sCoreIds[i], sIpiInterruptLine);
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
                spinlockAcquire(&sIpiParametersLocks[i]);
                sIpiParameters[i] = *kpParams;
                kspLapicDriver->pSendIPI(sCoreIds[i], sIpiInterruptLine);
            }
        }
    }

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_CORE_MGT_SEND_IPI_EXIT,
                       2,
                       kFlags,
                       kpParams->function);
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

void coreMgtApInit(const uint8_t kCpuId)
{
    (void)kCpuId;

    return;
}

void coreMgtSendIpi(const uint32_t kFlags, const ipi_params_t* kpParams)
{
    (void)kFlags;
    (void)kpParams;
}

#endif /* MAX_CPU_COUNT > 1 */

/************************************ EOF *************************************/