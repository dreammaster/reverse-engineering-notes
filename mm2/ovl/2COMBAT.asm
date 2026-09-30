; ===========================================================================

; Segment type: Pure code
ovl_2COMBAT     segment byte public 'CODE' use16
                assume cs:ovl_2COMBAT
                ;org 7E10h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; halves damage for protection effects, then char_apply_damage
; Attributes: bp-based frame

combat_damage_character proc near       ; CODE XREF: seg002:01C5↑J
                                        ; combat_monster_hits+8E↓p ...

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                cmp     byte_1DC36, 0
                jz      short loc_17E1E
                shr     word_27824, 1

loc_17E1E:                              ; CODE XREF: combat_damage_character+8↑j
                cmp     byte_22CF4, 0
                jnz     short loc_17E30
                cmp     byte_1DC35, 0
                jz      short loc_17E30
                shr     word_27824, 1

loc_17E30:                              ; CODE XREF: combat_damage_character+13↑j
                                        ; combat_damage_character+1A↑j
                push    [bp+arg_4]
                push    [bp+arg_2]
                push    [bp+arg_0]
                push    word_27824
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    thk_char_apply_damage
                add     sp, 0Ah
                pop     bp
                retn
combat_damage_character endp


; =============== S U B R O U T I N E =======================================

; " goes down!", possible touch effect of the monster
; Attributes: bp-based frame

combat_after_hit proc near              ; CODE XREF: combat_monster_hits+94↓p
                                        ; combat_spell_damage+8F↓p

var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_2], 0
                mov     ax, 11h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 40h ; '@'
                jb      short loc_17EBD
                push    ax
                call    thk_res_3E40
                add     sp, 2
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_17E96
                mov     ax, 1112h
                jmp     short loc_17E99
; ---------------------------------------------------------------------------

loc_17E96:                              ; CODE XREF: combat_after_hit+3D↑j
                mov     ax, offset aGoesDown ; " goes down!"

loc_17E99:                              ; CODE XREF: combat_after_hit+42↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                inc     [bp+var_2]
                mov     al, byte ptr word_1DD58
                cmp     byte_22CED, al
                jnb     short loc_17EB0
                inc     byte_22CED

loc_17EB0:                              ; CODE XREF: combat_after_hit+58↑j
                call    combat_draw_party_hp
                mov     ax, 6
                push    ax
                call    thk_play_sound_effect
                add     sp, 2

loc_17EBD:                              ; CODE XREF: combat_after_hit+2D↑j
                cmp     byte_27821, 0
                jz      short loc_17EC7
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17EC7:                              ; CODE XREF: combat_after_hit+70↑j
                cmp     byte_27677, 0
                jnz     short loc_17ED1
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17ED1:                              ; CODE XREF: combat_after_hit+7A↑j
                cmp     byte_22CF4, 0
                jz      short loc_17EDB
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17EDB:                              ; CODE XREF: combat_after_hit+84↑j
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_17EE7
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17EE7:                              ; CODE XREF: combat_after_hit+90↑j
                mov     bl, byte_27820
                sub     bh, bh
                mov     al, [bx-6980h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     [bp+var_6], al
                mov     bl, al
                mov     al, [bx+104Ah]
                add     [bp+var_6], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     al, [bp+var_6]
                jbe     short loc_17F19
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17F19:                              ; CODE XREF: combat_after_hit+C2↑j
                cmp     byte_27678, 0
                jz      short loc_17F23
                jmp     loc_17FA7
; ---------------------------------------------------------------------------

loc_17F23:                              ; CODE XREF: combat_after_hit+CC↑j
                push    [bp+var_4]
                call    thk_res_38A8
                add     sp, 2
                or      ax, ax
                jnz     short loc_17FA7
                cmp     byte_27677, 20h ; ' '
                jnb     short loc_17FA7
                mov     al, byte_27677
                mov     [bp+var_6], al
                cmp     al, 1Fh
                jnz     short loc_17F52
                mov     ax, 1Eh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_6], al

loc_17F52:                              ; CODE XREF: combat_after_hit+ED↑j
                mov     al, [bp+var_6]
                mov     byte_27677, al
                push    [bp+var_4]
                call    combat_apply_touch_effect
                add     sp, 2
                or      ax, ax
                jz      short loc_17F9A
                cmp     [bp+var_2], 0
                jnz     short loc_17F9A
                push    [bp+var_4]
                call    thk_res_3E40
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bl, byte_27677
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+106Ch]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 21h ; '!'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_17F9A:                              ; CODE XREF: combat_after_hit+111↑j
                                        ; combat_after_hit+117↑j
                call    combat_draw_party_hp
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2

loc_17FA7:                              ; CODE XREF: combat_after_hit+72↑j
                                        ; combat_after_hit+7C↑j ...
                call    combat_draw_party_hp
                call    sub_1A7D8
                mov     sp, bp
                pop     bp
                retn
combat_after_hit endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "<monster> shoots/attacks <character>"
; Attributes: bp-based frame

combat_monster_hits proc near           ; CODE XREF: combat_monster_advances:loc_18452↓p

var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                mov     [bp+var_2], 0
                mov     [bp+var_4], 0
                mov     [bp+var_6], 0FFh
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_8], ax
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                call    thk_res_3E76
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     byte_22CF4, 1
                jnz     short loc_17FF6
                mov     ax, offset aShoots ; "shoots"
                push    ax
                jmp     short loc_1800C
; ---------------------------------------------------------------------------

loc_17FF6:                              ; CODE XREF: combat_monster_hits+3C↑j
                mov     ax, 8
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     bx, ax
                shl     bx, 1
                push    word ptr [bx+1058h]

loc_1800C:                              ; CODE XREF: combat_monster_hits+42↑j
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_8]
                call    thk_text_puts
                add     sp, 2
                call    combat_attack_summary_text
                cmp     word_27824, 0
                jz      short loc_1804E
                mov     byte_27821, 0
                lea     ax, [bp+var_6]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                call    combat_damage_character
                add     sp, 6
                call    combat_after_hit
                call    combat_draw_party_hp
                jmp     short loc_18051
; ---------------------------------------------------------------------------

loc_1804E:                              ; CODE XREF: combat_monster_hits+7B↑j
                call    sub_1A7D8

loc_18051:                              ; CODE XREF: combat_monster_hits+9A↑j
                mov     sp, bp
                pop     bp
                retn
combat_monster_hits endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; " casts"
; Attributes: bp-based frame

combat_monster_casts proc near          ; CODE XREF: ovl_2COMBAT:loc_1864E↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     al, byte_27674
                mov     [bp+var_2], al
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                call    thk_res_3E76
                cmp     [bp+var_2], 0Fh
                jb      short loc_1808B
                cmp     [bp+var_2], 1Eh
                ja      short loc_1808B
                cmp     [bp+var_2], 1Dh
                jz      short loc_1808B
                mov     ax, offset aCasts ; " casts"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1808B:                              ; CODE XREF: combat_monster_casts+1D↑j
                                        ; combat_monster_casts+23↑j ...
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                mov     bx, si
                shl     bx, 1
                push    word ptr [bx+10AAh]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 21h ; '!'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     word_27824, 0
                push    si
                call    near ptr byte_1B5EA+122h
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_monster_casts endp


; =============== S U B R O U T I N E =======================================

; " waits for opening!" / " adds friends!"
; Attributes: bp-based frame

combat_monster_waits proc near          ; CODE XREF: combat_monster_advances+13A↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     [bp+var_2], 0
                mov     al, byte_27820
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                cmp     byte_22CE8, 0
                jnz     short loc_18128
                cmp     byte_2768F, 0
                jz      short loc_18128
                cmp     byte ptr word_1DD58, 6Eh ; 'n'
                jnb     short loc_18128
                cmp     byte ptr word_1DD58, 0Ah
                jbe     short loc_18128
                mov     bl, byte_27820
                sub     bh, bh
                mov     al, byte_26EDA
                cmp     [bx-6980h], al
                jnz     short loc_18128
                mov     al, byte ptr word_1DD58
                sub     al, 0Ah
                add     byte ptr word_1DD58, al
                inc     byte_22CE8
                inc     [bp+var_2]

loc_18128:                              ; CODE XREF: combat_monster_waits+2C↑j
                                        ; combat_monster_waits+33↑j ...
                call    thk_res_3E76
                mov     bl, [bp+var_2]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+106Ah]
                call    thk_text_puts
                add     sp, 2
                cmp     [bp+var_2], 0
                jz      short loc_18145
                call    combat_draw_monster_list

loc_18145:                              ; CODE XREF: combat_monster_waits+7A↑j
                mov     sp, bp
                pop     bp
                retn
combat_monster_waits endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; " advances!" (swap with the front rank)
; Attributes: bp-based frame

combat_monster_advances proc near       ; CODE XREF: ovl_2COMBAT:loc_1861C↓p

var_14          = word ptr -14h
var_12          = word ptr -12h
var_10          = byte ptr -10h
var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 14h
                push    di
                push    si
                mov     [bp+var_A], 0
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                cmp     byte_27685, 0
                jnz     short loc_18169
                jmp     loc_1827E
; ---------------------------------------------------------------------------

loc_18169:                              ; CODE XREF: combat_monster_advances+1A↑j
                mov     [bp+var_C], 0
                jmp     short loc_18173
; ---------------------------------------------------------------------------
                align 2

loc_18170:                              ; CODE XREF: combat_monster_advances+131↓j
                inc     [bp+var_C]

loc_18173:                              ; CODE XREF: combat_monster_advances+23↑j
                mov     al, byte_27815
                cmp     [bp+var_C], al
                jb      short loc_1817E
                jmp     loc_1827E
; ---------------------------------------------------------------------------

loc_1817E:                              ; CODE XREF: combat_monster_advances+2F↑j
                mov     al, byte_27820
                cmp     [bp+var_C], al
                jnz     short loc_18189
                jmp     loc_18275
; ---------------------------------------------------------------------------

loc_18189:                              ; CODE XREF: combat_monster_advances+3A↑j
                mov     al, [bp+var_C]
                sub     ah, ah
                mov     si, ax
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                cmp     byte_27685, 0
                jz      short loc_181A1
                jmp     loc_18275
; ---------------------------------------------------------------------------

loc_181A1:                              ; CODE XREF: combat_monster_advances+52↑j
                inc     [bp+var_A]
                mov     di, si
                shl     di, 1
                add     di, 9FAAh
                mov     ax, [di]
                mov     [bp+var_E], ax
                mov     al, [si-607Ah]
                mov     [bp+var_6], al
                mov     al, [si-606Eh]
                mov     [bp+var_8], al
                mov     al, [si-6062h]
                mov     byte ptr [bp+var_4], al
                mov     al, [si+5480h]
                mov     [bp+var_10], al
                mov     al, [si-6980h]
                mov     byte ptr [bp+var_2], al
                mov     al, byte_27820
                sub     ah, ah
                mov     [bp+var_12], ax
                shl     ax, 1
                add     ax, 9FAAh
                mov     [bp+var_14], ax
                mov     bx, ax
                mov     ax, [bx]
                mov     [di], ax
                mov     bx, [bp+var_12]
                mov     al, [bx-607Ah]
                mov     [si-607Ah], al  ; CODE XREF: seg002:01B9↑J
                mov     al, [bx-606Eh]
                mov     [si-606Eh], al
                mov     al, [bx-6062h]
                mov     [si-6062h], al
                mov     al, [bx+5480h]
                mov     [si+5480h], al
                mov     al, [bx-6980h]
                mov     [si-6980h], al
                mov     bx, [bp+var_14]
                mov     ax, [bp+var_E]
                mov     [bx], ax
                mov     bx, [bp+var_12]
                mov     al, [bp+var_6]
                mov     [bx-607Ah], al
                mov     al, [bp+var_8]
                mov     [bx-606Eh], al
                mov     al, byte ptr [bp+var_4]
                mov     [bx-6062h], al
                mov     al, [bp+var_10]
                mov     [bx+5480h], al
                mov     al, byte ptr [bp+var_2]
                mov     [bx-6980h], al
                mov     al, [bp+var_C]
                mov     byte_27820, al
                push    si
                call    combat_monster_show_line
                add     sp, 2
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     al, byte_27820
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                call    thk_res_3E76
                mov     ax, offset aAdvances ; " advances!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                call    combat_draw_monster_list

loc_18275:                              ; CODE XREF: combat_monster_advances+3C↑j
                                        ; combat_monster_advances+54↑j
                cmp     [bp+var_A], 0
                jnz     short loc_1827E
                jmp     loc_18170
; ---------------------------------------------------------------------------

loc_1827E:                              ; CODE XREF: combat_monster_advances+1C↑j
                                        ; combat_monster_advances+31↑j ...
                cmp     [bp+var_A], 0

loc_18282:                              ; CODE XREF: seg002:0609↑J
                jnz     short loc_18287
                call    combat_monster_waits

loc_18287:                              ; CODE XREF: combat_monster_advances:loc_18282↑j
                call    sub_1A7D8
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_pick_random_target:              ; CODE XREF: combat_monster_advances+315↓p
                                        ; ovl_2COMBAT:B2CE↓p
                push    bp              ; random living character
                mov     bp, sp
                sub     sp, 4
                push    g_party_size
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                dec     byte ptr [bp+var_2]

loc_182AA:                              ; CODE XREF: combat_monster_advances+1AB↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_182E0
                inc     byte ptr [bp+var_2]
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     al, byte ptr [bp+var_2]
                cmp     byte ptr g_party_size, al
                ja      short loc_182E0
                mov     byte ptr [bp+var_2], 0

loc_182E0:                              ; CODE XREF: combat_monster_advances+175↑j
                                        ; combat_monster_advances+190↑j
                mov     al, byte_2781F
                cmp     byte ptr g_party_size, al
                ja      short loc_182EE
                mov     byte_2781F, 0

loc_182EE:                              ; CODE XREF: combat_monster_advances+19D↑j
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_182AA
                mov     al, byte ptr [bp+var_2]
                mov     byte_2781F, al
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_next_front_rank_target:          ; CODE XREF: combat_monster_advances+32C↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     byte ptr [bp+var_4], 0

loc_1830C:                              ; CODE XREF: combat_monster_advances+218↓j
                mov     al, byte_22CED
                cmp     byte ptr [bp+var_4], al
                jb      short loc_1832A
                inc     byte_22CED
                mov     al, byte_22CED
                cmp     byte ptr g_party_size, al
                jnb     short loc_1832E
                mov     al, byte ptr g_party_size
                mov     byte_22CED, al
                jmp     short loc_1832E
; ---------------------------------------------------------------------------
                align 2

loc_1832A:                              ; CODE XREF: combat_monster_advances+1C8↑j
                inc     byte_2781F

loc_1832E:                              ; CODE XREF: combat_monster_advances+1D5↑j
                                        ; combat_monster_advances+1DD↑j
                mov     al, byte_22CED
                cmp     byte_2781F, al
                jb      short loc_1833C
                mov     byte_2781F, 0

loc_1833C:                              ; CODE XREF: combat_monster_advances+1EB↑j
                mov     al, byte_2781F
                cmp     byte ptr g_party_size, al
                ja      short loc_1834A
                mov     byte_2781F, 0

loc_1834A:                              ; CODE XREF: combat_monster_advances+1F9↑j
                inc     byte ptr [bp+var_4]
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                test    byte ptr [bx+26h], 0C0h
                jnz     short loc_1830C
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_spell_failed:                    ; CODE XREF: ovl_2COMBAT:loc_18648↓p
                sub     ax, ax          ; "*** Spell Failed ***"
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpellFailed ; "*** Spell Failed ***"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                call    sub_1A7D8
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_melee:                   ; CODE XREF: combat_monster_advances+31D↓p
                                        ; combat_monster_advances+32F↓p
                push    bp              ; monster attacks with record[14]+1 blows
                mov     bp, sp
                sub     sp, 0Ah
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     word_27824, 0
                mov     byte_22CEA, 0
                mov     al, byte_2767E
                mov     byte_22CEC, al
                mov     bl, byte_27820
                sub     bh, bh
                mov     bl, [bx-6980h]
                mov     cl, 4
                shr     bx, cl
                mov     al, [bx+103Ah]
                mov     byte ptr [bp+var_4], al
                mov     bx, [bp+var_2]
                cmp     [bx+24h], al
                jbe     short loc_183E2
                mov     [bp+var_6], 5
                jmp     short loc_183EB
; ---------------------------------------------------------------------------
                align 2

loc_183E2:                              ; CODE XREF: combat_monster_advances+28F↑j
                mov     al, byte ptr [bp+var_4]
                sub     al, [bx+24h]
                mov     [bp+var_6], al

loc_183EB:                              ; CODE XREF: combat_monster_advances+295↑j
                mov     bl, byte_27820
                sub     bh, bh
                test    byte ptr [bx-607Ah], 8
                jz      short loc_183FB
                shr     [bp+var_6], 1

loc_183FB:                              ; CODE XREF: combat_monster_advances+2AC↑j
                mov     [bp+var_A], 0
                jmp     short loc_18439
; ---------------------------------------------------------------------------
                align 2

loc_18402:                              ; CODE XREF: combat_monster_advances+2F5↓j
                mov     ax, 3F1h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                cwd
                mov     cx, 0Ah
                idiv    cx
                mov     [bp+var_8], al
                cmp     [bp+var_6], al
                jb      short loc_18436
                inc     byte_22CEA
                mov     al, byte_2767D
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     word_27824, ax

loc_18436:                              ; CODE XREF: combat_monster_advances+2D2↑j
                inc     [bp+var_A]

loc_18439:                              ; CODE XREF: combat_monster_advances+2B5↑j
                mov     al, byte_22CEC
                cmp     [bp+var_A], al
                jb      short loc_18402
                mov     bl, byte_27820
                sub     bh, bh
                test    byte ptr [bx-607Ah], 4
                jz      short loc_18452
                shr     word_27824, 1

loc_18452:                              ; CODE XREF: combat_monster_advances+302↑j
                call    combat_monster_hits
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_ranged_attack:           ; CODE XREF: ovl_2COMBAT:8616↓p
                mov     byte_22CF4, 1
                call    combat_pick_random_target
                mov     byte ptr word_27812+1, 0
                call    combat_monster_melee
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_melee_attack:            ; CODE XREF: ovl_2COMBAT:85F7↓p
                mov     byte_22CF4, 0
                mov     byte ptr word_27812+1, 0
                call    combat_next_front_rank_target
                call    combat_monster_melee
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_spell_roll:              ; CODE XREF: ovl_2COMBAT:85E4↓p
                push    bp              ; uses left, silenced flag, cast chance
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     byte ptr [bp+var_4], 0
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                mov     al, byte_27820
                sub     ah, ah
                mov     si, ax
                test    byte ptr [si-607Ah], 40h
                jnz     short loc_184B9
                cmp     [si-6062h], ah
                jz      short loc_184B9
                mov     al, byte_27675
                cmp     byte ptr [bp+var_2], al
                ja      short loc_184B9
                inc     byte ptr [bp+var_4]

loc_184B9:                              ; CODE XREF: combat_monster_advances+35C↑j
                                        ; combat_monster_advances+362↑j ...
                cmp     byte ptr [bp+var_4], 0
                jz      short loc_184C9
                mov     bl, byte_27820
                sub     bh, bh
                dec     byte ptr [bx-6062h]

loc_184C9:                              ; CODE XREF: combat_monster_advances+373↑j
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_show_line:               ; CODE XREF: combat_monster_advances+100↑p
                                        ; ovl_2COMBAT:85DE↓p
                push    bp
                mov     bp, sp
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    combat_draw_monster_line
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     byte_2781C, 1
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_monster_turn:                    ; CODE XREF: combat_party_turn+EA7↓p
                push    bp              ; one monster's action
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     [bp+var_8], 0
                mov     byte ptr [bp+var_4], 1
                mov     al, byte_27820
combat_monster_advances endp


loc_18510:                              ; CODE XREF: seg002:0771↑J
                mov     byte_2781E, al
                sub     ah, ah
                mov     si, ax
                inc     byte ptr [si+5480h]
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     byte_27821, 0
                mov     bl, byte_27820
                sub     bh, bh
                cmp     byte ptr [bx-607Ah], 80h
                jb      short loc_1857C
                mov     byte ptr word_27812+1, 2
                mov     al, byte_27819
                mov     [bp-2], al
                cmp     al, bh
                jz      short loc_1854F
                dec     byte ptr [bp-2]

loc_1854F:                              ; CODE XREF: ovl_2COMBAT:854A↑j
                mov     al, [bp-2]
                sub     ah, ah
                mov     si, ax
                mov     bx, si
                shl     bx, 1
                mov     ax, [bx+102Ah]
                mov     word_27816, ax
                mov     al, [si+1032h]
                mov     [bp-6], al
                sub     ah, ah
                push    ax
                mov     al, byte_27820
                push    ax
                mov     ax, 1
                push    ax
                call    combat_party_spell_hits
                add     sp, 6
                jmp     loc_1866E
; ---------------------------------------------------------------------------

loc_1857C:                              ; CODE XREF: ovl_2COMBAT:853B↑j
                mov     bl, byte_27820
                sub     bh, bh
                test    byte ptr [bx-607Ah], 0B0h
                jz      short loc_1858C
                jmp     loc_18651
; ---------------------------------------------------------------------------

loc_1858C:                              ; CODE XREF: ovl_2COMBAT:8587↑j
                cmp     byte_27814, bh
                jnz     short loc_185BB
                mov     bl, byte_2767A
                mov     al, [bx+1036h]
                mov     [bp-2], al
                mov     al, byte_1E812
                cmp     [bp-2], al
                jnb     short loc_185BB
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 32h ; '2'
                jg      short loc_185BB
                inc     byte ptr [bp-8]

loc_185BB:                              ; CODE XREF: ovl_2COMBAT:8590↑j
                                        ; ovl_2COMBAT:85A3↑j ...
                cmp     byte ptr [bp-8], 0
                jz      short loc_185D8
                mov     byte_22CF6, 1
                call    combat_monster_gone_text
                call    combat_remove_monster
                mov     byte_22CF6, 0
                call    sub_1A7D8
                jmp     short loc_18651
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_185D8:                              ; CODE XREF: ovl_2COMBAT:85BF↑j
                mov     al, byte_27820
                sub     ah, ah
                push    ax
                call    combat_monster_show_line
                add     sp, 2
                call    combat_monster_spell_roll
                mov     [bp-4], al
                or      al, al
                jnz     short loc_18622
                mov     al, byte_27815
                cmp     byte_27820, al
                jnb     short loc_185FC
                call    combat_monster_melee_attack
                jmp     short loc_18651
; ---------------------------------------------------------------------------

loc_185FC:                              ; CODE XREF: ovl_2COMBAT:85F5↑j
                cmp     byte_27682, 0
                jz      short loc_1861C
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 50h ; 'P'
                jg      short loc_1861C
                call    combat_monster_ranged_attack
                jmp     short loc_18651
; ---------------------------------------------------------------------------
                align 2

loc_1861C:                              ; CODE XREF: ovl_2COMBAT:8601↑j
                                        ; ovl_2COMBAT:8614↑j
                call    combat_monster_advances
                jmp     short loc_18651
; ---------------------------------------------------------------------------
                align 2

loc_18622:                              ; CODE XREF: ovl_2COMBAT:85EC↑j
                mov     al, byte_27674
                mov     [bp-6], al
                cmp     al, 0Fh
                jb      short loc_1864E
                cmp     al, 1Dh
                jz      short loc_1864E
                cmp     al, 1Fh
                jnb     short loc_1864E
                mov     bl, byte_27820
                sub     bh, bh
                test    byte ptr [bx-607Ah], 2
                jnz     short loc_18648
                test    byte_23218, 2
                jz      short loc_1864E

loc_18648:                              ; CODE XREF: ovl_2COMBAT:863F↑j
                call    combat_spell_failed
                jmp     short loc_18651
; ---------------------------------------------------------------------------
                align 2

loc_1864E:                              ; CODE XREF: ovl_2COMBAT:862A↑j
                                        ; ovl_2COMBAT:862E↑j ...
                call    combat_monster_casts

loc_18651:                              ; CODE XREF: ovl_2COMBAT:8589↑j
                                        ; ovl_2COMBAT:85D4↑j ...
                mov     byte_22CF4, 0
                cmp     byte_2781C, 0
                jz      short loc_18669
                mov     al, byte_27820
                sub     ah, ah
                push    ax
                call    combat_draw_monster_line
                add     sp, 2

loc_18669:                              ; CODE XREF: ovl_2COMBAT:865B↑j
                mov     byte_2781C, 0

loc_1866E:                              ; CODE XREF: ovl_2COMBAT:8579↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_target_flag proc near            ; CODE XREF: combat_party_spell_hits+BE↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 9E36h
                mov     bl, [bp+arg_0]
                sub     bh, bh
                cmp     [bx-61CAh], bh
                jz      short loc_18690
                mov     ax, 1
                jmp     short loc_18692
; ---------------------------------------------------------------------------
                align 2

loc_18690:                              ; CODE XREF: combat_target_flag+14↑j
                sub     ax, ax

