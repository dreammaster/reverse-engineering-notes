; ===========================================================================

; Segment type: Pure code
ovl_1RETINN     segment byte public 'CODE' use16
                assume cs:ovl_1RETINN
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C130       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 0Ah, 57h, 56h, 0C6h, 6, 30h, 4, 6
                                        ; DATA XREF: seg002:0038↑o
                db 2Bh, 0C0h, 50h, 0E8h, 0B3h, 0ACh, 83h, 0C4h, 2, 2Bh
                db 0F6h, 89h, 76h, 0FAh, 8Ah, 1Eh, 92h, 3, 2Ah, 0FFh, 0B1h
                db 3, 0D3h, 0E3h, 3, 5Eh, 0FAh, 8Bh, 0BFh, 0BAh, 20h, 0Bh
                db 0FFh, 74h, 15h, 8Dh, 44h, 13h, 50h, 0B8h, 1, 0, 50h
                db 0E8h, 0C3h, 0ADh, 83h, 0C4h, 4, 57h, 0E8h, 0E0h, 0ADh
                db 83h, 0C4h, 2, 83h, 46h, 0FAh, 2, 46h, 83h, 0FEh, 4
                db 7Ch, 0CCh, 89h, 7Eh, 0FEh, 89h, 76h, 0FCh, 0E8h, 26h
                db 0B2h, 80h, 3Eh, 2Fh, 4, 0, 74h, 51h, 2Bh, 0C0h, 2 dup(50h)
                db 0B8h, 2 dup(0FFh), 50h, 0E8h, 0B4h, 0AEh, 83h, 0C4h
                db 6, 0C7h, 46h, 0FCh, 2 dup(0), 83h, 3Eh, 26h, 4, 0, 7Eh
                db 27h, 0A0h, 92h, 3, 0FEh, 0C0h, 88h, 46h, 0FAh, 0BEh
                db 16h, 4, 8Bh, 0Eh, 26h, 4, 8Bh, 0C1h, 1, 46h, 0FCh, 0B8h
                db 82h, 0, 0F7h, 2Ch, 8Bh, 0D8h, 8Ah, 46h, 0FAh, 88h, 87h
                db 2Bh, 7Eh, 83h, 0C6h, 2, 0E2h, 0EDh, 0A0h, 92h, 3, 0A2h
                db 0D4h, 3, 0E8h, 65h, 0ACh, 0E8h
byte_1C1DA      db 0Eh, 0, 0EBh, 5, 2 dup(90h), 0E8h, 0F3h, 0B0h, 5Eh
                                        ; CODE XREF: seg002:08CD↑J
                db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h
sub_1C130       endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1EA       proc near               ; CODE XREF: seg002:07F5↑J
                                        ; sub_1CB40+B9↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                call    thk_2PLAY_B0F6
                mov     byte_1DBE8, 0
                mov     al, byte_1DC1E
                mov     [bp+var_2], al
                call    sub_1C5C0
                mov     al, byte_1DBE2
                mov     [bp+var_4], al
                mov     byte_1DBE2, 0FFh
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                call    thk_res_49E2
                mov     ax, 20E2h
                push    ax
                call    thk_res_410A
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_142A
                add     sp, 4
                sub     ax, ax          ; CODE XREF: seg002:08E5↑J
                push    ax
                call    thk_res_1392
                add     sp, 2           ; CODE XREF: seg002:0639↑J
                mov     al, [bp+var_2]
                mov     byte_1DC1E, al
                mov     byte_1DBE9, 0FFh
                mov     byte_1DBE2, 0FFh
                mov     byte_1DBEC, 7
                mov     al, byte_1DBE4
                sub     ah, ah
                push    ax
                mov     al, byte_1DBE3
                push    ax
                mov     al, byte_1DC24
                push    ax
                call    thk_2PLAY_B5EA
                add     sp, 6
                mov     word_1DC76, 0
                mov     si, 416h
                mov     cx, word_1DC76

