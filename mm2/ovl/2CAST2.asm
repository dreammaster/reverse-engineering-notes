; ===========================================================================

; Segment type: Pure code
ovl_2CAST2      segment byte public 'CODE' use16
                assume cs:ovl_2CAST2
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cast2_common_helper proc near           ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 2, 0E8h, 75h, 11h, 88h, 46h, 0FEh
                                        ; DATA XREF: seg002:0038↑o
                db 3Ch, 1Bh, 74h, 26h, 0B8h, 1, 0, 50h, 0B8h, 5, 0, 50h
                db 0E8h, 5Fh, 0B0h, 83h, 0C4h, 4, 2Bh, 0C0h, 50h, 8Ah
                db 46h, 0FEh, 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h, 0E8h, 0ECh
                db 0AFh, 83h, 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh, 0E5h, 5Dh
                db 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 0E8h, 3Bh, 11h
                db 88h, 46h, 0FEh, 3Ch, 1Bh, 74h, 2Dh, 0B8h, 5, 0, 50h
                db 0B8h, 1, 0, 50h, 0E8h, 0F1h, 0ADh, 83h, 0C4h, 4, 5
                db 3, 0, 0A3h, 0C6h, 9Fh, 0B8h, 1, 0, 50h, 8Ah, 46h, 0FEh
                db 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h, 0E8h, 0ABh, 0AFh, 83h
                db 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh, 0E5h, 5Dh, 0C3h, 90h
                db 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h, 0F9h, 10h, 88h
                db 46h, 0FCh, 3Ch, 1Bh, 74h, 29h, 0E8h, 0DFh, 0AFh, 88h
                db 46h, 0FEh, 0C6h, 6, 0C2h, 9Fh, 4, 0C6h, 6, 0C3h, 9Fh
                db 1, 0B8h, 5, 0, 50h, 8Ah, 46h, 0FCh, 2Ah, 0E4h, 50h
                db 8Ah, 46h, 0FEh, 50h
cast2_common_helper endp ; sp-analysis failed

byte_1C1DA      db 0E8h, 6Dh, 0AFh, 83h, 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh
                                        ; CODE XREF: seg002:08CD↑J
                db 0E5h, 5Dh, 0C3h, 90h

; =============== S U B R O U T I N E =======================================

; Electric Arrow
; Attributes: bp-based frame

spell_cb_Electric_Arrow proc near       ; CODE XREF: seg002:07F5↑J
                                        ; cast_spell_dispatch:loc_1CF58↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C227
                mov     ax, 9
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 7
                mov     word_27816, ax
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C227:                              ; CODE XREF: spell_cb_Electric_Arrow+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Electric_Arrow endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "HP =", "AC =", "Undead (", "Special Power (", "Bonus on Touch (", "Magic Resistance ("
; Attributes: bp-based frame

spell_view_monster proc near            ; CODE XREF: cast_spell_dispatch:loc_1CF5E↓p

var_2           = byte ptr -2

; FUNCTION CHUNK AT C40D SIZE 00000004 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jnz     short loc_1C240 ; CODE XREF: seg002:08E5↑J
                jmp     loc_1C3D0
; ---------------------------------------------------------------------------