loc_18692:                              ; CODE XREF: combat_target_flag+19↑j
                mov     sp, bp
                pop     bp
                retn
combat_target_flag endp


; =============== S U B R O U T I N E =======================================

; " casts a spell:", " is not affected!", " takes ", " is <status>!"
; Attributes: bp-based frame

combat_party_spell_hits proc near       ; CODE XREF: seg002:0561↑J
                                        ; ovl_2COMBAT:8573↑p

var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6
arg_4           = byte ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     [bp+var_8], 0
                mov     [bp+var_6], 0Ah
                mov     al, byte ptr word_1DD58
                cmp     [bp+arg_0], al
                jbe     short loc_186B0
                mov     [bp+arg_0], al

loc_186B0:                              ; CODE XREF: combat_party_spell_hits+15↑j
                cmp     byte_2781A, 0
                jz      short loc_186BA
                inc     [bp+var_6]

loc_186BA:                              ; CODE XREF: combat_party_spell_hits+1F↑j
                cmp     byte ptr word_27812+1, 2
                jz      short loc_186E0
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                push    word_23626
                call    thk_res_3E40
                add     sp, 2
                mov     ax, offset aCastsASpell ; " casts a spell:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_186E5
; ---------------------------------------------------------------------------

loc_186E0:                              ; CODE XREF: combat_party_spell_hits+29↑j
                mov     byte ptr word_27812+1, 0

loc_186E5:                              ; CODE XREF: combat_party_spell_hits+48↑j
                mov     ax, word_27816
                mov     [bp+var_C], ax
                mov     al, byte ptr word_27812+1
                mov     [bp+var_2], al

loc_186F1:                              ; CODE XREF: combat_party_spell_hits+256↓j
                mov     ax, 1
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     al, [bp+arg_2]
                mov     byte_2781E, al
                mov     [bp+var_4], 0
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                cmp     byte_27810, 0
                jz      short loc_18718
                jmp     loc_187AB
; ---------------------------------------------------------------------------

loc_18718:                              ; CODE XREF: combat_party_spell_hits+7D↑j
                cmp     byte_27681, 0
                jz      short loc_18741
                mov     ax, 5Ah ; 'Z'
                push    ax
                mov     bx, word_23626
                mov     al, [bx+71h]
                sub     ah, ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_A], al
                mov     al, byte_27681
                cmp     [bp+var_A], al
                jnb     short loc_18741
                inc     [bp+var_4]

loc_18741:                              ; CODE XREF: combat_party_spell_hits+87↑j
                                        ; combat_party_spell_hits+A6↑j ...
                cmp     [bp+var_4], 0
                jnz     short loc_1875D
                cmp     [bp+arg_4], 0
                jz      short loc_1875D
                mov     al, [bp+arg_4]
                sub     ah, ah
                dec     ax
                push    ax
                call    combat_target_flag
                add     sp, 2
                mov     [bp+var_4], al

loc_1875D:                              ; CODE XREF: combat_party_spell_hits+AF↑j
                                        ; combat_party_spell_hits+B5↑j
                cmp     [bp+var_4], 0
                jnz     short loc_187AB
                mov     ax, 0BFh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_A], al
                mov     bl, [bp+arg_2]
                sub     bh, bh
                cmp     [bx-6980h], al
                jb      short loc_187AB
                cmp     byte ptr word_27812+1, bh
                jnz     short loc_1878C
                shr     word_27816, 1
                jmp     short loc_187AB
; ---------------------------------------------------------------------------
                align 2

loc_1878C:                              ; CODE XREF: combat_party_spell_hits+ED↑j
                cmp     byte ptr word_27812+1, 1
                jnz     short loc_187AB
                cmp     byte ptr word_27812, 9
                jnb     short loc_187A0
                inc     [bp+var_4]
                jmp     short loc_187AB
; ---------------------------------------------------------------------------
                align 2

loc_187A0:                              ; CODE XREF: combat_party_spell_hits+102↑j
                mov     word_27816, 32h ; '2'
                mov     byte ptr word_27812+1, 0

loc_187AB:                              ; CODE XREF: combat_party_spell_hits+7F↑j
                                        ; combat_party_spell_hits+CB↑j ...
                cmp     [bp+var_4], 0
                jnz     short loc_187CA
                cmp     byte ptr word_27812+1, 1
                jnz     short loc_187CA
                cmp     byte ptr word_27812, 8
                jnz     short loc_187CA
                cmp     byte_27683, 0
                jz      short loc_187CA
                mov     [bp+var_4], 0

loc_187CA:                              ; CODE XREF: combat_party_spell_hits+119↑j
                                        ; combat_party_spell_hits+120↑j ...
                cmp     [bp+var_4], 0
                jnz     short loc_187E1
                cmp     byte ptr word_27812+1, 0
                jnz     short loc_187E1
                cmp     word_27816, 0
                jnz     short loc_187E1
                inc     [bp+var_4]

loc_187E1:                              ; CODE XREF: combat_party_spell_hits+138↑j
                                        ; combat_party_spell_hits+13F↑j ...
                call    thk_res_3E76
                cmp     [bp+var_4], 0
                jz      short loc_187F8
                mov     ax, offset aIsNotAffected ; " is not affected!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     loc_188B2
; ---------------------------------------------------------------------------
                align 2

loc_187F8:                              ; CODE XREF: combat_party_spell_hits+152↑j
                cmp     byte ptr word_27812+1, 0
                jnz     short loc_1884C
                mov     ax, offset aTakes ; " takes "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_27816
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 1182h
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word_27816, 1
                jbe     short loc_18836
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_18836:                              ; CODE XREF: combat_party_spell_hits+194↑j
                mov     ax, word_27816
                mov     word_27824, ax
                call    combat_damage_monster
                cmp     word_27824, 0FFFFh
                jnz     short loc_188B2
                dec     [bp+arg_2]
                jmp     short loc_188B2
; ---------------------------------------------------------------------------
                align 2

loc_1884C:                              ; CODE XREF: combat_party_spell_hits+167↑j
                mov     ax, offset aIs  ; " is "
                push    ax
                call    thk_text_puts
                add     sp, 2
                dec     byte ptr word_27812
                cmp     byte ptr word_27812, 7
                jnb     short loc_18876
                mov     bl, [bp+arg_2]
                sub     bh, bh
                mov     si, word_27812
                and     si, 0FFh
                mov     al, [si+1022h]
                or      [bx-607Ah], al

loc_18876:                              ; CODE XREF: combat_party_spell_hits+1C9↑j
                mov     bl, byte ptr word_27812
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+0FEAh]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 21h ; '!'
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     byte ptr word_27812, 7
                jnb     short loc_1889E
                call    combat_draw_monster_list
                jmp     short loc_188AE
; ---------------------------------------------------------------------------

loc_1889E:                              ; CODE XREF: combat_party_spell_hits+201↑j
                mov     byte_2781C, 1
                call    combat_kill_monster
                mov     byte_2781C, 0
                dec     [bp+arg_2]

loc_188AE:                              ; CODE XREF: combat_party_spell_hits+206↑j
                inc     byte ptr word_27812

loc_188B2:                              ; CODE XREF: combat_party_spell_hits+15E↑j
                                        ; combat_party_spell_hits+1AE↑j ...
                mov     ax, [bp+var_C]
                mov     word_27816, ax
                mov     al, [bp+var_2]
                mov     byte ptr word_27812+1, al
                dec     [bp+arg_0]
                inc     [bp+arg_2]
                cmp     [bp+arg_0], 0
                jnz     short loc_188CD
                inc     [bp+var_8]

loc_188CD:                              ; CODE XREF: combat_party_spell_hits+232↑j
                mov     al, [bp+var_6]
                cmp     [bp+arg_2], al
                jnz     short loc_188D8
                inc     [bp+var_8]

loc_188D8:                              ; CODE XREF: combat_party_spell_hits+23D↑j
                mov     al, byte ptr word_1DD58
                cmp     [bp+arg_2], al
                jb      short loc_188E3
                inc     [bp+var_8]

loc_188E3:                              ; CODE XREF: combat_party_spell_hits+248↑j
                call    sub_1A7D8
                cmp     [bp+var_8], 0
                jnz     short loc_188EF
                jmp     loc_186F1
; ---------------------------------------------------------------------------

loc_188EF:                              ; CODE XREF: combat_party_spell_hits+254↑j
                sub     al, al
                mov     byte_2781A, al
                mov     byte_2781E, al
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_party_spell_hits endp


; =============== S U B R O U T I N E =======================================

; exp/gold/gems for a kill
; Attributes: bp-based frame

combat_monster_rewards proc near        ; CODE XREF: combat_kill_monster+9↓p

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     [bp+var_2], 0
                cmp     byte_27672, 0
                jz      short loc_1891F
                mov     ax, 0Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     word_241AA, ax

loc_1891F:                              ; CODE XREF: combat_monster_rewards+F↑j
                cmp     byte_27671, 0
                jz      short loc_18995
                mov     bl, byte_2781E
                sub     bh, bh
                mov     al, [bx-6980h]
                mov     [bp+var_2], al
                cmp     byte_27671, 2
                jnz     short loc_1893F
                mov     cl, 4
                shr     [bp+var_2], cl

loc_1893F:                              ; CODE XREF: combat_monster_rewards+3C↑j
                cmp     byte_27671, 3
                jb      short loc_18949
                shr     [bp+var_2], 1

loc_18949:                              ; CODE XREF: combat_monster_rewards+48↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     [bp+var_2], al
                mov     ax, 32h ; '2'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 6
                mov     [bp+var_4], ax
                cmp     byte_27671, 1
                jnz     short loc_1897B
                mov     [bp+var_2], 0

loc_1897B:                              ; CODE XREF: combat_monster_rewards+79↑j
                mov     ax, [bp+var_4]
                sub     dx, dx
                add     word_241AC, ax
                adc     word_241AE, dx
                mov     ah, [bp+var_2]
                sub     al, al
                add     word_241AC, ax
                adc     word_241AE, dx

loc_18995:                              ; CODE XREF: combat_monster_rewards+28↑j
                cmp     byte_27673, 0
                jz      short loc_189BE
                mov     al, byte_22CE4
                cmp     byte_27673, al
                jb      short loc_189BE
                mov     bl, byte_2781E
                sub     bh, bh
                mov     al, [bx-6980h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte_22CE7, al
                mov     al, byte_27673
                mov     byte_22CE4, al

loc_189BE:                              ; CODE XREF: combat_monster_rewards+9E↑j
                                        ; combat_monster_rewards+A7↑j
                mov     ax, word_27692
                mov     dx, word_27694
                add     word_1E80E, ax
                adc     word_1E810, dx
                mov     sp, bp
                pop     bp
                retn
combat_monster_rewards endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_hireling_flags proc near         ; CODE XREF: combat_kill_monster+6↓p

var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     bl, byte_2781E
                sub     bh, bh
                mov     al, [bx-6980h]
                mov     [bp+var_6], al
                mov     [bp+var_4], 0
                jmp     short loc_18A15
; ---------------------------------------------------------------------------

loc_189EC:                              ; CODE XREF: combat_hireling_flags+49↓j
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+78h], 0
                jz      short loc_18A12
                mov     al, [bp+var_6]
                cmp     [bx+78h], al
                jnz     short loc_18A12
                test    byte ptr [bx+7Ch], 1
                jz      short loc_18A12
                or      byte ptr [bx+7Ch], 2

loc_18A12:                              ; CODE XREF: combat_hireling_flags+2C↑j
                                        ; combat_hireling_flags+34↑j ...
                inc     [bp+var_4]

loc_18A15:                              ; CODE XREF: combat_hireling_flags+18↑j
                mov     ax, g_party_size
                cmp     [bp+var_4], ax
                jl      short loc_189EC
                mov     sp, bp
                pop     bp
                retn
combat_hireling_flags endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; shift monster arrays down
; Attributes: bp-based frame

combat_remove_monster proc near         ; CODE XREF: ovl_2COMBAT:85C9↑p
                                        ; combat_kill_monster+F↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                dec     byte ptr word_1DD58
                mov     al, byte ptr word_1DD58
                cmp     byte_27815, al
                jbe     short loc_18A3A
                mov     byte_27815, al

loc_18A3A:                              ; CODE XREF: combat_remove_monster+13↑j
                cmp     byte_2781E, 0Ah
                jnb     short loc_18AA4
                mov     al, byte ptr word_1DD58
                mov     [bp+var_2], al
                cmp     al, 9
                jbe     short loc_18A4F
                mov     [bp+var_2], 0Ah

loc_18A4F:                              ; CODE XREF: combat_remove_monster+27↑j
                cmp     byte ptr word_1DD58, 0
                jz      short loc_18AA4
                mov     al, byte_2781E
                mov     [bp+var_4], al
                jmp     short loc_18A9C
; ---------------------------------------------------------------------------

loc_18A5E:                              ; CODE XREF: combat_remove_monster+80↓j
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     si, ax
                mov     al, [si-6079h]
                mov     [si-607Ah], al
                mov     al, [si-606Dh]
                mov     [si-606Eh], al
                mov     al, [si-6061h]
                mov     [si-6062h], al
                mov     di, si
                shl     di, 1
                mov     ax, [di-6054h]
                mov     [di-6056h], ax
                mov     al, [si+5481h]
                mov     [si+5480h], al
                mov     al, [si-697Fh]
                mov     [si-6980h], al
                inc     [bp+var_4]

loc_18A9C:                              ; CODE XREF: combat_remove_monster+3A↑j
                mov     al, [bp+var_2]
                cmp     [bp+var_4], al
                jb      short loc_18A5E

loc_18AA4:                              ; CODE XREF: combat_remove_monster+1D↑j
                                        ; combat_remove_monster+32↑j
                call    combat_draw_monster_list
                mov     ax, 8
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
combat_remove_monster endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; " runs away!" / " goes down!"

combat_monster_gone_text proc near      ; CODE XREF: ovl_2COMBAT:85C6↑p
                                        ; combat_kill_monster+C↓p
                mov     al, byte_2781E
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     byte_2781C, 0
                jnz     short locret_18AF2
                call    thk_res_3E76
                cmp     byte_22CF6, 0
                jz      short loc_18AE8
                mov     ax, offset aRunsAway ; " runs away!"
                jmp     short loc_18AEB
; ---------------------------------------------------------------------------

loc_18AE8:                              ; CODE XREF: combat_monster_gone_text+29↑j
                mov     ax, offset aGoesDown_0 ; " goes down!"

loc_18AEB:                              ; CODE XREF: combat_monster_gone_text+2E↑j
                push    ax
                call    thk_text_puts
                add     sp, 2

locret_18AF2:                           ; CODE XREF: combat_monster_gone_text+1F↑j
                retn
combat_monster_gone_text endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_kill_monster proc near           ; CODE XREF: seg002:03F9↑J
                                        ; combat_party_spell_hits+20D↑p ...

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    combat_hireling_flags
                call    combat_monster_rewards
                call    combat_monster_gone_text
                call    combat_remove_monster
                cmp     byte_2781E, 0Ah
                jb      short loc_18B3A
                mov     al, byte ptr word_1DD58
                sub     al, 9
                mov     [bp+var_2], al
                mov     byte ptr word_1DD58, 0Ah
                mov     [bp+var_4], 0
                jmp     short loc_18B32
; ---------------------------------------------------------------------------

loc_18B20:                              ; CODE XREF: combat_kill_monster+44↓j
                mov     ax, word_27692
                mov     dx, word_27694
                add     word_1E80E, ax
                adc     word_1E810, dx
                inc     [bp+var_4]

loc_18B32:                              ; CODE XREF: combat_kill_monster+2A↑j
                mov     al, [bp+var_2]
                cmp     [bp+var_4], al
                jb      short loc_18B20

loc_18B3A:                              ; CODE XREF: combat_kill_monster+17↑j
                mov     sp, bp
                pop     bp
                retn
combat_kill_monster endp


; =============== S U B R O U T I N E =======================================

; word_27824 damage to monster byte_2781E
; Attributes: bp-based frame

combat_damage_monster proc near         ; CODE XREF: combat_party_spell_hits+1A6↑p
                                        ; combat_party_attack_result+F8↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     al, byte_2781E
                mov     [bp+var_2], al
                cmp     al, 0Ah
                jbe     short loc_18B54
                mov     byte_2781E, 0Ah

loc_18B54:                              ; CODE XREF: combat_damage_monster+F↑j
                mov     bl, byte_2781E
                sub     bh, bh
                mov     al, [bx-607Ah]
                mov     [bp+var_4], al
                cmp     al, 80h
                ja      short loc_18B68
                mov     [bp+var_4], bh

loc_18B68:                              ; CODE XREF: combat_damage_monster+25↑j
                and     [bp+var_4], 0EFh
                or      [bp+var_4], 1
                mov     al, byte_2781E
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_4]
                mov     [si-607Ah], al
                mov     bx, si
                shl     bx, 1
                mov     ax, word_27824
                cmp     [bx-6056h], ax
                ja      short loc_18B96
                call    combat_kill_monster
                mov     word_27824, 0FFFFh
                jmp     short loc_18BA5
; ---------------------------------------------------------------------------

loc_18B96:                              ; CODE XREF: combat_damage_monster+4B↑j
                mov     bl, byte_2781E
                sub     bh, bh
                shl     bx, 1
                mov     ax, word_27824
                sub     [bx-6056h], ax

loc_18BA5:                              ; CODE XREF: combat_damage_monster+56↑j
                call    combat_draw_monster_list
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_damage_monster endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "N time(s) and hit M time(s) for X point(s)"

combat_attack_summary_text proc near    ; CODE XREF: combat_monster_hits+73↑p
                                        ; combat_party_attack_result:loc_18D66↓p
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte_22CEC
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, offset aTime ; " time"
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_22CEC, 1
                jbe     short loc_18BEB
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_18BEB:                              ; CODE XREF: combat_attack_summary_text+31↑j
                mov     ax, offset aAnd ; " and "
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word_27824, 0
                jnz     short loc_18C06
                mov     ax, offset aMissed ; "missed!"
                push    ax
                call    thk_text_puts
                jmp     short loc_18C73
; ---------------------------------------------------------------------------
                align 2

loc_18C06:                              ; CODE XREF: combat_attack_summary_text+4C↑j
                mov     ax, offset aHit ; "hit "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte_22CEA
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, offset aTime_0 ; " time"
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_22CEA, 1
                jbe     short loc_18C3F
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_18C3F:                              ; CODE XREF: combat_attack_summary_text+85↑j
                mov     ax, offset aFor ; " for "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_27824
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 11C8h
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word_27824, 1
                jbe     short locret_18C76
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc

loc_18C73:                              ; CODE XREF: combat_attack_summary_text+55↑j
                add     sp, 2

locret_18C76:                           ; CODE XREF: combat_attack_summary_text+BC↑j
                retn
combat_attack_summary_text endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; " shoots ", " attacks ", " back stabs", " criticals"
; Attributes: bp-based frame

combat_party_attack_result proc near    ; CODE XREF: combat_party_attack:loc_190B3↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_6], 0
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                push    [bp+var_2]
                call    thk_res_3E40
                add     sp, 2
                cmp     byte_22CF4, 1
                jnz     short loc_18CB4
                mov     ax, offset aShoots_0 ; " shoots "

loc_18CAD:                              ; CODE XREF: combat_party_attack_result+46↓j
                push    ax
                call    thk_text_puts
                jmp     short loc_18CFA
; ---------------------------------------------------------------------------
                align 2

loc_18CB4:                              ; CODE XREF: combat_party_attack_result+30↑j
                cmp     byte_22CE6, 0
                jnz     short loc_18CC0
                mov     ax, offset aAttacks_0 ; " attacks "
                jmp     short loc_18CAD
; ---------------------------------------------------------------------------

loc_18CC0:                              ; CODE XREF: combat_party_attack_result+41↑j
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                cmp     byte_22CE6, 1
                jnz     short loc_18CE0
                mov     ax, offset aBackStabs ; "back stabs"
                jmp     short loc_18CE3
; ---------------------------------------------------------------------------

loc_18CE0:                              ; CODE XREF: combat_party_attack_result+61↑j
                mov     ax, offset aCriticals ; "criticals"

loc_18CE3:                              ; CODE XREF: combat_party_attack_result+66↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc

loc_18CFA:                              ; CODE XREF: combat_party_attack_result+39↑j
                add     sp, 2
                call    thk_res_3E76
                mov     al, byte_27680
                mov     [bp+var_4], al
                or      al, al
                jz      short loc_18D1B
                dec     [bp+var_4]
                mov     bx, [bp+var_2]
                mov     al, [bp+var_4]
                cmp     [bx+0Ch], al
                jnz     short loc_18D1B
                inc     [bp+var_6]

loc_18D1B:                              ; CODE XREF: combat_party_attack_result+90↑j
                                        ; combat_party_attack_result+9E↑j
                cmp     byte_22CF4, 1
                jnz     short loc_18D2A
                mov     bx, [bp+var_2]
                mov     al, [bx+4Eh]
                jmp     short loc_18D30
; ---------------------------------------------------------------------------

loc_18D2A:                              ; CODE XREF: combat_party_attack_result+A8↑j
                mov     bx, [bp+var_2]
                mov     al, [bx+4Ch]

loc_18D30:                              ; CODE XREF: combat_party_attack_result+B0↑j
                mov     [bp+var_4], al
                cmp     [bp+var_6], 0
                jnz     short loc_18D43
                cmp     byte_2768C, 0
                jz      short loc_18D43
                inc     [bp+var_6]

loc_18D43:                              ; CODE XREF: combat_party_attack_result+BF↑j
                                        ; combat_party_attack_result+C6↑j
                cmp     [bp+var_6], 0
                jz      short loc_18D66
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                call    thk_res_3E76
                mov     ax, offset aIsNotAffected_0 ; " is not affected!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_18D73
; ---------------------------------------------------------------------------

loc_18D66:                              ; CODE XREF: combat_party_attack_result+CF↑j
                call    combat_attack_summary_text
                cmp     word_27824, 0   ; CODE XREF: seg002:0789↑J
                jz      short loc_18D73
                call    combat_damage_monster

loc_18D73:                              ; CODE XREF: combat_party_attack_result+EC↑j
                                        ; combat_party_attack_result+F6↑j
                call    sub_1A7D8
                mov     sp, bp
                pop     bp
                retn
combat_party_attack_result endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_text_reset proc near             ; CODE XREF: seg002:04A1↑J
                                        ; combat_monster_hits+24↑p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                add     si, 0Fh
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                push    si
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                push    si
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                pop     si
                pop     bp
                retn
combat_text_reset endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; fight/shoot: attacks per round, to-hit rolls, damage
; Attributes: bp-based frame

combat_party_attack proc near           ; CODE XREF: combat_party_shoot+E↓p
                                        ; combat_party_fight+E↓p

var_18          = byte ptr -18h
var_16          = byte ptr -16h
var_14          = word ptr -14h
var_12          = byte ptr -12h
var_10          = byte ptr -10h
var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 18h
                push    di
                push    si
                mov     [bp+var_E], 1
                cmp     byte_22CF4, 1
                jnz     short loc_18DC4
                mov     al, byte ptr word_1DD58
                jmp     short loc_18DC7
; ---------------------------------------------------------------------------
                align 2

loc_18DC4:                              ; CODE XREF: combat_party_attack+12↑j
                mov     al, byte_27815

loc_18DC7:                              ; CODE XREF: combat_party_attack+17↑j
                mov     [bp+var_C], al
                cmp     al, 0Ah
                jbe     short loc_18DD2
                mov     [bp+var_C], 0Ah

loc_18DD2:                              ; CODE XREF: combat_party_attack+22↑j
                cmp     [bp+var_C], 1
                ja      short loc_18DDC
                mov     [bp+arg_0], 0

loc_18DDC:                              ; CODE XREF: combat_party_attack+2C↑j
                cmp     [bp+arg_0], 0FFh
                jz      short loc_18DE5
                jmp     loc_18E78
; ---------------------------------------------------------------------------

loc_18DE5:                              ; CODE XREF: combat_party_attack+36↑j
                mov     al, [bp+var_C]
                add     al, 40h ; '@'
                mov     [bp+var_18], al
                mov     bx, word_1E860
                mov     [bx+0Ch], al
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                cmp     byte_22CF4, 0
                jnz     short loc_18E0A
                mov     ax, offset aFight ; "Fight"
                jmp     short loc_18E0D
; ---------------------------------------------------------------------------
                align 2

loc_18E0A:                              ; CODE XREF: combat_party_attack+58↑j
                mov     ax, offset aShoot ; "Shoot"

loc_18E0D:                              ; CODE XREF: combat_party_attack+5D↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_1E860
                call    thk_text_puts
                add     sp, 2
                call    thk_res_5440

loc_18E21:                              ; CODE XREF: combat_party_attack+B5↓j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_14], ax
                cmp     ax, 1Bh
                jnz     short loc_18E38
                mov     ax, 1
                jmp     short loc_18E3A
; ---------------------------------------------------------------------------

loc_18E38:                              ; CODE XREF: combat_party_attack+87↑j
                sub     ax, ax

