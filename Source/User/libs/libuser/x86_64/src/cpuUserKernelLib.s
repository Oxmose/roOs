;-------------------------------------------------------------------------------
;
; File: cpuUserKernelLib.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 27/10/2024
;
; Version: 1.0
;
; User kernel library. This library provides non standard link
; between the user and the kernel space.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INCLUDES
;-------------------------------------------------------------------------------
; None

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
; None

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global syscallPerform

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text

; @brief Performs a system call.
;
; @details Performs a system call. The underlying CPU system call facility will
; be called to perform the required operation and issue the system call.
; The parameters for input and output are provided by the pParams parameter.
;
; @param[in] kSyscallId The system call identifier to use.
; @param[in, out] pParams  The parameter must contain an attribute of type
; syscall_min_params_t at the very begining of the structure. Otherwise, the
; existing attribute will be overwritten.
syscallPerform:
    ; Check that the parameters are not null
    cmp rsi, 0
    je _syscallPerformEnd

    ; Performs the system call
    syscall

_syscallPerformEnd:
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

section .data
; None