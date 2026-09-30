; ===========================================================================

; Segment type: Pure code
ovl_2MISC2      segment byte public 'CODE' use16
                assume cs:ovl_2MISC2
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; 'D' Dismiss: pick party member 1-N; hirelings (id >= 18h) are removed
; Attributes: bp-based frame

dismiss_hireling proc near              ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 4
                push    di
                push    si
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                call    thk_res_5440
                mov     bx, word ptr unk_20304
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+10h], al
                push    bx
                call    thk_res_410A
                add     sp, 2
                mov     byte_1DC80, 1

loc_1C15C:                              ; CODE XREF: dismiss_hireling+78↓j
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                sub     ah, ah
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1C17C
                mov     ax, 1
                jmp     short loc_1C17E
; ---------------------------------------------------------------------------

loc_1C17C:                              ; CODE XREF: dismiss_hireling+45↑j
                sub     ax, ax

loc_1C17E:                              ; CODE XREF: dismiss_hireling+4A↑j
                mov     di, ax
                or      di, di
                jnz     short loc_1C1A6
                sub     si, 31h ; '1'
                mov     bx, si
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1C1A6
                mov     bx, si
                shl     bx, 1
                push    word ptr [bx+416h]
                call    thk_party_remove
                add     sp, 2
                mov     byte_1DC80, 3
                inc     di

loc_1C1A6:                              ; CODE XREF: dismiss_hireling+52↑j
                                        ; dismiss_hireling+60↑j
                or      di, di
                jz      short loc_1C15C
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                call    thk_res_35A8
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
dismiss_hireling endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1BC       proc near               ; CODE XREF: party_exchange+22↓p
                                        ; sub_1C370+23↓p

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6
arg_4           = word ptr  8
arg_6           = word ptr  0Ah

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 1Bh
                mov     bx, word_20324
                mov     al, [bp+arg_0]
                mov     [bx+0Eh], al
                mov     bx, word ptr unk_20326
                mov     [bx+0Ah], al
                sub     ax, ax
                push    ax

loc_1C1DA:                              ; CODE XREF: seg002:08CD↑J
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                mov     ax, 1Eh
                push    ax
                mov     ax, 8           ; CODE XREF: seg002:07F5↑J
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_6], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 1
                mov     al, byte_1DB92
                mov     [bx+7], al
                push    bx
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
                mov     ax, 1
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_20324
                call    thk_text_puts
                add     sp, 2

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                mov     ax, 2
                push    ax
                mov     ax, 6           ; CODE XREF: seg002:0639↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr unk_20326
                call    thk_text_puts
                add     sp, 2
                mov     ax, 4
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1DCF0
                call    thk_text_puts
                add     sp, 2
                mov     ax, 1
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+arg_2], 0FFh
                jnz     short loc_1C292
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                jmp     short loc_1C297
; ---------------------------------------------------------------------------

loc_1C292:                              ; CODE XREF: sub_1C1BC+C2↑j
                mov     al, [bp+arg_2]
                add     al, 31h ; '1'

loc_1C297:                              ; CODE XREF: sub_1C1BC+D4↑j
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C2DD
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range ; CODE XREF: seg002:026D↑J
                add     sp, 4
                mov     [bp+var_4], al
                cmp     al, 1Bh
                jz      short loc_1C2DD
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     [bp+var_2], 31h ; '1'
                sub     [bp+var_4], 31h ; '1'

loc_1C2DD:                              ; CODE XREF: sub_1C1BC+E0↑j
                                        ; sub_1C1BC+10E↑j
                mov     bx, [bp+arg_4]
                mov     al, [bp+var_2]
                mov     [bx], al
                mov     bx, [bp+arg_6]
                mov     al, [bp+var_4]
                mov     [bx], al
                push    [bp+var_6]
                call    thk_text_window_close
                mov     sp, bp
                pop     bp
                retn
sub_1C1BC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; 'E' Exchange party order
; Attributes: bp-based frame

party_exchange  proc near               ; CODE XREF: seg002:0651↑J

var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bp+var_8], al

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                lea     ax, [bp+var_4]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                mov     ax, 0FFh
                push    ax
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    sub_1C1BC
                add     sp, 8
                cmp     [bp+var_2], 1Bh
                jz      short loc_1C35F
                cmp     [bp+var_4], 1Bh
                jz      short loc_1C35F
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                add     si, 416h
                mov     ax, [si]
                mov     [bp+var_6], ax
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     di, ax
                shl     di, 1
                add     di, 416h
                mov     ax, [di]
                mov     [si], ax
                mov     ax, [bp+var_6]
                mov     [di], ax
                mov     al, [bp+var_4]
                cmp     [bp+var_2], al
                jz      short loc_1C35F
                call    thk_draw_party_list

loc_1C35F:                              ; CODE XREF: party_exchange+2C↑j
                                        ; party_exchange+32↑j ...
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
party_exchange  endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C370       proc near               ; CODE XREF: seg002:0471↑J

