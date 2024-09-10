/*******************************************************************************
 * @file uart.c
 *
 * @see uart.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.1
 *
 * @brief UART communication driver.
 *
 * @details UART communication driver. Initializes the uart ports as in and
 * output. The uart can be used to output data or communicate with other
 * prepherals that support this communication method
 *
 * @warning Only one UART can be used as input at the moment.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>          /* Virtual File System*/
#include <ioctl.h>        /* IOCTL commands */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Memory allocation */
#include <x86cpu.h>       /* CPU port manipulation */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipualtion */
#include <kerror.h>       /* Kernel error */
#include <devtree.h>      /* Device tree */
#include <console.h>      /* Console driver manager */
#include <critical.h>     /* Kernel locks */
#include <drivermgr.h>    /* Driver manager service */
#include <semaphore.h>    /* Kernel semaphores */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <puart.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 UART"

/** @brief FDT property for baudrate */
#define UART_FDT_RATE_PROP   "baudrate"
/** @brief FDT property for comm ports */
#define UART_FDT_COMM_PROP   "comm"
/** @brief FDT property for interrupt  */
#define UART_FDT_INT_PROP "interrupts"
/** @brief FDT property for device path */
#define UART_FDT_DEVICE_PROP "device"

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

/** @brief Defines the maximal size of the UART input buffer */
#define UART_INPUT_BUFFER_SIZE 128

/** @brief UART Interrupt status data available mask */
#define UART_INT_STATUS_DATA_AVAILABLE 0x1

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

