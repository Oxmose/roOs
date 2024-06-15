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
#include <critical.h> /* Kernel critical sections */

/* Configuration files */
#include <config.h>

/* Header file */
#include <kerneloutput.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the maximal size of the buffer before sending for a print. */
#define KPRINTF_BUFFER_SIZE 256

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Output descriptor, used to define the handlers that manage outputs */
typedef struct
{
    /** @brief The handler used to print character. */
    void (*pPutc)(const char);
    /** @brief The handler used to print string. */
    void (*pPuts)(const char*);
} output_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Adds a padding sequence before a formated input.
 */
#define PAD_SEQ                         \
{                                       \
    strSize = strlen(tmpSeq);           \
                                        \
    while(paddingMod > strSize)         \
    {                                   \
        _toBufferChar(padCharMod);      \
        --paddingMod;                   \
    }                                   \
}

/**
 * @brief Get a sequence value argument.
 */
#define GET_SEQ_VAL(VAL, ARGS, LENGTH_MOD)                     \
{                                                              \
                                                               \
    /* Harmonize length */                                     \
    if(LENGTH_MOD > 8)                                         \
    {                                                          \
        LENGTH_MOD = 8;                                        \
    }                                                          \
                                                               \
    switch(LENGTH_MOD)                                         \
    {                                                          \
        case 1:                                                \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFF);   \
            break;                                             \
        case 2:                                                \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFFFF); \
            break;                                             \
        case 4:                                                \
            VAL = __builtin_va_arg(ARGS, uint32_t);            \
            break;                                             \
        case 8:                                                \
            VAL = __builtin_va_arg(ARGS, uint64_t);            \
            break;                                             \
        default:                                               \
            VAL = __builtin_va_arg(ARGS, uint32_t);            \
    }                                                          \
                                                               \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all lowercase character of a NULL terminated string to
 * uppercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static void _toUpper(char* pString);

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all uppercase character of a NULL terminated string to
 * lowercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static void _toLower(char* pString);

/**
 * @brief Prints a formated string.
 *
 * @details Prints a formated string to the output and managing the formated
 * string arguments.
 *
 * @param[in] kpStr The formated string to output.
 * @param[in] args The arguments to use with the formated string.
 */
static void _formater(const char* kpStr, __builtin_va_list args);

/**
 * @brief Prints the tag for kernel output functions.
 *
 * @details Prints the tag for kernel output functions.
 *
 * @param[in] kpStr The formated string to print.
 * @param[in] ... The associated arguments to the formated string.
 */
static void _tagPrintf(const char* kpStr, ...);

/**
 * @brief Sends a string to the printf buffer.
 *
 * @details Sends a string to the printf buffer. If the buffer is full, it
 * is flushed to the current output device. If the string contains a carriage
 * return, the buffer is flushed at this position.
 *
 * @param[in] kpString The string to push to the buffer.
*/
static inline void _toBufferStr(const char* kpString);

/**
 * @brief Sends a character to the printf buffer.
 *
 * @details Sends a character to the printf buffer. If the buffer is full, it
 * is flushed to the current output device. If the character is a carriage
 * return, the buffer is flushed at this position.
 *
 * @param[in] kCharacter The character to push to the buffer.
*/
static inline void _toBufferChar(const char kCharacter);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the current output type. */
static output_t sCurrentOutput = {
    .pPutc = consolePutChar,
    .pPuts = consolePutString
};

/** @brief Stores the current buffer size */
static size_t sBufferSize = 0;

/** @brief Stores the current buffer size */
static char spBuffer[KPRINTF_BUFFER_SIZE + 1];

/** @brief Kernel output lock */
static kernel_spinlock_t lock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _toUpper(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is lowercase, makes it uppercase */
        if(*pString > 96 && *pString < 123)
        {
            *pString = *pString - 32;
        }
        ++pString;
    }
}

static void _toLower(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is uppercase, makes it lowercase */
        if(*pString > 64 && *pString < 91)
        {
            *pString = *pString + 32;
        }
        ++pString;
    }
}

static void _formater(const char* kpStr, __builtin_va_list args)
{
    size_t   pos;
    size_t   strLength;
    uint64_t seqVal;
    size_t   strSize;
    uint8_t  modifier;
    uint8_t  lengthMod;
    uint8_t  paddingMod;
    bool_t   upperMod;
    char     padCharMod;
    char     tmpSeq[128];
    char*    pArgsValue;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_FORMATER_ENTRY,
                       2,
                       0,
                       (uint32_t)kpStr);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_FORMATER_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpStr >> 32),
                       (uint32_t)(uintptr_t)kpStr);