var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2
arg_0           = byte ptr  4
arg_2           = byte ptr  6
arg_4           = word ptr  8
arg_6           = word ptr  0Ah

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_8], 0
                add     [bp+arg_0], 30h ; '0'
                lea     ax, [bp+var_6]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                mov     al, [bp+arg_0]
                push    ax
                call    sub_1C1BC
                add     sp, 8
                cmp     [bp+var_4], 1Bh
                jz      short loc_1C3EC
                cmp     [bp+var_6], 1Bh
                jz      short loc_1C3EC
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     si, ax
                mov     di, si
                shl     di, 1
                add     di, [bp+arg_4]
                mov     ax, [di]
                mov     [bp+var_A], ax
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     [bp+var_C], ax
                shl     ax, 1
                add     ax, [bp+arg_4]
                mov     [bp+var_E], ax
                mov     bx, ax
                mov     ax, [bx]
                mov     [di], ax
                mov     ax, [bp+var_A]
                mov     [bx], ax
                mov     bx, [bp+arg_6]
                mov     al, [bx+si]
                mov     [bp+var_2], al
                mov     di, [bp+var_C]
                add     di, bx
                mov     al, [di]
                mov     [bx+si], al
                mov     al, [bp+var_2]
                mov     [di], al
                inc     [bp+var_8]

loc_1C3EC:                              ; CODE XREF: sub_1C370+2D↑j
                                        ; sub_1C370+33↑j
                mov     ax, [bp+var_8]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C370       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Controls": 1) Sounds 2) Walk Beep 3) Disposition 4) Delay
; Attributes: bp-based frame

game_controls   proc near               ; CODE XREF: seg002:047D↑J

var_14          = word ptr -14h
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_8], 1
                mov     [bp+var_A], 1
                mov     [bp+var_C], 1
                mov     [bp+var_2], 1
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 1Eh
                push    ax
                mov     ax, 3
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp+var_4], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 1
                mov     al, byte_1DB92
                mov     [bx+7], al
                push    bx
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
                mov     ax, 1

loc_1C462:                              ; CODE XREF: seg002:086D↑J
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aControls ; "Controls"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 3
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset a1Sounds ; "1) Sounds       /"
                push    ax
                call    thk_text_puts
                add     sp, 2           ; CODE XREF: seg002:0B31↑J
                mov     ax, 4
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset a2WalkBeep ; "2) Walk Beep    /"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 6
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset a3Disposition ; "3) Disposition:"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Ch
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset a4Delay ; "4) Delay"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Eh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aPress14ToToggl ; "Press 1-4 to toggle"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_1DCF0
                call    thk_text_puts
                add     sp, 2

loc_1C507:                              ; CODE XREF: game_controls+2B9↓j
                cmp     [bp+var_8], 0
                jz      short loc_1C557
                mov     ax, 3
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_1DC20
                sub     ah, ah
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 2B69h
                push    ax
                call    thk_text_puts   ; CODE XREF: seg002:0879↑J
                add     sp, 2
                mov     ax, 3
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_1DC20
                sub     ah, ah
                xor     al, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 2B6Ch
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C557:                              ; CODE XREF: game_controls+115↑j
                cmp     [bp+var_A], 0
                jz      short loc_1C5A7
                mov     ax, 4
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_1DC21
                sub     ah, ah
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 2B70h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 4
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, byte_1DC21
                sub     ah, ah
                xor     al, 1
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 2B73h
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C5A7:                              ; CODE XREF: game_controls+165↑j
                cmp     [bp+var_C], 0
                jz      short loc_1C5EF ; CODE XREF: seg002:0885↑J
                sub     si, si
                mov     di, 2B78h

loc_1C5B2:                              ; CODE XREF: game_controls+1F4↓j
                lea     ax, [si+7]
                push    ax
                mov     ax, 6
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                mov     ax, si
                mov     cl, byte_1DC22
                sub     ch, ch
                cmp     ax, cx
                jnz     short loc_1C5D2
                mov     ax, 1
                jmp     short loc_1C5D4
; ---------------------------------------------------------------------------
                align 2

loc_1C5D2:                              ; CODE XREF: game_controls+1D4↑j
                sub     ax, ax

loc_1C5D4:                              ; CODE XREF: game_controls+1D9↑j
                push    ax
                call    thk_text_set_flag_8

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                add     sp, 2
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jle     short loc_1C5B2
                mov     [bp+var_6], si

loc_1C5EF:                              ; CODE XREF: game_controls+1B5↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C634
                mov     ax, 0Ch
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                sub     si, si

loc_1C605:                              ; CODE XREF: game_controls+239↓j
                mov     ax, si
                mov     cl, byte_1DC23
                sub     ch, ch
                cmp     ax, cx
                jnz     short loc_1C616
                mov     ax, 1
                jmp     short loc_1C618
; ---------------------------------------------------------------------------

loc_1C616:                              ; CODE XREF: game_controls+219↑j
                sub     ax, ax

loc_1C618:                              ; CODE XREF: game_controls+21E↑j
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, si
                add     ax, 30h ; '0'
                push    ax
                call    thk_text_putc
                add     sp, 2
                inc     si
                cmp     si, 0Ah
                jl      short loc_1C605
                mov     [bp+var_6], si

loc_1C634:                              ; CODE XREF: game_controls+1FD↑j
                sub     ax, ax
                mov     [bp+var_2], ax
                mov     [bp+var_C], ax
                mov     [bp+var_A], ax
                mov     [bp+var_8], ax
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_6], ax
                cmp     ax, 31h ; '1'
                jz      short loc_1C66C
                cmp     ax, 32h ; '2'
                jz      short loc_1C676
                cmp     ax, 33h ; '3'
                jz      short loc_1C680
                cmp     ax, 34h ; '4'
                jz      short loc_1C696
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------
                align 2

