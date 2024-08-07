/*******************************************************************************
 * roOs linker file for x86 i386 target
 *
 * Created: 29/03/2023
 *
 * Author: Alexy Torres Aurora Dugo
 *
 ******************************************************************************/

/* Entry point */
ENTRY(__kinit)

/* Kernel starts at 1M in physical addresse space
 * Add the offset, to fit with values in config.h AND kinit.S
 * MUST BE 512 MB aligned
 */
KERNEL_MEM_OFFSET = 0xFFFFFFFF80000000;

/* Memory map */
MEMORY
{
    LOW_AP_STARTUP_CODE (rx) :   ORIGIN = 0x00008000,            LENGTH = 1K
    BIOS_CALL_REGION    (rwx):   ORIGIN = 0x00009000,            LENGTH = 28K
    LOW_STARTUP_CODE    (rx) :   ORIGIN = 0x00100000,            LENGTH = 1K

    KERNEL_CODE         (rx) :   ORIGIN = 0xFFFFFFFF80101000,    LENGTH = 256K
    KERNEL_RO_DATA      (r)  :   ORIGIN = 0xFFFFFFFF80141000,    LENGTH = 128K
    KERNEL_RW_DATA      (rw) :   ORIGIN = 0xFFFFFFFF80161000,    LENGTH = 64K

    /* 1 page / frame left blank to catch overflow */
    KERNEL_STACKS       (rw) :   ORIGIN = 0xFFFFFFFF80172000,    LENGTH = 32K
    /* 1 page / frame left blank to catch overflow */
    KERNEL_HEAP         (rw) :   ORIGIN = 0xFFFFFFFF8017B000,    LENGTH = 2M
    /* 1 page / frame left blank to catch overflow */
    KERNEL_END_PAGE     (r)  :   ORIGIN = 0xFFFFFFFF8037C000,    LENGTH = 1K

    KERNEL_TRACE_BUFFER (rw) :   ORIGIN = 0xFFFFFFFF8037C000,    LENGTH = 128K
    KERNEL_TEST_BUFFER  (rw) :   ORIGIN = 0xFFFFFFFF8039C000,    LENGTH = 256K
}

/* Memory layout */
SECTIONS
{
    /* Contains the startup code for secondary CPUs */
    . = ORIGIN(LOW_AP_STARTUP_CODE);
    .low_startup_ap ALIGN(4K) :
    {
        _START_LOW_AP_STARTUP_ADDR = .;
        *(.low_ap_startup_code)
        _END_LOW_AP_STARTUP_ADDR = .;
    } > LOW_AP_STARTUP_CODE

    /* Contains the bios call code and data */
    . = ORIGIN(BIOS_CALL_REGION);
    .bios_call_region ALIGN(4K) :
    {
        _START_BIOS_CALL_ADDR = .;
        *(.bios_call_code)
        . = ALIGN(16);
        *(.bios_call_data)
        _END_BIOS_CALL_ADDR = .;
    } > BIOS_CALL_REGION

    /* Contains the startup code used by grub to bootstrap the kernel */
    . = ORIGIN(LOW_STARTUP_CODE);
    .low_startup ALIGN(4K) :
    {
        _START_LOW_STARTUP_ADDR = .;
        *(.multiboot)
        . = ALIGN(16);
        *init_32.o
        _END_LOW_STARTUP_ADDR = .;
    } > LOW_STARTUP_CODE

    /* Contains the kernel code */
    . = ORIGIN(KERNEL_CODE);
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_MEM_OFFSET)
    {
        _START_TEXT_ADDR = .;
        *(.text)
        *(.text*)
        _END_TEXT_ADDR = .;
    } > KERNEL_CODE

    /* Contains the kernel read only data */
    . = ORIGIN(KERNEL_RO_DATA);
    /* Contains the DTB */
    .dtb_raw ALIGN(4K) : AT(ADDR(.dtb_raw) - KERNEL_MEM_OFFSET)
    {
        _START_RO_DATA_ADDR = .;
        _KERNEL_DEV_TREE_START = .;
        KEEP(*(.dtb_raw))
        KEEP(*(.dtb_raw*))
        _KERNEL_DEV_TREE_END = .;
    } > KERNEL_RO_DATA
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_MEM_OFFSET)
    {
        *(.rodata)
        *(.rodata*)
    } > KERNEL_RO_DATA
    .roos_driver_tbl ALIGN(4K) : AT(ADDR(.roos_driver_tbl) - KERNEL_MEM_OFFSET)
    {
        _START_DRV_TABLE_ADDR = .;
        *(.roos_driver_tbl)
        QUAD(0x0);
        _END_DRV_TABLE_ADDR = .;
    } > KERNEL_RO_DATA
    .eh_frame ALIGN(4K) : AT(ADDR(.eh_frame) - KERNEL_MEM_OFFSET)
    {
        *(.eh_frame)

        _END_RO_DATA_ADDR = .;
    } > KERNEL_RO_DATA

    /* Contains the kernel data */
    . = ORIGIN(KERNEL_RW_DATA);
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_MEM_OFFSET)
    {
        _START_RW_DATA_ADDR = .;
        _START_DATA_ADDR = .;
        *(.data)
        *(.data*)
        _END_DATA_ADDR = .;
        . = ALIGN(8);
        _START_BSS_ADDR = .;
        *(COMMON)
        *(.bss)
        _END_BSS_ADDR = .;
        _END_RW_DATA_ADDR = .;
    } > KERNEL_RW_DATA
}

/* Symbols */
_KERNEL_STACKS_BASE = ORIGIN(KERNEL_STACKS);
_KERNEL_STACKS_SIZE = LENGTH(KERNEL_STACKS);

_KERNEL_HEAP_BASE  = ORIGIN(KERNEL_HEAP);
_KERNEL_HEAP_SIZE  = LENGTH(KERNEL_HEAP);

_KERNEL_DEV_TREE_BASE = _KERNEL_DEV_TREE_START;
_KERNEL_DEV_TREE_SIZE = _KERNEL_DEV_TREE_END - _KERNEL_DEV_TREE_START;

_KERNEL_TRACE_BUFFER_BASE = ORIGIN(KERNEL_TRACE_BUFFER);
_KERNEL_TRACE_BUFFER_SIZE = LENGTH(KERNEL_TRACE_BUFFER);

_KERNEL_TEST_BUFFER_BASE = ORIGIN(KERNEL_TEST_BUFFER);
_KERNEL_TEST_BUFFER_SIZE = LENGTH(KERNEL_TEST_BUFFER);

_KERNEL_MEMORY_START = ORIGIN(LOW_STARTUP_CODE) + KERNEL_MEM_OFFSET;
_KERNEL_MEMORY_END   = ORIGIN(KERNEL_END_PAGE);