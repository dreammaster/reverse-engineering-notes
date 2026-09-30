; ===========================================================================

; Segment type: Pure code
ovl_2MISC       segment byte public 'CODE' use16
                assume cs:ovl_2MISC
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; 'B' Bash Door
; Attributes: bp-based frame

bash_door       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 6
                push    si
                mov     [bp+var_6], 0
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                sub     ah, ah
                mov     cl, byte_23217
                shr     ax, cl
                and     al, 3
                mov     [bp+var_4], al
                cmp     g_outdoors, 1
                jz      short loc_1C175
                cmp     al, bh
                jz      short loc_1C175
                mov     al, byte_23218
                and     al, byte_23216
                test    al, 55h
                jnz     short loc_1C18C

loc_1C175:                              ; CODE XREF: bash_door+34↑j
                                        ; bash_door+38↑j ...
                mov     ax, 0F2h
                push    ax
                call    thk_party_move_key
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_advance_time

loc_1C186:                              ; CODE XREF: bash_door+69↓j
                add     sp, 2
                jmp     loc_1C23C
; ---------------------------------------------------------------------------

loc_1C18C:                              ; CODE XREF: bash_door+43↑j
                cmp     [bp+var_4], 2
                jz      short loc_1C19C
                mov     ax, 1

loc_1C195:                              ; CODE XREF: bash_door+C8↓j
                push    ax
                call    thk_res_4478
                jmp     short loc_1C186
; ---------------------------------------------------------------------------
                align 2

loc_1C19C:                              ; CODE XREF: bash_door+60↑j
                sub     ax, ax
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+6Bh]
                mov     [bp+var_2], al
                cmp     g_party_size, 1
                jle     short loc_1C1C6
                mov     ax, 1
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+6Bh]
                add     [bp+var_2], al

loc_1C1C6:                              ; CODE XREF: bash_door+82↑j
                mov     ax, 6Dh ; 'm'
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                sub     ah, ah
                mov     cl, 0Ah
                div     cl

loc_1C1DA:                              ; CODE XREF: seg002:08CD↑J
                mov     [bp+var_4], al
                cmp     al, 5
                jz      short loc_1C1EC
                add     [bp+var_2], al
                mov     al, byte_231E8
                cmp     [bp+var_2], al

loc_1C1EA:                              ; CODE XREF: seg002:07F5↑J
                jb      short loc_1C1EF

loc_1C1EC:                              ; CODE XREF: bash_door+AF↑j
                inc     [bp+var_6]

loc_1C1EF:                              ; CODE XREF: bash_door:loc_1C1EA↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1C1FA
                mov     ax, 2
                jmp     short loc_1C195
; ---------------------------------------------------------------------------

loc_1C1FA:                              ; CODE XREF: bash_door+C3↑j
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                cmp     al, 33h ; '3'
                jb      short loc_1C216
                call    thk_map_cell_update
                jmp     loc_1C175
; ---------------------------------------------------------------------------
                align 2

loc_1C216:                              ; CODE XREF: bash_door+DD↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    loc_1C41E
                mov     [bp+var_4], al
                sub     ax, ax
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    sub_1C390
                add     sp, 4
                mov     byte_1DC80, 3
                call    thk_2PLAY_A580

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                                        ; bash_door+59↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
bash_door       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; 'U' Unlock
; Attributes: bp-based frame

unlock_door     proc near               ; CODE XREF: seg002:0639↑J

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                cmp     g_outdoors, 0
                jz      short loc_1C25C
                jmp     loc_1C333
; ---------------------------------------------------------------------------

loc_1C25C:                              ; CODE XREF: unlock_door+15↑j
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                and     al, byte_23216
                mov     [bp+var_2], al
                sub     ah, ah
                mov     cl, byte_23217
                shr     ax, cl
                cmp     ax, 2
                jz      short loc_1C289
                jmp     loc_1C333
; ---------------------------------------------------------------------------

loc_1C289:                              ; CODE XREF: unlock_door+42↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                shr     ax, 1
                mov     cl, byte_23218
                sub     ch, ch
                test    ax, cx
                jnz     short loc_1C2A0
                mov     ax, 3
                jmp     loc_1C32C
; ---------------------------------------------------------------------------

loc_1C2A0:                              ; CODE XREF: unlock_door+56↑j
                call    sub_1C7AA
                mov     [bp+var_6], ax
                cmp     ax, 1Bh
                jnz     short loc_1C2B2
                call    thk_res_421E
                jmp     loc_1C333
; ---------------------------------------------------------------------------
                align 2

loc_1C2B2:                              ; CODE XREF: unlock_door+67↑j
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4

loc_1C2C0:                              ; CODE XREF: seg002:026D↑J
                mov     [bp+var_2], al
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+1Eh]
                mov     [bp+var_4], al
                cmp     [bp+var_2], 60h ; '`'
                jnb     short loc_1C2E2
                mov     al, [bp+var_2]
                cmp     [bp+var_4], al
                jnb     short loc_1C326

loc_1C2E2:                              ; CODE XREF: unlock_door+96↑j
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                mov     al, byte_231E9
                mov     [bp+var_4], al  ; CODE XREF: seg002:0651↑J
                mov     al, [bp+var_2]
                cmp     [bp+var_4], al
                jb      short loc_1C306
                mov     ax, 2
                jmp     short loc_1C32C
; ---------------------------------------------------------------------------

loc_1C306:                              ; CODE XREF: unlock_door+BD↑j
                                        ; seg002:08F1↑J
                call    loc_1C41E
                mov     [bp+var_2], al
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_1C390
                add     sp, 4
                mov     byte_1DC80, 3
                call    thk_2PLAY_A580
                jmp     short loc_1C333
; ---------------------------------------------------------------------------
                align 2

loc_1C326:                              ; CODE XREF: unlock_door+9E↑j
                call    thk_map_cell_update
                mov     ax, 4