loc_1C66C:                              ; CODE XREF: game_controls+262↑j
                                        ; seg002:0891↑J
                xor     byte_1DC20, 1
                inc     [bp+var_8]
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------

loc_1C676:                              ; CODE XREF: game_controls+267↑j
                xor     byte_1DC21, 1
                inc     [bp+var_A]
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------

loc_1C680:                              ; CODE XREF: game_controls+26C↑j
                inc     byte_1DC22
                cmp     byte_1DC22, 3
                jbe     short loc_1C690
                mov     byte_1DC22, 0

loc_1C690:                              ; CODE XREF: game_controls+293↑j
                inc     [bp+var_C]
                jmp     short loc_1C6A9
; ---------------------------------------------------------------------------
                align 2

loc_1C696:                              ; CODE XREF: game_controls+271↑j
                inc     byte_1DC23
                cmp     byte_1DC23, 9
                jbe     short loc_1C6A6
                mov     byte_1DC23, 0

loc_1C6A6:                              ; CODE XREF: game_controls+2A9↑j
                inc     [bp+var_2]

loc_1C6A9:                              ; CODE XREF: game_controls+273↑j
                                        ; game_controls+27E↑j ...
                cmp     [bp+var_6], 1Bh
                jz      short loc_1C6B2
                jmp     loc_1C507
; ---------------------------------------------------------------------------

loc_1C6B2:                              ; CODE XREF: game_controls+2B7↑j
                push    [bp+var_4]
                call    thk_text_window_close
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C6CC:                              ; CODE XREF: ovl_2MISC2:CA69↓p
                push    bp
                mov     bp, sp
                sub     sp, 16h
                push    di
                push    si
                mov     [bp+var_A], 1
                mov     [bp+var_14], 0
                mov     byte ptr [bp+var_8], 0
                push    [bp+arg_0]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_6], ax
                mov     bx, ax
                mov     al, [bx+23h]    ; CODE XREF: seg002:0B19↑J
game_controls   endp

                mov     [bp-12h], al
                mov     al, [bx+20h]
                mov     [bp-8], al
                cmp     byte ptr [bx+0Fh], 2
                jz      short loc_1C708
                cmp     byte ptr [bx+0Fh], 1
                jnz     short loc_1C727

loc_1C708:                              ; CODE XREF: ovl_2MISC2:C700↑j
                inc     word ptr [bp-14h]
                sub     byte ptr [bp-8], 6
                cmp     byte ptr [bp-8], 0F0h
                jb      short loc_1C719
                mov     byte ptr [bp-8], 0

loc_1C719:                              ; CODE XREF: ovl_2MISC2:C713↑j
                mov     bx, [bp-6]
                cmp     byte ptr [bx+20h], 6
                jnb     short loc_1C727
                mov     word ptr [bp-0Ah], 0

loc_1C727:                              ; CODE XREF: ovl_2MISC2:C706↑j
                                        ; ovl_2MISC2:C720↑j
                cmp     word ptr [bp-0Ah], 0
                jz      short loc_1C767
                mov     al, [bp-8]
                sub     ah, ah
                inc     ax
                shr     ax, 1
                mov     [bp-2], al
                mov     al, [bp-12h]    ; CODE XREF: seg002:089D↑J
                cmp     [bp-2], al
                jbe     short loc_1C746
                cmp     byte ptr [bp-2], 0Ah
                jb      short loc_1C74C

loc_1C746:                              ; CODE XREF: ovl_2MISC2:C73E↑j
                dec     word ptr [bp-0Ah]
                jmp     short loc_1C752
; ---------------------------------------------------------------------------
                align 2

loc_1C74C:                              ; CODE XREF: ovl_2MISC2:C744↑j
                mov     al, [bp-2]
                mov     [bp-12h], al

loc_1C752:                              ; CODE XREF: ovl_2MISC2:C749↑j
                cmp     word ptr [bp-0Ah], 0
                jz      short loc_1C767
                cmp     word ptr [bp-14h], 0
                jz      short loc_1C767
                cmp     byte ptr [bp-12h], 8
                jb      short loc_1C767
                dec     word ptr [bp-0Ah]

loc_1C767:                              ; CODE XREF: ovl_2MISC2:C72B↑j
                                        ; ovl_2MISC2:C756↑j ...
                cmp     word ptr [bp-0Ah], 0
                jnz     short loc_1C770
                jmp     loc_1C7F4
; ---------------------------------------------------------------------------

loc_1C770:                              ; CODE XREF: ovl_2MISC2:C76B↑j
                mov     bx, [bp-6]
                mov     al, [bp-12h]
                mov     [bx+23h], al
                mov     [bx+72h], al
                cmp     al, 9
                jb      short loc_1C783
                jmp     loc_1C815
; ---------------------------------------------------------------------------

loc_1C783:                              ; CODE XREF: ovl_2MISC2:C77E↑j
                sub     ah, ah
                shl     ax, 1
                shl     ax, 1
                add     ax, 2F2Ch
                mov     [bp-0Ch], ax
                cmp     byte ptr [bx+0Fh], 3
                jz      short loc_1C79B
                cmp     byte ptr [bx+0Fh], 1
                jnz     short loc_1C7AA

loc_1C79B:                              ; CODE XREF: ovl_2MISC2:C793↑j
                mov     al, [bp-12h]
                sub     ah, ah
                shl     ax, 1
                shl     ax, 1
                add     ax, 2F4Ch
                mov     [bp-0Ch], ax

