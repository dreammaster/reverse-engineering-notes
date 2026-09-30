; ===========================================================================

; Segment type: Pure code
ovl_2SMITH      segment byte public 'CODE' use16
                assume cs:ovl_2SMITH
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

smith_common_helper proc near           ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                mov     bx, word_2308E
                mov     ax, [bp+arg_0]
                mov     dx, [bp+arg_2]
                add     [bx+66h], ax
                adc     [bx+68h], dx
                sub     ax, ax
                push    ax
                push    ax
                call    sub_1C6FC
                add     sp, 4
                pop     bp
                retn
smith_common_helper endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C150       proc near               ; CODE XREF: sub_1C8E0:loc_1C90B↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     si, 5802h
                mov     bx, word_2308E
                mov     cx, 3
                push    si
                mov     di, 57BAh
                lea     si, [bx+3Ah]
                push    ds
                pop     es
                assume es:DGROUP
                repne movsw
                pop     si
                mov     cx, 3
                push    si
                mov     di, 5840h
                lea     si, [bx+40h]
                repne movsw
                pop     si
                mov     cx, 3
                push    si
                mov     di, 580Eh
                lea     si, [bx+46h]
                repne movsw
                pop     si
                mov     di, bx
                mov     cx, [bp+var_2]

loc_1C192:                              ; CODE XREF: sub_1C150+55↓j
                mov     bx, cx
                mov     al, 14h
                mul     byte ptr [bx+di+3Ah]
                add     ax, 6960h
                mov     [si], ax
                add     si, 2
                inc     cx
                cmp     cx, 6
                jl      short loc_1C192
                mov     [bp+var_2], cx
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C150       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1B0       proc near               ; CODE XREF: smith_action_prompt+88↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_8], 0
                mov     bx, word_2308E
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1C1D4
                mov     ax, 8

loc_1C1CA:                              ; CODE XREF: sub_1C1B0+31↓j
                                        ; sub_1C1B0+4D↓j
                push    ax
                call    loc_1CBE8
                add     sp, 2
                jmp     loc_1C59C
; ---------------------------------------------------------------------------

loc_1C1D4:                              ; CODE XREF: sub_1C1B0+15↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+57BAh], 0
                                        ; CODE XREF: seg002:08CD↑J
                jnz     short loc_1C1E4
                mov     ax, 6
                jmp     short loc_1C1CA
; ---------------------------------------------------------------------------
                align 2

loc_1C1E4:                              ; CODE XREF: sub_1C1B0+2C↑j
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+57C2h] ; CODE XREF: seg002:07F5↑J
                push    word ptr [bx+57C0h]
                call    sub_1C6FC
                add     sp, 4
                or      ax, ax
                jnz     short loc_1C200
                mov     ax, 4
                jmp     short loc_1C1CA
; ---------------------------------------------------------------------------
                align 2

loc_1C200:                              ; CODE XREF: sub_1C1B0+48↑j
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 44C6h

loc_1C20F:                              ; CODE XREF: sub_1C1B0+7C↓j
                lea     ax, [si+11h]
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
                cmp     si, 5
                jl      short loc_1C20F
                mov     [bp+var_6], si
                mov     ax, 11h
                push    ax
                mov     ax, 0Dh
                push    ax
                call    thk_text_goto_xy

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                add     sp, 4
                mov     bx, [bp+arg_0]

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                shl     bx, 1
                push    word ptr [bx+5802h]
                call    thk_text_puts
                add     sp, 2
                mov     bx, [bp+arg_0]
                mov     al, [bx+580Eh]
                and     al, 3Fh
                mov     [bp+var_2], al
                or      al, al
                jz      short loc_1C27C
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C27C:                              ; CODE XREF: sub_1C1B0+AC↑j
                mov     bx, [bp+arg_0]
                mov     al, [bx+580Eh]
                sub     ah, ah
                mov     cl, 6
                shr     ax, cl
                mov     [bp+var_2], al
                or      al, al
                jz      short loc_1C2AC
                mov     ax, 14h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, [bp+var_2]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+44CEh]
                call    thk_text_puts
                add     sp, 2

loc_1C2AC:                              ; CODE XREF: sub_1C1B0+DE↑j
                mov     bx, [bp+arg_0]
                mov     al, [bx+5838h]
                mov     [bp+var_2], al
                mov     ax, 12h
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:026D↑J
                add     sp, 4
                cmp     [bp+var_2], 0
                jnz     short loc_1C2D6
                mov     ax, 4512h
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1C318
; ---------------------------------------------------------------------------

loc_1C2D6:                              ; CODE XREF: sub_1C1B0+118↑j
                sub     si, si
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     [bp+var_A], ax
                mov     di, [bp+var_8]

loc_1C2E3:                              ; CODE XREF: sub_1C1B0+160↓j
                mov     al, [si+44BEh]
                sub     ah, ah
                test    [bp+var_A], ax
                jnz     short loc_1C30C
                or      di, di
                jz      short loc_1C2FC
                mov     ax, 2Ch ; ','
                push    ax
                call    thk_text_putc   ; CODE XREF: seg002:0651↑J
                add     sp, 2

loc_1C2FC:                              ; CODE XREF: sub_1C1B0+140↑j
                mov     al, [si+44D6h]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2           ; CODE XREF: seg002:08F1↑J
                mov     di, 1

loc_1C30C:                              ; CODE XREF: sub_1C1B0+13C↑j
                inc     si
                cmp     si, 8
                jl      short loc_1C2E3
                mov     [bp+var_8], di
                mov     [bp+var_6], si

loc_1C318:                              ; CODE XREF: sub_1C1B0+124↑j
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, [bx+5802h]
                mov     al, [bx+0Eh]
                mov     [bp+var_2], al
                mov     ax, 13h
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+var_2], 0F0h
                jnz     short loc_1C348
                push    word_21D2E
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1C3B0
; ---------------------------------------------------------------------------
                align 2

loc_1C348:                              ; CODE XREF: sub_1C1B0+189↑j
                push    word_21D30
                call    thk_text_puts
                add     sp, 2
                mov     bx, [bp+arg_0]
                mov     al, [bx+580Eh]
                and     al, 3Fh
                mov     [bp+var_4], al
                test    [bp+var_2], 0Fh
                jz      short loc_1C3B0
                mov     ax, 13h
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4           ; CODE XREF: seg002:0471↑J
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                mov     bx, si
                mov     cl, 4
                shr     bx, cl
                shl     bx, 1
                push    word ptr [bx+44E2h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cx, si
                and     cx, 0Fh
                add     ax, cx
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C3B0:                              ; CODE XREF: sub_1C1B0+195↑j
                                        ; sub_1C1B0+1B2↑j
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, [bx+5802h]
                mov     al, [bx+0Fh]
                mov     [bp+var_2], al
                mov     ax, 14h
                push    ax
                mov     ax, 0Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+var_2], 0
                jnz     short loc_1C3E0
                push    word_21D2E
                call    thk_text_puts

loc_1C3DA:                              ; CODE XREF: sub_1C1B0+2BB↓j
                add     sp, 2
                jmp     loc_1C4D0
; ---------------------------------------------------------------------------

loc_1C3E0:                              ; CODE XREF: sub_1C1B0+221↑j
                push    word_21D30
                call    thk_text_puts
                add     sp, 2
                cmp     [bp+var_2], 7Fh
                jbe     short loc_1C46E
                mov     ax, 15h
                push    ax
                mov     ax, 14h         ; CODE XREF: seg002:047D↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpellNumber ; "Spell Number "
                push    ax
                call    thk_text_puts
                add     sp, 2
                and     [bp+var_2], 7Fh
                mov     [bp+var_4], 53h ; 'S'
                cmp     [bp+var_2], 31h ; '1'
                jb      short loc_1C41E
                mov     [bp+var_4], 43h ; 'C'
                sub     [bp+var_2], 30h ; '0'