loc_1C32C:                              ; CODE XREF: unlock_door+5B↑j
                                        ; unlock_door+C2↑j
                push    ax
                call    thk_res_4478
                add     sp, 2

loc_1C333:                              ; CODE XREF: unlock_door+17↑j
                                        ; unlock_door+44↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
unlock_door     endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C338       proc near               ; CODE XREF: sub_1C390+3B↓p
                                        ; sub_1C390+59↓p

var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                mov     [bp+var_2], 1
                mov     [bp+var_4], 1
                mov     al, byte_231EA
                mov     [bp+var_6], al
                call    loc_1CA00
                mov     bx, ax
                shl     bx, 1
                mov     ax, [bx+2946h]
                mov     [bp+var_8], ax
                mov     dx, ax
                mov     cl, [bp+var_6]
                jmp     short loc_1C364
; ---------------------------------------------------------------------------
                align 2

loc_1C362:                              ; CODE XREF: sub_1C338+32↓j
                shl     dx, 1

loc_1C364:                              ; CODE XREF: sub_1C338+27↑j
                mov     al, cl
                dec     cl
                or      al, al
                jnz     short loc_1C362
                mov     [bp+var_8], dx
                mov     [bp+var_6], cl  ; CODE XREF: seg002:0471↑J
                lea     ax, [bp+arg_2]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                push    dx
                push    [bp+arg_0]
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    thk_char_apply_damage
                mov     sp, bp
                pop     bp
                retn
sub_1C338       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C390       proc near               ; CODE XREF: bash_door+FE↑p
                                        ; unlock_door+D3↑p ...

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                dec     [bp+arg_0]
                call    loc_1C400
                mov     ax, 64h ; 'd'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                push    [bp+arg_2]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+0Fh], 6
                jz      short loc_1C3C2
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_1C3D1

loc_1C3C2:                              ; CODE XREF: sub_1C390+2A↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                push    [bp+arg_2]
                call    sub_1C338
                add     sp, 4

loc_1C3D1:                              ; CODE XREF: sub_1C390+30↑j
                mov     [bp+var_4], 0
                cmp     g_party_size, 0
                jle     short loc_1C3F9
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     di, ax
                mov     si, [bp+var_4]

loc_1C3E7:                              ; CODE XREF: sub_1C390+64↓j
                push    di
                push    si
                call    sub_1C338
                add     sp, 4
                inc     si
                cmp     si, g_party_size
                jl      short loc_1C3E7

loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                mov     [bp+var_4], si

loc_1C3F9:                              ; CODE XREF: sub_1C390+4B↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C400:                              ; CODE XREF: sub_1C390+B↑p
                                        ; ovl_2MISC:C49A↓p
                sub     ax, ax
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C41E:                              ; CODE XREF: bash_door+EF↑p
                                        ; unlock_door:loc_1C306↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, offset aExplosion ; "Explosion!"
                push    ax
                call    thk_res_410A
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_4], al
                and     byte ptr [bp+var_4], 3
                call    loc_1CA00
                mov     [bp+var_2], ax
                mov     ax, 13h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
sub_1C390       endp


loc_1C462:                              ; CODE XREF: seg002:086D↑J
                mov     al, [bp-4]
                sub     ah, ah
                mov     si, ax
                mov     di, si
                mov     cl, 2
                shl     di, cl
                mov     ax, [bp-2]
                mov     cl, 4
                shl     ax, cl
                add     di, ax
                push    word ptr [di+28F2h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4           ; CODE XREF: seg002:0B31↑J
                push    word ptr [di+28F4h]
                call    thk_text_puts
                add     sp, 2
                call    loc_1C400
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

trap_or_explosion proc near             ; CODE XREF: sub_1C824+61↓p
                                        ; ovl_2MISC:C903↓p

var_C           = word ptr -0Ch
var_8           = word ptr -8
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                call    loc_1C41E
                mov     [bp+arg_0], al
                shl     al, 1
                add     al, 4
                mov     [bp+var_2], al
                mov     [bp+var_4], 3
                sub     ah, ah
                mov     [bp+var_C], ax
                inc     ax
                mov     di, ax
                mov     ax, [bp+var_C]
                mov     [bp+var_8], ax
                mov     si, 3

loc_1C4D1:                              ; CODE XREF: trap_or_explosion+7B↓j
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                push    [bp+var_8]
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     ax, 64h ; 'd'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                push    di
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     ax, 64h ; 'd'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                sub     ax, ax
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     ax, 12Ch
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                dec     si
                jnz     short loc_1C4D1
                push    [bp+arg_2]
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax

loc_1C52C:                              ; CODE XREF: seg002:0879↑J
                call    sub_1C390
                add     sp, 4
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
trap_or_explosion endp


; =============== S U B R O U T I N E =======================================

; "Backpacks full!", " found "
; Attributes: bp-based frame

treasure_give_item proc near            ; CODE XREF: treasure_share+134↓p

var_10          = word ptr -10h
var_E           = byte ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                mov     [bp+var_8], 0
                mov     bx, [bp+arg_0]
                mov     al, [bx+6950h]
                mov     [bp+var_C], al
                mov     al, [bx+6953h]
                mov     [bp+var_E], al
                mov     al, [bx+6956h]
                mov     [bp+var_2], al
                cmp     [bp+var_C], 0
                jnz     short loc_1C566
                jmp     loc_1C644
; ---------------------------------------------------------------------------

loc_1C566:                              ; CODE XREF: treasure_give_item+29↑j
                mov     [bp+var_6], 0
                mov     di, [bp+var_8]
                jmp     short loc_1C57F
; ---------------------------------------------------------------------------

loc_1C570:                              ; CODE XREF: treasure_give_item+71↓j
                or      di, di
                jnz     short loc_1C5D4
                inc     cx
                cmp     cx, 6
                jge     short loc_1C5D4
                jmp     short loc_1C5A0
; ---------------------------------------------------------------------------

loc_1C57C:                              ; CODE XREF: treasure_give_item+A1↓j
                inc     [bp+var_6]

loc_1C57F:                              ; CODE XREF: treasure_give_item+36↑j
                mov     ax, g_party_size
                cmp     [bp+var_6], ax
                jge     short loc_1C5DB
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_A], 0
                mov     si, [bp+arg_0]
                mov     dx, [bp+var_10]
                sub     cx, cx