loc_1C7AA:                              ; CODE XREF: ovl_2MISC2:C799↑j
                sub     si, si
                mov     di, [bp-16h]

loc_1C7AF:                              ; CODE XREF: ovl_2MISC2:C7E6↓j
                mov     bx, [bp-0Ch]
                mov     dl, [bx+si]
                cmp     dl, 80h
                jz      short loc_1C7E2
                cmp     dl, 2Fh ; '/'
                jbe     short loc_1C7C1
                sub     dl, 30h ; '0'

loc_1C7C1:                              ; CODE XREF: ovl_2MISC2:C7BC↑j
                mov     ax, dx
                sub     ah, ah
                mov     cl, 3
                shr     ax, cl
                mov     [bp-0Eh], ax
                mov     ax, dx
                sub     ah, ah
                and     ax, 7
                mov     di, ax
                mov     bx, [bp-0Eh]
                add     bx, [bp-6]
                mov     al, [di+2F28h]
                or      [bx+51h], al

loc_1C7E2:                              ; CODE XREF: seg002:0849↑J
                                        ; ovl_2MISC2:C7B7↑j
                inc     si
                cmp     si, 4
                jl      short loc_1C7AF

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                mov     [bp-16h], di
                mov     [bp-10h], si
                mov     [bp-2], dl
                jmp     short loc_1C815
; ---------------------------------------------------------------------------
                align 2

loc_1C7F4:                              ; CODE XREF: ovl_2MISC2:C76D↑j
                cmp     word ptr [bp-14h], 0
                jz      short loc_1C800
                mov     byte ptr [bp-2], 8
                jmp     short loc_1C804
; ---------------------------------------------------------------------------

loc_1C800:                              ; CODE XREF: ovl_2MISC2:C7F8↑j
                mov     byte ptr [bp-2], 9

loc_1C804:                              ; CODE XREF: ovl_2MISC2:C7FE↑j
                mov     bx, [bp-6]
                mov     al, [bp-2]
                cmp     [bx+23h], al
                jb      short loc_1C815
                mov     al, [bx+20h]
                mov     [bp-12h], al

loc_1C815:                              ; CODE XREF: ovl_2MISC2:C780↑j
                                        ; ovl_2MISC2:C7F1↑j ...
                mov     bx, [bp-6]
                mov     al, [bx+12h]
                mov     [bp-2], al
                cmp     word ptr [bp-14h], 0
                jz      short loc_1C82A
                mov     al, [bx+11h]
                mov     [bp-2], al

loc_1C82A:                              ; CODE XREF: ovl_2MISC2:C822↑j
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                mov     [bp-2], al
                cmp     al, 0F2h
                jb      short loc_1C841
                mov     byte ptr [bp-2], 0

loc_1C841:                              ; CODE XREF: ovl_2MISC2:C83B↑j
                mov     al, [bp-2]
                sub     ah, ah
                add     ax, 3
                mov     [bp-4], ax
                mov     al, [bp-12h]
                sub     ah, ah
                mul     word ptr [bp-4]
                mov     [bp-4], ax
                mov     bx, [bp-6]
                mov     [bx+5Ah], ax
                mov     [bx+58h], ax
                mov     ax, [bp-0Ah]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: training_hall+228↓p
                mov     bp, sp
                sub     sp, 10h
                push    si
                mov     word ptr [bp-2], 0
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                push    word ptr [bp+4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-0Ah], ax
                mov     bx, ax
                mov     ax, [bx+66h]
                mov     dx, [bx+68h]
                mov     [bp-8], ax
                mov     [bp-6], dx
                mov     ax, [bp+6]
                mov     dx, [bp+8]
                cmp     [bp-6], dx
                jb      short loc_1C8B2
                ja      short loc_1C8AC
                cmp     [bp-8], ax
                jb      short loc_1C8B2

loc_1C8AC:                              ; CODE XREF: ovl_2MISC2:C8A5↑j
                mov     ax, 1
                jmp     short loc_1C8B4
; ---------------------------------------------------------------------------
                align 2

loc_1C8B2:                              ; CODE XREF: ovl_2MISC2:C8A3↑j
                                        ; ovl_2MISC2:C8AA↑j
                sub     ax, ax

loc_1C8B4:                              ; CODE XREF: ovl_2MISC2:C8AF↑j
                mov     [bp-10h], ax
                mov     bx, [bp-0Ah]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1C8F4
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYouHaveToBeWel ; "You have to be well"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aToTrainHere ; "   to train here."
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     loc_1CB09
; ---------------------------------------------------------------------------
                align 2

loc_1C8F4:                              ; CODE XREF: ovl_2MISC2:C8BE↑j
                mov     ax, [bp+6]
                or      ax, [bp+8]
                jnz     short loc_1C92A
                shr     word ptr [bp-6], 1
                rcr     word ptr [bp-8], 1
                mov     bx, [bp-0Ah]
                mov     ax, [bp-8]
                mov     dx, [bp-6]
                add     [bx+66h], ax
                adc     [bx+68h], dx
                cmp     word ptr [bx+68h], 0
                jnz     short loc_1C91E
                cmp     word ptr [bx+66h], 0C350h
                jbe     short loc_1C960

loc_1C91E:                              ; CODE XREF: ovl_2MISC2:C915↑j
                mov     word ptr [bx+66h], 0C350h
                mov     word ptr [bx+68h], 0
                jmp     short loc_1C960
; ---------------------------------------------------------------------------

