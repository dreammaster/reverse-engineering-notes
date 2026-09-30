; ===========================================================================

; Segment type: Pure code
ovl_2TEMPLE     segment byte public 'CODE' use16
                assume cs:ovl_2TEMPLE
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

temple_common_helper proc near          ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 2
                push    di
                push    si
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, [bp+arg_0]

loc_1C147:                              ; CODE XREF: temple_common_helper+39↓j
                lea     ax, [si+12h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, di
                add     bx, si
                shl     bx, 1
                push    word ptr [bx+591Eh]
                call    thk_text_puts
                add     sp, 2
                inc     si
                cmp     si, 4
                jl      short loc_1C147
                mov     [bp+var_2], si
                call    thk_monster_anim_step
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
temple_common_helper endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C178       proc near               ; CODE XREF: ovl_2TEMPLE:C8A8↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    word_23134
                push    word_23132
                push    [bp+arg_0]
                call    sub_1C326
                add     sp, 6
                or      ax, ax
                jz      short loc_1C1A6
                push    [bp+arg_0]
                call    sub_1C698
                add     sp, 2
                mov     bx, [bp+arg_0]
                mov     byte ptr [bx+26h], 0
                mov     ax, 10h
                jmp     short loc_1C1A9
; ---------------------------------------------------------------------------
                align 2

loc_1C1A6:                              ; CODE XREF: sub_1C178+16↑j
                mov     ax, 0Ch

loc_1C1A9:                              ; CODE XREF: sub_1C178+2B↑j
                push    ax
                call    temple_common_helper
                add     sp, 2
                pop     bp
                retn
sub_1C178       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1B2       proc near               ; CODE XREF: ovl_2TEMPLE:C8BA↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    word_23138
                push    word_23136
                push    [bp+arg_0]
                call    sub_1C326
                add     sp, 6
                or      ax, ax
                jz      short loc_1C1DC
                mov     bx, [bp+arg_0]
                mov     si, bx
                mov     al, [si+0Dh]
                mov     [bx+6Ah], al
                mov     ax, 14h
                jmp     short loc_1C1DF ; CODE XREF: seg002:08CD↑J
; ---------------------------------------------------------------------------
                align 2

loc_1C1DC:                              ; CODE XREF: sub_1C1B2+17↑j
                mov     ax, 0Ch

loc_1C1DF:                              ; CODE XREF: sub_1C1B2+27↑j
                push    ax
                call    temple_common_helper
                add     sp, 2
                pop     si
                pop     bp
                retn
sub_1C1B2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1EA       proc near               ; CODE XREF: seg002:07F5↑J
                                        ; ovl_2TEMPLE:C8CA↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0
                push    word_2313C
                push    word_2313A
                push    [bp+arg_0]
                call    sub_1C326
                add     sp, 6
                or      ax, ax
                jnz     short loc_1C20D
                jmp     loc_1C2A6
; ---------------------------------------------------------------------------

loc_1C20D:                              ; CODE XREF: sub_1C1EA+1E↑j
                mov     bl, g_map_id
                sub     bh, bh
                mov     al, [bx+470Ch]
                or      byte_1DC32, al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 5Ah ; 'Z'
                jle     short loc_1C234
                mov     ax, 1
                jmp     short loc_1C236
; ---------------------------------------------------------------------------
                align 2

loc_1C234:                              ; CODE XREF: sub_1C1EA+42↑j
                sub     ax, ax

loc_1C236:                              ; CODE XREF: sub_1C1EA+47↑j
                mov     [bp+var_2], ax
                cmp     byte_1DC32, 1Fh ; CODE XREF: seg002:08E5↑J
                jnz     short loc_1C24F
                mov     byte_1DC84, 0FEh ; CODE XREF: seg002:0639↑J
                mov     byte_241A0, 0D4h
                mov     byte_1DC32, 0

loc_1C24F:                              ; CODE XREF: sub_1C1EA+54↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C2A0
                sub     ax, ax
                push    ax
                call    temple_common_helper
                add     sp, 2
                mov     byte_1DC25, 0C8h
                mov     byte_1DC26, 3Ch ; '<'
                mov     byte_1DC27, 3Ch ; '<'
                mov     byte_1DC28, 1
                mov     byte_1DC29, 1
                mov     byte_1DC2A, 1
                mov     byte_1DC2B, 0
                mov     al, 1
                mov     byte_1DC2F, al
                mov     byte_1DC2E, al
                mov     byte_1DC2D, al
                mov     byte_1DC2C, al
                mov     byte_1DC30, 0C8h
                mov     byte_1DC31, 0C8h
                inc     word_23130
                jmp     short loc_1C2B0
; ---------------------------------------------------------------------------
                align 2

loc_1C2A0:                              ; CODE XREF: sub_1C1EA+69↑j
                mov     ax, 4
                jmp     short loc_1C2A9
; ---------------------------------------------------------------------------
                align 2

loc_1C2A6:                              ; CODE XREF: sub_1C1EA+20↑j
                mov     ax, 0Ch

loc_1C2A9:                              ; CODE XREF: sub_1C1EA+B9↑j
                push    ax
                call    temple_common_helper
                add     sp, 2

loc_1C2B0:                              ; CODE XREF: sub_1C1EA+B3↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C1EA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C2B4       proc near               ; CODE XREF: ovl_2TEMPLE:C8D8↓p
                                        ; mage_guild_menu+307↓p ...

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                push    si
                mov     bx, [bp+arg_4]
                shl     bx, 1
                shl     bx, 1
                mov     ax, [bx+58E2h]  ; CODE XREF: seg002:026D↑J
                or      ax, [bx+58E4h]
                jnz     short loc_1C2CE
                mov     ax, 18h
                jmp     short loc_1C31C
; ---------------------------------------------------------------------------

loc_1C2CE:                              ; CODE XREF: sub_1C2B4+13↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1C2DC
                mov     ax, 8
                jmp     short loc_1C31C
; ---------------------------------------------------------------------------

loc_1C2DC:                              ; CODE XREF: sub_1C2B4+21↑j
                mov     bx, [bp+arg_4]
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+58E4h]
                push    word ptr [bx+58E2h]
                push    [bp+arg_0]
                call    sub_1C326
                add     sp, 6
                or      ax, ax
                jnz     short loc_1C2FE

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                mov     ax, 0Ch
                jmp     short loc_1C31C
