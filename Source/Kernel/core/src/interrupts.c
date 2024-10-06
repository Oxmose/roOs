/*******************************************************************************
 * @file interrupts.c
 *
 * @see interrupts.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>            /* CPU management */
#include <panic.h>          /* Kernel panic */
#include <kheap.h>          /* Kernel heap */
#include <kqueue.h>         /* Kernel queue */
#include <stdint.h>         /* Generic int types */
#include <stddef.h>         /* Standard definitions */
#include <string.h>         /* String manipulation */
#include <syslog.h>         /* Kernel Syslog */
#include <stdbool.h>        /* Bool types */
#include <critical.h>       /* Critical sections */
#include <scheduler.h>      /* Kernel scheduler */
#include <ksemaphore.h>     /* Kernel semaphores */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <interrupts.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module's name */
#define MODULE_NAME "INTERRUPTS"

/** @brief Defered interrupt thread priority */
#define DEFERED_INTERRUPT_THREAD_PRIO KERNEL_HIGHEST_PRIORITY

/** @brief Defered interrupt thread stack size  */
#define DEFERED_INTERRUPT_THREAD_STACK_SIZE 0x1000

/** @brief Defered interrupt thread cpu affinity set */
#define DEFERED_INTERRUPT_THREAD_AFFINITY 0

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the defered interrupt jobs */
typedef struct
{
    /**
     * @brief The routine to call on a defered interrupt.
     *
     * @param[in, out] args The arguments to pass to the defered routine.
     */
    void (*jobRoutine)(void* args);

    /**
     * @brief The arguments to pass to the defered routine.
     */
    void* args;
} interrupt_defered_job;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define INTERRUPT_ASSERT(COND, MSG, ERROR) {                \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initial placeholder for the IRQ mask driver.
 *
 * @param kIrqNumber Unused.
 * @param enabled Unused.
 */
static void _initDriverSetIrqMask(const uint32_t kIrqNumber,
                                  const bool     kEnabled);

/**
 * @brief Initial placeholder for the IRQ EOI driver.
 *
 * @param kIrqNumber Unused.
 */
static void _initDriverSetIrqEOI(const uint32_t kIrqNumber);

/**
 * @brief Initial placeholder for the spurious handler driver.
 *
 * @param kIntNumber Unused.
 */
static INTERRUPT_TYPE_E _initDriverHandleSpurious(const uint32_t kIntNumber);

/**
 * @brief Initial placeholder for the get int line driver.
 *
 * @param kIrqNumber Unused.
 */
static int32_t _initDriverGetIrqIntLine(const uint32_t kIrqNumber);


/**
 * @brief Kernel's spurious interrupt handler.
 *
 * @details Spurious interrupt handler. This function should only be
 * called by an assembly interrupt handler. The function will handle spurious
 * interrupts.
 */
static void _spuriousHandler(void);

/**
 * @brief Executes defered intrrupts functions.
 *
 * @details  Executes defered intrrupts functions. The routine waits for a new
 * function to execute before waking up.
 *
 * @param[in] args Unused.
 */
static void* _interruptDeferedRoutine(void* args);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief Stores the handlers for each interrupt. not static because used
 * by the exceptions module.
 */
custom_handler_t* pKernelInterruptHandlerTable;

/** @brief Table lock */
kernel_spinlock_t sKernelInterruptHandlerTableLock;

/************************** Static global variables ***************************/
/** @brief The current interrupt driver to be used by the kernel. */
static interrupt_driver_t sInterruptDriver;

/** @brief Stores the number of spurious interrupts since the initialization of
 * the kernel.
 */
static u32_atomic_t sSpuriousIntCount;

/** @brief Stores the CPU's interrupt configuration. */
static const cpu_interrupt_config_t* kspCpuInterruptConfig;

/** @brief Defered interrupt thread */
static kernel_thread_t* spDeferedIntThread;

/** @brief Defered interrupt function queue */
static kqueue_t* spDeferedIntQueue;

/** @brief Defered interrupt semaphore */
static ksemaphore_t sDefereIntQueueSem;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _initDriverSetIrqMask(const uint32_t kIrqNumber,
                                  const bool     kEnabled)
{
    (void)kIrqNumber;
    (void)kEnabled;
}

static void _initDriverSetIrqEOI(const uint32_t kIrqNumber)
{
    (void)kIrqNumber;
}

static INTERRUPT_TYPE_E _initDriverHandleSpurious(const uint32_t kIntNumber)
{
    (void)kIntNumber;
    return INTERRUPT_TYPE_REGULAR;
}

static int32_t _initDriverGetIrqIntLine(const uint32_t kIrqNumber)
{
    (void)kIrqNumber;
    return 0;
}

static void _spuriousHandler(void)
{
#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Spurious interrupt %d",
           sSpuriousIntCount);
#endif

    atomicIncrement32(&sSpuriousIntCount);
    interruptIRQSetEOI(kspCpuInterruptConfig->spuriousInterruptLine);

    return;
}

static void* _interruptDeferedRoutine(void* args)
{
    OS_RETURN_E            error;
    kqueue_node_t*         pJobNode;
    interrupt_defered_job* pJob;

    (void)args;

    while(true)
    {
        /* Wait for a new job */
        error = ksemWait(&sDefereIntQueueSem);

        if(error == OS_NO_ERR)
        {
            pJobNode = kQueuePop(spDeferedIntQueue);
            if(pJobNode != NULL)
            {
                pJob = pJobNode->pData;

                if(pJob->jobRoutine != NULL)
                {
                    pJob->jobRoutine(pJob->args);
                    kfree(pJobNode->pData);
                }
                else
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Tried to execute a NULL interrupt job");
                }

                /* Destroy the node */
                kQueueDestroyNode(&pJobNode);
            }
            else
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Got a NULL interrupt job");
            }
        }
        else
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to wait on defered interrupt semaphore (%d)",
                   error);
        }
    }

    return NULL;
}

void interruptMainHandler(void)
{
    custom_handler_t handler;
    kernel_thread_t* pCurrentThread;
    uint32_t         intId;

    /* Get the current thread */
    pCurrentThread = schedGetCurrentThread();
    intId = cpuGetContextIntNumber(pCurrentThread->pVCpu);

    if(intId == kspCpuInterruptConfig->panicInterruptLine)
    {
        kernelPanicHandler(pCurrentThread);
    }

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Interrupt %d", intId);
#endif

    /* Check for spurious interrupt */
    if(sInterruptDriver.pHandleSpurious(intId) == INTERRUPT_TYPE_SPURIOUS)
    {
        _spuriousHandler();
        /* Schedule, we will never return */
        schedScheduleNoInt(false);
    }

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Non spurious %d | Handler 0x%p",
           intId,
           pKernelInterruptHandlerTable[intId]);
#endif

    /* Select custom handlers */
    if(intId < kspCpuInterruptConfig->totalInterruptLineCount &&
       pKernelInterruptHandlerTable[intId] != NULL)
    {
        handler = pKernelInterruptHandlerTable[intId];
    }
    else
    {
        handler = kernelPanicHandler;
    }

    /* Execute the handler */
    handler(pCurrentThread);

    /* Schedule, we will never return */
    schedScheduleNoInt(false);
    PANIC(OS_ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Schedule int returned");
}

void interruptInit(void)
{

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Initializing interrupt manager.");
#endif

    KERNEL_SPINLOCK_INIT(sKernelInterruptHandlerTableLock);

    /* Get the CPU interrupt configuration */
    kspCpuInterruptConfig = cpuGetInterruptConfig();

    /* Attach the special PANIC interrupt for when we don't know what to do */
    pKernelInterruptHandlerTable[kspCpuInterruptConfig->panicInterruptLine] =
        kernelPanicHandler;

    /* Allocate and blank custom interrupt handlers */
    pKernelInterruptHandlerTable = kmalloc(sizeof(custom_handler_t) *
                                kspCpuInterruptConfig->totalInterruptLineCount);
    if(pKernelInterruptHandlerTable == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate interrupt table.");
    }
    memset(pKernelInterruptHandlerTable,
           0,
           sizeof(custom_handler_t) *
           kspCpuInterruptConfig->totalInterruptLineCount);

    /* Init state */
    interruptDisable();
    sSpuriousIntCount = 0;

    /* Init driver */
    sInterruptDriver.pGetIrqInterruptLine = _initDriverGetIrqIntLine;
    sInterruptDriver.pHandleSpurious      = _initDriverHandleSpurious;
    sInterruptDriver.pSetIrqEOI           = _initDriverSetIrqEOI;
    sInterruptDriver.pSetIrqMask          = _initDriverSetIrqMask;
}

OS_RETURN_E interruptSetDriver(const interrupt_driver_t* kpDriver)
{

    if(kpDriver == NULL ||
       kpDriver->pSetIrqEOI == NULL ||
       kpDriver->pSetIrqMask == NULL ||
       kpDriver->pHandleSpurious == NULL ||
       kpDriver->pGetIrqInterruptLine == NULL)
    {

        return OS_ERR_NULL_POINTER;
    }

    /* We can only set one interrupt manager*/
    INTERRUPT_ASSERT(sInterruptDriver.pGetIrqInterruptLine ==
                     _initDriverGetIrqIntLine,
                     "Only one interrupt driver can be registered.",
                     OS_ERR_UNAUTHORIZED_ACTION)

    sInterruptDriver = *kpDriver;

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Set new interrupt driver at 0x%p",
           kpDriver);
#endif

    return OS_NO_ERR;
}

OS_RETURN_E interruptRegister(const uint32_t   kInterruptLine,
                              custom_handler_t handler)
{

    if(kInterruptLine < kspCpuInterruptConfig->minInterruptLine ||
       kInterruptLine > kspCpuInterruptConfig->maxInterruptLine)
    {

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {

        return OS_ERR_NULL_POINTER;
    }

    KERNEL_LOCK(sKernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kInterruptLine] != NULL)
    {
        KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

        return OS_ERR_ALREADY_EXIST;
    }

    pKernelInterruptHandlerTable[kInterruptLine] = handler;

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Added INT %u handler at 0x%p",
           kInterruptLine,
           handler);
#endif

    KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

    return OS_NO_ERR;
}

OS_RETURN_E interruptRemove(const uint32_t kInterruptLine)
{

    if(kInterruptLine < kspCpuInterruptConfig->minInterruptLine ||
       kInterruptLine > kspCpuInterruptConfig->maxInterruptLine)
    {

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    KERNEL_LOCK(sKernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kInterruptLine] == NULL)
    {
        KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    pKernelInterruptHandlerTable[kInterruptLine] = NULL;

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Removed interrupt %u handle",
           kInterruptLine);
#endif

    KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

    return OS_NO_ERR;
}

OS_RETURN_E interruptIRQRegister(const uint32_t   kIrqNumber,
                                 custom_handler_t handler)
{
    int32_t     interruptLine;
    OS_RETURN_E retCode;

    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIrqInterruptLine(kIrqNumber);

    if(interruptLine < 0)
    {

        return OS_ERR_NO_SUCH_IRQ;
    }

    retCode = interruptRegister(interruptLine, handler);

    return retCode;
}

OS_RETURN_E interruptIRQRemove(const uint32_t kIrqNumber)
{
    int32_t     interruptLine;
    OS_RETURN_E retCode;

    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIrqInterruptLine(kIrqNumber);

    if(interruptLine < 0)
    {

        return OS_ERR_NO_SUCH_IRQ;
    }

    retCode = interruptRemove(interruptLine);

    return retCode;
}

void interruptRestore(const uint32_t kPrevState)
{
    if(kPrevState != 0)
    {

#if INTERRUPTS_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Enabled HW INT");
#endif

        cpuSetInterrupt();
    }
}

uint32_t interruptDisable(void)
{
    uint32_t prevState;

    prevState = cpuGetIntState();
    cpuClearInterrupt();

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Disabled HW INT");
#endif

    return prevState;
}

void interruptIRQSetMask(const uint32_t kIrqNumber, const bool kEnabled)
{

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "IRQ Mask change: %u %u",
           kIrqNumber,
           kEnabled);
#endif

    sInterruptDriver.pSetIrqMask(kIrqNumber, kEnabled);
}

void interruptIRQSetEOI(const uint32_t kIrqNumber)
{

#if INTERRUPTS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "IRQ EOI: %u", kIrqNumber);
#endif

    sInterruptDriver.pSetIrqEOI(kIrqNumber);
}

void interruptDeferInit(void)
{
    OS_RETURN_E error;

    /* Create the defered interrupts queue */
    spDeferedIntQueue = kQueueCreate(true);

    error = ksemInit(&sDefereIntQueueSem, 0, KSEMAPHORE_FLAG_QUEUING_PRIO);
    INTERRUPT_ASSERT(error == OS_NO_ERR,
                     "Failed to create defered interrupt semaphore",
                     error);

    /* Create the defered interrupts thread */
    error = schedCreateKernelThread(&spDeferedIntThread,
                                    DEFERED_INTERRUPT_THREAD_PRIO,
                                    "defISR",
                                    DEFERED_INTERRUPT_THREAD_STACK_SIZE,
                                    DEFERED_INTERRUPT_THREAD_AFFINITY,
                                    _interruptDeferedRoutine,
                                    NULL);
    INTERRUPT_ASSERT(error == OS_NO_ERR,
                     "Failed to create defered interrupt thread",
                     error);
}

OS_RETURN_E interruptDeferIsr(void (*pRoutine)(void*), void* pArgs)
{
    kqueue_node_t*         pNewNode;
    interrupt_defered_job* pDefererJob;
    OS_RETURN_E            error;

    if(pRoutine == NULL)
    {

        return OS_ERR_NULL_POINTER;
    }

    /* Create defered job */
    pDefererJob = kmalloc(sizeof(interrupt_defered_job));
    if(pDefererJob == NULL)
    {

        return OS_ERR_NO_MORE_MEMORY;
    }
    pDefererJob->jobRoutine = pRoutine;
    pDefererJob->args       = pArgs;

    /* Create defered node */
    pNewNode = kQueueCreateNode(pDefererJob, false);
    if(pNewNode == NULL)
    {
        kfree(pDefererJob);

        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Push to defered queue */
    kQueuePush(pNewNode, spDeferedIntQueue);

    /* Notify the defered thread */
    error = ksemPost(&sDefereIntQueueSem);
    if(error != OS_NO_ERR)
    {
        kQueueRemove(spDeferedIntQueue, pNewNode, true);
        kQueueDestroyNode(&pNewNode);
        kfree(pDefererJob);
    }

    return error;
}


/************************************ EOF *************************************/