loc_1C5A0:                              ; CODE XREF: treasure_give_item+42↑j
                mov     bx, [bp+var_4]
                add     bx, cx
                cmp     byte ptr [bx+3Ah], 0
                jnz     short loc_1C570
                mov     dx, [bp+var_4]  ; CODE XREF: seg002:0885↑J
                add     dx, cx
                mov     bx, dx
                mov     al, [bp+var_C]
                mov     [bx+3Ah], al
                mov     al, [bp+var_2]
                mov     [bx+40h], al
                mov     al, [bp+var_E]  ; CODE XREF: seg002:029D↑J
                mov     [bx+46h], al
                mov     byte ptr [si+6950h], 0
                mov     byte ptr [si+6956h], 0
                mov     byte ptr [si+6953h], 0
                inc     di

loc_1C5D4:                              ; CODE XREF: treasure_give_item+3A↑j
                                        ; treasure_give_item+40↑j
                mov     [bp+var_A], cx
                or      di, di          ; CODE XREF: seg002:01A1↑J
                jz      short loc_1C57C

loc_1C5DB:                              ; CODE XREF: treasure_give_item+4D↑j
                mov     [bp+var_8], di
                or      di, di
                jnz     short loc_1C5FE
                cmp     byte_22E10, 0
                jnz     short loc_1C5F3
                mov     ax, offset aBackpacksFull ; "Backpacks full!"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C5F3:                              ; CODE XREF: treasure_give_item+AF↑j
                inc     byte_22E10
                mov     byte_1DC84, 0FFh
                jmp     short loc_1C644
; ---------------------------------------------------------------------------

loc_1C5FE:                              ; CODE XREF: treasure_give_item+A8↑j
                push    [bp+var_4]
                call    thk_res_3E40
                add     sp, 2
                mov     ax, offset aFound ; " found "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     al, 14h
                mul     [bp+var_C]
                add     ax, 6960h
                push    ax
                call    thk_text_puts
                add     sp, 2
                and     [bp+var_E], 3Fh
                jz      short loc_1C644
                mov     ax, 2Bh ; '+'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_E]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C644:                              ; CODE XREF: treasure_give_item+2B↑j
                                        ; treasure_give_item+C4↑j ...
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
treasure_give_item endp


; =============== S U B R O U T I N E =======================================

; "Each share = ", " Gold", " Gems"
; Attributes: bp-based frame

treasure_share  proc near               ; CODE XREF: sub_1C824+78↓p
                                        ; party_search+2BE↓p

var_10          = word ptr -10h
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                mov     [bp+var_A], 0
                mov     [bp+var_4], 0
                cmp     g_party_size, 0
                jle     short loc_1C680
                mov     si, 416h
                mov     cx, g_party_size
                mov     ax, cx
                add     [bp+var_4], ax  ; CODE XREF: seg002:0891↑J
                mov     dx, [bp+var_A]

loc_1C672:                              ; CODE XREF: treasure_share+31↓j
                cmp     word ptr [si], 18h
                jge     short loc_1C678
                inc     dx

loc_1C678:                              ; CODE XREF: treasure_share+2B↑j
                add     si, 2
                loop    loc_1C672
                mov     [bp+var_A], dx

loc_1C680:                              ; CODE XREF: treasure_share+17↑j
                sub     ax, ax
                push    ax
                push    [bp+var_A]
                push    word_241AE
                push    word_241AC
                call    thk__aFuldiv
                mov     [bp+var_8], ax
                mov     [bp+var_6], dx
                mov     ax, word_241AA
                sub     dx, dx
                div     g_party_size
                mov     [bp+var_C], ax
                sub     ax, ax
                mov     word_241AE, ax
                mov     word_241AC, ax
                mov     word_241AA, ax
                mov     byte_22E10, 0
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aEachShare ; "Each share = "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    [bp+var_6]
                push    [bp+var_8]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, offset aGold ; " Gold"
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     [bp+var_C], 0   ; CODE XREF: seg002:0B19↑J
                jz      short loc_1C71B
                mov     ax, 2987h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    [bp+var_C]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, offset aGems ; " Gems"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C71B:                              ; CODE XREF: treasure_share+AA↑j
                mov     [bp+var_4], 0
                cmp     g_party_size, 0
                jle     short loc_1C763
                mov     [bp+var_10], 416h
                mov     si, [bp+var_4]

loc_1C72F:                              ; CODE XREF: treasure_share+111↓j
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                mov     ax, [bp+var_C]  ; CODE XREF: seg002:089D↑J
                add     [di+5Ch], ax
                mov     bx, [bp+var_10]
                cmp     word ptr [bx], 18h
                jge     short loc_1C752
                mov     ax, [bp+var_8]
                mov     dx, [bp+var_6]
                add     [di+66h], ax
                adc     [di+68h], dx

loc_1C752:                              ; CODE XREF: treasure_share+FA↑j
                add     [bp+var_10], 2
                inc     si
                cmp     si, g_party_size
                jl      short loc_1C72F
                mov     [bp+var_2], di
                mov     [bp+var_4], si

loc_1C763:                              ; CODE XREF: treasure_share+DB↑j
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                sub     si, si

