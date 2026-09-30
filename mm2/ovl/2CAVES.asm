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
caves_common_helper endp ; sp-analysis failed


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

sub_1CB4A       proc near               ; CODE XREF: ovl_2CAVES:D1C9↓p
                                        ; ovl_2CAVES:D1FD↓p

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

sub_1CBCA       proc near               ; CODE XREF: ovl_2CAVES:D1DF↓p
                                        ; ovl_2CAVES:D242↓p

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

sub_1CC8A       proc near               ; CODE XREF: ovl_2CAVES:CDC6↓p

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

sub_1CD1C       proc near               ; CODE XREF: ovl_2CAVES:CDD7↓p

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

sub_1CD4C       proc near               ; CODE XREF: ovl_2CAVES:D0B4↓p

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
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    di
                push    si
                mov     byte ptr [bp-0Ah], 1
                cmp     byte_22E14, 0
                jnz     short loc_1CDD4
                push    word ptr [bp+4]
                call    sub_1CC8A
                add     sp, 2
                mov     [bp-2], al
                dec     byte ptr [bp-0Ah]
                jmp     short loc_1CDE0
; ---------------------------------------------------------------------------

loc_1CDD4:                              ; CODE XREF: ovl_2CAVES:CDC1↑j
                push    word ptr [bp+4]
                call    sub_1CD1C
                add     sp, 2
                mov     [bp-2], al

loc_1CDE0:                              ; CODE XREF: ovl_2CAVES:CDD2↑j
                sub     di, di
                mov     si, [bp-6]
                jmp     short loc_1CE02
; ---------------------------------------------------------------------------
                align 2

loc_1CDE8:                              ; CODE XREF: ovl_2CAVES:CE06↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [bp-2]
                mov     [si+78h], al
                and     byte ptr [si+7Ch], 0FEh
                mov     al, [bp-0Ah]
                or      [si+7Ch], al
                inc     di

loc_1CE02:                              ; CODE XREF: ovl_2CAVES:CDE5↑j
                cmp     di, g_party_size
                jl      short loc_1CDE8
                mov     [bp-8], di
                mov     [bp-6], si
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                cmp     byte_22E14, 1
                jnz     short loc_1CE36
                mov     al, [bp-2]
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     word ptr [bp-4], 9E0Eh
                                        ; CODE XREF: seg002:0831↑J
                jmp     short loc_1CE41
; ---------------------------------------------------------------------------
                align 2

loc_1CE36:                              ; CODE XREF: ovl_2CAVES:CE1D↑j
                mov     al, 14h
                mul     byte ptr [bp-2]
                add     ax, 6960h
                mov     [bp-4], ax

loc_1CE41:                              ; CODE XREF: ovl_2CAVES:CE33↑j
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
                push    word ptr [bp-4]
                call    thk_text_puts
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CE7E:                              ; CODE XREF: ovl_2CAVES:CEA7↓j
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
                mov     [bp-8], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     byte ptr [bp-0Ah], 0
                mov     byte ptr [bp-2], 10h
                mov     byte ptr [bp-4], 1
                cmp     byte_22E14, 0   ; CODE XREF: seg002:07AD↑J
                jnz     short loc_1CED4
                mov     byte ptr [bp-2], 8
                dec     byte ptr [bp-4]

loc_1CED4:                              ; CODE XREF: ovl_2CAVES:CECB↑j
                mov     word ptr [bp-8], 0
                cmp     g_party_size, 0
                jle     short loc_1CF25
                mov     al, [bp-2]
                sub     ah, ah
                mov     [bp-0Eh], ax
                mov     si, [bp-8]

loc_1CEEB:                              ; CODE XREF: ovl_2CAVES:CF1D↓j
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                mov     al, [di+7Ch]
                mov     [bp-0Ch], al
                sub     ah, ah
                test    [bp-0Eh], ax
                jnz     short loc_1CF18
                inc     byte ptr [bp-0Ah]
                and     byte ptr [bp-0Ch], 0FEh
                or      byte ptr [bp-0Ch], 4
                mov     al, [bp-4]
                or      [bp-0Ch], al
                mov     al, [bp-0Ch]
                mov     [di+7Ch], al

loc_1CF18:                              ; CODE XREF: ovl_2CAVES:CEFF↑j
                inc     si
                cmp     si, g_party_size
                jl      short loc_1CEEB
                mov     [bp-6], di
                mov     [bp-8], si

