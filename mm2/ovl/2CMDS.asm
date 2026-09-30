; ===========================================================================

; Segment type: Pure code
ovl_2CMDS       segment byte public 'CODE' use16
                assume cs:ovl_2CMDS
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cmds_common_helper proc near            ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 2
                push    di
                push    si
                mov     si, [bp+arg_0]
                mov     dx, [bp+arg_2]
                mov     cx, [bp+var_2]
                jmp     short loc_1C163
; ---------------------------------------------------------------------------
                align 2

loc_1C144:                              ; CODE XREF: cmds_common_helper+36↓j
                mov     cx, dx
                add     cx, si
                mov     bx, cx
                mov     di, cx
                mov     al, [di+29h]
                mov     [bx+28h], al
                mov     al, [di+2Fh]
                mov     [bx+2Eh], al
                inc     dx
                mov     bx, dx
                mov     al, [bx+si+34h]
                mov     bx, cx
                mov     [bx+34h], al

loc_1C163:                              ; CODE XREF: cmds_common_helper+11↑j
                cmp     dx, 5
                jnz     short loc_1C144
                mov     [bp+arg_2], dx
                mov     bx, [bp+arg_0]
                mov     byte ptr [bx+2Dh], 0
                mov     byte ptr [bx+33h], 0
                mov     byte ptr [bx+39h], 0
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
cmds_common_helper endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C180       proc near               ; CODE XREF: sub_1C212+42↓p
                                        ; sub_1C2A4+32↓p ...

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                mov     [bp+var_2], 4
                cmp     [bp+arg_0], 13h
                jl      short loc_1C197
                mov     [bp+var_2], 0

loc_1C197:                              ; CODE XREF: sub_1C180+10↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, [bp+var_2]
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, [bx+3378h]
                mov     dx, ax
                mov     di, bx
                mov     ax, ds
                mov     es, ax
                assume es:DGROUP
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                add     dx, cx
                sub     dx, 28h ; '('
                neg     dx
                shr     dx, 1
                push    dx
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+var_2], 0
                jz      short loc_1C1E3
                push    word_20BC8      ; CODE XREF: seg002:08CD↑J
                call    thk_text_puts
                add     sp, 2

loc_1C1E3:                              ; CODE XREF: sub_1C180+57↑j
                mov     bx, [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+3378h] ; CODE XREF: seg002:07F5↑J
                call    thk_text_puts
                add     sp, 2
                cmp     [bp+var_2], 0
                jz      short loc_1C202
                push    word_20BCA
                call    thk_text_puts
                add     sp, 2

loc_1C202:                              ; CODE XREF: sub_1C180+76↑j
                mov     ax, 32h ; '2'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C180       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C212       proc near               ; CODE XREF: char_trade+150↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    word_20A90
                call    thk_text_puts
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], 0
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                cmp     [bx+7E88h], dx
                ja      short loc_1C25C
                jb      short loc_1C250
                cmp     [bx+7E86h], ax
                jnb     short loc_1C25C

loc_1C250:                              ; CODE XREF: sub_1C212+36↑j
                mov     ax, 14h

loc_1C253:                              ; CODE XREF: sub_1C212+71↓j
                push    ax
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C29C
; ---------------------------------------------------------------------------

loc_1C25C:                              ; CODE XREF: sub_1C212+34↑j
                                        ; sub_1C212+3C↑j
                mov     ax, [bp+var_4]
                or      ax, [bp+var_2]
                jz      short loc_1C29C
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                sub     [bx+7E86h], ax
                sbb     [bx+7E88h], dx
                cmp     [bp+arg_2], 17h
                jle     short loc_1C286
                mov     ax, 13h
                jmp     short loc_1C253
; ---------------------------------------------------------------------------
                align 2

loc_1C286:                              ; CODE XREF: sub_1C212+6C↑j
                mov     ax, 82h
                imul    [bp+arg_2]
                mov     bx, ax
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                add     [bx+7E86h], ax
                adc     [bx+7E88h], dx

loc_1C29C:                              ; CODE XREF: sub_1C212+48↑j
                                        ; sub_1C212+50↑j
                mov     ax, 1
                mov     sp, bp
                pop     bp
                retn
sub_1C212       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C2A4       proc near               ; CODE XREF: char_trade+163↓p

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    word_20A90
                call    thk_text_puts
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     [bp+var_2], ax  ; CODE XREF: seg002:026D↑J
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     ax, [bp+var_2]
                cmp     [bx+7E7Ch], ax
                jnb     short loc_1C2DE
                mov     ax, 14h
                push    ax
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C2FC
; ---------------------------------------------------------------------------

loc_1C2DE:                              ; CODE XREF: sub_1C2A4+2C↑j
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     ax, [bp+var_2]
                sub     [bx+7E7Ch], ax
                mov     ax, 82h
                imul    [bp+arg_2]
                mov     bx, ax
                mov     ax, [bp+var_2]

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                add     [bx+7E7Ch], ax

loc_1C2FC:                              ; CODE XREF: sub_1C2A4+38↑j
                mov     ax, 1
                mov     sp, bp
                pop     bp
                retn
sub_1C2A4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C304       proc near               ; CODE XREF: char_trade+175↓p

var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 0Ah         ; CODE XREF: seg002:08F1↑J
                mov     [bp+var_6], 0
                push    word_20A90
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     [bp+var_2], ax
                cmp     ax, 28h ; '('
                jbe     short loc_1C330
                mov     [bp+var_6], 14h

loc_1C330:                              ; CODE XREF: sub_1C304+25↑j
                mov     al, byte ptr [bp+var_2]
                mov     [bp+var_4], al
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     al, [bp+var_4]
                cmp     [bx+7E45h], al
                jnb     short loc_1C34C
                mov     [bp+var_6], 14h

loc_1C34C:                              ; CODE XREF: sub_1C304+41↑j
                mov     ax, 82h
                imul    [bp+arg_2]
                mov     bx, ax
                mov     al, [bx+7E45h]
                mov     [bp+var_A], al
                add     al, [bp+var_4]
                mov     [bp+var_8], al
                cmp     [bp+var_A], al
                ja      short loc_1C36A
                cmp     al, 28h ; '('
                jbe     short loc_1C36F

loc_1C36A:                              ; CODE XREF: sub_1C304+60↑j
                mov     [bp+var_6], 14h

loc_1C36F:                              ; CODE XREF: sub_1C304+64↑j
                                        ; seg002:0471↑J
                cmp     [bp+var_6], 0
                jz      short loc_1C380
                push    [bp+var_6]
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C39E
; ---------------------------------------------------------------------------

loc_1C380:                              ; CODE XREF: sub_1C304+6F↑j
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     al, [bp+var_4]
                sub     [bx+7E45h], al
                mov     ax, 82h
                imul    [bp+arg_2]
                mov     bx, ax
                mov     al, [bp+var_4]
                add     [bx+7E45h], al

loc_1C39E:                              ; CODE XREF: sub_1C304+7A↑j
                mov     ax, 1
                mov     sp, bp
                pop     bp
                retn
sub_1C304       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C3A6       proc near               ; CODE XREF: char_trade+187↓p