loc_1C76F:                              ; CODE XREF: treasure_share+158↓j
                lea     ax, [si+14h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    si
                call    treasure_give_item
                add     sp, 2
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                cmp     byte_22E10, 0
                jz      short loc_1C79C

loc_1C795:                              ; CODE XREF: treasure_share+156↓j
                mov     [bp+var_4], si
                jmp     short loc_1C7A4
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1C79C:                              ; CODE XREF: treasure_share+149↑j
                inc     si
                cmp     si, 3
                jge     short loc_1C795
                jmp     short loc_1C76F
; ---------------------------------------------------------------------------

loc_1C7A4:                              ; CODE XREF: treasure_share+14E↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
treasure_share  endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C7AA       proc near               ; CODE XREF: unlock_door:loc_1C2A0↑p
                                        ; sub_1C824+11↓p ...

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     bx, word ptr unk_201F8
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+12h], al
                call    thk_res_5440
                push    word ptr unk_201F8
                call    thk_res_410A
                add     sp, 2

loc_1C7CB:                              ; CODE XREF: sub_1C7AA+67↓j
                mov     ax, 38h ; '8'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1C7E8

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                mov     ax, 1
                jmp     short loc_1C7EA
; ---------------------------------------------------------------------------
                align 2

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                                        ; sub_1C7AA+36↑j
                sub     ax, ax

loc_1C7EA:                              ; CODE XREF: sub_1C7AA+3B↑j
                mov     di, ax
                or      di, di
                jnz     short loc_1C80F
                sub     si, 31h ; '1'
                cmp     si, g_party_size
                jge     short loc_1C80F
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+26h]
                and     al, 0F0h
                cmp     al, 1
                sbb     cx, cx
                neg     cx
                mov     di, cx

loc_1C80F:                              ; CODE XREF: sub_1C7AA+44↑j
                                        ; sub_1C7AA+4D↑j
                or      di, di
                jz      short loc_1C7CB
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                call    thk_res_35A8
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C7AA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C824       proc near               ; CODE XREF: ovl_2MISC:C90D↓p
                                        ; party_search+238↓p

var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 1
                cmp     [bp+arg_0], 0FFh
                jz      short loc_1C88B
                call    sub_1C7AA
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C848
                call    thk_res_421E
                sub     ax, ax
                jmp     short loc_1C8AA
; ---------------------------------------------------------------------------
                align 2

loc_1C848:                              ; CODE XREF: sub_1C824+1A↑j
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+1Eh]
                mov     [bp+var_6], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                cmp     al, 60h ; '`'
                ja      short loc_1C876
                mov     al, [bp+var_6]
                cmp     [bp+var_2], al
                jbe     short loc_1C88B

loc_1C876:                              ; CODE XREF: sub_1C824+48↑j
                cmp     [bp+arg_0], 0
                jz      short loc_1C88B
                push    [bp+var_4]
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    trap_or_explosion
                add     sp, 4

loc_1C88B:                              ; CODE XREF: sub_1C824+F↑j
                                        ; sub_1C824+50↑j ...
                call    thk_party_count_able
                or      ax, ax
                jz      short loc_1C8A7
                mov     ax, 29AAh
                push    ax
                call    thk_res_410A
                add     sp, 2
                call    treasure_share
                mov     byte_2294F, 0FDh
                call    thk_monster_anim_step

loc_1C8A7:                              ; CODE XREF: sub_1C824+6C↑j
                mov     ax, 1

loc_1C8AA:                              ; CODE XREF: sub_1C824+21↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C824       endp

; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: party_search+24C↓p
                mov     bp, sp
                sub     sp, 6
                call    sub_1C7AA
                mov     [bp-4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C8C6
                call    thk_res_421E
                sub     ax, ax
                jmp     short loc_1C916
; ---------------------------------------------------------------------------

loc_1C8C6:                              ; CODE XREF: ovl_2MISC:C8BD↑j
                push    word ptr [bp-4]
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+1Eh]
                mov     [bp-6], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp-2], al
                cmp     al, 60h ; '`'
                ja      short loc_1C8F4
                mov     al, [bp-6]
                cmp     [bp-2], al
                jbe     short loc_1C909

loc_1C8F4:                              ; CODE XREF: ovl_2MISC:C8EA↑j
                cmp     byte ptr [bp+4], 0
                jz      short loc_1C909
                push    word ptr [bp-4]
                mov     al, [bp+4]
                sub     ah, ah
                push    ax
                call    trap_or_explosion
                add     sp, 4

loc_1C909:                              ; CODE XREF: ovl_2MISC:C8F2↑j
                                        ; ovl_2MISC:C8F8↑j
                mov     ax, 0FFh
                push    ax
                call    sub_1C824
                add     sp, 2
                mov     ax, 1

loc_1C916:                              ; CODE XREF: ovl_2MISC:C8C4↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: party_search+25C↓p
                mov     bp, sp
                sub     sp, 4
                push    si
                sub     cx, cx
                sub     si, si

loc_1C925:                              ; CODE XREF: ovl_2MISC:C938↓j
                cmp     byte ptr [si+6953h], 0
                jnz     short loc_1C933
                cmp     byte ptr [si+6956h], 0
                jz      short loc_1C934

loc_1C933:                              ; CODE XREF: ovl_2MISC:C92A↑j
                inc     cx

loc_1C934:                              ; CODE XREF: ovl_2MISC:C931↑j
                inc     si
                cmp     si, 3
                jl      short loc_1C925
                mov     [bp-4], si
                mov     [bp-2], cx
                mov     ax, 29B4h
                push    ax
                call    thk_res_410A
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aContentsMagica ; "Contents magical ("
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word ptr [bp-2], 0
                jz      short loc_1C96E
                push    word_20192
                jmp     short loc_1C972
; ---------------------------------------------------------------------------

loc_1C96E:                              ; CODE XREF: ovl_2MISC:C966↑j
                push    word_20194

loc_1C972:                              ; CODE XREF: ovl_2MISC:C96C↑j
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aHasTrap ; "), has trap ("
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte ptr [bp+4], 0
                jz      short loc_1C98E
                push    word_20192
                jmp     short loc_1C992