loc_1C27A:                              ; CODE XREF: sub_1C1EA+A5↓j
                cmp     word ptr [si], 0FFFFh
                jnz     short loc_1C286

loc_1C27F:                              ; CODE XREF: sub_1C1EA+A3↓j
                mov     word_1DC76, cx
                jmp     short loc_1C292
; ---------------------------------------------------------------------------
                align 2

loc_1C286:                              ; CODE XREF: sub_1C1EA+93↑j
                add     si, 2
                inc     cx
                cmp     cx, 8
                jge     short loc_1C27F
                jmp     short loc_1C27A
; ---------------------------------------------------------------------------
                align 2

loc_1C292:                              ; CODE XREF: sub_1C1EA+99↑j
                mov     byte_1DC7E, 1
                mov     byte_1DBEB, 0
                mov     byte_1DC80, 0
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C1EA       endp


; =============== S U B R O U T I N E =======================================


sub_1C2A6       proc near               ; CODE XREF: sub_1C5C0:loc_1C828↓p
                                        ; sub_1C5C0+521↓p
                mov     ax, 17h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 20E4h
                push    ax
                call    thk_res_1726
                add     sp, 2
                retn
sub_1C2A6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C2C0       proc near               ; CODE XREF: seg002:026D↑J
                                        ; sub_1C5C0+255↓p

var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

; FUNCTION CHUNK AT C536 SIZE 0000006B BYTES
; FUNCTION CHUNK AT C5A2 SIZE 0000001E BYTES

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     ax, 2
                push    ax
                call    thk_res_165C
                add     sp, 2
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                cmp     [bp+arg_0], 0
                jnz     short loc_1C2F4
                mov     ax, 20FBh
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 2107h
                jmp     short loc_1C301
; ---------------------------------------------------------------------------

loc_1C2F4:                              ; CODE XREF: sub_1C2C0+23↑j
                mov     ax, 2112h
                push    ax

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                call    thk_res_1726
                add     sp, 2
                mov     ax, 211Eh

loc_1C301:                              ; CODE XREF: sub_1C2C0+32↑j
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                sub     ax, ax
                push    ax
                call    thk_res_165C
                add     sp, 2
                sub     si, si
                mov     di, [bp+arg_0]

loc_1C316:                              ; CODE XREF: sub_1C2C0+1E4↓j
                mov     ax, si
                add     ax, di
                mov     cx, 82h
                imul    cx
                mov     bx, ax
                mov     al, [bx+7E2Bh]
                sub     ah, ah
                and     ax, 7Fh
                mov     [bp+var_2], ax
                mov     ax, si
                cwd
                mov     cx, 0Ch
                idiv    cx
                add     dx, 5
                push    dx
                cmp     si, 0Bh
                jle     short loc_1C344
                mov     ax, 1
                jmp     short loc_1C346
; ---------------------------------------------------------------------------
                align 2

loc_1C344:                              ; CODE XREF: sub_1C2C0+7C↑j
                sub     ax, ax

loc_1C346:                              ; CODE XREF: sub_1C2C0+81↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     [bp+var_8], 20h ; ' '
                cmp     [bp+arg_2], 0
                jz      short loc_1C378
                mov     ax, si
                add     ax, di
                push    ax
                call    thk_res_27E4
                add     sp, 2
                or      ax, ax
                jz      short loc_1C378
                mov     ax, [bp+arg_2]
                cmp     [bp+var_2], ax  ; CODE XREF: seg002:0471↑J
                jnz     short loc_1C378
                mov     [bp+var_8], 17h

loc_1C378:                              ; CODE XREF: sub_1C2C0+9B↑j
                                        ; sub_1C2C0+AA↑j ...
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, si
                add     ax, 41h ; 'A'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_res_0D22
                add     sp, 2
                cmp     [bp+arg_2], 0
                jz      short loc_1C3BC
                mov     ax, [bp+arg_2]
                cmp     [bp+var_2], ax
                jnz     short loc_1C3B8
                mov     ax, 1
                jmp     short loc_1C3C3
