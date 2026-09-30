; ===========================================================================

; Segment type: Pure code
ovl_1MENU1      segment byte public 'CODE' use16
                assume cs:ovl_1MENU1
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================


sub_1C130       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                push    word_1F120
                call    thk_res_02F2
                add     sp, 2
                mov     ax, 17E8h
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1DB8C
                call    thk_res_02F2
                add     sp, 2
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aCopyrightC1989 ; "Copyright (C) 1989 New World Computing,"...
                push    ax
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aIbmVersionByIn ; "IBM Version by Inside Out Software, Inc"...
                push    ax
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aValidArguement ; "Valid arguements:\r\n"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aEEga ; "  E - EGA"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F11A
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aTTandy100016Co ; "  T - Tandy 1000 16 color"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F11A
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aMMcgaVga ; "  M - MCGA/VGA"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F11A
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aCCga ; "  C - CGA"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F11A
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aHHerculesMono ; "  H - Hercules mono"
                push    ax
                call    thk_res_02F2
                add     sp, 2

loc_1C1DA:                              ; CODE XREF: seg002:08CD↑J
                push    word_1F11A
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aNoteMcgaVgaReq ; "NOTE: MCGA/VGA requires 448K\n\r"
                push    ax
                call    thk_res_02F2    ; CODE XREF: seg002:07F5↑J
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_exit
                add     sp, 2
                retn
sub_1C130       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1FA       proc near               ; CODE XREF: sub_1C298+66↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                sub     si, si
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     di, di
                mov     [bp+var_6], 18D8h

loc_1C215:                              ; CODE XREF: sub_1C1FA+86↓j
                mov     bx, [bp+var_6]
                mov     si, [bx]
                mov     ax, si
                shl     ax, 1
                mov     [bp+var_8], ax
                mov     bx, ax
                push    word ptr [bx+1954h]
                push    word ptr [bx+1936h]
                push    si
                push    word_1F11E
                push    word_1F11C
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                or      di, di

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                jnz     short loc_1C25E
                mov     word_22266, 0FFFFh ; CODE XREF: seg002:0639↑J
                sub     si, si
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
                jmp     short loc_1C26A
; ---------------------------------------------------------------------------

loc_1C25E:                              ; CODE XREF: sub_1C1FA:loc_1C23C↑j
                mov     ax, 46h ; 'F'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     si, ax

loc_1C26A:                              ; CODE XREF: sub_1C1FA+62↑j
                or      si, si
                jz      short loc_1C276

loc_1C26E:                              ; CODE XREF: sub_1C1FA+84↓j
                mov     [bp+var_4], di
                mov     [bp+var_2], si
                jmp     short loc_1C282
; ---------------------------------------------------------------------------

loc_1C276:                              ; CODE XREF: sub_1C1FA+72↑j
                add     [bp+var_6], 2
                inc     di
                cmp     di, 2Fh ; '/'
                jge     short loc_1C26E
                jmp     short loc_1C215
; ---------------------------------------------------------------------------

loc_1C282:                              ; CODE XREF: sub_1C1FA+7A↑j
                mov     word_22266, 0FFFEh
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C1FA       endp


; =============== S U B R O U T I N E =======================================


sub_1C298       proc near               ; CODE XREF: sub_1C298+13↓j
                                        ; game_init+16C↓p
                push    word_1DD22
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1F11C, ax
                mov     word_1F11E, dx
                or      ax, dx
                jz      short sub_1C298
                mov     ax, 3Ch ; '<'
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                push    dx
                push    word_1F11C
                call    thk_gfx_draw_op13
                add     sp, 0Ah

loc_1C2C0:                              ; CODE XREF: seg002:026D↑J
                push    word_1F11E
                push    word_1F11C
                call    thk_free_far_block
                add     sp, 4

loc_1C2CE:                              ; CODE XREF: sub_1C298+49↓j
                push    word_1DD24
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1F11C, ax
                mov     word_1F11E, dx
                or      ax, dx
                jz      short loc_1C2CE
                mov     ax, 1F4h
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_kbd_poll    ; CODE XREF: seg002:0651↑J
                or      ax, ax
                jnz     short loc_1C301
                call    sub_1C1FA

loc_1C301:                              ; CODE XREF: sub_1C298+64↑j
                push    word_1F11E
                push    word_1F11C      ; CODE XREF: seg002:08F1↑J
                call    thk_free_far_block
                add     sp, 4
                mov     ax, 4
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     ax, ax
                mov     word_1F11E, ax
                mov     word_1F11C, ax
                retn
sub_1C298       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C322       proc near               ; CODE XREF: game_init+8F↓p

var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

; FUNCTION CHUNK AT C4D8 SIZE 00000035 BYTES
; FUNCTION CHUNK AT C50E SIZE 0000005D BYTES

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_4], 23h ; '#'
                mov     [bp+var_8], 520h
                mov     [bp+var_A], 1Fh

loc_1C339:                              ; CODE XREF: sub_1C322+45↓j
                mov     bx, [bp+var_8]
                mov     di, [bx]
                push    di
                mov     ax, ds
                mov     es, ax
                assume es:DGROUP
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                pop     di
                mov     si, cx
                dec     si
                mov     bx, si
                add     bx, di
                mov     byte ptr [bx], 0
                dec     si
                mov     bx, si
                add     bx, di
                mov     byte ptr [bx], 34h ; '4'
                add     [bp+var_8], 2
                dec     [bp+var_A]
                jnz     short loc_1C339
                mov     [bp+var_2], di
                mov     [bp+var_6], si
                pop     si

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C376:                              ; CODE XREF: game_init+51↓p
                mov     ax, word_1DD12
                mov     word_1DD70, ax
                mov     ax, word_1DD14
                mov     word_1DD72, ax
                mov     ax, word_1DD16
                mov     word_1DD74, ax
                mov     ax, word_1DD18
                mov     word_1DD76, ax
                mov     ax, word_1DD1A
                mov     word_1DD78, ax
                mov     ax, word_1DD1C
                mov     word_1DD7A, ax
                mov     ax, word_1DD1E
                mov     word_1DD7C, ax
                mov     ax, word_1DD22
                mov     word_1DD7E, ax
                mov     ax, word_1DD24
                mov     word_1DD80, ax
                mov     ax, word_1DD26
                mov     word_1DD82, ax
                mov     ax, word_1DD28
                mov     word_1DD84, ax
                mov     ax, word_1DD2A
                mov     word_1DD86, ax
                mov     ax, word_1DD2C
                mov     word_1DD88, ax
                mov     ax, word_1DD2E
                mov     word_1DD8A, ax
                mov     ax, word_1DD30
                mov     word_1DD8C, ax
                mov     ax, word_1DD32
                mov     word_1DD8E, ax
                mov     ax, word_1DD34
                mov     word_1DD90, ax
                mov     ax, word_1DD36
                mov     word_1DD92, ax
                mov     ax, word_1DD38
                mov     word_1DD94, ax
                mov     ax, word_1DD3A
                mov     word_1DD96, ax
                mov     ax, word_1DD3C
                mov     word_1DD98, ax
                mov     ax, word_1DD48  ; CODE XREF: seg002:047D↑J
                mov     word_1DD9A, ax
                mov     ax, word_1DD4A
                mov     word_1DD9C, ax
                mov     ax, word_1DD4C
                mov     word_1DD9E, ax
                mov     ax, word_1DD4E
                mov     word_1DDA0, ax
                mov     ax, word_1DD50
                mov     word_1DDA2, ax
                mov     ax, word_1DD3E
                mov     word_1DDA4, ax
                mov     ax, word_1DD40
                mov     word_1DDA6, ax
                mov     ax, word_1DD42
                mov     word_1DDA8, ax
                mov     ax, word_1DD44
                mov     word_1DDAA, ax
                mov     ax, word_1DD46
                mov     word_1DDAC, ax
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C432:                              ; CODE XREF: game_init:loc_1C626↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     word_1DC82, 1
                sub     si, si

loc_1C441:                              ; CODE XREF: sub_1C322+134↓j
                cmp     word_1DC82, 0
                jnz     short loc_1C452
                push    si
                call    disk_copy_routine
                add     sp, 2
                mov     word_1DC82, ax

loc_1C452:                              ; CODE XREF: sub_1C322+124↑j
                inc     si
                cmp     si, 2
                jl      short loc_1C441
                mov     [bp+var_2], si
                cmp     word_1DC82, 0
                jnz     short loc_1C4D8