var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     ax, 82h
                imul    [bp+arg_0]
                add     ax, 7E20h
                mov     [bp+var_C], ax
                mov     ax, 82h
                imul    [bp+arg_2]
                add     ax, 7E20h
                mov     [bp+var_4], ax
                mov     ax, 33A2h
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C3D5:                              ; CODE XREF: sub_1C3A6+68↓j
                mov     ax, 66h ; 'f'
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     di, ax
                cmp     di, 1Bh
                jnz     short loc_1C3F8
                mov     ax, 1

loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                jmp     short loc_1C3FA
; ---------------------------------------------------------------------------

loc_1C3F8:                              ; CODE XREF: sub_1C3A6+4B↑j
                sub     ax, ax

loc_1C3FA:                              ; CODE XREF: sub_1C3A6:loc_1C3F6↑j
                mov     si, ax
                or      si, si
                jnz     short loc_1C40C
                mov     bx, [bp+var_C]
                cmp     byte ptr [bx+di-7], 1
                sbb     ax, ax
                inc     ax
                mov     si, ax

loc_1C40C:                              ; CODE XREF: sub_1C3A6+58↑j
                or      si, si
                jz      short loc_1C3D5
                mov     [bp+var_A], di
                mov     [bp+var_6], si
                cmp     di, 1Bh
                jz      short loc_1C484
                inc     [bp+var_2]
                sub     cx, cx
                mov     dx, [bp+var_4]

loc_1C423:                              ; CODE XREF: sub_1C3A6+A2↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C442

loc_1C42D:                              ; CODE XREF: sub_1C3A6+A0↓j
                mov     [bp+var_8], cx
                cmp     cx, 6
                jnz     short loc_1C44A
                mov     ax, 2
                push    ax
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C484
; ---------------------------------------------------------------------------
                align 2

loc_1C442:                              ; CODE XREF: sub_1C3A6+85↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C42D
                jmp     short loc_1C423
; ---------------------------------------------------------------------------

loc_1C44A:                              ; CODE XREF: sub_1C3A6+8D↑j
                sub     [bp+var_A], 41h ; 'A'
                mov     si, [bp+var_A]
                add     si, [bp+var_C]
                mov     di, [bp+var_8]
                add     di, [bp+var_4]
                mov     al, [si+3Ah]
                mov     [di+3Ah], al
                mov     al, [si+40h]    ; CODE XREF: seg002:086D↑J
                mov     [di+40h], al
                mov     al, [si+46h]
                mov     [di+46h], al
                mov     byte ptr [si+3Ah], 0
                mov     byte ptr [si+40h], 0
                mov     byte ptr [si+46h], 0
                push    [bp+var_A]
                push    [bp+var_C]
                call    thk_res_3766
                add     sp, 4

loc_1C484:                              ; CODE XREF: sub_1C3A6+73↑j
                                        ; sub_1C3A6+99↑j
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
sub_1C3A6       endp


; =============== S U B R O U T I N E =======================================

; "Trade: With (1-"
; Attributes: bp-based frame

char_trade      proc near               ; CODE XREF: seg002:0B31↑J

var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

; FUNCTION CHUNK AT C67A SIZE 00000026 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     ax, [bp+arg_0]
                shl     ax, 1
                add     ax, 416h
                mov     [bp+var_C], ax
                mov     [bp+var_E], ax

loc_1C4A9:                              ; CODE XREF: char_trade+19B↓j
                mov     ax, 14h
                push    ax
                mov     ax, 8
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTrade ; " Trade:  "
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_20A88
                call    thk_text_puts
                add     sp, 2
                push    word_20A8C
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_20A8A
                call    thk_text_puts
                add     sp, 2
                push    word_20A8E
                call    thk_text_puts
                add     sp, 2
                mov     ax, [bp+var_E]
                mov     [bp+var_A], ax

loc_1C4FD:                              ; CODE XREF: char_trade+95↓j
                mov     si, 1
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     di, ax
                cmp     ax, 31h ; '1'
                jnz     short loc_1C521
                mov     bx, [bp+var_A]
                cmp     word ptr [bx], 18h
                jl      short loc_1C521
                sub     si, si

loc_1C521:                              ; CODE XREF: char_trade+87↑j
                                        ; char_trade+8F↑j
                or      si, si
                jz      short loc_1C4FD
                sub     si, si
                cmp     di, 1Bh
                jnz     short loc_1C531

loc_1C52C:                              ; CODE XREF: seg002:0879↑J
                mov     [bp+var_2], 1Bh

loc_1C531:                              ; CODE XREF: char_trade+9C↑j
                cmp     di, 1Bh
                jz      short loc_1C5A9
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aWith1_0 ; "With (1-"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, g_party_size
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 33C2h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     al, byte ptr g_party_size
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_2], ax
                mov     ax, 14h
                push    ax
                mov     ax, 19h
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_clear_text_rect
                add     sp, 8

loc_1C5A9:                              ; CODE XREF: char_trade+A6↑j
                                        ; seg002:0885↑J
                cmp     [bp+var_2], 1Bh
                jz      short loc_1C61D
                mov     bx, [bp+var_2]
                shl     bx, 1
                mov     ax, [bx+3B4h]
                mov     [bp+var_2], ax
                mov     bx, [bp+var_C]
                mov     ax, [bx]

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                mov     [bp+var_6], ax
                mov     ax, 14h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, di
                cmp     ax, 31h ; '1'
                jnz     short loc_1C5E4

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                push    [bp+var_2]
                push    [bp+var_6]
                call    sub_1C212
                jmp     short loc_1C618
; ---------------------------------------------------------------------------
                align 2

loc_1C5E4:                              ; CODE XREF: char_trade+148↑j
                mov     ax, di
                cmp     ax, 32h ; '2'
                jnz     short loc_1C5F6
                push    [bp+var_2]
                push    [bp+var_6]
                call    sub_1C2A4
                jmp     short loc_1C618
; ---------------------------------------------------------------------------

loc_1C5F6:                              ; CODE XREF: char_trade+15B↑j
                mov     ax, di
                cmp     ax, 33h ; '3'
                jnz     short loc_1C608
                push    [bp+var_2]
                push    [bp+var_6]
                call    sub_1C304
                jmp     short loc_1C618
; ---------------------------------------------------------------------------

loc_1C608:                              ; CODE XREF: char_trade+16D↑j
                mov     ax, di
                cmp     ax, 34h ; '4'
                jnz     short loc_1C61D
                push    [bp+var_2]
                push    [bp+var_6]
                call    sub_1C3A6

loc_1C618:                              ; CODE XREF: char_trade+153↑j
                                        ; char_trade+166↑j ...
                add     sp, 4
                mov     si, ax

loc_1C61D:                              ; CODE XREF: char_trade+11F↑j
                                        ; char_trade+17F↑j
                or      si, si
                jz      short loc_1C624
                mov     di, 1Bh

loc_1C624:                              ; CODE XREF: char_trade+191↑j
                cmp     di, 1Bh
                jz      short loc_1C62C
                jmp     loc_1C4A9
; ---------------------------------------------------------------------------

loc_1C62C:                              ; CODE XREF: char_trade+199↑j
                mov     [bp+var_8], di
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C638:                              ; CODE XREF: char_use_item+D7↓p
                push    bp
                mov     bp, sp
                push    si
                mov     si, [bp+arg_4]
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si+40h], 0
                jnz     short loc_1C64E
                mov     ax, 10h
                jmp     short loc_1C696
; ---------------------------------------------------------------------------
                align 2