loc_1CF25:                              ; CODE XREF: ovl_2CAVES:CEDE↑j
                cmp     byte ptr [bp-0Ah], 0
                jz      short loc_1CF67
                mov     ax, 2           ; CODE XREF: seg002:050D↑J
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CF39:                              ; CODE XREF: ovl_2CAVES:CF62↓j
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
                mov     [bp-8], si

loc_1CF67:                              ; CODE XREF: ovl_2CAVES:CF29↑j
                cmp     byte ptr [bp-0Ah], 0
                jz      short loc_1CF72
                mov     ax, 1
                jmp     short loc_1CF75
; ---------------------------------------------------------------------------

loc_1CF72:                              ; CODE XREF: ovl_2CAVES:CF6B↑j
                mov     ax, 0FFFFh

loc_1CF75:                              ; CODE XREF: ovl_2CAVES:CF70↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CF7C       proc near               ; CODE XREF: ovl_2CAVES:loc_1D214↓p
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


sub_1D00C       proc near               ; CODE XREF: ovl_2CAVES:D20F↓p
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
                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                sub     di, di
                mov     [bp-12h], di
                mov     [bp-8], di
                mov     si, [bp-4]
                jmp     short loc_1D10A
; ---------------------------------------------------------------------------
                align 2

loc_1D0AA:                              ; CODE XREF: ovl_2CAVES:D138↓j
                test    byte ptr [bp-2], 8
                jnz     short loc_1D0C0
                or      di, di
                jnz     short loc_1D0B9
                call    sub_1CD4C
                mov     di, ax

loc_1D0B9:                              ; CODE XREF: ovl_2CAVES:D0B2↑j
                or      di, di
                jz      short loc_1D0C0

loc_1D0BD:                              ; CODE XREF: ovl_2CAVES:loc_1D154↓j
                inc     word ptr [bp-12h]

loc_1D0C0:                              ; CODE XREF: ovl_2CAVES:D0AE↑j
                                        ; ovl_2CAVES:D0BB↑j ...
                cmp     word ptr [bp-12h], 0
                jz      short loc_1D107
                inc     di
                and     byte ptr [si+7Ch], 0FBh
                mov     byte ptr [bp-2], 10h
                cmp     byte_22E14, 0
                jnz     short loc_1D0DA
                mov     byte ptr [bp-2], 8

loc_1D0DA:                              ; CODE XREF: ovl_2CAVES:D0D4↑j
                mov     al, [bp-2]
                or      [si+7Ch], al
                mov     word ptr [bp-10h], 86A0h
                mov     word ptr [bp-0Eh], 1
                cmp     byte_22E14, 1
                jnz     short loc_1D0FB
                mov     word ptr [bp-10h], 4240h
                mov     word ptr [bp-0Eh], 0Fh

loc_1D0FB:                              ; CODE XREF: ovl_2CAVES:D0EF↑j
                mov     ax, [bp-10h]
                mov     dx, [bp-0Eh]
                add     [si+62h], ax
                adc     [si+64h], dx

loc_1D107:                              ; CODE XREF: ovl_2CAVES:D0C4↑j
                                        ; ovl_2CAVES:D127↓j ...
                inc     word ptr [bp-8]

loc_1D10A:                              ; CODE XREF: ovl_2CAVES:D0A7↑j
                mov     ax, g_party_size
                cmp     [bp-8], ax
                jge     short loc_1D158
                push    word ptr [bp-8]
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [si+7Ch]
                mov     [bp-2], al
                test    byte ptr [bp-2], 4
                jz      short loc_1D107
                and     al, 1
                cmp     al, byte_22E14
                jnz     short loc_1D107
                cmp     byte_22E14, 1
                jz      short loc_1D13B
                jmp     loc_1D0AA
; ---------------------------------------------------------------------------

loc_1D13B:                              ; CODE XREF: ovl_2CAVES:D136↑j
                mov     al, [bp-2]
                and     al, 0E0h
                cmp     al, 0E0h
                jz      short loc_1D147
                jmp     loc_1D0C0
; ---------------------------------------------------------------------------