sub_1C322       endp ; sp-analysis failed


loc_1C462:                              ; CODE XREF: seg002:086D↑J
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aInsert ; "Insert "
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F120
                call    thk_res_02F2
                add     sp, 2
                push    word_1F122

loc_1C48E:                              ; CODE XREF: seg002:0B31↑J
                call    thk_res_02F2
                add     sp, 2
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aInAFloppyDrive ; "in a floppy drive"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F124
                call    thk_res_02F2
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                sub     si, si

loc_1C4BE:                              ; CODE XREF: ovl_1MENU1:C4D3↓j
                cmp     word_1DC82, 0
                jnz     short loc_1C4CF
                push    si
                call    disk_copy_routine
                add     sp, 2
                mov     word_1DC82, ax

loc_1C4CF:                              ; CODE XREF: ovl_1MENU1:C4C3↑j
                inc     si
                cmp     si, 2
                jl      short loc_1C4BE
                mov     [bp-2], si
; START OF FUNCTION CHUNK FOR sub_1C322

loc_1C4D8:                              ; CODE XREF: sub_1C322+13E↑j
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                cmp     word_1DC82, 0
                jnz     short loc_1C50E
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                push    word_1F120
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aOriginalDisk1N ; "Original Disk 1 not found - \n\r"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aRosterWillNotB ; "Roster will not be saved during game pl"...
                push    ax
                jmp     short loc_1C556
; END OF FUNCTION CHUNK FOR sub_1C322
; ---------------------------------------------------------------------------
                align 2
; START OF FUNCTION CHUNK FOR sub_1C322

loc_1C50E:                              ; CODE XREF: sub_1C322+1C5↑j
                call    thk_get_drive
                cmp     ax, 2
                jg      short loc_1C566
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aReplaceTheOrig ; "Replace the original "
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F120      ; CODE XREF: seg002:0879↑J
                call    thk_res_02F2
                add     sp, 2
                push    word_1F122
                call    thk_res_02F2
                add     sp, 2
                push    word_1F126
                call    thk_res_02F2
                add     sp, 2
                mov     ax, offset aWithYourBackup ; "with your backup copy"
                push    ax
                call    thk_res_02F2
                add     sp, 2
                push    word_1F124

loc_1C556:                              ; CODE XREF: sub_1C322+1E9↑j
                call    thk_res_02F2
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                call    thk_wait_for_key
                add     sp, 2

loc_1C566:                              ; CODE XREF: sub_1C322+1F2↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1C322
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C56C       proc near               ; CODE XREF: game_init:loc_1C66A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0
                call    thk_res_40F4
                cmp     byte_1DB96, 3
                jnz     short loc_1C58D
                cmp     word_221AC, 62h ; 'b'
                jge     short loc_1C58D
                mov     [bp+var_2], 1

loc_1C58D:                              ; CODE XREF: sub_1C56C+13↑j
                                        ; sub_1C56C+1A↑j
                cmp     byte_1DB96, 0Fh
                jnz     short loc_1C5A1
                cmp     word_221AC, 0C8h
                jge     short loc_1C5A1
                mov     [bp+var_2], 2

loc_1C5A1:                              ; CODE XREF: sub_1C56C+26↑j
                                        ; sub_1C56C+2E↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C5D4
                push    word_1F2AC
                call    thk_res_02F2    ; CODE XREF: seg002:0885↑J
                add     sp, 2
                mov     bx, [bp+var_2]
                shl     bx, 1
                push    word ptr [bx+1A5Eh]
                call    thk_res_02F2
                add     sp, 2

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                push    word_1F2AE
                call    thk_res_02F2
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_exit
                add     sp, 2

loc_1C5D4:                              ; CODE XREF: sub_1C56C+39↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C56C       endp


; =============== S U B R O U T I N E =======================================

; video/driver setup, opens main windows, loads MM2.CH font, ITEMS.DAT and SPELLS.DAT
; Attributes: bp-based frame

game_init       proc near               ; CODE XREF: seg002:01A1↑J

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+arg_0], al
                call    thk_detect_hardware
                mov     al, [bp+arg_0]
                sub     ah, ah
                cmp     ax, 43h ; 'C'
                jz      short loc_1C620
                cmp     ax, 45h ; 'E'
                jnz     short loc_1C5FF
                jmp     loc_1C6AA
; ---------------------------------------------------------------------------

loc_1C5FF:                              ; CODE XREF: game_init+22↑j
                cmp     ax, 48h ; 'H'
                jnz     short loc_1C607
                jmp     loc_1C6BE
; ---------------------------------------------------------------------------

loc_1C607:                              ; CODE XREF: game_init+2A↑j
                cmp     ax, 4Dh ; 'M'
                jnz     short loc_1C60F
                jmp     loc_1C6C8
; ---------------------------------------------------------------------------

loc_1C60F:                              ; CODE XREF: game_init+32↑j
                cmp     ax, 54h ; 'T'
                jnz     short loc_1C617
                jmp     loc_1C6B4
; ---------------------------------------------------------------------------

loc_1C617:                              ; CODE XREF: game_init+3A↑j
                cmp     al, 20h ; ' '
                jz      short loc_1C626
                call    sub_1C130
                jmp     short loc_1C626
; ---------------------------------------------------------------------------

loc_1C620:                              ; CODE XREF: game_init+1D↑j
                mov     word_221BA, 0

loc_1C626:                              ; CODE XREF: game_init+41↑j
                                        ; game_init+46↑j ...
                call    loc_1C432
                call    loc_1C376
                cmp     word_221BA, 0
                jz      short loc_1C63A
                cmp     word_221BA, 3
                jnz     short loc_1C66A

loc_1C63A:                              ; CODE XREF: game_init+59↑j
                mov     byte_1DB8E, 2
                mov     byte_1DB8F, 2
                mov     byte_1DB90, 1
                mov     byte_1DB91, 1
                mov     byte_1DB92, 0
                mov     byte_1DB93, 3
                mov     byte_1DB94, 0
                mov     byte_1DB95, 3
                mov     byte_1DB96, 3
                call    sub_1C322

loc_1C66A:                              ; CODE XREF: game_init+60↑j
                call    sub_1C56C
                call    thk_res_1DFA    ; CODE XREF: seg002:0891↑J
                mov     ax, 75BEh
                mov     dx, 1000h
                push    dx
                push    ax
                call    thk_set_error_handler
                add     sp, 4
                call    thk_load_timer_driver
                call    thk_res_1D5B
                cmp     word_221C8, 0
                jz      short loc_1C69E
                cmp     word_221BA, 4
                jnz     short loc_1C69E
                cmp     [bp+arg_0], 4Dh ; 'M'
                jz      short loc_1C69E
                mov     word_221BA, 1

loc_1C69E:                              ; CODE XREF: game_init+B1↑j
                                        ; game_init+B8↑j ...
                call    thk_load_video_driver
                or      dx, ax
                jz      short loc_1C69E
                call    thk_gfx_reset
                jmp     short loc_1C6D2
; ---------------------------------------------------------------------------

loc_1C6AA:                              ; CODE XREF: game_init+24↑j
                mov     word_221BA, 1
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6B4:                              ; CODE XREF: game_init+3C↑j
                mov     word_221BA, 2
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6BE:                              ; CODE XREF: game_init+2C↑j
                mov     word_221BA, 3
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6C8:                              ; CODE XREF: game_init+34↑j
                mov     word_221BA, 4
                jmp     loc_1C626
; ---------------------------------------------------------------------------
                align 2

loc_1C6D2:                              ; CODE XREF: game_init+D0↑j
                                        ; game_init+109↓j
                sub     ax, ax
                push    ax
                mov     ax, offset aMm2Ch ; "mm2.ch"
                push    ax
                call    thk_load_resource_cached
                add     sp, 4
                or      dx, ax
                jz      short loc_1C6D2
                mov     ax, 17h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_text_window_create

loc_1C6F2:                              ; CODE XREF: seg002:0B19↑J
                add     sp, 8
                mov     g_main_text_win, ax
                mov     ax, 0C7h
                push    ax
                mov     ax, 13Fh
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                call    thk_gfx_window_create
                add     sp, 8
                mov     g_main_gfx_win, ax
                push    ax
                call    thk_gfx_window_open
                add     sp, 2
                push    g_main_text_win
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     ax, 75B2h
                mov     dx, 1000h
                push    dx
                push    ax
                call    thk_set_error_handler
                add     sp, 4
                call    thk_get_drive
                mov     word_277D2, ax  ; CODE XREF: seg002:089D↑J
                mov     word_277D0, ax
                mov     g_disk_needed, 1
                call    sub_1C298

loc_1C747:                              ; CODE XREF: game_init+186↓j
                mov     ax, 1400h
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 6960h
                push    ax
                mov     ax, offset aItemsDat ; "items.dat"
                push    ax
                call    thk_read_file_to_buffer
                add     sp, 8
                or      ax, ax
                jz      short loc_1C747

loc_1C760:                              ; CODE XREF: game_init+19F↓j
                mov     ax, 0C0h
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 7D60h
                push    ax
                mov     ax, offset aSpellsDat ; "spells.dat"
                push    ax
                call    thk_read_file_to_buffer
                add     sp, 8
                or      ax, ax
                jz      short loc_1C760
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C77C:                              ; CODE XREF: title_screen:loc_1CD96↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     ax, 6
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 5
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     al, byte_1DB92
                sub     ah, ah
                mov     [bp+var_2], ax
                cmp     byte_1DB96, 3
                jnz     short loc_1C7B9
                mov     [bp+var_2], 2

loc_1C7B9:                              ; CODE XREF: game_init+1DA↑j
                push    [bp+var_2]
                call    thk_gfx_set_color
                add     sp, 2
                mov     ax, 4Fh ; 'O'
                push    ax
                mov     ax, 137h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_gfx_fill_rect
                add     sp, 8
                mov     [bp+var_2], 0Eh
                mov     si, 8

loc_1C7DD:                              ; CODE XREF: ovl_1MENU1:C804↓j
                mov     ax, 0Ah
                push    ax
                push    si
game_init       endp


loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                sub     ax, ax
                push    ax
                push    word_1DBA4      ; CODE XREF: seg002:0B0D↑J
                push    word_1DBA2
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 32h ; '2'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                add     si, 10h
                cmp     si, 0E8h
                jl      short loc_1C7DD
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, 1CE8h

loc_1C815:                              ; CODE XREF: ovl_1MENU1:C831↓j
                lea     ax, [si+0Bh]
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Bh
                jl      short loc_1C815
                mov     [bp-2], si
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                call    thk_print_gold_label
                mov     ax, 1Bh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_set_color
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

init_helper_a   proc near               ; CODE XREF: title_screen:loc_1CD8A↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
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
                call    thk_print_gold_label
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     si, si
                mov     di, 1CFEh

loc_1C8B1:                              ; CODE XREF: init_helper_a+62↓j
                push    si
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 14h
                jl      short loc_1C8B1
                mov     [bp+var_2], si
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1DB8C
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_145E
                add     sp, 4
                mov     ax, 1Bh
                push    ax
                call    thk_wait_for_key
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
init_helper_a   endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


init_helper_b   proc near               ; CODE XREF: ovl_1MENU1:loc_1C9B9↓p
                                        ; init_helper_c:loc_1C9E5↓p
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 10h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 8
                push    ax
                mov     ax, 0F8h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 10h
                push    ax
                push    word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 8
                push    ax
                mov     ax, 0F8h
                push    ax
                push    word_1F576
                inc     word_1F576
                push    word_1DBA0
                push    word_1DB9E
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 3Ch ; '<'   ; CODE XREF: seg002:07DD↑J
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                cmp     word_1F576, 0Ch
                jnz     short locret_1C9B0
                mov     word_1F576, 0

locret_1C9B0:                           ; CODE XREF: init_helper_b+8E↑j
                retn
init_helper_b   endp

; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: title_screen:loc_1CD68↓p
                mov     bp, sp
                sub     sp, 2
                push    si

loc_1C9B9:                              ; CODE XREF: ovl_1MENU1:C9C3↓j
                call    init_helper_b
                call    thk_kbd_poll
                mov     si, ax
                or      si, si
                jz      short loc_1C9B9
                mov     [bp-2], si
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

init_helper_c   proc near               ; CODE XREF: ovl_1MENU1:CB80↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, word_221B4
                mov     [bp+var_2], ax
                mov     word_221B4, 0
                mov     di, [bp+arg_2]

