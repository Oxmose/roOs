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
global __kinit_x86_64

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
global _kernel_multiboot_ptr
global _kernel_cpu_count

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
align 4
__kinit_x86_64:
    ; Get multiboot pointer from 32 bit stage
    mov rax, _kernel_multiboot_ptr
    mov [rax], rbx

    ; Init BSS
    mov  rbx, _END_BSS_ADDR
    mov  rdi, _START_BSS_ADDR
    xor  rsi, rsi
__bss_init:
    mov  [rdi], rsi
    add  rdi, 8
    cmp  rdi, rbx
    jb   __bss_init

    ; Init stack
    mov rax, _KERNEL_STACKS_BASE
    mov rbx, KERNEL_STACK_SIZE - 16
    add rax, rbx
    mov rsp, rax
    mov rbp, rsp

    ; Update the booted CPU count
    mov eax, 1
    mov rbx, _kernel_cpu_count
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

__kinit_x64_end:
    mov rax, 0xB8F00
    mov rbx, _kinit_end_of_line
    mov cl, 0xF0

__kinit_x64_end_print:
    mov dl, [rbx]
    cmp dl, 0
    jbe __kinit_x64_end_print_end
    mov [rax], dl
    add rax, 1
    mov [rax], cl
    add rbx, 1
    add rax, 1
    jmp __kinit_x64_end_print

__kinit_x64_end_print_end:
    ; Disable interrupt and loop forever
    cli
    hlt
    jmp __kinit_x64_end_print_end

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

section .data

_kernel_multiboot_ptr:
    dd 0x00000000
    dd 0x00000000

_kernel_cpu_count:
    dd 0x00000000

_kinit_end_of_line:
    db " END OF LINE "
    db 0