loc_1C41E:                              ; CODE XREF: sub_1C1B0+264↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     [bp+var_2]
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                push    si
                call    thk_res_4C7A
                add     sp, 2
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    si
                call    thk_res_4CA8
                add     sp, 2

loc_1C462:                              ; CODE XREF: seg002:086D↑J
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                jmp     loc_1C3DA
; ---------------------------------------------------------------------------

loc_1C46E:                              ; CODE XREF: sub_1C1B0+23E↑j
                mov     bx, [bp+arg_0]
                mov     al, [bx+580Eh]
                and     al, 3Fh
                mov     [bp+var_4], al
                or      al, al
                jnz     short loc_1C484
                test    [bp+var_2], 0Fh
                jz      short loc_1C4D0

loc_1C484:                              ; CODE XREF: sub_1C1B0+2CC↑j
                mov     ax, 15h
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:0B31↑J
                add     sp, 4
                mov     bl, [bp+var_2]
                and     bx, 70h
                mov     cl, 4
                shr     bx, cl
                shl     bx, 1
                push    word ptr [bx+4502h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                and     ax, 0Fh
                mov     cl, [bp+var_4]
                sub     ch, ch
                add     ax, cx
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C4D0:                              ; CODE XREF: sub_1C1B0+22D↑j
                                        ; sub_1C1B0+2D2↑j
                mov     ax, 15h
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+arg_0]
                mov     al, [bx+5840h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                mov     al, [bx+580Eh]
                and     al, 3Fh
                mov     [bp+var_2], al
                cmp     byte ptr [bx+57BAh], 6Fh ; 'o'
                jnb     short loc_1C550
                mov     ax, offset aDamage1 ; "Damage = 1-"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+arg_0]

loc_1C52C:                              ; CODE XREF: seg002:0879↑J
                shl     bx, 1
                mov     bx, [bx+5802h]
                mov     al, [bx+10h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                cmp     [bp+var_2], 0
                jz      short loc_1C58E
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                jmp     short loc_1C57A
; ---------------------------------------------------------------------------

loc_1C550:                              ; CODE XREF: sub_1C1B0+365↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+57BAh], 0A0h
                jnb     short loc_1C58E
                cmp     byte ptr [bx+57BAh], 72h ; 'r'
                jbe     short loc_1C58E
                mov     ax, offset aArmorBonus ; "Armor bonus = "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, [bx+5802h]
                mov     al, [bx+10h]
                add     [bp+var_2], al

loc_1C57A:                              ; CODE XREF: sub_1C1B0+39E↑j
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C58E:                              ; CODE XREF: sub_1C1B0+392↑j
                                        ; sub_1C1B0+3A8↑j ...
                call    thk_monster_anim_step
                cmp     ax, 1Bh
                jnz     short loc_1C58E
                call    loc_1CC38
                call    loc_1CB9A

loc_1C59C:                              ; CODE XREF: sub_1C1B0+21↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C5A2:                              ; CODE XREF: smith_action_prompt+78↓p
                push    bp
                mov     bp, sp
                mov     bx, word_2308E
                cmp     byte ptr [bx+26h], 0 ; CODE XREF: seg002:0885↑J
sub_1C1B0       endp

                jz      short loc_1C5BC
                mov     ax, 8

loc_1C5B2:                              ; CODE XREF: ovl_2SMITH:C5C9↓j
                push    ax
                call    loc_1CBE8
                add     sp, 2
                jmp     short loc_1C5F4
; ---------------------------------------------------------------------------
                align 2

loc_1C5BC:                              ; CODE XREF: ovl_2SMITH:C5AD↑j
                mov     bx, [bp+4]
                cmp     byte ptr [bx+57BAh], 0
                                        ; CODE XREF: seg002:029D↑J
                jnz     short loc_1C5CC
                mov     ax, 6
                jmp     short loc_1C5B2
; ---------------------------------------------------------------------------
                align 2

loc_1C5CC:                              ; CODE XREF: ovl_2SMITH:C5C4↑j
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+57C2h]
                push    word ptr [bx+57C0h]

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                call    smith_common_helper
                add     sp, 4
                sub     ax, ax
                push    ax
                call    loc_1CBE8
                add     sp, 2
                push    word ptr [bp+4]
                push    word_2308E
                call    thk_char_backpack_remove
                add     sp, 4

loc_1C5F4:                              ; CODE XREF: ovl_2SMITH:C5B9↑j
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5F6       proc near               ; CODE XREF: smith_action_prompt+51↓p

var_8           = word ptr -8
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 5802h
                mov     [bp+var_8], 57C0h

loc_1C612:                              ; CODE XREF: sub_1C5F6+F9↓j
                mov     [bp+var_2], 20h ; ' '
                lea     ax, [si+11h]
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     byte ptr [si+5838h], 0
                jz      short loc_1C648
                mov     bx, word_2308E
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+44BEh]
                sub     ah, ah
                mov     cl, [si+5838h]
                sub     ch, ch
                test    ax, cx
                jz      short loc_1C648
                mov     [bp+var_2], 2Dh ; '-'

loc_1C648:                              ; CODE XREF: sub_1C5F6+33↑j
                                        ; sub_1C5F6+4C↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, si
                add     ax, 41h ; 'A'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax

loc_1C66E:                              ; CODE XREF: seg002:0891↑J
                call    thk_text_putc
                add     sp, 2
                cmp     byte ptr [si+57BAh], 0
                jz      short loc_1C6E2
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                mov     al, [si+580Eh]
                and     al, 3Fh
                mov     [bp+var_2], al
                or      al, al
                jz      short loc_1C6BC
                lea     ax, [si+11h]
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C6BC:                              ; CODE XREF: sub_1C5F6+98↑j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 22h ; '"'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+var_8]
                push    word ptr [bx+2]
                push    word ptr [bx]
                call    thk_res_53D0
                add     sp, 4

loc_1C6E2:                              ; CODE XREF: sub_1C5F6+83↑j
                add     di, 2
                add     [bp+var_8], 4
                inc     si
                cmp     si, 6
                jge     short loc_1C6F2
                jmp     loc_1C612
; ---------------------------------------------------------------------------

loc_1C6F2:                              ; CODE XREF: seg002:0B19↑J
                                        ; sub_1C5F6+F7↑j
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C5F6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C6FC       proc near               ; CODE XREF: smith_common_helper+17↑p
                                        ; sub_1C1B0+40↑p ...

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, word_2308E
                mov     ax, [bp+arg_0]
                mov     dx, [bp+arg_2]
                cmp     [bx+68h], dx
                jb      short loc_1C71E
                ja      short loc_1C718
                cmp     [bx+66h], ax
                jb      short loc_1C71E

loc_1C718:                              ; CODE XREF: sub_1C6FC+15↑j
                mov     ax, 1
                jmp     short loc_1C720
; ---------------------------------------------------------------------------
                align 2

loc_1C71E:                              ; CODE XREF: sub_1C6FC+13↑j
                                        ; sub_1C6FC+1A↑j
                sub     ax, ax

loc_1C720:                              ; CODE XREF: sub_1C6FC+1F↑j
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C76F
                mov     ax, 13h
                push    ax
                mov     ax, 0Eh
                push    ax
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_clear_text_rect

loc_1C73A:                              ; CODE XREF: seg002:089D↑J
                add     sp, 8
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, word_2308E
                mov     ax, [bp+arg_0]
                mov     dx, [bp+arg_2]
                sub     [bx+66h], ax
                sbb     [bx+68h], dx
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                add     sp, 8