loc_1C240:                              ; CODE XREF: spell_view_monster+F↑j
                sub     ax, ax

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                push    si
                call    thk_monster_decode_stats
                add     sp, 2
                mov     ax, 23h ; '#'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [si-6980h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                call    thk_res_3E76
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aHp  ; "HP = "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, si
                shl     bx, 1
                push    word ptr [bx-6056h]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 10h
                push    ax
                mov     ax, 0Ah         ; CODE XREF: seg002:026D↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAc  ; "AC = "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte_2767C
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 11h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aUndead ; "Undead ("

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_27683, 0
                jz      short loc_1C30C
                mov     ax, 59h ; 'Y'   ; CODE XREF: seg002:08F1↑J
                jmp     short loc_1C30F
; ---------------------------------------------------------------------------
                align 2

loc_1C30C:                              ; CODE XREF: spell_view_monster+D8↑j
                mov     ax, 4Eh ; 'N'

loc_1C30F:                              ; CODE XREF: spell_view_monster+DD↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0Fh
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpecialPower ; "Special Power ("
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_27676, 0
                jz      short loc_1C344
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C347
; ---------------------------------------------------------------------------

loc_1C344:                              ; CODE XREF: spell_view_monster+111↑j
                mov     ax, 4Eh ; 'N'

loc_1C347:                              ; CODE XREF: spell_view_monster+116↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBonusOnTouch ; "Bonus on Touch ("
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                cmp     byte_27677, 0
                jz      short loc_1C37C
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C37F
; ---------------------------------------------------------------------------

loc_1C37C:                              ; CODE XREF: spell_view_monster+149↑j
                mov     ax, 4Eh ; 'N'

loc_1C37F:                              ; CODE XREF: spell_view_monster+14E↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMagicResistanc ; "Magic Resistance ("
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_27681, 0
                jz      short loc_1C3B4
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C3B7
; ---------------------------------------------------------------------------

loc_1C3B4:                              ; CODE XREF: spell_view_monster+181↑j
                mov     ax, 4Eh ; 'N'

loc_1C3B7:                              ; CODE XREF: spell_view_monster+186↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                call    thk_monster_anim_step
                mov     byte_1DC78, 1

loc_1C3D0:                              ; CODE XREF: spell_view_monster+11↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

spell_cb_Acid_Stream:                   ; CODE XREF: cast_spell_dispatch:loc_1CF64↓p
                push    bp              ; Acid Stream
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C40D
                mov     ax, 3
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 4           ; CODE XREF: seg002:047D↑J
spell_view_monster endp

                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1
; START OF FUNCTION CHUNK FOR spell_view_monster

loc_1C40D:                              ; CODE XREF: spell_view_monster+1B8↑j
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR spell_view_monster
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Invisibility

spell_cb_Invisibility proc near         ; CODE XREF: cast_spell_dispatch:loc_1CF6A↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C42C
                cmp     byte_1DC34, 0FFh
                jnb     short loc_1C424
                inc     byte_1DC34

loc_1C424:                              ; CODE XREF: spell_cb_Invisibility+C↑j
                call    loc_1D13E
                mov     byte_1DC78, 1

locret_1C42C:                           ; CODE XREF: spell_cb_Invisibility+5↑j
                retn
spell_cb_Invisibility endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Lightning Bolt
; Attributes: bp-based frame

spell_cb_Lightning_Bolt proc near       ; CODE XREF: cast_spell_dispatch:loc_1CF70↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C465
                mov     ax, 1
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 4
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1   ; CODE XREF: seg002:086D↑J

loc_1C465:                              ; CODE XREF: spell_cb_Lightning_Bolt+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Lightning_Bolt endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Web
; Attributes: bp-based frame

spell_cb_Web    proc near               ; CODE XREF: cast_spell_dispatch:loc_1CF76↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    cast2_show_text
                mov     [bp+var_4], al
                cmp     al, 1Bh
                jz      short loc_1C4B0
                mov     byte_1DC78, 1
                call    thk_2COMBAT_A7EA
                mov     [bp+var_2], al
                mov     al, byte_27815
                cmp     [bp+var_4], al
                jnb     short loc_1C492
                call    loc_1D170       ; CODE XREF: seg002:0B31↑J
                jmp     short loc_1C4B0
; ---------------------------------------------------------------------------

loc_1C492:                              ; CODE XREF: spell_cb_Web+21↑j
                mov     byte_27812, 5
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C4B0:                              ; CODE XREF: spell_cb_Web+E↑j
                                        ; spell_cb_Web+26↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Web    endp


; =============== S U B R O U T I N E =======================================

; Cold Beam
; Attributes: bp-based frame

spell_cb_Cold_Beam proc near            ; CODE XREF: cast_spell_dispatch:loc_1CF7C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C4EA
                mov     byte_1DC78, 1
                mov     ax, 6
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C4EA:                              ; CODE XREF: spell_cb_Cold_Beam+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Cold_Beam endp


; =============== S U B R O U T I N E =======================================

; Feeble Mind
; Attributes: bp-based frame

spell_cb_Feeble_Mind proc near          ; CODE XREF: cast_spell_dispatch:loc_1CF82↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C520
                mov     byte_1DC78, 1
                mov     byte_27812, 6
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C520:                              ; CODE XREF: spell_cb_Feeble_Mind+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Feeble_Mind endp


; =============== S U B R O U T I N E =======================================

; Fire Ball
; Attributes: bp-based frame

spell_cb_Fire_Ball proc near            ; CODE XREF: cast_spell_dispatch:loc_1CF88↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text ; CODE XREF: seg002:0879↑J
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C568
                mov     byte_1DC78, 1
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1C546
                call    loc_1D170
                jmp     short loc_1C568
; ---------------------------------------------------------------------------

loc_1C546:                              ; CODE XREF: spell_cb_Fire_Ball+1B↑j
                mov     ax, 1
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 6
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C568:                              ; CODE XREF: spell_cb_Fire_Ball+E↑j
                                        ; spell_cb_Fire_Ball+20↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Fire_Ball endp


; =============== S U B R O U T I N E =======================================

; Shield

spell_cb_Shield proc near               ; CODE XREF: cast_spell_dispatch:loc_1CF8E↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C586
                cmp     byte_1DC35, 0FFh
                jnb     short loc_1C57E
                inc     byte_1DC35

loc_1C57E:                              ; CODE XREF: spell_cb_Shield+C↑j
                call    loc_1D13E
                mov     byte_1DC78, 1

locret_1C586:                           ; CODE XREF: spell_cb_Shield+5↑j
                retn
spell_cb_Shield endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Time Distortion

spell_cb_Time_Distortion proc near      ; CODE XREF: cast_spell_dispatch:loc_1CF94↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C5A7
                mov     byte_1DC78, 1
                test    byte_231F0, 8
                jz      short loc_1C5A0
                call    loc_1D170
                jmp     short locret_1C5A7
; ---------------------------------------------------------------------------

loc_1C5A0:                              ; CODE XREF: spell_cb_Time_Distortion+11↑j
                inc     byte_27818
                call    loc_1D13E

locret_1C5A7:                           ; CODE XREF: spell_cb_Time_Distortion+5↑j
                                        ; spell_cb_Time_Distortion+16↑j
                retn
spell_cb_Time_Distortion endp


; =============== S U B R O U T I N E =======================================

; Disrupt
; Attributes: bp-based frame

spell_cb_Disrupt proc near              ; CODE XREF: cast_spell_dispatch:loc_1CF9A↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2           ; CODE XREF: seg002:0885↑J
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C5E3
                mov     byte_1DC78, 1
                mov     al, byte_27815

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                cmp     [bp+var_2], al
                jnb     short loc_1C5CA
                call    loc_1D170
                jmp     short loc_1C5E3
; ---------------------------------------------------------------------------

loc_1C5CA:                              ; CODE XREF: spell_cb_Disrupt+1B↑j
                mov     word_27816, 64h ; 'd'
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C5E3:                              ; CODE XREF: spell_cb_Disrupt+E↑j
                                        ; spell_cb_Disrupt+20↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Disrupt endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Fingers of Death
; Attributes: bp-based frame

spell_cb_Fingers_of_Death proc near     ; CODE XREF: cast_spell_dispatch:loc_1CFA0↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C61B
                mov     byte_27812, 8
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C61B:                              ; CODE XREF: spell_cb_Fingers_of_Death+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Fingers_of_Death endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Sand Storm
; Attributes: bp-based frame

spell_cb_Sand_Storm proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFA6↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C656
                mov     ax, 1
                push    ax
                mov     ax, 7
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C656:                              ; CODE XREF: spell_cb_Sand_Storm+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Sand_Storm endp


; =============== S U B R O U T I N E =======================================

; Disintegration
; Attributes: bp-based frame

spell_cb_Disintegration proc near       ; CODE XREF: cast_spell_dispatch:loc_1CFAC↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C68C
                mov     byte_27812, 9   ; CODE XREF: seg002:0891↑J
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C68C:                              ; CODE XREF: spell_cb_Disintegration+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Disintegration endp


; =============== S U B R O U T I N E =======================================

; Entrapment

spell_cb_Entrapment proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFB2↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C6AF
                mov     byte_1DC78, 1
                test    byte_231F0, 1
                jz      short loc_1C6A8
                call    loc_1D170
                jmp     short locret_1C6AF
; ---------------------------------------------------------------------------

loc_1C6A8:                              ; CODE XREF: spell_cb_Entrapment+11↑j
                inc     byte_27814
                call    loc_1D13E

locret_1C6AF:                           ; CODE XREF: spell_cb_Entrapment+5↑j
                                        ; spell_cb_Entrapment+16↑j
                retn
spell_cb_Entrapment endp


; =============== S U B R O U T I N E =======================================

; Fantastic Freeze
; Attributes: bp-based frame

spell_cb_Fantastic_Freeze proc near     ; CODE XREF: cast_spell_dispatch:loc_1CFB8↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C6F3
                mov     byte_1DC78, 1
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1C6D2
                call    loc_1D170
                jmp     short loc_1C6F3
; ---------------------------------------------------------------------------

loc_1C6D2:                              ; CODE XREF: spell_cb_Fantastic_Freeze+1B↑j
                mov     ax, 0Ah
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6           ; CODE XREF: seg002:0B19↑J

loc_1C6F3:                              ; CODE XREF: spell_cb_Fantastic_Freeze+E↑j
                                        ; spell_cb_Fantastic_Freeze+20↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Fantastic_Freeze endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Super Shock
; Attributes: bp-based frame

spell_cb_Super_Shock proc near          ; CODE XREF: cast_spell_dispatch:loc_1CFBE↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C72E
                mov     ax, 14h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C72E:                              ; CODE XREF: spell_cb_Super_Shock+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Super_Shock endp


; =============== S U B R O U T I N E =======================================

; Dancing Sword

spell_cb_Dancing_Sword proc near        ; CODE XREF: cast_spell_dispatch:loc_1CFC4↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C75A
                mov     ax, 1           ; CODE XREF: seg002:089D↑J
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C75A:                           ; CODE XREF: spell_cb_Dancing_Sword+5↑j
                retn
spell_cb_Dancing_Sword endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Prismatic Light
; Attributes: bp-based frame

spell_cb_Prismatic_Light proc near      ; CODE XREF: cast_spell_dispatch:loc_1CFCA↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_prompt_return
                or      ax, ax
                jz      short loc_1C7AD
                mov     ax, 9
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                cmp     al, 7
                jnz     short loc_1C78F
                mov     ax, 4
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte_27819, al

loc_1C78F:                              ; CODE XREF: spell_cb_Prismatic_Light+20↑j
                mov     al, [bp+var_2]
                mov     byte_27812, al
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C7AD:                              ; CODE XREF: spell_cb_Prismatic_Light+B↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Prismatic_Light endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Incinerate
; Attributes: bp-based frame

spell_cb_Incinerate proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFD0↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C7E9
                mov     ax, 13h
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6           ; CODE XREF: seg002:0849↑J
                mov     byte_1DC78, 1   ; CODE XREF: seg002:0B0D↑J

loc_1C7E9:                              ; CODE XREF: spell_cb_Incinerate+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Incinerate endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Mega Volts

spell_cb_Mega_Volts proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFD6↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C819
                mov     ax, 7
                push    ax
                mov     ax, 9
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C819:                           ; CODE XREF: spell_cb_Mega_Volts+5↑j
                retn
spell_cb_Mega_Volts endp


; =============== S U B R O U T I N E =======================================

; Meteor Shower

spell_cb_Meteor_Shower proc near        ; CODE XREF: cast_spell_dispatch:loc_1CFDC↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C84E
                mov     ax, 15h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 18h
                mov     word_27816, ax
                inc     byte_2781A
                sub     ax, ax
                push    ax
                push    ax
                mov     al, byte_1DD58
                sub     ah, ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C84E:                           ; CODE XREF: spell_cb_Meteor_Shower+5↑j
                retn
spell_cb_Meteor_Shower endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Power Shield

spell_cb_Power_Shield proc near         ; CODE XREF: cast_spell_dispatch:loc_1CFE2↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C86A
                cmp     byte_1DC36, 0FFh
                jnb     short loc_1C862
                inc     byte_1DC36

loc_1C862:                              ; CODE XREF: spell_cb_Power_Shield+C↑j
                call    loc_1D13E
                mov     byte_1DC78, 1

locret_1C86A:                           ; CODE XREF: spell_cb_Power_Shield+5↑j
                retn
spell_cb_Power_Shield endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Implosion
; Attributes: bp-based frame

spell_cb_Implosion proc near            ; CODE XREF: cast_spell_dispatch:loc_1CFE8↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C89A
                mov     word_27816, 3E8h
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C89A:                              ; CODE XREF: spell_cb_Implosion+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Implosion endp


; =============== S U B R O U T I N E =======================================

; Inferno

spell_cb_Inferno proc near              ; CODE XREF: cast_spell_dispatch:loc_1CFEE↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C8C9
                mov     ax, 4
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C8C9:                           ; CODE XREF: spell_cb_Inferno+5↑j
                retn
spell_cb_Inferno endp


; =============== S U B R O U T I N E =======================================

; Star Burst

spell_cb_Star_Burst proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFF4↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C8FE
                mov     ax, 0A1h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 27h ; '''
                mov     word_27816, ax
                inc     byte_2781A
                sub     ax, ax
                push    ax
                push    ax
                mov     al, byte_1DD58
                sub     ah, ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C8FE:                           ; CODE XREF: spell_cb_Star_Burst+5↑j
                retn
spell_cb_Star_Burst endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Apparition

spell_cb_Apparition proc near           ; CODE XREF: cast_spell_dispatch:loc_1CFFA↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C927
                mov     byte_27812, 3
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C927:                           ; CODE XREF: spell_cb_Apparition+5↑j
                retn
spell_cb_Apparition endp


; =============== S U B R O U T I N E =======================================

; Bless

spell_cb_Bless  proc near               ; CODE XREF: cast_spell_dispatch:loc_1D000↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1C942
                cmp     byte_1DC33, 0FFh
                jnb     short loc_1C93A
                inc     byte_1DC33

loc_1C93A:                              ; CODE XREF: spell_cb_Bless+C↑j
                call    loc_1D13E
                mov     byte_1DC78, 1

locret_1C942:                           ; CODE XREF: spell_cb_Bless+5↑j
                retn
spell_cb_Bless  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Turn Undead
; Attributes: bp-based frame

spell_cb_Turn_Undead proc near          ; CODE XREF: cast_spell_dispatch:loc_1D006↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_2], al
                cmp     al, 10h
                jnb     short loc_1C95D
                mov     cl, 4
                shl     [bp+var_2], cl

loc_1C95D:                              ; CODE XREF: spell_cb_Turn_Undead+12↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_1C96A
                mov     sp, bp
                pop     bp
                retn
spell_cb_Turn_Undead endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C96A       proc near               ; CODE XREF: spell_cb_Turn_Undead+1F↑p
                                        ; spell_cb_Holy_Word+8↓p

var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     [bp+var_8], 0
                mov     [bp+var_2], 0
                mov     [bp+var_A], 0
                call    cast2_prompt_return
                or      ax, ax
                jnz     short loc_1C987
                jmp     loc_1CA36
; ---------------------------------------------------------------------------

loc_1C987:                              ; CODE XREF: sub_1C96A+18↑j
                cmp     byte_2781B, 0
                jz      short loc_1C991
                jmp     loc_1CA33
; ---------------------------------------------------------------------------

loc_1C991:                              ; CODE XREF: sub_1C96A+22↑j
                inc     byte_2781B
                mov     al, byte_1DD58
                mov     [bp+var_4], al  ; CODE XREF: seg002:07DD↑J
                cmp     [bp+arg_0], 0
                jz      short loc_1C9A9
                cmp     al, 0Ah
                jbe     short loc_1C9A9
                mov     [bp+var_4], 0Ah

loc_1C9A9:                              ; CODE XREF: sub_1C96A+35↑j
                                        ; sub_1C96A+39↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax

loc_1C9B0:                              ; CODE XREF: sub_1C96A+C1↓j
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                cmp     byte_27683, 0
                jz      short loc_1CA1B
                mov     [bp+var_6], 0FFh
                cmp     [bp+arg_0], 0
                jz      short loc_1C9DB
                push    si
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_6], al

loc_1C9DB:                              ; CODE XREF: sub_1C96A+61↑j
                mov     bl, [bp+var_8]
                sub     bh, bh
                mov     al, [bp+var_6]
                cmp     [bx-6980h], al  ; CODE XREF: seg002:0B01↑J
                ja      short loc_1CA1B
                inc     [bp+var_2]
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                call    thk_res_3E76
                mov     ax, offset aIsEradicated_0 ; " is eradicated!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     byte_2781C, 1
                mov     al, [bp+var_8]
                mov     byte_2781E, al
                call    thk_2COMBAT_8AF4
                mov     byte_2781C, 0
                dec     [bp+var_8]
                call    thk_2COMBAT_A7D8

loc_1CA1B:                              ; CODE XREF: sub_1C96A+57↑j
                                        ; sub_1C96A+7D↑j
                inc     [bp+var_8]
                cmp     [bp+var_8], 0Bh
                jnz     short loc_1CA28
                mov     [bp+var_4], 1

loc_1CA28:                              ; CODE XREF: sub_1C96A+B8↑j
                dec     [bp+var_4]
                jnz     short loc_1C9B0
                cmp     [bp+var_2], 0
                jnz     short loc_1CA36

loc_1CA33:                              ; CODE XREF: sub_1C96A+24↑j
                call    loc_1D170

loc_1CA36:                              ; CODE XREF: sub_1C96A+1A↑j
                                        ; sub_1C96A+C7↑j
                mov     byte_2781A, 0
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C96A       endp


; =============== S U B R O U T I N E =======================================

; Heroism
; Attributes: bp-based frame

spell_cb_Heroism proc near              ; CODE XREF: cast_spell_dispatch:loc_1D00C↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    cast2_return_prompt
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CA69
                mov     byte_1DC78, 1   ; CODE XREF: seg002:0A89↑J
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                add     byte ptr [bx+71h], 6
                call    loc_1D13E

loc_1CA69:                              ; CODE XREF: spell_cb_Heroism+F↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Heroism endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Pain
; Attributes: bp-based frame

spell_cb_Pain   proc near               ; CODE XREF: cast_spell_dispatch:loc_1D012↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CAAA
                mov     ax, 0Ch
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range  ; CODE XREF: seg002:0801↑J
                add     sp, 4
                add     ax, 3
                mov     word_27816, ax
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CAAA:                              ; CODE XREF: spell_cb_Pain+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Pain   endp


; =============== S U B R O U T I N E =======================================

; Silence
; Attributes: bp-based frame

spell_cb_Silence proc near              ; CODE XREF: cast_spell_dispatch:loc_1D018↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    cast2_show_text
                mov     [bp+var_4], al
                cmp     al, 1Bh
                jz      short loc_1CAE7
                call    thk_2COMBAT_A7EA
                mov     [bp+var_2], al
                mov     byte_27812, 1
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CAE7:                              ; CODE XREF: spell_cb_Silence+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Silence endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Weaken

spell_cb_Weaken proc near               ; CODE XREF: cast_spell_dispatch:loc_1D01E↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1CB10
                mov     byte_27812, 2
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1CB10:                           ; CODE XREF: spell_cb_Weaken+5↑j
                retn
spell_cb_Weaken endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Cold Ray
; Attributes: bp-based frame

spell_cb_Cold_Ray proc near             ; CODE XREF: cast_spell_dispatch:loc_1D024↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CB4E
                mov     byte_1DC78, 1
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1CB34
                call    loc_1D170
                jmp     short loc_1CB4E
; ---------------------------------------------------------------------------

loc_1CB34:                              ; CODE XREF: spell_cb_Cold_Ray+1B↑j
                mov     word_27816, 19h
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]  ; CODE XREF: seg002:0A65↑J
                sub     ah, ah
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CB4E:                              ; CODE XREF: spell_cb_Cold_Ray+E↑j
                                        ; spell_cb_Cold_Ray+20↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Cold_Ray endp


; =============== S U B R O U T I N E =======================================

; Immobilize
; Attributes: bp-based frame

spell_cb_Immobilize proc near           ; CODE XREF: cast_spell_dispatch:loc_1D02A↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CB85
                mov     byte_1DC78, 1
                mov     byte_27812, 5
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CB85:                              ; CODE XREF: spell_cb_Immobilize+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Immobilize endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Acid Spray
; Attributes: bp-based frame

spell_cb_Acid_Spray proc near           ; CODE XREF: seg002:0AF5↑J
                                        ; cast_spell_dispatch:loc_1D030↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CBD4
                mov     byte_1DC78, 1   ; CODE XREF: seg002:080D↑J
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1CBAC
                call    loc_1D170
                jmp     short loc_1CBD4
; ---------------------------------------------------------------------------

loc_1CBAC:                              ; CODE XREF: spell_cb_Acid_Spray+1B↑j
                mov     ax, 31h ; '1'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 0Bh
                mov     word_27816, ax
                mov     ax, 4
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CBD4:                              ; CODE XREF: spell_cb_Acid_Spray+E↑j
                                        ; spell_cb_Acid_Spray+20↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Acid_Spray endp


; =============== S U B R O U T I N E =======================================

; Holy Bonus

spell_cb_Holy_Bonus proc near           ; CODE XREF: cast_spell_dispatch:loc_1D036↓p
                call    cast2_prompt_return
                or      ax, ax          ; CODE XREF: seg002:01AD↑J
                jz      short locret_1CBF6
                mov     bx, word_23626
                mov     al, [bx+71h]
                sub     ah, ah
                shr     ax, 1
                add     byte_1DC37, al
                call    loc_1D13E
                mov     byte_1DC78, 1

locret_1CBF6:                           ; CODE XREF: spell_cb_Holy_Bonus+5↑j
                retn
spell_cb_Holy_Bonus endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Air Encasement
; Attributes: bp-based frame

spell_cb_Air_Encasement proc near       ; CODE XREF: cast_spell_dispatch:loc_1D03C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CC2F
                mov     byte_27819, 1
                mov     byte_27812, 7
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CC2F:                              ; CODE XREF: spell_cb_Air_Encasement+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Air_Encasement endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Deadly Swarm

spell_cb_Deadly_Swarm proc near         ; CODE XREF: cast_spell_dispatch:loc_1D042↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1CC62
                mov     ax, 21h ; '!'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 7
                mov     word_27816, ax
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1CC62:                           ; CODE XREF: spell_cb_Deadly_Swarm+5↑j
                retn
spell_cb_Deadly_Swarm endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Frenzy
; Attributes: bp-based frame

spell_cb_Frenzy proc near               ; CODE XREF: cast_spell_dispatch:loc_1D048↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    cast2_return_prompt
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CCD7
                mov     byte_1DC78, 1
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                cmp     byte_27811, 0
                jz      short loc_1CC90

loc_1CC8B:                              ; CODE XREF: spell_cb_Frenzy+37↓j
                                        ; spell_cb_Frenzy+41↓j
                call    loc_1D170
                jmp     short loc_1CCD7
; ---------------------------------------------------------------------------

loc_1CC90:                              ; CODE XREF: spell_cb_Frenzy+25↑j
                inc     byte_27811
                mov     bx, [bp+var_2]
                cmp     byte ptr [bx+26h], 0
                jnz     short loc_1CC8B
                mov     byte ptr [bx+26h], 40h ; '@'
                cmp     byte ptr [bx+73h], 0
                jz      short loc_1CC8B
                dec     byte ptr [bx+73h]
                mov     word ptr [bx+5Eh], 0
                mov     al, [bx+4Ch]
                sub     ah, ah
                mov     cl, [bx+4Dh]
                sub     ch, ch
                add     ax, cx          ; CODE XREF: seg002:08FD↑J
                add     ax, 0Ah
                mov     word_27816, ax
                shl     word_27816, 1
                inc     byte_27810
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CCD7:                              ; CODE XREF: spell_cb_Frenzy+F↑j
                                        ; spell_cb_Frenzy+2A↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Frenzy endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Paralyze

spell_cb_Paralyze proc near             ; CODE XREF: cast_spell_dispatch:loc_1D04E↓p
                call    cast2_prompt_return
                or      ax, ax
                jz      short locret_1CD03
                mov     byte_27812, 5
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1CD03:                           ; CODE XREF: spell_cb_Paralyze+5↑j
                retn
spell_cb_Paralyze endp


; =============== S U B R O U T I N E =======================================

; Water Encasement
; Attributes: bp-based frame

spell_cb_Water_Encasement proc near     ; CODE XREF: cast_spell_dispatch:loc_1D054↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CD3C
                mov     byte_27819, 2
                mov     byte_27812, 7
                mov     byte_27813, 1
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CD3C:                              ; CODE XREF: spell_cb_Water_Encasement+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Water_Encasement endp


; =============== S U B R O U T I N E =======================================

; Earth Encasement
; Attributes: bp-based frame

spell_cb_Earth_Encasement proc near     ; CODE XREF: cast_spell_dispatch:loc_1D05A↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CD78
                mov     byte_27819, 3
                mov     byte_27812, 7
                mov     byte_27813, 1
                mov     ax, 4
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CD78:                              ; CODE XREF: spell_cb_Earth_Encasement+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Earth_Encasement endp


; =============== S U B R O U T I N E =======================================

; Fiery Flail
; Attributes: bp-based frame

spell_cb_Fiery_Flail proc near          ; CODE XREF: cast_spell_dispatch:loc_1D060↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CDB6
                mov     ax, 190h
                push    ax
                mov     ax, 0FFh
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     word_27816, ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CDB6:                              ; CODE XREF: spell_cb_Fiery_Flail+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Fiery_Flail endp


; =============== S U B R O U T I N E =======================================

; Moon Ray
; Attributes: bp-based frame

spell_cb_Moon_Ray proc near             ; CODE XREF: cast_spell_dispatch:loc_1D066↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                call    cast2_prompt_return
                or      ax, ax
                jz      short loc_1CE3A
                mov     byte_1DC78, 1
                mov     ax, 5Bh ; '['
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 9
                mov     word_27816, ax
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                sub     di, di
                mov     si, [bp+var_2]
                jmp     short loc_1CE2E
; ---------------------------------------------------------------------------
                align 2

loc_1CDF8:                              ; CODE XREF: spell_cb_Moon_Ray+78↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     ax, 5Bh ; '['
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     ax, 9
                mov     word_27816, ax
                cmp     byte ptr [si+26h], 80h
                jnb     short loc_1CE2D
                and     byte ptr [si+26h], 2Fh
                add     [si+5Eh], ax
                mov     ax, [si+74h]
                cmp     [si+5Eh], ax
                jbe     short loc_1CE2D
                mov     [si+5Eh], ax

loc_1CE2D:                              ; CODE XREF: spell_cb_Moon_Ray+5F↑j
                                        ; spell_cb_Moon_Ray+6E↑j
                inc     di

loc_1CE2E:                              ; CODE XREF: spell_cb_Moon_Ray+3B↑j
                                        ; seg002:0831↑J
                cmp     di, g_party_size
                jl      short loc_1CDF8
                mov     [bp+var_4], di
                mov     [bp+var_2], si

loc_1CE3A:                              ; CODE XREF: spell_cb_Moon_Ray+D↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
spell_cb_Moon_Ray endp


; =============== S U B R O U T I N E =======================================

; Fire Encasement
; Attributes: bp-based frame

spell_cb_Fire_Encasement proc near      ; CODE XREF: cast_spell_dispatch:loc_1D06C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CE78
                mov     byte_1DC78, 1
                mov     byte_27819, 4
                mov     byte_27812, 7
                mov     byte_27813, 1
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CE78:                              ; CODE XREF: spell_cb_Fire_Encasement+E↑j
                mov     sp, bp
                pop     bp
                retn
spell_cb_Fire_Encasement endp


; =============== S U B R O U T I N E =======================================

; Mass Distortion
; Attributes: bp-based frame

spell_cb_Mass_Distortion proc near      ; CODE XREF: cast_spell_dispatch:loc_1D072↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    cast2_show_text
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CEB1
                mov     byte_1DC78, 1
                sub     ah, ah
                mov     si, ax
                mov     bx, si
                shl     bx, 1
                mov     ax, [bx-6056h]
                shr     ax, 1
                mov     word_27816, ax
                sub     ax, ax
                push    ax
                push    si
                mov     ax, 2
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CEB1:                              ; CODE XREF: spell_cb_Mass_Distortion+F↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
spell_cb_Mass_Distortion endp


; =============== S U B R O U T I N E =======================================

; Divine intervention
; Attributes: bp-based frame

spell_cb_Divine_intervention proc near  ; CODE XREF: cast_spell_dispatch:loc_1D078↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                call    cast2_prompt_return
                or      ax, ax
                jz      short loc_1CF16
                mov     byte_1DC78, 1   ; CODE XREF: seg002:07AD↑J
                cmp     byte_2781D, 0
                jz      short loc_1CED6
                call    loc_1D170
                jmp     short loc_1CF16
; ---------------------------------------------------------------------------

loc_1CED6:                              ; CODE XREF: spell_cb_Divine_intervention+19↑j
                inc     byte_2781D
                mov     ax, 5
                push    ax
                push    word_23626
                call    thk_char_add_age
                add     sp, 4
                sub     di, di
                mov     si, [bp+var_2]
                jmp     short loc_1CF0A
; ---------------------------------------------------------------------------
                align 2

loc_1CEF0:                              ; CODE XREF: spell_cb_Divine_intervention+58↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                cmp     byte ptr [si+26h], 0FFh
                jz      short loc_1CF03
                mov     byte ptr [si+26h], 0

loc_1CF03:                              ; CODE XREF: spell_cb_Divine_intervention+47↑j
                mov     ax, [si+74h]
                mov     [si+5Eh], ax
                inc     di

loc_1CF0A:                              ; CODE XREF: spell_cb_Divine_intervention+37↑j
                cmp     di, g_party_size
                jl      short loc_1CEF0
                mov     [bp+var_4], di
                mov     [bp+var_2], si

loc_1CF16:                              ; CODE XREF: spell_cb_Divine_intervention+D↑j
                                        ; spell_cb_Divine_intervention+1E↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
spell_cb_Divine_intervention endp


; =============== S U B R O U T I N E =======================================

; Holy Word

spell_cb_Holy_Word proc near            ; CODE XREF: cast_spell_dispatch:loc_1D07E↓p
                mov     byte_2781A, 1
                sub     ax, ax
                push    ax
                call    sub_1C96A
                add     sp, 2
                retn
spell_cb_Holy_Word endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; (spell index) jump to the per-spell handler
; Attributes: bp-based frame

cast_spell_dispatch proc near           ; CODE XREF: seg002:050D↑J

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ax, [bp+arg_0]
                sub     ax, 2           ; switch 31 cases
                cmp     ax, 5Bh
                jbe     short loc_1CF3D
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF3D:                              ; CODE XREF: cast_spell_dispatch+C↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1CF40[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1CF46:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    cast2_common_helper ; jumptable 0001CF40 case 2
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF4C:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    near ptr byte_1C132+38h ; jumptable 0001CF40 case 3
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF52:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    near ptr byte_1C132+7Ah ; jumptable 0001CF40 case 6
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF58:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Electric_Arrow ; jumptable 0001CF40 case 8
                jmp     def_1CF40       ; CODE XREF: seg002:0AE9↑J
                                        ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF5E:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_view_monster ; jumptable 0001CF40 case 9
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF64:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Acid_Stream ; jumptable 0001CF40 case 14
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF6A:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Invisibility ; jumptable 0001CF40 case 16
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF70:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Lightning_Bolt ; jumptable 0001CF40 case 17
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF76:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Web    ; jumptable 0001CF40 case 18
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF7C:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Cold_Beam ; jumptable 0001CF40 case 20
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF82:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; seg002:062D↑J
                                        ; DATA XREF: ...
                call    spell_cb_Feeble_Mind ; jumptable 0001CF40 case 21
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF88:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Fire_Ball ; jumptable 0001CF40 case 22
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF8E:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Shield ; jumptable 0001CF40 case 24
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF94:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Time_Distortion ; jumptable 0001CF40 case 25
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CF9A:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Disrupt ; jumptable 0001CF40 case 26
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFA0:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Fingers_of_Death ; jumptable 0001CF40 case 27
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFA6:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Sand_Storm ; jumptable 0001CF40 case 28
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFAC:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Disintegration ; jumptable 0001CF40 case 31
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFB2:                              ; CODE XREF: cast_spell_dispatch+14↑j
                                        ; DATA XREF: cast_spell_dispatch:jpt_1CF40↓o
                call    spell_cb_Entrapment ; jumptable 0001CF40 case 32
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFB8:                              ; DATA XREF: cast_spell_dispatch:off_1D0C2↓o
                call    spell_cb_Fantastic_Freeze
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFBE:                              ; DATA XREF: cast_spell_dispatch+19A↓o
                call    spell_cb_Super_Shock
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFC4:                              ; DATA XREF: cast_spell_dispatch+19C↓o
                call    spell_cb_Dancing_Sword
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFCA:                              ; DATA XREF: cast_spell_dispatch+1A2↓o
                call    spell_cb_Prismatic_Light
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFD0:                              ; DATA XREF: cast_spell_dispatch+1A4↓o
                call    spell_cb_Incinerate
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFD6:                              ; DATA XREF: cast_spell_dispatch+1A6↓o
                call    spell_cb_Mega_Volts
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFDC:                              ; DATA XREF: cast_spell_dispatch+1A8↓o
                call    spell_cb_Meteor_Shower
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFE2:                              ; DATA XREF: cast_spell_dispatch+1AA↓o
                call    spell_cb_Power_Shield
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFE8:                              ; DATA XREF: cast_spell_dispatch+1AC↓o
                call    spell_cb_Implosion
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFEE:                              ; DATA XREF: cast_spell_dispatch+1AE↓o
                call    spell_cb_Inferno
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFF4:                              ; DATA XREF: cast_spell_dispatch+1B0↓o
                call    spell_cb_Star_Burst
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1CFFA:                              ; DATA XREF: cast_spell_dispatch+1B4↓o
                call    spell_cb_Apparition
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D000:                              ; DATA XREF: cast_spell_dispatch+1B8↓o
                call    spell_cb_Bless
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D006:                              ; DATA XREF: cast_spell_dispatch+1C0↓o
                call    spell_cb_Turn_Undead
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D00C:                              ; DATA XREF: cast_spell_dispatch+1C4↓o
                call    spell_cb_Heroism
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D012:                              ; DATA XREF: cast_spell_dispatch+1C8↓o
                call    spell_cb_Pain
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D018:                              ; DATA XREF: cast_spell_dispatch+1CC↓o
                call    spell_cb_Silence
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D01E:                              ; DATA XREF: cast_spell_dispatch+1CE↓o
                call    spell_cb_Weaken
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D024:                              ; DATA XREF: cast_spell_dispatch+1D0↓o
                call    spell_cb_Cold_Ray
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D02A:                              ; DATA XREF: cast_spell_dispatch+1D6↓o
                call    spell_cb_Immobilize
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D030:                              ; DATA XREF: cast_spell_dispatch+1DC↓o
                call    spell_cb_Acid_Spray
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D036:                              ; DATA XREF: cast_spell_dispatch+1E6↓o
                call    spell_cb_Holy_Bonus
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D03C:                              ; DATA XREF: cast_spell_dispatch+1E8↓o
                call    spell_cb_Air_Encasement
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D042:                              ; DATA XREF: cast_spell_dispatch+1EA↓o
                call    spell_cb_Deadly_Swarm
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D048:                              ; DATA XREF: cast_spell_dispatch+1EC↓o
                call    spell_cb_Frenzy
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D04E:                              ; DATA XREF: cast_spell_dispatch+1EE↓o
                call    spell_cb_Paralyze
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D054:                              ; DATA XREF: cast_spell_dispatch+1F8↓o
                call    spell_cb_Water_Encasement
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D05A:                              ; DATA XREF: cast_spell_dispatch+1FC↓o
                call    spell_cb_Earth_Encasement
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D060:                              ; DATA XREF: cast_spell_dispatch+1FE↓o
                call    spell_cb_Fiery_Flail
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D066:                              ; DATA XREF: cast_spell_dispatch+200↓o
                call    spell_cb_Moon_Ray
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D06C:                              ; DATA XREF: cast_spell_dispatch+204↓o
                call    spell_cb_Fire_Encasement
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D072:                              ; DATA XREF: cast_spell_dispatch+208↓o
                call    spell_cb_Mass_Distortion
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D078:                              ; DATA XREF: cast_spell_dispatch+20C↓o
                call    spell_cb_Divine_intervention
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------

loc_1D07E:                              ; DATA XREF: cast_spell_dispatch+20E↓o
                call    spell_cb_Holy_Word
                jmp     def_1CF40       ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
; ---------------------------------------------------------------------------
jpt_1CF40       dw offset loc_1CF46     ; DATA XREF: cast_spell_dispatch+14↑r
                dw offset loc_1CF4C     ; jump table for switch statement
                dw offset def_1CF40
                dw offset def_1CF40
                dw offset loc_1CF52
                dw offset def_1CF40
                dw offset loc_1CF58
                dw offset loc_1CF5E
                dw offset def_1CF40
                dw offset def_1CF40
                dw offset def_1CF40
                dw offset def_1CF40
                dw offset loc_1CF64
                dw offset def_1CF40
                dw offset loc_1CF6A
                dw offset loc_1CF70
                dw offset loc_1CF76
                dw offset def_1CF40
                dw offset loc_1CF7C
                dw offset loc_1CF82
                dw offset loc_1CF88
                dw offset def_1CF40
                dw offset loc_1CF8E
                dw offset loc_1CF94
                dw offset loc_1CF9A
                dw offset loc_1CFA0
                dw offset loc_1CFA6
                dw offset def_1CF40
                dw offset def_1CF40
                dw offset loc_1CFAC
                dw offset loc_1CFB2
off_1D0C2       dw offset loc_1CFB8     ; CODE XREF: seg002:04DD↑J
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1CFBE
                dw offset loc_1CFC4
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1CFCA
                dw offset loc_1CFD0
                dw offset loc_1CFD6
                dw offset loc_1CFDC
                dw offset loc_1CFE2
                dw offset loc_1CFE8
                dw offset loc_1CFEE
                dw offset loc_1CFF4
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1CFFA
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D000
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D006
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D00C
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D012
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D018
                dw offset loc_1D01E
                dw offset loc_1D024
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D02A
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D030
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D036
                dw offset loc_1D03C
                dw offset loc_1D042
                dw offset loc_1D048
                dw offset loc_1D04E
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D054
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D05A
                dw offset loc_1D060
                dw offset loc_1D066
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D06C
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D072
                dw offset def_1CF40     ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                dw offset loc_1D078
                dw offset loc_1D07E
; ---------------------------------------------------------------------------

def_1CF40:                              ; CODE XREF: cast_spell_dispatch+E↑j
                                        ; cast_spell_dispatch+14↑j ...
                pop     bp              ; jumptable 0001CF40 default case, cases 4,5,7,10-13,15,19,23,29,30
                retn
; ---------------------------------------------------------------------------

loc_1D13E:                              ; CODE XREF: spell_cb_Invisibility:loc_1C424↑p
                                        ; spell_cb_Shield:loc_1C57E↑p ...
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDone_0 ; "* Done *"
                push    ax
; ---------------------------------------------------------------------------
                db 0E8h
; ---------------------------------------------------------------------------

loc_1D15A:                              ; CODE XREF: seg002:0825↑J
                neg     byte ptr [di-3B7Dh]
                add     bh, [bx+si+32h]
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     byte_1DC78, 1
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1D170:                              ; CODE XREF: spell_cb_Web+23↑p
                                        ; spell_cb_Fire_Ball+1D↑p ...
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpellFailed_1 ; "* Spell Failed *"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                retn
cast_spell_dispatch endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cast2_return_prompt proc near           ; CODE XREF: spell_cb_Heroism+6↑p
                                        ; spell_cb_Frenzy+6↑p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     byte_1DC78, 0
                cmp     g_party_size, 1
                jnz     short loc_1D1CE
                call    cast2_prompt_return
                or      ax, ax
                jz      short loc_1D1C6
                mov     [bp+var_2], 31h ; '1'
                jmp     short loc_1D20B
; ---------------------------------------------------------------------------

loc_1D1C6:                              ; CODE XREF: cast2_return_prompt+17↑j
                mov     [bp+var_2], 1Bh
                jmp     short loc_1D20B
; ---------------------------------------------------------------------------
                align 2

loc_1D1CE:                              ; CODE XREF: cast2_return_prompt+10↑j
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+0Bh], al
                mov     ax, 0Fh
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr aL1ReturnToCast ; "L1'Return' to cast"
                call    thk_text_puts
                add     sp, 2
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, [bx+0Bh]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_2], ax

loc_1D20B:                              ; CODE XREF: cast2_return_prompt+1E↑j
                                        ; cast2_return_prompt+25↑j
                call    thk_res_35A8
                cmp     [bp+var_2], 1Bh
                jz      short loc_1D21D
                sub     [bp+var_2], 31h ; '1'
                mov     byte_1DC78, 1

loc_1D21D:                              ; CODE XREF: cast2_return_prompt+6C↑j
                cmp     byte_1DC78, 0
                jz      short loc_1D233
                test    byte_23218, 2
                jz      short loc_1D233
                mov     [bp+var_2], 1Bh
                call    loc_1D170

loc_1D233:                              ; CODE XREF: cast2_return_prompt+7C↑j
                                        ; cast2_return_prompt+83↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
cast2_return_prompt endp


; =============== S U B R O U T I N E =======================================

; "'Return' to cast"
; Attributes: bp-based frame

cast2_prompt_return proc near           ; CODE XREF: spell_cb_Invisibility↑p
                                        ; spell_cb_Shield↑p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     byte_2419E, 0
                jz      short loc_1D24E
                mov     [bp+var_2], 1
                jmp     short loc_1D28E
; ---------------------------------------------------------------------------

loc_1D24E:                              ; CODE XREF: cast2_prompt_return+B↑j
                mov     byte_1DC78, 0
                mov     ax, 0Fh
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aReturnToCast ; "'Return' to cast"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_2], ax
                cmp     ax, 0Dh
                jnz     short loc_1D284
                mov     al, 1
                jmp     short loc_1D286
; ---------------------------------------------------------------------------

loc_1D284:                              ; CODE XREF: cast2_prompt_return+44↑j
                sub     al, al

loc_1D286:                              ; CODE XREF: cast2_prompt_return+48↑j
                mov     byte_1DC78, al
                sub     ah, ah
                mov     [bp+var_2], ax

loc_1D28E:                              ; CODE XREF: cast2_prompt_return+12↑j
                call    thk_res_35A8
                cmp     [bp+var_2], 0
                jz      short loc_1D2A6
                test    byte_23218, 2
                jz      short loc_1D2A6
                mov     [bp+var_2], 0
                call    loc_1D170

loc_1D2A6:                              ; CODE XREF: cast2_prompt_return+5B↑j
                                        ; cast2_prompt_return+62↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
cast2_prompt_return endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

cast2_show_text proc near               ; CODE XREF: spell_cb_Electric_Arrow+6↑p
                                        ; spell_view_monster+7↑p ...

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     byte_1DC78, 0
                cmp     byte_1DD58, 1
                jnz     short loc_1D2D8
                call    cast2_prompt_return
                or      ax, ax
                jz      short loc_1D2D0
                mov     [bp+var_6], 41h ; 'A'
                jmp     short loc_1D34B
; ---------------------------------------------------------------------------
                align 4

loc_1D2D0:                              ; CODE XREF: cast2_show_text+18↑j
                mov     [bp+var_6], 1Bh
                jmp     short loc_1D34B
; ---------------------------------------------------------------------------
                align 4

loc_1D2D8:                              ; CODE XREF: cast2_show_text+11↑j
                mov     al, byte_1DD58
                mov     [bp+var_4], al
                cmp     al, 0Ah
                jbe     short loc_1D2E6
                mov     [bp+var_4], 0Ah

loc_1D2E6:                              ; CODE XREF: cast2_show_text+32↑j
                add     [bp+var_4], 40h ; '@'
                mov     bx, word ptr unk_20A56
                mov     al, [bp+var_4]
                mov     [bx+0Ch], al
                mov     ax, 10h
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr unk_20A56
                call    thk_text_puts
                add     sp, 2

loc_1D30C:                              ; CODE XREF: cast2_show_text+98↓j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_6], al
                cmp     al, 1Bh
                jnz     short loc_1D322
                mov     al, 1
                jmp     short loc_1D324
; ---------------------------------------------------------------------------
                align 2

loc_1D322:                              ; CODE XREF: cast2_show_text+6D↑j
                sub     al, al

loc_1D324:                              ; CODE XREF: cast2_show_text+71↑j
                sub     ah, ah
                mov     si, ax
                or      si, si
                jnz     short loc_1D344
                cmp     [bp+var_6], 41h ; 'A'
                jb      short loc_1D33E
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                ja      short loc_1D33E
                mov     al, 1
                jmp     short loc_1D340
; ---------------------------------------------------------------------------

loc_1D33E:                              ; CODE XREF: cast2_show_text+82↑j
                                        ; cast2_show_text+8A↑j
                sub     al, al

loc_1D340:                              ; CODE XREF: cast2_show_text+8E↑j
                sub     ah, ah
                mov     si, ax

loc_1D344:                              ; CODE XREF: cast2_show_text+7C↑j
                or      si, si
                jz      short loc_1D30C
                mov     [bp+var_2], si

loc_1D34B:                              ; CODE XREF: cast2_show_text+1E↑j
                                        ; cast2_show_text+26↑j
                call    thk_res_35A8
                cmp     [bp+var_6], 1Bh
                jz      short loc_1D35D
                sub     [bp+var_6], 41h ; 'A'
                mov     byte_1DC78, 1

loc_1D35D:                              ; CODE XREF: cast2_show_text+A4↑j
                cmp     byte_1DC78, 0
                jz      short loc_1D372
                test    byte_23218, 2
                jz      short loc_1D372
                mov     [bp+var_6], 1Bh
                call    loc_1D170

loc_1D372:                              ; CODE XREF: cast2_show_text+B4↑j
                                        ; cast2_show_text+BB↑j
                mov     al, [bp+var_6]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
cast2_show_text endp

; ---------------------------------------------------------------------------
                align 8
ovl_2CAST2      ends

