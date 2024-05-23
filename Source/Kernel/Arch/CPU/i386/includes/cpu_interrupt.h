/*******************************************************************************
 * @file cpu_interrupt.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 2.0
 *
 * @brief i386 interrupt manager.
 *
 * @details i386 interrupt manager. Stores the interrupt settings such as the
 * interrupt lines.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __I386_INTERRUPT_
#define __I386_INTERRUPT_

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

/** @brief Defines the number of possible interrupt on the i386 processor. */
#define INT_ENTRY_COUNT IDT_ENTRY_COUNT

/** @brief Minimal customizable accepted exception line. */
#define MIN_EXCEPTION_LINE 0x0
/** @brief Maximal customizable accepted exception line. */
#define MAX_EXCEPTION_LINE 0x1F

/** @brief Offset of the first line of an IRQ interrupt from IO-APIC. */
#define INT_IOAPIC_IRQ_OFFSET  0x40

/** @brief Keyboard IRQ number. */
#define KBD_IRQ_LINE              1
/** @brief Serial COM2-4 IRQ number. */
#define SERIAL_2_4_IRQ_LINE       3
/** @brief Serial COM1-3 IRQ number. */
#define SERIAL_1_3_IRQ_LINE       4
/** @brief RTC IRQ number. */
#define RTC_IRQ_LINE              8
/** @brief Mouse IRQ number. */
#define MOUSE_IRQ_LINE            12

/** @brief Divide by zero exception line. */
#define DIV_BY_ZERO_LINE           0x00
/** @brief Device not found exception line. */
#define DEVICE_NOT_FOUND_LINE      0x07
/** @brief Page fault exception line.*/
#define PAGE_FAULT_LINE            0x0E
/** @brief LAPIC Timer interrupt line. */
#define LAPIC_TIMER_INTERRUPT_LINE 0x20
/** @brief Scheduler software interrupt line. */
#define SCHEDULER_SW_INT_LINE      0x21
/** @brief Defines the panic interrupt line. */
#define PANIC_INT_LINE             0x2A
/** @brief Defines the sys call interrupt line. */
#define SYSCALL_INT_LINE           0x30

/** @brief LAPIC spurious interrupt vector. */
#define LAPIC_SPURIOUS_INT_LINE MAX_INTERRUPT_LINE

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

#endif /* #ifndef __I386_INTERRUPT_ */

/************************************ EOF *************************************/