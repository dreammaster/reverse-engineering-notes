; ===========================================================================

; Segment type: Pure code
ovl_1MENU1      segment byte public 'CODE' use16
                assume cs:ovl_1MENU1
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing
byte_1C130      db 0FFh, 36h            ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
byte_1C132      db 0D6h, 18h, 0E8h, 0DBh, 0B4h, 83h, 0C4h, 2, 0FFh, 36h
                                        ; DATA XREF: seg002:0038↑o
                db 0D0h, 18h, 0E8h, 0D1h, 0B4h, 83h, 0C4h, 2, 0B8h, 0E8h
                db 17h, 50h, 0E8h, 0C7h, 0B4h, 83h, 0C4h, 2, 0FFh, 36h
                db 3Ch, 3, 0E8h, 0BDh, 0B4h, 83h, 0C4h, 2, 0FFh, 36h, 0D6h
                db 18h, 0E8h, 0B3h, 0B4h, 83h, 0C4h, 2, 0B8h, 0EBh, 17h
                db 50h, 0E8h, 0A9h, 0B4h, 83h, 0C4h, 2, 0B8h, 1Ah, 18h
                db 50h, 0E8h, 9Fh, 0B4h, 83h, 0C4h, 2, 0B8h, 45h, 18h
                db 50h, 0E8h, 95h, 0B4h, 83h, 0C4h, 2, 0B8h, 59h, 18h
                db 50h, 0E8h, 8Bh, 0B4h, 83h, 0C4h, 2, 0FFh, 36h, 0CAh
                db 18h, 0E8h, 81h, 0B4h, 83h, 0C4h, 2, 0B8h, 63h, 18h
                db 50h, 0E8h, 77h, 0B4h, 83h, 0C4h, 2, 0FFh, 36h, 0CAh
                db 18h, 0E8h, 6Dh, 0B4h, 83h, 0C4h, 2, 0B8h, 7Dh, 18h
                db 50h, 0E8h, 63h, 0B4h, 83h, 0C4h, 2, 0FFh, 36h, 0CAh
                db 18h, 0E8h, 59h, 0B4h, 83h, 0C4h, 2, 0B8h, 8Ch, 18h
                db 50h, 0E8h, 4Fh, 0B4h, 83h, 0C4h, 2, 0FFh, 36h, 0CAh
                db 18h, 0E8h, 45h, 0B4h, 83h, 0C4h, 2, 0B8h, 96h, 18h
                db 50h, 0E8h, 3Bh, 0B4h, 83h, 0C4h, 2
byte_1C1DA      db 0FFh, 36h, 0CAh, 18h, 0E8h, 31h, 0B4h, 83h, 0C4h, 2
                                        ; CODE XREF: seg002:08CD↑J
                db 0B8h, 0AAh, 18h, 50h, 0E8h, 27h
byte_1C1EA      db 0B4h, 83h, 0C4h, 2, 0B8h, 1, 0, 50h, 0E8h, 0EDh, 0B3h
                                        ; CODE XREF: seg002:07F5↑J
                db 83h, 0C4h, 2, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 8, 57h, 56h, 2Bh, 0F6h, 0B8h, 1, 0, 50h, 0E8h, 57h
                db 0ACh, 83h, 0C4h, 2, 2Bh, 0FFh, 0C7h, 46h, 0FAh, 0D8h
                db 18h, 8Bh, 5Eh, 0FAh, 8Bh, 37h, 8Bh, 0C6h, 0D1h, 0E0h
                db 89h, 46h, 0F8h, 8Bh, 0D8h, 0FFh, 0B7h, 54h, 19h, 0FFh
                db 0B7h, 36h, 19h, 56h, 0FFh, 36h, 0CEh, 18h, 0FFh, 36h
                db 0CCh, 18h, 0E8h, 0BBh, 0ACh, 83h, 0C4h, 0Ah, 0Bh, 0FFh
byte_1C23C      db 75h, 20h, 0C7h, 6, 16h, 4Ah
                                        ; CODE XREF: seg002:08E5↑J
