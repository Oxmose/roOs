/*******************************************************************************
 * @file uart.c
 *
 * @see uart.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.0
 *
 * @brief UART communication driver.
 *
 * @details UART communication driver. Initializes the uart ports as in and
 * output. The uart can be used to output data or communicate with other
 * prepherals that support this communication method
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>          /* CPU port manipulation */
#include <kheap.h>        /* Memory allocation */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipualtion */
#include <kerror.h>       /* Kernel error */
#include <devtree.h>      /* Device tree */
#include <console.h>      /* Console driver manager */
#include <drivermgr.h>    /* Driver manager service */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <uart.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 UART"

/** @brief FDT property for baudrate */
#define UART_FDT_RATE_PROP   "baudrate"
/** @brief FDT property for comm ports */
#define UART_FDT_COMM_PROP   "comm"
/** @brief FDT property for console output set */
#define UART_FDT_IS_CON_PROP "is-console"

/** @brief Serial data length flag: 5 bits. */
#define SERIAL_DATA_LENGTH_5 0x00
/** @brief Serial data length flag: 6 bits. */
#define SERIAL_DATA_LENGTH_6 0x01
/** @brief Serial data length flag: 7 bits. */
#define SERIAL_DATA_LENGTH_7 0x02
/** @brief Serial data length flag: 8 bits. */
#define SERIAL_DATA_LENGTH_8 0x03

/** @brief Serial parity bit flag: 1 bit. */
#define SERIAL_STOP_BIT_1   0x00
/** @brief Serial parity bit flag: 2 bits. */
#define SERIAL_STOP_BIT_2   0x04

/** @brief Serial parity bit settings flag: none. */
#define SERIAL_PARITY_NONE  0x00
/** @brief Serial parity bit settings flag: odd. */
#define SERIAL_PARITY_ODD   0x01
/** @brief Serial parity bit settings flag: even. */
#define SERIAL_PARITY_EVEN  0x03
/** @brief Serial parity bit settings flag: mark. */
#define SERIAL_PARITY_MARK  0x05
/** @brief Serial parity bit settings flag: space. */
#define SERIAL_PARITY_SPACE 0x07

/** @brief Serial break control flag enabled. */
#define SERIAL_BREAK_CTRL_ENABLED  0x40
/** @brief Serial break control flag disabled. */
#define SERIAL_BREAK_CTRL_DISABLED 0x00

/** @brief Serial dlab flag enabled. */
#define SERIAL_DLAB_ENABLED  0x80
/** @brief Serial dlab flag disabled. */
#define SERIAL_DLAB_DISABLED 0x00

/** @brief Serial fifo enable flag. */
#define SERIAL_ENABLE_FIFO       0x01
/** @brief Serial fifo clear receive flag. */
#define SERIAL_CLEAR_RECV_FIFO   0x02
/** @brief Serial fifo clear send flag. */
#define SERIAL_CLEAR_SEND_FIFO   0x04
/** @brief Serial DMA accessed fifo flag. */
#define SERIAL_DMA_ACCESSED_FIFO 0x08

/** @brief Serial fifo depth flag: 14 bits. */
#define SERIAL_FIFO_DEPTH_14     0x00
/** @brief Serial fifo depth flag: 64 bits. */
#define SERIAL_FIFO_DEPTH_64     0x10

/**
 * @brief Computes the data port for the serial port which base port ID is
 * given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_DATA_PORT(port)          (port)
/**
 * @brief Computes the aux data port for the serial port which base port ID is
 * given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_DATA_PORT_2(port)        (port + 1)
/**
 * @brief Computes the fifo command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_FIFO_COMMAND_PORT(port)  (port + 2)
/**
 * @brief Computes the line command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_LINE_COMMAND_PORT(port)  (port + 3)
/**
 * @brief Computes the modem command port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_MODEM_COMMAND_PORT(port) (port + 4)
/**
 * @brief Computes the line status port for the serial port which base port ID
 * is given as parameter.
 *
 * @param[in] port The base port ID of the serial port.
 */
#define SERIAL_LINE_STATUS_PORT(port)   (port + 5)

#if DEBUG_LOG_UART
/** @brief Defines the port that is used to print debug data. */
#define SERIAL_DEBUG_PORT 0x3F8
#endif

/** @brief Cast a pointer to a UART driver controler */
#define GET_CONTROLER(PTR) ((uart_controler_t*)PTR)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Serial baudrate enumation. Enumerates all the supported baudrates.
 * The value of the enumeration is the transmission rate divider.
 */
