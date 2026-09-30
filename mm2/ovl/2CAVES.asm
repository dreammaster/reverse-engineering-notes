; ===========================================================================

; Segment type: Pure code
ovl_2CAVES      segment byte public 'CODE' use16
                assume cs:ovl_2CAVES
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_common_helper proc near           ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 8
                push    di
                push    si
                or      byte_1DC80, 6
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, offset a44a4u4magicalS ; "\"4@4A4U4Magical slide trap!"

loc_1C14B:                              ; CODE XREF: caves_common_helper+38↓j
                lea     ax, [si+13h]
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1C14B
                mov     [bp+var_6], si
                mov     ax, 15h
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_1C17B:                              ; CODE XREF: caves_common_helper+5A↓j
                mov     ax, 2
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     si, ax
                cmp     si, 0Fh
                jg      short loc_1C17B
                mov     [bp+var_6], si
                mov     al, byte ptr [bp+var_6]
                mov     [bp+var_2], al
                mov     ax, 16h
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_1C1A3:                              ; CODE XREF: caves_common_helper+82↓j
                mov     ax, 2
                push    ax
                call    thk_read_number
                add     sp, 2
                mov     si, ax
                cmp     si, 0Fh
                jg      short loc_1C1A3
                mov     [bp+var_6], si
                mov     al, byte ptr [bp+var_6]
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                mov     g_party_x, al
                mov     al, [bp+var_4]
                mov     byte ptr g_party_y, al
                call    thk_2PLAY_B75E
                call    thk_2PLAY_A580
                mov     byte_1DC7E, 1
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
caves_common_helper endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_event_a   proc near               ; CODE XREF: seg002:08CD↑J

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
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                dec     [bp+var_2]
                mov     al, byte ptr g_party_y
                mov     cl, 4
                shl     al, cl
                add     [bp+var_2], al
                sub     si, si
                mov     cx, g_party_size
                jmp     short loc_1C22C
; ---------------------------------------------------------------------------
                align 2

loc_1C224:                              ; CODE XREF: caves_event_a+54↓j
                mov     al, [bp+var_2]
                mov     [si-6980h], al
                inc     si

loc_1C22C:                              ; CODE XREF: caves_event_a+47↑j
                cmp     si, cx
                jl      short loc_1C224
                mov     [bp+var_4], si
                call    thk_start_combat
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
caves_event_a   endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_event_b   proc near               ; CODE XREF: seg002:08E5↑J

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
                mov     dl, byte ptr g_party_y

loc_1C24C:                              ; CODE XREF: caves_event_b+8A↓j
                mov     al, g_party_x
                cmp     [si+3486h], al
                jnz     short loc_1C25C
                cmp     [si+3490h], dl
                jnz     short loc_1C25C
                inc     cx

loc_1C25C:                              ; CODE XREF: caves_event_b+17↑j
                                        ; caves_event_b+1D↑j
                or      cx, cx
                jz      short loc_1C2C0

loc_1C260:                              ; CODE XREF: caves_event_b+88↓j
                mov     [bp+var_6], si
                mov     [bp+var_4], cx
                cmp     si, 0Ah
                jl      short loc_1C26E
                jmp     loc_1C302
; ---------------------------------------------------------------------------

loc_1C26E:                              ; CODE XREF: caves_event_b+2D↑j
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh
                mov     bx, [bp+var_6]
                mov     al, [bx+349Ah]
                mov     g_party_x, al
                mov     al, [bx+34A4h]
                mov     byte ptr g_party_y, al
                call    thk_2PLAY_B75E
                mov     byte_1DC7E, 1
                or      byte_1DC80, 5
                mov     ax, 3472h
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_1C2B2:                              ; CODE XREF: caves_event_b+7B↓j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1C2B2
                sub     di, di
                mov     si, [bp+var_2]
                jmp     short loc_1C2F3
; ---------------------------------------------------------------------------

loc_1C2C0:                              ; CODE XREF: seg002:026D↑J
                                        ; caves_event_b+22↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1C260
                jmp     short loc_1C24C
; ---------------------------------------------------------------------------

loc_1C2C8:                              ; CODE XREF: caves_event_b+BB↓j
                push    di
                call    thk_char_ptr
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

loc_1C2F3:                              ; CODE XREF: caves_event_b+82↑j
                cmp     di, g_party_size
                jl      short loc_1C2C8 ; CODE XREF: seg002:0651↑J
                mov     [bp+var_8], di
                mov     [bp+var_2], si
                call    thk_2PLAY_A580

loc_1C302:                              ; CODE XREF: caves_event_b+2F↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
caves_event_b   endp


; =============== S U B R O U T I N E =======================================

; "You have found a"
; Attributes: bp-based frame

caves_found_item proc near              ; CODE XREF: seg002:08F1↑J

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
                call    thk_rand_range
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

loc_1C33E:                              ; CODE XREF: caves_found_item+66↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C370
                jmp     short loc_1C363
; ---------------------------------------------------------------------------

loc_1C346:                              ; CODE XREF: caves_found_item+70↓j
                inc     di

loc_1C347:                              ; CODE XREF: caves_found_item+33↑j
                cmp     di, g_party_size
                jge     short loc_1C37A
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_8], 0
                mov     si, ax
                mov     dx, [bp+var_4]
                sub     cx, cx

loc_1C363:                              ; CODE XREF: caves_found_item+3C↑j
                mov     bx, cx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C36C
                inc     dx

loc_1C36C:                              ; CODE XREF: caves_found_item+61↑j
                or      dx, dx
                jz      short loc_1C33E

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                                        ; caves_found_item+3A↑j
                mov     [bp+var_4], dx
                mov     [bp+var_8], cx
                or      dx, dx
                jz      short loc_1C346

loc_1C37A:                              ; CODE XREF: caves_found_item+43↑j
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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYouHaveFoundA ; "You have found a "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     al, 14h
                mul     [bp+var_A]
                add     ax, 6960h
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C3C4:                              ; CODE XREF: caves_found_item+C1↓j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1C3C4
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh

loc_1C3E7:                              ; CODE XREF: caves_found_item+79↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C3F0:                              ; CODE XREF: caves_donate_experience+BE↓p
                                        ; caves_event_c+73↓p ...
                push    bp
                mov     bp, sp
                sub     sp, 2
caves_found_item endp


loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                cmp     word ptr [bp+4], 0
                jl      short loc_1C45A
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 14h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+4]
                shl     ax, 1
                mov     [bp-2], ax
                mov     bx, ax
                push    word ptr [bx+3600h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp-2]
                push    word ptr [bx+3602h]
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2

loc_1C453:                              ; CODE XREF: ovl_2CAVES:C458↓j
                call    thk_kbd_poll
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

; "for experience (1-8) ?"
; Attributes: bp-based frame

