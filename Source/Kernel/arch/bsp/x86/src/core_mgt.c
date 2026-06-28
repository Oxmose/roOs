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
#include <kheap.h>        /* Kernel heap */
#include <string.h>       /* String manipulation */
#include <kqueue.h>       /* Kernel queues */
#include <x86cpu.h>       /* CPU management */
#include <stdint.h>       /* Generic int types */
#include <syslog.h>       /* Kernel Syslog */
#include <devtree.h>      /* Device tree library */
#include <stdbool.h>      /* Bool types */
#include <console.h>      /* Console service */
#include <critical.h>     /* Critical sections */
#include <drivermgr.h>    /* Driver manager */
#include <scheduler.h>    /* Kernel scheduler */
#include <ctrl_block.h>   /* Thread's control block */
#include <interrupts.h>   /* Interrupt manager */
#include <lapic_timer.h>  /* LAPIC timer driver */

/* Configuration files */
#include <config.h>

/* Header file */
#include <core_mgt.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "CORE MGT"

/** @brief Compatible property name in FDT */
#define COMPATIBLE_PROP_NAME "compatible"

/** @brief Status property name in FDT */
#define STATUS_PROP_NAME "status"

/** @brief LAPIC flag: enabled (running) */
#define LAPIC_FLAG_ENABLED 0x1

/** @brief LAPIC flag: capable (available to be started) */
#define LAPIC_FLAG_CAPABLE 0x2

/** @brief IPI send flag CPU mask */
#define CPU_IPI_SEND_TO_CPU_MASK (CPU_IPI_SEND_TO(0xFFFFFFFF));

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
#define CORE_MGT_ASSERT(COND, MSG, ERROR) {                 \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief IPI interrupt handler.
 *
 * @details IPI interrupt handler. Based on the IPI parameters, the handler
 * dispatches the IPI request.
 *
 * @param[in] pCurrThread The executing thread at the moment of the IPI.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _ipiInterruptHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Attaches the Core Manager driver to the system.
 *
 * @details Attaches the Core Manager driver to the system. This function will
 * use the FDT to initialize the Core Manager hardware and retreive the Core
 * Manager parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _coreMgtAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Initializes the number of CPU based on the FDT.
 *
 * @param[in] kpFdtNone The current FDT node to walk.
 *
 * @details Initializes the number of CPU based on the FDT. If an error is
 * detected, a kernel panic is raised.
 */
static void _coreMgtWalkCpuCount(const fdt_node_t* kpFdtNone);

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
static uint8_t* spCoreIds;

/** @brief Stores the LAPIC driver instance */
static const lapic_driver_t* kspLapicDriver = NULL;

/** @brief Stores the LAPIC timer driver instance */
static const lapic_timer_driver_t* kspLapicTimerDriver = NULL;

/** @brief Stores the IPI interrupt line. */
static uint32_t sIpiInterruptLine;

/** @brief Stores the IPI parameters */
static kqueue_t** spIpiParametersList;

/** @brief CPU driver instance. */
static driver_t sX86CPUDriver = {
    .pName         = "X86 CPU Driver",
    .pDescription  = "X86 CPU Driver for roOs",
    .pCompatible   = "generic,x86_64",
    .pVersion      = "1.0",
    .pDriverAttach = _coreMgtAttach
};

/** @brief Stores the number of detected CPUs from the FDT */
static uint32_t sCpuFromFDTCount = 0;

/** @brief Stores the number of CPU that were attached. */
static uint32_t sAttachedCpus = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _coreMgtAttach(const fdt_node_t* pkFdtNode)
{
    (void)pkFdtNode;

    if(sAttachedCpus < sCpuFromFDTCount)
    {
        ++sAttachedCpus;
    }
    else
    {
        return OS_ERR_NO_SUCH_ID;
    }

    return OS_NO_ERR;
}

static void _coreMgtWalkCpuCount(const fdt_node_t* kpFdtNone)
{
    const char* kpCompatible;
    const char* kpStatus;
    size_t      propLen;

    if(kpFdtNone == NULL)
    {
        return;
    }

    /* Manage disabled nodes */
    kpStatus = fdtGetProp(kpFdtNone, STATUS_PROP_NAME, &propLen);
    if(kpStatus == NULL || (propLen == 5 && strcmp(kpStatus, "okay") == 0))
    {
        /* Get the node compatible */
        kpCompatible = fdtGetProp(kpFdtNone, COMPATIBLE_PROP_NAME, &propLen);
        if(kpCompatible != NULL && propLen > 0)
        {
            if(strcmp(sX86CPUDriver.pCompatible, kpCompatible) == 0)
            {
                ++sCpuFromFDTCount;
            }
        }
    }

    /* Got to next nodes */
    _coreMgtWalkCpuCount(fdtGetChild(kpFdtNone));
    _coreMgtWalkCpuCount(fdtGetNextNode(kpFdtNone));
}

