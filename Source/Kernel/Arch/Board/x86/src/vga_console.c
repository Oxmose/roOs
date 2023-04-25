/*******************************************************************************
 * @file vga_console.c
 *
 * @see vga_console.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/02/2021
 *
 * @version 1.5
 *
 * @brief VGA text mode driver.
 *
 * @details Allows the kernel to display text and general ASCII characters to be
 * displayed on the screen. Includes cursor management, screen colors management
 * and other fancy screen driver things.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h>         /* Generic int types */
#include <string.h>         /* String manipualtion */
#include <cpu.h>            /* CPU port manipulation */
#include <kerror.h>         /* Kernel error */
#include <kernel_output.h>  /* Kernel output manager */
#include <console.h>        /* Console driver manager */
#include <uart.h>           /* UART driver */
/* Configuration files */
#include <config.h>

/* Header file */
#include <vga_console.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define MODULE_NAME "X86 VGA TEXT"

/** @brief VGA frame buffer base physical address. */
#define VGA_CONSOLE_FRAMEBUFFER 0xB8000

/** @brief VGA frame buffer size */
#define VGA_CONSOLE_FRAMEBUFFER_SIZE 0x7D00

/** @brief VGA CPU management data port. */
#define VGA_CONSOLE_SCREEN_DATA_PORT 0x3D5
/** @brief VGA CPU management command port. */
#define VGA_CONSOLE_SCREEN_COMM_PORT 0x3D4
/** @brief VGA screen width. */
#define VGA_CONSOLE_SCREEN_COL_SIZE  80
/** @brief VGA screen height. */
#define VGA_CONSOLE_SCREEN_LINE_SIZE 25

/** @brief VGA cursor position command low. */
#define VGA_CONSOLE_CURSOR_COMM_LOW  0x0F
/** @brief VGA cursor position command high. */
#define VGA_CONSOLE_CURSOR_COMM_HIGH 0x0E

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

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
/* Screen runtime parameters */
/** @brief Stores the curent screen's color scheme. */
static colorscheme_t screen_scheme = {
    .background = BG_BLACK,
    .foreground = FG_WHITE
};

/** @brief Stores the curent screen's cursor settings. */
static cursor_t screen_cursor;
/**
 * @brief Stores the curent screen's cursor settings ofthe last printed
 * character.
 */
static cursor_t last_printed_cursor;

/* Set the last column printed with a char */
/**
 * @brief Stores the column index of the last printed character for each lines
 * of the screen.
 */
static uint32_t last_columns[VGA_CONSOLE_SCREEN_LINE_SIZE] = {0};

/** @brief VGA frame buffer address. */
static uint16_t* vga_console_framebuffer = (uint16_t*)VGA_CONSOLE_FRAMEBUFFER;

/**
 * @brief VGA text driver instance.
 */
