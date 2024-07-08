;-------------------------------------------------------------------------------
;
; File: initAp.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 07/06/2024
;
; Version: 1.0
;
; Kernel entry point for secondary cores. The CPU are switched to protected mode
; and the existing kernel page table is setup.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INCLUDES
;-------------------------------------------------------------------------------
%include "config.inc"

;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 16]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

%define CODE_LOCATION 0x8000
%define OFFSET_ADDR(ADDR)  (((ADDR) - __kinitApCores) + CODE_LOCATION)

%define CODE32 0x08
%define DATA32 0x10

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

extern _KERNEL_STACKS_BASE
extern _kernelPGDir
extern _bootedCPUCount
extern _fcw

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------

extern cpuApInit

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .low_ap_startup_code
;-------------------------------------------------------------------------------
; Kernel entry AP point
__kinitApCores:
    cli

    ; Set GDT
    lgdt [OFFSET_ADDR(_gdt16Ptr)]

    ; Set PE bit in cr0
    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax

    ; Jump to protected mode
    jmp dword CODE32:OFFSET_ADDR(__kinitApPM)

[bits 32]
__kinitApPM:
    ; Set Segments
    mov ax, DATA32
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    ; Map the higher half addresses
    mov eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
    mov cr3, eax

    ; Enable 4MB pages and PGE
    mov eax, cr4
    or  eax, 0x00000090
    mov cr4, eax

    ; Enable paging and write protect
    mov eax, cr0
    or  eax, 0x80010000
    mov cr0, eax

    ; Init FPU
    fninit
    fldcw [_fcw]

    ; Enable SSE
    mov eax, cr0
    and al, ~0x04
    or  al, 0x22
    mov cr0, eax
    mov eax, cr4
    or  ax, 0x600
    mov cr4, eax

    ; Get our CPU id based on the booted CPU count and update booted CPU count
    mov ecx, [_bootedCPUCount]
    mov eax, ecx
    add eax, 1
    mov [_bootedCPUCount], eax

    ; Set the stack base on the CPU id
    mov ebx, KERNEL_STACK_SIZE
    mul ebx
    mov ebx, _KERNEL_STACKS_BASE - 16
    add eax, ebx
    mov esp, eax
    mov ebp, 0

    ; Update the booted CPU count and set as call parameters
    push ecx
    mov  eax, cpuApInit
    call eax

; We should never return
__kinitApPMEnd:
    cli
    hlt
    jmp __kinitApPMEnd

; Temporary GDT for AP
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


;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .data
