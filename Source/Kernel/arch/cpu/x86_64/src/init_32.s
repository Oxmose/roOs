;-------------------------------------------------------------------------------
;
; File: init32.s
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
__kinitLow        equ (__kinit - KERNEL_MEM_OFFSET)

; Kernel memory layout
KERNEL_PML4_ENTRY equ (KERNEL_MEM_OFFSET >> 39)
KERNEL_PDP_ENTRY  equ (KERNEL_MEM_OFFSET >> 30)

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern __kinitx86_64
extern _kernelPGDir
extern _pagingPDP
extern _pagingPD

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global __kinit
global __kinitLow

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
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

section .text
align 4
;-------------------------------------------------------------------------------
; Kernel entry point
__kinit:
    ; Make sure interrupts are disabled
    cli

    ; Check for 64 bits compatibility
    jmp __kinitCheckLMAvailable

__kinitLMAvailable:
    ; Set 64 bits capable GDT
    lgdt [_gdtTmpPtr]

    ; Update code segment
    jmp dword (_gdtTmp.code_32 - _gdtTmp):__kinitInitLM

__kinitInitLM:
    ; Update data segments
    mov eax, (_gdtTmp.data_32 - _gdtTmp)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

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

__kinitInitPGdir:
    ; Init the page directory using its physical address
    mov ebx, (_pagingPDP - KERNEL_MEM_OFFSET)
    mov ecx, (_pagingPD - KERNEL_MEM_OFFSET)

    ; Set first 4MB in PDP
    or ecx, 0x3
    mov [ebx], ecx

    ; Set high 4MB in PDP
    mov eax, KERNEL_PDP_ENTRY
    and eax, 0x1FF
    mov edx, 8
    mul edx
    add ebx, eax
    mov [ebx], ecx

    ; Set PML4
    mov ebx, (_pagingPDP - KERNEL_MEM_OFFSET)
    or ebx, 0x3
    mov eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
    mov [eax], ebx

    mov eax, KERNEL_PML4_ENTRY
    and eax, 0x1FF
    mov ecx, 8
    mul ecx
    add eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
    mov [eax], ebx

    ; Enable PAE and PGE
    mov eax, cr4
    or  eax, 0xA0
    mov cr4, eax

    ; Switch to compatibility mode, enable NXE and SYSCALL/SYSRET
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 0x00000901
    wrmsr

    ; Set CR3
    mov eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    and eax, 0x0FFFFFFF
    or  eax, 0x80010000
    mov cr0, eax

    ; Far jump to 64 bit mode
    jmp (_gdtTmp.code_64 - _gdtTmp):__kinit64bEntry

;-------------------------------------------------------------------------------
; ARCH
[bits 64]

__kinit64bEntry:
    mov rax, __kinitx86_64
    jmp rax

__kinitEnd:
    ; Disable interrupt and loop forever
    cli
    hlt
    jmp __kinitEnd


;-------------------------------------------------------------------------------
; ARCH
[bits 32]

;-------------------------------------------------------------------------------
; Long mode availability test
__kinitCheckLMAvailable:
    ; Setup mini stack
    mov eax, _miniStackEnd
    sub eax, 8
    mov esp, eax

    ; 1. Check CPUID availability

    ; Get flags
    pushfd
    pop eax

    ; Update flags
    mov  ebx, eax
    xor  eax, 0x00200000
    push eax
    popfd

    ; Get flags
    pushfd
    pop eax

    ; Update back flags
    push ebx
    popfd

    ; Compare eax and ecx, should have one bit flipped
    cmp eax, ecx
    je  __kinitLMUnAvailable

    ; 2. Check CPUID extended features
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb  __kinitLMUnAvailable

    ; 3. Detect LM with CPUID
    mov eax, 0x80000001
    cpuid
    and edx, 0x20000000
    cmp edx, 0
    je __kinitLMUnAvailable

    jmp __kinitLMAvailable

__kinitLMUnAvailable:
    mov eax, 0
    mov ebx, 0
    mov ecx, _noLMMessage
    jmp __kinitKernelError

;-------------------------------------------------------------------------------
; Error output function
__kinitKernelError: ; Print string pointed by BX
    ; Compute start address
    mov edx, 160
    mul edx      ; eax now contains the offset in lines
    add eax, ebx ; Offset is half complete
    add eax, ebx ; Offset is complete

    add eax, 0xB8000 ; VGA buffer address
    mov ebx, ecx

__kinitKernelErrorLoop:      ; Display loop
    mov cl, [ebx]            ; Load character into cx
    cmp cl, 0                 ; Compare for NULL end
    je __kinitKernelErrorEnd ; If NULL exit
    mov ch, 0x0007
    mov [eax], cx             ; Printf current character
    add eax, 2                ; Move cursor

    add ebx, 1                   ; Move to next character
    jmp __kinitKernelErrorLoop ; Loop

__kinitKernelErrorEnd:
    cli
    hlt
    jmp  __kinitKernelErrorEnd


;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

section .data

_noLMMessage:
    db "Your system is not 64 bits compatible, use the 32 bits version of roOs"
    db 0

;-------------------------------------------------------------------------------
; Boot temporary GDT
align 32
_gdtTmp:
    .null:
        dd 0x00000000
        dd 0x00000000
    .code_32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xCF
        db 0x00
    .data_32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0xCF
        db 0x00
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

_gdtTmpPtr:                     ; GDT pointer for 16bit access
    dw _gdtTmpPtr - _gdtTmp - 1 ; GDT limit
    dd _gdtTmp                    ; GDT base address

_miniStack:
    times 0x20 db 0x00
_miniStackEnd:

_fcw:
    dw 0x037F