byte_1C242      db 2 dup(0FFh), 2Bh, 0F6h, 2Bh, 0C0h, 50h, 0B8h, 1, 0
                                        ; CODE XREF: seg002:0639↑J
                db 50h, 0E8h, 42h, 0ACh, 83h, 0C4h, 4, 2Bh, 0C0h, 50h
                db 0E8h, 9, 0ACh, 83h, 0C4h, 2, 0EBh, 0Ch, 0B8h, 46h, 0
                db 50h, 0E8h, 9Dh, 0AEh, 83h, 0C4h, 2, 8Bh, 0F0h, 0Bh
                db 0F6h, 74h, 8, 89h, 7Eh, 0FCh, 89h, 76h, 0FEh, 0EBh
                db 0Ch, 83h, 46h, 0FAh, 2, 47h, 83h, 0FFh, 2Fh, 7Dh, 0EEh
                db 0EBh, 93h, 0C7h, 6, 16h, 4Ah, 0FEh, 0FFh, 0B8h, 1, 0
                db 50h, 0E8h, 0D3h, 0ABh, 83h, 0C4h, 2, 5Eh, 5Fh, 8Bh
                db 0E5h, 5Dh, 0C3h, 0FFh, 36h, 0D2h, 4, 0E8h, 0Bh, 0ACh
                db 83h, 0C4h, 2, 0A3h, 0CCh, 18h, 89h, 16h, 0CEh, 18h
                db 0Bh, 0C2h, 74h, 0EBh, 0B8h, 3Ch, 0, 50h, 2Bh, 0C0h
                db 2 dup(50h), 52h, 0FFh, 36h, 0CCh, 18h, 0E8h, 35h, 0ACh
                db 83h, 0C4h, 0Ah
byte_1C2C0      db 0FFh, 36h, 0CEh, 18h, 0FFh, 36h, 0CCh, 18h, 0E8h, 3
                                        ; CODE XREF: seg002:026D↑J
                db 0ACh, 83h, 0C4h, 4, 0FFh, 36h, 0D4h, 4, 0E8h, 0D5h
                db 0ABh, 83h, 0C4h, 2, 0A3h, 0CCh, 18h, 89h, 16h, 0CEh
                db 18h, 0Bh, 0C2h, 74h, 0EBh, 0B8h, 0F4h, 1, 50h, 0E8h
                db 0DCh, 0AAh, 83h, 0C4h, 2, 0B8h, 1, 0, 50h, 0E8h, 6Eh
                db 0ABh, 83h, 0C4h, 2, 0E8h
byte_1C2F8      db 0C0h, 0AAh, 0Bh, 0C0h, 75h, 3, 0E8h, 0F9h, 0FEh, 0FFh
                                        ; CODE XREF: seg002:0651↑J
                db 36h, 0CEh, 18h, 0FFh, 36h, 0CCh
byte_1C308      db 18h, 0E8h, 0C2h, 0ABh, 83h, 0C4h, 4, 0B8h, 4, 0, 50h
                                        ; CODE XREF: seg002:08F1↑J
                db 0E8h, 0E0h, 0AAh, 83h, 0C4h, 2, 2Bh, 0C0h, 0A3h, 0CEh
                db 18h, 0A3h, 0CCh, 18h, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 0Ah, 57h, 56h, 0C7h, 46h, 0FCh, 23h, 0, 0C7h, 46h, 0F8h
                db 20h, 5, 0C7h, 46h, 0F6h, 1Fh, 0, 8Bh, 5Eh, 0F8h, 8Bh
                db 3Fh, 57h, 8Ch, 0D8h, 8Eh, 0C0h, 0B9h, 2 dup(0FFh), 33h
                db 0C0h, 0F2h, 0AEh, 0F7h, 0D1h, 49h, 5Fh, 8Bh, 0F1h, 4Eh
                db 8Bh, 0DEh, 3, 0DFh, 0C6h, 7, 0, 4Eh, 8Bh, 0DEh, 3, 0DFh
                db 0C6h, 7, 34h, 83h, 46h, 0F8h, 2, 0FFh, 4Eh, 0F6h, 75h
                db 0D0h, 89h, 7Eh, 0FEh, 89h, 76h, 0FAh, 5Eh
byte_1C370      db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 0A1h, 0C2h, 4, 0A3h
                                        ; CODE XREF: seg002:0471↑J
                                        ; sub_1C5D8+51↓p
                db 20h, 5, 0A1h, 0C4h, 4, 0A3h, 22h, 5, 0A1h, 0C6h, 4
                db 0A3h, 24h, 5, 0A1h, 0C8h, 4, 0A3h, 26h, 5, 0A1h, 0CAh
                db 4, 0A3h, 28h, 5, 0A1h, 0CCh, 4, 0A3h, 2Ah, 5, 0A1h
                db 0CEh, 4, 0A3h, 2Ch, 5, 0A1h, 0D2h, 4, 0A3h, 2Eh, 5
                db 0A1h, 0D4h, 4, 0A3h, 30h, 5, 0A1h, 0D6h, 4, 0A3h, 32h
                db 5, 0A1h, 0D8h, 4, 0A3h, 34h, 5, 0A1h, 0DAh, 4, 0A3h
                db 36h, 5, 0A1h, 0DCh, 4, 0A3h, 38h, 5, 0A1h, 0DEh, 4
                db 0A3h, 3Ah, 5, 0A1h, 0E0h, 4, 0A3h, 3Ch, 5, 0A1h, 0E2h
                db 4, 0A3h, 3Eh, 5, 0A1h, 0E4h, 4, 0A3h, 40h, 5, 0A1h
                db 0E6h, 4, 0A3h, 42h, 5, 0A1h, 0E8h, 4, 0A3h, 44h, 5
                db 0A1h, 0EAh, 4, 0A3h, 46h, 5, 0A1h, 0ECh, 4, 0A3h, 48h
                db 5, 0A1h, 0F8h