caves_donate_experience proc near       ; CODE XREF: seg002:086D↑J

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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 3624h
                push    ax

loc_1C48E:                              ; CODE XREF: seg002:0B31↑J
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aForExperience1 ; "    for experience (1-8) ?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C4AC:                              ; CODE XREF: caves_donate_experience+64↓j
                mov     al, byte ptr g_party_size
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C4AC
                mov     [bp+var_6], si
                sub     [bp+var_6], 31h ; '1'
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, [bp+var_6]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1C4EE
                mov     [bp+var_2], 2
                jmp     short loc_1C4FE
; ---------------------------------------------------------------------------

loc_1C4EE:                              ; CODE XREF: caves_donate_experience+83↑j
                mov     bx, [bp+var_4]
                mov     ax, [bx+66h]
                or      ax, [bx+68h]
                jnz     short loc_1C4FE
                mov     [bp+var_2], 4

loc_1C4FE:                              ; CODE XREF: caves_donate_experience+8A↑j
                                        ; caves_donate_experience+95↑j
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

loc_1C51D:                              ; CODE XREF: caves_donate_experience+A0↑j
                push    [bp+var_2]
                call    loc_1C3F0
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
caves_donate_experience endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_event_c   proc near               ; CODE XREF: seg002:0879↑J

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
                cmp     g_party_size, 0
                jle     short loc_1C59B
                mov     [bp+var_A], 416h
                mov     di, [bp+var_8]
                mov     si, [bp+var_6]

loc_1C550:                              ; CODE XREF: caves_event_c+67↓j
                mov     bx, [bp+var_A]
                cmp     word ptr [bx], 18h
                jge     short loc_1C58A
                push    di
                call    thk_char_ptr
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

loc_1C58A:                              ; CODE XREF: caves_event_c+2A↑j
                add     [bp+var_A], 2
                inc     di
                cmp     di, g_party_size
                jl      short loc_1C550
                mov     [bp+var_8], di
                mov     [bp+var_6], si

loc_1C59B:                              ; CODE XREF: caves_event_c+17↑j
                mov     ax, 6
                push    ax
                call    loc_1C3F0
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
caves_event_c   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Which character shall donate all his or her gems (1-8) ?"
; Attributes: bp-based frame

caves_donate_gems proc near             ; CODE XREF: seg002:0885↑J

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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aWhichCharacter ; "Which character shall donate all"
                push    ax

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aHisOrHerGems18 ; "   his or her gems (1-8) ?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C5F6:                              ; CODE XREF: caves_donate_gems+64↓j
                mov     al, byte ptr g_party_size
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jz      short loc_1C5F6
                mov     [bp+var_A], si
                sub     ax, 31h ; '1'
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_8], ax
                mov     bx, ax
                cmp     word ptr [bx+5Ch], 0
                jnz     short loc_1C632
                mov     [bp+var_2], 0Ah
                jmp     short loc_1C65F
; ---------------------------------------------------------------------------
                align 2

loc_1C632:                              ; CODE XREF: caves_donate_gems+7C↑j
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

loc_1C65F:                              ; CODE XREF: caves_donate_gems+83↑j
                push    [bp+var_2]
                call    loc_1C3F0
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
caves_donate_gems endp


; =============== S U B R O U T I N E =======================================

; "Which character (1-8) ?"
; Attributes: bp-based frame

caves_pick_character proc near          ; CODE XREF: seg002:0891↑J

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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aWhichCharacter_0 ; "Which character (1-8) ?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C6A0:                              ; CODE XREF: caves_pick_character+4C↓j
                mov     al, byte ptr g_party_size
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
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

loc_1C6D6:                              ; CODE XREF: caves_pick_character+5F↑j
                push    [bp+var_A]
                call    thk_char_ptr
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

loc_1C71A:                              ; CODE XREF: caves_pick_character+A3↑j
                cmp     word ptr [bx+68h], 0Fh
                ja      short loc_1C72A
                jnb     short loc_1C72A
                mov     [bp+var_2], 0Eh
                jmp     short loc_1C735
; ---------------------------------------------------------------------------
                align 2

loc_1C72A:                              ; CODE XREF: caves_pick_character+B0↑j
                                        ; caves_pick_character+B2↑j
                mov     si, bx
                mov     ax, [bp+var_8]
                mov     [si+74h], ax
                mov     [bx+60h], ax

loc_1C735:                              ; CODE XREF: caves_pick_character+66↑j
                                        ; caves_pick_character+AA↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
caves_pick_character endp


; =============== S U B R O U T I N E =======================================

; "What era do you desire (1-8)?"
; Attributes: bp-based frame

caves_time_travel proc near             ; CODE XREF: seg002:089D↑J

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

loc_1C756:                              ; CODE XREF: caves_time_travel+39↓j
                inc     si

loc_1C757:                              ; CODE XREF: caves_time_travel+1A↑j
                cmp     si, g_party_size
                jge     short loc_1C775
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                test    byte ptr [bx+80h], 2
                jz      short loc_1C771
                inc     di

loc_1C771:                              ; CODE XREF: caves_time_travel+34↑j
                or      di, di
                jz      short loc_1C756

loc_1C775:                              ; CODE XREF: caves_time_travel+21↑j
                mov     [bp+var_4], di
                mov     [bp+var_6], si
                or      di, di
                jz      short loc_1C7FA
                mov     ax, 14h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aWhatEraDoYouDe ; "What era do you desire (1-8)?"
                push    ax
                call    thk_text_puts
                add     sp, 2
                call    thk_res_5440
                mov     ax, 38h ; '8'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
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
                mov     g_era, ax

loc_1C7C8:                              ; CODE XREF: caves_time_travel+84↑j
                dec     [bp+var_8]
                mov     bx, [bp+var_8]
                mov     al, [bx+36DAh]
                mov     g_party_x, al
                mov     al, [bx+36E2h]
                mov     byte ptr g_party_y, al
                sub     ah, ah
                push    ax
                mov     al, g_party_x

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

loc_1C7FA:                              ; CODE XREF: caves_time_travel+43↑j
                mov     ax, 10h
                push    ax
                call    loc_1C3F0
                add     sp, 2

loc_1C804:                              ; CODE XREF: caves_time_travel+7A↑j
                                        ; caves_time_travel+BC↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C80A:                              ; CODE XREF: caves_time_travel+1B7↓p
                                        ; caves_time_travel+253↓p
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
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+3A18h]
                call    thk_text_puts
                add     sp, 2

loc_1C834:                              ; CODE XREF: caves_time_travel+FF↓j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1C834
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C83E:                              ; CODE XREF: caves_event_d+DC↓p
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