loc_1C92A:                              ; CODE XREF: ovl_2MISC2:C8FA↑j
                cmp     word ptr [bp-10h], 0
                jnz     short loc_1C960
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSorryYouNeed ; "Sorry - you need"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMoreGold ; "   more gold."
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C960:                              ; CODE XREF: ovl_2MISC2:C91C↑j
                                        ; ovl_2MISC2:C928↑j ...
                cmp     word ptr [bp-10h], 0
                jnz     short loc_1C969
                jmp     loc_1CB09
; ---------------------------------------------------------------------------

loc_1C969:                              ; CODE XREF: ovl_2MISC2:C964↑j
                mov     ax, [bp+6]
                or      ax, [bp+8]
                jz      short loc_1C980
                mov     bx, [bp-0Ah]
                mov     ax, [bp+6]
                mov     dx, [bp+8]
                sub     [bx+66h], ax
                sbb     [bx+68h], dx

loc_1C980:                              ; CODE XREF: ovl_2MISC2:C96F↑j
                mov     ax, 1
                push    ax
                mov     ax, [bp-0Ah]
                add     ax, 20h ; ' '
                push    ax
                call    thk_res_3608
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp-0Ah]
                add     ax, 71h ; 'q'   ; CODE XREF: seg002:07DD↑J
                push    ax
                call    thk_res_3608
                add     sp, 4
                mov     bx, [bp-0Ah]
                cmp     byte ptr [bx+0Fh], 6
                jz      short loc_1C9B1
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_1C9C7

loc_1C9B1:                              ; CODE XREF: ovl_2MISC2:C9A9↑j
                cmp     byte ptr [bx+1Eh], 0
                jz      short loc_1C9C7
                mov     ax, 1
                push    ax
                mov     ax, bx
                add     ax, 1Eh
                push    ax
                call    thk_res_3608
                add     sp, 4

loc_1C9C7:                              ; CODE XREF: ovl_2MISC2:C9AF↑j
                                        ; ovl_2MISC2:C9B5↑j
                mov     al, g_map_id
                sub     ah, ah
                mov     si, ax
                shl     si, 1
                mov     bx, [bp-0Ah]
                mov     bl, [bx+0Fh]
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+2F18h]
                mul     word ptr [si+2F04h]
                mov     [bp-4], ax
                sub     dx, dx          ; CODE XREF: seg002:0B01↑J
                div     word ptr [si+2F0Eh]
                mov     [bp-0Eh], dx
                mov     ax, [bp-4]
                sub     dx, dx
                div     word ptr [si+2F0Eh]
                mov     [bp-4], ax
                mov     bx, [bp-0Ah]
                cmp     byte ptr [bx+0Fh], 3
                jz      short loc_1CA0F
                cmp     byte ptr [bx+0Fh], 6
                jz      short loc_1CA0F
                cmp     byte ptr [bx+0Fh], 5
                jnz     short loc_1CA14

loc_1CA0F:                              ; CODE XREF: ovl_2MISC2:CA01↑j
                                        ; ovl_2MISC2:CA07↑j
                mov     word ptr [bp-0Eh], 0

loc_1CA14:                              ; CODE XREF: ovl_2MISC2:CA0D↑j
                cmp     word ptr [bp-0Eh], 0
                jz      short loc_1CA1D
                inc     word ptr [bp-4]

loc_1CA1D:                              ; CODE XREF: ovl_2MISC2:CA18↑j
                mov     bx, [bp-0Ah]
                mov     al, [bx+27h]
                sub     ah, ah
                push    ax
                call    thk_res_354A
                add     sp, 2
                mov     [bp-0Ch], al
                cmp     al, 0F0h
                jb      short loc_1CA37
                mov     byte ptr [bp-0Ch], 0

loc_1CA37:                              ; CODE XREF: ovl_2MISC2:CA31↑j
                mov     al, [bp-0Ch]
                sub     ah, ah
                add     [bp-4], ax
                mov     bx, [bp-0Ah]
                mov     ax, [bp-4]
                add     [bx+60h], ax
                add     [bx+74h], ax
                add     [bx+5Eh], ax
                cmp     byte ptr [bx+0Fh], 3

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                jz      short loc_1CA66
                cmp     byte ptr [bx+0Fh], 4
                jz      short loc_1CA66
                cmp     byte ptr [bx+0Fh], 2
                jz      short loc_1CA66
                cmp     byte ptr [bx+0Fh], 1
                jnz     short loc_1CA72

loc_1CA66:                              ; CODE XREF: ovl_2MISC2:loc_1CA52↑j
                                        ; ovl_2MISC2:CA58↑j ...
                push    word ptr [bp+4]
                call    loc_1C6CC
                add     sp, 2
                mov     [bp-2], ax

loc_1CA72:                              ; CODE XREF: ovl_2MISC2:CA64↑j
                mov     ax, 13h
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aYouGained ; "You gained "
                push    ax
                call    thk_text_puts
                add     sp, 2           ; CODE XREF: seg002:0801↑J
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [bp-4]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 14h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aHitPoints ; " hit points"
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word ptr [bp-2], 0
                jz      short loc_1CACE
                mov     ax, 15h
                push    ax
                mov     ax, 14h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAndNewSpells ; "and new spells"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CACE:                              ; CODE XREF: ovl_2MISC2:CAB4↑j
                mov     ax, 13h
                push    ax
                mov     ax, 0Fh
                push    ax
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp-0Ah]
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                add     sp, 8