/** @brief x86 UART driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief Baudrate */
    SERIAL_BAUDRATE_E baudrate;

    /** @brief The UART IRQ number */
    uint32_t irqNumber;

    /** @brief Current start UART input buffer cursor */
    size_t inputBufferStartCursor;

    /** @brief Current end UART input buffer cursor */
    size_t inputBufferEndCursor;

    /** @brief Input buffer */
    uint8_t* pInputBuffer;

    /** @brief Input buffer lock */
    kernel_spinlock_t inputBufferLock;

    /** @brief Input buffer semaphore */
    semaphore_t inputBufferSem;

    /** @brief Stores the VFS driver */
    vfs_driver_t vfsDriver;

    /** @brief Driver's lock */
    kernel_spinlock_t lock;
} uart_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the UART to ensure correctness of execution.
 *
 * @details Assert macro used by the UART to ensure correctness of execution.
 * Due to the critical nature of the UART, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define UART_ASSERT(COND, MSG, ERROR) {                     \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

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
 * @param[in, out] pLock The device lock.
 * @param[in] kPort The desired port to write the data to.
 * @param[in] kData The byte to write to the uart port.
 */
static inline void _uartWrite(kernel_spinlock_t* pLock,
                              const uint16_t     kPort,
                              const uint8_t      kData);

/**
 * @brief Clears the screen.
 *
 * @param[in, out] pDrvCtrl The UART driver controler to use.
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
 * @param[in, out] pDrvCtrl The UART driver controler to use.
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

/**
 * @brief Handles a UART interrupt.
 *
 * @details Handles a UART interrupt. Fills the input buffer with the input data
 * and unblock a thread if it is blocked on the input.
 *
 * @param[in] pCurrentThread Unused.
 */
static void _uartInterruptHandler(kernel_thread_t* pCurrentThread);

/**
 * @brief Reads data from the UART input buffer.
 *
 * @details Reads data from the UART input buffer. The function returns the
 * number of bytes read. If the buffer is empty, the function is blocking until
 * the buffer is filled with the required number of bytes.
 *
 * @param[in] pDrvCtrl The driver to be used.
 * @param[out] pBuffer The buffer used to receive data.
 * @param[in] kBufferSize The number of bytes to read.
 *
 * @return The function returns the number of bytes read or -1 on error.
 */
static ssize_t _uartRead(void*        pDrvCtrl,
                         char*        pBuffer,
                         const size_t kBufferSize);

/**
 * @brief UART VFS open hook.
 *
 * @details UART VFS open hook. This function returns a handle to control the
 * UART driver through VFS.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] kpPath The path in the UART driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _uartVfsOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode);

/**
 * @brief UART VFS close hook.
 *
 * @details UART VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _uartVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief UART VFS write hook.
 *
 * @details UART VFS write hook. This function writes a string to the UART.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _uartVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count);

/**
 * @brief UART VFS read hook.
 *
 * @details UART VFS read hook. This function reads a string from the UART.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] kpBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _uartVfsRead(void*  pDrvCtrl,
                            void*  pHandle,
                            void*  kpBuffer,
                            size_t count);

/**
 * @brief UART VFS IOCTL hook.
 *
 * @details UART VFS IOCTL hook. This function performs the IOCTL for the UART
 * driver.
 *
 * @param[in, out] pDrvCtrl The UART driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _uartVfsIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief UART driver instance. */
static driver_t sX86UARTDriver = {
    .pName         = "X86 UART Driver",
    .pDescription  = "X86 UART Driver for roOs",
    .pCompatible   = "x86,x86-generic-serial",
    .pVersion      = "2.1",
    .pDriverAttach = _uartAttach
};

/** @brief Stores the UART used for input, only one can be used */
static uart_controler_t* spInputCtrl = NULL;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static OS_RETURN_E _uartAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*   kpUintProp;
    const char*       kpStrProp;
    size_t            propLen;
    OS_RETURN_E       retCode;
    OS_RETURN_E       error;
    uart_controler_t* pDrvCtrl;
    bool_t            isSemInit;
    bool_t            isInputBufferSet;

    isSemInit        = FALSE;
    isInputBufferSet = FALSE;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(uart_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(uart_controler_t));
    pDrvCtrl->vfsDriver = VFS_DRIVER_INVALID;
    KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);


    /* Get the UART CPU communication ports */
    kpUintProp = fdtGetProp(pkFdtNode, UART_FDT_COMM_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);

    /* Get the UART CPU baudrate */
    kpUintProp = fdtGetProp(pkFdtNode, UART_FDT_RATE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->baudrate = FDTTOCPU32(*kpUintProp);

    /* Init line */
    _cpuOutB(0x00, SERIAL_DATA_PORT_2(pDrvCtrl->cpuCommPort));
    _uartSetBaudrate(_uartGetCanonicalRate(pDrvCtrl->baudrate),
                     pDrvCtrl->cpuCommPort);
    _uartSetLine(SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1,
                 pDrvCtrl->cpuCommPort);
    _uartSetBuffer(0xC0 | SERIAL_ENABLE_FIFO | SERIAL_CLEAR_RECV_FIFO |
                   SERIAL_CLEAR_SEND_FIFO | SERIAL_FIFO_DEPTH_14,
                   pDrvCtrl->cpuCommPort);

    /* Get IRQ lines */
    kpUintProp = fdtGetProp(pkFdtNode, UART_FDT_INT_PROP, &propLen);
    if(kpUintProp != NULL && propLen == sizeof(uint32_t) * 2)
    {
        /* Check that we are the only input port */
        if(spInputCtrl != NULL)
        {
            retCode = OS_ERR_ALREADY_EXIST;
            goto ATTACH_END;
        }

        /* Init buffer */
        pDrvCtrl->inputBufferStartCursor = 0;
        pDrvCtrl->inputBufferEndCursor   = 0;
        pDrvCtrl->pInputBuffer = kmalloc(UART_INPUT_BUFFER_SIZE);
        if(pDrvCtrl->pInputBuffer == NULL)
        {
            retCode = OS_ERR_NO_MORE_MEMORY;
            goto ATTACH_END;
        }
        isInputBufferSet = TRUE;
        KERNEL_SPINLOCK_INIT(pDrvCtrl->inputBufferLock);

        retCode = semInit(&pDrvCtrl->inputBufferSem,
                          0,
                          SEMAPHORE_FLAG_QUEUING_PRIO | SEMAPHORE_FLAG_BINARY);
        if(retCode != OS_NO_ERR)
        {
            goto ATTACH_END;
        }
        isSemInit = TRUE;

        pDrvCtrl->irqNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

        /* Register the interrupt */
        retCode = interruptIRQRegister(pDrvCtrl->irqNumber,
                                       _uartInterruptHandler);

        /* If we are already registered, it might be another uart, it's ok */
        if(retCode != OS_NO_ERR)
        {
            goto ATTACH_END;
        }

        /* Set the interrupt mask */
        interruptIRQSetMask(pDrvCtrl->irqNumber, TRUE);
        interruptIRQSetEOI(pDrvCtrl->irqNumber);

        /* Enable interrupt on receive */
        _cpuOutB(0x0B, SERIAL_MODEM_COMMAND_PORT(pDrvCtrl->cpuCommPort));
        _cpuOutB(0x01, SERIAL_DATA_PORT_2(pDrvCtrl->cpuCommPort));

        /* Set the input driver */
        spInputCtrl = pDrvCtrl;
    }
    else
    {
        pDrvCtrl->irqNumber = -1;
    }

    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, UART_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pDrvCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                            pDrvCtrl,
                                            _uartVfsOpen,
                                            _uartVfsClose,
                                            _uartVfsRead,
                                            _uartVfsWrite,
                                            NULL,
                                            _uartVfsIOCTL);
    if(pDrvCtrl->vfsDriver == VFS_DRIVER_INVALID)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