loc_1C9E5:                              ; CODE XREF: init_helper_c+51↓j
                                        ; seg002:0B01↑J
                call    init_helper_b
                call    thk_kbd_poll
                mov     si, ax
                or      si, si
                jnz     short loc_1CA13
                mov     bx, word_1DD62
                inc     word_1DD62
                mov     al, [bx+50Ah]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     word_1DD62, 8
                jnz     short loc_1CA13
                mov     word_1DD62, 0

loc_1CA13:                              ; CODE XREF: init_helper_c+21↑j
                                        ; init_helper_c+3D↑j
                cmp     si, [bp+arg_0]
                jl      short loc_1CA1C
                cmp     si, di
                jle     short loc_1CA21

loc_1CA1C:                              ; CODE XREF: init_helper_c+48↑j
                cmp     si, 1Bh
                jnz     short loc_1C9E5

loc_1CA21:                              ; CODE XREF: init_helper_c+4C↑j
                mov     [bp+var_4], si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_2]
                mov     word_221B4, ax
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
init_helper_c   endp

; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: title_screen:loc_1CD9C↓p
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     bx, word_1F5F6
                mov     byte ptr [bx+0Eh], 32h ; '2'
                cmp     byte_1DB96, 3
                jnz     short loc_1CA56 ; CODE XREF: seg002:0A89↑J
                inc     byte ptr [bx+0Eh]

loc_1CA56:                              ; CODE XREF: ovl_1MENU1:CA51↑j
                cmp     word_27844, 1
                jnz     short loc_1CA60
                jmp     loc_1CBB4
; ---------------------------------------------------------------------------

loc_1CA60:                              ; CODE XREF: ovl_1MENU1:CA5B↑j
                mov     ax, word_27844
                add     ax, 40h ; '@'
                mov     [bp-2], ax
                mov     bx, word_1F5F4
                mov     al, [bp-2]
                mov     [bx+12h], al
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2           ; CODE XREF: seg002:0801↑J
                mov     ax, 14h
                push    ax
                mov     ax, 22h ; '"'
                push    ax
                mov     ax, 0Ch
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp-6], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 1
                mov     al, byte_1DB92
                mov     [bx+7], al
                push    bx
                call    thk_text_window_open
                add     sp, 2
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                call    thk_draw_frame_alt
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 1
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTypeTheLetterO ; "Type the letter of the drive\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_1F5F6
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aLocatedDuringG ; "located during game play:\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                push    word_1F5F4
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 7
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDDisk ; "D - Disk "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bx, word_1F5F6
                mov     al, [bx+0Eh]    ; CODE XREF: seg002:0A65↑J
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, offset aDrive ; " - Drive "
                push    ax
                call    thk_text_puts
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
                mov     di, [bp-2]
                jmp     short loc_1CB74
; ---------------------------------------------------------------------------
                align 2

loc_1CB70:                              ; CODE XREF: ovl_1MENU1:CB92↓j
                cmp     si, di
                jle     short loc_1CB94

loc_1CB74:                              ; CODE XREF: ovl_1MENU1:CB6D↑j
                push    di
                call    thk_res_0128
                add     sp, 2
                push    ax
                mov     ax, 41h ; 'A'
                push    ax
                call    init_helper_c
                add     sp, 4
                push    ax
                call    thk_toupper

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1CB70

loc_1CB94:                              ; CODE XREF: ovl_1MENU1:CB72↑j
                mov     [bp-4], si
                cmp     si, 1Bh
                jz      short loc_1CBAB

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                push    si
                call    thk_text_putc
                add     sp, 2
                mov     ax, si
                add     ax, 0FFC0h
                mov     word_277D2, ax

loc_1CBAB:                              ; CODE XREF: ovl_1MENU1:CB9A↑j
                push    word ptr [bp-6]
                call    thk_text_window_close
                add     sp, 2

loc_1CBB4:                              ; CODE XREF: ovl_1MENU1:CA5D↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                mov     ax, 1           ; CODE XREF: title_screen+76↓p
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aNewWorldComput_1 ; "\nNew World Computing\n\nPresents\n\nMI"...
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aBookTwo_0 ; "\nBook Two\n\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                retn

; =============== S U B R O U T I N E =======================================

