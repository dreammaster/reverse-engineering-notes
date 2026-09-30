; ===========================================================================

; Segment type: Pure code
ovl_1RETINN     segment byte public 'CODE' use16
                assume cs:ovl_1RETINN
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

inn_common_helper proc near             ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 0Ah
                push    di
                push    si
                mov     byte_1DC80, 6
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     [bp+var_6], si

loc_1C14B:                              ; CODE XREF: inn_common_helper+4D↓j
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 3
                shl     bx, cl
                add     bx, [bp+var_6]
                mov     di, [bx+20BAh]
                or      di, di
                jz      short loc_1C175
                lea     ax, [si+13h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    di
                call    thk_text_puts
                add     sp, 2

loc_1C175:                              ; CODE XREF: inn_common_helper+2E↑j
                add     [bp+var_6], 2
                inc     si
                cmp     si, 4
                jl      short loc_1C14B
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jz      short loc_1C1E0
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     [bp+var_4], 0
                cmp     g_party_size, 0
                jle     short loc_1C1D0
                mov     al, g_map_id
                inc     al
                mov     byte ptr [bp+var_6], al
                mov     si, 416h
                mov     cx, g_party_size
                mov     ax, cx
                add     [bp+var_4], ax

loc_1C1BD:                              ; CODE XREF: inn_common_helper+9E↓j
                mov     ax, 82h
                imul    word ptr [si]
                mov     bx, ax
                mov     al, byte ptr [bp+var_6]
                mov     [bx+7E2Bh], al
                add     si, 2
                loop    loc_1C1BD

loc_1C1D0:                              ; CODE XREF: inn_common_helper+77↑j
                mov     al, g_map_id
                mov     byte_1DC24, al
                call    thk_save_roster
                call    inn_leave       ; CODE XREF: seg002:08CD↑J
                jmp     short loc_1C1E3
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1C1E0:                              ; CODE XREF: inn_common_helper+5D↑j
                call    thk_2PLAY_A580

loc_1C1E3:                              ; CODE XREF: inn_common_helper+AC↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
inn_common_helper endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

inn_leave       proc near               ; CODE XREF: seg002:07F5↑J
                                        ; inn_common_helper+A9↑p ...

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                call    thk_2PLAY_B0F6
                mov     byte_1DBE8, 0
                mov     al, g_view_mode
                mov     [bp+var_2], al
                call    inn_menu
                mov     al, g_map_id
                mov     [bp+var_4], al
                mov     g_map_id, 0FFh
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
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
                call    thk_gfx_copy_page
                add     sp, 4
                sub     ax, ax          ; CODE XREF: seg002:08E5↑J
                push    ax
                call    thk_gfx_select_page
                add     sp, 2           ; CODE XREF: seg002:0639↑J
                mov     al, [bp+var_2]
                mov     g_view_mode, al
                mov     byte_1DBE9, 0FFh
                mov     g_map_id, 0FFh
                mov     byte_1DBEC, 7
                mov     al, g_party_y
                sub     ah, ah
                push    ax
                mov     al, g_party_x
                push    ax
                mov     al, byte_1DC24
                push    ax
                call    thk_2PLAY_B5EA
                add     sp, 6
                mov     g_party_size, 0
                mov     si, 416h
                mov     cx, g_party_size

loc_1C27A:                              ; CODE XREF: inn_leave+A5↓j
                cmp     word ptr [si], 0FFFFh
                jnz     short loc_1C286

loc_1C27F:                              ; CODE XREF: inn_leave+A3↓j
                mov     g_party_size, cx
                jmp     short loc_1C292
; ---------------------------------------------------------------------------
                align 2

loc_1C286:                              ; CODE XREF: inn_leave+93↑j
                add     si, 2
                inc     cx
                cmp     cx, 8
                jge     short loc_1C27F
                jmp     short loc_1C27A
; ---------------------------------------------------------------------------
                align 2

loc_1C292:                              ; CODE XREF: inn_leave+99↑j
                mov     byte_1DC7E, 1
                mov     byte_1DBEB, 0
                mov     byte_1DC80, 0
                pop     si
                mov     sp, bp
                pop     bp
                retn
inn_leave       endp


; =============== S U B R O U T I N E =======================================

; "('ESC' to exit to DOS)"

inn_esc_prompt  proc near               ; CODE XREF: inn_menu:loc_1C828↓p
                                        ; inn_menu+521↓p
                mov     ax, 17h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aEscToExitToDos ; "('ESC' to exit to DOS)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                retn
inn_esc_prompt  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Characters" / "Hirelings"
; Attributes: bp-based frame

inn_draw_lists  proc near               ; CODE XREF: seg002:026D↑J
                                        ; inn_menu+255↓p

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
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+arg_0], 0
                jnz     short loc_1C2F4
                mov     ax, offset aCharacters_0 ; "Characters\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2107h
                jmp     short loc_1C301
