/*******************************************************************************
 * @file vgatext.h
 *
 * @see vgatext.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 2.0
 *
 * @brief VGA text mode driver.
 *
 * @details Allows the kernel to display text and general ASCII characters to be
 * displayed on the screen. Includes cursor management, screen colors management
 * and other fancy screen driver things.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_VGA_TEXT_H_
#define __X86_VGA_TEXT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief VGA background color definition: black. */
#define BG_BLACK            0x00
/** @brief VGA background color definition: blue. */
#define BG_BLUE             0x10
/** @brief VGA background color definition: green. */
#define BG_GREEN            0x20
/** @brief VGA background color definition: cyan. */
#define BG_CYAN             0x30
/** @brief VGA background color definition: red. */
#define BG_RED              0x40
/** @brief VGA background color definition: magenta. */
#define BG_MAGENTA          0x50
/** @brief VGA background color definition: brown. */
#define BG_BROWN            0x60
/** @brief VGA background color definition: grey. */
#define BG_GREY             0x70
/** @brief VGA background color definition: dark grey. */
#define BG_DARKGREY         0x80
/** @brief VGA background color definition: bright blue. */
#define BG_BRIGHTBLUE       0x90
/** @brief VGA background color definition: bright green. */
#define BG_BRIGHTGREEN      0xA0
/** @brief VGA background color definition: bright cyan. */
#define BG_BRIGHTCYAN       0xB0
/** @brief VGA background color definition: bright red. */
#define BG_BRIGHTRED        0xC0
/** @brief VGA background color definition: bright magenta. */
#define BG_BRIGHTMAGENTA    0xD0
/** @brief VGA background color definition: yellow. */
#define BG_YELLOW           0xE0
/** @brief VGA background color definition: white. */
#define BG_WHITE            0xF0

/** @brief VGA foreground color definition: black. */
#define FG_BLACK            0x00
/** @brief VGA foreground color definition: blue. */
#define FG_BLUE             0x01
/** @brief VGA foreground color definition: green. */
#define FG_GREEN            0x02
/** @brief VGA foreground color definition: cyan. */
#define FG_CYAN             0x03
/** @brief VGA foreground color definition: red. */
#define FG_RED              0x04
/** @brief VGA foreground color definition: magenta. */
#define FG_MAGENTA          0x05
/** @brief VGA foreground color definition: brown. */
#define FG_BROWN            0x06
/** @brief VGA foreground color definition: grey. */
#define FG_GREY             0x07
/** @brief VGA foreground color definition: dark grey. */
#define FG_DARKGREY         0x08
/** @brief VGA foreground color definition: bright blue. */
#define FG_BRIGHTBLUE       0x09
/** @brief VGA foreground color definition: bright green. */
#define FG_BRIGHTGREEN      0x0A
/** @brief VGA foreground color definition: bright cyan. */
#define FG_BRIGHTCYAN       0x0B
/** @brief VGA foreground color definition: bright red. */
#define FG_BRIGHTRED        0x0C
/** @brief VGA foreground color definition: bright magenta. */
#define FG_BRIGHTMAGENTA    0x0D
/** @brief VGA foreground color definition: yellow. */
#define FG_YELLOW           0x0E
/** @brief VGA foreground color definition: white. */
#define FG_WHITE            0x0F

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
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* None */

#endif /* #ifndef __X86_VGA_TEXT_H_ */

/************************************ EOF *************************************/
