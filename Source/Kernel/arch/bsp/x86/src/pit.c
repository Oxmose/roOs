/*******************************************************************************
 * @file pit.c
 *
 * @see pit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.0
 *
 * @brief PIT (Programmable interval timer) driver.
 *
 * @details PIT (Programmable interval timer) driver. Used as the basic timer
 * source in the kernel. This driver provides basic access to the PIT.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <x86cpu.h>       /* CPU port manipulation */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */
#include <syslog.h>       /* Kernel Syslog */
#include <critical.h>     /* Kernel critical locks */
#include <time_mgt.h>     /* Timers manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <pit.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for interrupt  */
#define PIT_FDT_INT_PROP        "interrupts"
/** @brief FDT property for comm ports */
#define PIT_FDT_COMM_PROP       "comm"
/** @brief FDT property for comm ports */
#define PIT_FDT_QUARTZ_PROP     "qartz-freq"
/** @brief FDT property for frequency */
#define PIT_FDT_SELFREQ_PROP    "freq"
/** @brief FDT property for frequency range */
#define PIT_FDT_FREQRANGE_PROP  "freq-range"

/** @brief PIT set tick frequency divider command. */
#define PIT_COMM_SET_FREQ 0x43

/** @brief Current module name */
#define MODULE_NAME "X86 PIT"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 PIT driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief CPU data port. */
    uint16_t cpuDataPort;

    /** @brief PIT IRQ number. */
    uint8_t irqNumber;

    /** @brief Main quarts frequency. */
    uint32_t quartzFrequency;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Frequency range low. */
    uint32_t frequencyLow;

    /** @brief Frequency range low. */
    uint32_t frequencyHigh;

    /** @brief Keeps track on the PIT enabled state. */
    uint32_t disabledNesting;

    /** @brief The driver's lock */
    kernel_spinlock_t lock;
} pit_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the PIT to ensure correctness of execution.
 *
 * @details Assert macro used by the PIT to ensure correctness of execution.
 * Due to the critical nature of the PIT, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define PIT_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/** @brief Cast a pointer to a PIT driver controler */
#define GET_CONTROLER(PTR) ((pit_controler_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the PIT driver to the system.
 *
 * @details Attaches the PIT driver to the system. This function will use the
 * FDT to initialize the PIT hardware and retreive the PIT parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _pitAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Initial PIT interrupt handler.
 *
 * @details PIT interrupt handler set at the initialization of the PIT.
 * Dummy routine setting EOI.
 *
 * @param[in] pCurrThread Unused, the current thread at the
 * interrupt.
 */
static void _pitDummyHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Enables PIT ticks.
 *
 * @details Enables PIT ticks by clearing the PIT's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _pitEnable(void* pDrvCtrl);

/**
 * @brief Disables PIT ticks.
 *
 * @details Disables PIT ticks by setting the PIT's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _pitDisable(void* pDrvCtrl);

/**
 * @brief Sets the PIT's tick frequency.
 *
 * @details Sets the PIT's tick frequency. The value must be between the PIT
 * frequency range.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] kFreq The new frequency to be set to the PIT.
 *
 * @warning The value must be between in the PIT frequency range.
 */
static void _pitSetFrequency(void* pDrvCtrl, const uint32_t kFreq);

/**
 * @brief Returns the PIT tick frequency in Hz.
 *
 * @details Returns the PIT tick frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The PIT tick frequency in Hz.
 */
static uint32_t _pitGetFrequency(void* pDrvCtrl);

/**
 * @brief Sets the PIT tick handler.
 *
 * @details Sets the PIT tick handler. This function will be called at each PIT
 * tick received.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] pHandler The handler of the PIT interrupt.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
  * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the PIT interrupt line
  * is not allowed.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
 * - OS_ERR_ALREADY_EXIST is returned if a handler is already
 * registered for the PIT.
 */
static OS_RETURN_E _pitSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*));

/**
 * @brief Removes the PIT tick handler.
 *
 * @details Removes the PIT tick handler.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the PIT interrupt line
 * is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the PIT line has no handler
 * attached.
 */
static OS_RETURN_E _pitRemoveHandler(void* pDrvCtrl);

/**
 * @brief Acknowledge interrupt.
 *
 * @details Acknowledge interrupt.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _pitAckInterrupt(void* pDrvCtrl);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PIT driver instance. */
static driver_t sX86PITDriver = {
    .pName         = "X86 PIT Driver",
    .pDescription  = "X86 Programable Interval Timer Driver for roOs",
    .pCompatible   = "x86,x86-pit",
    .pVersion      = "2.0",
    .pDriverAttach = _pitAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _pitAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*  kpUintProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    pit_controler_t* pDrvCtrl;
    kernel_timer_t*  pTimerDrv;

    pDrvCtrl  = NULL;
    pTimerDrv = NULL;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(pit_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(pit_controler_t));

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pTimerDrv, 0, sizeof(kernel_timer_t));

    pTimerDrv->pGetFrequency  = _pitGetFrequency;
    pTimerDrv->pEnable        = _pitEnable;
    pTimerDrv->pDisable       = _pitDisable;
    pTimerDrv->pSetHandler    = _pitSetHandler;
    pTimerDrv->pRemoveHandler = _pitRemoveHandler;
    pTimerDrv->pTickManager   = _pitAckInterrupt;
    pTimerDrv->pDriverCtrl    = pDrvCtrl;

    /* Get IRQ lines */
    kpUintProp = fdtGetProp(pkFdtNode, PIT_FDT_INT_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->irqNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "IRQ: %d", pDrvCtrl->irqNumber);