loc_1C76F:                              ; CODE XREF: sub_1C6FC+29↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1C6FC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C776       proc near               ; CODE XREF: smith_action_prompt+8F↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, word_2308E
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1C78C
                mov     ax, 8
                jmp     short loc_1C7EF
; ---------------------------------------------------------------------------

loc_1C78C:                              ; CODE XREF: sub_1C776+F↑j
                sub     cx, cx
                mov     dx, bx

loc_1C790:                              ; CODE XREF: sub_1C776+38↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C7A8

loc_1C79A:                              ; CODE XREF: sub_1C776+36↓j
                mov     [bp+var_2], cx
                cmp     cx, 6
                jnz     short loc_1C7B0
                mov     ax, 2
                jmp     short loc_1C7EF
; ---------------------------------------------------------------------------
                align 2

loc_1C7A8:                              ; CODE XREF: sub_1C776+22↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C79A
                jmp     short loc_1C790
; ---------------------------------------------------------------------------

loc_1C7B0:                              ; CODE XREF: sub_1C776+2A↑j
                mov     bx, [bp+arg_0]
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+57C2h]
                push    word ptr [bx+57C0h]
                call    sub_1C6FC
                add     sp, 4
                or      ax, ax
                jnz     short loc_1C7CE
                mov     ax, 4
                jmp     short loc_1C7EF
; ---------------------------------------------------------------------------

loc_1C7CE:                              ; CODE XREF: sub_1C776+51↑j
                mov     si, [bp+var_2]
                add     si, word_2308E
                mov     bx, [bp+arg_0]
                mov     al, [bx+57BAh]
                mov     [si+3Ah], al
                mov     al, [bx+5840h]  ; CODE XREF: seg002:0849↑J
                mov     [si+40h], al
                mov     al, [bx+580Eh]  ; CODE XREF: seg002:0B0D↑J
                mov     [si+46h], al
                sub     ax, ax

loc_1C7EF:                              ; CODE XREF: sub_1C776+14↑j
                                        ; sub_1C776+2F↑j ...
                push    ax
                call    loc_1CBE8
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C776       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C7FC       proc near               ; CODE XREF: sub_1C8E0+17B↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                sub     ax, ax
                mov     [bp+var_6], ax
                mov     [bp+var_8], ax
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+57BAh], 0
                jnz     short loc_1C817
                jmp     loc_1C8D6
; ---------------------------------------------------------------------------

loc_1C817:                              ; CODE XREF: sub_1C7FC+16↑j
                mov     al, [bx+580Eh]
                and     al, 3Fh
                mov     [bp+var_2], al
                cmp     word_2307A, 6
                jnz     short loc_1C854
                or      al, al
                jnz     short loc_1C838
                mov     [bp+var_8], 0Ah
                mov     [bp+var_6], 0
                jmp     loc_1C8D6
; ---------------------------------------------------------------------------

loc_1C838:                              ; CODE XREF: sub_1C7FC+2D↑j
                mov     ax, 64h ; 'd'
                cwd
                push    dx
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                mov     [bp+var_8], ax
                mov     [bp+var_6], dx
                jmp     loc_1C8D6
; ---------------------------------------------------------------------------
                align 2

loc_1C854:                              ; CODE XREF: sub_1C7FC+29↑j
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, [bx+5802h]
                mov     ax, [bx+12h]
                mov     [bp+var_8], ax
                mov     [bp+var_6], 0
                cmp     [bp+var_2], 0
                jz      short loc_1C877
                shl     [bp+var_8], 1
                rcl     [bp+var_6], 1
                dec     [bp+var_2]

loc_1C877:                              ; CODE XREF: sub_1C7FC+70↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C8A5
                mov     al, [bp+var_2]
                neg     al
                cbw
                neg     ax
                mov     [bp+var_A], ax
                mov     ax, 3E8h
                cwd
                push    dx
                push    ax
                mov     ax, [bp+var_A]
                cwd
                push    dx
                push    ax
                call    thk__aFulmul
                add     [bp+var_8], ax
                adc     [bp+var_6], dx
                mov     ax, [bp+var_A]
                neg     ax
                add     [bp+var_2], al

loc_1C8A5:                              ; CODE XREF: sub_1C7FC+7F↑j
                mov     ax, 0Ah
                push    ax
                push    word_2308E
                call    thk_res_3664
                add     sp, 4
                mov     [bp+var_4], ax
                cmp     word_2307A, 5
                jnz     short loc_1C8CA
                shr     [bp+var_6], 1
                rcr     [bp+var_8], 1
                or      ax, ax
                jnz     short loc_1C8D6
                jmp     short loc_1C8D0
; ---------------------------------------------------------------------------
                align 2

loc_1C8CA:                              ; CODE XREF: sub_1C7FC+BF↑j
                cmp     [bp+var_4], 0
                jz      short loc_1C8D6

loc_1C8D0:                              ; CODE XREF: sub_1C7FC+CB↑j
                shr     [bp+var_6], 1
                rcr     [bp+var_8], 1

loc_1C8D6:                              ; CODE XREF: sub_1C7FC+18↑j
                                        ; sub_1C7FC+39↑j ...
                mov     ax, [bp+var_8]
                mov     dx, [bp+var_6]
                mov     sp, bp
                pop     bp
                retn
sub_1C7FC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C8E0       proc near               ; CODE XREF: smith_action_prompt+4E↓p

var_16          = word ptr -16h
var_14          = word ptr -14h
var_12          = word ptr -12h
var_10          = word ptr -10h
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 16h
                push    di
                push    si
                sub     si, si

loc_1C8EA:                              ; CODE XREF: sub_1C8E0+18↓j
                sub     al, al
                mov     [si+580Eh], al
                mov     [si+5840h], al
                inc     si
                cmp     si, 6
                jl      short loc_1C8EA
                mov     [bp+var_A], si
                cmp     word_2307A, 5
                jz      short loc_1C90B
                cmp     word_2307A, 6
                jnz     short loc_1C912

loc_1C90B:                              ; CODE XREF: sub_1C8E0+22↑j
                call    sub_1C150
                jmp     loc_1CA50
; ---------------------------------------------------------------------------
                align 2

loc_1C912:                              ; CODE XREF: sub_1C8E0+29↑j
                mov     ax, word_2307A
                cmp     ax, 1
                jz      short loc_1C934
                cmp     ax, 2
                jnz     short loc_1C922
                jmp     loc_1C9A2
; ---------------------------------------------------------------------------

loc_1C922:                              ; CODE XREF: sub_1C8E0+3D↑j
                cmp     ax, 3
                jnz     short loc_1C92A
                jmp     loc_1C9AE
; ---------------------------------------------------------------------------

loc_1C92A:                              ; CODE XREF: sub_1C8E0+45↑j
                cmp     ax, 4
                jnz     short loc_1C932
                jmp     loc_1C9BA
; ---------------------------------------------------------------------------

loc_1C932:                              ; CODE XREF: sub_1C8E0+4D↑j
                jmp     short loc_1C93E
; ---------------------------------------------------------------------------

loc_1C934:                              ; CODE XREF: sub_1C8E0+38↑j
                mov     [bp+var_4], 43C8h
                mov     [bp+var_8], 43E6h

loc_1C93E:                              ; CODE XREF: sub_1C8E0:loc_1C932↑j
                                        ; sub_1C8E0+CB↓j ...
                mov     al, g_map_id
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                mov     [bp+var_12], ax
                add     [bp+var_4], ax
                add     [bp+var_8], ax
                mov     [bp+var_A], 0
                mov     si, 5802h
                mov     ax, [bp+var_4]
                mov     cx, 3
                push    si
                mov     di, 57BAh
                mov     si, ax
                push    ds
                pop     es
                repne movsw
                pop     si
                mov     ax, [bp+var_8]
                mov     cx, 3
                push    si
                mov     di, 580Eh
                mov     si, ax
                repne movsw
                pop     si
                mov     [bp+var_10], 6
                add     [bp+var_A], 6
                add     [bp+var_8], 6
                mov     di, [bp+var_4]
                mov     cx, [bp+var_10]

