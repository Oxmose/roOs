/*******************************************************************************
 * @file cpu.c
 *
 * @see cpu.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief x64 CPU management functions
 *
 * @details x64 CPU manipulation functions. Wraps inline assembly calls for
 * ease of development.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>         /* Generic int types */
#include <stddef.h>         /* Standard definition */
#include <string.h>         /* Memory manipulation */
#include <kernel_output.h>  /* Kernel output */

/* Header file */
#include <cpu.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "CPU_X64"

/** @brief Kernel's 32 bits code segment descriptor. */
#define KERNEL_CS_32 0x08
/** @brief Kernel's 32 bits data segment descriptor. */
#define KERNEL_DS_32 0x10
/** @brief Kernel's 16 bits ode segment descriptor. */
#define KERNEL_CS_16 0x18
/** @brief Kernel's 16 bits data segment descriptor. */
#define KERNEL_DS_16 0x20
/** @brief Kernel's 64 bits ode segment descriptor. */
#define KERNEL_CS_64 0x28
/** @brief Kernel's 64 bits data segment descriptor. */
#define KERNEL_DS_64 0x30

/** @brief User's 32 bits code segment descriptor. */
#define USER_CS_32 0x38
/** @brief User's 32 bits data segment descriptor. */
#define USER_DS_32 0x40
/** @brief User's 64 bits code segment descriptor. */
#define USER_CS_64 0x48
/** @brief User's 64 bits data segment descriptor. */
#define USER_DS_64 0x50

/** @brief Kernel's TSS segment descriptor. */
#define TSS_SEGMENT  0x60

/** @brief Select the thread code segment. */
#define THREAD_KERNEL_CS KERNEL_CS_64
/** @brief Select the thread code segment. */
#define THREAD_KERNEL_DS KERNEL_DS_64

/** @brief Number of entries in the kernel's IDT. */
#define IDT_ENTRY_COUNT 256

/** @brief Kernel's 64 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief Kernel's 64 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief Kernel's 32 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_32  0x00000000
/** @brief Kernel's 32 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_32 0x000FFFFF
/** @brief Kernel's 32 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_32  0x00000000
/** @brief Kernel's 32 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_32 0x000FFFFF

/** @brief Kernel's 16 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_16  0x00000000
/** @brief Kernel's 16 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_16 0x000FFFFF
/** @brief Kernel's 16 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_16  0x00000000
/** @brief Kernel's 16 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_16 0x000FFFFF

/** @brief User's 64 bits code segment base address. */
#define USER_CODE_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits code segment limit address. */
#define USER_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief User's 64 bits data segment base address. */
#define USER_DATA_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits data segment limit address. */
#define USER_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief User's 32 bits code segment base address. */
#define USER_CODE_SEGMENT_BASE_32  0x00000000
/** @brief User's 32 bits code segment limit address. */
#define USER_CODE_SEGMENT_LIMIT_32 0x000FFFFF
/** @brief User's 32 bits data segment base address. */
#define USER_DATA_SEGMENT_BASE_32  0x00000000
/** @brief User's 32 bits data segment limit address. */
#define USER_DATA_SEGMENT_LIMIT_32 0x000FFFFF

/***************************
 * GDT Flags
 **************************/

/** @brief GDT granularity flag: 4K block. */
#define GDT_FLAG_GRANULARITY_4K   0x800000
/** @brief GDT granularity flag: 1B block. */
#define GDT_FLAG_GRANULARITY_BYTE 0x000000
/** @brief GDT size flag: 16b protected mode. */
#define GDT_FLAG_16_BIT_SEGMENT   0x000000
/** @brief GDT size flag: 32b protected mode. */
#define GDT_FLAG_32_BIT_SEGMENT   0x400000
/** @brief GDT size flag: 64b protected mode. */
#define GDT_FLAG_64_BIT_SEGMENT   0x200000
/** @brief GDT AVL flag. */
#define GDT_FLAG_AVL              0x100000
/** @brief GDT segment present flag. */
#define GDT_FLAG_SEGMENT_PRESENT  0x008000
/** @brief GDT privilege level flag: Ring 0 (kernel). */
#define GDT_FLAG_PL0              0x000000
/** @brief GDT privilege level flag: Ring 1 (kernel-). */
#define GDT_FLAG_PL1              0x002000
/** @brief GDT privilege level flag: Ring 2 (kernel--). */
#define GDT_FLAG_PL2              0x004000
/** @brief GDT privilege level flag: Ring 3 (user). */
#define GDT_FLAG_PL3              0x006000
/** @brief GDT data type flag: code. */
#define GDT_FLAG_CODE_TYPE        0x001000
/** @brief GDT data type flag: data. */
#define GDT_FLAG_DATA_TYPE        0x001000
/** @brief GDT data type flag: system. */
#define GDT_FLAG_SYSTEM_TYPE      0x000000
/** @brief GDT TSS flag. */
#define GDT_FLAG_TSS              0x09

/** @brief GDT access byte flag: executable. */
#define GDT_TYPE_EXECUTABLE       0x8
/** @brief GDT access byte flag: growth direction up. */
#define GDT_TYPE_GROW_UP          0x4
/** @brief GDT access byte flag: growth direction down. */
#define GDT_TYPE_GROW_DOWN        0x0
/** @brief GDT access byte flag: conforming code. */
#define GDT_TYPE_CONFORMING       0x4
/** @brief GDT access byte flag: protected. */
#define GDT_TYPE_PROTECTED        0x0
/** @brief GDT access byte flag: readable. */
#define GDT_TYPE_READABLE         0x2
/** @brief GDT access byte flag: writable. */
#define GDT_TYPE_WRITABLE         0x2
/** @brief GDT access byte flag: accessed byte. */
#define GDT_TYPE_ACCESSED         0x1


/***************************
 * IDT Flags
 **************************/

/** @brief IDT flag: storage segment. */
#define IDT_FLAG_STORAGE_SEG 0x10
/** @brief IDT flag: privilege level, ring 0. */
#define IDT_FLAG_PL0         0x00
/** @brief IDT flag: privilege level, ring 1. */
#define IDT_FLAG_PL1         0x20
/** @brief IDT flag: privilege level, ring 2. */
#define IDT_FLAG_PL2         0x40
/** @brief IDT flag: privilege level, ring 3. */
#define IDT_FLAG_PL3         0x60
/** @brief IDT flag: interrupt present. */
#define IDT_FLAG_PRESENT     0x80

/** @brief IDT flag: interrupt type task gate. */
#define IDT_TYPE_TASK_GATE 0x05
/** @brief IDT flag: interrupt type interrupt gate. */
#define IDT_TYPE_INT_GATE  0x0E
/** @brief IDT flag: interrupt type trap gate. */
#define IDT_TYPE_TRAP_GATE 0x0F

/** @brief Number of entries in the kernel's GDT. */
#define GDT_ENTRY_COUNT (13 + MAX_CPU_COUNT)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief CPU IDT entry. Describes an entry in the IDT.
 */
struct cpu_idt_entry
{
    /** @brief ISR low address. */
    uint16_t off_low;