byte_1C3F6      db 4, 0A3h, 4Ah, 5, 0A1h, 0FAh, 4, 0A3h, 4Ch, 5, 0A1h
                                        ; CODE XREF: seg002:047D↑J
                db 0FCh, 4, 0A3h, 4Eh, 5, 0A1h, 0FEh, 4, 0A3h, 50h, 5
                db 0A1h, 0, 5, 0A3h, 52h, 5, 0A1h, 0EEh, 4, 0A3h, 54h
                db 5, 0A1h, 0F0h, 4, 0A3h, 56h, 5, 0A1h, 0F2h, 4, 0A3h
                db 58h, 5, 0A1h, 0F4h, 4, 0A3h, 5Ah, 5, 0A1h, 0F6h, 4
                db 0A3h, 5Ch, 5, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 2, 56h, 0C7h, 6, 32h, 4, 1, 0, 2Bh, 0F6h, 83h, 3Eh
                db 32h, 4, 0, 75h, 0Ah, 56h, 0E8h, 9Ah, 9, 83h, 0C4h, 2
                db 0A3h, 32h, 4, 46h, 83h, 0FEh, 2, 7Ch, 0E9h, 89h, 76h
                db 0FEh, 83h, 3Eh, 32h, 4, 0, 75h, 76h
byte_1C462      db 0FFh, 36h, 0D6h, 18h, 0E8h, 0A9h, 0B1h, 83h, 0C4h, 2
                                        ; CODE XREF: seg002:086D↑J
                db 0FFh, 36h, 0D6h, 18h, 0E8h, 9Fh, 0B1h, 83h, 0C4h, 2
                db 0B8h, 72h, 19h, 50h, 0E8h, 95h, 0B1h, 83h, 0C4h, 2
                db 0FFh, 36h, 0D0h, 18h, 0E8h, 8Bh, 0B1h, 83h, 0C4h, 2
                db 0FFh, 36h, 0D2h, 18h
byte_1C48E      db 0E8h, 81h, 0B1h, 83h, 0C4h, 2, 0FFh, 36h, 0D6h, 18h
                                        ; CODE XREF: seg002:0B31↑J
                db 0E8h, 77h, 0B1h, 83h, 0C4h, 2, 0B8h, 7Ah, 19h, 50h
                db 0E8h, 6Dh, 0B1h, 83h, 0C4h, 2, 0FFh, 36h, 0D4h, 18h
                db 0E8h, 63h, 0B1h, 83h, 0C4h, 2, 0B8h, 0Dh, 0, 50h, 0E8h
                db 0FDh, 0A9h, 83h, 0C4h, 2, 2Bh, 0F6h, 83h, 3Eh, 32h
                db 4, 0, 75h, 0Ah, 56h, 0E8h, 1Dh, 9, 83h, 0C4h, 2, 0A3h
                db 32h, 4, 46h, 83h, 0FEh, 2, 7Ch, 0E9h, 89h, 76h, 0FEh
                db 0FFh, 36h, 0D6h, 18h, 0E8h, 33h, 0B1h, 83h, 0C4h, 2
                db 83h, 3Eh, 32h, 4, 0, 75h, 25h, 0FFh, 36h, 0D6h, 18h
                db 0E8h, 22h, 0B1h, 83h, 0C4h, 2, 0FFh, 36h, 0D0h, 18h
                db 0E8h, 18h, 0B1h, 83h, 0C4h, 2, 0B8h, 8Ch, 19h, 50h
                db 0E8h, 0Eh, 0B1h, 83h, 0C4h, 2, 0B8h, 0ABh, 19h, 50h
                db 0EBh, 49h, 90h, 0E8h, 0DDh, 0B0h, 3Dh, 2, 0, 7Fh, 50h
                db 0FFh, 36h, 0D6h, 18h, 0E8h, 0F5h, 0B0h, 83h, 0C4h, 2
                db 0B8h, 0E1h, 19h, 50h, 0E8h, 0EBh, 0B0h, 83h, 0C4h, 2
                db 0FFh, 36h
