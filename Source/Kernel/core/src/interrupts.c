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
#include <stdint.h>         /* Generic int types */
#include <stddef.h>         /* Standard definitions */
#include <string.h>         /* String manipulation */
#include <critical.h>       /* Critical sections */
#include <scheduler.h>      /* Kernel scheduler */
#include <kerneloutput.h>   /* Kernel output methods */

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

/**
 * @brief Initial placeholder for the IRQ mask driver.
 *
 * @param kIrqNumber Unused.
 * @param enabled Unused.
 */
static void _initDriverSetIrqMask(const uint32_t kIrqNumber,
                                  const bool_t   kEnabled);

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
kernel_spinlock_t kernelInterruptHandlerTableLock;

/************************** Static global variables ***************************/
/** @brief The current interrupt driver to be used by the kernel. */
static interrupt_driver_t sInterruptDriver;

/** @brief Stores the number of spurious interrupts since the initialization of
 * the kernel.
 */
static uint64_t sSpuriousIntCount;

/** @brief Stores the CPU's interrupt configuration. */
static const cpu_interrupt_config_t* kspCpuInterruptConfig;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _initDriverSetIrqMask(const uint32_t kIrqNumber,
                                  const bool_t   kEnabled)
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
    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Spurious interrupt %d",
                 sSpuriousIntCount);

    ++sSpuriousIntCount;

    return;
}

void interruptMainHandler(void)
{
    custom_handler_t handler;
    kernel_thread_t* pCurrentThread;
    uint32_t         intId;
    uint32_t         intState;

    /* Get the current thread */
    pCurrentThread = schedGetCurrentThread();
    intId    = cpuGetContextIntNumber(pCurrentThread->pVCpu);
    intState = cpuGetContextIntState(pCurrentThread->pVCpu);

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_HANDLER_ENTRY,
                       2,
                       (uint32_t)pCurrentThread->tid,
                       intId);

    /* If interrupts are disabled */
    if(intState == 0 &&
       intId != kspCpuInterruptConfig->panicInterruptLine &&
       intId >= kspCpuInterruptConfig->minInterruptLine &&
       intId <= kspCpuInterruptConfig->maxInterruptLine)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Blocked interrupt %u",
                     intId);

        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_HANDLER_EXIT,
                           3,
                           (uint32_t)pCurrentThread->tid,
                           intId,
                           1);
        return;
    }

    if(intId == kspCpuInterruptConfig->panicInterruptLine)
    {
        kernelPanicHandler(pCurrentThread);
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Int %d", intId);

    /* Check for spurious interrupt */
    if(sInterruptDriver.pHandleSpurious(intId) == INTERRUPT_TYPE_SPURIOUS)
    {
        _spuriousHandler();
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_HANDLER_EXIT,
                           3,
                           (uint32_t)pCurrentThread->tid,
                           intId,
                           2);
        return;
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Non spurious %d | Handler 0x%p",
                 intId,
                 pKernelInterruptHandlerTable[intId]);

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

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_HANDLER_EXIT,
                       3,
                       (uint32_t)pCurrentThread->tid,
                       intId,
                       0);
}

void interruptInit(void)
{
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED, TRACE_INTERRUPT_INIT_ENTRY, 0);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Initializing interrupt manager.");

    KERNEL_SPINLOCK_INIT(kernelInterruptHandlerTableLock);

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
              "Failed to allocate interrupt table.",
              TRUE);
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

    TEST_POINT_FUNCTION_CALL(interrupt_test, TEST_INTERRUPT_ENABLED);

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED, TRACE_INTERRUPT_INIT_EXIT, 0);
}

OS_RETURN_E interruptSetDriver(const interrupt_driver_t* kpDriver)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_SET_DRIVER_ENTRY,
                       2,
                       0,
                       (uint32_t)kpDriver);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_SET_DRIVER_ENTRY,
                       2,
                       (uint32_t)(kpDriver >> 32),
                       (uint32_t)(kpDriver & 0xFFFFFFFF));