loc_18E3A:                              ; CODE XREF: combat_party_attack+8C↑j
                mov     [bp+var_A], ax
                or      ax, ax
                jnz     short loc_18E5B
                cmp     [bp+var_14], 41h ; 'A'
                jb      short loc_18E56
                mov     al, [bp+var_18]
                sub     ah, ah
                cmp     [bp+var_14], ax
                ja      short loc_18E56
                mov     ax, 1
                jmp     short loc_18E58
; ---------------------------------------------------------------------------

loc_18E56:                              ; CODE XREF: combat_party_attack+9B↑j
                                        ; combat_party_attack+A5↑j
                sub     ax, ax

loc_18E58:                              ; CODE XREF: combat_party_attack+AA↑j
                mov     [bp+var_A], ax

loc_18E5B:                              ; CODE XREF: combat_party_attack+95↑j
                cmp     [bp+var_A], 0
                jz      short loc_18E21
                call    thk_res_35A8
                cmp     [bp+var_14], 1Bh
                jnz     short loc_18E70
                dec     [bp+var_E]
                jmp     short loc_18E78
; ---------------------------------------------------------------------------
                align 2

loc_18E70:                              ; CODE XREF: combat_party_attack+BE↑j
                mov     al, byte ptr [bp+var_14]
                sub     al, 41h ; 'A'
                mov     [bp+arg_0], al

loc_18E78:                              ; CODE XREF: combat_party_attack+38↑j
                                        ; combat_party_attack+C3↑j
                cmp     [bp+var_E], 0
                jnz     short loc_18E81
                jmp     loc_190B6
; ---------------------------------------------------------------------------

loc_18E81:                              ; CODE XREF: combat_party_attack+D2↑j
                mov     al, [bp+arg_0]
                mov     byte_2781E, al
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_8], ax
                mov     al, byte_2781E
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                sub     al, al
                mov     byte_22CE6, al
                mov     byte_22CF1, al
                mov     byte_22CEF, al
                mov     byte_22CEA, al
                mov     word_27824, 0
                mov     bx, [bp+var_8]
                mov     al, [bx+0Fh]
                sub     ah, ah
                mov     si, ax
                mov     al, [bx+71h]
                mov     di, ax
                div     byte ptr [si+1012h]
                mov     byte_22CF2, al
                mov     ax, di
                div     byte ptr [si+101Ah]
                mov     byte_22CEB, al
                inc     byte_22CEB
                mov     al, byte_22CEB
                mov     byte_22CEC, al
                mov     al, [bx+4Ch]
                mov     byte_22CEF, al
                mov     al, [bx+4Dh]
                mov     byte_22CF3, al
                cmp     byte_22CF4, 1
                jnz     short loc_18F27
                mov     al, [bx+4Fh]
                mov     byte_22CF3, al
                cmp     byte ptr [bx+0Fh], 2
                jz      short loc_18F06
                mov     al, [bx+4Eh]
                mov     byte_22CEF, al
                jmp     short loc_18F27
; ---------------------------------------------------------------------------

loc_18F06:                              ; CODE XREF: combat_party_attack+152↑j
                mov     al, [bx+71h]
                mov     [bp+var_10], al
                cmp     al, 64h ; 'd'
                jbe     short loc_18F14
                mov     [bp+var_10], 64h ; 'd'

loc_18F14:                              ; CODE XREF: combat_party_attack+164↑j
                mov     al, [bp+var_10]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte_22CF1, al

loc_18F27:                              ; CODE XREF: combat_party_attack+146↑j
                                        ; combat_party_attack+15A↑j
                mov     al, byte_22CF3
                add     byte_22CF1, al
                mov     bx, [bp+var_8]
                mov     al, [bx+6Bh]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                add     byte_22CF1, al
                mov     bx, [bp+var_8]
                mov     al, [bx+6Fh]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                add     byte_22CF3, al
                mov     al, byte_1DC33
                add     byte_22CF3, al
                mov     [bp+var_10], 0
                jmp     loc_19011
; ---------------------------------------------------------------------------

loc_18F62:                              ; CODE XREF: combat_party_attack+28C↓j
                cmp     [bp+var_12], 9
                jnb     short loc_18F6E

loc_18F68:                              ; CODE XREF: combat_party_attack+20F↓j
                                        ; combat_party_attack+21F↓j ...
                inc     [bp+var_6]
                jmp     short loc_18FDC
; ---------------------------------------------------------------------------
                align 2

loc_18F6E:                              ; CODE XREF: combat_party_attack+1BC↑j
                mov     [bp+var_16], 19h
                mov     bx, [bp+var_8]
                test    byte ptr [bx+26h], 1
                jz      short loc_18F7F
                mov     [bp+var_16], 3

loc_18F7F:                              ; CODE XREF: combat_party_attack+1CF↑j
                mov     al, [bp+var_16]
                sub     ah, ah
                mov     cl, byte_22CF2
                sub     ch, ch
                add     ax, cx
                mov     [bp+var_2], ax
                cmp     ax, 0FAh
                jle     short loc_18F99
                mov     [bp+var_2], 0FAh

loc_18F99:                              ; CODE XREF: combat_party_attack+1E8↑j
                push    [bp+var_2]
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     cl, byte_22CF3
                sub     ch, ch
                add     ax, cx
                mov     [bp+var_2], ax
                cmp     ax, 0FFh
                jg      short loc_18FD9
                cmp     ax, 0Ah
                jle     short loc_18F68
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     cl, byte_1DC2B
                sub     ax, cx
                cmp     ax, 80h
                jnb     short loc_18F68
                mov     al, byte ptr [bp+var_2]
                mov     [bp+var_16], al
                mov     al, byte_2767C
                cmp     [bp+var_16], al
                jb      short loc_18F68

loc_18FD9:                              ; CODE XREF: combat_party_attack+20A↑j
                                        ; combat_party_attack:loc_19039↓j
                inc     [bp+var_4]

loc_18FDC:                              ; CODE XREF: combat_party_attack+1C1↑j
                cmp     [bp+var_4], 0
                jz      short loc_1900E
                mov     al, byte_22CEF
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     al, byte_22CF1
                mov     [bp+var_16], al
                cmp     al, 0FAh
                jbe     short loc_19001
                mov     [bp+var_16], 1

loc_19001:                              ; CODE XREF: combat_party_attack+251↑j
                inc     byte_22CEA
                mov     al, [bp+var_16]
                sub     ah, ah
                add     word_27824, ax

loc_1900E:                              ; CODE XREF: combat_party_attack+236↑j
                inc     [bp+var_10]

loc_19011:                              ; CODE XREF: combat_party_attack+1B5↑j
                mov     al, byte_22CEB
                cmp     [bp+var_10], al
                jnb     short loc_1903C
                sub     al, al
                mov     [bp+var_4], al
                mov     [bp+var_6], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_12], al
                cmp     al, 6
                jb      short loc_19039
                jmp     loc_18F62
; ---------------------------------------------------------------------------

loc_19039:                              ; CODE XREF: combat_party_attack+28A↑j
                jmp     short loc_18FD9
; ---------------------------------------------------------------------------
                align 2

loc_1903C:                              ; CODE XREF: combat_party_attack+26D↑j
                cmp     byte_22CEA, 0
                jz      short loc_1904C
                mov     al, byte_1DC37
                sub     ah, ah
                add     word_27824, ax

loc_1904C:                              ; CODE XREF: combat_party_attack+297↑j
                cmp     byte_22CF4, 0
                jnz     short loc_190B3
                mov     bx, [bp+var_8]
                mov     al, [bx+72h]
                mov     [bp+var_16], al
                cmp     al, 64h ; 'd'
                jbe     short loc_19064
                mov     [bp+var_16], 64h ; 'd'

loc_19064:                              ; CODE XREF: combat_party_attack+2B4↑j
                mov     al, [bp+var_16]
                sub     ah, ah
                add     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_16], al
                mov     bx, [bp+var_8]
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_19096
                cmp     al, 5Ah ; 'Z'
                ja      short loc_1908B
                cmp     al, 5
                jnb     short loc_190B3

loc_1908B:                              ; CODE XREF: combat_party_attack+2DB↑j
                inc     byte_22CE6
                shl     word_27824, 1
                jmp     short loc_190B3
; ---------------------------------------------------------------------------
                align 2

loc_19096:                              ; CODE XREF: combat_party_attack+2D7↑j
                cmp     byte ptr [bx+0Fh], 6
                jnz     short loc_190B3
                cmp     [bp+var_16], 5Eh ; '^'
                ja      short loc_190A8
                cmp     [bp+var_16], 5
                jnb     short loc_190B3

loc_190A8:                              ; CODE XREF: combat_party_attack+2F6↑j
                mov     byte_22CE6, 2
                mov     cl, 2
                shl     word_27824, cl

loc_190B3:                              ; CODE XREF: combat_party_attack+2A7↑j
                                        ; combat_party_attack+2DF↑j ...
                call    combat_party_attack_result

loc_190B6:                              ; CODE XREF: combat_party_attack+D4↑j
                mov     ax, [bp+var_E]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
combat_party_attack endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_party_shoot proc near            ; CODE XREF: combat_party_turn+7A↓p
                                        ; combat_party_turn+19C↓p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                mov     byte_22CF4, 1
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    combat_party_attack
                add     sp, 2
                pop     bp
                retn
combat_party_shoot endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_party_fight proc near            ; CODE XREF: combat_party_turn+8A↓p
                                        ; combat_party_turn+15E↓p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                mov     byte_22CF4, 0
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    combat_party_attack
                add     sp, 2
                pop     bp
                retn
combat_party_fight endp


; =============== S U B R O U T I N E =======================================

; back to last safe cell; status >= 10h becomes 81h (dead) unless byte_27818
; Attributes: bp-based frame

combat_party_flees proc near            ; CODE XREF: combat_party_turn:loc_1A29E↓p
                                        ; combat_encounter:loc_1A6F8↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     al, byte_231E4
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, byte_231E4
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     g_party_y, al
                call    thk_clear_spell_effects
                inc     word_1DC62
                mov     word_238A0, 1
                cmp     byte_27818, 0
                jnz     short loc_19145
                mov     [bp+var_2], 0
                jmp     short loc_1913D
; ---------------------------------------------------------------------------
                align 2

loc_19122:                              ; CODE XREF: combat_party_flees+57↓j
                push    [bp+var_2]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 10h
                jb      short loc_1913A
                mov     byte ptr [bx+26h], 81h

loc_1913A:                              ; CODE XREF: combat_party_flees+48↑j
                inc     [bp+var_2]

loc_1913D:                              ; CODE XREF: combat_party_flees+33↑j
                mov     ax, g_party_size
                cmp     [bp+var_2], ax
                jl      short loc_19122

loc_19145:                              ; CODE XREF: combat_party_flees+2C↑j
                mov     sp, bp
                pop     bp
                retn
combat_party_flees endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_char_runs proc near              ; CODE XREF: combat_party_turn:loc_19544↓p

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                mov     al, byte_231E3
                cmp     [bp+var_2], al
                jnb     short loc_191C7
                mov     byte_22CCE, 1
                dec     g_party_size
                mov     al, byte_27822
                cmp     byte ptr g_party_size, al
                jz      short loc_191A9
                mov     bx, g_party_size
                mov     al, [bx+548Ch]
                mov     [bp+var_2], al
                shl     bx, 1
                mov     ax, [bx+416h]
                mov     [bp+var_4], ax
                mov     al, byte_27822
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_2]
                mov     [si+548Ch], al
                mov     bx, si
                shl     bx, 1
                mov     ax, [bp+var_4]
                mov     [bx+416h], ax

loc_191A9:                              ; CODE XREF: combat_char_runs+30↑j
                mov     al, byte_22CED
                cmp     byte ptr g_party_size, al
                jnb     short loc_191B8
                mov     al, byte ptr g_party_size
                mov     byte_22CED, al

loc_191B8:                              ; CODE XREF: combat_char_runs+66↑j
                mov     byte_2781F, 0
                cmp     g_party_size, 0
                jz      short loc_191C7
                call    combat_draw_party_hp

loc_191C7:                              ; CODE XREF: combat_char_runs+1E↑j
                                        ; combat_char_runs+78↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_char_runs endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_wait_command_key proc near       ; CODE XREF: combat_party_turn:loc_193E4↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     byte_2294F, 0FDh

loc_191D7:                              ; CODE XREF: combat_wait_command_key+93↓j
                mov     [bp+var_2], 0
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_4], ax
                cmp     ax, 42h ; 'B'
                jz      short loc_19216
                cmp     ax, 44h ; 'D'
                jz      short loc_19216
                cmp     ax, 50h ; 'P'
                jz      short loc_19216
                cmp     ax, 51h ; 'Q'
                jz      short loc_19216
                cmp     ax, 56h ; 'V'
                jz      short loc_19216
                cmp     ax, 55h ; 'U'
                jz      short loc_19216
                cmp     ax, 45h ; 'E'
                jz      short loc_19216
                cmp     ax, 52h ; 'R'
                jz      short loc_19216
                cmp     ax, 1
                jnz     short loc_19219

loc_19216:                              ; CODE XREF: combat_wait_command_key+20↑j
                                        ; combat_wait_command_key+25↑j ...
                inc     [bp+var_2]

loc_19219:                              ; CODE XREF: combat_wait_command_key+48↑j
                cmp     [bp+var_4], 41h ; 'A'
                jnz     short loc_19229
                cmp     byte_22CE5, 0
                jz      short loc_19229
                inc     [bp+var_2]

loc_19229:                              ; CODE XREF: combat_wait_command_key+51↑j
                                        ; combat_wait_command_key+58↑j
                cmp     [bp+var_4], 46h ; 'F'
                jnz     short loc_19239
                cmp     byte_22CE5, 0
                jz      short loc_19239
                inc     [bp+var_2]

loc_19239:                              ; CODE XREF: combat_wait_command_key+61↑j
                                        ; combat_wait_command_key+68↑j
                cmp     [bp+var_4], 53h ; 'S'
                jnz     short loc_19249
                cmp     byte_22CDB, 0
                jz      short loc_19249
                inc     [bp+var_2]

loc_19249:                              ; CODE XREF: combat_wait_command_key+71↑j
                                        ; combat_wait_command_key+78↑j
                cmp     [bp+var_4], 43h ; 'C'
                jnz     short loc_19259
                cmp     byte_22CF0, 0
                jz      short loc_19259
                inc     [bp+var_2]

loc_19259:                              ; CODE XREF: combat_wait_command_key+81↑j
                                        ; combat_wait_command_key+88↑j
                cmp     [bp+var_2], 0
                jnz     short loc_19262
                jmp     loc_191D7
; ---------------------------------------------------------------------------

loc_19262:                              ; CODE XREF: combat_wait_command_key+91↑j
                mov     ax, [bp+var_4]
                mov     sp, bp
                pop     bp
                retn
combat_wait_command_key endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

combat_menu_item proc near              ; CODE XREF: combat_options_menu+A3↓p
                                        ; combat_options_menu+B6↓p ...

arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                push    si
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+1006h]
                push    ax
                mov     al, [si+0FFCh]
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, [bp+arg_0]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+0FD8h]
                call    thk_text_puts
                add     sp, 2
                pop     si
                pop     bp
                retn
combat_menu_item endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; " Options for:"
; Attributes: bp-based frame

combat_options_menu proc near           ; CODE XREF: combat_party_turn+2F↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 0
                sub     al, al
                mov     byte_22CE9, al
                mov     byte_22CF0, al
                mov     byte_22CDB, al
                mov     byte_22CE5, al
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     ax, offset aOptionsFor ; " Options for:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    [bp+var_2]
                call    thk_text_puts
                add     sp, 2
                mov     al, byte_22CED
                cmp     byte_27822, al
                jnb     short loc_192F8
                inc     byte_22CE5

loc_192F8:                              ; CODE XREF: combat_options_menu+58↑j
                mov     bx, [bp+var_2]
                cmp     byte ptr [bx+0Fh], 2
                jz      short loc_1930A
                mov     al, byte_22CED
                cmp     byte_27822, al
                jb      short loc_19314

loc_1930A:                              ; CODE XREF: combat_options_menu+65↑j
                cmp     byte ptr [bx+4Eh], 0
                jz      short loc_19314
                inc     byte_22CDB

loc_19314:                              ; CODE XREF: combat_options_menu+6E↑j
                                        ; combat_options_menu+74↑j
                test    byte ptr [bx+26h], 2
                jnz     short loc_1932A
                cmp     byte ptr [bx+72h], 0
                jz      short loc_1932A
                cmp     word ptr [bx+58h], 0
                jz      short loc_1932A
                inc     byte_22CF0

loc_1932A:                              ; CODE XREF: combat_options_menu+7E↑j
                                        ; combat_options_menu+84↑j ...
                cmp     byte_22CE5, 0
                jz      short loc_19356
                mov     al, [bp+var_4]
                inc     [bp+var_4]
                sub     ah, ah
                push    ax
                sub     ax, ax
                push    ax
                call    combat_menu_item
                add     sp, 4
                mov     al, [bp+var_4]
                inc     [bp+var_4]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    combat_menu_item
                add     sp, 4

loc_19356:                              ; CODE XREF: combat_options_menu+95↑j
                cmp     byte_22CDB, 0
                jz      short loc_19370
                mov     al, [bp+var_4]
                inc     [bp+var_4]
                sub     ah, ah
                push    ax
                mov     ax, 2
                push    ax
                call    combat_menu_item
                add     sp, 4

loc_19370:                              ; CODE XREF: combat_options_menu+C1↑j
                cmp     byte_22CF0, 0
                jz      short loc_1938A
                mov     al, [bp+var_4]
                inc     [bp+var_4]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    combat_menu_item
                add     sp, 4

loc_1938A:                              ; CODE XREF: combat_options_menu+DB↑j
                mov     [bp+var_6], 0

loc_1938E:                              ; CODE XREF: combat_options_menu+111↓j
                mov     al, [bp+var_4]
                inc     [bp+var_4]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_6]
                add     ax, 4
                push    ax
                call    combat_menu_item
                add     sp, 4
                inc     [bp+var_6]
                cmp     [bp+var_6], 5
                jb      short loc_1938E
                mov     sp, bp
                pop     bp
                retn
combat_options_menu endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; one character's command loop
; Attributes: bp-based frame

combat_party_turn proc near             ; CODE XREF: combat_party_turn+E9A↓p

var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_6], 1
                mov     al, byte_27822
                sub     ah, ah
                mov     si, ax
                inc     byte ptr [si+548Ch]
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 10h
                jb      short loc_193DB
                jmp     loc_195A3
; ---------------------------------------------------------------------------

loc_193DB:                              ; CODE XREF: combat_party_turn+24↑j
                                        ; combat_party_turn+1EE↓j
                cmp     [bp+var_6], 0
                jz      short loc_193E4
                call    combat_options_menu

loc_193E4:                              ; CODE XREF: combat_party_turn+2D↑j
                call    combat_wait_command_key
                mov     [bp+var_4], ax
                sub     ax, ax
                mov     [bp+var_6], ax
                mov     [bp+var_2], ax
                mov     ax, [bp+var_4]
                cmp     ax, 45h ; 'E'
                jnz     short loc_193FD
                jmp     loc_194E4
; ---------------------------------------------------------------------------

loc_193FD:                              ; CODE XREF: combat_party_turn+46↑j
                jle     short loc_19402
                jmp     loc_19568
; ---------------------------------------------------------------------------

loc_19402:                              ; CODE XREF: combat_party_turn:loc_193FD↑j
                cmp     ax, 1
                jz      short loc_19422
                cmp     ax, 41h ; 'A'
                jz      short loc_19439
                cmp     ax, 42h ; 'B'
                jz      short loc_19442
                cmp     ax, 43h ; 'C'
                jz      short loc_19448
                cmp     ax, 44h ; 'D'
                jnz     short loc_1941E
                jmp     loc_194D4
; ---------------------------------------------------------------------------

loc_1941E:                              ; CODE XREF: seg002:07B9↑J
                                        ; combat_party_turn+67↑j
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------
                align 2

loc_19422:                              ; CODE XREF: combat_party_turn+53↑j
                cmp     byte_22CDB, 0
                jz      short loc_19432
                sub     ax, ax
                push    ax
                call    combat_party_shoot
                jmp     short loc_1943F
; ---------------------------------------------------------------------------
                align 2

loc_19432:                              ; CODE XREF: combat_party_turn+75↑j
                cmp     byte_22CE5, 0
                jz      short loc_19442

loc_19439:                              ; CODE XREF: combat_party_turn+58↑j
                sub     ax, ax
                push    ax
                call    combat_party_fight

loc_1943F:                              ; CODE XREF: combat_party_turn+7D↑j
                add     sp, 2

loc_19442:                              ; CODE XREF: combat_party_turn+5D↑j
                                        ; combat_party_turn+85↑j ...
                inc     [bp+var_2]
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------

loc_19448:                              ; CODE XREF: combat_party_turn+62↑j
                call    thk_res_5440
                sub     ax, ax
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    thk_cast_spell_menu
                add     sp, 2
                mov     [bp+var_2], ax
                cmp     ax, 0FFFFh
                jnz     short loc_19482 ; CODE XREF: seg002:07C5↑J

loc_1946F:                              ; CODE XREF: combat_party_turn+117↓j
                mov     [bp+var_2], 0

loc_19474:                              ; CODE XREF: combat_party_turn+11F↓j
                mov     byte_1DBE6, 0
                inc     [bp+var_6]
                call    thk_res_35A8
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------

loc_19482:                              ; CODE XREF: combat_party_turn+BB↑j
                mov     word_27816, 0
                mov     byte ptr word_27812+1, 0
                mov     byte_1DBE6, 1
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     word_23626, ax
                push    [bp+var_2]
                call    sub_1AB02
                add     sp, 2
                or      ax, ax
                jz      short loc_194B6
                push    [bp+var_2]
                call    thk_2CAST1_D0C2
                jmp     short loc_194BC
; ---------------------------------------------------------------------------

loc_194B6:                              ; CODE XREF: combat_party_turn+FA↑j
                push    [bp+var_2]
                call    thk_2CAST2_CF2C

loc_194BC:                              ; CODE XREF: combat_party_turn+102↑j
                add     sp, 2
                mov     [bp+var_2], 1
                cmp     byte_1DC78, 0
                jz      short loc_1946F
                call    thk_res_5D1A
                call    combat_draw_party_hp
                jmp     short loc_19474
; ---------------------------------------------------------------------------
                align 2

loc_194D4:                              ; CODE XREF: combat_party_turn+69↑j
                call    thk_2MISC2_C3F6

loc_194D7:                              ; CODE XREF: combat_party_turn+190↓j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------
                align 2

loc_194E4:                              ; CODE XREF: combat_party_turn+48↑j
                mov     ax, 548Ch
                push    ax
                mov     ax, 416h
                push    ax
                mov     al, byte_27822
                sub     ah, ah
                push    ax
                mov     al, byte ptr g_party_size
                push    ax
                call    thk_2MISC2_C370
                add     sp, 8
                mov     [bp+var_2], ax
                or      ax, ax
                jnz     short loc_19506
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------

loc_19506:                              ; CODE XREF: combat_party_turn+14F↑j
                call    combat_draw_party_hp
                jmp     def_19573       ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------

loc_1950C:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn:jpt_19573↓o
                mov     ax, 0FFh        ; jumptable 00019573 case 70
                push    ax
                call    combat_party_fight

loc_19513:                              ; CODE XREF: combat_party_turn+19F↓j
                add     sp, 2
                mov     [bp+var_2], ax

loc_19519:                              ; CODE XREF: combat_party_turn+1AA↓j
                                        ; combat_party_turn+1B4↓j
                inc     [bp+var_6]
                jmp     short def_19573 ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_19520:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn+1DA↓o
                call    combat_show_protection ; jumptable 00019573 case 80
                call    combat_draw_monster_list
                jmp     short def_19573 ; jumptable 00019573 default case, cases 71-79,84
; ---------------------------------------------------------------------------

loc_19528:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn+1DC↓o ...
                cmp     [bp+var_4], 56h ; 'V' ; jumptable 00019573 cases 81,86
                jnz     short loc_19539
                mov     al, byte_27822
                sub     ah, ah
                add     ax, 31h ; '1'
                mov     [bp+var_4], ax

loc_19539:                              ; CODE XREF: combat_party_turn+17A↑j
                push    [bp+var_4]
                call    thk_party_status_loop
                add     sp, 2
                jmp     short loc_194D7
; ---------------------------------------------------------------------------

loc_19544:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn+1DE↓o
                call    combat_char_runs ; jumptable 00019573 case 82
                jmp     loc_19442
; ---------------------------------------------------------------------------

loc_1954A:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn+1E0↓o
                mov     ax, 0FFh        ; jumptable 00019573 case 83
                push    ax
                call    combat_party_shoot
                jmp     short loc_19513
; ---------------------------------------------------------------------------
                align 2

loc_19554:                              ; CODE XREF: combat_party_turn+1C1↓j
                                        ; DATA XREF: combat_party_turn+1E4↓o
                call    near ptr byte_1B862+1B6h ; jumptable 00019573 case 85
                cmp     byte_2419E, 0
                jz      short loc_19519
                inc     [bp+var_2]
                mov     byte_2419E, 0
                jmp     short loc_19519
