/*******************************************************************************
 * @file cpu_interruptWS.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 2.0
 *
 * @brief x86_64 interrupt manager.
 *
 * @details x86_64 interrupt manager. Stores the interrupt settings such as the
 * interrupt lines.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_64_INTERRUPT_
#define __X86_64_INTERRUPT_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Number of entries in the kernel's IDT. */
#define IDT_ENTRY_COUNT 256

/** @brief Minimal customizable accepted interrupt line. */
#define MIN_INTERRUPT_LINE 0x20
/** @brief Maximal customizable accepted interrupt line. */
#define MAX_INTERRUPT_LINE (IDT_ENTRY_COUNT - 1)

/** @brief Defines the number of possible interrupt on the x86_64 processor. */
#define INT_ENTRY_COUNT IDT_ENTRY_COUNT

/** @brief Minimal customizable accepted exception line. */
#define MIN_EXCEPTION_LINE 0x0
/** @brief Maximal customizable accepted exception line. */
#define MAX_EXCEPTION_LINE 0x1F

/** @brief Defines the panic interrupt line. */
#define PANIC_INT_LINE 0x20

/** @brief Defines the spurious interrupt line */
#define SPURIOUS_INT_LINE MAX_INTERRUPT_LINE

/** @brief Defines the software interrupt number for scheduling. */
#define SCHEDULER_SW_INT_LINE 0x22


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

#endif /* #ifndef __X86_64_INTERRUPT_ */

/************************************ EOF *************************************/