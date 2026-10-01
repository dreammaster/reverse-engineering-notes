; ===========================================================================

; Segment type: Pure code
ovl_1MENU2      segment byte public 'CODE' use16
                assume cs:ovl_1MENU2
                ;org 7E10h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; "Name:" line editor
; Attributes: bp-based frame

prompt_name     proc near               ; CODE XREF: seg002:01C5↑J
                                        ; roster_view_all+1FD↓p
                                        ; DATA XREF: ...

var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     ax, 15h
                push    ax
                mov     ax, 24h ; '$'
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aName ; "Name:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                lea     ax, [bp+var_C]
                push    ax
                call    thk_read_string
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_E], ax
                or      ax, ax
                jz      short loc_17EC2
                cmp     ax, 0Ah
                jge     short loc_17E80
                mov     ax, 0Ah
                sub     ax, [bp+var_E]
                mov     [bp+var_10], ax
                mov     si, [bp+var_E]
                mov     al, 20h ; ' '
                mov     cx, [bp+var_10]
                lea     di, [bp+si+var_C]
                push    ss
                pop     es
                repne stosb
                mov     ax, [bp+var_10]
                add     [bp+var_E], ax

loc_17E80:                              ; CODE XREF: prompt_name+50↑j
                mov     [bp+var_2], 0
                mov     [bp+var_E], 0
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     [bp+var_10], ax
                mov     bx, ax
                mov     cx, 5
                lea     di, [bx+7E20h]
                lea     si, [bp+var_C]
                push    ds
                pop     es
                assume es:DGROUP
                repne movsw
                movsb
                add     [bp+var_E], 0Bh
                call    thk_save_roster
                mov     ax, 1
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                lea     ax, [bp+var_C]
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_17EC2:                              ; CODE XREF: prompt_name+4B↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
prompt_name     endp


; =============== S U B R O U T I N E =======================================

; "Are You Sure (Y/N)?"
; Attributes: bp-based frame

prompt_confirm_yn proc near             ; CODE XREF: roster_view_all+1E3↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     ax, 15h
                push    ax
                mov     ax, 24h ; '$'
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
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
                mov     ax, 14h
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAreYouSureYN ; "Are You Sure (Y/N)?"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_17F21
; ---------------------------------------------------------------------------
                align 2

loc_17F1C:                              ; CODE XREF: prompt_confirm_yn+68↓j
                cmp     ax, 59h ; 'Y'
                jz      short loc_17F32

loc_17F21:                              ; CODE XREF: prompt_confirm_yn+51↑j
                call    thk_kbd_poll
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_17F1C

loc_17F32:                              ; CODE XREF: prompt_confirm_yn+57↑j
                mov     [bp+var_2], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_17F64
                mov     ax, 82h
                imul    [bp+arg_0]
                mov     bx, ax
                mov     byte ptr [bx+7E2Bh], 0
                mov     g_party_size, 0
                mov     [bp+var_2], 8
                mov     ax, 0FFFFh
                mov     cx, 8
                mov     di, 416h
                push    ds
                pop     es
                repne stosw
                call    thk_save_roster
                jmp     short loc_17F7E
; ---------------------------------------------------------------------------

loc_17F64:                              ; CODE XREF: prompt_confirm_yn+70↑j
                mov     ax, 0C7h
                push    ax
                mov     ax, 13Fh
                push    ax
                mov     ax, 0B7h
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_rect_pages
                add     sp, 0Ch

loc_17F7E:                              ; CODE XREF: prompt_confirm_yn+9A↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
prompt_confirm_yn endp


; =============== S U B R O U T I N E =======================================

; view / rename (Ctrl-N) / delete (Ctrl-D) characters
; Attributes: bp-based frame

roster_view_all proc near               ; CODE XREF: main_options_menu+263↓p

var_12          = word ptr -12h
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
                sub     sp, 12h
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     ax, 3
                push    ax
                call    thk_clear_text_preset
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
                sub     ax, ax
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aViewAll ; "(View All)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                call    thk_print_gold_label
                sub     ax, ax
                push    ax
                push    ax
                call    thk_1RETINN_C2C0
                add     sp, 4
                mov     ax, 13h
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAXToView ; "'A' - 'X' to View"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpaceFor ; "'Space' for "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1DE44
                call    thk_text_puts
                add     sp, 2

loc_1801F:                              ; CODE XREF: roster_view_all+25C↓j
                mov     ax, 78h ; 'x'
                push    ax
                mov     ax, 20h ; ' '
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_6], 0
                cmp     ax, 20h ; ' '
                jnz     short loc_18082
                add     [bp+var_2], 18h
                cmp     [bp+var_2], 18h
                jle     short loc_18052
                mov     [bp+var_2], 0

loc_18052:                              ; CODE XREF: roster_view_all+C7↑j
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk_1RETINN_C2C0
                add     sp, 4
                mov     ax, 15h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+var_2], 0
                jle     short loc_18074
                mov     bx, 1
                jmp     short loc_18076
; ---------------------------------------------------------------------------

loc_18074:                              ; CODE XREF: roster_view_all+E9↑j
                sub     bx, bx

loc_18076:                              ; CODE XREF: roster_view_all+EE↑j
                shl     bx, 1
                push    word ptr [bx+5F4h]
                call    thk_text_puts
                add     sp, 2

loc_18082:                              ; CODE XREF: roster_view_all+BD↑j
                cmp     [bp+var_4], 41h ; 'A'
                jnb     short loc_1808B
                jmp     loc_181DA
; ---------------------------------------------------------------------------

loc_1808B:                              ; CODE XREF: roster_view_all+102↑j
                cmp     [bp+var_4], 58h ; 'X'
                jbe     short loc_18094
                jmp     loc_181DA
; ---------------------------------------------------------------------------

loc_18094:                              ; CODE XREF: roster_view_all+10B↑j
                mov     ax, [bp+var_4]
                add     ax, [bp+var_2]
                mov     cx, 82h
                mul     cx
                mov     bx, ax
                cmp     byte ptr [bx+5D29h], 0
                jnz     short loc_180AB
                jmp     loc_181DA
; ---------------------------------------------------------------------------

loc_180AB:                              ; CODE XREF: roster_view_all+122↑j
                cmp     [bp+var_2], 0
                jz      short loc_180C7
                cmp     [bp+var_2], 18h
                jz      short loc_180BA
                jmp     loc_181DA
; ---------------------------------------------------------------------------

loc_180BA:                              ; CODE XREF: roster_view_all+131↑j
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+3B5h], 0
                jnz     short loc_180C7
                jmp     loc_181DA
; ---------------------------------------------------------------------------

loc_180C7:                              ; CODE XREF: roster_view_all+12B↑j
                                        ; roster_view_all+13E↑j
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     ax, [bp+var_4]
                add     ax, [bp+var_2]
                mov     [bp+var_10], ax
                sub     ax, 41h ; 'A'
                mov     [bp+var_12], ax
                push    ax
                mov     al, byte ptr [bp+var_4]
                sub     ah, ah
                push    ax
                call    thk_show_character_sheet
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, [bp+var_12]
                mov     [bp+var_A], ax
                mov     ax, 82h
                mul     [bp+var_10]
                add     ax, 5D29h
                mov     [bp+var_C], ax
                mov     ax, [bp+var_A]
                mov     [bp+var_E], ax
                mov     di, [bp+var_6]
                mov     si, [bp+var_8]

loc_18117:                              ; CODE XREF: roster_view_all+218↓j
                cmp     [bp+var_2], 0
                jnz     short loc_1818A
                mov     ax, 14h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCtrlNReNameCha ; "(Ctrl)-'N' Re-Name Character"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCtrlDDeleteCha ; "(Ctrl)-'D' Delete Character"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Eh
                push    ax
                mov     ax, 4
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 4
                jnz     short loc_18179
                push    [bp+var_E]
                call    prompt_confirm_yn
                add     sp, 2
                inc     di
                mov     bx, [bp+var_C]
                cmp     byte ptr [bx], 0
                jnz     short loc_18179
                mov     si, 1Bh

loc_18179:                              ; CODE XREF: roster_view_all+1DE↑j
                                        ; roster_view_all+1F0↑j
                cmp     si, 0Eh
                jnz     short loc_18197
                push    [bp+var_A]
                call    prompt_name
                add     sp, 2
                inc     di
                jmp     short loc_18197
; ---------------------------------------------------------------------------

loc_1818A:                              ; CODE XREF: roster_view_all+197↑j
                mov     ax, 1Bh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                mov     si, 1Bh

loc_18197:                              ; CODE XREF: roster_view_all+1F8↑j
                                        ; roster_view_all+204↑j
                cmp     si, 1Bh
                jz      short loc_1819F
                jmp     loc_18117
; ---------------------------------------------------------------------------

loc_1819F:                              ; CODE XREF: roster_view_all+216↑j
                mov     [bp+var_6], di
                mov     [bp+var_8], si
                or      di, di
                jz      short loc_181CD
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk_1RETINN_C2C0
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     [bp+var_6], 0

loc_181CD:                              ; CODE XREF: roster_view_all+223↑j
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4

loc_181DA:                              ; CODE XREF: roster_view_all+104↑j
                                        ; roster_view_all+10D↑j ...
                cmp     [bp+var_4], 1Bh
                jz      short loc_181E3
                jmp     loc_1801F