byte_1C52C      db 0D0h, 18h, 0E8h, 0E1h, 0B0h, 83h, 0C4h, 2, 0FFh, 36h
                                        ; CODE XREF: seg002:0879↑J
                db 0D2h, 18h, 0E8h, 0D7h, 0B0h, 83h, 0C4h, 2, 0FFh, 36h
                db 0D6h, 18h, 0E8h, 0CDh, 0B0h, 83h, 0C4h, 2, 0B8h, 0F7h
                db 19h, 50h, 0E8h, 0C3h, 0B0h, 83h, 0C4h, 2, 0FFh, 36h
                db 0D4h, 18h, 0E8h, 0B9h, 0B0h, 83h, 0C4h, 2, 0B8h, 0Dh
                db 0, 50h, 0E8h, 53h, 0A9h, 83h, 0C4h, 2, 5Eh, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 0C7h
                db 46h, 0FEh, 2 dup(0), 0E8h, 2Ch, 0B0h, 80h, 3Eh, 46h
                db 2 dup(3), 75h, 0Ch, 83h, 3Eh, 5Ch, 49h, 62h, 7Dh, 5
                db 0C7h, 46h, 0FEh, 1, 0, 80h, 3Eh, 46h, 3, 0Fh, 75h, 0Dh
                db 81h, 3Eh, 5Ch, 49h, 0C8h, 0, 7Dh, 5, 0C7h, 46h, 0FEh
                db 2, 0, 83h, 7Eh, 0FEh, 0, 74h, 2Dh, 0FFh, 36h, 5Ch, 1Ah
                db 0E8h
byte_1C5AC      db 64h, 0B0h, 83h, 0C4h, 2, 8Bh, 5Eh, 0FEh, 0D1h, 0E3h
                                        ; CODE XREF: seg002:0885↑J
                db 0FFh, 0B7h, 5Eh, 1Ah, 0E8h, 55h, 0B0h, 83h, 0C4h, 2
byte_1C5C0      db 0FFh, 36h, 5Eh, 1Ah, 0E8h, 4Bh, 0B0h, 83h, 0C4h, 2
                                        ; CODE XREF: seg002:029D↑J
                db 0B8h, 1, 0, 50h, 0E8h, 11h, 0B0h, 83h, 0C4h, 2, 8Bh
                db 0E5h, 5Dh, 0C3h

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5D8       proc near               ; CODE XREF: seg002:01A1↑J

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+arg_0], al
                call    thk_detect_hardware
                mov     al, [bp+arg_0]
                sub     ah, ah
                cmp     ax, 43h ; 'C'
                jz      short loc_1C620
                cmp     ax, 45h ; 'E'
                jnz     short loc_1C5FF
                jmp     loc_1C6AA
; ---------------------------------------------------------------------------

loc_1C5FF:                              ; CODE XREF: sub_1C5D8+22↑j
                cmp     ax, 48h ; 'H'
                jnz     short loc_1C607
                jmp     loc_1C6BE
; ---------------------------------------------------------------------------

loc_1C607:                              ; CODE XREF: sub_1C5D8+2A↑j
                cmp     ax, 4Dh ; 'M'
                jnz     short loc_1C60F
                jmp     loc_1C6C8
; ---------------------------------------------------------------------------

loc_1C60F:                              ; CODE XREF: sub_1C5D8+32↑j
                cmp     ax, 54h ; 'T'
                jnz     short loc_1C617
                jmp     loc_1C6B4
; ---------------------------------------------------------------------------

loc_1C617:                              ; CODE XREF: sub_1C5D8+3A↑j
                cmp     al, 20h ; ' '
                jz      short loc_1C626
                call    near ptr byte_1C130
                jmp     short loc_1C626
; ---------------------------------------------------------------------------

loc_1C620:                              ; CODE XREF: sub_1C5D8+1D↑j
                mov     word_221BA, 0

loc_1C626:                              ; CODE XREF: sub_1C5D8+41↑j
                                        ; sub_1C5D8+46↑j ...
                call    near ptr byte_1C3F6+3Ch
                call    near ptr byte_1C370+6
                cmp     word_221BA, 0
                jz      short loc_1C63A
                cmp     word_221BA, 3
                jnz     short loc_1C66A