loc_1D147:                              ; CODE XREF: ovl_2CAVES:D142↑j
                and     byte ptr [si+7Ch], 1Fh
                test    byte ptr [bp-2], 10h
                jz      short loc_1D154
                jmp     loc_1D0C0
; ---------------------------------------------------------------------------

loc_1D154:                              ; CODE XREF: ovl_2CAVES:D14F↑j
                jmp     loc_1D0BD
; ---------------------------------------------------------------------------
                align 2

loc_1D158:                              ; CODE XREF: ovl_2CAVES:D110↑j
                                        ; seg002:0825↑J
                mov     [bp-0Ah], di
                mov     [bp-4], si
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
                push    word ptr [bp-0Eh]
                push    word ptr [bp-10h]
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

loc_1D1BE:                              ; CODE XREF: ovl_2CAVES:D160↑j
                mov     word ptr [bp-8], 0
                jmp     short loc_1D21D
; ---------------------------------------------------------------------------
                align 2

loc_1D1C6:                              ; CODE XREF: ovl_2CAVES:D23D↓j
                push    word ptr [bp-4]
                call    sub_1CB4A

loc_1D1CC:                              ; CODE XREF: ovl_2CAVES:D245↓j
                add     sp, 2
                mov     [bp-2], al
                or      al, al
                jz      short loc_1D21A
                sub     si, si
                mov     di, [bp-6]
                jmp     short loc_1D1E6
; ---------------------------------------------------------------------------
                align 2

loc_1D1DE:                              ; CODE XREF: ovl_2CAVES:D1FA↓j
                push    di
                call    sub_1CBCA

loc_1D1E2:                              ; CODE XREF: ovl_2CAVES:D200↓j
                add     sp, 2
                inc     si

loc_1D1E6:                              ; CODE XREF: ovl_2CAVES:D1DB↑j
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

loc_1D202:                              ; CODE XREF: ovl_2CAVES:D1EA↑j
                mov     [bp-6], di
                mov     [bp-0Ch], si
                cmp     byte_22E14, 0
                jnz     short loc_1D214
                call    sub_1D00C
                jmp     short loc_1D217
; ---------------------------------------------------------------------------

loc_1D214:                              ; CODE XREF: ovl_2CAVES:D20D↑j
                call    sub_1CF7C

loc_1D217:                              ; CODE XREF: ovl_2CAVES:D212↑j
                inc     word ptr [bp-0Ah]

loc_1D21A:                              ; CODE XREF: ovl_2CAVES:D1D4↑j
                inc     word ptr [bp-8]

loc_1D21D:                              ; CODE XREF: ovl_2CAVES:D1C3↑j
                mov     ax, g_party_size
                cmp     [bp-8], ax
                jge     short loc_1D248
                push    word ptr [bp-8]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-4], ax
                sub     al, al
                mov     byte_22E13, al
                mov     byte_22E12, al
                cmp     byte_22E14, al
                jnz     short loc_1D1C6
                push    word ptr [bp-4]
                call    sub_1CBCA
                jmp     short loc_1D1CC
; ---------------------------------------------------------------------------
                align 2

loc_1D248:                              ; CODE XREF: ovl_2CAVES:D223↑j
                mov     ax, [bp-0Ah]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     word ptr [bp-6], 0
                sub     di, di
                mov     si, [bp-8]
                jmp     loc_1D2F1
; ---------------------------------------------------------------------------
                align 2

loc_1D268:                              ; CODE XREF: ovl_2CAVES:D345↓j
                mov     ax, offset aThreeSwords ; "three swords."

loc_1D26B:                              ; CODE XREF: ovl_2CAVES:D34B↓j
                push    ax
                jmp     short loc_1D2D9
; ---------------------------------------------------------------------------

loc_1D26E:                              ; CODE XREF: ovl_2CAVES:D30C↓j
                cmp     byte ptr [si+78h], 0
                jz      short loc_1D2E2
                test    byte ptr [bp-2], 1
                jz      short loc_1D294
                mov     al, [si+78h]
                mov     byte_22E12, al
                mov     byte_26ED0, al
                sub     ax, ax
                push    ax
                call    thk_monster_decode_stats
                add     sp, 2
                mov     word ptr [bp-4], 9E0Eh
                jmp     short loc_1D2A6
; ---------------------------------------------------------------------------
                align 2

