/*******************************************************************************
 * @file cpuInterrupt.h
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

/** @brief Defines the IPI interrupt line. */
#define IPI_INT_LINE 0x23

/** @brief Defines the division by zero exception line */
#define DIVISION_BY_ZERO_EXC_LINE 0x00
/** @brief Defines debug exception line */
#define DEBUG_EXC_LINE 0x01
/** @brief Defines NMI interrupt exception line */
#define NMI_INTERRUPT_EXC_LINE 0x02
/** @brief Defines breakpoint exception line */
#define BREAKPOINT_EXC_LINE 0x03
/** @brief Defines overflow exception line */
#define OVERFLOW_EXC_LINE 0x04
/** @brief Defines bound range exceeded exception line */
#define BOUND_RANGE_EXCEEDED_EXC_LINE 0x05
/** @brief Defines invalid instruction exception line */
#define INVALID_INSTRUCTION_EXC_LINE 0x06
/** @brief Defines device not available exception line */
#define DEVICE_NOT_AVAILABLE_EXC_LINE 0x07
/** @brief Defines double fault exception line */
#define DOUBLE_FAULT_EXC_LINE 0x08
/** @brief Defines coprocessor segment overrun exception line */
#define COPROC_SEGMENT_OVERRUN_EXC_LINE 0x09
/** @brief Defines invalid TSS exception line */
#define INVALID_TSS_EXC_LINE 0x0A
/** @brief Defines segment not present exception line */
#define SEGMENT_NOT_PRESENT_EXC_LINE 0x0B
/** @brief Defines stack segment fault exception line */
#define STACK_SEGMENT_FAULT_EXC_LINE 0x0C
/** @brief Defines general protection fault exception line */
#define GENERAL_PROTECTION_FAULT_EXC_LINE 0x0D
/** @brief Defines the page fault exception line */
#define PAGE_FAULT_EXC_LINE 0x0E
/** @brief Defines x87 floating point exception line */
#define X87_FLOATING_POINT_EXC_LINE 0x10
/** @brief Defines alignement check exception line */
#define ALIGNEMENT_CHECK_EXC_LINE 0x11
/** @brief Defines machine check exception line */
#define MACHINE_CHECK_EXC_LINE 0x12
/** @brief Defines SIMD floating point exception line */
#define SIMD_FLOATING_POINT_EXC_LINE 0x13
/** @brief Defines virtualization exception line */
#define VIRTUALIZATION_EXC_LINE 0x14
/** @brief Defines control protection exception line */
#define CONTROL_PROTECTION_EXC_LINE 0x15
/** @brief Defines hypervisor injection exception line */
#define HYPERVISOR_INJECTION_EXC_LINE 0x1C
/** @brief Defines VMM communication exception line */
#define VMM_COMMUNICATION_EXC_LINE 0x1D
/** @brief Defines security exception line */
#define SECURITY_EXC_LINE 0x1E



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