loc_1C98F:                              ; CODE XREF: sub_1C8E0+BF↓j
                mov     al, 14h
                mul     byte ptr [di]
                add     ax, 6960h
                mov     [si], ax
                inc     di
                add     si, 2           ; CODE XREF: seg002:07DD↑J
                dec     cx
                jz      short loc_1C9C8
                jmp     short loc_1C98F
; ---------------------------------------------------------------------------
                align 2

loc_1C9A2:                              ; CODE XREF: sub_1C8E0+3F↑j
                mov     ax, 447Ch
                mov     [bp+var_8], ax
                mov     [bp+var_4], ax
                jmp     short loc_1C93E
; ---------------------------------------------------------------------------
                align 2

loc_1C9AE:                              ; CODE XREF: sub_1C8E0+47↑j
                mov     [bp+var_4], 4404h
                mov     [bp+var_8], 4422h
                jmp     short loc_1C93E
; ---------------------------------------------------------------------------

loc_1C9BA:                              ; CODE XREF: sub_1C8E0+4F↑j
                mov     [bp+var_4], 4440h
                mov     [bp+var_8], 445Eh
                jmp     loc_1C93E
; ---------------------------------------------------------------------------
                align 2

loc_1C9C8:                              ; CODE XREF: sub_1C8E0+BD↑j
                mov     [bp+var_4], di
                cmp     word_2307A, 2
                jnz     short loc_1CA20
                mov     bx, g_era
                shl     bx, 1
                mov     ax, [bx+3A2h]
                cwd
                mov     [bp+var_16], ax
                mov     [bp+var_14], dx
                mov     cx, 1Eh

loc_1C9E6:                              ; CODE XREF: seg002:0B01↑J
                idiv    cx
                mov     [bp+var_2], dx
                mov     ax, [bp+var_16]
                mov     dx, [bp+var_14]
                idiv    cx
                mov     [bp+var_C], ax
                cmp     [bp+var_2], 1Dh
                jnz     short loc_1CA04
                mov     bx, ax
                mov     al, [bx+449Ah]
                jmp     short loc_1CA0B
; ---------------------------------------------------------------------------

loc_1CA04:                              ; CODE XREF: sub_1C8E0+11A↑j
                mov     bx, [bp+var_2]
                mov     al, [bx+44A0h]

loc_1CA0B:                              ; CODE XREF: sub_1C8E0+122↑j
                mov     [bp+var_6], al
                sub     si, si

loc_1CA10:                              ; CODE XREF: sub_1C8E0+13B↓j
                mov     al, [bp+var_6]
                mov     [si+580Eh], al
                inc     si
                cmp     si, 6
                jl      short loc_1CA10
                mov     [bp+var_A], si

loc_1CA20:                              ; CODE XREF: sub_1C8E0+F0↑j
                cmp     word_2307A, 4
                jnz     short loc_1CA50
                sub     si, si

loc_1CA29:                              ; CODE XREF: sub_1C8E0+15A↓j
                mov     al, [si+580Eh]
                mov     [si+5840h], al
                mov     byte ptr [si+580Eh], 0
                inc     si
                cmp     si, 6
                jl      short loc_1CA29
                mov     [bp+var_A], si
                cmp     g_map_id, 1
                jnz     short loc_1CA50
                mov     byte_23060, 5
                mov     byte_23062, 2

loc_1CA50:                              ; CODE XREF: sub_1C8E0+2E↑j
                                        ; sub_1C8E0+145↑j ...
                sub     si, si

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                mov     di, 57C0h
                mov     [bp+var_12], 5802h

loc_1CA5A:                              ; CODE XREF: sub_1C8E0+19D↓j
                push    si
                call    sub_1C7FC
                add     sp, 2
                mov     [di], ax
                mov     [di+2], dx
                mov     bx, [bp+var_12]
                mov     bx, [bx]
                mov     al, [bx+0Dh]
                mov     [si+5838h], al
                add     di, 4
                add     [bp+var_12], 2
                inc     si
                cmp     si, 6
                jl      short loc_1CA5A
                mov     [bp+var_A], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C8E0       endp


; =============== S U B R O U T I N E =======================================

; Identify / Sell / Buy (A-F)
; Attributes: bp-based frame

smith_action_prompt proc near           ; CODE XREF: seg002:0801↑J
                                        ; blacksmith_menu+18C↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     di, 1
                mov     ax, [bp+arg_0]
                mov     word_2307A, ax

loc_1CA99:                              ; CODE XREF: smith_action_prompt+D1↓j
                or      di, di
                jz      short loc_1CADC
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+arg_0], 6
                jnz     short loc_1CAB6
                mov     ax, offset aIdentifyAF ; "Identify(A-F)"
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------

loc_1CAB6:                              ; CODE XREF: smith_action_prompt+27↑j
                cmp     [bp+arg_0], 5
                jnz     short loc_1CAC2
                mov     ax, offset aSellAF ; "Sell (A-F)   "
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------
                align 2

loc_1CAC2:                              ; CODE XREF: smith_action_prompt+32↑j
                mov     ax, offset aBuyAF ; "Buy (A-F)    "

loc_1CAC5:                              ; CODE XREF: smith_action_prompt+2C↑j
                                        ; smith_action_prompt+37↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                call    sub_1C8E0
                call    sub_1C5F6

loc_1CADC:                              ; CODE XREF: smith_action_prompt+13↑j
                mov     di, 1
                call    thk_monster_anim_step
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 41h ; 'A'
                jb      short loc_1CB1C
                cmp     ax, 46h ; 'F'
                ja      short loc_1CB1C
                sub     si, 41h ; 'A'
                cmp     word_2307A, 5
                jnz     short loc_1CB08
                push    si
                call    loc_1C5A2

loc_1CB03:                              ; CODE XREF: smith_action_prompt+8B↓j
                                        ; smith_action_prompt+92↓j
                add     sp, 2
                jmp     short loc_1CB54
; ---------------------------------------------------------------------------

loc_1CB08:                              ; CODE XREF: smith_action_prompt+75↑j
                cmp     word_2307A, 6
                jnz     short loc_1CB16
                push    si
                call    sub_1C1B0
                jmp     short loc_1CB03
; ---------------------------------------------------------------------------
                align 2

loc_1CB16:                              ; CODE XREF: smith_action_prompt+85↑j
                push    si
                call    sub_1C776
                jmp     short loc_1CB03
; ---------------------------------------------------------------------------

loc_1CB1C:                              ; CODE XREF: smith_action_prompt+66↑j
                                        ; smith_action_prompt+6B↑j
                mov     ax, si
                cmp     ax, 47h ; 'G'
                jnz     short loc_1CB3A
                push    word_23028
                call    thk_party_gather_gold
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    sub_1C6FC
                add     sp, 4
                jmp     short loc_1CB52
; ---------------------------------------------------------------------------
                align 2

loc_1CB3A:                              ; CODE XREF: smith_action_prompt+99↑j
                push    si
                call    smith_draw
                add     sp, 2           ; CODE XREF: seg002:0A65↑J
                or      ax, ax
                jz      short loc_1CB52
                mov     ax, si
                sub     ax, 31h ; '1'
                mov     word_23028, ax
                call    loc_1CB9A
                jmp     short loc_1CB54
; ---------------------------------------------------------------------------

loc_1CB52:                              ; CODE XREF: smith_action_prompt+AF↑j
                                        ; smith_action_prompt+BB↑j
                sub     di, di