; ---------------------------------------------------------------------------

loc_181E3:                              ; CODE XREF: roster_view_all+25A↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
roster_view_all endp


; =============== S U B R O U T I N E =======================================

; C create, V view all, T transfer, G go to town
; Attributes: bp-based frame

main_options_menu proc near             ; CODE XREF: seg002:01B9↑J

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_4], 0
                mov     [bp+var_2], 0
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_load_roster

loc_18210:                              ; CODE XREF: main_options_menu+31↓j
                push    word_1DD18
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBAA, ax
                mov     word_1DBAC, dx
                or      dx, ax
                jz      short loc_18210
                sub     ax, ax
                mov     word_1DBB0, ax
                mov     word_1DBAE, ax

loc_1822D:                              ; CODE XREF: main_options_menu+4E↓j
                push    word_1DD1A
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBA6, ax
                mov     word_1DBA8, dx
                or      dx, ax
                jz      short loc_1822D
                mov     ax, 17h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_8], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 81h
                mov     al, g_inn_town
                mov     g_map_id, al
                mov     ax, 14h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Bh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 0Ch
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:0609↑J
                add     sp, 4
                mov     ax, offset aMainOptions ; "Main Options\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_6], 0Ch
                mov     si, 0Ch

loc_182A7:                              ; CODE XREF: main_options_menu+C0↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_182A7
                mov     ax, 0Fh
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCCreateNewChar ; "C - Create New Characters\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aVViewAllCharac ; "V - View All Characters\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTTransferChara ; "T - Transfer Characters\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGGoToTown ; "G - GO TO TOWN             \n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     di, [bp+var_2]

loc_18317:                              ; CODE XREF: main_options_menu+2C8↓j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 6
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMight ; "MIGHT"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 3
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 676h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 5
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMagic ; "MAGIC"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 7
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBookTwo ; "Book Two"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 9
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGatesToAnother ; "Gates To Another World!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 8
                push    ax
                push    ax
                push    di
                push    word_1DBAC
                push    word_1DBAA
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 8
                push    ax
                mov     ax, 0D8h
                push    ax
                push    di
                inc     di
                push    word_1DBAC
                push    word_1DBAA
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 4Eh ; 'N'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_rect_pages
                add     sp, 0Ch
                cmp     di, 5
                jnz     short loc_183F0
                sub     di, di

loc_183F0:                              ; CODE XREF: main_options_menu+1FA↑j
                mov     ax, 64h ; 'd'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                call    thk_kbd_poll
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1840E
                call    thk_quit_to_dos

loc_1840E:                              ; CODE XREF: main_options_menu+217↑j
                mov     ax, si
                cmp     ax, 43h ; 'C'
                jz      short loc_18427
                cmp     ax, 56h ; 'V'
                jz      short loc_18427
                cmp     ax, 54h ; 'T'
                jz      short loc_18427
                cmp     ax, 47h ; 'G'
                jz      short loc_18427
                jmp     loc_184B4
; ---------------------------------------------------------------------------

loc_18427:                              ; CODE XREF: main_options_menu+221↑j
                                        ; main_options_menu+226↑j ...
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                push    [bp+var_8]
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     ax, si
                cmp     ax, 43h ; 'C'
                jnz     short loc_1844E
                call    loc_18FE4
                jmp     short loc_18496
; ---------------------------------------------------------------------------

loc_1844E:                              ; CODE XREF: main_options_menu+255↑j
                mov     ax, si
                cmp     ax, 56h ; 'V'
                jnz     short loc_1845A
                call    roster_view_all
                jmp     short loc_18496
; ---------------------------------------------------------------------------

loc_1845A:                              ; CODE XREF: main_options_menu+261↑j
                mov     ax, si
                cmp     ax, 54h ; 'T'
                jnz     short loc_18490
                mov     ax, word_1DBAE
                or      ax, word_1DBB0
                jnz     short loc_1848B
                mov     g_disk_needed, 2

loc_18470:                              ; CODE XREF: main_options_menu+291↓j
                push    word_1DD1C
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBAE, ax
                mov     word_1DBB0, dx
                or      dx, ax
                jz      short loc_18470
                mov     g_disk_needed, 1

loc_1848B:                              ; CODE XREF: main_options_menu+276↑j
                call    transfer_characters
                jmp     short loc_18496
; ---------------------------------------------------------------------------

loc_18490:                              ; CODE XREF: main_options_menu+26D↑j
                call    thk_1RETINN_C5C0
                mov     [bp+var_4], ax

loc_18496:                              ; CODE XREF: main_options_menu+25A↑j
                                        ; main_options_menu+266↑j ...
                cmp     [bp+var_4], 0
                jz      short loc_184A2
                mov     ax, 1
                jmp     short loc_184A4
; ---------------------------------------------------------------------------
                align 2

loc_184A2:                              ; CODE XREF: main_options_menu+2A8↑j
                sub     ax, ax

loc_184A4:                              ; CODE XREF: main_options_menu+2AD↑j
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                push    [bp+var_8]
                call    thk_text_window_close
                add     sp, 2

loc_184B4:                              ; CODE XREF: main_options_menu+232↑j
                cmp     [bp+var_4], 0
                jnz     short loc_184BD
                jmp     loc_18317
; ---------------------------------------------------------------------------

loc_184BD:                              ; CODE XREF: main_options_menu+2C6↑j
                mov     [bp+var_2], di
                mov     [bp+var_6], si
                push    word_1DBA8
                push    word_1DBA6
                call    thk_free_far_block
                add     sp, 4
                mov     ax, word_1DBAE
                or      ax, word_1DBB0
                jz      short loc_184E8
                push    word_1DBB0
                push    word_1DBAE
                call    thk_free_far_block
                add     sp, 4

loc_184E8:                              ; CODE XREF: main_options_menu+2E6↑j
                push    word_1DBAC
                push    word_1DBAA
                call    thk_free_far_block
                add     sp, 4
                mov     g_disk_needed, 1

loc_184FC:                              ; CODE XREF: main_options_menu+321↓j
                lea     ax, [bp+var_6]
                push    ax
                mov     ax, offset aAttribDat_0 ; "attrib.dat"
                push    ax
                call    thk_load_file_alloc
                add     sp, 4
                mov     word ptr dword_1DBDE, ax
                mov     word ptr dword_1DBDE+2, dx
                                        ; CODE XREF: seg002:0771↑J
                or      dx, ax
                jz      short loc_184FC

loc_18515:                              ; CODE XREF: main_options_menu+336↓j
                push    word_1DD1E
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBD2, ax
                mov     word_1DBD4, dx
                or      dx, ax
                jz      short loc_18515
                push    g_main_text_win
                call    thk_text_window_close
                add     sp, 2
                mov     ax, 17h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     g_main_text_win, ax
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     g_party_size, 0
                mov     si, 416h
                mov     cx, g_party_size

loc_1855F:                              ; CODE XREF: main_options_menu+381↓j
                cmp     word ptr [si], 0FFFFh
                jnz     short loc_1856A

loc_18564:                              ; CODE XREF: main_options_menu+37F↓j
                mov     g_party_size, cx
                jmp     short loc_18576
; ---------------------------------------------------------------------------

loc_1856A:                              ; CODE XREF: main_options_menu+370↑j
                add     si, 2
                inc     cx
                cmp     cx, 8
                jge     short loc_18564
                jmp     short loc_1855F
; ---------------------------------------------------------------------------
                align 2

loc_18576:                              ; CODE XREF: main_options_menu+376↑j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                push    g_main_text_win
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     ax, 0C7h
                push    ax
                mov     ax, 13Fh
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                call    thk_res_49E2
                mov     ax, 6ACh
                push    ax
                call    thk_print_message_line
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     g_map_id, 0FFh
                mov     al, g_party_y
                sub     ah, ah
                push    ax
                mov     al, g_party_x
                push    ax
                mov     al, g_inn_town
                push    ax
                call    thk_2PLAY_B5EA
                add     sp, 6
                mov     byte_1DBEB, 0
                mov     byte_1DC80, 0
                mov     g_disk_needed, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
main_options_menu endp


; =============== S U B R O U T I N E =======================================

; first empty of the 24 character slots or -1
; Attributes: bp-based frame

roster_find_free_slot proc near         ; CODE XREF: create_character_record+8↓p
                                        ; create_char_pick_class+7↓p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                sub     cx, cx
                mov     si, 7E2Bh

loc_185FA:                              ; CODE XREF: roster_find_free_slot+20↓j
                cmp     byte ptr [si], 0
                jnz     short loc_18604

loc_185FF:                              ; CODE XREF: roster_find_free_slot+1E↓j
                mov     [bp+var_2], cx
                jmp     short loc_18610
; ---------------------------------------------------------------------------

loc_18604:                              ; CODE XREF: roster_find_free_slot+F↑j
                add     si, 82h
                inc     cx
                cmp     cx, 18h
                jge     short loc_185FF
                jmp     short loc_185FA
; ---------------------------------------------------------------------------

loc_18610:                              ; CODE XREF: roster_find_free_slot+14↑j
                cmp     [bp+var_2], 18h
                jnz     short loc_1861B
                mov     [bp+var_2], 0FFFFh

loc_1861B:                              ; CODE XREF: roster_find_free_slot+26↑j
                mov     ax, [bp+var_2]
                pop     si
                mov     sp, bp
                pop     bp
                retn
roster_find_free_slot endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; (class, race, alignment, sex, name, stats) fills g_characters[free], saves roster
; Attributes: bp-based frame

create_character_record proc near       ; CODE XREF: create_character+487↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6
arg_4           = byte ptr  8
arg_6           = byte ptr  0Ah
arg_8           = word ptr  0Ch
arg_A           = word ptr  0Eh

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                call    roster_find_free_slot
                mov     [bp+var_4], ax
                mov     ax, 82h
                imul    [bp+var_4]
                add     ax, 7E20h
                mov     [bp+var_2], ax
                mov     cx, 82h
                push    ds
                pop     es
                mov     di, ax
                sub     ax, ax
                repne stosb
                mov     [bp+var_4], ax
                mov     bx, [bp+var_2]
                mov     ax, [bp+arg_8]
                mov     cx, 5
                mov     di, bx
                mov     si, ax
                repne movsw
                movsb
                add     [bp+var_4], 0Bh
                mov     byte ptr [bx+0Bh], 1
                mov     al, [bp+arg_6]
                mov     [bx+0Ch], al
                mov     al, [bp+arg_2]
                mov     [bx+0Eh], al
                mov     al, [bp+arg_0]
                mov     [bx+0Fh], al
                mov     di, bx
                mov     al, [bp+arg_4]
                mov     [di+6Ah], al
                mov     [bx+0Dh], al
                mov     di, [bp+arg_A]
                mov     al, [di]
                mov     [bx+6Bh], al
                mov     [bx+10h], al
                mov     al, [di+1]
                mov     [bx+6Ch], al
                mov     [bx+11h], al
                mov     al, [di+2]      ; CODE XREF: seg002:0561↑J
                mov     [bx+6Dh], al
                mov     [bx+12h], al
                mov     al, [di+3]
                mov     [bx+73h], al
                mov     [bx+27h], al
                mov     al, [di+4]
                mov     [bx+6Eh], al
                mov     [bx+13h], al
                mov     al, [di+5]
                mov     [bx+6Fh], al
                mov     [bx+14h], al
                mov     al, [di+6]
                mov     [bx+70h], al
                mov     [bx+15h], al
                mov     di, bx
                mov     al, 1
                mov     [di+71h], al
                mov     [bx+20h], al
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+6AEh]
                mov     [bx+16h], al
                mov     al, [si+6B4h]
                mov     [bx+17h], al
                mov     al, [si+6BAh]
                mov     [bx+18h], al
                mov     al, [si+6C0h]
                mov     [bx+19h], al
                mov     al, [si+6C6h]
                mov     [bx+1Ah], al
                mov     al, [si+6CCh]
                mov     [bx+1Bh], al
                mov     al, [si+6D2h]
                mov     [bx+1Ch], al
                mov     al, [si+6D8h]
                mov     [bx+1Dh], al
                mov     al, [bp+arg_0]
                mov     si, ax
                mov     al, [si+6DEh]
                mov     [bx+1Eh], al
                mov     di, [bp+arg_A]
                mov     di, [di+3]
                and     di, 0FFh
                shl     di, 1
                mov     ax, [di+6F2h]
                mov     di, si
                shl     di, 1
                add     ax, [di+6E6h]
                mov     [bx+5Eh], ax
                mov     di, bx
                mov     ax, [di+5Eh]
                mov     [bx+74h], ax
                mov     [bx+60h], ax
                cmp     [bp+arg_0], 3
                jz      short loc_18749
                cmp     [bp+arg_0], 4   ; CODE XREF: seg002:0765↑J
                jnz     short loc_18790

loc_18749:                              ; CODE XREF: create_character_record+11D↑j
                mov     si, bx
                mov     al, 1
                mov     [si+72h], al
                mov     [bx+23h], al
                cmp     [bp+arg_0], 3
                jnz     short loc_18776
                mov     si, [bp+arg_A]
                mov     si, [si+2]
                and     si, 0FFh
                shl     si, 1
                mov     ax, [si+71Eh]
                mov     [bx+58h], ax
                mov     [bx+5Ah], ax
                mov     byte ptr [bx+51h], 5Ch ; '\'
                jmp     short loc_18790
; ---------------------------------------------------------------------------
                align 2

loc_18776:                              ; CODE XREF: create_character_record+133↑j
                mov     si, [bp+arg_A]
                mov     si, [si+1]
                and     si, 0FFh
                shl     si, 1
                mov     ax, [si+71Eh]
                mov     [bx+58h], ax
                mov     [bx+5Ah], ax
                mov     byte ptr [bx+51h], 3Ah ; ':'

loc_18790:                              ; CODE XREF: create_character_record+123↑j
                                        ; create_character_record+14F↑j
                mov     byte ptr [bx+21h], 12h
                mov     si, [bp+arg_A]
                mov     si, [si+4]
                and     si, 0FFh
                mov     al, [si+74Dh]
                mov     [bx+24h], al
                mov     byte ptr [bx+25h], 0Ah
                mov     bx, [bp+arg_A]
                mov     bl, [bx+6]
                sub     bh, bh
                shl     bx, 1
                cmp     word ptr [bx+6F2h], 0
                jnz     short loc_187C4
                mov     bx, [bp+var_2]
                mov     byte ptr [bx+3Ah], 1
                jmp     short loc_187E5
; ---------------------------------------------------------------------------
                align 2

loc_187C4:                              ; CODE XREF: create_character_record+194↑j
                mov     bx, [bp+arg_A]
                mov     bl, [bx+6]
                sub     bh, bh
                shl     bx, 1
                mov     si, [bx+6F2h]
                mov     cl, 3
                shl     si, cl
                mov     bl, [bp+arg_0]
                sub     bh, bh
                mov     al, [bx+si+75Ch]
                mov     bx, [bp+var_2]
                mov     [bx+3Ah], al

loc_187E5:                              ; CODE XREF: create_character_record+19D↑j
                mov     byte ptr [bx+26h], 0
                call    thk_save_roster
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
create_character_record endp


; =============== S U B R O U T I N E =======================================

; "Select Class (1-8)", "Exchange Stat (A-G) (ENT) to Reroll", "Roster is Full"
; Attributes: bp-based frame

create_char_pick_class proc near        ; CODE XREF: create_character:loc_18B11↓p
                                        ; create_character+4BE↓p ...

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                call    roster_find_free_slot
                inc     ax
                jnz     short loc_18817
                mov     ax, 0Bh
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRosterIsFull ; "*** Roster is Full ***"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_18817:                              ; CODE XREF: create_char_pick_class+B↑j
                mov     [bp+var_2], 0
                mov     si, 876h

loc_1881E:                              ; CODE XREF: create_char_pick_class+51↓j
                mov     al, [bp+var_2]
                sub     ah, ah
                add     ax, 0Ch
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [si]
                call    thk_text_puts
                add     sp, 2
                add     si, 2
                inc     [bp+var_2]
                cmp     [bp+var_2], 7
                jb      short loc_1881E
                mov     [bp+var_2], 0
                mov     si, 446h

loc_1884C:                              ; CODE XREF: create_char_pick_class+89↓j
                mov     al, [bp+var_2]
                sub     ah, ah
                add     ax, 0Ch
                push    ax
                mov     ax, 1Bh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 839h
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word ptr [si]
                call    thk_text_puts
                add     sp, 2
                add     si, 2
                inc     [bp+var_2]
                cmp     [bp+var_2], 7
                jbe     short loc_1884C
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSelectClass18 ; "Select Class  (1-8)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aExchangeStatAG ; "Exchange Stat (A-G)  (ENT) to Reroll"
                push    ax
                call    thk_text_puts
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
create_char_pick_class endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

show_stats_and_allowed_classes proc near
                                        ; CODE XREF: create_character+AB↓p
                                        ; create_character+229↓p ...

var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     di, [bp+arg_0]

loc_188C1:                              ; CODE XREF: show_stats_and_allowed_classes+42↓j
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+0Ch]
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 2
                push    ax
                mov     bx, si
                add     bx, di
                mov     al, [bx]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                inc     [bp+var_2]
                cmp     [bp+var_2], 7
                jb      short loc_188C1
                cmp     [bp+arg_2], 0
                jz      short loc_1894B
                mov     [bp+var_2], 0
                mov     di, [bp+arg_2]

loc_18903:                              ; CODE XREF: show_stats_and_allowed_classes+97↓j
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+0Ch]
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, si
                add     bx, di
                cmp     byte ptr [bx], 0
                jz      short loc_18938
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                inc     ax
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                jmp     short loc_18942
; ---------------------------------------------------------------------------

loc_18938:                              ; CODE XREF: show_stats_and_allowed_classes+6D↑j
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_18942:                              ; CODE XREF: show_stats_and_allowed_classes+84↑j
                inc     [bp+var_2]
                cmp     [bp+var_2], 7
                jbe     short loc_18903

loc_1894B:                              ; CODE XREF: show_stats_and_allowed_classes+48↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
show_stats_and_allowed_classes endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; per-class minimum-stat tests (8 classes)
; Attributes: bp-based frame

class_allowed   proc near               ; CODE XREF: create_character+9F↓p

var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     cl, cl
                mov     bx, [bp+arg_0]
                mov     al, [bx]
                mov     [bp+var_4], al
                mov     si, bx
                mov     dx, [bp+arg_2]

loc_18969:                              ; CODE XREF: class_allowed+91↓j
                mov     ax, cx
                sub     ah, ah
                cmp     ax, 7           ; switch 8 cases
                ja      short def_18975 ; jumptable 00018975 default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_18975[bx] ; switch jump
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1897C:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed:jpt_18975↓o
                cmp     [bp+var_4], 0Fh ; jumptable 00018975 case 0
                jb      short loc_18997

loc_18982:                              ; CODE XREF: class_allowed:loc_18995↓j
                mov     al, 1
                jmp     short loc_18999
; ---------------------------------------------------------------------------

loc_18986:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+7E↓o
                cmp     byte ptr [si], 0Dh ; jumptable 00018975 case 1
                jb      short loc_18997
                cmp     byte ptr [si+2], 0Dh
                jb      short loc_18997
                cmp     byte ptr [si+3], 0Dh

loc_18995:                              ; CODE XREF: class_allowed+5C↓j
                                        ; class_allowed+62↓j ...
                jnb     short loc_18982

loc_18997:                              ; CODE XREF: class_allowed+2E↑j
                                        ; class_allowed+37↑j ...
                sub     al, al

loc_18999:                              ; CODE XREF: class_allowed+32↑j
                mov     bx, cx
                sub     bh, bh
                mov     di, dx
                mov     [bx+di], al
                jmp     short def_18975 ; jumptable 00018975 default case
; ---------------------------------------------------------------------------
                align 2

loc_189A4:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+80↓o
                cmp     byte ptr [si+1], 0Dh ; jumptable 00018975 case 2

loc_189A8:                              ; CODE XREF: class_allowed+74↓j
                jb      short loc_18997
                cmp     byte ptr [si+5], 0Dh
                jmp     short loc_18995
; ---------------------------------------------------------------------------

loc_189B0:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+82↓o
                cmp     byte ptr [si+2], 0Dh ; jumptable 00018975 case 3
                jmp     short loc_18995
; ---------------------------------------------------------------------------

loc_189B6:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+84↓o
                cmp     byte ptr [si+1], 0Dh ; jumptable 00018975 case 4
                jmp     short loc_18995
; ---------------------------------------------------------------------------

loc_189BC:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+86↓o
                cmp     byte ptr [si+6], 0Dh ; jumptable 00018975 case 5
                jmp     short loc_18995
; ---------------------------------------------------------------------------

loc_189C2:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+88↓o
                cmp     byte ptr [si+4], 0Dh ; jumptable 00018975 case 6
                jmp     short loc_189A8
; ---------------------------------------------------------------------------

loc_189C8:                              ; CODE XREF: class_allowed+23↑j
                                        ; DATA XREF: class_allowed+8A↓o
                cmp     byte ptr [si+3], 0Fh ; jumptable 00018975 case 7
                jmp     short loc_18995
; ---------------------------------------------------------------------------
jpt_18975       dw offset loc_1897C     ; DATA XREF: class_allowed+23↑r
                                        ; jump table for switch statement
                dw offset loc_18986     ; jumptable 00018975 case 1
                dw offset loc_189A4     ; jumptable 00018975 case 2
                dw offset loc_189B0     ; jumptable 00018975 case 3
                dw offset loc_189B6     ; jumptable 00018975 case 4
                dw offset loc_189BC     ; jumptable 00018975 case 5
                dw offset loc_189C2     ; jumptable 00018975 case 6
                dw offset loc_189C8     ; jumptable 00018975 case 7
; ---------------------------------------------------------------------------

def_18975:                              ; CODE XREF: class_allowed+1E↑j
                                        ; class_allowed+4F↑j
                inc     cl              ; jumptable 00018975 default case
                cmp     cl, 7
                jbe     short loc_18969
                mov     [bp+var_2], cl
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
class_allowed   endp


; =============== S U B R O U T I N E =======================================

; random stats (7)
; Attributes: bp-based frame

roll_stats      proc near               ; CODE XREF: create_character+4D0↓p
                                        ; create_character_finish+E↓p ...

var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     [bp+var_2], 0
                mov     bx, [bp+arg_0]
                sub     ax, ax
                mov     cx, 3
                mov     di, bx
                push    ds
                pop     es
                repne stosw
                stosb
                add     [bp+var_2], 7
                mov     [bp+var_4], 3
                mov     di, 3
                mov     [bp+var_2], 0
                mov     si, [bp+arg_0]
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_rand_range
                add     sp, 4
                sub     ah, ah
                mov     cl, 0Ah
                div     cl
                mov     bl, [bp+var_2]
                sub     bh, bh
; ---------------------------------------------------------------------------
                db    0
                db    0
                db 0FEh
                db  46h ; F
                db 0FEh
                db  80h
                db  7Eh ; ~
                db 0FEh
                db    7
                db  72h ; r
                db 0DCh
                db  4Fh ; O
                db  75h ; u
                db 0D2h
                db 0FFh
                db  76h ; v
                db    6
                db 0FFh
                db  76h ; v
                db    4
                db 0E8h
                db    7
                db 0FFh
                db  83h
                db 0C4h
                db    4
                db 0FFh
                db  76h ; v
                db    6
                db 0FFh
                db  76h ; v
                db    4
                db 0E8h
                db  5Bh ; [
                db 0FEh
                db  83h
                db 0C4h
                db    4
                db  5Eh ; ^
                db  5Fh ; _
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h
roll_stats      endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; class / race (1-5) / alignment (1-3) screens
; Attributes: bp-based frame

create_character proc near              ; CODE XREF: create_character_finish+3E↓p

var_20          = byte ptr -20h
var_1E          = byte ptr -1Eh
var_1C          = byte ptr -1Ch
var_1A          = byte ptr -1Ah
var_18          = byte ptr -18h
var_16          = byte ptr -16h
var_E           = byte ptr -0Eh
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

; FUNCTION CHUNK AT 8EC8 SIZE 00000027 BYTES
; FUNCTION CHUNK AT 8EF0 SIZE 00000058 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     bx, word_1E0FA
                mov     al, byte ptr [bp+arg_0]
                add     al, 41h ; 'A'
                mov     [bx+0Fh], al
                mov     ax, 14h
                push    ax
                mov     ax, 1Dh
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1E0FA
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_18AA9
; ---------------------------------------------------------------------------

loc_18AA4:                              ; CODE XREF: create_character+65↓j
                cmp     ax, 47h ; 'G'
                jbe     short loc_18AC7

loc_18AA9:                              ; CODE XREF: create_character+42↑j
                mov     ax, 67h ; 'g'
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_18AA4

loc_18AC7:                              ; CODE XREF: create_character+47↑j
                mov     [bp+var_4], si
                cmp     si, 1Bh
                jz      short loc_18B11
                sub     [bp+var_4], 41h ; 'A'
                mov     al, byte ptr [bp+arg_0]
                sub     ah, ah
                cmp     ax, [bp+var_4]
                jz      short loc_18B11
                mov     si, [bp+var_4]
                add     si, [bp+arg_2]
                mov     al, [si]
                mov     [bp+var_2], al
                mov     al, byte ptr [bp+arg_0]
                mov     di, ax
                add     di, [bp+arg_2]
                mov     al, [di]
                mov     [si], al

loc_18AF4:                              ; CODE XREF: seg002:03F9↑J
                mov     al, [bp+var_2]
                mov     [di], al
                push    [bp+arg_4]
                push    [bp+arg_2]
                call    class_allowed
                add     sp, 4
                push    [bp+arg_4]
                push    [bp+arg_2]
                call    show_stats_and_allowed_classes
                add     sp, 4

loc_18B11:                              ; CODE XREF: create_character+6D↑j
                                        ; create_character+7B↑j
                call    create_char_pick_class
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

create_character_screen:                ; CODE XREF: create_character_finish+67↓p
                push    bp              ; class -> race -> alignment -> sex -> name
                mov     bp, sp
                sub     sp, 24h
                push    di
                push    si
                mov     [bp+var_18], 0
                mov     [bp+var_1C], 0
                mov     ax, [bp+arg_2]
                mov     cx, 3
                lea     di, [bp+var_16]
                mov     si, ax
                push    ss
                pop     es
                assume es:nothing
                repne movsw
                movsb
                add     [bp+var_1C], 7
                mov     bx, [bp+arg_0]
                mov     si, [bp+arg_4]
                cmp     byte ptr [bx+si], 0
                jnz     short loc_18B4C
                jmp     loc_18EC8
; ---------------------------------------------------------------------------

loc_18B4C:                              ; CODE XREF: create_character+E7↑j
                mov     al, byte ptr [bp+arg_0]
                mov     [bp+var_1A], al
                mov     ax, 14h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Ch
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 0Ch
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aClass ; "Class= "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bl, [bp+var_1A]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+446h]
                call    thk_text_puts
                add     sp, 2
                mov     [bp+var_1C], 0
                mov     di, 456h

loc_18BAE:                              ; CODE XREF: create_character+191↓j
                mov     al, [bp+var_1C]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+0Eh]
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                lea     ax, [si+1]
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 8B4h
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     [bp+var_1C]
                cmp     [bp+var_1C], 4
                jbe     short loc_18BAE
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRace15 ; "Race (1-5)   "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 35h ; '5'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                mov     [bp+var_1E], al
                cmp     al, 1Bh
                jnz     short loc_18C23
                jmp     loc_18EC8
; ---------------------------------------------------------------------------

loc_18C23:                              ; CODE XREF: create_character+1BE↑j
                sub     [bp+var_1E], 31h ; '1'
                mov     ax, 0Dh
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRace ; " Race= "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bl, [bp+var_1E]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+456h]
                call    thk_text_puts
                add     sp, 2
                mov     [bp+var_1C], 0
                mov     al, [bp+var_1E]
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                mov     si, ax
                mov     di, [bp+arg_2]
                mov     cl, [bp+var_1C]

loc_18C6B:                              ; CODE XREF: create_character+21E↓j
                mov     ax, cx
                sub     ah, ah
                mov     dx, ax
                mov     bx, dx
                mov     al, [bx+si+93Ch]
                add     [bx+di], al
                inc     cl
                cmp     cl, 7
                jb      short loc_18C6B
                mov     [bp+var_1C], cl
                sub     ax, ax
                push    ax
                push    [bp+arg_2]
                call    show_stats_and_allowed_classes
                add     sp, 4
                mov     ax, 13h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Eh
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     [bp+var_1C], 0
                mov     di, 460h

loc_18CAC:                              ; CODE XREF: create_character+28F↓j
                mov     al, [bp+var_1C]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+0Fh]
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                lea     ax, [si+1]
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 8CDh
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     [bp+var_1C]
                cmp     [bp+var_1C], 2
                jbe     short loc_18CAC
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAlignment13 ; "Alignment (1-3)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 33h ; '3'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                mov     [bp+var_20], al
                cmp     al, 1Bh
                jnz     short loc_18D21
                jmp     loc_18EC8
; ---------------------------------------------------------------------------

loc_18D21:                              ; CODE XREF: create_character+2BC↑j
                sub     [bp+var_20], 31h ; '1'
                mov     ax, 0Eh
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAlign ; "Align= "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bl, [bp+var_20]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+460h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Fh
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     [bp+var_1C], 0
                mov     di, 466h

loc_18D6B:                              ; CODE XREF: ovl_1MENU2:8DAE↓j
                                        ; seg002:0789↑J
                mov     al, [bp+var_1C]
create_character endp

                sub     ah, ah
                mov     si, ax
                lea     ax, [si+10h]
                push    ax
                mov     ax, 19h
                push    ax

loc_18D7A:                              ; CODE XREF: seg002:04A1↑J
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                lea     ax, [si+1]
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 8E8h
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     byte ptr [bp-1Ch]
                cmp     byte ptr [bp-1Ch], 1
                jbe     short loc_18D6B
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSex12 ; "Sex (1-2)      "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                mov     [bp-2], al
                cmp     al, 1Bh
                jnz     short loc_18DE0
                jmp     loc_18EC8
; ---------------------------------------------------------------------------

loc_18DE0:                              ; CODE XREF: ovl_1MENU2:8DDB↑j
                sub     byte ptr [bp-2], 31h ; '1'
                mov     ax, 0Fh
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSex ; "  Sex= "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bl, [bp-2]
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+466h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 10h
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 15h
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTypeNameOfChar ; "Type Name of Character and\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aPressReturnToS ; "Press 'Return' to Save"
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aName_0 ; "Name:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                lea     ax, [bp-0Eh]
                push    ax
                call    thk_read_string
                add     sp, 4
                mov     [bp-1Ch], al
                or      al, al
                jz      short loc_18EC8
                inc     byte ptr [bp-18h]
                cmp     al, 0Ah
                jnb     short loc_18EC4
                mov     al, 0Ah
                sub     al, [bp-1Ch]
                cbw
                mov     [bp-24h], ax
                mov     si, [bp-1Ch]
                and     si, 0FFh
                mov     al, 20h ; ' '
                mov     cx, [bp-24h]
                lea     di, [bp+si-0Eh]
                push    ss
                pop     es
                repne stosb
                mov     al, [bp-24h]
                add     [bp-1Ch], al

loc_18EC4:                              ; CODE XREF: ovl_1MENU2:8EA0↑j
                mov     byte ptr [bp-4], 0
; START OF FUNCTION CHUNK FOR create_character

loc_18EC8:                              ; CODE XREF: create_character+E9↑j
                                        ; create_character+1C0↑j ...
                cmp     [bp+var_18], 0
                jz      short loc_18EF0
                push    [bp+arg_2]
                lea     ax, [bp+var_E]
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_20]
                push    ax
                mov     al, [bp+var_1E]
                push    ax
                mov     al, [bp+var_1A]
                push    ax
                call    create_character_record
                add     sp, 0Ch
                jmp     short loc_18F08
; END OF FUNCTION CHUNK FOR create_character
; ---------------------------------------------------------------------------
                align 2
; START OF FUNCTION CHUNK FOR create_character

loc_18EF0:                              ; CODE XREF: create_character+46C↑j
                mov     [bp+var_1C], 0
                mov     bx, [bp+arg_2]
                mov     cx, 3
                mov     di, bx
                lea     si, [bp+var_16]
                push    ds
                pop     es
                assume es:DGROUP
                repne movsw
                movsb
                add     [bp+var_1C], 7

loc_18F08:                              ; CODE XREF: create_character+48D↑j
                mov     ax, 15h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Ch
                push    ax
                mov     ax, 2
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                call    create_char_pick_class
                cmp     [bp+var_18], 0
                jz      short loc_18F36
                call    create_helper_e
                push    [bp+arg_4]
                push    [bp+arg_2]
                call    roll_stats
                add     sp, 4

loc_18F36:                              ; CODE XREF: create_character+4C5↑j
                push    [bp+arg_4]
                push    [bp+arg_2]
                call    show_stats_and_allowed_classes
                add     sp, 4
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR create_character

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

create_character_finish proc near       ; CODE XREF: ovl_1MENU2:901A↓p

var_12          = byte ptr -12h
var_10          = byte ptr -10h
var_8           = byte ptr -8

                push    bp
                mov     bp, sp
                sub     sp, 12h
                lea     ax, [bp+var_8]
                push    ax
                lea     ax, [bp+var_10]
                push    ax
                call    roll_stats
                add     sp, 4

loc_18F5C:                              ; CODE XREF: create_character_finish+23↓j
                                        ; create_character_finish+94↓j
                call    thk_kbd_poll
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_12], al
                or      al, al
                jz      short loc_18F5C
                cmp     al, 41h ; 'A'
                jb      short loc_18F8C
                cmp     al, 47h ; 'G'
                ja      short loc_18F8C
                lea     ax, [bp+var_8]
                push    ax
                lea     ax, [bp+var_10]
                push    ax
                mov     al, [bp+var_12]
                sub     ah, ah
                sub     ax, 41h ; 'A'
                push    ax
                call    create_character
                add     sp, 6

loc_18F8C:                              ; CODE XREF: create_character_finish+27↑j
                                        ; create_character_finish+2B↑j
                cmp     [bp+var_12], 31h ; '1'
                jb      short loc_18FB5
                cmp     [bp+var_12], 38h ; '8'
                ja      short loc_18FB5
                call    roster_find_free_slot
                inc     ax
                jz      short loc_18FB5
                lea     ax, [bp+var_8]
                push    ax
                lea     ax, [bp+var_10]
                push    ax
                mov     al, [bp+var_12]
                sub     ah, ah
                sub     ax, 31h ; '1'
                push    ax
                call    create_character_screen
                add     sp, 6

loc_18FB5:                              ; CODE XREF: create_character_finish+48↑j
                                        ; create_character_finish+4E↑j ...
                cmp     [bp+var_12], 20h ; ' '
                jnz     short loc_18FC2
                call    create_helper_e
                mov     [bp+var_12], 0Dh

loc_18FC2:                              ; CODE XREF: create_character_finish+71↑j
                cmp     [bp+var_12], 0Dh
                jnz     short loc_18FD6
                lea     ax, [bp+var_8]
                push    ax
                lea     ax, [bp+var_10]
                push    ax
                call    roll_stats
                add     sp, 4

loc_18FD6:                              ; CODE XREF: create_character_finish+7E↑j
                cmp     [bp+var_12], 1Bh
                jz      short loc_18FDF
                jmp     loc_18F5C
; ---------------------------------------------------------------------------

loc_18FDF:                              ; CODE XREF: create_character_finish+92↑j
                mov     sp, bp
                pop     bp
                retn
create_character_finish endp

; ---------------------------------------------------------------------------
                align 2

loc_18FE4:                              ; CODE XREF: main_options_menu+257↑p
                call    thk_print_gold_label
                mov     ax, 16h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 0Bh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCreateNewChara ; "(Create New Characters)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                call    create_char_pick_class
                call    create_helper_e
                call    create_character_finish
                call    thk_text_clear_prompt_line
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

create_helper_e proc near               ; CODE XREF: create_character+4C7↑p
                                        ; create_character_finish+73↑p ...

var_8           = word ptr -8
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     al, byte_1DB95
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     byte_1DB96, 3
                jnz     short loc_1903E
                mov     [bp+var_4], 1

loc_1903E:                              ; CODE XREF: create_helper_e+15↑j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     si, si
                mov     di, 98Eh
                mov     [bp+var_8], 978h

loc_19052:                              ; CODE XREF: create_helper_e+F9↓j
                cmp     si, 5
                jge     short loc_1908E
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 33h ; '3'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                push    [bp+var_4]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 60h ; '`'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 8
                jmp     short loc_190C5
; ---------------------------------------------------------------------------
                align 2

loc_1908E:                              ; CODE XREF: create_helper_e+33↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 33h ; '3'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                mov     ax, 0D8h
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                push    [bp+var_4]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 60h ; '`'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 0D8h

loc_190C5:                              ; CODE XREF: create_helper_e+69↑j
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                mov     ax, [di]
                add     ax, 8
                push    ax
                mov     bx, [bp+var_8]
                mov     ax, [bx]
                add     ax, 8
                push    ax
                push    si
                push    word_1DBA8
                push    word_1DBA6
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 4Eh ; 'N'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_rect_pages
                add     sp, 0Ch
                mov     ax, 4Bh ; 'K'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                add     di, 2
                add     [bp+var_8], 2
                inc     si
                cmp     si, 0Bh
                jge     short loc_1911E
                jmp     loc_19052
; ---------------------------------------------------------------------------

loc_1911E:                              ; CODE XREF: create_helper_e+F7↑j
                mov     [bp+var_2], si
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
create_helper_e endp

; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: load_default_roster+7B↓p
                mov     bp, sp
                sub     sp, 16h
                push    di
                push    si
                mov     byte ptr [bp-2], 20h ; ' '
                mov     bx, [bp+4]
                mov     al, 82h
                mul     byte ptr [bx+7Eh]
                add     ax, 812Ch
                mov     [bp-0Eh], ax
                mov     bx, ax
                mov     byte ptr [bx+0Bh], 1
                mov     byte ptr [bx+0Ah], 0
                sub     si, si
                mov     di, ax

loc_19162:                              ; CODE XREF: ovl_1MENU2:919F↓j
                mov     bx, [bp+4]
                mov     al, [bx+si]
                sub     ah, ah
                push    ax
                call    thk_res_0128
                add     sp, 2
                mov     [bp-0Ch], al
                cmp     byte ptr [bp-2], 20h ; ' '
                jnz     short loc_19185
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp-0Ch], al

loc_19185:                              ; CODE XREF: ovl_1MENU2:9177↑j
                cmp     byte ptr [bp-0Ch], 0
                jnz     short loc_1918F
                mov     byte ptr [bp-0Ch], 20h ; ' '

loc_1918F:                              ; CODE XREF: ovl_1MENU2:9189↑j
                mov     bx, si
                add     bx, di
                mov     al, [bp-0Ch]
                mov     [bx], al
                mov     [bp-2], al
                inc     si
                cmp     si, 0Ah
                jl      short loc_19162
                mov     [bp-14h], si
                mov     bx, [bp+4]
                mov     al, [bx+10h]
                mov     [bp-0Ch], al
                or      al, al
                jz      short loc_191BF
                cmp     al, 3
                jb      short loc_191BC
                mov     byte ptr [bp-0Ch], 0
                jmp     short loc_191BF
; ---------------------------------------------------------------------------
                align 2

loc_191BC:                              ; CODE XREF: ovl_1MENU2:91B3↑j
                dec     byte ptr [bp-0Ch]

loc_191BF:                              ; CODE XREF: ovl_1MENU2:91AF↑j
                                        ; ovl_1MENU2:91B9↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+0Ch], al
                mov     bx, [bp+4]
                mov     al, [bx+11h]
                mov     [bp-0Ch], al
                or      al, al
                jz      short loc_191E2
                dec     byte ptr [bp-0Ch]
                cmp     byte ptr [bp-0Ch], 3
                jb      short loc_191E2
                mov     byte ptr [bp-0Ch], 0

loc_191E2:                              ; CODE XREF: ovl_1MENU2:91D3↑j
                                        ; ovl_1MENU2:91DC↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+0Dh], al
                mov     [bx+6Ah], al
                mov     bx, [bp+4]
                mov     al, [bx+13h]
                mov     [bp-0Ch], al
                or      al, al
                jz      short loc_1920A
                dec     byte ptr [bp-0Ch]
                cmp     byte ptr [bp-0Ch], 5
                jb      short loc_1920A
                mov     byte ptr [bp-0Ch], 0

loc_1920A:                              ; CODE XREF: ovl_1MENU2:91FB↑j
                                        ; ovl_1MENU2:9204↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+0Eh], al
                mov     bx, [bp+4]
                mov     al, [bx+14h]
                mov     [bp-0Ch], al
                or      al, al
                jz      short loc_1922D
                dec     byte ptr [bp-0Ch]
                cmp     byte ptr [bp-0Ch], 6
                jb      short loc_1922D
                mov     byte ptr [bp-0Ch], 0

loc_1922D:                              ; CODE XREF: ovl_1MENU2:921E↑j
                                        ; ovl_1MENU2:9227↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+0Fh], al
                mov     bx, [bp+4]
                mov     al, [bx+17h]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_19247
                mov     byte ptr [bp-0Ch], 14h

loc_19247:                              ; CODE XREF: ovl_1MENU2:9241↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+10h], al
                mov     [bx+6Bh], al
                mov     bx, [bp+4]
                mov     al, [bx+15h]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_19266
                mov     byte ptr [bp-0Ch], 14h

loc_19266:                              ; CODE XREF: ovl_1MENU2:9260↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+11h], al
                mov     [bx+6Ch], al
                mov     bx, [bp+4]
                mov     al, [bx+19h]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_19285
                mov     byte ptr [bp-0Ch], 14h

loc_19285:                              ; CODE XREF: ovl_1MENU2:927F↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+12h], al
                mov     [bx+6Dh], al
                mov     bx, [bp+4]
                mov     al, [bx+1Dh]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_192A4
                mov     byte ptr [bp-0Ch], 14h

loc_192A4:                              ; CODE XREF: ovl_1MENU2:929E↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+13h], al
                mov     [bx+6Eh], al
                mov     bx, [bp+4]
                mov     al, [bx+1Fh]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_192C3
                mov     byte ptr [bp-0Ch], 14h

loc_192C3:                              ; CODE XREF: ovl_1MENU2:92BD↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+14h], al
                mov     [bx+6Fh], al
                mov     bx, [bp+4]
                mov     al, [bx+21h]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_192E2
                mov     byte ptr [bp-0Ch], 14h

loc_192E2:                              ; CODE XREF: ovl_1MENU2:92DC↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+15h], al
                mov     [bx+70h], al
                mov     bx, [bp+4]
                mov     al, [bx+1Bh]
                mov     [bp-0Ch], al
                cmp     al, 15h
                jb      short loc_19301
                mov     byte ptr [bp-0Ch], 14h

loc_19301:                              ; CODE XREF: ovl_1MENU2:92FB↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+27h], al
                mov     [bx+73h], al
                mov     bx, [bp+4]
                mov     al, [bx+58h]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_19320
                mov     byte ptr [bp-0Ch], 1Eh

loc_19320:                              ; CODE XREF: ovl_1MENU2:931A↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+16h], al
                mov     bx, [bp+4]
                mov     al, [bx+5Ah]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_1933A
                mov     byte ptr [bp-0Ch], 1Eh

loc_1933A:                              ; CODE XREF: ovl_1MENU2:9334↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+17h], al
                mov     bx, [bp+4]
                mov     al, [bx+5Ch]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_19354
                mov     byte ptr [bp-0Ch], 1Eh

loc_19354:                              ; CODE XREF: ovl_1MENU2:934E↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+18h], al
                mov     bx, [bp+4]
                mov     al, [bx+5Eh]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_1936E
                mov     byte ptr [bp-0Ch], 1Eh

loc_1936E:                              ; CODE XREF: ovl_1MENU2:9368↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+19h], al
                mov     byte ptr [bx+1Ah], 0
                mov     bx, [bp+4]
                mov     al, [bx+66h]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_1938C
                mov     byte ptr [bp-0Ch], 1Eh

loc_1938C:                              ; CODE XREF: ovl_1MENU2:9386↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+1Bh], al
                mov     bx, [bp+4]
                mov     al, [bx+64h]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_193A6
                mov     byte ptr [bp-0Ch], 1Eh

loc_193A6:                              ; CODE XREF: ovl_1MENU2:93A0↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+1Ch], al
                mov     bx, [bp+4]
                mov     al, [bx+60h]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_193C0
                mov     byte ptr [bp-0Ch], 1Eh

loc_193C0:                              ; CODE XREF: ovl_1MENU2:93BA↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+1Dh], al
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_193E9
                mov     bx, [bp+4]
                mov     al, [bx+6Ch]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_193E0
                mov     byte ptr [bp-0Ch], 1Eh

loc_193E0:                              ; CODE XREF: ovl_1MENU2:93DA↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+1Eh], al

loc_193E9:                              ; CODE XREF: ovl_1MENU2:93CD↑j
                mov     bx, [bp+4]
                mov     al, [bx+23h]
                mov     [bp-0Ch], al
                or      al, al
                jnz     short loc_193F9
                inc     byte ptr [bp-0Ch]

loc_193F9:                              ; CODE XREF: ovl_1MENU2:93F4↑j
                cmp     byte ptr [bp-0Ch], 7
                jb      short loc_19410
                cmp     byte ptr [bx+7Dh], 80h
                jnb     short loc_1940C
                mov     byte ptr [bp-0Ch], 6
                jmp     short loc_19410
; ---------------------------------------------------------------------------
                align 2

loc_1940C:                              ; CODE XREF: ovl_1MENU2:9403↑j
                mov     byte ptr [bp-0Ch], 7

loc_19410:                              ; CODE XREF: ovl_1MENU2:93FD↑j
                                        ; ovl_1MENU2:9409↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-0Ch]
                mov     [si+20h], al
                mov     [bx+71h], al

loc_1941E:                              ; CODE XREF: seg002:07B9↑J
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_19429
                dec     al
                add     [bx+1Eh], al

loc_19429:                              ; CODE XREF: ovl_1MENU2:9422↑j
                mov     byte ptr [bx+22h], 0
                mov     bx, [bp+4]
                mov     al, [bx+25h]
                mov     [bp-0Ch], al
                cmp     al, 1Fh
                jb      short loc_1943E
                mov     byte ptr [bp-0Ch], 1Eh

loc_1943E:                              ; CODE XREF: ovl_1MENU2:9438↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-0Ch]
                mov     [bx+21h], al
                mov     word ptr [bx+5Ch], 64h ; 'd'
                mov     word ptr [bx+66h], 3E8h
                mov     word ptr [bx+68h], 0
                mov     byte ptr [bx+25h], 28h ; '('
                mov     byte ptr [bx+26h], 0
                mov     al, [bx+20h]
                mov     [bp-0Ch], al
                mov     bl, [bx+27h]
                sub     bh, bh
                mov     al, [bx+0A6Ch]
                mov     [bp-10h], al    ; CODE XREF: seg002:07C5↑J
                mov     bx, [bp-0Eh]
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+0A82h]
                add     al, [bp-10h]
                mov     [bp-12h], al
                mov     al, [bp-0Ch]
                dec     byte ptr [bp-0Ch]
                or      al, al
                jz      short loc_194A9
                mov     al, [bp-12h]
                sub     ah, ah
                mov     si, ax
                mov     dx, [bp-0Eh]
                mov     cl, [bp-0Ch]

loc_19499:                              ; CODE XREF: ovl_1MENU2:94A4↓j
                mov     bx, dx
                add     [bx+5Eh], si
                mov     al, cl
                dec     cl
                or      al, al
                jnz     short loc_19499
                mov     [bp-0Ch], cl

loc_194A9:                              ; CODE XREF: ovl_1MENU2:948A↑j
                mov     bx, [bp-0Eh]
                mov     di, bx
                mov     ax, [di+5Eh]
                mov     [bx+74h], ax
                mov     [bx+60h], ax
                mov     al, [bx+0Fh]
                sub     ah, ah
                mov     si, ax
                cmp     [si+0AC8h], ah
                jnz     short loc_194C7
                jmp     loc_19573
; ---------------------------------------------------------------------------

loc_194C7:                              ; CODE XREF: ovl_1MENU2:94C2↑j
                mov     al, [bx+20h]
                mov     [bp-12h], al
                mov     [bp-0Ch], ah
                cmp     [si+0ACEh], ah
                jz      short loc_194DA
                mov     byte ptr [bp-0Ch], 6

loc_194DA:                              ; CODE XREF: ovl_1MENU2:94D4↑j
                inc     byte ptr [bp-12h]
                mov     al, [bp-12h]
                sub     al, [bp-0Ch]
                mov     [bp-10h], al
                cmp     al, 80h
                jb      short loc_194EE
                mov     byte ptr [bp-10h], 0

loc_194EE:                              ; CODE XREF: ovl_1MENU2:94E8↑j
                shr     byte ptr [bp-10h], 1
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     al, [bp-10h]
                mov     [si+72h], al
                mov     [bx+23h], al
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+0AE0h]
                mov     [bp-12h], al
                cmp     al, bh
                jz      short loc_19573
                mov     bx, si
                cmp     byte ptr [bx+23h], 0
                jz      short loc_19573
                mov     byte ptr [bp-0Ch], 0
                mov     al, [bx+11h]
                mov     [bp-10h], al
                cmp     byte ptr [bp-12h], 2
                jnz     short loc_1952D
                mov     al, [bx+12h]
                mov     [bp-10h], al

loc_1952D:                              ; CODE XREF: ovl_1MENU2:9525↑j
                mov     bl, [bp-10h]
                sub     bh, bh
                mov     al, [bx+0A6Ch]
                mov     [bp-10h], al
                mov     bx, [bp-0Eh]
                mov     al, [bx+20h]
                mov     [bp-12h], al
                dec     byte ptr [bp-12h]
                or      al, al
                jz      short loc_19568
                mov     al, [bp-10h]
                sub     ah, ah
                mov     si, ax
                add     si, 3
                mov     dx, bx
                mov     cl, [bp-12h]

loc_19558:                              ; CODE XREF: ovl_1MENU2:9563↓j
                mov     bx, dx
                add     [bx+58h], si
                mov     al, cl
                dec     cl
                or      al, al
                jnz     short loc_19558
                mov     [bp-12h], cl

loc_19568:                              ; CODE XREF: ovl_1MENU2:9547↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     ax, [si+58h]
                mov     [bx+5Ah], ax

loc_19573:                              ; CODE XREF: ovl_1MENU2:94C4↑j
                                        ; ovl_1MENU2:950D↑j ...
                mov     bx, [bp-0Eh]
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+0AD4h]
                mov     [bp-0Ch], al
                cmp     al, bh
                jz      short loc_195A0
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     si, [si+20h]
                and     si, 0FFh
                shl     si, 1
                shl     si, 1
                mov     ax, [si+0AA8h]
                mov     dx, [si+0AAAh]
                jmp     short loc_195B8
; ---------------------------------------------------------------------------

loc_195A0:                              ; CODE XREF: ovl_1MENU2:9584↑j
                mov     bx, [bp-0Eh]
                mov     si, bx
                mov     si, [si+20h]
                and     si, 0FFh
                shl     si, 1
                shl     si, 1
                mov     ax, [si+0A88h]
                mov     dx, [si+0A8Ah]

loc_195B8:                              ; CODE XREF: ovl_1MENU2:959E↑j
                mov     [bx+62h], ax
                mov     [bx+64h], dx
                mov     al, [bx+24h]
                mov     [bp-0Ch], al
                mov     byte ptr [bp-10h], 0
                cmp     al, 8
                jb      short loc_195D7
                mov     bl, al
                sub     bh, bh
                mov     al, [bx+0A6Ch]
                mov     [bp-10h], al

loc_195D7:                              ; CODE XREF: ovl_1MENU2:95CA↑j
                mov     bx, [bp-0Eh]
                mov     al, [bp-10h]
                mov     [bx+1Fh], al
                mov     al, [bx+23h]
                mov     [bp-0Ch], al
                or      al, al
                jz      short loc_1965A
                mov     bl, [bx+0Fh]
                sub     bh, bh
                mov     al, [bx+0ADAh]
                mov     [bp-10h], al
                cmp     al, bh
                jz      short loc_1961C
                mov     al, [bp-0Ch]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+0AFEh]
                mov     [bp-4], al
                mov     al, [si+0B04h]
                mov     [bp-6], al
                mov     al, [si+0B0Ah]
                mov     [bp-8], al
                mov     al, [si+0B10h]
                jmp     short loc_1963C
; ---------------------------------------------------------------------------

loc_1961C:                              ; CODE XREF: ovl_1MENU2:95F8↑j
                mov     al, [bp-0Ch]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+0AE6h]
                mov     [bp-4], al
                mov     al, [si+0AECh]
                mov     [bp-6], al
                mov     al, [si+0AF2h]
                mov     [bp-8], al
                mov     al, [si+0AF8h]

loc_1963C:                              ; CODE XREF: ovl_1MENU2:961A↑j
                mov     [bp-0Ah], al
                mov     bx, [bp-0Eh]
                mov     al, [bp-4]
                mov     [bx+51h], al
                mov     al, [bp-6]
                mov     [bx+52h], al
                mov     al, [bp-8]
                mov     [bx+53h], al
                mov     al, [bp-0Ah]
                mov     [bx+54h], al

loc_1965A:                              ; CODE XREF: ovl_1MENU2:95E8↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; DEFAULT.DAT
; Attributes: bp-based frame

load_default_roster proc near           ; CODE XREF: transfer_characters+2CA↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 8EEh
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 6052h
                push    ax
                push    [bp+arg_0]
                call    thk_read_file_to_buffer
                add     sp, 8
                mov     ax, 12h
                push    ax
                mov     ax, 8EEh
                push    ax
                mov     ax, 546Ch
                push    ax
                push    [bp+arg_0]
                call    thk_read_file_to_buffer
                add     sp, 8
                mov     cx, 0C30h
                mov     di, 7E20h
                mov     ax, ds
                mov     es, ax
                sub     ax, ax
                repne stosb
                mov     [bp+var_2], 6052h

loc_196A4:                              ; CODE XREF: load_default_roster+63↓j
                mov     g_disk_needed, 1
                mov     ax, 30Ch
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 7E20h
                push    ax
                mov     ax, offset aDefaultDat ; "default.dat"
                push    ax
                call    thk_read_file_to_buffer
                add     sp, 8
                mov     si, ax
                or      si, si
                jz      short loc_196A4
                mov     [bp+var_4], si
                mov     g_disk_needed, 2
                sub     si, si
                mov     di, [bp+var_2]

loc_196D3:                              ; CODE XREF: load_default_roster+88↓j
                cmp     byte ptr [si+546Ch], 0
                jz      short loc_196E1
                push    di
                call    loc_1913A
                add     sp, 2

loc_196E1:                              ; CODE XREF: load_default_roster+78↑j
                add     di, 7Fh
                inc     si
                cmp     si, 12h
                jl      short loc_196D3
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
load_default_roster endp


; =============== S U B R O U T I N E =======================================

; from another MM game roster ("Path:", "File not found...", "Save new roster (y/n)?")
; Attributes: bp-based frame

transfer_characters proc near           ; CODE XREF: main_options_menu:loc_1848B↑p

var_66          = word ptr -66h
var_64          = word ptr -64h
var_62          = word ptr -62h
var_60          = word ptr -60h
var_5E          = word ptr -5Eh
var_5C          = word ptr -5Ch
var_5A          = word ptr -5Ah
var_58          = byte ptr -58h
var_34          = word ptr -34h
var_32          = word ptr -32h
var_30          = word ptr -30h
var_2E          = byte ptr -2Eh
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6Eh
                push    di
                push    si
                mov     [bp+var_5C], 18h
                mov     [bp+var_34], 0
                mov     al, byte_1DB93
                sub     ah, ah
                mov     [bp+var_5A], ax
                mov     al, byte_1DB90
                mov     [bp+var_2], ax
                cmp     byte_1DB96, 3
                jnz     short loc_19727
                mov     [bp+var_5A], 1
                mov     [bp+var_2], 2

loc_19727:                              ; CODE XREF: transfer_characters+25↑j
                mov     ax, 6
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 5
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTransferCharac ; "(Transfer Characters)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                call    thk_print_gold_label
                sub     si, si
                mov     di, 0A5Ah

loc_1975A:                              ; CODE XREF: transfer_characters+81↓j
                lea     ax, [si+0Ch]
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
                cmp     si, 7
                jl      short loc_1975A
                mov     [bp+var_5E], si
                sub     si, si
                mov     di, [bp+var_2]

loc_19781:                              ; CODE XREF: transfer_characters+CF↓j
                push    si
                call    thk_gfx_select_page
                add     sp, 2
                push    [bp+var_5A]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 36h ; '6'
                push    ax
                mov     ax, 136h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                push    di
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 136h
                push    ax
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 8
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                inc     si
                cmp     si, 2
                jl      short loc_19781
                mov     [bp+var_30], si
                mov     [bp+var_5E], 6
                mov     [bp+var_66], 6

loc_197D4:                              ; CODE XREF: transfer_characters+18D↓j
                sub     di, di
                mov     [bp+var_64], 0B74h
                mov     si, [bp+var_5C]

loc_197DE:                              ; CODE XREF: transfer_characters+17F↓j
                mov     bx, [bp+var_64]
                mov     ax, [bx]
                mov     [bp+var_32], ax
                mov     ax, 8
                push    ax
                push    si
                push    [bp+var_32]
                push    word_1DBB0
                push    word_1DBAE
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 4Eh ; 'N'
                push    ax
                lea     ax, [si+40h]
                push    ax
                mov     ax, 9
                push    ax
                lea     ax, [si-10h]
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_rect_pages
                add     sp, 0Ch
                push    [bp+var_5A]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 36h ; '6'
                push    ax
                mov     ax, 136h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                push    [bp+var_2]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 136h
                push    ax
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 8
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                cmp     di, 1
                jz      short loc_1985E
                cmp     di, 3
                jnz     short loc_19861

loc_1985E:                              ; CODE XREF: transfer_characters+161↑j
                add     si, 10h

loc_19861:                              ; CODE XREF: transfer_characters+166↑j
                mov     ax, 96h
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                add     [bp+var_64], 2
                inc     di
                cmp     di, 4
                jge     short loc_19878
                jmp     loc_197DE
; ---------------------------------------------------------------------------

loc_19878:                              ; CODE XREF: transfer_characters+17D↑j
                mov     [bp+var_30], di
                mov     [bp+var_5C], si
                dec     [bp+var_66]
                jz      short loc_19886
                jmp     loc_197D4
; ---------------------------------------------------------------------------

loc_19886:                              ; CODE XREF: transfer_characters+18B↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     [bp+var_5C], 10h
                mov     bx, g_cur_gfx_win
                mov     byte ptr [bx+11h], 1
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                mov     [bp+var_5E], 4
                mov     ax, [bp+var_5C]
                add     ax, 3Fh ; '?'
                mov     [bp+var_66], ax
                mov     si, 4
                mov     di, [bp+var_5C]

loc_198BB:                              ; CODE XREF: transfer_characters+1E2↓j
                mov     ax, 4Fh ; 'O'
                push    ax
                push    [bp+var_66]
                mov     ax, 8
                push    ax
                push    di
                call    thk_gfx_fill_rect
                add     sp, 8
                mov     ax, 1Eh
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                dec     si
                jnz     short loc_198BB
                sub     [bp+var_5C], 8
                mov     ax, 8
                push    ax
                push    [bp+var_5C]
                mov     ax, 3
                push    ax
                push    word_1DBB0
                push    word_1DBAE
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     [bp+var_5E], 6
                mov     ax, [bp+var_5C]
                add     ax, 57h ; 'W'
                mov     [bp+var_66], ax
                mov     si, 6
                mov     di, [bp+var_5C]

loc_1990B:                              ; CODE XREF: transfer_characters+232↓j
                mov     ax, 4Fh ; 'O'
                push    ax
                push    [bp+var_66]
                mov     ax, 8
                push    ax
                push    di
                call    thk_gfx_fill_rect
                add     sp, 8
                mov     ax, 1Eh
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                dec     si
                jnz     short loc_1990B
                mov     bx, g_cur_gfx_win
                mov     byte ptr [bx+11h], 0

loc_19932:                              ; CODE XREF: transfer_characters+375↓j
                mov     ax, 1
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aPath ; "Path: "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 22h ; '"'
                push    ax
                lea     ax, [bp+var_58]
                push    ax
                call    thk_read_string
                add     sp, 4
                mov     di, ax
                cmp     di, 1
                sbb     ax, ax
                neg     ax
                mov     [bp+var_34], ax
                or      di, di
                jnz     short loc_19975
                jmp     loc_19A65
; ---------------------------------------------------------------------------

loc_19975:                              ; CODE XREF: transfer_characters+27A↑j
                lea     ax, [bp+var_2E]
                push    ax
                sub     ax, ax
                push    ax
                lea     ax, [bp+var_58]
                push    ax
                call    thk_res_010A
                add     sp, 6
                mov     di, ax
                mov     ax, 1
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                or      di, di
                jz      short loc_199BC
                mov     ax, 14h
                push    ax
                mov     ax, 6
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aFileNotFoundPr ; "File not found... press ENTER"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                jmp     loc_19A65
; ---------------------------------------------------------------------------
                align 2

loc_199BC:                              ; CODE XREF: transfer_characters+29E↑j
                lea     ax, [bp+var_58]
                push    ax
                call    load_default_roster
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSaveNewRosterY ; "Save new roster (y/n)? "
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_199E5
; ---------------------------------------------------------------------------

loc_199E0:                              ; CODE XREF: transfer_characters+2FE↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_199F6

loc_199E5:                              ; CODE XREF: transfer_characters+2E8↑j
                call    thk_wait_key
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_199E0

loc_199F6:                              ; CODE XREF: transfer_characters+2ED↑j
                mov     di, si
                mov     ax, di
                cmp     ax, 59h ; 'Y'
                jnz     short loc_19A04
                mov     ax, 1
                jmp     short loc_19A06
; ---------------------------------------------------------------------------

loc_19A04:                              ; CODE XREF: transfer_characters+307↑j
                sub     ax, ax

loc_19A06:                              ; CODE XREF: transfer_characters+30C↑j
                mov     [bp+var_34], ax
                or      ax, ax
                jz      short loc_19A62
                mov     di, 8
                mov     ax, 0FFFFh
                mov     cx, di
                push    di
                mov     di, 416h
                push    ds
                pop     es
                repne stosw
                pop     di
                mov     g_party_size, 0
                sub     di, di
                sub     ax, ax
                mov     cx, 0Ch
                push    di
                mov     di, 3F6h
                repne stosw
                pop     di
                mov     di, 18h
                sub     al, al
                mov     g_map_id, al
                mov     g_inn_town, al
                mov     [bp+var_60], 3Ch ; '<'
                mov     [bp+var_62], 10h
                sub     si, si

loc_19A49:                              ; CODE XREF: transfer_characters+367↓j
                sub     ax, ax
                mov     cx, 10h
                push    di
                lea     di, [si-6974h]
                repne stosw
                pop     di
                add     si, 20h ; ' '
                cmp     si, 780h
                jl      short loc_19A49
                call    thk_save_roster

loc_19A62:                              ; CODE XREF: transfer_characters+315↑j
                call    thk_load_roster

loc_19A65:                              ; CODE XREF: transfer_characters+27C↑j
                                        ; transfer_characters+2C2↑j
                cmp     [bp+var_34], 0
                jnz     short loc_19A6E
                jmp     loc_19932
; ---------------------------------------------------------------------------

loc_19A6E:                              ; CODE XREF: transfer_characters+373↑j
                mov     [bp+var_5E], di
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
transfer_characters endp

ovl_1MENU2      ends