; ---------------------------------------------------------------------------

loc_19568:                              ; CODE XREF: combat_party_turn+4D↑j
                sub     ax, 46h ; 'F'   ; switch 17 cases
                cmp     ax, 10h
                ja      short def_19573 ; jumptable 00019573 default case, cases 71-79,84
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_19573[bx] ; switch jump
; ---------------------------------------------------------------------------
jpt_19573       dw offset loc_1950C     ; DATA XREF: combat_party_turn+1C1↑r
                                        ; jump table for switch statement
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset loc_19520     ; jumptable 00019573 case 80
                dw offset loc_19528     ; jumptable 00019573 cases 81,86
                dw offset loc_19544     ; jumptable 00019573 case 82
                dw offset loc_1954A     ; jumptable 00019573 case 83
                dw offset def_19573     ; jumptable 00019573 default case, cases 71-79,84
                dw offset loc_19554     ; jumptable 00019573 case 85
                dw offset loc_19528     ; jumptable 00019573 cases 81,86
; ---------------------------------------------------------------------------

def_19573:                              ; CODE XREF: combat_party_turn:loc_1941E↑j
                                        ; combat_party_turn+93↑j ...
                cmp     [bp+var_2], 0   ; jumptable 00019573 default case, cases 71-79,84
                jnz     short loc_195A3
                jmp     loc_193DB
; ---------------------------------------------------------------------------

loc_195A3:                              ; CODE XREF: combat_party_turn+26↑j
                                        ; combat_party_turn+1EC↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_init_monster:                    ; CODE XREF: combat_party_turn+278↓p
                push    bp
                mov     bp, sp
                push    si
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                mov     byte ptr [si-607Ah], 0
                mov     bx, si
                shl     bx, 1
                mov     ax, word_27690
                mov     [bx-6056h], ax
                mov     al, byte_2767F
                mov     [si-606Eh], al
                mov     al, byte_27676
                mov     [si-6062h], al
                pop     si
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_init_monsters:                   ; CODE XREF: combat_party_turn+D40↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     byte ptr [bp+var_2], 0

loc_195E7:                              ; CODE XREF: combat_party_turn+25B↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     si, ax
                sub     al, al
                mov     [si-6062h], al
                mov     [si-606Eh], al
                mov     [si-607Ah], al
                mov     bx, si
                shl     bx, 1
                mov     word ptr [bx-6056h], 0
                inc     byte ptr [bp+var_2]
                cmp     byte ptr [bp+var_2], 0Bh
                jb      short loc_195E7
                mov     al, byte ptr word_1DD58
                mov     byte ptr [bp+var_4], al
                cmp     al, 0Ah
                jbe     short loc_1961D
                mov     byte ptr [bp+var_4], 0Bh

loc_1961D:                              ; CODE XREF: combat_party_turn+265↑j
                mov     byte ptr [bp+var_2], 0
                jmp     short loc_19633
; ---------------------------------------------------------------------------
                align 2

loc_19624:                              ; CODE XREF: combat_party_turn+287↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    combat_init_monster
                add     sp, 2
                inc     byte ptr [bp+var_2]

loc_19633:                              ; CODE XREF: combat_party_turn+26F↑j
                mov     al, byte ptr [bp+var_4]
                cmp     byte ptr [bp+var_2], al
                jb      short loc_19624
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_init_ranks:                      ; CODE XREF: combat_party_turn+D3D↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     byte_27815, 0
                jnz     short loc_19699
                cmp     g_outdoors, 0
                jnz     short loc_19670
                mov     ax, 27h ; '''
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                cwd
                mov     cx, 0Ah
                idiv    cx
                add     al, 3
                mov     byte ptr [bp+var_2], al
                jmp     short loc_19693
; ---------------------------------------------------------------------------
                align 2

loc_19670:                              ; CODE XREF: combat_party_turn+2A0↑j
                mov     ax, 45h ; 'E'
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                cwd
                mov     cx, 0Ah
                idiv    cx
                mov     cx, ax
                mov     ax, g_party_size
                cwd
                sub     ax, dx
                sar     ax, 1
                add     cl, al
                mov     byte ptr [bp+var_2], cl

loc_19693:                              ; CODE XREF: combat_party_turn+2BB↑j
                mov     al, byte ptr [bp+var_2]
                mov     byte_27815, al

loc_19699:                              ; CODE XREF: combat_party_turn+299↑j
                cmp     byte_1DC65, 2
                jnz     short loc_196A4
                shr     byte_27815, 1

loc_196A4:                              ; CODE XREF: combat_party_turn+2EC↑j
                cmp     byte_1DC65, 3
                jnz     short loc_196AF
                shl     byte_27815, 1

loc_196AF:                              ; CODE XREF: combat_party_turn+2F7↑j
                mov     al, byte ptr word_1DD58
                cmp     byte_27815, al
                jbe     short loc_196BB
                mov     byte_27815, al

loc_196BB:                              ; CODE XREF: combat_party_turn+304↑j
                cmp     byte_27815, 0Ah
                jbe     short loc_196C7
                mov     byte_27815, 0Ah

loc_196C7:                              ; CODE XREF: combat_party_turn+30E↑j
                cmp     g_outdoors, 0
                jnz     short loc_196EC
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                cwd
                mov     cx, 0Ah
                idiv    cx
                sub     ah, ah
                add     ax, 3
                shr     ax, 1
                jmp     short loc_19714
; ---------------------------------------------------------------------------
                align 2

loc_196EC:                              ; CODE XREF: combat_party_turn+31A↑j
                mov     al, byte ptr g_party_size
                mov     byte_22CED, al
                cmp     al, 6
                jb      short loc_19717
                mov     ax, 27h ; '''
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                cwd
                mov     cx, 0Ah
                idiv    cx
                sub     ah, ah
                shr     ax, 1
                add     al, byte ptr g_party_size
                sub     al, 2

loc_19714:                              ; CODE XREF: combat_party_turn+337↑j
                mov     byte_22CED, al

loc_19717:                              ; CODE XREF: combat_party_turn+342↑j
                cmp     byte_1DC65, 3
                jnz     short loc_19722
                shl     byte_22CED, 1

loc_19722:                              ; CODE XREF: combat_party_turn+36A↑j
                cmp     byte_1DC65, 2
                jnz     short loc_19739
                cmp     g_party_size, 2
                jge     short loc_19735
                mov     byte_22CED, 2

loc_19735:                              ; CODE XREF: combat_party_turn+37C↑j
                dec     byte_22CED

loc_19739:                              ; CODE XREF: combat_party_turn+375↑j
                mov     al, byte_22CED
                cmp     byte ptr g_party_size, al
                jnb     short loc_19748
                mov     al, byte ptr g_party_size
                mov     byte_22CED, al

loc_19748:                              ; CODE XREF: combat_party_turn+38E↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_party_strength:                  ; CODE XREF: combat_encounter+8F↓p
                push    bp
                mov     bp, sp
                sub     sp, 8
                mov     byte ptr [bp+var_2], 0
                sub     ax, ax
                mov     word_1E80C, ax
                mov     word_1E80A, ax
                mov     [bp+var_8], ax
                jmp     short loc_1979A
; ---------------------------------------------------------------------------
                align 2

loc_19764:                              ; CODE XREF: combat_party_turn+3EE↓j
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, ax
                mov     ax, [bx+74h]
                sub     dx, dx
                add     word_1E80A, ax
                adc     word_1E80C, dx
                mov     al, [bx+71h]
                sub     ah, ah
                shr     ax, 1
                mov     byte ptr [bp+var_6], al
                mov     al, byte ptr [bp+var_2]
                cmp     byte ptr [bp+var_6], al
                jbe     short loc_19797
                mov     al, byte ptr [bp+var_6]
                mov     byte ptr [bp+var_2], al

loc_19797:                              ; CODE XREF: combat_party_turn+3DD↑j
                inc     [bp+var_8]

loc_1979A:                              ; CODE XREF: combat_party_turn+3AF↑j
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jl      short loc_19764
                mov     al, 3
                push    ax
                mov     ax, 0FBAh
                push    ax
                call    thk_res_0140
                cmp     byte_1DC22, 0
                jnz     short loc_197BD
                mov     al, 2
                push    ax
                mov     ax, 0FBAh
                push    ax
                call    thk_res_0140

loc_197BD:                              ; CODE XREF: combat_party_turn+3FF↑j
                cmp     byte_1DC22, 1
                jnz     short loc_197CC
                shr     word_1E80C, 1
                rcr     word_1E80A, 1

loc_197CC:                              ; CODE XREF: combat_party_turn+410↑j
                cmp     byte_1DC22, 3
                jnz     short loc_197DB
                shl     word_1E80A, 1
                rcl     word_1E80C, 1

loc_197DB:                              ; CODE XREF: combat_party_turn+41F↑j
                mov     al, byte ptr [bp+var_2]
                mov     byte_1E812, al
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_generate_encounter:              ; CODE XREF: combat_party_turn:loc_199BA↓p
                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     bl, byte_1DC22
                sub     bh, bh
                mov     al, [bx+0FC4h]
                add     al, byte_1E812
                inc     al
                mov     byte ptr [bp+var_4], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                cmp     al, 3Dh ; '='
                jnb     short loc_1981A
                mov     byte ptr [bp+var_2], 0
                jmp     short loc_19836
; ---------------------------------------------------------------------------

loc_1981A:                              ; CODE XREF: combat_party_turn+460↑j
                cmp     byte ptr [bp+var_2], 51h ; 'Q'
                jnb     short loc_19826
                mov     byte ptr [bp+var_2], 1
                jmp     short loc_19836
; ---------------------------------------------------------------------------

loc_19826:                              ; CODE XREF: combat_party_turn+46C↑j
                cmp     byte ptr [bp+var_2], 60h ; '`'
                jnb     short loc_19832
                mov     byte ptr [bp+var_2], 2
                jmp     short loc_19836
; ---------------------------------------------------------------------------

loc_19832:                              ; CODE XREF: combat_party_turn+478↑j
                mov     byte ptr [bp+var_2], 3

loc_19836:                              ; CODE XREF: combat_party_turn+466↑j
                                        ; combat_party_turn+472↑j ...
                mov     al, byte ptr [bp+var_2]
                add     byte ptr [bp+var_4], al
                cmp     byte ptr [bp+var_4], 0Dh
                jbe     short loc_19846
                mov     byte ptr [bp+var_4], 0Eh

loc_19846:                              ; CODE XREF: combat_party_turn+48E↑j
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_4], al
                mov     al, byte_231E1
                cmp     byte ptr [bp+var_4], al
                jbe     short loc_19864
                mov     byte ptr [bp+var_4], al

loc_19864:                              ; CODE XREF: combat_party_turn+4AD↑j
                mov     al, byte_231E2
                cmp     byte ptr [bp+var_4], al
                jnb     short loc_1986F
                mov     byte ptr [bp+var_4], al

loc_1986F:                              ; CODE XREF: combat_party_turn+4B8↑j
                dec     byte ptr [bp+var_4]
                mov     cl, 4
                shl     byte ptr [bp+var_4], cl
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                dec     al
                add     byte ptr [bp+var_4], al
                mov     al, byte_26ED0
                mov     byte ptr [bp+var_6], al
                mov     al, byte ptr [bp+var_4]
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     al, byte ptr [bp+var_6]
                mov     byte_26ED0, al
                mov     al, byte_27679
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                cmp     byte ptr word_1DD58, 0Bh
                jnb     short loc_198DC

loc_198BF:                              ; CODE XREF: combat_party_turn+528↓j
                mov     bl, byte ptr word_1DD58
                inc     word_1DD58
                sub     bh, bh
                mov     al, byte ptr [bp+var_4]
                mov     [bx-6980h], al
                cmp     byte ptr word_1DD58, 0Bh
                jnb     short loc_198DC
                dec     byte ptr [bp+var_2]
                jnz     short loc_198BF

loc_198DC:                              ; CODE XREF: combat_party_turn+50B↑j
                                        ; combat_party_turn+523↑j
                cmp     byte ptr [bp+var_2], 0F0h
                jbe     short loc_198E6
                mov     byte ptr [bp+var_2], 0F0h

loc_198E6:                              ; CODE XREF: combat_party_turn+52E↑j
                mov     al, byte ptr [bp+var_2]
                add     byte ptr word_1DD58, al
                mov     al, byte ptr word_1DD58
                cmp     byte ptr [bp+var_2], al
                jbe     short loc_198FA
                mov     byte ptr word_1DD58, 0FAh

loc_198FA:                              ; CODE XREF: combat_party_turn+541↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_encounter_ok:                    ; CODE XREF: combat_party_turn:loc_199BD↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                mov     byte_1E813, 0
                mov     al, byte ptr word_1DD58
                mov     [bp+var_A], al
                cmp     al, 0Ah
                jbe     short loc_1991F
                mov     [bp+var_A], 0Ah

loc_1991F:                              ; CODE XREF: combat_party_turn+567↑j
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_19941
; ---------------------------------------------------------------------------
                align 2

loc_19926:                              ; CODE XREF: combat_party_turn+595↓j
                mov     bl, byte ptr [bp+var_8]
                sub     bh, bh
                mov     al, [bx-6980h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                inc     ax
                sub     dx, dx
                add     [bp+var_6], ax
                adc     [bp+var_4], dx
                inc     byte ptr [bp+var_8]

loc_19941:                              ; CODE XREF: combat_party_turn+571↑j
                mov     al, [bp+var_A]
                cmp     byte ptr [bp+var_8], al
                jb      short loc_19926
                cmp     byte ptr word_1DD58, 0Ah
                jbe     short loc_19984
                mov     al, byte ptr word_1DD58
                sub     al, 0Ah
                mov     [bp+var_A], al
                mov     al, byte_26EDA
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                inc     al
                mov     byte ptr [bp+var_2], al
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_1997C
; ---------------------------------------------------------------------------

loc_1996C:                              ; CODE XREF: combat_party_turn+5D0↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                sub     dx, dx
                add     [bp+var_6], ax
                adc     [bp+var_4], dx
                inc     byte ptr [bp+var_8]

loc_1997C:                              ; CODE XREF: combat_party_turn+5B8↑j
                mov     al, [bp+var_A]
                cmp     byte ptr [bp+var_8], al
                jb      short loc_1996C

loc_19984:                              ; CODE XREF: combat_party_turn+59C↑j
                mov     ax, word_1E80A
                mov     dx, word_1E80C
                cmp     [bp+var_4], dx
                jb      short loc_1999B
                ja      short loc_19997
                cmp     [bp+var_6], ax
                jb      short loc_1999B

loc_19997:                              ; CODE XREF: combat_party_turn+5DE↑j
                inc     byte_1E813

loc_1999B:                              ; CODE XREF: combat_party_turn+5DC↑j
                                        ; combat_party_turn+5E3↑j
                cmp     byte_1E813, 0
                jnz     short loc_199B3
                mov     al, byte ptr word_1DD58
                cmp     byte_231E0, al
                jbe     short loc_199AF
                cmp     al, 0FAh
                jb      short loc_199B3

loc_199AF:                              ; CODE XREF: combat_party_turn+5F7↑j
                inc     byte_1E813

loc_199B3:                              ; CODE XREF: combat_party_turn+5EE↑j
                                        ; combat_party_turn+5FB↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_build_encounter:                 ; CODE XREF: combat_encounter+99↓p
                jmp     short loc_199BD
; ---------------------------------------------------------------------------

loc_199BA:                              ; CODE XREF: combat_party_turn+613↓j
                call    combat_generate_encounter

loc_199BD:                              ; CODE XREF: combat_party_turn:combat_build_encounter↑j
                call    combat_encounter_ok
                cmp     byte_1E813, 0
                jz      short loc_199BA
                retn
; ---------------------------------------------------------------------------

combat_battle_number_text:              ; CODE XREF: combat_party_turn+98C↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                inc     g_battle_count
                mov     ax, g_battle_count
                cwd
                mov     cx, 0Ah
                idiv    cx
                mov     [bp+var_2], dx
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    g_battle_count
                call    thk_text_put_number_pad
                add     sp, 6
                cmp     [bp+var_2], 1
                jnz     short loc_19A02
                cmp     g_battle_count, 0Bh
                jz      short loc_19A02
                mov     ax, 1220h
                jmp     short loc_19A29
; ---------------------------------------------------------------------------

loc_19A02:                              ; CODE XREF: combat_party_turn+642↑j
                                        ; combat_party_turn+649↑j
                cmp     [bp+var_2], 2
                jnz     short loc_19A14
                cmp     g_battle_count, 0Ch
                jz      short loc_19A14
                mov     ax, 1223h
                jmp     short loc_19A29
; ---------------------------------------------------------------------------

loc_19A14:                              ; CODE XREF: combat_party_turn+654↑j
                                        ; combat_party_turn+65B↑j
                cmp     [bp+var_2], 3
                jnz     short loc_19A26
                cmp     g_battle_count, 0Dh
                jz      short loc_19A26
                mov     ax, 1226h
                jmp     short loc_19A29
; ---------------------------------------------------------------------------

loc_19A26:                              ; CODE XREF: combat_party_turn+666↑j
                                        ; combat_party_turn+66D↑j
                mov     ax, 1229h

loc_19A29:                              ; CODE XREF: combat_party_turn+64E↑j
                                        ; combat_party_turn+660↑j ...
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aBattle ; " battle."
                push    ax
                call    thk_text_puts
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_drop_treasure_item:              ; CODE XREF: combat_party_turn+831↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     byte ptr [bp+var_6], 0
                mov     byte ptr [bp+var_2], 0
                cmp     byte_22CE4, 2
                jbe     short loc_19A57
                mov     byte_22CE4, 2

loc_19A57:                              ; CODE XREF: combat_party_turn+69E↑j
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_4], al
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_19A71
; ---------------------------------------------------------------------------

loc_19A6E:                              ; CODE XREF: combat_party_turn+6D1↓j
                inc     byte ptr [bp+var_8]

loc_19A71:                              ; CODE XREF: combat_party_turn+6BA↑j
                cmp     byte ptr [bp+var_8], 7
                jnb     short loc_19A85
                mov     bl, byte ptr [bp+var_8]
                sub     bh, bh
                mov     al, byte ptr [bp+var_4]
                cmp     [bx+10EAh], al
                jb      short loc_19A6E

loc_19A85:                              ; CODE XREF: combat_party_turn+6C3↑j
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     si, ax
                mov     cl, 2
                shl     si, cl
                mov     al, [si+10F6h]
                mov     [bp+var_A], al
                mov     bl, byte_22CE4
                sub     bh, bh
                mov     al, [bx+si+10F7h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     [bp+var_A], al
                mov     al, 14h
                mul     [bp+var_A]
                add     ax, 6960h
                mov     [bp+var_C], ax
                mov     bl, [bp+arg_0]
                sub     bh, bh
                mov     al, [bp+var_A]
                mov     [bx+6950h], al
                mov     bx, [bp+var_C]
                cmp     byte ptr [bx+0Fh], 0
                jz      short loc_19ADC
                mov     bl, byte_22CE4
                sub     bh, bh
                mov     al, [bx+10F2h]
                mov     byte ptr [bp+var_6], al

loc_19ADC:                              ; CODE XREF: combat_party_turn+71B↑j
                mov     bx, [bp+var_C]
                cmp     byte ptr [bx+0Eh], 0F0h
                jnz     short loc_19AE8
                jmp     loc_19B6E
; ---------------------------------------------------------------------------

loc_19AE8:                              ; CODE XREF: combat_party_turn+731↑j
                cmp     byte_22CE7, 2
                jb      short loc_19B6E
                mov     ax, 7
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                cmp     byte_22CE7, 2
                jb      short loc_19B21
                cmp     byte_22CE7, 0Ch
                ja      short loc_19B21
                mov     al, byte_22CE7
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al

loc_19B21:                              ; CODE XREF: combat_party_turn+753↑j
                                        ; combat_party_turn+75A↑j
                cmp     byte_22CE7, 0Dh
                jnz     short loc_19B3B
                mov     ax, 15h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     al, 0Bh
                mov     byte ptr [bp+var_2], al

loc_19B3B:                              ; CODE XREF: combat_party_turn+774↑j
                cmp     byte ptr [bp+var_2], 5
                jb      short loc_19B6E
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_4], al
                cmp     al, 29h ; ')'
                jnb     short loc_19B5E
                or      byte ptr [bp+var_2], 80h
                jmp     short loc_19B6E
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_19B5E:                              ; CODE XREF: combat_party_turn+7A2↑j
                cmp     byte ptr [bp+var_4], 47h ; 'G'
                jnb     short loc_19B6A
                or      byte ptr [bp+var_2], 40h
                jmp     short loc_19B6E
; ---------------------------------------------------------------------------

loc_19B6A:                              ; CODE XREF: combat_party_turn+7B0↑j
                or      byte ptr [bp+var_2], 0C0h

loc_19B6E:                              ; CODE XREF: combat_party_turn+733↑j
                                        ; combat_party_turn+73B↑j ...
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                mov     al, byte ptr [bp+var_2]
                mov     [si+6953h], al
                mov     al, byte ptr [bp+var_6]
                mov     [si+6956h], al
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_treasure_roll:                   ; CODE XREF: combat_party_turn:loc_19C1B↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_2], al
                cmp     al, 0Bh
                jnb     short loc_19BAA
                mov     byte ptr [bp+var_2], 3
                jmp     short loc_19BC6
; ---------------------------------------------------------------------------
                align 2

loc_19BAA:                              ; CODE XREF: combat_party_turn+7EF↑j
                cmp     byte ptr [bp+var_2], 2Eh ; '.'
                jnb     short loc_19BB6
                mov     byte ptr [bp+var_2], 2
                jmp     short loc_19BC6
; ---------------------------------------------------------------------------

loc_19BB6:                              ; CODE XREF: combat_party_turn+7FC↑j
                cmp     byte ptr [bp+var_2], 5Bh ; '['
                jnb     short loc_19BC2
                mov     byte ptr [bp+var_2], 1
                jmp     short loc_19BC6
; ---------------------------------------------------------------------------

loc_19BC2:                              ; CODE XREF: combat_party_turn+808↑j
                mov     byte ptr [bp+var_2], 0

loc_19BC6:                              ; CODE XREF: combat_party_turn+7F5↑j
                                        ; combat_party_turn+802↑j ...
                cmp     byte ptr [bp+var_2], 0
                jz      short loc_19BF4
                mov     byte ptr [bp+var_4], 0
                jmp     short loc_19BEC
; ---------------------------------------------------------------------------

loc_19BD2:                              ; CODE XREF: combat_party_turn+840↓j
                cmp     byte_22CE4, 0
                jz      short loc_19BDD
                dec     byte_22CE4

loc_19BDD:                              ; CODE XREF: combat_party_turn+825↑j
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                push    ax
                call    combat_drop_treasure_item
                add     sp, 2
                inc     byte ptr [bp+var_4]

loc_19BEC:                              ; CODE XREF: combat_party_turn+81E↑j
                mov     al, byte ptr [bp+var_2]
                cmp     byte ptr [bp+var_4], al
                jb      short loc_19BD2

loc_19BF4:                              ; CODE XREF: combat_party_turn+818↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_victory:                         ; CODE XREF: combat_party_turn+ED8↓p
                push    bp              ; "Victory!", experience, treasure
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 0
                mov     byte ptr word_1DD58+1, 1
                mov     byte_1DC84, 0
                cmp     byte_22CE4, 0
                jnz     short loc_19C1B
                cmp     byte_22CE7, 0
                jz      short loc_19C1E

loc_19C1B:                              ; CODE XREF: combat_party_turn+860↑j
                call    combat_treasure_roll

loc_19C1E:                              ; CODE XREF: combat_party_turn+867↑j
                mov     [bp+var_6], 0
                jmp     short loc_19C3D
; ---------------------------------------------------------------------------
                align 2

loc_19C26:                              ; CODE XREF: combat_party_turn+891↓j
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_19C3A
                inc     [bp+var_4]

loc_19C3A:                              ; CODE XREF: combat_party_turn+883↑j
                inc     [bp+var_6]

loc_19C3D:                              ; CODE XREF: combat_party_turn+871↑j
                mov     ax, g_party_size
                cmp     [bp+var_6], ax
                jl      short loc_19C26
                sub     ax, ax
                push    ax
                push    [bp+var_4]
                mov     ax, 0FBEh
                push    ax
                call    thk_res_0160
                mov     [bp+var_6], 0
                jmp     short loc_19C7E
; ---------------------------------------------------------------------------
                align 2

loc_19C5A:                              ; CODE XREF: combat_party_turn+8D2↓j
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_19C7B
                mov     ax, word_1E80E
                mov     dx, word_1E810
                add     [bx+62h], ax
                adc     [bx+64h], dx

loc_19C7B:                              ; CODE XREF: combat_party_turn+8BA↑j
                inc     [bp+var_6]

loc_19C7E:                              ; CODE XREF: combat_party_turn+8A5↑j
                mov     ax, g_party_size
                cmp     [bp+var_6], ax
                jl      short loc_19C5A
                mov     ax, 0Dh
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 3
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     al, byte_1DB91
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 4
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_6], 0

