;-------------------------------------------------------------------------------
;
; File: biosCall.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 08/07/2024
;
; Version: 1.0
;
; Switch to real mode to do a bios call then switch back to protected mode.
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

%define BIOS_CALL_STACK_SIZE 16
%define BIOS_CALL_DATA_SIZE 0x1000
%define CODERM 0x0000
%define DATARM 0x0000
%define CODE32 0x0008
%define DATA32 0x0010
%define CODE16 0x0018
%define DATA16 0x0020

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpuBiosCall

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------
section .bios_call_code

;-------------------------------------------------------------------------------
; Bios Call function
;
; Param:
;     Input: ESP + 4: The address of the register array used for the call
;            ESP + 8: The interrupt number to raise
;            ESP + 12: The buffer to receive produced data
;            ESP + 16: The size for the buffer to receive produced data
;            ESP + 20: The initial data location pointer

[bits 32]
cpuBiosCall:
    ; Save flags and disable interrupts
    pushf
    cli

    ; Save all context
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    mov eax, [esp + 36]
    mov [__savedRegPointer], eax
    mov eax, [esp + 44]
    mov [__savedBufferAddr], eax
    mov eax, [esp + 48]
    mov [__savedBufferSize], eax
    mov eax, [esp + 52]
    mov [__savedInitialLocationPtr], eax

    mov ecx, [esp + 48]
    cmp ecx, 0
    je _biosCallDataToCopyEnd

    ; Check the maximal size
    cmp ecx, BIOS_CALL_DATA_SIZE
    jbe _biosCallDataToCopy

    ; If greater, we copy the max size
    mov ecx, BIOS_CALL_DATA_SIZE

_biosCallDataToCopy:
    ; Copy the pre-call data
    mov esi, [esp + 44]
    mov edi, __savedDataBuffer
    rep movsb

_biosCallDataToCopyEnd:
    ; Get the interrupt number and set it in the code directly
    mov eax, [esp + 40]
    mov [_biosCallIntIssue], al

    ; Copy the data to the 16bits stack
    mov esi, [esp + 36]
    mov edi, __16BitsStackEnd
    mov ecx, BIOS_CALL_STACK_SIZE
    sub edi, ecx
    rep movsb

    ; Save the current Stack, IDT and GDT
    mov [__savedStackPointer], esp
    sgdt [__savedGDTPointer]
    sidt [__savedIDTPointer]

    ; Move to 16 bits stack
    mov esp, __16BitsStackEnd
    sub esp, BIOS_CALL_STACK_SIZE
    mov ebp, 0

    ; Disable paging
    mov eax, cr0
    and eax, 0x7FFEFFFF
    mov cr0, eax

    ; Load the 16bit Real Mode GDT
    lgdt [_gdt16Ptr]
    jmp word CODE16:_biosCall16BitsGdt

_biosCall16BitsGdt:
[bits 16]
    mov ax, DATA16
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Load IVT
    lidt [_idt16Ptr]

    ; Disable protected mode
    mov  eax, cr0
    and  al,  ~0x01
    mov  cr0, eax

    ; Jump to real mode
    jmp word CODERM:_biosCallRealMode

_biosCallRealMode:
    mov ax, DATARM
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Set destination register for data
    lea ax, [__savedDataBuffer]
    mov di, ax

    ; Get the registers
    pop ax
    pop bx
    pop cx
    pop dx

    db 0xCD  ; INT OPCODE
_biosCallIntIssue:
    db 0x00
    nop
    nop
    nop
    nop

    ; Save the registers and flags
    add sp, 2
    pushf
    push dx
    push cx
    push bx
    push ax

    ; Enable protected mode
    mov  eax, cr0
    or   eax, 0x00000001
    mov  cr0, eax

    ; Jump to protected mode
    jmp dword CODE32:_biosCallProtecteMode

[bits 32]
_biosCallProtecteMode:
    mov eax, DATA32
    mov ds, eax
    mov es, eax
    mov ss, eax
    mov fs, eax
    mov gs, eax

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80010000
    mov cr0, eax

    ; Restore the initial Stack, IDT and GDT
    lgdt [__savedGDTPointer]
    lidt [__savedIDTPointer]
    mov esp, [__savedStackPointer]

    ; Copy the data to the 16bits stack
    mov esi, __16BitsStackEnd
    mov edi, [__savedRegPointer]
    mov ecx, BIOS_CALL_STACK_SIZE
    sub esi, ecx
    rep movsb

    ; Check if some data need to be copied from the save data area
    mov ecx, [__savedBufferSize]
    cmp ecx, 0
    je _biosCallEnd

    ; Check the maximal size
    cmp ecx, BIOS_CALL_DATA_SIZE
    jbe _biosCallDataFromCopy

    ; If greater, we copy the max size
    mov ecx, BIOS_CALL_DATA_SIZE

_biosCallDataFromCopy:
    ; Copy the produced data
    mov esi, __savedDataBuffer
    mov edi, [__savedBufferAddr]
    rep movsb

    ; Copy the initial data location
    mov eax, [__savedInitialLocationPtr]
    lea ebx, [__savedDataBuffer]
    mov [eax], ebx

_biosCallEnd:
    ; Restore all context
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax

    ; Restore flags
    popf
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .bios_call_data
align 16
; Temporary GDT for Bios Calls
_gdt16:
    .null:
        dd 0x00000000
        dd 0x00000000
    .code32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xCF
        db 0x00
    .data32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0xCF
        db 0x00
    .code16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0x0F
        db 0x00
    .data16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0x0F
        db 0x00
_gdt16Ptr:                                 ; GDT pointer for 16bit access
    dw _gdt16Ptr - _gdt16 - 1              ; GDT limit
    dd _gdt16                              ; GDT base address

align 16
; BIOS IVT pointer
_idt16Ptr:
    dw 0x03FF                              ; IVT limit
    dd 0x00000000                          ; IVT base address

align 16
__16BitsStack:
    times 0x200 db 0x00 ; 512B stack
__16BitsStackEnd:

__savedStackPointer:
    dd 0x00000000

__savedIDTPointer:
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000

__savedGDTPointer:
    dd 0x00000000
    dd 0x00000000
    dd 0x00000000

__savedRegPointer:
    dd 0x00000000
    dd 0x00000000

__savedBufferAddr:
    dd 0x00000000
    dd 0x00000000

__savedBufferSize:
    dd 0x00000000
    dd 0x00000000

__savedInitialLocationPtr:
    dd 0x00000000
    dd 0x00000000

__savedDataBuffer:
    times BIOS_CALL_DATA_SIZE db 0x00 ; Saved data

;************************************ EOF **************************************