; ---------------------------------------------------------------------------

loc_1C2F4:                              ; CODE XREF: inn_draw_lists+23↑j
                mov     ax, offset aHirelings_0 ; "Hirelings \n"
                push    ax

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                call    thk_text_puts
                add     sp, 2
                mov     ax, 211Eh

loc_1C301:                              ; CODE XREF: inn_draw_lists+32↑j
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, [bp+arg_0]

loc_1C316:                              ; CODE XREF: inn_draw_lists+1E4↓j
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

loc_1C344:                              ; CODE XREF: inn_draw_lists+7C↑j
                sub     ax, ax

loc_1C346:                              ; CODE XREF: inn_draw_lists+81↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_8], 20h ; ' '
                cmp     [bp+arg_2], 0
                jz      short loc_1C378
                mov     ax, si
                add     ax, di
                push    ax
                call    thk_party_contains
                add     sp, 2
                or      ax, ax
                jz      short loc_1C378
                mov     ax, [bp+arg_2]
                cmp     [bp+var_2], ax  ; CODE XREF: seg002:0471↑J
                jnz     short loc_1C378
                mov     [bp+var_8], 17h

loc_1C378:                              ; CODE XREF: inn_draw_lists+9B↑j
                                        ; inn_draw_lists+AA↑j ...
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, si
                add     ax, 41h ; 'A'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2Dh ; '-'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
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

loc_1C3B8:                              ; CODE XREF: inn_draw_lists+F0↑j
                sub     ax, ax
                jmp     short loc_1C3C3
; ---------------------------------------------------------------------------

loc_1C3BC:                              ; CODE XREF: inn_draw_lists+E8↑j
                cmp     [bp+var_2], 1
                sbb     ax, ax
                inc     ax

loc_1C3C3:                              ; CODE XREF: inn_draw_lists+F5↑j
                                        ; inn_draw_lists+FA↑j
                mov     [bp+var_6], ax
                cmp     di, 18h
                jnz     short loc_1C3DB
                or      ax, ax
                jz      short loc_1C3DB
                cmp     byte ptr [si+3F6h], 0
                jnz     short loc_1C3DB
                mov     [bp+var_6], 0

loc_1C3DB:                              ; CODE XREF: inn_draw_lists+109↑j
                                        ; inn_draw_lists+10D↑j ...
                cmp     [bp+var_6], 0
                jnz     short loc_1C3E4
                jmp     loc_1C494
; ---------------------------------------------------------------------------

loc_1C3E4:                              ; CODE XREF: inn_draw_lists+11F↑j
                mov     ax, si
                add     ax, di
                mov     cx, 82h
                imul    cx
                mov     [bp+var_A], ax
                add     ax, 7E20h
                push    ax
                call    thk_text_puts   ; CODE XREF: seg002:047D↑J
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bx, [bp+var_A]
                mov     bl, [bx+7E2Fh]
                sub     bh, bh
                shl     bx, 1
                mov     bx, [bx+446h]
                mov     al, [bx]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     [bp+arg_2], 0
                jnz     short loc_1C454
                mov     ax, 2Fh ; '/'
                push    ax
                call    thk_text_putc
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
                call    thk_text_put_number_pad
                add     sp, 6
                jmp     short loc_1C49E
; ---------------------------------------------------------------------------
                align 2

loc_1C454:                              ; CODE XREF: inn_draw_lists+162↑j
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
                call    thk_text_putc
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
                call    thk_text_putc
                jmp     short loc_1C49B
; ---------------------------------------------------------------------------
                align 2

loc_1C494:                              ; CODE XREF: inn_draw_lists+121↑j
                mov     ax, offset asc_1F979 ; "              "
                push    ax
                call    thk_text_puts

loc_1C49B:                              ; CODE XREF: inn_draw_lists+1D1↑j
                add     sp, 2

loc_1C49E:                              ; CODE XREF: inn_draw_lists+191↑j
                inc     si
                cmp     si, 18h
                jge     short loc_1C4A7
                jmp     loc_1C316
; ---------------------------------------------------------------------------

loc_1C4A7:                              ; CODE XREF: inn_draw_lists+1E2↑j
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C4B0:                              ; CODE XREF: inn_menu+473↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     [bp+var_4], 0
                cmp     [bp+arg_0], 18h
                jl      short loc_1C4CB
                mov     [bp+var_4], 18h
                sub     [bp+arg_0], 18h

loc_1C4CB:                              ; CODE XREF: inn_draw_lists+200↑j
                mov     si, [bp+arg_0]
                add     si, [bp+var_4]
                push    si
                call    thk_party_contains
                add     sp, 2
                or      ax, ax
                jz      short loc_1C512
                push    si
                call    thk_party_remove
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

loc_1C4FC:                              ; CODE XREF: inn_draw_lists+234↑j
                sub     ax, ax

loc_1C4FE:                              ; CODE XREF: inn_draw_lists+239↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                jmp     loc_1C5B4
; ---------------------------------------------------------------------------
                align 2

loc_1C512:                              ; CODE XREF: inn_draw_lists+21A↑j
                cmp     [bp+var_4], 0
                jnz     short loc_1C53B
                cmp     byte_22E0E, 6
                jnb     short loc_1C536
                mov     al, byte_22E0E
                sub     ah, ah
                mov     cl, byte_22E0F
                sub     ch, ch
                add     ax, cx
inn_draw_lists  endp


loc_1C52C:                              ; CODE XREF: seg002:0879↑J
                cmp     ax, 8
                jz      short loc_1C536
                mov     ax, 1
                jmp     short loc_1C538
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR inn_draw_lists

loc_1C536:                              ; CODE XREF: inn_draw_lists+25D↑j
                                        ; ovl_1RETINN:C52F↑j
                sub     ax, ax

loc_1C538:                              ; CODE XREF: ovl_1RETINN:C534↑j
                mov     [bp+var_2], ax

loc_1C53B:                              ; CODE XREF: inn_draw_lists+256↑j
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

loc_1C558:                              ; CODE XREF: inn_draw_lists+291↑j
                sub     ax, ax

loc_1C55A:                              ; CODE XREF: inn_draw_lists+296↑j
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1C56F
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+3F6h], 1
                sbb     ax, ax
                inc     ax
                mov     [bp+var_2], ax

loc_1C56F:                              ; CODE XREF: inn_draw_lists+27F↑j
                                        ; inn_draw_lists+29F↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C5BB
                mov     bx, g_party_size
                inc     g_party_size
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
; END OF FUNCTION CHUNK FOR inn_draw_lists
; ---------------------------------------------------------------------------
                align 2
; START OF FUNCTION CHUNK FOR inn_draw_lists

loc_1C5A2:                              ; CODE XREF: inn_draw_lists+2DA↑j
                sub     ax, ax

loc_1C5A4:                              ; CODE XREF: inn_draw_lists+2DF↑j
                mov     cx, 14h
                imul    cx
                inc     ax
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:0885↑J
                add     sp, 4
                mov     ax, 17h

loc_1C5B4:                              ; CODE XREF: inn_draw_lists+24E↑j
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_1C5BB:                              ; CODE XREF: inn_draw_lists+2B3↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR inn_draw_lists

; =============== S U B R O U T I N E =======================================

; A-X view, Ctrl A-X add/remove, 1-5 other towns; "PARTY"
; Attributes: bp-based frame

inn_menu        proc near               ; CODE XREF: seg002:029D↑J
                                        ; inn_leave+15↑p

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
                call    thk_gfx_select_page
                add     sp, 2
                mov     al, byte_1DB8E
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
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAXToView_0 ; "'A' - 'X' to View"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCtrlAXToAddRem ; "(Ctrl) 'A' - 'X' to Add/Remove"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOtherTowns ; "Other Towns"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset a15  ; " '1' - '5'"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4           ; CODE XREF: seg002:0891↑J
                mov     ax, offset aParty ; "PARTY"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 2
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCH  ; "C=  / H="
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C692:                              ; CODE XREF: inn_menu+552↓j
                sub     al, al
                mov     byte_22E0F, al
                mov     byte_22E0E, al
                mov     [bp+var_4], 0
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------
                align 2

loc_1C6A2:                              ; CODE XREF: inn_menu+101↓j
                                        ; inn_menu+106↓j
                inc     byte_22E0F

loc_1C6A6:                              ; CODE XREF: inn_menu+FD↓j
                                        ; inn_menu+10C↓j
                inc     [bp+var_4]

loc_1C6A9:                              ; CODE XREF: inn_menu+DF↑j
                mov     ax, g_party_size
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

loc_1C6CE:                              ; CODE XREF: inn_menu+EF↑j
                mov     ax, 2
                push    ax
                mov     ax, 1Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_22E0E
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2
                push    ax
                mov     ax, 25h ; '%'

loc_1C6F2:                              ; CODE XREF: seg002:0B19↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_22E0F
                sub     ah, ah
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     byte_22E0E, 0
                jz      short loc_1C72A
                mov     ax, 15h
                push    ax
                mov     ax, 1Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aZToExit ; "'Z' to exit"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1C740
; ---------------------------------------------------------------------------
                align 2

loc_1C72A:                              ; CODE XREF: inn_menu+14D↑j
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 15h
                push    ax
                mov     ax, 1Bh
                push    ax

loc_1C73A:                              ; CODE XREF: seg002:089D↑J
                call    thk_clear_text_rect
                add     sp, 8

loc_1C740:                              ; CODE XREF: inn_menu+167↑j
                mov     ax, 4
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
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
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, offset aPartyIsFull ; "*** Party is Full ***"
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                jmp     short loc_1C796
; ---------------------------------------------------------------------------
                align 2

loc_1C780:                              ; CODE XREF: inn_menu+19E↑j
                mov     ax, 4
                push    ax
                mov     ax, 1Eh
                push    ax
                mov     ax, 4
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_clear_text_rect
                add     sp, 8

loc_1C796:                              ; CODE XREF: inn_menu+1BD↑j
                cmp     [bp+var_E], 0
                jnz     short loc_1C79F
                jmp     loc_1C861
; ---------------------------------------------------------------------------

loc_1C79F:                              ; CODE XREF: inn_menu+1DA↑j
                sub     ax, ax
                push    ax
                mov     bl, g_map_id
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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 28h ; '('
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, g_map_id
                sub     ah, ah
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                add     sp, 2
                mov     ax, 2Dh ; '-'

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+43Ch]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, g_map_id
                sub     ah, ah
                inc     ax
                push    ax
                push    [bp+var_A]
                call    inn_draw_lists
                add     sp, 4
                cmp     g_outdoors, 2
                jnz     short loc_1C828
                call    thk_res_5440
                jmp     short loc_1C82B
; ---------------------------------------------------------------------------
                align 2

loc_1C828:                              ; CODE XREF: inn_menu+260↑j
                call    inn_esc_prompt

loc_1C82B:                              ; CODE XREF: inn_menu+265↑j
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpaceFor_0 ; "'Space' for "
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     [bp+var_A], 0
                jle     short loc_1C84E
                mov     bx, 1
                jmp     short loc_1C850
; ---------------------------------------------------------------------------

loc_1C84E:                              ; CODE XREF: inn_menu+287↑j
                sub     bx, bx