#endif

    if(kpDriver == NULL ||
       kpDriver->pSetIrqEOI == NULL ||
       kpDriver->pSetIrqMask == NULL ||
       kpDriver->pHandleSpurious == NULL ||
       kpDriver->pGetIrqInterruptLine == NULL)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_SET_DRIVER_EXIT,
                           3,
                           0,
                           (uint32_t)kpDriver,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_SET_DRIVER_EXIT,
                           3,
                           (uint32_t)(kpDriver >> 32),
                           (uint32_t)(kpDriver & 0xFFFFFFFF),
                           OS_ERR_NULL_POINTER);
#endif

        return OS_ERR_NULL_POINTER;
    }

    /* We can only set one interrupt manager*/
    if(sInterruptDriver.pGetIrqInterruptLine != _initDriverGetIrqIntLine)
    {
        KERNEL_ERROR("Only one interrupt driver can be registered.\n");
        PANIC(OS_ERR_UNAUTHORIZED_ACTION,
              MODULE_NAME,
              "Only one interrupt driver can be registered.",
              TRUE);
    }

    sInterruptDriver = *kpDriver;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Set new interrupt driver at 0x%p",
                 kpDriver);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_SET_DRIVER_EXIT,
                       3,
                       0,
                       (uint32_t)kpDriver,
                       OS_ERR_NULL_POINTER);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_SET_DRIVER_EXIT,
                       3,
                       (uint32_t)(kpDriver >> 32),
                       (uint32_t)(kpDriver & 0xFFFFFFFF),
                       OS_NO_ERR);
#endif

    return OS_NO_ERR;
}

OS_RETURN_E interruptRegister(const uint32_t   kInterruptLine,
                              custom_handler_t handler)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_ENTRY,
                       3,
                       0,
                       (uint32_t)handler,
                       kInterruptLine);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_ENTRY,
                       3,
                       (uint32_t)(handler >> 32),
                       (uint32_t)(handler & 0xFFFFFFFF),
                       kInterruptLine);
#endif

    if(kInterruptLine < kspCpuInterruptConfig->minInterruptLine ||
       kInterruptLine > kspCpuInterruptConfig->maxInterruptLine)
    {
        KERNEL_ERROR("Invalid registered interrupt line: %d\n", kInterruptLine);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kInterruptLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);
#else
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           (uint32_t)(handler >> 32),
                           (uint32_t)(handler & 0xFFFFFFFF),
                           kInterruptLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);
#endif

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_ERROR("NULL registered interrupt handler\n");
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kInterruptLine,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           (uint32_t)(handler >> 32),
                           (uint32_t)(handler & 0xFFFFFFFF),
                           kInterruptLine,
                           OS_ERR_NULL_POINTER);
#endif

        return OS_ERR_NULL_POINTER;
    }

    KERNEL_CRITICAL_LOCK(kernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kInterruptLine] != NULL)
    {
        KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);
        KERNEL_ERROR("Already registered interrupt line: %d\n", kInterruptLine);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kInterruptLine,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);
#else
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_EXIT,
                           4,
                           (uint32_t)(handler >> 32),
                           (uint32_t)(handler & 0xFFFFFFFF),
                           kInterruptLine,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);
#endif

        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    pKernelInterruptHandlerTable[kInterruptLine] = handler;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Added INT %u handler at 0x%p",
                 kInterruptLine,
                 handler);

    KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_EXIT,
                       4,
                       0,
                       (uint32_t)handler,
                       kInterruptLine,
                       OS_NO_ERR);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_EXIT,
                       4,
                       (uint32_t)(handler >> 32),
                       (uint32_t)(handler & 0xFFFFFFFF),
                       kInterruptLine,
                       OS_NO_ERR);
#endif

    return OS_NO_ERR;
}