#endif

    modifier   = 0;
    lengthMod  = 4;
    paddingMod = 0;
    upperMod   = FALSE;
    padCharMod = ' ';
    strLength  = strlen(kpStr);

    for(pos = 0; pos < strLength; ++pos)
    {
        if(kpStr[pos] == '%')
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
                _toBufferChar(kpStr[pos]);
            }
        }
        else if(modifier)
        {
            switch(kpStr[pos])
            {
                /* Length mods */
                case 'h':
                    lengthMod /= 2;
                    continue;
                case 'l':
                    lengthMod *= 2;
                    continue;

                /* Specifier mods */
                case 's':
                    pArgsValue = __builtin_va_arg(args, char*);
                    _toBufferStr(pArgsValue);
                    break;
                case 'd':
                case 'i':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    itoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _toBufferStr(tmpSeq);
                    break;
                case 'u':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _toBufferStr(tmpSeq);
                    break;
                case 'X':
                    upperMod = TRUE;
                    __attribute__ ((fallthrough));
                case 'x':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == TRUE)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _toBufferStr(tmpSeq);
                    break;
                case 'P':
                    upperMod = TRUE;
                    __attribute__ ((fallthrough));
                case 'p':
                    paddingMod  = 2 * sizeof(uintptr_t);
                    padCharMod = '0';
                    lengthMod = sizeof(uintptr_t);
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == TRUE)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _toBufferStr(tmpSeq);
                    break;
                case 'c':
                    lengthMod = sizeof(char);
                    GET_SEQ_VAL(tmpSeq[0], args, lengthMod);
                    _toBufferChar(tmpSeq[0]);
                    break;

                /* Padding mods */
                case '0':
                    if(paddingMod == 0)
                    {
                        padCharMod = '0';
                    }
                    else
                    {
                        paddingMod *= 10;
                    }
                    continue;
                case '1':
                    paddingMod = paddingMod * 10 + 1;
                    continue;
                case '2':
                    paddingMod = paddingMod * 10 + 2;
                    continue;
                case '3':
                    paddingMod = paddingMod * 10 + 3;
                    continue;
                case '4':
                    paddingMod = paddingMod * 10 + 4;
                    continue;
                case '5':
                    paddingMod = paddingMod * 10 + 5;
                    continue;
                case '6':
                    paddingMod = paddingMod * 10 + 6;
                    continue;
                case '7':
                    paddingMod = paddingMod * 10 + 7;
                    continue;
                case '8':
                    paddingMod = paddingMod * 10 + 8;
                    continue;
                case '9':
                    paddingMod = paddingMod * 10 + 9;
                    continue;
                default:
                    continue;
            }
        }
        else
        {
            _toBufferChar(kpStr[pos]);
        }

        /* Reinit mods */
        lengthMod  = 4;
        paddingMod = 0;
        upperMod   = FALSE;
        padCharMod = ' ';
        modifier   = 0;
    }

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_FORMATER_EXIT,
                       2,
                       0,
                       (uint32_t)kpStr);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_FORMATER_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpStr >> 32),
                       (uint32_t)(uintptr_t)kpStr);
#endif
}

static void _tagPrintf(const char* kpFmt, ...)
{
    __builtin_va_list args;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TAGPRINTF_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TAGPRINTF_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_TAGPRINTF_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_TAGPRINTF_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }
    /* Prtinf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);
    kprintfFlush();

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TAGPRINTF_EXIT,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TAGPRINTF_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

static inline void _toBufferStr(const char* kpString)
{
    size_t newSize;
    size_t i;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_STR_ENTRY,
                       2,
                       0,
                       (uint32_t)kpString);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_STR_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpString >> 32),
                       (uint32_t)(uintptr_t)kpString);
#endif

    if(kpString == NULL)
    {
        return;
    }

    newSize = strlen(kpString);
    for(i = 0; i < newSize; ++i)
    {
        _toBufferChar(kpString[i]);
    }

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_STR_EXIT,
                       2,
                       0,
                       (uint32_t)kpString);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_STR_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpString >> 32),
                       (uint32_t)(uintptr_t)kpString);
#endif

}

static inline void _toBufferChar(const char kCharacter)
{
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_CHAR_ENTRY,
                       1,
                       kCharacter);
    /* Save until \n or size of reached */
    if(sBufferSize == KPRINTF_BUFFER_SIZE)
    {
        spBuffer[sBufferSize] = 0;
        sCurrentOutput.pPuts(spBuffer);
        sBufferSize = 0;
    }

    if(kCharacter == '\n')
    {
        spBuffer[sBufferSize++] = '\n';
        spBuffer[sBufferSize] = 0;
        sCurrentOutput.pPuts(spBuffer);
        sBufferSize = 0;
    }
    else
    {
        spBuffer[sBufferSize++] = kCharacter;
    }
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_TOBUFFER_CHAR_EXIT,
                       1,
                       kCharacter);
}