loc_19CBB:                              ; CODE XREF: combat_party_turn+91A↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                inc     [bp+var_6]
                cmp     [bp+var_6], 17h
                jl      short loc_19CBB
                mov     ax, 0Ch
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_6], 0

loc_19CE1:                              ; CODE XREF: combat_party_turn+940↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                inc     [bp+var_6]
                cmp     [bp+var_6], 17h
                jl      short loc_19CE1
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 5
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aVictory ; "Victory!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 7
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYourPartyHasWo ; "Your party has won its"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                call    combat_battle_number_text
                mov     ax, 0Ah
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aEachSurvivorRe ; "Each survivor receives"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Bh
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_1E810
                push    word_1E80E
                call    thk_text_put_number
                add     sp, 8
                mov     ax, offset aExperienceP ; " experience p"
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word_1E810, 1
                jb      short loc_19D9E
                ja      short loc_19D98
                cmp     word_1E80E, 86A0h
                jb      short loc_19D9E

loc_19D98:                              ; CODE XREF: combat_party_turn+9DC↑j
                mov     ax, 127Ah
                jmp     short loc_19DA1
; ---------------------------------------------------------------------------
                align 2

loc_19D9E:                              ; CODE XREF: combat_party_turn+9DA↑j
                                        ; combat_party_turn+9E4↑j
                mov     ax, offset aOints ; "oints"

loc_19DA1:                              ; CODE XREF: combat_party_turn+9E9↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 3
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                call    thk_wait_key_timeout
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_draw_monster_line:               ; CODE XREF: combat_monster_advances+39D↑p
                                        ; ovl_2COMBAT:8663↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     byte ptr [bp+var_6], 0
                cmp     [bp+arg_0], 0Ah
                jbe     short loc_19DD3
                mov     [bp+arg_0], 0Ah

loc_19DD3:                              ; CODE XREF: combat_party_turn+A1B↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+arg_0], 0Ah
                jz      short loc_19DEF
                jmp     loc_19E7A
; ---------------------------------------------------------------------------

loc_19DEF:                              ; CODE XREF: combat_party_turn+A38↑j
                mov     al, byte ptr word_1DD58
                sub     al, 0Ah
                mov     byte ptr [bp+var_4], al
                mov     ax, 0Dh
                push    ax
                mov     ax, 10h
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
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     [bp+arg_0], 0
                call    thk_res_3E76
                cmp     byte ptr [bp+var_4], 1
                jz      short loc_19E6F
                mov     ax, 9E0Eh
                push    ax
                call    thk_strlen_trimmed
                add     sp, 2
                dec     al
                mov     byte ptr [bp+var_6], al
                cmp     byte ptr word_1DD58, 0Ah
                jbe     short loc_19E6F
                mov     bl, al
                sub     bh, bh
                cmp     byte ptr [bx-61F2h], 73h ; 's'
                jz      short loc_19E6F
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_19E6F:                              ; CODE XREF: combat_party_turn+A90↑j
                                        ; combat_party_turn+AA6↑j ...
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                jmp     loc_19F3C
; ---------------------------------------------------------------------------
                align 2

loc_19E7A:                              ; CODE XREF: combat_party_turn+A3A↑j
                mov     al, byte_27815
                cmp     [bp+arg_0], al
                jnb     short loc_19E88
                mov     ax, 17h
                jmp     short loc_19E8B
; ---------------------------------------------------------------------------
                align 2

loc_19E88:                              ; CODE XREF: combat_party_turn+ACE↑j
                mov     ax, 20h ; ' '

loc_19E8B:                              ; CODE XREF: combat_party_turn+AD3↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+41h]
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
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                cmp     byte ptr [si-607Ah], 0
                jnz     short loc_19ED6
                mov     ax, 9E0Eh
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset asc_1EAD3 ; "     "
                push    ax
                jmp     short loc_19F39
; ---------------------------------------------------------------------------
                align 2

loc_19ED6:                              ; CODE XREF: combat_party_turn+B11↑j
                call    thk_res_3E76
                mov     ax, 9E0Eh
                push    ax
                call    thk_strlen_trimmed
                add     sp, 2
                mov     byte ptr [bp+var_6], al
                jmp     short loc_19EF2
; ---------------------------------------------------------------------------

loc_19EE8:                              ; CODE XREF: combat_party_turn+B48↓j
                mov     ax, 2Eh ; '.'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_19EF2:                              ; CODE XREF: combat_party_turn+B34↑j
                mov     al, byte ptr [bp+var_6]
                inc     byte ptr [bp+var_6]
                cmp     al, 0Eh
                jb      short loc_19EE8
                mov     ax, 2Fh ; '/'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bl, [bp+arg_0]
                sub     bh, bh
                mov     al, [bx-607Ah]
                mov     byte ptr [bp+var_2], al
                mov     byte ptr [bp+var_6], bh
                jmp     short loc_19F1E
; ---------------------------------------------------------------------------
                align 2

loc_19F18:                              ; CODE XREF: combat_party_turn+B70↓j
                inc     byte ptr [bp+var_6]
                shl     byte ptr [bp+var_2], 1

loc_19F1E:                              ; CODE XREF: combat_party_turn+B63↑j
                cmp     byte ptr [bp+var_2], 80h
                jb      short loc_19F18
                cmp     byte ptr [bp+var_6], 7
                jbe     short loc_19F2E
                mov     byte ptr [bp+var_6], 7

loc_19F2E:                              ; CODE XREF: combat_party_turn+B76↑j
                mov     bl, byte ptr [bp+var_6]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+0FC8h]

loc_19F39:                              ; CODE XREF: combat_party_turn+B21↑j
                call    thk_text_puts

loc_19F3C:                              ; CODE XREF: combat_party_turn+AC4↑j
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_draw_party_hp:                   ; CODE XREF: combat_after_hit:loc_17EB0↑p
                                        ; combat_after_hit:loc_17F9A↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     byte ptr [bp+var_2], 1
                mov     [bp+var_8], 0
                jmp     loc_1A03D
; ---------------------------------------------------------------------------

loc_19F58:                              ; CODE XREF: combat_party_turn+CC5↓j
                mov     ax, 20h ; ' '

loc_19F5B:                              ; CODE XREF: combat_party_turn+CCB↓j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_8]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_19F8C
                mov     ax, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2

loc_19F8C:                              ; CODE XREF: combat_party_turn+BCE↑j
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_4]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 2Fh ; '/'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+var_4]
                mov     ax, [bx+5Eh]
                mov     [bp+var_6], ax
                cmp     ax, 3E8h
                jnb     short loc_19FFE
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [bx+5Eh]
                call    thk_text_put_number_pad
                add     sp, 6
                cmp     [bp+var_6], 64h ; 'd'
                jnb     short loc_19FEB
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_19FEB:                              ; CODE XREF: combat_party_turn+C2D↑j
                cmp     [bp+var_6], 0Ah
                jnb     short loc_1A029
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc

loc_19FF8:                              ; CODE XREF: combat_party_turn+C53↓j
                add     sp, 2
                jmp     short loc_1A029
; ---------------------------------------------------------------------------
                align 2

loc_19FFE:                              ; CODE XREF: combat_party_turn+C16↑j
                mov     ax, 1289h
                push    ax
                call    thk_text_puts
                jmp     short loc_19FF8
; ---------------------------------------------------------------------------
                align 2

loc_1A008:                              ; CODE XREF: combat_party_turn+C97↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     si, ax
                mov     ax, [bp+var_8]
                cwd
                sub     ax, dx
                sar     ax, 1
                mov     di, ax
                add     di, 13h
                push    di
                lea     ax, [si+12h]
                push    ax
                push    di
                push    si
                call    thk_clear_text_rect
                add     sp, 8

loc_1A029:                              ; CODE XREF: combat_party_turn+C3D↑j
                                        ; combat_party_turn+C49↑j
                cmp     byte ptr [bp+var_2], 1
                jnz     short loc_1A036
                mov     byte ptr [bp+var_2], 14h
                jmp     short loc_1A03A
; ---------------------------------------------------------------------------
                align 2

loc_1A036:                              ; CODE XREF: combat_party_turn+C7B↑j
                mov     byte ptr [bp+var_2], 1

loc_1A03A:                              ; CODE XREF: combat_party_turn+C81↑j
                inc     [bp+var_8]

loc_1A03D:                              ; CODE XREF: combat_party_turn+BA3↑j
                cmp     [bp+var_8], 8
                jge     short loc_1A080
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1A008
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     ax, [bp+var_8]
                cwd
                sub     ax, dx
                sar     ax, 1
                add     ax, 13h
                push    ax
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_22CED
                cmp     byte ptr [bp+var_8], al
                jb      short loc_1A07A
                jmp     loc_19F58
; ---------------------------------------------------------------------------

loc_1A07A:                              ; CODE XREF: combat_party_turn+CC3↑j
                mov     ax, 17h
                jmp     loc_19F5B
; ---------------------------------------------------------------------------

loc_1A080:                              ; CODE XREF: combat_party_turn+C8F↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

combat_draw_monster_list:               ; CODE XREF: combat_monster_waits+7C↑p
                                        ; combat_monster_advances+128↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte ptr [bp+var_2], 0
                jmp     short loc_1A0B1
; ---------------------------------------------------------------------------
                align 2

loc_1A094:                              ; CODE XREF: combat_party_turn+D0B↓j
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                mov     si, ax
                add     si, 3
                push    si
                mov     ax, 26h ; '&'
                push    ax
                push    si
                mov     ax, 10h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8

loc_1A0AE:                              ; CODE XREF: combat_party_turn+D19↓j
                inc     byte ptr [bp+var_2]

loc_1A0B1:                              ; CODE XREF: combat_party_turn+CDF↑j
                cmp     byte ptr [bp+var_2], 0Bh
                jnb     short loc_1A0CE
                mov     al, byte ptr word_1DD58
                cmp     byte ptr [bp+var_2], al
                jnb     short loc_1A094
                mov     al, byte ptr [bp+var_2]
                sub     ah, ah
                push    ax
                call    combat_draw_monster_line
                add     sp, 2
                jmp     short loc_1A0AE
; ---------------------------------------------------------------------------
                align 2

loc_1A0CE:                              ; CODE XREF: combat_party_turn+D03↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_battle_loop:                     ; CODE XREF: combat_encounter+467↓p
                                        ; combat_encounter:loc_1A734↓p
                push    bp              ; speed-ordered actions until victory/defeat/flight
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     byte ptr [bp+var_2], 0
                mov     g_view_mode, 2
                sub     al, al
                mov     byte_22CF6, al
                mov     byte_2781C, al
                call    combat_delay_prompt
                call    combat_init_ranks
                call    combat_init_monsters
                push    g_party_size
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte_2781F, al
                dec     byte_2781F
                mov     ax, 0Ch
                push    ax
                mov     ax, 10h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6

loc_1A11B:                              ; CODE XREF: combat_party_turn+ECE↓j
                mov     byte ptr [bp+var_C], 0
                mov     byte ptr [bp+var_8], 0

loc_1A123:                              ; CODE XREF: combat_party_turn+D81↓j
                mov     bl, byte ptr [bp+var_8]
                sub     bh, bh
                mov     [bx+5480h], bh
                inc     byte ptr [bp+var_8]
                cmp     byte ptr [bp+var_8], 0Ah
                jb      short loc_1A123
                mov     byte ptr [bp+var_8], bh

loc_1A138:                              ; CODE XREF: combat_party_turn+D96↓j
                mov     bl, byte ptr [bp+var_8]
                sub     bh, bh
                mov     [bx+548Ch], bh
                inc     byte ptr [bp+var_8]
                cmp     byte ptr [bp+var_8], 8
                jb      short loc_1A138
                mov     al, byte ptr word_1DD58
                mov     [bp+var_A], al
                cmp     al, 0Ah
                jbe     short loc_1A158
                mov     [bp+var_A], 0Ah

loc_1A158:                              ; CODE XREF: combat_party_turn+DA0↑j
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_1A19C
; ---------------------------------------------------------------------------

loc_1A15E:                              ; CODE XREF: combat_party_turn+DF0↓j
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     si, ax
                mov     al, [si-607Ah]
                and     al, 0FEh
                mov     byte ptr [bp+var_4], al
                cmp     al, ah
                jz      short loc_1A199
                mov     al, [si-6980h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte ptr [bp+var_6], al
                and     byte ptr [bp+var_6], 0FEh
                xor     byte ptr [bp+var_6], 0FEh
                mov     al, byte ptr [bp+var_4]
                and     byte ptr [bp+var_6], al
                mov     al, byte ptr [bp+var_6]
                mov     [si-607Ah], al

loc_1A199:                              ; CODE XREF: combat_party_turn+DBE↑j
                inc     byte ptr [bp+var_8]

loc_1A19C:                              ; CODE XREF: combat_party_turn+DAA↑j
                mov     al, [bp+var_A]
                cmp     byte ptr [bp+var_8], al
                jb      short loc_1A15E
                call    combat_draw_monster_list
                call    combat_draw_party_hp

loc_1A1AA:                              ; CODE XREF: combat_party_turn+EC1↓j
                sub     al, al
                mov     byte_27822, al
                mov     byte_22CDA, al
                mov     byte_27820, al
                mov     byte_22CF5, al
                mov     al, byte ptr word_1DD58
                mov     [bp+var_A], al
                cmp     al, 0Ah
                jbe     short loc_1A1C6
                mov     [bp+var_A], 0Ah

loc_1A1C6:                              ; CODE XREF: combat_party_turn+E0E↑j
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_1A1F2
; ---------------------------------------------------------------------------

loc_1A1CC:                              ; CODE XREF: combat_party_turn+E46↓j
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     si, ax
                cmp     [si+5480h], ah
                jnz     short loc_1A1EF
                mov     al, byte_22CF5
                cmp     [si-606Eh], al
                jbe     short loc_1A1EF
                mov     al, [si-606Eh]
                mov     byte_22CF5, al
                mov     al, byte ptr [bp+var_8]
                mov     byte_27820, al

loc_1A1EF:                              ; CODE XREF: combat_party_turn+E25↑j
                                        ; combat_party_turn+E2E↑j
                inc     byte ptr [bp+var_8]

loc_1A1F2:                              ; CODE XREF: combat_party_turn+E18↑j
                mov     al, [bp+var_A]
                cmp     byte ptr [bp+var_8], al
                jb      short loc_1A1CC
                mov     byte ptr [bp+var_8], 0
                jmp     short loc_1A233
; ---------------------------------------------------------------------------

loc_1A200:                              ; CODE XREF: combat_party_turn+E88↓j
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     si, ax
                cmp     [si+548Ch], ah
                jnz     short loc_1A230
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx+6Eh]
                mov     byte ptr [bp+var_4], al
                mov     al, byte_22CDA
                cmp     byte ptr [bp+var_4], al
                jbe     short loc_1A230
                mov     al, byte ptr [bp+var_4]
                mov     byte_22CDA, al
                mov     al, byte ptr [bp+var_8]
                mov     byte_27822, al

loc_1A230:                              ; CODE XREF: combat_party_turn+E59↑j
                                        ; combat_party_turn+E70↑j
                inc     byte ptr [bp+var_8]

loc_1A233:                              ; CODE XREF: combat_party_turn+E4C↑j
                mov     al, byte ptr [bp+var_8]
                cmp     byte ptr g_party_size, al
                ja      short loc_1A200
                cmp     byte_22CDA, 0
                jz      short loc_1A252
                mov     al, byte_22CF5
                cmp     byte_22CDA, al
                jb      short loc_1A252
                call    combat_party_turn
                jmp     short loc_1A261
; ---------------------------------------------------------------------------
                align 2

loc_1A252:                              ; CODE XREF: combat_party_turn+E8F↑j
                                        ; combat_party_turn+E98↑j
                cmp     byte_22CF5, 0
                jz      short loc_1A25E
                call    combat_monster_turn
                jmp     short loc_1A261
; ---------------------------------------------------------------------------

loc_1A25E:                              ; CODE XREF: combat_party_turn+EA5↑j
                inc     byte ptr [bp+var_C]

loc_1A261:                              ; CODE XREF: combat_party_turn+E9D↑j
                                        ; combat_party_turn+EAA↑j
                cmp     byte ptr [bp+var_C], 0
                jnz     short loc_1A26D
                call    combat_over_check
                mov     byte ptr [bp+var_C], al

loc_1A26D:                              ; CODE XREF: combat_party_turn+EB3↑j
                cmp     byte ptr [bp+var_C], 0
                jnz     short loc_1A276
                jmp     loc_1A1AA
; ---------------------------------------------------------------------------

loc_1A276:                              ; CODE XREF: combat_party_turn+EBF↑j
                call    combat_over_check
                mov     byte ptr [bp+var_2], al
                or      al, al
                jnz     short loc_1A283
                jmp     loc_1A11B
; ---------------------------------------------------------------------------

loc_1A283:                              ; CODE XREF: combat_party_turn+ECC↑j
                cmp     byte ptr word_1DD58, 0
                jnz     short loc_1A290
                call    combat_victory
                jmp     short loc_1A2A1
; ---------------------------------------------------------------------------
                align 2

loc_1A290:                              ; CODE XREF: combat_party_turn+ED6↑j
                cmp     byte_22CCE, 0
                jnz     short loc_1A29E
                cmp     byte_27818, 0
                jz      short loc_1A2A1

loc_1A29E:                              ; CODE XREF: combat_party_turn+EE3↑j
                call    combat_party_flees

loc_1A2A1:                              ; CODE XREF: combat_party_turn+EDB↑j
                                        ; combat_party_turn+EEA↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_party_turn endp


; =============== S U B R O U T I N E =======================================

; load monsters.dat, surprise, A-Attack B-Bribe H-Hide R-Run
; Attributes: bp-based frame

combat_encounter proc near              ; CODE XREF: seg002:0489↑J

var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    si
                mov     [bp+var_4], 1
                sub     al, al
                mov     byte_2781D, al
                mov     byte ptr word_1DD58+1, al
                mov     byte_22CE7, al
                mov     byte_22CE4, al
                mov     byte_22CCE, al
                mov     byte_22CED, al
                mov     byte_27815, al
                mov     byte_22CE8, al
                mov     byte_2781B, al
                mov     byte_2781A, al
                mov     byte_27819, al
                mov     byte_27814, al
                mov     byte_27818, al
                mov     byte_27811, al
                mov     byte_27810, al
                sub     ax, ax
                mov     word_1E810, ax
                mov     word_1E80E, ax
                mov     al, g_view_mode
                mov     [bp+var_6], al

loc_1A2EF:                              ; CODE XREF: combat_encounter+60↓j
                lea     ax, [bp+var_C]
                push    ax
                mov     ax, offset aMonstersDat_0 ; "monsters.dat"
                push    ax
                call    thk_load_file_alloc
                add     sp, 4
                mov     word ptr dword_1DD54, ax
                mov     word ptr dword_1DD54+2, dx
                or      dx, ax
                jz      short loc_1A2EF
                mov     [bp+var_2], 0
                jmp     short loc_1A311
; ---------------------------------------------------------------------------

loc_1A30E:                              ; CODE XREF: combat_encounter+7A↓j
                inc     [bp+var_2]

loc_1A311:                              ; CODE XREF: combat_encounter+66↑j
                cmp     [bp+var_2], 0Ah
                jnb     short loc_1A322
                mov     bl, [bp+var_2]
                sub     bh, bh
                cmp     [bx-6980h], bh
                jnz     short loc_1A30E

loc_1A322:                              ; CODE XREF: combat_encounter+6F↑j
                cmp     byte_26EDA, 0
                jz      short loc_1A32F
                mov     al, byte ptr word_1DD58
                add     [bp+var_2], al

loc_1A32F:                              ; CODE XREF: combat_encounter+81↑j
                mov     al, [bp+var_2]
                mov     byte ptr word_1DD58, al
                call    combat_party_strength
                cmp     byte_1DC65, 80h
                jnb     short loc_1A344
                call    combat_build_encounter
                jmp     short loc_1A349
; ---------------------------------------------------------------------------

loc_1A344:                              ; CODE XREF: combat_encounter+97↑j
                sub     byte_1DC65, 80h

loc_1A349:                              ; CODE XREF: combat_encounter+9C↑j
                cmp     byte ptr word_1DD58, 0
                jnz     short loc_1A367
                inc     byte ptr word_1DD58
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                dec     al
                mov     byte_26ED0, al

loc_1A367:                              ; CODE XREF: combat_encounter+A8↑j
                mov     al, byte ptr word_1DD58
                mov     [bp+var_2], al
                cmp     al, 0Ah
                jbe     short loc_1A375
                mov     [bp+var_2], 0Ah

loc_1A375:                              ; CODE XREF: combat_encounter+C9↑j
                call    thk_res_3A9E
                mov     byte_22CEE, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     al, byte_277D4
                cmp     byte_2767B, al
                jz      short loc_1A3A7
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     al, byte_2767B
                sub     ah, ah
                push    ax
                call    thk_monster_gfx_load
                add     sp, 2

loc_1A3A7:                              ; CODE XREF: combat_encounter+E5↑j
                cmp     byte ptr word_1DD58, 0Ah
                jbe     short loc_1A3B4
                mov     ax, 1
                jmp     short loc_1A3B6
; ---------------------------------------------------------------------------
                align 2

loc_1A3B4:                              ; CODE XREF: combat_encounter+106↑j
                sub     ax, ax

loc_1A3B6:                              ; CODE XREF: combat_encounter+10B↑j
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                mov     cl, [bp+var_2]
                sub     ch, ch
                add     ax, cx
                add     ax, 2
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 1
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_E], ax
                push    ax
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
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     [bp+var_8], 0
                jmp     short loc_1A430
; ---------------------------------------------------------------------------
                align 2

loc_1A40E:                              ; CODE XREF: combat_encounter+190↓j
                mov     al, [bp+var_8]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+1]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                call    thk_res_3E76
                inc     [bp+var_8]

loc_1A430:                              ; CODE XREF: combat_encounter+165↑j
                mov     al, [bp+var_2]
                cmp     [bp+var_8], al
                jb      short loc_1A40E
                cmp     byte ptr word_1DD58, 0Ah
                ja      short loc_1A442
                jmp     loc_1A4D9
; ---------------------------------------------------------------------------

loc_1A442:                              ; CODE XREF: combat_encounter+197↑j
                mov     al, byte ptr word_1DD58
                sub     al, 0Ah
                mov     [bp+var_2], al
                mov     ax, 0Ch
                push    ax
                mov     ax, 4
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
                mov     ax, offset aMore ; " more"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 0Ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                call    thk_res_3E76
                cmp     [bp+var_2], 1
                jz      short loc_1A4D9
                mov     ax, 9E0Eh
                push    ax
                call    thk_strlen_trimmed
                add     sp, 2
                dec     ax
                mov     [bp+var_C], ax
                mov     bx, ax
                mov     al, [bx-61F2h]
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                cmp     ax, 53h ; 'S'
                jz      short loc_1A4D9
                mov     word_221B4, 0
                mov     ax, 73h ; 's'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     word_221B4, 1

loc_1A4D9:                              ; CODE XREF: combat_encounter+199↑j
                                        ; combat_encounter+1F9↑j ...
                push    [bp+var_E]
                call    thk_text_window_close
                add     sp, 2
                mov     byte_2294F, 0FDh
                cmp     byte_1DC65, 0
                jnz     short loc_1A524
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_8], al
                cmp     al, 28h ; '('
                ja      short loc_1A512
                mov     al, byte_22CEE
                cmp     [bp+var_8], al
                ja      short loc_1A512
                mov     byte_1DC65, 2
                jmp     short loc_1A524
; ---------------------------------------------------------------------------

loc_1A512:                              ; CODE XREF: combat_encounter+25B↑j
                                        ; combat_encounter+263↑j
                cmp     g_fx_guard_dog, 0
                jnz     short loc_1A524
                cmp     [bp+var_8], 5Ah ; 'Z'
                jb      short loc_1A524
                mov     byte_1DC65, 3

loc_1A524:                              ; CODE XREF: combat_encounter+246↑j
                                        ; combat_encounter+26A↑j ...
                cmp     byte_1DC65, 0
                jz      short loc_1A550
                cmp     byte_1DC65, 2
                jnz     short loc_1A538
                mov     ax, offset aYouSurprisedTh ; "You surprised the monsters!"
                jmp     short loc_1A542
; ---------------------------------------------------------------------------
                align 2

loc_1A538:                              ; CODE XREF: combat_encounter+28A↑j
                cmp     byte_1DC65, 3
                jnz     short loc_1A549
                mov     ax, offset aTheMonstersSur ; "The monsters surprised you!"

loc_1A542:                              ; CODE XREF: combat_encounter+28F↑j
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_1A549:                              ; CODE XREF: combat_encounter+297↑j
                                        ; combat_encounter+2A8↓j
                call    thk_monster_anim_step
                or      ax, ax
                jz      short loc_1A549

loc_1A550:                              ; CODE XREF: combat_encounter+283↑j
                mov     al, byte_231E7
                mov     [bp+var_2], al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                cmp     byte_1DC65, 3
                jnz     short loc_1A569
                jmp     loc_1A734