loc_1C850:                              ; CODE XREF: inn_menu+28C↑j
                shl     bx, 1
                push    word ptr [bx+5F4h]
                call    thk_text_puts
                add     sp, 2
                mov     [bp+var_E], 0

loc_1C861:                              ; CODE XREF: inn_menu+1DC↑j
                mov     ax, 7Ah ; 'z'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_get_key_in_range_nowait
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

loc_1C883:                              ; CODE XREF: inn_menu+2BE↑j
                cmp     ax, 58h ; 'X'
                jbe     short loc_1C88B
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C88B:                              ; CODE XREF: inn_menu+2C6↑j
                add     ax, [bp+var_A]
                mov     cx, 82h
                mul     cx
                mov     bx, ax
                mov     al, [bx+5D29h]
                sub     ah, ah
                mov     cl, g_map_id
                sub     ch, ch
                inc     cx
                cmp     ax, cx
                jz      short loc_1C8A9
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C8A9:                              ; CODE XREF: inn_menu+2E4↑j
                cmp     [bp+var_A], 18h
                jnz     short loc_1C8B8
                mov     bx, [bp+var_8]
                cmp     [bx+3B5h], ah
                jnz     short loc_1C8C1

loc_1C8B8:                              ; CODE XREF: inn_menu+2ED↑j
                cmp     [bp+var_A], 0
                jz      short loc_1C8C1
                jmp     loc_1C962
; ---------------------------------------------------------------------------

loc_1C8C1:                              ; CODE XREF: inn_menu+2F6↑j
                                        ; inn_menu+2FC↑j
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     al, byte ptr [bp+var_8]
                sub     ah, ah
                mov     [bp+var_12], ax
                mov     ax, [bp+var_A]
                add     ax, [bp+var_12]
                sub     ax, 41h ; 'A'
                push    ax
                push    [bp+var_12]
                call    thk_show_character_sheet
                add     sp, 4
                mov     ax, [bp+var_8]
                add     ax, [bp+var_A]
                mov     cx, 82h
                mul     cx
                mov     di, ax
                add     di, 5D1Eh

loc_1C8FA:                              ; CODE XREF: inn_menu+390↓j
                mov     ax, 15h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aVViewSpellBook ; "'V' View spell book"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 76h ; 'v'
                push    ax
                mov     ax, 56h ; 'V'
                push    ax
                call    thk_get_key_in_range_nowait
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
                call    thk_clear_text_rect
                add     sp, 8
                push    di
                call    thk_res_4EA6
                add     sp, 2

loc_1C94D:                              ; CODE XREF: inn_menu+36E↑j
                cmp     si, 1Bh
                jnz     short loc_1C8FA
                mov     [bp+var_C], si
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4

loc_1C962:                              ; CODE XREF: inn_menu+2C0↑j
                                        ; inn_menu+2C8↑j ...
                cmp     [bp+var_8], 20h ; ' '
                jnz     short loc_1C97C
                add     [bp+var_A], 18h
                cmp     [bp+var_A], 18h
                jle     short loc_1C977
                mov     [bp+var_A], 0

loc_1C977:                              ; CODE XREF: inn_menu+3B0↑j
                mov     [bp+var_E], 1

loc_1C97C:                              ; CODE XREF: inn_menu+3A6↑j
                cmp     [bp+var_8], 31h ; '1'
                jb      short loc_1C9EC
                cmp     [bp+var_8], 35h ; '5'
                ja      short loc_1C9EC
                mov     [bp+var_E], 1
                mov     al, byte ptr [bp+var_8]
                sub     al, 31h ; '1'
                mov     g_map_id, al
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
                call    thk_text_set_fg
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_6], 14h
                mov     si, 14h

loc_1C9C8:                              ; CODE XREF: inn_menu+413↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_1C9C8
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     g_party_size, 0 ; CODE XREF: seg002:0B01↑J
                mov     [bp+var_A], 0

loc_1C9EC:                              ; CODE XREF: inn_menu+3C0↑j
                                        ; inn_menu+3C6↑j
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
                mov     cl, g_map_id
                sub     ch, ch
                inc     cx
                cmp     ax, cx
                jnz     short loc_1CA3E
                cmp     [bp+var_A], 18h
                jnz     short loc_1CA25
                mov     bx, [bp+var_8]
                cmp     [bx+3F5h], ah
                jnz     short loc_1CA2B