; ---------------------------------------------------------------------------
                align 2

loc_1C3B8:                              ; CODE XREF: sub_1C2C0+F0↑j
                sub     ax, ax
                jmp     short loc_1C3C3
; ---------------------------------------------------------------------------

loc_1C3BC:                              ; CODE XREF: sub_1C2C0+E8↑j
                cmp     [bp+var_2], 1
                sbb     ax, ax
                inc     ax

loc_1C3C3:                              ; CODE XREF: sub_1C2C0+F5↑j
                                        ; sub_1C2C0+FA↑j
                mov     [bp+var_6], ax
                cmp     di, 18h
                jnz     short loc_1C3DB
                or      ax, ax
                jz      short loc_1C3DB
                cmp     byte ptr [si+3F6h], 0
                jnz     short loc_1C3DB
                mov     [bp+var_6], 0

loc_1C3DB:                              ; CODE XREF: sub_1C2C0+109↑j
                                        ; sub_1C2C0+10D↑j ...
                cmp     [bp+var_6], 0
                jnz     short loc_1C3E4
                jmp     loc_1C494
; ---------------------------------------------------------------------------

loc_1C3E4:                              ; CODE XREF: sub_1C2C0+11F↑j
                mov     ax, si
                add     ax, di
                mov     cx, 82h
                imul    cx
                mov     [bp+var_A], ax
                add     ax, 7E20h
                push    ax
                call    thk_res_1726    ; CODE XREF: seg002:047D↑J
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     bx, [bp+var_A]
                mov     bl, [bx+7E2Fh]
                sub     bh, bh
                shl     bx, 1
                mov     bx, [bx+446h]
                mov     al, [bx]
                sub     ah, ah
                push    ax
                call    thk_res_0D22
                add     sp, 2
                cmp     [bp+arg_2], 0
                jnz     short loc_1C454
                mov     ax, 2Fh ; '/'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     ax, si
                add     ax, di
                mov     cx, 82h
                imul    cx
                mov     bx, ax
                mov     al, [bx+7E2Bh]
                sub     ah, ah
                and     ax, 7Fh
                push    ax
                call    thk_res_1940
                add     sp, 6
                jmp     short loc_1C49E
; ---------------------------------------------------------------------------
                align 2

loc_1C454:                              ; CODE XREF: sub_1C2C0+162↑j
                mov     ax, si
                add     ax, di
                mov     cx, 82h
                imul    cx
                add     ax, 7E2Fh
                mov     [bp+var_C], ax  ; CODE XREF: seg002:086D↑J
                mov     bx, ax
                mov     bl, [bx]
                sub     bh, bh
                shl     bx, 1
                mov     bx, [bx+446h]
                mov     al, [bx+1]
                sub     ah, ah
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     bx, [bp+var_C]
                mov     bl, [bx]
                sub     bh, bh
                shl     bx, 1
                mov     bx, [bx+446h]
                mov     al, [bx+2]
                sub     ah, ah
                push    ax

loc_1C48E:                              ; CODE XREF: seg002:0B31↑J
                call    thk_res_0D22
                jmp     short loc_1C49B
; ---------------------------------------------------------------------------
                align 2

loc_1C494:                              ; CODE XREF: sub_1C2C0+121↑j
                mov     ax, 2129h
                push    ax
                call    thk_res_1726

loc_1C49B:                              ; CODE XREF: sub_1C2C0+1D1↑j
                add     sp, 2

loc_1C49E:                              ; CODE XREF: sub_1C2C0+191↑j
                inc     si
                cmp     si, 18h
                jge     short loc_1C4A7
                jmp     loc_1C316
; ---------------------------------------------------------------------------

loc_1C4A7:                              ; CODE XREF: sub_1C2C0+1E2↑j
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C4B0:                              ; CODE XREF: sub_1C5C0+473↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     [bp+var_4], 0
                cmp     [bp+arg_0], 18h
                jl      short loc_1C4CB
                mov     [bp+var_4], 18h
                sub     [bp+arg_0], 18h