loc_1CB09:                              ; CODE XREF: ovl_2MISC2:C8F0↑j
                                        ; ovl_2MISC2:C966↑j
                call    thk_monster_anim_step
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: training_hall+20A↓p
                mov     bp, sp
                push    word ptr [bp+4]
                call    thk_res_6532
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 7
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+4]
                shl     bx, 1
                mov     ax, 82h
                imul    word ptr [bx+416h]

loc_1CB40:                              ; CODE XREF: seg002:0A65↑J
                mov     bx, ax
                push    word ptr [bx+7E88h]
                push    word ptr [bx+7E86h]
                call    thk_text_put_number
                add     sp, 8
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: training_hall+DA↓p
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, [bp+4]
                shl     bx, 1
                mov     ax, [bx+416h]
                mov     [bp-2], ax
                mov     ax, 14h
                push    ax
                mov     ax, 0Fh
                push    ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+4]      ; CODE XREF: seg002:0AF5↑J
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 2FDCh
                push    ax
                call    thk_text_puts   ; CODE XREF: seg002:080D↑J
                add     sp, 2
                mov     ax, 82h
                imul    word ptr [bp-2]
                mov     si, ax
                add     ax, 7E20h
                push    ax
                call    thk_text_puts
                add     sp, 2
                cmp     word ptr [bp-2], 18h
                jge     short loc_1CC00
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGold_0 ; "Gold="
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [si+7E88h]

loc_1CBDC:                              ; CODE XREF: seg002:01AD↑J
                push    word ptr [si+7E86h]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGGatherGold ; "G-Gather Gold"
                push    ax
                call    thk_text_puts
                add     sp, 2
                jmp     short loc_1CC36
; ---------------------------------------------------------------------------

loc_1CC00:                              ; CODE XREF: ovl_2MISC2:CBB6↑j
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCost ; "Cost="
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     ax, 82h
                imul    word ptr [bp-2]
                mov     bx, ax
                push    word ptr [bx+7E88h]
                push    word ptr [bx+7E86h]
                call    thk_text_put_number
                add     sp, 8

loc_1CC36:                              ; CODE XREF: ovl_2MISC2:CBFE↑j
                mov     ax, 82h
                imul    word ptr [bp-2]
                add     ax, 7E20h
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: training_hall+194↓p
                mov     bp, sp
                sub     sp, 4
                mov     bx, [bp+6]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jl      short loc_1CC5C
                sub     ax, ax
                cwd
                jmp     short loc_1CC88
; ---------------------------------------------------------------------------
                align 2

loc_1CC5C:                              ; CODE XREF: ovl_2MISC2:CC54↑j
                mov     bx, [bp+4]
                mov     al, [bx+20h]
                sub     ah, ah
                mov     [bp-2], ax
                cmp     ax, 0FFh
                jz      short loc_1CC6F
                inc     word ptr [bp-2]

loc_1CC6F:                              ; CODE XREF: ovl_2MISC2:CC6A↑j
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+2F04h]
                mul     word ptr [bp-2]
                mov     cx, 32h ; '2'
                mul     cx
                mov     [bp-4], ax
                sub     dx, dx

loc_1CC88:                              ; CODE XREF: ovl_2MISC2:CC59↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: training_hall+F0↓p
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     bx, [bp+4]
                mov     al, [bx+20h]
                mov     [bp-6], al
                cmp     al, 0FFh
                jz      short loc_1CCA3
                inc     byte ptr [bp-6]

loc_1CCA3:                              ; CODE XREF: ovl_2MISC2:CC9E↑j
                cmp     byte ptr [bx+0Fh], 1
                jz      short loc_1CCBB
                cmp     byte ptr [bx+0Fh], 2
                jz      short loc_1CCBB
                cmp     byte ptr [bx+0Fh], 4
                jz      short loc_1CCBB
                cmp     byte ptr [bx+0Fh], 6
                jnz     short loc_1CCC0 ; CODE XREF: seg002:08FD↑J

loc_1CCBB:                              ; CODE XREF: ovl_2MISC2:CCA7↑j
                                        ; ovl_2MISC2:CCAD↑j ...
                mov     ax, 1
                jmp     short loc_1CCC2
; ---------------------------------------------------------------------------

loc_1CCC0:                              ; CODE XREF: ovl_2MISC2:CCB9↑j
                sub     ax, ax

loc_1CCC2:                              ; CODE XREF: ovl_2MISC2:CCBE↑j
                mov     [bp-2], ax
                mov     al, [bp-6]
                mov     [bp-4], al
                cmp     al, 0Ah
                jb      short loc_1CCD3
                mov     byte ptr [bp-4], 0Ah

loc_1CCD3:                              ; CODE XREF: ovl_2MISC2:CCCD↑j
                mov     ax, 24h ; '$'
                imul    word ptr [bp-2]
                mov     si, ax
                mov     bl, [bp-4]
                sub     bh, bh
                shl     bx, 1
                shl     bx, 1
                mov     ax, [bx+si+2E5Ch]
                mov     dx, [bx+si+2E5Eh]
                mov     [bp-0Ah], ax
                mov     [bp-8], dx
                cmp     byte ptr [bp-6], 0Bh
                jb      short loc_1CD01
                add     word ptr [bp-0Ah], 0EE00h
                adc     word ptr [bp-8], 2