static kernel_console_driver_t vga_console_driver = {
    .clear_screen           = vga_console_clear_screen,
    .put_cursor_at          = vga_console_put_cursor_at,
    .save_cursor            = vga_console_save_cursor,
    .restore_cursor         = vga_console_restore_cursor,
    .scroll                 = vga_console_scroll,
    .set_color_scheme       = vga_console_set_color_scheme,
    .save_color_scheme      = vga_console_save_color_scheme,
    .put_string             = vga_console_put_string,
    .put_char               = vga_console_put_char,
    .console_write_keyboard = vga_console_write_keyboard
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Prints a character to the selected coordinates.
 *
 * @details Prints a character to the selected coordinates by setting the memory
 * accordingly.
 *
 * @param[in] line The line index where to write the character.
 * @param[in] column The colums index where to write the character.
 * @param[in] character The character to display on the screem.
 */
inline static void _vga_console_print_char(const uint32_t line,
                                           const uint32_t column,
                                           const char character);

/**
 * @brief Returns the VGA frame buffer virtual address.
 *
 * @details Returns the VGA frame buffer virtual address correponding to a
 * certain region of the buffer given the parameters.
 *
 * @param[in] line The frame buffer line.
 * @param[in] column The frame buffer column.
 *
 * @return The frame buffer virtual address is returned correponding to a
 * certain region of the buffer given the parameters is returned.
 */
inline static uint16_t* _vga_console_get_framebuffer(const uint32_t line,
                                                     const uint32_t column);

/**
 * @brief Processes the character in parameters.
 *
 * @details Check the character nature and code. Corresponding to the
 * character's code, an action is taken. A regular character will be printed
 * whereas \n will create a line feed.
 *
 * @param[in] character The character to process.
 */
static void _vga_console_process_char(const char character);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

inline static void _vga_console_print_char(const uint32_t line,
                                           const uint32_t column,
                                           const char character)
{
    uint16_t* screen_mem;

    if(line > VGA_CONSOLE_SCREEN_LINE_SIZE - 1 ||
       column > VGA_CONSOLE_SCREEN_COL_SIZE - 1)
    {
        return;
    }

    /* Get address to inject */
    screen_mem = _vga_console_get_framebuffer(line, column);

    /* Inject the character with the current colorscheme */
    *screen_mem = character |
                  ((screen_scheme.background << 8) & 0xF000) |
                  ((screen_scheme.foreground << 8) & 0x0F00);
}

static void _vga_console_process_char(const char character)
{
#if DEBUG_LOG_UART
    uart_put_char(character);
#endif

    /* If character is a normal ASCII character */
    if(character > 31 && character < 127)
    {
         /* Manage end of line cursor position */
        if(screen_cursor.x > VGA_CONSOLE_SCREEN_COL_SIZE - 1)
        {
            vga_console_put_cursor_at(screen_cursor.y + 1, 0);
            last_columns[screen_cursor.y] = screen_cursor.x;
        }

        /* Manage end of screen cursor position */
        if(screen_cursor.y >= VGA_CONSOLE_SCREEN_LINE_SIZE)
        {
            vga_console_scroll(SCROLL_DOWN, 1);

        }
        else
        {
            /* Move cursor */
            vga_console_put_cursor_at(screen_cursor.y, screen_cursor.x);
            last_columns[screen_cursor.y] = screen_cursor.x;
        }

        /* Display character and move cursor */
        _vga_console_print_char(screen_cursor.y, screen_cursor.x++,
                                character);
    }
    else
    {
        /* Manage special ACSII characters*/
        switch(character)
        {
            /* Backspace */
            case '\b':
                if(last_printed_cursor.y == screen_cursor.y)
                {
                    if(screen_cursor.x > last_printed_cursor.x)
                    {
                        vga_console_put_cursor_at(screen_cursor.y,
                                                  screen_cursor.x - 1);
                        last_columns[screen_cursor.y] = screen_cursor.x;
                        _vga_console_print_char(screen_cursor.y,
                                                screen_cursor.x, ' ');
                    }
                }
                else if(last_printed_cursor.y < screen_cursor.y)
                {
                    if(screen_cursor.x > 0)
                    {
                        vga_console_put_cursor_at(screen_cursor.y,
                                                  screen_cursor.x - 1);
                            last_columns[screen_cursor.y] = screen_cursor.x;
                        _vga_console_print_char(screen_cursor.y,
                                                screen_cursor.x, ' ');
                    }
                    else
                    {
                        if(last_columns[screen_cursor.y - 1] >=
                           VGA_CONSOLE_SCREEN_COL_SIZE)
                        {
                            last_columns[screen_cursor.y - 1] =
                               VGA_CONSOLE_SCREEN_COL_SIZE - 1;
                        }

                        vga_console_put_cursor_at(screen_cursor.y - 1,
                                      last_columns[screen_cursor.y - 1]);
                        _vga_console_print_char(screen_cursor.y,
                                                screen_cursor.x, ' ');
                    }
                }
                break;
            /* Tab */
            case '\t':
                if(screen_cursor.x + 8 < VGA_CONSOLE_SCREEN_COL_SIZE - 1)
                {
                    vga_console_put_cursor_at(screen_cursor.y,
                            screen_cursor.x  +
                            (8 - screen_cursor.x % 8));
                }
                else
                {
                    vga_console_put_cursor_at(screen_cursor.y,
                           VGA_CONSOLE_SCREEN_COL_SIZE - 1);
                }
                last_columns[screen_cursor.y] = screen_cursor.x;
                break;
            /* Line feed */
            case '\n':
                if(screen_cursor.y < VGA_CONSOLE_SCREEN_LINE_SIZE - 1)
                {
                    vga_console_put_cursor_at(screen_cursor.y + 1, 0);
                    last_columns[screen_cursor.y] = screen_cursor.x;
                }
                else
                {
                    vga_console_scroll(SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                vga_console_clear_screen();
                break;
            /* Line return */
            case '\r':
                vga_console_put_cursor_at(screen_cursor.y, 0);
                last_columns[screen_cursor.y] = screen_cursor.x;
                break;
            /* Undefined */
            default:
                break;
        }
    }
}

static inline uint16_t* _vga_console_get_framebuffer(const uint32_t line,
                                                     const uint32_t column)
{
    /* Avoid overflow on text mode */
    if(line > VGA_CONSOLE_SCREEN_LINE_SIZE - 1 ||
       column > VGA_CONSOLE_SCREEN_COL_SIZE -1)
    {
        return vga_console_framebuffer;
    }

    /* Returns the mem adress of the coordinates */
    return (uint16_t*)vga_console_framebuffer +
           (column + line * VGA_CONSOLE_SCREEN_COL_SIZE);
}

void vga_console_init(void)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_VGA_INIT_START, 0);

    /* Init framebuffer */
    vga_console_framebuffer = (uint16_t*)VGA_CONSOLE_FRAMEBUFFER;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_VGA_INIT_END, 3,
                       (uintptr_t)vga_console_framebuffer & 0xFFFFFFFF,
#ifdef ARCH_64_BITS
                       (uintptr_t)vga_console_framebuffer >> 32,
#else
                        0,
#endif
                       VGA_CONSOLE_FRAMEBUFFER_SIZE);

    KERNEL_DEBUG(VGA_DEBUG_ENABLED, MODULE_NAME, "VGA text driver initialized");
}

