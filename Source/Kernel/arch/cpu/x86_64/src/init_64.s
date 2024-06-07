;-------------------------------------------------------------------------------
;
; File: init64.s
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
[bits 64]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

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
global __kinitx86_64

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
align 4
__kinitx86_64:
    ; Init BSS
    mov  rbx, _END_BSS_ADDR
    mov  rdi, _START_BSS_ADDR
    xor  rsi, rsi
__bssInit:
    mov  [rdi], rsi
    add  rdi, 8
    cmp  rdi, rbx
    jb   __bssInit

    ; Init stack
    mov rax, _KERNEL_STACKS_BASE
    mov rbx, KERNEL_STACK_SIZE - 16
    add rax, rbx
    mov rsp, rax
    mov rbp, rsp

    ; Update the booted CPU count
    mov eax, 1
    mov rbx, _bootedCPUCount
    mov [rbx], eax

    ; Enable SSE
    fninit
    mov rax, cr0
    and rax, 0xFFFFFFFFFFFFFFFB
    or  rax, 0x0000000000000002
    mov cr0, rax
    mov rax, cr4
    or  rax, 0x00000600
    mov cr4, rax

    call kickstart

__kinitx64End:
    mov rax, 0xB8F00
    mov rbx, _kinitEndOfLine
    mov cl, 0xF0

__kinitx64EndPrint:
    mov dl, [rbx]
    cmp dl, 0
    jbe __kinitx64EndPrintEnd
    mov [rax], dl
    add rax, 1
    mov [rax], cl
    add rbx, 1
    add rax, 1
    jmp __kinitx64EndPrint

__kinitx64EndPrintEnd:
    ; Disable interrupt and loop forever
    cli
    hlt
    jmp __kinitx64EndPrintEnd

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

section .data

_bootedCPUCount:
    dd 0x00000000

_kinitEndOfLine:
    db " END OF LINE "
    db 0