; ---------------------------------------------------------------------------

loc_1C98E:                              ; CODE XREF: ovl_2MISC:C986↑j
                push    word_20194

loc_1C992:                              ; CODE XREF: ovl_2MISC:C98C↑j
                call    thk_text_puts
                add     sp, 2
                mov     ax, 29h ; ')'   ; CODE XREF: seg002:07DD↑J
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     ax, ax
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: party_search+FE↓p
                mov     bp, sp
                sub     sp, 2
                mov     al, byte_1DBEC
                sub     ah, ah
                cmp     ax, 6           ; switch 7 cases
                ja      short def_1C9BD ; jumptable 0001C9BD default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1C9BD[bx] ; switch jump
; ---------------------------------------------------------------------------

loc_1C9C2:                              ; CODE XREF: ovl_2MISC:C9BD↑j
                                        ; DATA XREF: ovl_2MISC:jpt_1C9BD↓o
                mov     word ptr [bp-2], 46h ; 'F' ; jumptable 0001C9BD case 0
                jmp     short def_1C9BD ; jumptable 0001C9BD default case
; ---------------------------------------------------------------------------
                align 2

loc_1C9CA:                              ; CODE XREF: ovl_2MISC:C9BD↑j
                                        ; DATA XREF: ovl_2MISC:C9EC↓o
                mov     word ptr [bp-2], 48h ; 'H' ; jumptable 0001C9BD case 1
                jmp     short def_1C9BD ; jumptable 0001C9BD default case
; ---------------------------------------------------------------------------
                align 2

loc_1C9D2:                              ; CODE XREF: ovl_2MISC:C9BD↑j
                                        ; DATA XREF: ovl_2MISC:C9EE↓o ...
                mov     word ptr [bp-2], 47h ; 'G' ; jumptable 0001C9BD cases 2,5
                jmp     short def_1C9BD ; jumptable 0001C9BD default case
; ---------------------------------------------------------------------------
                align 2

loc_1C9DA:                              ; CODE XREF: ovl_2MISC:C9BD↑j
                                        ; DATA XREF: ovl_2MISC:C9F0↓o
                mov     word ptr [bp-2], 49h ; 'I' ; jumptable 0001C9BD case 3
                jmp     short def_1C9BD ; jumptable 0001C9BD default case
; ---------------------------------------------------------------------------
                align 2

loc_1C9E2:                              ; CODE XREF: ovl_2MISC:C9BD↑j
                                        ; seg002:0B01↑J
                                        ; DATA XREF: ...
                mov     word ptr [bp-2], 4Ah ; 'J' ; jumptable 0001C9BD cases 4,6
                jmp     short def_1C9BD ; jumptable 0001C9BD default case
; ---------------------------------------------------------------------------
                align 2
jpt_1C9BD       dw offset loc_1C9C2     ; DATA XREF: ovl_2MISC:C9BD↑r
                                        ; jump table for switch statement
                dw offset loc_1C9CA     ; jumptable 0001C9BD case 1
                dw offset loc_1C9D2     ; jumptable 0001C9BD cases 2,5
                dw offset loc_1C9DA     ; jumptable 0001C9BD case 3
                dw offset loc_1C9E2     ; jumptable 0001C9BD cases 4,6
                dw offset loc_1C9D2     ; jumptable 0001C9BD cases 2,5
                dw offset loc_1C9E2     ; jumptable 0001C9BD cases 4,6
; ---------------------------------------------------------------------------

def_1C9BD:                              ; CODE XREF: ovl_2MISC:C9B8↑j
                                        ; ovl_2MISC:C9C7↑j ...
                mov     ax, [bp-2]      ; jumptable 0001C9BD default case
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CA00:                              ; CODE XREF: sub_1C338+14↑p
                                        ; sub_1C390+BE↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     byte_1DBEC, 0
                jnz     short loc_1CA14
                mov     word ptr [bp-2], 0
                jmp     short loc_1CA4B
; ---------------------------------------------------------------------------

loc_1CA14:                              ; CODE XREF: ovl_2MISC:CA0B↑j
                cmp     byte_1DBEC, 3
                jnz     short loc_1CA22
                mov     word ptr [bp-2], 1
                jmp     short loc_1CA4B
; ---------------------------------------------------------------------------

loc_1CA22:                              ; CODE XREF: ovl_2MISC:CA19↑j
                cmp     byte_1DBEC, 1
                jnz     short loc_1CA30
                mov     word ptr [bp-2], 2
                jmp     short loc_1CA4B
; ---------------------------------------------------------------------------

loc_1CA30:                              ; CODE XREF: ovl_2MISC:CA27↑j
                cmp     byte_1DBEC, 6
                jz      short loc_1CA3E
                cmp     byte_1DBEC, 4
                jnz     short loc_1CA46

loc_1CA3E:                              ; CODE XREF: ovl_2MISC:CA35↑j
                mov     word ptr [bp-2], 3
                jmp     short loc_1CA4B
; ---------------------------------------------------------------------------
                align 2

loc_1CA46:                              ; CODE XREF: ovl_2MISC:CA3C↑j
                mov     word ptr [bp-2], 4

loc_1CA4B:                              ; CODE XREF: ovl_2MISC:CA12↑j
                                        ; ovl_2MISC:CA20↑j ...
                mov     ax, [bp-2]
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; "Search...", "The Party Has", "found a:", "Treasure!"
; Attributes: bp-based frame

party_search    proc near               ; CODE XREF: seg002:0A89↑J

var_18          = word ptr -18h
var_16          = word ptr -16h
var_14          = word ptr -14h
var_12          = word ptr -12h
var_10          = byte ptr -10h
var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