void kprintf(const char* kpFmt, ...)
{
    __builtin_va_list args;

    KERNEL_CRITICAL_LOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTF_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTF_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(lock);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTF_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTF_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }

    /* Prtinf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);

    KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTF_EXIT,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTF_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

void kprintfError(const char* kpFmt, ...)
{
    __builtin_va_list args;
    colorscheme_t     buffer;
    colorscheme_t     newScheme;

    KERNEL_CRITICAL_LOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFERROR_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFERROR_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(lock);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFERROR_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFERROR_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }

    newScheme.foreground = FG_RED;
    newScheme.background = BG_BLACK;
    newScheme.vgaColor   = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&newScheme);

    /* Print tag */
    _tagPrintf("[ERROR] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);

    KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFERROR_EXIT,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFERROR_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

void kprintfSuccess(const char* kpFmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        newScheme;

    KERNEL_CRITICAL_LOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFSUCCESS_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFSUCCESS_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFSUCCESS_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFSUCCESS_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }

    newScheme.foreground = FG_GREEN;
    newScheme.background = BG_BLACK;
    newScheme.vgaColor   = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&newScheme);

    /* Print tag */
    _tagPrintf("[OK] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);

    KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFSUCCESS_EXIT,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFSUCCESS_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

void kprintfInfo(const char* kpFmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        newScheme;

    KERNEL_CRITICAL_LOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFINFO_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFINFO_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFINFO_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFINFO_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }

    newScheme.foreground = FG_CYAN;
    newScheme.background = BG_BLACK;
    newScheme.vgaColor   = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&newScheme);

    /* Print tag */
    _tagPrintf("[INFO] ");

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);

    KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFINFO_EXIT,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFINFO_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

void kprintfDebug(const char* kpFmt, ...)
{
    __builtin_va_list    args;
    colorscheme_t        buffer;
    colorscheme_t        newScheme;
    uint64_t             uptime;

    KERNEL_CRITICAL_LOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFDEBUG_ENTRY,
                       2,
                       0,
                       (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFDEBUG_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif

    if(kpFmt == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFDEBUG_EXIT,
                           2,
                           0,
                           (uint32_t)kpFmt);
#else
        KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                           TRACE_KOUTPUT_KPRINTFDEBUG_EXIT,
                           2,
                           (uint32_t)((uintptr_t)kpFmt >> 32),
                           (uint32_t)(uintptr_t)kpFmt);
#endif
        return;
    }

    newScheme.foreground = FG_YELLOW;
    newScheme.background = BG_BLACK;
    newScheme.vgaColor   = TRUE;

    /* No need to test return value */
    consoleSaveColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    consoleSetColorScheme(&newScheme);

    /* Print tag */
    uptime = timeGetUptime();
    _tagPrintf("[DEBUG | %02llu.%03llu.%03llu.%03llu]",
               uptime / 1000000000,
               (uptime / 1000000) % 1000,
               (uptime / 1000) % 1000,
               uptime % 1000);

    /* Restore original screen color scheme */
    consoleSetColorScheme(&buffer);

    /* Printf format string */
    __builtin_va_start(args, kpFmt);
    _formater(kpFmt, args);
    __builtin_va_end(args);

    KERNEL_CRITICAL_UNLOCK(lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFDEBUG_EXIT,
                       2,
                       0,
                        (uint32_t)kpFmt);
#else
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFDEBUG_EXIT,
                       2,
                       (uint32_t)((uintptr_t)kpFmt >> 32),
                       (uint32_t)(uintptr_t)kpFmt);
#endif
}

void kprintfFlush(void)
{
    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFFLUSH_ENTRY,
                       0);

    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_TRACE_EVENT(TRACE_KOUTPUT_ENABLED,
                       TRACE_KOUTPUT_KPRINTFFLUSH_EXIT,
                       0);
}

/************************************ EOF *************************************/