loc_1C63A:                              ; CODE XREF: sub_1C5D8+59↑j
                mov     byte_1DB8E, 2
                mov     byte_1DB8F, 2
                mov     byte_1DB90, 1
                mov     byte_1DB91, 1
                mov     byte_1DB92, 0
                mov     byte_1DB93, 3
                mov     byte_1DB94, 0
                mov     byte_1DB95, 3
                mov     byte_1DB96, 3
                call    near ptr byte_1C308+1Ah

loc_1C66A:                              ; CODE XREF: sub_1C5D8+60↑j
                call    near ptr byte_1C52C+40h
                call    thk_res_1DFA    ; CODE XREF: seg002:0891↑J
                mov     ax, 75BEh
                mov     dx, 1000h
                push    dx
                push    ax
                call    thk_set_error_handler
                add     sp, 4
                call    thk_load_timer_driver
                call    thk_res_1D5B
                cmp     word_221C8, 0
                jz      short loc_1C69E
                cmp     word_221BA, 4
                jnz     short loc_1C69E
                cmp     [bp+arg_0], 4Dh ; 'M'
                jz      short loc_1C69E
                mov     word_221BA, 1

loc_1C69E:                              ; CODE XREF: sub_1C5D8+B1↑j
                                        ; sub_1C5D8+B8↑j ...
                call    thk_load_video_driver
                or      dx, ax
                jz      short loc_1C69E
                call    thk_gfx_reset
                jmp     short loc_1C6D2
; ---------------------------------------------------------------------------

loc_1C6AA:                              ; CODE XREF: sub_1C5D8+24↑j
                mov     word_221BA, 1
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6B4:                              ; CODE XREF: sub_1C5D8+3C↑j
                mov     word_221BA, 2
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6BE:                              ; CODE XREF: sub_1C5D8+2C↑j
                mov     word_221BA, 3
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6C8:                              ; CODE XREF: sub_1C5D8+34↑j
                mov     word_221BA, 4
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6D2:                              ; CODE XREF: sub_1C5D8+D0↑j
                                        ; sub_1C5D8+109↓j
                sub     ax, ax
                push    ax
                mov     ax, offset aMm2Ch ; "mm2.ch"
                push    ax
                call    thk_load_resource_cached
                add     sp, 4
                or      dx, ax
                jz      short loc_1C6D2
                mov     ax, 17h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_text_window_create

loc_1C6F2:                              ; CODE XREF: seg002:0B19↑J
                add     sp, 8
                mov     g_main_text_win, ax
                mov     ax, 0C7h
                push    ax
                mov     ax, 13Fh
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_gfx_window_create
                add     sp, 8
                mov     g_main_gfx_win, ax
                push    ax
                call    thk_gfx_window_open
                add     sp, 2
                push    g_main_text_win
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     ax, 75B2h
                mov     dx, 1000h
                push    dx
                push    ax
                call    thk_set_error_handler
                add     sp, 4
                call    thk_get_drive
                mov     word_277D2, ax  ; CODE XREF: seg002:089D↑J
                mov     word_277D0, ax
                mov     g_disk_needed, 1
                call    near ptr byte_1C242+56h

loc_1C747:                              ; CODE XREF: sub_1C5D8+186↓j
                mov     ax, 1400h
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 6960h
                push    ax
                mov     ax, offset aItemsDat ; "items.dat"
                push    ax
                call    thk_read_file_to_buffer
                add     sp, 8
                or      ax, ax
                jz      short loc_1C747

loc_1C760:                              ; CODE XREF: sub_1C5D8+19F↓j
                mov     ax, 0C0h
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 7D60h
                push    ax
                mov     ax, offset aSpellsDat ; "spells.dat"
                push    ax
                call    thk_read_file_to_buffer
                add     sp, 8
                or      ax, ax
                jz      short loc_1C760
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C77C:                              ; CODE XREF: sub_1CBDC:loc_1CD96↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     ax, 6
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 5
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     al, byte_1DB92
                sub     ah, ah
                mov     [bp+var_2], ax
                cmp     byte_1DB96, 3
                jnz     short loc_1C7B9
                mov     [bp+var_2], 2

loc_1C7B9:                              ; CODE XREF: sub_1C5D8+1DA↑j
                push    [bp+var_2]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                mov     [bp+var_2], 0Eh
                mov     si, 8

loc_1C7DD:                              ; CODE XREF: ovl_1MENU1:C804↓j
                mov     ax, 0Ah
                push    ax
                push    si
sub_1C5D8       endp


loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                sub     ax, ax
                push    ax
                push    word_1DBA4      ; CODE XREF: seg002:0B0D↑J
                push    word_1DBA2
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 32h ; '2'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                add     si, 10h
                cmp     si, 0E8h
                jl      short loc_1C7DD
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, 1CE8h

