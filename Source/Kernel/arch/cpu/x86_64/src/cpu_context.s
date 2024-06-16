;-------------------------------------------------------------------------------
;
; File: cpu_context.S
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
;-------------------------------------------------------------------------------
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

extern pCurrentThread

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------

global __cpuSaveContext
global __cpuRestoreContext

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
__cpuSaveContext:
        ; Save a bit of context
        push rax
        push rbx

        ; Get the current thread handle
        mov rbx, pCurrentThread
        mov rax, [rbx]
        mov rax, [rax]

        ; Save the interrupt context
        mov rbx, [rsp+16]  ; Int id
        mov [rax], rbx
        mov rbx, [rsp+24]  ; Int code
        mov [rax+8], rbx
        mov rbx, [rsp+32]  ; RIP
        mov [rax+16], rbx
        mov rbx, [rsp+40]  ; CS
        mov [rax+24], rbx
        mov rbx, [rsp+48]  ; RFLAGS
        mov [rax+32], rbx

        ; Save CPU state before calling interrupt
        mov rbx, rsp       ; RSP before int save
        add rbx, 32
        mov [rax+40], rbx

        mov [rax+48], rbp
        mov [rax+56], rdi
        mov [rax+64], rsi
        mov [rax+72], rdx
        mov [rax+80], rcx

        pop rbx             ; restore prelude rbx
        mov [rax+88], rbx
        push rbx            ; save rbx

        mov rbx, [rsp+8]    ; pre int rax
        mov [rax+96], rbx
        pop rbx             ; restore prelude rbx

        mov [rax+104], r8
        mov [rax+112], r9
        mov [rax+120], r10
        mov [rax+128], r11
        mov [rax+136], r12
        mov [rax+144], r13
        mov [rax+152], r14
        mov [rax+160], r15

        mov [rax+168], ss
        mov [rax+176], gs
        mov [rax+184], fs
        mov [rax+192], es
        mov [rax+200], ds

        pop rax ; Restore

        ; call the C generic interrupt handler
        call interruptMainHandler

        ; Get the current thread handle
        mov rbx, pCurrentThread
        mov rax, [rbx]
        mov rax, [rax]

        ; Restore registers
        mov ss, [rax+168]
        mov gs, [rax+176]
        mov fs, [rax+184]
        mov es, [rax+192]
        mov ds, [rax+200]

        mov r8,  [rax+104]
        mov r9,  [rax+112]
        mov r10, [rax+120]
        mov r11, [rax+128]
        mov r12, [rax+136]
        mov r13, [rax+144]
        mov r14, [rax+152]
        mov r15, [rax+160]

        mov rsp, [rax+40]
        mov rbp, [rax+48]
        mov rdi, [rax+56]
        mov rsi, [rax+64]
        mov rdx, [rax+72]
        mov rcx, [rax+80]

        ; Restore the interrupt context
        mov rbx, [rax+16]  ; RIP
        mov [rsp], rbx
        mov rbx, [rax+24]  ; CS
        mov [rsp+8], rbx
        mov rbx, [rax+32]  ; RFLAGS
        mov [rsp+16], rbx

        mov rbx, [rax+88]
        mov rax, [rax+96]

        ; Return from interrupt
        iretq

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