loc_1C64E:                              ; CODE XREF: char_trade+1B8↑j
                mov     si, [bp+arg_4]
                add     si, bx
                dec     byte ptr [si+40h]
                jnz     short loc_1C660
                mov     byte ptr [si+3Ah], 0FFh
                mov     byte ptr [si+46h], 0

loc_1C660:                              ; CODE XREF: char_trade+1C8↑j
                mov     bx, [bp+arg_2]
                cmp     byte ptr [bx+0Fh], 80h
                jb      short loc_1C67A
                mov     al, [bx+0Fh]
                sub     ah, ah
char_trade      endp


loc_1C66E:                              ; CODE XREF: seg002:0891↑J
                push    ax
                push    word ptr [bp+4]
                call    loc_1CED8
                add     sp, 4
                jmp     short loc_1C69D
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR char_trade

loc_1C67A:                              ; CODE XREF: char_trade+1D9↑j
                mov     ax, [bp+arg_4]
                add     ax, 6
                push    ax
                mov     bx, [bp+arg_2]
                mov     al, [bx+0Fh]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    loc_1CF34
                add     sp, 6
                mov     ax, 11h

loc_1C696:                              ; CODE XREF: char_trade+1BD↑j
                push    ax
                call    sub_1C180
                add     sp, 2

loc_1C69D:                              ; CODE XREF: ovl_2CMDS:C678↑j
                pop     si
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR char_trade

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cmds_helper_a   proc near               ; CODE XREF: char_use_item+E9↓p

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                push    si
                mov     si, [bp+arg_4]
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si+2Eh], 0
                jnz     short loc_1C6B6
                mov     ax, 10h
                jmp     short loc_1C6E8
; ---------------------------------------------------------------------------
                align 2

loc_1C6B6:                              ; CODE XREF: cmds_helper_a+E↑j
                mov     bx, [bp+arg_2]
                cmp     byte ptr [bx+0Fh], 80h
                jb      short loc_1C6D0
                mov     al, [bx+0Fh]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    loc_1CED8
                add     sp, 4
                jmp     short loc_1C6EF
; ---------------------------------------------------------------------------

loc_1C6D0:                              ; CODE XREF: cmds_helper_a+1D↑j
                push    [bp+arg_4]
                mov     bx, [bp+arg_2]
                mov     al, [bx+0Fh]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    loc_1CF34
                add     sp, 6
                mov     ax, 11h

loc_1C6E8:                              ; CODE XREF: cmds_helper_a+13↑j
                push    ax
                call    sub_1C180
                add     sp, 2

loc_1C6EF:                              ; CODE XREF: cmds_helper_a+2E↑j
                pop     si
                pop     bp
                retn
cmds_helper_a   endp


; =============== S U B R O U T I N E =======================================

; "Use Which (A-F)/(1-6)?"
; Attributes: bp-based frame

char_use_item   proc near               ; CODE XREF: seg002:0B19↑J

var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aUseWhichAF16_0 ; "Use Which (A-F)/(1-6)?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C712:                              ; CODE XREF: char_use_item+78↓j
                mov     ax, 66h ; 'f'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1C736
                mov     ax, 1
                jmp     short loc_1C738
; ---------------------------------------------------------------------------
                align 2

loc_1C736:                              ; CODE XREF: char_use_item+3C↑j
                sub     ax, ax

loc_1C738:                              ; CODE XREF: char_use_item+41↑j
                mov     di, ax

loc_1C73A:                              ; CODE XREF: seg002:089D↑J
                mov     ax, si
                cmp     ax, 31h ; '1'
                jb      short loc_1C750
                cmp     ax, 36h ; '6'
                ja      short loc_1C750
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si-9], 1
                jmp     short loc_1C763
; ---------------------------------------------------------------------------
                align 2

loc_1C750:                              ; CODE XREF: char_use_item+4D↑j
                                        ; char_use_item+52↑j
                mov     ax, si
                cmp     ax, 41h ; 'A'
                jb      short loc_1C768
                cmp     ax, 46h ; 'F'
                ja      short loc_1C768
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si-7], 1

loc_1C763:                              ; CODE XREF: char_use_item+5B↑j
                sbb     ax, ax
                inc     ax
                mov     di, ax

loc_1C768:                              ; CODE XREF: char_use_item+63↑j
                                        ; char_use_item+68↑j
                or      di, di
                jz      short loc_1C712
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                cmp     si, 1Bh
                jz      short loc_1C7E1
                cmp     si, 41h ; 'A'
                jb      short loc_1C78A
                cmp     si, 46h ; 'F'
                ja      short loc_1C78A
                mov     bx, [bp+arg_0]
                mov     al, [bx+si-7]
                jmp     short loc_1C793
; ---------------------------------------------------------------------------
                align 2

loc_1C78A:                              ; CODE XREF: char_use_item+88↑j
                                        ; char_use_item+8D↑j
                mov     si, [bp+var_4]
                mov     bx, [bp+arg_0]
                mov     al, [bx+si-9]

loc_1C793:                              ; CODE XREF: char_use_item+95↑j
                mov     [bp+var_6], al
                mov     al, 14h
                mul     [bp+var_6]
                add     ax, 6960h
                mov     [bp+var_8], ax
                mov     bx, ax
                cmp     byte ptr [bx+0Fh], 0
                jnz     short loc_1C7B6
                mov     ax, 0Fh
                push    ax
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C7E1
; ---------------------------------------------------------------------------
                align 2

loc_1C7B6:                              ; CODE XREF: char_use_item+B5↑j
                cmp     [bp+var_4], 41h ; 'A'
                jb      short loc_1C7CE
                mov     ax, [bp+var_4]
                sub     ax, 41h ; 'A'
                push    ax
                push    [bp+var_8]
                push    [bp+arg_0]
                call    loc_1C638
                jmp     short loc_1C7DE
; ---------------------------------------------------------------------------

loc_1C7CE:                              ; CODE XREF: char_use_item+C8↑j
                mov     ax, [bp+var_4]
                sub     ax, 31h ; '1'
                push    ax
                push    [bp+var_8]
                push    [bp+arg_0]
                call    cmds_helper_a

loc_1C7DE:                              ; CODE XREF: char_use_item+DA↑j
                add     sp, 6

loc_1C7E1:                              ; CODE XREF: char_use_item+83↑j
                                        ; char_use_item+C1↑j
                pop     si

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
char_use_item   endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; "Remove Which (1-6)?"
; Attributes: bp-based frame

char_remove_item proc near              ; CODE XREF: seg002:0B0D↑J

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     ax, 14h
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRemoveWhich16 ; "Remove Which (1-6)?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C808:                              ; CODE XREF: char_remove_item+4A↓j
                mov     ax, 36h ; '6'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C827
                mov     bx, [bp+arg_0]
                cmp     [bx+si-9], ah
                jz      short loc_1C82C

loc_1C827:                              ; CODE XREF: char_remove_item+35↑j
                mov     ax, 1
                jmp     short loc_1C82E
; ---------------------------------------------------------------------------

loc_1C82C:                              ; CODE XREF: char_remove_item+3D↑j
                sub     ax, ax

loc_1C82E:                              ; CODE XREF: char_remove_item+42↑j
                mov     di, ax
                or      di, di
                jz      short loc_1C808
                mov     [bp+var_4], di
                mov     [bp+var_6], si
                cmp     si, 1Bh
                jz      short loc_1C8A4
                sub     cx, cx
                mov     dx, [bp+arg_0]