loc_1C4CB:                              ; CODE XREF: sub_1C2C0+200↑j
                mov     si, [bp+arg_0]
                add     si, [bp+var_4]
                push    si
                call    thk_res_27E4
                add     sp, 2
                or      ax, ax
                jz      short loc_1C512
                push    si
                call    thk_res_2852
                add     sp, 2
                mov     ax, [bp+arg_0]
                cwd
                mov     cx, 0Ch
                idiv    cx
                add     dx, 5
                push    dx
                cmp     [bp+arg_0], 0Bh
                jle     short loc_1C4FC
                mov     ax, 1
                jmp     short loc_1C4FE
; ---------------------------------------------------------------------------
                align 2

loc_1C4FC:                              ; CODE XREF: sub_1C2C0+234↑j
                sub     ax, ax

loc_1C4FE:                              ; CODE XREF: sub_1C2C0+239↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 20h ; ' '
                jmp     loc_1C5B4
; ---------------------------------------------------------------------------
                align 2

loc_1C512:                              ; CODE XREF: sub_1C2C0+21A↑j
                cmp     [bp+var_4], 0
                jnz     short loc_1C53B
                cmp     byte_22E0E, 6
                jnb     short loc_1C536
                mov     al, byte_22E0E
                sub     ah, ah
                mov     cl, byte_22E0F
                sub     ch, ch
                add     ax, cx
sub_1C2C0       endp


loc_1C52C:                              ; CODE XREF: seg002:0879↑J
                cmp     ax, 8
                jz      short loc_1C536
                mov     ax, 1
                jmp     short loc_1C538
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_1C2C0

loc_1C536:                              ; CODE XREF: sub_1C2C0+25D↑j
                                        ; ovl_1RETINN:C52F↑j
                sub     ax, ax

loc_1C538:                              ; CODE XREF: ovl_1RETINN:C534↑j
                mov     [bp+var_2], ax

loc_1C53B:                              ; CODE XREF: sub_1C2C0+256↑j
                cmp     [bp+var_4], 18h
                jnz     short loc_1C56F
                mov     al, byte_22E0E
                sub     ah, ah
                mov     cl, byte_22E0F
                sub     ch, ch
                add     ax, cx
                cmp     ax, 8
                jz      short loc_1C558
                mov     ax, 1
                jmp     short loc_1C55A
; ---------------------------------------------------------------------------

loc_1C558:                              ; CODE XREF: sub_1C2C0+291↑j
                sub     ax, ax

loc_1C55A:                              ; CODE XREF: sub_1C2C0+296↑j
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C56F
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+3F6h], 1
                sbb     ax, ax
                inc     ax
                mov     [bp+var_2], ax

loc_1C56F:                              ; CODE XREF: sub_1C2C0+27F↑j
                                        ; sub_1C2C0+29F↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C5BB
                mov     bx, word_1DC76
                inc     word_1DC76
                shl     bx, 1
                mov     ax, [bp+var_4]
                add     ax, [bp+arg_0]
                mov     [bx+416h], ax
                mov     ax, [bp+arg_0]
                cwd
                mov     cx, 0Ch
                idiv    cx
                add     dx, 5
                push    dx
                cmp     [bp+arg_0], 0Bh
                jle     short loc_1C5A2
                mov     ax, 1
                jmp     short loc_1C5A4
; END OF FUNCTION CHUNK FOR sub_1C2C0
; ---------------------------------------------------------------------------
                align 2
; START OF FUNCTION CHUNK FOR sub_1C2C0

loc_1C5A2:                              ; CODE XREF: sub_1C2C0+2DA↑j
                sub     ax, ax

loc_1C5A4:                              ; CODE XREF: sub_1C2C0+2DF↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_res_1676    ; CODE XREF: seg002:0885↑J
                add     sp, 4
                mov     ax, 17h

