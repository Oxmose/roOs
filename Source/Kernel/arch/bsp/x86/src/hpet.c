/*******************************************************************************
 * @file hpet.c
 *
 * @see hpet.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/07/2024
 *
 * @version 1.0
 *
 * @brief HPET (High Precision Event Timer) driver.
 *
 * @details HPET (High Precision Event Timer) driver. Timer
 * source in the kernel. This driver provides basic access to the HPET and 
 * its features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <mmio.h>         /* Memory mapped accessses */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */
#include <critical.h>     /* Kernel critical locks */
#include <time_mgt.h>     /* Timers manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <hpet.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for inetrrupt  */
#define HPET_FDT_INT_PROP "interrupts"
/** @brief FDT property for frequency */
#define HPET_FDT_SELFREQ_PROP "freq"


/** @brief Current module name */
#define MODULE_NAME "X86 HPET"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 HPET Timer driver controler. */
typedef struct
{
    /** @brief HPET Timer interrupt number. */
    uint8_t interruptNumber;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Keeps track on the HPET enabled state. */
    uint32_t disabledNesting;

    /** @brief Timer base addresss */
    uintptr_t baseAddress;

    /** @brief Time base driver */
    const kernel_timer_t* kpBaseTimer;
} hpet_ctrl_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the HPET to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the HPET to ensure correctness of
 * execution. Due to the critical nature of the HPET, any error generates
 * a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define LAPICT_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/** @brief Cast a pointer to a LAPIC driver controler */
#define GET_CONTROLER(PTR) ((hpet_ctrl_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the HPET driver to the system.
 *
 * @details Attaches the HPET driver to the system. This function will
 * use the FDT to initialize the HPEThardware  and retreive the HPET
 * Timer parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _hpetAttach(const fdt_node_t* pkFdtNode);


/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief LAPIC Timer driver instance. */
static driver_t sX86HPETDriver = {
    .pName         = "X86 HPET Driver",
    .pDescription  = "X86 High Precision Event Timer for roOs.",
    .pCompatible   = "x86,x86-hpetr",
    .pVersion      = "1.0",
    .pDriverAttach = _hpetAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _lapicTimerAttach(const fdt_node_t* pkFdtNode)
{
    (void)pkFdtNode;
    return OS_NO_ERR;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86HPETDriver);

/************************************ EOF *************************************/