loc_1CD01:                              ; CODE XREF: ovl_2MISC2:CCF6↑j
                cmp     byte ptr [bp-6], 0Ch
                jb      short loc_1CD10
                add     word ptr [bp-0Ah], 0EE00h
                adc     word ptr [bp-8], 2

loc_1CD10:                              ; CODE XREF: ovl_2MISC2:CD05↑j
                cmp     byte ptr [bp-6], 0Dh
                jb      short loc_1CD1F
                add     word ptr [bp-0Ah], 0EE00h
                adc     word ptr [bp-8], 2

loc_1CD1F:                              ; CODE XREF: ovl_2MISC2:CD14↑j
                cmp     byte ptr [bp-6], 0Eh
                jb      short loc_1CD2E
                add     word ptr [bp-0Ah], 0DC00h
                adc     word ptr [bp-8], 5

loc_1CD2E:                              ; CODE XREF: ovl_2MISC2:CD23↑j
                cmp     byte ptr [bp-6], 0Fh
                jb      short loc_1CD3D
                add     word ptr [bp-0Ah], 0DC00h
                adc     word ptr [bp-8], 5

loc_1CD3D:                              ; CODE XREF: ovl_2MISC2:CD32↑j
                cmp     byte ptr [bp-6], 10h
                jb      short loc_1CD6D
                mov     al, [bp-6]
                sub     al, 0Fh
                mov     [bp-4], al
                cmp     al, 5
                jbe     short loc_1CD53
                mov     byte ptr [bp-4], 5

loc_1CD53:                              ; CODE XREF: ovl_2MISC2:CD4D↑j
                mov     ax, 0B800h
                mov     dx, 0Bh
                push    dx
                push    ax
                mov     al, [bp-4]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                add     [bp-0Ah], ax
                adc     [bp-8], dx

loc_1CD6D:                              ; CODE XREF: ovl_2MISC2:CD41↑j
                cmp     byte ptr [bp-6], 15h
                jb      short loc_1CD9D
                mov     al, [bp-6]
                sub     al, 14h
                mov     [bp-4], al
                cmp     al, 0Ah
                jbe     short loc_1CD83
                mov     byte ptr [bp-4], 0Ah

loc_1CD83:                              ; CODE XREF: ovl_2MISC2:CD7D↑j
                mov     ax, 7000h
                mov     dx, 17h
                push    dx
                push    ax
                mov     al, [bp-4]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                add     [bp-0Ah], ax
                adc     [bp-8], dx

loc_1CD9D:                              ; CODE XREF: ovl_2MISC2:CD71↑j
                cmp     byte ptr [bp-6], 1Fh
                jb      short loc_1CDCD
                mov     al, [bp-6]
                sub     al, 1Eh
                mov     [bp-4], al
                cmp     al, 14h
                jbe     short loc_1CDB3
                mov     byte ptr [bp-4], 14h

loc_1CDB3:                              ; CODE XREF: ovl_2MISC2:CDAD↑j
                mov     ax, 0E000h
                mov     dx, 2Eh ; '.'
                push    dx
                push    ax
                mov     al, [bp-4]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                add     [bp-0Ah], ax
                adc     [bp-8], dx

loc_1CDCD:                              ; CODE XREF: ovl_2MISC2:CDA1↑j
                cmp     byte ptr [bp-6], 33h ; '3'
                jb      short loc_1CDFC
                mov     al, [bp-6]
                sub     al, 32h ; '2'
                mov     [bp-4], al
                cmp     al, 19h
                jbe     short loc_1CDE3
                mov     byte ptr [bp-4], 19h

loc_1CDE3:                              ; CODE XREF: ovl_2MISC2:CDDD↑j
                sub     ax, ax
                mov     dx, 19h
                push    dx
                push    ax
                mov     al, [bp-4]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                add     [bp-0Ah], ax
                adc     [bp-8], dx

loc_1CDFC:                              ; CODE XREF: ovl_2MISC2:CDD1↑j
                cmp     byte ptr [bp-6], 4Ch ; 'L'
                jb      short loc_1CE24
                mov     al, [bp-6]
                sub     al, 4Bh ; 'K'
                mov     [bp-4], al
                mov     ax, 0C000h
                mov     dx, 5Dh ; ']'
                push    dx
                push    ax
                mov     al, [bp-4]
                sub     ah, ah
                sub     cx, cx
                push    cx
                push    ax
                call    thk__aFulmul
                add     [bp-0Ah], ax
                adc     [bp-8], dx

loc_1CE24:                              ; CODE XREF: ovl_2MISC2:CE00↑j
                mov     ax, [bp-0Ah]
                mov     dx, [bp-8]
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Training", "Train for level ", "You need ... more experience", "Cost in gold ="
; Attributes: bp-based frame

training_hall   proc near               ; CODE XREF: seg002:0831↑J

var_18          = word ptr -18h
var_16          = word ptr -16h
var_14          = word ptr -14h
var_12          = word ptr -12h
var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 1Ah
                push    di
                push    si
                mov     [bp+var_8], 0
                mov     [bp+var_18], 1
                mov     byte_2294F, 0FDh
                or      byte_1DC80, 6
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CE59:                              ; CODE XREF: training_hall+52↓j
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
                push    word ptr [bx+di+2E3Ch]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1CE59
                mov     [bp+var_14], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jnz     short loc_1CE94
                jmp     loc_1D09B
; ---------------------------------------------------------------------------