loc_1C815:                              ; CODE XREF: ovl_1MENU1:C831↓j
                lea     ax, [si+0Bh]
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Bh
                jl      short loc_1C815
                mov     [bp-2], si
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                call    thk_res_5440
                mov     ax, 1Bh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C868       proc near               ; CODE XREF: sub_1CBDC:loc_1CD8A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_main_frame
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_res_5440
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, 1CFEh

loc_1C8B1:                              ; CODE XREF: sub_1C868+62↓j
                push    si
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 14h
                jl      short loc_1C8B1
                mov     [bp+var_2], si
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1DB8C
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_145E
                add     sp, 4
                mov     ax, 1Bh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C868       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C91A       proc near               ; CODE XREF: ovl_1MENU1:loc_1C9B9↓p
                                        ; sub_1C9CE:loc_1C9E5↓p
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 10h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 8
                push    ax
                mov     ax, 0F8h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 10h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 8
                push    ax
                mov     ax, 0F8h
                push    ax
                push    word_1F576
                inc     word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 3Ch ; '<'   ; CODE XREF: seg002:07DD↑J
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                cmp     word_1F576, 0Ch
                jnz     short locret_1C9B0
                mov     word_1F576, 0

locret_1C9B0:                           ; CODE XREF: sub_1C91A+8E↑j
                retn
sub_1C91A       endp

; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: sub_1CBDC:loc_1CD68↓p
                mov     bp, sp
                sub     sp, 2
                push    si

loc_1C9B9:                              ; CODE XREF: ovl_1MENU1:C9C3↓j
                call    sub_1C91A
                call    thk_kbd_poll
                mov     si, ax
                or      si, si
                jz      short loc_1C9B9
                mov     [bp-2], si
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C9CE       proc near               ; CODE XREF: ovl_1MENU1:CB80↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, word_221B4
                mov     [bp+var_2], ax
                mov     word_221B4, 0
                mov     di, [bp+arg_2]

loc_1C9E5:                              ; CODE XREF: sub_1C9CE+51↓j
                                        ; seg002:0B01↑J
                call    sub_1C91A
                call    thk_kbd_poll
                mov     si, ax
                or      si, si
                jnz     short loc_1CA13
                mov     bx, word_1DD62
                inc     word_1DD62
                mov     al, [bx+50Ah]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     word_1DD62, 8
                jnz     short loc_1CA13
                mov     word_1DD62, 0

loc_1CA13:                              ; CODE XREF: sub_1C9CE+21↑j
                                        ; sub_1C9CE+3D↑j
                cmp     si, [bp+arg_0]
                jl      short loc_1CA1C
                cmp     si, di
                jle     short loc_1CA21

loc_1CA1C:                              ; CODE XREF: sub_1C9CE+48↑j
                cmp     si, 1Bh
                jnz     short loc_1C9E5

loc_1CA21:                              ; CODE XREF: sub_1C9CE+4C↑j
                mov     [bp+var_4], si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_2]
                mov     word_221B4, ax
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C9CE       endp

; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: sub_1CBDC:loc_1CD9C↓p
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     bx, word_1F5F6
                mov     byte ptr [bx+0Eh], 32h ; '2'
                cmp     byte_1DB96, 3
                jnz     short loc_1CA56 ; CODE XREF: seg002:0A89↑J
                inc     byte ptr [bx+0Eh]

loc_1CA56:                              ; CODE XREF: ovl_1MENU1:CA51↑j
                cmp     word_27844, 1
                jnz     short loc_1CA60
                jmp     loc_1CBB4
; ---------------------------------------------------------------------------

loc_1CA60:                              ; CODE XREF: ovl_1MENU1:CA5B↑j
                mov     ax, word_27844
                add     ax, 40h ; '@'
                mov     [bp-2], ax
                mov     bx, word_1F5F4
                mov     al, [bp-2]
                mov     [bx+12h], al
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2           ; CODE XREF: seg002:0801↑J
                mov     ax, 14h
                push    ax
                mov     ax, 22h ; '"'
                push    ax
                mov     ax, 0Ch
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp-6], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 1
                mov     al, byte_1DB92
                mov     [bx+7], al
                push    bx
                call    thk_text_window_open
                add     sp, 2
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                call    thk_draw_frame_alt
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTypeTheLetterO ; "Type the letter of the drive\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_1F5F6
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aLocatedDuringG ; "located during game play:\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_1F5F4
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 7
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDDisk ; "D - Disk "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bx, word_1F5F6
                mov     al, [bx+0Eh]    ; CODE XREF: seg002:0A65↑J
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, offset aDrive ; " - Drive "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     di, [bp-2]
                jmp     short loc_1CB74
