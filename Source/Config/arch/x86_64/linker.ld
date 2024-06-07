/*******************************************************************************
 * UTK linker file for x86 i386 target
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
KERNEL_MEM_OFFSET = 0xFFFFFFF800000000;

/* Memory map */
MEMORY
{
    LOW_STARTUP_CODE        (rx)    :   ORIGIN = 0x00100000,            LENGTH = 32K

    KERNEL_CODE             (rx)    :   ORIGIN = 0xFFFFFFF800110000,    LENGTH = 256K
    KERNEL_RO_DATA          (r)     :   ORIGIN = 0xFFFFFFF800150000,    LENGTH = 64K
    KERNEL_RW_DATA          (rw)    :   ORIGIN = 0xFFFFFFF800160000,    LENGTH = 64K
    KERNEL_DEVTREE_MEM      (rw)    :   ORIGIN = 0xFFFFFFF800170000,    LENGTH = 128K

    KERNEL_TRACE_BUFFER     (rw)    :   ORIGIN = 0xFFFFFFF800180000,    LENGTH = 128K
    KERNEL_TEST_BUFFER      (rw)    :   ORIGIN = 0xFFFFFFF8001A0000,    LENGTH = 256K

    KERNEL_STACKS           (rw)    :   ORIGIN = 0xFFFFFFF800200000,    LENGTH = 64K
    KERNEL_HEAP             (rw)    :   ORIGIN = 0xFFFFFFF800210000,    LENGTH = 10M
}

/* Memory layout */
SECTIONS
{
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
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_MEM_OFFSET)
    {
        _START_RO_DATA_ADDR = .;

        *(.rodata)
        *(.rodata*)

    } > KERNEL_RO_DATA
    .utk_driver_tbl ALIGN(4K) : AT(ADDR(.utk_driver_tbl) - KERNEL_MEM_OFFSET)
    {
        _START_DRV_TABLE_ADDR = .;
        *(.utk_driver_tbl)
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
        _START_DATA_ADDR = .;

        *(.data)
        *(.data*)

        _END_DATA_ADDR = .;
    } > KERNEL_RW_DATA

    /* Contains the kernel BSS */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_MEM_OFFSET)
    {
        _START_BSS_ADDR = .;

        *(COMMON)
        *(.bss)

        _END_BSS_ADDR = .;
    } > KERNEL_RW_DATA

    . = ORIGIN(KERNEL_DEVTREE_MEM);
    /* Contains the DTB (TODO: load as separate file ) */
    .dtb_raw ALIGN(4K) : AT(ADDR(.dtb_raw) - KERNEL_MEM_OFFSET)
    {
        KEEP(*(.dtb_raw))
        KEEP(*(.dtb_raw*))
    } > KERNEL_DEVTREE_MEM
}

/* Symbols */
_KERNEL_STACKS_BASE = ORIGIN(KERNEL_STACKS);
_KERNEL_STACKS_SIZE = LENGTH(KERNEL_STACKS);

_KERNEL_HEAP_BASE  = ORIGIN(KERNEL_HEAP);
_KERNEL_HEAP_SIZE  = LENGTH(KERNEL_HEAP);

_KERNEL_DEV_TREE_BASE = ORIGIN(KERNEL_DEVTREE_MEM);
_KERNEL_DEV_TREE_SIZE = LENGTH(KERNEL_DEVTREE_MEM);

_KERNEL_TRACE_BUFFER_BASE = ORIGIN(KERNEL_TRACE_BUFFER);
_KERNEL_TRACE_BUFFER_SIZE = LENGTH(KERNEL_TRACE_BUFFER);

_KERNEL_TEST_BUFFER_BASE = ORIGIN(KERNEL_TEST_BUFFER);
_KERNEL_TEST_BUFFER_SIZE = LENGTH(KERNEL_TEST_BUFFER);

_KERNEL_MEMORY_END = _KERNEL_HEAP_BASE + _KERNEL_HEAP_SIZE;