; ---------------------------------------------------------------------------

loc_1A569:                              ; CODE XREF: combat_encounter+2BE↑j
                                        ; combat_encounter+488↓j
                mov     ax, offset aOptionsAAttack ; "Options: A-Attack B-Bribe H-Hide R-Run"
                push    ax
                call    thk_res_410A
                add     sp, 2
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_C], ax

loc_1A580:                              ; CODE XREF: seg002:06ED↑J
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_8], al
                cmp     [bp+var_C], 53h ; 'S'
                jnz     short loc_1A59C
                mov     [bp+var_C], 58h ; 'X'

loc_1A59C:                              ; CODE XREF: combat_encounter+2EF↑j
                cmp     [bp+var_C], 42h ; 'B'
                jz      short loc_1A5A5
                jmp     loc_1A6B8
; ---------------------------------------------------------------------------

loc_1A5A5:                              ; CODE XREF: combat_encounter+2FA↑j
                call    thk_res_5440
                mov     ax, offset aBribeWith1Food ; "Bribe with:  1-Food  2-Gold  3-Gems"
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_1A5B2:                              ; CODE XREF: combat_encounter+324↓j
                call    thk_monster_anim_step
                mov     [bp+var_C], ax
                cmp     ax, 1Bh
                jz      short loc_1A5CC
                cmp     ax, 31h ; '1'
                jz      short loc_1A5CC
                cmp     ax, 32h ; '2'
                jz      short loc_1A5CC
                cmp     ax, 33h ; '3'
                jnz     short loc_1A5B2

loc_1A5CC:                              ; CODE XREF: combat_encounter+315↑j
                                        ; combat_encounter+31A↑j ...
                cmp     [bp+var_C], 1Bh
                jz      short loc_1A5E9
                mov     ax, offset aHowMuch ; "How much? "
                push    ax
                call    thk_res_410A
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     [bp+var_4], ax

loc_1A5E9:                              ; CODE XREF: combat_encounter+32A↑j
                call    thk_res_35A8
                cmp     [bp+var_C], 1Bh
                jnz     short loc_1A5F5
                jmp     loc_1A6B8
; ---------------------------------------------------------------------------

loc_1A5F5:                              ; CODE XREF: combat_encounter+34A↑j
                cmp     [bp+var_4], 0
                jnz     short loc_1A5FE
                jmp     loc_1A6B8
; ---------------------------------------------------------------------------

loc_1A5FE:                              ; CODE XREF: combat_encounter+353↑j
                mov     al, byte ptr [bp+var_4]
                mov     [bp+var_A], al
                cmp     [bp+var_C], 32h ; '2'
                jnz     short loc_1A612
                mov     al, byte ptr [bp+var_4+1]
                add     [bp+var_A], al
                jmp     short loc_1A61F
; ---------------------------------------------------------------------------

loc_1A612:                              ; CODE XREF: combat_encounter+362↑j
                mov     al, byte_26ED0
                cmp     [bp+var_A], al
                jnb     short loc_1A61F
                mov     [bp+var_C], 41h ; 'A'

loc_1A61F:                              ; CODE XREF: combat_encounter+36A↑j
                                        ; combat_encounter+372↑j
                cmp     [bp+var_C], 31h ; '1'
                jnz     short loc_1A653
                mov     [bp+var_C], 41h ; 'A'
                cmp     byte_2766E, 0
                jz      short loc_1A653
                mov     al, [bp+var_2]
                cmp     [bp+var_8], al
                jb      short loc_1A653
                mov     al, [bp+var_A]
                sub     ah, ah
                push    ax
                call    thk_party_pay_food
                add     sp, 2
                or      ax, ax
                jz      short loc_1A653
                mov     [bp+var_C], 53h ; 'S'
                mov     byte ptr word_1DD58+1, 1

loc_1A653:                              ; CODE XREF: combat_encounter+37D↑j
                                        ; combat_encounter+389↑j ...
                cmp     [bp+var_C], 32h ; '2'
                jnz     short loc_1A687
                mov     [bp+var_C], 41h ; 'A'
                cmp     byte_2766F, 0
                jz      short loc_1A687
                mov     al, [bp+var_2]
                cmp     [bp+var_8], al
                jb      short loc_1A687
                sub     ax, ax
                push    ax
                push    [bp+var_4]
                call    thk_party_pay_gold
                add     sp, 4
                or      ax, ax
                jz      short loc_1A687
                mov     [bp+var_C], 53h ; 'S'
                mov     byte ptr word_1DD58+1, 1

loc_1A687:                              ; CODE XREF: combat_encounter+3B1↑j
                                        ; combat_encounter+3BD↑j ...
                cmp     [bp+var_C], 33h ; '3'
                jnz     short loc_1A6B8
                mov     [bp+var_C], 41h ; 'A'
                cmp     byte_27670, 0
                jz      short loc_1A6B8
                mov     al, [bp+var_2]
                cmp     [bp+var_8], al
                jb      short loc_1A6B8
                push    [bp+var_4]
                call    thk_party_pay_gems
                add     sp, 2
                or      ax, ax
                jz      short loc_1A6B8
                mov     [bp+var_C], 53h ; 'S'
                mov     byte ptr word_1DD58+1, 1

loc_1A6B8:                              ; CODE XREF: combat_encounter+2FC↑j
                                        ; combat_encounter+34C↑j ...
                cmp     [bp+var_C], 48h ; 'H'
                jnz     short loc_1A6D7
                mov     al, byte_22CEE
                cmp     [bp+var_8], al
                jnb     short loc_1A6D2
                mov     byte ptr word_1DD58+1, 1
                mov     [bp+var_C], 53h ; 'S'
                jmp     short loc_1A6D7
; ---------------------------------------------------------------------------

loc_1A6D2:                              ; CODE XREF: combat_encounter+41E↑j
                mov     [bp+var_C], 41h ; 'A'

loc_1A6D7:                              ; CODE XREF: combat_encounter+416↑j
                                        ; combat_encounter+42A↑j
                cmp     [bp+var_C], 52h ; 'R'
                jnz     short loc_1A707
                cmp     byte_1DC65, 2
                jz      short loc_1A6F8
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     al, byte_231E3
                jnb     short loc_1A702

loc_1A6F8:                              ; CODE XREF: combat_encounter+43C↑j
                call    combat_party_flees
                mov     [bp+var_C], 53h ; 'S'
                jmp     short loc_1A707
; ---------------------------------------------------------------------------

loc_1A702:                              ; CODE XREF: combat_encounter+450↑j
                mov     [bp+var_C], 41h ; 'A'

loc_1A707:                              ; CODE XREF: combat_encounter+435↑j
                                        ; combat_encounter+45A↑j
                cmp     [bp+var_C], 41h ; 'A'
                jnz     short loc_1A710
                call    combat_battle_loop

loc_1A710:                              ; CODE XREF: combat_encounter+465↑j
                cmp     [bp+var_C], 52h ; 'R'
                jz      short loc_1A737
                cmp     [bp+var_C], 41h ; 'A'
                jz      short loc_1A737
                cmp     [bp+var_C], 42h ; 'B'
                jz      short loc_1A737
                cmp     [bp+var_C], 48h ; 'H'
                jz      short loc_1A737
                cmp     [bp+var_C], 53h ; 'S'
                jz      short loc_1A731
                jmp     loc_1A569
; ---------------------------------------------------------------------------

loc_1A731:                              ; CODE XREF: combat_encounter+486↑j
                jmp     short loc_1A737
; ---------------------------------------------------------------------------
                align 2

loc_1A734:                              ; CODE XREF: combat_encounter+2C0↑j
                call    combat_battle_loop

loc_1A737:                              ; CODE XREF: combat_encounter+46E↑j
                                        ; combat_encounter+474↑j ...
                cmp     [bp+var_C], 53h ; 'S'
                jnz     short loc_1A751
                mov     ax, offset aSuccess ; "Success!"
                push    ax
                call    thk_res_410A
                add     sp, 2
                mov     ax, 19h
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2

loc_1A751:                              ; CODE XREF: combat_encounter+495↑j
                mov     al, [bp+var_6]
                mov     g_view_mode, al
                push    word ptr dword_1DD54+2
                push    word ptr dword_1DD54
                call    thk_free_far_block
                add     sp, 4
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     byte_277D4, 0FFh
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

combat_over_check:                      ; CODE XREF: combat_party_turn+EB5↑p
                                        ; combat_party_turn:loc_1A276↑p
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     [bp+var_2], 1
                cmp     byte ptr word_1DD58, 0
                jnz     short loc_1A794

loc_1A78F:                              ; CODE XREF: combat_encounter+4F3↓j
                                        ; combat_encounter+4FA↓j
                mov     ax, 1
                jmp     short loc_1A7D4
; ---------------------------------------------------------------------------

loc_1A794:                              ; CODE XREF: combat_encounter+4E7↑j
                cmp     g_party_size, 0
                jz      short loc_1A78F
                cmp     byte_27818, 0
                jnz     short loc_1A78F
                mov     [bp+var_4], 0
                jmp     short loc_1A7AD
; ---------------------------------------------------------------------------
                align 2

loc_1A7AA:                              ; CODE XREF: combat_encounter+527↓j
                inc     [bp+var_4]

loc_1A7AD:                              ; CODE XREF: combat_encounter+501↑j
                mov     ax, g_party_size
                cmp     [bp+var_4], ax
                jge     short loc_1A7CF
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 10h
                ja      short loc_1A7C9
                dec     [bp+var_2]

loc_1A7C9:                              ; CODE XREF: combat_encounter+51E↑j
                cmp     [bp+var_2], 0
                jnz     short loc_1A7AA

loc_1A7CF:                              ; CODE XREF: combat_encounter+50D↑j
                mov     al, [bp+var_2]
                sub     ah, ah

loc_1A7D4:                              ; CODE XREF: combat_encounter+4EC↑j
                mov     sp, bp
                pop     bp
                retn
combat_encounter endp


; =============== S U B R O U T I N E =======================================


sub_1A7D8       proc near               ; CODE XREF: seg002:0495↑J
                                        ; combat_after_hit+158↑p ...
                call    thk_kbd_flush
                mov     al, 19h
                mul     byte_1DC23
                inc     ax
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                retn
sub_1A7D8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A7EA       proc near               ; CODE XREF: seg002:05B5↑J

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_6], 0Ah
                mov     al, byte ptr word_1DD58
                mov     [bp+var_2], al
                cmp     al, 0Ah
                jbe     short loc_1A802
                mov     [bp+var_2], 0Ah

loc_1A802:                              ; CODE XREF: sub_1A7EA+12↑j
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_4], al
                cmp     al, 7
                jnb     short loc_1A818
                add     al, 4
                mov     [bp+var_6], al
                jmp     short loc_1A823
; ---------------------------------------------------------------------------
                align 2

loc_1A818:                              ; CODE XREF: sub_1A7EA+24↑j
                mov     al, [bp+var_2]
                cmp     [bp+var_4], al
                jb      short loc_1A823
                inc     [bp+var_6]

loc_1A823:                              ; CODE XREF: sub_1A7EA+2B↑j
                                        ; sub_1A7EA+34↑j
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     sp, bp
                pop     bp
                retn
sub_1A7EA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A82C       proc near               ; CODE XREF: seg002:05C1↑J

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     word_27816, 0
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_2], al
                mov     [bp+var_6], 0
                jmp     short loc_1A875
; ---------------------------------------------------------------------------

loc_1A848:                              ; CODE XREF: sub_1A82C+4F↓j
                mov     al, [bp+arg_0]
                mov     [bp+var_4], al
                or      al, al
                jz      short loc_1A862
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al

loc_1A862:                              ; CODE XREF: sub_1A82C+24↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cl, [bp+arg_2]
                sub     ch, ch
                add     ax, cx
                add     word_27816, ax
                inc     [bp+var_6]

loc_1A875:                              ; CODE XREF: sub_1A82C+1A↑j
                mov     al, [bp+var_2]
                cmp     [bp+var_6], al
                jb      short loc_1A848
                mov     sp, bp
                pop     bp
                retn
sub_1A82C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Protection Spells", "Magic", "Forces", "Cursed -"
; Attributes: bp-based frame

combat_show_protection proc near        ; CODE XREF: combat_party_turn:loc_19520↑p

var_8           = word ptr -8
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     [bp+var_2], 5
                mov     ax, 0Dh
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 3
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 3
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aProtectionSpel ; "Protection Spells"
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     di, di
                mov     si, 136Ch
                mov     [bp+var_8], 1376h