    /** @brief Code segment selector. */
    uint16_t c_sel;

    /** @brief Entry IST number. */
    uint8_t ist;

    /** @brief Entry flags. */
    uint8_t flags;

    /** @brief ISR middle address. */
    uint16_t off_mid;

    /** @brief ISR high address. */
    uint32_t off_hig;

    /** @brief Must be zero. */
    uint32_t reserved1;
};

/**
 * @brief Defines the cpu_idt_entry_t type as a shortcut for struct
 * cpu_idt_entry.
 */
typedef struct cpu_idt_entry cpu_idt_entry_t;

/**
 * @brief Define the GDT pointer, contains the  address and limit of the GDT.
 */
typedef struct
{
    /** @brief The GDT size. */
    uint16_t size;

    /** @brief The GDT address. */
    uintptr_t base;
}__attribute__((packed)) gdt_ptr_t;

/**
 * @brief Define the IDT pointer, contains the  address and limit of the IDT.
 */
typedef struct
{
    /** @brief The IDT size. */
    uint16_t size;

    /** @brief The IDT address. */
    uintptr_t base;
}__attribute__((packed)) idt_ptr_t;


/**
 * @brief CPU TSS abstraction structure. This is the representation the kernel
 * has of an intel's TSS entry.
 */
typedef struct cpu_tss_entry
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t iomap_base;
    uint16_t reserved3;
} __attribute__((__packed__)) cpu_tss_entry_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel stacks base symbol. */
extern int8_t _KERNEL_STACKS_BASE;