typedef enum
{
    /** @brief Baudrate 50Bd. */
    BAURDATE_50     = 2304,
    /** @brief Baudrate 75Bd. */
    BAUDRATE_75     = 1536,
    /** @brief Baudrate 150Bd. */
    BAUDRATE_150    = 768,
    /** @brief Baudrate 300Bd. */
    BAUDRATE_300    = 384,
    /** @brief Baudrate 600Bd. */
    BAUDRATE_600    = 192,
    /** @brief Baudrate 1200Bd. */
    BAUDRATE_1200   = 96,
    /** @brief Baudrate 1800Bd. */
    BAUDRATE_1800   = 64,
    /** @brief Baudrate 2400Bd. */
    BAUDRATE_2400   = 48,
    /** @brief Baudrate 4800Bd. */
    BAUDRATE_4800   = 24,
    /** @brief Baudrate 7200Bd. */
    BAUDRATE_7200   = 16,
    /** @brief Baudrate 9600Bd. */
    BAUDRATE_9600   = 12,
    /** @brief Baudrate 14400Bd. */
    BAUDRATE_14400  = 8,
    /** @brief Baudrate 19200Bd. */
    BAUDRATE_19200  = 6,
    /** @brief Baudrate 38400Bd. */
    BAUDRATE_38400  = 3,
    /** @brief Baudrate 57600Bd. */
    BAUDRATE_57600  = 2,
    /** @brief Baudrate 115200Bd. */
    BAUDRATE_115200 = 1,
} SERIAL_BAUDRATE_E;

/** @brief x86 UART Text driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief Baudrate */
    SERIAL_BAUDRATE_E baudrate;
} uart_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the UART driver to the system.
 *
 * @details Attaches the UART driver to the system. This function will use the
 * FDT to initialize the UART hardware and retreive the UART parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _uartAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Sets line parameters for the desired port.
 *
 * @details Sets line parameters for the desired port.
 *
 * @param[in] kAttr The settings for the port's line.
 * @param[in] kCom The port to set.
 */
static inline void _uartSetLine(const uint8_t kAttr, const uint16_t kCom);

/**
 * @brief Sets buffer parameters for the desired port.
 *
 * @details Sets buffer parameters for the desired port.
 *
 * @param[in] kAttr The settings for the port's line.
 * @param[in] kCom The port to set.
 *
 * @return OS_NO_ERR on success, no other value is returned.
 */
static inline void _uartSetBuffer(const uint8_t kAttr, const uint16_t kCom);

/**
 * @brief Sets the port's baudrate.
 *
 * @details Sets the port's baudrate.
 *
 * @param[in] kRate The desired baudrate for the port.
 * @param[in] kCom The port to set.
 */
static inline void _uartSetBaudrate(const SERIAL_BAUDRATE_E kRate,
                                    const uint16_t          kCom);

/**
 * @brief Writes the data given as patameter on the desired port.
 *
 * @details The function will output the data given as parameter on the selected
 * port. This call is blocking until the data has been sent to the uart port
 * controler.
 *
 * @param[in] kPort The desired port to write the data to.
 * @param[in] kData The byte to write to the uart port.
 */
static inline void _uartWrite(const uint16_t kPort, const uint8_t kData);

/**
 * @brief Write the string given as patameter on the debug port.
 *
 * @details The function will output the data given as parameter on the debug
 * port. This call is blocking until the data has been sent to the uart port
 * controler.
 *
 * @param[in-out] pDrvCtrl The UART driver controler to use.
 * @param[in] kpString The string to write to the uart port.
 *
 * @warning string must be NULL terminated.
 */
static void _uartPutString(void* pDrvCtrl, const char* kpString);

/**
 * @brief Write the character given as patameter on the debug port.
 *
 * @details The function will output the character given as parameter on the
 * debug port. This call is blocking until the data has been sent to the uart
 * port controler.
 *
 * @param[in-out] pDrvCtrl The UART driver controler to use.
 * @param[in] kCharacter The character to write to the uart port.
 */
static void _uartPutChar(void* pDrvCtrl, const char kCharacter);

/**
 * @brief Clears the screen.
 *
 * @param[in-out] pDrvCtrl The UART driver controler to use.
 * @details On 80x25 uart screen, this function will print 80 line feeds
 * and thus, clear the screen.
 */
static void _uartClear(void* pDrvCtrl);

/**
 * @brief Scrolls the screen downn.
 *
 * @details Scrolls the screen by printing lines feed to the uart.
 * This function can only be called with parameter direction to
 * SCROLL_DOWN. Otherwise, this function has no effect.
 *
 * @param[in-out] pDrvCtrl The UART driver controler to use.
 * @param[in] kDirection Should always be SCROLL_DOWN.
 * @param[in] kLines The amount of lines to scroll down.
 */
static void _uartScroll(void*                    pDrvCtrl,
                        const SCROLL_DIRECTION_E kDirection,
                        const uint32_t           kLines);

/**
 * @brief Returns the canonical baudrate for a given BPS baudrate
 *
 * @details Returns the canonical baudrate for a given BPS baudrate based on the
 * driver's specifications.
 *
 * @param[in] kBaudrate The BPS baudrate to convert.
 *
 * @return The canonical baudrate for a given BPS baudrate is returned.
*/
static SERIAL_BAUDRATE_E _uartGetCanonicalRate(const uint32_t kBaudrate);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief UART driver instance. */
driver_t x86UARTDriver = {
    .pName         = "X86 UART Text Driver",
    .pDescription  = "X86 UART Text Driver for UTK",
    .pCompatible   = "x86,x86-generic-serial",
    .pVersion      = "2.0",
    .pDriverAttach = _uartAttach
};

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static OS_RETURN_E _uartAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t*  ptrProp;
    size_t            propLen;
    OS_RETURN_E       retCode;
    uart_controler_t* pDrvCtrl;
    console_driver_t* pConsoleDrv;

    pDrvCtrl    = NULL;
    pConsoleDrv = NULL;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_UART_INIT_START, 0);

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(uart_controler_t));
    if(pDrvCtrl == NULL)
    {
        KERNEL_ERROR("Failed to allocate driver controler.\n");
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    pConsoleDrv = kmalloc(sizeof(uart_controler_t));
    if(pConsoleDrv == NULL)
    {
        KERNEL_ERROR("Failed to allocate driver instance.\n");
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    pConsoleDrv->pClear           = _uartClear,
    pConsoleDrv->pPutCursor       = NULL,
    pConsoleDrv->pSaveCursor      = NULL,
    pConsoleDrv->pRestoreCursor   = NULL,
    pConsoleDrv->pScroll          = _uartScroll,
    pConsoleDrv->pSetColorScheme  = NULL,
    pConsoleDrv->pSaveColorScheme = NULL,
    pConsoleDrv->pPutString       = _uartPutString,
    pConsoleDrv->pPutChar         = _uartPutChar,
    pConsoleDrv->pDriverCtrl      = pDrvCtrl;

    /* Get the UART CPU communication ports */
    ptrProp = fdtGetProp(pkFdtNode, UART_FDT_COMM_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*ptrProp);

#if DEBUG_LOG_UART
    /* Check if we are trying to attach the debug port */
    if(pDrvCtrl->cpuCommPort == SERIAL_DEBUG_PORT)
    {
        retCode = OS_ERR_UNAUTHORIZED_ACTION;
        goto ATTACH_END;
    }
#endif

    /* Get the UART CPU baudrate */
    ptrProp = fdtGetProp(pkFdtNode, UART_FDT_RATE_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the baudrate from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->baudrate = FDTTOCPU32(*ptrProp);

    /* Init line */
    _uartSetBaudrate(_uartGetCanonicalRate(pDrvCtrl->baudrate),
                     pDrvCtrl->cpuCommPort);
    _uartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1,
                 pDrvCtrl->cpuCommPort);
    _uartSetBuffer(0xC0 | SERIAL_ENABLE_FIFO | SERIAL_CLEAR_RECV_FIFO |
                   SERIAL_CLEAR_SEND_FIFO | SERIAL_FIFO_DEPTH_14,
                   pDrvCtrl->cpuCommPort);

    /* Set as output if needed */
    if(fdtGetProp(pkFdtNode, UART_FDT_IS_CON_PROP, &propLen) != NULL)
    {
        retCode = consoleSetDriver(pConsoleDrv);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set UART driver as console driver.\n");
            goto ATTACH_END;
        }
    }

    retCode = OS_NO_ERR;

ATTACH_END:

    if(retCode != OS_NO_ERR)
    {
        if(pDrvCtrl != NULL)
        {
            kfree(pDrvCtrl);
        }
        if(pConsoleDrv != NULL)
        {
            kfree(pConsoleDrv);
        }
    }
    KERNEL_TRACE_EVENT(EVENT_KERNEL_UART_INIT_END, 1, (uintptr_t(retCode)));
    return retCode;
}


