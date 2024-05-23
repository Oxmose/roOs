/*******************************************************************************
 * @file uart.c
 *
 * @see uart.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/04/2023
 *
 * @version 1.0
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
#include <stdint.h>         /* Generic int types */
#include <stddef.h>         /* Standard definitions */
#include <string.h>         /* String manipulation */
#include <console.h>        /* Console driver manager */
#include <cpu.h>            /* CPU Ports */
#include <kerneloutput.h>  /* Kernel outputs */
#include <critical.h>       /* Critical sections */

/* Configuration files */
#include <config.h>

/* Header file */
#include <uart.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 UART"

/** @brief Serial COM1 base port ID. */
#define SERIAL_COM1_BASE 0x3F8
/** @brief Serial COM2 base port ID. */
#define SERIAL_COM2_BASE 0x2F8
/** @brief Serial COM3 base port ID. */
#define SERIAL_COM3_BASE 0x3E8
/** @brief Serial COM4 base port ID. */
#define SERIAL_COM4_BASE 0x2E8

/** @brief Redefinition of serial COM1 base port ID for ease of use. */
#define COM1 SERIAL_COM1_BASE
/** @brief Redefinition of serial COM2 base port ID for ease of use. */
#define COM2 SERIAL_COM2_BASE
/** @brief Redefinition of serial COM3 base port ID for ease of use. */
#define COM3 SERIAL_COM3_BASE
/** @brief Redefinition of serial COM4 base port ID for ease of use. */
#define COM4 SERIAL_COM4_BASE

/** @brief Defines the port that is used to print data. */
#define SERIAL_OUTPUT_PORT COM1

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

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/**
 * @brief Serial text driver instance.
 */
static console_driver_t uart_text_driver =
{
    .pClear           = uart_clear_screen,
    .pPutCursor       = NULL,
    .pSaveCursor      = NULL,
    .pRestoreCursor   = NULL,
    .pScroll          = uart_scroll,
    .pSetColorScheme  = NULL,
    .pSaveColorScheme = NULL,
    .pPutString       = uart_put_string,
    .pPutChar         = uart_put_char,
};

/** @brief Concurrency lock for the uart driver*/
static uint32_t uart_lock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Sets line parameters for the desired port.
 *
 * @details Sets line parameters for the desired port.
 *
 * @param[in] attr The settings for the port's line.
 * @param[in] com The port to set.
 */
static void _set_line(const uint8_t attr, const uint8_t com);

/**
 * @brief Sets buffer parameters for the desired port.
 *
 * @details Sets buffer parameters for the desired port.
 *
 * @param[in] attr The settings for the port's buffer.
 * @param[in] com The port to set.
 *
 * @return OS_NO_ERR on success, no other value is returned.
 */
static void _set_buffer(const uint8_t attr, const uint8_t com);

/**
 * @brief Sets the port's baudrate.
 *
 * @details Sets the port's baudrate.
 *
 * @param[in] rate The desired baudrate for the port.
 * @param[in] com The port to set.
 */
static void _set_baudrate(SERIAL_BAUDRATE_E rate, const uint8_t com);

/**
 * @brief Writes the data given as patameter on the desired port.
 *
 * @details The function will output the data given as parameter on the selected
 * port. This call is blocking until the data has been sent to the uart port
 * controler.
 *
 * @param[in] port The desired port to write the data to.
 * @param[in] data The byte to write to the uart port.
 */
static void _uart_write(const uint32_t port, const uint8_t data);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _set_line(const uint8_t attr, const uint8_t com)
{
    _cpu_outb(attr, SERIAL_LINE_COMMAND_PORT(com));

    KERNEL_DEBUG(SERIAL_DEBUG_ENABLED, MODULE_NAME,
                 "Set line attributes of port 0x%04x to %u",
                 com, attr);
}

static void _set_buffer(const uint8_t attr, const uint8_t com)
{
    _cpu_outb(attr, SERIAL_FIFO_COMMAND_PORT(com));

    KERNEL_DEBUG(SERIAL_DEBUG_ENABLED, MODULE_NAME,
                 "Set buffer attributes of port 0x%04x to %u",
                 com, attr);
}