/**
 * @brief Assembly interrupt handler for line 0.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_0(void);
/**
 * @brief Assembly interrupt handler for line 1.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_1(void);
/**
 * @brief Assembly interrupt handler for line 2.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_2(void);
/**
 * @brief Assembly interrupt handler for line 3.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_3(void);
/**
 * @brief Assembly interrupt handler for line 4.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_4(void);
/**
 * @brief Assembly interrupt handler for line 5.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_5(void);
/**
 * @brief Assembly interrupt handler for line 6.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_6(void);
/**
 * @brief Assembly interrupt handler for line 7.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_7(void);
/**
 * @brief Assembly interrupt handler for line 8.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_8(void);
/**
 * @brief Assembly interrupt handler for line 9.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_9(void);
/**
 * @brief Assembly interrupt handler for line 10.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_10(void);
/**
 * @brief Assembly interrupt handler for line 11.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_11(void);
/**
 * @brief Assembly interrupt handler for line 12.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_12(void);
/**
 * @brief Assembly interrupt handler for line 13.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_13(void);
/**
 * @brief Assembly interrupt handler for line 14.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_14(void);
/**
 * @brief Assembly interrupt handler for line 15.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_15(void);
/**
 * @brief Assembly interrupt handler for line 16.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_16(void);
/**
 * @brief Assembly interrupt handler for line 17.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_17(void);
/**
 * @brief Assembly interrupt handler for line 18.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_18(void);
/**
 * @brief Assembly interrupt handler for line 19.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_19(void);
/**
 * @brief Assembly interrupt handler for line 20.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_20(void);
/**
 * @brief Assembly interrupt handler for line 21.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_21(void);
/**
 * @brief Assembly interrupt handler for line 22.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_22(void);
/**
 * @brief Assembly interrupt handler for line 23.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_23(void);
/**
 * @brief Assembly interrupt handler for line 24.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_24(void);
/**
 * @brief Assembly interrupt handler for line 25.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_25(void);
/**
 * @brief Assembly interrupt handler for line 26.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_26(void);
/**
 * @brief Assembly interrupt handler for line 27.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_27(void);
/**
 * @brief Assembly interrupt handler for line 28.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_28(void);
/**
 * @brief Assembly interrupt handler for line 29.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_29(void);
/**
 * @brief Assembly interrupt handler for line 30.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_30(void);
/**
 * @brief Assembly interrupt handler for line 31.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_31(void);
/**
 * @brief Assembly interrupt handler for line 32.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_32(void);
/**
 * @brief Assembly interrupt handler for line 33.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_33(void);
/**
 * @brief Assembly interrupt handler for line 34.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_34(void);
/**
 * @brief Assembly interrupt handler for line 35.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_35(void);
/**
 * @brief Assembly interrupt handler for line 36.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_36(void);
/**
 * @brief Assembly interrupt handler for line 37.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_37(void);
/**
 * @brief Assembly interrupt handler for line 38.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_38(void);
/**
 * @brief Assembly interrupt handler for line 39.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_39(void);
/**
 * @brief Assembly interrupt handler for line 40.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_40(void);
/**
 * @brief Assembly interrupt handler for line 41.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_41(void);
/**
 * @brief Assembly interrupt handler for line 42.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_42(void);
/**
 * @brief Assembly interrupt handler for line 43.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_43(void);
/**
 * @brief Assembly interrupt handler for line 44.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_44(void);
/**
 * @brief Assembly interrupt handler for line 45.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_45(void);
/**
 * @brief Assembly interrupt handler for line 46.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_46(void);
/**
 * @brief Assembly interrupt handler for line 47.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_47(void);
/**
 * @brief Assembly interrupt handler for line 48.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_48(void);
/**
 * @brief Assembly interrupt handler for line 49.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_49(void);
/**
 * @brief Assembly interrupt handler for line 50.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_50(void);
/**
 * @brief Assembly interrupt handler for line 51.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_51(void);
/**
 * @brief Assembly interrupt handler for line 52.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_52(void);
/**
 * @brief Assembly interrupt handler for line 53.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_53(void);
/**
 * @brief Assembly interrupt handler for line 54.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_54(void);
/**
 * @brief Assembly interrupt handler for line 55.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_55(void);
/**
 * @brief Assembly interrupt handler for line 56.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_56(void);
/**
 * @brief Assembly interrupt handler for line 57.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_57(void);
/**
 * @brief Assembly interrupt handler for line 58.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_58(void);
/**
 * @brief Assembly interrupt handler for line 59.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_59(void);
/**
 * @brief Assembly interrupt handler for line 60.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_60(void);
/**
 * @brief Assembly interrupt handler for line 61.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_61(void);
/**
 * @brief Assembly interrupt handler for line 62.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_62(void);
/**
 * @brief Assembly interrupt handler for line 63.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_63(void);
/**
 * @brief Assembly interrupt handler for line 64.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_64(void);
/**
 * @brief Assembly interrupt handler for line 65.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_65(void);
/**
 * @brief Assembly interrupt handler for line 66.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_66(void);
/**
 * @brief Assembly interrupt handler for line 67.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_67(void);
/**
 * @brief Assembly interrupt handler for line 68.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_68(void);
/**
 * @brief Assembly interrupt handler for line 69.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_69(void);
/**
 * @brief Assembly interrupt handler for line 70.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_70(void);
/**
 * @brief Assembly interrupt handler for line 71.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_71(void);
/**
 * @brief Assembly interrupt handler for line 72.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_72(void);
/**
 * @brief Assembly interrupt handler for line 73.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_73(void);
/**
 * @brief Assembly interrupt handler for line 74.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_74(void);
/**
 * @brief Assembly interrupt handler for line 75.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_75(void);
/**
 * @brief Assembly interrupt handler for line 76.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_76(void);
/**
 * @brief Assembly interrupt handler for line 77.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_77(void);
/**
 * @brief Assembly interrupt handler for line 78.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_78(void);
/**
 * @brief Assembly interrupt handler for line 79.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_79(void);
/**
 * @brief Assembly interrupt handler for line 80.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_80(void);
/**
 * @brief Assembly interrupt handler for line 81.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_81(void);
/**
 * @brief Assembly interrupt handler for line 82.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_82(void);
/**
 * @brief Assembly interrupt handler for line 83.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_83(void);
/**
 * @brief Assembly interrupt handler for line 84.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_84(void);
/**
 * @brief Assembly interrupt handler for line 85.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_85(void);
/**
 * @brief Assembly interrupt handler for line 86.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_86(void);
/**
 * @brief Assembly interrupt handler for line 87.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_87(void);
/**
 * @brief Assembly interrupt handler for line 88.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_88(void);
/**
 * @brief Assembly interrupt handler for line 89.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_89(void);
/**
 * @brief Assembly interrupt handler for line 90.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_90(void);
/**
 * @brief Assembly interrupt handler for line 91.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_91(void);
/**
 * @brief Assembly interrupt handler for line 92.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_92(void);
/**
 * @brief Assembly interrupt handler for line 93.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_93(void);
/**
 * @brief Assembly interrupt handler for line 94.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_94(void);
/**
 * @brief Assembly interrupt handler for line 95.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_95(void);
/**
 * @brief Assembly interrupt handler for line 96.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_96(void);
/**
 * @brief Assembly interrupt handler for line 97.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_97(void);
/**
 * @brief Assembly interrupt handler for line 98.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_98(void);
/**
 * @brief Assembly interrupt handler for line 99.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_99(void);
/**
 * @brief Assembly interrupt handler for line 100.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_100(void);
/**
 * @brief Assembly interrupt handler for line 101.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_101(void);
/**
 * @brief Assembly interrupt handler for line 102.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_102(void);
/**
 * @brief Assembly interrupt handler for line 103.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_103(void);
/**
 * @brief Assembly interrupt handler for line 104.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_104(void);
/**
 * @brief Assembly interrupt handler for line 105.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_105(void);
/**
 * @brief Assembly interrupt handler for line 106.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_106(void);
/**
 * @brief Assembly interrupt handler for line 107.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_107(void);
/**
 * @brief Assembly interrupt handler for line 108.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_108(void);
/**
 * @brief Assembly interrupt handler for line 109.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_109(void);
/**
 * @brief Assembly interrupt handler for line 110.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_110(void);
/**
 * @brief Assembly interrupt handler for line 111.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_111(void);
/**
 * @brief Assembly interrupt handler for line 112.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_112(void);
/**
 * @brief Assembly interrupt handler for line 113.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_113(void);
/**
 * @brief Assembly interrupt handler for line 114.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_114(void);
/**
 * @brief Assembly interrupt handler for line 115.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_115(void);
/**
 * @brief Assembly interrupt handler for line 116.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_116(void);
/**
 * @brief Assembly interrupt handler for line 117.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_117(void);
/**
 * @brief Assembly interrupt handler for line 118.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_118(void);
/**
 * @brief Assembly interrupt handler for line 119.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_119(void);
/**
 * @brief Assembly interrupt handler for line 120.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_120(void);
/**
 * @brief Assembly interrupt handler for line 121.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_121(void);
/**
 * @brief Assembly interrupt handler for line 122.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_122(void);
/**
 * @brief Assembly interrupt handler for line 123.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_123(void);
/**
 * @brief Assembly interrupt handler for line 124.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_124(void);
/**
 * @brief Assembly interrupt handler for line 125.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_125(void);
/**
 * @brief Assembly interrupt handler for line 126.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_126(void);
/**
 * @brief Assembly interrupt handler for line 127.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_127(void);
/**
 * @brief Assembly interrupt handler for line 128.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_128(void);
/**
 * @brief Assembly interrupt handler for line 129.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_129(void);
/**
 * @brief Assembly interrupt handler for line 130.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_130(void);
/**
 * @brief Assembly interrupt handler for line 131.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_131(void);
/**
 * @brief Assembly interrupt handler for line 132.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_132(void);
/**
 * @brief Assembly interrupt handler for line 133.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_133(void);
/**
 * @brief Assembly interrupt handler for line 134.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_134(void);
/**
 * @brief Assembly interrupt handler for line 135.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_135(void);
/**
 * @brief Assembly interrupt handler for line 136.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_136(void);
/**
 * @brief Assembly interrupt handler for line 137.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_137(void);
/**
 * @brief Assembly interrupt handler for line 138.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_138(void);
/**
 * @brief Assembly interrupt handler for line 139.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_139(void);
/**
 * @brief Assembly interrupt handler for line 140.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_140(void);
/**
 * @brief Assembly interrupt handler for line 141.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_141(void);
/**
 * @brief Assembly interrupt handler for line 142.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_142(void);
/**
 * @brief Assembly interrupt handler for line 143.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_143(void);
/**
 * @brief Assembly interrupt handler for line 144.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_144(void);
/**
 * @brief Assembly interrupt handler for line 145.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_145(void);
/**
 * @brief Assembly interrupt handler for line 146.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_146(void);
/**
 * @brief Assembly interrupt handler for line 147.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_147(void);
/**
 * @brief Assembly interrupt handler for line 148.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_148(void);
/**
 * @brief Assembly interrupt handler for line 149.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_149(void);
/**
 * @brief Assembly interrupt handler for line 150.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_150(void);
/**
 * @brief Assembly interrupt handler for line 151.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_151(void);
/**
 * @brief Assembly interrupt handler for line 152.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_152(void);
/**
 * @brief Assembly interrupt handler for line 153.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_153(void);
/**
 * @brief Assembly interrupt handler for line 154.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_154(void);
/**
 * @brief Assembly interrupt handler for line 155.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_155(void);
/**
 * @brief Assembly interrupt handler for line 156.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_156(void);
/**
 * @brief Assembly interrupt handler for line 157.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_157(void);
/**
 * @brief Assembly interrupt handler for line 158.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_158(void);
/**
 * @brief Assembly interrupt handler for line 159.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_159(void);
/**
 * @brief Assembly interrupt handler for line 160.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_160(void);
/**
 * @brief Assembly interrupt handler for line 161.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_161(void);
/**
 * @brief Assembly interrupt handler for line 162.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_162(void);
/**
 * @brief Assembly interrupt handler for line 163.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_163(void);
/**
 * @brief Assembly interrupt handler for line 164.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_164(void);
/**
 * @brief Assembly interrupt handler for line 165.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_165(void);
/**
 * @brief Assembly interrupt handler for line 166.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_166(void);
/**
 * @brief Assembly interrupt handler for line 167.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_167(void);
/**
 * @brief Assembly interrupt handler for line 168.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_168(void);
/**
 * @brief Assembly interrupt handler for line 169.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_169(void);
/**
 * @brief Assembly interrupt handler for line 170.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_170(void);
/**
 * @brief Assembly interrupt handler for line 171.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_171(void);
/**
 * @brief Assembly interrupt handler for line 172.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_172(void);
/**
 * @brief Assembly interrupt handler for line 173.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_173(void);
/**
 * @brief Assembly interrupt handler for line 174.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_174(void);
/**
 * @brief Assembly interrupt handler for line 175.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_175(void);
/**
 * @brief Assembly interrupt handler for line 176.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_176(void);
/**
 * @brief Assembly interrupt handler for line 177.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_177(void);
/**
 * @brief Assembly interrupt handler for line 178.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_178(void);
/**
 * @brief Assembly interrupt handler for line 179.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_179(void);
/**
 * @brief Assembly interrupt handler for line 180.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_180(void);
/**
 * @brief Assembly interrupt handler for line 181.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_181(void);
/**
 * @brief Assembly interrupt handler for line 182.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_182(void);
/**
 * @brief Assembly interrupt handler for line 183.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_183(void);
/**
 * @brief Assembly interrupt handler for line 184.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_184(void);
/**
 * @brief Assembly interrupt handler for line 185.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_185(void);
/**
 * @brief Assembly interrupt handler for line 186.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_186(void);
/**
 * @brief Assembly interrupt handler for line 187.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_187(void);
/**
 * @brief Assembly interrupt handler for line 188.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_188(void);
/**
 * @brief Assembly interrupt handler for line 189.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_189(void);
/**
 * @brief Assembly interrupt handler for line 190.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_190(void);
/**
 * @brief Assembly interrupt handler for line 191.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_191(void);
/**
 * @brief Assembly interrupt handler for line 192.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_192(void);
/**
 * @brief Assembly interrupt handler for line 193.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_193(void);
/**
 * @brief Assembly interrupt handler for line 194.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_194(void);
/**
 * @brief Assembly interrupt handler for line 195.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_195(void);
/**
 * @brief Assembly interrupt handler for line 196.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_196(void);
/**
 * @brief Assembly interrupt handler for line 197.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_197(void);
/**
 * @brief Assembly interrupt handler for line 198.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_198(void);
/**
 * @brief Assembly interrupt handler for line 199.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_199(void);
/**
 * @brief Assembly interrupt handler for line 200.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_200(void);
/**
 * @brief Assembly interrupt handler for line 201.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_201(void);
/**
 * @brief Assembly interrupt handler for line 202.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_202(void);
/**
 * @brief Assembly interrupt handler for line 203.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_203(void);
/**
 * @brief Assembly interrupt handler for line 204.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_204(void);
/**
 * @brief Assembly interrupt handler for line 205.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_205(void);
/**
 * @brief Assembly interrupt handler for line 206.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_206(void);
/**
 * @brief Assembly interrupt handler for line 207.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_207(void);
/**
 * @brief Assembly interrupt handler for line 208.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_208(void);
/**
 * @brief Assembly interrupt handler for line 209.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_209(void);
/**
 * @brief Assembly interrupt handler for line 210.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_210(void);
/**
 * @brief Assembly interrupt handler for line 211.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_211(void);
/**
 * @brief Assembly interrupt handler for line 212.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_212(void);
/**
 * @brief Assembly interrupt handler for line 213.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_213(void);
/**
 * @brief Assembly interrupt handler for line 214.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_214(void);
/**
 * @brief Assembly interrupt handler for line 215.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_215(void);
/**
 * @brief Assembly interrupt handler for line 216.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_216(void);
/**
 * @brief Assembly interrupt handler for line 217.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_217(void);
/**
 * @brief Assembly interrupt handler for line 218.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_218(void);
/**
 * @brief Assembly interrupt handler for line 219.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_219(void);
/**
 * @brief Assembly interrupt handler for line 220.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_220(void);
/**
 * @brief Assembly interrupt handler for line 221.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_221(void);
/**
 * @brief Assembly interrupt handler for line 222.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_222(void);
/**
 * @brief Assembly interrupt handler for line 223.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_223(void);
/**
 * @brief Assembly interrupt handler for line 224.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_224(void);
/**
 * @brief Assembly interrupt handler for line 225.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_225(void);
/**
 * @brief Assembly interrupt handler for line 226.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_226(void);
/**
 * @brief Assembly interrupt handler for line 227.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_227(void);
/**
 * @brief Assembly interrupt handler for line 228.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_228(void);
/**
 * @brief Assembly interrupt handler for line 229.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_229(void);
/**
 * @brief Assembly interrupt handler for line 230.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_230(void);
/**
 * @brief Assembly interrupt handler for line 231.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_231(void);
/**
 * @brief Assembly interrupt handler for line 232.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_232(void);
/**
 * @brief Assembly interrupt handler for line 233.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_233(void);
/**
 * @brief Assembly interrupt handler for line 234.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_234(void);
/**
 * @brief Assembly interrupt handler for line 235.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_235(void);
/**
 * @brief Assembly interrupt handler for line 236.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_236(void);
/**
 * @brief Assembly interrupt handler for line 237.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_237(void);
/**
 * @brief Assembly interrupt handler for line 238.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_238(void);
/**
 * @brief Assembly interrupt handler for line 239.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_239(void);
/**
 * @brief Assembly interrupt handler for line 240.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_240(void);
/**
 * @brief Assembly interrupt handler for line 241.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_241(void);
/**
 * @brief Assembly interrupt handler for line 242.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_242(void);
/**
 * @brief Assembly interrupt handler for line 243.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_243(void);
/**
 * @brief Assembly interrupt handler for line 244.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_244(void);
/**
 * @brief Assembly interrupt handler for line 245.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_245(void);
/**
 * @brief Assembly interrupt handler for line 246.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_246(void);
/**
 * @brief Assembly interrupt handler for line 247.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_247(void);
/**
 * @brief Assembly interrupt handler for line 248.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_248(void);
/**
 * @brief Assembly interrupt handler for line 249.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_249(void);
/**
 * @brief Assembly interrupt handler for line 250.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_250(void);
/**
 * @brief Assembly interrupt handler for line 251.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_251(void);
/**
 * @brief Assembly interrupt handler for line 252.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_252(void);
/**
 * @brief Assembly interrupt handler for line 253.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_253(void);
/**
 * @brief Assembly interrupt handler for line 254.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_254(void);
/**
 * @brief Assembly interrupt handler for line 255.
 * Saves the context and calls the generic interrupt handler
 */