loc_1C5B4:                              ; CODE XREF: sub_1C2C0+24E↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2

loc_1C5BB:                              ; CODE XREF: sub_1C2C0+2B3↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1C2C0

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5C0       proc near               ; CODE XREF: seg002:029D↑J
                                        ; sub_1C1EA+15↑p

var_12          = word ptr -12h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     [bp+var_A], 0
                mov     [bp+var_E], 1
                mov     [bp+var_2], 0
                sub     ax, ax          ; CODE XREF: seg002:01A1↑J
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                call    thk_res_3204
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 3
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 2138h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 214Ah
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 2
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 2169h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 2
                push    ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 2175h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_res_1676
                add     sp, 4           ; CODE XREF: seg002:0891↑J
                mov     ax, 2180h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 2
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 2186h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C692:                              ; CODE XREF: sub_1C5C0+552↓j
                sub     al, al
                mov     byte_22E0F, al
                mov     byte_22E0E, al
                mov     [bp+var_4], 0
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------
                align 2

loc_1C6A2:                              ; CODE XREF: sub_1C5C0+101↓j
                                        ; sub_1C5C0+106↓j
                inc     byte_22E0F

loc_1C6A6:                              ; CODE XREF: sub_1C5C0+FD↓j
                                        ; sub_1C5C0+10C↓j
                inc     [bp+var_4]

loc_1C6A9:                              ; CODE XREF: sub_1C5C0+DF↑j
                mov     ax, word_1DC76
                cmp     [bp+var_4], ax
                jge     short loc_1C6CE
                mov     bx, [bp+var_4]
                shl     bx, 1
                mov     si, [bx+416h]
                cmp     si, 0FFFFh
                jz      short loc_1C6A6
                or      si, si
                jl      short loc_1C6A2
                cmp     si, 17h
                jg      short loc_1C6A2
                inc     byte_22E0E
                jmp     short loc_1C6A6
; ---------------------------------------------------------------------------

loc_1C6CE:                              ; CODE XREF: sub_1C5C0+EF↑j
                mov     ax, 2
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     al, byte_22E0E
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 2
                push    ax
                mov     ax, 25h ; '%'

loc_1C6F2:                              ; CODE XREF: seg002:0B19↑J
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     al, byte_22E0F
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                cmp     byte_22E0E, 0
                jz      short loc_1C72A
                mov     ax, 15h
                push    ax
                mov     ax, 1Bh
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 218Fh
                push    ax
                call    thk_res_1726
                add     sp, 2
                jmp     short loc_1C740
; ---------------------------------------------------------------------------
                align 2

loc_1C72A:                              ; CODE XREF: sub_1C5C0+14D↑j
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 15h
                push    ax
                mov     ax, 1Bh
                push    ax

loc_1C73A:                              ; CODE XREF: seg002:089D↑J
                call    thk_res_3292
                add     sp, 8

loc_1C740:                              ; CODE XREF: sub_1C5C0+167↑j
                mov     ax, 4
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     al, byte_22E0E
                sub     ah, ah
                mov     cl, byte_22E0F
                sub     ch, ch
                add     ax, cx
                cmp     ax, 8
                jnz     short loc_1C780
                mov     ax, 1
                push    ax
                call    thk_res_163E
                add     sp, 2
                mov     ax, 219Bh
                push    ax
                call    thk_res_1726
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_163E
                add     sp, 2
                jmp     short loc_1C796
; ---------------------------------------------------------------------------
                align 2

loc_1C780:                              ; CODE XREF: sub_1C5C0+19E↑j
                mov     ax, 4
                push    ax
                mov     ax, 1Eh
                push    ax
                mov     ax, 4
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_res_3292
                add     sp, 8

loc_1C796:                              ; CODE XREF: sub_1C5C0+1BD↑j
                cmp     [bp+var_E], 0
                jnz     short loc_1C79F
                jmp     loc_1C861
; ---------------------------------------------------------------------------

