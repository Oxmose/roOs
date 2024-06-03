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
#include <stdint.h>         /* Generic int types */
#include <stddef.h>         /* Standard definitions */
#include <string.h>         /* String manipulation */
#include <critical.h>       /* Critical sections */
#include <scheduler.h>      /* Kernel scheduler */
#include <kerneloutput.h>   /* Kernel output methods */
#include <cpu_interrupt.h>  /* CPU interrupts settings */

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
custom_handler_t kernelInterruptHandlerTable[INT_ENTRY_COUNT];

/************************** Static global variables ***************************/
/** @brief The current interrupt driver to be used by the kernel. */
static interrupt_driver_t sInterruptDriver;

/** @brief Stores the number of spurious interrupts since the initialization of
 * the kernel.
 */
static uint64_t sSpuriousIntCount;

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


    /* Get the current thread */
    pCurrentThread = schedGetCurrentThread();
    intId          = pCurrentThread->vCpu.intContext.intId;



    /* If interrupts are disabled */
    if(_cpuGetContextIntState(&pCurrentThread->vCpu) == 0 &&
       intId != PANIC_INT_LINE &&
       intId >= MIN_INTERRUPT_LINE)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Blocked interrupt %u",
                     intId);
        return;
    }

    if(intId == PANIC_INT_LINE)
    {
        kernelPanicHandler(pCurrentThread);
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Int %d", intId);

    /* Check for spurious interrupt */
    if(sInterruptDriver.pHandleSpurious(intId) == INTERRUPT_TYPE_SPURIOUS)
    {
        _spuriousHandler();
        return;
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Non spurious %d",
                 intId);

    /* Select custom handlers */
    if(intId < INT_ENTRY_COUNT && kernelInterruptHandlerTable[intId] != NULL)
    {
        handler = kernelInterruptHandlerTable[intId];
    }
    else
    {
        handler = kernelPanicHandler;
    }

    /* Execute the handler */
    handler(pCurrentThread);


}

void interruptInit(void)
{
    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Initializing interrupt manager.");

    /* Blank custom interrupt handlers */
    memset(kernelInterruptHandlerTable,
           0,
           sizeof(custom_handler_t) * INT_ENTRY_COUNT);

    /* Attach the special PANIC interrupt for when we don't know what to do */
    kernelInterruptHandlerTable[PANIC_INT_LINE] = kernelPanicHandler;

    /* Init state */
    interruptDisable();
    sSpuriousIntCount = 0;

    /* Init driver */
    sInterruptDriver.pGetIrqInterruptLine = _initDriverGetIrqIntLine;
    sInterruptDriver.pHandleSpurious      = _initDriverHandleSpurious;
    sInterruptDriver.pSetIrqEOI           = _initDriverSetIrqEOI;
    sInterruptDriver.pSetIrqMask          = _initDriverSetIrqMask;

    TEST_POINT_FUNCTION_CALL(interrupt_test, TEST_INTERRUPT_ENABLED);
}

OS_RETURN_E interruptSetDriver(const interrupt_driver_t* kpDriver)
{
    uint32_t intState;

    if(kpDriver == NULL ||
       kpDriver->pSetIrqEOI == NULL ||
       kpDriver->pSetIrqMask == NULL ||
       kpDriver->pHandleSpurious == NULL ||
       kpDriver->pGetIrqInterruptLine == NULL)
    {


        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(intState);

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

    EXIT_CRITICAL(intState);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Set new interrupt driver at 0x%p",
                 kpDriver);



    return OS_NO_ERR;
}

OS_RETURN_E interruptRegister(const uint32_t kInterruptLine,
                              custom_handler_t handler)
{
    uint32_t intState;

    if(kInterruptLine < MIN_INTERRUPT_LINE ||
       kInterruptLine > MAX_INTERRUPT_LINE)
    {
        KERNEL_ERROR("Invalid registered interrupt line: %d\n", kInterruptLine);


        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_ERROR("NULL registered interrupt handler\n");


        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(intState);

    if(kernelInterruptHandlerTable[kInterruptLine] != NULL)
    {
        EXIT_CRITICAL(intState);
        KERNEL_ERROR("Already registered interrupt line: %d\n", kInterruptLine);


        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    kernelInterruptHandlerTable[kInterruptLine] = handler;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Added INT %u handler at 0x%p",
                 kInterruptLine,
                 handler);



    EXIT_CRITICAL(intState);

    return OS_NO_ERR;
}

OS_RETURN_E interruptRemove(const uint32_t kInterruptLine)
{
    uint32_t intState;



    if(kInterruptLine < MIN_INTERRUPT_LINE ||
       kInterruptLine > MAX_INTERRUPT_LINE)
    {
        KERNEL_ERROR("Invalid removed interrupt line: %d\n", kInterruptLine);


        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    ENTER_CRITICAL(intState);

    if(kernelInterruptHandlerTable[kInterruptLine] == NULL)
    {
        EXIT_CRITICAL(intState);

        KERNEL_ERROR("Not registered interrupt line: %d\n", kInterruptLine);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    kernelInterruptHandlerTable[kInterruptLine] = NULL;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Removed interrupt %u handle", kInterruptLine);



    EXIT_CRITICAL(intState);

    return OS_NO_ERR;
}

OS_RETURN_E interruptIRQRegister(const uint32_t kIrqNumber,
                                 custom_handler_t handler)
{
    int32_t     interruptLine;
    OS_RETURN_E retCode;

    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIrqInterruptLine(kIrqNumber);

    if(interruptLine < 0)
    {
        KERNEL_ERROR("Invalid IRQ interrupt line: %d\n", kIrqNumber);

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
        KERNEL_ERROR("Invalid IRQ interrupt line: %d\n", kIrqNumber);


        return OS_ERR_NO_SUCH_IRQ;
    }

    retCode = interruptRemove(interruptLine);



    return retCode;
}

void interruptRestore(const uint32_t kPrevState)
{

    if(kPrevState != 0)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Enabled HW INT");
        _cpuSetInterrupt();
    }
}

uint32_t interruptDisable(void)
{
    uint32_t prevState;

    prevState = _cpuGeIntState();



    if(prevState == 0)
    {
        return 0;
    }

    _cpuClearInterrupt();

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Disabled HW INT");

    return prevState;
}

void interruptIRQSetMask(const uint32_t kIrqNumber, const uint32_t kEnabled)
{


    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IRQ Mask change: %u %u",
                 kIrqNumber,
                 kEnabled);

    sInterruptDriver.pSetIrqMask(kIrqNumber, kEnabled);
}

void interruptIRQSetEOI(const uint32_t kIrqNumber)
{


    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,
                 "INTERRUPTS",
                 "IRQ EOI: %u",
                 kIrqNumber);

    sInterruptDriver.pSetIrqEOI(kIrqNumber);
}

/************************************ EOF *************************************/