loc_1CE94:                              ; CODE XREF: training_hall+5F↑j
                or      byte_1DC80, 1
                call    thk_res_34BA
                call    thk_res_5440
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
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
                mov     ax, offset aTraining ; "  Training  "
                push    ax
                call    thk_text_puts

loc_1CEC8:                              ; CODE XREF: seg002:07AD↑J
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOtherChar ; "#-Other Char"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     di, [bp+var_6]

loc_1CEEF:                              ; CODE XREF: training_hall+25F↓j
                cmp     [bp+var_18], 0
                jnz     short loc_1CEF8
                jmp     loc_1D01A
; ---------------------------------------------------------------------------

loc_1CEF8:                              ; CODE XREF: training_hall+C3↑j
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     [bp+var_18], 0
                push    [bp+var_8]
                call    loc_1CB52
                add     sp, 2
                mov     di, ax
                mov     al, [di+20h]
                mov     [bp+var_A], al
                cmp     al, 0FFh
                jz      short loc_1CF1F
                inc     [bp+var_A]

loc_1CF1F:                              ; CODE XREF: training_hall+EA↑j
                push    di
                call    loc_1CC8C
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx

loc_1CF2C:                              ; CODE XREF: seg002:050D↑J
                mov     ax, 12h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTrainForLevel ; "Train for level "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_A]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]  ; CODE XREF: seg002:0AE9↑J
                cmp     [di+64h], dx
                jb      short loc_1CF70
                ja      short loc_1CF6A
                cmp     [di+62h], ax
                jb      short loc_1CF70

loc_1CF6A:                              ; CODE XREF: training_hall+133↑j
                mov     ax, 1
                jmp     short loc_1CF72
; ---------------------------------------------------------------------------
                align 2

loc_1CF70:                              ; CODE XREF: training_hall+131↑j
                                        ; training_hall+138↑j
                sub     ax, ax

loc_1CF72:                              ; CODE XREF: training_hall+13D↑j
                mov     [bp+var_16], ax
                mov     ax, 14h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+var_16], 0  ; CODE XREF: seg002:062D↑J
                jnz     short loc_1CFC0
                mov     ax, offset aYouNeed ; "  You need "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     ax, [bp+var_4]
                mov     dx, [bp+var_2]
                sub     ax, [di+62h]
                sbb     dx, [di+64h]
                push    dx
                push    ax
                call    thk_text_put_number
                add     sp, 8
                mov     ax, 15h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMoreExperience ; "more experience."
                jmp     short loc_1D013
; ---------------------------------------------------------------------------
                align 2

loc_1CFC0:                              ; CODE XREF: training_hall+157↑j
                push    [bp+var_8]
                push    di
                call    loc_1CC44
                add     sp, 4
                mov     [bp+var_E], ax
                mov     [bp+var_C], dx
                mov     ax, offset aCostInGold ; "Cost in gold = "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, [bp+var_E]
                or      ax, [bp+var_C]
                jz      short loc_1CFF8
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    [bp+var_C]
                push    [bp+var_E]
                call    thk_text_put_number
                add     sp, 8
                jmp     short loc_1D002
; ---------------------------------------------------------------------------

loc_1CFF8:                              ; CODE XREF: training_hall+1B0↑j
                mov     ax, offset aFree ; "free"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1D002:                              ; CODE XREF: training_hall+1C6↑j
                mov     ax, 15h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aPressTToTrain ; "Press 'T' to train"

loc_1D013:                              ; CODE XREF: training_hall+18D↑j
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1D01A:                              ; CODE XREF: training_hall+C5↑j
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 47h ; 'G'
                jnz     short loc_1D042
                mov     bx, [bp+var_8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1D042
                push    [bp+var_8]
                call    loc_1CB12
                add     sp, 2
                jmp     short loc_1D08A
; ---------------------------------------------------------------------------

loc_1D042:                              ; CODE XREF: training_hall+1F9↑j
                                        ; training_hall+205↑j
                mov     ax, si
                cmp     ax, 54h ; 'T'
                jnz     short loc_1D066
                cmp     [bp+var_16], 0
                jz      short loc_1D066
                push    [bp+var_C]
                push    [bp+var_E]
                push    [bp+var_8]
                call    loc_1C86A
                add     sp, 6
                mov     [bp+var_18], 1
                jmp     short loc_1D08A
; ---------------------------------------------------------------------------
                align 2

loc_1D066:                              ; CODE XREF: training_hall+217↑j
                                        ; training_hall+21D↑j
                cmp     si, 1Bh
                jz      short loc_1D08A
                mov     ax, si
                sub     ax, 31h ; '1'
                mov     [bp+var_12], ax
                or      ax, ax
                jl      short loc_1D08A
                mov     ax, g_party_size
                cmp     [bp+var_12], ax
                jge     short loc_1D08A
                mov     [bp+var_18], 1
                mov     ax, [bp+var_12]
                mov     [bp+var_8], ax

loc_1D08A:                              ; CODE XREF: training_hall+210↑j
                                        ; training_hall+233↑j ...
                cmp     si, 1Bh
                jz      short loc_1D092
                jmp     loc_1CEEF
; ---------------------------------------------------------------------------

loc_1D092:                              ; CODE XREF: training_hall+25D↑j
                mov     [bp+var_6], di
                mov     [bp+var_10], si
                call    thk_res_35A8

loc_1D09B:                              ; CODE XREF: training_hall+61↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 10h
training_hall   endp

ovl_2MISC2      ends

