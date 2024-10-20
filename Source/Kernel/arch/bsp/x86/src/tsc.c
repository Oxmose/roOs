/*******************************************************************************
 * @file tsc.c
 *
 * @see tsc.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 04/06/2024
 *
 * @version 1.0
 *
 * @brief TSC (Timestamp Counter) driver.
 *
 * @details TSC (Timestamp Counter) driver. Used as the tick timer
 * source in the kernel. This driver provides basic access to the TSC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>          /* CPU port manipulation */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */
#include <syslog.h>       /* Kernel Syslog */
#include <time_mgt.h>     /* Timers manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <tsc.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for frequency */
#define TSC_FDT_SELFREQ_PROP    "freq"

/** @brief Current module name */
#define MODULE_NAME "X86 TSC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 TSC driver controler. */
typedef struct
{
    /** @brief Counter frequency. */
    uint32_t frequency;

    /** @brief Offset time */
    int64_t offsetTime;
} tsc_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/** @brief Cast a pointer to a RTC driver controler */
#define GET_CONTROLER(PTR) ((tsc_controler_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the TSC driver to the system.
 *
 * @details Attaches the TSC driver to the system. This function will use the
 * FDT to initialize the TSC hardware and retreive the TSC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _tscAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Unused, TSC does not support enabling / disabling.
 *
 * @details Unused, TSC does not support enabling / disabling.
 *
 * @param[in, out] pDrvCtrl Unused.
 */
static void _tscEnable(void* pDrvCtrl);

/**
 * @brief Unused, TSC does not support enabling / disabling.
 *
 * @details Unused, TSC does not support enabling / disabling.
 *
 * @param[in, out] pDrvCtrl Unused.
 */
static void _tscDisable(void* pDrvCtrl);

/**
 * @brief Returns the TSC count frequency in Hz.
 *
 * @details Returns the TSC count frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The TSC count frequency in Hz.
 */
static uint32_t _tscGetFrequency(void* pDrvCtrl);

/**
 * @brief Unused, TSC does not support interrupts.
 *
 * @details Unused, TSC does not support interrupts.
 *
 * @param[in, out] pDrvCtrl Unused.
 * @param[in] pHandler Unused.
 *
 * @return OS_ERR_NOT_SUPPORTED is always returned.
 */
static OS_RETURN_E _tscSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*));

/**
 * @brief Unused, TSC does not support interrupts.
 *
 * @details Unused, TSC does not support interrupts.
 *
 * @param[in, out] pDrvCtrl Unused.
 *
 * @return OS_ERR_NOT_SUPPORTED is always returned.
 */
static OS_RETURN_E _tscRemoveHandler(void* pDrvCtrl);

/**
 * @brief Returns the time elasped since the last timer's reset in ns.
 *
 * @details Returns the time elasped since the last timer's reset in ns. The
 * timer can be set with the pSetTimeNs function.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The time in nanosecond since the last timer reset is returned.
 */
static uint64_t _tscGetTimeNs(void* pDrvCtrl);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief TSC driver instance. */
static driver_t sX86TSCDriver = {
    .pName         = "X86 TSC Driver",
    .pDescription  = "X86 Timestamp Counter for roOs",
    .pCompatible   = "x86,x86-tsc",
    .pVersion      = "1.0",
    .pDriverAttach = _tscAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _tscAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*  kpUintProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    tsc_controler_t* pDrvCtrl;
    kernel_timer_t*  pTimerDrv;

    pDrvCtrl  = NULL;
    pTimerDrv = NULL;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(tsc_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(tsc_controler_t));

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pTimerDrv, 0, sizeof(kernel_timer_t));

    pTimerDrv->pGetFrequency  = _tscGetFrequency;
    pTimerDrv->pGetTimeNs     = _tscGetTimeNs;
    pTimerDrv->pEnable        = _tscEnable;
    pTimerDrv->pDisable       = _tscDisable;
    pTimerDrv->pSetHandler    = _tscSetHandler;
    pTimerDrv->pRemoveHandler = _tscRemoveHandler;
    pTimerDrv->pDriverCtrl    = pDrvCtrl;

    /* Get frequency */
    kpUintProp = fdtGetProp(pkFdtNode, TSC_FDT_SELFREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->frequency = FDTTOCPU32(*kpUintProp);

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

#if TSC_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "TSC Initialization end");
#endif

    return retCode;
}

static void _tscEnable(void* pDrvCtrl)
{
    (void)pDrvCtrl;
}

static void _tscDisable(void* pDrvCtrl)
{
    (void)pDrvCtrl;
}

static uint32_t _tscGetFrequency(void* pDrvCtrl)
{
    tsc_controler_t* pTscCtrl;

    pTscCtrl = GET_CONTROLER(pDrvCtrl);

    return pTscCtrl->frequency;
}

static OS_RETURN_E _tscSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*))
{
    (void)pDrvCtrl;
    (void)pHandler;

    return OS_ERR_NOT_SUPPORTED;
}

static OS_RETURN_E _tscRemoveHandler(void* pDrvCtrl)
{
    (void)pDrvCtrl;
    return OS_ERR_NOT_SUPPORTED;
}

static uint64_t _tscGetTimeNs(void* pDrvCtrl)
{
    tsc_controler_t* pTscCtrl;
    uint64_t         time;
    uint32_t         highPart;
    uint32_t         lowPart;

    pTscCtrl = GET_CONTROLER(pDrvCtrl);

    /* Get time */
    __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
    /* Manage Ghz frequencies: use picoseconds */
    time = 1000000000000UL / pTscCtrl->frequency *
           (((uint64_t)highPart << 32) | (uint64_t)lowPart);

    /* Return to ns and apply offset */
    time /= 1000;

    return time;
}

#ifdef _TRACING_ENABLED
uint64_t tracingTimerGetTick(void)
{
    uint32_t highPart;
    uint32_t lowPart;

    /* Get time */
    __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
    return (((uint64_t)highPart << 32) | (uint64_t)lowPart);
}
#endif

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86TSCDriver);

/************************************ EOF *************************************/