#endif

    /* Get communication ports */
    kpUintProp = fdtGetProp(pkFdtNode, PIT_FDT_COMM_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "COMM: 0x%x | DATA: 0x%x",
           pDrvCtrl->cpuCommPort,
           pDrvCtrl->cpuDataPort);
#endif

    /* Get quartz frequency */
    kpUintProp = fdtGetProp(pkFdtNode, PIT_FDT_QUARTZ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->quartzFrequency = FDTTOCPU32(*kpUintProp);

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Quartz Frequency: %dHz",
           pDrvCtrl->quartzFrequency);
#endif

    /* Get selected frequency */
    kpUintProp = fdtGetProp(pkFdtNode, PIT_FDT_SELFREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->selectedFrequency = FDTTOCPU32(*kpUintProp);

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Selected Frequency: %dHz",
           pDrvCtrl->selectedFrequency);
#endif

    /* Get the frequency range */
    kpUintProp = fdtGetProp(pkFdtNode, PIT_FDT_FREQRANGE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->frequencyLow  = (uint32_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->frequencyHigh = (uint32_t)FDTTOCPU32(*(kpUintProp + 1));

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, PIT_DEBUG_ENABLED,
           MODULE_NAME,
           "Frequency Range: %dHz / %dHz",
           pDrvCtrl->frequencyLow,
           pDrvCtrl->frequencyHigh);
#endif

    /* Check if frequency is within bounds */
    if(pDrvCtrl->frequencyLow > pDrvCtrl->selectedFrequency ||
       pDrvCtrl->frequencyHigh < pDrvCtrl->selectedFrequency)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Init lock */
    KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);

    /* Init system times */
    pDrvCtrl->disabledNesting = 1;

    /* Set PIT frequency */
    _pitSetFrequency(pDrvCtrl, pDrvCtrl->selectedFrequency);

    /* Set the API driver */
    retCode = driverManagerSetDeviceData(pkFdtNode, pTimerDrv);

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        if(pDrvCtrl != NULL)
        {
            kfree(pDrvCtrl);
        }
        if(pTimerDrv != NULL)
        {
            kfree(pTimerDrv);
        }
        driverManagerSetDeviceData(pkFdtNode, NULL);
    }

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "PIT Initialization end");
#endif

    return retCode;
}

static void _pitDummyHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    PANIC(OS_ERR_UNAUTHORIZED_ACTION,
          MODULE_NAME,
          "PIT Dummy handler called");

    return;
}

static void _pitEnable(void* pDrvCtrl)
{
    pit_controler_t* pPitCtrl;

    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_LOCK(pPitCtrl->lock);

    if(pPitCtrl->disabledNesting > 0)
    {
        --pPitCtrl->disabledNesting;
    }

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Enable (nesting %d)",
           pPitCtrl->disabledNesting);
#endif

    if(pPitCtrl->disabledNesting == 0)
    {
        interruptIRQSetMask(pPitCtrl->irqNumber, true);
    }

    KERNEL_UNLOCK(pPitCtrl->lock);
}

static void _pitDisable(void* pDrvCtrl)
{
    pit_controler_t* pPitCtrl;

    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_LOCK(pPitCtrl->lock);

    if(pPitCtrl->disabledNesting < UINT32_MAX)
    {
        ++pPitCtrl->disabledNesting;
    }

    KERNEL_UNLOCK(pPitCtrl->lock);

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Disable (nesting %d)",
           pPitCtrl->disabledNesting);
#endif

    interruptIRQSetMask(pPitCtrl->irqNumber, false);

}

static void _pitSetFrequency(void* pDrvCtrl, const uint32_t kFreq)
{
    uint16_t         tickFreq;
    pit_controler_t* pPitCtrl;

    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    if(pPitCtrl->frequencyLow > kFreq || pPitCtrl->frequencyHigh < kFreq)
    {

        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Selected PIT frequency is not within range.");
        return;
    }

    KERNEL_LOCK(pPitCtrl->lock);

    pPitCtrl->selectedFrequency = kFreq;

    /* Set clock frequency */
    tickFreq = (uint16_t)(pPitCtrl->quartzFrequency / kFreq);
    _cpuOutB(PIT_COMM_SET_FREQ, pPitCtrl->cpuCommPort);
    _cpuOutB(tickFreq & 0x00FF, pPitCtrl->cpuDataPort);
    _cpuOutB(tickFreq >> 8, pPitCtrl->cpuDataPort);

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "New PIT frequency set (%d)",
           kFreq);
#endif

    KERNEL_UNLOCK(pPitCtrl->lock);
}

static uint32_t _pitGetFrequency(void* pDrvCtrl)
{
    pit_controler_t* pPitCtrl;

    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    return pPitCtrl->selectedFrequency;
}

static OS_RETURN_E _pitSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*))
{
    OS_RETURN_E      err;
    pit_controler_t* pPitCtrl;

    if(pHandler == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    _pitDisable(pDrvCtrl);

    err = interruptIRQRegister(pPitCtrl->irqNumber, pHandler);
    if(err != OS_NO_ERR)
    {

        return err;
    }

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "New PIT handler set 0x%p",
           pHandler);
#endif

    _pitEnable(pDrvCtrl);

    return err;
}

static OS_RETURN_E _pitRemoveHandler(void* pDrvCtrl)
{

#if PIT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Default PIT handler set 0x%p",
           _pitDummyHandler);
#endif
    return _pitSetHandler(pDrvCtrl, _pitDummyHandler);
}

static void _pitAckInterrupt(void* pDrvCtrl)
{
    pit_controler_t* pPitCtrl;
    pPitCtrl = GET_CONTROLER(pDrvCtrl);

    /* Set EOI */
    interruptIRQSetEOI(pPitCtrl->irqNumber);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86PITDriver);

/************************************ EOF *************************************/