loc_1CB54:                              ; CODE XREF: smith_action_prompt+7E↑j
                                        ; smith_action_prompt+C8↑j
                cmp     si, 1Bh
                jz      short loc_1CB5C
                jmp     loc_1CA99
; ---------------------------------------------------------------------------

loc_1CB5C:                              ; CODE XREF: smith_action_prompt+CF↑j
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
smith_action_prompt endp


; =============== S U B R O U T I N E =======================================

; " Blacksmith ", "Gold=", G-Gather Gold, #-Other Char
; Attributes: bp-based frame

smith_draw      proc near               ; CODE XREF: smith_action_prompt+B3↑p
                                        ; blacksmith_menu+1D1↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     [bp+arg_0], 31h ; '1'
                mov     ax, word_23028
                cmp     [bp+arg_0], ax
                jz      short loc_1CB96
                cmp     [bp+arg_0], 0
                jl      short loc_1CB96
                mov     ax, g_party_size
                cmp     [bp+arg_0], ax
                jge     short loc_1CB96
                mov     bx, [bp+arg_0]
                shl     bx, 1

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CB96
                mov     ax, 1
                jmp     short loc_1CB98
; ---------------------------------------------------------------------------

loc_1CB96:                              ; CODE XREF: smith_draw+D↑j
                                        ; smith_draw+13↑j ...
                sub     ax, ax

loc_1CB98:                              ; CODE XREF: smith_draw+2C↑j
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CB9A:                              ; CODE XREF: sub_1C1B0+3E9↑p
                                        ; smith_action_prompt+C5↑p ...
                push    word_23028
                call    thk_char_ptr
                add     sp, 2
                mov     word_2308E, ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, word_23028
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word_2308E
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    sub_1C6FC       ; CODE XREF: seg002:01AD↑J
                add     sp, 4
                mov     word_23078, 0
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CBE8:                              ; CODE XREF: sub_1C1B0+1B↑p
                                        ; ovl_2SMITH:C5B3↑p ...
                push    bp
                mov     bp, sp
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                inc     [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+5814h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+5814h]
                call    thk_text_puts
                add     sp, 2
                call    thk_monster_anim_step
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CC38:                              ; CODE XREF: sub_1C1B0+3E6↑p
                                        ; blacksmith_menu+EF↓p
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                call    thk_print_gold_label
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBlacksmith ; " Blacksmith "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGold_3 ; "Gold="
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGGatherGold_1 ; "G-Gather Gold"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOtherChar_1 ; "#-Other Char"
                push    ax
                call    thk_text_puts
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2
smith_draw      endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

blacksmith_menu proc near               ; CODE XREF: seg002:08FD↑J

var_C           = word ptr -0Ch
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_2], 1
                mov     ax, 2
                push    ax
                call    thk_load_building_text
                add     sp, 2
                mov     [bp+var_6], 0Ah
                mov     si, 5814h
                mov     di, 0Ah

loc_1CCDC:                              ; CODE XREF: blacksmith_menu+2B↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CCDC
                mov     [bp+var_6], 5
                mov     [bp+var_8], 4
                mov     [bp+var_C], 0

loc_1CCF6:                              ; CODE XREF: blacksmith_menu+59↓j
                mov     si, [bp+var_C]
                add     si, 57DAh
                mov     di, 4

loc_1CD00:                              ; CODE XREF: blacksmith_menu+4F↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CD00
                add     [bp+var_C], 8
                cmp     [bp+var_C], 28h ; '('
                jl      short loc_1CCF6
                mov     [bp+var_6], 6
                mov     si, 582Ch
                mov     di, 6

loc_1CD20:                              ; CODE XREF: blacksmith_menu+6F↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CD20
                mov     byte_2294F, 0FDh
                or      byte_1DC80, 6
                mov     word_23028, 0
                cmp     g_party_size, 0
                jle     short loc_1CD59
                mov     si, 416h
                mov     dx, g_party_size
                mov     cx, word_23028

loc_1CD4D:                              ; CODE XREF: blacksmith_menu:loc_1CE3F↓j
                cmp     word ptr [si], 18h
                jl      short loc_1CD55
                jmp     loc_1CE34
; ---------------------------------------------------------------------------

loc_1CD55:                              ; CODE XREF: blacksmith_menu+96↑j
                                        ; blacksmith_menu+182↓j
                mov     word_23028, cx

loc_1CD59:                              ; CODE XREF: blacksmith_menu+86↑j
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CD66:                              ; CODE XREF: blacksmith_menu+D5↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 3
                shl     bx, cl
                push    word ptr [bx+di+57DAh]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1CD66
                mov     [bp+var_6], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jnz     short loc_1CDA1
                jmp     loc_1CEBE
; ---------------------------------------------------------------------------

loc_1CDA1:                              ; CODE XREF: blacksmith_menu+E2↑j
                or      byte_1DC80, 1
                call    thk_draw_screen_rows
                call    loc_1CC38
                mov     word_23078, 1

loc_1CDB2:                              ; CODE XREF: blacksmith_menu+1FE↓j
                mov     word_2307A, 0
                cmp     word_23078, 0
                jz      short loc_1CDC2
                call    loc_1CB9A

loc_1CDC2:                              ; CODE XREF: blacksmith_menu+103↑j
                cmp     [bp+var_2], 0
                jz      short loc_1CE11
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSelectAF ; "Select (A-F) "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     si, si
                mov     di, 582Ch

loc_1CDEF:                              ; CODE XREF: blacksmith_menu+152↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 6
                jl      short loc_1CDEF
                mov     [bp+var_6], si

loc_1CE11:                              ; CODE XREF: blacksmith_menu+10C↑j
                mov     [bp+var_2], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_4], ax
                sub     ax, 41h ; 'A'   ; switch 7 cases
                cmp     ax, 6
                ja      short def_1CE2E ; jumptable 0001CE2E default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1CE2E[bx] ; CODE XREF: seg002:0831↑J
                                        ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1CE34:                              ; CODE XREF: blacksmith_menu+98↑j
                add     si, 2
                inc     cx
                cmp     cx, dx
                jl      short loc_1CE3F
                jmp     loc_1CD55
; ---------------------------------------------------------------------------

loc_1CE3F:                              ; CODE XREF: blacksmith_menu+180↑j
                jmp     loc_1CD4D
; ---------------------------------------------------------------------------

loc_1CE42:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu:jpt_1CE2E↓o
                mov     ax, 1           ; jumptable 0001CE2E case 65

loc_1CE45:                              ; CODE XREF: blacksmith_menu+197↓j
                                        ; blacksmith_menu+19D↓j ...
                push    ax
                call    smith_action_prompt
                add     sp, 2
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------

loc_1CE4E:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1EC↓o
                mov     ax, 2           ; jumptable 0001CE2E case 66
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE54:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1EE↓o
                mov     ax, 3           ; jumptable 0001CE2E case 67
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE5A:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F0↓o
                mov     ax, 4           ; jumptable 0001CE2E case 68
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE60:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F2↓o
                mov     ax, 5           ; jumptable 0001CE2E case 69
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE66:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F4↓o
                mov     ax, 6           ; jumptable 0001CE2E case 70
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE6C:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F6↓o
                push    word_23028      ; jumptable 0001CE2E case 71
                call    thk_party_gather_gold
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    sub_1C6FC
                add     sp, 4

loc_1CE80:                              ; CODE XREF: blacksmith_menu+1DC↓j
                mov     [bp+var_2], 0
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------
                align 2

def_1CE2E:                              ; CODE XREF: blacksmith_menu+16F↑j
                push    [bp+var_4]      ; jumptable 0001CE2E default case
                call    smith_draw
                add     sp, 2
                mov     word_23078, ax
                or      ax, ax
                jz      short loc_1CE80
                mov     ax, [bp+var_4]
                sub     ax, 31h ; '1'
                mov     word_23028, ax
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------
                align 2
