/*******************************************************************************
 * @file console.c
 *
 * @see console.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.0
 *
 * @brief Console drivers abtraction.
 *
 * @details Console driver abtraction layer. The functions of this module allows
 * to abtract the use of any supported console driver and the selection of the
 * desired driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h> /* Generic int types */
#include <kerror.h> /* Kernel error codes */

/* Configuration files */
#include <config.h>

/* Header file */
#include <console.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

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
/** @brief Stores the currently selected driver */
static kernel_console_driver_t console_driver = {NULL};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

OS_RETURN_E console_set_selected_driver(const kernel_console_driver_t* driver)
{
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_START, 2,
                       (uintptr_t)driver & 0xFFFFFFFF,
                       (uintptr_t)driver >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_START, 2,
                       irq_number,
                       driver,
                       0);
#endif

    if(driver == NULL ||
       driver->clear_screen == NULL ||
       driver->put_string == NULL ||
       driver->put_char == NULL ||
       driver->console_write_keyboard == NULL)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_END, 1,
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    console_driver = *driver;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_CONSOLE_SET_DRIVER_END, 1,
                       OS_NO_ERR);
    return OS_NO_ERR;
}

void console_clear_screen(void)
{
    if(console_driver.clear_screen != NULL)
    {
        console_driver.clear_screen();
    }
}

void console_put_cursor_at(const uint32_t line, const uint32_t column)
{
    if(console_driver.put_cursor_at != NULL)
    {
        console_driver.put_cursor_at(line, column);
    }
}

void console_save_cursor(cursor_t* buffer)
{
    if(console_driver.save_cursor != NULL)
    {
        return console_driver.save_cursor(buffer);
    }
}

void console_restore_cursor(const cursor_t buffer)
{
    if(console_driver.restore_cursor != NULL)
    {
        return console_driver.restore_cursor(buffer);
    }
}

void console_scroll(const SCROLL_DIRECTION_E direction,
                    const uint32_t lines_count)
{
    if(console_driver.scroll != NULL)
    {
        console_driver.scroll(direction, lines_count);
    }
}

void console_set_color_scheme(colorscheme_t color_scheme)
{
    if(console_driver.set_color_scheme != NULL)
    {
        console_driver.set_color_scheme(color_scheme);
    }
}

void console_save_color_scheme(colorscheme_t* buffer)
{
    if(console_driver.save_color_scheme != NULL)
    {
        return console_driver.save_color_scheme(buffer);
    }
}

void console_put_string(const char* str)
{
    if(console_driver.put_string != NULL)
    {
        console_driver.put_string(str);
    }
}

void console_put_char(const char character)
{
    if(console_driver.put_char != NULL)
    {
        console_driver.put_char(character);
    }
}

void console_console_write_keyboard(const char* str, const size_t len)
{
    if(console_driver.console_write_keyboard != NULL)
    {
        console_driver.console_write_keyboard(str, len);
    }
}

/************************************ EOF *************************************/