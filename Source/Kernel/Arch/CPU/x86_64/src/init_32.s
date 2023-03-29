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

; Multiboot header values
FLAGS       equ ((1<<0) | (1<<1))
MAGIC       equ 0xE85250D6
CHECKSUM    equ -(MAGIC + FLAGS)

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; Kernel memory layout
KERNEL_PML4_ENTRY    equ (KERNEL_MEM_OFFSET >> 39)
KERNEL_PDPT_ENTRY    equ (KERNEL_MEM_OFFSET >> 30)
KERNEL_PDT_ENTRY     equ (KERNEL_MEM_OFFSET >> 21)
__kinit_low          equ (__kinit - KERNEL_MEM_OFFSET)

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern __kinit_x86_64

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global __kinit
global __kinit_low

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; ARCH
[bits 32]

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
    ; Make sure interrupts are disabled and clear flags
    push  0
    popfd

    ; Save Multiboot info to memory
    mov [__multiboot_data_ptr_tmp], ebx

    ; Set temporary stack
    mov esp, _kinit_stack_end
    mov ebp, esp

    ; Check for 64 bits compatibility
    call __kinit_check_lm_available

    ; Set 64 bits capable GDT
    lgdt [_gdt_tmp_ptr]

    ; Update code segment
    jmp dword (_gdt_tmp.code_32 - _gdt_tmp):__kinit_set_segments

__kinit_set_segments:
    ; Update data segments
    mov eax, (_gdt_tmp.data_32 - _gdt_tmp)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

__kinit_init_pml4t:
    ; Set PML4T
    mov ebx, _paging_pdpt0             ; First entry to PDPT0
    or  ebx, 0x3                       ; Present and writable
    mov eax, _paging_pml4t
    mov [eax], ebx

    xor ebx, ebx
__kinit_blank_pml4t:
    add eax, 8
    cmp eax, _paging_pdpt0
    mov [eax], ebx
    jne __kinit_blank_pml4t

    ; Set PDPT0
    mov ebx, _paging_pdt0             ; First entry to PDT0
    or  ebx, 0x3                      ; Present and writable
    mov eax, _paging_pdpt0
    mov [eax], ebx

    xor ebx, ebx
__kinit_blank_pdpt:
    add eax, 8
    cmp eax, _paging_pdt0
    mov [eax], ebx
    jne __kinit_blank_pdpt

    ; Set PDPT0
    mov ebx, 0x83                       ; Present, writable, 2MB page
    mov eax, _paging_pdt0
__kinit_init_pdt0:
    mov [eax], ebx
    add eax, 8
    add ebx, 0x200000
    xor ebx, ebx                        ; Rest entries will be not present
    cmp eax, (_paging_pdt0 + 0x1000)
    jne __kinit_init_pdt0

__kinit_init_pml4tk:
    ; Set PML4T for kernel
    mov eax, KERNEL_PML4_ENTRY
    and eax, 0x1FF
    shl eax, 3

    ; If it is the same as the loader, skip
    mov ebx, _paging_pdpt0
    cmp eax, 0
    je  __kinit_init_pdptk

    add eax, _paging_pml4t
    mov ebx, _paging_pdptk
    or  ebx, 0x3                ; Present and writable
    mov [eax], ebx

__kinit_init_pdptk:
    and ebx, 0xFFFFFF00
    ; Set the PDPT for kernel
    mov eax, KERNEL_PDPT_ENTRY
    and eax, 0x1FF
    shl eax, 3

    add eax, ebx
    mov ebx, _paging_pdtk
    or  ebx, 0x3                ; Present and writable
    mov [eax], ebx

__kinit_init_pdtk:
    ; Set the PDT entries for kernel
    and ebx, 0xFFFFFF00
    mov eax, KERNEL_PDT_ENTRY
    and eax, 0x1FF
    shl eax, 3
    add eax, ebx
    mov ecx, eax
    add ecx, 0x800
    mov ebx, 0x83                     ; Present, writable, 2MB page

