; ===========================================================================

; Segment type: Pure code
ovl_2CAVES      segment byte public 'CODE' use16
                assume cs:ovl_2CAVES
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C130       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 8, 57h, 56h, 80h, 0Eh, 30h, 4, 6, 2Bh
                                        ; DATA XREF: seg002:0038↑o
                db 0C0h, 50h, 0E8h, 0B3h, 0ACh, 83h, 0C4h, 2, 2Bh, 0F6h
                db 0BFh, 6Ah, 34h, 8Dh, 44h, 13h, 50h, 0B8h, 5, 0, 50h
                db 0E8h, 0D8h, 0ADh, 83h, 0C4h, 4, 0FFh, 35h, 0E8h, 0F4h
                db 0ADh, 83h, 0C4h, 2, 83h, 0C7h, 2, 46h, 83h, 0FEh, 4
                db 7Ch, 0E1h, 89h, 76h, 0FAh, 0B8h, 15h, 0, 50h, 0B8h
                db 19h, 0, 50h, 0E8h, 0B6h, 0ADh, 83h, 0C4h, 4, 0B8h, 2
                db 0, 50h, 0E8h, 5Ch, 0AFh, 83h, 0C4h, 2, 8Bh, 0F0h, 83h
                db 0FEh, 0Fh, 7Fh, 0EFh, 89h, 76h, 0FAh, 8Ah, 46h, 0FAh
                db 88h, 46h, 0FEh, 0B8h, 16h, 0, 50h, 0B8h, 19h, 0, 50h
                db 0E8h, 8Eh, 0ADh, 83h, 0C4h, 4, 0B8h, 2, 0, 50h, 0E8h
                db 34h, 0AFh, 83h, 0C4h, 2, 8Bh, 0F0h, 83h, 0FEh, 0Fh
                db 7Fh, 0EFh, 89h, 76h, 0FAh, 8Ah, 46h, 0FAh, 88h, 46h
                db 0FCh, 8Ah, 46h, 0FEh, 0A2h, 93h, 3, 8Ah, 46h, 0FCh
                db 0A2h, 94h, 3, 0E8h, 0DAh, 0B0h, 0E8h, 7, 0B1h, 0C6h
                db 6, 2Eh, 4, 1, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h
sub_1C130       endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1DA       proc near               ; CODE XREF: seg002:08CD↑J

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     [bp+var_4], 0
                sub     ax, ax
                mov     cx, 5           ; CODE XREF: seg002:07F5↑J
                mov     di, 9680h
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                stosb
                add     [bp+var_4], 0Bh
                mov     byte_1DD58, 0
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_2], al
                dec     [bp+var_2]
                mov     al, byte ptr word_1DBE4
                mov     cl, 4
                shl     al, cl
                add     [bp+var_2], al
                sub     si, si
                mov     cx, word_1DC76
                jmp     short loc_1C22C
; ---------------------------------------------------------------------------
                align 2

loc_1C224:                              ; CODE XREF: sub_1C1DA+54↓j
                mov     al, [bp+var_2]
                mov     [si-6980h], al
                inc     si

loc_1C22C:                              ; CODE XREF: sub_1C1DA+47↑j
                cmp     si, cx
                jl      short loc_1C224
                mov     [bp+var_4], si
                call    thk_res_3EB2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C1DA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C23C       proc near               ; CODE XREF: seg002:08E5↑J

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                push    di
                push    si
                sub     cx, cx
                sub     si, si
                mov     dl, byte ptr word_1DBE4

loc_1C24C:                              ; CODE XREF: sub_1C23C+8A↓j
                mov     al, byte_1DBE3
                cmp     [si+3486h], al
                jnz     short loc_1C25C
                cmp     [si+3490h], dl
                jnz     short loc_1C25C
                inc     cx

loc_1C25C:                              ; CODE XREF: sub_1C23C+17↑j
                                        ; sub_1C23C+1D↑j
                or      cx, cx
                jz      short loc_1C2C0

loc_1C260:                              ; CODE XREF: sub_1C23C+88↓j
                mov     [bp+var_6], si
                mov     [bp+var_4], cx
                cmp     si, 0Ah
                jl      short loc_1C26E
                jmp     loc_1C302
; ---------------------------------------------------------------------------

loc_1C26E:                              ; CODE XREF: sub_1C23C+2D↑j
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte_1DBE3
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh
                mov     bx, [bp+var_6]
                mov     al, [bx+349Ah]
                mov     byte_1DBE3, al
                mov     al, [bx+34A4h]
                mov     byte ptr word_1DBE4, al
                call    thk_2PLAY_B75E
                mov     byte_1DC7E, 1
                or      byte_1DC80, 5
                mov     ax, 3472h
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_1C2B2:                              ; CODE XREF: sub_1C23C+7B↓j
                call    thk_res_1A30
                or      ax, ax
                jz      short loc_1C2B2
                sub     di, di
                mov     si, [bp+var_2]
                jmp     short loc_1C2F3
; ---------------------------------------------------------------------------

loc_1C2C0:                              ; CODE XREF: seg002:026D↑J
                                        ; sub_1C23C+22↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1C260
                jmp     short loc_1C24C
; ---------------------------------------------------------------------------

loc_1C2C8:                              ; CODE XREF: sub_1C23C+BB↓j
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     si, ax
                shr     byte ptr [si+70h], 1
                shr     byte ptr [si+6Dh], 1
                shr     byte ptr [si+6Ch], 1
                shr     byte ptr [si+73h], 1
                shr     word ptr [si+58h], 1
                shr     byte ptr [si+72h], 1
                shr     byte ptr [si+71h], 1
                shr     byte ptr [si+6Ah], 1
                shr     byte ptr [si+6Fh], 1
                shr     byte ptr [si+6Eh], 1
                shr     byte ptr [si+6Bh], 1
                inc     di

loc_1C2F3:                              ; CODE XREF: sub_1C23C+82↑j
                cmp     di, word_1DC76
                jl      short loc_1C2C8 ; CODE XREF: seg002:0651↑J
                mov     [bp+var_8], di
                mov     [bp+var_2], si
                call    thk_2PLAY_A580

loc_1C302:                              ; CODE XREF: sub_1C23C+2F↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C23C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C308       proc near               ; CODE XREF: seg002:08F1↑J

var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_4], 0
                mov     bx, [bp+arg_0]
                mov     al, [bx+34C0h]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_A], al
                mov     bx, [bp+arg_0]
                mov     al, [bx+34C4h]
                add     [bp+var_A], al
                dec     [bp+var_A]
                sub     di, di
                jmp     short loc_1C347
; ---------------------------------------------------------------------------
                align 2

loc_1C33E:                              ; CODE XREF: sub_1C308+66↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C370
                jmp     short loc_1C363
; ---------------------------------------------------------------------------

loc_1C346:                              ; CODE XREF: sub_1C308+70↓j
                inc     di

loc_1C347:                              ; CODE XREF: sub_1C308+33↑j
                cmp     di, word_1DC76
                jge     short loc_1C37A
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_8], 0
                mov     si, ax
                mov     dx, [bp+var_4]
                sub     cx, cx

loc_1C363:                              ; CODE XREF: sub_1C308+3C↑j
                mov     bx, cx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C36C
                inc     dx

loc_1C36C:                              ; CODE XREF: sub_1C308+61↑j
                or      dx, dx
                jz      short loc_1C33E

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                                        ; sub_1C308+3A↑j
                mov     [bp+var_4], dx
                mov     [bp+var_8], cx
                or      dx, dx
                jz      short loc_1C346

loc_1C37A:                              ; CODE XREF: sub_1C308+43↑j
                mov     [bp+var_6], di
                cmp     [bp+var_4], 0
                jz      short loc_1C3E7
                mov     si, [bp+var_8]
                mov     bx, [bp+var_2]
                mov     al, [bp+var_A]
                mov     [bx+si+3Ah], al
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 34AEh
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     al, 14h
                mul     [bp+var_A]
                add     ax, 6960h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C3C4:                              ; CODE XREF: sub_1C308+C1↓j
                call    thk_res_1A30
                or      ax, ax
                jz      short loc_1C3C4
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte_1DBE3
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh

loc_1C3E7:                              ; CODE XREF: sub_1C308+79↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C3F0:                              ; CODE XREF: sub_1C462+BE↓p
                                        ; sub_1C52C+73↓p ...
                push    bp
                mov     bp, sp
                sub     sp, 2
sub_1C308       endp


loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                cmp     word ptr [bp+4], 0
                jl      short loc_1C45A
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_res_165C
                add     sp, 2
                mov     ax, 14h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, [bp+4]
                shl     ax, 1
                mov     [bp-2], ax
                mov     bx, ax
                push    word ptr [bx+3600h]
                call    thk_res_1726
                add     sp, 2
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     bx, [bp-2]
                push    word ptr [bx+3602h]
                call    thk_res_1726
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_165C
                add     sp, 2

loc_1C453:                              ; CODE XREF: ovl_2CAVES:C458↓j
                call    thk_res_1A30
                or      ax, ax
                jz      short loc_1C453

loc_1C45A:                              ; CODE XREF: ovl_2CAVES:C3FA↑j
                call    thk_2PLAY_A580
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C462       proc near               ; CODE XREF: seg002:086D↑J

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 0
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3624h
                push    ax

loc_1C48E:                              ; CODE XREF: seg002:0B31↑J
                call    thk_res_1726
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3645h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C4AC:                              ; CODE XREF: sub_1C462+64↓j
                mov     al, byte ptr word_1DC76
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C4AC
                mov     [bp+var_6], si
                sub     [bp+var_6], 31h ; '1'
                push    [bp+var_6]
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, [bp+var_6]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1C4EE
                mov     [bp+var_2], 2
                jmp     short loc_1C4FE
; ---------------------------------------------------------------------------

loc_1C4EE:                              ; CODE XREF: sub_1C462+83↑j
                mov     bx, [bp+var_4]
                mov     ax, [bx+66h]
                or      ax, [bx+68h]
                jnz     short loc_1C4FE
                mov     [bp+var_2], 4

loc_1C4FE:                              ; CODE XREF: sub_1C462+8A↑j
                                        ; sub_1C462+95↑j
                cmp     [bp+var_2], 0
                jnz     short loc_1C51D
                mov     bx, [bp+var_4]
                mov     si, bx
                mov     ax, [si+66h]
                mov     dx, [si+68h]
                add     [bx+62h], ax
                adc     [bx+64h], dx
                sub     ax, ax
                mov     [bx+68h], ax
                mov     [bx+66h], ax

loc_1C51D:                              ; CODE XREF: sub_1C462+A0↑j
                push    [bp+var_2]
                call    loc_1C3F0
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
sub_1C462       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C52C       proc near               ; CODE XREF: seg002:0879↑J

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                or      byte_1DC80, 2
                mov     [bp+var_8], 0
                cmp     word_1DC76, 0
                jle     short loc_1C59B
                mov     [bp+var_A], 416h
                mov     di, [bp+var_8]
                mov     si, [bp+var_6]

loc_1C550:                              ; CODE XREF: sub_1C52C+67↓j
                mov     bx, [bp+var_A]
                cmp     word ptr [bx], 18h
                jge     short loc_1C58A
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     si, ax
                mov     ax, [si+66h]
                mov     dx, [si+68h]
                mov     cx, ax
                mov     bx, dx
                shl     ax, 1
                rcl     dx, 1
                add     ax, cx
                adc     dx, bx
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                sub     ax, ax
                mov     [si+68h], ax
                mov     [si+66h], ax
                mov     ax, [bp+var_4]
                add     [si+62h], ax
                adc     [si+64h], dx

loc_1C58A:                              ; CODE XREF: sub_1C52C+2A↑j
                add     [bp+var_A], 2
                inc     di
                cmp     di, word_1DC76
                jl      short loc_1C550
                mov     [bp+var_8], di
                mov     [bp+var_6], si

loc_1C59B:                              ; CODE XREF: sub_1C52C+17↑j
                mov     ax, 6
                push    ax
                call    loc_1C3F0
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C52C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5AC       proc near               ; CODE XREF: seg002:0885↑J

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     [bp+var_2], 8
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3660h
                push    ax

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                call    thk_res_1726
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3681h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C5F6:                              ; CODE XREF: sub_1C5AC+64↓j
                mov     al, byte ptr word_1DC76
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C5F6
                mov     [bp+var_A], si
                sub     ax, 31h ; '1'
                push    ax
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_8], ax
                mov     bx, ax
                cmp     word ptr [bx+5Ch], 0
                jnz     short loc_1C632
                mov     [bp+var_2], 0Ah
                jmp     short loc_1C65F
; ---------------------------------------------------------------------------
                align 2

loc_1C632:                              ; CODE XREF: sub_1C5AC+7C↑j
                mov     ax, [bx+5Ch]
                sub     dx, dx
                mov     cx, ax
                mov     bx, dx
                shl     ax, 1
                rcl     dx, 1
                shl     ax, 1
                rcl     dx, 1
                add     ax, cx
                adc     dx, bx
                shl     ax, 1
                rcl     dx, 1
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx
                mov     bx, [bp+var_8]
                add     [bx+62h], ax
                adc     [bx+64h], dx
                mov     word ptr [bx+5Ch], 0

loc_1C65F:                              ; CODE XREF: sub_1C5AC+83↑j
                push    [bp+var_2]
                call    loc_1C3F0
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
sub_1C5AC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C66E       proc near               ; CODE XREF: seg002:0891↑J

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     [bp+var_2], 0FFFFh
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 369Ch
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C6A0:                              ; CODE XREF: sub_1C66E+4C↓j
                mov     al, byte ptr word_1DC76
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C6A0
                mov     [bp+var_A], si
                sub     [bp+var_A], 31h ; '1'
                mov     bx, [bp+var_A]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1C6D6
                mov     [bp+var_2], 2
                jmp     short loc_1C735
; ---------------------------------------------------------------------------

loc_1C6D6:                              ; CODE XREF: sub_1C66E+5F↑j
                push    [bp+var_A]
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_6], ax
                mov     bx, ax
                mov     al, [bx+27h]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                mov     [bp+var_4], al  ; CODE XREF: seg002:0B19↑J
                mov     bx, [bp+var_6]
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+36B4h]
                add     [bp+var_4], al
                mov     bx, [bp+var_6]
                mov     al, [bx+20h]
                mul     [bp+var_4]
                mov     [bp+var_8], ax
                cmp     [bx+60h], ax
                jb      short loc_1C71A
                mov     [bp+var_2], 0Ch
                jmp     short loc_1C735
; ---------------------------------------------------------------------------

loc_1C71A:                              ; CODE XREF: sub_1C66E+A3↑j
                cmp     word ptr [bx+68h], 0Fh
                ja      short loc_1C72A
                jnb     short loc_1C72A
                mov     [bp+var_2], 0Eh
                jmp     short loc_1C735
; ---------------------------------------------------------------------------
                align 2

loc_1C72A:                              ; CODE XREF: sub_1C66E+B0↑j
                                        ; sub_1C66E+B2↑j
                mov     si, bx
                mov     ax, [bp+var_8]
                mov     [si+74h], ax
                mov     [bx+60h], ax

loc_1C735:                              ; CODE XREF: sub_1C66E+66↑j
                                        ; sub_1C66E+AA↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C66E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C73A       proc near               ; CODE XREF: seg002:089D↑J

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                sub     di, di
                or      byte_1DC80, 3
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                jmp     short loc_1C757
; ---------------------------------------------------------------------------

loc_1C756:                              ; CODE XREF: sub_1C73A+39↓j
                inc     si

loc_1C757:                              ; CODE XREF: sub_1C73A+1A↑j
                cmp     si, word_1DC76
                jge     short loc_1C775
                push    si
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                test    byte ptr [bx+80h], 2
                jz      short loc_1C771
                inc     di

loc_1C771:                              ; CODE XREF: sub_1C73A+34↑j
                or      di, di
                jz      short loc_1C756

loc_1C775:                              ; CODE XREF: sub_1C73A+21↑j
                mov     [bp+var_4], di
                mov     [bp+var_6], si
                or      di, di
                jz      short loc_1C7FA
                mov     ax, 14h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 36BCh
                push    ax
                call    thk_res_1726
                add     sp, 2
                call    thk_res_5440
                mov     ax, 38h ; '8'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_8], ax
                call    thk_res_35A8
                cmp     [bp+var_8], 1Bh
                jz      short loc_1C804
                sub     [bp+var_8], 30h ; '0'
                cmp     [bp+var_8], 5
                jl      short loc_1C7C8
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     word_1DC1A, ax