jpt_1CE2E       dw offset loc_1CE42     ; DATA XREF: blacksmith_menu+174↑r
                                        ; jump table for switch statement
                dw offset loc_1CE4E     ; jumptable 0001CE2E case 66
                dw offset loc_1CE54     ; jumptable 0001CE2E case 67
                dw offset loc_1CE5A     ; jumptable 0001CE2E case 68
                dw offset loc_1CE60     ; jumptable 0001CE2E case 69
                dw offset loc_1CE66     ; jumptable 0001CE2E case 70
                dw offset loc_1CE6C     ; jumptable 0001CE2E case 71
; ---------------------------------------------------------------------------

loc_1CEB2:                              ; CODE XREF: blacksmith_menu+192↑j
                                        ; blacksmith_menu+1CB↑j ...
                cmp     [bp+var_4], 1Bh
                jz      short loc_1CEBB
                jmp     loc_1CDB2
; ---------------------------------------------------------------------------

loc_1CEBB:                              ; CODE XREF: blacksmith_menu+1FC↑j
                call    thk_text_clear_prompt_line

loc_1CEBE:                              ; CODE XREF: blacksmith_menu+E4↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
blacksmith_menu endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CEC8       proc near               ; CODE XREF: seg002:07AD↑J

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     byte_2294F, 0FDh
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     [bp+var_2], 0
                sub     ax, ax
                mov     cx, 5
                mov     di, 9680h
                push    ds
                pop     es
                repne stosw
                stosb
                add     [bp+var_2], 0Bh
                sub     si, si
                mov     di, 58B8h

loc_1CEF9:                              ; CODE XREF: sub_1CEC8+4E↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1CEF9
                mov     [bp+var_2], si
                call    thk_monster_anim_step
                mov     byte_26ED0, 0FFh
                mov     byte_26ED1, 0E1h
                mov     byte_26ED2, 0C2h ; CODE XREF: seg002:050D↑J
                mov     byte_26ED3, 0C1h
                mov     byte_26ED4, 0E0h
                mov     byte_1DC65, 83h
                mov     [bp+var_2], 5
                sub     ax, ax
                mov     cx, 3
                mov     di, 9685h
                push    ds
                pop     es
                repne stosw
                add     [bp+var_2], 6
                call    thk_start_combat
                cmp     byte_1DD59, 0
                jz      short loc_1CF6C
                mov     ax, 0FFFFh      ; CODE XREF: seg002:0AE9↑J
                push    ax
                push    ax
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                call    loc_1D2A4
                jmp     short loc_1CF71
; ---------------------------------------------------------------------------

loc_1CF6C:                              ; CODE XREF: sub_1CEC8+91↑j
                mov     byte_1DBE5, 1

loc_1CF71:                              ; CODE XREF: sub_1CEC8+A2↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CEC8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CF78       proc near               ; CODE XREF: sub_1D098+18↓p

var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 10h
                mov     [bp+var_10], 30h ; '0'
                mov     ax, word_1DC86  ; CODE XREF: seg002:062D↑J
                mov     dx, word_1DC88
                mov     [bp+var_A], ax
                mov     [bp+var_8], dx
                call    thk_text_get_x
                mov     [bp+var_C], ax
                call    thk_text_get_y
                mov     [bp+var_E], ax
                mov     word_221B4, 1
                sub     ax, ax
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0EA60h
                sub     dx, dx
                push    dx
                push    ax
                push    [bp+var_8]
                push    [bp+var_A]
                call    thk__aFuldiv
                mov     [bp+var_2], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_2]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0EA60h
                sub     dx, dx
                push    dx
                push    ax
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk__aFulmul
                sub     [bp+var_A], ax
                sbb     [bp+var_8], dx
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 3E8h
                cwd
                push    dx
                push    ax
                push    [bp+var_8]
                push    [bp+var_A]
                call    thk__aFuldiv
                mov     [bp+var_4], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_4]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 3E8h
                mul     [bp+var_4]
                sub     dx, dx
                sub     [bp+var_A], ax
                sbb     [bp+var_8], dx
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_A]
                sub     dx, dx
                mov     cx, 0Ah
                div     cx
                mov     [bp+var_6], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_6]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_E]
                push    [bp+var_C]
                call    thk_text_goto_xy
                add     sp, 4
                mov     word_221B4, 0
                cmp     [bp+var_2], 0
                jnz     short loc_1D094
                cmp     [bp+var_4], 0
                jnz     short loc_1D094
                cmp     [bp+var_6], 0
                jnz     short loc_1D094
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D092
                mov     ax, 1
                jmp     short loc_1D094
; ---------------------------------------------------------------------------

loc_1D092:                              ; CODE XREF: sub_1CF78+113↑j
                sub     ax, ax

loc_1D094:                              ; CODE XREF: sub_1CF78+FE↑j
                                        ; sub_1CF78+104↑j ...
                mov     sp, bp
                pop     bp
                retn
sub_1CF78       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D098       proc near               ; CODE XREF: sub_1D098:loc_1D127↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

; FUNCTION CHUNK AT D16A SIZE 00000032 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     ax, word_221B4
                mov     [bp+var_2], ax
                mov     word_221B4, 0

loc_1D0AB:                              ; CODE XREF: sub_1D098+66↓j
                call    thk_kbd_poll
                mov     si, ax
                call    sub_1CF78
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D0C2
                mov     si, 0Dh
                jmp     short loc_1D0CC
; ---------------------------------------------------------------------------
                align 2

loc_1D0C2:                              ; CODE XREF: seg002:04DD↑J
                                        ; sub_1D098+22↑j
                sub     word_1DC86, 4Bh ; 'K'
                sbb     word_1DC88, 0

loc_1D0CC:                              ; CODE XREF: sub_1D098+27↑j
                or      si, si
                jnz     short loc_1D0FC
                mov     bx, word_1DD62
                inc     word_1DD62
                mov     al, [bx+50Ah]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     word_1DD62, 8
                jnz     short loc_1D0F2
                mov     word_1DD62, 0

loc_1D0F2:                              ; CODE XREF: sub_1D098+52↑j
                mov     ax, 32h ; '2'
                push    ax
                call    thk_delay_ticks
                add     sp, 2

loc_1D0FC:                              ; CODE XREF: sub_1D098+36↑j
                or      si, si
                jz      short loc_1D0AB
                mov     [bp+var_4], si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_2]
                mov     word_221B4, ax
                mov     ax, si
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1D11A:                              ; CODE XREF: ovl_2SMITH:D5F1↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D127:                              ; CODE XREF: sub_1D098+DC↓j
                call    sub_1D098
                mov     byte ptr [bp+var_4], al
                cmp     al, 8
                jz      short loc_1D135
                cmp     al, 0F0h
                jnz     short loc_1D146

loc_1D135:                              ; CODE XREF: sub_1D098+97↑j
                or      si, si
                jz      short loc_1D146
                mov     ax, 1
                push    ax
                call    thk_text_erase_chars
                add     sp, 2
                dec     si
                jmp     short loc_1D16A
; ---------------------------------------------------------------------------

loc_1D146:                              ; CODE XREF: sub_1D098+9B↑j
                                        ; sub_1D098+9F↑j
                cmp     byte ptr [bp+var_4], 20h ; ' '
                jb      short loc_1D16A
                cmp     byte ptr [bp+var_4], 7Fh
                ja      short loc_1D16A
                cmp     si, 8
                jge     short loc_1D16A
                mov     bx, si
                add     bx, di          ; CODE XREF: seg002:0825↑J
sub_1D098       endp

                mov     al, [bp-4]
                mov     [bx], al
                inc     si
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
; START OF FUNCTION CHUNK FOR sub_1D098

