;-------------------------------------------------------------------------------
;
; File: int_handlers.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 30/03/2023
;
; Version: 1.0
;
; Global handlers for the 256 interrupts of the CPU.
; Setup the stack and call the C kernel general interrupt dispatcher.
;
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
%macro NOERR_CODE_INT_HANDLER 1           ; Interrupt that do not come with an
                                          ; err code.
global __intHandler%1
__intHandler%1:
    push    dword 0                       ; push 0 as dummy error code
    push    dword %1                      ; push the interrupt number
    jmp     __intHandlerEntry             ; jump to the common handler
%endmacro

%macro ERR_CODE_INT_HANDLER 1             ; Interrupt that do not come with an
                                          ; err code.

global __intHandler%1
__intHandler%1:
    push    dword %1                      ; push the interrupt number
    jmp     __intHandlerEntry             ; jump to the common handler
%endmacro

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

extern pCurrentThread

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------

extern interruptMainHandler

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
__intHandlerEntry:
        ; Save a bit of context
        push eax
        push ebx

        ; Get the current thread handle
        mov eax, 4
        mov ebx, gs ; GS stores the CPU id
        mul ebx
        mov ebx, pCurrentThread
        add eax, ebx
        mov eax, [eax]
        mov eax, [eax]

        ; Save the interrupt context
        mov ebx, [esp+8]  ; Int id
        mov [eax], ebx
        mov ebx, [esp+12]  ; Int code
        mov [eax+4], ebx
        mov ebx, [esp+16]  ; EIP
        mov [eax+8], ebx
        mov ebx, [esp+20]  ; CS
        mov [eax+12], ebx
        mov ebx, [esp+24]  ; EFLAGS
        mov [eax+16], ebx

        ; Save CPU state before calling interrupt
        mov ebx, esp       ; ESP before int save
        add ebx, 16
        mov [eax+20], ebx

        mov [eax+24], ebp
        mov [eax+28], edi
        mov [eax+32], esi
        mov [eax+36], edx
        mov [eax+40], ecx

        pop ebx             ; restore prelude ebx
        mov [eax+44], ebx
        push ebx            ; save ebx

        mov ebx, [esp+4]    ; pre int eax
        mov [eax+48], ebx
        pop ebx             ; restore prelude ebx

        mov [eax+52], ss
        mov [eax+56], gs
        mov [eax+60], fs
        mov [eax+64], es
        mov [eax+68], ds

        pop eax ; Restore

        ; call the C generic interrupt handler
        call interruptMainHandler

        ; Get the current thread handle
        mov eax, 4
        mov ebx, gs ; GS stores the CPU id
        mul ebx
        mov ebx, pCurrentThread
        add eax, ebx
        mov eax, [eax]
        mov eax, [eax]

        ; Restore registers
        mov ds,  [eax+68]
        mov es,  [eax+64]
        mov fs,  [eax+60]
        mov gs,  [eax+56]
        mov ss,  [eax+52]
        mov ecx, [eax+40]
        mov edx, [eax+36]
        mov esi, [eax+32]
        mov edi, [eax+28]
        mov ebp, [eax+24]
        mov esp, [eax+20]

        ; Restore the interrupt context
        mov ebx, [eax+8]  ; EIP
        mov [esp], ebx
        mov ebx, [esp+12]  ; CS
        mov [eax+4], ebx
        mov ebx, [esp+16]  ; EFLAGS
        mov [eax+8], ebx

        mov ebx, [eax+44]
        mov eax, [eax+48]

        ; Return from interrupt
        iret

    ; Now create handlers for each interrupt
    ERR_CODE_INT_HANDLER 8
    ERR_CODE_INT_HANDLER 10
    ERR_CODE_INT_HANDLER 11
    ERR_CODE_INT_HANDLER 12
    ERR_CODE_INT_HANDLER 13
    ERR_CODE_INT_HANDLER 14
    ERR_CODE_INT_HANDLER 17
    ERR_CODE_INT_HANDLER 30

    NOERR_CODE_INT_HANDLER 0
    NOERR_CODE_INT_HANDLER 1
    NOERR_CODE_INT_HANDLER 2
    NOERR_CODE_INT_HANDLER 3
    NOERR_CODE_INT_HANDLER 4
    NOERR_CODE_INT_HANDLER 5
    NOERR_CODE_INT_HANDLER 6
    NOERR_CODE_INT_HANDLER 7
    NOERR_CODE_INT_HANDLER 9
    NOERR_CODE_INT_HANDLER 15
    NOERR_CODE_INT_HANDLER 16
    NOERR_CODE_INT_HANDLER 18
    NOERR_CODE_INT_HANDLER 19
    NOERR_CODE_INT_HANDLER 20
    NOERR_CODE_INT_HANDLER 21
    NOERR_CODE_INT_HANDLER 22
    NOERR_CODE_INT_HANDLER 23
    NOERR_CODE_INT_HANDLER 24
    NOERR_CODE_INT_HANDLER 25
    NOERR_CODE_INT_HANDLER 26
    NOERR_CODE_INT_HANDLER 27
    NOERR_CODE_INT_HANDLER 28
    NOERR_CODE_INT_HANDLER 29
    NOERR_CODE_INT_HANDLER 31
    NOERR_CODE_INT_HANDLER 32
    NOERR_CODE_INT_HANDLER 33
    NOERR_CODE_INT_HANDLER 34
    NOERR_CODE_INT_HANDLER 35
    NOERR_CODE_INT_HANDLER 36
    NOERR_CODE_INT_HANDLER 37
    NOERR_CODE_INT_HANDLER 38
    NOERR_CODE_INT_HANDLER 39
    NOERR_CODE_INT_HANDLER 40
    NOERR_CODE_INT_HANDLER 41
    NOERR_CODE_INT_HANDLER 42
    NOERR_CODE_INT_HANDLER 43
    NOERR_CODE_INT_HANDLER 44
    NOERR_CODE_INT_HANDLER 45
    NOERR_CODE_INT_HANDLER 46
    NOERR_CODE_INT_HANDLER 47
    NOERR_CODE_INT_HANDLER 48
    NOERR_CODE_INT_HANDLER 49
    NOERR_CODE_INT_HANDLER 50
    NOERR_CODE_INT_HANDLER 51
    NOERR_CODE_INT_HANDLER 52
    NOERR_CODE_INT_HANDLER 53
    NOERR_CODE_INT_HANDLER 54
    NOERR_CODE_INT_HANDLER 55
    NOERR_CODE_INT_HANDLER 56
    NOERR_CODE_INT_HANDLER 57
    NOERR_CODE_INT_HANDLER 58
    NOERR_CODE_INT_HANDLER 59
    NOERR_CODE_INT_HANDLER 60
    NOERR_CODE_INT_HANDLER 61
    NOERR_CODE_INT_HANDLER 62
    NOERR_CODE_INT_HANDLER 63
    NOERR_CODE_INT_HANDLER 64
    NOERR_CODE_INT_HANDLER 65
    NOERR_CODE_INT_HANDLER 66
    NOERR_CODE_INT_HANDLER 67
    NOERR_CODE_INT_HANDLER 68
    NOERR_CODE_INT_HANDLER 69
    NOERR_CODE_INT_HANDLER 70
    NOERR_CODE_INT_HANDLER 71
    NOERR_CODE_INT_HANDLER 72
    NOERR_CODE_INT_HANDLER 73
    NOERR_CODE_INT_HANDLER 74
    NOERR_CODE_INT_HANDLER 75
    NOERR_CODE_INT_HANDLER 76
    NOERR_CODE_INT_HANDLER 77
    NOERR_CODE_INT_HANDLER 78
    NOERR_CODE_INT_HANDLER 79
    NOERR_CODE_INT_HANDLER 80
    NOERR_CODE_INT_HANDLER 81
    NOERR_CODE_INT_HANDLER 82
    NOERR_CODE_INT_HANDLER 83
    NOERR_CODE_INT_HANDLER 84
    NOERR_CODE_INT_HANDLER 85
    NOERR_CODE_INT_HANDLER 86
    NOERR_CODE_INT_HANDLER 87
    NOERR_CODE_INT_HANDLER 88
    NOERR_CODE_INT_HANDLER 89
    NOERR_CODE_INT_HANDLER 90
    NOERR_CODE_INT_HANDLER 91
    NOERR_CODE_INT_HANDLER 92
    NOERR_CODE_INT_HANDLER 93
    NOERR_CODE_INT_HANDLER 94
    NOERR_CODE_INT_HANDLER 95
    NOERR_CODE_INT_HANDLER 96
    NOERR_CODE_INT_HANDLER 97
    NOERR_CODE_INT_HANDLER 98
    NOERR_CODE_INT_HANDLER 99
    NOERR_CODE_INT_HANDLER 100
    NOERR_CODE_INT_HANDLER 101
    NOERR_CODE_INT_HANDLER 102
    NOERR_CODE_INT_HANDLER 103
    NOERR_CODE_INT_HANDLER 104
    NOERR_CODE_INT_HANDLER 105
    NOERR_CODE_INT_HANDLER 106
    NOERR_CODE_INT_HANDLER 107
    NOERR_CODE_INT_HANDLER 108
    NOERR_CODE_INT_HANDLER 109
    NOERR_CODE_INT_HANDLER 110
    NOERR_CODE_INT_HANDLER 111
    NOERR_CODE_INT_HANDLER 112
    NOERR_CODE_INT_HANDLER 113
    NOERR_CODE_INT_HANDLER 114
    NOERR_CODE_INT_HANDLER 115
    NOERR_CODE_INT_HANDLER 116
    NOERR_CODE_INT_HANDLER 117
    NOERR_CODE_INT_HANDLER 118
    NOERR_CODE_INT_HANDLER 119
    NOERR_CODE_INT_HANDLER 120
    NOERR_CODE_INT_HANDLER 121
    NOERR_CODE_INT_HANDLER 122
    NOERR_CODE_INT_HANDLER 123
    NOERR_CODE_INT_HANDLER 124
    NOERR_CODE_INT_HANDLER 125
    NOERR_CODE_INT_HANDLER 126
    NOERR_CODE_INT_HANDLER 127
    NOERR_CODE_INT_HANDLER 128
    NOERR_CODE_INT_HANDLER 129
    NOERR_CODE_INT_HANDLER 130
    NOERR_CODE_INT_HANDLER 131
    NOERR_CODE_INT_HANDLER 132
    NOERR_CODE_INT_HANDLER 133
    NOERR_CODE_INT_HANDLER 134
    NOERR_CODE_INT_HANDLER 135
    NOERR_CODE_INT_HANDLER 136
    NOERR_CODE_INT_HANDLER 137
    NOERR_CODE_INT_HANDLER 138
    NOERR_CODE_INT_HANDLER 139
    NOERR_CODE_INT_HANDLER 140
    NOERR_CODE_INT_HANDLER 141
    NOERR_CODE_INT_HANDLER 142
    NOERR_CODE_INT_HANDLER 143
    NOERR_CODE_INT_HANDLER 144
    NOERR_CODE_INT_HANDLER 145
    NOERR_CODE_INT_HANDLER 146
    NOERR_CODE_INT_HANDLER 147
    NOERR_CODE_INT_HANDLER 148
    NOERR_CODE_INT_HANDLER 149
    NOERR_CODE_INT_HANDLER 150
    NOERR_CODE_INT_HANDLER 151
    NOERR_CODE_INT_HANDLER 152
    NOERR_CODE_INT_HANDLER 153
    NOERR_CODE_INT_HANDLER 154
    NOERR_CODE_INT_HANDLER 155
    NOERR_CODE_INT_HANDLER 156
    NOERR_CODE_INT_HANDLER 157
    NOERR_CODE_INT_HANDLER 158
    NOERR_CODE_INT_HANDLER 159
    NOERR_CODE_INT_HANDLER 160
    NOERR_CODE_INT_HANDLER 161
    NOERR_CODE_INT_HANDLER 162
    NOERR_CODE_INT_HANDLER 163
    NOERR_CODE_INT_HANDLER 164
    NOERR_CODE_INT_HANDLER 165
    NOERR_CODE_INT_HANDLER 166
    NOERR_CODE_INT_HANDLER 167
    NOERR_CODE_INT_HANDLER 168
    NOERR_CODE_INT_HANDLER 169
    NOERR_CODE_INT_HANDLER 170
    NOERR_CODE_INT_HANDLER 171
    NOERR_CODE_INT_HANDLER 172
    NOERR_CODE_INT_HANDLER 173
    NOERR_CODE_INT_HANDLER 174
    NOERR_CODE_INT_HANDLER 175
    NOERR_CODE_INT_HANDLER 176
    NOERR_CODE_INT_HANDLER 177
    NOERR_CODE_INT_HANDLER 178
    NOERR_CODE_INT_HANDLER 179
    NOERR_CODE_INT_HANDLER 180
    NOERR_CODE_INT_HANDLER 181
    NOERR_CODE_INT_HANDLER 182
    NOERR_CODE_INT_HANDLER 183
    NOERR_CODE_INT_HANDLER 184
    NOERR_CODE_INT_HANDLER 185
    NOERR_CODE_INT_HANDLER 186
    NOERR_CODE_INT_HANDLER 187
    NOERR_CODE_INT_HANDLER 188
    NOERR_CODE_INT_HANDLER 189
    NOERR_CODE_INT_HANDLER 190
    NOERR_CODE_INT_HANDLER 191
    NOERR_CODE_INT_HANDLER 192
    NOERR_CODE_INT_HANDLER 193
    NOERR_CODE_INT_HANDLER 194
    NOERR_CODE_INT_HANDLER 195
    NOERR_CODE_INT_HANDLER 196
    NOERR_CODE_INT_HANDLER 197
    NOERR_CODE_INT_HANDLER 198
    NOERR_CODE_INT_HANDLER 199
    NOERR_CODE_INT_HANDLER 200
    NOERR_CODE_INT_HANDLER 201
    NOERR_CODE_INT_HANDLER 202
    NOERR_CODE_INT_HANDLER 203
    NOERR_CODE_INT_HANDLER 204
    NOERR_CODE_INT_HANDLER 205
    NOERR_CODE_INT_HANDLER 206
    NOERR_CODE_INT_HANDLER 207
    NOERR_CODE_INT_HANDLER 208
    NOERR_CODE_INT_HANDLER 209
    NOERR_CODE_INT_HANDLER 210
    NOERR_CODE_INT_HANDLER 211
    NOERR_CODE_INT_HANDLER 212
    NOERR_CODE_INT_HANDLER 213
    NOERR_CODE_INT_HANDLER 214
    NOERR_CODE_INT_HANDLER 215
    NOERR_CODE_INT_HANDLER 216
    NOERR_CODE_INT_HANDLER 217
    NOERR_CODE_INT_HANDLER 218
    NOERR_CODE_INT_HANDLER 219
    NOERR_CODE_INT_HANDLER 220
    NOERR_CODE_INT_HANDLER 221
    NOERR_CODE_INT_HANDLER 222
    NOERR_CODE_INT_HANDLER 223
    NOERR_CODE_INT_HANDLER 224
    NOERR_CODE_INT_HANDLER 225
    NOERR_CODE_INT_HANDLER 226
    NOERR_CODE_INT_HANDLER 227
    NOERR_CODE_INT_HANDLER 228
    NOERR_CODE_INT_HANDLER 229
    NOERR_CODE_INT_HANDLER 230
    NOERR_CODE_INT_HANDLER 231
    NOERR_CODE_INT_HANDLER 232
    NOERR_CODE_INT_HANDLER 233
    NOERR_CODE_INT_HANDLER 234
    NOERR_CODE_INT_HANDLER 235
    NOERR_CODE_INT_HANDLER 236
    NOERR_CODE_INT_HANDLER 237
    NOERR_CODE_INT_HANDLER 238
    NOERR_CODE_INT_HANDLER 239
    NOERR_CODE_INT_HANDLER 240
    NOERR_CODE_INT_HANDLER 241
    NOERR_CODE_INT_HANDLER 242
    NOERR_CODE_INT_HANDLER 243
    NOERR_CODE_INT_HANDLER 244
    NOERR_CODE_INT_HANDLER 245
    NOERR_CODE_INT_HANDLER 246
    NOERR_CODE_INT_HANDLER 247
    NOERR_CODE_INT_HANDLER 248
    NOERR_CODE_INT_HANDLER 249
    NOERR_CODE_INT_HANDLER 250
    NOERR_CODE_INT_HANDLER 251
    NOERR_CODE_INT_HANDLER 252
    NOERR_CODE_INT_HANDLER 253
    NOERR_CODE_INT_HANDLER 254
    NOERR_CODE_INT_HANDLER 255

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------