loc_1C79F:                              ; CODE XREF: sub_1C5C0+1DA↑j
                sub     ax, ax
                push    ax
                mov     bl, byte_1DBE2
                sub     bh, bh
                shl     bx, 1
                mov     bx, [bx+43Ch]
                mov     di, bx
                mov     ax, ds
                mov     es, ax
                assume es:DGROUP
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                shr     cx, 1
                sub     cx, 12h
                neg     cx
                push    cx
                call    thk_res_1676
                add     sp, 4
                mov     ax, 28h ; '('
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     al, byte_1DBE2
                sub     ah, ah
                add     ax, 31h ; '1'
                push    ax
                call    thk_res_0D22

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                add     sp, 2
                mov     ax, 2Dh ; '-'

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     bl, byte_1DBE2
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+43Ch]
                call    thk_res_1726
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     al, byte_1DBE2
                sub     ah, ah
                inc     ax
                push    ax
                push    [bp+var_A]
                call    sub_1C2C0
                add     sp, 4
                cmp     byte_1DBED, 2
                jnz     short loc_1C828
                call    thk_res_5440
                jmp     short loc_1C82B
; ---------------------------------------------------------------------------
                align 2

loc_1C828:                              ; CODE XREF: sub_1C5C0+260↑j
                call    sub_1C2A6

loc_1C82B:                              ; CODE XREF: sub_1C5C0+265↑j
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 21B1h
                push    ax
                call    thk_res_1726
                add     sp, 2
                cmp     [bp+var_A], 0
                jle     short loc_1C84E
                mov     bx, 1
                jmp     short loc_1C850
; ---------------------------------------------------------------------------

loc_1C84E:                              ; CODE XREF: sub_1C5C0+287↑j
                sub     bx, bx

loc_1C850:                              ; CODE XREF: sub_1C5C0+28C↑j
                shl     bx, 1
                push    word ptr [bx+5F4h]
                call    thk_res_1726
                add     sp, 2
                mov     [bp+var_E], 0

loc_1C861:                              ; CODE XREF: sub_1C5C0+1DC↑j
                mov     ax, 7Ah ; 'z'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_8], ax
                cmp     ax, 41h ; 'A'
                jnb     short loc_1C883
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C883:                              ; CODE XREF: sub_1C5C0+2BE↑j
                cmp     ax, 58h ; 'X'
                jbe     short loc_1C88B
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C88B:                              ; CODE XREF: sub_1C5C0+2C6↑j
                add     ax, [bp+var_A]
                mov     cx, 82h
                mul     cx
                mov     bx, ax
                mov     al, [bx+5D29h]
                sub     ah, ah
                mov     cl, byte_1DBE2
                sub     ch, ch
                inc     cx
                cmp     ax, cx
                jz      short loc_1C8A9
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C8A9:                              ; CODE XREF: sub_1C5C0+2E4↑j
                cmp     [bp+var_A], 18h
                jnz     short loc_1C8B8
                mov     bx, [bp+var_8]
                cmp     [bx+3B5h], ah
                jnz     short loc_1C8C1

loc_1C8B8:                              ; CODE XREF: sub_1C5C0+2ED↑j
                cmp     [bp+var_A], 0
                jz      short loc_1C8C1
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C8C1:                              ; CODE XREF: sub_1C5C0+2F6↑j
                                        ; sub_1C5C0+2FC↑j
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_142A
                add     sp, 4
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     [bp+var_12], ax
                mov     ax, [bp+var_A]
                add     ax, [bp+var_12]
                sub     ax, 41h ; 'A'
                push    ax
                push    [bp+var_12]
                call    thk_res_2A6A
                add     sp, 4
                mov     ax, [bp+var_8]
                add     ax, [bp+var_A]
                mov     cx, 82h
                mul     cx
                mov     di, ax
                add     di, 5D1Eh