; ---------------------------------------------------------------------------
                align 2

loc_1C2FE:                              ; CODE XREF: sub_1C2B4+42↑j
                mov     bl, byte ptr [bp+arg_2]
                and     bx, 7
                mov     al, [bx+470Ch]

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                mov     si, [bp+arg_2]
                and     si, 0FFh
                mov     cl, 3
                shr     si, cl
                mov     bx, [bp+arg_0]
                or      [bx+si+51h], al
                mov     ax, 4

loc_1C31C:                              ; CODE XREF: sub_1C2B4+18↑j
                                        ; sub_1C2B4+26↑j ...
                push    ax
                call    temple_common_helper
                add     sp, 2
                pop     si
                pop     bp
                retn
sub_1C2B4       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C326       proc near               ; CODE XREF: sub_1C178+E↑p
                                        ; sub_1C1B2+F↑p ...

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_2]
                mov     dx, [bp+arg_4]
                cmp     [bx+68h], dx
                jb      short loc_1C346
                ja      short loc_1C341
                cmp     [bx+66h], ax
                jb      short loc_1C346

loc_1C341:                              ; CODE XREF: sub_1C326+14↑j
                mov     ax, 1
                jmp     short loc_1C348
; ---------------------------------------------------------------------------

loc_1C346:                              ; CODE XREF: sub_1C326+12↑j
                                        ; sub_1C326+19↑j
                sub     ax, ax

loc_1C348:                              ; CODE XREF: sub_1C326+1E↑j
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C399
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_2]
                mov     dx, [bp+arg_4]
                sub     [bx+66h], ax
                sbb     [bx+68h], dx
                mov     ax, 13h
                push    ax
                mov     ax, 10h
                push    ax
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_clear_text_rect ; CODE XREF: seg002:0471↑J
                add     sp, 8
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+arg_0]
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                add     sp, 8

loc_1C399:                              ; CODE XREF: sub_1C326+27↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1C326       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C3A0       proc near               ; CODE XREF: mage_guild_menu+1ED↓p

var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                sub     ax, ax
                mov     [bp+var_6], ax
                mov     [bp+var_8], ax
                mov     al, g_map_id
                sub     ah, ah
                mov     si, ax
                mov     cl, 2
                shl     si, cl
                add     si, [bp+arg_4]
                mov     al, [si+46DAh]
                mov     [bp+var_2], al
                mov     bx, [bp+arg_2]
                mov     [bx], al
                mov     al, [si+46EEh]
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                mov     si, ax
                push    si
                call    thk_res_4C7A
                add     sp, 2
                mov     [bp+var_A], al
                push    si
                call    thk_res_4CA8
                add     sp, 2
                mov     [bp+var_2], al
                mov     ax, [bp+arg_4]
                add     ax, 12h
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:047D↑J
                add     sp, 4
                mov     al, [bp+var_A]
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, [bp+var_2]
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+0Fh], 4
                jz      short loc_1C431
                cmp     byte ptr [bx+0Fh], 2
                jnz     short loc_1C497

loc_1C431:                              ; CODE XREF: sub_1C3A0+89↑j
                mov     al, [bp+var_A]
                cmp     [bx+23h], al
                jb      short loc_1C497
                mov     bx, [bp+arg_2]
                mov     al, [bx]
                sub     ah, ah
                push    ax
                push    [bp+arg_0]
                call    thk_res_362C
                add     sp, 4
                or      ax, ax
                jnz     short loc_1C497
                mov     al, [bp+var_4]
                sub     ah, ah
                sub     dx, dx
                and     ax, 1Fh
                mov     [bp+var_8], ax
                mov     [bp+var_6], dx
                test    [bp+var_4], 20h

loc_1C462:                              ; CODE XREF: seg002:086D↑J
                jz      short loc_1C471
                mov     ax, 0Ah
                cwd
                push    dx
                push    ax
                lea     ax, [bp+var_8]
                push    ax
                call    thk__aFulmul_assign

loc_1C471:                              ; CODE XREF: sub_1C3A0:loc_1C462↑j
                test    [bp+var_4], 40h
                jz      short loc_1C484
                mov     ax, 64h ; 'd'
                cwd
                push    dx
                push    ax
                lea     ax, [bp+var_8]
                push    ax
                call    thk__aFulmul_assign

loc_1C484:                              ; CODE XREF: sub_1C3A0+D5↑j
                test    [bp+var_4], 80h
                jz      short loc_1C497
                mov     ax, 3E8h
                cwd

loc_1C48E:                              ; CODE XREF: seg002:0B31↑J
                push    dx
                push    ax
                lea     ax, [bp+var_8]
                push    ax
                call    thk__aFulmul_assign

loc_1C497:                              ; CODE XREF: sub_1C3A0+8F↑j
                                        ; sub_1C3A0+97↑j ...
                mov     ax, [bp+var_8]
                mov     dx, [bp+var_6]
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C4A2:                              ; CODE XREF: ovl_2TEMPLE:C7FD↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                sub     ax, ax
                mov     [bp+var_6], ax
                mov     [bp+var_8], ax
                mov     al, g_map_id
                sub     ah, ah
                mov     si, ax
                mov     cl, 2
                shl     si, cl
                add     si, [bp+arg_4]
                mov     al, [si+46B2h]
                sub     al, 30h ; '0'
                mov     [bp+var_2], al
                mov     bx, [bp+arg_2]
                mov     [bx], al
                mov     al, [si+46C6h]
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                mov     si, ax
                push    si
                call    thk_res_4C7A
                add     sp, 2
                mov     [bp+var_A], al
                push    si
                call    thk_res_4CA8
                add     sp, 2
                mov     [bp+var_2], al
                mov     ax, [bp+arg_4]
                add     ax, 14h
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, [bp+var_A]
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, [bp+var_2]
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+0Fh], 3 ; CODE XREF: seg002:0879↑J
sub_1C3A0       endp

                jz      short loc_1C535
                cmp     byte ptr [bx+0Fh], 1
                jnz     short loc_1C59B