loc_1C844:                              ; CODE XREF: char_remove_item+80↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C862

loc_1C84E:                              ; CODE XREF: char_remove_item+7E↓j
                mov     [bp+var_2], cx
                cmp     cx, 6
                jnz     short loc_1C86A
                mov     ax, 2
                push    ax
                call    sub_1C180
                add     sp, 2
                jmp     short loc_1C8A4
; ---------------------------------------------------------------------------

loc_1C862:                              ; CODE XREF: char_remove_item+64↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C84E
                jmp     short loc_1C844
; ---------------------------------------------------------------------------

loc_1C86A:                              ; CODE XREF: char_remove_item+6C↑j
                sub     [bp+var_6], 31h ; '1'
                push    [bp+var_6]
                push    [bp+arg_0]
                call    loc_1CD54
                add     sp, 4
                mov     si, [bp+var_6]
                add     si, [bp+arg_0]
                mov     di, [bp+var_2]
                add     di, [bp+arg_0]
                mov     al, [si+28h]
                mov     [di+3Ah], al
                mov     al, [si+34h]
                mov     [di+46h], al
                mov     al, [si+2Eh]
                mov     [di+40h], al
                push    [bp+var_6]
                push    [bp+arg_0]
                call    cmds_common_helper
                add     sp, 4

loc_1C8A4:                              ; CODE XREF: char_remove_item+55↑j
                                        ; char_remove_item+78↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
char_remove_item endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

item_effect_dispatch proc near          ; CODE XREF: char_equip_item:loc_1CB40↓p

var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     [bp+var_2], 0
                mov     si, [bp+arg_2]
                mov     bx, [bp+arg_0]
                mov     al, [bx+si+3Ah]
                mov     [bp+var_4], al
                sub     ah, ah
                push    ax
                call    sub_1CFDE
                add     sp, 2
                or      ax, ax
                jz      short loc_1C8E8
                push    [bp+arg_0]
                call    sub_1D234
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jnz     short loc_1C8E2
                jmp     loc_1C9D8
; ---------------------------------------------------------------------------

loc_1C8E2:                              ; CODE XREF: item_effect_dispatch+33↑j
                                        ; item_effect_dispatch+5C↓j
                mov     ax, 6
                jmp     loc_1C9D1
; ---------------------------------------------------------------------------

loc_1C8E8:                              ; CODE XREF: item_effect_dispatch+23↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    sub_1CFF6
                add     sp, 2
                or      ax, ax
                jz      short loc_1C922
                push    [bp+arg_0]
                call    sub_1D234
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jnz     short loc_1C8E2
                push    [bp+arg_0]
                call    sub_1D11C
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jnz     short loc_1C91B
                jmp     loc_1C9D8
; ---------------------------------------------------------------------------

loc_1C91B:                              ; CODE XREF: item_effect_dispatch+6C↑j
                mov     ax, 0Ch
                jmp     loc_1C9D1
; ---------------------------------------------------------------------------
                align 2

loc_1C922:                              ; CODE XREF: item_effect_dispatch+4C↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    loc_1D00E
                add     sp, 2
                or      ax, ax
                jz      short loc_1C94C
                push    [bp+arg_0]
                call    sub_1D162
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jnz     short loc_1C945
                jmp     loc_1C9D8
; ---------------------------------------------------------------------------

loc_1C945:                              ; CODE XREF: item_effect_dispatch+96↑j
                mov     ax, 7
                jmp     loc_1C9D1
; ---------------------------------------------------------------------------
                align 2

loc_1C94C:                              ; CODE XREF: item_effect_dispatch+86↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    sub_1D026
                add     sp, 2
                or      ax, ax
                jz      short loc_1C988
                push    [bp+arg_0]
                call    sub_1D11C
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C972
                mov     ax, 8
                jmp     short loc_1C9D1
; ---------------------------------------------------------------------------
                align 2

loc_1C972:                              ; CODE XREF: item_effect_dispatch+C0↑j
                push    [bp+arg_0]
                call    loc_1D0D6
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C9D8
                mov     ax, 0Dh
                jmp     short loc_1C9D1
; ---------------------------------------------------------------------------
                align 2

loc_1C988:                              ; CODE XREF: item_effect_dispatch+B0↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    loc_1D03E
                add     sp, 2
                or      ax, ax
                jz      short loc_1C9AE
                push    [bp+arg_0]      ; CODE XREF: seg002:07DD↑J
                call    sub_1D1A8
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C9D8
                mov     ax, 9
                jmp     short loc_1C9D1
; ---------------------------------------------------------------------------
                align 2

loc_1C9AE:                              ; CODE XREF: item_effect_dispatch+EC↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    loc_1D056
                add     sp, 2
                or      ax, ax
                jz      short loc_1C9D8
                push    [bp+arg_0]
                call    sub_1D1EE
                add     sp, 2
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C9D8
                mov     ax, 0Ah

loc_1C9D1:                              ; CODE XREF: item_effect_dispatch+3B↑j
                                        ; item_effect_dispatch+74↑j ...
                push    ax
                call    sub_1C180
                add     sp, 2

loc_1C9D8:                              ; CODE XREF: item_effect_dispatch+35↑j
                                        ; item_effect_dispatch+6E↑j ...
                cmp     [bp+var_2], 1
                sbb     ax, ax
                neg     ax
                pop     si
                mov     sp, bp
                pop     bp
                retn
item_effect_dispatch endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Equip Which (A-F)?"
; Attributes: bp-based frame

char_equip_item proc near               ; CODE XREF: seg002:0B01↑J

var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     [bp+var_4], 0
                mov     ax, 14h
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aEquipWhichAF ; "Equip Which (A-F)?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CA0B:                              ; CODE XREF: char_equip_item+58↓j
                mov     ax, 66h ; 'f'
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1CA32
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si-7], 0
                jz      short loc_1CA38

loc_1CA32:                              ; CODE XREF: char_equip_item+41↑j
                mov     ax, 1
                jmp     short loc_1CA3A
; ---------------------------------------------------------------------------
                align 2

loc_1CA38:                              ; CODE XREF: char_equip_item+4A↑j
                sub     ax, ax

loc_1CA3A:                              ; CODE XREF: char_equip_item+4F↑j
                mov     di, ax
                or      di, di
                jz      short loc_1CA0B
                mov     [bp+var_6], di
                mov     [bp+var_A], si
                cmp     si, 1Bh
                jnz     short loc_1CA4E
                jmp     loc_1CB83
; ---------------------------------------------------------------------------

loc_1CA4E:                              ; CODE XREF: char_equip_item+63↑j
                sub     [bp+var_A], 41h ; 'A'

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                sub     cx, cx
                mov     dx, [bp+arg_0]

loc_1CA57:                              ; CODE XREF: char_equip_item+92↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+28h], 0
                jnz     short loc_1CA72

loc_1CA61:                              ; CODE XREF: char_equip_item+90↓j
                mov     [bp+var_2], cx
                cmp     cx, 6
                jnz     short loc_1CA7A
                mov     ax, 2
                push    ax

loc_1CA6D:                              ; CODE XREF: char_equip_item+150↓j
                call    sub_1C180
                jmp     short loc_1CADE
; ---------------------------------------------------------------------------

loc_1CA72:                              ; CODE XREF: char_equip_item+79↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1CA61
                jmp     short loc_1CA57
