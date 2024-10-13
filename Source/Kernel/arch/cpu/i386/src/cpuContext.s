;-------------------------------------------------------------------------------
;
; File: cpuContext.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 16/06/2024
;
; Version: 1.0
;
; CPU context manager for i386
; Allows saving and restoring the CPU context.
;
;
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------


; The following define shall correspond to the first TSS entry in the GDT
%define FIRST_GDT_TSS_SEGMENT 0x28

; The following defines shall correspond to the virtual CPU context
%define VCPU_OFF_INT 0x00
%define VCPU_OFF_ERR 0x04
%define VCPU_OFF_EIP 0x08
%define VCPU_OFF_CS  0x0C
%define VCPU_OFF_FLG 0x10
%define VCPU_OFF_ESP 0x14
%define VCPU_OFF_EBP 0x18
%define VCPU_OFF_EDI 0x1C
%define VCPU_OFF_ESI 0x20
%define VCPU_OFF_EDX 0x24
%define VCPU_OFF_ECX 0x28
%define VCPU_OFF_EBX 0x2C
%define VCPU_OFF_EAX 0x30
%define VCPU_OFF_SS  0x34
%define VCPU_OFF_GS  0x38
%define VCPU_OFF_FS  0x3C
%define VCPU_OFF_ES  0x40
%define VCPU_OFF_DS  0x44
%define VCPU_OFF_FXD 0x48

%define VCPU_OFF_SAVED 0x258

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern pCurrentThreadsPtr

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern schedScheduleNoInt

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpuSaveContext
global cpuRestoreContext
global cpuGetId
global cpuSignalHandler

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------
section .text

cpuSaveContext:
    ; Save a bit of context
    push eax
    push ebx
    push edx

    ; Get the current thread handle and VCPU
    call cpuGetId
    mov ebx, 4
    mul ebx
    mov ebx, pCurrentThreadsPtr
    add eax, ebx
    mov eax, [eax]
    mov eax, [eax]

    ; Restore EDX used with mul
    pop edx

    ; Save the interrupt context
    mov ebx, [esp + 12]  ; Int id
    mov [eax + VCPU_OFF_INT], ebx
    mov ebx, [esp + 16]  ; Int code
    mov [eax + VCPU_OFF_EIP], ebx
    mov ebx, [esp + 20]  ; EIP
    mov [eax + VCPU_OFF_EIP], ebx
    mov ebx, [esp + 24]  ; CS
    mov [eax + VCPU_OFF_CS], ebx
    mov ebx, [esp + 28]  ; EFLAGS
    mov [eax + VCPU_OFF_FLG], ebx

    ; Save CPU state before calling interrupt
    mov ebx, esp       ; ESP before int save
    add ebx, 32
    mov [eax + VCPU_OFF_ESP], ebx

    mov [eax + VCPU_OFF_EBP], ebp
    mov [eax + VCPU_OFF_EDI], edi
    mov [eax + VCPU_OFF_ESI], esi
    mov [eax + VCPU_OFF_EDX], edx
    mov [eax + VCPU_OFF_ECX], ecx

    pop ebx                         ; restore prelude ebx
    mov [eax + VCPU_OFF_EBX], ebx
    pop ebx                         ; restore prelude eax
    mov [eax + VCPU_OFF_EAX], ebx

    mov [eax + VCPU_OFF_SS], ss
    mov [eax + VCPU_OFF_GS], gs
    mov [eax + VCPU_OFF_FS], fs
    mov [eax + VCPU_OFF_ES], es
    mov [eax + VCPU_OFF_DS], ds

    ; Save the FxData
    mov ebx, eax
    add ebx, VCPU_OFF_FXD
    add ebx, 0xF                   ; ALIGN Region
    and ebx, 0xFFFFFFF0            ; ALIGN Region
    fxsave [ebx]

    ; Set the last context as saved
    mov ebx, 1
    mov [eax + VCPU_OFF_SAVED], ebx

    ret

cpuRestoreContext:
    ; The current thread is sent as parameter, load the VCPU
    mov eax, [esp + 4]
    mov eax, [eax]

    ; Set the last context as not been saved
    mov ebx, 0
    mov [eax + VCPU_OFF_SAVED], ebx

    ; Restore the FxData
    mov ebx, eax
    add ebx, VCPU_OFF_FXD
    add ebx, 0xF                   ; ALIGN Region
    and ebx, 0xFFFFFFF0            ; ALIGN Region
    fxrstor [ebx]

    ; Restore registers
    mov gs,  [eax + VCPU_OFF_GS]
    mov fs,  [eax + VCPU_OFF_FS]
    mov es,  [eax + VCPU_OFF_ES]
    mov ds,  [eax + VCPU_OFF_DS]
    mov ss,  [eax + VCPU_OFF_SS]

    mov ecx, [eax + VCPU_OFF_ECX]
    mov edx, [eax + VCPU_OFF_EDX]
    mov esi, [eax + VCPU_OFF_ESI]
    mov edi, [eax + VCPU_OFF_EDI]
    mov ebp, [eax + VCPU_OFF_EBP]
    mov esp, [eax + VCPU_OFF_ESP]

    ; Restore the interrupt context
    sub esp, 12
    mov ebx, [eax + VCPU_OFF_EIP]  ; EIP
    mov [esp], ebx
    mov ebx, [eax + VCPU_OFF_CS]  ; CS
    mov [esp + 4], ebx
    mov ebx, [eax + VCPU_OFF_FLG]  ; EFLAGS
    mov [esp + 8], ebx

    mov ebx, [eax + VCPU_OFF_EBX]
    mov eax, [eax + VCPU_OFF_EAX]

    ; Return from interrupt
    iret

cpuGetId:
    ; Get the TSS segment
    str eax
    sub eax, FIRST_GDT_TSS_SEGMENT
    shr eax, 3
    ret

cpuSignalHandler:
    ; Get the function to execute
    pop eax

    ; Force stack alignement
    and esp, 0xFFFFFFF0

    ; Call signal handler
    call eax

    ; We are finished now, update the context to the regular thread context
    cli

    ; Get the current thread handle
    call cpuGetId
    mov ebx, 4
    mul ebx
    mov ebx, pCurrentThreadsPtr
    add eax, ebx
    mov ebx, [eax]

    ; RBX contains the pointer to the current VCPU, replace with the regular
    ; VCPU at eBX + 4
    mov eax, ebx
    add eax, 4
    mov eax, [eax]
    mov [ebx], eax

    call schedScheduleNoInt

cpuSignalHandlerLoop:
    ; We should never come back
    cli
    hlt
    jmp cpuSignalHandlerLoop

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------