loc_1C535:                              ; CODE XREF: ovl_2TEMPLE:C52D↑j
                mov     al, [bp-0Ah]
                cmp     [bx+23h], al
                jb      short loc_1C59B
                mov     bx, [bp+6]
                mov     al, [bx]
                sub     ah, ah
                push    ax
                push    word ptr [bp+4]
                call    thk_res_362C
                add     sp, 4
                or      ax, ax
                jnz     short loc_1C59B
                mov     al, [bp-4]
                sub     ah, ah
                sub     dx, dx
                and     ax, 1Fh
                mov     [bp-8], ax
                mov     [bp-6], dx
                test    byte ptr [bp-4], 20h
                jz      short loc_1C575
                mov     ax, 0Ah
                cwd
                push    dx
                push    ax
                lea     ax, [bp-8]
                push    ax
                call    thk__aFulmul_assign

loc_1C575:                              ; CODE XREF: ovl_2TEMPLE:C566↑j
                test    byte ptr [bp-4], 40h
                jz      short loc_1C588
                mov     ax, 64h ; 'd'
                cwd
                push    dx
                push    ax
                lea     ax, [bp-8]
                push    ax
                call    thk__aFulmul_assign

loc_1C588:                              ; CODE XREF: ovl_2TEMPLE:C579↑j
                test    byte ptr [bp-4], 80h
                jz      short loc_1C59B
                mov     ax, 3E8h
                cwd
                push    dx
                push    ax
                lea     ax, [bp-8]
                push    ax
                call    thk__aFulmul_assign

loc_1C59B:                              ; CODE XREF: ovl_2TEMPLE:C533↑j
                                        ; ovl_2TEMPLE:C53B↑j ...
                mov     ax, [bp-8]
                mov     dx, [bp-6]
                pop     si
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================


sub_1C5A6       proc near               ; CODE XREF: ovl_2TEMPLE:C7E6↓p
                mov     bl, g_map_id
                sub     bh, bh

loc_1C5AC:                              ; CODE XREF: seg002:0885↑J
                shl     bx, 1
                mov     ax, 64h ; 'd'
                mul     word ptr [bx+46A8h]
                sub     dx, dx
                retn
sub_1C5A6       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5B8       proc near               ; CODE XREF: ovl_2TEMPLE:C7D9↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                sub     ax, ax          ; CODE XREF: seg002:029D↑J
                mov     [bp+var_2], ax
                mov     [bp+var_4], ax
                mov     bx, [bp+arg_0]
                mov     si, bx
                mov     al, [si+6Ah]
                cmp     [bx+0Dh], al
                jz      short loc_1C60A
                mov     [bp+var_4], 64h ; 'd' ; CODE XREF: seg002:01A1↑J
                mov     [bp+var_2], 0
                cmp     byte ptr [bx+71h], 0
                jz      short loc_1C5F4
                mov     al, [bx+71h]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                call    thk__aFulmul_assign

loc_1C5F4:                              ; CODE XREF: sub_1C5B8+2A↑j
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                sub     ax, ax
                push    ax
                push    word ptr [bx+46A8h]
                lea     ax, [bp+var_4]
                push    ax
                call    thk__aFulmul_assign

loc_1C60A:                              ; CODE XREF: sub_1C5B8+1A↑j
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C5B8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C616       proc near               ; CODE XREF: ovl_2TEMPLE:C7C9↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                sub     ax, ax
                mov     [bp+var_2], ax
                mov     [bp+var_4], ax
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+26h], 0FFh
                jnz     short loc_1C636
                mov     [bp+var_4], 3E8h
                jmp     short loc_1C65E
; ---------------------------------------------------------------------------
                align 2

loc_1C636:                              ; CODE XREF: sub_1C616+16↑j
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_1C644
                mov     [bp+var_4], 64h ; 'd'
                jmp     short loc_1C659
; ---------------------------------------------------------------------------
                align 2

loc_1C644:                              ; CODE XREF: sub_1C616+24↑j
                cmp     byte ptr [bx+26h], 0
                jnz     short loc_1C654
                mov     si, bx
                mov     ax, [si+5Eh]
                cmp     [bx+60h], ax
                jz      short loc_1C65E

loc_1C654:                              ; CODE XREF: sub_1C616+32↑j
                mov     [bp+var_4], 0Ah

loc_1C659:                              ; CODE XREF: sub_1C616+2B↑j
                mov     [bp+var_2], 0

loc_1C65E:                              ; CODE XREF: sub_1C616+1D↑j
                                        ; sub_1C616+3C↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+71h], 0
                jz      short loc_1C677
                mov     al, [bx+71h]
                sub     ah, ah
                sub     cx, cx

loc_1C66E:                              ; CODE XREF: seg002:0891↑J
                push    cx
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                call    thk__aFulmul_assign

loc_1C677:                              ; CODE XREF: sub_1C616+4F↑j
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                sub     ax, ax
                push    ax
                push    word ptr [bx+46A8h]
                lea     ax, [bp+var_4]
                push    ax
                call    thk__aFulmul_assign
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C616       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C698       proc near               ; CODE XREF: sub_1C178+1B↑p
                                        ; ovl_2TEMPLE:C9DD↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     bx, [bp+arg_0]
                mov     ax, [bx+60h]
                mov     [bp+var_2], ax
                cmp     [bx+5Eh], ax
                jnb     short loc_1C6B2
                mov     ax, 1
                jmp     short loc_1C6B4
; ---------------------------------------------------------------------------
                align 2

loc_1C6B2:                              ; CODE XREF: sub_1C698+12↑j
                sub     ax, ax