ATTACH_END:

    if(retCode != OS_NO_ERR)
    {
        if(isSemInit == TRUE)
        {
            semDestroy(&pDrvCtrl->inputBufferSem);
        }
        if(isInputBufferSet == TRUE)
        {
            kfree(pDrvCtrl->pInputBuffer);
        }
        if(pDrvCtrl != NULL)
        {
            if(pDrvCtrl->vfsDriver != VFS_DRIVER_INVALID)
            {
                error = vfsUnregisterDriver(&pDrvCtrl->vfsDriver);
                if(error != OS_NO_ERR)
                {
                    PANIC(error,
                          MODULE_NAME,
                          "Failed to unregister VFS driver");
                }
            }

            kfree(pDrvCtrl);
        }
    }

    return retCode;
}


static inline void _uartSetLine(const uint8_t kAttr, const uint16_t kCom)
{
    _cpuOutB(kAttr, SERIAL_LINE_COMMAND_PORT(kCom));
}

static inline void _uartSetBuffer(const uint8_t kAttr, const uint16_t kCom)
{
    _cpuOutB(kAttr, SERIAL_FIFO_COMMAND_PORT(kCom));
}

static inline void _uartSetBaudrate(const SERIAL_BAUDRATE_E kRate,
                                    const uint16_t          kCom)
{
    _cpuOutB(SERIAL_DLAB_ENABLED, SERIAL_LINE_COMMAND_PORT(kCom));
    _cpuOutB((kRate >> 8) & 0x00FF, SERIAL_DATA_PORT(kCom));
    _cpuOutB(kRate & 0x00FF, SERIAL_DATA_PORT_2(kCom));
}

static inline void _uartWrite(kernel_spinlock_t* pLock,
                              uint16_t           kPort,
                              const uint8_t      kData)
{

    /* Wait for empty transmit */
    KERNEL_LOCK(*pLock);
    while((_cpuInB(SERIAL_LINE_STATUS_PORT(kPort)) & 0x20) == 0){}
    if(kData == '\n')
    {
        _cpuOutB('\r', kPort);
        while((_cpuInB(SERIAL_LINE_STATUS_PORT(kPort)) & 0x20) == 0){}
        _cpuOutB('\n', kPort);
    }
    else
    {
        _cpuOutB(kData, kPort);
    }
    KERNEL_UNLOCK(*pLock);
}

void _uartClear(void* pDrvCtrl)
{
    uint8_t i;

    /* On 80x25 screen, just print 25 line feed. */
    for(i = 0; i < 25; ++i)
    {
        _uartWrite(&(GET_CONTROLER(pDrvCtrl)->lock),
                   GET_CONTROLER(pDrvCtrl)->cpuCommPort,
                   '\n');
    }
}

void _uartScroll(void*                    pDrvCtrl,
                 const SCROLL_DIRECTION_E kDirection,
                 const uint32_t           kLines)
{
    uint32_t i;

    if(kDirection == SCROLL_DOWN)
    {
        /* Just print lines_count line feed. */
        for(i = 0; i < kLines; ++i)
        {
            _uartWrite(&(GET_CONTROLER(pDrvCtrl)->lock),
                       GET_CONTROLER(pDrvCtrl)->cpuCommPort,
                       '\n');
        }
    }
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

static void _uartInterruptHandler(kernel_thread_t* pCurrentThread)
{
    uint8_t     intStatus;
    uint8_t     data;
    OS_RETURN_E error;
    size_t      availableSpace;

    (void)pCurrentThread;

    if(spInputCtrl == NULL)
    {
        /* Set EOI */
        interruptIRQSetEOI(spInputCtrl->irqNumber);
        return;
    }

    /* Check is we received a data */
    intStatus = _cpuInB(SERIAL_LINE_STATUS_PORT(spInputCtrl->cpuCommPort));
    if((intStatus & UART_INT_STATUS_DATA_AVAILABLE) != 0)
    {
        data = _cpuInB(SERIAL_DATA_PORT(spInputCtrl->cpuCommPort));

        /* Try to add the new data to the buffer */
        KERNEL_LOCK(spInputCtrl->inputBufferLock);

        if(spInputCtrl->inputBufferEndCursor >=
           spInputCtrl->inputBufferStartCursor)
        {
            availableSpace = UART_INPUT_BUFFER_SIZE -
                             spInputCtrl->inputBufferEndCursor +
                             spInputCtrl->inputBufferStartCursor;
        }
        else
        {
            availableSpace = spInputCtrl->inputBufferStartCursor -
                             spInputCtrl->inputBufferEndCursor;
        }

        if(availableSpace > 0)
        {
            /* Read the data */
            spInputCtrl->pInputBuffer[spInputCtrl->inputBufferEndCursor] = data;

            spInputCtrl->inputBufferEndCursor =
                (spInputCtrl->inputBufferEndCursor + 1) %
                UART_INPUT_BUFFER_SIZE;
        }

        KERNEL_UNLOCK(spInputCtrl->inputBufferLock);

        /* Post the semaphore */
        error = semPost(&spInputCtrl->inputBufferSem);
        UART_ASSERT(error == OS_NO_ERR, "Failed to post UART semaphore", error);

    }

    /* Set EOI */
    interruptIRQSetEOI(spInputCtrl->irqNumber);
}

static ssize_t _uartRead(void*        pDrvCtrl,
                         char*        pBuffer,
                         const size_t kBufferSize)
{
    OS_RETURN_E error;
    size_t      toRead;
    size_t      bytesToRead;
    size_t      usedSpace;
    size_t      i;

    if(pDrvCtrl != spInputCtrl || spInputCtrl == NULL || pBuffer == NULL)
    {
        return -1;
    }

    toRead = kBufferSize;
    while(toRead != 0)
    {
        /* Copy if we can */
        error = semWait(&spInputCtrl->inputBufferSem);
        UART_ASSERT(error == OS_NO_ERR,
                    "Failed to wait UART semaphore",
                    error);

        KERNEL_LOCK(spInputCtrl->inputBufferLock);

        if(spInputCtrl->inputBufferEndCursor >=
           spInputCtrl->inputBufferStartCursor)
        {
            usedSpace = spInputCtrl->inputBufferEndCursor -
                        spInputCtrl->inputBufferStartCursor;
        }
        else
        {
            usedSpace = UART_INPUT_BUFFER_SIZE -
                        spInputCtrl->inputBufferStartCursor +
                        spInputCtrl->inputBufferEndCursor;
        }

        /* Get what we can read */
        bytesToRead = MIN(toRead, usedSpace);

        /* Copy */
        for(i = 0; i < bytesToRead; ++i)
        {
            *pBuffer =
                spInputCtrl->pInputBuffer[spInputCtrl->inputBufferStartCursor];
            ++pBuffer;
            spInputCtrl->inputBufferStartCursor =
                (spInputCtrl->inputBufferStartCursor + 1) %
                UART_INPUT_BUFFER_SIZE;
        }

        toRead -= bytesToRead;
        usedSpace -= bytesToRead;

        KERNEL_UNLOCK(spInputCtrl->inputBufferLock);

        /* If we can still read data, post the semaphore, we read what we had to
         * since we used
         * bytesToRead = MIN(toRead, spInputCtrl->inputBufferCursor)
         */
        if(usedSpace > 0)
        {
            error = semPost(&spInputCtrl->inputBufferSem);
            UART_ASSERT(error == OS_NO_ERR,
                        "Failed to post UART semaphore",
                        error);
        }

    }

    return kBufferSize;
}

static void* _uartVfsOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode)
{
    (void)pDrvCtrl;
    (void)mode;

    /* The path must be empty */
    if((*kpPath == '/' && *(kpPath + 1) != 0) || *kpPath != 0)
    {
        return (void*)-1;
    }

    /* The flags must be O_RDWR */
    if(flags != O_RDWR)
    {
        return (void*)-1;
    }

    /* We don't need a handle, return NULL */
    return NULL;
}

