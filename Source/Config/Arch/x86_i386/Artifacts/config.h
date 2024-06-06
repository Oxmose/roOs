/*******************************************************************************
 * @file config.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/03/2023
 *
 * @version 1.0
 *
 * @brief X86 i386 global configuration file.
 *
 * @details X86 i386 global configuration file. This file contains all the
 * settings that the user can set before generating the kernel's binary.
 ******************************************************************************/

#ifndef __GLOBAL_CONFIG_H_
#define __GLOBAL_CONFIG_H_

/* Tracing feature */
#include <tracing.h>

/* Architecture definitions */
#define ARCH_I386    1
#define ARCH_32_BITS 1

/* Kernel memory offset
 * WARNING This value should be updated to fit other configuration files
 */
#define KERNEL_MEM_OFFSET 0xE0000000
#define KERNEL_MEM_START  0x00100000

/* Kernel stack default size
 * WARNING This value should be updated to fit other configuration files
 */
#define KERNEL_STACK_SIZE 0x1000

/* Maximal number of CPU supported by the architecture */
#define MAX_CPU_COUNT 4

/* Kernel log level */
#define KERNEL_LOG_LEVEL DEBUG_LOG_LEVEL

/* Kernel log on UART */
#define DEBUG_LOG_UART      1
#define DEBUG_LOG_UART_RATE BAUDRATE_115200

/* Defines the limit address allocable by the kernel */
#define KERNEL_VIRTUAL_ADDR_MAX      0x100000000
#define KERNEL_VIRTUAL_ADDR_MAX_MASK 0xFFFFFFFF

/** @brief Defines the maximum number of process in the system. This number is
 * limited by the PCID feature.*/
#define KERNEL_MAX_PROCESS 4096

/** @brief Current year */
#define CURRENT_YEAR 2024

/** @brief Stack default alignement */
#define STACK_ALIGN 4

/*******************************************************************************
 * DEBUG Configuration
 *
 * Set to 0 to disable debug output for a specific module
 * Set to 1 to enable debug output for a specific module
 ******************************************************************************/
#define CPU_DEBUG_ENABLED 0
#define EXCEPTIONS_DEBUG_ENABLED 0
#define INTERRUPTS_DEBUG_ENABLED 0
#define KHEAP_DEBUG_ENABLED 0
#define KICKSTART_DEBUG_ENABLED 0
#define PIC_DEBUG_ENABLED 0
#define PIT_DEBUG_ENABLED 0
#define QUEUE_DEBUG_ENABLED 0
#define KQUEUE_DEBUG_ENABLED 0
#define RTC_DEBUG_ENABLED 0
#define SCHED_DEBUG_ENABLED 0
#define SERIAL_DEBUG_ENABLED 0
#define TIME_MGT_DEBUG_ENABLED 0
#define VGA_DEBUG_ENABLED 0
#define DTB_DEBUG_ENABLED 0
#define DEVMGR_DEBUG_ENABLED 0
#define TSC_DEBUG_ENABLED 0
#define ACPI_DEBUG_ENABLED 0
#define IOAPIC_DEBUG_ENABLED 0
#define LAPIC_DEBUG_ENABLED 0
#define LAPICT_DEBUG_ENABLED 1

#endif /* #ifndef __GLOBAL_CONFIG_H_ */