loc_1C6B4:                              ; CODE XREF: sub_1C698+17↑j
                mov     [bp+var_4], ax
                or      ax, ax
                jz      short loc_1C6C4
                mov     ax, [bp+var_2]
                mov     [bx+5Eh], ax
                mov     [bx+74h], ax

loc_1C6C4:                              ; CODE XREF: sub_1C698+21↑j
                mov     ax, [bp+var_4]
                mov     sp, bp
                pop     bp
                retn
sub_1C698       endp

; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: temple_menu+DF↓p
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                push    word ptr [bp+4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-8], ax
                mov     ax, word_23118
                cmp     [bp+4], ax
                jz      short loc_1C754
                mov     bx, ax
                shl     bx, 1
                cmp     word ptr [bx+416h], 17h
                jg      short loc_1C6F8 ; CODE XREF: seg002:0B19↑J
                cmp     ax, 0FFFFh
                jnz     short loc_1C754

loc_1C6F8:                              ; CODE XREF: ovl_2TEMPLE:C6F1↑j
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, offset aTemple ; "  Temple  "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                sub     si, si
                mov     di, 58CAh

loc_1C732:                              ; CODE XREF: ovl_2TEMPLE:C74F↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 2
                push    ax

loc_1C73A:                              ; CODE XREF: seg002:089D↑J
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1C732
                mov     [bp-0Ch], si

loc_1C754:                              ; CODE XREF: ovl_2TEMPLE:C6E6↑j
                                        ; ovl_2TEMPLE:C6F6↑j
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 58D2h

loc_1C763:                              ; CODE XREF: ovl_2TEMPLE:C780↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 6
                jl      short loc_1C763
                mov     [bp-0Ch], si
                mov     ax, [bp+4]
                mov     word_23118, ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+4]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word ptr [bp-8]
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    word ptr [bp-8]
                call    sub_1C326
                add     sp, 6
                push    word ptr [bp-8]
                call    sub_1C616
                add     sp, 2
                mov     word_23132, ax
                mov     word_23134, dx
                push    word ptr [bp-8]
                call    sub_1C5B8
                add     sp, 2
                mov     word_23136, ax

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                mov     word_23138, dx
                call    sub_1C5A6       ; CODE XREF: seg002:0B0D↑J
                mov     word_2313A, ax
                mov     word_2313C, dx
                sub     si, si
                mov     di, 58EEh

loc_1C7F5:                              ; CODE XREF: ovl_2TEMPLE:C80F↓j
                push    si
                lea     ax, [bp+si-4]
                push    ax
                push    word ptr [bp-8]
                call    loc_1C4A2
                add     sp, 6
                mov     [di], ax
                mov     [di+2], dx
                add     di, 4
                inc     si
                cmp     si, 3
                jl      short loc_1C7F5
                mov     [bp-0Ch], si
                sub     si, si
                mov     di, 58E2h
                mov     [bp-10h], di

loc_1C81C:                              ; CODE XREF: ovl_2TEMPLE:C857↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 23h ; '#'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [di]
                or      ax, [di+2]
                jnz     short loc_1C83E
                mov     ax, 471Fh
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1C84C
; ---------------------------------------------------------------------------
                align 2

loc_1C83E:                              ; CODE XREF: ovl_2TEMPLE:C82F↑j
                mov     bx, [bp-10h]
                push    word ptr [bx+2]
                push    word ptr [bx]
                call    thk_res_53D0
                add     sp, 4

loc_1C84C:                              ; CODE XREF: ovl_2TEMPLE:C83B↑j
                add     di, 4
                add     word ptr [bp-10h], 4
                inc     si
                cmp     si, 6
                jl      short loc_1C81C
                mov     [bp-0Ch], si
                mov     al, [bp-2]
                sub     ah, ah
                mov     [bp-10h], ax
                mov     al, [bp-3]
                mov     [bp-0Eh], ax
                mov     al, [bp-4]
                mov     [bp-12h], ax
                mov     di, [bp-8]

loc_1C873:                              ; CODE XREF: ovl_2TEMPLE:C8E9↓j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp-0Ah], ax
                sub     si, si
                cmp     ax, 43h ; 'C'
                jz      short loc_1C8C0
                jle     short loc_1C88C
                jmp     loc_1C91C
; ---------------------------------------------------------------------------

loc_1C88C:                              ; CODE XREF: ovl_2TEMPLE:C887↑j
                cmp     ax, 1Bh
                jz      short loc_1C8E4
                cmp     ax, 41h ; 'A'
                jz      short loc_1C89E
                cmp     ax, 42h ; 'B'
                jz      short loc_1C8B0
                jmp     loc_1C930
; ---------------------------------------------------------------------------

loc_1C89E:                              ; CODE XREF: ovl_2TEMPLE:C894↑j
                mov     ax, word_23132
                or      ax, word_23134
                jz      short loc_1C8E5
                push    di
                call    sub_1C178

loc_1C8AB:                              ; CODE XREF: ovl_2TEMPLE:C8BD↓j
                                        ; ovl_2TEMPLE:C8CD↓j
                add     sp, 2
                jmp     short loc_1C8DE
; ---------------------------------------------------------------------------

loc_1C8B0:                              ; CODE XREF: ovl_2TEMPLE:C899↑j
                mov     ax, word_23136
                or      ax, word_23138
                jz      short loc_1C8E5
                push    di
                call    sub_1C1B2
                jmp     short loc_1C8AB
; ---------------------------------------------------------------------------
                align 2

loc_1C8C0:                              ; CODE XREF: ovl_2TEMPLE:C885↑j
                mov     ax, word_2313A
                or      ax, word_2313C
                jz      short loc_1C8E5
                push    di
                call    sub_1C1EA
                jmp     short loc_1C8AB
; ---------------------------------------------------------------------------
                align 2

loc_1C8D0:                              ; CODE XREF: ovl_2TEMPLE:C91F↓j
                mov     ax, 3
                push    ax
                push    word ptr [bp-12h]