static void _set_baudrate(SERIAL_BAUDRATE_E rate, const uint8_t com)
{
    _cpu_outb(SERIAL_DLAB_ENABLED, SERIAL_LINE_COMMAND_PORT(com));
    _cpu_outb((rate >> 8) & 0x00FF, SERIAL_DATA_PORT(com));
    _cpu_outb(rate & 0x00FF, SERIAL_DATA_PORT_2(com));

    KERNEL_DEBUG(SERIAL_DEBUG_ENABLED, MODULE_NAME,
                 "Set baud rate of port 0x%04x to %u",
                 com, rate);
}

static void _uart_write(const uint32_t port, const uint8_t data)
{
    /* Wait for empty transmit */
    KERNEL_SPINLOCK_LOCK(uart_lock);
    while((_cpu_inb(SERIAL_LINE_STATUS_PORT(port)) & 0x20) == 0){}
    if(data == '\n')
    {
        _cpu_outb('\r', port);
        _cpu_outb('\n', port);
    }
    else
    {
        _cpu_outb(data, port);
    }
    KERNEL_SPINLOCK_UNLOCK(uart_lock);
}


void uart_init(void)
{
    uint8_t i;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_UART_INIT_START, 0);

    /* Init all comm ports */
    for(i = 0; i < 4; ++i)
    {
        uint8_t  attr;
        uint32_t com;

        if(i == 0)
        {
            com = SERIAL_COM1_BASE;
        }
        else if(i == 1)
        {
            com = SERIAL_COM2_BASE;
        }
        else if(i == 2)
        {
            com = SERIAL_COM3_BASE;
        }
        else if(i == 3)
        {
            com = SERIAL_COM4_BASE;
        }
        else
        {
            com = SERIAL_COM1_BASE;
        }

        attr = SERIAL_DATA_LENGTH_8 | SERIAL_STOP_BIT_1;

        /* Enable interrupt on recv for COM1 and COM2 */
        if(com == SERIAL_COM1_BASE || com == SERIAL_COM2_BASE)
        {
            _cpu_outb(0x01, SERIAL_DATA_PORT_2(com));
        }
        else
        {
            _cpu_outb(0x00, SERIAL_DATA_PORT_2(com));
        }

        /* Init line */
        _set_baudrate(BAUDRATE_115200, com);
        _set_line(attr, com);
        _set_buffer(0xC0 | SERIAL_ENABLE_FIFO | SERIAL_CLEAR_RECV_FIFO |
                    SERIAL_CLEAR_SEND_FIFO | SERIAL_FIFO_DEPTH_14, com);

        /* Enable interrupt */
        _cpu_outb(0x0B, SERIAL_MODEM_COMMAND_PORT(com));
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_UART_INIT_END, 0);
    KERNEL_DEBUG(SERIAL_DEBUG_ENABLED, MODULE_NAME, "UART Initialized");
}

void uart_clear_screen(void)
{
    uint8_t i;

    /* On 80x25 screen, just print 25 line feed. */
    for(i = 0; i < 25; ++i)
    {
        _uart_write(SERIAL_OUTPUT_PORT, '\n');
    }
}

void uart_scroll(const SCROLL_DIRECTION_E direction,
                 const uint32_t lines_count)
{
    uint32_t i;

    if(direction == SCROLL_DOWN)
    {
        /* Just print lines_count line feed. */
        for(i = 0; i < lines_count; ++i)
        {
            _uart_write(SERIAL_OUTPUT_PORT, '\n');
        }
    }
}

uint8_t uart_read(const uint32_t port)
{
    uint8_t  val;

    /* Wait for data to be received */
    KERNEL_SPINLOCK_LOCK(uart_lock);

    while(uart_received(port) == 0){}

    /* Read available data on port */
    val = _cpu_inb(SERIAL_DATA_PORT(port));

    KERNEL_SPINLOCK_UNLOCK(uart_lock);

    return val;
}

void uart_put_string(const char* string)
{
    size_t i;
    for(i = 0; i < strlen(string); ++i)
    {
        _uart_write(SERIAL_OUTPUT_PORT, string[i]);
    }
}

void uart_put_char(const char character)
{
    _uart_write(SERIAL_OUTPUT_PORT, character);
}

bool_t uart_received(const uint32_t port)
{
    bool_t   ret;

    /* Read on LINE status port */
    ret = _cpu_inb(SERIAL_LINE_STATUS_PORT(port)) & 0x01;

    return ret;
}

const console_driver_t* uart_get_driver(void)
{
    return &uart_text_driver;
}


/************************************ EOF *************************************/