void vga_console_clear_screen(void)
{
    /* Clear all screen */
    memset(_vga_console_get_framebuffer(0, 0), 0, VGA_CONSOLE_FRAMEBUFFER_SIZE);
}

void vga_console_put_cursor_at(const uint32_t line, const uint32_t column)
{
    int16_t  cursor_position;

    /* Checks the values of line and column */
    if(column > VGA_CONSOLE_SCREEN_COL_SIZE ||
       line > VGA_CONSOLE_SCREEN_LINE_SIZE)
    {
        return;
    }

    /* Set new cursor position */
    screen_cursor.x = column;
    screen_cursor.y = line;

    /* Display new position on screen */
    cursor_position = column + line * VGA_CONSOLE_SCREEN_COL_SIZE;

    /* Send low part to the screen */
    _cpu_outb(VGA_CONSOLE_CURSOR_COMM_LOW, VGA_CONSOLE_SCREEN_COMM_PORT);
    _cpu_outb((int8_t)(cursor_position & 0x00FF), VGA_CONSOLE_SCREEN_DATA_PORT);

    /* Send high part to the screen */
    _cpu_outb(VGA_CONSOLE_CURSOR_COMM_HIGH, VGA_CONSOLE_SCREEN_COMM_PORT);
    _cpu_outb((int8_t)((cursor_position & 0xFF00) >> 8),
             VGA_CONSOLE_SCREEN_DATA_PORT);
}

void vga_console_save_cursor(cursor_t* buffer)
{
    if(buffer != NULL)
    {
        /* Save cursor attributes */
        buffer->x = screen_cursor.x;
        buffer->y = screen_cursor.y;
    }
}

void vga_console_restore_cursor(const cursor_t buffer)
{
    if(buffer.x >= VGA_CONSOLE_SCREEN_COL_SIZE ||
       buffer.y >= VGA_CONSOLE_SCREEN_LINE_SIZE)
    {
        return;
    }
    /* Restore cursor attributes */
    vga_console_put_cursor_at(buffer.y, buffer.x);
}

void vga_console_scroll(const SCROLL_DIRECTION_E direction, const uint32_t lines_count)
{
    uint32_t to_scroll;

    if(VGA_CONSOLE_SCREEN_LINE_SIZE < lines_count)
    {
        to_scroll = VGA_CONSOLE_SCREEN_LINE_SIZE;
    }
    else
    {
        to_scroll = lines_count;
    }

    /* Select scroll direction */
    if(direction == SCROLL_DOWN)
    {
        uint32_t i;
        uint32_t j;


        /* For each line scroll we want */
        for(j = 0; j < to_scroll; ++j)
        {
            /* Copy all the lines to the above one */
            for(i = 0; i < VGA_CONSOLE_SCREEN_LINE_SIZE - 1; ++i)
            {
                memmove(_vga_console_get_framebuffer(i, 0),
                        _vga_console_get_framebuffer(i + 1, 0),
                        sizeof(uint16_t) * VGA_CONSOLE_SCREEN_COL_SIZE);
                last_columns[i] = last_columns[i+1];
            }
            last_columns[VGA_CONSOLE_SCREEN_LINE_SIZE - 1] = 0;
        }
        /* Clear last line */
        for(i = 0; i < VGA_CONSOLE_SCREEN_COL_SIZE; ++i)
        {
            _vga_console_print_char(VGA_CONSOLE_SCREEN_LINE_SIZE - 1, i, ' ');
        }

    }

    /* Replace cursor */
    vga_console_put_cursor_at(VGA_CONSOLE_SCREEN_LINE_SIZE - to_scroll, 0);

    if(to_scroll <= last_printed_cursor.y)
    {
        last_printed_cursor.y -= to_scroll;
    }
    else
    {
        last_printed_cursor.x = 0;
        last_printed_cursor.y = 0;
    }
}

void vga_console_set_color_scheme(const colorscheme_t color_scheme)
{
    screen_scheme.foreground = color_scheme.foreground;
    screen_scheme.background = color_scheme.background;
}

void vga_console_save_color_scheme(colorscheme_t* buffer)
{
    if(buffer != NULL)
    {
        /* Save color scheme into buffer */
        buffer->foreground = screen_scheme.foreground;
        buffer->background = screen_scheme.background;
    }
}

void vga_console_put_string(const char* string)
{
    size_t i;

    /* Output each character of the string */
    for(i = 0; i < strlen(string); ++i)
    {
        vga_console_put_char(string[i]);
    }
}

void vga_console_put_char(const char character)
{
    _vga_console_process_char(character);
    last_printed_cursor = screen_cursor;
}

void vga_console_write_keyboard(const char* string, const size_t size)
{
    size_t i;

    /* Output each character of the string */
    for(i = 0; i < size; ++i)
    {
        _vga_console_process_char(string[i]);
    }
}

const kernel_console_driver_t* vga_console_get_driver(void)
{
    return &vga_console_driver;
}

/************************************ EOF *************************************/