; ---------------------------------------------------------------------------

loc_1CA7A:                              ; CODE XREF: char_equip_item+81↑j
                mov     si, [bp+var_A]
                add     si, [bp+arg_0]
                mov     al, 14h
                mul     byte ptr [si+3Ah]
                add     ax, 6960h

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                mov     [bp+var_C], ax
                mov     al, [si+46h]
                sub     ah, ah
                mov     cl, 6
                shr     ax, cl
                mov     [bp+var_8], al
                cmp     byte ptr [si+46h], 0FFh
                jnz     short loc_1CAE4
                mov     bx, [bp+arg_0]
                or      byte ptr [bx+26h], 1
                mov     ax, 3
                push    ax
                call    sub_1C180
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Ah
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 0Ah
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                mov     al, [bx+26h]
                sub     ah, ah
                push    ax
                call    thk_print_condition

loc_1CADE:                              ; CODE XREF: char_equip_item+8A↑j
                add     sp, 2
                jmp     loc_1CB83
; ---------------------------------------------------------------------------

loc_1CAE4:                              ; CODE XREF: char_equip_item+B5↑j
                mov     bx, [bp+arg_0]
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+3408h]
                sub     ah, ah
                mov     bx, [bp+var_C]
                mov     cl, [bx+0Dh]
                sub     ch, ch
                test    ax, cx
                jz      short loc_1CB03
                mov     [bp+var_4], 4

loc_1CB03:                              ; CODE XREF: char_equip_item+116↑j
                cmp     [bp+var_8], 0
                jz      short loc_1CB1F
                mov     bl, [bp+var_8]
                sub     bh, bh
                mov     si, [bp+arg_0]
                mov     al, [si+6Ah]
                cmp     [bx+3404h], al
                jz      short loc_1CB1F
                mov     [bp+var_4], 5

loc_1CB1F:                              ; CODE XREF: char_equip_item+121↑j
                                        ; char_equip_item+132↑j
                mov     bx, [bp+var_C]
                cmp     byte ptr [bx+0Eh], 0F0h
                jnz     short loc_1CB2D
                mov     [bp+var_4], 0Eh

loc_1CB2D:                              ; CODE XREF: char_equip_item+140↑j
                cmp     [bp+var_4], 0
                jz      short loc_1CB3A
                push    [bp+var_4]
                jmp     loc_1CA6D
; ---------------------------------------------------------------------------
                align 2

loc_1CB3A:                              ; CODE XREF: char_equip_item+14B↑j
                push    [bp+var_A]
                push    [bp+arg_0]

loc_1CB40:                              ; CODE XREF: seg002:0A65↑J
                call    item_effect_dispatch
                add     sp, 4
                or      ax, ax
                jz      short loc_1CB83
                mov     si, [bp+var_A]
                add     si, [bp+arg_0]
                mov     di, [bp+var_2]
                add     di, [bp+arg_0]
                mov     al, [si+3Ah]
                mov     [di+28h], al
                mov     al, [si+40h]
                mov     [di+2Eh], al
                mov     al, [si+46h]
                mov     [di+34h], al
                push    [bp+var_2]
                push    [bp+arg_0]
                call    loc_1CE12
                add     sp, 4
                push    [bp+var_A]
                push    [bp+arg_0]
                call    thk_res_3766
                add     sp, 4
                call    thk_res_4F3A

loc_1CB83:                              ; CODE XREF: char_equip_item+65↑j
                                        ; char_equip_item+FB↑j ...
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
char_equip_item endp


; =============== S U B R O U T I N E =======================================

; "Drop Which (A-F)?"
; Attributes: bp-based frame

char_drop_item  proc near               ; CODE XREF: seg002:0AF5↑J

var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

; FUNCTION CHUNK AT CCCC SIZE 00000008 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     di, di
                mov     ax, 14h
                push    ax
                mov     ax, 0Ch
                push    ax

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDropWhichAF ; "Drop Which (A-F)?"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     si, [bp+var_4]

loc_1CBAF:                              ; CODE XREF: char_drop_item+46↓j
                                        ; char_drop_item+68↓j
                mov     ax, 66h ; 'f'
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1CBD2
                cmp     ax, 46h ; 'F'
                ja      short loc_1CBAF

loc_1CBD2:                              ; CODE XREF: char_drop_item+41↑j
                cmp     si, 1Bh
                jnz     short loc_1CBDC
                mov     ax, 1
                jmp     short loc_1CBDE
; ---------------------------------------------------------------------------

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                                        ; char_drop_item+4B↑j
                sub     ax, ax

loc_1CBDE:                              ; CODE XREF: char_drop_item+50↑j
                mov     di, ax
                or      di, di
                jnz     short loc_1CBF0
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+si-7], 1
                sbb     ax, ax
                inc     ax
                mov     di, ax

loc_1CBF0:                              ; CODE XREF: char_drop_item+58↑j
                or      di, di
                jz      short loc_1CBAF
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                cmp     si, 1Bh
                jz      short loc_1CC0E
                mov     ax, si
                sub     ax, 41h ; 'A'
                push    ax
                push    [bp+arg_0]
                call    thk_res_3766
                add     sp, 4