extern void interrupt_handler_255(void);

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief CPU GDT space in memory. */
static uint64_t cpu_gdt[GDT_ENTRY_COUNT] __attribute__((aligned(8)));
/** @brief Kernel GDT structure */
static gdt_ptr_t cpu_gdt_ptr __attribute__((aligned(8)));

/** @brief CPU IDT space in memory. */
static cpu_idt_entry_t cpu_idt[IDT_ENTRY_COUNT] __attribute__((aligned(8)));
/** @brief Kernel IDT structure */
static idt_ptr_t cpu_idt_ptr __attribute__((aligned(8)));

/** @brief CPU TSS structures */
static cpu_tss_entry_t cpu_tss[MAX_CPU_COUNT] __attribute__((aligned(8)));

/** @brief Stores the CPU interrupt handlers entry point */
static uintptr_t cpu_int_handlers[IDT_ENTRY_COUNT] = {
    (uintptr_t)interrupt_handler_0,
    (uintptr_t)interrupt_handler_1,
    (uintptr_t)interrupt_handler_2,
    (uintptr_t)interrupt_handler_3,
    (uintptr_t)interrupt_handler_4,
    (uintptr_t)interrupt_handler_5,
    (uintptr_t)interrupt_handler_6,
    (uintptr_t)interrupt_handler_7,
    (uintptr_t)interrupt_handler_8,
    (uintptr_t)interrupt_handler_9,
    (uintptr_t)interrupt_handler_10,
    (uintptr_t)interrupt_handler_11,
    (uintptr_t)interrupt_handler_12,
    (uintptr_t)interrupt_handler_13,
    (uintptr_t)interrupt_handler_14,
    (uintptr_t)interrupt_handler_15,
    (uintptr_t)interrupt_handler_16,
    (uintptr_t)interrupt_handler_17,
    (uintptr_t)interrupt_handler_18,
    (uintptr_t)interrupt_handler_19,
    (uintptr_t)interrupt_handler_20,
    (uintptr_t)interrupt_handler_21,
    (uintptr_t)interrupt_handler_22,
    (uintptr_t)interrupt_handler_23,
    (uintptr_t)interrupt_handler_24,
    (uintptr_t)interrupt_handler_25,
    (uintptr_t)interrupt_handler_26,
    (uintptr_t)interrupt_handler_27,
    (uintptr_t)interrupt_handler_28,
    (uintptr_t)interrupt_handler_29,
    (uintptr_t)interrupt_handler_30,
    (uintptr_t)interrupt_handler_31,
    (uintptr_t)interrupt_handler_32,
    (uintptr_t)interrupt_handler_33,
    (uintptr_t)interrupt_handler_34,
    (uintptr_t)interrupt_handler_35,
    (uintptr_t)interrupt_handler_36,
    (uintptr_t)interrupt_handler_37,
    (uintptr_t)interrupt_handler_38,
    (uintptr_t)interrupt_handler_39,
    (uintptr_t)interrupt_handler_40,
    (uintptr_t)interrupt_handler_41,
    (uintptr_t)interrupt_handler_42,
    (uintptr_t)interrupt_handler_43,
    (uintptr_t)interrupt_handler_44,
    (uintptr_t)interrupt_handler_45,
    (uintptr_t)interrupt_handler_46,
    (uintptr_t)interrupt_handler_47,
    (uintptr_t)interrupt_handler_48,
    (uintptr_t)interrupt_handler_49,
    (uintptr_t)interrupt_handler_50,
    (uintptr_t)interrupt_handler_51,
    (uintptr_t)interrupt_handler_52,
    (uintptr_t)interrupt_handler_53,
    (uintptr_t)interrupt_handler_54,
    (uintptr_t)interrupt_handler_55,
    (uintptr_t)interrupt_handler_56,
    (uintptr_t)interrupt_handler_57,
    (uintptr_t)interrupt_handler_58,
    (uintptr_t)interrupt_handler_59,
    (uintptr_t)interrupt_handler_60,
    (uintptr_t)interrupt_handler_61,
    (uintptr_t)interrupt_handler_62,
    (uintptr_t)interrupt_handler_63,
    (uintptr_t)interrupt_handler_64,
    (uintptr_t)interrupt_handler_65,
    (uintptr_t)interrupt_handler_66,
    (uintptr_t)interrupt_handler_67,
    (uintptr_t)interrupt_handler_68,
    (uintptr_t)interrupt_handler_69,
    (uintptr_t)interrupt_handler_70,
    (uintptr_t)interrupt_handler_71,
    (uintptr_t)interrupt_handler_72,
    (uintptr_t)interrupt_handler_73,
    (uintptr_t)interrupt_handler_74,
    (uintptr_t)interrupt_handler_75,
    (uintptr_t)interrupt_handler_76,
    (uintptr_t)interrupt_handler_77,
    (uintptr_t)interrupt_handler_78,
    (uintptr_t)interrupt_handler_79,
    (uintptr_t)interrupt_handler_80,
    (uintptr_t)interrupt_handler_81,
    (uintptr_t)interrupt_handler_82,
    (uintptr_t)interrupt_handler_83,
    (uintptr_t)interrupt_handler_84,
    (uintptr_t)interrupt_handler_85,
    (uintptr_t)interrupt_handler_86,
    (uintptr_t)interrupt_handler_87,
    (uintptr_t)interrupt_handler_88,
    (uintptr_t)interrupt_handler_89,
    (uintptr_t)interrupt_handler_90,
    (uintptr_t)interrupt_handler_91,
    (uintptr_t)interrupt_handler_92,
    (uintptr_t)interrupt_handler_93,
    (uintptr_t)interrupt_handler_94,
    (uintptr_t)interrupt_handler_95,
    (uintptr_t)interrupt_handler_96,
    (uintptr_t)interrupt_handler_97,
    (uintptr_t)interrupt_handler_98,
    (uintptr_t)interrupt_handler_99,
    (uintptr_t)interrupt_handler_100,
    (uintptr_t)interrupt_handler_101,
    (uintptr_t)interrupt_handler_102,
    (uintptr_t)interrupt_handler_103,
    (uintptr_t)interrupt_handler_104,
    (uintptr_t)interrupt_handler_105,
    (uintptr_t)interrupt_handler_106,
    (uintptr_t)interrupt_handler_107,
    (uintptr_t)interrupt_handler_108,
    (uintptr_t)interrupt_handler_109,
    (uintptr_t)interrupt_handler_110,
    (uintptr_t)interrupt_handler_111,
    (uintptr_t)interrupt_handler_112,
    (uintptr_t)interrupt_handler_113,
    (uintptr_t)interrupt_handler_114,
    (uintptr_t)interrupt_handler_115,
    (uintptr_t)interrupt_handler_116,
    (uintptr_t)interrupt_handler_117,
    (uintptr_t)interrupt_handler_118,
    (uintptr_t)interrupt_handler_119,
    (uintptr_t)interrupt_handler_120,
    (uintptr_t)interrupt_handler_121,
    (uintptr_t)interrupt_handler_122,
    (uintptr_t)interrupt_handler_123,
    (uintptr_t)interrupt_handler_124,
    (uintptr_t)interrupt_handler_125,
    (uintptr_t)interrupt_handler_126,
    (uintptr_t)interrupt_handler_127,
    (uintptr_t)interrupt_handler_128,
    (uintptr_t)interrupt_handler_129,
    (uintptr_t)interrupt_handler_130,
    (uintptr_t)interrupt_handler_131,
    (uintptr_t)interrupt_handler_132,
    (uintptr_t)interrupt_handler_133,
    (uintptr_t)interrupt_handler_134,
    (uintptr_t)interrupt_handler_135,
    (uintptr_t)interrupt_handler_136,
    (uintptr_t)interrupt_handler_137,
    (uintptr_t)interrupt_handler_138,
    (uintptr_t)interrupt_handler_139,
    (uintptr_t)interrupt_handler_140,
    (uintptr_t)interrupt_handler_141,
    (uintptr_t)interrupt_handler_142,
    (uintptr_t)interrupt_handler_143,
    (uintptr_t)interrupt_handler_144,
    (uintptr_t)interrupt_handler_145,
    (uintptr_t)interrupt_handler_146,
    (uintptr_t)interrupt_handler_147,
    (uintptr_t)interrupt_handler_148,
    (uintptr_t)interrupt_handler_149,
    (uintptr_t)interrupt_handler_150,
    (uintptr_t)interrupt_handler_151,
    (uintptr_t)interrupt_handler_152,
    (uintptr_t)interrupt_handler_153,
    (uintptr_t)interrupt_handler_154,
    (uintptr_t)interrupt_handler_155,
    (uintptr_t)interrupt_handler_156,
    (uintptr_t)interrupt_handler_157,
    (uintptr_t)interrupt_handler_158,
    (uintptr_t)interrupt_handler_159,
    (uintptr_t)interrupt_handler_160,
    (uintptr_t)interrupt_handler_161,
    (uintptr_t)interrupt_handler_162,
    (uintptr_t)interrupt_handler_163,
    (uintptr_t)interrupt_handler_164,
    (uintptr_t)interrupt_handler_165,
    (uintptr_t)interrupt_handler_166,
    (uintptr_t)interrupt_handler_167,
    (uintptr_t)interrupt_handler_168,
    (uintptr_t)interrupt_handler_169,
    (uintptr_t)interrupt_handler_170,
    (uintptr_t)interrupt_handler_171,
    (uintptr_t)interrupt_handler_172,
    (uintptr_t)interrupt_handler_173,
    (uintptr_t)interrupt_handler_174,
    (uintptr_t)interrupt_handler_175,
    (uintptr_t)interrupt_handler_176,
    (uintptr_t)interrupt_handler_177,
    (uintptr_t)interrupt_handler_178,
    (uintptr_t)interrupt_handler_179,
    (uintptr_t)interrupt_handler_180,
    (uintptr_t)interrupt_handler_181,
    (uintptr_t)interrupt_handler_182,
    (uintptr_t)interrupt_handler_183,
    (uintptr_t)interrupt_handler_184,
    (uintptr_t)interrupt_handler_185,
    (uintptr_t)interrupt_handler_186,
    (uintptr_t)interrupt_handler_187,
    (uintptr_t)interrupt_handler_188,
    (uintptr_t)interrupt_handler_189,
    (uintptr_t)interrupt_handler_190,
    (uintptr_t)interrupt_handler_191,
    (uintptr_t)interrupt_handler_192,
    (uintptr_t)interrupt_handler_193,
    (uintptr_t)interrupt_handler_194,
    (uintptr_t)interrupt_handler_195,
    (uintptr_t)interrupt_handler_196,
    (uintptr_t)interrupt_handler_197,
    (uintptr_t)interrupt_handler_198,
    (uintptr_t)interrupt_handler_199,
    (uintptr_t)interrupt_handler_200,
    (uintptr_t)interrupt_handler_201,
    (uintptr_t)interrupt_handler_202,
    (uintptr_t)interrupt_handler_203,
    (uintptr_t)interrupt_handler_204,
    (uintptr_t)interrupt_handler_205,
    (uintptr_t)interrupt_handler_206,
    (uintptr_t)interrupt_handler_207,
    (uintptr_t)interrupt_handler_208,
    (uintptr_t)interrupt_handler_209,
    (uintptr_t)interrupt_handler_210,
    (uintptr_t)interrupt_handler_211,
    (uintptr_t)interrupt_handler_212,
    (uintptr_t)interrupt_handler_213,
    (uintptr_t)interrupt_handler_214,
    (uintptr_t)interrupt_handler_215,
    (uintptr_t)interrupt_handler_216,
    (uintptr_t)interrupt_handler_217,
    (uintptr_t)interrupt_handler_218,
    (uintptr_t)interrupt_handler_219,
    (uintptr_t)interrupt_handler_220,
    (uintptr_t)interrupt_handler_221,
    (uintptr_t)interrupt_handler_222,
    (uintptr_t)interrupt_handler_223,
    (uintptr_t)interrupt_handler_224,
    (uintptr_t)interrupt_handler_225,
    (uintptr_t)interrupt_handler_226,
    (uintptr_t)interrupt_handler_227,
    (uintptr_t)interrupt_handler_228,
    (uintptr_t)interrupt_handler_229,
    (uintptr_t)interrupt_handler_230,
    (uintptr_t)interrupt_handler_231,
    (uintptr_t)interrupt_handler_232,
    (uintptr_t)interrupt_handler_233,
    (uintptr_t)interrupt_handler_234,
    (uintptr_t)interrupt_handler_235,
    (uintptr_t)interrupt_handler_236,
    (uintptr_t)interrupt_handler_237,
    (uintptr_t)interrupt_handler_238,
    (uintptr_t)interrupt_handler_239,
    (uintptr_t)interrupt_handler_240,
    (uintptr_t)interrupt_handler_241,
    (uintptr_t)interrupt_handler_242,
    (uintptr_t)interrupt_handler_243,
    (uintptr_t)interrupt_handler_244,
    (uintptr_t)interrupt_handler_245,
    (uintptr_t)interrupt_handler_246,
    (uintptr_t)interrupt_handler_247,
    (uintptr_t)interrupt_handler_248,
    (uintptr_t)interrupt_handler_249,
    (uintptr_t)interrupt_handler_250,
    (uintptr_t)interrupt_handler_251,
    (uintptr_t)interrupt_handler_252,
    (uintptr_t)interrupt_handler_253,
    (uintptr_t)interrupt_handler_254,
    (uintptr_t)interrupt_handler_255
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Setups the kernel's GDT in memory and loads it in the GDT register.
 *
 * @details Setups a GDT for the kernel. Fills the entries in the GDT table and
 * load the new GDT in the CPU's GDT register.
 * Once done, the function sets the segment registers (CS, DS, ES, FS, GS, SS)
 * of the CPU according to the kernel's settings.
 */
static void _cpu_setup_gdt(void);

/**
 * @brief Setups the generic kernel's IDT in memory and loads it in the IDT
 * register.
 *
 * @details Setups a simple IDT for the kernel. Fills the entries in the IDT
 * table by adding basic support to the x86 exception (interrutps 0 to 32).
 * The rest of the interrupts are not set.
 */
static void _cpu_setup_idt(void);

/**
 *  @brief Setups the main CPU TSS for the kernel.
 *
 * @details Initializes the main CPU's TSS with kernel settings in memory and
 * loads it in the TSS register.
 */
static void _cpu_setup_tss(void);

/**
 * @brief Formats a GDT entry.
 *
 * @details Formats data given as parameter into a standard GDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] entry The pointer to the entry structure to format.
 * @param[in] base  The base address of the segment for the GDT entry.
 * @param[in] limit The limit address of the segment for the GDT entry.
 * @param[in] type  The type of segment for the GDT entry.
 * @param[in] flags The flags to be set for the GDT entry.
 */
static void _format_gdt_entry(uint64_t* entry,
                              const uint32_t base, const uint32_t limit,
                              const uint8_t type, const uint32_t flags);

/**
 * @brief Formats an IDT entry.
 *
 * @details Formats data given as parameter into a standard IDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] entry The pointer to the entry structure to format.
 * @param[in] handler The handler function for the IDT entry.
 * @param[in] type  The type of segment for the IDT entry.
 * @param[in] flags The flags to be set for the IDT entry.
 */
static void _format_idt_entry(cpu_idt_entry_t* entry,
                              const uintptr_t handler,
                              const uint8_t type, const uint32_t flags);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _format_gdt_entry(uint64_t* entry,
                              const uint32_t base, const uint32_t limit,
                              const uint8_t type, const uint32_t flags)
{
    uint32_t lo_part = 0;
    uint32_t hi_part = 0;

    /*
     * Low part[31;0] = Base[15;0] Limit[15;0]
     */
    lo_part = ((base & 0xFFFF) << 16) | (limit & 0xFFFF);

    /*
     * High part[7;0] = Base[23;16]
     */
    hi_part = (base >> 16) & 0xFF;
    /*
     * High part[11;8] = Type[3;0]
     */
    hi_part |= (type & 0xF) << 8;
    /*
     * High part[15;12] = Seg_Present[1;0]Privilege[2;0]Descriptor_Type[1;0]
     * High part[23;20] = Granularity[1;0]Op_Size[1;0]L[1;0]AVL[1;0]
     */
    hi_part |= flags & 0x00F0F000;

    /*
     * High part[19;16] = Limit[19;16]
     */
    hi_part |= limit & 0xF0000;
    /*
     * High part[31;24] = Base[31;24]
     */
    hi_part |= base & 0xFF000000;

    /* Set the value of the entry */
    *entry = lo_part | (((uint64_t) hi_part) << 32);
}

static void _format_idt_entry(cpu_idt_entry_t* entry,
                              const uintptr_t handler,
                              const uint8_t type, const uint32_t flags)
{
    /* Set offset */
    entry->off_low   = handler & 0x000000000000FFFF;
    entry->off_mid   = (handler >> 16) & 0x000000000000FFFF;
    entry->off_hig   = (handler >> 32) & 0x00000000FFFFFFFF;

    /* Set selector and flags */
    entry->c_sel     = KERNEL_CS_64;
    entry->flags     = (flags & 0xF0) | (type & 0x0F);

    /* Zeroise */
    entry->ist       = 0;
    entry->reserved1 = 0;
}

static void _cpu_setup_gdt(void)
{
    uint32_t i;

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting GDT");

    /************************************
     * KERNEL GDT ENTRIES
     ***********************************/

    /* Set the kernel 64 bits code descriptor */
    uint32_t kernel_code_64_seg_flags = GDT_FLAG_64_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_CODE_TYPE;

    uint32_t kernel_code_64_seg_type =  GDT_TYPE_EXECUTABLE |
                                        GDT_TYPE_READABLE |
                                        GDT_TYPE_PROTECTED;

    /* Set the kernel 64 bits data descriptor */
    uint32_t kernel_data_64_seg_flags = GDT_FLAG_64_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_DATA_TYPE;

    uint32_t kernel_data_64_seg_type =  GDT_TYPE_WRITABLE |
                                        GDT_TYPE_GROW_DOWN;

    /* Set the kernel code descriptor */
    uint32_t kernel_code_32_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                        GDT_FLAG_32_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_CODE_TYPE;

    uint32_t kernel_code_32_seg_type =  GDT_TYPE_EXECUTABLE |
                                        GDT_TYPE_READABLE |
                                        GDT_TYPE_PROTECTED;

    /* Set the kernel data descriptor */
    uint32_t kernel_data_32_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                        GDT_FLAG_32_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_DATA_TYPE;

    uint32_t kernel_data_32_seg_type =  GDT_TYPE_WRITABLE |
                                        GDT_TYPE_GROW_DOWN;

    /* Set the kernel 16 bits code descriptor */
    uint32_t kernel_code_16_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                        GDT_FLAG_16_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_CODE_TYPE;

    uint32_t kernel_code_16_seg_type =  GDT_TYPE_EXECUTABLE |
                                        GDT_TYPE_READABLE |
                                        GDT_TYPE_PROTECTED;

    /* Set the kernel 16 bits data descriptor */
    uint32_t kernel_data_16_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                        GDT_FLAG_16_BIT_SEGMENT |
                                        GDT_FLAG_PL0 |
                                        GDT_FLAG_SEGMENT_PRESENT |
                                        GDT_FLAG_DATA_TYPE;

    uint32_t kernel_data_16_seg_type =  GDT_TYPE_WRITABLE |
                                        GDT_TYPE_GROW_DOWN;

    /* Set the user 32 bits code descriptor */
    uint32_t user_code_32_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                      GDT_FLAG_32_BIT_SEGMENT |
                                      GDT_FLAG_PL3 |
                                      GDT_FLAG_SEGMENT_PRESENT |
                                      GDT_FLAG_CODE_TYPE;

    uint32_t user_code_32_seg_type =  GDT_TYPE_EXECUTABLE |
                                      GDT_TYPE_READABLE |
                                      GDT_TYPE_PROTECTED;

    /* Set the user 32 bits data descriptor */
    uint32_t user_data_32_seg_flags = GDT_FLAG_GRANULARITY_4K |
                                      GDT_FLAG_32_BIT_SEGMENT |
                                      GDT_FLAG_PL3 |
                                      GDT_FLAG_SEGMENT_PRESENT |
                                      GDT_FLAG_DATA_TYPE;

    uint32_t user_data_32_seg_type =  GDT_TYPE_WRITABLE |
                                      GDT_TYPE_GROW_DOWN;

    /* Set the user 64 bits code descriptor */
    uint32_t user_code_64_seg_flags = GDT_FLAG_64_BIT_SEGMENT |
                                      GDT_FLAG_PL3 |
                                      GDT_FLAG_SEGMENT_PRESENT |
                                      GDT_FLAG_CODE_TYPE;

    uint32_t user_code_64_seg_type =  GDT_TYPE_EXECUTABLE |
                                      GDT_TYPE_READABLE |
                                      GDT_TYPE_PROTECTED;

    /* Set the user 64 bits data descriptor */
    uint32_t user_data_64_seg_flags = GDT_FLAG_64_BIT_SEGMENT |
                                      GDT_FLAG_PL3 |
                                      GDT_FLAG_SEGMENT_PRESENT |
                                      GDT_FLAG_DATA_TYPE;

    uint32_t user_data_64_seg_type =  GDT_TYPE_WRITABLE |
                                      GDT_TYPE_GROW_DOWN;

    /************************************
     * TSS ENTRY
     ***********************************/

    uint32_t tss_seg_flags = GDT_FLAG_GRANULARITY_4K |
                             GDT_FLAG_64_BIT_SEGMENT |
                             GDT_FLAG_SEGMENT_PRESENT |
                             GDT_FLAG_PL0 |
                             GDT_FLAG_TSS;

    uint32_t tss_seg_type = GDT_TYPE_ACCESSED |
                            GDT_TYPE_EXECUTABLE;

    /* Blank the GDT, set the NULL descriptor */
    memset(cpu_gdt, 0, sizeof(uint64_t) * GDT_ENTRY_COUNT);

    /* Load the segments */
    _format_gdt_entry(&cpu_gdt[KERNEL_CS_64 / 8],
                      KERNEL_CODE_SEGMENT_BASE_64, KERNEL_CODE_SEGMENT_LIMIT_64,
                      kernel_code_64_seg_type, kernel_code_64_seg_flags);

    _format_gdt_entry(&cpu_gdt[KERNEL_DS_64 / 8],
                      KERNEL_DATA_SEGMENT_BASE_64, KERNEL_DATA_SEGMENT_LIMIT_64,
                      kernel_data_64_seg_type, kernel_data_64_seg_flags);

    _format_gdt_entry(&cpu_gdt[KERNEL_CS_32 / 8],
                      KERNEL_CODE_SEGMENT_BASE_32, KERNEL_CODE_SEGMENT_LIMIT_32,
                      kernel_code_32_seg_type, kernel_code_32_seg_flags);

    _format_gdt_entry(&cpu_gdt[KERNEL_DS_32 / 8],
                      KERNEL_DATA_SEGMENT_BASE_32, KERNEL_DATA_SEGMENT_LIMIT_32,
                      kernel_data_32_seg_type, kernel_data_32_seg_flags);

    _format_gdt_entry(&cpu_gdt[KERNEL_CS_16 / 8],
                      KERNEL_CODE_SEGMENT_BASE_16, KERNEL_CODE_SEGMENT_LIMIT_16,
                      kernel_code_16_seg_type, kernel_code_16_seg_flags);

    _format_gdt_entry(&cpu_gdt[KERNEL_DS_16 / 8],
                      KERNEL_DATA_SEGMENT_BASE_16, KERNEL_DATA_SEGMENT_LIMIT_16,
                      kernel_data_16_seg_type, kernel_data_16_seg_flags);

    _format_gdt_entry(&cpu_gdt[USER_CS_64 / 8],
                      USER_CODE_SEGMENT_BASE_64, USER_CODE_SEGMENT_LIMIT_64,
                      user_code_64_seg_type, user_code_64_seg_flags);

    _format_gdt_entry(&cpu_gdt[USER_DS_64 / 8],
                      USER_DATA_SEGMENT_BASE_64, USER_DATA_SEGMENT_LIMIT_64,
                      user_data_64_seg_type, user_data_64_seg_flags);

    _format_gdt_entry(&cpu_gdt[USER_CS_32 / 8],
                      USER_CODE_SEGMENT_BASE_32, USER_CODE_SEGMENT_LIMIT_32,
                      user_code_32_seg_type, user_code_32_seg_flags);

    _format_gdt_entry(&cpu_gdt[USER_DS_32 / 8],
                      USER_DATA_SEGMENT_BASE_32, USER_DATA_SEGMENT_LIMIT_32,
                      user_data_32_seg_type, user_data_32_seg_flags);

    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        _format_gdt_entry(&cpu_gdt[(TSS_SEGMENT + i * 0x08) / 8],
                          (uintptr_t)&cpu_tss[i],
                          ((uintptr_t)(&cpu_tss[i])) +
                          sizeof(cpu_tss_entry_t),
                          tss_seg_type, tss_seg_flags);
    }

    /* Set the GDT descriptor */
    cpu_gdt_ptr.size = ((sizeof(uint64_t) * GDT_ENTRY_COUNT) - 1);
    cpu_gdt_ptr.base = (uintptr_t)&cpu_gdt;

    /* Load the GDT */
    __asm__ __volatile__("lgdt %0" :: "m" (cpu_gdt_ptr.size),
                                      "m" (cpu_gdt_ptr.base));

    /* Load segment selectors with a far jump for CS*/
    __asm__ __volatile__("movw %w0,%%ds\n\t"
                         "movw %w0,%%es\n\t"
                         "movw %w0,%%fs\n\t"
                         "movw %w0,%%gs\n\t"
                         "movw %w0,%%ss\n\t" :: "r" (KERNEL_DS_64));

    __asm__ __volatile__("mov %0, %%rax\n\t"
                         "push %%rax\n\t"
                         "movabs $new_gdt_seg_, %%rax\n\t"
                         "push %%rax\n\t"
                         "lretq\n\t"
                         "new_gdt_seg_: \n\t" :: "i" (KERNEL_CS_64) : "rax");

    KERNEL_SUCCESS("GDT Initialized at 0x%P\n", cpu_gdt_ptr.base);
}