loc_1D294:                              ; CODE XREF: ovl_2CAVES:D278↑j
                mov     al, [si+78h]
                mov     byte_22E13, al
                mov     al, 14h
                mul     byte_22E13
                add     ax, 6960h
                mov     [bp-4], ax

loc_1D2A6:                              ; CODE XREF: ovl_2CAVES:D291↑j
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
                push    word ptr [bp-4]

loc_1D2D9:                              ; CODE XREF: ovl_2CAVES:D26C↑j
                call    thk_text_puts
                add     sp, 2
                inc     word ptr [bp-6]

loc_1D2E2:                              ; CODE XREF: ovl_2CAVES:D272↑j
                cmp     word ptr [bp-6], 0
                jz      short loc_1D2F0

loc_1D2E8:                              ; CODE XREF: ovl_2CAVES:D2F5↓j
                mov     [bp-0Ah], di
                mov     [bp-8], si
                jmp     short loc_1D34E
; ---------------------------------------------------------------------------

loc_1D2F0:                              ; CODE XREF: ovl_2CAVES:D2E6↑j
                inc     di

loc_1D2F1:                              ; CODE XREF: ovl_2CAVES:D264↑j
                cmp     di, g_party_size
                jge     short loc_1D2E8
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, [si+7Ch]
                mov     [bp-2], al
                test    byte ptr [bp-2], 4
                jnz     short loc_1D30F
                jmp     loc_1D26E
; ---------------------------------------------------------------------------

loc_1D30F:                              ; CODE XREF: ovl_2CAVES:D30A↑j
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
                test    byte ptr [bp-2], 1
                jnz     short loc_1D348
                jmp     loc_1D268
; ---------------------------------------------------------------------------

loc_1D348:                              ; CODE XREF: ovl_2CAVES:D343↑j
                mov     ax, offset aThreeBeasts ; "three beasts."
                jmp     loc_1D26B
; ---------------------------------------------------------------------------

loc_1D34E:                              ; CODE XREF: ovl_2CAVES:D2EE↑j
                cmp     word ptr [bp-6], 0
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

loc_1D384:                              ; CODE XREF: ovl_2CAVES:D352↑j
                mov     ax, [bp-6]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                jmp     short loc_1D39D
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D398:                              ; CODE XREF: ovl_2CAVES:D3AC↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_1D3AE

loc_1D39D:                              ; CODE XREF: ovl_2CAVES:D395↑j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1D398

loc_1D3AE:                              ; CODE XREF: ovl_2CAVES:D39B↑j
                mov     [bp-2], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_1D3BC
                mov     ax, 1
                jmp     short loc_1D3BE
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1D3BC:                              ; CODE XREF: ovl_2CAVES:D3B4↑j
                sub     ax, ax

loc_1D3BE:                              ; CODE XREF: ovl_2CAVES:D3B9↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db  90h
                db  55h ; U             ; CODE XREF: caves_event_e+3↓p
                                        ; caves_event_f+4↓p
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db  0Ah
                db  57h ; W
                db  56h ; V
                db 0C7h
                db  46h ; F
                db 0FEh
                db    0
                db    0
                db  8Dh
                db  46h ; F
                db 0FAh
                db  50h ; P
                db 0B8h
                db  5Bh ; [
                db  3Fh ; ?
                db  50h ; P
                db 0E8h
                db  56h ; V
                db  9Ah
                db  83h
                db 0C4h
                db    4
                db 0A3h
                db    4
                db    5
                db  89h
                db  16h
                db    6
                db    5
                db  0Bh
                db 0D0h
                db  74h ; t
                db 0E7h
                db 0C6h
                db    6
                db 0FFh
                db  50h ; P
                db 0FDh
                db 0C6h
                db    6
                db  30h ; 0
                db    4
                db    7
                db 0E8h
                db  9Bh
                db 0A0h
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db 0F8h
                db  99h
                db  83h
                db 0C4h
                db    2
                db  8Ah
                db  46h ; F
                db    4
                db 0A2h
                db 0C4h
                db  55h ; U
                db 0E8h
                db  8Ah
                db 0FCh
                db  0Bh
                db 0C0h
                db  74h ; t
                db    3
                db 0E9h
                db  4Eh ; N
                db    1
                db 0E8h
                db  3Eh ; >
                db 0FEh
                db  0Bh
                db 0C0h
                db  74h ; t
                db    3
                db 0E9h
                db  44h ; D
                db    1
                db  2Bh ; +
                db 0F6h
                db  8Bh
                db  46h ; F
                db    4
                db 0B1h
                db    3
                db 0D3h
                db 0E0h
                db  89h
                db  46h ; F
                db 0F8h
                db  8Bh
                db 0F8h
                db  81h
                db 0C7h
                db 0D2h
                db  3Dh ; =
                db  8Dh
                db  44h ; D
                db  11h
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db 0F6h
                db  9Ah
                db  83h
                db 0C4h
                db    4
                db 0FFh
                db  35h ; 5
                db 0E8h
                db  12h
                db  9Bh
                db  83h
                db 0C4h
                db    2
                db  83h
                db 0C7h
                db    2
                db  46h ; F
                db  83h
                db 0FEh
                db    4
                db  7Ch ; |
                db 0E1h
                db  89h
                db  76h ; v
                db 0FAh
                db 0E8h
                db  3Ch ; <
                db 0FFh
                db  89h
                db  46h ; F
                db 0FAh
                db  0Bh
                db 0C0h
                db  75h ; u
                db    7
                db 0FFh
                db  46h ; F
                db 0FEh
                db 0E9h
                db    0
                db    1
                db  90h
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db  8Fh
                db  99h
                db  83h
                db 0C4h
                db    2
                db 0E8h
                db 0D9h
                db  9Ah
                db  2Bh ; +
                db 0F6h
                db 0BFh
                db 0FAh
                db  3Dh ; =
                db  8Dh
                db  44h ; D
                db  12h
                db  50h ; P
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db 0B1h
                db  9Ah
                db  83h
                db 0C4h
                db    4
                db 0FFh
                db  35h ; 5
                db 0E8h
                db 0CDh
                db  9Ah
                db  83h
                db 0C4h
                db    2
                db  83h
                db 0C7h
                db    2
                db  46h ; F
                db  83h
                db 0FEh
                db    3
                db  7Ch ; |
                db 0E1h
                db  89h
                db  76h ; v
                db 0FAh
                db 0B8h
                db  15h
                db    0
                db  50h ; P
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db  8Fh
                db  9Ah
                db  83h
                db 0C4h
                db    4
                db  80h
                db  3Eh ; >
                db 0C4h
                db  55h ; U
                db    0
                db  75h ; u
                db    5
                db 0B8h
                db  68h ; h
                db  3Fh ; ?
                db 0EBh
                db    3
                db 0B8h
                db  78h ; x
                db  3Fh ; ?
                db  50h ; P
                db 0E8h
                db  9Dh
                db  9Ah
                db  83h
                db 0C4h
                db    2
                db 0B8h
                db  15h
                db    0
                db  50h ; P
                db 0B8h
                db  26h ; &
                db    0
                db  50h ; P
                db 0B8h
                db  12h
                db    0
                db  50h ; P
                db 0B8h
                db  15h
                db    0
                db  50h ; P
                db 0E8h
                db  33h ; 3
                db  9Ah
                db  83h
                db 0C4h
                db    8
                db  2Bh ; +
                db 0F6h
                db 0BFh
                db    0
                db  3Eh ; >
                db  8Dh
                db  44h ; D
                db  12h
                db  50h ; P
                db 0B8h
                db  15h
                db    0
                db  50h ; P
                db 0E8h
                db  50h ; P
                db  9Ah
                db  83h
                db 0C4h
                db    4
                db 0FFh
                db  35h ; 5
                db 0E8h
                db  6Ch ; l
                db  9Ah
                db  83h
                db 0C4h
                db    2
                db  83h
                db 0C7h
                db    2
                db  46h ; F
                db  83h
                db 0FEh
                db    4
                db  7Ch ; |
                db 0E1h
                db  89h
                db  76h ; v
                db 0FAh
                db 0E8h
                db  4Ah ; J
                db  9Bh
                db  50h ; P
                db 0E8h
                db  26h ; &
                db  9Ah
                db  83h
                db 0C4h
                db    2
                db  8Bh
                db 0F8h
                db  83h
                db 0FFh
                db  1Bh
                db  75h ; u
                db    6
                db 0B8h
                db    1
                db    0
                db 0EBh
                db    3
                db  90h
                db  2Bh ; +
                db 0C0h
                db  8Bh
                db 0F0h
                db  0Bh
                db 0F6h
                db  75h ; u
                db  16h
                db  8Bh
                db 0C7h
                db  3Dh ; =
                db  41h ; A
                db    0
                db  72h ; r
                db  0Bh
                db  3Dh ; =
                db  44h ; D
                db    0
                db  77h ; w
                db    6
                db 0B8h
                db    1
                db    0
                db 0EBh
                db    3
                db  90h
                db  2Bh ; +
                db 0C0h
                db  8Bh
                db 0F0h
                db  0Bh
                db 0F6h
                db  74h ; t
                db 0C7h
                db  89h
                db  76h ; v
                db 0FAh
                db  83h
                db 0FFh
                db  1Bh
                db  74h ; t
                db    3
                db  83h
                db 0EFh
                db  41h ; A
                db  83h
                db 0FFh
                db    3
                db  7Dh ; }
                db    7
                db  57h ; W
                db 0E8h
                db  6Eh ; n
                db 0F8h
                db  83h
                db 0C4h
                db    2
                db  83h
                db 0FFh
                db    3
                db  75h ; u
                db    5
                db 0E8h
                db  65h ; e
                db 0F9h
                db  8Bh
                db 0F8h
                db  83h
                db 0FFh
                db  1Bh
                db  75h ; u
                db    3
                db 0FFh
                db  46h ; F
                db 0FEh
                db  83h
                db 0FFh
                db 0FFh
                db  74h ; t
                db  99h
                db  89h
                db  7Eh ; ~
                db 0FCh
                db  83h
                db  7Eh ; ~
                db 0FEh
                db    0
                db  74h ; t
                db  51h ; Q
                db 0B8h
                db    2
                db    0
                db  50h ; P
                db 0E8h
                db  8Ah
                db  98h
                db  83h
                db 0C4h
                db    2
                db 0B8h
                db  13h
                db    0
                db  50h ; P
                db 0B8h
                db  0Ah
                db    0
                db  50h ; P
                db 0E8h
                db 0B4h
                db  99h
                db  83h
                db 0C4h
                db    4
                db 0B8h
                db  86h
                db  3Fh ; ?
                db  50h ; P
                db 0E8h
                db 0CEh
                db  99h
                db  83h
                db 0C4h
                db    2
                db 0B8h
                db  64h ; d
                db    0
                db  50h ; P
                db 0E8h
                db  74h ; t
                db  9Bh
                db  83h
                db 0C4h
                db    2
                db 0C6h
                db    6
                db  93h
                db    3
                db    9
                db 0C6h
                db    6
                db  94h
                db    3
                db  0Ah
                db  80h
                db  3Eh ; >
                db 0C4h
                db  55h ; U
                db    1
                db  75h ; u
                db  0Ah
                db 0C6h
                db    6
                db  93h
                db    3
                db    3
                db 0C6h
                db    6
                db  94h
                db    3
                db    5
                db 0E8h
                db 0F7h
                db  9Ch
                db 0C6h
                db    6
                db  2Eh ; .
                db    4
                db    1
                db 0EBh
                db  0Eh
                db 0E8h
                db 0A5h
                db  99h
                db 0E8h
                db    6
                db  9Fh
                db 0E8h
                db  83h
                db  9Ah
                db  3Dh ; =
                db  20h
                db    0
                db  75h ; u
                db 0F8h
                db 0FFh
                db  36h ; 6
                db    6
                db    5
                db 0FFh
                db  36h ; 6
                db    4
                db    5
                db 0E8h
                db 0FFh
                db  98h
                db  83h
                db 0C4h
                db    4
                db 0E8h
                db  89h
                db  99h
                db 0E8h
                db 0FEh
                db  9Ch
                db  5Eh ; ^
                db  5Fh ; _
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h

; =============== S U B R O U T I N E =======================================


caves_event_e   proc near               ; CODE XREF: seg002:0795↑J
                sub     ax, ax
                push    ax
                call    near ptr unk_1D3C4
                add     sp, 2
                retn
caves_event_e   endp


; =============== S U B R O U T I N E =======================================


caves_event_f   proc near               ; CODE XREF: seg002:07A1↑J
                mov     ax, 1
                push    ax
                call    near ptr unk_1D3C4
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

