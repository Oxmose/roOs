/*******************************************************************************
 * @file kerneloutput.c
 *
 * @see kerneloutput.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.1
 *
 * @brief Kernel's output methods.
 *
 * @details Simple output functions to print messages to screen. These are
 * really basic output too allow early kernel boot output and debug. These
 * functions can be used in interrupts handlers since no lock is required to use
 * them. This also makes them non thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <string.h>   /* memset, strlen */
#include <stdlib.h>   /* uitoa, itoa */
#include <console.h>  /* Console driver */
#include <time_mgt.h> /* System time management */

/* Configuration files */
#include <config.h>

/* Header file */
#include <kerneloutput.h>

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

/** @brief Output descriptor, used to define the handlers that manage outputs */
typedef struct
{
    /** @brief The handler used to print character. */
    void (*putc)(const char);
    /** @brief The handler used to print string. */
    void (*puts)(const char*);
} output_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Adds a padding sequence before a formated input.
 */
#define PAD_SEQ                         \
{                                       \
    str_size = strlen(tmp_seq);         \
                                        \
    while(padding_mod > str_size)       \
    {                                   \
        used_output.putc(pad_char_mod); \
        --padding_mod;                  \
    }                                   \
}

/**
 * @brief Get a sequence value argument.
 */
#define GET_SEQ_VAL(val, args, length_mod)                     \
{                                                              \
                                                               \
    /* Harmonize length */                                     \
    if(length_mod > 8)                                         \
    {                                                          \
        length_mod = 8;                                        \
    }                                                          \
                                                               \
    switch(length_mod)                                         \
    {                                                          \
        case 1:                                                \
            val = (__builtin_va_arg(args, uint32_t) & 0xFF);   \
            break;                                             \
        case 2:                                                \
            val = (__builtin_va_arg(args, uint32_t) & 0xFFFF); \
            break;                                             \
        case 4:                                                \
            val = __builtin_va_arg(args, uint32_t);            \
            break;                                             \
        case 8:                                                \
            val = __builtin_va_arg(args, uint64_t);            \
            break;                                             \
        default:                                               \
           val = __builtin_va_arg(args, uint32_t);             \
    }                                                          \
                                                               \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the current output type. */
static output_t current_output = {
    .putc = consolePutChar,
    .puts = consolePutString
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all lowercase character of a NULL terminated string to
 * uppercase characters.
 *
 * @param[in,out] string The string to tranform.
 */
static void _toupper(char* string);

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all uppercase character of a NULL terminated string to
 * lowercase characters.
 *
 * @param[in,out] string The string to tranform.
 */
static void _tolower(char* string);

/**
 * @brief Prints a formated string.
 *
 * @details Prints a formated string to the output and managing the formated
 * string arguments.
 *
 * @param[in] str The formated string to output.
 * @param[in] args The arguments to use with the formated string.
 * @param[in] used_output The output to use.
 */
static void _formater(const char* str,
                      __builtin_va_list args,
                      output_t used_output);

/**
 * @brief Prints a formated string.
 *
 * @details Prints a formated string to the output and managing the formated
 * string arguments.
 *
 * @param[in] str The formated string to output.
 * @param[in] args The arguments to use with the formated string.
 */
static void _kprint_fmt(const char* str, __builtin_va_list args);

/**
 * @brief Prints the tag for kernel output functions.
 *
 * @details Prints the tag for kernel output functions.
 *
 * @param[in] fmt The formated string to print.
 * @param[in] ... The associated arguments to the formated string.
 */
static void _tag_printf(const char* fmt, ...);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _toupper(char* string)
{
    /* For each character of the string */
    while(*string != 0)
    {
        /* If the character is lowercase, makes it uppercase */
        if(*string > 96 && *string < 123)
        {
            *string = *string - 32;
        }
        ++string;
    }
}

static void _tolower(char* string)
{
    /* For each character of the string */
    while(*string != 0)
    {
        /* If the character is uppercase, makes it lowercase */
        if(*string > 64 && *string < 91)
        {
            *string = *string + 32;
        }
        ++string;
    }
}

static void _formater(const char* str,
                      __builtin_va_list args,
                      output_t used_output)
{
    size_t   pos;
    size_t   str_length;
    uint64_t seq_val;
    size_t   str_size;


    uint8_t  modifier;

    uint8_t  length_mod;
    uint8_t  padding_mod;
    bool_t   upper_mod;
    char     pad_char_mod;

    char tmp_seq[128];

    char* args_value;

    modifier     = 0;
    length_mod   = 4;
    padding_mod  = 0;
    upper_mod    = FALSE;
    pad_char_mod = ' ';
    str_length   = strlen(str);

    for(pos = 0; pos < str_length; ++pos)
    {
        if(str[pos] == '%')
        {
            /* If we encouter this character in a modifier sequence, it was
             * just an escape one.
             */
            modifier = !modifier;
            if(modifier)
            {
                continue;
            }
            else
            {
                used_output.putc(str[pos]);
            }
        }
        else if(modifier)
        {
            switch(str[pos])
            {
                /* Length mods */
                case 'h':
                    length_mod /= 2;
                    continue;
                case 'l':
                    length_mod *= 2;
                    continue;

                /* Specifier mods */
                case 's':
                    args_value = __builtin_va_arg(args, char*);
                    used_output.puts(args_value);
                    break;
                case 'd':
                case 'i':
                    GET_SEQ_VAL(seq_val, args, length_mod);
                    memset(tmp_seq, 0, sizeof(tmp_seq));
                    itoa(seq_val, tmp_seq, 10);
                    PAD_SEQ
                    used_output.puts(tmp_seq);
                    break;
                case 'u':
                    GET_SEQ_VAL(seq_val, args, length_mod);
                    memset(tmp_seq, 0, sizeof(tmp_seq));
                    uitoa(seq_val, tmp_seq, 10);
                    PAD_SEQ
                    used_output.puts(tmp_seq);
                    break;
                case 'X':
                    upper_mod = TRUE;
                    __attribute__ ((fallthrough));
                case 'x':
                    GET_SEQ_VAL(seq_val, args, length_mod);
                    memset(tmp_seq, 0, sizeof(tmp_seq));
                    uitoa(seq_val, tmp_seq, 16);
                    PAD_SEQ
                    if(upper_mod == TRUE)
                    {
                        _toupper(tmp_seq);
                    }
                    else
                    {
                        _tolower(tmp_seq);
                    }
                    used_output.puts(tmp_seq);
                    break;
                case 'P':
                    upper_mod = TRUE;
                    __attribute__ ((fallthrough));
                case 'p':
                    padding_mod  = 2 * sizeof(uintptr_t);
                    pad_char_mod = '0';
                    length_mod = sizeof(uintptr_t);
                    GET_SEQ_VAL(seq_val, args, length_mod);
                    memset(tmp_seq, 0, sizeof(tmp_seq));
                    uitoa(seq_val, tmp_seq, 16);
                    PAD_SEQ
                    if(upper_mod == TRUE)
                    {
                        _toupper(tmp_seq);
                    }
                    else
                    {
                        _tolower(tmp_seq);
                    }
                    used_output.puts(tmp_seq);
                    break;
                case 'c':
                    length_mod = sizeof(char);
                    GET_SEQ_VAL(tmp_seq[0], args, length_mod);
                    used_output.putc(tmp_seq[0]);
                    break;

                /* Padding mods */
                case '0':
                    if(padding_mod == 0)
                    {
                        pad_char_mod = '0';
                    }
                    else
                    {
                        padding_mod *= 10;
                    }
                    continue;
                case '1':
                    padding_mod = padding_mod * 10 + 1;
                    continue;
                case '2':
                    padding_mod = padding_mod * 10 + 2;
                    continue;
                case '3':
                    padding_mod = padding_mod * 10 + 3;
                    continue;
                case '4':
                    padding_mod = padding_mod * 10 + 4;
                    continue;
                case '5':
                    padding_mod = padding_mod * 10 + 5;
                    continue;
                case '6':
                    padding_mod = padding_mod * 10 + 6;
                    continue;
                case '7':
                    padding_mod = padding_mod * 10 + 7;
                    continue;
                case '8':
                    padding_mod = padding_mod * 10 + 8;
                    continue;
                case '9':
                    padding_mod = padding_mod * 10 + 9;
                    continue;
                default:
                    continue;
            }
        }
        else
        {
            used_output.putc(str[pos]);
        }

        /* Reinit mods */
        length_mod   = 4;
        padding_mod  = 0;
        upper_mod    = FALSE;
        pad_char_mod = ' ';
        modifier     = 0;

    }
}

static void _kprint_fmt(const char* str, __builtin_va_list args)
{
    _formater(str, args, current_output);
}

static void _tag_printf(const char* fmt, ...)
{
    __builtin_va_list args;

    if(fmt == NULL)
    {
        return;
    }
    /* Prtinf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_printf(const char* fmt, ...)
{
    __builtin_va_list args;

    if(fmt == NULL)
    {
        return;
    }

    /* Prtinf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_error(const char* fmt, ...)
{
    __builtin_va_list args;
    colorscheme_t     buffer;
    colorscheme_t     new_scheme;

    if(fmt == NULL)
    {
        return;
    }

    new_scheme.foreground = FG_RED;
    new_scheme.background = BG_BLACK;
    new_scheme.vgaColor  = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&new_scheme);

    /* Print tag */
    _tag_printf("[ERROR] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_success(const char* fmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        new_scheme;

    if(fmt == NULL)
    {
        return;
    }

    new_scheme.foreground = FG_GREEN;
    new_scheme.background = BG_BLACK;
    new_scheme.vgaColor  = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&new_scheme);

    /* Print tag */
    _tag_printf("[OK] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_info(const char* fmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        new_scheme;

    if(fmt == NULL)
    {
        return;
    }

    new_scheme.foreground = FG_CYAN;
    new_scheme.background = BG_BLACK;
    new_scheme.vgaColor  = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&new_scheme);

    /* Print tag */
    _tag_printf("[INFO] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_warning(const char* fmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        new_scheme;

    if(fmt == NULL)
    {
        return;
    }

    new_scheme.foreground = FG_BROWN;
    new_scheme.background = BG_BLACK;
    new_scheme.vgaColor  = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&new_scheme);

    /* Print tag */
    _tag_printf("[WARNING] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_debug(const char* fmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        new_scheme;

    if(fmt == NULL)
    {
        return;
    }

    new_scheme.foreground = FG_YELLOW;
    new_scheme.background = BG_BLACK;
    new_scheme.vgaColor  = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&new_scheme);

    /* Print tag */
    _tag_printf("[DEBUG | %lu]", time_get_current_uptime());

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, fmt);
    _kprint_fmt(fmt, args);
    __builtin_va_end(args);
}

void kernel_doprint(const char* str, __builtin_va_list args)
{
    if(str == NULL)
    {
        return;
    }

    _kprint_fmt(str, args);
}

/************************************ EOF *************************************/