loc_1CC0E:                              ; CODE XREF: char_drop_item+73↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CC14:                              ; CODE XREF: char_drop_item+118↓p
                                        ; cmds_helper_b+4E↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     ax, [bp+arg_0]
                add     ax, 10h
                mov     [bp+var_2], ax
                mov     al, byte ptr [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                add     [bp+var_2], si
                mov     [bp+var_4], 0
                cmp     al, 5
                ja      short loc_1CC43
                mov     ax, [bp+arg_0]
                add     ax, 6Bh ; 'k'
                mov     [bp+var_4], ax
                add     [bp+var_4], si

loc_1CC43:                              ; CODE XREF: char_drop_item+AB↑j
                mov     bx, [bp+arg_4]
                mov     ax, [bp+var_4]
                mov     [bx], ax
                mov     ax, [bp+var_2]
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CC54:                              ; CODE XREF: ovl_2CMDS:CE07↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     si, [bp+arg_2]
                add     si, [bp+arg_0]
                mov     al, [si+28h]
                mov     [bp+var_C], al
                mov     al, 14h
                mul     [bp+var_C]
                mov     bx, ax
                mov     al, [bx+696Eh]
                mov     byte ptr [bp+var_2], al
                mov     al, [si+34h]
                and     al, 3Fh
                mov     [bp+var_6], al
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr [bp+var_4], al
                and     byte ptr [bp+var_2], 0Fh
                jz      short loc_1CCCC
                mov     al, byte ptr [bp+var_2]
                add     [bp+var_6], al
                lea     ax, [bp+var_A]
                push    ax
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    loc_1CC14
                add     sp, 6
                mov     [bp+var_8], ax
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     si, ax
                push    si
                push    [bp+var_8]
                call    thk_res_35F0
                add     sp, 4           ; CODE XREF: seg002:08FD↑J
char_drop_item  endp

                cmp     word ptr [bp-0Ah], 0
                jz      short loc_1CCCC
                push    si
                push    word ptr [bp-0Ah]
                call    thk_res_35F0
                add     sp, 4
; START OF FUNCTION CHUNK FOR char_drop_item

loc_1CCCC:                              ; CODE XREF: char_drop_item+103↑j
                                        ; ovl_2CMDS:CCC0↑j
                call    thk_res_4F3A
                pop     si
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR char_drop_item

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cmds_helper_b   proc near               ; CODE XREF: ovl_2CMDS:CECD↓p

var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     si, [bp+arg_2]
                add     si, [bp+arg_0]
                mov     al, [si+28h]
                mov     [bp+var_C], al
                mov     al, 14h
                mul     [bp+var_C]
                mov     bx, ax
                mov     al, [bx+696Eh]
                mov     [bp+var_2], al
                mov     al, [si+34h]
                and     al, 3Fh
                mov     [bp+var_6], al
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     [bp+var_4], al
                and     [bp+var_2], 0Fh
                jz      short loc_1CD4C
                mov     al, [bp+var_2]
                add     [bp+var_6], al
                lea     ax, [bp+var_A]
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    loc_1CC14
                add     sp, 6
                mov     [bp+var_8], ax
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     si, ax
                push    si
                push    [bp+var_8]
                call    thk_res_3608
                add     sp, 4
                cmp     [bp+var_A], 0
                jz      short loc_1CD4C
                push    si
                push    [bp+var_A]
                call    thk_res_3608
                add     sp, 4

loc_1CD4C:                              ; CODE XREF: cmds_helper_b+39↑j
                                        ; cmds_helper_b+6C↑j
                call    thk_res_4F3A
                pop     si
                mov     sp, bp
                pop     bp
                retn
cmds_helper_b   endp

; ---------------------------------------------------------------------------

loc_1CD54:                              ; CODE XREF: char_remove_item+8C↑p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     si, [bp+6]
                add     si, [bp+4]
                mov     al, [si+28h]
                mov     [bp-4], al
                mov     al, [si+34h]
                and     al, 3Fh
                mov     [bp-2], al
                mov     al, 14h
                mul     byte ptr [bp-4]
                mov     bx, ax
                mov     al, [bx+6970h]
                mov     [bp-6], al
                mov     al, [bp-4]
                sub     ah, ah
                push    ax
                call    loc_1D06E
                add     sp, 2
                or      ax, ax
                jz      short loc_1CD98
                mov     bx, [bp+4]
                mov     byte ptr [bx+4Ch], 0
                mov     byte ptr [bx+4Dh], 0

loc_1CD98:                              ; CODE XREF: ovl_2CMDS:CD8B↑j
                mov     al, [bp-4]
                sub     ah, ah
                push    ax
                call    loc_1D00E
                add     sp, 2
                or      ax, ax
                jz      short loc_1CDB3
                mov     bx, [bp+4]
                mov     byte ptr [bx+4Eh], 0
                mov     byte ptr [bx+4Fh], 0

loc_1CDB3:                              ; CODE XREF: ovl_2CMDS:CDA6↑j
                mov     al, [bp-4]
                sub     ah, ah
                mov     si, ax
                push    si
                call    sub_1D026
                add     sp, 2
                or      ax, ax
                jnz     short loc_1CDDB
                push    si
                call    loc_1D03E
                add     sp, 2
                or      ax, ax
                jnz     short loc_1CDDB
                push    si
                call    loc_1D056
                add     sp, 2
                or      ax, ax
                jz      short loc_1CE01

loc_1CDDB:                              ; CODE XREF: ovl_2CMDS:CDC3↑j
                                        ; ovl_2CMDS:CDCE↑j
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Fh
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     al, [bp-6]
                sub     ah, ah
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Fh
                push    ax
                call    thk_res_35F0
                add     sp, 4

loc_1CE01:                              ; CODE XREF: ovl_2CMDS:CDD9↑j
                push    word ptr [bp+6]
                push    word ptr [bp+4]
                call    loc_1CC54
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CE12:                              ; CODE XREF: char_equip_item+188↑p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     si, [bp+6]
                add     si, [bp+4]
                mov     al, [si+28h]
                mov     [bp-4], al
                mov     al, [si+34h]
                and     al, 3Fh
                mov     [bp-2], al
                mov     al, 14h
                mul     byte ptr [bp-4] ; CODE XREF: seg002:0831↑J
                mov     bx, ax
                mov     al, [bx+6970h]
                mov     [bp-6], al
                mov     al, [bp-4]
                sub     ah, ah
                push    ax
                call    loc_1D06E
                add     sp, 2
                or      ax, ax
                jz      short loc_1CE5A
                mov     bx, [bp+4]
                mov     al, [bp-6]
                mov     [bx+4Ch], al
                mov     al, [bp-2]
                mov     [bx+4Dh], al

loc_1CE5A:                              ; CODE XREF: ovl_2CMDS:CE49↑j
                mov     al, [bp-4]
                sub     ah, ah
                push    ax
                call    loc_1D00E
                add     sp, 2
                or      ax, ax
                jz      short loc_1CE79
                mov     bx, [bp+4]
                mov     al, [bp-6]
                mov     [bx+4Eh], al
                mov     al, [bp-2]
                mov     [bx+4Fh], al

loc_1CE79:                              ; CODE XREF: ovl_2CMDS:CE68↑j
                mov     al, [bp-4]
                sub     ah, ah
                mov     si, ax
                push    si
                call    sub_1D026
                add     sp, 2
                or      ax, ax
                jnz     short loc_1CEA1
                push    si
                call    loc_1D03E
                add     sp, 2
                or      ax, ax
                jnz     short loc_1CEA1
                push    si
                call    loc_1D056
                add     sp, 2
                or      ax, ax
                jz      short loc_1CEC7

loc_1CEA1:                              ; CODE XREF: ovl_2CMDS:CE89↑j
                                        ; ovl_2CMDS:CE94↑j
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Fh
                push    ax
                call    thk_res_3608
                add     sp, 4
                mov     al, [bp-6]
                sub     ah, ah
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Fh
                push    ax
                call    thk_res_3608
                add     sp, 4

loc_1CEC7:                              ; CODE XREF: ovl_2CMDS:CE9F↑j
                                        ; seg002:07AD↑J
                push    word ptr [bp+6]
                push    word ptr [bp+4]
                call    cmds_helper_b
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CED8:                              ; CODE XREF: ovl_2CMDS:C672↑p
                                        ; cmds_helper_a+28↑p
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     al, [bp+6]
                and     al, 7Fh
                dec     al
                mov     [bp+6], al
                sub     ah, ah
                mov     si, ax
                shl     ax, 1
                add     ax, 7D60h
                mov     [bp-2], ax
                push    ax
                call    thk_res_545A
                add     sp, 2
                or      ax, ax
                jz      short loc_1CF2E
                push    word ptr [bp-2]
                call    thk_res_54AE
                add     sp, 2
                or      ax, ax
                jz      short loc_1CF2E
                mov     ax, [bp+4]
                mov     word_23626, ax
                mov     byte_2419E, 1
                mov     byte_1DBE6, 1
                push    si
                call    thk_2CAST1_D0C2
                add     sp, 2
                mov     byte_2419E, 0
                mov     byte_1DBE6, 0   ; CODE XREF: seg002:050D↑J

loc_1CF2E:                              ; CODE XREF: ovl_2CMDS:CEFE↑j
                                        ; ovl_2CMDS:CF0B↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CF34:                              ; CODE XREF: char_trade+1FF↑p
                                        ; cmds_helper_a+3F↑p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     si, [bp+8]
                add     si, [bp+4]
                mov     al, [si+34h]
                mov     [bp-2], al
                cmp     word ptr [bp+8], 6
                jl      short loc_1CF53
                mov     al, [si+40h]
                mov     [bp-2], al

loc_1CF53:                              ; CODE XREF: ovl_2CMDS:CF4B↑j
                mov     al, [bp+6]
                and     al, 0Fh
                add     [bp-2], al
                mov     cl, 4           ; CODE XREF: seg002:0AE9↑J
                shr     byte ptr [bp+6], cl
                and     byte ptr [bp+6], 7
                mov     al, [bp+6]
                sub     ah, ah
                cmp     ax, 7           ; switch 8 cases
                ja      short def_1CF71 ; jumptable 0001CF71 default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1CF71[bx] ; switch jump
; ---------------------------------------------------------------------------

loc_1CF76:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:jpt_1CF71↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 0
                add     ax, 75h ; 'u'

loc_1CF7C:                              ; CODE XREF: ovl_2CMDS:CF88↓j
                                        ; ovl_2CMDS:CF90↓j ...
                mov     [bp-4], ax
                jmp     short def_1CF71 ; jumptable 0001CF71 default case
; ---------------------------------------------------------------------------
                align 2

loc_1CF82:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; seg002:062D↑J
                                        ; DATA XREF: ...
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 1
                add     ax, 6Bh ; 'k'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CF8A:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFBE↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 2
                add     ax, 6Eh ; 'n'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CF92:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFC0↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 3
                add     ax, 6Fh ; 'o'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CF9A:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFC2↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 4
                add     ax, 6Ah ; 'j'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CFA2:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFC4↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 5
                add     ax, 71h ; 'q'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CFAA:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFC6↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 6
                add     ax, 72h ; 'r'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------

loc_1CFB2:                              ; CODE XREF: ovl_2CMDS:CF71↑j
                                        ; DATA XREF: ovl_2CMDS:CFC8↓o
                mov     ax, [bp+4]      ; jumptable 0001CF71 case 7
                add     ax, 58h ; 'X'
                jmp     short loc_1CF7C
; ---------------------------------------------------------------------------
jpt_1CF71       dw offset loc_1CF76     ; DATA XREF: ovl_2CMDS:CF71↑r
                                        ; jump table for switch statement
                dw offset loc_1CF82     ; jumptable 0001CF71 case 1
                dw offset loc_1CF8A     ; jumptable 0001CF71 case 2
                dw offset loc_1CF92     ; jumptable 0001CF71 case 3
                dw offset loc_1CF9A     ; jumptable 0001CF71 case 4
                dw offset loc_1CFA2     ; jumptable 0001CF71 case 5
                dw offset loc_1CFAA     ; jumptable 0001CF71 case 6
                dw offset loc_1CFB2     ; jumptable 0001CF71 case 7
; ---------------------------------------------------------------------------

def_1CF71:                              ; CODE XREF: ovl_2CMDS:CF6C↑j
                                        ; ovl_2CMDS:CF7F↑j
                mov     al, [bp-2]      ; jumptable 0001CF71 default case
                sub     ah, ah
                push    ax
                push    word ptr [bp-4]
                call    thk_res_3608
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CFDE       proc near               ; CODE XREF: item_effect_dispatch+1B↑p
                                        ; ovl_2CMDS:D084↓p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                cmp     [bp+arg_0], 1
                jb      short loc_1CFF2
                cmp     [bp+arg_0], 41h ; 'A'
                ja      short loc_1CFF2
                mov     ax, 1
                jmp     short loc_1CFF4
; ---------------------------------------------------------------------------

loc_1CFF2:                              ; CODE XREF: sub_1CFDE+7↑j
                                        ; sub_1CFDE+D↑j
                sub     ax, ax

loc_1CFF4:                              ; CODE XREF: sub_1CFDE+12↑j
                pop     bp
                retn
sub_1CFDE       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CFF6       proc near               ; CODE XREF: item_effect_dispatch+44↑p
                                        ; ovl_2CMDS:D07B↓p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                cmp     [bp+arg_0], 42h ; 'B'
                jb      short loc_1D00A
                cmp     [bp+arg_0], 5Bh ; '['
                ja      short loc_1D00A
                mov     ax, 1
                jmp     short loc_1D00C
; ---------------------------------------------------------------------------

loc_1D00A:                              ; CODE XREF: sub_1CFF6+7↑j
                                        ; sub_1CFF6+D↑j
                sub     ax, ax

loc_1D00C:                              ; CODE XREF: sub_1CFF6+12↑j
                pop     bp
                retn
sub_1CFF6       endp

; ---------------------------------------------------------------------------

loc_1D00E:                              ; CODE XREF: item_effect_dispatch+7E↑p
                                        ; ovl_2CMDS:CD9E↑p ...
                push    bp
                mov     bp, sp
                cmp     byte ptr [bp+4], 5Ch ; '\'
                jb      short loc_1D022
                cmp     byte ptr [bp+4], 72h ; 'r'
                ja      short loc_1D022
                mov     ax, 1
                jmp     short loc_1D024
; ---------------------------------------------------------------------------

loc_1D022:                              ; CODE XREF: ovl_2CMDS:D015↑j
                                        ; ovl_2CMDS:D01B↑j
                sub     ax, ax

loc_1D024:                              ; CODE XREF: ovl_2CMDS:D020↑j
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D026       proc near               ; CODE XREF: item_effect_dispatch+A8↑p
                                        ; ovl_2CMDS:CDBB↑p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                cmp     [bp+arg_0], 73h ; 's'
                jb      short loc_1D03A
                cmp     [bp+arg_0], 7Eh ; '~'
                ja      short loc_1D03A
                mov     ax, 1
                jmp     short loc_1D03C
; ---------------------------------------------------------------------------

loc_1D03A:                              ; CODE XREF: sub_1D026+7↑j
                                        ; sub_1D026+D↑j
                sub     ax, ax

loc_1D03C:                              ; CODE XREF: sub_1D026+12↑j
                pop     bp
                retn
sub_1D026       endp

; ---------------------------------------------------------------------------

loc_1D03E:                              ; CODE XREF: item_effect_dispatch+E4↑p
                                        ; ovl_2CMDS:CDC6↑p ...
                push    bp
                mov     bp, sp
                cmp     byte ptr [bp+4], 7Fh
                jb      short loc_1D052
                cmp     byte ptr [bp+4], 9Ah
                ja      short loc_1D052
                mov     ax, 1
                jmp     short loc_1D054
; ---------------------------------------------------------------------------

loc_1D052:                              ; CODE XREF: ovl_2CMDS:D045↑j
                                        ; ovl_2CMDS:D04B↑j
                sub     ax, ax

loc_1D054:                              ; CODE XREF: ovl_2CMDS:D050↑j
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1D056:                              ; CODE XREF: item_effect_dispatch+10A↑p
                                        ; ovl_2CMDS:CDD1↑p ...
                push    bp
                mov     bp, sp
                cmp     byte ptr [bp+4], 9Bh
                jb      short loc_1D06A
                cmp     byte ptr [bp+4], 9Fh
                ja      short loc_1D06A
                mov     ax, 1
                jmp     short loc_1D06C
; ---------------------------------------------------------------------------

loc_1D06A:                              ; CODE XREF: ovl_2CMDS:D05D↑j
                                        ; ovl_2CMDS:D063↑j
                sub     ax, ax

loc_1D06C:                              ; CODE XREF: ovl_2CMDS:D068↑j
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1D06E:                              ; CODE XREF: ovl_2CMDS:CD83↑p
                                        ; ovl_2CMDS:CE41↑p ...
                push    bp
                mov     bp, sp
                push    di
                push    si
                mov     al, [bp+4]
                sub     ah, ah
                mov     si, ax
                push    si
                call    sub_1CFF6
                add     sp, 2
                push    si
                mov     di, ax
                call    sub_1CFDE
                add     sp, 2
                add     ax, di
                pop     si
                pop     di
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     word ptr [bp-2], 0
                sub     si, si
                mov     di, [bp+4]

loc_1D0A2:                              ; CODE XREF: ovl_2CMDS:loc_1D0C2↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    sub_1CFDE
                add     sp, 2
                or      ax, ax
                jz      short loc_1D0BC

loc_1D0B6:                              ; CODE XREF: ovl_2CMDS:D0C0↓j
                mov     [bp-4], si
                jmp     short loc_1D0C4
; ---------------------------------------------------------------------------
                align 2

loc_1D0BC:                              ; CODE XREF: ovl_2CMDS:D0B4↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D0B6

loc_1D0C2:                              ; CODE XREF: seg002:04DD↑J
                jmp     short loc_1D0A2
; ---------------------------------------------------------------------------

loc_1D0C4:                              ; CODE XREF: ovl_2CMDS:D0B9↑j
                cmp     word ptr [bp-4], 6
                jz      short loc_1D0CD
                inc     word ptr [bp-2]

loc_1D0CD:                              ; CODE XREF: ovl_2CMDS:D0C8↑j
                mov     ax, [bp-2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1D0D6:                              ; CODE XREF: item_effect_dispatch+CB↑p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     word ptr [bp-2], 0
                sub     si, si
                mov     di, [bp+4]

loc_1D0E8:                              ; CODE XREF: ovl_2CMDS:D108↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    sub_1CFF6
                add     sp, 2
                or      ax, ax
                jz      short loc_1D102

loc_1D0FC:                              ; CODE XREF: ovl_2CMDS:D106↓j
                mov     [bp-4], si
                jmp     short loc_1D10A
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D102:                              ; CODE XREF: ovl_2CMDS:D0FA↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D0FC
                jmp     short loc_1D0E8
; ---------------------------------------------------------------------------

loc_1D10A:                              ; CODE XREF: ovl_2CMDS:D0FF↑j
                cmp     word ptr [bp-4], 6
                jz      short loc_1D113
                inc     word ptr [bp-2]

loc_1D113:                              ; CODE XREF: ovl_2CMDS:D10E↑j
                mov     ax, [bp-2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D11C       proc near               ; CODE XREF: item_effect_dispatch+61↑p
                                        ; item_effect_dispatch+B5↑p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D12E:                              ; CODE XREF: sub_1D11C+32↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    sub_1D026
                add     sp, 2
                or      ax, ax
                jz      short loc_1D148

loc_1D142:                              ; CODE XREF: sub_1D11C+30↓j
                mov     [bp+var_4], si
                jmp     short loc_1D150
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D148:                              ; CODE XREF: sub_1D11C+24↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D142
                jmp     short loc_1D12E
; ---------------------------------------------------------------------------

loc_1D150:                              ; CODE XREF: sub_1D11C+29↑j
                cmp     [bp+var_4], 6
                jz      short loc_1D159
                inc     [bp+var_2]

loc_1D159:                              ; CODE XREF: sub_1D11C+38↑j
                                        ; seg002:0825↑J
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D11C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D162       proc near               ; CODE XREF: item_effect_dispatch+8B↑p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D174:                              ; CODE XREF: sub_1D162+32↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    loc_1D00E
                add     sp, 2
                or      ax, ax
                jz      short loc_1D18E

loc_1D188:                              ; CODE XREF: sub_1D162+30↓j
                mov     [bp+var_4], si
                jmp     short loc_1D196
; ---------------------------------------------------------------------------
                align 2

loc_1D18E:                              ; CODE XREF: sub_1D162+24↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D188
                jmp     short loc_1D174
; ---------------------------------------------------------------------------

loc_1D196:                              ; CODE XREF: sub_1D162+29↑j
                cmp     [bp+var_4], 6
                jz      short loc_1D19F
                inc     [bp+var_2]

loc_1D19F:                              ; CODE XREF: sub_1D162+38↑j
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D162       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D1A8       proc near               ; CODE XREF: item_effect_dispatch+F1↑p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D1BA:                              ; CODE XREF: sub_1D1A8+32↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    loc_1D03E
                add     sp, 2
                or      ax, ax
                jz      short loc_1D1D4

loc_1D1CE:                              ; CODE XREF: sub_1D1A8+30↓j
                mov     [bp+var_4], si
                jmp     short loc_1D1DC
; ---------------------------------------------------------------------------
                align 2

loc_1D1D4:                              ; CODE XREF: sub_1D1A8+24↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D1CE
                jmp     short loc_1D1BA
; ---------------------------------------------------------------------------

loc_1D1DC:                              ; CODE XREF: sub_1D1A8+29↑j
                cmp     [bp+var_4], 6
                jz      short loc_1D1E5
                inc     [bp+var_2]

loc_1D1E5:                              ; CODE XREF: sub_1D1A8+38↑j
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D1A8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D1EE       proc near               ; CODE XREF: item_effect_dispatch+117↑p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D200:                              ; CODE XREF: sub_1D1EE+32↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    loc_1D056
                add     sp, 2
                or      ax, ax
                jz      short loc_1D21A

loc_1D214:                              ; CODE XREF: sub_1D1EE+30↓j
                mov     [bp+var_4], si
                jmp     short loc_1D222
; ---------------------------------------------------------------------------
                align 2

loc_1D21A:                              ; CODE XREF: sub_1D1EE+24↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D214
                jmp     short loc_1D200
; ---------------------------------------------------------------------------

loc_1D222:                              ; CODE XREF: sub_1D1EE+29↑j
                cmp     [bp+var_4], 6
                jz      short loc_1D22B
                inc     [bp+var_2]

loc_1D22B:                              ; CODE XREF: sub_1D1EE+38↑j
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D1EE       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D234       proc near               ; CODE XREF: item_effect_dispatch+28↑p
                                        ; item_effect_dispatch+51↑p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D246:                              ; CODE XREF: sub_1D234+32↓j
                mov     bx, si
                add     bx, di
                mov     al, [bx+28h]
                sub     ah, ah
                push    ax
                call    loc_1D06E
                add     sp, 2
                or      ax, ax
                jz      short loc_1D260

loc_1D25A:                              ; CODE XREF: sub_1D234+30↓j
                mov     [bp+var_4], si
                jmp     short loc_1D268
; ---------------------------------------------------------------------------
                align 2

loc_1D260:                              ; CODE XREF: sub_1D234+24↑j
                inc     si
                cmp     si, 6
                jge     short loc_1D25A
                jmp     short loc_1D246
; ---------------------------------------------------------------------------

loc_1D268:                              ; CODE XREF: sub_1D234+29↑j
                cmp     [bp+var_4], 6
                jz      short loc_1D271
                inc     [bp+var_2]

loc_1D271:                              ; CODE XREF: sub_1D234+38↑j
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D234       endp

; ---------------------------------------------------------------------------
                align 8
ovl_2CMDS       ends