static bool _ipiInterruptHandler(kernel_thread_t* pCurrThread)
{
    kqueue_node_t* pNode;
    ipi_params_t   params;
    uint8_t        cpuId;
    bool           doSchedule;

    interruptIRQSetEOI(sIpiInterruptLine);

    cpuId = cpuGetId();

    /* Get the parameters */
    pNode = kQueuePop(spIpiParametersList[cpuId]);
    CORE_MGT_ASSERT(pNode != NULL,
                    "IPI without parameters",
                    OS_ERR_UNAUTHORIZED_ACTION);

    params = *((ipi_params_t*)pNode->pData);
    kQueueDestroyNode(&pNode);

    /* Dispatch */
    doSchedule = false;
    switch(params.function)
    {
        case IPI_FUNC_PANIC:
            kernelPanicHandler(pCurrThread);
            break;
        case IPI_FUNC_TLB_INVAL:
            cpuInvalidateTlbEntry((uintptr_t)params.pData);
            break;
        case IPI_FUNC_SCHEDULE:
            /* Request a schedule */
            doSchedule = true;
            break;
        default:
            PANIC(OS_ERR_INCORRECT_VALUE,
                  MODULE_NAME,
                  "Unknown IPI function");
    }

    return doSchedule;
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
    size_t                        lastBooted;
    OS_RETURN_E                   error;
    const lapic_node_t*           kpLapicNode;
    const cpu_interrupt_config_t* kpCpuIntConfig;

    /* Check if the LAPIC driver was registered */
    if(kspLapicDriver == NULL)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "LAPIC driver was not registered to core manager.\n"
               "Continuing with one core.");
        return;
    }

    if(_bootedCPUCount != 1)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Multiple booted CPU count (%d). Core manager must be "
               "initialized with only one core running.",
               _bootedCPUCount);
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
    for(i = 0; i < sCpuFromFDTCount; ++i)
    {
        spIpiParametersList[i] = kQueueCreate(true);
    }

    /* Init the current core information */
    spCoreIds[0] = kspLapicDriver->pGetLAPICId();

    /* Check if we need to enable more cores */
    kpLapicNode = kspLapicDriver->pGetLAPICList();
    while(kpLapicNode != NULL && _bootedCPUCount < sCpuFromFDTCount)
    {
        /* If not self */
        if(spCoreIds[0] != kpLapicNode->lapic.lapicId)
        {

#if CORE_MGT_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "CPU With LAPIC id %d flags: 0x%x",
                   kpLapicNode->lapic.lapicId,
                   kpLapicNode->lapic.flags);
#endif

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

    /* Wait for all CPU to have booted */
    lastBooted = 0;
    while(_bootedCPUCount < sCpuFromFDTCount)
    {
        if(lastBooted != _bootedCPUCount)
        {
            lastBooted = _bootedCPUCount;
            syslog(SYSLOG_LEVEL_INFO,
                   MODULE_NAME,
                   "Waiting for other CPUs, currenlty %d/%d",
                   lastBooted,
                   sCpuFromFDTCount);
        }
    }

    /* Last check */
    CORE_MGT_ASSERT(sAttachedCpus == _bootedCPUCount,
                    "Attached CPUs count does not match booted CPU count.",
                    OS_ERR_INCORRECT_VALUE);
}

void coreMgtApInit(const uint8_t kCpuId)
{
    /* Init our LAPIC ID */
    spCoreIds[kCpuId] = kspLapicDriver->pGetLAPICId();

    /* Init LAPIC for the calling CPU */
    kspLapicDriver->pInitApCore();

    /* Init LAPIC timer for the calling CPU is exists */
    if(kspLapicTimerDriver != NULL)
    {
        kspLapicTimerDriver->pInitApCore(kCpuId);
    }
}