loc_1D16A:                              ; CODE XREF: sub_1D098+AC↑j
                                        ; sub_1D098+B2↑j ...
                cmp     byte ptr [bp+var_4], 1Bh
                jz      short loc_1D176
                cmp     byte ptr [bp+var_4], 0Dh
                jnz     short loc_1D127

loc_1D176:                              ; CODE XREF: sub_1D098+D6↑j
                mov     [bp+var_2], si
                cmp     byte ptr [bp+var_4], 1Bh
                jnz     short loc_1D18B
                push    si
                call    thk_text_erase_chars
                add     sp, 2
                mov     [bp+var_2], 0

loc_1D18B:                              ; CODE XREF: sub_1D098+E5↑j
                mov     bx, [bp+var_2]
                mov     si, [bp+arg_0]
                mov     byte ptr [bx+si], 0
                mov     ax, bx
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1D098

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D19C       proc near               ; CODE XREF: ovl_2SMITH:D558↓p

var_1E          = byte ptr -1Eh
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 20h
                push    di
                push    si
                mov     [bp+var_4], 0
                sub     ax, ax
                mov     cx, 0Dh
                lea     di, [bp+var_1E]
                push    ss
                pop     es
                assume es:nothing
                repne stosw
                add     [bp+var_4], 1Ah
                sub     si, si

loc_1D1BB:                              ; CODE XREF: sub_1D19C+3D↓j
                                        ; sub_1D19C+55↓j
                mov     ax, 1Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                dec     al
                mov     byte ptr [bp+var_2], al
                mov     di, [bp+var_2]
                and     di, 0FFh
                cmp     [bp+di+var_1E], 0
                jnz     short loc_1D1BB
                mov     di, [bp+var_2]
                and     di, 0FFh
                mov     [bp+di+var_1E], 1
                mov     al, byte ptr [bp+var_2]
                mov     [si+584Eh], al
                inc     si
                cmp     si, 1Ah
                jl      short loc_1D1BB
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D19C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D1FC       proc near               ; CODE XREF: sub_1D236+11↓p
                                        ; ovl_2SMITH:D5A5↓p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                test    byte ptr [si+482Bh], 1
                jz      short loc_1D216
                mov     al, [si+580Dh]
                add     al, 41h ; 'A'
                jmp     short loc_1D22A
; ---------------------------------------------------------------------------

loc_1D216:                              ; CODE XREF: sub_1D1FC+10↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                test    byte ptr [si+482Bh], 2
                jz      short loc_1D22D
                mov     al, [si+57EDh]
                add     al, 61h ; 'a'

loc_1D22A:                              ; CODE XREF: sub_1D1FC+18↑j
                mov     [bp+arg_0], al

loc_1D22D:                              ; CODE XREF: sub_1D1FC+26↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                pop     si
                pop     bp
                retn
sub_1D1FC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D236       proc near               ; CODE XREF: ovl_2SMITH:D570↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     si, [bp+arg_0]
                jmp     short loc_1D256
; ---------------------------------------------------------------------------
                align 2

loc_1D240:                              ; CODE XREF: sub_1D236+23↓j
                inc     si
                mov     al, [si-1]
                sub     ah, ah
                push    ax
                call    sub_1D1FC
                add     sp, 2
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_1D256:                              ; CODE XREF: sub_1D236+7↑j
                cmp     byte ptr [si], 0
                jnz     short loc_1D240
                mov     [bp+arg_0], si
                pop     si
                pop     bp
                retn
sub_1D236       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D262       proc near               ; CODE XREF: ovl_2SMITH:D60F↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    di
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D274
                mov     ax, 2
                jmp     short loc_1D2A0
; ---------------------------------------------------------------------------

loc_1D274:                              ; CODE XREF: sub_1D262+B↑j
                push    ds
                pop     es
                assume es:DGROUP
                mov     di, [bp+arg_0]
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                cmp     cx, 8
                jz      short loc_1D28C

loc_1D288:                              ; CODE XREF: sub_1D262+39↓j
                sub     ax, ax
                jmp     short loc_1D2A0
; ---------------------------------------------------------------------------

loc_1D28C:                              ; CODE XREF: sub_1D262+24↑j
                mov     ax, 58AEh
                push    ax
                push    [bp+arg_0]
                call    thk__stricmp
                add     sp, 4
                or      ax, ax
                jnz     short loc_1D288
                mov     ax, 1

loc_1D2A0:                              ; CODE XREF: sub_1D262+10↑j
                                        ; sub_1D262+28↑j
                pop     di
                pop     bp
                retn
sub_1D262       endp

; ---------------------------------------------------------------------------
                align 2

loc_1D2A4:                              ; CODE XREF: sub_1CEC8+9F↑p
                push    bp
                mov     bp, sp
                sub     sp, 24h
                push    di
                push    si
                mov     word ptr [bp-1Ah], 0
                sub     ax, ax
                mov     [bp-1Ch], ax
                mov     [bp-1Eh], ax
                mov     [bp-0Eh], ax
                mov     [bp-10h], ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 3
                push    ax
                call    thk_load_building_text
                add     sp, 2
                mov     word ptr [bp-18h], 4
                mov     si, 58B8h
                mov     di, 4

loc_1D2DB:                              ; CODE XREF: ovl_2SMITH:D2E4↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D2DB
                mov     word ptr [bp-18h], 4
                mov     si, 58C0h
                mov     di, 4

loc_1D2F1:                              ; CODE XREF: ovl_2SMITH:D2FA↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D2F1
                mov     word ptr [bp-18h], 0Eh
                mov     si, 5892h
                mov     di, 0Eh

loc_1D307:                              ; CODE XREF: ovl_2SMITH:D310↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D307
                mov     word ptr [bp-18h], 4
                mov     si, 5846h
                mov     di, 4

loc_1D31D:                              ; CODE XREF: ovl_2SMITH:D326↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D31D
                mov     word ptr [bp-18h], 0Bh
                mov     si, 5868h
                mov     di, 0Bh

loc_1D333:                              ; CODE XREF: ovl_2SMITH:D33C↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D333
                mov     word ptr [bp-18h], 0Ah
                mov     si, 587Eh
                mov     di, 0Ah

loc_1D349:                              ; CODE XREF: ovl_2SMITH:D352↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D349
                mov     g_disk_needed, 1

loc_1D35A:                              ; CODE XREF: ovl_2SMITH:D36C↓j
                push    word_1DD50
                call    thk_gfx_load_image
                add     sp, 2
                mov     [bp-10h], ax
                mov     [bp-0Eh], dx
                or      ax, dx
                jz      short loc_1D35A
                mov     g_disk_needed, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                sub     ax, ax
                push    ax
                push    dx
                push    word ptr [bp-10h]
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 45BCh
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTheControlRoom ; "        the control room..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                call    thk_draw_screen_rows
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 58C0h

loc_1D3DE:                              ; CODE XREF: ovl_2SMITH:D3FB↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1D3DE
                mov     [bp-18h], si
                mov     ax, 15h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_1D40E:                              ; CODE XREF: ovl_2SMITH:D420↓j
                mov     ax, 0Ah
                push    ax
                lea     ax, [bp-0Ch]
                push    ax
                call    thk_read_string
                add     sp, 4
                mov     si, ax
                or      si, si
                jz      short loc_1D40E
                mov     [bp-18h], si
                cmp     si, 0Ah
                jz      short loc_1D444
                mov     ax, 0Ah
                sub     ax, si
                mov     [bp-24h], ax
                mov     al, 20h ; ' '
                mov     cx, [bp-24h]
                lea     di, [bp+si-0Ch]
                push    ss
                pop     es
                assume es:nothing
                repne stosb
                mov     ax, [bp-24h]
                add     [bp-18h], ax