loc_1C8D7:                              ; CODE XREF: ovl_2TEMPLE:C8F3↓j
                                        ; ovl_2TEMPLE:C8FD↓j
                push    di
                call    sub_1C2B4
                add     sp, 6

loc_1C8DE:                              ; CODE XREF: ovl_2TEMPLE:C8AE↑j
                mov     word_23118, 0FFFFh

loc_1C8E4:                              ; CODE XREF: ovl_2TEMPLE:C88F↑j
                inc     si

loc_1C8E5:                              ; CODE XREF: ovl_2TEMPLE:C8A5↑j
                                        ; ovl_2TEMPLE:C8B7↑j ...
                or      si, si
                jnz     short loc_1C94C
                jmp     short loc_1C873
; ---------------------------------------------------------------------------
                align 2

loc_1C8EC:                              ; CODE XREF: ovl_2TEMPLE:C924↓j
                mov     ax, 4
                push    ax
                push    word ptr [bp-0Eh]
                jmp     short loc_1C8D7
; ---------------------------------------------------------------------------
                align 2

loc_1C8F6:                              ; CODE XREF: ovl_2TEMPLE:C929↓j
                mov     ax, 5
                push    ax
                push    word ptr [bp-10h]
                jmp     short loc_1C8D7
; ---------------------------------------------------------------------------
                align 2

loc_1C900:                              ; CODE XREF: ovl_2TEMPLE:C92E↓j
                push    word ptr [bp+4]
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    di
                call    sub_1C326
                add     sp, 6
                jmp     short loc_1C8E5
; ---------------------------------------------------------------------------

loc_1C916:                              ; CODE XREF: ovl_2TEMPLE:C934↓j
                                        ; ovl_2TEMPLE:C93C↓j ...
                sub     ax, ax

loc_1C918:                              ; CODE XREF: ovl_2TEMPLE:C949↓j
                mov     si, ax
                jmp     short loc_1C8E5
; ---------------------------------------------------------------------------

loc_1C91C:                              ; CODE XREF: ovl_2TEMPLE:C889↑j
                cmp     ax, 44h ; 'D'
                jz      short loc_1C8D0
                cmp     ax, 45h ; 'E'
                jz      short loc_1C8EC
                cmp     ax, 46h ; 'F'
                jz      short loc_1C8F6
                cmp     ax, 47h ; 'G'
                jz      short loc_1C900

loc_1C930:                              ; CODE XREF: ovl_2TEMPLE:C89B↑j
                sub     word ptr [bp-0Ah], 31h ; '1'
                js      short loc_1C916
                mov     ax, g_party_size
                cmp     [bp-0Ah], ax
                jge     short loc_1C916
                mov     ax, [bp+4]
                cmp     [bp-0Ah], ax
                jz      short loc_1C916
                mov     ax, 1
                jmp     short loc_1C918
; ---------------------------------------------------------------------------
                align 2

loc_1C94C:                              ; CODE XREF: ovl_2TEMPLE:C8E7↑j
                mov     [bp-6], si
                cmp     word_23118, 0FFFFh
                jnz     short loc_1C95F
                mov     ax, [bp+4]
                mov     [bp-0Ah], ax
                mov     word_23118, ax

