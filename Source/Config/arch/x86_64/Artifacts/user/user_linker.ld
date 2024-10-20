/*******************************************************************************
 * roOs user linker file for x86_64 target
 *
 * Created: 29/03/2023
 *
 * Author: Alexy Torres Aurora Dugo
 *
 ******************************************************************************/

/* Entry point */
ENTRY(__user_entry)

/* Symbols */
_USER_MEMORY_START = 0x0000000000100000;
_USER_MEMORY_END   = 0x0000800000000000;

/* Memory layout */
SECTIONS
{
    . = _USER_MEMORY_START;

    /* Contains the user code */
    . = ALIGN(4K);
    .text ALIGN(4K) : AT(ADDR(.text))
    {
        _USER_START_INIT_ADDR = .;
        *(.init)
        *(.init*)
        _USER_END_INIT_ADDR = .;
        _USER_START_TEXT_ADDR = .;
        *(.text)
        *(.text*)
        _USER_END_TEXT_ADDR = .;
    }

    /* Contains the user read only data */
    . = ALIGN(4K);
    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        _USER_START_RO_DATA_ADDR = .;
        *(.rodata)
        *(.rodata*)
        *(.eh_frame)
        _USER_END_RO_DATA_ADDR = .;
    }

    /* Contains the user data */
    . = ALIGN(4K);
    .data ALIGN(4K) : AT(ADDR(.data))
    {
        _USER_START_RW_DATA_ADDR = .;
        _USER_START_DATA_ADDR = .;
        *(.data)
        *(.data*)
        _USER_END_DATA_ADDR = .;
    }
    . = ALIGN(8B);
    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        _USER_START_BSS_ADDR = .;
        *(COMMON)
        *(.bss)
        _USER_END_BSS_ADDR = .;
        _USER_END_RW_DATA_ADDR = .;
    }
}