loc_1CA25:                              ; CODE XREF: inn_menu+45A↑j
                cmp     [bp+var_A], 0
                jnz     short loc_1CA3E

loc_1CA2B:                              ; CODE XREF: inn_menu+463↑j
                mov     ax, [bp+var_8]
                add     ax, [bp+var_A]
                dec     ax
                push    ax
                call    loc_1C4B0
                add     sp, 2
                mov     [bp+var_E], 0

loc_1CA3E:                              ; CODE XREF: inn_menu+430↑j
                                        ; inn_menu+436↑j ...
                cmp     [bp+var_8], 1Bh
                jz      short loc_1CA47
                jmp     loc_1CAEF
; ---------------------------------------------------------------------------

loc_1CA47:                              ; CODE XREF: inn_menu+482↑j
                cmp     g_outdoors, 2
                jnz     short loc_1CA51
                jmp     loc_1CAEF
; ---------------------------------------------------------------------------

loc_1CA51:                              ; CODE XREF: inn_menu+48C↑j
                                        ; seg002:0A89↑J
                call    thk_res_35A8
                mov     ax, 17h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 5Bh ; '['
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, offset aExitToDosYN ; " Exit to DOS (Y/N)?  "
                push    ax

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                call    thk_text_puts
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 5Dh ; ']'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 17h
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------

loc_1CAC0:                              ; CODE XREF: inn_menu+514↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_1CAD6

loc_1CAC5:                              ; CODE XREF: inn_menu+4FE↑j
                call    thk_wait_key
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1CAC0

loc_1CAD6:                              ; CODE XREF: inn_menu+503↑j
                mov     [bp+var_8], si
                cmp     si, 4Eh ; 'N'
                jnz     short loc_1CAEC
                call    thk_res_35A8
                call    inn_esc_prompt
                mov     [bp+var_8], 0
                jmp     short loc_1CAEF
; ---------------------------------------------------------------------------
                align 2

loc_1CAEC:                              ; CODE XREF: inn_menu+51C↑j
                call    thk_res_3FC4

loc_1CAEF:                              ; CODE XREF: inn_menu+484↑j
                                        ; inn_menu+48E↑j ...
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

loc_1CB0C:                              ; CODE XREF: inn_menu+533↑j
                                        ; inn_menu+542↑j
                cmp     [bp+var_8], 1Bh
                jz      short loc_1CB15
                jmp     loc_1C692
; ---------------------------------------------------------------------------

loc_1CB15:                              ; CODE XREF: inn_menu+550↑j
                mov     al, g_map_id
                mov     byte_1DC24, al
                sub     ah, ah
                mov     si, ax
                mov     al, [si+21E8h]
                mov     g_party_x, al
                mov     al, [si+21EEh]
                mov     g_party_y, al
                mov     al, [si+21F4h]
                mov     g_facing, al
                call    thk_set_facing_masks
                mov     ax, [bp+var_2]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
inn_menu        endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

inn_run         proc near               ; CODE XREF: seg002:0A65↑J

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
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
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_4], ax
                push    ax
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     al, byte_1DB92
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                cmp     byte_1DB96, 3
                jnz     short loc_1CB9D
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2           ; CODE XREF: seg002:080D↑J

loc_1CB9D:                              ; CODE XREF: inn_run+4F↑j
                call    thk_draw_frame_alt
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                sub     si, si
                mov     di, 22A6h

loc_1CBB1:                              ; CODE XREF: inn_run+8E↓j
                lea     ax, [si+1]
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
                cmp     si, 0Ah
                jl      short loc_1CBB1
                mov     [bp+var_2], si
                push    [bp+var_4]
                call    thk_text_window_close
                add     sp, 2

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                mov     ax, 7
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                mov     al, byte_1DC24
                mov     g_map_id, al
                call    thk_load_roster
                call    inn_leave
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 10h
inn_run         endp

ovl_1RETINN     ends