loc_1C95F:                              ; CODE XREF: ovl_2TEMPLE:C954↑j
                mov     ax, [bp-0Ah]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: temple_menu+CE↓p
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     word ptr [bp-0Ah], 0
                mov     word ptr [bp-6], 0
                mov     ax, [bp+4]
                mov     word_23118, ax
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-4], ax
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 0Dh         ; CODE XREF: seg002:07DD↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+4]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word ptr [bp-4]
                call    thk_text_puts
                add     sp, 2
                mov     bx, [bp-4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1C9DC
                inc     word ptr [bp-0Ah]
                mov     byte ptr [bx+26h], 0

loc_1C9DC:                              ; CODE XREF: ovl_2TEMPLE:C9D3↑j
                push    bx
                call    sub_1C698
                add     sp, 2
                mov     [bp-0Ch], ax

loc_1C9E6:                              ; CODE XREF: seg002:0B01↑J
                mov     bx, [bp-4]
                mov     si, bx
                mov     al, [si+0Dh]
                cmp     [bx+6Ah], al
                jz      short loc_1C9F9
                inc     word ptr [bp-6]
                mov     [bx+6Ah], al

loc_1C9F9:                              ; CODE XREF: ovl_2TEMPLE:C9F1↑j
                mov     ax, 13h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     word ptr [bp-6], 0
                jnz     short loc_1CA19
                cmp     word ptr [bp-0Ch], 0
                jnz     short loc_1CA19
                cmp     word ptr [bp-0Ah], 0
                jz      short loc_1CA1E

loc_1CA19:                              ; CODE XREF: ovl_2TEMPLE:CA0B↑j
                                        ; ovl_2TEMPLE:CA11↑j
                mov     ax, offset aHasBeenHealed ; "has been healed."
                jmp     short loc_1CA21
; ---------------------------------------------------------------------------

loc_1CA1E:                              ; CODE XREF: ovl_2TEMPLE:CA17↑j
                mov     ax, offset aIsHealthy ; "  is healthy."

loc_1CA21:                              ; CODE XREF: ovl_2TEMPLE:CA1C↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aSelectAnother ; "  Select another"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCharacter ; "character."
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CA4A:                              ; CODE XREF: ovl_2TEMPLE:CA78↓j
                call    thk_monster_anim_step
                mov     si, ax
                cmp     si, 1Bh

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                jnz     short loc_1CA5A
                mov     ax, 1
                jmp     short loc_1CA5C
; ---------------------------------------------------------------------------
                align 2

loc_1CA5A:                              ; CODE XREF: ovl_2TEMPLE:loc_1CA52↑j
                sub     ax, ax

loc_1CA5C:                              ; CODE XREF: ovl_2TEMPLE:CA57↑j
                mov     di, ax
                or      di, di
                jnz     short loc_1CA76
                sub     si, 31h ; '1'
                js      short loc_1CA72
                cmp     si, g_party_size
                jge     short loc_1CA72
                mov     ax, 1
                jmp     short loc_1CA74
; ---------------------------------------------------------------------------

loc_1CA72:                              ; CODE XREF: ovl_2TEMPLE:CA65↑j
                                        ; ovl_2TEMPLE:CA6B↑j
                sub     ax, ax

loc_1CA74:                              ; CODE XREF: ovl_2TEMPLE:CA70↑j
                mov     di, ax

loc_1CA76:                              ; CODE XREF: ovl_2TEMPLE:CA60↑j
                or      di, di
                jz      short loc_1CA4A
                mov     [bp-2], di
                mov     [bp-8], si
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

temple_menu     proc near               ; CODE XREF: seg002:0801↑J

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                call    loc_1CF52
                mov     bx, word_23128
                mov     byte ptr [bx+9], 43h ; 'C'
                mov     bx, word_2312A
                mov     byte ptr [bx+9], 43h ; 'C'
                mov     bx, word_2312C
                mov     byte ptr [bx+9], 43h ; 'C'
                mov     bx, word_23128
                mov     byte ptr [bx], 44h ; 'D'
                mov     bx, word_2312A
                mov     byte ptr [bx], 45h ; 'E'
                mov     bx, word_2312C
                mov     byte ptr [bx], 46h ; 'F'
                mov     word_23118, 0FFFFh
                mov     byte_2294F, 0FDh
                or      byte_1DC80, 6
                mov     word_23130, 0
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CAE3:                              ; CODE XREF: temple_menu+87↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                push    word ptr [bx+di+58FAh]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CAE3
                mov     [bp+var_4], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jz      short loc_1CB7A
                or      byte_1DC80, 1
                mov     [bp+var_2], 0
                cmp     g_party_size, 0
                jle     short loc_1CB41
                mov     si, 416h
                mov     dx, g_party_size
                mov     cx, [bp+var_2]

loc_1CB39:                              ; CODE XREF: temple_menu+DC↓j
                cmp     word ptr [si], 18h
                jge     short loc_1CB5C

loc_1CB3E:                              ; CODE XREF: temple_menu+DA↓j
                                        ; seg002:0A65↑J
                mov     [bp+var_2], cx

loc_1CB41:                              ; CODE XREF: temple_menu+A5↑j
                call    thk_res_34BA
                call    thk_res_5440
                mov     si, [bp+var_2]

loc_1CB4A:                              ; CODE XREF: temple_menu+EA↓j
                mov     bx, si
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1CB66
                push    si
                call    loc_1C968
                jmp     short loc_1CB6A
; ---------------------------------------------------------------------------
                align 2

loc_1CB5C:                              ; CODE XREF: temple_menu+B4↑j
                add     si, 2
                inc     cx
                cmp     cx, dx
                jge     short loc_1CB3E
                jmp     short loc_1CB39
; ---------------------------------------------------------------------------

loc_1CB66:                              ; CODE XREF: temple_menu+CB↑j
                push    si
                call    loc_1C6CC

loc_1CB6A:                              ; CODE XREF: temple_menu+D1↑j
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1CB4A
                mov     [bp+var_2], si
                call    thk_res_35A8

loc_1CB7A:                              ; CODE XREF: temple_menu+94↑j
                call    thk_2PLAY_A580
                cmp     word_23130, 0
                jz      short loc_1CB95
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                add     sp, 2
                mov     g_view_mode, 0
                call    thk_res_47D8

loc_1CB95:                              ; CODE XREF: temple_menu+FA↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
temple_menu     endp


; =============== S U B R O U T I N E =======================================

; " Mage Guild ", "Learn (A-D)"
; Attributes: bp-based frame

mage_guild_menu proc near               ; CODE XREF: seg002:080D↑J

var_18          = word ptr -18h
var_12          = word ptr -12h
var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_3           = byte ptr -3
var_2           = byte ptr -2
var_1           = byte ptr -1

                push    bp
                mov     bp, sp
                sub     sp, 1Ah
                push    di
                push    si
                sub     di, di
                call    loc_1CF52
                mov     word_23118, 0FFFFh
                sub     si, si
                jmp     short loc_1CBB5
; ---------------------------------------------------------------------------
                align 2

loc_1CBB4:                              ; CODE XREF: mage_guild_menu+43↓j
                inc     si

loc_1CBB5:                              ; CODE XREF: mage_guild_menu+15↑j
                cmp     si, g_party_size
                jge     short loc_1CBE1
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_6], ax
                mov     bx, ax
                mov     al, [bx+79h]
                sub     ah, ah
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, [bx+477Ch]
                sub     ch, ch
                test    ax, cx
                jz      short loc_1CBDD

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                inc     di

loc_1CBDD:                              ; CODE XREF: mage_guild_menu+3E↑j
                or      di, di
                jz      short loc_1CBB4

loc_1CBE1:                              ; CODE XREF: mage_guild_menu+1D↑j
                mov     [bp+var_A], di
                mov     [bp+var_10], si
                mov     bx, word_23128
                mov     byte ptr [bx+9], 53h ; 'S'
                mov     bx, word_2312A
                mov     byte ptr [bx+9], 53h ; 'S'
                mov     bx, word_2312C
                mov     byte ptr [bx+9], 53h ; 'S'
                mov     bx, word_23128
                mov     byte ptr [bx], 41h ; 'A'
                mov     bx, word_2312A
                mov     byte ptr [bx], 42h ; 'B'
                mov     bx, word_2312C
                mov     byte ptr [bx], 43h ; 'C'
                mov     byte_2294F, 0FDh
                or      byte_1DC80, 6
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CC2B:                              ; CODE XREF: mage_guild_menu+BB↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                push    word ptr [bx+di+5956h]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CC2B
                mov     [bp+var_10], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jnz     short loc_1CC69
                jmp     loc_1CF48
; ---------------------------------------------------------------------------

loc_1CC69:                              ; CODE XREF: mage_guild_menu+C8↑j
                cmp     word_23118, 0FFFFh
                jnz     short loc_1CC76
                mov     ax, 1
                jmp     short loc_1CC78