; FUNCTION CHUNK AT CE3F SIZE 000000AF BYTES

                push    bp
                mov     bp, sp
                sub     sp, 18h
                push    di
                push    si
                mov     [bp+var_C], 0
                mov     [bp+var_10], 0
                mov     byte ptr [bp+var_2], 0
                mov     byte ptr [bp+var_6], 0
                mov     byte_1DBEB, 0
                cmp     byte_1DC84, 0
                jz      short loc_1CA79
                jmp     loc_1CCEA
; ---------------------------------------------------------------------------

loc_1CA79:                              ; CODE XREF: party_search+22↑j
                mov     ax, word_241AC
                mov     dx, word_241AE
                mov     [bp+var_A], ax
                mov     [bp+var_8], dx
                sub     cx, cx

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                mov     dl, [bp+var_C]

loc_1CA8B:                              ; CODE XREF: party_search+47↓j
                mov     si, cx
                cmp     byte ptr [bp+si+var_A], 0
                jz      short loc_1CA95
                inc     dl

loc_1CA95:                              ; CODE XREF: party_search+3F↑j
                inc     cx
                cmp     cx, 3
                jl      short loc_1CA8B
                mov     [bp+var_C], dl
                mov     [bp+var_E], cx
                mov     ax, word_241AA
                mov     [bp+var_A], ax
                cmp     byte ptr [bp+var_A], 0
                jz      short loc_1CAB0
                inc     [bp+var_C]

loc_1CAB0:                              ; CODE XREF: party_search+59↑j
                cmp     byte ptr [bp+var_A+1], 0
                jz      short loc_1CAB9
                inc     [bp+var_C]

loc_1CAB9:                              ; CODE XREF: party_search+62↑j
                cmp     [bp+var_C], 0
                jnz     short loc_1CAC2
                inc     [bp+var_C]

loc_1CAC2:                              ; CODE XREF: party_search+6B↑j
                sub     si, si
                mov     cl, [bp+var_10]

loc_1CAC7:                              ; CODE XREF: party_search+88↓j
                mov     al, [si+6953h]
                and     al, 3Fh
                mov     byte ptr [bp+var_2], al
                cmp     al, cl
                jbe     short loc_1CAD6
                mov     cl, al

loc_1CAD6:                              ; CODE XREF: party_search+80↑j
                inc     si
                cmp     si, 3
                jl      short loc_1CAC7
                mov     [bp+var_E], si
                mov     [bp+var_10], cl
                cmp     cl, 1
                jbe     short loc_1CAEA
                inc     [bp+var_C]

loc_1CAEA:                              ; CODE XREF: party_search+93↑j
                mov     al, [bp+var_10]
                sub     ah, ah
                shr     ax, 1
                shr     ax, 1
                add     [bp+var_C], al
                cmp     [bp+var_C], 8
                jbe     short loc_1CB00
                mov     [bp+var_C], 8

loc_1CB00:                              ; CODE XREF: party_search+A8↑j
                mov     al, [bp+var_C]
                mov     byte ptr [bp+var_6], al
                cmp     al, 4
                jnb     short loc_1CB28
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                cmp     al, 1Eh
                jb      short loc_1CB24
                mov     byte ptr [bp+var_6], al
                jmp     short loc_1CB28
; ---------------------------------------------------------------------------

loc_1CB24:                              ; CODE XREF: party_search+CB↑j
                mov     byte ptr [bp+var_6], 0

loc_1CB28:                              ; CODE XREF: party_search+B6↑j
                                        ; party_search+D0↑j
                dec     [bp+var_C]
                mov     ax, 0Fh
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 3
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_window_create
                add     sp, 8           ; CODE XREF: seg002:0A65↑J
                mov     [bp+var_12], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 1
                mov     al, byte_1DB92
                mov     [bx+7], al
                call    loc_1C9AA
                push    ax
                call    thk_monster_gfx_load
                add     sp, 2
                call    thk_res_421E
                push    [bp+var_12]
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_frame_alt
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                sub     ax, ax
                push    ax
                call    thk_monster_gfx_draw ; CODE XREF: seg002:0AF5↑J
                add     sp, 6
                mov     byte_1DBEB, 1
                mov     ax, 1
                push    ax
                push    ax
                call    thk_text_goto_xy

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                add     sp, 4
                mov     ax, offset aSearch ; "Search..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 3
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aThePartyHas ; "The Party Has"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 4
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aFoundA ; "found a:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 6

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                call    loc_1CA00
                mov     bx, ax
                mov     cl, 4
                shl     bx, cl
                mov     al, [bp+var_C]
                sub     ah, ah
                shl     ax, 1
                add     bx, ax
                push    word ptr [bx+28A2h]
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, 2A36h

loc_1CC2D:                              ; CODE XREF: party_search+1F8↓j
                lea     ax, [si+8]
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
                jl      short loc_1CC2D
                mov     [bp+var_E], si
                mov     ax, 5
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, g_main_text_win
                mov     g_cur_text_win, ax
                mov     al, byte ptr [bp+var_6]
                sub     ah, ah
                mov     [bp+var_14], ax
                mov     [bp+var_16], ax
                mov     [bp+var_18], ax
                mov     si, [bp+var_4]

loc_1CC70:                              ; CODE XREF: party_search+26E↓j
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     di, ax
                cmp     ax, 31h ; '1'
                jnz     short loc_1CC94
                push    [bp+var_18]
                call    sub_1C824

loc_1CC8D:                              ; CODE XREF: party_search+24F↓j
                                        ; party_search+25F↓j
                add     sp, 2
                mov     si, ax
                jmp     short loc_1CCBE
; ---------------------------------------------------------------------------

loc_1CC94:                              ; CODE XREF: party_search+233↑j
                mov     ax, di
                cmp     ax, 32h ; '2'
                jnz     short loc_1CCA4
                push    [bp+var_16]
                call    loc_1C8AE
                jmp     short loc_1CC8D
; ---------------------------------------------------------------------------
                align 2

