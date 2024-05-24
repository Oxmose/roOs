/*******************************************************************************
 * @file console.h
 *
 * @see console.c
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

#ifndef __IO_CONSOLE_H_
#define __IO_CONSOLE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic int types and bool */
#include <stddef.h> /* Standard definition */
#include <kerror.h> /* Kernel error codes */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Scroll direction enumeration, enumerates all the possible scrolling
 * direction supported by the driver.
 */
typedef enum
{
    /** @brief Scroll down direction. */
    SCROLL_DOWN,
    /** @brief Scroll up direction. */
    SCROLL_UP
} SCROLL_DIRECTION_E;

/**
 * @brief Console cursor representation for the driver. The structures contains
 * the required data to keep track of the current cursor's position.
 */
typedef struct
{
    /** @brief The x position of the cursor. */
    uint32_t x;
    /** @brief The y position of the cursor. */
    uint32_t y;
} cursor_t;

/**
 * @brief Console color scheme representation. Keeps the different display
 * format such as background color in memory.
 */
typedef struct
{
    /** @brief The foreground color to be used when outputing data. */
    uint32_t foreground;
    /** @brief The background color to be used when outputing data. */
    uint32_t background;

    /**
     * @brief Set to 1 if using the VGA color designation for foreground and
     * background. If set to 0, then regular 32 bits RGBA designation is used.
     */
    bool_t vga_color;
} colorscheme_t;

/**
 * @brief The kernel's console driver abstraction.
 */
typedef struct
{
    /**
     * @brief Clears the console, the background color is set to black.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     */
    void (*pClear)(void* pDriverCtrl);

    /**
     * @brief Places the cursor to the coordinates given as parameters.
     *
     * @details The function places the console cursor at the desired coordinates
     * based on the line and column parameter.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kLine The line index where to place the cursor.
     * @param[in] kColumn The column index where to place the cursor.
     */
    void (*pPutCursor)(void* pDriverCtrl,
                       const uint32_t kLine,
                       const uint32_t kColumn);

    /**
     * @brief Saves the cursor attributes in the buffer given as paramter.
     *
     * @details Fills the buffer given s parameter with the current value of the
     * cursor.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[out] pBuffer The cursor buffer in which the current
     * cursor position is going to be saved.
     */
    void (*pSaveCursor)(void* pDriverCtrl, cursor_t* pBuffer);

    /**
     * @brief Restores the cursor attributes from the buffer given as parameter.
     *
     * @details The function will restores the cursor attributes from the buffer
     * given as parameter.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] pBuffer The buffer containing the cursor's
     * attributes.
     */
    void (*pRestoreCursor)(void* pDriverCtrl, const cursor_t* pBuffer);

    /**
     * @brief Scrolls in the desired direction of lines_count lines.
     *
     * @details The function will use the driver to scroll of lines_count line
     * in the desired direction.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kDirection The direction to which the
     * console should be scrolled.
     * @param[in] kLines The number of lines to scroll.
     */
    void (*pScroll)(void* pDriverCtrl,
                    const SCROLL_DIRECTION_E kDirection,
                    const uint32_t kLines);

    /**
     * @brief Sets the color scheme of the console.
     *
     * @details Replaces the curent color scheme used t output data with the new
     * one given as parameter.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kpColorScheme The new color scheme to apply
     * to the console.
     */
    void (*pSetColorScheme)(void* pDriverCtrl,
                            const colorscheme_t* kpColorScheme);

    /**
     * @brief Saves the color scheme in the buffer given as parameter.
     *
     * @details Fills the buffer given as parameter with the current console's
     * color scheme value.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[out] pBuffer The buffer that will receive the
     * current color scheme used by the console.
     */
    void (*pSaveColorScheme)(void* pDriverCtrl, colorscheme_t* pBuffer);

    /**
     * ­@brief Put a string to console.
     *
     * @details The function will display the string given as parameter to the
     * console using the selected driver.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kpString The string to display on the console.
     *
     * @warning kpString must be NULL terminated.
     */
    void (*pPutString)(void* pDriverCtrl, const char* kpString);

    /**
     * ­@brief Put a character to console.
     *
     * @details The function will display the character given as parameter to
     * the console using the selected driver.
     *
     * @param[in-out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kCharacter The char to display on the console.
     */
    void (*pPutChar)(void* pDriverCtrl, const char kCharacter);

    /** @brief Contains a pointer to the driver controler, set by the driver
     * at the moment of the initialization of this structure.
     */
    void* pDriverCtrl;
} console_driver_t;

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
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Sets the current selected driver.
 *
 * @details Changes the current selected driver to ouput data with the new one
 * as defined by the parameter driver.
 *
 * @param[in] console_driver_t* pDriver The driver to select.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E consoleSetDriver(const console_driver_t* pDriver);

/**
 * @brief Clears the console, the background color is set to black.
 */
void consoleClear(void);

/**
 * @brief Places the cursor to the coordinates given as parameters.
 *
 * @details The function places the console cursor at the desired coordinates
 * based on the line and column parameter.
 *
 * @param[in] kLine The line index where to place the cursor.
 * @param[in] kColumn The column index where to place the cursor.
 */
void consolePutCursor(const uint32_t kLine, const uint32_t kColumn);

/**
 * @brief Saves the cursor attributes in the buffer given as paramter.
 *
 * @details Fills the buffer given s parameter with the current value of the
 * cursor.
 *
 * @param[out] pBuffer The cursor buffer in which the current cursor
 * position is going to be saved.
 */
void consoleSaveCursor(cursor_t* pBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in] cursor_t* kpBuffer The buffer containing the cursor's attributes.
 */
void consoleRestoreCursor(const cursor_t* kpBuffer);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will use the driver to scroll of lines_count line in
 * the desired direction.
 *
 * @param[in] kDirection The direction to which the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
void consoleSroll(const SCROLL_DIRECTION_E kDirection, const uint32_t kLines);

/**
 * @brief Sets the color scheme of the console.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in] pkColorScheme The new color scheme to apply to
 * the console.
 */
void consoleSetColorScheme(const colorscheme_t* pkColorScheme);

/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current console's
 * color scheme value.
 *
 * @param[out] pBuffer The buffer that will receive the current
 * color scheme used by the console.
 */
void consoleSaveColorScheme(colorscheme_t* pBuffer);

/**
 * ­@brief Put a string to console.
 *
 * @details The function will display the string given as parameter to the
 * console using the selected driver.
 *
 * @param[in] kpString The string to display on the console.
 *
 * @warning kpString must be NULL terminated.
 */
void consolePutString(const char* kpString);

/**
 * ­@brief Put a character to console.
 *
 * @details The function will display the character given as parameter to the
 * console using the selected driver.
 *
 * @param[in] kCharacter The char to display on the console.
 */
void consolePutChar(const char kCharacter);

#endif /* #ifndef __IO_CONSOLE_H_ */

/************************************ EOF *************************************/