; ---------------------------------------------------------------------------
                align 2

loc_1CB70:                              ; CODE XREF: ovl_1MENU1:CB92↓j
                cmp     si, di
                jle     short loc_1CB94

loc_1CB74:                              ; CODE XREF: ovl_1MENU1:CB6D↑j
                push    di
                call    thk_res_0128
                add     sp, 2
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    sub_1C9CE
                add     sp, 4
                push    ax
                call    thk_res_00E8

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1CB70

loc_1CB94:                              ; CODE XREF: ovl_1MENU1:CB72↑j
                mov     [bp-4], si
                cmp     si, 1Bh
                jz      short loc_1CBAB

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                push    si
                call    thk_text_putc
                add     sp, 2
                mov     ax, si
                add     ax, 0FFC0h
                mov     word_277D2, ax

loc_1CBAB:                              ; CODE XREF: ovl_1MENU1:CB9A↑j
                push    word ptr [bp-6]
                call    thk_text_window_close
                add     sp, 2

loc_1CBB4:                              ; CODE XREF: ovl_1MENU1:CA5D↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                mov     ax, 1           ; CODE XREF: sub_1CBDC+76↓p
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aNewWorldComput_1 ; "\nNew World Computing\n\nPresents\n\nMI"...
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aBookTwo_0 ; "\nBook Two\n\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CBDC       proc near               ; CODE XREF: seg002:01AD↑J

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 0

loc_1CBE8:                              ; CODE XREF: sub_1CBDC+1F↓j
                push    word_1DD14
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DB9E, ax
                mov     word_1DBA0, dx
                or      dx, ax
                jz      short loc_1CBE8

loc_1CBFD:                              ; CODE XREF: sub_1CBDC+34↓j
                push    word_1DD16
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBA2, ax
                mov     word_1DBA4, dx
                or      dx, ax
                jz      short loc_1CBFD
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_draw_main_frame
                mov     ax, 0Ah
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                call    thk_draw_frame_hline
                add     sp, 6
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                call    loc_1CBBA
                mov     ax, 5
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCopyright1989N ; "Copyright 1989 New World Computing,Inc"...
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aAllRightsReser ; "All Rights Reserved"
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 0Ch
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOptions ; "OPTIONS\n\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Fh
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSStartGame ; "S - Start Game"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 10h         ; CODE XREF: seg002:08FD↑J
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCCopyPlayerDis ; "C - Copy Player Disk"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAAboutBookTwo ; "A - About Book Two"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDDisk_0 ; "D - Disk "
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_1DB96, 3
                jnz     short loc_1CD0E
                mov     ax, 33h ; '3'
                jmp     short loc_1CD11
; ---------------------------------------------------------------------------
                align 2

loc_1CD0E:                              ; CODE XREF: sub_1CBDC+12A↑j
                mov     ax, 32h ; '2'

loc_1CD11:                              ; CODE XREF: sub_1CBDC+12F↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, offset aDrive_0 ; " - Drive "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, word_277D2
                add     ax, 40h ; '@'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_4], 7
                mov     si, 7

loc_1CD45:                              ; CODE XREF: sub_1CBDC+174↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_1CD45
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4

loc_1CD68:                              ; CODE XREF: sub_1CBDC+1B8↓j
                call    loc_1C9B2
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 1Bh
                jz      short loc_1CDBC
                cmp     ax, 41h ; 'A'
                jz      short loc_1CD8A
                cmp     ax, 43h ; 'C'
                jz      short loc_1CD96
                cmp     ax, 44h ; 'D'
                jz      short loc_1CD9C
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------

loc_1CD8A:                              ; CODE XREF: sub_1CBDC+1A0↑j
                call    sub_1C868

loc_1CD8D:                              ; CODE XREF: sub_1CBDC+1AC↑j
                                        ; sub_1CBDC+1BD↓j ...
                mov     ax, si
                cmp     ax, 53h ; 'S'
                jz      short loc_1CDC2
                jmp     short loc_1CD68
; ---------------------------------------------------------------------------

loc_1CD96:                              ; CODE XREF: sub_1CBDC+1A5↑j
                call    loc_1C77C
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------
                align 2

loc_1CD9C:                              ; CODE XREF: sub_1CBDC+1AA↑j
                call    loc_1CA3C
                mov     ax, 13h
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, word_277D2
                add     ax, 40h ; '@'
                push    ax
                call    thk_text_putc
                add     sp, 2
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------