loc_1C7C8:                              ; CODE XREF: sub_1C73A+84↑j
                dec     [bp+var_8]
                mov     bx, [bp+var_8]
                mov     al, [bx+36DAh]
                mov     byte_1DBE3, al
                mov     al, [bx+36E2h]
                mov     byte ptr word_1DBE4, al
                sub     ah, ah
                push    ax
                mov     al, byte_1DBE3

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                push    ax
                mov     al, [bx+36EAh]
                push    ax

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                call    thk_2PLAY_B5EA
                add     sp, 6
                mov     byte_1DC80, 0
                call    thk_2PLAY_A580
                jmp     short loc_1C804
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1C7FA:                              ; CODE XREF: sub_1C73A+43↑j
                mov     ax, 10h
                push    ax
                call    loc_1C3F0
                add     sp, 2

loc_1C804:                              ; CODE XREF: sub_1C73A+7A↑j
                                        ; sub_1C73A+BC↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C80A:                              ; CODE XREF: sub_1C73A+1B7↓p
                                        ; sub_1C73A+253↓p
                push    bp
                mov     bp, sp
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     bx, [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+3A18h]
                call    thk_res_1726
                add     sp, 2

loc_1C834:                              ; CODE XREF: sub_1C73A+FF↓j
                call    thk_res_1A30
                or      ax, ax
                jz      short loc_1C834
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C83E:                              ; CODE XREF: sub_1C99A+DC↓p
                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     [bp+var_8], 0
                mov     di, [bp+var_6]
                mov     si, [bp+var_4]
                jmp     short loc_1C8BA
; ---------------------------------------------------------------------------
                align 2