loc_1C8FA:                              ; CODE XREF: sub_1C5C0+390↓j
                mov     ax, 15h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 21BEh
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 76h ; 'v'
                push    ax
                mov     ax, 56h ; 'V'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 56h ; 'V'
                jnz     short loc_1C94D
                mov     ax, 15h
                push    ax
                mov     ax, 1Dh
                push    ax
                mov     ax, 15h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_res_3292
                add     sp, 8
                push    di
                call    thk_res_4EA6
                add     sp, 2

loc_1C94D:                              ; CODE XREF: sub_1C5C0+36E↑j
                cmp     si, 1Bh
                jnz     short loc_1C8FA
                mov     [bp+var_C], si
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_142A
                add     sp, 4

loc_1C962:                              ; CODE XREF: sub_1C5C0+2C0↑j
                                        ; sub_1C5C0+2C8↑j ...
                cmp     [bp+var_8], 20h ; ' '
                jnz     short loc_1C97C
                add     [bp+var_A], 18h
                cmp     [bp+var_A], 18h
                jle     short loc_1C977
                mov     [bp+var_A], 0

loc_1C977:                              ; CODE XREF: sub_1C5C0+3B0↑j
                mov     [bp+var_E], 1

loc_1C97C:                              ; CODE XREF: sub_1C5C0+3A6↑j
                cmp     [bp+var_8], 31h ; '1'
                jb      short loc_1C9EC
                cmp     [bp+var_8], 35h ; '5'
                ja      short loc_1C9EC
                mov     [bp+var_E], 1
                mov     al, byte ptr [bp+var_8]
                sub     al, 31h ; '1'
                mov     byte_1DBE2, al
                mov     [bp+var_6], 8

loc_1C99A:                              ; CODE XREF: seg002:07DD↑J
                mov     ax, 0FFFFh
                mov     cx, 8
                mov     di, 416h
                push    ds
                pop     es
                repne stosw
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     [bp+var_6], 14h
                mov     si, 14h

loc_1C9C8:                              ; CODE XREF: sub_1C5C0+413↓j
                mov     ax, 5
                push    ax
                call    thk_res_0D22
                add     sp, 2
                dec     si
                jnz     short loc_1C9C8
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     word_1DC76, 0   ; CODE XREF: seg002:0B01↑J
                mov     [bp+var_A], 0

loc_1C9EC:                              ; CODE XREF: sub_1C5C0+3C0↑j
                                        ; sub_1C5C0+3C6↑j
                cmp     [bp+var_8], 1
                jl      short loc_1CA3E
                cmp     [bp+var_8], 18h
                jg      short loc_1CA3E
                mov     ax, [bp+var_8]
                add     ax, [bp+var_A]
                mov     cx, 82h
                imul    cx
                mov     bx, ax
                mov     al, [bx+7DA9h]
                sub     ah, ah
                mov     cl, byte_1DBE2
                sub     ch, ch
                inc     cx
                cmp     ax, cx
                jnz     short loc_1CA3E
                cmp     [bp+var_A], 18h
                jnz     short loc_1CA25
                mov     bx, [bp+var_8]
                cmp     [bx+3F5h], ah
                jnz     short loc_1CA2B

loc_1CA25:                              ; CODE XREF: sub_1C5C0+45A↑j
                cmp     [bp+var_A], 0
                jnz     short loc_1CA3E

loc_1CA2B:                              ; CODE XREF: sub_1C5C0+463↑j
                mov     ax, [bp+var_8]
                add     ax, [bp+var_A]
                dec     ax
                push    ax
                call    loc_1C4B0
                add     sp, 2
                mov     [bp+var_E], 0

loc_1CA3E:                              ; CODE XREF: sub_1C5C0+430↑j
                                        ; sub_1C5C0+436↑j ...
                cmp     [bp+var_8], 1Bh
                jz      short loc_1CA47
                jmp     loc_1CAEF
; ---------------------------------------------------------------------------

loc_1CA47:                              ; CODE XREF: sub_1C5C0+482↑j
                cmp     byte_1DBED, 2
                jnz     short loc_1CA51
                jmp     loc_1CAEF
; ---------------------------------------------------------------------------