loc_1A8C6:                              ; CODE XREF: combat_show_protection+8E↓j
                mov     bx, [si]
                cmp     byte ptr [bx], 0
                jz      short loc_1A905
                mov     al, [bp+var_2]
                inc     [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+var_8]
                push    word ptr [bx]
                call    thk_text_puts
                add     sp, 2
                cmp     di, 4
                jnz     short loc_1A905
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [si]
                mov     al, [bx]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1A905:                              ; CODE XREF: combat_show_protection+49↑j
                                        ; combat_show_protection+6C↑j
                add     si, 2
                add     [bp+var_8], 2
                inc     di
                cmp     di, 5
                jl      short loc_1A8C6
                mov     [bp+var_4], di
                mov     al, [bp+var_2]
                inc     [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMagic_0 ; "Magic  "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, g_fx_magic
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 25h ; '%'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, [bp+var_2]
                inc     [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aForces ; "Forces "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, g_fx_forces
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 25h ; '%'
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     byte_1DC2B, 0
                jz      short loc_1A9CD
                mov     al, [bp+var_2]
                inc     [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCursed_0 ; "Cursed -"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte_1DC2B
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, offset aToAttacks ; " to Attacks!"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1A9CD:                              ; CODE XREF: combat_show_protection+10E↑j
                call    thk_res_5440

loc_1A9D0:                              ; CODE XREF: combat_show_protection+154↓j
                call    thk_monster_anim_step
                cmp     ax, 1Bh
                jnz     short loc_1A9D0
                call    thk_res_35A8
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
combat_show_protection endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "D-Delay P-Prot Q-Quick"
; Attributes: bp-based frame

combat_delay_prompt proc near           ; CODE XREF: combat_party_turn+D3A↑p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 1
                push    ax
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 0Eh
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                call    thk_draw_frame_hline
                add     sp, 6
                mov     ax, 0Fh
                push    ax
                mov     ax, 0Eh
                push    ax
                sub     ax, ax
                push    ax
                call    thk_draw_frame_vline
                add     sp, 6
                mov     ax, 2
                push    ax
                mov     ax, 27h ; '''
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_draw_frame_hline
                add     sp, 6
                mov     ax, 10h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 0Bh
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 0Bh
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 9
                push    ax
                mov     ax, 27h ; '''
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 0Bh
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDDelayPProtQQu ; "D-Delay P-Prot Q-Quick"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 0Eh
                push    ax
                mov     ax, 1
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 80h
                push    ax
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
                push    [bp+var_2]
                call    thk_text_window_close
                mov     sp, bp
                pop     bp
                retn
combat_delay_prompt endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AB02       proc near               ; CODE XREF: combat_party_turn+F2↑p
                                        ; ovl_2COMBAT:BB7D↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 0
                sub     cx, cx
                mov     si, 13D0h
                mov     dx, [bp+arg_0]

loc_1AB16:                              ; CODE XREF: sub_1AB02+27↓j
                cmp     [si], dx
                jnz     short loc_1AB20

loc_1AB1A:                              ; CODE XREF: sub_1AB02+25↓j
                mov     [bp+var_4], cx
                jmp     short loc_1AB2C
; ---------------------------------------------------------------------------
                align 2

loc_1AB20:                              ; CODE XREF: sub_1AB02+16↑j
                add     si, 2
                inc     cx
                cmp     cx, 0Ch
                jge     short loc_1AB1A
                jmp     short loc_1AB16
; ---------------------------------------------------------------------------
                align 2

loc_1AB2C:                              ; CODE XREF: sub_1AB02+1B↑j
                cmp     [bp+var_4], 0Ch
                jz      short loc_1AB35
                inc     [bp+var_2]

loc_1AB35:                              ; CODE XREF: sub_1AB02+2E↑j
                mov     ax, [bp+var_2]
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1AB02       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AB3E       proc near               ; CODE XREF: sub_1AE50+26↓p
                                        ; sub_1AE50+67↓p ...

arg_0           = word ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                mov     word_22CFA, 1
                mov     bx, [bp+arg_0]
                mov     al, [bp+arg_2]
                cmp     [bx], al
                jb      short loc_1AB56
                sub     [bx], al
                jmp     short loc_1AB67
; ---------------------------------------------------------------------------
                align 2

loc_1AB56:                              ; CODE XREF: sub_1AB3E+11↑j
                mov     byte ptr [bx], 0
                mov     bx, word_22CFD+1
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1AB67
                mov     byte ptr [bx+26h], 81h

loc_1AB67:                              ; CODE XREF: sub_1AB3E+15↑j
                                        ; sub_1AB3E+23↑j
                pop     bp
                retn
sub_1AB3E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AB6A       proc near               ; CODE XREF: sub_1ABB4+9↓p
                                        ; sub_1ABDC+16↓p ...

var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                cmp     [bp+arg_0], 0
                jnz     short loc_1AB7C
                inc     word_22CFA
                jmp     short loc_1ABAF
; ---------------------------------------------------------------------------

loc_1AB7C:                              ; CODE XREF: sub_1AB6A+A↑j
                sub     [bp+arg_0], 16h
                mov     ax, [bp+arg_0]
                add     ax, word_22CFD+1
                add     ax, 16h
                mov     [bp+var_2], ax
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                mov     bx, [bp+var_2]
                cmp     [bx], al
                ja      short loc_1ABAA
                mov     ax, 1
                jmp     short loc_1ABAC
; ---------------------------------------------------------------------------

loc_1ABAA:                              ; CODE XREF: sub_1AB6A+39↑j
                sub     ax, ax

loc_1ABAC:                              ; CODE XREF: sub_1AB6A+3E↑j
                mov     word_22CFA, ax

loc_1ABAF:                              ; CODE XREF: sub_1AB6A+10↑j
                mov     sp, bp
                pop     bp
                retn
sub_1AB6A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1ABB4       proc near               ; CODE XREF: sub_1AC96+8↓p
                                        ; sub_1ACA6+8↓p ...

arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1ABDA
                mov     bx, word_22CFD+1
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1ABDA
                mov     al, [bp+arg_0]
                or      [bx+26h], al

loc_1ABDA:                              ; CODE XREF: sub_1ABB4+14↑j
                                        ; sub_1ABB4+1E↑j
                pop     bp
                retn
sub_1ABB4       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1ABDC       proc near               ; CODE XREF: sub_1ADF2+2A↓p
                                        ; combat_apply_touch_effect+2B↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     bl, byte_2781F
                sub     bh, bh
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1AC50
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1AC50
                cmp     [bp+arg_0], 0
                jz      short loc_1AC14
                mov     bx, word_22CFD+1
                sub     ax, ax
                mov     [bx+68h], ax
                mov     [bx+66h], ax
                jmp     short loc_1AC50
; ---------------------------------------------------------------------------
                align 2

loc_1AC14:                              ; CODE XREF: sub_1ABDC+27↑j
                mov     bx, word_22CFD+1
                cmp     word ptr [bx+68h], 1
                jb      short loc_1AC28
                sub     word ptr [bx+66h], 0
                sbb     word ptr [bx+68h], 1
                jmp     short loc_1AC50
; ---------------------------------------------------------------------------

loc_1AC28:                              ; CODE XREF: sub_1ABDC+40↑j
                cmp     word ptr [bx+68h], 0
                jnz     short loc_1AC35
                cmp     word ptr [bx+66h], 100h
                jb      short loc_1AC3C

loc_1AC35:                              ; CODE XREF: sub_1ABDC+50↑j
                sub     word ptr [bx+66h], 100h
                jmp     short loc_1AC4C
; ---------------------------------------------------------------------------

loc_1AC3C:                              ; CODE XREF: sub_1ABDC+57↑j
                cmp     word ptr [bx+68h], 0
                jnz     short loc_1AC48
                cmp     word ptr [bx+66h], 0
                jz      short loc_1AC50

loc_1AC48:                              ; CODE XREF: sub_1ABDC+64↑j
                sub     word ptr [bx+66h], 1

loc_1AC4C:                              ; CODE XREF: sub_1ABDC+5E↑j
                sbb     word ptr [bx+68h], 0

loc_1AC50:                              ; CODE XREF: sub_1ABDC+10↑j
                                        ; sub_1ABDC+21↑j ...
                pop     bp
                retn
sub_1ABDC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AC52       proc near               ; CODE XREF: sub_1ADF2+1A↓p
                                        ; combat_apply_touch_effect+37↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1AC93
                cmp     [bp+arg_0], 0
                jz      short loc_1AC78
                mov     bx, word_22CFD+1
                mov     word ptr [bx+5Ch], 0
                jmp     short loc_1AC93
; ---------------------------------------------------------------------------
                align 2

loc_1AC78:                              ; CODE XREF: sub_1AC52+18↑j
                mov     bx, word_22CFD+1
                cmp     word ptr [bx+5Ch], 100h
                jb      short loc_1AC8A
                sub     word ptr [bx+5Ch], 100h
                jmp     short loc_1AC93
; ---------------------------------------------------------------------------

loc_1AC8A:                              ; CODE XREF: sub_1AC52+2F↑j
                cmp     word ptr [bx+5Ch], 0
                jz      short loc_1AC93
                dec     word ptr [bx+5Ch]

loc_1AC93:                              ; CODE XREF: sub_1AC52+12↑j
                                        ; sub_1AC52+23↑j ...
                pop     bp
                retn
sub_1AC52       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AC96       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B01E↓p
                mov     ax, 1Ch
                push    ax
                mov     ax, 8
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1AC96       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACA6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B024↓p
                mov     ax, 1Ch
                push    ax
                mov     ax, 4
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACA6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACB6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B02A↓p
                mov     ax, 1Bh
                push    ax
                mov     ax, 10h
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACB6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACC6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B030↓p
                mov     ax, 16h
                push    ax
                mov     ax, 1
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACC6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACD6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B036↓p
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACD6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACE6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B03C↓p
                mov     ax, 1Ch
                push    ax
                mov     ax, 20h ; ' '
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACE6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1ACF6       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B042↓p
                mov     ax, 1Ch
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                call    sub_1ABB4
                add     sp, 4
                retn
sub_1ACF6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AD06       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B048↓p
                mov     bx, word_22CFD+1
                mov     byte ptr [bx+26h], 81h
                retn
sub_1AD06       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AD10       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B04E↓p
                mov     bx, word_22CFD+1
                mov     byte ptr [bx+26h], 82h
                retn
sub_1AD10       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AD1A       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B054↓p
                mov     bx, word_22CFD+1
                mov     byte ptr [bx+26h], 0FFh
                retn
sub_1AD1A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AD24       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B05A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1AD80
                mov     [bp+var_2], 0
                jmp     short loc_1AD47
; ---------------------------------------------------------------------------
                align 2

loc_1AD44:                              ; CODE XREF: sub_1AD24+34↓j
                inc     [bp+var_2]

loc_1AD47:                              ; CODE XREF: sub_1AD24+1D↑j
                cmp     [bp+var_2], 6
                jge     short loc_1AD5A
                mov     si, [bp+var_2]
                mov     bx, word_22CFD+1
                cmp     byte ptr [bx+si+3Ah], 0
                jz      short loc_1AD44

loc_1AD5A:                              ; CODE XREF: sub_1AD24+27↑j
                cmp     [bp+var_2], 6
                jz      short loc_1AD80
                mov     si, [bp+var_2]
                add     si, word_22CFD+1
                mov     byte ptr [si+3Ah], 0
                mov     byte ptr [si+46h], 0
                mov     byte ptr [si+40h], 0
                push    [bp+var_2]
                push    word_22CFD+1
                call    thk_res_3766
                add     sp, 4

loc_1AD80:                              ; CODE XREF: sub_1AD24+16↑j
                                        ; sub_1AD24+3A↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1AD24       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AD86       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B060↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1ADBF
                mov     [bp+var_2], 0

loc_1ADA3:                              ; CODE XREF: sub_1AD86+37↓j
                mov     si, [bp+var_2]
                add     si, word_22CFD+1
                mov     byte ptr [si+3Ah], 0
                mov     byte ptr [si+46h], 0
                mov     byte ptr [si+40h], 0
                inc     [bp+var_2]
                cmp     [bp+var_2], 6
                jl      short loc_1ADA3

loc_1ADBF:                              ; CODE XREF: sub_1AD86+16↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1AD86       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1ADC4       proc near               ; CODE XREF: sub_1ADF2+A↓p
                                        ; combat_apply_touch_effect+87↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1ADEF
                cmp     [bp+arg_0], 0
                jz      short loc_1ADE8
                mov     bx, word_22CFD+1
                mov     byte ptr [bx+25h], 0
                jmp     short loc_1ADEF
; ---------------------------------------------------------------------------

loc_1ADE8:                              ; CODE XREF: sub_1ADC4+18↑j
                mov     bx, word_22CFD+1
                shr     byte ptr [bx+25h], 1

loc_1ADEF:                              ; CODE XREF: sub_1ADC4+12↑j
                                        ; sub_1ADC4+22↑j
                pop     bp
                retn
sub_1ADC4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1ADF2       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B080↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     ax, 1
                push    ax
                call    sub_1ADC4
                add     sp, 2
                mov     ax, word_22CFA
                mov     [bp+var_2], ax
                mov     ax, 1
                push    ax
                call    sub_1AC52
                add     sp, 2
                mov     ax, word_22CFA
                mov     [bp+var_4], ax
                mov     ax, 1
                push    ax
                call    sub_1ABDC
                mov     ax, [bp+var_2]
                or      word_22CFA, ax
                mov     ax, [bp+var_4]
                or      word_22CFA, ax
                mov     sp, bp
                pop     bp
                retn
sub_1ADF2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AE32       proc near               ; CODE XREF: combat_apply_touch_effect+A8↓p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                mov     bx, word_22CFD+1
                mov     al, [bp+arg_0]
                add     [bx+21h], al
                cmp     byte ptr [bx+21h], 0C8h
                jbe     short loc_1AE49
                mov     byte ptr [bx+21h], 0C8h

loc_1AE49:                              ; CODE XREF: sub_1AE32+11↑j
                inc     word_22CFA
                pop     bp
                retn
sub_1AE32       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AE50       proc near               ; CODE XREF: combat_apply_touch_effect+B7↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                cmp     [bp+arg_0], 2
                jnz     short loc_1AE7E
                mov     ax, 1
                push    ax
                mov     ax, 6
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, word_22CFD+1
                add     ax, 0Fh
                push    ax
                call    sub_1AB3E
                add     sp, 4
                jmp     short loc_1AEC6
; ---------------------------------------------------------------------------

loc_1AE7E:                              ; CODE XREF: sub_1AE50+A↑j
                mov     [bp+var_6], 0

loc_1AE83:                              ; CODE XREF: sub_1AE50+74↓j
                mov     [bp+var_2], 2
                mov     bx, [bp+var_6]
                shl     bx, 1
                mov     ax, [bx+13E8h]
                add     ax, word_22CFD+1
                add     ax, 6Bh ; 'k'
                mov     [bp+var_4], ax
                cmp     [bp+arg_0], 1
                jnz     short loc_1AEAE
                mov     bx, ax
                mov     al, [bx]
                mov     [bp+var_2], al
                cmp     al, 2
                jb      short loc_1AEAE
                shr     [bp+var_2], 1

loc_1AEAE:                              ; CODE XREF: sub_1AE50+4E↑j
                                        ; sub_1AE50+59↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                push    [bp+var_4]
                call    sub_1AB3E
                add     sp, 4
                inc     [bp+var_6]
                cmp     [bp+var_6], 7
                jl      short loc_1AE83

loc_1AEC6:                              ; CODE XREF: sub_1AE50+2C↑j
                mov     sp, bp
                pop     bp
                retn
sub_1AE50       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AECA       proc near               ; CODE XREF: combat_apply_touch_effect+CD↓p

var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 2
                cmp     [bp+arg_0], 0
                jz      short loc_1AEEF
                mov     bx, word_22CFD+1
                mov     al, [bx+71h]
                sub     ah, ah
                shr     ax, 1
                mov     [bp+var_2], al
                or      al, al
                jnz     short loc_1AEEF
                inc     [bp+var_2]

loc_1AEEF:                              ; CODE XREF: sub_1AECA+E↑j
                                        ; sub_1AECA+20↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, word_22CFD+1
                add     ax, 71h ; 'q'
                push    ax
                call    sub_1AB3E
                mov     sp, bp
                pop     bp
                retn
sub_1AECA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AF04       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B0BC↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     ax, 50h ; 'P'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 14h
                cwd
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                mov     bx, word_22CFD+1
                cmp     [bx+64h], dx
                ja      short loc_1AF3C
                jb      short loc_1AF32
                cmp     [bx+62h], ax
                ja      short loc_1AF3C

loc_1AF32:                              ; CODE XREF: sub_1AF04+27↑j
                sub     ax, ax
                mov     [bx+64h], ax
                mov     [bx+62h], ax
                jmp     short loc_1AF48
; ---------------------------------------------------------------------------

loc_1AF3C:                              ; CODE XREF: sub_1AF04+25↑j
                                        ; sub_1AF04+2C↑j
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                sub     [bx+62h], ax
                sbb     [bx+64h], dx

loc_1AF48:                              ; CODE XREF: sub_1AF04+36↑j
                inc     word_22CFA
                mov     sp, bp
                pop     bp
                retn
sub_1AF04       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1AF50       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B0C2↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short loc_1AF9A
                mov     [bp+var_2], 0

loc_1AF6D:                              ; CODE XREF: sub_1AF50+48↓j
                mov     si, [bp+var_2]
                add     si, word_22CFD+1
                add     si, 3Ah ; ':'
                cmp     byte ptr [si], 0
                jz      short loc_1AF91
                cmp     byte ptr [si], 0D0h
                jnb     short loc_1AF91
                mov     ax, 0CFh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [si], al

loc_1AF91:                              ; CODE XREF: sub_1AF50+2A↑j
                                        ; sub_1AF50+2F↑j
                inc     [bp+var_2]
                cmp     [bp+var_2], 6
                jl      short loc_1AF6D

loc_1AF9A:                              ; CODE XREF: sub_1AF50+16↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1AF50       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AFA0       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B0C8↓p
                mov     ax, 1Ah
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short locret_1AFBA
                mov     bx, word_22CFD+1
                mov     word ptr [bx+58h], 0

locret_1AFBA:                           ; CODE XREF: sub_1AFA0+F↑j
                retn
sub_1AFA0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1AFBC       proc near               ; CODE XREF: combat_apply_touch_effect:loc_1B0CE↓p
                mov     ax, 1Eh
                push    ax
                call    sub_1AB6A
                add     sp, 2
                cmp     word_22CFA, 0
                jz      short locret_1AFE0
                mov     bx, word_22CFD+1
                mov     word ptr [bx+5Eh], 0
                cmp     byte ptr [bx+26h], 80h
                jnb     short locret_1AFE0
                or      byte ptr [bx+26h], 40h

locret_1AFE0:                           ; CODE XREF: sub_1AFBC+F↑j
                                        ; sub_1AFBC+1E↑j
                retn
sub_1AFBC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; (char) apply the monster's touch effect
; Attributes: bp-based frame

combat_apply_touch_effect proc near     ; CODE XREF: combat_after_hit+109↑p

var_12          = byte ptr -12h
var_10          = byte ptr -10h
var_E           = byte ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

; FUNCTION CHUNK AT B1EB SIZE 00000030 BYTES
; FUNCTION CHUNK AT B21C SIZE 0000000A BYTES

                push    bp
                mov     bp, sp
                mov     ax, [bp+arg_0]
                mov     word_22CFD+1, ax
                mov     word_22CFA, 0
                mov     al, byte_27677
                sub     ah, ah
                sub     ax, 1           ; switch 17 cases
                cmp     ax, 1Dh
                jbe     short loc_1B001
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B001:                              ; CODE XREF: combat_apply_touch_effect+1A↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1B004[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1B00A:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                sub     ax, ax          ; jumptable 0001B004 case 1

loc_1B00C:                              ; CODE XREF: combat_apply_touch_effect+95↓j
                push    ax
                call    sub_1ABDC

loc_1B010:                              ; CODE XREF: combat_apply_touch_effect+3A↓j
                                        ; combat_apply_touch_effect+8A↓j ...
                add     sp, 2
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B016:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                sub     ax, ax          ; jumptable 0001B004 case 2

loc_1B018:                              ; CODE XREF: combat_apply_touch_effect+9B↓j
                push    ax
                call    sub_1AC52
                jmp     short loc_1B010
; ---------------------------------------------------------------------------

loc_1B01E:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AC96       ; jumptable 0001B004 case 3
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B024:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACA6       ; jumptable 0001B004 case 4
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B02A:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACB6       ; jumptable 0001B004 case 5
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B030:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACC6       ; jumptable 0001B004 case 6
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B036:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACD6       ; jumptable 0001B004 case 7
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B03C:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACE6       ; jumptable 0001B004 case 8
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B042:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1ACF6       ; jumptable 0001B004 case 9
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B048:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AD06       ; jumptable 0001B004 case 10
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B04E:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AD10       ; jumptable 0001B004 case 11
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B054:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AD1A       ; jumptable 0001B004 case 12
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B05A:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AD24       ; jumptable 0001B004 case 13
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B060:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                call    sub_1AD86       ; jumptable 0001B004 case 14
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B066:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                sub     ax, ax          ; jumptable 0001B004 case 15

loc_1B068:                              ; CODE XREF: combat_apply_touch_effect+8F↓j
                push    ax
                call    sub_1ADC4
                jmp     short loc_1B010
; ---------------------------------------------------------------------------

loc_1B06E:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                mov     ax, 1           ; jumptable 0001B004 case 16
                jmp     short loc_1B068
; ---------------------------------------------------------------------------
                align 2

loc_1B074:                              ; CODE XREF: combat_apply_touch_effect+22↑j
                                        ; DATA XREF: combat_apply_touch_effect:jpt_1B004↓o
                mov     ax, 1           ; jumptable 0001B004 case 17
                jmp     short loc_1B00C
; ---------------------------------------------------------------------------
                align 2

loc_1B07A:                              ; DATA XREF: combat_apply_touch_effect:off_1B0F6↓o
                mov     ax, 1
                jmp     short loc_1B018
; ---------------------------------------------------------------------------
                align 2

loc_1B080:                              ; DATA XREF: combat_apply_touch_effect+116↓o
                call    sub_1ADF2
                jmp     def_1B004       ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------

loc_1B086:                              ; DATA XREF: combat_apply_touch_effect+118↓o
                mov     ax, 1

loc_1B089:                              ; CODE XREF: combat_apply_touch_effect+B1↓j
                push    ax
                call    sub_1AE32
                jmp     short loc_1B010
; ---------------------------------------------------------------------------
                align 2

loc_1B090:                              ; DATA XREF: combat_apply_touch_effect+11A↓o
                mov     ax, 5
                jmp     short loc_1B089
; ---------------------------------------------------------------------------
                align 2

loc_1B096:                              ; DATA XREF: combat_apply_touch_effect+11C↓o
                sub     ax, ax

loc_1B098:                              ; CODE XREF: combat_apply_touch_effect+C1↓j
                                        ; combat_apply_touch_effect+C7↓j
                push    ax
                call    sub_1AE50
                jmp     loc_1B010
; ---------------------------------------------------------------------------
                align 2

loc_1B0A0:                              ; DATA XREF: combat_apply_touch_effect+11E↓o
                mov     ax, 1
                jmp     short loc_1B098
; ---------------------------------------------------------------------------
                align 2

loc_1B0A6:                              ; DATA XREF: combat_apply_touch_effect+120↓o
                mov     ax, 2
                jmp     short loc_1B098
; ---------------------------------------------------------------------------
                align 2

loc_1B0AC:                              ; DATA XREF: combat_apply_touch_effect+122↓o
                sub     ax, ax

loc_1B0AE:                              ; CODE XREF: combat_apply_touch_effect+D7↓j
                push    ax
                call    sub_1AECA
                jmp     loc_1B010
; ---------------------------------------------------------------------------
                align 2

loc_1B0B6:                              ; DATA XREF: combat_apply_touch_effect+124↓o
                mov     ax, 1
                jmp     short loc_1B0AE
; ---------------------------------------------------------------------------
                align 2

loc_1B0BC:                              ; DATA XREF: combat_apply_touch_effect+126↓o
                call    sub_1AF04
                jmp     short def_1B004 ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------
                align 2

loc_1B0C2:                              ; DATA XREF: combat_apply_touch_effect+128↓o
                call    sub_1AF50
                jmp     short def_1B004 ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------
                align 2

loc_1B0C8:                              ; DATA XREF: combat_apply_touch_effect+12A↓o
                call    sub_1AFA0
                jmp     short def_1B004 ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------
                align 2

loc_1B0CE:                              ; DATA XREF: combat_apply_touch_effect+12C↓o
                call    sub_1AFBC
                jmp     short def_1B004 ; jumptable 0001B004 default case
; ---------------------------------------------------------------------------
                align 2
jpt_1B004       dw offset loc_1B00A     ; DATA XREF: combat_apply_touch_effect+22↑r
                dw offset loc_1B016     ; jump table for switch statement
                dw offset loc_1B01E
                dw offset loc_1B024
                dw offset loc_1B02A
                dw offset loc_1B030
                dw offset loc_1B036
                dw offset loc_1B03C
                dw offset loc_1B042
                dw offset loc_1B048
                dw offset loc_1B04E
                dw offset loc_1B054
                dw offset loc_1B05A
                dw offset loc_1B060
                dw offset loc_1B066
                dw offset loc_1B06E
                dw offset loc_1B074
off_1B0F6       dw offset loc_1B07A     ; CODE XREF: seg002:0909↑J
                dw offset loc_1B080
                dw offset loc_1B086
                dw offset loc_1B090
                dw offset loc_1B096
                dw offset loc_1B0A0
                dw offset loc_1B0A6
                dw offset loc_1B0AC
                dw offset loc_1B0B6
                dw offset loc_1B0BC
                dw offset loc_1B0C2
                dw offset loc_1B0C8
                dw offset loc_1B0CE
; ---------------------------------------------------------------------------

def_1B004:                              ; CODE XREF: combat_apply_touch_effect+1C↑j
                                        ; combat_apply_touch_effect+31↑j ...
                mov     ax, word_22CFA  ; jumptable 0001B004 default case
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1B116:                              ; CODE XREF: ovl_2COMBAT:B542↓p
                push    bp
                mov     bp, sp
                sub     sp, 12h
                mov     al, byte_2781F
                mov     [bp+var_8], al
                mov     ax, 4
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     al, 3
                mov     [bp+var_10], al
                mov     [bp+var_A], 0
                jmp     short loc_1B159
; ---------------------------------------------------------------------------
                align 2

loc_1B13C:                              ; CODE XREF: combat_apply_touch_effect+17E↓j
                mov     al, [bp+var_A]
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_1B156
                dec     [bp+var_10]

loc_1B156:                              ; CODE XREF: combat_apply_touch_effect+16F↑j
                inc     [bp+var_A]

loc_1B159:                              ; CODE XREF: combat_apply_touch_effect+157↑j
                mov     al, [bp+var_A]
                cmp     byte ptr g_party_size, al
                ja      short loc_1B13C
                mov     al, [bp+var_10]
                cmp     byte ptr g_party_size, al
                jnb     short loc_1B171
                mov     al, byte ptr g_party_size
                mov     [bp+var_10], al

loc_1B171:                              ; CODE XREF: combat_apply_touch_effect+187↑j
                cmp     [bp+var_10], 0
                jnz     short loc_1B17A
                inc     [bp+var_10]

loc_1B17A:                              ; CODE XREF: combat_apply_touch_effect+193↑j
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                mov     [bp+var_6], 0
                cmp     al, 28h ; '('
                jb      short loc_1B196
                inc     [bp+var_6]

loc_1B196:                              ; CODE XREF: combat_apply_touch_effect+1AF↑j
                cmp     [bp+var_4], 3Ch ; '<'
                jb      short loc_1B19F
                inc     [bp+var_6]

loc_1B19F:                              ; CODE XREF: combat_apply_touch_effect+1B8↑j
                cmp     [bp+var_4], 50h ; 'P'
                jb      short loc_1B1A8
                inc     [bp+var_6]

loc_1B1A8:                              ; CODE XREF: combat_apply_touch_effect+1C1↑j
                mov     cl, 3
                shl     [bp+var_6], cl
                mov     al, byte_22CFC
                mov     [bp+var_C], al
                mov     al, byte_22CF8
                mov     [bp+var_E], al
                mov     al, byte ptr word_22CFD
                mov     [bp+var_12], al
                cmp     [bp+var_6], 18h
                jbe     short loc_1B1C9
                mov     [bp+var_6], 0

loc_1B1C9:                              ; CODE XREF: combat_apply_touch_effect+1E1↑j
                mov     [bp+var_A], 0
                jmp     short loc_1B1EB
; ---------------------------------------------------------------------------
                align 2

loc_1B1D0:                              ; CODE XREF: combat_apply_touch_effect+221↓j
                                        ; combat_apply_touch_effect+232↓j
                call    sub_1A7D8

loc_1B1D3:                              ; CODE XREF: combat_apply_touch_effect+237↓j
                                        ; seg002:0921↑J
                inc     [bp+var_6]
combat_apply_touch_effect endp

                mov     al, [bp-12h]
                mov     byte ptr word_22CFD, al
                mov     al, [bp-0Eh]
                mov     byte_22CF8, al
                mov     al, [bp-0Ch]
                mov     byte_22CFC, al
                inc     byte ptr [bp-0Ah]
; START OF FUNCTION CHUNK FOR combat_apply_touch_effect

loc_1B1EB:                              ; CODE XREF: combat_apply_touch_effect+1EB↑j
                mov     al, [bp+var_10]
                cmp     [bp+var_A], al
                jnb     short loc_1B21C
                mov     bl, [bp+var_6]
                sub     bh, bh
                mov     al, [bx+1462h]
                mov     byte_2781F, al
                cmp     byte ptr g_party_size, al
                jbe     short loc_1B1D0
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1B1D0
                call    combat_spell_damage
                jmp     short loc_1B1D3
; END OF FUNCTION CHUNK FOR combat_apply_touch_effect
; ---------------------------------------------------------------------------
                align 2
; START OF FUNCTION CHUNK FOR combat_apply_touch_effect

loc_1B21C:                              ; CODE XREF: combat_apply_touch_effect+20F↑j
                mov     al, [bp+var_8]
                mov     byte_2781F, al
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR combat_apply_touch_effect

; =============== S U B R O U T I N E =======================================

; " resisted and", " takes ", " pts"
; Attributes: bp-based frame

combat_spell_damage proc near           ; CODE XREF: combat_apply_touch_effect+234↑p
                                        ; ovl_2COMBAT:B2D1↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     ax, word_27824
                mov     [bp+var_2], ax
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     ax, 54ADh
                push    ax
                mov     ax, 54A8h
                push    ax
                mov     ax, 54ACh
                push    ax
                call    combat_damage_character
                add     sp, 6
                mov     ax, 1
                push    ax
                call    combat_text_reset
                add     sp, 2
                push    [bp+var_4]
                call    thk_res_3E40
                add     sp, 2
                cmp     byte_22CF8, 0
                jz      short loc_1B272
                mov     ax, offset aIsNotAffected_1 ; " is not affected!"
                jmp     short loc_1B2A9
; ---------------------------------------------------------------------------

loc_1B272:                              ; CODE XREF: combat_spell_damage+45↑j
                cmp     byte_22CFC, 0
                jnz     short loc_1B280
                cmp     byte ptr word_22CFD, 0
                jz      short loc_1B28A

loc_1B280:                              ; CODE XREF: combat_spell_damage+51↑j
                mov     ax, offset aResistedAnd ; " resisted and"
                push    ax
                call    thk_text_puts
                add     sp, 2           ; CODE XREF: seg002:092D↑J

loc_1B28A:                              ; CODE XREF: combat_spell_damage+58↑j
                mov     ax, offset aTakes_0 ; " takes "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_27824
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, offset aPts ; " pts"

loc_1B2A9:                              ; CODE XREF: combat_spell_damage+4A↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     byte_27821, 1
                call    combat_after_hit
                mov     ax, [bp+var_2]
                mov     word_27824, ax
                mov     sp, bp
                pop     bp
                retn
combat_spell_damage endp

; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     al, byte_2781F
                mov     [bp-2], al
                call    combat_pick_random_target
                call    combat_spell_damage
                mov     al, [bp-2]
                mov     byte_2781F, al
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B2DE       proc near               ; CODE XREF: combat_spell_effect+27↓p

var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                cmp     byte_22CF8, 0
                jz      short loc_1B312
                mov     bx, [bp+arg_0]
                mov     al, [bx+16h]
                add     al, g_fx_magic
                mov     [bp+var_2], al
                mov     al, [bp+var_4]
                cmp     [bp+var_2], al
                jnb     short loc_1B35D

loc_1B312:                              ; CODE XREF: sub_1B2DE+1D↑j
                mov     byte_22CF8, 0
                cmp     byte_22CFC, 0
                jz      short loc_1B32E
                push    [bp+arg_0]
                call    thk_res_38A8
                add     sp, 2
                mov     byte_22CFC, al
                or      al, al
                jnz     short loc_1B35D

loc_1B32E:                              ; CODE XREF: sub_1B2DE+3E↑j
                mov     byte_22CFC, 0
                cmp     byte ptr word_22CFD, 0
                jz      short loc_1B358
                mov     bx, [bp+arg_0]
                mov     si, word_22CFD
                and     si, 0FFh
                mov     al, [bx+si]
                mov     [bp+var_2], al
                mov     al, g_fx_forces
                add     [bp+var_2], al
                mov     al, [bp+var_4]
                cmp     [bp+var_2], al
                jnb     short loc_1B35D

loc_1B358:                              ; CODE XREF: sub_1B2DE+5A↑j
                mov     byte ptr word_22CFD, 0

loc_1B35D:                              ; CODE XREF: sub_1B2DE+32↑j
                                        ; sub_1B2DE+4E↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1B2DE       endp

; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     bl, byte_27820
                sub     bh, bh
                mov     al, [bx-6980h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     [bp-2], al
                mov     [bp-4], bh
                jmp     short loc_1B394
; ---------------------------------------------------------------------------

loc_1B380:                              ; CODE XREF: ovl_2COMBAT:B39A↓j
                push    word ptr [bp+4]
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     word_27824, ax
                inc     byte ptr [bp-4]

loc_1B394:                              ; CODE XREF: ovl_2COMBAT:B37E↑j
                mov     al, [bp-2]
                cmp     [bp-4], al
                jb      short loc_1B380
                shl     word_27824, 1
                inc     word_27824
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                mov     al, byte_2781F
                mov     [bp-2], al
                mov     al, byte_22CF8
                mov     [bp-8], al
                mov     al, byte ptr word_22CFD
                mov     [bp-0Ah], al
                mov     al, byte_22CFC
                mov     [bp-6], al
                mov     byte ptr [bp-4], 0
                jmp     short loc_1B3FD
; ---------------------------------------------------------------------------

loc_1B3CC:                              ; CODE XREF: ovl_2COMBAT:B404↓j
                mov     al, [bp-4]
                mov     byte_2781F, al
                mov     al, [bp+4]
                sub     ah, ah
                push    ax
                call    combat_spell_effect
                add     sp, 2
                mov     ax, 1
                push    ax
                call    combat_text_reset
                add     sp, 2
                mov     al, [bp-6]
                mov     byte_22CFC, al
                mov     al, [bp-8]
                mov     byte_22CF8, al
                mov     al, [bp-0Ah]
                mov     byte ptr word_22CFD, al
                inc     byte ptr [bp-4]

loc_1B3FD:                              ; CODE XREF: ovl_2COMBAT:B3CA↑j
                mov     al, [bp-4]
                cmp     byte ptr g_party_size, al
                ja      short loc_1B3CC
                mov     al, [bp-2]
                mov     byte_2781F, al
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; " is not affected!", " resisted!"
; Attributes: bp-based frame

combat_spell_effect proc near           ; CODE XREF: ovl_2COMBAT:B3D8↑p
                                        ; ovl_2COMBAT:B523↓p

var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_1B436
                mov     byte_22CF8, 1
                jmp     short loc_1B43D
; ---------------------------------------------------------------------------
                align 2

loc_1B436:                              ; CODE XREF: combat_spell_effect+1C↑j
                push    bx
                call    sub_1B2DE
                add     sp, 2

loc_1B43D:                              ; CODE XREF: combat_spell_effect+23↑j
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    [bp+var_2]
                call    thk_res_3E40
                add     sp, 2
                cmp     byte_22CF8, 0
                jz      short loc_1B468
                mov     ax, offset aIsNotAffected_2 ; " is not affected!"

loc_1B45E:                              ; CODE XREF: combat_spell_effect+69↓j
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1B4BE
; ---------------------------------------------------------------------------
                align 2

loc_1B468:                              ; CODE XREF: combat_spell_effect+49↑j
                cmp     byte_22CFC, 0
                jnz     short loc_1B476
                cmp     byte ptr word_22CFD, 0
                jz      short loc_1B47C

loc_1B476:                              ; CODE XREF: combat_spell_effect+5D↑j
                mov     ax, offset aResisted ; " resisted!"
                jmp     short loc_1B45E
; ---------------------------------------------------------------------------
                align 2

loc_1B47C:                              ; CODE XREF: combat_spell_effect+64↑j
                mov     bx, [bp+var_2]
                mov     al, [bp+arg_0]
                or      [bx+26h], al
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     [bp+var_4], 0

loc_1B493:                              ; CODE XREF: combat_spell_effect+AC↓j
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+arg_0]
                cmp     [si+1456h], al
                jnz     short loc_1B4B5
                mov     bl, [si+145Ch]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+106Eh]
                call    thk_text_puts
                add     sp, 2

loc_1B4B5:                              ; CODE XREF: combat_spell_effect+91↑j
                inc     [bp+var_4]
                cmp     [bp+var_4], 6
                jb      short loc_1B493

loc_1B4BE:                              ; CODE XREF: combat_spell_effect+55↑j
                call    combat_draw_party_hp
                call    sub_1A7D8
                pop     si
                mov     sp, bp
                pop     bp
                retn
combat_spell_effect endp

; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     al, byte_2781F
                mov     [bp-4], al
                push    g_party_size
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte_2781F, al
                dec     byte_2781F

loc_1B4EB:                              ; CODE XREF: ovl_2COMBAT:B51B↓j
                mov     al, byte_2781F
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_1B514
                inc     byte_2781F
                mov     al, byte_2781F
                cmp     byte ptr g_party_size, al
                ja      short loc_1B514
                mov     byte_2781F, 0

loc_1B514:                              ; CODE XREF: ovl_2COMBAT:B500↑j
                                        ; ovl_2COMBAT:B50D↑j
                mov     bx, [bp-2]
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1B4EB
                mov     al, [bp+4]
                sub     ah, ah
                push    ax
                call    combat_spell_effect
                mov     al, [bp-4]
                mov     byte_2781F, al
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                mov     bl, byte_27820
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx-6056h]
                shr     ax, 1
                inc     ax
                mov     word_27824, ax
                call    loc_1B116
                mov     al, byte_27820
                mov     byte_2781E, al
                mov     byte_2781C, 0
                call    combat_kill_monster
                call    combat_draw_monster_list
                retn
; ---------------------------------------------------------------------------
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    4
                db 0C6h
                db  46h ; F
                db 0FCh
                db    0
                db 0EBh
                db  19h
                db  8Ah
                db  46h ; F
                db 0FCh
                db  2Ah ; *
                db 0E4h
                db  50h ; P
                db 0E8h
                db 0F5h
                db 0BBh
                db  83h
                db 0C4h
                db    2
                db  89h
                db  46h ; F
                db 0FEh
                db  8Bh
                db 0D8h
                db 0C7h
                db  47h ; G
                db  58h ; X
                db    0
                db    0
                db 0FEh
                db  46h ; F
                db 0FCh
                db  8Ah
                db  46h ; F
                db 0FCh
                db  38h ; 8
                db    6
                db  26h ; &
                db    4
                db  77h ; w
                db 0DEh
                db 0E8h
                db  4Fh ; O
                db 0F2h
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    4
                db 0C6h
                db  46h ; F
                db 0FCh
                db    0
                db 0EBh
                db  1Dh
                db  8Ah
                db  46h ; F
                db 0FCh
                db  2Ah ; *
                db 0E4h
                db  50h ; P
                db 0E8h
                db 0BFh
                db 0BBh
                db  83h
                db 0C4h
                db    2
                db  89h
                db  46h ; F
                db 0FEh
                db  8Bh
                db 0D8h
                db  80h
                db  7Fh ; 
                db  72h ; r
                db    0
                db  74h ; t
                db    3
                db 0FEh
                db  4Fh ; O
                db  72h ; r
                db 0FEh
                db  46h ; F
                db 0FCh
                db  8Ah
                db  46h ; F
                db 0FCh
                db  38h ; 8
                db    6
                db  26h ; &
                db    4
                db  77h ; w
                db 0DAh
                db 0E8h
                db  15h
                db 0F2h
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    6
                db  56h ; V
                db 0B8h
                db  64h ; d
                db    0
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db  9Ch
                db 0B9h
                db  83h
                db 0C4h
                db    4
                db  89h
                db  46h ; F
                db 0FAh
                db 0C6h
                db  46h ; F
                db 0FCh
                db    0
                db 0EBh
                db  0Bh
                db  8Bh
                db  5Eh ; ^
                db 0FEh
                db 0C7h
byte_1B5EA      db 47h, 5Ch, 2 dup(0), 0FEh, 46h, 0FCh, 8Ah, 46h, 0FCh
                                        ; CODE XREF: seg002:02F1↑J
                db 38h, 6, 26h, 4, 76h, 2Ch, 2Ah, 0E4h, 8Bh, 0F0h, 56h
                db 0E8h, 60h, 0BBh, 83h, 0C4h, 2, 89h, 46h, 0FEh, 83h
                db 7Eh, 0FAh, 41h, 7Eh, 0E0h, 8Bh, 0DEh, 0D1h, 0E3h, 83h
                db 0BFh, 16h, 4, 18h, 7Dh, 0CDh, 8Bh, 0D8h, 2Bh, 0C0h
                db 89h, 47h, 68h, 89h, 47h, 66h, 0EBh, 0C9h, 90h, 0E8h
                db 0AFh, 0F1h, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 0Ah, 57h, 56h, 0B8h, 8, 0, 50h, 0B8h, 1
                db 0, 50h, 0E8h, 35h, 0B9h, 83h, 0C4h, 4, 88h, 46h, 0FAh
                db 0B8h, 3, 0, 50h, 0E8h, 14h, 0FDh, 83h, 0C4h, 2, 0E8h
                db 0C2h, 0FAh, 83h, 3Eh, 26h, 4, 3, 7Ch, 5Dh, 0C6h, 46h
                db 0F6h, 0, 0EBh, 4Fh, 90h, 0FFh, 36h, 26h, 4, 0B8h, 1
                db 0, 50h, 0E8h, 9, 0B9h, 83h, 0C4h, 4, 0FEh, 0C8h, 88h
                db 46h, 0F8h, 0FFh, 36h, 26h, 4, 0B8h, 1, 0, 50h, 0E8h
                db 0F6h, 0B8h, 83h, 0C4h, 4, 0FEh, 0C8h, 88h, 46h, 0FEh
                db 2Ah, 0E4h, 8Bh, 0F0h, 0D1h, 0E6h, 81h, 0C6h, 16h, 4
                db 8Bh, 4, 89h, 46h, 0FCh, 8Ah, 46h, 0F8h, 2Ah, 0E4h, 8Bh
                db 0F8h, 0D1h, 0E7h, 81h, 0C7h, 16h, 4, 8Bh, 5, 89h, 4
                db 8Bh, 46h, 0FCh, 89h, 5, 0FEh, 46h, 0F6h, 8Ah, 46h, 0FAh
                db 38h, 46h, 0F6h, 72h, 0AAh, 0E8h, 89h, 0E8h, 5Eh, 5Fh
                db 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 4, 0A0h, 2Eh, 9Eh, 88h, 46h, 0FEh, 0C6h, 46h, 0FCh
                db 0, 0EBh, 17h, 0A0h, 2Dh, 9Eh, 2Ah, 0E4h, 50h, 0B8h
                db 1, 0, 50h, 0E8h, 95h, 0B8h, 83h, 0C4h, 4, 1, 6, 0D4h
                db 9Fh, 0FEh, 46h, 0FCh, 8Ah, 46h, 0FEh, 38h, 46h, 0FCh
                db 72h, 0E1h, 0E8h, 20h, 0FAh, 0A0h, 0D0h, 9Fh, 0A2h, 0CEh
                db 9Fh, 0C6h, 6, 0CCh, 9Fh, 0, 0E8h, 0F0h, 0D3h, 0E8h
                db 7Fh, 0E9h, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 2, 56h, 0C6h, 46h, 0FEh, 0, 8Ah, 46h, 4
                db 2Ah, 0E4h, 8Bh, 0F0h, 8Ah, 84h, 0F6h, 13h, 0A2h, 0ACh
                db 54h, 8Ah, 84h, 16h, 14h, 0A2h, 0A8h, 54h, 8Ah, 84h
                db 36h, 14h, 2Ch, 11h, 0A2h, 0ADh, 54h, 80h, 7Eh, 4, 2
                db 73h, 3, 0FEh, 46h, 0FEh, 80h, 7Eh, 4, 3, 72h, 9, 80h
                db 7Eh, 4, 8, 77h, 3, 0FEh, 46h, 0FEh, 80h, 7Eh, 4, 18h
                db 74h, 6, 80h, 7Eh, 4, 1Fh, 75h, 3, 0FEh, 46h, 0FEh, 80h
                db 7Eh
byte_1B75E      db 0FEh, 0, 74h, 2Eh, 0A0h, 0D0h, 9Fh, 2Ah, 0E4h, 8Bh
                                        ; CODE XREF: seg002:06BD↑J
                db 0F0h, 8Bh, 0DEh, 0D1h, 0E3h, 8Bh, 87h, 0AAh, 9Fh, 0A3h
                db 0D4h, 9Fh, 80h, 0BCh, 80h, 96h, 0B3h, 73h, 4, 0D1h
                db 2Eh, 0D4h, 9Fh, 83h, 3Eh, 0D4h, 9Fh, 0, 75h, 4, 0FFh
                db 6, 0D4h, 9Fh, 0E8h, 89h, 0F9h, 0E9h, 8, 1, 8Ah, 46h
                db 4, 2Ah, 0E4h, 2Dh, 2, 0, 3Dh, 1Ch, 0, 76h, 3, 0E9h
                db 0F8h, 0, 3, 0C0h, 93h, 2Eh, 0FFh, 0A7h, 5Eh, 0B8h, 80h
                db 3Eh, 0DBh, 3, 0FFh, 75h, 3, 0E9h, 0E6h, 0, 0FEh, 6
                db 0DBh, 3, 0E9h, 0DFh, 0, 90h, 0E8h, 73h, 0FDh, 0E9h
                db 0D8h, 0, 0B8h, 82h, 0, 50h, 0E8h, 0E1h, 0FBh, 83h, 0C4h
                db 2, 0E9h, 0CBh, 0, 90h, 0E8h, 87h, 0FDh, 0E9h, 0C4h
                db 0, 0E8h, 0B7h, 0FDh, 0E9h, 0BEh, 0, 0E8h, 0EBh, 0FDh
                db 0E9h, 0B8h, 0, 0E8h, 4Bh, 0FEh, 0E9h, 0B2h, 0, 0B8h
                db 6, 0, 50h, 0E8h, 75h, 0FBh, 83h, 0C4h, 2, 0E8h, 0CFh
                db 0FAh, 0E9h, 0A2h, 0, 0B8h, 10h, 0, 0EBh, 0C8h, 90h
                db 0B8h, 6, 0, 50h, 0E8h, 5Fh, 0FBh, 83h, 0C4h, 2, 0EBh
                db 82h, 0B8h, 81h, 0, 50h, 0E8h, 0BBh, 0FCh, 0EBh, 0B6h
                db 90h, 0B8h, 0FFh, 0, 0EBh, 0F4h, 90h, 0B8h, 28h, 0, 0EBh
                db 0CCh, 90h, 0B8h, 0Ch, 0, 0EBh, 0DCh, 90h, 0B8h, 32h
                db 0, 0EBh, 0C0h, 90h, 0C7h, 6, 0D4h, 9Fh, 0E8h, 3, 0EBh
                db 0BEh, 0B8h, 14h, 0, 0EBh, 0C8h, 90h, 0B8h, 0Fh, 0, 50h
                db 0B8h, 1, 0, 50h, 0E8h, 33h, 0B7h, 83h, 0C4h, 4, 40h
                db 0A3h, 0D4h, 9Fh, 0EBh, 0A4h, 0B8h, 2, 0, 0E9h, 71h
                db 0FFh, 0E8h, 6Dh, 0FEh, 0EBh, 41h, 90h, 0B8h, 20h, 0
                db 0E9h, 65h, 0FFh, 0A8h, 0B7h, 98h, 0B8h
byte_1B862      db 98h, 0B8h, 98h, 0B8h, 98h, 0B8h, 98h, 0B8h, 98h, 0B8h
                                        ; CODE XREF: seg002:065D↑J
                db 0BAh, 0B7h, 0C0h, 0B7h, 0CEh, 0B7h, 0D4h, 0B7h, 0DAh
                db 0B7h, 0E0h, 0B7h, 0E6h, 0B7h, 0F6h, 0B7h, 0FCh, 0B7h
                db 0FCh, 0B7h, 8, 0B8h, 12h, 0B8h, 18h, 0B8h, 1Eh, 0B8h
                db 24h, 0B8h, 98h, 0B8h, 2Ah, 0B8h, 32h, 0B8h, 38h, 0B8h
                db 4Ch, 0B8h, 52h, 0B8h, 58h, 0B8h, 5Eh, 8Bh, 0E5h, 5Dh
                db 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 57h, 0C7h
                db 46h, 0FEh, 4, 0, 83h, 7Eh, 4, 13h, 7Ch, 5, 0C7h, 46h
                db 0FEh, 2 dup(0), 2Bh, 0C0h, 50h, 0E8h, 0BFh, 0D4h, 83h
                db 0C4h, 2, 0B8h, 10h, 0, 50h, 8Bh, 46h, 0FEh, 8Bh, 5Eh
                db 4, 0D1h, 0E3h, 8Bh, 9Fh, 78h, 33h, 8Bh, 0D0h, 8Bh, 0FBh
                db 8Ch, 0D8h, 8Eh, 0C0h, 0B9h, 2 dup(0FFh), 33h, 0C0h
                db 0F2h, 0AEh, 0F7h, 0D1h, 49h, 3, 0D1h, 83h, 0EAh, 28h
                db 0F7h, 0DAh, 0D1h, 0EAh, 52h, 0E8h, 41h, 0B6h, 83h, 0C4h
                db 4, 83h, 7Eh, 0FEh, 0, 74h, 0Ah, 0FFh, 36h, 78h, 33h
                db 0E8h, 55h, 0B6h, 83h, 0C4h, 2, 8Bh, 5Eh, 4, 0D1h, 0E3h
                db 0FFh, 0B7h, 78h, 33h, 0E8h, 46h, 0B6h, 83h, 0C4h, 2
                db 83h, 7Eh, 0FEh, 0, 74h, 0Ah, 0FFh, 36h, 7Ah, 33h, 0E8h
                db 36h, 0B6h, 83h, 0C4h, 2, 0B8h, 32h, 0, 50h, 0E8h, 0DCh
                db 0B7h, 83h, 0C4h, 2, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 55h
                db 8Bh, 0ECh, 56h, 8Bh, 76h, 8, 8Bh, 5Eh, 4, 80h, 78h
                db 40h, 0, 75h, 0Ch, 0B8h, 10h, 0, 50h, 0E8h, 59h, 0FFh
                db 83h, 0C4h, 2, 0EBh, 56h, 8Bh, 76h, 8, 3, 76h, 4, 0FEh
                db 4Ch, 40h, 75h, 8, 0C6h, 44h, 3Ah, 0FFh, 0C6h, 44h, 46h
                db 0, 8Bh, 5Eh, 6, 80h, 7Fh, 0Fh, 80h, 72h, 12h, 8Ah, 47h
                db 0Fh, 2Ah, 0E4h, 50h, 0FFh, 76h, 4, 0E8h, 0BAh, 1, 83h
                db 0C4h, 4, 0EBh, 29h, 90h, 8Bh, 46h, 8, 5, 6, 0, 50h
                db 8Bh, 5Eh, 6, 8Ah, 47h, 0Fh, 2Ah, 0E4h, 50h, 0FFh, 76h
                db 4, 0E8h, 20h, 2, 83h, 0C4h, 6, 0B8h, 11h, 0, 50h, 0E8h
                db 6, 0FFh, 83h, 0C4h, 2, 0C6h, 6, 4Eh, 69h, 1, 5Eh, 5Dh
                db 0C3h, 90h, 55h, 8Bh, 0ECh, 56h, 8Bh, 76h, 8, 8Bh, 5Eh
                db 4, 80h, 78h, 2Eh, 0, 75h, 0Ch, 0B8h, 10h, 0, 50h, 0E8h
                db 0E3h, 0FEh, 83h, 0C4h, 2, 0EBh, 54h, 8Bh, 76h, 8, 3
                db 76h, 4, 0FEh, 4Ch, 2Eh, 75h, 0Bh, 0C6h, 44h, 28h, 0FFh
                db 0C6h, 44h, 34h, 0, 0EBh, 3Fh, 90h, 8Bh, 5Eh, 6, 80h
                db 7Fh, 0Fh, 80h, 72h, 11h, 8Ah, 47h, 0Fh, 2Ah, 0E4h, 50h
                db 0FFh, 76h, 4, 0E8h, 41h, 1, 83h, 0C4h, 4, 0EBh, 24h
                db 0FFh, 76h, 8, 8Bh, 5Eh, 6, 8Ah, 47h, 0Fh, 2Ah, 0E4h
                db 50h, 0FFh, 76h, 4, 0E8h, 0ACh, 1, 83h, 0C4h, 6, 0B8h
                db 11h, 0, 50h, 0E8h, 92h, 0FEh, 83h, 0C4h, 2, 0C6h, 6
                db 4Eh, 69h, 1, 5Eh, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h
                db 0ECh, 0Ah, 57h, 56h, 2Bh, 0FFh, 0A0h, 0D2h, 9Fh, 2Ah
                db 0E4h, 50h, 0E8h, 37h, 0B7h, 83h, 0C4h, 2, 89h, 46h
                db 0FCh, 0C6h, 6, 4Eh, 69h, 0, 0C6h, 6, 28h, 4, 0, 2Bh
                db 0C0h, 50h, 0E8h, 39h, 0D3h, 83h, 0C4h, 2, 0B8h, 0Fh
                db 0, 50h, 0B8h, 9, 0, 50h, 0E8h, 0DFh, 0B4h, 83h, 0C4h
                db 4, 0B8h, 0CCh, 14h, 50h, 0E8h, 0F9h, 0B4h, 83h, 0C4h
                db 2, 0E8h, 0E7h, 0B4h, 0E8h, 0E0h, 0B5h, 50h, 0E8h, 0BCh
                db 0B4h, 83h, 0C4h, 2, 8Bh, 0F0h, 83h, 0FEh, 1Bh, 75h
                db 6, 0B8h, 1, 0, 0EBh, 3, 90h, 2Bh, 0C0h, 8Bh, 0F8h, 0Bh
                db 0FFh, 75h, 2Ch, 8Bh, 0C6h, 3Dh, 41h, 0, 72h, 0Fh, 3Dh
                db 46h, 0, 77h, 0Ah, 8Bh, 5Eh, 0FCh, 80h, 78h, 0F9h, 0
                db 74h, 1, 47h, 8Bh, 0C6h, 3Dh, 31h, 0, 72h, 0Fh, 3Dh
                db 36h, 0, 77h, 0Ah, 8Bh, 5Eh, 0FCh, 80h, 78h, 0F7h, 0
                db 74h, 1, 47h, 0Bh, 0FFh, 74h, 0B1h, 89h, 7Eh, 0FEh, 89h
                db 76h, 0FAh, 0E8h, 0A7h, 0B4h, 83h, 0FEh, 1Bh, 74h, 69h
                db 83h, 0FEh, 41h, 72h, 0Dh, 83h, 0FEh, 46h, 77h, 8, 8Bh
                db 5Eh, 0FCh, 8Ah, 40h, 0F9h, 0EBh, 9, 8Bh, 76h, 0FAh
                db 8Bh, 5Eh, 0FCh, 8Ah, 40h, 0F7h, 88h, 46h, 0F8h, 0B0h
                db 14h, 0F6h, 66h, 0F8h, 5, 60h, 69h, 89h, 46h, 0F6h, 8Bh
                db 0D8h, 80h, 7Fh, 0Fh, 0, 75h, 0Dh, 0B8h, 0Fh, 0, 50h
                db 0E8h, 0AAh, 0FDh, 83h, 0C4h, 2, 0EBh, 2Ch, 90h, 83h
                db 7Eh, 0FAh, 41h, 72h, 12h, 8Bh, 46h, 0FAh, 2Dh, 41h
                db 0, 50h, 0FFh, 76h, 0F6h, 0FFh, 76h, 0FCh, 0E8h, 1Eh
                db 0FEh, 0EBh, 10h, 8Bh, 46h, 0FAh, 2Dh, 31h, 0, 50h, 0FFh
                db 76h, 0F6h, 0FFh, 76h, 0FCh, 0E8h, 82h, 0FEh, 83h, 0C4h
                db 6, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 2, 56h, 8Ah, 46h, 6, 24h, 7Fh, 0FEh, 0C8h
                db 88h, 46h, 6, 2Ah, 0E4h, 8Bh, 0F0h, 0D1h, 0E0h, 5, 60h
                db 7Dh, 89h, 46h, 0FEh, 50h, 0E8h, 8Dh, 0B6h, 83h
; ---------------------------------------------------------------------------

loc_1BB4E:                              ; CODE XREF: seg002:06C9↑J
                les     ax, [bp+si]
                or      ax, ax
                jz      short loc_1BBA8
                push    word ptr [bp-2]
                call    thk_res_54AE
                add     sp, 2
                or      ax, ax
                jz      short loc_1BBA8
                mov     ax, [bp+4]
                mov     word_23626, ax
                mov     byte_2419E, 1
                mov     byte_1DBE6, 1
                mov     word_27816, 0
                mov     byte ptr word_27812+1, 0
                push    si
                call    sub_1AB02
                add     sp, 2
                or      ax, ax
                jz      short loc_1BB8E
                push    si
                call    thk_2CAST1_D0C2
                jmp     short loc_1BB97
; ---------------------------------------------------------------------------
                align 2

loc_1BB8E:                              ; CODE XREF: ovl_2COMBAT:BB85↑j
                mov     al, [bp+6]
                sub     ah, ah
                push    ax
                call    thk_2CAST2_CF2C

loc_1BB97:                              ; CODE XREF: ovl_2COMBAT:BB8B↑j
                add     sp, 2
                mov     al, byte_1DC78
                mov     byte_2419E, al
                mov     byte_1DBE6, 0
                call    combat_draw_party_hp

loc_1BBA8:                              ; CODE XREF: ovl_2COMBAT:BB52↑j
                                        ; ovl_2COMBAT:BB5F↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1BBAE       proc near

var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = byte ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     si, [bp+arg_4]
                add     si, [bp+arg_0]
                mov     al, [si+34h]
                mov     [bp+var_2], al
                cmp     [bp+arg_4], 6
                jl      short loc_1BBCD
                mov     al, [si+40h]
                mov     [bp+var_2], al

loc_1BBCD:                              ; CODE XREF: sub_1BBAE+17↑j
                mov     al, [bp+arg_2]
                and     al, 0Fh
                add     [bp+var_2], al
                mov     cl, 4
                shr     [bp+arg_2], cl
                and     [bp+arg_2], 7
                mov     al, [bp+arg_2]
                sub     ah, ah
                cmp     ax, 7           ; switch 8 cases
                ja      short def_1BBEB ; jumptable 0001BBEB default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1BBEB[bx] ; switch jump
; ---------------------------------------------------------------------------

loc_1BBF0:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE:jpt_1BBEB↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 0
                add     ax, 75h ; 'u'

loc_1BBF6:                              ; CODE XREF: sub_1BBAE+54↓j
                                        ; sub_1BBAE+5C↓j ...
                mov     [bp+var_4], ax
                jmp     short def_1BBEB ; jumptable 0001BBEB default case
; ---------------------------------------------------------------------------
                align 2

loc_1BBFC:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+88↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 1
                add     ax, 6Bh ; 'k'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC04:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+8A↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 2
                add     ax, 6Eh ; 'n'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC0C:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+8C↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 3
                add     ax, 6Fh ; 'o'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC14:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+8E↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 4
                add     ax, 6Ah ; 'j'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC1C:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+90↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 5
                add     ax, 71h ; 'q'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC24:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+92↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 6
                add     ax, 72h ; 'r'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------

loc_1BC2C:                              ; CODE XREF: sub_1BBAE+3D↑j
                                        ; DATA XREF: sub_1BBAE+94↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001BBEB case 7
                add     ax, 58h ; 'X'
                jmp     short loc_1BBF6
; ---------------------------------------------------------------------------
jpt_1BBEB       dw offset loc_1BBF0     ; DATA XREF: sub_1BBAE+3D↑r
                                        ; jump table for switch statement
                dw offset loc_1BBFC     ; jumptable 0001BBEB case 1
                dw offset loc_1BC04     ; jumptable 0001BBEB case 2
                dw offset loc_1BC0C     ; jumptable 0001BBEB case 3
                dw offset loc_1BC14     ; jumptable 0001BBEB case 4
                dw offset loc_1BC1C     ; jumptable 0001BBEB case 5
                dw offset loc_1BC24     ; jumptable 0001BBEB case 6
                dw offset loc_1BC2C     ; jumptable 0001BBEB case 7
; ---------------------------------------------------------------------------

def_1BBEB:                              ; CODE XREF: sub_1BBAE+38↑j
                                        ; sub_1BBAE+4B↑j
                mov     al, [bp+var_2]  ; jumptable 0001BBEB default case
                sub     ah, ah
                push    ax
                push    [bp+var_4]
                call    thk_res_3608
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1BBAE       endp

; ---------------------------------------------------------------------------
                align 10h
ovl_2COMBAT     ends