static void _cpu_setup_idt(void)
{
    uint32_t i;

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting IDT");

    /* Blank the IDT */
    memset(cpu_idt, 0, sizeof(uint64_t) * IDT_ENTRY_COUNT);

    /* Set interrupt handlers for each interrupt
     * This allows to redirect all interrupts to a global handler in C
     */
    for(i = 0; i < IDT_ENTRY_COUNT; ++i)
    {
        _format_idt_entry(&cpu_idt[i],
                          cpu_int_handlers[i],
                          IDT_TYPE_INT_GATE, IDT_FLAG_PRESENT | IDT_FLAG_PL0);
    }

    /* Set the GDT descriptor */
    cpu_idt_ptr.size = ((sizeof(uint64_t) * IDT_ENTRY_COUNT) - 1);
    cpu_idt_ptr.base = (uintptr_t)&cpu_idt;

    /* Load the GDT */
    __asm__ __volatile__("lidt %0" :: "m" (cpu_idt_ptr.size),
                                      "m" (cpu_idt_ptr.base));

    KERNEL_SUCCESS("IDT Initialized at 0x%P\n", cpu_idt_ptr.base);
}

static void _cpu_setup_tss(void)
{
    uint32_t i;

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting TSS");

    /* Blank the TSS */
    memset(cpu_tss, 0, sizeof(cpu_tss_entry_t) * MAX_CPU_COUNT);

    /* Set basic values */
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        cpu_tss[i].rsp0 = ((uintptr_t)&_KERNEL_STACKS_BASE) +
                          KERNEL_STACK_SIZE * (i + 1) - sizeof(uint32_t);
        cpu_tss[i].iomap_base = sizeof(cpu_tss_entry_t);
    }

    /* Load TSS */
    //__asm__ __volatile__("ltr %0" : : "rm" ((uint16_t)(TSS_SEGMENT)));

    KERNEL_SUCCESS("TSS Initialized at 0x%P\n", cpu_tss);
}

void cpu_init(void)
{
    /* Init the GDT, IDT and TSS */
    _cpu_setup_gdt();
    _cpu_setup_idt();
    _cpu_setup_tss();
}

/************************************ EOF *************************************/