; ---------------------------------------------------------------------------
                align 2

loc_1CC76:                              ; CODE XREF: mage_guild_menu+D2↑j
                sub     ax, ax

loc_1CC78:                              ; CODE XREF: mage_guild_menu+D7↑j
                mov     [bp+var_12], ax
                or      byte_1DC80, 1
                mov     [bp+var_8], 0
                cmp     g_party_size, 0
                jle     short loc_1CCA1
                mov     si, 416h
                mov     dx, g_party_size
                mov     cx, [bp+var_8]

loc_1CC96:                              ; CODE XREF: mage_guild_menu:loc_1CDD5↓j
                cmp     word ptr [si], 18h
                jl      short loc_1CC9E
                jmp     loc_1CDCA
; ---------------------------------------------------------------------------

loc_1CC9E:                              ; CODE XREF: mage_guild_menu+FD↑j
                                        ; mage_guild_menu+236↓j
                mov     [bp+var_8], cx

loc_1CCA1:                              ; CODE XREF: mage_guild_menu+EE↑j
                call    thk_res_34BA
                call    thk_res_5440
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:08FD↑J
                add     sp, 4
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, offset aMageGuild ; " Mage Guild "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                sub     si, si
                mov     di, 58CAh

loc_1CCE1:                              ; CODE XREF: mage_guild_menu+162↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CCE1
                mov     [bp+var_10], si
                cmp     [bp+var_A], 0
                jz      short loc_1CD21
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aLearnAD ; "Learn (A-D)"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CD21:                              ; CODE XREF: mage_guild_menu+16B↑j
                                        ; mage_guild_menu+3A6↓j
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_6], ax
                cmp     [bp+var_12], 0
                jnz     short loc_1CD36
                jmp     loc_1CE1D
; ---------------------------------------------------------------------------

loc_1CD36:                              ; CODE XREF: mage_guild_menu+195↑j
                mov     [bp+var_12], 0
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                cmp     [bp+var_A], 0
                jnz     short loc_1CD4E
                jmp     loc_1CDF6
; ---------------------------------------------------------------------------

loc_1CD4E:                              ; CODE XREF: mage_guild_menu+1AD↑j
                mov     [bp+var_10], 3

loc_1CD53:                              ; CODE XREF: mage_guild_menu+1DE↓j
                mov     ax, [bp+var_10]
                add     ax, 0Fh
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+var_10]
                shl     bx, 1
                push    word ptr [bx+58D2h]
                call    thk_text_puts
                add     sp, 2
                inc     [bp+var_10]
                cmp     [bp+var_10], 7
                jl      short loc_1CD53
                sub     si, si
                mov     di, 58E2h

loc_1CD81:                              ; CODE XREF: mage_guild_menu+1FF↓j
                push    si
                lea     ax, [bp+si+var_4]
                push    ax
                push    [bp+var_6]
                call    sub_1C3A0
                add     sp, 6
                mov     [di], ax
                mov     [di+2], dx
                add     di, 4
                inc     si
                cmp     si, 4
                jl      short loc_1CD81
                mov     [bp+var_10], si
                sub     si, si
                mov     di, 58E2h
                mov     [bp+var_18], di

loc_1CDA8:                              ; CODE XREF: mage_guild_menu+255↓j
                lea     ax, [si+12h]
                push    ax
                mov     ax, 23h ; '#'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [di]
                or      ax, [di+2]
                jnz     short loc_1CDD8
                mov     ax, 4777h
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1CDE6
; ---------------------------------------------------------------------------
                align 2

loc_1CDCA:                              ; CODE XREF: mage_guild_menu+FF↑j
                add     si, 2
                inc     cx
                cmp     cx, dx
                jl      short loc_1CDD5
                jmp     loc_1CC9E
; ---------------------------------------------------------------------------

loc_1CDD5:                              ; CODE XREF: mage_guild_menu+234↑j
                jmp     loc_1CC96
; ---------------------------------------------------------------------------

loc_1CDD8:                              ; CODE XREF: mage_guild_menu+21F↑j
                mov     bx, [bp+var_18]
                push    word ptr [bx+2]
                push    word ptr [bx]
                call    thk_res_53D0
                add     sp, 4

loc_1CDE6:                              ; CODE XREF: mage_guild_menu+22B↑j
                add     di, 4
                add     [bp+var_18], 4
                inc     si
                cmp     si, 4
                jl      short loc_1CDA8
                jmp     short loc_1CE1A
; ---------------------------------------------------------------------------
                align 2

loc_1CDF6:                              ; CODE XREF: mage_guild_menu+1AF↑j
                sub     si, si
                mov     di, 5918h

loc_1CDFB:                              ; CODE XREF: mage_guild_menu+27C↓j
                lea     ax, [si+12h]
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CDFB

loc_1CE1A:                              ; CODE XREF: mage_guild_menu+257↑j
                mov     [bp+var_10], si

loc_1CE1D:                              ; CODE XREF: mage_guild_menu+197↑j
                mov     ax, word_23118
                cmp     [bp+var_8], ax
                jz      short loc_1CE66
                mov     ax, [bp+var_8]
                mov     word_23118, ax
                mov     ax, 12h
                push    ax
                mov     ax, 2           ; CODE XREF: seg002:0831↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+var_8]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_6]
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_6]
                call    sub_1C326
                add     sp, 6

loc_1CE66:                              ; CODE XREF: mage_guild_menu+287↑j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_C], ax
                cmp     ax, 41h ; 'A'
                jz      short loc_1CE8E
                cmp     ax, 42h ; 'B'
                jz      short loc_1CEB2
                cmp     ax, 43h ; 'C'
                jz      short loc_1CEC4
                cmp     ax, 44h ; 'D'
                jz      short loc_1CED4
                cmp     ax, 47h ; 'G'
                jz      short loc_1CEF0
                jmp     short loc_1CF08
; ---------------------------------------------------------------------------

