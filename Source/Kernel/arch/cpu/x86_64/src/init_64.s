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
; kernel by mapping the first 4MB of memory 1:1 and 4MB in high memory.
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
; None

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; None

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
global _bootedCPUCount
global _kernelPGDir
global _pagingPDP
global _pagingPD

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
__stackInit:
    mov rax, _KERNEL_STACKS_BASE
    mov rbx, KERNEL_STACK_SIZE - 16
    add rax, rbx
    mov rbx, 0
    mov [rax], rbx
    mov rsp, rax
    mov rbp, rsp

    ; Update the booted CPU count
    mov rax, 1
    mov rbx, _bootedCPUCount
    mov [rbx], rax

    ; GS contains the CPU id
    mov rax, 0
    mov gs, rax
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

; Number of booted CPUs
_bootedCPUCount:
    dd 0x00000000

_kinitEndOfLine:
    db " END OF LINE "
    db 0

;-------------------------------------------------------------------------------
; Boot temporary paging structures
align 0x1000
_kernelPGDir:
    ; Keep entry 1 clear, it is used by the memory manager at boot
    times (512) dq 0x00
_pagingPDP:
    times (512) dq 0x00
_pagingPD:
    dq 0x0000000000000083
    dq 0x0000000000200083
    times (508) dq 0x00