loc_1D444:                              ; CODE XREF: ovl_2SMITH:D428↑j
                mov     byte ptr [bp-2], 0
                sub     si, si
                mov     di, [bp-1Ah]
                jmp     short loc_1D451
; ---------------------------------------------------------------------------
                align 2

loc_1D450:                              ; CODE XREF: ovl_2SMITH:D46A↓j
                inc     si

loc_1D451:                              ; CODE XREF: ovl_2SMITH:D44D↑j
                cmp     si, g_party_size
                jge     short loc_1D46C
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                test    byte ptr [bx+81h], 20h
                jz      short loc_1D468
                inc     di

loc_1D468:                              ; CODE XREF: ovl_2SMITH:D465↑j
                or      di, di
                jz      short loc_1D450

loc_1D46C:                              ; CODE XREF: ovl_2SMITH:D455↑j
                mov     [bp-1Ah], di
                mov     [bp-18h], si
                mov     ax, offset aWafe ; "WAFE      "
                push    ax
                lea     ax, [bp-0Ch]
                push    ax
                call    thk__stricmp
                add     sp, 4
                or      ax, ax
                jz      short loc_1D487
                jmp     loc_1D7E8
; ---------------------------------------------------------------------------

loc_1D487:                              ; CODE XREF: ovl_2SMITH:D482↑j
                or      di, di
                jnz     short loc_1D48E
                jmp     loc_1D7E8
; ---------------------------------------------------------------------------

loc_1D48E:                              ; CODE XREF: ovl_2SMITH:D489↑j
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aThankYou ; "         Thank you."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRecalculatingT ; "Recalculating trajectory now..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aErrorErrorComp ; "Error, Error!  Computer malfunction."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aInternalProgra ; "    Internal program override..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_clear_text_preset
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
                sub     si, si
                mov     di, 5892h

loc_1D536:                              ; CODE XREF: ovl_2SMITH:D553↓j
                lea     ax, [si+2]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Eh
                jl      short loc_1D536
                mov     [bp-18h], si
                call    sub_1D19C
                sub     si, si
                mov     di, 5846h

loc_1D560:                              ; CODE XREF: ovl_2SMITH:D57D↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    sub_1D236
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1D560
                mov     [bp-18h], si
                mov     ax, 16h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAnswerPreamble ; "Answer= Preamble     Code="
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     si, si

loc_1D59C:                              ; CODE XREF: ovl_2SMITH:D5B3↓j
                mov     bx, word ptr unk_21E0A
                mov     al, [bx+si]
                sub     ah, ah
                push    ax
                call    sub_1D1FC
                add     sp, 2
                mov     [si+58AEh], al
                inc     si
                cmp     si, 8
                jl      short loc_1D59C
                mov     [bp-18h], si
                mov     byte_23106, 0
                mov     word_1DC86, 0BBA0h
                mov     word_1DC88, 0Dh

loc_1D5C9:                              ; CODE XREF: ovl_2SMITH:D609↓j
                                        ; ovl_2SMITH:D619↓j
                mov     ax, 16h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 16h
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_clear_text_rect
                add     sp, 8           ; CODE XREF: seg002:0795↑J
                mov     ax, 16h
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:07A1↑J
                add     sp, 4
                lea     ax, [bp-0Ch]
                push    ax
                call    loc_1D11A

loc_1D5F4:                              ; CODE XREF: seg002:0819↑J
                add     sp, 2
                mov     si, ax
                or      si, si
                jnz     short loc_1D607
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D607
                inc     si

loc_1D607:                              ; CODE XREF: ovl_2SMITH:D5FB↑j
                                        ; ovl_2SMITH:D604↑j
                or      si, si
                jz      short loc_1D5C9
                lea     ax, [bp-0Ch]
                push    ax
                call    sub_1D262
                add     sp, 2
                mov     si, ax
                or      si, si
                jz      short loc_1D5C9
                mov     [bp-18h], si
                cmp     si, 2
                jnz     short loc_1D62C
                mov     byte_1DBE5, 3
                jmp     loc_1D830
; ---------------------------------------------------------------------------
                align 2

loc_1D62C:                              ; CODE XREF: ovl_2SMITH:D621↑j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     di, di
                mov     si, [bp-12h]
                jmp     short loc_1D66A
; ---------------------------------------------------------------------------
                align 2

loc_1D63E:                              ; CODE XREF: ovl_2SMITH:D66E↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                test    byte ptr [si+81h], 8
                jnz     short loc_1D65D
                or      byte ptr [si+81h], 8
                add     word ptr [si+62h], 0F080h
                adc     word ptr [si+64h], 2FAh

loc_1D65D:                              ; CODE XREF: ovl_2SMITH:D64C↑j
                mov     ax, [si+62h]
                mov     dx, [si+64h]
                add     [bp-1Eh], ax
                adc     [bp-1Ch], dx
                inc     di

loc_1D66A:                              ; CODE XREF: ovl_2SMITH:D63B↑j
                cmp     di, g_party_size
                jl      short loc_1D63E
                mov     [bp-18h], di
                mov     [bp-12h], si
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
                mov     ax, 3
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 5868h

loc_1D6A0:                              ; CODE XREF: ovl_2SMITH:D6BD↓j
                lea     ax, [si+1]
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Bh
                jl      short loc_1D6A0
                mov     [bp-18h], si
                sub     si, si
                mov     di, 587Eh

loc_1D6C7:                              ; CODE XREF: ovl_2SMITH:D6E4↓j
                lea     ax, [si+0Dh]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Ah
                jl      short loc_1D6C7
                mov     [bp-18h], si
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    g_battle_count
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0Dh
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_1DC62
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0Eh
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [bp-1Ch]
                push    word ptr [bp-1Eh]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, 0Ch
                push    ax
                mov     ax, 0Eh
                push    ax
                mov     ax, 1
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp-20h], ax
                push    ax
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_frame_alt
                push    word ptr [bp-20h]
                call    thk_text_window_close
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
                sub     si, si

loc_1D79E:                              ; CODE XREF: ovl_2SMITH:D7D5↓j
                mov     word ptr [bp-14h], 10h
                mov     di, 0Dh
                or      si, si
                jz      short loc_1D7AD
                add     di, 16h

loc_1D7AD:                              ; CODE XREF: ovl_2SMITH:D7A8↑j
                push    di
                push    word ptr [bp-14h]
                push    si
                inc     si
                push    word ptr [bp-0Eh]
                push    word ptr [bp-10h]
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 4Bh ; 'K'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                cmp     si, 0Ch
                jnz     short loc_1D7D0
                sub     si, si

loc_1D7D0:                              ; CODE XREF: ovl_2SMITH:D7CC↑j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1D79E
                mov     [bp-16h], di
                mov     [bp-18h], si
                call    thk_save_roster
                mov     byte_1DBE5, 2
                jmp     short loc_1D830
; ---------------------------------------------------------------------------
                align 2

loc_1D7E8:                              ; CODE XREF: ovl_2SMITH:D484↑j
                                        ; ovl_2SMITH:D48B↑j
                mov     ax, 16h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aIncorrect ; "Incorrect!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0C8h
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                sub     al, al
                mov     g_party_y, al
                mov     g_party_x, al
                call    thk_2PLAY_B75E
                mov     byte_1DC80, 7
                call    thk_2PLAY_A580
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                cmp     g_view_mode, 1
                jnz     short loc_1D830
                call    thk_2PLAY_B862

loc_1D830:                              ; CODE XREF: ovl_2SMITH:D628↑j
                                        ; ovl_2SMITH:D7E5↑j ...
                push    word ptr [bp-0Eh]
                push    word ptr [bp-10h]
                call    thk_free_far_block
                add     sp, 4
                sub     ax, ax
                mov     word_1DC88, ax
                mov     word_1DC86, ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
ovl_2SMITH      ends