; "Copyright 1989 New World Computing" + OPTIONS (S start game, C copy player disk, A about Book Two)
; Attributes: bp-based frame

title_screen    proc near               ; CODE XREF: seg002:01AD↑J

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     [bp+var_2], 0

loc_1CBE8:                              ; CODE XREF: title_screen+1F↓j
                push    word_1DD14
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DB9E, ax
                mov     word_1DBA0, dx
                or      dx, ax
                jz      short loc_1CBE8

loc_1CBFD:                              ; CODE XREF: title_screen+34↓j
                push    word_1DD16
                call    thk_gfx_load_image
                add     sp, 2
                mov     word_1DBA2, ax
                mov     word_1DBA4, dx
                or      dx, ax
                jz      short loc_1CBFD
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_draw_main_frame
                mov     ax, 0Ah
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
                mov     ax, 2
                push    ax
                call    thk_text_set_align
                add     sp, 2
                call    loc_1CBBA
                mov     ax, 5
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 15h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCopyright1989N ; "Copyright 1989 New World Computing,Inc"...
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, offset aAllRightsReser ; "All Rights Reserved"
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_align
                add     sp, 2
                mov     ax, 0Ch
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOptions ; "OPTIONS\n\n\n"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Fh
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSStartGame ; "S - Start Game"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 10h         ; CODE XREF: seg002:08FD↑J
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCCopyPlayerDis ; "C - Copy Player Disk"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAAboutBookTwo ; "A - About Book Two"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDDisk_0 ; "D - Disk "
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     byte_1DB96, 3
                jnz     short loc_1CD0E
                mov     ax, 33h ; '3'
                jmp     short loc_1CD11
; ---------------------------------------------------------------------------
                align 2

loc_1CD0E:                              ; CODE XREF: title_screen+12A↑j
                mov     ax, 32h ; '2'

loc_1CD11:                              ; CODE XREF: title_screen+12F↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, offset aDrive_0 ; " - Drive "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, word_277D2
                add     ax, 40h ; '@'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     [bp+var_4], 7
                mov     si, 7

loc_1CD45:                              ; CODE XREF: title_screen+174↓j
                mov     ax, 5
                push    ax
                call    thk_text_putc
                add     sp, 2
                dec     si
                jnz     short loc_1CD45
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4

loc_1CD68:                              ; CODE XREF: title_screen+1B8↓j
                call    loc_1C9B2
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 1Bh
                jz      short loc_1CDBC
                cmp     ax, 41h ; 'A'
                jz      short loc_1CD8A
                cmp     ax, 43h ; 'C'
                jz      short loc_1CD96
                cmp     ax, 44h ; 'D'
                jz      short loc_1CD9C
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------

loc_1CD8A:                              ; CODE XREF: title_screen+1A0↑j
                call    init_helper_a

loc_1CD8D:                              ; CODE XREF: title_screen+1AC↑j
                                        ; title_screen+1BD↓j ...
                mov     ax, si
                cmp     ax, 53h ; 'S'
                jz      short loc_1CDC2
                jmp     short loc_1CD68
; ---------------------------------------------------------------------------

loc_1CD96:                              ; CODE XREF: title_screen+1A5↑j
                call    loc_1C77C
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------
                align 2

loc_1CD9C:                              ; CODE XREF: title_screen+1AA↑j
                call    loc_1CA3C
                mov     ax, 13h
                push    ax
                mov     ax, 1Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, word_277D2
                add     ax, 40h ; '@'
                push    ax
                call    thk_text_putc
                add     sp, 2
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------

loc_1CDBC:                              ; CODE XREF: title_screen+19B↑j
                call    thk_quit_to_dos
                jmp     short loc_1CD8D
; ---------------------------------------------------------------------------
                align 2

loc_1CDC2:                              ; CODE XREF: title_screen+1B6↑j
                mov     [bp+var_4], si
                push    word_1DBA4
                push    word_1DBA2
                call    thk_free_far_block
                add     sp, 4
                push    word_1DBA0
                push    word_1DB9E
                call    thk_free_far_block
                add     sp, 4
                pop     si
                mov     sp, bp
                pop     bp
                retn
title_screen    endp


; =============== S U B R O U T I N E =======================================

; int 13h based; unreferenced
; Attributes: bp-based frame