loc_1CCA4:                              ; CODE XREF: party_search+247↑j
                mov     ax, di
                cmp     ax, 33h ; '3'
                jnz     short loc_1CCB4
                push    [bp+var_14]
                call    loc_1C91A
                jmp     short loc_1CC8D
; ---------------------------------------------------------------------------
                align 2

loc_1CCB4:                              ; CODE XREF: party_search+257↑j
                mov     ax, di
                cmp     ax, 34h ; '4'
                jnz     short loc_1CCBE ; CODE XREF: seg002:08FD↑J
                mov     si, 1

loc_1CCBE:                              ; CODE XREF: party_search+240↑j
                                        ; party_search+267↑j
                or      si, si
                jz      short loc_1CC70
                mov     [bp+var_E], di
                mov     [bp+var_4], si
                mov     ax, [bp+var_12]
                mov     g_cur_text_win, ax
                push    ax
                call    thk_text_window_close
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     byte_1DC80, 7
                jmp     short loc_1CD28
; ---------------------------------------------------------------------------

loc_1CCEA:                              ; CODE XREF: party_search+24↑j
                mov     ax, offset aTreasure ; "Treasure!"
                push    ax
                call    thk_res_410A
                add     sp, 2
                cmp     byte_1DC84, 0FFh
                jz      short loc_1CD0B
                mov     al, byte_1DC84
                mov     byte_241A0, al
                mov     byte_241A6, 0
                mov     byte_241A3, 0

loc_1CD0B:                              ; CODE XREF: party_search+2A7↑j
                mov     byte_1DC84, 0
                call    treasure_share
                call    thk_res_5426
                mov     ax, 20h ; ' '
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                mov     byte_1DC80, 3
                call    thk_res_35A8

loc_1CD28:                              ; CODE XREF: party_search+296↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CD32:                              ; CODE XREF: party_rest:loc_1CFF0↓p
                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                mov     [bp+var_2], ax
                cmp     g_party_size, ax
                jle     short loc_1CD77
                mov     di, 416h
                mov     si, ax

loc_1CD50:                              ; CODE XREF: party_search+320↓j
                cmp     word ptr [di], 18h
                jl      short loc_1CD6A
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     ax, [bx+66h]
                mov     dx, [bx+68h]
                add     [bp+var_6], ax
                adc     [bp+var_4], dx

loc_1CD6A:                              ; CODE XREF: party_search+301↑j
                add     di, 2
                inc     si
                cmp     si, g_party_size
                jl      short loc_1CD50
                mov     [bp+var_2], si

loc_1CD77:                              ; CODE XREF: party_search+2F7↑j
                push    [bp+var_4]
                push    [bp+var_6]
                call    thk_party_pay_gold
                add     sp, 4
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CD8A:                              ; CODE XREF: party_rest+7F↓p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     al, al
                mov     byte_1DC2A, al
                mov     byte_1DC29, al
                mov     byte_1DC28, al
                mov     byte_1DC27, al
                mov     byte_1DC26, al
                mov     byte_1DC25, al
                mov     byte_1DC31, al
                mov     byte_1DC30, al
                mov     byte_1DC2B, al
                mov     byte_1DC2F, al
                mov     byte_1DC2E, al
                mov     byte_1DC2D, al
                mov     byte_1DC2C, al
                sub     di, di
                mov     si, [bp+var_4]
                jmp     short loc_1CE40
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1CDC4:                              ; CODE XREF: party_search+435↓j
                mov     ax, [si+60h]
                mov     [si+74h], ax

loc_1CDCA:                              ; CODE XREF: party_search+43B↓j
                cmp     byte ptr [si+25h], 0
                jz      short loc_1CE3F
                dec     byte ptr [si+25h]
                test    byte ptr [si+26h], 4
                jnz     short loc_1CDDF
                mov     ax, [si+74h]
                mov     [si+5Eh], ax

loc_1CDDF:                              ; CODE XREF: party_search+385↑j
                cmp     byte ptr [si+23h], 0
                jz      short loc_1CE26
                mov     al, [si+12h]
                mov     byte ptr [bp+var_2], al
                cmp     byte ptr [si+0Fh], 4
                jz      short loc_1CDF7
                cmp     byte ptr [si+0Fh], 2
                jnz     short loc_1CDFD

loc_1CDF7:                              ; CODE XREF: party_search+39D↑j
                mov     al, [si+11h]
                mov     byte ptr [bp+var_2], al

loc_1CDFD:                              ; CODE XREF: party_search+3A3↑j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                mov     byte ptr [bp+var_2], al
                cmp     al, 0F2h
                jb      short loc_1CE14
                mov     byte ptr [bp+var_2], 0

loc_1CE14:                              ; CODE XREF: party_search+3BC↑j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     cx, ax
                add     cx, 3
                mov     al, [si+20h]
                mul     cx
                mov     [si+5Ah], ax

loc_1CE26:                              ; CODE XREF: party_search+391↑j
                push    si
                call    thk_char_reset_current_stats
                add     sp, 2
                mov     al, [si+27h]
party_search    endp


loc_1CE30:                              ; CODE XREF: seg002:0831↑J
                mov     [si+73h], al
                mov     al, [si+15h]
                mov     [si+70h], al
                mov     ax, [si+5Ah]
                mov     [si+58h], ax
; START OF FUNCTION CHUNK FOR party_search

loc_1CE3F:                              ; CODE XREF: party_search+37C↑j
                                        ; party_search+401↓j
                inc     di

loc_1CE40:                              ; CODE XREF: party_search+36E↑j
                cmp     di, g_party_size
                jge     short loc_1CE90
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                cmp     byte ptr [si+26h], 80h
                jnb     short loc_1CE3F
                and     byte ptr [si+26h], 0Dh
                cmp     byte ptr [si+21h], 50h ; 'P'
                jb      short loc_1CE76
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 32h ; '2'
                jge     short loc_1CE76
                mov     byte ptr [si+26h], 81h