loc_1C854:                              ; CODE XREF: caves_time_travel+1A3↓j
                cmp     [bp+arg_0], 1
                jnz     short loc_1C862
                mov     di, si
                add     di, 12h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C862:                              ; CODE XREF: caves_time_travel+11E↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_1C870
                mov     di, si
                add     di, 14h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C870:                              ; CODE XREF: caves_time_travel+12C↑j
                cmp     [bp+arg_0], 3
                jnz     short loc_1C87E
                mov     di, si
                add     di, 27h ; '''
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C87E:                              ; CODE XREF: caves_time_travel+13A↑j
                cmp     [bp+arg_0], 4
                jnz     short loc_1C88C
                mov     di, si
                add     di, 13h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C88C:                              ; CODE XREF: caves_time_travel+148↑j
                cmp     [bp+arg_0], 5
                jnz     short loc_1C89A
                mov     di, si
                add     di, 15h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C89A:                              ; CODE XREF: caves_time_travel+156↑j
                mov     di, si
                add     di, 11h

loc_1C89F:                              ; CODE XREF: caves_time_travel+125↑j
                                        ; caves_time_travel+133↑j ...
                mov     al, [di]
                mov     byte ptr [bp+var_2], al
                cmp     al, 5Ah ; 'Z'
                jbe     short loc_1C8AE
                mov     byte ptr [bp+var_2], 64h ; 'd'
                jmp     short loc_1C8B2
; ---------------------------------------------------------------------------

loc_1C8AE:                              ; CODE XREF: caves_time_travel+16C↑j
                add     byte ptr [bp+var_2], 0Ah

loc_1C8B2:                              ; CODE XREF: caves_time_travel+172↑j
                mov     al, byte ptr [bp+var_2]
                mov     [di], al

loc_1C8B7:                              ; CODE XREF: caves_time_travel+197↓j
                inc     [bp+var_8]

loc_1C8BA:                              ; CODE XREF: caves_time_travel+117↑j
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1C8E8
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                test    byte ptr [si+7Dh], 2
                jz      short loc_1C8B7
                and     byte ptr [si+7Dh], 0FDh
                cmp     [bp+arg_0], 0
                jz      short loc_1C8E0
                jmp     loc_1C854
; ---------------------------------------------------------------------------

loc_1C8E0:                              ; CODE XREF: caves_time_travel+1A1↑j
                mov     di, si
                add     di, 10h
                jmp     short loc_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C8E8:                              ; CODE XREF: caves_time_travel+186↑j
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

loc_1C8FE:                              ; CODE XREF: caves_event_d+EB↓p
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
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 7Fh
                jg      short loc_1C986
                mov     [bp+var_6], 0
                mov     si, [bp+var_2]
                jmp     short loc_1C933
; ---------------------------------------------------------------------------

loc_1C928:                              ; CODE XREF: caves_time_travel+236↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C972
                jmp     short loc_1C953
; ---------------------------------------------------------------------------

loc_1C930:                              ; CODE XREF: caves_time_travel+23D↓j
                inc     [bp+var_6]

loc_1C933:                              ; CODE XREF: caves_time_travel+1EC↑j
                mov     ax, g_party_size
                cmp     [bp+var_6], ax
                jge     short loc_1C979
                push    [bp+var_6]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_8], 0
                mov     di, ax
                mov     dx, [bp+var_A]
                sub     cx, cx

loc_1C953:                              ; CODE XREF: caves_time_travel+1F4↑j
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

loc_1C96E:                              ; CODE XREF: caves_time_travel+21F↑j
                or      si, si
                jz      short loc_1C928

loc_1C972:                              ; CODE XREF: caves_time_travel+1F2↑j
                mov     [bp+var_8], cx
                or      si, si
                jz      short loc_1C930

loc_1C979:                              ; CODE XREF: caves_time_travel+1FF↑j
                mov     [bp+var_2], si
                or      si, si
                jz      short loc_1C986
                mov     ax, 0Eh
                jmp     short loc_1C98C
; ---------------------------------------------------------------------------
                align 2

loc_1C986:                              ; CODE XREF: caves_time_travel+1E2↑j
                                        ; caves_time_travel+244↑j
                mov     ax, [bp+arg_0]
                add     ax, 7

loc_1C98C:                              ; CODE XREF: caves_time_travel+249↑j
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
caves_time_travel endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_event_d   proc near               ; CODE XREF: seg002:07DD↑J

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

loc_1C9B9:                              ; CODE XREF: caves_event_d+3C↓j
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

loc_1C9EE:                              ; CODE XREF: caves_event_d+4F↑j
                call    thk_res_5440
                or      byte_1DC80, 1
                call    thk_res_34BA

loc_1C9F9:                              ; CODE XREF: caves_event_d+F7↓j
                sub     si, si
                jmp     short loc_1C9FF
; ---------------------------------------------------------------------------
                align 2

loc_1C9FE:                              ; CODE XREF: caves_event_d+82↓j
                inc     si

loc_1C9FF:                              ; CODE XREF: caves_event_d+61↑j
                cmp     g_party_size, si
                jle     short loc_1CA1E
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                test    byte ptr [bx+7Dh], 2
                jz      short loc_1CA18
                mov     byte ptr [bp+var_6], 1

loc_1CA18:                              ; CODE XREF: caves_event_d+78↑j
                cmp     byte ptr [bp+var_6], 0
                jz      short loc_1C9FE

loc_1CA1E:                              ; CODE XREF: caves_event_d+69↑j
                mov     [bp+var_4], si
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 3A0Ch

loc_1CA30:                              ; CODE XREF: caves_event_d+B3↓j
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
                cmp     si, 6
                jl      short loc_1CA30
                mov     [bp+var_4], si

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
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

loc_1CA82:                              ; CODE XREF: caves_event_d+D7↑j
                push    [bp+var_2]
                call    loc_1C8FE

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                add     sp, 2

loc_1CA8B:                              ; CODE XREF: caves_event_d+CE↑j
                                        ; caves_event_d+E6↑j
                cmp     [bp+var_2], 1Bh
                jz      short loc_1CA94
                jmp     loc_1C9F9
; ---------------------------------------------------------------------------

loc_1CA94:                              ; CODE XREF: caves_event_d+F5↑j
                call    thk_res_35A8

loc_1CA97:                              ; CODE XREF: caves_event_d+51↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CAA0:                              ; CODE XREF: caves_event_d+176↓p
                                        ; caves_event_d+192↓p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     di, di
                jmp     short loc_1CAB5
; ---------------------------------------------------------------------------

loc_1CAAC:                              ; CODE XREF: caves_event_d+13C↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1CAD8
                jmp     short loc_1CAD1
; ---------------------------------------------------------------------------

loc_1CAB4:                              ; CODE XREF: caves_event_d+144↓j
                inc     di

loc_1CAB5:                              ; CODE XREF: caves_event_d+110↑j
                cmp     di, g_party_size
                jge     short loc_1CAE0
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_6], 0
                mov     si, ax
                mov     dl, [bp+arg_0]
                sub     cx, cx

loc_1CAD1:                              ; CODE XREF: caves_event_d+118↑j
                mov     bx, cx
                cmp     [bx+si+3Ah], dl
                jnz     short loc_1CAAC

loc_1CAD8:                              ; CODE XREF: caves_event_d+116↑j
                mov     [bp+var_6], cx
                cmp     cx, 6
                jz      short loc_1CAB4

loc_1CAE0:                              ; CODE XREF: caves_event_d+11F↑j
                mov     [bp+var_4], di
                cmp     [bp+var_6], 6
                jnz     short loc_1CAEE
                mov     [bp+var_6], 0FFFFh

loc_1CAEE:                              ; CODE XREF: caves_event_d+14D↑j
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

loc_1CB00:                              ; CODE XREF: sub_1CBCA+5B↓p
                                        ; sub_1CD4C+1A↓p
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

loc_1CB1C:                              ; CODE XREF: sub_1CBCA+72↓p
                                        ; sub_1CD4C+4B↓p
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
caves_event_d   endp

                add     sp, 4
; START OF FUNCTION CHUNK FOR caves_event_d

loc_1CB45:                              ; CODE XREF: caves_event_d+19F↑j
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR caves_event_d
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB4A       proc near               ; CODE XREF: sub_1D094+135↓p
                                        ; sub_1D094+169↓p

var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 0
                mov     bx, [bp+arg_0]
                test    byte ptr [bx+7Ch], 2
                jz      short loc_1CBBF
                mov     al, [bx+78h]
                mov     [bp+var_6], al
                cmp     byte_22E12, 0
                jnz     short loc_1CB6E
                mov     byte_22E12, al

loc_1CB6E:                              ; CODE XREF: sub_1CB4A+1F↑j
                mov     al, byte_22E12
                cmp     [bp+var_6], al
                jnz     short loc_1CBBF
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CBBF
                inc     [bp+var_2]
                sub     si, si
                mov     cl, [bp+var_6]

loc_1CB84:                              ; CODE XREF: sub_1CB4A+4C↓j
                cmp     [si+3E3Eh], cl
                jbe     short loc_1CB90

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                                        ; sub_1CB4A+4A↓j
                mov     [bp+var_4], si
                jmp     short loc_1CB98
; ---------------------------------------------------------------------------
                align 2

loc_1CB90:                              ; CODE XREF: sub_1CB4A+3E↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1CB8A
                jmp     short loc_1CB84
; ---------------------------------------------------------------------------

loc_1CB98:                              ; CODE XREF: sub_1CB4A+43↑j
                mov     bx, [bp+var_4]
                shl     bx, 1           ; CODE XREF: seg002:080D↑J
                shl     bx, 1
                mov     ax, [bx+3E48h]
                mov     dx, [bx+3E4Ah]
                mov     word_216C0, ax
                mov     word_216C2, dx
                mov     bx, [bp+arg_0]
                add     [bx+62h], ax
                adc     [bx+64h], dx
                mov     byte ptr [bx+78h], 0
                and     byte ptr [bx+7Ch], 0FDh

loc_1CBBF:                              ; CODE XREF: sub_1CB4A+12↑j
                                        ; sub_1CB4A+2A↑j ...
                mov     al, [bp+var_2]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CB4A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CBCA       proc near               ; CODE XREF: sub_1D094+14B↓p
                                        ; sub_1D094+1AE↓p

var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                mov     [bp+var_6], 0
                mov     [bp+var_4], 0
                mov     [bp+var_2], 0   ; CODE XREF: seg002:01AD↑J
                mov     [bp+var_C], 0
                mov     [bp+var_8], 0
                mov     bx, [bp+arg_0]
                mov     al, [bx+78h]
                mov     [bp+var_A], al
                or      al, al
                jnz     short loc_1CBF8

loc_1CBF3:                              ; CODE XREF: sub_1CBCA+42↓j
                sub     ax, ax
                jmp     loc_1CC84
; ---------------------------------------------------------------------------

loc_1CBF8:                              ; CODE XREF: sub_1CBCA+27↑j
                cmp     byte_22E13, 0
                jnz     short loc_1CC02
                inc     [bp+var_C]

loc_1CC02:                              ; CODE XREF: sub_1CBCA+33↑j
                mov     al, byte_22E13
                cmp     [bp+var_A], al
                jz      short loc_1CC0E
                or      al, al
                jnz     short loc_1CBF3

loc_1CC0E:                              ; CODE XREF: sub_1CBCA+3E↑j
                cmp     [bp+var_C], 0
                jnz     short loc_1CC17
                inc     [bp+var_2]

loc_1CC17:                              ; CODE XREF: sub_1CBCA+48↑j
                cmp     [bp+var_C], 0
                jz      short loc_1CC42
                mov     al, [bp+var_A]
                sub     ah, ah
                mov     si, ax
                push    si
                call    loc_1CB00
                add     sp, 2
                mov     [bp+var_8], ax
                or      ax, ax
                jz      short loc_1CC42
                inc     [bp+var_2]
                mov     al, [bp+var_A]
                mov     byte_22E13, al
                push    si
                call    loc_1CB1C
                add     sp, 2

loc_1CC42:                              ; CODE XREF: sub_1CBCA+51↑j
                                        ; sub_1CBCA+66↑j
                cmp     [bp+var_2], 0
                jz      short loc_1CC7F
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CC7F
                inc     [bp+var_6]
                mov     byte ptr [bx+78h], 0
                mov     al, 14h
                mul     [bp+var_A]
                mov     bx, ax
                mov     ax, [bx+6972h]
                sub     dx, dx
                mov     cl, 3

loc_1CC67:                              ; CODE XREF: sub_1CBCA+A3↓j
                shl     ax, 1
                rcl     dx, 1
                dec     cl
                jnz     short loc_1CC67
                mov     word_216C0, ax
                mov     word_216C2, dx
                mov     bx, [bp+arg_0]
                add     [bx+62h], ax
                adc     [bx+64h], dx

loc_1CC7F:                              ; CODE XREF: sub_1CBCA+7C↑j
                                        ; sub_1CBCA+85↑j
                mov     al, [bp+var_6]
                sub     ah, ah

loc_1CC84:                              ; CODE XREF: sub_1CBCA+2B↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CBCA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CC8A       proc near               ; CODE XREF: sub_1CDB0+16↓p

var_C           = word ptr -0Ch
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     ax, [bp+arg_0]
                mov     cl, 3
                shl     ax, cl
                mov     [bp+var_C], ax
                mov     bx, ax
                mov     al, [bx+3E1Eh]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_6], al
                dec     [bp+var_6]
                mov     [bp+var_4], 1   ; CODE XREF: seg002:08FD↑J
                mov     si, [bp+var_C]
                mov     di, si
                mov     dl, [bp+var_2]
                mov     cx, [bp+var_4]

loc_1CCC9:                              ; CODE XREF: sub_1CC8A+6A↓j
                mov     al, [bp+var_6]
                mov     bx, cx
                cmp     [bx+di+3E1Eh], al
                jbe     short loc_1CCD8
                inc     dl
                jmp     short loc_1CCE1
; ---------------------------------------------------------------------------

loc_1CCD8:                              ; CODE XREF: sub_1CC8A+48↑j
                mov     bx, cx
                mov     al, [bx+si+3E1Fh]
                sub     [bp+var_6], al

loc_1CCE1:                              ; CODE XREF: sub_1CC8A+4C↑j
                or      dl, dl
                jz      short loc_1CCEE

loc_1CCE5:                              ; CODE XREF: sub_1CC8A+68↓j
                mov     [bp+var_2], dl
                mov     [bp+var_4], cx
                jmp     short loc_1CCF6
; ---------------------------------------------------------------------------
                align 2

loc_1CCEE:                              ; CODE XREF: sub_1CC8A+59↑j
                inc     cx
                cmp     cx, 7
                jge     short loc_1CCE5
                jmp     short loc_1CCC9
; ---------------------------------------------------------------------------

loc_1CCF6:                              ; CODE XREF: sub_1CC8A+61↑j
                dec     [bp+var_4]
                mov     si, [bp+arg_0]
                mov     ax, si
                shl     si, 1
                add     si, ax
                shl     si, 1
                mov     bx, [bp+var_4]
                mov     al, [bx+si+3E0Ch]
                add     [bp+var_6], al
                mov     al, [bp+var_6]
                mov     byte_22E13, al
                sub     ah, ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CC8A       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD1C       proc near               ; CODE XREF: sub_1CDB0+27↓p

var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, [bp+arg_0]
                mov     al, [bx+3E36h]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                mov     [bp+var_2], al
                mov     bx, [bp+arg_0]
                mov     al, [bx+3E3Ah]
                add     [bp+var_2], al
                mov     al, [bp+var_2]
                mov     byte_22E12, al
                sub     ah, ah
                mov     sp, bp
                pop     bp
                retn
sub_1CD1C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD4C       proc near               ; CODE XREF: sub_1D094+20↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     [bp+var_2], 0
                sub     si, si
                mov     di, bp
                sub     di, 0Ah

loc_1CD60:                              ; CODE XREF: sub_1CD4C+29↓j
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
                mov     [bp+var_4], si
                cmp     [bp+var_A], 0
                jz      short loc_1CDA6
                cmp     [bp+var_8], 0
                jz      short loc_1CDA6
                cmp     [bp+var_6], 0
                jz      short loc_1CDA6
                inc     [bp+var_2]
                sub     si, si

loc_1CD91:                              ; CODE XREF: sub_1CD4C+55↓j
                mov     ax, si
                add     ax, 0E2h
                push    ax
                call    loc_1CB1C
                add     sp, 2
                inc     si
                cmp     si, 3
                jl      short loc_1CD91
                mov     [bp+var_4], si

loc_1CDA6:                              ; CODE XREF: sub_1CD4C+32↑j
                                        ; sub_1CD4C+38↑j ...
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CD4C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CDB0       proc near               ; CODE XREF: sub_1D3C4+17B↓p

var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     [bp+var_A], 1
                cmp     byte_22E14, 0
                jnz     short loc_1CDD4
                push    [bp+arg_0]
                call    sub_1CC8A
                add     sp, 2
                mov     [bp+var_2], al
                dec     [bp+var_A]
                jmp     short loc_1CDE0
; ---------------------------------------------------------------------------

loc_1CDD4:                              ; CODE XREF: sub_1CDB0+11↑j
                push    [bp+arg_0]
                call    sub_1CD1C
                add     sp, 2
                mov     [bp+var_2], al

loc_1CDE0:                              ; CODE XREF: sub_1CDB0+22↑j
                sub     di, di
                mov     si, [bp+var_6]
                jmp     short loc_1CE02
; ---------------------------------------------------------------------------
                align 2

loc_1CDE8:                              ; CODE XREF: sub_1CDB0+56↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [bp+var_2]
                mov     [si+78h], al
                and     byte ptr [si+7Ch], 0FEh
                mov     al, [bp+var_A]
                or      [si+7Ch], al
                inc     di

loc_1CE02:                              ; CODE XREF: sub_1CDB0+35↑j
                cmp     di, g_party_size
                jl      short loc_1CDE8
                mov     [bp+var_8], di
                mov     [bp+var_6], si
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                cmp     byte_22E14, 1
                jnz     short loc_1CE36
                mov     al, [bp+var_2]
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     [bp+var_4], 9E0Eh ; CODE XREF: seg002:0831↑J
                jmp     short loc_1CE41
; ---------------------------------------------------------------------------
                align 2

loc_1CE36:                              ; CODE XREF: sub_1CDB0+6D↑j
                mov     al, 14h
                mul     [bp+var_2]
                add     ax, 6960h
                mov     [bp+var_4], ax

loc_1CE41:                              ; CODE XREF: sub_1CDB0+83↑j
                mov     ax, 12h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 3E7Ch
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aPartyIsToSeekT ; " party, is to seek the "
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    [bp+var_4]
                call    thk_text_puts
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CE7E:                              ; CODE XREF: sub_1CDB0+F7↓j
                lea     ax, [si+14h]
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, byte_22E14
                sub     bh, bh
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+di+3DF2h]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 2
                jl      short loc_1CE7E
                mov     [bp+var_8], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CDB0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CEB2       proc near               ; CODE XREF: sub_1D3C4+186↓p

var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_A], 0
                mov     [bp+var_2], 10h
                mov     [bp+var_4], 1
                cmp     byte_22E14, 0   ; CODE XREF: seg002:07AD↑J
                jnz     short loc_1CED4
                mov     [bp+var_2], 8
                dec     [bp+var_4]

loc_1CED4:                              ; CODE XREF: sub_1CEB2+19↑j
                mov     [bp+var_8], 0
                cmp     g_party_size, 0
                jle     short loc_1CF25
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     [bp+var_E], ax
                mov     si, [bp+var_8]

loc_1CEEB:                              ; CODE XREF: sub_1CEB2+6B↓j
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                mov     al, [di+7Ch]
                mov     [bp+var_C], al
                sub     ah, ah
                test    [bp+var_E], ax
                jnz     short loc_1CF18
                inc     [bp+var_A]
                and     [bp+var_C], 0FEh
                or      [bp+var_C], 4
                mov     al, [bp+var_4]
                or      [bp+var_C], al
                mov     al, [bp+var_C]
                mov     [di+7Ch], al

loc_1CF18:                              ; CODE XREF: sub_1CEB2+4D↑j
                inc     si
                cmp     si, g_party_size
                jl      short loc_1CEEB
                mov     [bp+var_6], di
                mov     [bp+var_8], si

loc_1CF25:                              ; CODE XREF: sub_1CEB2+2C↑j
                cmp     [bp+var_A], 0
                jz      short loc_1CF67
                mov     ax, 2           ; CODE XREF: seg002:050D↑J
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CF39:                              ; CODE XREF: sub_1CEB2+B0↓j
                lea     ax, [si+12h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, byte_22E14
                sub     bh, bh
                mov     cl, 3
                shl     bx, cl
                push    word ptr [bx+di+3DE2h]
                call    thk_text_puts
                add     sp, 2
                add     di, 2           ; CODE XREF: seg002:0AE9↑J
                inc     si
                cmp     si, 4
                jl      short loc_1CF39
                mov     [bp+var_8], si

loc_1CF67:                              ; CODE XREF: sub_1CEB2+77↑j
                cmp     [bp+var_A], 0
                jz      short loc_1CF72
                mov     ax, 1
                jmp     short loc_1CF75
; ---------------------------------------------------------------------------

loc_1CF72:                              ; CODE XREF: sub_1CEB2+B9↑j
                mov     ax, 0FFFFh

loc_1CF75:                              ; CODE XREF: sub_1CEB2+BE↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CEB2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CF7C       proc near               ; CODE XREF: sub_1D094:loc_1D214↓p
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax

loc_1CF84:                              ; CODE XREF: seg002:062D↑J
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C4
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C6
                call    thk_text_puts
                add     sp, 2
                mov     al, byte_22E12
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     ax, 9E0Eh
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C8
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr unk_216CA
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_216C2
                push    word_216C0
                call    thk_text_put_number
                add     sp, 8
                retn
sub_1CF7C       endp


; =============== S U B R O U T I N E =======================================


sub_1D00C       proc near               ; CODE XREF: sub_1D094+17B↓p
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C4
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C6
                call    thk_text_puts
                add     sp, 2
                mov     al, 14h
                mul     byte_22E13
                add     ax, 6960h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_216C8
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr unk_216CA
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_216C2
                push    word_216C0
                call    thk_text_put_number
                add     sp, 8
                retn
sub_1D00C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D094       proc near               ; CODE XREF: sub_1D3C4+43↓p

var_12          = word ptr -12h
var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                sub     di, di
                mov     [bp+var_12], di
                mov     [bp+var_8], di
                mov     si, [bp+var_4]
                jmp     short loc_1D10A
; ---------------------------------------------------------------------------
                align 2

loc_1D0AA:                              ; CODE XREF: sub_1D094+A4↓j
                test    [bp+var_2], 8
                jnz     short loc_1D0C0
                or      di, di
                jnz     short loc_1D0B9
                call    sub_1CD4C
                mov     di, ax

loc_1D0B9:                              ; CODE XREF: sub_1D094+1E↑j
                or      di, di
                jz      short loc_1D0C0

loc_1D0BD:                              ; CODE XREF: sub_1D094:loc_1D154↓j
                inc     [bp+var_12]

loc_1D0C0:                              ; CODE XREF: sub_1D094+1A↑j
                                        ; sub_1D094+27↑j ...
                cmp     [bp+var_12], 0
                jz      short loc_1D107
                inc     di
                and     byte ptr [si+7Ch], 0FBh
                mov     [bp+var_2], 10h
                cmp     byte_22E14, 0
                jnz     short loc_1D0DA
                mov     [bp+var_2], 8

loc_1D0DA:                              ; CODE XREF: sub_1D094+40↑j
                mov     al, [bp+var_2]
                or      [si+7Ch], al
                mov     [bp+var_10], 86A0h
                mov     [bp+var_E], 1
                cmp     byte_22E14, 1
                jnz     short loc_1D0FB
                mov     [bp+var_10], 4240h
                mov     [bp+var_E], 0Fh

loc_1D0FB:                              ; CODE XREF: sub_1D094+5B↑j
                mov     ax, [bp+var_10]
                mov     dx, [bp+var_E]
                add     [si+62h], ax
                adc     [si+64h], dx

loc_1D107:                              ; CODE XREF: sub_1D094+30↑j
                                        ; sub_1D094+93↓j ...
                inc     [bp+var_8]

loc_1D10A:                              ; CODE XREF: sub_1D094+13↑j
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1D158
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [si+7Ch]
                mov     [bp+var_2], al
                test    [bp+var_2], 4
                jz      short loc_1D107
                and     al, 1
                cmp     al, byte_22E14
                jnz     short loc_1D107
                cmp     byte_22E14, 1
                jz      short loc_1D13B
                jmp     loc_1D0AA
; ---------------------------------------------------------------------------

loc_1D13B:                              ; CODE XREF: sub_1D094+A2↑j
                mov     al, [bp+var_2]
                and     al, 0E0h
                cmp     al, 0E0h
                jz      short loc_1D147
                jmp     loc_1D0C0
; ---------------------------------------------------------------------------

loc_1D147:                              ; CODE XREF: sub_1D094+AE↑j
                and     byte ptr [si+7Ch], 1Fh
                test    [bp+var_2], 10h
                jz      short loc_1D154
                jmp     loc_1D0C0
; ---------------------------------------------------------------------------

loc_1D154:                              ; CODE XREF: sub_1D094+BB↑j
                jmp     loc_1D0BD
; ---------------------------------------------------------------------------
                align 2

loc_1D158:                              ; CODE XREF: sub_1D094+7C↑j
                                        ; seg002:0825↑J
                mov     [bp+var_A], di
                mov     [bp+var_4], si
                or      di, di
                jz      short loc_1D1BE
                mov     ax, 12h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYouHaveDoneEve ; "You have done everyone a great service"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAndYouShallBeR ; "and you shall be rewarded. "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    [bp+var_E]
                push    [bp+var_10]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aExperiencePoin ; "  experience points!"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1D1BE:                              ; CODE XREF: sub_1D094+CC↑j
                mov     [bp+var_8], 0
                jmp     short loc_1D21D
; ---------------------------------------------------------------------------
                align 2

loc_1D1C6:                              ; CODE XREF: sub_1D094+1A9↓j
                push    [bp+var_4]
                call    sub_1CB4A

loc_1D1CC:                              ; CODE XREF: sub_1D094+1B1↓j
                add     sp, 2
                mov     [bp+var_2], al
                or      al, al
                jz      short loc_1D21A
                sub     si, si
                mov     di, [bp+var_6]
                jmp     short loc_1D1E6
; ---------------------------------------------------------------------------
                align 2

loc_1D1DE:                              ; CODE XREF: sub_1D094+166↓j
                push    di
                call    sub_1CBCA

loc_1D1E2:                              ; CODE XREF: sub_1D094+16C↓j
                add     sp, 2
                inc     si

loc_1D1E6:                              ; CODE XREF: sub_1D094+147↑j
                cmp     g_party_size, si
                jle     short loc_1D202
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                cmp     byte_22E14, 1
                jnz     short loc_1D1DE
                push    di
                call    sub_1CB4A
                jmp     short loc_1D1E2
; ---------------------------------------------------------------------------

loc_1D202:                              ; CODE XREF: sub_1D094+156↑j
                mov     [bp+var_6], di
                mov     [bp+var_C], si
                cmp     byte_22E14, 0
                jnz     short loc_1D214
                call    sub_1D00C
                jmp     short loc_1D217
; ---------------------------------------------------------------------------

loc_1D214:                              ; CODE XREF: sub_1D094+179↑j
                call    sub_1CF7C

loc_1D217:                              ; CODE XREF: sub_1D094+17E↑j
                inc     [bp+var_A]

loc_1D21A:                              ; CODE XREF: sub_1D094+140↑j
                inc     [bp+var_8]

loc_1D21D:                              ; CODE XREF: sub_1D094+12F↑j
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1D248
                push    [bp+var_8]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                sub     al, al
                mov     byte_22E13, al
                mov     byte_22E12, al
                cmp     byte_22E14, al
                jnz     short loc_1D1C6
                push    [bp+var_4]
                call    sub_1CBCA
                jmp     short loc_1D1CC
; ---------------------------------------------------------------------------
                align 2

loc_1D248:                              ; CODE XREF: sub_1D094+18F↑j
                mov     ax, [bp+var_A]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D094       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D252       proc near               ; CODE XREF: sub_1D3C4:loc_1D411↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_6], 0
                sub     di, di
                mov     si, [bp+var_8]
                jmp     loc_1D2F1
; ---------------------------------------------------------------------------
                align 2

loc_1D268:                              ; CODE XREF: sub_1D252+F3↓j
                mov     ax, offset aThreeSwords ; "three swords."

loc_1D26B:                              ; CODE XREF: sub_1D252+F9↓j
                push    ax
                jmp     short loc_1D2D9
; ---------------------------------------------------------------------------

loc_1D26E:                              ; CODE XREF: sub_1D252+BA↓j
                cmp     byte ptr [si+78h], 0
                jz      short loc_1D2E2
                test    [bp+var_2], 1
                jz      short loc_1D294
                mov     al, [si+78h]
                mov     byte_22E12, al
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     [bp+var_4], 9E0Eh
                jmp     short loc_1D2A6
; ---------------------------------------------------------------------------
                align 2

loc_1D294:                              ; CODE XREF: sub_1D252+26↑j
                mov     al, [si+78h]
                mov     byte_22E13, al
                mov     al, 14h
                mul     byte_22E13
                add     ax, 6960h
                mov     [bp+var_4], ax

loc_1D2A6:                              ; CODE XREF: sub_1D252+3F↑j
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_21658
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_2165A
                call    thk_text_puts
                add     sp, 2
                push    [bp+var_4]

loc_1D2D9:                              ; CODE XREF: sub_1D252+1A↑j
                call    thk_text_puts
                add     sp, 2
                inc     [bp+var_6]

loc_1D2E2:                              ; CODE XREF: sub_1D252+20↑j
                cmp     [bp+var_6], 0
                jz      short loc_1D2F0

loc_1D2E8:                              ; CODE XREF: sub_1D252+A3↓j
                mov     [bp+var_A], di
                mov     [bp+var_8], si
                jmp     short loc_1D34E
; ---------------------------------------------------------------------------

loc_1D2F0:                              ; CODE XREF: sub_1D252+94↑j
                inc     di

loc_1D2F1:                              ; CODE XREF: sub_1D252+12↑j
                cmp     di, g_party_size
                jge     short loc_1D2E8
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [si+7Ch]
                mov     [bp+var_2], al
                test    [bp+var_2], 4
                jnz     short loc_1D30F
                jmp     loc_1D26E
; ---------------------------------------------------------------------------

loc_1D30F:                              ; CODE XREF: sub_1D252+B8↑j
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_21658
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_2165A
                call    thk_text_puts
                add     sp, 2
                test    [bp+var_2], 1
                jnz     short loc_1D348
                jmp     loc_1D268
; ---------------------------------------------------------------------------

loc_1D348:                              ; CODE XREF: sub_1D252+F1↑j
                mov     ax, offset aThreeBeasts ; "three beasts."
                jmp     loc_1D26B
; ---------------------------------------------------------------------------

loc_1D34E:                              ; CODE XREF: sub_1D252+9C↑j
                cmp     [bp+var_6], 0
                jz      short loc_1D384
                mov     ax, 14h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBegoneUntilYou ; "Begone until you have completed"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYourQuest ; "your quest!"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1D384:                              ; CODE XREF: sub_1D252+100↑j
                mov     ax, [bp+var_6]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D252       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D38E       proc near               ; CODE XREF: sub_1D3C4+8B↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                jmp     short loc_1D39D
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D398:                              ; CODE XREF: sub_1D38E+1E↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_1D3AE

loc_1D39D:                              ; CODE XREF: sub_1D38E+7↑j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1D398

loc_1D3AE:                              ; CODE XREF: sub_1D38E+D↑j
                mov     [bp+var_2], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_1D3BC
                mov     ax, 1
                jmp     short loc_1D3BE
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D3BC:                              ; CODE XREF: sub_1D38E+26↑j
                sub     ax, ax

loc_1D3BE:                              ; CODE XREF: sub_1D38E+2B↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1D38E       endp

; ---------------------------------------------------------------------------
                db  90h

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D3C4       proc near               ; CODE XREF: caves_event_e+3↓p
                                        ; caves_event_f+4↓p

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
                mov     [bp+var_2], 0

loc_1D3D1:                              ; CODE XREF: sub_1D3C4+24↓j
                lea     ax, [bp+var_6]
                push    ax
                mov     ax, offset aMonstersDat_1 ; "monsters.dat"
                push    ax
                call    thk_load_file_alloc
                add     sp, 4
                mov     word ptr dword_1DD54, ax
                mov     word ptr dword_1DD54+2, dx
                or      dx, ax
                jz      short loc_1D3D1
                mov     byte_2294F, 0FDh
                mov     byte_1DC80, 7
                call    thk_res_34BA
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     al, byte ptr [bp+arg_0]
                mov     byte_22E14, al
                call    sub_1D094
                or      ax, ax
                jz      short loc_1D411
                jmp     loc_1D55F
; ---------------------------------------------------------------------------

loc_1D411:                              ; CODE XREF: sub_1D3C4+48↑j
                call    sub_1D252
                or      ax, ax
                jz      short loc_1D41B
                jmp     loc_1D55F
; ---------------------------------------------------------------------------

loc_1D41B:                              ; CODE XREF: sub_1D3C4+52↑j
                sub     si, si
                mov     ax, [bp+arg_0]
                mov     cl, 3
                shl     ax, cl
                mov     [bp+var_8], ax
                mov     di, ax
                add     di, 3DD2h

loc_1D42D:                              ; CODE XREF: sub_1D3C4+86↓j
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
                jl      short loc_1D42D
                mov     [bp+var_6], si
                call    sub_1D38E
                mov     [bp+var_6], ax
                or      ax, ax
                jnz     short loc_1D460
                inc     [bp+var_2]
                jmp     loc_1D55F
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D460:                              ; CODE XREF: sub_1D3C4+93↑j
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                call    thk_res_5440
                sub     si, si
                mov     di, 3DFAh

loc_1D472:                              ; CODE XREF: sub_1D3C4+CB↓j
                lea     ax, [si+12h]
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
                jl      short loc_1D472
                mov     [bp+var_6], si
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     byte_22E14, 0
                jnz     short loc_1D4AE
                mov     ax, offset aHoardallAD ; "Hoardall (A-D)?"
                jmp     short loc_1D4B1
; ---------------------------------------------------------------------------

loc_1D4AE:                              ; CODE XREF: sub_1D3C4+E3↑j
                mov     ax, offset aSlayerAD ; "Slayer (A-D)?"

loc_1D4B1:                              ; CODE XREF: sub_1D3C4+E8↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 12h
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                sub     si, si
                mov     di, 3E00h

loc_1D4D3:                              ; CODE XREF: sub_1D3C4+12C↓j
                lea     ax, [si+12h]
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1D4D3
                mov     [bp+var_6], si

loc_1D4F5:                              ; CODE XREF: sub_1D3C4+168↓j
                                        ; sub_1D3C4+196↓j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     di, ax
                cmp     di, 1Bh
                jnz     short loc_1D50C
                mov     ax, 1
                jmp     short loc_1D50E
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D50C:                              ; CODE XREF: sub_1D3C4+140↑j
                sub     ax, ax

loc_1D50E:                              ; CODE XREF: sub_1D3C4+145↑j
                mov     si, ax
                or      si, si
                jnz     short loc_1D52A
                mov     ax, di
                cmp     ax, 41h ; 'A'
                jb      short loc_1D526
                cmp     ax, 44h ; 'D'
                ja      short loc_1D526
                mov     ax, 1
                jmp     short loc_1D528
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D526:                              ; CODE XREF: sub_1D3C4+155↑j
                                        ; sub_1D3C4+15A↑j
                sub     ax, ax

loc_1D528:                              ; CODE XREF: sub_1D3C4+15F↑j
                mov     si, ax

loc_1D52A:                              ; CODE XREF: sub_1D3C4+14E↑j
                or      si, si
                jz      short loc_1D4F5
                mov     [bp+var_6], si
                cmp     di, 1Bh
                jz      short loc_1D539
                sub     di, 41h ; 'A'

loc_1D539:                              ; CODE XREF: sub_1D3C4+170↑j
                cmp     di, 3
                jge     short loc_1D545
                push    di
                call    sub_1CDB0
                add     sp, 2

loc_1D545:                              ; CODE XREF: sub_1D3C4+178↑j
                cmp     di, 3
                jnz     short loc_1D54F
                call    sub_1CEB2
                mov     di, ax

loc_1D54F:                              ; CODE XREF: sub_1D3C4+184↑j
                cmp     di, 1Bh
                jnz     short loc_1D557
                inc     [bp+var_2]

loc_1D557:                              ; CODE XREF: sub_1D3C4+18E↑j
                cmp     di, 0FFFFh
                jz      short loc_1D4F5
                mov     [bp+var_4], di

loc_1D55F:                              ; CODE XREF: sub_1D3C4+4A↑j
                                        ; sub_1D3C4+54↑j ...
                cmp     [bp+var_2], 0
                jz      short loc_1D5B6
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aThenBegoneKnav ; "Then begone, knave!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 64h ; 'd'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     g_party_x, 9
                mov     byte ptr g_party_y, 0Ah
                cmp     byte_22E14, 1
                jnz     short loc_1D5AC
                mov     g_party_x, 3
                mov     byte ptr g_party_y, 5

loc_1D5AC:                              ; CODE XREF: sub_1D3C4+1DC↑j
                call    thk_2PLAY_B75E
                mov     byte_1DC7E, 1
                jmp     short loc_1D5C4
; ---------------------------------------------------------------------------

loc_1D5B6:                              ; CODE XREF: sub_1D3C4+19F↑j
                call    thk_res_35A8
                call    thk_res_5426

loc_1D5BC:                              ; CODE XREF: sub_1D3C4+1FE↓j
                call    thk_monster_anim_step
                cmp     ax, 20h ; ' '
                jnz     short loc_1D5BC

loc_1D5C4:                              ; CODE XREF: sub_1D3C4+1F0↑j
                push    word ptr dword_1DD54+2
                push    word ptr dword_1DD54
                call    thk_free_far_block
                add     sp, 4
                call    thk_res_35A8
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D3C4       endp


; =============== S U B R O U T I N E =======================================


caves_event_e   proc near               ; CODE XREF: seg002:0795↑J
                sub     ax, ax
                push    ax
                call    sub_1D3C4
                add     sp, 2
                retn
caves_event_e   endp


; =============== S U B R O U T I N E =======================================


caves_event_f   proc near               ; CODE XREF: seg002:07A1↑J
                mov     ax, 1
                push    ax
                call    sub_1D3C4
                add     sp, 2
                retn
caves_event_f   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

caves_event_g   proc near               ; CODE XREF: seg002:0819↑J

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

loc_1D614:                              ; CODE XREF: caves_event_g+3E↓j
                mov     si, [bp+var_6]
                add     si, 55C6h
                mov     di, 4

loc_1D61E:                              ; CODE XREF: caves_event_g+33↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D61E
                add     [bp+var_6], 8
                cmp     [bp+var_6], 0B0h
                jl      short loc_1D614
                or      byte_1DC80, 6
                mov     bx, g_era
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

loc_1D667:                              ; CODE XREF: caves_event_g+90↓j
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
                jl      short loc_1D667
                mov     [bp+var_4], si
                call    thk_res_5426

loc_1D68C:                              ; CODE XREF: caves_event_g+9E↓j
                call    thk_monster_anim_step
                cmp     ax, 20h ; ' '
                jnz     short loc_1D68C
                call    thk_res_35A8
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
caves_event_g   endp

ovl_2CAVES      ends