__kinit_init_pdtk_loop:
    ; Set the rest of the entries to reach 512Mb mapping
    mov [eax], ebx
    add eax, 8
    add ebx, 0x200000
    cmp eax, ecx
    jne __kinit_init_pdtk_loop

__kinit_init_lm:
    ; Long mode switch
    ; Set CR3
    mov eax, _paging_pml4t
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or  eax, 0x20
    mov cr4, eax

    ; Switch to compatibility mode
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 0x00000100
    wrmsr

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80010000
    mov cr0, eax

    ; Far jump to 64 bit mode
    jmp (_gdt_tmp.code_64 - _gdt_tmp):__kinit_64b_entry

;-------------------------------------------------------------------------------
; ARCH
[bits 64]

__kinit_64b_entry:
    ; Save multiboot information
    mov rbx, [__multiboot_data_ptr_tmp]
    mov rax, __kinit_x86_64
    jmp rax

__kinit_end:
    ; Disable interrupt and loop forever
    cli
    hlt
    jmp __kinit_end


;-------------------------------------------------------------------------------
; ARCH
[bits 32]

;-------------------------------------------------------------------------------
; Long mode availability test
__kinit_check_lm_available:
    push eax
    push ebx
    push ecx
    push edx

    ; 1) Check CPUID availability

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
    je  __kinit_lm_unavailable


    ; 2) Check CPUID extended features
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb  __kinit_lm_unavailable


    ; 3) Detect LM with CPUID
    mov eax, 0x80000001
    cpuid
    and edx, 0x20000000
    cmp edx, 0
    je __kinit_lm_unavailable

    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

__kinit_lm_unavailable:
    mov eax, 0
    mov ebx, 0
    mov ecx, _no_lm_msg
    call __kinit_kernel_error
__kinit_lm_unavailable_loop:
    cli
    hlt
    jmp  __kinit_lm_unavailable_loop

;-------------------------------------------------------------------------------
; Error output function
__kinit_kernel_error: ; Print string pointed by BX
    pusha       ; Save registers

    ; Compute start address
    mov edx, 160
    mul edx      ; eax now contains the offset in lines
    add eax, ebx ; Offset is half complete
    add eax, ebx ; Offset is complete

    add eax, 0xB8000 ; VGA buffer address
    mov ebx, ecx

__kinit_kernel_error_loop:      ; Display loop
    mov cl, [ebx]            ; Load character into cx
    cmp cl, 0                 ; Compare for NULL end
    je __kinit_kernel_error_end ; If NULL exit
    mov ch, 0x0007
    mov [eax], cx             ; Printf current character
    add eax, 2                ; Move cursor

    add ebx, 1                   ; Move to next character
    jmp __kinit_kernel_error_loop ; Loop

__kinit_kernel_error_end:
    popa ; Restore registers
    ret

;-
;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

section .data

__multiboot_data_ptr_tmp:
    dd 0x00000000
    dd 0x00000000

_no_lm_msg:
    db "System not 64 bits compatible, use the 32 bits version of UTK", 0

;-------------------------------------------------------------------------------
; Boot temporary GDT
align 32
_gdt_tmp:
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

    .code_16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0x0F
        db 0x00

    .data_16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0x0F
        db 0x00

    .code_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x98
        db 0x20
        db 0x00

    .data_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x90
        db 0x20
        db 0x00

_gdt_tmp_ptr:                      ; GDT pointer for 16bit access
    dw _gdt_tmp_ptr - _gdt_tmp - 1 ; GDT limit
    dd _gdt_tmp                    ; GDT base address

;-------------------------------------------------------------------------------
; Boot temporary paging structures
align 0x1000
_paging_pml4t:
    times 0x1000 db 0x00
_paging_pdpt0:
    times 0x1000 db 0x00
_paging_pdptk:
    times 0x1000 db 0x00
_paging_pdt0:
    times 0x1000 db 0x00
_paging_pdtk:
    times 0x1000 db 0x00

section .bss

_kinit_stack_start:
    resb 0x200
_kinit_stack_end: