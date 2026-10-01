; ===========================================================================

; Segment type: Pure code
ovl_2PLAY       segment byte public 'CODE' use16
                assume cs:ovl_2PLAY
                ;org 7E10h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; main exploration loop: input keys B,C,D,E,M,O,P,Q,R,S,U,V, arrows F0-F3, 1-8 = character screens; runs random encounters, cell triggers
; Attributes: bp-based frame

game_main_loop  proc near               ; CODE XREF: seg002:01C5↑J
                                        ; DATA XREF: seg002:0008↑o ...

var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                mov     [bp+var_E], 1
                mov     [bp+var_6], 0
                mov     [bp+var_2], 0

loc_17E26:                              ; CODE XREF: game_main_loop+468↓j
                call    thk_party_all_disabled
                or      ax, ax
                jz      short loc_17E30
                call    thk_res_3FD8

loc_17E30:                              ; CODE XREF: game_main_loop+1B↑j
                cmp     byte_1DBEB, 0
                jz      short loc_17EAF
                cmp     byte_1DC80, 0
                jnz     short loc_17EAF
                call    thk_res_3FFC
                mov     ax, 7Fh
                push    ax
                mov     ax, 0D7h
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
                cmp     g_view_mode, 1
                jnz     short loc_17E82
                call    draw_minimap
                mov     ax, 47h ; 'G'
                push    ax
                mov     ax, 12Fh
                push    ax
                mov     ax, 10h
                push    ax
                mov     ax, 0E0h
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_rect_pages
                add     sp, 0Ch

loc_17E82:                              ; CODE XREF: game_main_loop+50↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, g_facing
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2

loc_17EAF:                              ; CODE XREF: game_main_loop+25↑j
                                        ; game_main_loop+2C↑j
                cmp     byte_23218, 80h
                jb      short loc_17EB9
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17EB9:                              ; CODE XREF: game_main_loop+A4↑j
                mov     al, byte_231DF
                mov     [bp+var_4], al
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_2], al
                cmp     word_238A0, 0
                jnz     short loc_17EDD
                cmp     byte_1DD59, 0
                jz      short loc_17EE1

loc_17EDD:                              ; CODE XREF: game_main_loop+C4↑j
                mov     [bp+var_2], 0

loc_17EE1:                              ; CODE XREF: game_main_loop+CB↑j
                cmp     [bp+var_2], 1
                jnz     short loc_17F32
                cmp     g_outdoors, 1
                jnz     short loc_17F36
                cmp     byte_1EF2A, 0Ah
                jnz     short loc_17F36
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                sub     ah, ah
                push    ax
                call    thk_res_5F40
                add     sp, 2
                mov     [bp+var_4], al
                cmp     al, 4
                jnz     short loc_17F32
                mov     ax, 5
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                mov     [bp+var_2], 2
                jmp     short loc_17F36
; ---------------------------------------------------------------------------

loc_17F32:                              ; CODE XREF: game_main_loop+D5↑j
                                        ; game_main_loop+109↑j
                mov     [bp+var_2], 0

loc_17F36:                              ; CODE XREF: game_main_loop+DC↑j
                                        ; game_main_loop+E3↑j ...
                cmp     [bp+var_2], 0
                jz      short loc_17F90
                mov     [bp+var_8], 0
                sub     bx, bx
                sub     ax, ax
                mov     cx, 5
                lea     di, [bx-6980h]
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                stosb
                add     [bp+var_8], 0Bh
                mov     byte_1DD58, 0
                mov     byte_1DC65, 0
                cmp     [bp+var_2], 2
                jnz     short loc_17F8A
                mov     al, 6
                sub     al, [bp+var_4]
                mov     [bp+var_A], al
                add     [bp+var_4], 0E1h
                sub     cl, cl
                mov     dl, al
                jmp     short loc_17F83
; ---------------------------------------------------------------------------

loc_17F76:                              ; CODE XREF: game_main_loop+175↓j
                mov     bx, cx
                sub     bh, bh
                mov     al, [bp+var_4]
                mov     [bx-6980h], al
                inc     cl

loc_17F83:                              ; CODE XREF: game_main_loop+164↑j
                cmp     cl, dl
                jb      short loc_17F76
                mov     [bp+var_8], cl

loc_17F8A:                              ; CODE XREF: game_main_loop+152↑j
                call    thk_start_combat
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17F90:                              ; CODE XREF: game_main_loop+12A↑j
                cmp     g_outdoors, 1
                jz      short loc_17F9A
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17F9A:                              ; CODE XREF: game_main_loop+185↑j
                cmp     byte_1EF2A, 0Ah
                jnz     short loc_17FA4
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17FA4:                              ; CODE XREF: game_main_loop+18F↑j
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                sub     ah, ah
                push    ax
                call    thk_res_5F40
                add     sp, 2
                mov     [bp+var_4], al
                cmp     al, 4
                jnz     short loc_1802B
                mov     ax, 0Ch
                push    ax
                call    thk_res_36A6
                add     sp, 2
                mov     [bp+var_C], ax
                or      ax, ax
                jnz     short loc_1802B
                mov     ax, 3Ch ; '<'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 0Ah
                jge     short loc_1802B
                mov     ax, offset aYouReLost ; "You're lost!"
                push    ax
                call    thk_print_message_line
                add     sp, 2

loc_17FF8:                              ; CODE XREF: game_main_loop+1ED↓j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_17FF8
                mov     al, byte_231E4
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, byte_231E4
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr g_party_y, al
                call    map_edge_transition
                call    thk_res_4076
                jmp     short loc_1802B
; ---------------------------------------------------------------------------
                align 2

loc_1801C:                              ; CODE XREF: game_main_loop+220↓j
                cmp     byte_1DC7E, 0
                jz      short loc_18032
                mov     byte_1DC7E, 0
                call    evt_check_cell_triggers

loc_1802B:                              ; CODE XREF: game_main_loop+A6↑j
                                        ; game_main_loop+17D↑j ...
                cmp     byte_23218, 80h
                jnb     short loc_1801C

loc_18032:                              ; CODE XREF: game_main_loop+211↑j
                cmp     byte_1DC84, 0FEh
                jnz     short loc_18040
                inc     byte_1DC84
                call    thk_res_3814

loc_18040:                              ; CODE XREF: game_main_loop+227↑j
                mov     [bp+var_2], 0
                mov     byte ptr g_party_y+1, 0
                mov     word_238A0, 0
                call    thk_party_count_able
                or      ax, ax
                jnz     short loc_18059
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18059:                              ; CODE XREF: game_main_loop+244↑j
                mov     byte_1DBEB, 0
                mov     si, [bp+var_C]

loc_18061:                              ; CODE XREF: game_main_loop+26D↓j
                cmp     g_outdoors, 0
                jnz     short loc_18076
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    view_idle_step
                jmp     short loc_18079
; ---------------------------------------------------------------------------

loc_18076:                              ; CODE XREF: game_main_loop+256↑j
                call    thk_kbd_poll

loc_18079:                              ; CODE XREF: game_main_loop+264↑j
                mov     si, ax
                or      si, si
                jz      short loc_18061
                mov     [bp+var_C], si
                push    si
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_C], ax
                cmp     byte_1DC80, 0
                jz      short loc_18096
                call    evt_finish

loc_18096:                              ; CODE XREF: game_main_loop+281↑j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, [bp+var_C]
                cmp     ax, 4Fh ; 'O'
                jnz     short loc_180AB
                jmp     loc_18190
; ---------------------------------------------------------------------------

loc_180AB:                              ; CODE XREF: game_main_loop+296↑j
                jle     short loc_180B0
                jmp     loc_181F4
; ---------------------------------------------------------------------------

loc_180B0:                              ; CODE XREF: game_main_loop:loc_180AB↑j
                cmp     ax, 11h
                jz      short loc_180E0
                cmp     ax, 42h ; 'B'
                jnz     short loc_180BD
                jmp     loc_18166
; ---------------------------------------------------------------------------

loc_180BD:                              ; CODE XREF: game_main_loop+2A8↑j
                cmp     ax, 43h ; 'C'
                jnz     short loc_180C5
                jmp     loc_1816C
; ---------------------------------------------------------------------------

loc_180C5:                              ; CODE XREF: game_main_loop+2B0↑j
                cmp     ax, 44h ; 'D'
                jnz     short loc_180CD
                jmp     loc_18176
; ---------------------------------------------------------------------------

loc_180CD:                              ; CODE XREF: game_main_loop+2B8↑j
                cmp     ax, 45h ; 'E'
                jnz     short loc_180D5
                jmp     loc_1817C
; ---------------------------------------------------------------------------

loc_180D5:                              ; CODE XREF: game_main_loop+2C0↑j
                cmp     ax, 4Dh ; 'M'
                jnz     short loc_180DD
                jmp     loc_18186
; ---------------------------------------------------------------------------

loc_180DD:                              ; CODE XREF: game_main_loop+2C8↑j
                jmp     loc_18205
; ---------------------------------------------------------------------------

loc_180E0:                              ; CODE XREF: game_main_loop+2A3↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_print_gold_label
                mov     ax, offset aQuitToDosWitho ; "Quit to DOS without game save (y/n)?"
                push    ax
                call    thk_print_message_line
                add     sp, 2

loc_180F6:                              ; CODE XREF: game_main_loop+311↓j
                mov     ax, 79h ; 'y'
                push    ax
                mov     ax, 4Eh ; 'N'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_18117
                mov     si, 4Eh ; 'N'

loc_18117:                              ; CODE XREF: game_main_loop+302↑j
                mov     ax, si
                cmp     ax, 59h ; 'Y'
                jz      short loc_18123
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_180F6

loc_18123:                              ; CODE XREF: game_main_loop+30C↑j
                mov     [bp+var_C], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_1812E
                call    thk_quit_to_dos

loc_1812E:                              ; CODE XREF: game_main_loop+319↑j
                call    thk_text_clear_prompt_line
                call    thk_draw_status_line
                mov     byte_1DBEA, 0
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1813C:                              ; CODE XREF: game_main_loop+43A↓j
                call    thk_res_3FE2
                push    [bp+var_C]
                call    thk_party_turn_key
                add     sp, 2
                mov     byte_1DC7E, 1
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18150:                              ; CODE XREF: game_main_loop+447↓j
                push    [bp+var_C]
                call    thk_party_move_key
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_advance_time

loc_18160:                              ; CODE XREF: game_main_loop+39D↓j
                                        ; game_main_loop+3C1↓j
                add     sp, 2
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18166:                              ; CODE XREF: game_main_loop+2AA↑j
                call    thk_2MISC_C130
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1816C:                              ; CODE XREF: game_main_loop+2B2↑j
                call    thk_res_3FE2
                call    thk_2MISC2_C3F6
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18176:                              ; CODE XREF: game_main_loop+2BA↑j
                call    thk_2MISC2_C130
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1817C:                              ; CODE XREF: game_main_loop+2C2↑j
                call    thk_res_3FE2
                call    thk_2MISC2_C2F8
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18186:                              ; CODE XREF: game_main_loop+2CA↑j
                call    thk_res_3FE2
                call    sub_1BB4E
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18190:                              ; CODE XREF: game_main_loop+298↑j
                cmp     g_view_mode, 1
                jz      short loc_1819A
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1819A:                              ; CODE XREF: game_main_loop+385↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_res_471E

loc_181A6:                              ; CODE XREF: game_main_loop+3B6↓j
                                        ; game_main_loop+41E↓j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                jmp     short loc_18160
; ---------------------------------------------------------------------------
                align 2

loc_181B0:                              ; CODE XREF: game_main_loop+3EE↓j
                cmp     g_view_mode, 0
                jz      short loc_181BA
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_181BA:                              ; CODE XREF: game_main_loop+3A5↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_res_47D8
                jmp     short loc_181A6
; ---------------------------------------------------------------------------

loc_181C8:                              ; CODE XREF: game_main_loop+3F3↓j
                call    thk_res_3FE2
                push    [bp+var_C]
                call    thk_party_status_loop
                jmp     short loc_18160
; ---------------------------------------------------------------------------
                align 2

loc_181D4:                              ; CODE XREF: game_main_loop+3E7↓j
                call    thk_2MISC_CF84
                call    thk_res_40E6
                mov     byte_1DC7E, 1
                mov     byte_1DBEA, 1
                jmp     short loc_1825C
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_181E8:                              ; CODE XREF: game_main_loop+42C↓j
                call    thk_res_3814
                jmp     short loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_181EE:                              ; CODE XREF: game_main_loop+425↓j
                call    thk_2MISC_C242
                jmp     short loc_1825C ; CODE XREF: seg002:01B9↑J
; ---------------------------------------------------------------------------
                align 2

loc_181F4:                              ; CODE XREF: game_main_loop+29D↑j
                cmp     ax, 52h ; 'R'
                jz      short loc_181D4
                jg      short loc_18232
                cmp     ax, 50h ; 'P'
                jz      short loc_181B0
                cmp     ax, 51h ; 'Q'
                jz      short loc_181C8

loc_18205:                              ; CODE XREF: game_main_loop:loc_180DD↑j
                                        ; game_main_loop+42E↓j ...
                call    thk_res_3FE2
                cmp     [bp+var_C], 31h ; '1'
                jb      short loc_1825C
                mov     ax, g_party_size
                add     ax, 30h ; '0'
                cmp     [bp+var_C], ax
                ja      short loc_1825C
                push    [bp+var_C]
                call    thk_party_status_loop
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_draw_party_list
                jmp     loc_181A6
; ---------------------------------------------------------------------------
                align 2

loc_18232:                              ; CODE XREF: game_main_loop+3E9↑j
                cmp     ax, 55h ; 'U'
                jz      short loc_181EE
                jg      short loc_18240
                cmp     ax, 53h ; 'S'
                jz      short loc_181E8
                jmp     short loc_18205
; ---------------------------------------------------------------------------

loc_18240:                              ; CODE XREF: game_main_loop+427↑j
                cmp     ax, 0F0h
                jl      short loc_18205
                cmp     ax, 0F1h
                jg      short loc_1824D
                jmp     loc_1813C
; ---------------------------------------------------------------------------

loc_1824D:                              ; CODE XREF: game_main_loop+438↑j
                cmp     ax, 0F2h
                jl      short loc_18205
                cmp     ax, 0F3h
                jg      short loc_1825A
                jmp     loc_18150
; ---------------------------------------------------------------------------

loc_1825A:                              ; CODE XREF: game_main_loop+445↑j
                jmp     short loc_18205
; ---------------------------------------------------------------------------

loc_1825C:                              ; CODE XREF: game_main_loop+246↑j
                                        ; game_main_loop+329↑j ...
                call    thk_party_count_able
                or      ax, ax
                jz      short loc_18272
                cmp     word_238A0, 0
                jz      short loc_18272
                mov     byte_1DC7E, 1
                call    thk_res_3FE2

loc_18272:                              ; CODE XREF: game_main_loop+451↑j
                                        ; game_main_loop+458↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1827B
                jmp     loc_17E26
; ---------------------------------------------------------------------------

loc_1827B:                              ; CODE XREF: game_main_loop+466↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
game_main_loop  endp


; =============== S U B R O U T I N E =======================================

; animate + poll key
; Attributes: bp-based frame

view_idle_step  proc near               ; CODE XREF: seg002:0609↑J
                                        ; game_main_loop+261↑p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    word_1ED84
                inc     word_1ED84
                call    view_draw_sprite_list
                add     sp, 2
                cmp     word_1ED84, 4
                jnz     short loc_182A3
                mov     word_1ED84, 1

loc_182A3:                              ; CODE XREF: view_idle_step+19↑j
                mov     ax, 8
                push    ax
                call    thk_wait_key_timeout
                mov     [bp+var_2], ax
                mov     sp, bp
                pop     bp
                retn
view_idle_step  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; draws queued view sprites (arrays DGROUP:6014/6028/603C)
; Attributes: bp-based frame

view_draw_sprite_list proc near         ; CODE XREF: view_idle_step+E↑p
                                        ; draw_view_indoors+26C↓p

var_8           = word ptr -8
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                cmp     word_1DBF0, 0
                jl      short loc_18308
                mov     [bp+var_2], 0
                cmp     word_1DBF0, 0
                jl      short loc_18308
                mov     si, 603Ch
                mov     di, 6028h
                mov     [bp+var_8], 6014h

loc_182D8:                              ; CODE XREF: view_draw_sprite_list+54↓j
                push    word ptr [si]
                push    word ptr [di]
                mov     bx, [bp+var_8]
                mov     ax, [bx]
                add     ax, [bp+arg_0]
                push    ax
                push    word_1DBB8
                push    word_1DBB6
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                add     si, 2
                add     di, 2
                add     [bp+var_8], 2
                inc     [bp+var_2]
                mov     ax, word_1DBF0
                cmp     [bp+var_2], ax
                jle     short loc_182D8

loc_18308:                              ; CODE XREF: view_draw_sprite_list+D↑j
                                        ; view_draw_sprite_list+19↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_draw_sprite_list endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_queue_sprite_a proc near           ; CODE XREF: view_draw_wall_a+1D↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                mov     bl, [bp+arg_0]
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+1516h]
                sub     ax, 8
                mov     [bp+var_2], ax
                cmp     byte_1DBEC, 2
                jz      short loc_18335
                cmp     byte_1DBEC, 5
                jnz     short loc_1833F

loc_18335:                              ; CODE XREF: view_queue_sprite_a+1E↑j
                cmp     [bp+arg_0], 0
                jnz     short loc_1833F
                sub     [bp+var_2], 8

loc_1833F:                              ; CODE XREF: view_queue_sprite_a+25↑j
                                        ; view_queue_sprite_a+2B↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                inc     word_1DBF0
                mov     bx, word_1DBF0
                shl     bx, 1
                shl     ax, 1
                shl     ax, 1
                add     ax, 18h
                mov     [bx+6014h], ax
                mov     di, word_1DBF0
                shl     di, 1
                mov     ax, [bp+var_2]
                mov     [di+6028h], ax
                mov     bx, si
                shl     bx, 1
                mov     ax, [bx+151Ch]
                mov     [di+603Ch], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_queue_sprite_a endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_queue_sprite_b proc near           ; CODE XREF: view_draw_wall_b+2F↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                mov     bl, [bp+arg_0]
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+1522h]
                mov     [bp+var_2], ax
                cmp     byte_1DBEC, 2
                jz      short loc_1839E
                cmp     byte_1DBEC, 5
                jnz     short loc_183A8

loc_1839E:                              ; CODE XREF: view_queue_sprite_b+1B↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_183A8
                add     [bp+var_2], 8

loc_183A8:                              ; CODE XREF: view_queue_sprite_b+22↑j
                                        ; view_queue_sprite_b+28↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                inc     word_1DBF0
                mov     bx, word_1DBF0
                shl     bx, 1
                shl     ax, 1
                shl     ax, 1
                mov     [bx+6014h], ax
                mov     di, word_1DBF0
                shl     di, 1
                mov     ax, [bp+var_2]
                mov     [di+6028h], ax
                mov     bx, si
                shl     bx, 1
                mov     ax, [bx+1528h]
                mov     [di+603Ch], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_queue_sprite_b endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_queue_sprite_c proc near           ; CODE XREF: view_draw_wall_c+2F↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                inc     word_1DBF0
                mov     bx, word_1DBF0
                shl     bx, 1
                shl     ax, 1
                shl     ax, 1
                add     ax, 0Ch
                mov     [bx+6014h], ax
                mov     di, word_1DBF0
                shl     di, 1
                mov     ax, si
                shl     ax, 1
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     ax, 0C8h
                sub     ax, [bx+1522h]
                mov     [di+6028h], ax
                mov     ax, [bx+1528h]
                mov     [di+603Ch], ax
                cmp     [bp+arg_0], 2
                jnz     short loc_18438
                cmp     byte_1DBEC, 0
                jnz     short loc_18438
                sub     word ptr [di+603Ch], 2

loc_18438:                              ; CODE XREF: view_queue_sprite_c+4A↑j
                                        ; view_queue_sprite_c+51↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_queue_sprite_c endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_queue_sprite_d proc near           ; CODE XREF: view_draw_wall_b+3A↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                cmp     [bp+arg_0], 0
                jz      short loc_18489
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                inc     word_1DBF0
                mov     bx, word_1DBF0
                shl     bx, 1
                shl     ax, 1
                shl     ax, 1
                add     ax, 18h
                mov     [bx+6014h], ax
                mov     di, si
                shl     di, 1
                mov     ax, word_1DBF0
                shl     ax, 1
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     ax, [di+152Eh]
                sub     ax, 8
                mov     [bx+6028h], ax
                mov     ax, [di+151Ch]
                mov     [bx+603Ch], ax

loc_18489:                              ; CODE XREF: view_queue_sprite_d+C↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_184A1
                cmp     byte_1DBEC, 0
                jnz     short loc_184A1
                mov     bx, word_1DBF0
                shl     bx, 1
                sub     word ptr [bx+603Ch], 2

loc_184A1:                              ; CODE XREF: view_queue_sprite_d+4F↑j
                                        ; view_queue_sprite_d+56↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_queue_sprite_d endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_queue_sprite_e proc near           ; CODE XREF: view_draw_wall_c+3A↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                cmp     [bp+arg_0], 0
                jz      short loc_1850A
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                inc     word_1DBF0
                mov     bx, word_1DBF0
                shl     bx, 1
                shl     ax, 1
                shl     ax, 1
                add     ax, 18h
                mov     [bx+6014h], ax
                mov     di, si
                shl     di, 1
                mov     ax, word_1DBF0
                shl     ax, 1
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     ax, 0D0h
                sub     ax, [di+152Eh]
                mov     [bx+6028h], ax
                mov     ax, [di+151Ch]
                mov     [bx+603Ch], ax
                cmp     [bp+arg_0], 2
                jz      short loc_184FF
                cmp     [bp+arg_0], 1
                jnz     short loc_1850A

loc_184FF:                              ; CODE XREF: view_queue_sprite_e+4F↑j
                mov     bx, word_1DBF0
                shl     bx, 1
                add     word ptr [bx+6028h], 8

loc_1850A:                              ; CODE XREF: view_queue_sprite_e+C↑j
                                        ; view_queue_sprite_e+55↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_queue_sprite_e endp


; =============== S U B R O U T I N E =======================================

; facing-dependent test on map bytes at DGROUP:59A6
; Attributes: bp-based frame

view_indoor_daylight proc near          ; CODE XREF: seg002:0771↑J
                                        ; draw_view_indoors+2F↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                cmp     g_party_x, 7
                jbe     short loc_18524
                mov     si, 1
                jmp     short loc_18526
; ---------------------------------------------------------------------------
                align 2

loc_18524:                              ; CODE XREF: view_indoor_daylight+C↑j
                sub     si, si

loc_18526:                              ; CODE XREF: view_indoor_daylight+11↑j
                mov     bl, byte ptr g_party_y
                sub     bh, bh
                shl     bx, 1
                mov     al, [bx+si+59A6h]
                mov     [bp+var_2], al
                mov     bl, g_party_x
                and     bx, 7
                mov     al, [bx+1536h]
                sub     ah, ah
                mov     cl, [bp+var_2]
                sub     ch, ch
                test    ax, cx
                jz      short loc_18550
                mov     ax, 1
                jmp     short loc_18552
; ---------------------------------------------------------------------------

loc_18550:                              ; CODE XREF: view_indoor_daylight+39↑j
                sub     ax, ax

loc_18552:                              ; CODE XREF: view_indoor_daylight+3E↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
view_indoor_daylight endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_draw_wall_a proc near              ; CODE XREF: draw_view_indoors+10A↓p
                                        ; draw_view_indoors+17D↓p ...

arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                push    si
                dec     [bp+arg_2]
                cmp     [bp+arg_2], 3
                jz      short loc_1857B
                cmp     [bp+arg_0], 3
                jnz     short loc_1857B
                mov     [bp+arg_0], 1
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    view_queue_sprite_a
                add     sp, 2

loc_1857B:                              ; CODE XREF: view_draw_wall_a+B↑j
                                        ; view_draw_wall_a+11↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_18588
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_1858B
; ---------------------------------------------------------------------------

loc_18588:                              ; CODE XREF: view_draw_wall_a+27↑j
                mov     al, [bp+arg_2]

loc_1858B:                              ; CODE XREF: view_draw_wall_a+2E↑j
                mov     [bp+arg_0], al
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                push    word ptr [si+1546h]
                push    word ptr [si+153Eh]
                mov     al, [bp+arg_0]
                push    ax
                push    word_1DBD0
                push    word_1DBCE
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                pop     si
                pop     bp
                retn
view_draw_wall_a endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_draw_wall_b proc near              ; CODE XREF: draw_view_indoors+AE↓p
                                        ; draw_view_indoors+C5↓p ...

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                dec     [bp+arg_2]
                mov     al, [bp+arg_2]
                mov     [bp+var_2], al
                and     [bp+arg_2], 7Fh
                cmp     [bp+arg_2], 3
                jz      short loc_185F4
                cmp     [bp+arg_0], 3
                jnz     short loc_185F4
                mov     [bp+arg_0], 1
                cmp     al, 80h
                jnb     short loc_185E8
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    view_queue_sprite_b
                jmp     short loc_185F1
; ---------------------------------------------------------------------------

loc_185E8:                              ; CODE XREF: view_draw_wall_b+27↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    view_queue_sprite_d

loc_185F1:                              ; CODE XREF: view_draw_wall_b+32↑j
                add     sp, 2

loc_185F4:                              ; CODE XREF: view_draw_wall_b+19↑j
                                        ; view_draw_wall_b+1F↑j
                cmp     [bp+var_2], 80h
                jnb     short loc_1862A
                cmp     [bp+arg_0], 2
                jnz     short loc_18608
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_1860B
; ---------------------------------------------------------------------------
                align 2

loc_18608:                              ; CODE XREF: view_draw_wall_b+4A↑j
                mov     al, [bp+arg_2]

loc_1860B:                              ; CODE XREF: view_draw_wall_b+51↑j
                mov     [bp+arg_0], al
                add     al, 4
                mov     [bp+var_4], al
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                mov     ax, [si+1552h]
                mov     [bp+var_6], ax
                mov     ax, [si+155Ah]
                jmp     short loc_1865A
; ---------------------------------------------------------------------------
                align 2

loc_1862A:                              ; CODE XREF: view_draw_wall_b+44↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_18636
                mov     [bp+arg_0], 10h
                jmp     short loc_1863A
; ---------------------------------------------------------------------------

loc_18636:                              ; CODE XREF: view_draw_wall_b+7A↑j
                mov     [bp+arg_0], 0

loc_1863A:                              ; CODE XREF: view_draw_wall_b+80↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+154Eh]
                add     al, [bp+arg_0]
                mov     [bp+var_4], al
                mov     di, si
                shl     di, 1
                mov     ax, [di+1562h]
                mov     [bp+var_6], ax
                mov     ax, [di+156Ah]

loc_1865A:                              ; CODE XREF: view_draw_wall_b+73↑j
                mov     [bp+var_8], ax
                push    ax
                push    [bp+var_6]
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    word_1DBD0
                push    word_1DBCE
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_draw_wall_b endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_draw_wall_c proc near              ; CODE XREF: draw_view_indoors+DC↓p
                                        ; draw_view_indoors+F3↓p ...

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                dec     [bp+arg_2]
                mov     al, [bp+arg_2]
                mov     [bp+var_2], al
                and     [bp+arg_2], 7Fh
                cmp     [bp+arg_2], 3
                jz      short loc_186BC ; CODE XREF: seg002:0561↑J
                cmp     [bp+arg_0], 3
                jnz     short loc_186BC
                mov     [bp+arg_0], 1
                cmp     al, 80h
                jnb     short loc_186B0
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    view_queue_sprite_c
                jmp     short loc_186B9
; ---------------------------------------------------------------------------

loc_186B0:                              ; CODE XREF: view_draw_wall_c+27↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    view_queue_sprite_e

loc_186B9:                              ; CODE XREF: view_draw_wall_c+32↑j
                add     sp, 2

loc_186BC:                              ; CODE XREF: view_draw_wall_c+19↑j
                                        ; view_draw_wall_c+1F↑j
                cmp     [bp+var_2], 80h
                jnb     short loc_186F2
                cmp     [bp+arg_0], 2
                jnz     short loc_186D0
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_186D3
; ---------------------------------------------------------------------------
                align 2

loc_186D0:                              ; CODE XREF: view_draw_wall_c+4A↑j
                mov     al, [bp+arg_2]

loc_186D3:                              ; CODE XREF: view_draw_wall_c+51↑j
                mov     [bp+arg_0], al
                add     al, 8
                mov     [bp+var_4], al
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                mov     ax, [si+1576h]
                mov     [bp+var_6], ax
                mov     ax, [si+157Eh]
                jmp     short loc_18722
; ---------------------------------------------------------------------------
                align 2

loc_186F2:                              ; CODE XREF: view_draw_wall_c+44↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_186FE
                mov     [bp+arg_0], 10h
                jmp     short loc_18702
; ---------------------------------------------------------------------------

loc_186FE:                              ; CODE XREF: view_draw_wall_c+7A↑j
                mov     [bp+arg_0], 0

loc_18702:                              ; CODE XREF: view_draw_wall_c+80↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                mov     si, ax
                mov     al, [si+1572h]
                add     al, [bp+arg_0]
                mov     [bp+var_4], al
                mov     di, si
                shl     di, 1
                mov     ax, [di+1586h]
                mov     [bp+var_6], ax
                mov     ax, [di+158Eh]

loc_18722:                              ; CODE XREF: view_draw_wall_c+73↑j
                mov     [bp+var_8], ax
                push    ax
                push    [bp+var_6]
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    word_1DBD0
                push    word_1DBCE
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_draw_wall_c endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; 3D maze view for towns/dungeons (g_outdoors == 0)
; Attributes: bp-based frame

draw_view_indoors proc near             ; CODE XREF: seg002:0765↑J

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     word_1DBF0, 0FFFFh
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 44h ; 'D'
                push    ax
                mov     ax, 8
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBB4
                push    word_1DBB2
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                call    view_indoor_daylight
                mov     [bp+var_2], ax
                mov     ax, 8
                push    ax
                push    ax
                push    [bp+var_2]
                push    word_1DBD4
                push    word_1DBD2
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                cmp     g_day_fraction, 80h
                jl      short loc_187A0
                cmp     [bp+var_2], 0
                jnz     short loc_187A0
                call    thk_res_4FB2

loc_187A0:                              ; CODE XREF: draw_view_indoors+51↑j
                                        ; draw_view_indoors+57↑j
                sub     al, al
                mov     byte_2782E, al
                mov     byte_2782D, al
                mov     byte_2782C, al
                mov     byte_2782B, al
                mov     byte_2782A, al
                mov     byte_27829, al
                mov     byte_27828, al
                mov     byte_27827, al
                mov     byte_27826, al
                mov     byte_27839, al
                mov     byte_27838, al
                mov     byte_27837, al
                mov     byte_27836, al
                mov     byte_27835, al
                mov     byte_27834, al
                mov     byte_27833, al
                mov     byte_27832, al
                mov     byte_27831, al
                mov     byte_27830, al
                mov     byte_2782F, al
                call    sub_1BEBA
                cmp     byte_27826, 0
                jz      short loc_187F8
                mov     ax, 4
                push    ax
                mov     al, byte_27826
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_187F8:                              ; CODE XREF: draw_view_indoors+A2↑j
                cmp     byte_27836, 0
                jz      short loc_1880F
                mov     ax, 84h
                push    ax
                mov     al, byte_27836
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_1880F:                              ; CODE XREF: draw_view_indoors+B9↑j
                cmp     byte_2782A, 0
                jz      short loc_18826
                mov     ax, 4
                push    ax
                mov     al, byte_2782A
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_18826:                              ; CODE XREF: draw_view_indoors+D0↑j
                cmp     byte_27832, 0
                jz      short loc_1883D
                mov     ax, 84h
                push    ax
                mov     al, byte_27832
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_1883D:                              ; CODE XREF: draw_view_indoors+E7↑j
                cmp     byte_2782E, 0
                jz      short loc_18854
                mov     ax, 4
                push    ax
                mov     al, byte_2782E
                sub     ah, ah
                push    ax
                call    view_draw_wall_a
                add     sp, 4

loc_18854:                              ; CODE XREF: draw_view_indoors+FE↑j
                cmp     byte_27839, 0
                jz      short loc_1886B
                mov     ax, 3
                push    ax
                mov     al, byte_27839
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_1886B:                              ; CODE XREF: draw_view_indoors+115↑j
                cmp     byte_27835, 0
                jz      short loc_18882
                mov     ax, 83h
                push    ax
                mov     al, byte_27835
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_18882:                              ; CODE XREF: draw_view_indoors+12C↑j
                cmp     byte_27829, 0
                jz      short loc_18899
                mov     ax, 3
                push    ax
                mov     al, byte_27829
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_18899:                              ; CODE XREF: draw_view_indoors+143↑j
                cmp     byte_27831, 0
                jz      short loc_188B0
                mov     ax, 83h
                push    ax
                mov     al, byte_27831
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_188B0:                              ; CODE XREF: draw_view_indoors+15A↑j
                cmp     byte_2782D, 0
                jz      short loc_188C7
                mov     ax, 3
                push    ax
                mov     al, byte_2782D
                sub     ah, ah
                push    ax
                call    view_draw_wall_a
                add     sp, 4

loc_188C7:                              ; CODE XREF: draw_view_indoors+171↑j
                cmp     byte_27838, 0
                jz      short loc_188DE
                mov     ax, 2
                push    ax
                mov     al, byte_27838
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_188DE:                              ; CODE XREF: draw_view_indoors+188↑j
                cmp     byte_27834, 0
                jz      short loc_188F5
                mov     ax, 82h
                push    ax
                mov     al, byte_27834
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_188F5:                              ; CODE XREF: draw_view_indoors+19F↑j
                cmp     byte_27828, 0
                jz      short loc_1890C
                mov     ax, 2
                push    ax
                mov     al, byte_27828
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_1890C:                              ; CODE XREF: draw_view_indoors+1B6↑j
                cmp     byte_27830, 0
                jz      short loc_18923
                mov     ax, 82h
                push    ax
                mov     al, byte_27830
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_18923:                              ; CODE XREF: draw_view_indoors+1CD↑j
                cmp     byte_2782C, 0
                jz      short loc_1893A
                mov     ax, 2
                push    ax
                mov     al, byte_2782C
                sub     ah, ah
                push    ax
                call    view_draw_wall_a
                add     sp, 4

loc_1893A:                              ; CODE XREF: draw_view_indoors+1E4↑j
                cmp     byte_27837, 0
                jz      short loc_18951
                mov     ax, 1
                push    ax
                mov     al, byte_27837
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_18951:                              ; CODE XREF: draw_view_indoors+1FB↑j
                cmp     byte_27833, 0
                jz      short loc_18968
                mov     ax, 81h
                push    ax
                mov     al, byte_27833
                sub     ah, ah
                push    ax
                call    view_draw_wall_b
                add     sp, 4

loc_18968:                              ; CODE XREF: draw_view_indoors+212↑j
                cmp     byte_27827, 0
                jz      short loc_1897F
                mov     ax, 1
                push    ax
                mov     al, byte_27827
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_1897F:                              ; CODE XREF: draw_view_indoors+229↑j
                cmp     byte_2782F, 0
                jz      short loc_18996
                mov     ax, 81h
                push    ax
                mov     al, byte_2782F
                sub     ah, ah
                push    ax
                call    view_draw_wall_c
                add     sp, 4

loc_18996:                              ; CODE XREF: draw_view_indoors+240↑j
                cmp     byte_2782B, 0
                jz      short loc_189AD
                mov     ax, 1
                push    ax
                mov     al, byte_2782B
                sub     ah, ah
                push    ax
                call    view_draw_wall_a
                add     sp, 4

loc_189AD:                              ; CODE XREF: draw_view_indoors+257↑j
                sub     ax, ax
                push    ax
                call    view_draw_sprite_list
                mov     sp, bp
                pop     bp
                retn
draw_view_indoors endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_189B8       proc near               ; CODE XREF: draw_view_outdoors:loc_18DB6↓p

var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                sub     si, si
                mov     [bp+var_C], 159Eh
                mov     [bp+var_E], 1596h

loc_189CC:                              ; CODE XREF: sub_189B8+109↓j
                mov     bx, [bp+var_C]
                mov     ax, 80h
                sub     ax, [bx]
                mov     di, ax
                cmp     byte ptr [si+5FD8h], 3
                jbe     short loc_189E2
                mov     ax, 1
                jmp     short loc_189E4
; ---------------------------------------------------------------------------

loc_189E2:                              ; CODE XREF: sub_189B8+23↑j
                sub     ax, ax

loc_189E4:                              ; CODE XREF: sub_189B8+28↑j
                mov     [bp+var_A], ax
                cmp     byte ptr [si+5FDCh], 3
                jbe     short loc_189F4
                mov     ax, 1
                jmp     short loc_189F6
; ---------------------------------------------------------------------------
                align 2

loc_189F4:                              ; CODE XREF: sub_189B8+34↑j
                sub     ax, ax

loc_189F6:                              ; CODE XREF: sub_189B8+39↑j
                mov     [bp+var_2], ax
                cmp     byte ptr [si+5FE0h], 3
                jbe     short loc_18A06
                mov     ax, 1
                jmp     short loc_18A08
; ---------------------------------------------------------------------------
                align 2

loc_18A06:                              ; CODE XREF: sub_189B8+46↑j
                sub     ax, ax

loc_18A08:                              ; CODE XREF: sub_189B8+4B↑j
                mov     [bp+var_6], ax
                cmp     [bp+var_A], 0
                jz      short loc_18A58
                mov     byte ptr [si+5FD8h], 0
                push    di
                mov     ax, 8
                push    ax
                push    si
                push    word_1DBCC
                push    word_1DBCA
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                cmp     [bp+var_2], 0
                jz      short loc_18A47
                push    di
                mov     ax, 8
                push    ax
                lea     ax, [si+0Ch]
                push    ax
                push    word_1DBCC
                push    word_1DBCA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18A47:                              ; CODE XREF: sub_189B8+76↑j
                cmp     [bp+var_6], 0
                jz      short loc_18A92
                push    di
                mov     bx, [bp+var_E]
                push    word ptr [bx]
                lea     ax, [si+10h]
                jmp     short loc_18A83
; ---------------------------------------------------------------------------

loc_18A58:                              ; CODE XREF: sub_189B8+57↑j
                cmp     [bp+var_2], 0
                jz      short loc_18A75
                push    di
                mov     ax, 8
                push    ax
                lea     ax, [si+4]
                push    ax
                push    word_1DBCC
                push    word_1DBCA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18A75:                              ; CODE XREF: sub_189B8+A4↑j
                cmp     [bp+var_6], 0
                jz      short loc_18A92
                push    di
                mov     ax, 70h ; 'p'
                push    ax
                lea     ax, [si+8]

loc_18A83:                              ; CODE XREF: sub_189B8+9E↑j
                push    ax
                push    word_1DBCC
                push    word_1DBCA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18A92:                              ; CODE XREF: sub_189B8+93↑j
                                        ; sub_189B8+C1↑j
                cmp     [bp+var_A], 0
                jz      short loc_18A9D
                mov     byte ptr [si+5FD8h], 0

loc_18A9D:                              ; CODE XREF: sub_189B8+DE↑j
                cmp     [bp+var_2], 0
                jz      short loc_18AA8
                mov     byte ptr [si+5FDCh], 0

loc_18AA8:                              ; CODE XREF: sub_189B8+E9↑j
                cmp     [bp+var_6], 0
                jz      short loc_18AB3
                mov     byte ptr [si+5FE0h], 0

loc_18AB3:                              ; CODE XREF: sub_189B8+F4↑j
                add     [bp+var_C], 2
                add     [bp+var_E], 2
                inc     si
                cmp     si, 4
                jge     short loc_18AC4
                jmp     loc_189CC
; ---------------------------------------------------------------------------

loc_18AC4:                              ; CODE XREF: sub_189B8+107↑j
                mov     [bp+var_4], di
                mov     [bp+var_8], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_189B8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18AD0       proc near               ; CODE XREF: sub_18CC6+61↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B0h], 0FFh
                jz      short loc_18B09
                mov     si, bx
                shl     si, 1
                push    word ptr [si+15B2h]
                push    word ptr [si+15AAh]
                mov     al, [bx+15A6h]
                sub     ah, ah
                push    ax
                mov     bl, [bx+54B0h]  ; CODE XREF: seg002:03F9↑J
                sub     bh, bh
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+370h]
                push    word ptr [bx+36Eh]
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18B09:                              ; CODE XREF: sub_18AD0+C↑j
                pop     si
                pop     bp
                retn
sub_18AD0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18B0C       proc near               ; CODE XREF: sub_18CC6+77↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                cmp     [bp+arg_0], 0
                jz      short loc_18B30
                mov     ax, [bp+arg_2]
                cmp     [bp+arg_0], ax
                jnz     short loc_18B30
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B0h], 0FFh
                jz      short loc_18B30
                mov     ax, 1
                jmp     short loc_18B32
; ---------------------------------------------------------------------------

loc_18B30:                              ; CODE XREF: sub_18B0C+B↑j
                                        ; sub_18B0C+13↑j ...
                sub     ax, ax

loc_18B32:                              ; CODE XREF: sub_18B0C+22↑j
                mov     [bp+var_8], ax
                or      ax, ax
                jz      short loc_18B48
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B3h], 0FFh
                jz      short loc_18B48
                mov     byte ptr [bx+54B4h], 0FFh

loc_18B48:                              ; CODE XREF: sub_18B0C+2B↑j
                                        ; sub_18B0C+35↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B4h], 0FFh
                jnz     short loc_18B55
                jmp     loc_18BE6
; ---------------------------------------------------------------------------

loc_18B55:                              ; CODE XREF: sub_18B0C+44↑j
                mov     si, bx
                shl     si, 1
                mov     ax, [si+15BAh]
                mov     [bp+var_2], ax
                mov     ax, [si+15C2h]
                mov     [bp+var_4], ax
                mov     ax, [si+15CAh]
                mov     [bp+var_6], ax
                cmp     [bp+var_8], 0
                jz      short loc_18B97
                mov     bx, [bp+arg_2]
                cmp     byte ptr [bx+54B4h], 0FFh
                jz      short loc_18B97
                mov     si, bx
                shl     si, 1
                mov     ax, [si+15B8h]
                mov     [bp+var_2], ax
                mov     ax, [si+15C0h]
                mov     [bp+var_4], ax
                mov     ax, [si+15B2h]
                mov     [bp+var_6], ax

loc_18B97:                              ; CODE XREF: sub_18B0C+66↑j
                                        ; sub_18B0C+70↑j
                cmp     [bp+arg_0], 1
                jnz     short loc_18BA9
                cmp     byte_22D04, 0FFh
                jnz     short loc_18BA9
                mov     [bp+var_4], 8

loc_18BA9:                              ; CODE XREF: sub_18B0C+8F↑j
                                        ; sub_18B0C+96↑j
                cmp     [bp+arg_0], 1
                jz      short loc_18BBD
                cmp     [bp+arg_0], 2
                jnz     short loc_18BC2
                mov     ax, [bp+arg_2]
                cmp     [bp+arg_0], ax
                jnz     short loc_18BC2

loc_18BBD:                              ; CODE XREF: sub_18B0C+A1↑j
                mov     [bp+var_4], 8

loc_18BC2:                              ; CODE XREF: sub_18B0C+A7↑j
                                        ; sub_18B0C+AF↑j
                push    [bp+var_6]
                push    [bp+var_4]
                push    [bp+var_2]
                mov     bx, [bp+arg_0]
                mov     bl, [bx+54B4h]
                sub     bh, bh
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+370h]
                push    word ptr [bx+36Eh]
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18BE6:                              ; CODE XREF: sub_18B0C+46↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_18B0C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18BEC       proc near               ; CODE XREF: sub_18CC6+93↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                cmp     [bp+arg_0], 0
                jz      short loc_18C10
                mov     ax, [bp+arg_2]
                cmp     [bp+arg_0], ax
                jnz     short loc_18C10
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B0h], 0FFh
                jz      short loc_18C10
                mov     ax, 1
                jmp     short loc_18C12
; ---------------------------------------------------------------------------

loc_18C10:                              ; CODE XREF: sub_18BEC+B↑j
                                        ; sub_18BEC+13↑j ...
                sub     ax, ax

loc_18C12:                              ; CODE XREF: sub_18BEC+22↑j
                mov     [bp+var_8], ax
                or      ax, ax
                jz      short loc_18C28
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B7h], 0FFh
                jz      short loc_18C28
                mov     byte ptr [bx+54B8h], 0FFh

loc_18C28:                              ; CODE XREF: sub_18BEC+2B↑j
                                        ; sub_18BEC+35↑j
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+54B8h], 0FFh
                jnz     short loc_18C35
                jmp     loc_18CC1
; ---------------------------------------------------------------------------

loc_18C35:                              ; CODE XREF: sub_18BEC+44↑j
                mov     al, [bx+15D2h]
                sub     ah, ah
                mov     [bp+var_2], ax
                mov     si, bx
                shl     si, 1
                mov     ax, [si+15D6h]
                mov     [bp+var_4], ax
                mov     ax, [si+15DEh]
                mov     [bp+var_6], ax
                cmp     [bp+var_8], 0
                jz      short loc_18C7B
                mov     bx, [bp+arg_2]
                cmp     byte ptr [bx+54B8h], 0FFh
                jz      short loc_18C7B
                mov     al, [bx+15D1h]
                sub     ah, ah
                mov     [bp+var_2], ax
                mov     si, bx
                shl     si, 1
                mov     ax, [si+15D4h]
                mov     [bp+var_4], ax
                mov     ax, [si+15B2h]
                mov     [bp+var_6], ax

loc_18C7B:                              ; CODE XREF: sub_18BEC+68↑j
                                        ; sub_18BEC+72↑j
                cmp     [bp+arg_0], 1
                jnz     short loc_18C9D
                cmp     byte_22D08, 0FFh
                jnz     short loc_18C9D
                mov     ax, [bp+arg_2]
                cmp     [bp+arg_0], ax
                jnz     short loc_18C98
                mov     [bp+var_4], 0B0h
                jmp     short loc_18C9D
; ---------------------------------------------------------------------------
                align 2

loc_18C98:                              ; CODE XREF: sub_18BEC+A2↑j
                mov     [bp+var_4], 98h

loc_18C9D:                              ; CODE XREF: sub_18BEC+93↑j
                                        ; sub_18BEC+9A↑j ...
                push    [bp+var_6]
                push    [bp+var_4]
                push    [bp+var_2]
                mov     bx, [bp+arg_0]
                mov     bl, [bx+54B8h]
                sub     bh, bh
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+370h]
                push    word ptr [bx+36Eh]
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18CC1:                              ; CODE XREF: sub_18BEC+46↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_18BEC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18CC6       proc near               ; CODE XREF: draw_view_outdoors+4D↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                sub     si, si

loc_18CD0:                              ; CODE XREF: sub_18CC6+2C↓j
                mov     al, [si+5FDCh]
                dec     al
                mov     [si+54B4h], al
                mov     al, [si+5FE0h]
                dec     al
                mov     [si+54B8h], al
                mov     al, [si+5FD8h]
                dec     al
                mov     [si+54B0h], al
                inc     si
                cmp     si, 4
                jl      short loc_18CD0
                mov     [bp+var_4], si
                sub     si, si

loc_18CF9:                              ; CODE XREF: sub_18CC6+46↓j
                cmp     byte ptr [si+54B0h], 0FFh
                jz      short loc_18D06

loc_18D00:                              ; CODE XREF: sub_18CC6+44↓j
                mov     [bp+var_4], si
                jmp     short loc_18D0E
; ---------------------------------------------------------------------------
                align 2

loc_18D06:                              ; CODE XREF: sub_18CC6+38↑j
                inc     si
                cmp     si, 4
                jge     short loc_18D00
                jmp     short loc_18CF9
; ---------------------------------------------------------------------------

loc_18D0E:                              ; CODE XREF: sub_18CC6+3D↑j
                mov     ax, [bp+var_4]
                mov     [bp+var_2], ax
                cmp     ax, 4
                jnz     short loc_18D1E
                mov     [bp+var_4], 3

loc_18D1E:                              ; CODE XREF: sub_18CC6+51↑j
                cmp     [bp+var_2], 4
                jz      short loc_18D2D
                push    [bp+var_2]
                call    sub_18AD0
                add     sp, 2

loc_18D2D:                              ; CODE XREF: sub_18CC6+5C↑j
                mov     ax, [bp+var_4]
                mov     [bp+var_6], ax
                or      ax, ax
                jl      short loc_18D49
                mov     di, ax
                mov     si, ax

loc_18D3B:                              ; CODE XREF: sub_18CC6+7E↓j
                push    di
                push    si
                call    sub_18B0C
                add     sp, 4
                dec     si
                jns     short loc_18D3B
                mov     [bp+var_6], si

loc_18D49:                              ; CODE XREF: sub_18CC6+6F↑j
                mov     ax, [bp+var_4]
                mov     [bp+var_6], ax
                or      ax, ax
                jl      short loc_18D65
                mov     di, ax
                mov     si, ax

loc_18D57:                              ; CODE XREF: sub_18CC6+9A↓j
                push    di
                push    si
                call    sub_18BEC
                add     sp, 4
                dec     si
                jns     short loc_18D57
                mov     [bp+var_6], si

loc_18D65:                              ; CODE XREF: sub_18CC6+8B↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_18CC6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; 3D view for outdoor maps

draw_view_outdoors proc near            ; CODE XREF: seg002:0789↑J
                call    thk_res_5F54
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 44h ; 'D'   ; CODE XREF: seg002:04A1↑J
                push    ax
                mov     ax, 8
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBB4
                push    word_1DBB2
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                cmp     g_day_fraction, 80h
                jl      short loc_18DA0
                call    thk_res_4FB2
                jmp     short loc_18DB6
; ---------------------------------------------------------------------------
                align 2

loc_18DA0:                              ; CODE XREF: draw_view_outdoors+2C↑j
                mov     ax, 8
                push    ax
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBD4
                push    word_1DBD2
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_18DB6:                              ; CODE XREF: draw_view_outdoors+31↑j
                call    sub_189B8
                call    sub_18CC6
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                retn
draw_view_outdoors endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; next byte of the event chunk at DGROUP:6052 (pointer word_1DC7A)

evt_read_byte   proc near               ; CODE XREF: evt_read_word+6↓p
                                        ; evt_read_word+E↓p ...
                mov     bx, word_1DC7A
                inc     word_1DC7A
                mov     al, [bx+6052h]
                sub     ah, ah
                retn
evt_read_byte   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_read_word   proc near               ; CODE XREF: evt_read_dword24+6↓p
                                        ; evt_op36_pay_gold+6↓p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                sub     ah, ah
                mov     [bp+var_2], ax
                call    evt_read_byte
                mov     ch, al
                sub     cl, cl
                add     [bp+var_2], cx
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
evt_read_word   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_read_dword24 proc near              ; CODE XREF: evt_op31_modify_char+6B↓p
                                        ; evt_op42_place_treasure+7↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    evt_read_word
                mov     [bp+var_4], ax
                mov     [bp+var_2], 0
                call    evt_read_byte
                sub     ah, ah
                mov     dx, ax
                sub     ax, ax
                add     [bp+var_4], ax
                adc     [bp+var_2], dx
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
evt_read_dword24 endp


; =============== S U B R O U T I N E =======================================

; event variable index -> DGROUP address (flags/counters at 3F6.., 3D8, 3E0/3E1, 3EA, 3F1..)
; Attributes: bp-based frame

evt_var_addr    proc near               ; CODE XREF: evt_op23_get_var+6↓p
                                        ; evt_op26_set_var+18↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0
                cmp     [bp+arg_0], 18h
                jnb     short loc_18E42
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3F6h

loc_18E3B:                              ; CODE XREF: evt_var_addr+7A↓j
                                        ; evt_var_addr+90↓j ...
                mov     [bp+var_2], ax
                jmp     loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E42:                              ; CODE XREF: evt_var_addr+F↑j
                cmp     [bp+arg_0], 23h ; '#'
                jnz     short loc_18E50
                mov     [bp+var_2], 3D8h
                jmp     loc_18EDF
; ---------------------------------------------------------------------------

loc_18E50:                              ; CODE XREF: evt_var_addr+24↑j
                cmp     [bp+arg_0], 2Bh ; '+'
                jnz     short loc_18E5E
                mov     [bp+var_2], 3E0h
                jmp     loc_18EDF
; ---------------------------------------------------------------------------

loc_18E5E:                              ; CODE XREF: evt_var_addr+32↑j
                cmp     [bp+arg_0], 2Ch ; ','
                jnz     short loc_18E6C
                mov     [bp+var_2], 3E1h
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E6C:                              ; CODE XREF: evt_var_addr+40↑j
                cmp     [bp+arg_0], 32h ; '2'
                jnz     short loc_18E7A
                mov     [bp+var_2], 3EAh
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E7A:                              ; CODE XREF: evt_var_addr+4E↑j
                cmp     [bp+arg_0], 33h ; '3'
                jnz     short loc_18E88
                mov     [bp+var_2], 3F1h
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E88:                              ; CODE XREF: evt_var_addr+5C↑j
                cmp     [bp+arg_0], 27h ; '''
                jb      short loc_18E9E
                cmp     [bp+arg_0], 2Ah ; '*'
                ja      short loc_18E9E
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3B5h
                jmp     short loc_18E3B
; ---------------------------------------------------------------------------

loc_18E9E:                              ; CODE XREF: evt_var_addr+6A↑j
                                        ; evt_var_addr+70↑j
                cmp     [bp+arg_0], 3Bh ; ';'
                jb      short loc_18EB4
                cmp     [bp+arg_0], 3Eh ; '>'
                ja      short loc_18EB4
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3B7h
                jmp     short loc_18E3B
; ---------------------------------------------------------------------------

loc_18EB4:                              ; CODE XREF: evt_var_addr+80↑j
                                        ; evt_var_addr+86↑j
                cmp     [bp+arg_0], 84h
                jnz     short loc_18EC2
                mov     [bp+var_2], 3CAh
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18EC2:                              ; CODE XREF: evt_var_addr+96↑j
                cmp     [bp+arg_0], 80h
                jb      short loc_18EDA
                cmp     [bp+arg_0], 84h
                jnb     short loc_18EDA
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 36Ch
                jmp     loc_18E3B
; ---------------------------------------------------------------------------
                align 2

loc_18EDA:                              ; CODE XREF: evt_var_addr+A4↑j
                                        ; evt_var_addr+AA↑j
                or      byte_1DC80, 1

loc_18EDF:                              ; CODE XREF: evt_var_addr+1C↑j
                                        ; evt_var_addr+2B↑j ...
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
evt_var_addr    endp


; =============== S U B R O U T I N E =======================================

; terrain type (byte_1DBEC) picks monster id table at DGROUP:164C..
; Attributes: bp-based frame

random_monster_for_terrain proc near    ; CODE XREF: evt_op11_show_monster_pic+18↓p

var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     [bp+arg_0], 80h
                jb      short loc_18EF8
                mov     ax, 4Bh ; 'K'
                jmp     short loc_18F5F
; ---------------------------------------------------------------------------
                align 2

loc_18EF8:                              ; CODE XREF: random_monster_for_terrain+A↑j
                dec     [bp+arg_0]
                mov     al, byte_1DBEC
                sub     ah, ah
                cmp     ax, 6           ; switch 7 cases
                ja      short def_18F08 ; jumptable 00018F08 default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_18F08[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_18F0E:                              ; CODE XREF: random_monster_for_terrain+22↑j
                                        ; DATA XREF: random_monster_for_terrain:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 0
                sub     bh, bh
                mov     al, [bx+164Ch]

loc_18F17:                              ; CODE XREF: random_monster_for_terrain+3F↓j
                                        ; random_monster_for_terrain+4B↓j ...
                mov     [bp+var_2], al
                jmp     short def_18F08 ; jumptable 00018F08 default case
; ---------------------------------------------------------------------------

loc_18F1C:                              ; CODE XREF: random_monster_for_terrain+22↑j
                                        ; DATA XREF: random_monster_for_terrain+6C↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 3
                sub     bh, bh
                mov     al, [bx+1662h]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F28:                              ; CODE XREF: random_monster_for_terrain+22↑j
                                        ; DATA XREF: random_monster_for_terrain+68↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 1
                sub     bh, bh
                mov     al, [bx+167Ch]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F34:                              ; CODE XREF: random_monster_for_terrain+22↑j
                                        ; DATA XREF: random_monster_for_terrain+6E↓o ...
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 cases 4,6
                sub     bh, bh
                mov     al, [bx+1694h]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F40:                              ; CODE XREF: random_monster_for_terrain+22↑j
                                        ; DATA XREF: random_monster_for_terrain+6A↓o ...
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 cases 2,5
                sub     bh, bh
                mov     al, [bx+16ACh]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2
jpt_18F08       dw offset loc_18F0E     ; DATA XREF: random_monster_for_terrain+22↑r
                                        ; jump table for switch statement
                dw offset loc_18F28     ; jumptable 00018F08 case 1
                dw offset loc_18F40     ; jumptable 00018F08 cases 2,5
                dw offset loc_18F1C     ; jumptable 00018F08 case 3
                dw offset loc_18F34     ; jumptable 00018F08 cases 4,6
                dw offset loc_18F40     ; jumptable 00018F08 cases 2,5
                dw offset loc_18F34     ; jumptable 00018F08 cases 4,6
; ---------------------------------------------------------------------------

def_18F08:                              ; CODE XREF: random_monster_for_terrain+1D↑j
                                        ; random_monster_for_terrain+34↑j
                mov     al, [bp+var_2]  ; jumptable 00018F08 default case
                sub     ah, ah

loc_18F5F:                              ; CODE XREF: random_monster_for_terrain+F↑j
                mov     sp, bp
                pop     bp
                retn
random_monster_for_terrain endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; skip n commands using length table at DGROUP:15E6
; Attributes: bp-based frame

evt_skip_commands proc near             ; CODE XREF: evt_op16_skip_if_cond+16↓p
                                        ; evt_op17_skip_if_not_cond+16↓p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     si, word_1DC7A
                mov     cl, [bp+arg_0]
                jmp     short loc_18F7E
; ---------------------------------------------------------------------------
                align 2

loc_18F72:                              ; CODE XREF: evt_skip_commands+20↓j
                mov     bl, [si+6052h]
                sub     bh, bh
                shl     bx, 1
                add     si, [bx+15E6h]

loc_18F7E:                              ; CODE XREF: evt_skip_commands+B↑j
                mov     al, cl
                dec     cl
                or      al, al
                jnz     short loc_18F72
                mov     word_1DC7A, si
                mov     [bp+arg_0], cl
                pop     si
                pop     bp
                retn
evt_skip_commands endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_count_message_lines proc near       ; CODE XREF: evt_op05_message_box+56↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_2], 0
                sub     cl, cl
                jmp     short loc_18FB5
; ---------------------------------------------------------------------------

loc_18F9E:                              ; CODE XREF: evt_count_message_lines+2F↓j
                inc     cl
                inc     dl
                mov     bx, cx
                sub     bh, bh
                cmp     byte ptr [bx+54D0h], 0Ah
                jz      short loc_18FB2
                cmp     dl, 14h
                jnz     short loc_18FB7

loc_18FB2:                              ; CODE XREF: evt_count_message_lines+1B↑j
                inc     [bp+var_2]

loc_18FB5:                              ; CODE XREF: evt_count_message_lines+C↑j
                sub     dl, dl

loc_18FB7:                              ; CODE XREF: evt_count_message_lines+20↑j
                mov     bx, cx
                sub     bh, bh
                cmp     [bx+54D0h], bh
                jnz     short loc_18F9E
                mov     [bp+var_6], dl
                mov     [bp+var_4], cl
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     sp, bp
                pop     bp
                retn
evt_count_message_lines endp


; =============== S U B R O U T I N E =======================================

; find nth 0FFh-terminated message after the scripts
; Attributes: bp-based frame

evt_seek_message proc near              ; CODE XREF: evt_op01_message+5↓p
                                        ; evt_message_at_row+11↓p ...

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                call    evt_read_byte
                mov     [bp+var_2], al
                mov     ax, word_1DC7C
                mov     word_22D10, ax
                mov     [bp+var_4], 0
                cmp     [bp+var_2], 0
                jz      short loc_1900F
                mov     al, [bp+var_2]
                cbw
                mov     cx, ax
                add     [bp+var_4], al
                mov     di, word_22D10

loc_18FFB:                              ; CODE XREF: evt_seek_message+39↓j
                mov     si, di

loc_18FFD:                              ; CODE XREF: evt_seek_message+35↓j
                mov     bx, si
                inc     si
                cmp     byte ptr [bx+6052h], 0FFh
                jnz     short loc_18FFD
                mov     di, si
                loop    loc_18FFB
                mov     word_22D10, di

loc_1900F:                              ; CODE XREF: evt_seek_message+1C↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_seek_message endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; message bytes & 7Fh (40h = newline) -> DGROUP:54D0
; Attributes: bp-based frame

evt_decode_message proc near            ; CODE XREF: evt_op01_message+8↓p
                                        ; evt_message_at_row+14↓p ...

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     di, di
                mov     si, word_22D10

loc_19024:                              ; CODE XREF: evt_decode_message+38↓j
                mov     al, [si+6052h]
                mov     [bp+var_2], al
                inc     si
                cmp     al, 0FFh
                jz      short loc_19040
                and     [bp+var_2], 7Fh
                cmp     [bp+var_2], 40h ; '@'
                jnz     short loc_19044
                mov     [bp+var_2], 0Ah
                jmp     short loc_19044
; ---------------------------------------------------------------------------

loc_19040:                              ; CODE XREF: evt_decode_message+18↑j
                mov     [bp+var_2], 0

loc_19044:                              ; CODE XREF: evt_decode_message+22↑j
                                        ; evt_decode_message+28↑j
                mov     al, [bp+var_2]
                mov     [di+54D0h], al
                inc     di
                or      al, al
                jnz     short loc_19024
                mov     [bp+var_4], di
                mov     word_22D10, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_decode_message endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op01_message proc near              ; CODE XREF: evt_run_script:loc_1A67C↓p
                or      byte_1DC80, 1
                call    evt_seek_message
                call    evt_decode_message
                mov     ax, 54D0h
                push    ax
                call    thk_print_message_line
                add     sp, 2
                retn
evt_op01_message endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_message_at_row proc near            ; CODE XREF: evt_op03_message_window+16↓p
                                        ; evt_run_script+80↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                sub     si, si
                sub     di, di
                or      byte_1DC80, 2
                call    evt_seek_message
                call    evt_decode_message
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     [bp+var_A], ax
                mov     ax, 16h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                push    [bp+var_A]
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, [bp+var_A]
                mov     [bp+var_8], ax

loc_190AE:                              ; CODE XREF: evt_message_at_row+70↓j
                mov     ax, di
                add     ax, [bp+var_8]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_190BE:                              ; CODE XREF: evt_message_at_row+69↓j
                mov     al, [si+54D0h]
                mov     [bp+var_2], al
                inc     si
                or      al, al
                jz      short loc_190D3
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_190D3:                              ; CODE XREF: evt_message_at_row+54↑j
                cmp     [bp+var_2], 0Ah
                jz      short loc_190DF
                cmp     [bp+var_2], 0
                jnz     short loc_190BE

loc_190DF:                              ; CODE XREF: evt_message_at_row+63↑j
                inc     di
                cmp     [bp+var_2], 0
                jnz     short loc_190AE
                mov     [bp+var_6], di
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_message_at_row endp


; =============== S U B R O U T I N E =======================================


evt_op03_message_window proc near       ; CODE XREF: evt_run_script:loc_1A690↓p
                or      byte_1DC80, 3
                call    thk_draw_screen_rows
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 11h
                push    ax
                call    evt_message_at_row
                add     sp, 2
                retn
evt_op03_message_window endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op04_title_centered proc near       ; CODE XREF: evt_run_script:loc_1A696↓p
                push    di
                call    evt_seek_message
                call    evt_decode_message
                cmp     byte_1DBEE, 0
                jnz     short loc_1915E
                mov     ax, 0FFh
                push    ax
                call    thk_text_set_bg
                add     sp, 2
                mov     ax, 3
                push    ax
                mov     di, 54D0h
                mov     ax, ds
                mov     es, ax
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                sub     cx, 1Ch
                neg     cx
                shr     cx, 1
                push    cx
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 54D0h
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_bg
                add     sp, 2

loc_1915E:                              ; CODE XREF: evt_op04_title_centered+C↑j
                pop     di
                retn
evt_op04_title_centered endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op05_message_box proc near          ; CODE XREF: evt_run_script:loc_1A69C↓p

var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    evt_seek_message
                call    evt_decode_message
                cmp     byte_1DBEE, 0
                jnz     short loc_191E8
                mov     ax, 0Dh
                push    ax
                mov     ax, 18h
                push    ax
                mov     ax, 3
                push    ax
                mov     ax, 4
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
                mov     ax, 0FFh
                push    ax
                call    thk_text_set_bg
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                call    evt_count_message_lines
                sub     ah, ah
                shr     ax, 1
                mov     [bp+var_4], al
                cmp     al, 5
                jnb     short loc_191D5
                sub     ah, ah
                sub     ax, 4
                neg     ax
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_191D5:                              ; CODE XREF: evt_op05_message_box+62↑j
                mov     ax, 54D0h
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    [bp+var_2]
                call    thk_text_window_close
                add     sp, 2

loc_191E8:                              ; CODE XREF: evt_op05_message_box+11↑j
                mov     sp, bp
                pop     bp
                retn
evt_op05_message_box endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op06_message_framed proc near       ; CODE XREF: evt_run_script:loc_1A6A2↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     [bp+var_6], 0
                call    evt_seek_message
                call    evt_decode_message
                mov     si, 54D0h

loc_19201:                              ; CODE XREF: evt_op06_message_framed+21↓j
                cmp     byte ptr [si], 2Dh ; '-'
                jnz     short loc_19209
                mov     byte ptr [si], 7Bh ; '{'

loc_19209:                              ; CODE XREF: evt_op06_message_framed+18↑j
                inc     si
                cmp     byte ptr [si], 0
                jnz     short loc_19201
                mov     [bp+var_2], si
                cmp     byte_1DBEE, 0
                jz      short loc_1921C
                jmp     loc_193B2
; ---------------------------------------------------------------------------

loc_1921C:                              ; CODE XREF: evt_op06_message_framed+2B↑j
                mov     ax, 9
                push    ax
                mov     ax, 12h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_4], ax
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 7
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 10h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     [bp+var_6], 0Bh
                mov     si, 0Bh

loc_1925B:                              ; CODE XREF: evt_op06_message_framed+7A↓j
                mov     ax, 0Eh
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_1925B
                mov     ax, 11h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 14h
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word_1EF20
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 9
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 14h
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word_1EF20
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 12h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     [bp+var_6], 0Bh
                mov     si, 0Bh

loc_192EA:                              ; CODE XREF: evt_op06_message_framed+109↓j
                mov     ax, 0Fh
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_192EA
                mov     ax, 13h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0FFh
                push    ax
                call    thk_text_set_bg
                add     sp, 2
                sub     si, si

loc_1930D:                              ; CODE XREF: evt_op06_message_framed+151↓j
                lea     ax, [si+0Bh]
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 15h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 7Eh ; '~'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 14h
                push    ax
                call    thk_text_putc
                add     sp, 2
                inc     si
                cmp     si, 3
                jl      short loc_1930D
                mov     [bp+var_6], si
                mov     ax, 0Eh
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 13h
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 7Eh ; '~'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 12h
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_bg
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                push    [bp+var_4]
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 54D0h
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    [bp+var_4]
                call    thk_text_window_close
                add     sp, 2

loc_193B2:                              ; CODE XREF: evt_op06_message_framed+2D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_op06_message_framed endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_wait_key    proc near               ; CODE XREF: evt_op08_wait_key_music+9↓p
                                        ; evt_run_script+A5↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    thk_print_gems_label
                mov     si, [bp+var_2]

loc_193C5:                              ; CODE XREF: evt_wait_key+2B↓j
                                        ; evt_wait_key+30↓j ...
                cmp     [bp+arg_0], 0
                jz      short loc_193D0
                call    thk_monster_anim_step
                jmp     short loc_193DF
; ---------------------------------------------------------------------------

loc_193D0:                              ; CODE XREF: evt_wait_key+11↑j
                cmp     g_outdoors, 0
                jnz     short loc_193DC
                call    view_idle_step
                jmp     short loc_193DF
; ---------------------------------------------------------------------------

loc_193DC:                              ; CODE XREF: evt_wait_key+1D↑j
                call    thk_kbd_poll

loc_193DF:                              ; CODE XREF: evt_wait_key+16↑j
                                        ; evt_wait_key+22↑j
                mov     si, ax
                or      si, si
                jz      short loc_193C5
                cmp     si, 0Dh
                jz      short loc_193C5
                cmp     si, 0F0h
                jz      short loc_193C5
                cmp     si, 0F2h
                jz      short loc_193C5
                cmp     si, 0F1h
                jz      short loc_193C5
                cmp     si, 0F3h
                jz      short loc_193C5
                mov     [bp+var_2], si
                call    thk_text_clear_prompt_line
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_wait_key    endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op08_wait_key_music proc near       ; CODE XREF: evt_run_script:loc_1A6B0↓p
                mov     byte_2294F, 0FDh
                mov     ax, 1
                push    ax
                call    evt_wait_key
                add     sp, 2
                retn
evt_op08_wait_key_music endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_ask_yes_no  proc near               ; CODE XREF: seg002:07B9↑J
                                        ; evt_op10_ask_yes_no_music+9↓p ...

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte_1DC7F, 0
                mov     si, [bp+var_2]

loc_1942D:                              ; CODE XREF: evt_ask_yes_no+3C↓j
                cmp     [bp+arg_0], 0
                jz      short loc_19438
                call    thk_monster_anim_step
                jmp     short loc_19447
; ---------------------------------------------------------------------------

loc_19438:                              ; CODE XREF: evt_ask_yes_no+13↑j
                cmp     g_outdoors, 0
                jnz     short loc_19444
                call    view_idle_step
                jmp     short loc_19447
; ---------------------------------------------------------------------------

loc_19444:                              ; CODE XREF: evt_ask_yes_no+1F↑j
                call    thk_kbd_poll

loc_19447:                              ; CODE XREF: evt_ask_yes_no+18↑j
                                        ; evt_ask_yes_no+24↑j
                mov     si, ax
                push    si
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jz      short loc_1945C
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_1942D

loc_1945C:                              ; CODE XREF: evt_ask_yes_no+37↑j
                mov     [bp+var_2], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_19468
                inc     byte_1DC7F

loc_19468:                              ; CODE XREF: evt_ask_yes_no+44↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_ask_yes_no  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op10_ask_yes_no_music proc near     ; CODE XREF: seg002:07C5↑J
                                        ; evt_run_script:loc_1A6BE↓p
                mov     byte_2294F, 0FDh
                mov     ax, 1
                push    ax
                call    evt_ask_yes_no
                add     sp, 2
                retn
evt_op10_ask_yes_no_music endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op11_show_monster_pic proc near     ; CODE XREF: evt_run_script:loc_1A6C4↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    evt_read_byte
                mov     [bp+var_2], al
                call    evt_read_byte
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    random_monster_for_terrain
                add     sp, 2
                mov     byte_277D4, al
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     al, byte_277D4
                sub     ah, ah
                push    ax
                call    thk_monster_gfx_load
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    thk_monster_gfx_draw
                or      byte_1DC80, 4
                mov     sp, bp
                pop     bp
                retn
evt_op11_show_monster_pic endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; byte map id (bit7 = random location), byte x|y<<4
; Attributes: bp-based frame

evt_op12_teleport proc near             ; CODE XREF: evt_run_script:loc_1A6CA↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    evt_read_byte
                mov     [bp+var_2], al
                call    evt_read_byte
                mov     [bp+var_4], al
                test    [bp+var_2], 40h
                jz      short loc_1950B
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     al, 5
                mov     [bp+var_2], al
                cmp     al, 11h
                jb      short loc_19507
                add     [bp+var_2], 10h

loc_19507:                              ; CODE XREF: evt_op12_teleport+2D↑j
                or      [bp+var_2], 80h

loc_1950B:                              ; CODE XREF: evt_op12_teleport+16↑j
                cmp     [bp+var_2], 80h
                jb      short loc_19522
                mov     ax, 0FFh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al

loc_19522:                              ; CODE XREF: evt_op12_teleport+3B↑j
                mov     al, [bp+var_4]
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr g_party_y, al
                and     [bp+var_2], 3Fh
                sub     ah, ah
                push    ax
                mov     al, g_party_x
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    enter_map
                add     sp, 6
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    evt_op15_end
                mov     byte_1DC7E, 1
                mov     sp, bp
                pop     bp
                retn
evt_op12_teleport endp


; =============== S U B R O U T I N E =======================================


evt_op13_play_sound proc near           ; CODE XREF: evt_run_script:loc_1A6D0↓p
                call    evt_read_byte
                sub     ah, ah
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                retn
evt_op13_play_sound endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; maps entrance ids to a destination map
; Attributes: bp-based frame

evt_map_group_for_entrance proc near    ; CODE XREF: evt_op14_enter_location+142↓p

var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 10h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_19598
                mov     [bp+var_2], 3Ch ; '<'
                sub     [bp+arg_0], 8
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19598:                              ; CODE XREF: evt_map_group_for_entrance+1C↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_195BC
                mov     [bp+var_2], 3Dh ; '='
                sub     [bp+arg_0], 10h
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_195BC:                              ; CODE XREF: evt_map_group_for_entrance+40↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 4Bh ; 'K'
                push    ax
                mov     ax, 38h ; '8'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_195E0
                mov     [bp+var_2], 3Eh ; '>'
                sub     [bp+arg_0], 37h ; '7'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_195E0:                              ; CODE XREF: evt_map_group_for_entrance+64↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 54h ; 'T'
                push    ax
                mov     ax, 4Ch ; 'L'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_19604
                mov     [bp+var_2], 3Fh ; '?'
                sub     [bp+arg_0], 4Bh ; 'K'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19604:                              ; CODE XREF: evt_map_group_for_entrance+88↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 5Bh ; '['
                push    ax
                mov     ax, 56h ; 'V'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_19628
                mov     [bp+var_2], 40h ; '@'
                sub     [bp+arg_0], 55h ; 'U'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19628:                              ; CODE XREF: evt_map_group_for_entrance+AC↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 5Eh ; '^'
                push    ax
                mov     ax, 5Ch ; '\'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_1964C
                mov     [bp+var_2], 41h ; 'A'
                sub     [bp+arg_0], 5Bh ; '['
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_1964C:                              ; CODE XREF: evt_map_group_for_entrance+D0↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 69h ; 'i'
                push    ax
                mov     ax, 65h ; 'e'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_19670
                mov     [bp+var_2], 42h ; 'B'
                sub     [bp+arg_0], 64h ; 'd'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19670:                              ; CODE XREF: evt_map_group_for_entrance+F4↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 7Ch ; '|'
                push    ax
                mov     ax, 6Ah ; 'j'
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_19692
                mov     [bp+var_2], 43h ; 'C'
                sub     [bp+arg_0], 69h ; 'i'
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_19692:                              ; CODE XREF: evt_map_group_for_entrance+118↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 98h
                push    ax
                mov     ax, 97h
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_196B4
                mov     [bp+var_2], 44h ; 'D'
                sub     [bp+arg_0], 96h
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_196B4:                              ; CODE XREF: evt_map_group_for_entrance+13A↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 0F3h
                push    ax
                mov     ax, 0E3h
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_196D6
                mov     [bp+var_2], 45h ; 'E'
                sub     [bp+arg_0], 0E2h
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_196D6:                              ; CODE XREF: evt_map_group_for_entrance+15C↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 0FBh
                push    ax
                mov     ax, 0F4h
                push    ax
                call    thk_in_range
                add     sp, 6
                or      ax, ax
                jz      short loc_196F6
                mov     [bp+var_2], 46h ; 'F'
                sub     [bp+arg_0], 0F3h

loc_196F6:                              ; CODE XREF: evt_map_group_for_entrance+26↑j
                                        ; evt_map_group_for_entrance+4A↑j ...
                mov     al, g_map_id
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                mov     g_map_id, al
                call    thk_load_map_events
                mov     al, [bp+var_4]
                mov     g_map_id, al
                mov     al, [bp+arg_0]
                mov     byte_22D13, al
                mov     sp, bp
                pop     bp
                retn
evt_map_group_for_entrance endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; sub-id: 1 inn, 2 training, 3 tavern, 4/5 temple, 6 blacksmith, 100.. caves-overlay events, else map entrance
; Attributes: bp-based frame

evt_op14_enter_location proc near       ; CODE XREF: evt_run_script:loc_1A6D6↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     byte ptr g_party_y+1, 1
                call    evt_read_byte
                mov     [bp+var_2], al
                mov     g_disk_needed, 2
                sub     ah, ah
                cmp     ax, 80h
                jz      short loc_197A2
                jbe     short loc_19739
                jmp     loc_1985E
; ---------------------------------------------------------------------------

loc_19739:                              ; CODE XREF: evt_op14_enter_location+1E↑j
                cmp     ax, 5
                jz      short loc_19778
                jbe     short loc_19743
                jmp     loc_19822
; ---------------------------------------------------------------------------

loc_19743:                              ; CODE XREF: evt_op14_enter_location+28↑j
                cmp     ax, 1
                jz      short loc_1975A
                cmp     ax, 2
                jz      short loc_19766
                cmp     ax, 3
                jz      short loc_1976C
                cmp     ax, 4
                jz      short loc_19772
                jmp     loc_19852
; ---------------------------------------------------------------------------

loc_1975A:                              ; CODE XREF: evt_op14_enter_location+30↑j
                mov     g_disk_needed, 1
                call    thk_1RETINN_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19766:                              ; CODE XREF: evt_op14_enter_location+35↑j
                call    thk_2MISC2_CE30
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1976C:                              ; CODE XREF: evt_op14_enter_location+3A↑j
                call    thk_2BRAIN_D15A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19772:                              ; CODE XREF: evt_op14_enter_location+3F↑j
                call    thk_2TEMPLE_CA88
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19778:                              ; CODE XREF: evt_op14_enter_location+26↑j
                call    thk_2TEMPLE_CB9C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1977E:                              ; CODE XREF: evt_op14_enter_location+111↓j
                call    thk_2SMITH_CCBA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19784:                              ; CODE XREF: evt_op14_enter_location+119↓j
                call    thk_2BRAIN_C7E2
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1978A:                              ; CODE XREF: evt_op14_enter_location+121↓j
                call    thk_2BRAIN_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19790:                              ; CODE XREF: evt_op14_enter_location+129↓j
                call    thk_2CAVES_C99A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19796:                              ; CODE XREF: evt_op14_enter_location+131↓j
                call    thk_2CAVES_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1979C:                              ; CODE XREF: evt_op14_enter_location+139↓j
                call    thk_2CAVES_C1DA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197A2:                              ; CODE XREF: evt_op14_enter_location+1C↑j
                call    thk_2CAVES_C23C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197A8:                              ; CODE XREF: evt_op14_enter_location+157↓j
                sub     ax, ax

loc_197AA:                              ; CODE XREF: evt_op14_enter_location+A1↓j
                                        ; evt_op14_enter_location+A7↓j
                push    ax
                call    thk_2CAVES_C308

loc_197AE:                              ; CODE XREF: evt_op14_enter_location+145↓j
                add     sp, 2
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197B4:                              ; CODE XREF: evt_op14_enter_location+15F↓j
                mov     ax, 1
                jmp     short loc_197AA
; ---------------------------------------------------------------------------
                align 2

loc_197BA:                              ; CODE XREF: evt_op14_enter_location+167↓j
                mov     ax, 2
                jmp     short loc_197AA
; ---------------------------------------------------------------------------
                align 2

loc_197C0:                              ; CODE XREF: evt_op14_enter_location+16F↓j
                call    thk_2CAVES_D5DE
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197C6:                              ; CODE XREF: evt_op14_enter_location+177↓j
                call    thk_2CAVES_D5E8
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197CC:                              ; CODE XREF: evt_op14_enter_location+14D↓j
                call    thk_2CAVES_C462
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197D2:                              ; CODE XREF: evt_op14_enter_location+181↓j
                call    thk_2CAVES_C52C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197D8:                              ; CODE XREF: evt_op14_enter_location+189↓j
                call    thk_2CAVES_C5AC
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197DE:                              ; CODE XREF: evt_op14_enter_location+191↓j
                call    thk_2CAVES_C66E
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197E4:                              ; CODE XREF: evt_op14_enter_location+199↓j
                call    thk_2CAVES_C73A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197EA:                              ; CODE XREF: evt_op14_enter_location+1A1↓j
                call    thk_2CAVES_D5F4
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197F0:                              ; CODE XREF: evt_op14_enter_location+1A9↓j
                call    thk_2SMITH_CEC8
                cmp     byte ptr g_party_y+1, 2
                jnz     short loc_1980C
                mov     al, g_inn_town
                mov     g_map_id, al
                mov     g_disk_needed, 1
                call    thk_1RETINN_C1EA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1980C:                              ; CODE XREF: evt_op14_enter_location+E2↑j
                cmp     byte ptr g_party_y+1, 3
                jnz     short loc_1981A
                call    thk_res_3FD8
                jmp     loc_198C4
; ---------------------------------------------------------------------------
                align 2

loc_1981A:                              ; CODE XREF: evt_op14_enter_location+FB↑j
                mov     byte ptr g_party_y+1, 1
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19822:                              ; CODE XREF: evt_op14_enter_location+2A↑j
                cmp     ax, 6
                jnz     short loc_1982A
                jmp     loc_1977E
; ---------------------------------------------------------------------------

loc_1982A:                              ; CODE XREF: evt_op14_enter_location+10F↑j
                cmp     ax, 7
                jnz     short loc_19832
                jmp     loc_19784
; ---------------------------------------------------------------------------

loc_19832:                              ; CODE XREF: evt_op14_enter_location+117↑j
                cmp     ax, 8
                jnz     short loc_1983A
                jmp     loc_1978A
; ---------------------------------------------------------------------------

loc_1983A:                              ; CODE XREF: evt_op14_enter_location+11F↑j
                cmp     ax, 64h ; 'd'
                jnz     short loc_19842
                jmp     loc_19790
; ---------------------------------------------------------------------------

loc_19842:                              ; CODE XREF: evt_op14_enter_location+127↑j
                cmp     ax, 7Eh ; '~'
                jnz     short loc_1984A
                jmp     loc_19796
; ---------------------------------------------------------------------------

loc_1984A:                              ; CODE XREF: evt_op14_enter_location+12F↑j
                cmp     ax, 7Fh
                jnz     short loc_19852
                jmp     loc_1979C
; ---------------------------------------------------------------------------

loc_19852:                              ; CODE XREF: evt_op14_enter_location+41↑j
                                        ; evt_op14_enter_location+137↑j ...
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    evt_map_group_for_entrance
                jmp     loc_197AE
; ---------------------------------------------------------------------------

loc_1985E:                              ; CODE XREF: evt_op14_enter_location+20↑j
                cmp     ax, 0CBh
                jnz     short loc_19866
                jmp     loc_197CC
; ---------------------------------------------------------------------------

loc_19866:                              ; CODE XREF: evt_op14_enter_location+14B↑j
                ja      short loc_19892
                cmp     ax, 81h
                jnz     short loc_19870
                jmp     loc_197A8
; ---------------------------------------------------------------------------

loc_19870:                              ; CODE XREF: evt_op14_enter_location+155↑j
                cmp     ax, 82h
                jnz     short loc_19878
                jmp     loc_197B4
; ---------------------------------------------------------------------------

loc_19878:                              ; CODE XREF: evt_op14_enter_location+15D↑j
                cmp     ax, 83h
                jnz     short loc_19880
                jmp     loc_197BA
; ---------------------------------------------------------------------------

loc_19880:                              ; CODE XREF: evt_op14_enter_location+165↑j
                cmp     ax, 0C9h
                jnz     short loc_19888
                jmp     loc_197C0
; ---------------------------------------------------------------------------

loc_19888:                              ; CODE XREF: evt_op14_enter_location+16D↑j
                cmp     ax, 0CAh
                jnz     short loc_19890
                jmp     loc_197C6
; ---------------------------------------------------------------------------

loc_19890:                              ; CODE XREF: evt_op14_enter_location+175↑j
                jmp     short loc_19852
; ---------------------------------------------------------------------------

loc_19892:                              ; CODE XREF: evt_op14_enter_location:loc_19866↑j
                cmp     ax, 0CCh
                jnz     short loc_1989A
                jmp     loc_197D2
; ---------------------------------------------------------------------------

loc_1989A:                              ; CODE XREF: evt_op14_enter_location+17F↑j
                cmp     ax, 0CDh
                jnz     short loc_198A2
                jmp     loc_197D8
; ---------------------------------------------------------------------------

loc_198A2:                              ; CODE XREF: evt_op14_enter_location+187↑j
                cmp     ax, 0CEh
                jnz     short loc_198AA
                jmp     loc_197DE
; ---------------------------------------------------------------------------

loc_198AA:                              ; CODE XREF: evt_op14_enter_location+18F↑j
                cmp     ax, 0CFh
                jnz     short loc_198B2
                jmp     loc_197E4
; ---------------------------------------------------------------------------

loc_198B2:                              ; CODE XREF: evt_op14_enter_location+197↑j
                cmp     ax, 0E2h
                jnz     short loc_198BA
                jmp     loc_197EA
; ---------------------------------------------------------------------------

loc_198BA:                              ; CODE XREF: evt_op14_enter_location+19F↑j
                cmp     ax, 0FDh
                jnz     short loc_198C2
                jmp     loc_197F0
; ---------------------------------------------------------------------------

loc_198C2:                              ; CODE XREF: evt_op14_enter_location+1A7↑j
                jmp     short loc_19852
; ---------------------------------------------------------------------------

loc_198C4:                              ; CODE XREF: evt_op14_enter_location+4D↑j
                                        ; evt_op14_enter_location+53↑j ...
                mov     sp, bp
                pop     bp
                retn
evt_op14_enter_location endp


; =============== S U B R O U T I N E =======================================


evt_op15_end    proc near               ; CODE XREF: evt_op12_teleport+80↑p
                                        ; evt_run_script:loc_1A6DC↓p
                mov     byte ptr g_party_y+1, 1
                call    evt_finish
                retn
evt_op15_end    endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op16_skip_if_cond proc near         ; CODE XREF: evt_run_script:loc_1A6E2↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     byte_1DC7F, 0
                jz      short loc_198EE
                sub     ah, ah
                push    ax
                call    evt_skip_commands
                add     sp, 2

loc_198EE:                              ; CODE XREF: evt_op16_skip_if_cond+11↑j
                mov     sp, bp
                pop     bp
                retn
evt_op16_skip_if_cond endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op17_skip_if_not_cond proc near     ; CODE XREF: evt_run_script:loc_1A6E8↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     byte_1DC7F, 0
                jnz     short loc_1990E
                sub     ah, ah
                push    ax
                call    evt_skip_commands
                add     sp, 2

loc_1990E:                              ; CODE XREF: evt_op17_skip_if_not_cond+11↑j
                mov     sp, bp
                pop     bp
                retn
evt_op17_skip_if_not_cond endp


; =============== S U B R O U T I N E =======================================

; 10 monster ids + parameters -> start_combat
; Attributes: bp-based frame

evt_fight       proc near               ; CODE XREF: evt_op19_fight_noflags+4↓p
                                        ; evt_run_script+EB↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte_1DC65, 80h
                mov     word_238A0, 0
                cmp     [bp+arg_0], 0
                jz      short loc_1992F
                mov     byte_1DC65, 0

loc_1992F:                              ; CODE XREF: evt_fight+16↑j
                sub     si, si

loc_19931:                              ; CODE XREF: evt_fight+2A↓j
                call    evt_read_byte
                mov     [si-6980h], al
                inc     si
                cmp     si, 0Ah
                jl      short loc_19931
                mov     [bp+var_2], si
                cmp     byte_1DC65, 80h
                jnz     short loc_19954
                call    evt_read_byte
                mov     byte_26EDA, al
                call    evt_read_byte
                jmp     short loc_19959
; ---------------------------------------------------------------------------
                align 2

loc_19954:                              ; CODE XREF: evt_fight+34↑j
                sub     al, al
                mov     byte_26EDA, al

loc_19959:                              ; CODE XREF: evt_fight+3F↑j
                mov     byte_1DD58, al
                call    thk_start_combat
                mov     byte ptr g_party_y+1, 1
                call    thk_party_count_able
                or      ax, ax
                jz      short loc_1997E
                cmp     byte_1DD59, 0
                jnz     short loc_19979
                cmp     word_238A0, 0
                jz      short loc_1997E

loc_19979:                              ; CODE XREF: evt_fight+5E↑j
                mov     byte_1DC7E, 1

loc_1997E:                              ; CODE XREF: evt_fight+57↑j
                                        ; evt_fight+65↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_fight       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op19_fight_noflags proc near        ; CODE XREF: evt_run_script:loc_1A6F6↓p
                mov     ax, 1
                push    ax
                call    evt_fight
                add     sp, 2
                retn
evt_op19_fight_noflags endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; clears one-shot flag (bit 7) of the current cell

evt_op20_clear_trigger proc near        ; CODE XREF: evt_run_script:loc_1A6FC↓p
                push    si
                mov     byte ptr g_party_y+1, 1
                call    evt_finish
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh
                pop     si
                retn
evt_op20_clear_trigger endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_199B8       proc near               ; CODE XREF: evt_op21_check_char+76↓p

var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     [bp+arg_0], 9
                jnz     short loc_199DA
                mov     al, byte_22D0E
                mov     [bp+var_2], al
                or      al, al
                jnz     short loc_199E0
                mov     al, byte_22D12
                mov     [bp+var_2], al
                mov     byte_22D0E, al
                jmp     short loc_199E0
; ---------------------------------------------------------------------------
                align 2

loc_199DA:                              ; CODE XREF: sub_199B8+A↑j
                mov     al, [bp+arg_0]
                mov     [bp+var_2], al

loc_199E0:                              ; CODE XREF: sub_199B8+14↑j
                                        ; sub_199B8+1F↑j
                mov     al, byte_22D0C
                sub     ah, ah
                push    ax
                mov     al, [bp+var_2]
                dec     ax
                push    ax
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    loc_1AA00
                mov     bx, word_27842
                mov     al, [bx]
                sub     ah, ah
                mov     sp, bp
                pop     bp
                retn
sub_199B8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op21_check_char proc near           ; CODE XREF: evt_op24_check_char_alt+4↓p
                                        ; evt_run_script+FF↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 1
                mov     al, byte_1DC7F
                mov     byte_22D12, al
                sub     al, al
                mov     byte_22D0D, al
                mov     byte_1DC7F, al
                call    evt_read_byte
                mov     [bp+var_2], al
                call    evt_read_byte
                mov     byte_22D0C, al
                call    evt_read_byte
                mov     byte_22D0F, al
                cmp     [bp+arg_0], 0
                jz      short loc_19A38
                call    evt_read_byte
                mov     byte_22D0D, al

loc_19A38:                              ; CODE XREF: evt_op21_check_char+2E↑j
                cmp     [bp+var_2], 80h
                jb      short loc_19A44
                mov     al, byte_22D12
                mov     byte_22D0D, al

loc_19A44:                              ; CODE XREF: evt_op21_check_char+3A↑j
                and     [bp+var_2], 7Fh
                jz      short loc_19A5D
                cmp     [bp+var_2], 9
                jz      short loc_19A5D
                mov     al, [bp+var_2]
                cmp     byte ptr g_party_size, al
                jnb     short loc_19A5D
                mov     [bp+var_2], 1

loc_19A5D:                              ; CODE XREF: evt_op21_check_char+46↑j
                                        ; evt_op21_check_char+4C↑j ...
                cmp     [bp+var_2], 0
                jnz     short loc_19A6C
                mov     al, byte ptr g_party_size
                mov     [bp+var_4], al
                mov     [bp+var_2], al

loc_19A6C:                              ; CODE XREF: evt_op21_check_char+5F↑j
                                        ; evt_op21_check_char+B4↓j
                dec     [bp+var_4]
                mov     al, [bp+var_2]
                dec     [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_199B8
                add     sp, 2
                mov     [bp+var_6], al
                cmp     [bp+arg_0], 0
                jz      short loc_19A9E
                mov     al, byte_22D0F
                and     [bp+var_6], al
                mov     al, byte_22D0D
                or      [bp+var_6], al
                mov     bx, word_27842
                mov     al, [bp+var_6]
                mov     [bx], al
                jmp     short loc_19AB2
; ---------------------------------------------------------------------------

loc_19A9E:                              ; CODE XREF: evt_op21_check_char+83↑j
                cmp     byte_22D0F, 0
                jz      short loc_19AAB
                mov     al, byte_22D0F
                and     [bp+var_6], al

loc_19AAB:                              ; CODE XREF: evt_op21_check_char+A1↑j
                mov     al, [bp+var_6]
                or      byte_1DC7F, al

loc_19AB2:                              ; CODE XREF: evt_op21_check_char+9A↑j
                cmp     [bp+var_4], 0
                jnz     short loc_19A6C
                mov     sp, bp
                pop     bp
                retn
evt_op21_check_char endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op22_has_item proc near             ; CODE XREF: evt_run_script:loc_1A70C↓p

var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     byte_1DC7F, 0
                call    evt_read_byte
                mov     [bp+var_8], al
                call    evt_read_byte
                mov     [bp+var_8], al
                sub     di, di
                jmp     short loc_19ADB
; ---------------------------------------------------------------------------
                align 2

loc_19ADA:                              ; CODE XREF: evt_op22_has_item+59↓j
                inc     di

loc_19ADB:                              ; CODE XREF: evt_op22_has_item+1B↑j
                cmp     di, g_party_size
                jge     short loc_19B17
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_6], 0
                mov     si, ax
                mov     dl, [bp+var_8]
                sub     cx, cx

loc_19AF7:                              ; CODE XREF: evt_op22_has_item+4F↓j
                mov     bx, cx
                cmp     [bx+si+3Ah], dl
                jz      short loc_19B03
                cmp     [bx+si+28h], dl
                jnz     short loc_19B07

loc_19B03:                              ; CODE XREF: evt_op22_has_item+40↑j
                inc     byte_1DC7F

loc_19B07:                              ; CODE XREF: evt_op22_has_item+45↑j
                inc     cx
                cmp     cx, 6
                jl      short loc_19AF7
                mov     [bp+var_6], cx
                cmp     byte_1DC7F, 0
                jz      short loc_19ADA

loc_19B17:                              ; CODE XREF: evt_op22_has_item+23↑j
                mov     [bp+var_4], di
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_op22_has_item endp


; =============== S U B R O U T I N E =======================================


evt_op23_get_var proc near              ; CODE XREF: evt_run_script:loc_1A712↓p
                call    evt_read_byte
                sub     ah, ah
                push    ax
                call    evt_var_addr
                add     sp, 2
                mov     bx, ax
                mov     al, [bx]
                mov     byte_1DC7F, al
                call    evt_read_byte
                retn
evt_op23_get_var endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op24_check_char_alt proc near       ; CODE XREF: evt_run_script:loc_1A718↓p
                mov     ax, 1
                push    ax
                call    evt_op21_check_char
                add     sp, 2
                retn
evt_op24_check_char_alt endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op25_give_item proc near            ; CODE XREF: evt_run_script:loc_1A71E↓p

var_10          = word ptr -10h
var_E           = byte ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                call    evt_read_byte
                mov     [bp+var_6], al
                call    evt_read_byte
                mov     [bp+var_E], al
                call    evt_read_byte
                mov     [bp+var_C], al
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     [bp+var_6], 80h
                jb      short loc_19B70
                mov     al, byte_1DC7F
                mov     [bp+var_E], al

loc_19B70:                              ; CODE XREF: evt_op25_give_item+24↑j
                mov     byte_1DC7F, 0
                sub     di, di
                jmp     short loc_19B8B
; ---------------------------------------------------------------------------
                align 2

loc_19B7A:                              ; CODE XREF: evt_op25_give_item+69↓j
                cmp     byte_1DC7F, 0
                jnz     short loc_19BCB
                inc     cx
                cmp     cx, 6
                jge     short loc_19BCB
                jmp     short loc_19BA7
; ---------------------------------------------------------------------------
                align 2

loc_19B8A:                              ; CODE XREF: evt_op25_give_item+8F↓j
                inc     di

loc_19B8B:                              ; CODE XREF: evt_op25_give_item+33↑j
                cmp     di, g_party_size
                jge     short loc_19BD5
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_A], 0
                mov     si, ax
                mov     dx, [bp+var_10]
                sub     cx, cx

loc_19BA7:                              ; CODE XREF: evt_op25_give_item+43↑j
                mov     bx, cx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_19B7A
                mov     dx, cx
                add     dx, si
                mov     bx, dx
                mov     al, [bp+var_E]
                mov     [bx+3Ah], al
                mov     al, [bp+var_C]
                mov     [bx+40h], al
                mov     al, [bp+var_2]
                mov     [bx+46h], al
                inc     byte_1DC7F

loc_19BCB:                              ; CODE XREF: evt_op25_give_item+3B↑j
                                        ; evt_op25_give_item+41↑j
                mov     [bp+var_A], cx
                cmp     byte_1DC7F, 0
                jz      short loc_19B8A

loc_19BD5:                              ; CODE XREF: evt_op25_give_item+4B↑j
                mov     [bp+var_8], di
                cmp     byte_1DC7F, 0
                jnz     short loc_19C13
                sub     si, si

loc_19BE1:                              ; CODE XREF: evt_op25_give_item+B0↓j
                cmp     byte ptr [si+6950h], 0
                jnz     short loc_19BEE

loc_19BE8:                              ; CODE XREF: evt_op25_give_item+AE↓j
                mov     [bp+var_8], si
                jmp     short loc_19BF6
; ---------------------------------------------------------------------------
                align 2

loc_19BEE:                              ; CODE XREF: evt_op25_give_item+A2↑j
                inc     si
                cmp     si, 2
                jge     short loc_19BE8
                jmp     short loc_19BE1
; ---------------------------------------------------------------------------

loc_19BF6:                              ; CODE XREF: evt_op25_give_item+A7↑j
                mov     bx, [bp+var_8]
                mov     al, [bp+var_E]
                mov     [bx+6950h], al
                mov     al, [bp+var_2]
                mov     [bx+6953h], al
                mov     al, [bp+var_C]
                mov     [bx+6956h], al
                mov     byte_1DC84, 0FFh

loc_19C13:                              ; CODE XREF: evt_op25_give_item+99↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_op25_give_item endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op26_set_var proc near              ; CODE XREF: evt_run_script:loc_1A724↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    evt_read_byte
                mov     [bp+var_2], al
                call    evt_read_byte
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    evt_var_addr
                mov     bx, ax
                mov     al, [bp+var_4]
                mov     [bx], al
                mov     sp, bp
                pop     bp
                retn
evt_op26_set_var endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op27_cond_limit proc near           ; CODE XREF: evt_run_script:loc_1A72A↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                mov     [bp+var_2], al
                mov     al, byte_1DC7F
                cmp     [bp+var_2], al
                jbe     short loc_19C59
                mov     byte_1DC7F, 0

loc_19C59:                              ; CODE XREF: evt_op27_cond_limit+12↑j
                mov     sp, bp
                pop     bp
                retn
evt_op27_cond_limit endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op28_random proc near               ; CODE XREF: evt_run_script:loc_1A730↓p
                call    evt_read_byte
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     byte_1DC7F, al
                retn
evt_op28_random endp


; =============== S U B R O U T I N E =======================================


evt_op29_wait   proc near               ; CODE XREF: evt_run_script:loc_1A736↓p
                call    evt_read_byte
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                inc     ax
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                retn
evt_op29_wait   endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op30_delay  proc near               ; CODE XREF: evt_run_script:loc_1A73C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                mov     [bp+var_2], al
                jmp     short loc_19CA9
; ---------------------------------------------------------------------------

loc_19C98:                              ; CODE XREF: evt_op30_delay+27↓j
                mov     ax, 0Ah
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                call    thk_kbd_poll
                or      ax, ax
                jnz     short loc_19CB3

loc_19CA9:                              ; CODE XREF: evt_op30_delay+C↑j
                mov     al, [bp+var_2]
                dec     [bp+var_2]
                or      al, al
                jnz     short loc_19C98

loc_19CB3:                              ; CODE XREF: evt_op30_delay+1D↑j
                mov     sp, bp
                pop     bp
                retn
evt_op30_delay  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19CB8       proc near               ; CODE XREF: evt_op31_modify_char+9D↓p
                                        ; evt_op31_modify_char+D7↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6
arg_4           = byte ptr  8
arg_6           = byte ptr  0Ah
arg_8           = word ptr  0Ch
arg_A           = word ptr  0Eh
arg_C           = word ptr  10h

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                mov     al, byte_22D0C
                sub     ah, ah
                push    ax
                mov     al, [bp+arg_0]
                dec     ax
                push    ax
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    loc_1AA00
                add     sp, 4
                cmp     [bp+arg_2], 0
                jz      short loc_19CE9
                jmp     loc_19D78
; ---------------------------------------------------------------------------

loc_19CE9:                              ; CODE XREF: sub_19CB8+2C↑j
                cmp     byte_27841, 1
                jnz     short loc_19D10
                mov     al, [bp+arg_6]
                sub     ah, ah
                mov     cl, byte_27840
                sub     ch, ch
                add     ax, cx
                mov     [bp+var_6], ax
                cmp     ax, 100h
                jnb     short loc_19D08
                jmp     loc_19DF1
; ---------------------------------------------------------------------------

loc_19D08:                              ; CODE XREF: sub_19CB8+4B↑j
                mov     [bp+var_6], 0FFh
                jmp     loc_19DF1
; ---------------------------------------------------------------------------

loc_19D10:                              ; CODE XREF: sub_19CB8+36↑j
                cmp     byte_27841, 2
                jnz     short loc_19D3E
                mov     ax, [bp+arg_8]
                sub     dx, dx
                add     ax, word_2783E
                adc     dx, dx
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx
                cmp     dx, 1
                jnb     short loc_19D30
                jmp     loc_19DF1
; ---------------------------------------------------------------------------

loc_19D30:                              ; CODE XREF: sub_19CB8+73↑j
                mov     [bp+var_6], 0
                mov     [bp+var_4], 1
                jmp     loc_19DF1
; ---------------------------------------------------------------------------
                align 2

loc_19D3E:                              ; CODE XREF: sub_19CB8+5D↑j
                mov     ax, [bp+arg_A]
                mov     dx, [bp+arg_C]
                add     ax, word_2783A
                adc     dx, word_2783C
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx
                mov     ax, [bp+arg_A]
                mov     dx, [bp+arg_C]
                cmp     [bp+var_4], dx
                jbe     short loc_19D60
                jmp     loc_19DF1
; ---------------------------------------------------------------------------

loc_19D60:                              ; CODE XREF: sub_19CB8+A3↑j
                jb      short loc_19D6A
                cmp     [bp+var_6], ax
                jb      short loc_19D6A
                jmp     loc_19DF1
; ---------------------------------------------------------------------------

loc_19D6A:                              ; CODE XREF: sub_19CB8:loc_19D60↑j
                                        ; sub_19CB8+AD↑j
                mov     [bp+var_6], 0FFFFh
                mov     [bp+var_4], 0FFFFh
                jmp     short loc_19DF1
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_19D78:                              ; CODE XREF: sub_19CB8+2E↑j
                cmp     byte_27841, 1
                jnz     short loc_19D9E
                mov     al, byte_27840
                cmp     [bp+arg_6], al
                jbe     short loc_19D92
                mov     byte ptr [bp+var_6], 0

loc_19D8B:                              ; CODE XREF: sub_19CB8+FA↓j
                                        ; sub_19CB8+123↓j
                mov     byte_1DC7F, 0
                jmp     short loc_19DF1
; ---------------------------------------------------------------------------

loc_19D92:                              ; CODE XREF: sub_19CB8+CD↑j
                mov     al, byte_27840
                sub     al, [bp+arg_6]
                mov     byte ptr [bp+var_6], al
                jmp     short loc_19DF1
; ---------------------------------------------------------------------------
                align 2

loc_19D9E:                              ; CODE XREF: sub_19CB8+C5↑j
                cmp     byte_27841, 2
                jnz     short loc_19DC0
                mov     ax, word_2783E
                cmp     [bp+arg_8], ax
                jbe     short loc_19DB4
                mov     [bp+var_6], 0
                jmp     short loc_19D8B
; ---------------------------------------------------------------------------

loc_19DB4:                              ; CODE XREF: sub_19CB8+F3↑j
                mov     ax, word_2783E
                sub     ax, [bp+arg_8]
                mov     [bp+var_6], ax
                jmp     short loc_19DF1
; ---------------------------------------------------------------------------
                align 2

loc_19DC0:                              ; CODE XREF: sub_19CB8+EB↑j
                mov     ax, word_2783A
                mov     dx, word_2783C
                cmp     [bp+arg_C], dx
                jb      short loc_19DDE
                ja      short loc_19DD3
                cmp     [bp+arg_A], ax
                jbe     short loc_19DDE

loc_19DD3:                              ; CODE XREF: sub_19CB8+114↑j
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                jmp     short loc_19D8B
; ---------------------------------------------------------------------------
                align 2

loc_19DDE:                              ; CODE XREF: sub_19CB8+112↑j
                                        ; sub_19CB8+119↑j
                mov     ax, word_2783A
                mov     dx, word_2783C
                sub     ax, [bp+arg_A]
                sbb     dx, [bp+arg_C]
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx

loc_19DF1:                              ; CODE XREF: sub_19CB8+4D↑j
                                        ; sub_19CB8+55↑j ...
                cmp     [bp+arg_4], 3
                jnz     short loc_19DFA
                inc     [bp+arg_4]

loc_19DFA:                              ; CODE XREF: sub_19CB8+13D↑j
                cmp     byte_1DC7F, 0
                jz      short loc_19E3A
                mov     byte ptr [bp+var_2], 0
                cmp     [bp+arg_4], 0
                jz      short loc_19E3A
                mov     al, [bp+arg_4]
                cbw
                mov     [bp+var_8], ax
                mov     bx, word_27842
                mov     si, [bp+var_2]
                and     si, 0FFh
                mov     cx, ax
                shr     cx, 1
                mov     di, bx
                lea     si, [bp+si+var_6]
                push    ds
                pop     es
                repne movsw
                jnb     short loc_19E2D
                movsb

loc_19E2D:                              ; CODE XREF: sub_19CB8+172↑j
                mov     al, byte ptr [bp+var_8]
                add     byte ptr [bp+var_2], al
                mov     ax, [bp+var_8]
                add     word_27842, ax

loc_19E3A:                              ; CODE XREF: sub_19CB8+147↑j
                                        ; sub_19CB8+151↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_19CB8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op31_modify_char proc near          ; CODE XREF: evt_op32_modify_all_chars+4↓p
                                        ; evt_run_script+13F↓p

var_12          = word ptr -12h
var_C           = byte ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     [bp+var_C], 0
                sub     ax, ax
                mov     [bp+var_8], ax
                mov     [bp+var_A], ax
                call    evt_read_byte
                mov     [bp+var_6], al
                mov     [bp+var_4], al
                cmp     al, 80h
                jb      short loc_19E6F
                and     [bp+var_4], 7Fh
                and     [bp+var_6], 7Fh
                mov     al, byte_1DC7F
                mov     byte ptr [bp+var_A], al

loc_19E6F:                              ; CODE XREF: evt_op31_modify_char+1F↑j
                cmp     [bp+var_6], 9
                jnz     short loc_19E85
                mov     al, byte_22D0E
                mov     [bp+var_6], al
                or      al, al
                jnz     short loc_19E85
                mov     al, byte_1DC7F
                mov     [bp+var_6], al

loc_19E85:                              ; CODE XREF: evt_op31_modify_char+33↑j
                                        ; evt_op31_modify_char+3D↑j
                mov     byte_1DC7F, 1
                mov     al, [bp+var_6]
                cmp     byte ptr g_party_size, al
                jnb     short loc_19E96
                jmp     loc_19F2C
; ---------------------------------------------------------------------------

loc_19E96:                              ; CODE XREF: evt_op31_modify_char+51↑j
                mov     byte_22D12, al
                call    evt_read_byte
                mov     byte_22D0C, al
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     byte ptr [bp+var_A], 0
                jnz     short loc_19EB6
                call    evt_read_dword24
                mov     [bp+var_A], ax
                mov     [bp+var_8], dx
                jmp     short loc_19EBB
; ---------------------------------------------------------------------------

loc_19EB6:                              ; CODE XREF: evt_op31_modify_char+69↑j
                add     word_1DC7A, 3

loc_19EBB:                              ; CODE XREF: evt_op31_modify_char+74↑j
                cmp     byte_22D12, 0
                jz      short loc_19EE6
                push    [bp+var_8]
                push    [bp+var_A]
                push    [bp+var_A]
                mov     al, byte ptr [bp+var_A]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_2]
                push    ax
                mov     al, [bp+arg_0]
                push    ax
                mov     al, byte_22D12
                push    ax
                call    sub_19CB8
                add     sp, 0Eh
                jmp     short loc_19F31
; ---------------------------------------------------------------------------
                align 2

loc_19EE6:                              ; CODE XREF: evt_op31_modify_char+80↑j
                mov     [bp+var_C], 1
                cmp     byte ptr g_party_size, 1
                jb      short loc_19F31
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_2]
                mov     di, ax
                mov     al, byte ptr [bp+var_A]
                mov     [bp+var_12], ax

loc_19F03:                              ; CODE XREF: evt_op31_modify_char+E7↓j
                push    [bp+var_8]
                push    [bp+var_A]
                push    [bp+var_A]
                push    [bp+var_12]
                push    di
                push    si
                mov     al, [bp+var_C]
                sub     ah, ah
                push    ax
                call    sub_19CB8
                add     sp, 0Eh
                inc     [bp+var_C]
                mov     al, [bp+var_C]
                cmp     byte ptr g_party_size, al
                jnb     short loc_19F03
                jmp     short loc_19F31
; ---------------------------------------------------------------------------
                align 2

loc_19F2C:                              ; CODE XREF: evt_op31_modify_char+53↑j
                add     word_1DC7A, 5

loc_19F31:                              ; CODE XREF: evt_op31_modify_char+A3↑j
                                        ; evt_op31_modify_char+AF↑j ...
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_op31_modify_char endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


evt_op32_modify_all_chars proc near     ; CODE XREF: evt_run_script:loc_1A74C↓p
                mov     ax, 1
                push    ax
                call    evt_op31_modify_char
                add     sp, 2
                retn
evt_op32_modify_all_chars endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; pos, wall byte, flag byte -> DGROUP:59D6/5AD6
; Attributes: bp-based frame

evt_op33_set_map_cell proc near         ; CODE XREF: evt_run_script:loc_1A752↓p

var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                call    evt_read_byte
                mov     [bp+var_6], al
                call    evt_read_byte
                mov     [bp+var_4], al
                call    evt_read_byte
                mov     [bp+var_2], al
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     [bp+var_8], al
                and     [bp+var_6], 0Fh
                sub     ah, ah
                mov     si, ax
                shl     si, cl
                mov     al, [bp+var_6]
                add     si, ax
                mov     al, [bp+var_4]
                mov     [si+59D6h], al
                mov     al, [bp+var_2]
                mov     [si+5AD6h], al
                or      byte_1DC80, cl
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_op33_set_map_cell endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op34_check_hour proc near           ; CODE XREF: evt_run_script:loc_1A758↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     al, byte ptr g_era
                mov     [bp+var_6], al
                call    evt_read_byte
                mov     [bp+var_4], al
                call    evt_read_byte
                mov     [bp+var_2], al
                mov     byte_1DC7F, 0
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                jb      short loc_19FC2
                mov     al, [bp+var_2]
                cmp     [bp+var_6], al
                ja      short loc_19FC2
                mov     byte_1DC7F, 1

loc_19FC2:                              ; CODE XREF: evt_op34_check_hour+23↑j
                                        ; evt_op34_check_hour+2B↑j
                mov     sp, bp
                pop     bp
                retn
evt_op34_check_hour endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op35_check_date proc near           ; CODE XREF: evt_run_script:loc_1A75E↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     byte_1DC7F, 0
                mov     bx, g_era
                shl     bx, 1
                mov     al, [bx+3A2h]
                mov     [bp+var_6], al
                call    evt_read_byte
                mov     [bp+var_4], al
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     [bp+var_4], 0B5h
                jnz     short loc_19FF8
                test    [bp+var_6], 1
                jz      short loc_1A01A
                jmp     short loc_1A016
; ---------------------------------------------------------------------------

loc_19FF8:                              ; CODE XREF: evt_op35_check_date+28↑j
                cmp     [bp+var_4], 0B6h
                jnz     short loc_1A006
                test    [bp+var_6], 1
                jnz     short loc_1A01A
                jmp     short loc_1A016
; ---------------------------------------------------------------------------

loc_1A006:                              ; CODE XREF: evt_op35_check_date+36↑j
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                jb      short loc_1A01A
                mov     al, [bp+var_2]
                cmp     [bp+var_6], al
                ja      short loc_1A01A

loc_1A016:                              ; CODE XREF: evt_op35_check_date+30↑j
                                        ; evt_op35_check_date+3E↑j
                inc     byte_1DC7F

loc_1A01A:                              ; CODE XREF: evt_op35_check_date+2E↑j
                                        ; evt_op35_check_date+3C↑j ...
                mov     sp, bp
                pop     bp
                retn
evt_op35_check_date endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op36_pay_gold proc near             ; CODE XREF: evt_run_script:loc_1A764↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_word
                mov     [bp+var_2], ax
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk_party_pay_gold
                add     sp, 4
                or      ax, ax
                jz      short loc_1A042
                mov     byte_1DC7F, 1
                jmp     short loc_1A047
; ---------------------------------------------------------------------------
                align 2

loc_1A042:                              ; CODE XREF: evt_op36_pay_gold+1A↑j
                mov     byte_1DC7F, 0

loc_1A047:                              ; CODE XREF: evt_op36_pay_gold+21↑j
                mov     sp, bp
                pop     bp
                retn
evt_op36_pay_gold endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op37_pay_gems proc near             ; CODE XREF: evt_run_script:loc_1A76A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    evt_read_byte
                sub     ah, ah
                mov     si, ax
                call    evt_read_byte
                mov     ch, al
                sub     cl, cl
                add     cx, si
                mov     [bp+var_2], cx
                push    cx
                call    thk_party_pay_gems
                add     sp, 2
                or      ax, ax
                jz      short loc_1A078
                mov     byte_1DC7F, 1
                jmp     short loc_1A07D
; ---------------------------------------------------------------------------

loc_1A078:                              ; CODE XREF: evt_op37_pay_gems+23↑j
                mov     byte_1DC7F, 0

loc_1A07D:                              ; CODE XREF: evt_op37_pay_gems+2A↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_op37_pay_gems endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_select_char proc near               ; CODE XREF: evt_run_script+16E↓p

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
                mov     ax, g_party_size
                add     ax, 30h ; '0'
                mov     [bp+var_8], ax
                mov     di, [bp+var_2]
                mov     si, [bp+var_6]

loc_1A099:                              ; CODE XREF: evt_select_char+7B↓j
                cmp     [bp+arg_0], 0
                jz      short loc_1A0B2
                cmp     g_outdoors, 0
                jnz     short loc_1A0AC
                call    view_idle_step
                jmp     short loc_1A0B5
; ---------------------------------------------------------------------------
                align 2

loc_1A0AC:                              ; CODE XREF: evt_select_char+22↑j
                call    thk_kbd_poll
                jmp     short loc_1A0B5
; ---------------------------------------------------------------------------
                align 2

loc_1A0B2:                              ; CODE XREF: evt_select_char+1B↑j
                call    thk_monster_anim_step

loc_1A0B5:                              ; CODE XREF: evt_select_char+27↑j
                                        ; evt_select_char+2D↑j
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1A0C2
                mov     ax, 1
                jmp     short loc_1A0C4
; ---------------------------------------------------------------------------
                align 2

loc_1A0C2:                              ; CODE XREF: evt_select_char+38↑j
                sub     ax, ax

loc_1A0C4:                              ; CODE XREF: evt_select_char+3D↑j
                mov     [bp+var_4], ax
                or      ax, ax
                jnz     short loc_1A0F9
                mov     ax, si
                sub     ax, 30h ; '0'
                mov     di, ax
                cmp     di, 1
                jl      short loc_1A0F4
                cmp     di, g_party_size
                jg      short loc_1A0F4
                lea     ax, [di-1]
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 81h
                jnb     short loc_1A0F4
                mov     ax, 1
                jmp     short loc_1A0F6
; ---------------------------------------------------------------------------

loc_1A0F4:                              ; CODE XREF: evt_select_char+53↑j
                                        ; evt_select_char+59↑j ...
                sub     ax, ax

loc_1A0F6:                              ; CODE XREF: evt_select_char+70↑j
                mov     [bp+var_4], ax

loc_1A0F9:                              ; CODE XREF: evt_select_char+47↑j
                cmp     [bp+var_4], 0
                jz      short loc_1A099
                mov     [bp+var_2], di
                mov     [bp+var_6], si
                cmp     si, 1Bh
                jnz     short loc_1A114
                mov     byte ptr g_party_y+1, 1
                call    evt_finish
                jmp     short loc_1A120
; ---------------------------------------------------------------------------

loc_1A114:                              ; CODE XREF: evt_select_char+86↑j
                mov     al, byte ptr [bp+var_2]
                mov     byte_1DC7F, al
                mov     byte_22D0E, al
                mov     byte_22D12, al

loc_1A120:                              ; CODE XREF: evt_select_char+90↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_select_char endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op40_take_item proc near            ; CODE XREF: evt_run_script:loc_1A77E↓p

var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     byte_1DC7F, 0
                call    evt_read_byte
                mov     [bp+var_8], al
                call    evt_read_byte
                mov     [bp+var_8], al
                mov     [bp+var_4], 0
                jmp     short loc_1A158
; ---------------------------------------------------------------------------

loc_1A146:                              ; CODE XREF: evt_op40_take_item+67↓j
                inc     si
                cmp     si, 6
                jge     short loc_1A18F
                jmp     short loc_1A170
; ---------------------------------------------------------------------------

loc_1A14E:                              ; CODE XREF: evt_op40_take_item+6C↓j
                cmp     byte_1DC7F, 0
                jnz     short loc_1A194
                inc     [bp+var_4]

loc_1A158:                              ; CODE XREF: evt_op40_take_item+1E↑j
                mov     ax, g_party_size
                cmp     [bp+var_4], ax
                jge     short loc_1A194
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                sub     si, si
                mov     di, ax

loc_1A170:                              ; CODE XREF: evt_op40_take_item+26↑j
                mov     bx, si
                add     bx, di
                mov     al, [bp+var_8]
                cmp     [bx+3Ah], al
                jnz     short loc_1A188
                inc     byte_1DC7F
                push    si
                push    di
                call    thk_char_backpack_remove
                add     sp, 4

loc_1A188:                              ; CODE XREF: evt_op40_take_item+54↑j
                cmp     byte_1DC7F, 0
                jz      short loc_1A146

loc_1A18F:                              ; CODE XREF: evt_op40_take_item+24↑j
                mov     [bp+var_6], si
                jmp     short loc_1A14E
; ---------------------------------------------------------------------------

loc_1A194:                              ; CODE XREF: evt_op40_take_item+2D↑j
                                        ; evt_op40_take_item+38↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_op40_take_item endp


; =============== S U B R O U T I N E =======================================


evt_op41_stop   proc near               ; CODE XREF: evt_run_script:loc_1A784↓p
                mov     byte ptr g_party_y+1, 1
                retn
evt_op41_stop   endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op42_place_treasure proc near       ; CODE XREF: evt_run_script:loc_1A78A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    evt_read_dword24
                mov     word_241AC, ax
                mov     word_241AE, dx
                call    evt_read_word
                mov     word_241AA, ax
                sub     si, si

loc_1A1B9:                              ; CODE XREF: evt_op42_place_treasure+32↓j
                call    evt_read_byte
                mov     [si+6950h], al
                call    evt_read_byte
                mov     [si+6956h], al
                call    evt_read_byte
                mov     [si+6953h], al
                inc     si
                cmp     si, 3
                jl      short loc_1A1B9
                mov     [bp+var_2], si
                mov     byte_1DC84, 0FFh
                pop     si
                mov     sp, bp
                pop     bp
                retn
evt_op42_place_treasure endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op43_skip_if_night proc near        ; CODE XREF: evt_run_script:loc_1A790↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                mov     [bp+var_2], al
                cmp     byte_1DD59, 0
                jz      short loc_1A1FE
                sub     ah, ah
                push    ax
                call    evt_skip_commands
                add     sp, 2

loc_1A1FE:                              ; CODE XREF: evt_op43_skip_if_night+11↑j
                mov     sp, bp
                pop     bp
                retn
evt_op43_skip_if_night endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op44_add_value proc near            ; CODE XREF: evt_run_script:loc_1A796↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    evt_read_byte
                sub     ah, ah
                mov     [bp+var_2], ax
                add     word_1DC18, ax
                or      byte_1DC80, 1
                mov     sp, bp
                pop     bp
                retn
evt_op44_add_value endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

evt_op45_check_char_class proc near     ; CODE XREF: evt_run_script:loc_1A79C↓p

var_18          = byte ptr -18h
var_16          = word ptr -16h
var_14          = byte ptr -14h
var_12          = byte ptr -12h
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
                sub     sp, 18h
                push    di
                push    si
                mov     byte ptr [bp+var_E], 0
                mov     [bp+var_18], 0
                mov     byte ptr [bp+var_6], 1
                mov     [bp+var_12], 0
                mov     [bp+var_8], 0
                call    evt_read_byte
                mov     byte ptr [bp+var_4], al
                call    evt_read_byte
                mov     byte ptr [bp+var_2], al
                test    byte ptr [bp+var_4], 80h
                jz      short loc_1A250
                inc     byte ptr [bp+var_E]

loc_1A250:                              ; CODE XREF: evt_op45_check_char_class+2D↑j
                test    byte ptr [bp+var_4], 40h
                jz      short loc_1A259
                inc     [bp+var_18]

loc_1A259:                              ; CODE XREF: evt_op45_check_char_class+36↑j
                cmp     [bp+var_18], 0
                jnz     short loc_1A265
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A268

loc_1A265:                              ; CODE XREF: evt_op45_check_char_class+3F↑j
                dec     byte ptr [bp+var_6]

loc_1A268:                              ; CODE XREF: evt_op45_check_char_class+45↑j
                test    byte ptr [bp+var_4], 20h
                jz      short loc_1A271
                inc     [bp+var_12]

loc_1A271:                              ; CODE XREF: evt_op45_check_char_class+4E↑j
                mov     al, byte ptr [bp+var_4]
                and     al, 0Fh
                mov     [bp+var_14], al
                mov     byte ptr [bp+var_10], al
                test    byte ptr [bp+var_4], 0E0h
                jnz     short loc_1A28A
                mov     al, byte ptr [bp+var_2]
                and     al, 0Fh
                mov     [bp+var_14], al

loc_1A28A:                              ; CODE XREF: evt_op45_check_char_class+62↑j
                mov     byte_1DC7F, 0
                mov     [bp+var_C], 0
                mov     di, [bp+var_A]
                mov     si, [bp+var_16]
                jmp     loc_1A349
; ---------------------------------------------------------------------------
                align 2

loc_1A29E:                              ; CODE XREF: evt_op45_check_char_class+14F↓j
                sub     ax, ax

loc_1A2A0:                              ; CODE XREF: evt_op45_check_char_class+155↓j
                mov     si, ax

loc_1A2A2:                              ; CODE XREF: evt_op45_check_char_class+144↓j
                cmp     [bp+var_18], 0

loc_1A2A6:                              ; CODE XREF: seg002:0489↑J
                jz      short loc_1A2BA
                mov     al, byte ptr [bp+var_10]
                cmp     [di+0Ch], al
                jnz     short loc_1A2B6
                mov     ax, 1
                jmp     short loc_1A2B8
; ---------------------------------------------------------------------------
                align 2

loc_1A2B6:                              ; CODE XREF: evt_op45_check_char_class+90↑j
                sub     ax, ax

loc_1A2B8:                              ; CODE XREF: evt_op45_check_char_class+95↑j
                mov     si, ax

loc_1A2BA:                              ; CODE XREF: evt_op45_check_char_class:loc_1A2A6↑j
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A2D2
                mov     al, byte ptr [bp+var_10]
                cmp     [di+0Eh], al
                jnz     short loc_1A2CE
                mov     ax, 1
                jmp     short loc_1A2D0
; ---------------------------------------------------------------------------
                align 2

loc_1A2CE:                              ; CODE XREF: evt_op45_check_char_class+A8↑j
                sub     ax, ax

loc_1A2D0:                              ; CODE XREF: evt_op45_check_char_class+AD↑j
                mov     si, ax

loc_1A2D2:                              ; CODE XREF: evt_op45_check_char_class+A0↑j
                or      si, si
                jnz     short loc_1A31E
                cmp     byte ptr [bp+var_6], 0
                jz      short loc_1A2EE
                mov     al, [bp+var_14]
                cmp     [di+0Fh], al
                jnz     short loc_1A2EA
                mov     ax, 1
                jmp     short loc_1A2EC
; ---------------------------------------------------------------------------
                align 2

loc_1A2EA:                              ; CODE XREF: evt_op45_check_char_class+C4↑j
                sub     ax, ax

loc_1A2EC:                              ; CODE XREF: evt_op45_check_char_class+C9↑j
                mov     si, ax

loc_1A2EE:                              ; CODE XREF: evt_op45_check_char_class+BC↑j
                cmp     [bp+var_18], 0
                jz      short loc_1A306
                mov     al, [bp+var_14]
                cmp     [di+0Ch], al
                jnz     short loc_1A302
                mov     ax, 1
                jmp     short loc_1A304
; ---------------------------------------------------------------------------
                align 2

loc_1A302:                              ; CODE XREF: evt_op45_check_char_class+DC↑j
                sub     ax, ax

loc_1A304:                              ; CODE XREF: evt_op45_check_char_class+E1↑j
                mov     si, ax

loc_1A306:                              ; CODE XREF: evt_op45_check_char_class+D4↑j
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A31E
                mov     al, [bp+var_14]
                cmp     [di+0Eh], al
                jnz     short loc_1A31A
                mov     ax, 1
                jmp     short loc_1A31C
; ---------------------------------------------------------------------------
                align 2

loc_1A31A:                              ; CODE XREF: evt_op45_check_char_class+F4↑j
                sub     ax, ax

loc_1A31C:                              ; CODE XREF: evt_op45_check_char_class+F9↑j
                mov     si, ax

loc_1A31E:                              ; CODE XREF: evt_op45_check_char_class+B6↑j
                                        ; evt_op45_check_char_class+EC↑j
                cmp     [bp+var_12], 0
                jnz     short loc_1A32B
                or      si, si
                jnz     short loc_1A32B
                inc     [bp+var_8]

loc_1A32B:                              ; CODE XREF: evt_op45_check_char_class+104↑j
                                        ; evt_op45_check_char_class+108↑j
                cmp     [bp+var_12], 0
                jz      short loc_1A338
                or      si, si
                jz      short loc_1A338
                inc     [bp+var_8]

loc_1A338:                              ; CODE XREF: evt_op45_check_char_class+111↑j
                                        ; evt_op45_check_char_class+115↑j
                cmp     [bp+var_8], 0
                jz      short loc_1A346

loc_1A33E:                              ; CODE XREF: evt_op45_check_char_class+131↓j
                mov     [bp+var_A], di
                mov     [bp+var_16], si
                jmp     short loc_1A376
; ---------------------------------------------------------------------------

loc_1A346:                              ; CODE XREF: evt_op45_check_char_class+11E↑j
                inc     [bp+var_C]

loc_1A349:                              ; CODE XREF: evt_op45_check_char_class+7C↑j
                mov     ax, g_party_size
                cmp     [bp+var_C], ax
                jge     short loc_1A33E
                push    [bp+var_C]
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                cmp     byte ptr [bp+var_6], 0
                jnz     short loc_1A365
                jmp     loc_1A2A2
; ---------------------------------------------------------------------------

loc_1A365:                              ; CODE XREF: evt_op45_check_char_class+142↑j
                mov     al, byte ptr [bp+var_10]
                cmp     [di+0Fh], al
                jz      short loc_1A370
                jmp     loc_1A29E
; ---------------------------------------------------------------------------

loc_1A370:                              ; CODE XREF: evt_op45_check_char_class+14D↑j
                mov     ax, 1
                jmp     loc_1A2A0
; ---------------------------------------------------------------------------

loc_1A376:                              ; CODE XREF: evt_op45_check_char_class+126↑j
                cmp     [bp+var_16], 0
                jz      short loc_1A380
                inc     byte_1DC7F

loc_1A380:                              ; CODE XREF: evt_op45_check_char_class+15C↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1A386:                              ; CODE XREF: evt_run_script:loc_1A7A2↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                call    evt_read_byte
                mov     byte ptr [bp+var_6], al
                call    evt_read_byte
                mov     byte ptr [bp+var_2], al
                mov     byte ptr [bp+var_A], 4
                mov     byte ptr [bp+var_C], 2
                cmp     byte ptr [bp+var_6], 80h
                jb      short loc_1A3B4
                mov     byte ptr [bp+var_A], 3
                mov     byte ptr [bp+var_C], 1
                and     byte ptr [bp+var_6], 7Fh

loc_1A3B4:                              ; CODE XREF: evt_op45_check_char_class+188↑j
                sub     byte ptr [bp+var_6], 6Eh ; 'n'
                mov     [bp+var_8], 0
                cmp     g_party_size, 0
                jle     short loc_1A3FE
                mov     al, byte ptr [bp+var_6]
                sub     ah, ah
                mov     [bp+var_E], ax
                mov     di, [bp+var_8]

loc_1A3CF:                              ; CODE XREF: evt_op45_check_char_class+1D8↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                mov     al, byte ptr [bp+var_A]
                cmp     [si+0Fh], al
                jz      short loc_1A3E8
                mov     al, byte ptr [bp+var_C]
                cmp     [si+0Fh], al
                jnz     short loc_1A3F1

loc_1A3E8:                              ; CODE XREF: evt_op45_check_char_class+1C0↑j
                mov     bx, [bp+var_E]
                mov     al, byte ptr [bp+var_2]
                or      [bx+si+51h], al

loc_1A3F1:                              ; CODE XREF: evt_op45_check_char_class+1C8↑j
                inc     di
                cmp     di, g_party_size
                jl      short loc_1A3CF
                mov     [bp+var_8], di
                mov     [bp+var_4], si

loc_1A3FE:                              ; CODE XREF: evt_op45_check_char_class+1A4↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1A404:                              ; CODE XREF: evt_run_script:loc_1A7A8↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_1A416:                              ; CODE XREF: evt_op45_check_char_class+20A↓j
                mov     ax, 0Ah
                push    ax
                mov     ax, 54C4h
                push    ax
                call    thk_read_string
                add     sp, 4
                mov     si, ax
                or      si, si
                jz      short loc_1A416
                mov     [bp+var_2], si
                cmp     si, 0Ah
                jz      short loc_1A44F
                mov     ax, 0Ah
                sub     ax, si
                mov     [bp+var_4], ax
                mov     bx, si
                mov     al, 20h ; ' '
                mov     cx, [bp+var_4]
                lea     di, [bx+54C4h]
                push    ds
                pop     es
                repne stosb
                mov     ax, [bp+var_4]
                add     [bp+var_2], ax

loc_1A44F:                              ; CODE XREF: evt_op45_check_char_class+212↑j
                mov     byte_22D1E, 0
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1A45A:                              ; CODE XREF: evt_run_script:loc_1A7AE↓p
                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                sub     si, si

loc_1A464:                              ; CODE XREF: evt_op45_check_char_class+250↓j
                call    evt_read_byte
                mov     byte ptr [bp+si+var_C], al
                inc     si
                cmp     si, 0Ah
                jl      short loc_1A464
                mov     [bp+var_E], si
                sub     si, si

loc_1A475:                              ; CODE XREF: evt_op45_check_char_class+290↓j
                mov     al, [si+54C4h]
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_10], ax
                mov     al, byte ptr [bp+si+var_C]
                sub     ah, ah
                sub     ax, 11Ah
                neg     ax
                mov     di, ax
                cmp     [bp+var_10], di
                jz      short loc_1A4A8

loc_1A496:                              ; CODE XREF: evt_op45_check_char_class+28E↓j
                mov     [bp+var_2], di
                mov     [bp+var_E], si
                cmp     si, 0Ah
                jnz     short loc_1A4B0
                mov     byte_1DC7F, 1
                jmp     short loc_1A4B5
; ---------------------------------------------------------------------------

loc_1A4A8:                              ; CODE XREF: evt_op45_check_char_class+276↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1A496
                jmp     short loc_1A475
; ---------------------------------------------------------------------------

loc_1A4B0:                              ; CODE XREF: evt_op45_check_char_class+281↑j
                mov     byte_1DC7F, 0

loc_1A4B5:                              ; CODE XREF: evt_op45_check_char_class+288↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1A4BC:                              ; CODE XREF: evt_run_script:loc_1A7B4↓p
                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     byte ptr [bp+var_4], 1
                mov     byte ptr [bp+var_2], 0
                or      byte_1DC80, 2
                call    evt_read_byte
                mov     byte ptr [bp+var_8], al
                call    evt_read_word
                mov     [bp+var_6], ax
                cmp     byte ptr [bp+var_8], 80h
                jb      short loc_1A4EE
                and     byte ptr [bp+var_8], 7Fh
                mov     al, byte_1DC7F
                sub     ah, ah
                mov     [bp+var_6], ax

loc_1A4EE:                              ; CODE XREF: evt_op45_check_char_class+2C2↑j
                cmp     byte ptr [bp+var_8], 0
                jnz     short loc_1A4FC
                mov     al, byte ptr g_party_size
                mov     byte ptr [bp+var_4], al
                jmp     short loc_1A52D
; ---------------------------------------------------------------------------

loc_1A4FC:                              ; CODE XREF: evt_op45_check_char_class+2D4↑j
                dec     byte ptr [bp+var_8]
                cmp     byte ptr [bp+var_8], 8
                jnz     short loc_1A520
                mov     al, byte_22D0E
                mov     byte ptr [bp+var_8], al
                or      al, al
                jnz     short loc_1A515
                mov     al, byte_1DC7F
                mov     byte ptr [bp+var_8], al

loc_1A515:                              ; CODE XREF: evt_op45_check_char_class+2EF↑j
                cmp     byte ptr [bp+var_8], 0
                jz      short loc_1A52D
                dec     byte ptr [bp+var_8]
                jmp     short loc_1A52D
; ---------------------------------------------------------------------------

loc_1A520:                              ; CODE XREF: evt_op45_check_char_class+2E5↑j
                mov     al, byte ptr [bp+var_8]
                cmp     byte ptr g_party_size, al
                ja      short loc_1A52D
                mov     byte ptr [bp+var_8], 0

loc_1A52D:                              ; CODE XREF: evt_op45_check_char_class+2DC↑j
                                        ; evt_op45_check_char_class+2FB↑j ...
                mov     si, [bp+var_6]
                jmp     short loc_1A555
; ---------------------------------------------------------------------------

loc_1A532:                              ; CODE XREF: evt_op45_check_char_class+33F↓j
                lea     ax, [bp+var_2]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                push    si
                mov     al, byte ptr [bp+var_8]
                inc     byte ptr [bp+var_8]
                sub     ah, ah
                push    ax
                call    thk_char_ptr
                add     sp, 2
                push    ax
                call    thk_char_apply_damage
                add     sp, 0Ah

loc_1A555:                              ; CODE XREF: evt_op45_check_char_class+312↑j
                mov     al, byte ptr [bp+var_4]
                dec     byte ptr [bp+var_4]
                or      al, al
                jnz     short loc_1A532
                call    thk_party_all_disabled
                or      ax, ax
                jz      short loc_1A56B
                mov     byte ptr g_party_y+1, 1

loc_1A56B:                              ; CODE XREF: evt_op45_check_char_class+346↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1A570:                              ; CODE XREF: evt_run_script:loc_1A7BA↓p
                call    evt_read_byte
                sub     ah, ah
                push    ax
                call    thk_res_36A6
                add     sp, 2
                mov     byte_1DC7F, al
                retn
evt_op45_check_char_class endp


; =============== S U B R O U T I N E =======================================

; redraw according to byte_1DC80 bits (1 message area, 2 party list, 4 view)

evt_finish      proc near               ; CODE XREF: seg002:06ED↑J
                                        ; game_main_loop+283↑p ...
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                mov     byte_277D4, 0FFh
                call    thk_end_monster_gfx
                mov     al, byte_1DC80
                and     al, 3
                cmp     al, 3
                jnz     short loc_1A5C8
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                call    thk_draw_frame_hline
                add     sp, 6
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2

loc_1A5C8:                              ; CODE XREF: evt_finish+1D↑j
                test    byte_1DC80, 1
                jz      short loc_1A5D2
                call    thk_draw_status_line

loc_1A5D2:                              ; CODE XREF: evt_finish+4D↑j
                test    byte_1DC80, 2
                jz      short loc_1A5DC
                call    thk_draw_party_list

loc_1A5DC:                              ; CODE XREF: evt_finish+57↑j
                test    byte_1DC80, 4
                jz      short loc_1A600
                call    thk_res_3FFC
                mov     ax, 7Fh
                push    ax
                mov     ax, 0D7h
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

loc_1A600:                              ; CODE XREF: evt_finish+61↑j
                mov     byte_1DC80, 0
                retn
evt_finish      endp


; =============== S U B R O U T I N E =======================================

; (script number) executes commands until 0FFh; opcode byte 1..50 dispatches through jump table
; Attributes: bp-based frame

evt_run_script  proc near               ; CODE XREF: evt_run_script+34B↓p
                                        ; evt_run_script+395↓p

var_E           = byte ptr -0Eh
var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     byte ptr g_party_y+1, 0
                mov     byte ptr [bp+var_4], 0
                cmp     byte ptr [bp+arg_0], 0
                jz      short loc_1A62F
                mov     al, byte ptr [bp+arg_0]
                cbw
                mov     si, ax
                add     byte ptr [bp+var_4], al

loc_1A625:                              ; CODE XREF: evt_run_script+24↓j
                                        ; evt_run_script+27↓j
                call    evt_read_byte
                cmp     al, 0FFh
                jnz     short loc_1A625
                dec     si
                jnz     short loc_1A625

loc_1A62F:                              ; CODE XREF: evt_run_script+14↑j
                mov     ax, word_1DC7C
                cmp     word_1DC7A, ax
                jl      short loc_1A63B
                jmp     loc_1A841
; ---------------------------------------------------------------------------

loc_1A63B:                              ; CODE XREF: evt_run_script+30↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    evt_read_byte
                mov     byte ptr [bp+var_4], al
                dec     word_1DC7A
                cmp     al, 22h ; '"'
                jz      short loc_1A663
                mov     al, byte ptr g_era
                mov     byte ptr [bp+var_2], al
                mov     al, byte_231E5
                cmp     byte ptr [bp+var_2], al
                jz      short loc_1A663
                jmp     loc_1A856
; ---------------------------------------------------------------------------

loc_1A663:                              ; CODE XREF: evt_run_script+4A↑j
                                        ; evt_run_script+58↑j ...
                call    evt_read_byte
                sub     ah, ah
                sub     ax, 1           ; switch 8 cases
                cmp     ax, 31h
                jbe     short loc_1A673
                jmp     def_1A676       ; jumptable 0001A676 default case
; ---------------------------------------------------------------------------

loc_1A673:                              ; CODE XREF: evt_run_script+68↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1A676[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1A67C:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op01_message ; jumptable 0001A676 case 1
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A682:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                mov     ax, 13h         ; jumptable 0001A676 case 2
                push    ax
                call    evt_message_at_row

loc_1A689:                              ; CODE XREF: evt_run_script+A8↓j
                                        ; evt_run_script+B6↓j ...
                add     sp, 2
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                align 2

loc_1A690:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op03_message_window ; jumptable 0001A676 case 3
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A696:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op04_title_centered ; jumptable 0001A676 case 4
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A69C:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op05_message_box ; jumptable 0001A676 case 5
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6A2:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op06_message_framed ; jumptable 0001A676 case 6
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6A8:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                sub     ax, ax          ; jumptable 0001A676 case 7
                push    ax
                call    evt_wait_key
                jmp     short loc_1A689
; ---------------------------------------------------------------------------

loc_1A6B0:                              ; CODE XREF: evt_run_script+70↑j
                                        ; DATA XREF: evt_run_script:jpt_1A676↓o
                call    evt_op08_wait_key_music ; jumptable 0001A676 case 8
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6B6:                              ; DATA XREF: evt_run_script:off_1A7D8↓o
                sub     ax, ax
                push    ax
                call    evt_ask_yes_no
                jmp     short loc_1A689
; ---------------------------------------------------------------------------

loc_1A6BE:                              ; DATA XREF: evt_run_script+1D4↓o
                call    evt_op10_ask_yes_no_music
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6C4:                              ; DATA XREF: evt_run_script+1D6↓o
                call    evt_op11_show_monster_pic
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6CA:                              ; DATA XREF: evt_run_script+1D8↓o
                call    evt_op12_teleport
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6D0:                              ; DATA XREF: evt_run_script+1DA↓o
                call    evt_op13_play_sound
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6D6:                              ; DATA XREF: evt_run_script+1DC↓o
                call    evt_op14_enter_location
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6DC:                              ; DATA XREF: evt_run_script+1DE↓o
                call    evt_op15_end
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6E2:                              ; DATA XREF: evt_run_script+1E0↓o
                call    evt_op16_skip_if_cond
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6E8:                              ; DATA XREF: evt_run_script+1E2↓o
                call    evt_op17_skip_if_not_cond
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6EE:                              ; DATA XREF: evt_run_script:off_1A7EA↓o
                sub     ax, ax
                push    ax
                call    evt_fight
                jmp     short loc_1A689
; ---------------------------------------------------------------------------

loc_1A6F6:                              ; DATA XREF: evt_run_script+1E6↓o
                call    evt_op19_fight_noflags
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6FC:                              ; DATA XREF: evt_run_script+1E8↓o
                call    evt_op20_clear_trigger
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A702:                              ; DATA XREF: evt_run_script+1EA↓o
                sub     ax, ax
                push    ax
                call    evt_op21_check_char
                jmp     loc_1A689
; ---------------------------------------------------------------------------
                align 2

loc_1A70C:                              ; DATA XREF: evt_run_script+1EC↓o
                call    evt_op22_has_item
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A712:                              ; DATA XREF: evt_run_script+1EE↓o
                call    evt_op23_get_var
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A718:                              ; DATA XREF: evt_run_script+1F0↓o
                call    evt_op24_check_char_alt
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A71E:                              ; DATA XREF: evt_run_script+1F2↓o
                call    evt_op25_give_item
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A724:                              ; DATA XREF: evt_run_script+1F4↓o
                call    evt_op26_set_var
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A72A:                              ; DATA XREF: evt_run_script+1F6↓o
                call    evt_op27_cond_limit
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A730:                              ; DATA XREF: evt_run_script+1F8↓o
                call    evt_op28_random
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A736:                              ; DATA XREF: evt_run_script+1FA↓o
                call    evt_op29_wait
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A73C:                              ; DATA XREF: evt_run_script+1FC↓o
                call    evt_op30_delay
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A742:                              ; DATA XREF: evt_run_script+1FE↓o
                sub     ax, ax
                push    ax
                call    evt_op31_modify_char
                jmp     loc_1A689
; ---------------------------------------------------------------------------
                align 2

loc_1A74C:                              ; DATA XREF: evt_run_script+200↓o
                call    evt_op32_modify_all_chars
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A752:                              ; DATA XREF: evt_run_script+202↓o
                call    evt_op33_set_map_cell
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A758:                              ; DATA XREF: evt_run_script+204↓o
                call    evt_op34_check_hour
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A75E:                              ; DATA XREF: evt_run_script+206↓o
                call    evt_op35_check_date
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A764:                              ; DATA XREF: evt_run_script+208↓o
                call    evt_op36_pay_gold
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A76A:                              ; DATA XREF: evt_run_script+20A↓o
                call    evt_op37_pay_gems
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A770:                              ; DATA XREF: evt_run_script+20C↓o
                mov     ax, 1

loc_1A773:                              ; CODE XREF: evt_run_script+176↓j
                push    ax
                call    evt_select_char
                jmp     loc_1A689
; ---------------------------------------------------------------------------

loc_1A77A:                              ; DATA XREF: evt_run_script+20E↓o
                sub     ax, ax
                jmp     short loc_1A773
; ---------------------------------------------------------------------------

loc_1A77E:                              ; DATA XREF: evt_run_script+210↓o
                call    evt_op40_take_item
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A784:                              ; DATA XREF: evt_run_script+212↓o
                call    evt_op41_stop
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A78A:                              ; DATA XREF: evt_run_script+214↓o
                call    evt_op42_place_treasure
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A790:                              ; DATA XREF: evt_run_script+216↓o
                call    evt_op43_skip_if_night
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A796:                              ; DATA XREF: evt_run_script+218↓o
                call    evt_op44_add_value
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A79C:                              ; DATA XREF: evt_run_script+21A↓o
                call    evt_op45_check_char_class
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A7A2:                              ; DATA XREF: evt_run_script+21C↓o
                call    loc_1A386
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A7A8:                              ; DATA XREF: evt_run_script+21E↓o
                call    loc_1A404
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2

loc_1A7AE:                              ; DATA XREF: evt_run_script+220↓o
                call    loc_1A45A
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2

loc_1A7B4:                              ; DATA XREF: evt_run_script+222↓o
                call    loc_1A4BC
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2

loc_1A7BA:                              ; DATA XREF: evt_run_script+224↓o
                call    loc_1A570
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2

def_1A676:                              ; CODE XREF: evt_run_script+6A↑j
                mov     byte ptr g_party_y+1, 1 ; jumptable 0001A676 default case
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2
jpt_1A676       dw offset loc_1A67C     ; DATA XREF: evt_run_script+70↑r
                dw offset loc_1A682     ; jump table for switch statement
                dw offset loc_1A690
                dw offset loc_1A696
                dw offset loc_1A69C
                dw offset loc_1A6A2
                dw offset loc_1A6A8
                dw offset loc_1A6B0
off_1A7D8       dw offset loc_1A6B6     ; CODE XREF: seg002:0495↑J
                dw offset loc_1A6BE
                dw offset loc_1A6C4
                dw offset loc_1A6CA
                dw offset loc_1A6D0
                dw offset loc_1A6D6
                dw offset loc_1A6DC
                dw offset loc_1A6E2
                dw offset loc_1A6E8
off_1A7EA       dw offset loc_1A6EE     ; CODE XREF: seg002:05B5↑J
                dw offset loc_1A6F6
                dw offset loc_1A6FC
                dw offset loc_1A702
                dw offset loc_1A70C
                dw offset loc_1A712
                dw offset loc_1A718
                dw offset loc_1A71E
                dw offset loc_1A724
                dw offset loc_1A72A
                dw offset loc_1A730
                dw offset loc_1A736
                dw offset loc_1A73C
                dw offset loc_1A742
                dw offset loc_1A74C
                dw offset loc_1A752
                dw offset loc_1A758
                dw offset loc_1A75E
                dw offset loc_1A764
                dw offset loc_1A76A
                dw offset loc_1A770
                dw offset loc_1A77A
                dw offset loc_1A77E
                dw offset loc_1A784
                dw offset loc_1A78A
                dw offset loc_1A790
                dw offset loc_1A796
                dw offset loc_1A79C
                dw offset loc_1A7A2
                dw offset loc_1A7A8
                dw offset loc_1A7AE
                dw offset loc_1A7B4
                dw offset loc_1A7BA
; ---------------------------------------------------------------------------

loc_1A82C:                              ; CODE XREF: seg002:05C1↑J
                                        ; evt_run_script+79↑j ...
                mov     bx, word_1DC7A
                cmp     byte ptr [bx+6052h], 0FFh
                jz      short loc_1A841
                cmp     byte ptr g_party_y+1, 0
                jnz     short loc_1A841
                jmp     loc_1A663
; ---------------------------------------------------------------------------

loc_1A841:                              ; CODE XREF: evt_run_script+32↑j
                                        ; evt_run_script+22F↑j ...
                mov     bx, word_1DC7A
                cmp     byte ptr [bx+6052h], 0FFh
                jnz     short loc_1A856
                cmp     byte ptr g_party_y+1, 0
                jnz     short loc_1A856
                call    evt_finish

loc_1A856:                              ; CODE XREF: evt_run_script+5A↑j
                                        ; evt_run_script+244↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

evt_load_triggers:                      ; CODE XREF: evt_run_script+2FB↓p
                                        ; evt_run_script+39E↓p
                push    bp              ; parse 3-byte trigger table (cell, script, facing mask), terminated by 0,0,0
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                sub     si, si

loc_1A865:                              ; CODE XREF: evt_run_script+273↓j
                                        ; evt_run_script+277↓j ...
                mov     al, [si+6052h]
                mov     byte ptr [bp+var_2], al
                inc     si
                mov     dl, [si+6052h]
                inc     si
                mov     cl, [si+6052h]
                inc     si
                or      al, al
                jnz     short loc_1A865
                or      dl, dl
                jnz     short loc_1A865
                or      cl, cl
                jnz     short loc_1A865
                mov     [bp+var_C], si
                mov     byte ptr [bp+var_4], dl
                mov     [bp+var_6], cl
                mov     ax, si
                mov     word_1DC7C, ax
                mov     bx, ax
                inc     [bp+var_C]
                mov     al, [bx+6052h]
                mov     byte ptr [bp+var_8], al
                mov     bx, [bp+var_C]
                inc     [bp+var_C]
                mov     al, [bx+6052h]
                mov     [bp+var_A], al
                mov     ah, al
                sub     al, al
                mov     cl, byte ptr [bp+var_8]
                sub     ch, ch
                add     ax, cx
                add     word_1DC7C, ax
                mov     ax, [bp+var_C]
                mov     word_22E0C, ax
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

evt_check_cell_triggers:                ; CODE XREF: game_main_loop+218↑p
                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                mov     [bp+var_8], 0
                mov     byte ptr [bp+var_C], 0
                mov     byte_22D0E, 0
                mov     byte_22D13, 0FFh
                mov     bl, byte_23217
                sub     bh, bh
                mov     al, [bx+16D2h]
                mov     [bp+var_A], al
                mov     al, byte ptr g_party_y
                mov     cl, 4
                shl     al, cl
                add     al, g_party_x
                mov     [bp+var_E], al
                cmp     word_1DC7C, 0FFFFh
                jnz     short loc_1A904
                call    evt_load_triggers

loc_1A904:                              ; CODE XREF: evt_run_script+2F9↑j
                mov     ax, word_22E0C
                mov     word_1DC7A, ax
                sub     al, al
                mov     byte_1DC80, al
                mov     byte ptr g_party_y+1, al
                mov     byte_277D4, 0FFh
                mov     al, [bp+var_A]
                sub     ah, ah
                mov     di, ax
                mov     si, [bp+var_8]

loc_1A921:                              ; CODE XREF: evt_run_script+360↓j
                                        ; evt_run_script+366↓j ...
                mov     al, [si+6052h]
                mov     byte ptr [bp+var_2], al
                inc     si
                mov     al, [si+6052h]
                mov     byte ptr [bp+var_4], al
                inc     si
                mov     al, [si+6052h]
                mov     [bp+var_6], al
                inc     si
                mov     al, [bp+var_E]
                cmp     byte ptr [bp+var_2], al
                jnz     short loc_1A962
                inc     byte ptr [bp+var_C]
                mov     al, [bp+var_6]
                sub     ah, ah
                test    ax, di
                jz      short loc_1A962
                mov     al, byte ptr [bp+var_4]
                push    ax
                call    evt_run_script
                add     sp, 2
                sub     al, al
                mov     [bp+var_6], al
                mov     byte ptr [bp+var_4], al
                mov     byte ptr [bp+var_2], al

loc_1A962:                              ; CODE XREF: evt_run_script+339↑j
                                        ; evt_run_script+345↑j
                cmp     byte ptr [bp+var_2], 0
                jnz     short loc_1A921
                cmp     byte ptr [bp+var_4], 0
                jnz     short loc_1A921
                cmp     [bp+var_6], 0
                jnz     short loc_1A921
                mov     [bp+var_8], si
                cmp     byte_22D13, 0FFh
                jz      short loc_1A9A7
                mov     ah, byte ptr word_238A2+1
                sub     al, al
                mov     cl, byte ptr word_238A2
                sub     ch, ch
                add     ax, cx
                mov     word_1DC7C, ax
                mov     word_1DC7A, 2
                mov     al, byte_22D13
                sub     ah, ah
                push    ax
                call    evt_run_script
                add     sp, 2
                call    thk_load_map_events
                call    evt_load_triggers

loc_1A9A7:                              ; CODE XREF: evt_run_script+376↑j
                cmp     byte ptr [bp+var_C], 0
                jnz     short loc_1A9F9
                mov     [bp+var_8], 0
                sub     ax, ax
                mov     cx, 5
                mov     di, 9680h
                push    ds
                pop     es
                repne stosw
                stosb
                add     [bp+var_8], 0Bh
                mov     byte_1DD58, 0
                mov     byte_1DC65, 0
                call    thk_start_combat
                mov     al, byte ptr g_party_y
                sub     ah, ah
                mov     si, ax
                mov     cl, 4
                shl     si, cl
                mov     al, g_party_x
                add     si, ax
                mov     al, [bp+var_E]
                cmp     ax, si
                jnz     short loc_1A9F4
                and     byte_23218, 7Fh
                and     byte ptr [si+5AD6h], 7Fh
                jmp     short loc_1A9F9
; ---------------------------------------------------------------------------
                align 2

loc_1A9F4:                              ; CODE XREF: evt_run_script+3DF↑j
                mov     byte_1DC7E, 1

loc_1A9F9:                              ; CODE XREF: evt_run_script+3A5↑j
                                        ; evt_run_script+3EB↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1AA00:                              ; CODE XREF: sub_199B8+3A↑p
                                        ; sub_19CB8+22↑p
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte_27841, 1
                cmp     [bp+arg_2], 31h ; '1'
                jz      short loc_1AA18
                cmp     [bp+arg_2], 3Eh ; '>'
                jnz     short loc_1AA1D

loc_1AA18:                              ; CODE XREF: evt_run_script+40A↑j
                mov     byte_27841, 4

loc_1AA1D:                              ; CODE XREF: evt_run_script+410↑j
                cmp     [bp+arg_2], 20h ; ' '
                jz      short loc_1AA41
                cmp     [bp+arg_2], 28h ; '('
                jz      short loc_1AA41
                cmp     [bp+arg_2], 35h ; '5'
                jz      short loc_1AA41
                cmp     [bp+arg_2], 38h ; '8'
                jz      short loc_1AA41
                cmp     [bp+arg_2], 3Ah ; ':'
                jz      short loc_1AA41
                cmp     [bp+arg_2], 3Ch ; '<'
                jnz     short loc_1AA46

loc_1AA41:                              ; CODE XREF: evt_run_script+41B↑j
                                        ; evt_run_script+421↑j ...
                mov     byte_27841, 2

loc_1AA46:                              ; CODE XREF: evt_run_script+439↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                cmp     ax, 7Fh         ; switch 128 cases
                jbe     short loc_1AA53
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA53:                              ; CODE XREF: evt_run_script+448↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1AA56[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1AA5C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                push    [bp+arg_0]      ; jumptable 0001AA56 case 0
                call    loc_1B0B2
                add     sp, 2
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA68:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                push    [bp+arg_0]      ; jumptable 0001AA56 case 1
                call    loc_1B0B2
                add     sp, 2
                cmp     byte_27840, 18h
                jnb     short loc_1AA7B
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA7B:                              ; CODE XREF: evt_run_script+470↑j
                mov     byte_27840, 80h
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------
                align 2

loc_1AA84:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 2

loc_1AA87:                              ; CODE XREF: evt_run_script+48C↓j
                                        ; evt_run_script+494↓j ...
                mov     word_27842, ax
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------
                align 2

loc_1AA8E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 3
                inc     ax
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AA94:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 4
                add     ax, 2
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AA9C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 5
                add     ax, 3
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAA4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 6
                add     ax, 4
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAAC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 7
                add     ax, 5
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAB4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 8
                add     ax, 6
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AABC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 9
                add     ax, 7
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAC4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 10
                add     ax, 8
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AACC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 11
                add     ax, 9
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAD4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 12
                add     ax, 0Ch
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AADC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 13
                add     ax, 0Dh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAE4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 14
                add     ax, 0Eh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAEC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 15
                add     ax, 0Fh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAF4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 16
                add     ax, 10h
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAFC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 17
                add     ax, 11h
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AB04:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 18
                add     ax, 12h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB0E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 19
                add     ax, 13h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB18:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 20
                add     ax, 14h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB22:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 22
                add     ax, 16h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB2C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 23
                add     ax, 17h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB36:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 24
                add     ax, 18h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB40:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 25
                add     ax, 19h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB4A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 26
                add     ax, 1Ah
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB54:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 27
                add     ax, 1Bh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB5E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 28
                add     ax, 1Ch
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB68:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 29
                add     ax, 1Dh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB72:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 30
                add     ax, 1Eh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB7C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 31
                add     ax, 1Fh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB86:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 32,33
                add     ax, 74h ; 't'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB90:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 34
                add     ax, 6Bh ; 'k'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB9A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 35
                add     ax, 6Eh ; 'n'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABA4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 36
                add     ax, 6Fh ; 'o'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABAE:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 37
                add     ax, 6Ah ; 'j'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABB8:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 38
                add     ax, 71h ; 'q'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABC2:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 39
                add     ax, 72h ; 'r'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABCC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 40,41
                add     ax, 58h ; 'X'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABD6:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 42
                add     ax, 73h ; 's'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABE0:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 43
                add     ax, 6Ch ; 'l'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABEA:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 44
                add     ax, 6Dh ; 'm'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABF4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 45
                add     ax, 70h ; 'p'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABFE:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 21,46
                add     ax, 15h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC08:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 47
                add     ax, 21h ; '!'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC12:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 48
                add     ax, 22h ; '"'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC1C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 49-52
                add     ax, 62h ; 'b'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC26:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 53,54
                add     ax, 5Ah ; 'Z'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC30:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 55
                add     ax, 23h ; '#'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC3A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 56,57
                add     ax, 5Ch ; '\'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC44:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 58,59
                add     ax, 5Eh ; '^'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC4E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 60,61
                add     ax, 60h ; '`'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC58:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 62-64
                add     ax, 66h ; 'f'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC62:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 65
                add     ax, 24h ; '$'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC6C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 66
                add     ax, 25h ; '%'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC76:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 67
                add     ax, 26h ; '&'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC80:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 68
                add     ax, 27h ; '''
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC8A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 69
                add     ax, 28h ; '('
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC94:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 70
                add     ax, 29h ; ')'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC9E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 71
                add     ax, 2Ah ; '*'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACA8:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 72
                add     ax, 2Bh ; '+'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACB2:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 73
                add     ax, 2Ch ; ','
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACBC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 74
                add     ax, 2Dh ; '-'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACC6:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 75
                add     ax, 3Ah ; ':'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACD0:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 76
                add     ax, 3Bh ; ';'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACDA:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 77
                add     ax, 3Ch ; '<'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACE4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 78
                add     ax, 3Dh ; '='
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACEE:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 79
                add     ax, 3Eh ; '>'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACF8:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 80
                add     ax, 3Fh ; '?'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD02:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 81
                add     ax, 2Eh ; '.'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD0C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 82
                add     ax, 2Fh ; '/'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD16:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 83
                add     ax, 30h ; '0'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD20:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 84
                add     ax, 31h ; '1'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD2A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 85
                add     ax, 32h ; '2'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD34:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 86
                add     ax, 33h ; '3'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD3E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 87
                add     ax, 40h ; '@'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD48:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 88
                add     ax, 41h ; 'A'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD52:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 89
                add     ax, 42h ; 'B'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD5C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 90
                add     ax, 43h ; 'C'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD66:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 91
                add     ax, 44h ; 'D'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD70:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 92
                add     ax, 45h ; 'E'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD7A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 93
                add     ax, 34h ; '4'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD84:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 94
                add     ax, 35h ; '5'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD8E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 95
                add     ax, 36h ; '6'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD98:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 96
                add     ax, 37h ; '7'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADA2:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 97
                add     ax, 38h ; '8'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADAC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 98
                add     ax, 39h ; '9'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADB6:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 99
                add     ax, 46h ; 'F'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADC0:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 100
                add     ax, 47h ; 'G'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADCA:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 101
                add     ax, 48h ; 'H'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADD4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 102
                add     ax, 49h ; 'I'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADDE:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 103
                add     ax, 4Ah ; 'J'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADE8:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 104
                add     ax, 4Bh ; 'K'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADF2:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 105
                add     ax, 4Ch ; 'L'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADFC:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 106
                add     ax, 4Dh ; 'M'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE06:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 107
                add     ax, 4Eh ; 'N'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE10:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 108
                add     ax, 4Fh ; 'O'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE1A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 109
                add     ax, 50h ; 'P'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE24:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 110
                add     ax, 51h ; 'Q'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE2E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 111
                add     ax, 52h ; 'R'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE38:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 112
                add     ax, 53h ; 'S'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE42:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 113
                add     ax, 54h ; 'T'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE4C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 114
                add     ax, 55h ; 'U'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE56:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 115
                add     ax, 56h ; 'V'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE60:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 116
                add     ax, 79h ; 'y'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE6A:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 117
                add     ax, 7Ah ; 'z'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE74:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 118
                add     ax, 7Bh ; '{'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE7E:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 119,120
                add     ax, 76h ; 'v'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE88:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 121
                add     ax, 78h ; 'x'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE92:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 122
                add     ax, 7Ch ; '|'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE9C:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 123
                add     ax, 7Dh ; '}'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEA6:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 124
                add     ax, 7Eh ; '~'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEB0:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 125
                add     ax, 7Fh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEBA:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 126
                add     ax, 80h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEC4:                              ; CODE XREF: evt_run_script+450↑j
                                        ; DATA XREF: evt_run_script:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 127
                add     ax, 81h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2
jpt_1AA56       dw offset loc_1AA5C, offset loc_1AA68, offset loc_1AA84
                                        ; DATA XREF: evt_run_script+450↑r
                dw offset loc_1AA8E, offset loc_1AA94, offset loc_1AA9C ; jump table for switch statement
                dw offset loc_1AAA4, offset loc_1AAAC, offset loc_1AAB4
                dw offset loc_1AABC, offset loc_1AAC4, offset loc_1AACC
                dw offset loc_1AAD4, offset loc_1AADC, offset loc_1AAE4
                dw offset loc_1AAEC, offset loc_1AAF4, offset loc_1AAFC
                dw offset loc_1AB04, offset loc_1AB0E, offset loc_1AB18
                dw offset loc_1ABFE, offset loc_1AB22, offset loc_1AB2C
                dw offset loc_1AB36, offset loc_1AB40, offset loc_1AB4A
                dw offset loc_1AB54, offset loc_1AB5E, offset loc_1AB68
                dw offset loc_1AB72, offset loc_1AB7C, offset loc_1AB86
                dw offset loc_1AB86, offset loc_1AB90, offset loc_1AB9A
                dw offset loc_1ABA4, offset loc_1ABAE, offset loc_1ABB8
                dw offset loc_1ABC2, offset loc_1ABCC, offset loc_1ABCC
                dw offset loc_1ABD6, offset loc_1ABE0, offset loc_1ABEA
                dw offset loc_1ABF4, offset loc_1ABFE, offset loc_1AC08
                dw offset loc_1AC12, offset loc_1AC1C, offset loc_1AC1C
                dw offset loc_1AC1C, offset loc_1AC1C, offset loc_1AC26
                dw offset loc_1AC26, offset loc_1AC30, offset loc_1AC3A
                dw offset loc_1AC3A, offset loc_1AC44, offset loc_1AC44
                dw offset loc_1AC4E, offset loc_1AC4E, offset loc_1AC58
                dw offset loc_1AC58, offset loc_1AC58, offset loc_1AC62
                dw offset loc_1AC6C, offset loc_1AC76, offset loc_1AC80
                dw offset loc_1AC8A, offset loc_1AC94, offset loc_1AC9E
                dw offset loc_1ACA8, offset loc_1ACB2, offset loc_1ACBC
                dw offset loc_1ACC6, offset loc_1ACD0, offset loc_1ACDA
                dw offset loc_1ACE4, offset loc_1ACEE, offset loc_1ACF8
                dw offset loc_1AD02, offset loc_1AD0C, offset loc_1AD16
                dw offset loc_1AD20, offset loc_1AD2A, offset loc_1AD34
                dw offset loc_1AD3E, offset loc_1AD48, offset loc_1AD52
                dw offset loc_1AD5C, offset loc_1AD66, offset loc_1AD70
                dw offset loc_1AD7A, offset loc_1AD84, offset loc_1AD8E
                dw offset loc_1AD98, offset loc_1ADA2, offset loc_1ADAC
                dw offset loc_1ADB6, offset loc_1ADC0, offset loc_1ADCA
                dw offset loc_1ADD4, offset loc_1ADDE, offset loc_1ADE8
                dw offset loc_1ADF2, offset loc_1ADFC, offset loc_1AE06
                dw offset loc_1AE10, offset loc_1AE1A, offset loc_1AE24
                dw offset loc_1AE2E, offset loc_1AE38, offset loc_1AE42
                dw offset loc_1AE4C, offset loc_1AE56, offset loc_1AE60
                dw offset loc_1AE6A, offset loc_1AE74, offset loc_1AE7E
                dw offset loc_1AE7E, offset loc_1AE88, offset loc_1AE92
                dw offset loc_1AE9C, offset loc_1AEA6, offset loc_1AEB0
                dw offset loc_1AEBA, offset loc_1AEC4
; ---------------------------------------------------------------------------

def_1AA56:                              ; CODE XREF: evt_run_script+44A↑j
                                        ; evt_run_script+45F↑j ...
                mov     ax, word_27842  ; jumptable 0001AA56 default case
                mov     [bp+var_2], ax
                cmp     [bp+arg_2], 21h ; '!'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 29h ; ')'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 32h ; '2'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 35h ; '5'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 39h ; '9'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 3Bh ; ';'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 3Dh ; '='
                jz      short loc_1B00A
                cmp     [bp+arg_2], 3Fh ; '?'
                jz      short loc_1B00A
                cmp     [bp+arg_2], 78h ; 'x'
                jnz     short loc_1B00E

loc_1B00A:                              ; CODE XREF: evt_run_script+9D2↑j
                                        ; evt_run_script+9D8↑j ...
                inc     word_27842

loc_1B00E:                              ; CODE XREF: evt_run_script+A02↑j
                cmp     [bp+arg_2], 33h ; '3'
                jz      short loc_1B01A
                cmp     [bp+arg_2], 40h ; '@'
                jnz     short loc_1B01F

loc_1B01A:                              ; CODE XREF: evt_run_script+A0C↑j
                add     word_27842, 2

loc_1B01F:                              ; CODE XREF: evt_run_script+A12↑j
                cmp     [bp+arg_2], 34h ; '4'
                jnz     short loc_1B02A
                add     word_27842, 3

loc_1B02A:                              ; CODE XREF: evt_run_script+A1D↑j
                mov     ax, word_27842
                cmp     [bp+var_2], ax
                jz      short loc_1B037
                mov     byte_27841, 1

loc_1B037:                              ; CODE XREF: evt_run_script+A2A↑j
                cmp     byte_27841, 1
                jnz     short loc_1B04A
                mov     bx, word_27842
                mov     al, [bx]
                mov     byte_27840, al
                jmp     short loc_1B0AD
; ---------------------------------------------------------------------------
                align 2

loc_1B04A:                              ; CODE XREF: evt_run_script+A36↑j
                cmp     byte_27841, 2
                jnz     short loc_1B066
                mov     bx, word_27842
                mov     ah, [bx+1]
                sub     al, al
                mov     cl, [bx]
                sub     ch, ch
                add     ax, cx
                mov     word_2783E, ax
                jmp     short loc_1B0AD
; ---------------------------------------------------------------------------
                align 2

loc_1B066:                              ; CODE XREF: evt_run_script+A49↑j
                mov     bx, word_27842
                mov     ah, [bx+1]
                sub     al, al
                sub     dx, dx
                mov     cl, [bx+2]
                sub     ch, ch
                mov     bx, cx
                sub     cx, cx
                add     ax, cx
                adc     dx, bx
                mov     bx, word_27842
                mov     cl, [bx+3]
                sub     ch, ch
                sub     bx, bx
                mov     si, cx
                mov     cl, 18h

loc_1B08D:                              ; CODE XREF: evt_run_script+A8D↓j
                shl     si, 1
                rcl     bx, 1
                dec     cl
                jnz     short loc_1B08D
                add     ax, si
                adc     dx, bx
                mov     bx, word_27842
                mov     cl, [bx]
                sub     ch, ch
                add     ax, cx
                adc     dx, 0
                mov     word_2783A, ax
                mov     word_2783C, dx

loc_1B0AD:                              ; CODE XREF: evt_run_script+A41↑j
                                        ; evt_run_script+A5D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1B0B2:                              ; CODE XREF: evt_run_script+459↑p
                                        ; evt_run_script+465↑p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     dl, 0FFh
                sub     cx, cx
                mov     si, 7E20h

loc_1B0C1:                              ; CODE XREF: evt_run_script+AE2↓j
                mov     di, si
                cmp     [bp+arg_0], di
                jnz     short loc_1B0CC
                mov     ax, cx
                mov     dl, al

loc_1B0CC:                              ; CODE XREF: evt_run_script+AC0↑j
                cmp     dl, 0FFh
                jz      short loc_1B0DE

loc_1B0D1:                              ; CODE XREF: evt_run_script+AE0↓j
                mov     [bp+var_2], di
                mov     byte_27840, dl
                mov     [bp+var_4], cx
                jmp     short loc_1B0EA
; ---------------------------------------------------------------------------
                align 2

loc_1B0DE:                              ; CODE XREF: evt_run_script+AC9↑j
                add     si, 82h
                inc     cx
                cmp     cx, 30h ; '0'
                jge     short loc_1B0D1
                jmp     short loc_1B0C1
; ---------------------------------------------------------------------------

loc_1B0EA:                              ; CODE XREF: evt_run_script+AD5↑j
                mov     word_27842, 9FF0h
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
evt_run_script  endp


; =============== S U B R O U T I N E =======================================


view_free_resources proc near           ; CODE XREF: seg002:0909↑J
                                        ; enter_map+98↓p
                mov     ax, word_1DBBE
                or      ax, word_1DBC0
                jz      short loc_1B129
                push    word_1DBC8
                push    word_1DBC6
                call    thk_free_far_block
                add     sp, 4
                push    word_1DBC4
                push    word_1DBC2
                call    thk_free_far_block
                add     sp, 4
                push    word_1DBC0
                push    word_1DBBE
                call    thk_free_far_block
                add     sp, 4

loc_1B129:                              ; CODE XREF: view_free_resources+7↑j
                mov     ax, word_1DBCE
                or      ax, word_1DBD0
                jz      short loc_1B14E
                push    word_1DBB8
                push    word_1DBB6
                call    thk_free_far_block
                add     sp, 4
                push    word_1DBD0
                push    word_1DBCE
                call    thk_free_far_block
                add     sp, 4

loc_1B14E:                              ; CODE XREF: view_free_resources+3A↑j
                mov     ax, word_1DBB2
                or      ax, word_1DBB4
                jz      short loc_1B165
                push    word_1DBB4
                push    word_1DBB2
                call    thk_free_far_block
                add     sp, 4

loc_1B165:                              ; CODE XREF: view_free_resources+5F↑j
                mov     ax, word_1DBBA
                or      ax, word_1DBBC
                jz      short loc_1B17C
                push    word_1DBBC
                push    word_1DBBA
                call    thk_free_far_block
                add     sp, 4

loc_1B17C:                              ; CODE XREF: view_free_resources+76↑j
                sub     ax, ax
                cwd
                mov     word_1DBC6, ax
                mov     word_1DBC8, dx
                mov     word_1DBC2, ax
                mov     word_1DBC4, dx
                mov     word_1DBBE, ax
                mov     word_1DBC0, dx
                cwd
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                mov     word_1DBCE, ax
                mov     word_1DBD0, dx
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                mov     byte_1DBE8, 0
                retn
view_free_resources endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

load_image_retry proc near              ; CODE XREF: view_load_sky+93↓p
                                        ; view_load_style_graphics+62↓p ...

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     si, [bp+arg_0]

loc_1B1BA:                              ; CODE XREF: load_image_retry+19↓j
                push    si
                call    thk_gfx_load_image
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                or      ax, dx
                jz      short loc_1B1BA
                mov     ax, [bp+var_4]
                pop     si
                mov     sp, bp
                pop     bp
                retn
load_image_retry endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_load_sky   proc near               ; CODE XREF: seg002:0921↑J
                                        ; enter_map+CA↓p

var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                mov     [bp+var_2], ax
                mov     al, byte_231DA
                and     al, 0Fh
                mov     [bp+var_8], al
                mov     al, byte_1EF2A
                cmp     [bp+var_8], al
                jz      short loc_1B270
                mov     ax, word_1DBCA
                or      ax, word_1DBCC
                jz      short loc_1B20C
                push    word_1DBCC
                push    word_1DBCA
                call    thk_free_far_block
                add     sp, 4

loc_1B20C:                              ; CODE XREF: view_load_sky+28↑j
                cmp     g_map_id, 29h ; ')'
                jb      short loc_1B227
                cmp     g_map_id, 2Ch ; ','
                ja      short loc_1B227
                mov     bl, g_map_id
                sub     bh, bh
                mov     al, [bx+16B3h]
                mov     [bp+var_8], al

loc_1B227:                              ; CODE XREF: view_load_sky+3D↑j
                                        ; view_load_sky+44↑j
                cmp     [bp+var_8], 9
                jnz     short loc_1B234
                mov     ax, word_1DD48
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1B234:                              ; CODE XREF: view_load_sky+57↑j
                cmp     [bp+var_8], 0Bh
                jnz     short loc_1B240
                mov     ax, word_1DD4E
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                align 2

loc_1B240:                              ; CODE XREF: view_load_sky+64↑j
                cmp     [bp+var_8], 0Ch
                jnz     short loc_1B24C
                mov     ax, word_1DD4C
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                align 2

loc_1B24C:                              ; CODE XREF: view_load_sky+70↑j
                cmp     [bp+var_8], 0Ah
                jnz     short loc_1B258
                mov     ax, word_1DD4A

loc_1B255:                              ; CODE XREF: view_load_sky+5C↑j
                                        ; view_load_sky+69↑j ...
                mov     [bp+var_2], ax

loc_1B258:                              ; CODE XREF: view_load_sky+7C↑j
                mov     al, [bp+var_8]
                mov     byte_1EF2A, al
                cmp     [bp+var_2], 0
                jz      short loc_1B27D
                push    [bp+var_2]
                call    load_image_retry
                add     sp, 2
                jmp     short loc_1B277
; ---------------------------------------------------------------------------
                align 2

loc_1B270:                              ; CODE XREF: view_load_sky+1F↑j
                mov     ax, word_1DBCA
                mov     dx, word_1DBCC

loc_1B277:                              ; CODE XREF: view_load_sky+99↑j
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx

loc_1B27D:                              ; CODE XREF: view_load_sky+8E↑j
                mov     ax, [bp+var_6]
                mov     dx, [bp+var_4]
                mov     sp, bp
                pop     bp
                retn
view_load_sky   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; per map style
; Attributes: bp-based frame

view_load_style_graphics proc near      ; CODE XREF: seg002:092D↑J
                                        ; enter_map+B5↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                cmp     g_outdoors, 0
                jnz     short loc_1B2B6
                mov     ax, word_1DBCA
                or      ax, word_1DBCC
                jz      short loc_1B2B6
                push    word_1DBCC
                push    word_1DBCA
                call    thk_free_far_block
                add     sp, 4
                sub     ax, ax
                mov     word_1DBCC, ax
                mov     word_1DBCA, ax
                mov     byte_1EF2A, 0FFh

loc_1B2B6:                              ; CODE XREF: view_load_style_graphics+8↑j
                                        ; view_load_style_graphics+11↑j
                cmp     g_outdoors, 0
                jnz     short loc_1B2CC
                cmp     byte_1DB96, 0Fh
                jnz     short loc_1B2CC
                mov     g_disk_needed, 1
                jmp     short loc_1B2D2
; ---------------------------------------------------------------------------

loc_1B2CC:                              ; CODE XREF: view_load_style_graphics+33↑j
                                        ; view_load_style_graphics+3A↑j
                mov     g_disk_needed, 2

loc_1B2D2:                              ; CODE XREF: view_load_style_graphics+42↑j
                mov     ax, [bp+arg_0]
                cmp     ax, 6           ; switch 7 cases
                jbe     short loc_1B2DD
                jmp     def_1B2E0       ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------

loc_1B2DD:                              ; CODE XREF: view_load_style_graphics+50↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1B2E0[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1B2E6:                              ; CODE XREF: view_load_style_graphics+58↑j
                                        ; DATA XREF: view_load_style_graphics:jpt_1B2E0↓o
                push    word_1DD2A      ; jumptable 0001B2E0 case 0
                call    load_image_retry
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD26
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD28
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD2C

loc_1B31D:                              ; CODE XREF: view_load_style_graphics+DD↓j
                                        ; view_load_style_graphics+117↓j
                call    load_image_retry
                add     sp, 2
                mov     word_1DBCE, ax
                mov     word_1DBD0, dx
                jmp     def_1B2E0       ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------
                align 2

loc_1B32E:                              ; CODE XREF: view_load_style_graphics+58↑j
                                        ; DATA XREF: view_load_style_graphics+174↓o
                push    word_1DD32      ; jumptable 0001B2E0 case 1
                call    load_image_retry
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD2E
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD30
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD34
                jmp     short loc_1B31D
; ---------------------------------------------------------------------------
                align 2

loc_1B368:                              ; CODE XREF: view_load_style_graphics+58↑j
                                        ; DATA XREF: view_load_style_graphics+176↓o ...
                push    word_1DD3A      ; jumptable 0001B2E0 cases 2,5
                call    load_image_retry
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD36
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD38
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD3C
                jmp     loc_1B31D
; ---------------------------------------------------------------------------

loc_1B3A2:                              ; CODE XREF: view_load_style_graphics+58↑j
                                        ; DATA XREF: view_load_style_graphics+178↓o ...
                push    word_1DD44      ; jumptable 0001B2E0 cases 3,4,6
                call    load_image_retry
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD46
                call    load_image_retry
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD3E
                call    load_image_retry
                add     sp, 2
                mov     word_1DBBE, ax
                mov     word_1DBC0, dx
                push    word_1DD40
                call    load_image_retry
                add     sp, 2
                mov     word_1DBC2, ax
                mov     word_1DBC4, dx
                push    word_1DD42
                call    load_image_retry
                add     sp, 2
                mov     word_1DBC6, ax
                mov     word_1DBC8, dx
                jmp     short def_1B2E0 ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------
                align 2
jpt_1B2E0       dw offset loc_1B2E6     ; DATA XREF: view_load_style_graphics+58↑r
                                        ; jump table for switch statement
                dw offset loc_1B32E     ; jumptable 0001B2E0 case 1
                dw offset loc_1B368     ; jumptable 0001B2E0 cases 2,5
                dw offset loc_1B3A2     ; jumptable 0001B2E0 cases 3,4,6
                dw offset loc_1B3A2     ; jumptable 0001B2E0 cases 3,4,6
                dw offset loc_1B368     ; jumptable 0001B2E0 cases 2,5
                dw offset loc_1B3A2     ; jumptable 0001B2E0 cases 3,4,6
; ---------------------------------------------------------------------------

def_1B2E0:                              ; CODE XREF: view_load_style_graphics+52↑j
                                        ; view_load_style_graphics+A2↑j ...
                mov     g_disk_needed, 2 ; jumptable 0001B2E0 default case
                pop     bp
                retn
view_load_style_graphics endp


; =============== S U B R O U T I N E =======================================

; DGROUP:16E0/16E8/16F0 range tables
; Attributes: bp-based frame

map_style_for_id proc near              ; CODE XREF: enter_map+22↓p

var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     cl, 7
                sub     si, si
                mov     dl, [bp+arg_0]

loc_1B41E:                              ; CODE XREF: map_style_for_id+32↓j
                cmp     [si+16E8h], dl
                ja      short loc_1B42E
                cmp     [si+16F0h], dl
                jb      short loc_1B42E
                mov     cl, [si+16E0h]

loc_1B42E:                              ; CODE XREF: map_style_for_id+12↑j
                                        ; map_style_for_id+18↑j
                cmp     cl, 7
                jz      short loc_1B43C

loc_1B433:                              ; CODE XREF: map_style_for_id+30↓j
                mov     [bp+var_4], si
                mov     [bp+var_2], cl
                jmp     short loc_1B444
; ---------------------------------------------------------------------------
                align 2

loc_1B43C:                              ; CODE XREF: map_style_for_id+21↑j
                inc     si
                cmp     si, 7
                jge     short loc_1B433
                jmp     short loc_1B41E
; ---------------------------------------------------------------------------

loc_1B444:                              ; CODE XREF: map_style_for_id+29↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
map_style_for_id endp


; =============== S U B R O U T I N E =======================================

; samples 4 map bytes (with edge wrap into neighbouring outdoor maps 5BD6/5CD6/5DD8/5ED8)
; Attributes: bp-based frame

view_sample_row proc near               ; CODE XREF: view_prepare_visible_cells+9A↓p
                                        ; view_prepare_visible_cells+C0↓p ...

var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = byte ptr  6
arg_4           = byte ptr  8
arg_6           = byte ptr  0Ah
arg_8           = byte ptr  0Ch

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_4], 4
                mov     di, 4
                mov     si, [bp+arg_0]

loc_1B461:                              ; CODE XREF: view_sample_row+83↓j
                mov     al, [bp+arg_2]
                mov     [bp+var_6], al
                mov     al, [bp+arg_4]
                mov     [bp+var_8], al
                mov     dx, 59D6h
                cmp     [bp+var_6], 0Fh
                jbe     short loc_1B482
                cmp     [bp+var_6], 13h
                jnb     short loc_1B482
                mov     dx, 5DD8h
                jmp     short loc_1B48B
; ---------------------------------------------------------------------------
                align 2

loc_1B482:                              ; CODE XREF: view_sample_row+26↑j
                                        ; view_sample_row+2C↑j
                cmp     [bp+var_6], 0FCh
                jbe     short loc_1B48B
                mov     dx, 5ED8h

loc_1B48B:                              ; CODE XREF: view_sample_row+31↑j
                                        ; view_sample_row+38↑j
                cmp     [bp+var_8], 0Fh
                jbe     short loc_1B49C
                cmp     [bp+var_8], 13h
                jnb     short loc_1B49C
                mov     dx, 5BD6h
                jmp     short loc_1B4A5
; ---------------------------------------------------------------------------

loc_1B49C:                              ; CODE XREF: view_sample_row+41↑j
                                        ; view_sample_row+47↑j
                cmp     [bp+var_8], 0FCh
                jbe     short loc_1B4A5
                mov     dx, 5CD6h

loc_1B4A5:                              ; CODE XREF: view_sample_row+4C↑j
                                        ; view_sample_row+52↑j
                and     [bp+var_6], 0Fh
                and     [bp+var_8], 0Fh
                mov     bl, [bp+var_8]
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                mov     al, [bp+var_6]
                sub     ah, ah
                add     bx, ax
                add     bx, dx
                mov     al, [bx]
                mov     [si], al
                inc     si
                mov     al, [bp+arg_6]
                add     [bp+arg_2], al
                mov     al, [bp+arg_8]
                add     [bp+arg_4], al
                dec     di
                jnz     short loc_1B461
                mov     [bp+arg_0], si
                mov     [bp+var_2], dx
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_sample_row endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

view_prepare_visible_cells proc near    ; CODE XREF: enter_map+135↓p
                                        ; map_edge_transition:loc_1B7C0↓p

var_E           = word ptr -0Eh
var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                cmp     g_facing, 4Eh ; 'N'
                jnz     short loc_1B4F6
                mov     [bp+var_E], 16F8h
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B4F6:                              ; CODE XREF: view_prepare_visible_cells+D↑j
                cmp     g_facing, 53h ; 'S'
                jnz     short loc_1B504
                mov     [bp+var_E], 16FEh
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B504:                              ; CODE XREF: view_prepare_visible_cells+1B↑j
                cmp     g_facing, 45h ; 'E'
                jnz     short loc_1B512
                mov     [bp+var_E], 1704h
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B512:                              ; CODE XREF: view_prepare_visible_cells+29↑j
                cmp     g_facing, 57h ; 'W'
                jnz     short loc_1B51E
                mov     [bp+var_E], 170Ah

loc_1B51E:                              ; CODE XREF: view_prepare_visible_cells+14↑j
                                        ; view_prepare_visible_cells+22↑j ...
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_2], al
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_6], al
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_4], al
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_8], al
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_A], al
                mov     bx, [bp+var_E]
                inc     [bp+var_E]
                mov     al, [bx]
                mov     [bp+var_C], al
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_6]
                mov     di, ax
                push    di
                push    si
                mov     al, byte ptr g_party_y
                push    ax
                mov     al, g_party_x
                push    ax
                mov     ax, 59CAh
                push    ax
                call    view_sample_row
                add     sp, 0Ah
                push    di
                push    si
                mov     al, [bp+var_8]
                sub     ah, ah
                mov     cl, byte ptr g_party_y
                sub     ch, ch
                add     ax, cx
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cl, g_party_x
                add     ax, cx
                push    ax
                mov     ax, 59CEh
                push    ax
                call    view_sample_row
                add     sp, 0Ah
                push    di
                push    si
                mov     al, [bp+var_C]
                sub     ah, ah
                mov     cl, byte ptr g_party_y
                sub     ch, ch
                add     ax, cx
                push    ax
                mov     al, [bp+var_A]
                sub     ah, ah
                mov     cl, g_party_x
                add     ax, cx
                push    ax
                mov     ax, 59D2h
                push    ax
                call    view_sample_row
                add     sp, 0Ah
                mov     bl, byte ptr g_party_y
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                mov     al, g_party_x
                sub     ah, ah
                add     bx, ax
                mov     al, [bx+5AD6h]
                mov     byte_23218, al
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
view_prepare_visible_cells endp


; =============== S U B R O U T I N E =======================================

; (map id, x, y)
; Attributes: bp-based frame

enter_map       proc near               ; CODE XREF: seg002:02F1↑J
                                        ; evt_op12_teleport+71↑p ...

var_C           = byte ptr -0Ch
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
                mov     [bp+var_6], 0
                mov     [bp+var_2], 0
                mov     [bp+var_4], 0
                mov     [bp+var_A], 0
                mov     al, byte_1DBEC
                mov     [bp+var_C], al
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    map_style_for_id
                add     sp, 2
                mov     [bp+var_8], al
                cmp     al, 3
                jz      short loc_1B621
                cmp     al, 6
                jz      short loc_1B621
                cmp     al, 4
                jnz     short loc_1B635

loc_1B621:                              ; CODE XREF: enter_map+2D↑j
                                        ; enter_map+31↑j
                mov     [bp+var_6], 1
                cmp     g_outdoors, 1
                jnz     short loc_1B632
                inc     [bp+var_2]
                jmp     short loc_1B635
; ---------------------------------------------------------------------------
                align 2

loc_1B632:                              ; CODE XREF: enter_map+40↑j
                inc     [bp+var_A]

loc_1B635:                              ; CODE XREF: enter_map+35↑j
                                        ; enter_map+45↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1B646
                mov     al, [bp+var_C]
                cmp     [bp+var_8], al
                jz      short loc_1B646
                inc     [bp+var_A]

loc_1B646:                              ; CODE XREF: enter_map+4F↑j
                                        ; enter_map+57↑j
                cmp     [bp+var_A], 0
                jz      short loc_1B64F
                inc     [bp+var_4]

loc_1B64F:                              ; CODE XREF: enter_map+60↑j
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     al, g_map_id
                cmp     [bp+arg_0], al
                jz      short loc_1B6DC
                mov     al, [bp+var_6]
                mov     g_outdoors, al
                mov     al, [bp+arg_0]
                mov     g_map_id, al
                cmp     [bp+var_4], 0
                jz      short loc_1B67C
                mov     ax, offset aPleaseWait ; "Please wait..."
                push    ax
                call    thk_print_message_line
                add     sp, 2

loc_1B67C:                              ; CODE XREF: enter_map+86↑j
                cmp     [bp+var_A], 0
                jz      short loc_1B6A5
                call    view_free_resources
                cmp     byte_1DB96, 0Fh
                jnz     short loc_1B699
                cmp     g_outdoors, 0
                jnz     short loc_1B699
                mov     g_disk_needed, 1

loc_1B699:                              ; CODE XREF: enter_map+A0↑j
                                        ; enter_map+A7↑j
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    view_load_style_graphics
                add     sp, 2

loc_1B6A5:                              ; CODE XREF: enter_map+96↑j
                call    thk_res_6164
                cmp     [bp+var_6], 1
                jnz     short loc_1B6C4
                mov     g_disk_needed, 2
                call    view_load_sky
                mov     word_1DBCA, ax
                mov     word_1DBCC, dx
                mov     word_1DBF0, 0FFFFh

loc_1B6C4:                              ; CODE XREF: enter_map+C2↑j
                mov     al, [bp+var_8]
                mov     byte_1DBEC, al
                call    thk_load_map_events
                mov     word_1DC7C, 0FFFFh
                mov     g_disk_needed, 2
                call    view_load_current_map

loc_1B6DC:                              ; CODE XREF: enter_map+74↑j
                cmp     [bp+arg_2], 0FFh
                jnz     short loc_1B6F6
                mov     al, byte_231E4
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, byte_231E4
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                jmp     short loc_1B6FF
; ---------------------------------------------------------------------------
                align 2

loc_1B6F6:                              ; CODE XREF: enter_map+F6↑j
                mov     al, [bp+arg_2]
                mov     g_party_x, al
                mov     al, [bp+arg_4]

loc_1B6FF:                              ; CODE XREF: enter_map+109↑j
                mov     byte ptr g_party_y, al
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                call    thk_res_49E2
                call    thk_draw_status_line
                call    thk_draw_party_list
                call    view_prepare_visible_cells
                call    thk_res_3FFC
                cmp     g_view_mode, 1
                jnz     short loc_1B732
                call    thk_res_47D8
                jmp     short loc_1B735
; ---------------------------------------------------------------------------
                align 2

loc_1B732:                              ; CODE XREF: enter_map+140↑j
                call    thk_res_471E

loc_1B735:                              ; CODE XREF: enter_map+145↑j
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     byte_1DC80, 0
                mov     byte_1DC7E, 1
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                mov     g_disk_needed, 2
                mov     sp, bp
                pop     bp
                retn
enter_map       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; leaving the 16x16 map -> neighbour map ids at byte_231DB..DE
; Attributes: bp-based frame

map_edge_transition proc near           ; CODE XREF: seg002:06BD↑J
                                        ; game_main_loop+203↑p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0FFh
                cmp     g_party_x, 10h
                jnz     short loc_1B774
                mov     al, byte_231DC
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B774:                              ; CODE XREF: map_edge_transition+F↑j
                cmp     g_party_x, 0FFh
                jnz     short loc_1B780
                mov     al, byte_231DE
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B780:                              ; CODE XREF: map_edge_transition+1B↑j
                cmp     byte ptr g_party_y, 10h
                jnz     short loc_1B78C
                mov     al, byte_231DB
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B78C:                              ; CODE XREF: map_edge_transition+27↑j
                cmp     byte ptr g_party_y, 0FFh
                jnz     short loc_1B799
                mov     al, byte_231DD

loc_1B796:                              ; CODE XREF: map_edge_transition+14↑j
                                        ; map_edge_transition+20↑j ...
                mov     [bp+var_2], al

loc_1B799:                              ; CODE XREF: map_edge_transition+33↑j
                cmp     [bp+var_2], 0FFh
                jz      short loc_1B7C0
                and     g_party_x, 0Fh
                and     byte ptr g_party_y, 0Fh
                mov     al, byte ptr g_party_y
                sub     ah, ah
                push    ax
                mov     al, g_party_x
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    enter_map
                add     sp, 6
                jmp     short loc_1B7C3
; ---------------------------------------------------------------------------
                align 2

loc_1B7C0:                              ; CODE XREF: map_edge_transition+3F↑j
                call    view_prepare_visible_cells

loc_1B7C3:                              ; CODE XREF: map_edge_transition+5F↑j
                mov     sp, bp
                pop     bp
                retn
map_edge_transition endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

map_cell_at_offset proc near            ; CODE XREF: draw_minimap+87↓p

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 59D6h
                mov     al, [bp+arg_0]
                add     al, g_party_x
                sub     al, 2
                mov     [bp+var_4], al
                mov     al, [bp+arg_2]
                add     al, byte ptr g_party_y
                sub     al, 2
                mov     [bp+var_6], al
                cmp     g_outdoors, 0
                jnz     short loc_1B802
                cmp     [bp+var_4], 0Fh
                ja      short loc_1B7FD
                cmp     al, 0Fh
                jbe     short loc_1B802

loc_1B7FD:                              ; CODE XREF: map_cell_at_offset+2F↑j
                sub     ax, ax
                jmp     short loc_1B85C
; ---------------------------------------------------------------------------
                align 2

loc_1B802:                              ; CODE XREF: map_cell_at_offset+29↑j
                                        ; map_cell_at_offset+33↑j
                cmp     [bp+var_4], 0Fh
                jbe     short loc_1B816
                cmp     [bp+var_4], 13h
                jnb     short loc_1B816
                mov     [bp+var_2], 5DD8h
                jmp     short loc_1B821
; ---------------------------------------------------------------------------
                align 2

loc_1B816:                              ; CODE XREF: map_cell_at_offset+3E↑j
                                        ; map_cell_at_offset+44↑j
                cmp     [bp+var_4], 0FCh
                jbe     short loc_1B821
                mov     [bp+var_2], 5ED8h

loc_1B821:                              ; CODE XREF: map_cell_at_offset+4B↑j
                                        ; map_cell_at_offset+52↑j
                cmp     [bp+var_6], 0Fh
                jbe     short loc_1B834
                cmp     [bp+var_6], 13h
                jnb     short loc_1B834
                mov     [bp+var_2], 5BD6h
                jmp     short loc_1B83F
; ---------------------------------------------------------------------------

loc_1B834:                              ; CODE XREF: map_cell_at_offset+5D↑j
                                        ; map_cell_at_offset+63↑j
                cmp     [bp+var_6], 0FCh
                jbe     short loc_1B83F
                mov     [bp+var_2], 5CD6h

loc_1B83F:                              ; CODE XREF: map_cell_at_offset+6A↑j
                                        ; map_cell_at_offset+70↑j
                and     [bp+var_4], 0Fh
                and     [bp+var_6], 0Fh
                mov     bl, [bp+var_6]
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                mov     al, [bp+var_4]
                sub     ah, ah
                add     bx, ax
                mov     si, [bp+var_2]
                mov     al, [bx+si]

loc_1B85C:                              ; CODE XREF: map_cell_at_offset+37↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
map_cell_at_offset endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

draw_minimap    proc near               ; CODE XREF: seg002:065D↑J
                                        ; game_main_loop+52↑p

var_2A          = word ptr -2Ah
var_28          = byte ptr -28h
var_26          = word ptr -26h
var_24          = byte ptr -24h
var_22          = word ptr -22h
var_20          = word ptr -20h
var_1E          = word ptr -1Eh
var_1C          = byte ptr -1Ch
var_1A          = byte ptr -1Ah

                push    bp
                mov     bp, sp
                sub     sp, 2Ah
                push    di
                push    si
                mov     [bp+var_22], 11h
                mov     [bp+var_26], 47h ; 'G'
                mov     ax, 5
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 2
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     cx, 19h
                lea     di, [bp+var_1A]
                mov     ax, ss
                mov     es, ax
                assume es:nothing
                mov     ax, 1Eh
                repne stosb
                mov     [bp+var_20], 3E1h
                cmp     g_outdoors, 1
                jnz     short loc_1B8AA
                mov     [bp+var_20], 3E0h

loc_1B8AA:                              ; CODE XREF: draw_minimap+41↑j
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jz      short loc_1B92C
                mov     [bp+var_28], 0
                jmp     short loc_1B91F
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1B8BA:                              ; CODE XREF: draw_minimap+95↓j
                mov     al, [bp+var_28]
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                shl     si, 1
                add     si, ax
                mov     al, [bp+var_24]
                add     si, ax
                mov     al, [bp+var_1C]
                and     al, 1Fh
                mov     [bp+si+var_1A], al

loc_1B8D4:                              ; CODE XREF: draw_minimap+B7↓j
                inc     [bp+var_24]

loc_1B8D7:                              ; CODE XREF: draw_minimap+C7↓j
                cmp     [bp+var_24], 5
                jnb     short loc_1B91C
                mov     al, [bp+var_24]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_28]
                push    ax
                push    si
                call    map_cell_at_offset
                add     sp, 4
                mov     [bp+var_1C], al
                cmp     g_outdoors, 0
                jnz     short loc_1B8BA
                mov     bl, al
                sub     bh, bh
                shr     bx, 1
                shr     bx, 1
                mov     al, [bx+1720h]
                mov     cx, ax
                mov     al, [bp+var_28]
                sub     ah, ah
                mov     di, ax
                shl     di, 1
                shl     di, 1
                add     di, ax
                add     di, si
                mov     [bp+di+var_1A], cl
                jmp     short loc_1B8D4
; ---------------------------------------------------------------------------
                align 2

loc_1B91C:                              ; CODE XREF: draw_minimap+79↑j
                inc     [bp+var_28]

loc_1B91F:                              ; CODE XREF: draw_minimap+54↑j
                cmp     [bp+var_28], 5
                jnb     short loc_1B92C
                mov     [bp+var_24], 0
                jmp     short loc_1B8D7
; ---------------------------------------------------------------------------
                align 2

loc_1B92C:                              ; CODE XREF: draw_minimap+4E↑j
                                        ; draw_minimap+C1↑j
                mov     [bp+var_28], 0
                jmp     short loc_1B987
; ---------------------------------------------------------------------------

loc_1B932:                              ; CODE XREF: draw_minimap+120↓j
                inc     [bp+var_24]

loc_1B935:                              ; CODE XREF: draw_minimap+12F↓j
                cmp     [bp+var_24], 5
                jnb     short loc_1B984
                mov     al, [bp+var_24]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_28]
                mov     cx, ax
                shl     ax, 1
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                sub     ax, 3Dh ; '='
                neg     ax
                push    ax
                mov     ax, si
                mov     cl, 4
                shl     ax, cl
                add     ax, 0E0h
                push    ax
                mov     al, [bp+var_28]
                sub     ah, ah
                mov     di, ax
                shl     di, 1
                shl     di, 1
                add     di, ax
                add     di, si
                mov     al, [bp+di+var_1A]
                push    ax
                push    word_1DBBC
                push    word_1DBBA
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                jmp     short loc_1B932
; ---------------------------------------------------------------------------

loc_1B984:                              ; CODE XREF: draw_minimap+D7↑j
                inc     [bp+var_28]

loc_1B987:                              ; CODE XREF: draw_minimap+CE↑j
                cmp     [bp+var_28], 5
                jnb     short loc_1B994
                mov     [bp+var_24], 0
                jmp     short loc_1B935
; ---------------------------------------------------------------------------
                align 2

loc_1B994:                              ; CODE XREF: draw_minimap+129↑j
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jnz     short loc_1B99F
                jmp     loc_1BA89
; ---------------------------------------------------------------------------

loc_1B99F:                              ; CODE XREF: draw_minimap+138↑j
                cmp     g_party_x, 3
                jb      short loc_1B9A9
                jmp     loc_1BA39
; ---------------------------------------------------------------------------

loc_1B9A9:                              ; CODE XREF: draw_minimap+142↑j
                cmp     g_outdoors, 0
                jz      short loc_1B9B3
                jmp     loc_1BA39
; ---------------------------------------------------------------------------

loc_1B9B3:                              ; CODE XREF: draw_minimap+14C↑j
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                cmp     byte ptr g_party_y, 0Dh
                jbe     short loc_1B9DD
                mov     al, byte ptr g_party_y
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                sub     ax, 8Fh
                add     [bp+var_22], ax

loc_1B9DD:                              ; CODE XREF: draw_minimap+162↑j
                cmp     byte ptr g_party_y, 3
                jnb     short loc_1B9FD
                mov     al, byte ptr g_party_y
                sub     ah, ah
                sub     ax, 2
                neg     ax
                mov     cx, ax
                shl     ax, 1
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                sub     [bp+var_26], ax

loc_1B9FD:                              ; CODE XREF: draw_minimap+180↑j
                mov     al, g_party_x
                sub     ah, ah
                sub     ax, 2
                neg     ax
                mov     cl, 4
                shl     ax, cl
                add     ax, 0E0h
                mov     [bp+var_1E], ax
                mov     [bp+var_24], 0

loc_1BA15:                              ; CODE XREF: draw_minimap+1CC↓j
                push    [bp+var_26]
                push    [bp+var_22]
                push    [bp+var_1E]
                inc     [bp+var_1E]
                call    thk_gfx_vline
                add     sp, 6
                inc     [bp+var_24]
                cmp     [bp+var_24], 3
                jb      short loc_1BA15
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2

loc_1BA39:                              ; CODE XREF: draw_minimap+144↑j
                                        ; draw_minimap+14E↑j
                cmp     g_facing, 4Eh ; 'N'
                jnz     short loc_1BA48
                mov     [bp+var_2A], 20h ; ' '
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------
                align 2

loc_1BA48:                              ; CODE XREF: draw_minimap+1DC↑j
                cmp     g_facing, 53h ; 'S'
                jnz     short loc_1BA56
                mov     [bp+var_2A], 21h ; '!'
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------

loc_1BA56:                              ; CODE XREF: draw_minimap+1EB↑j
                cmp     g_facing, 45h ; 'E'
                jnz     short loc_1BA64
                mov     [bp+var_2A], 22h ; '"'
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------

loc_1BA64:                              ; CODE XREF: draw_minimap+1F9↑j
                cmp     g_facing, 57h ; 'W'
                jnz     short loc_1BA70
                mov     [bp+var_2A], 23h ; '#'

loc_1BA70:                              ; CODE XREF: draw_minimap+1E3↑j
                                        ; draw_minimap+1F2↑j ...
                mov     ax, 27h ; '''
                push    ax
                mov     ax, 100h
                push    ax
                push    [bp+var_2A]
                push    word_1DBBC
                push    word_1DBBA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_1BA89:                              ; CODE XREF: draw_minimap+13A↑j
                mov     ax, 1
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jnz     short loc_1BAA4
                mov     ax, offset aProtection ; "Protection"
                jmp     short loc_1BAB3
; ---------------------------------------------------------------------------

loc_1BAA4:                              ; CODE XREF: draw_minimap+23B↑j
                cmp     g_outdoors, 0
                jnz     short loc_1BAB0
                mov     ax, offset aWizardEye ; "Wizard Eye"
                jmp     short loc_1BAB3
; ---------------------------------------------------------------------------

loc_1BAB0:                              ; CODE XREF: draw_minimap+247↑j
                mov     ax, offset aEagleEye ; " Eagle Eye"

loc_1BAB3:                              ; CODE XREF: draw_minimap+240↑j
                                        ; draw_minimap+24C↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
draw_minimap    endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1BAC0       proc near               ; CODE XREF: sub_1BB4E+C1↓p

var_2           = byte ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     [bp+var_2], 0FFh
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     ax, [bx+1760h]
                mov     si, [bp+arg_2]
                shl     si, 1
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 5
                shl     bx, cl
                test    [bx+si-6974h], ax
                jz      short loc_1BB44
                mov     si, [bp+arg_2]
                mov     cl, 4
                shl     si, cl
                mov     bx, [bp+arg_0]
                mov     al, [bx+si+59D6h]
                mov     [bp+var_2], al
                cmp     g_outdoors, 0
                jnz     short loc_1BB40
                mov     bl, al
                sub     bh, bh
                shr     bx, 1
                shr     bx, 1
                mov     al, [bx+1720h]
                mov     [bp+var_2], al
                cmp     g_map_id, 29h ; ')'
                jnz     short loc_1BB1E
                mov     [bp+var_2], 8
                jmp     short loc_1BB44
; ---------------------------------------------------------------------------
                align 2

loc_1BB1E:                              ; CODE XREF: sub_1BAC0+55↑j
                cmp     g_map_id, 2Ah ; '*'
                jz      short loc_1BB2C
                cmp     g_map_id, 2Bh ; '+'
                jnz     short loc_1BB32

loc_1BB2C:                              ; CODE XREF: sub_1BAC0+63↑j
                mov     [bp+var_2], 4
                jmp     short loc_1BB44
; ---------------------------------------------------------------------------

loc_1BB32:                              ; CODE XREF: sub_1BAC0+6A↑j
                cmp     g_map_id, 2Ch ; ','
                jnz     short loc_1BB44
                mov     [bp+var_2], 5
                jmp     short loc_1BB44
; ---------------------------------------------------------------------------
                align 2

loc_1BB40:                              ; CODE XREF: sub_1BAC0+3F↑j
                and     [bp+var_2], 1Fh

loc_1BB44:                              ; CODE XREF: sub_1BAC0+27↑j
                                        ; sub_1BAC0+5B↑j ...
                mov     al, [bp+var_2]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1BAC0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1BB4E       proc near               ; CODE XREF: seg002:06C9↑J
                                        ; game_main_loop+379↑p

var_112         = word ptr -112h
var_110         = word ptr -110h
var_10E         = word ptr -10Eh
var_10C         = word ptr -10Ch
var_10A         = word ptr -10Ah
var_108         = word ptr -108h
var_106         = word ptr -106h
var_104         = byte ptr -104h
var_102         = byte ptr -102h
var_100         = byte ptr -100h

                push    bp
                mov     bp, sp
                sub     sp, 112h
                push    si
                mov     [bp+var_112], 0
                cmp     byte_1DBE6, 0
                jnz     short loc_1BB6D
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2

loc_1BB6D:                              ; CODE XREF: sub_1BB4E+13↑j
                mov     ax, 4
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
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 28h ; '('
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, g_party_x
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 2Ch ; ','
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte ptr g_party_y
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                call    thk_print_gold_label
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                mov     [bp+var_10C], 0
                jmp     short loc_1BC2C
; ---------------------------------------------------------------------------

loc_1BBFC:                              ; CODE XREF: sub_1BB4E+D7↓j
                inc     [bp+var_10A]

loc_1BC00:                              ; CODE XREF: sub_1BB4E+EB↓j
                cmp     [bp+var_10A], 10h
                jge     short loc_1BC28
                push    [bp+var_10C]
                push    [bp+var_10A]
                call    sub_1BAC0
                add     sp, 4
                mov     si, [bp+var_10C]
                mov     cl, 4
                shl     si, cl
                add     si, [bp+var_10A]
                mov     [bp+si+var_100], al
                jmp     short loc_1BBFC
; ---------------------------------------------------------------------------
                align 2

loc_1BC28:                              ; CODE XREF: sub_1BB4E+B7↑j
                inc     [bp+var_10C]

loc_1BC2C:                              ; CODE XREF: sub_1BB4E+AC↑j
                cmp     [bp+var_10C], 10h
                jge     short loc_1BC3C
                mov     [bp+var_10A], 0
                jmp     short loc_1BC00
; ---------------------------------------------------------------------------
                align 2

loc_1BC3C:                              ; CODE XREF: sub_1BB4E+E3↑j
                mov     [bp+var_108], 0ACh
                mov     [bp+var_10C], 0

loc_1BC48:                              ; CODE XREF: sub_1BB4E+192↓j
                mov     [bp+var_106], 110h
                mov     [bp+var_10A], 0Fh

loc_1BC54:                              ; CODE XREF: sub_1BB4E+180↓j
                mov     si, [bp+var_10C]
                mov     cl, 4
                shl     si, cl
                add     si, [bp+var_10A]
                mov     al, [bp+si+var_100]
                mov     [bp+var_102], al
                cmp     al, 0FFh
                jz      short loc_1BCC5
                push    [bp+var_108]
                push    [bp+var_106]
                sub     ah, ah
                push    ax
                push    word_1DBBC
                push    word_1DBBA
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     al, [si+59D6h]
                and     al, 3
                mov     [bp+var_104], al
                or      al, al
                jz      short loc_1BCC5
                cmp     g_outdoors, 0
                jnz     short loc_1BCC5
                dec     [bp+var_104]
                push    [bp+var_108]
                mov     ax, [bp+var_106]
                sub     ax, 10h
                push    ax
                mov     bl, [bp+var_104]
                sub     bh, bh
                mov     al, [bx+1780h]
                sub     ah, ah
                push    ax
                push    word_1DBBC
                push    word_1DBBA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_1BCC5:                              ; CODE XREF: sub_1BB4E+11C↑j
                                        ; sub_1BB4E+143↑j ...
                sub     [bp+var_106], 10h
                dec     [bp+var_10A]
                jns     short loc_1BC54
                sub     [bp+var_108], 0Bh
                inc     [bp+var_10C]
                cmp     [bp+var_10C], 10h
                jge     short loc_1BCE3
                jmp     loc_1BC48
; ---------------------------------------------------------------------------

loc_1BCE3:                              ; CODE XREF: sub_1BB4E+190↑j
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     al, g_party_x
                sub     ah, ah
                add     si, ax
                mov     al, [bp+si+var_100]
                mov     [bp+var_102], al
                cmp     g_facing, 4Eh ; 'N'
                jnz     short loc_1BD0E
                mov     [bp+var_10E], 20h ; ' '
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD0E:                              ; CODE XREF: sub_1BB4E+1B5↑j
                cmp     g_facing, 53h ; 'S'
                jnz     short loc_1BD1E
                mov     [bp+var_10E], 21h ; '!'
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD1E:                              ; CODE XREF: sub_1BB4E+1C5↑j
                cmp     g_facing, 45h ; 'E'
                jnz     short loc_1BD2E
                mov     [bp+var_10E], 22h ; '"'
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD2E:                              ; CODE XREF: sub_1BB4E+1D5↑j
                cmp     g_facing, 57h ; 'W'
                jnz     short loc_1BD3B
                mov     [bp+var_10E], 23h ; '#'

loc_1BD3B:                              ; CODE XREF: sub_1BB4E+1BD↑j
                                        ; sub_1BB4E+1CD↑j ...
                cmp     byte_1DBE6, 0
                jnz     short loc_1BD58
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_145E
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2

loc_1BD58:                              ; CODE XREF: sub_1BB4E+1F2↑j
                mov     al, g_party_x
                sub     ah, ah
                mov     cl, 4
                shl     ax, cl
                add     ax, 20h ; ' '
                mov     [bp+var_10A], ax
                mov     al, byte ptr g_party_y
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                sub     ax, 0ACh
                neg     ax
                mov     [bp+var_10C], ax

loc_1BD82:                              ; CODE XREF: sub_1BB4E+2B0↓j
                cmp     [bp+var_112], 0
                jz      short loc_1BD98
                push    [bp+var_10C]
                push    [bp+var_10A]
                push    [bp+var_10E]
                jmp     short loc_1BDD7
; ---------------------------------------------------------------------------
                align 2

loc_1BD98:                              ; CODE XREF: sub_1BB4E+239↑j
                cmp     [bp+var_102], 0FFh
                jnz     short loc_1BDC8
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, [bp+var_10C]
                add     ax, 0Ah
                push    ax
                mov     ax, [bp+var_10A]
                add     ax, 0Fh
                push    ax
                push    [bp+var_10C]
                push    [bp+var_10A]
                call    thk_gfx_fill_rect
                add     sp, 8
                jmp     short loc_1BDE5
; ---------------------------------------------------------------------------

loc_1BDC8:                              ; CODE XREF: sub_1BB4E+24F↑j
                push    [bp+var_10C]
                push    [bp+var_10A]
                mov     al, [bp+var_102]
                sub     ah, ah
                push    ax

loc_1BDD7:                              ; CODE XREF: sub_1BB4E+247↑j
                push    word_1DBBC
                push    word_1DBBA
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_1BDE5:                              ; CODE XREF: sub_1BB4E+278↑j
                xor     byte ptr [bp+var_112], 1
                mov     ax, 0FAh
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                call    thk_kbd_poll
                mov     [bp+var_110], ax
                cmp     ax, 1Bh
                jnz     short loc_1BD82
                cmp     byte_1DBE6, 0
                jnz     short loc_1BE1E
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4

loc_1BE1E:                              ; CODE XREF: sub_1BB4E+2B7↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1BB4E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


view_load_current_map proc near         ; CODE XREF: enter_map+EF↑p
                mov     al, byte_1DBE9
                cmp     g_map_id, al
                jz      short locret_1BE90
                mov     al, g_map_id
                mov     byte_1DBE9, al
                sub     ah, ah
                push    ax
                mov     ax, 5AD6h
                push    ax
                call    thk_load_map_b
                add     sp, 4
                mov     al, g_map_id
                sub     ah, ah
                push    ax
                mov     ax, 59D6h
                push    ax
                call    thk_load_map_a
                add     sp, 4
                mov     al, byte_231DB
                sub     ah, ah
                push    ax
                mov     ax, 5BD6h
                push    ax
                call    thk_load_map_a
                add     sp, 4
                mov     al, byte_231DD
                sub     ah, ah
                push    ax
                mov     ax, 5CD6h
                push    ax
                call    thk_load_map_a
                add     sp, 4
                mov     al, byte_231DC
                sub     ah, ah
                push    ax
                mov     ax, 5DD8h
                push    ax
                call    thk_load_map_a
                add     sp, 4
                mov     al, byte_231DE
                sub     ah, ah
                push    ax
                mov     ax, 5ED8h
                push    ax
                call    thk_load_map_a
                add     sp, 4

locret_1BE90:                           ; CODE XREF: view_load_current_map+7↑j
                retn
view_load_current_map endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1BE92       proc near               ; CODE XREF: seg002:0969↑J
                push    si
                mov     bl, g_party_x
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+1760h]
                mov     si, word ptr g_map_id
                and     si, 0FFh
                mov     cl, 5
                shl     si, cl
                mov     bl, byte ptr g_party_y
                sub     bh, bh
                shl     bx, 1
                or      [bx+si-6974h], ax
                pop     si
                retn
sub_1BE92       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1BEBA       proc near               ; CODE XREF: draw_view_indoors+9A↑p
                push    si
                push    di
                mov     cl, 2
                mov     ah, byte_23216
                mov     al, 0C0h
                cmp     ah, 3
                jz      short loc_1BECD
                mov     al, ah
                shr     al, cl

loc_1BECD:                              ; CODE XREF: sub_1BEBA+D↑j
                mov     byte_1EFF5, al
                mov     al, 3
                cmp     ah, 0C0h
                jz      short loc_1BEDB
                mov     al, ah
                shl     al, cl

loc_1BEDB:                              ; CODE XREF: sub_1BEBA+1B↑j
                mov     byte_1EFF4, al
                mov     al, byte_23217
                mov     ah, al
                add     ah, 2
                and     ah, 7
                mov     byte_1EFF6, ah
                mov     ah, 6
                or      al, al
                jz      short loc_1BEF8
                mov     ah, al
                sub     ah, 2

loc_1BEF8:                              ; CODE XREF: sub_1BEBA+37↑j
                mov     byte_1EFF7, ah
                mov     dl, byte_1EFF4
                mov     dh, byte_1EFF5
                mov     ch, byte_23216
                mov     al, byte_2321A
                and     al, dl
                jz      short loc_1BF1A
                mov     cl, byte_1EFF6
                shr     al, cl
                mov     byte_27837, al
                jmp     short loc_1BF2A
; ---------------------------------------------------------------------------

loc_1BF1A:                              ; CODE XREF: sub_1BEBA+53↑j
                mov     al, byte_2321E
                and     al, ch
                jz      short loc_1BF2A
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27833, al

loc_1BF2A:                              ; CODE XREF: sub_1BEBA+5E↑j
                                        ; sub_1BEBA+65↑j
                mov     al, byte_2321A
                and     al, dh
                jz      short loc_1BF3C
                mov     cl, byte_1EFF7
                shr     al, cl
                mov     byte_27827, al
                jmp     short loc_1BF4C
; ---------------------------------------------------------------------------

loc_1BF3C:                              ; CODE XREF: sub_1BEBA+75↑j
                mov     al, byte_23222
                and     al, ch
                jz      short loc_1BF4C
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_2782F, al

loc_1BF4C:                              ; CODE XREF: sub_1BEBA+80↑j
                                        ; sub_1BEBA+87↑j
                mov     al, byte_2321A
                and     al, ch
                jz      short loc_1BF97
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_2782B, al
                cmp     byte_27837, 0
                jnz     short loc_1BF76
                cmp     byte_27833, 0
                jnz     short loc_1BF76
                mov     al, byte_2321F
                and     al, ch
                jz      short loc_1BF76
                shr     al, cl
                mov     byte_27834, al

loc_1BF76:                              ; CODE XREF: sub_1BEBA+A7↑j
                                        ; sub_1BEBA+AE↑j ...
                cmp     byte_27827, 0
                jnz     short loc_1BF94
                cmp     byte_2782F, 0
                jnz     short loc_1BF94
                mov     al, byte_23223
                and     al, ch
                jz      short loc_1BF94
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27830, al

loc_1BF94:                              ; CODE XREF: sub_1BEBA+C1↑j
                                        ; sub_1BEBA+C8↑j ...
                jmp     loc_1C0D4
; ---------------------------------------------------------------------------

loc_1BF97:                              ; CODE XREF: sub_1BEBA+97↑j
                mov     al, byte_2321B
                and     al, dl
                jz      short loc_1BFA9
                mov     cl, byte_1EFF6
                shr     al, cl
                mov     byte_27838, al
                jmp     short loc_1BFB9
; ---------------------------------------------------------------------------

loc_1BFA9:                              ; CODE XREF: sub_1BEBA+E2↑j
                mov     al, byte_2321F
                and     al, ch
                jz      short loc_1BFB9
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27834, al

loc_1BFB9:                              ; CODE XREF: sub_1BEBA+ED↑j
                                        ; sub_1BEBA+F4↑j
                mov     al, byte_2321B
                and     al, dh
                jz      short loc_1BFCB
                mov     cl, byte_1EFF7
                shr     al, cl
                mov     byte_27828, al
                jmp     short loc_1BFDB
; ---------------------------------------------------------------------------

loc_1BFCB:                              ; CODE XREF: sub_1BEBA+104↑j
                mov     al, byte_23223
                and     al, ch
                jz      short loc_1BFDB
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27830, al

loc_1BFDB:                              ; CODE XREF: sub_1BEBA+10F↑j
                                        ; sub_1BEBA+116↑j
                mov     al, byte_2321B
                and     al, ch
                jz      short loc_1C02A
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_2782C, al
                cmp     byte_27838, 0
                jnz     short loc_1C009
                cmp     byte_27834, 0
                jnz     short loc_1C009
                mov     al, byte_23220
                and     al, ch
                jz      short loc_1C009
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27835, al

loc_1C009:                              ; CODE XREF: sub_1BEBA+136↑j
                                        ; sub_1BEBA+13D↑j ...
                cmp     byte_27828, 0
                jnz     short loc_1C027
                cmp     byte_27830, 0
                jnz     short loc_1C027
                mov     al, byte_23224
                and     al, ch
                jz      short loc_1C027
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27831, al

loc_1C027:                              ; CODE XREF: sub_1BEBA+154↑j
                                        ; sub_1BEBA+15B↑j ...
                jmp     loc_1C0D4
; ---------------------------------------------------------------------------

loc_1C02A:                              ; CODE XREF: sub_1BEBA+126↑j
                mov     al, byte_2321C
                and     al, dl
                jz      short loc_1C03C
                mov     cl, byte_1EFF6
                shr     al, cl
                mov     byte_27839, al
                jmp     short loc_1C04C
; ---------------------------------------------------------------------------

loc_1C03C:                              ; CODE XREF: sub_1BEBA+175↑j
                mov     al, byte_23220
                and     al, ch
                jz      short loc_1C04C
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27835, al

loc_1C04C:                              ; CODE XREF: sub_1BEBA+180↑j
                                        ; sub_1BEBA+187↑j
                mov     al, byte_2321C
                and     al, dh
                jz      short loc_1C05E
                mov     cl, byte_1EFF7
                shr     al, cl
                mov     byte_27829, al
                jmp     short loc_1C06E
; ---------------------------------------------------------------------------

loc_1C05E:                              ; CODE XREF: sub_1BEBA+197↑j
                mov     al, byte_23224
                and     al, ch
                jz      short loc_1C06E
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27831, al

loc_1C06E:                              ; CODE XREF: sub_1BEBA+1A2↑j
                                        ; sub_1BEBA+1A9↑j
                mov     al, byte_2321C
                and     al, ch
                jz      short loc_1C080
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_2782D, al
                jmp     short loc_1C0D4
; ---------------------------------------------------------------------------

loc_1C080:                              ; CODE XREF: sub_1BEBA+1B9↑j
                mov     al, byte_2321D
                and     al, dl
                jz      short loc_1C092
                mov     cl, byte_1EFF6
                shr     al, cl
                mov     byte_27826, al
                jmp     short loc_1C0A2
; ---------------------------------------------------------------------------

loc_1C092:                              ; CODE XREF: sub_1BEBA+1CB↑j
                mov     al, byte_23221
                and     al, ch
                jz      short loc_1C0A2
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27836, al

loc_1C0A2:                              ; CODE XREF: sub_1BEBA+1D6↑j
                                        ; sub_1BEBA+1DD↑j
                mov     al, byte_2321D
                and     al, dh
                jz      short loc_1C0B4
                mov     cl, byte_1EFF7
                shr     al, cl
                mov     byte_2782A, al
                jmp     short loc_1C0C4
; ---------------------------------------------------------------------------

loc_1C0B4:                              ; CODE XREF: sub_1BEBA+1ED↑j
                mov     al, byte_23225
                and     al, ch
                jz      short loc_1C0C4
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_27832, al

loc_1C0C4:                              ; CODE XREF: sub_1BEBA+1F8↑j
                                        ; sub_1BEBA+1FF↑j
                mov     al, byte_2321D
                and     al, ch
                jz      short loc_1C0D4
                mov     cl, byte_23217
                shr     al, cl
                mov     byte_2782E, al

loc_1C0D4:                              ; CODE XREF: sub_1BEBA:loc_1BF94↑j
                                        ; sub_1BEBA:loc_1C027↑j ...
                cmp     byte_27837, 0
                jz      short loc_1C0E7
                cmp     byte_27834, 3
                jnz     short loc_1C0E7
                mov     byte_27834, 1

loc_1C0E7:                              ; CODE XREF: sub_1BEBA+21F↑j
                                        ; sub_1BEBA+226↑j
                cmp     byte_27827, 0
                jz      short loc_1C0FA
                cmp     byte_27830, 3
                jnz     short loc_1C0FA
                mov     byte_27830, 1

loc_1C0FA:                              ; CODE XREF: sub_1BEBA+232↑j
                                        ; sub_1BEBA+239↑j
                cmp     byte_27838, 0
                jz      short loc_1C10D
                cmp     byte_27835, 3
                jnz     short loc_1C10D
                mov     byte_27835, 1

loc_1C10D:                              ; CODE XREF: sub_1BEBA+245↑j
                                        ; sub_1BEBA+24C↑j
                cmp     byte_27828, 0
                jz      short loc_1C120
                cmp     byte_27831, 3
                jnz     short loc_1C120
                mov     byte_27831, 1

loc_1C120:                              ; CODE XREF: sub_1BEBA+258↑j
                                        ; sub_1BEBA+25F↑j
                pop     di
                pop     si
                retn
sub_1BEBA       endp

; ---------------------------------------------------------------------------
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
ovl_2PLAY       ends