OS_RETURN_E interruptRemove(const uint32_t kInterruptLine)
{
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REMOVE_ENTRY,
                       1,
                       kInterruptLine);

    if(kInterruptLine < kspCpuInterruptConfig->minInterruptLine ||
       kInterruptLine > kspCpuInterruptConfig->maxInterruptLine)
    {
        KERNEL_ERROR("Invalid removed interrupt line: %d\n", kInterruptLine);

        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REMOVE_EXIT,
                           2,
                           kInterruptLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    KERNEL_CRITICAL_LOCK(kernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kInterruptLine] == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

        KERNEL_ERROR("Not registered interrupt line: %d\n", kInterruptLine);

        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REMOVE_EXIT,
                           2,
                           kInterruptLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    pKernelInterruptHandlerTable[kInterruptLine] = NULL;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Removed interrupt %u handle", kInterruptLine);

    KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REMOVE_EXIT,
                       2,
                       kInterruptLine,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E interruptIRQRegister(const uint32_t   kIrqNumber,
                                 custom_handler_t handler)
{
    int32_t     interruptLine;
    OS_RETURN_E retCode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_IRQ_ENTRY,
                       3,
                       0,
                       (uint32_t)handler,
                       kIrqNumber);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_IRQ_ENTRY,
                       3,
                       (uint32_t)(handler >> 32),
                       (uint32_t)(handler & 0xFFFFFFFF),
                       kIrqNumber);
#endif

    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIrqInterruptLine(kIrqNumber);

    if(interruptLine < 0)
    {
        KERNEL_ERROR("Invalid IRQ interrupt line: %d\n", kIrqNumber);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_IRQ_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kIrqNumber,
                           OS_ERR_NO_SUCH_IRQ);
#else
        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REGISTER_IRQ_EXIT,
                           4,
                           (uint32_t)(handler >> 32),
                           (uint32_t)(handler & 0xFFFFFFFF),
                           kIrqNumber,
                           OS_ERR_NO_SUCH_IRQ);
#endif

        return OS_ERR_NO_SUCH_IRQ;
    }

    retCode = interruptRegister(interruptLine, handler);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_IRQ_EXIT,
                       4,
                       0,
                       (uint32_t)handler,
                       kIrqNumber,
                       retCode);
#else
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REGISTER_IRQ_EXIT,
                       4,
                       (uint32_t)(handler >> 32),
                       (uint32_t)(handler & 0xFFFFFFFF),
                       kIrqNumber,
                       retCode);
#endif

    return retCode;
}

OS_RETURN_E interruptIRQRemove(const uint32_t kIrqNumber)
{
    int32_t     interruptLine;
    OS_RETURN_E retCode;

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REMOVE_IRQ_ENTRY,
                       1,
                       kIrqNumber);

    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIrqInterruptLine(kIrqNumber);

    if(interruptLine < 0)
    {
        KERNEL_ERROR("Invalid IRQ interrupt line: %d\n", kIrqNumber);

        KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                           TRACE_INTERRUPT_REMOVE_IRQ_EXIT,
                           2,
                           kIrqNumber,
                           OS_ERR_NO_SUCH_IRQ);

        return OS_ERR_NO_SUCH_IRQ;
    }

    retCode = interruptRemove(interruptLine);

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_REMOVE_IRQ_EXIT,
                       2,
                       kIrqNumber,
                       retCode);

    return retCode;
}

void interruptRestore(const uint32_t kPrevState)
{

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_INTERRUPT_RESTORE,
                       1,
                       kPrevState);
    if(kPrevState != 0)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Enabled HW INT");
        cpuSetInterrupt();
    }
}

uint32_t interruptDisable(void)
{
    uint32_t prevState;

    prevState = cpuGeIntState();

    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_INTERRUPT_DISABLE,
                       1,
                       prevState);

    if(prevState == 0)
    {
        return 0;
    }

    cpuClearInterrupt();

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Disabled HW INT");

    return prevState;
}

void interruptIRQSetMask(const uint32_t kIrqNumber, const bool_t kEnabled)
{
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_IRQ_SET_MASK,
                       2,
                       kIrqNumber,
                       kEnabled);


    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IRQ Mask change: %u %u",
                 kIrqNumber,
                 kEnabled);

    sInterruptDriver.pSetIrqMask(kIrqNumber, kEnabled);
}

void interruptIRQSetEOI(const uint32_t kIrqNumber)
{
    KERNEL_TRACE_EVENT(TRACE_INTERRUPT_ENABLED,
                       TRACE_INTERRUPT_IRQ_SET_EOI,
                       1,
                       kIrqNumber);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 "INTERRUPTS",
                 "IRQ EOI: %u",
                 kIrqNumber);

    sInterruptDriver.pSetIrqEOI(kIrqNumber);
}

/************************************ EOF *************************************/