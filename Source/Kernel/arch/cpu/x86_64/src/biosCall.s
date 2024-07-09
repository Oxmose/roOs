;-------------------------------------------------------------------------------
;
; File: biosCall.S
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 08/07/2024
;
; Version: 1.0
;
; Switch to real mode to do a bios call then switch back to long mode.
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

%define BIOS_CALL_STACK_SIZE 10
%define CODERM 0x0000
%define DATARM 0x0000
%define CODE64 0x0008
%define DATA64 0x0010
%define CODE32 0x0018
%define DATA32 0x0020
%define CODE16 0x0028
%define DATA16 0x0030

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern spinlockAcquire
extern spinlockRelease

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
;     Input: RSI: The interrupt number to raise
;            RDI: The address of the register array used for the call

[bits 64]
cpuBiosCall:
    ; Save flags and disable interrupts
    pushf
    cli

    ; Lock spinlock
    push rdi
    mov rdi, __biosCallLock
    call spinlockAcquire
    pop rdi

    ; Save all context
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp

    ; Get the interrupt number and set it in the code directly
    mov [_biosCallIntIssue], si

    ; Copy the data to the 16bits stack
    mov [__savedRegPointer], rdi
    mov rsi, rdi
    mov rdi, __16BitsStackEnd
    mov rcx, BIOS_CALL_STACK_SIZE
    sub rdi, rcx
    rep movsb

    ; Save the current Stack, IDT and GDT
    mov [__savedStackPointer], rsp
    sgdt [__savedGDTPointer]
    sidt [__savedIDTPointer]

    ; Move to 16 bits stack
    mov esp, __16BitsStackEnd
    sub esp, BIOS_CALL_STACK_SIZE
    mov ebp, 0

    ; Disable paging
    mov rax, cr0
    and rax, 0x7FFEFFFF
    mov cr0, rax

    ; Load the 16bit Real Mode GDT
    lgdt [_gdt16Ptr]
    lea rax, [_biosCall16BitsGdtSetup]
    push CODE16
    push rax
    retfq

_biosCall16BitsGdtSetup:
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

    ; Get the registers
    pop ax
    pop bx
    pop cx
    pop dx

    db 0xCD  ; INT OPCODE
_biosCallIntIssue:
    db 0x00

    ; Save the registers and flags
    push ax
    push bx
    push cx
    push dx
    pushf

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

    ; Jump to long mode
    jmp dword CODE64:_biosCallLongMode

[bits 64]
_biosCallLongMode:
    mov rax, DATA64
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Restore the initial Stack, IDT and GDT
    lgdt [__savedGDTPointer]
    lidt [__savedIDTPointer]
    mov rsp, [__savedStackPointer]

    ; Copy the data to the 16bits stack
    mov rsi, __16BitsStackEnd
    mov rdi, [__savedRegPointer]
    mov rcx, BIOS_CALL_STACK_SIZE
    sub rsi, rcx
    rep movsb

    ; Restore all context
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Release spinlock
    mov rdi, __biosCallLock
    call spinlockRelease

    ; Restore flags
    popf
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .bios_call_data

; Temporary GDT for Bios Calls
_gdt16:
    .null:
        dd 0x00000000
        dd 0x00000000
    .code_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xAF
        db 0x00
    .data_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0xCF
        db 0x00
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

; BIOS IVT pointer
_idt16Ptr:
    dw 0x03FF                              ; IVT limit
    dd 0x00000000                          ; IVT base address

__16BitsStack:
    times 0x200 db 0x00 ; 512B stack
__16BitsStackEnd:

__biosCallLock:
    dd 0x00000000

__savedStackPointer:
    dd 0x00000000
    dd 0x00000000

__savedIDTPointer:
    dd 0x00000000
    dd 0x00000000

__savedGDTPointer:
    dd 0x00000000
    dd 0x00000000

__savedRegPointer:
    dd 0x00000000
    dd 0x00000000

;************************************ EOF **************************************