disk_copy_routine proc near             ; CODE XREF: sub_1C322+127↑p
                                        ; ovl_1MENU1:C4C6↑p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                mov     ax, [bp+arg_0]
                and     al, 1
                mov     byte_1F6D7, al
                mov     bx, 40h ; '@'
                mov     ah, 48h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                mov     bx, ax
                and     ax, 0FFFh
                cmp     ax, 0FC1h
                jb      short loc_1CE18
                push    bx
                mov     bx, 40h ; '@'
                mov     ah, 48h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                pop     bx
                push    ax
                push    es
                mov     es, bx
                assume es:nothing
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                pop     bx

loc_1CE18:                              ; CODE XREF: disk_copy_routine+1D↑j
                mov     di, 3
                push    bx

loc_1CE1C:                              ; CODE XREF: disk_copy_routine+54↓j
                xor     ah, ah
                push    di
                int     13h             ; DISK - RESET DISK SYSTEM
                                        ; DL = drive (if bit 7 is set both hard disks and floppy disks reset)
                pop     di
                mov     ax, 201h
                mov     cl, 1
                mov     ch, 27h ; '''
                mov     dl, byte_1F6D7
                mov     dh, 0
                pop     es

loc_1CE30:                              ; CODE XREF: seg002:0831↑J
                push    es
                xor     bx, bx
                push    di
                int     13h             ; DISK - READ SECTORS INTO MEMORY
                                        ; AL = number of sectors to read, CH = track, CL = sector
                                        ; DH = head, DL = drive, ES:BX -> buffer to fill
                                        ; Return: CF set on error, AH = status, AL = number of sectors read
                pop     di
                jnb     short loc_1CE40
                dec     di
                jnz     short loc_1CE1C
                xor     ax, ax
                jmp     short loc_1CEB6
; ---------------------------------------------------------------------------

loc_1CE40:                              ; CODE XREF: disk_copy_routine+51↑j
                mov     al, 1Eh
                mov     ah, 35h
                int     21h             ; DOS - 2+ - GET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; Return: ES:BX = value of interrupt vector
                mov     word_1F6D3, bx
                mov     word_1F6D5, es
                mov     dx, 1E78h
                mov     al, 1Eh
                mov     ah, 25h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                mov     di, 3

loc_1CE5A:                              ; CODE XREF: disk_copy_routine+8F↓j
                                        ; disk_copy_routine+9B↓j ...
                mov     ax, 201h
                pop     es
                push    es
                xor     bx, bx
                mov     cl, 17h
                mov     ch, 27h ; '''
                mov     dl, byte_1F6D7
                mov     dh, 0
                push    di
                int     13h             ; DISK - READ SECTORS INTO MEMORY
                                        ; AL = number of sectors to read, CH = track, CL = sector
                                        ; DH = head, DL = drive, ES:BX -> buffer to fill
                                        ; Return: CF set on error, AH = status, AL = number of sectors read
                pop     di
                cmp     ah, 10h
                jz      short loc_1CE7B
                dec     di
                jnz     short loc_1CE5A
                xor     ax, ax
                jmp     short loc_1CEA6
; ---------------------------------------------------------------------------

loc_1CE7B:                              ; CODE XREF: disk_copy_routine+8C↑j
                cmp     byte ptr es:[bx+240h], 3Ch ; '<'
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+270h], 3Ch ; '<'
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+2A0h], 0F6h
                jnz     short loc_1CE5A
                cmp     byte ptr es:[bx+264h], 18h
                jnz     short loc_1CE5A
                cmp     word ptr es:[bx+28Eh], 3Fh ; '?'
                jnz     short loc_1CE5A
                mov     ax, 1

loc_1CEA6:                              ; CODE XREF: disk_copy_routine+93↑j
                push    ax
                mov     dx, word_1F6D3
                mov     ds, word_1F6D5
                mov     al, 1Eh
                mov     ah, 25h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                pop     ax

loc_1CEB6:                              ; CODE XREF: disk_copy_routine+58↑j
                pop     bx
                push    ax
                push    es
                mov     es, bx
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                pop     ax
                pop     ds
                pop     di
                pop     si
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db    0
                db    0
byte_1CEC8      db 8 dup(0)             ; CODE XREF: seg002:07AD↑J
disk_copy_routine endp

ovl_1MENU1      ends