loc_1CA51:                              ; CODE XREF: sub_1C5C0+48C↑j
                                        ; seg002:0A89↑J
                call    thk_res_35A8
                mov     ax, 17h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 5Bh ; '['
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 21D2h
                push    ax

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                call    thk_res_1726
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 5Dh ; ']'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 17h
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_res_1676
                add     sp, 4
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------

loc_1CAC0:                              ; CODE XREF: sub_1C5C0+514↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_1CAD6

loc_1CAC5:                              ; CODE XREF: sub_1C5C0+4FE↑j
                call    thk_res_2DFE
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1CAC0

loc_1CAD6:                              ; CODE XREF: sub_1C5C0+503↑j
                mov     [bp+var_8], si
                cmp     si, 4Eh ; 'N'
                jnz     short loc_1CAEC
                call    thk_res_35A8
                call    sub_1C2A6
                mov     [bp+var_8], 0
                jmp     short loc_1CAEF
; ---------------------------------------------------------------------------
                align 2

loc_1CAEC:                              ; CODE XREF: sub_1C5C0+51C↑j
                call    thk_res_3FC4

loc_1CAEF:                              ; CODE XREF: sub_1C5C0+484↑j
                                        ; sub_1C5C0+48E↑j ...
                cmp     [bp+var_8], 5Ah ; 'Z'
                jnz     short loc_1CB0C
                mov     al, byte_22E0E
                sub     ah, ah
                mov     cl, byte_22E0F
                sub     ch, ch
                or      ax, cx
                jz      short loc_1CB0C
                inc     [bp+var_2]
                mov     [bp+var_8], 1Bh

loc_1CB0C:                              ; CODE XREF: sub_1C5C0+533↑j
                                        ; sub_1C5C0+542↑j
                cmp     [bp+var_8], 1Bh
                jz      short loc_1CB15
                jmp     loc_1C692
; ---------------------------------------------------------------------------

loc_1CB15:                              ; CODE XREF: sub_1C5C0+550↑j
                mov     al, byte_1DBE2
                mov     byte_1DC24, al
                sub     ah, ah
                mov     si, ax
                mov     al, [si+21E8h]
                mov     byte_1DBE3, al
                mov     al, [si+21EEh]
                mov     byte_1DBE4, al
                mov     al, [si+21F4h]
                mov     byte_1DC1F, al
                call    thk_res_423E
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C5C0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB40       proc near               ; CODE XREF: seg002:0A65↑J

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                inc     word_1DC5E
                mov     ax, 11h
                push    ax
                mov     ax, 21h ; '!'
                push    ax
                mov     ax, 6
                push    ax
                mov     ax, 7
                push    ax
                call    thk_res_0B0E
                add     sp, 8
                mov     [bp+var_4], ax
                push    ax
                call    thk_res_0F8A
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_15CA
                add     sp, 2
                mov     al, byte_1DB92
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                cmp     byte_1DB96, 3
                jnz     short loc_1CB9D
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2           ; CODE XREF: seg002:080D↑J

loc_1CB9D:                              ; CODE XREF: sub_1CB40+4F↑j
                call    thk_res_3226
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                sub     si, si
                mov     di, 22A6h

loc_1CBB1:                              ; CODE XREF: sub_1CB40+8E↓j
                lea     ax, [si+1]
                push    ax
                mov     ax, 2
                push    ax
                call    thk_res_1676
                add     sp, 4
                push    word ptr [di]
                call    thk_res_1726
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Ah
                jl      short loc_1CBB1
                mov     [bp+var_2], si
                push    [bp+var_4]
                call    thk_res_0FF2
                add     sp, 2

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                mov     ax, 7
                push    ax
                call    thk_res_57E0
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                call    thk_res_324E
                add     sp, 2
                mov     al, byte_1DC24
                mov     byte_1DBE2, al
                call    thk_res_276C
                call    sub_1C1EA
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 10h
sub_1CB40       endp

ovl_1RETINN     ends