void cpuMgtSendIpi(const uint32_t kFlags,
                   ipi_params_t*  kpParams,
                   const bool     kAllocateParam)
{
    uint8_t        i;
    uint8_t        destCpuId;
    uint8_t        srcCpuId;
    uint32_t       intState;
    kqueue_node_t* pNode;
    ipi_params_t*  pParams;

    if(kspLapicDriver == NULL)
    {
        return;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    /* Check if we should only send to one CPU */
    if((kFlags & CPU_IPI_BROADCAST_TO_OTHER) == 0 &&
       (kFlags & CPU_IPI_BROADCAST_TO_ALL) == 0)
    {
        /* Get the core to send to */
        destCpuId = kFlags & CPU_IPI_SEND_TO_CPU_MASK;

        /* Check if in bounds */
        if(destCpuId < _bootedCPUCount)
        {
            if(kAllocateParam == true)
            {
                pParams = kmalloc(sizeof(ipi_params_t));
                CORE_MGT_ASSERT(pParams != NULL,
                                "Failed to allocate IPI parameters",
                                OS_ERR_NO_MORE_MEMORY);
                *pParams = *kpParams;
            }
            else
            {
                pParams = kpParams;
            }
            pNode = kQueueCreateNode(pParams, true);
            kQueuePush(pNode, spIpiParametersList[destCpuId]);

            kspLapicDriver->pSendIPI(spCoreIds[destCpuId], sIpiInterruptLine);
        }
    }
    else if((kFlags & CPU_IPI_BROADCAST_TO_ALL) ==
            CPU_IPI_BROADCAST_TO_ALL)
    {
        /* Send to all */
        for(i = 0; i < _bootedCPUCount; ++i)
        {
            if(kAllocateParam == true)
            {
                pParams = kmalloc(sizeof(ipi_params_t));
                CORE_MGT_ASSERT(pParams != NULL,
                                "Failed to allocate IPI parameters",
                                OS_ERR_NO_MORE_MEMORY);
                *pParams = *kpParams;
            }
            else
            {
                pParams = kpParams;
            }
            pNode = kQueueCreateNode(pParams, true);
            kQueuePush(pNode, spIpiParametersList[i]);

            kspLapicDriver->pSendIPI(spCoreIds[i], sIpiInterruptLine);
        }
    }
    else if((kFlags & CPU_IPI_BROADCAST_TO_OTHER) ==
            CPU_IPI_BROADCAST_TO_OTHER)
    {
        /* Send to all excepted the caller */
        srcCpuId = cpuGetId();
        for(i = 0; i < _bootedCPUCount; ++i)
        {
            if(i != srcCpuId)
            {
                if(kAllocateParam == true)
                {
                    pParams = kmalloc(sizeof(ipi_params_t));
                    CORE_MGT_ASSERT(pParams != NULL,
                                    "Failed to allocate IPI parameters",
                                    OS_ERR_NO_MORE_MEMORY);
                    *pParams = *kpParams;
                }
                else
                {
                    pParams = kpParams;
                }
                pNode = kQueueCreateNode(pParams, true);
                kQueuePush(pNode, spIpiParametersList[i]);

                kspLapicDriver->pSendIPI(spCoreIds[i], sIpiInterruptLine);
            }
        }
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

void coreMgtInitCpuCount(void)
{
    const fdt_node_t* kpFdtRootNode;

    /* Get the FDT root node and walk it to register CPUs */
    kpFdtRootNode = fdtGetRoot();
    CORE_MGT_ASSERT(kpFdtRootNode != NULL,
                    "NULL Device File Tree",
                    OS_ERR_NULL_POINTER);

    /* Perform the registration */
    _coreMgtWalkCpuCount(kpFdtRootNode);

    /* Initialize internal resources */
    spCoreIds = kmalloc(sizeof(uint8_t) * sCpuFromFDTCount);
    CORE_MGT_ASSERT(spCoreIds != NULL,
                    "Failed to allocate Core IDs table.",
                    OS_ERR_NO_MORE_MEMORY);
    spIpiParametersList = kmalloc(sizeof(kqueue_t*) * sCpuFromFDTCount);
    CORE_MGT_ASSERT(spIpiParametersList != NULL,
                    "Failed to allocate Core IPI table.",
                    OS_ERR_NO_MORE_MEMORY);
}

uint32_t coreMgtGetCpuCount(void)
{
    return sCpuFromFDTCount;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86CPUDriver);

/************************************ EOF *************************************/