static inline void _uartSetLine(const uint8_t kAttr, const uint16_t kCom)
{
    _cpu_outb(kAttr, SERIAL_LINE_COMMAND_PORT(kCom));
}

static inline void _uartSetBuffer(const uint8_t kAttr, const uint16_t kCom)
{
    _cpu_outb(kAttr, SERIAL_FIFO_COMMAND_PORT(kCom));
}

static inline void _uartSetBaudrate(const SERIAL_BAUDRATE_E kRate,
                                    const uint16_t kCom)
{
    _cpu_outb(SERIAL_DLAB_ENABLED, SERIAL_LINE_COMMAND_PORT(kCom));
    _cpu_outb((kRate >> 8) & 0x00FF, SERIAL_DATA_PORT(kCom));
    _cpu_outb(kRate & 0x00FF, SERIAL_DATA_PORT_2(kCom));
}

static inline void _uartWrite(const uint16_t kPort, const uint8_t kData)
{
    /* Wait for empty transmit */
    while((_cpu_inb(SERIAL_LINE_STATUS_PORT(kPort)) & 0x20) == 0){}
    if(kData == '\n')
    {
        _cpu_outb('\r', kPort);
        _cpu_outb('\n', kPort);
    }
    else
    {
        _cpu_outb(kData, kPort);
    }
}

void _uartClear(void* pDrvCtrl)
{
    uint8_t i;

    /* On 80x25 screen, just print 25 line feed. */
    for(i = 0; i < 25; ++i)
    {
        _uartWrite(GET_CONTROLER(pDrvCtrl)->cpuCommPort, '\n');
    }
}

void _uartScroll(void* pDrvCtrl,
                 const SCROLL_DIRECTION_E kDirection,
                 const uint32_t kLines)
{
    uint32_t i;

    if(kDirection == SCROLL_DOWN)
    {
        /* Just print lines_count line feed. */
        for(i = 0; i < kLines; ++i)
        {
            _uartWrite(GET_CONTROLER(pDrvCtrl)->cpuCommPort, '\n');
        }
    }
}

void _uartPutString(void* pDrvCtrl,
                    const char* kpString)
{
    size_t i;
    size_t stringLen;

    stringLen = strlen(kpString);

    for(i = 0; i < stringLen; ++i)
    {
        _uartWrite(GET_CONTROLER(pDrvCtrl)->cpuCommPort, kpString[i]);
    }
}

void _uartPutChar(void* pDrvCtrl, const char kCharacter)
{
    _uartWrite(GET_CONTROLER(pDrvCtrl)->cpuCommPort, kCharacter);
}

static SERIAL_BAUDRATE_E _uartGetCanonicalRate(const uint32_t kBaudrate)
{
    switch (kBaudrate)
    {
        case 50:
            return 2304;
            break;
        case 75:
            return 1536;
            break;
        case 150:
            return 768;
            break;
        case 300:
            return 384;
            break;
        case 600:
            return 192;
            break;
        case 1200:
            return 96;
            break;
        case 1800:
            return 64;
            break;
        case 2400:
            return 48;
            break;
        case 4800:
            return 24;
            break;
        case 7200:
            return 16;
            break;
        case 9600:
            return 12;
            break;
        case 14400:
            return 8;
            break;
        case 19200:
            return 6;
            break;
        case 38400:
            return 3;
            break;
        case 57600:
            return 2;
            break;
        case 115200:
        default:
            return 1;
    }
}

#if DEBUG_LOG_UART
void uartDebugInit(void)
{
    /* Init line */
    _uartSetBaudrate(DEBUG_LOG_UART_RATE, SERIAL_DEBUG_PORT);
    _uartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1,
                SERIAL_DEBUG_PORT);
    _uartSetBuffer(0xC0 |
                SERIAL_ENABLE_FIFO |
                SERIAL_CLEAR_RECV_FIFO |
                SERIAL_CLEAR_SEND_FIFO |
                SERIAL_FIFO_DEPTH_14,
                SERIAL_DEBUG_PORT);


}

void uartDebugPutString(const char* kpString)
{
    size_t i;
    size_t stringLen;

    stringLen = strlen(kpString);

    for(i = 0; i < stringLen; ++i)
    {
        _uartWrite(SERIAL_DEBUG_PORT, kpString[i]);
    }
}

void uartDebugPutChar(const char kCharacter)
{
    _uartWrite(SERIAL_DEBUG_PORT, kCharacter);
}
#endif

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86UARTDriver);

/************************************ EOF *************************************/