loc_1CE76:                              ; CODE XREF: party_search+40B↑j
                                        ; party_search+41E↑j
                cmp     word ptr [si+5Eh], 0
                jnz     short loc_1CE81
                mov     word ptr [si+5Eh], 1

loc_1CE81:                              ; CODE XREF: party_search+428↑j
                test    byte ptr [si+26h], 8
                jnz     short loc_1CE8A
                jmp     loc_1CDC4
; ---------------------------------------------------------------------------

loc_1CE8A:                              ; CODE XREF: party_search+433↑j
                shr     word ptr [si+74h], 1
                jmp     loc_1CDCA
; ---------------------------------------------------------------------------

loc_1CE90:                              ; CODE XREF: party_search+3F2↑j
                mov     [bp+var_6], di
                mov     [bp+var_4], si
                call    thk_res_4F3A
                mov     ax, 55h ; 'U'
                push    ax
                call    thk_advance_time
                add     sp, 2
                cmp     g_era, 9
                jz      short loc_1CEDB
                mov     ax, 3Ch ; '<'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 0Ah
                jge     short loc_1CEDB
                mov     g_era, 9
                mov     ax, 0FFh
                push    ax
                push    ax

loc_1CEC8:                              ; CODE XREF: seg002:07AD↑J
                sub     ax, ax
                push    ax
                call    thk_2PLAY_B5EA
                add     sp, 6
                mov     byte_1DC7E, 1
                mov     byte_1DBEB, 1

loc_1CEDB:                              ; CODE XREF: party_search+456↑j
                                        ; party_search+469↑j
                call    thk_draw_party_list
                mov     ax, 2A3Eh
                push    ax
                call    thk_res_410A
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR party_search

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CEEE       proc near               ; CODE XREF: party_rest:loc_1CFFC↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     [bp+var_6], 0
                mov     al, byte_1DC64
                sub     ah, ah
                mov     [bp+var_4], ax
                mov     byte_1DC64, ah
                cmp     byte_23218, 80h
                jnb     short loc_1CF7A
                cmp     byte_1DC2A, ah
                jnz     short loc_1CF7A
                or      ax, ax
                jnz     short loc_1CF7A
                mov     ax, 32h ; '2'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 2
                jnz     short loc_1CF7A
                sub     si, si          ; CODE XREF: seg002:050D↑J
                mov     di, [bp+var_2]
                jmp     short loc_1CF46
; ---------------------------------------------------------------------------

loc_1CF32:                              ; CODE XREF: sub_1CEEE+5C↓j
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                cmp     byte ptr [di+26h], 80h
                jnb     short loc_1CF45
                or      byte ptr [di+26h], 10h

loc_1CF45:                              ; CODE XREF: sub_1CEEE+51↑j
                inc     si

loc_1CF46:                              ; CODE XREF: sub_1CEEE+42↑j
                cmp     si, g_party_size
                jl      short loc_1CF32
                mov     [bp+var_2], di
                mov     [bp+var_6], si
                mov     byte_1DC65, 3
                mov     byte_1DD58, 0

loc_1CF5C:                              ; CODE XREF: seg002:0AE9↑J
                mov     [bp+var_6], 0
                sub     ax, ax
                mov     cx, 5
                mov     di, 9680h
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                stosb
                add     [bp+var_6], 0Bh
                call    thk_start_combat
                mov     [bp+var_6], 1

loc_1CF7A:                              ; CODE XREF: sub_1CEEE+1E↑j
                                        ; sub_1CEEE+24↑j ...
                mov     ax, [bp+var_6]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CEEE       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; 'R' Rest
; Attributes: bp-based frame

party_rest      proc near               ; CODE XREF: seg002:062D↑J

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte_1DC80, 1
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                test    byte_23218, 8
                jz      short loc_1CFAC
                mov     ax, offset aTooDangerous ; "Too dangerous!"

loc_1CFA3:                              ; CODE XREF: party_rest+76↓j
                push    ax
                call    thk_res_410A
                add     sp, 2
                jmp     short loc_1D006
; ---------------------------------------------------------------------------

loc_1CFAC:                              ; CODE XREF: party_rest+1A↑j
                mov     ax, offset aRestHereYN ; "Rest here? (Y/N)"
                push    ax
                call    thk_res_410A
                add     sp, 2
                mov     si, [bp+var_2]

loc_1CFB9:                              ; CODE XREF: party_rest+58↓j
                cmp     g_outdoors, 0
                jnz     short loc_1CFC6
                call    thk_2PLAY_8282
                jmp     short loc_1CFC9
; ---------------------------------------------------------------------------
                align 2

loc_1CFC6:                              ; CODE XREF: party_rest+3A↑j
                call    thk_kbd_poll

loc_1CFC9:                              ; CODE XREF: party_rest+3F↑j
                mov     si, ax
                push    si
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jz      short loc_1CFDE
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_1CFB9

loc_1CFDE:                              ; CODE XREF: party_rest+53↑j
                mov     [bp+var_2], si
                cmp     si, 4Eh ; 'N'
                jnz     short loc_1CFF0
                call    thk_res_421E
                mov     byte_1DC80, 0
                jmp     short loc_1D006
; ---------------------------------------------------------------------------

loc_1CFF0:                              ; CODE XREF: party_rest+60↑j
                call    loc_1CD32
                or      ax, ax
                jnz     short loc_1CFFC
                mov     ax, offset aNotEnoughGoldD ; "Not enough gold - Dismiss hirelings"
                jmp     short loc_1CFA3
; ---------------------------------------------------------------------------

loc_1CFFC:                              ; CODE XREF: party_rest+71↑j
                call    sub_1CEEE
                or      ax, ax
                jnz     short loc_1D006
                call    loc_1CD8A

loc_1D006:                              ; CODE XREF: party_rest+26↑j
                                        ; party_rest+6A↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
party_rest      endp

; ---------------------------------------------------------------------------
                align 8
ovl_2MISC       ends

