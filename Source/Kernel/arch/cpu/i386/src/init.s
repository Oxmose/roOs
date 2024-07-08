;-------------------------------------------------------------------------------
;
; File: init.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 20/03/2023
;
; Version: 1.0
;
; Kernel entry point and CPU initialization. This module setup high-memory
; kernel by mapping the first 4MB of memory 1:1 and 16MB in high memory.
; Paging is enabled.
; BSS is initialized (zeroed)
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INCLUDES
;-------------------------------------------------------------------------------
%include "config.inc"

;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

; Multiboot header values
FLAGS       equ ((1<<0) | (1<<1))
MAGIC       equ 0xE85250D6
CHECKSUM    equ -(MAGIC + FLAGS)

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; Kernel memory layout
KERNEL_START_PAGE_ID equ (KERNEL_MEM_OFFSET >> 22)
__kinitLow           equ (__kinit - KERNEL_MEM_OFFSET)

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern _START_BSS_ADDR
extern _END_BSS_ADDR
extern _KERNEL_STACKS_BASE

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern kickstart

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global __kinit
global __kinitLow

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
global _bootedCPUCount
global _kernelPGDir
global _fcw

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Multiboot header
section .multiboot
align 8
multiboot_header_start:
    dd MAGIC
    dd 0x00000000
    dd multiboot_header_end - multiboot_header_start
    dd -(MAGIC + (multiboot_header_end - multiboot_header_start))
align 8
    ; info request
    dw 1
    dw 0
    dd 4 * 2 + 8
    dd 3
    dd 6
align 8
    ; module align
    dw 6
    dw 0
    dd 8
; Multiboot terminate tag
align 8
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .low_startup_code

;-------------------------------------------------------------------------------
; Kernel entry point
__kinit:
    ; Make sure interrupts are disabled
    cli

    ; Map the higher half addresses
    mov eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
    mov cr3, eax

    ; Enable 4MB pages and PGE
    mov eax, cr4
    or  eax, 0x00000090
    mov cr4, eax

    ; Enable paging and write protect
    mov eax, cr0
    and eax, 0x7FFFFFFF
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

    ; Init BSS
    mov  edi, _START_BSS_ADDR
    xor  esi, esi
__bssInit:
    mov  [edi], esi
    add  edi, 4
    cmp  edi, _END_BSS_ADDR
    jb   __bssInit

    ; Load high mem kernel entry point
    mov eax, __kinitHigh
    jmp eax

section .text
align 4
; High memory loader
__kinitHigh:
    ; Init stack
    mov eax, _KERNEL_STACKS_BASE
    mov ebx, KERNEL_STACK_SIZE - 16
    add eax, ebx
    mov esp, eax
    mov ebp, 0

    ; Update the booted CPU count
    mov eax, 1
    mov [_bootedCPUCount], eax

    call kickstart

__kinitEnd:
    mov eax, 0xB8F00
    mov ebx, _kinitEndOfLine
    mov cl, 0xF0

__kinitEndPrint:
    mov dl, [ebx]
    cmp dl, 0
    jbe __kinitEndPrintEnd
    mov [eax], dl
    add eax, 1
    mov [eax], cl
    add ebx, 1
    add eax, 1
    jmp __kinitEndPrint

__kinitEndPrintEnd:
    ; Disable interrupt and loop forever
    cli
    hlt
    jmp __kinitEnd

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .data

;-------------------------------------------------------------------------------
; Kernel initial page directory:
; 16MB mapped for high addresses
; 4MB mapped 1-1 for low addresses
align 0x1000
_kernelPGDir:
    ; First 4MB R/W Present.
    dd 0x00000083
    ; Keep this entry free, it is used by the memory manager at boot
    dd 0x00000000
    ; Pages before kernel space.
    times (KERNEL_START_PAGE_ID - 2) dd 0
    ; This page directory entry defines a 4MB page containing the kernel.
    dd 0x00000083
    ; Pages after the kernel.
    times (1024 - KERNEL_START_PAGE_ID - 1) dd 0

; Number of booted CPUs
_bootedCPUCount:
    dd 0x00000000

_kinitEndOfLine:
    db " END OF LINE "
    db 0

_fcw:
    dw 0x037F