loc_1CDBC:                              ; CODE XREF: sub_1CBDC+19B↑j
                call    thk_res_3FC4
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------
                align 2

loc_1CDC2:                              ; CODE XREF: sub_1CBDC+1B6↑j
                mov     [bp+var_4], si
                push    word_1DBA4
                push    word_1DBA2
                call    thk_free_far_block
                add     sp, 4
                push    word_1DBA0
                push    word_1DB9E
                call    thk_free_far_block
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CBDC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CDE6       proc near

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                mov     ax, [bp+arg_0]
                and     al, 1
                mov     byte_1F6D7, al
                mov     bx, 40h ; '@'
                mov     ah, 48h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                mov     bx, ax
                and     ax, 0FFFh
                cmp     ax, 0FC1h
                jb      short loc_1CE18
                push    bx
                mov     bx, 40h ; '@'
                mov     ah, 48h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                pop     bx
                push    ax
                push    es
                mov     es, bx
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                pop     bx

loc_1CE18:                              ; CODE XREF: sub_1CDE6+1D↑j
                mov     di, 3
                push    bx

loc_1CE1C:                              ; CODE XREF: sub_1CDE6+54↓j
                xor     ah, ah
                push    di
                int     13h             ; DISK - RESET DISK SYSTEM
                                        ; DL = drive (if bit 7 is set both hard disks and floppy disks reset)
                pop     di
                mov     ax, 201h
                mov     cl, 1
                mov     ch, 27h ; '''
                mov     dl, byte_1F6D7
                mov     dh, 0
                pop     es

loc_1CE30:                              ; CODE XREF: seg002:0831↑J
                push    es
                xor     bx, bx
                push    di
                int     13h             ; DISK - READ SECTORS INTO MEMORY
                                        ; AL = number of sectors to read, CH = track, CL = sector
                                        ; DH = head, DL = drive, ES:BX -> buffer to fill
                                        ; Return: CF set on error, AH = status, AL = number of sectors read
                pop     di
                jnb     short loc_1CE40
                dec     di
                jnz     short loc_1CE1C
                xor     ax, ax
                jmp     short loc_1CEB6
; ---------------------------------------------------------------------------

loc_1CE40:                              ; CODE XREF: sub_1CDE6+51↑j
                mov     al, 1Eh
                mov     ah, 35h
                int     21h             ; DOS - 2+ - GET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; Return: ES:BX = value of interrupt vector
                mov     word_1F6D3, bx
                mov     word_1F6D5, es
                mov     dx, 1E78h
                mov     al, 1Eh
                mov     ah, 25h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                mov     di, 3

loc_1CE5A:                              ; CODE XREF: sub_1CDE6+8F↓j
                                        ; sub_1CDE6+9B↓j ...
                mov     ax, 201h
                pop     es
                push    es
                xor     bx, bx
                mov     cl, 17h
                mov     ch, 27h ; '''
                mov     dl, byte_1F6D7
                mov     dh, 0
                push    di
                int     13h             ; DISK - READ SECTORS INTO MEMORY
                                        ; AL = number of sectors to read, CH = track, CL = sector
                                        ; DH = head, DL = drive, ES:BX -> buffer to fill
                                        ; Return: CF set on error, AH = status, AL = number of sectors read
                pop     di
                cmp     ah, 10h
                jz      short loc_1CE7B
                dec     di
                jnz     short loc_1CE5A
                xor     ax, ax
                jmp     short loc_1CEA6
; ---------------------------------------------------------------------------

loc_1CE7B:                              ; CODE XREF: sub_1CDE6+8C↑j
                cmp     byte ptr es:[bx+240h], 3Ch ; '<'
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+270h], 3Ch ; '<'
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+2A0h], 0F6h
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+264h], 18h
                jnz     short loc_1CE5A
                cmp     word ptr es:[bx+28Eh], 3Fh ; '?'
                jnz     short loc_1CE5A
                mov     ax, 1

loc_1CEA6:                              ; CODE XREF: sub_1CDE6+93↑j
                push    ax
                mov     dx, word_1F6D3
                mov     ds, word_1F6D5
                mov     al, 1Eh
                mov     ah, 25h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                pop     ax

loc_1CEB6:                              ; CODE XREF: sub_1CDE6+58↑j
                pop     bx
                push    ax
                push    es
                mov     es, bx
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                pop     ax
                pop     ds
                pop     di
                pop     si
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db    0
                db    0
byte_1CEC8      db 8 dup(0)             ; CODE XREF: seg002:07AD↑J
sub_1CDE6       endp

ovl_1MENU1      ends