loc_1CE8E:                              ; CODE XREF: mage_guild_menu+2DA↑j
                cmp     [bp+var_A], 0
                jnz     short loc_1CE97
                jmp     loc_1CF3C
; ---------------------------------------------------------------------------

loc_1CE97:                              ; CODE XREF: mage_guild_menu+2F6↑j
                sub     ax, ax
                push    ax
                mov     al, [bp+var_4]

loc_1CE9D:                              ; CODE XREF: mage_guild_menu+326↓j
                                        ; mage_guild_menu+335↓j
                sub     ah, ah
                push    ax
                push    [bp+var_6]
                call    sub_1C2B4
                add     sp, 6
                mov     word_23118, 0FFFFh
                jmp     loc_1CF37
; ---------------------------------------------------------------------------

loc_1CEB2:                              ; CODE XREF: mage_guild_menu+2DF↑j
                cmp     [bp+var_A], 0
                jnz     short loc_1CEBB
                jmp     loc_1CF3C
; ---------------------------------------------------------------------------

loc_1CEBB:                              ; CODE XREF: mage_guild_menu+31A↑j
                mov     ax, 1
                push    ax
                mov     al, [bp+var_3]
                jmp     short loc_1CE9D
; ---------------------------------------------------------------------------

loc_1CEC4:                              ; CODE XREF: mage_guild_menu+2E4↑j
                cmp     [bp+var_A], 0

loc_1CEC8:                              ; CODE XREF: seg002:07AD↑J
                jz      short loc_1CF3C
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                jmp     short loc_1CE9D
; ---------------------------------------------------------------------------
                align 2

loc_1CED4:                              ; CODE XREF: mage_guild_menu+2E9↑j
                cmp     [bp+var_A], 0
                jz      short loc_1CF3C
                mov     ax, 3
                push    ax
                mov     al, [bp+var_1]
                sub     ah, ah
                push    ax
                push    [bp+var_6]
                call    sub_1C2B4
                add     sp, 6
                jmp     short loc_1CF37
; ---------------------------------------------------------------------------
                align 2

loc_1CEF0:                              ; CODE XREF: mage_guild_menu+2EE↑j
                push    [bp+var_8]
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_6]
                call    sub_1C326
                add     sp, 6
                jmp     short loc_1CF3C
; ---------------------------------------------------------------------------

loc_1CF08:                              ; CODE XREF: mage_guild_menu+2F0↑j
                mov     ax, [bp+var_C]
                sub     ax, 31h ; '1'
                mov     [bp+var_E], ax
                or      ax, ax
                jl      short loc_1CF3C
                mov     ax, g_party_size
                cmp     [bp+var_E], ax
                jge     short loc_1CF3C
                mov     ax, [bp+var_8]
                cmp     [bp+var_E], ax
                jz      short loc_1CF3C
                mov     bx, [bp+var_E]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                                        ; CODE XREF: seg002:050D↑J
                jge     short loc_1CF3C
                mov     ax, [bp+var_E]
                mov     [bp+var_8], ax

loc_1CF37:                              ; CODE XREF: mage_guild_menu+313↑j
                                        ; mage_guild_menu+351↑j
                mov     [bp+var_12], 1

loc_1CF3C:                              ; CODE XREF: mage_guild_menu+2F8↑j
                                        ; mage_guild_menu+31C↑j ...
                cmp     [bp+var_C], 1Bh
                jz      short loc_1CF45
                jmp     loc_1CD21
; ---------------------------------------------------------------------------

loc_1CF45:                              ; CODE XREF: mage_guild_menu+3A4↑j
                call    thk_res_35A8

loc_1CF48:                              ; CODE XREF: mage_guild_menu+CA↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CF52:                              ; CODE XREF: temple_menu+8↑p
                                        ; mage_guild_menu+A↑p
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     ax, 4           ; CODE XREF: seg002:0AE9↑J
mage_guild_menu endp

                push    ax
                call    thk_res_670A
                add     sp, 2
                mov     word ptr [bp-2], 5
                mov     word ptr [bp-4], 3
                mov     word ptr [bp-6], 0

loc_1CF73:                              ; CODE XREF: ovl_2TEMPLE:CF90↓j
                mov     si, [bp-6]
                add     si, 5956h
                mov     di, 3

loc_1CF7D:                              ; CODE XREF: ovl_2TEMPLE:CF86↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2           ; CODE XREF: seg002:062D↑J
                dec     di
                jnz     short loc_1CF7D
                add     word ptr [bp-6], 6
                cmp     word ptr [bp-6], 1Eh
                jl      short loc_1CF73
                mov     word ptr [bp-2], 5
                mov     word ptr [bp-4], 3
                mov     word ptr [bp-0Ah], 0

loc_1CFA1:                              ; CODE XREF: ovl_2TEMPLE:CFBE↓j
                mov     si, [bp-0Ah]
                add     si, 58FAh
                mov     di, 3

loc_1CFAB:                              ; CODE XREF: ovl_2TEMPLE:CFB4↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CFAB
                add     word ptr [bp-0Ah], 6
                cmp     word ptr [bp-0Ah], 1Eh
                jl      short loc_1CFA1
                mov     word ptr [bp-2], 4
                mov     si, 58CAh
                mov     di, 4

loc_1CFCB:                              ; CODE XREF: ovl_2TEMPLE:CFD4↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CFCB
                mov     word ptr [bp-2], 7
                mov     si, 58D2h
                mov     di, 7

loc_1CFE1:                              ; CODE XREF: ovl_2TEMPLE:CFEA↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CFE1
                mov     word ptr [bp-2], 3
                mov     si, 5918h
                mov     di, 3

loc_1CFF7:                              ; CODE XREF: ovl_2TEMPLE:D000↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CFF7
                mov     word ptr [bp-2], 1Ch
                mov     si, 591Eh
                mov     di, 1Ch

loc_1D00D:                              ; CODE XREF: ovl_2TEMPLE:D016↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D00D
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 4
ovl_2TEMPLE     ends