loc_1C854:                              ; CODE XREF: sub_1C73A+1A3↓j
                cmp     [bp+arg_0], 1
                jnz     short loc_1C862
                mov     di, si
                add     di, 12h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C862:                              ; CODE XREF: sub_1C73A+11E↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_1C870
                mov     di, si
                add     di, 14h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C870:                              ; CODE XREF: sub_1C73A+12C↑j
                cmp     [bp+arg_0], 3
                jnz     short loc_1C87E
                mov     di, si
                add     di, 27h ; '''
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C87E:                              ; CODE XREF: sub_1C73A+13A↑j
                cmp     [bp+arg_0], 4
                jnz     short loc_1C88C
                mov     di, si
                add     di, 13h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C88C:                              ; CODE XREF: sub_1C73A+148↑j
                cmp     [bp+arg_0], 5
                jnz     short loc_1C89A
                mov     di, si
                add     di, 15h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C89A:                              ; CODE XREF: sub_1C73A+156↑j
                mov     di, si
                add     di, 11h

loc_1C89F:                              ; CODE XREF: sub_1C73A+125↑j
                                        ; sub_1C73A+133↑j ...
                mov     al, [di]
                mov     byte ptr [bp+var_2], al
                cmp     al, 5Ah ; 'Z'
                jbe     short loc_1C8AE
                mov     byte ptr [bp+var_2], 64h ; 'd'
                jmp     short loc_1C8B2
; ---------------------------------------------------------------------------

loc_1C8AE:                              ; CODE XREF: sub_1C73A+16C↑j
                add     byte ptr [bp+var_2], 0Ah

loc_1C8B2:                              ; CODE XREF: sub_1C73A+172↑j
                mov     al, byte ptr [bp+var_2]
                mov     [di], al

loc_1C8B7:                              ; CODE XREF: sub_1C73A+197↓j
                inc     [bp+var_8]

loc_1C8BA:                              ; CODE XREF: sub_1C73A+117↑j
                mov     ax, word_1DC76
                cmp     [bp+var_8], ax
                jge     short loc_1C8E8
                push    [bp+var_8]
                call    thk_res_37B6
                add     sp, 2
                mov     si, ax
                test    byte ptr [si+7Dh], 2
                jz      short loc_1C8B7
                and     byte ptr [si+7Dh], 0FDh
                cmp     [bp+arg_0], 0
                jz      short loc_1C8E0
                jmp     loc_1C854
; ---------------------------------------------------------------------------

loc_1C8E0:                              ; CODE XREF: sub_1C73A+1A1↑j
                mov     di, si
                add     di, 10h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C8E8:                              ; CODE XREF: sub_1C73A+186↑j
                mov     [bp+var_6], di
                mov     [bp+var_4], si
                push    [bp+arg_0]
                call    loc_1C80A
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C8FE:                              ; CODE XREF: sub_1C99A+EB↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     ax, 0FEh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                cmp     ax, 7Fh
                jg      short loc_1C986
                mov     [bp+var_6], 0
                mov     si, [bp+var_2]
                jmp     short loc_1C933
; ---------------------------------------------------------------------------

loc_1C928:                              ; CODE XREF: sub_1C73A+236↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C972
                jmp     short loc_1C953
; ---------------------------------------------------------------------------

loc_1C930:                              ; CODE XREF: sub_1C73A+23D↓j
                inc     [bp+var_6]

loc_1C933:                              ; CODE XREF: sub_1C73A+1EC↑j
                mov     ax, word_1DC76
                cmp     [bp+var_6], ax
                jge     short loc_1C979
                push    [bp+var_6]
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_8], 0
                mov     di, ax
                mov     dx, [bp+var_A]
                sub     cx, cx

loc_1C953:                              ; CODE XREF: sub_1C73A+1F4↑j
                mov     bx, cx
                cmp     byte ptr [bx+di+3Ah], 0
                jnz     short loc_1C96E
                mov     dx, cx
                add     dx, di
                mov     bx, dx
                mov     byte ptr [bx+3Ah], 0DAh
                mov     byte ptr [bx+40h], 0
                mov     byte ptr [bx+46h], 0
                inc     si

loc_1C96E:                              ; CODE XREF: sub_1C73A+21F↑j
                or      si, si
                jz      short loc_1C928

loc_1C972:                              ; CODE XREF: sub_1C73A+1F2↑j
                mov     [bp+var_8], cx
                or      si, si
                jz      short loc_1C930

loc_1C979:                              ; CODE XREF: sub_1C73A+1FF↑j
                mov     [bp+var_2], si
                or      si, si
                jz      short loc_1C986
                mov     ax, 0Eh
                jmp     short loc_1C98C
; ---------------------------------------------------------------------------
                align 2

loc_1C986:                              ; CODE XREF: sub_1C73A+1E2↑j
                                        ; sub_1C73A+244↑j
                mov     ax, [bp+arg_0]
                add     ax, 7

loc_1C98C:                              ; CODE XREF: sub_1C73A+249↑j
                push    ax
                call    loc_1C80A
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
sub_1C73A       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C99A       proc near               ; CODE XREF: seg002:07DD↑J

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4
arg_2           = word ptr  6

; FUNCTION CHUNK AT CB45 SIZE 00000004 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     byte ptr [bp+var_6], 0
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 3A04h

loc_1C9B9:                              ; CODE XREF: sub_1C99A+3C↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                push    word ptr [di]
                call    thk_res_1726
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1C9B9
                mov     [bp+var_4], si
                sub     ax, ax
                push    ax
                call    thk_2PLAY_941E
                add     sp, 2
                cmp     byte_1DC7F, 0   ; CODE XREF: seg002:0B01↑J
                jnz     short loc_1C9EE
                jmp     loc_1CA97
; ---------------------------------------------------------------------------

loc_1C9EE:                              ; CODE XREF: sub_1C99A+4F↑j
                call    thk_res_5440
                or      byte_1DC80, 1
                call    thk_res_34BA

loc_1C9F9:                              ; CODE XREF: sub_1C99A+F7↓j
                sub     si, si
                jmp     short loc_1C9FF
; ---------------------------------------------------------------------------
                align 2

loc_1C9FE:                              ; CODE XREF: sub_1C99A+82↓j
                inc     si

loc_1C9FF:                              ; CODE XREF: sub_1C99A+61↑j
                cmp     word_1DC76, si
                jle     short loc_1CA1E
                push    si
                call    thk_res_37B6
                add     sp, 2
                mov     bx, ax
                test    byte ptr [bx+7Dh], 2
                jz      short loc_1CA18
                mov     byte ptr [bp+var_6], 1

loc_1CA18:                              ; CODE XREF: sub_1C99A+78↑j
                cmp     byte ptr [bp+var_6], 0
                jz      short loc_1C9FE

loc_1CA1E:                              ; CODE XREF: sub_1C99A+69↑j
                mov     [bp+var_4], si
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 3A0Ch

loc_1CA30:                              ; CODE XREF: sub_1C99A+B3↓j
                lea     ax, [si+11h]
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
                cmp     si, 6
                jl      short loc_1CA30
                mov     [bp+var_4], si

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_2], ax
                cmp     ax, 1Bh
                jz      short loc_1CA8B
                sub     [bp+var_2], 31h ; '1'
                cmp     byte ptr [bp+var_6], ah
                jz      short loc_1CA82
                push    [bp+var_2]
                call    loc_1C83E
                add     sp, 2
                mov     byte ptr [bp+var_6], 0
                jmp     short loc_1CA8B
; ---------------------------------------------------------------------------

loc_1CA82:                              ; CODE XREF: sub_1C99A+D7↑j
                push    [bp+var_2]
                call    loc_1C8FE

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                add     sp, 2

loc_1CA8B:                              ; CODE XREF: sub_1C99A+CE↑j
                                        ; sub_1C99A+E6↑j
                cmp     [bp+var_2], 1Bh
                jz      short loc_1CA94
                jmp     loc_1C9F9
; ---------------------------------------------------------------------------

loc_1CA94:                              ; CODE XREF: sub_1C99A+F5↑j
                call    thk_res_35A8

loc_1CA97:                              ; CODE XREF: sub_1C99A+51↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CAA0:                              ; CODE XREF: sub_1C99A+176↓p
                                        ; sub_1C99A+192↓p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     di, di
                jmp     short loc_1CAB5
; ---------------------------------------------------------------------------

loc_1CAAC:                              ; CODE XREF: sub_1C99A+13C↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1CAD8
                jmp     short loc_1CAD1
; ---------------------------------------------------------------------------

loc_1CAB4:                              ; CODE XREF: sub_1C99A+144↓j
                inc     di

loc_1CAB5:                              ; CODE XREF: sub_1C99A+110↑j
                cmp     di, word_1DC76
                jge     short loc_1CAE0
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_6], 0
                mov     si, ax
                mov     dl, [bp+arg_0]
                sub     cx, cx

loc_1CAD1:                              ; CODE XREF: sub_1C99A+118↑j
                mov     bx, cx
                cmp     [bx+si+3Ah], dl
                jnz     short loc_1CAAC

loc_1CAD8:                              ; CODE XREF: sub_1C99A+116↑j
                mov     [bp+var_6], cx
                cmp     cx, 6
                jz      short loc_1CAB4

loc_1CAE0:                              ; CODE XREF: sub_1C99A+11F↑j
                mov     [bp+var_4], di
                cmp     [bp+var_6], 6
                jnz     short loc_1CAEE
                mov     [bp+var_6], 0FFFFh

loc_1CAEE:                              ; CODE XREF: sub_1C99A+14D↑j
                mov     bx, [bp+arg_2]
                mov     ax, [bp+var_6]
                mov     [bx], ax
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CB00:                              ; CODE XREF: ovl_2CAVES:CC25↓p
                                        ; ovl_2CAVES:CD66↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                lea     ax, [bp+var_2]
                push    ax
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    loc_1CAA0
                mov     ax, [bp+var_2]
                inc     ax
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CB1C:                              ; CODE XREF: ovl_2CAVES:CC3C↓p
                                        ; ovl_2CAVES:CD97↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                lea     ax, [bp+var_4]
                push    ax
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    loc_1CAA0
                add     sp, 4
                mov     [bp+var_2], ax
                cmp     [bp+var_4], 0FFFFh
                jz      short loc_1CB45
                push    [bp+var_4]
                push    ax
                call    thk_res_3766    ; CODE XREF: seg002:0A65↑J
sub_1C99A       endp

                add     sp, 4
; START OF FUNCTION CHUNK FOR sub_1C99A

loc_1CB45:                              ; CODE XREF: sub_1C99A+19F↑j
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1C99A
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     byte ptr [bp-2], 0
                mov     bx, [bp+4]
                test    byte ptr [bx+7Ch], 2
                jz      short loc_1CBBF
                mov     al, [bx+78h]
                mov     [bp-6], al
                cmp     byte_22E12, 0
                jnz     short loc_1CB6E
                mov     byte_22E12, al

loc_1CB6E:                              ; CODE XREF: ovl_2CAVES:CB69↑j
                mov     al, byte_22E12
                cmp     [bp-6], al
                jnz     short loc_1CBBF
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CBBF
                inc     byte ptr [bp-2]
                sub     si, si
                mov     cl, [bp-6]

loc_1CB84:                              ; CODE XREF: ovl_2CAVES:CB96↓j
                cmp     [si+3E3Eh], cl
                jbe     short loc_1CB90

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                                        ; ovl_2CAVES:CB94↓j
                mov     [bp-4], si
                jmp     short loc_1CB98
; ---------------------------------------------------------------------------
                align 2

loc_1CB90:                              ; CODE XREF: ovl_2CAVES:CB88↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1CB8A
                jmp     short loc_1CB84
; ---------------------------------------------------------------------------

loc_1CB98:                              ; CODE XREF: ovl_2CAVES:CB8D↑j
                mov     bx, [bp-4]
                shl     bx, 1           ; CODE XREF: seg002:080D↑J
                shl     bx, 1
                mov     ax, [bx+3E48h]
                mov     dx, [bx+3E4Ah]
                mov     word_216C0, ax
                mov     word_216C2, dx
                mov     bx, [bp+4]
                add     [bx+62h], ax
                adc     [bx+64h], dx
                mov     byte ptr [bx+78h], 0
                and     byte ptr [bx+7Ch], 0FDh

loc_1CBBF:                              ; CODE XREF: ovl_2CAVES:CB5C↑j
                                        ; ovl_2CAVES:CB74↑j ...
                mov     al, [bp-2]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     byte ptr [bp-6], 0
                mov     byte ptr [bp-4], 0
                mov     byte ptr [bp-2], 0 ; CODE XREF: seg002:01AD↑J
                mov     byte ptr [bp-0Ch], 0
                mov     word ptr [bp-8], 0
                mov     bx, [bp+4]
                mov     al, [bx+78h]
                mov     [bp-0Ah], al
                or      al, al
                jnz     short loc_1CBF8

loc_1CBF3:                              ; CODE XREF: ovl_2CAVES:CC0C↓j
                sub     ax, ax
                jmp     loc_1CC84
; ---------------------------------------------------------------------------

loc_1CBF8:                              ; CODE XREF: ovl_2CAVES:CBF1↑j
                cmp     byte_22E13, 0
                jnz     short loc_1CC02
                inc     byte ptr [bp-0Ch]

loc_1CC02:                              ; CODE XREF: ovl_2CAVES:CBFD↑j
                mov     al, byte_22E13
                cmp     [bp-0Ah], al
                jz      short loc_1CC0E
                or      al, al
                jnz     short loc_1CBF3

loc_1CC0E:                              ; CODE XREF: ovl_2CAVES:CC08↑j
                cmp     byte ptr [bp-0Ch], 0
                jnz     short loc_1CC17
                inc     byte ptr [bp-2]

loc_1CC17:                              ; CODE XREF: ovl_2CAVES:CC12↑j
                cmp     byte ptr [bp-0Ch], 0
                jz      short loc_1CC42
                mov     al, [bp-0Ah]
                sub     ah, ah
                mov     si, ax
                push    si
                call    loc_1CB00
                add     sp, 2
                mov     [bp-8], ax
                or      ax, ax
                jz      short loc_1CC42
                inc     byte ptr [bp-2]
                mov     al, [bp-0Ah]
                mov     byte_22E13, al
                push    si
                call    loc_1CB1C
                add     sp, 2

loc_1CC42:                              ; CODE XREF: ovl_2CAVES:CC1B↑j
                                        ; ovl_2CAVES:CC30↑j
                cmp     byte ptr [bp-2], 0
                jz      short loc_1CC7F
                mov     bx, [bp+4]
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CC7F
                inc     byte ptr [bp-6]
                mov     byte ptr [bx+78h], 0
                mov     al, 14h
                mul     byte ptr [bp-0Ah]
                mov     bx, ax
                mov     ax, [bx+6972h]
                sub     dx, dx
                mov     cl, 3

loc_1CC67:                              ; CODE XREF: ovl_2CAVES:CC6D↓j
                shl     ax, 1
                rcl     dx, 1
                dec     cl
                jnz     short loc_1CC67
                mov     word_216C0, ax
                mov     word_216C2, dx
                mov     bx, [bp+4]
                add     [bx+62h], ax
                adc     [bx+64h], dx

loc_1CC7F:                              ; CODE XREF: ovl_2CAVES:CC46↑j
                                        ; ovl_2CAVES:CC4F↑j
                mov     al, [bp-6]
                sub     ah, ah

loc_1CC84:                              ; CODE XREF: ovl_2CAVES:CBF5↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     byte ptr [bp-2], 0
                mov     ax, [bp+4]
                mov     cl, 3
                shl     ax, cl
                mov     [bp-0Ch], ax
                mov     bx, ax
                mov     al, [bx+3E1Eh]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp-6], al
                dec     byte ptr [bp-6]
                mov     word ptr [bp-4], 1 ; CODE XREF: seg002:08FD↑J
                mov     si, [bp-0Ch]
                mov     di, si
                mov     dl, [bp-2]
                mov     cx, [bp-4]

loc_1CCC9:                              ; CODE XREF: ovl_2CAVES:CCF4↓j
                mov     al, [bp-6]
                mov     bx, cx
                cmp     [bx+di+3E1Eh], al
                jbe     short loc_1CCD8
                inc     dl
                jmp     short loc_1CCE1
; ---------------------------------------------------------------------------

loc_1CCD8:                              ; CODE XREF: ovl_2CAVES:CCD2↑j
                mov     bx, cx
                mov     al, [bx+si+3E1Fh]
                sub     [bp-6], al

loc_1CCE1:                              ; CODE XREF: ovl_2CAVES:CCD6↑j
                or      dl, dl
                jz      short loc_1CCEE

loc_1CCE5:                              ; CODE XREF: ovl_2CAVES:CCF2↓j
                mov     [bp-2], dl
                mov     [bp-4], cx
                jmp     short loc_1CCF6
; ---------------------------------------------------------------------------
                align 2

loc_1CCEE:                              ; CODE XREF: ovl_2CAVES:CCE3↑j
                inc     cx
                cmp     cx, 7
                jge     short loc_1CCE5
                jmp     short loc_1CCC9
; ---------------------------------------------------------------------------

loc_1CCF6:                              ; CODE XREF: ovl_2CAVES:CCEB↑j
                dec     word ptr [bp-4]
                mov     si, [bp+4]
                mov     ax, si
                shl     si, 1
                add     si, ax
                shl     si, 1
                mov     bx, [bp-4]
                mov     al, [bx+si+3E0Ch]
                add     [bp-6], al
                mov     al, [bp-6]
                mov     byte_22E13, al
                sub     ah, ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, [bp+4]
                mov     al, [bx+3E36h]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                mov     [bp-2], al
                mov     bx, [bp+4]
                mov     al, [bx+3E3Ah]
                add     [bp-2], al
                mov     al, [bp-2]
                mov     byte_22E12, al
                sub     ah, ah
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     word ptr [bp-2], 0
                sub     si, si
                mov     di, bp
                sub     di, 0Ah

loc_1CD60:                              ; CODE XREF: ovl_2CAVES:CD75↓j
                mov     ax, si
                add     ax, 0E2h
                push    ax
                call    loc_1CB00
                add     sp, 2
                mov     [di], ax
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CD60
                mov     [bp-4], si
                cmp     word ptr [bp-0Ah], 0
                jz      short loc_1CDA6
                cmp     word ptr [bp-8], 0
                jz      short loc_1CDA6
                cmp     word ptr [bp-6], 0
                jz      short loc_1CDA6
                inc     word ptr [bp-2]
                sub     si, si

loc_1CD91:                              ; CODE XREF: ovl_2CAVES:CDA1↓j
                mov     ax, si
                add     ax, 0E2h
                push    ax
                call    loc_1CB1C
                add     sp, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CD91
                mov     [bp-4], si

loc_1CDA6:                              ; CODE XREF: ovl_2CAVES:CD7E↑j
                                        ; ovl_2CAVES:CD84↑j ...
                mov     ax, [bp-2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db  0Ch
                db  57h ; W
                db  56h ; V
                db 0C6h
                db  46h ; F
                db 0F6h
                db    1
                db  80h
                db  3Eh ; >
                db 0C4h
                db  55h ; U
                db    0
                db  75h ; u
                db  11h
                db 0FFh
                db  76h ; v
                db    4
                db 0E8h
                db 0C1h
                db 0FEh
                db  83h
                db 0C4h
                db    2
                db  88h
                db  46h ; F
                db 0FEh
                db 0FEh
                db  4Eh ; N
                db 0F6h
                db 0EBh
                db  0Ch
                db 0FFh
                db  76h ; v
                db    4
                db 0E8h
                db  42h ; B
                db 0FFh
                db  83h
                db 0C4h
                db    2
                db  88h
                db  46h ; F
                db 0FEh
                db  2Bh ; +
                db 0FFh
                db  8Bh
                db  76h ; v
                db 0FAh
                db 0EBh
                db  1Bh
                db  90h
                db  57h ; W
                db 0E8h
                db  76h ; v
                db 0A3h
                db  83h
                db 0C4h
                db    2
                db  8Bh
                db 0F0h
                db  8Ah
                db  46h ; F
                db 0FEh
                db  88h
                db  44h ; D
                db  78h ; x
                db  80h
                db  64h ; d
                db  7Ch ; |
                db 0FEh
                db  8Ah
                db  46h ; F
                db 0F6h
                db    8
                db  44h ; D
                db  7Ch ; |
                db  47h ; G
                db  3Bh ; ;
                db  3Eh ; >
                db  26h ; &
                db    4
                db  7Ch ; |
                db 0E0h
                db  89h
                db  7Eh ; ~
                db 0F8h
                db  89h
                db  76h ; v
                db 0FAh
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db 0E1h
                db  9Fh
                db  83h
                db 0C4h
                db    2
                db  80h
                db  3Eh ; >
                db 0C4h
                db  55h ; U
                db    1
                db  75h ; u
                db  17h
                db  8Ah
                db  46h ; F
                db 0FEh
                db 0A2h
                db  80h
                db  96h
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0E8h
                db 0E7h
                db 0A1h
                db  83h
                db 0C4h
                db    2
                db 0C7h
                db  46h ; F
byte_1CE30      db 0FCh, 0Eh, 9Eh, 0EBh, 0Ch, 90h, 0B0h, 14h, 0F6h, 66h
                                        ; CODE XREF: seg002:0831↑J
                db 0FEh, 5, 60h, 69h, 89h, 46h, 0FCh, 0B8h, 12h, 0, 50h
                db 0B8h, 1, 0, 50h, 0E8h, 0E2h, 0A0h, 83h, 0C4h, 4, 0B8h
                db 7Ch, 3Eh, 50h, 0E8h, 0FCh, 0A0h, 83h, 0C4h, 2, 0B8h
                db 13h, 0, 50h, 0B8h, 3, 0, 50h, 0E8h, 0CAh, 0A0h, 83h
                db 0C4h, 4, 0B8h, 0A3h, 3Eh, 50h, 0E8h, 0E4h, 0A0h, 83h
                db 0C4h, 2, 0FFh, 76h, 0FCh, 0E8h, 0DBh, 0A0h, 83h, 0C4h
                db 2, 2Bh, 0F6h, 2Bh, 0FFh, 8Dh, 44h, 14h, 50h, 0B8h, 2
                db 0, 50h, 0E8h, 0A5h, 0A0h, 83h, 0C4h, 4, 8Ah, 1Eh, 0C4h
                db 55h, 2Ah, 0FFh, 0D1h, 0E3h, 0D1h, 0E3h, 0FFh, 0B1h
                db 0F2h, 3Dh, 0E8h, 0B5h, 0A0h, 83h, 0C4h, 2, 83h, 0C7h
                db 2, 46h, 83h, 0FEh, 2, 7Ch, 0D5h, 89h, 76h, 0F8h, 5Eh
                db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 0Eh, 57h, 56h, 0C6h, 46h, 0F6h, 0, 0C6h, 46h, 0FEh
                db 10h, 0C6h, 46h, 0FCh, 1, 80h, 3Eh
byte_1CEC8      db 0C4h, 55h, 0, 75h, 7, 0C6h, 46h, 0FEh, 8, 0FEh, 4Eh
                                        ; CODE XREF: seg002:07AD↑J
                db 0FCh, 0C7h, 46h, 0F8h, 2 dup(0), 83h, 3Eh, 26h, 4, 0
                db 7Eh, 45h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 89h, 46h, 0F2h
                db 8Bh, 76h, 0F8h, 56h, 0E8h, 73h, 0A2h, 83h, 0C4h, 2
                db 8Bh, 0F8h, 8Ah, 45h, 7Ch, 88h, 46h, 0F4h, 2Ah, 0E4h
                db 85h, 46h, 0F2h, 75h, 17h, 0FEh, 46h, 0F6h, 80h, 66h
                db 0F4h, 0FEh, 80h, 4Eh, 0F4h, 4, 8Ah, 46h, 0FCh, 8, 46h
                db 0F4h, 8Ah, 46h, 0F4h, 88h, 45h, 7Ch, 46h, 3Bh, 36h
                db 26h, 4, 7Ch, 0CCh, 89h, 7Eh, 0FAh, 89h, 76h, 0F8h, 80h
                db 7Eh, 0F6h, 0, 74h, 3Ch, 0B8h
byte_1CF2C      db 2, 0, 50h, 0E8h, 0C4h, 9Eh, 83h, 0C4h, 2, 2Bh, 0F6h
                                        ; CODE XREF: seg002:050D↑J
                db 2Bh, 0FFh, 8Dh, 44h, 12h, 50h, 0B8h, 1, 0, 50h, 0E8h
                db 0EAh, 9Fh, 83h, 0C4h, 4, 8Ah, 1Eh, 0C4h, 55h, 2Ah, 0FFh
                db 0B1h, 3, 0D3h, 0E3h, 0FFh, 0B1h, 0E2h, 3Dh, 0E8h, 0FAh
                db 9Fh, 83h, 0C4h, 2, 83h
byte_1CF5C      db 0C7h, 2, 46h, 83h, 0FEh, 4, 7Ch, 0D5h, 89h, 76h, 0F8h
                                        ; CODE XREF: seg002:0AE9↑J
                db 80h, 7Eh, 0F6h, 0, 74h, 5, 0B8h, 1, 0, 0EBh, 3, 0B8h
                db 2 dup(0FFh), 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 0B8h
                db 12h, 0, 50h, 0B8h, 2, 0, 50h
byte_1CF84      db 0E8h, 0A7h, 9Fh, 83h, 0C4h, 4, 0FFh, 36h, 74h, 3Eh
                                        ; CODE XREF: seg002:062D↑J
                db 0E8h, 0C1h, 9Fh, 83h, 0C4h, 2, 0B8h, 13h, 0, 50h, 0B8h
                db 4, 0, 50h, 0E8h, 8Fh, 9Fh, 83h, 0C4h, 4, 0FFh, 36h
                db 76h, 3Eh, 0E8h, 0A9h, 9Fh, 83h, 0C4h, 2, 0A0h, 0C2h
                db 55h, 0A2h, 80h, 96h, 2Bh, 0C0h, 50h, 0E8h, 5Ah, 0A0h
                db 83h, 0C4h, 2, 0B8h, 0Eh, 9Eh, 50h, 0E8h, 90h, 9Fh, 83h
                db 0C4h, 2, 0B8h, 14h, 0, 50h, 0B8h, 3, 0, 50h, 0E8h, 5Eh
                db 9Fh, 83h, 0C4h, 4, 0FFh, 36h, 78h, 3Eh, 0E8h, 78h, 9Fh
                db 83h, 0C4h, 2, 0B8h, 15h, 0, 50h, 0B8h, 3, 0, 50h, 0E8h
                db 46h, 9Fh, 83h, 0C4h, 4, 0FFh, 36h, 7Ah, 3Eh, 0E8h, 60h
                db 9Fh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h
                db 0FFh, 36h, 72h, 3Eh, 0FFh, 36h, 70h, 3Eh, 0E8h, 4Eh
                db 0A1h, 83h, 0C4h, 8, 0C3h, 0B8h, 12h, 0, 50h, 0B8h, 2
                db 0, 50h, 0E8h, 17h, 9Fh, 83h, 0C4h, 4, 0FFh, 36h, 74h
                db 3Eh, 0E8h, 31h, 9Fh, 83h, 0C4h, 2, 0B8h, 13h, 0, 50h
                db 0B8h, 4, 0, 50h, 0E8h, 0FFh, 9Eh, 83h, 0C4h, 4, 0FFh
                db 36h, 76h, 3Eh, 0E8h, 19h, 9Fh, 83h, 0C4h, 2, 0B0h, 14h
                db 0F6h, 26h, 0C3h, 55h, 5, 60h, 69h, 50h, 0E8h, 9, 9Fh
                db 83h, 0C4h, 2, 0B8h, 14h, 0, 50h, 0B8h, 3, 0, 50h, 0E8h
                db 0D7h, 9Eh, 83h, 0C4h, 4, 0FFh, 36h, 78h, 3Eh, 0E8h
                db 0F1h, 9Eh, 83h, 0C4h, 2, 0B8h, 15h, 0, 50h, 0B8h, 3
                db 0, 50h, 0E8h, 0BFh, 9Eh, 83h, 0C4h, 4, 0FFh, 36h, 7Ah
                db 3Eh, 0E8h, 0D9h, 9Eh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
                db 0B8h, 1, 0, 50h, 0FFh, 36h, 72h, 3Eh, 0FFh, 36h, 70h
                db 3Eh, 0E8h, 0C7h, 0A0h, 83h, 0C4h, 8, 0C3h, 90h, 55h
                db 8Bh, 0ECh, 83h, 0ECh, 12h, 57h, 56h, 2Bh, 0FFh, 89h
                db 7Eh, 0EEh, 89h, 7Eh, 0F8h, 8Bh, 76h, 0FCh, 0EBh, 61h
                db 90h, 0F6h, 46h, 0FEh, 8, 75h, 10h, 0Bh, 0FFh, 75h, 5
                db 0E8h, 95h, 0FCh, 8Bh, 0F8h, 0Bh, 0FFh, 74h, 3, 0FFh
                db 46h, 0EEh, 83h, 7Eh
byte_1D0C2      db 0EEh, 0, 74h, 41h, 47h, 80h, 64h, 7Ch, 0FBh, 0C6h, 46h
                                        ; CODE XREF: seg002:04DD↑J
                db 0FEh, 10h, 80h, 3Eh, 0C4h, 55h, 0, 75h, 4, 0C6h, 46h
                db 0FEh, 8, 8Ah, 46h, 0FEh, 8, 44h, 7Ch, 0C7h, 46h, 0F0h
                db 0A0h, 86h, 0C7h, 46h, 0F2h, 1, 0, 80h, 3Eh, 0C4h, 55h
                db 1, 75h, 0Ah, 0C7h, 46h, 0F0h, 40h, 42h, 0C7h, 46h, 0F2h
                db 0Fh, 0, 8Bh, 46h, 0F0h, 8Bh, 56h, 0F2h, 1, 44h, 62h
                db 11h, 54h, 64h, 0FFh, 46h, 0F8h, 0A1h, 26h, 4, 39h, 46h
                db 0F8h, 7Dh, 46h, 0FFh, 76h, 0F8h, 0E8h, 4Ah, 0A0h, 83h
                db 0C4h, 2, 8Bh, 0F0h, 8Ah, 44h, 7Ch, 88h, 46h, 0FEh, 0F6h
                db 46h, 0FEh, 4, 74h, 0DEh, 24h, 1, 3Ah, 6, 0C4h, 55h
                db 75h, 0D6h, 80h, 3Eh, 0C4h, 55h, 1, 74h, 3, 0E9h, 6Fh
                db 0FFh, 8Ah, 46h, 0FEh, 24h, 0E0h, 3Ch, 0E0h, 74h, 3
                db 0E9h, 79h, 0FFh, 80h, 64h, 7Ch, 1Fh, 0F6h, 46h, 0FEh
                db 10h, 74h, 3, 0E9h, 6Ch, 0FFh, 0E9h, 66h, 0FFh, 90h
                db 89h, 7Eh
byte_1D15A      db 0F6h, 89h, 76h, 0FCh, 0Bh, 0FFh, 74h, 5Ch, 0B8h, 12h
                                        ; CODE XREF: seg002:0825↑J
                db 0, 50h, 0B8h, 1, 0, 50h, 0E8h, 0C1h, 9Dh, 83h, 0C4h
                db 4, 0B8h, 0BBh, 3Eh, 50h, 0E8h, 0DBh, 9Dh, 83h, 0C4h
                db 2, 0B8h, 13h, 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 0A9h, 9Dh
                db 83h, 0C4h, 4, 0B8h, 0E2h, 3Eh, 50h, 0E8h, 0C3h, 9Dh
                db 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h, 0FFh
                db 76h, 0F2h, 0FFh, 76h, 0F0h, 0E8h, 0B3h, 9Fh, 83h, 0C4h
                db 8, 0B8h, 14h, 0, 50h, 0B8h, 9, 0, 50h, 0E8h, 7Dh, 9Dh
                db 83h, 0C4h, 4, 0B8h, 0FEh, 3Eh, 50h, 0E8h, 97h, 9Dh
                db 83h, 0C4h, 2, 0C7h, 46h, 0F8h, 2 dup(0), 0EBh, 58h
                db 90h, 0FFh, 76h, 0FCh, 0E8h, 7Eh, 0F9h, 83h, 0C4h, 2
                db 88h, 46h, 0FEh, 0Ah, 0C0h, 74h, 44h, 2Bh, 0F6h, 8Bh
                db 7Eh, 0FAh, 0EBh, 9, 90h, 57h, 2 dup(0E8h), 0F9h, 83h
                db 0C4h, 2, 46h, 39h, 36h, 26h, 4, 7Eh, 16h, 56h, 0E8h
                db 72h, 9Fh, 83h, 0C4h, 2, 8Bh, 0F8h, 80h, 3Eh, 0C4h, 55h
                db 1, 75h, 0E2h, 57h, 0E8h, 4Ah, 0F9h, 0EBh, 0E0h, 89h
                db 7Eh, 0FAh, 89h, 76h, 0F4h, 80h, 3Eh, 0C4h, 55h, 0, 75h
                db 5, 0E8h, 0FAh, 0FDh, 0EBh, 3, 0E8h, 65h, 0FDh, 0FFh
                db 46h, 0F6h, 0FFh, 46h, 0F8h, 0A1h, 26h, 4, 39h, 46h
                db 0F8h, 7Dh, 23h, 0FFh, 76h, 0F8h, 0E8h, 37h, 9Fh, 83h
                db 0C4h, 2, 89h, 46h, 0FCh, 2Ah, 0C0h, 0A2h, 0C3h, 55h
                db 0A2h, 0C2h, 55h, 38h, 6, 0C4h, 55h, 75h, 87h, 0FFh
                db 76h, 0FCh, 0E8h, 85h, 0F9h, 0EBh, 85h, 90h, 8Bh, 46h
                db 0F6h, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh
                db 0ECh, 83h, 0ECh, 0Ah, 57h, 56h, 0C7h, 46h, 0FAh, 2 dup(0)
                db 2Bh, 0FFh, 8Bh, 76h, 0F8h, 0E9h, 8Ah, 0, 90h, 0B8h
                db 21h, 3Fh, 50h, 0EBh, 6Bh, 80h, 7Ch, 78h, 0, 74h, 6Eh
                db 0F6h, 46h, 0FEh, 1, 74h, 1Ah, 8Ah, 44h, 78h, 0A2h, 0C2h
                db 55h, 0A2h, 80h, 96h, 2Bh, 0C0h, 50h, 0E8h, 89h, 9Dh
                db 83h, 0C4h, 2, 0C7h, 46h, 0FCh, 0Eh, 9Eh, 0EBh, 13h
                db 90h, 8Ah, 44h, 78h, 0A2h, 0C3h, 55h, 0B0h, 14h, 0F6h
                db 26h, 0C3h, 55h, 5, 60h, 69h, 89h, 46h, 0FCh, 0B8h, 12h
                db 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 7Dh, 9Ch, 83h, 0C4h
                db 4, 0FFh, 36h, 8, 3Eh, 0E8h, 97h, 9Ch, 83h, 0C4h, 2
                db 0B8h, 13h, 0, 50h, 0B8h, 4, 0, 50h, 0E8h, 65h, 9Ch
                db 83h, 0C4h, 4, 0FFh, 36h, 0Ah, 3Eh, 0E8h, 7Fh, 9Ch, 83h
                db 0C4h, 2, 0FFh, 76h, 0FCh, 0E8h, 76h, 9Ch, 83h, 0C4h
                db 2, 0FFh, 46h, 0FAh, 83h, 7Eh, 0FAh, 0, 74h, 8, 89h
                db 7Eh, 0F6h, 89h, 76h, 0F8h, 0EBh, 5Eh, 47h, 3Bh, 3Eh
                db 26h, 4, 7Dh, 0F1h, 57h, 0E8h, 67h, 9Eh, 83h, 0C4h, 2
                db 8Bh, 0F0h, 8Ah, 44h, 7Ch, 88h, 46h, 0FEh, 0F6h, 46h
                db 0FEh, 4, 75h, 3, 0E9h, 5Fh, 0FFh, 0B8h, 12h, 0, 50h
                db 0B8h, 2, 0, 50h, 0E8h, 14h, 9Ch, 83h, 0C4h, 4, 0FFh
                db 36h, 8, 3Eh, 0E8h, 2Eh, 9Ch, 83h, 0C4h, 2, 0B8h, 13h
                db 0, 50h, 0B8h, 4, 0, 50h, 0E8h, 0FCh, 9Bh, 83h, 0C4h
                db 4, 0FFh, 36h, 0Ah, 3Eh, 0E8h, 16h, 9Ch, 83h, 0C4h, 2
                db 0F6h, 46h, 0FEh, 1, 75h, 3, 0E9h, 20h, 0FFh, 0B8h, 13h
                db 3Fh, 0E9h, 1Dh, 0FFh, 83h, 7Eh, 0FAh, 0, 74h, 30h, 0B8h
                db 14h, 0, 50h, 0B8h, 5, 0, 50h, 0E8h, 0CFh, 9Bh, 83h
                db 0C4h, 4, 0B8h, 2Fh, 3Fh, 50h, 0E8h, 0E9h, 9Bh, 83h
                db 0C4h, 2, 0B8h, 15h, 0, 50h, 0B8h, 0Eh, 0, 50h, 0E8h
                db 0B7h, 9Bh, 83h, 0C4h, 4, 0B8h, 4Fh, 3Fh, 50h, 0E8h
                db 0D1h, 9Bh, 83h, 0C4h, 2, 8Bh, 46h, 0FAh, 5Eh, 5Fh, 8Bh
                db 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2
                db 56h, 0EBh, 6, 90h, 3Dh, 4Eh, 0, 74h, 11h, 0E8h, 0A2h
                db 9Ch, 50h, 0E8h, 7Eh, 9Bh, 83h, 0C4h, 2, 8Bh, 0F0h, 3Dh
                db 59h, 0, 75h, 0EAh, 89h, 76h, 0FEh, 83h, 0FEh, 59h, 75h
                db 6, 0B8h, 1, 0, 0EBh, 3, 90h, 2Bh, 0C0h, 5Eh, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 0Ah, 57h
                db 56h, 0C7h, 46h, 0FEh, 2 dup(0), 8Dh, 46h, 0FAh, 50h
                db 0B8h, 5Bh, 3Fh, 50h, 0E8h, 56h, 9Ah, 83h, 0C4h, 4, 0A3h
                db 4, 5, 89h, 16h, 6, 5, 0Bh, 0D0h, 74h, 0E7h, 0C6h, 6
                db 0FFh, 50h, 0FDh, 0C6h, 6, 30h, 4, 7, 0E8h, 9Bh, 0A0h
                db 0B8h, 2, 0, 50h, 0E8h, 0F8h, 99h, 83h, 0C4h, 2, 8Ah
                db 46h, 4, 0A2h, 0C4h, 55h, 0E8h, 8Ah, 0FCh, 0Bh, 0C0h
                db 74h, 3, 0E9h, 4Eh, 1, 0E8h, 3Eh, 0FEh, 0Bh, 0C0h, 74h
                db 3, 0E9h, 44h, 1, 2Bh, 0F6h, 8Bh, 46h, 4, 0B1h, 3, 0D3h
                db 0E0h, 89h, 46h, 0F8h, 8Bh, 0F8h, 81h, 0C7h, 0D2h, 3Dh
                db 8Dh, 44h, 11h, 50h, 0B8h, 1, 0, 50h, 0E8h, 0F6h, 9Ah
                db 83h, 0C4h, 4, 0FFh, 35h, 0E8h, 12h, 9Bh, 83h, 0C4h
                db 2, 83h, 0C7h, 2, 46h, 83h, 0FEh, 4, 7Ch, 0E1h, 89h
                db 76h, 0FAh, 0E8h, 3Ch, 0FFh, 89h, 46h, 0FAh, 0Bh, 0C0h
                db 75h, 7, 0FFh, 46h, 0FEh, 0E9h, 0, 1, 90h, 0B8h, 2, 0
                db 50h, 0E8h, 8Fh, 99h, 83h, 0C4h, 2, 0E8h, 0D9h, 9Ah
                db 2Bh, 0F6h, 0BFh, 0FAh, 3Dh, 8Dh, 44h, 12h, 50h, 0B8h
                db 2, 0, 50h, 0E8h, 0B1h, 9Ah, 83h, 0C4h, 4, 0FFh, 35h
                db 0E8h, 0CDh, 9Ah, 83h, 0C4h, 2, 83h, 0C7h, 2, 46h, 83h
                db 0FEh, 3, 7Ch, 0E1h, 89h, 76h, 0FAh, 0B8h, 15h, 0, 50h
                db 0B8h, 2, 0, 50h, 0E8h, 8Fh, 9Ah, 83h, 0C4h, 4, 80h
                db 3Eh, 0C4h, 55h, 0, 75h, 5, 0B8h, 68h, 3Fh, 0EBh, 3
                db 0B8h, 78h, 3Fh, 50h, 0E8h, 9Dh, 9Ah, 83h, 0C4h, 2, 0B8h
                db 15h, 0, 50h, 0B8h, 26h, 0, 50h, 0B8h, 12h, 0, 50h, 0B8h
                db 15h, 0, 50h, 0E8h, 33h, 9Ah, 83h, 0C4h, 8, 2Bh, 0F6h
                db 0BFh, 0, 3Eh, 8Dh, 44h, 12h, 50h, 0B8h, 15h, 0, 50h
                db 0E8h, 50h, 9Ah, 83h, 0C4h, 4, 0FFh, 35h, 0E8h, 6Ch
                db 9Ah, 83h, 0C4h, 2, 83h, 0C7h, 2, 46h, 83h, 0FEh, 4
                db 7Ch, 0E1h, 89h, 76h, 0FAh, 0E8h, 4Ah, 9Bh, 50h, 0E8h
                db 26h, 9Ah, 83h, 0C4h, 2, 8Bh, 0F8h, 83h, 0FFh, 1Bh, 75h
                db 6, 0B8h, 1, 0, 0EBh, 3, 90h, 2Bh, 0C0h, 8Bh, 0F0h, 0Bh
                db 0F6h, 75h, 16h, 8Bh, 0C7h, 3Dh, 41h, 0, 72h, 0Bh, 3Dh
                db 44h, 0, 77h, 6, 0B8h, 1, 0, 0EBh, 3, 90h, 2Bh, 0C0h
                db 8Bh, 0F0h, 0Bh, 0F6h, 74h, 0C7h, 89h, 76h, 0FAh, 83h
                db 0FFh, 1Bh, 74h, 3, 83h, 0EFh, 41h, 83h, 0FFh, 3, 7Dh
                db 7, 57h, 0E8h, 6Eh, 0F8h, 83h, 0C4h, 2, 83h, 0FFh, 3
                db 75h, 5, 0E8h, 65h, 0F9h, 8Bh, 0F8h, 83h, 0FFh, 1Bh
                db 75h, 3, 0FFh, 46h, 0FEh, 83h, 2 dup(0FFh), 74h, 99h
                db 89h, 7Eh, 0FCh, 83h, 7Eh, 0FEh, 0, 74h, 51h, 0B8h, 2
                db 0, 50h, 0E8h, 8Ah, 98h, 83h, 0C4h, 2, 0B8h, 13h, 0
                db 50h, 0B8h, 0Ah, 0, 50h, 0E8h, 0B4h, 99h, 83h, 0C4h
                db 4, 0B8h, 86h, 3Fh, 50h, 0E8h, 0CEh, 99h, 83h, 0C4h
                db 2, 0B8h, 64h, 0, 50h, 0E8h, 74h, 9Bh, 83h, 0C4h, 2
                db 0C6h, 6, 93h, 3, 9, 0C6h, 6, 94h, 3, 0Ah, 80h, 3Eh
                db 0C4h, 55h, 1, 75h, 0Ah, 0C6h, 6, 93h, 2 dup(3), 0C6h
                db 6, 94h, 3, 5, 0E8h, 0F7h, 9Ch, 0C6h, 6, 2Eh, 4, 1, 0EBh
                db 0Eh, 0E8h, 0A5h, 99h, 0E8h, 6, 9Fh, 0E8h, 83h, 9Ah
                db 3Dh, 20h, 0, 75h, 0F8h, 0FFh, 36h, 6, 5, 0FFh, 36h
                db 4, 5, 0E8h, 0FFh, 98h, 83h, 0C4h, 4, 0E8h, 89h, 99h
                db 0E8h, 0FEh, 9Ch, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h

; =============== S U B R O U T I N E =======================================


sub_1D5DE       proc near               ; CODE XREF: seg002:0795↑J
                sub     ax, ax
                push    ax
                call    near ptr byte_1D15A+26Ah
                add     sp, 2
                retn
sub_1D5DE       endp


; =============== S U B R O U T I N E =======================================


sub_1D5E8       proc near               ; CODE XREF: seg002:07A1↑J
                mov     ax, 1
                push    ax
                call    near ptr byte_1D15A+26Ah
                add     sp, 2
                retn
sub_1D5E8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D5F4       proc near               ; CODE XREF: seg002:0819↑J

var_A           = word ptr -0Ah
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                sub     ax, ax
                push    ax
                call    thk_res_670A
                add     sp, 2
                mov     [bp+var_2], 16h
                mov     [bp+var_4], 4
                mov     [bp+var_6], 0

loc_1D614:                              ; CODE XREF: sub_1D5F4+3E↓j
                mov     si, [bp+var_6]
                add     si, 55C6h
                mov     di, 4

loc_1D61E:                              ; CODE XREF: sub_1D5F4+33↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D61E
                add     [bp+var_6], 8
                cmp     [bp+var_6], 0B0h
                jl      short loc_1D614
                or      byte_1DC80, 6
                mov     bx, word_1DC1A
                shl     bx, 1
                mov     ax, [bx+3A2h]
                cwd
                mov     cx, 16h
                idiv    cx
                mov     [bp+var_2], dx
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     ax, [bp+var_2]
                mov     cl, 3
                shl     ax, cl
                mov     [bp+var_A], ax
                mov     di, ax
                add     di, 55C6h

loc_1D667:                              ; CODE XREF: sub_1D5F4+90↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                push    word ptr [di]
                call    thk_res_1726
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1D667
                mov     [bp+var_4], si
                call    thk_res_5426

loc_1D68C:                              ; CODE XREF: sub_1D5F4+9E↓j
                call    thk_res_56C6
                cmp     ax, 20h ; ' '
                jnz     short loc_1D68C
                call    thk_res_35A8
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D5F4       endp

ovl_2CAVES      ends