static int32_t _uartVfsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;
    (void)pHandle;

    /* Nothing to do */
    return 0;
}

static ssize_t _uartVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count)
{
    const char* pCursor;
    size_t      coutSave;

    (void)pHandle;

    pCursor = (char*)kpBuffer;

    /* Output each character of the string */
    coutSave = count;
    while(pCursor != NULL && *pCursor != 0 && count > 0)
    {
        _uartWrite(&(GET_CONTROLER(pDrvCtrl)->lock),
                   GET_CONTROLER(pDrvCtrl)->cpuCommPort,
                   *pCursor);
        ++pCursor;
        --count;
    }

    return coutSave - count;
}

static ssize_t _uartVfsRead(void*  pDrvCtrl,
                            void*  pHandle,
                            void*  pBuffer,
                            size_t count)
{
    (void)pHandle;

    /* Check the capability */
    if(GET_CONTROLER(pDrvCtrl)->irqNumber != (uint32_t)-1)
    {
        return -1;
    }

    return _uartRead(pDrvCtrl, pBuffer, count);
}

static ssize_t _uartVfsIOCTL(void*    pDriverData,
                            void*    pHandle,
                            uint32_t operation,
                            void*    pArgs)
{
    int32_t                   retVal;
    cons_ioctl_args_scroll_t* pScrollArgs;

    (void)pHandle;

    /* Switch on the operation */
    retVal = 0;
    switch(operation)
    {
        case VFS_IOCTL_CONS_SCROLL:
            pScrollArgs = pArgs;
            _uartScroll(pDriverData,
                       pScrollArgs->direction,
                       pScrollArgs->lineCount);
            break;
        case VFS_IOCTL_CONS_CLEAR:
            _uartClear(pDriverData);
            break;
        case VFS_IOCTL_CONS_FLUSH:
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

#if DEBUG_LOG_UART

static kernel_spinlock_t sLock = KERNEL_SPINLOCK_INIT_VALUE;

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
        _uartWrite(&sLock, SERIAL_DEBUG_PORT, kpString[i]);
    }
}

void uartDebugPutChar(const char kCharacter)
{
    _uartWrite(&sLock, SERIAL_DEBUG_PORT, kCharacter);
}
#endif

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86UARTDriver);

/************************************ EOF *************************************/