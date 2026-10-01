; ===========================================================================

; Segment type: Pure code
ovl_2BRAIN      segment byte public 'CODE' use16
                assume cs:ovl_2BRAIN
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

tavern_common_helper proc near          ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...

var_1A          = word ptr -1Ah
var_18          = word ptr -18h
var_16          = word ptr -16h
var_14          = word ptr -14h
var_12          = word ptr -12h
var_10          = word ptr -10h
var_E           = word ptr -0Eh
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp          ; DATA XREF: seg002:0038↑o
                sub     sp, 1Ah
                push    di
                push    si
                sub     si, si
                or      byte_1DC80, 2
                mov     [bp+var_A], si
                jmp     short loc_1C14F
; ---------------------------------------------------------------------------

loc_1C144:                              ; CODE XREF: tavern_common_helper+4E↓j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C180
                jmp     short loc_1C16C
; ---------------------------------------------------------------------------

loc_1C14C:                              ; CODE XREF: tavern_common_helper+58↓j
                inc     [bp+var_A]

loc_1C14F:                              ; CODE XREF: tavern_common_helper+12↑j
                mov     ax, g_party_size
                cmp     [bp+var_A], ax
                jge     short loc_1C18A
                push    [bp+var_A]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_10], 0
                mov     di, ax
                sub     cx, cx

loc_1C16C:                              ; CODE XREF: tavern_common_helper+1A↑j
                mov     bx, cx
                mov     dl, [bx+di+3Ah]
                cmp     dl, 0D0h
                jb      short loc_1C17C
                cmp     dl, 0D3h
                ja      short loc_1C17C
                inc     si

loc_1C17C:                              ; CODE XREF: tavern_common_helper+44↑j
                                        ; tavern_common_helper+49↑j
                or      si, si
                jz      short loc_1C144

loc_1C180:                              ; CODE XREF: tavern_common_helper+18↑j
                mov     [bp+var_2], dl
                mov     [bp+var_10], cx
                or      si, si
                jz      short loc_1C14C

loc_1C18A:                              ; CODE XREF: tavern_common_helper+25↑j
                mov     [bp+var_12], si
                or      si, si
                jnz     short loc_1C1C6
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSorryButYouMus ; "Sorry, but you must have a ticket to"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCompeteInThese ; "compete in these games."
                jmp     loc_1C3A1
; ---------------------------------------------------------------------------

loc_1C1C6:                              ; CODE XREF: tavern_common_helper+5F↑j
                mov     al, [bp+var_2]
                sub     al, 0D0h
                mov     byte_1DC7F, al
                push    [bp+var_10]
                push    [bp+var_4]
                call    thk_char_backpack_remove
                add     sp, 4

loc_1C1DA:                              ; CODE XREF: seg002:08CD↑J
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 1

loc_1C1EA:                              ; CODE XREF: seg002:07F5↑J
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTheGamesMaster ; "The games master accepts your ticket."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 9
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aLetTheBattleBe ; " Let the battle begin!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     al, byte_1DC7F
                mov     cx, ax
                shl     al, 1
                add     al, cl
                mov     [bp+var_6], al
                mov     bl, g_map_id
                sub     bh, bh
                mov     al, [bx+4102h]
                add     [bp+var_6], al
                mov     cl, 4
                shl     [bp+var_6], cl
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                add     sp, 4
                add     [bp+var_6], al

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                mov     [bp+var_A], 0
                sub     ax, ax
                mov     cx, 5
                mov     di, 9680h
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                stosb
                add     [bp+var_A], 0Bh
                sub     si, si
                mov     cx, g_party_size
                jmp     short loc_1C268
; ---------------------------------------------------------------------------

loc_1C260:                              ; CODE XREF: tavern_common_helper+13A↓j
                mov     al, [bp+var_6]
                mov     [si-6980h], al
                inc     si

loc_1C268:                              ; CODE XREF: tavern_common_helper+12E↑j
                cmp     si, cx
                jl      short loc_1C260
                mov     [bp+var_A], si
                mov     byte_1DC65, 80h
                call    thk_start_combat
                cmp     byte_1DD59, 0
                jnz     short loc_1C286
                mov     byte ptr g_party_y+1, 1
                jmp     loc_1C3A8
; ---------------------------------------------------------------------------

loc_1C286:                              ; CODE XREF: tavern_common_helper+14C↑j
                mov     byte_1DD59, 0
                call    thk_clear_spell_effects
                mov     al, [bp+var_2]
                sub     al, 0D0h
                mov     [bp+var_6], al
                mov     al, g_map_id
                mov     byte ptr [bp+var_8], al
                cmp     al, 2
                jbe     short loc_1C2A4
                mov     byte ptr [bp+var_8], 2

loc_1C2A4:                              ; CODE XREF: tavern_common_helper+16E↑j
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                shl     bx, 1
                mov     si, [bp+var_8]
                and     si, 0FFh
                shl     si, 1
                shl     si, 1
                mov     ax, [bx+si+4108h] ; CODE XREF: seg002:026D↑J
                mov     dx, [bx+si+410Ah]
                mov     [bp+var_E], ax
                mov     [bp+var_C], dx
                mov     [bp+var_A], 0
                cmp     g_party_size, 0
                jle     short loc_1C34D
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                mov     cl, byte ptr [bp+var_8]
                sub     ch, ch
                add     ax, cx
                mov     [bp+var_1A], ax
                add     ax, 4144h
                mov     [bp+var_14], ax
                mov     ax, [bp+var_1A]
                add     ax, 4138h       ; CODE XREF: seg002:0651↑J
                mov     [bp+var_16], ax
                mov     [bp+var_18], 416h
                mov     si, [bp+var_A]

loc_1C304:                              ; CODE XREF: tavern_common_helper+215↓j
                push    si
                call    thk_char_ptr

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                add     sp, 2
                mov     di, ax
                mov     bx, [bp+var_18]
                cmp     word ptr [bx], 18h
                jge     short loc_1C329
                mov     ax, [bp+var_E]
                mov     dx, [bp+var_C]
                add     [di+66h], ax
                adc     [di+68h], dx
                sub     ax, ax
                mov     [bp+var_C], ax
                mov     [bp+var_E], ax

loc_1C329:                              ; CODE XREF: tavern_common_helper+1E3↑j
                mov     bx, [bp+var_16]
                mov     bl, [bx]
                sub     bh, bh
                lea     ax, [bx+di+79h]
                mov     bx, [bp+var_14]
                mov     cl, [bx]
                mov     bx, ax
                or      [bx], cl
                add     [bp+var_18], 2
                inc     si
                cmp     si, g_party_size
                jl      short loc_1C304
                mov     [bp+var_4], di
                mov     [bp+var_A], si

loc_1C34D:                              ; CODE XREF: tavern_common_helper+1A6↑j
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aWinnerYouRecei ; "Winner, you receive "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '   ; CODE XREF: seg002:0471↑J
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [bp+var_6]
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                shl     bx, 1
                mov     si, [bp+var_8]
                and     si, 0FFh
                shl     si, 1
                shl     si, 1
                push    word ptr [bx+si+410Ah]
                push    word ptr [bx+si+4108h]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, offset aGold_1 ; " gold"

loc_1C3A1:                              ; CODE XREF: tavern_common_helper+93↑j
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C3A8:                              ; CODE XREF: tavern_common_helper+153↑j
                cmp     byte ptr g_party_y+1, 0
                jnz     short loc_1C3B9

loc_1C3AF:                              ; CODE XREF: tavern_common_helper+284↓j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1C3AF
                call    thk_2PLAY_A580

loc_1C3B9:                              ; CODE XREF: tavern_common_helper+27D↑j
                or      byte_23218, 80h
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
                sub     bh, bh
                or      byte ptr [bx+si+5AD6h], 80h
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
tavern_common_helper endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C3DC       proc near               ; CODE XREF: tavern_retrain_skills+14↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                or      byte_1DC80, 1
                mov     bx, word ptr asc_2194D+3 ; ">@"
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+1Eh], al
                call    thk_print_gold_label

loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                push    word ptr asc_2194D+3 ; ">@"
                call    thk_print_message_line
                add     sp, 2
                mov     bx, word ptr asc_2194D+3 ; ">@"
                mov     al, [bx+1Eh]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C421
                jmp     loc_1C5C2
; ---------------------------------------------------------------------------

loc_1C421:                              ; CODE XREF: sub_1C3DC+40↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     [bp+var_4], 31h ; '1'
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aMagic_1 ; "Magic ------- "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax

loc_1C462:                              ; CODE XREF: seg002:086D↑J
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+16h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 13h
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aEnergy ; "Energy ------ "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '   ; CODE XREF: seg002:0B31↑J
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+1Ah]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aFire ; "Fire -------- "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+17h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 14h
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSleep_0 ; "Sleep ------- "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+1Bh]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aElectricity ; "Electricity - "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+18h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                                        ; CODE XREF: seg002:0879↑J
                add     sp, 6
                mov     ax, 15h
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aPoison ; "Poison ------ "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+1Ch]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aCold ; "Cold -------- "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp+var_2]
                mov     al, [bx+19h]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 16h
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAcid ; "Acid -------- "
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax

loc_1C5AC:                              ; CODE XREF: seg002:0885↑J
                mov     bx, [bp+var_2]
                mov     al, [bx+1Dh]
                sub     ah, ah
                push    ax
                call    thk_text_put_number_pad
                add     sp, 6

loc_1C5BB:                              ; CODE XREF: sub_1C3DC:loc_1C5C0↓j
                call    thk_2PLAY_8282
                or      ax, ax

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                jz      short loc_1C5BB

loc_1C5C2:                              ; CODE XREF: sub_1C3DC+42↑j
                call    thk_text_clear_prompt_line
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C5CA:                              ; CODE XREF: tavern_retrain_skills+131↓p
                                        ; tavern_retrain_skills+145↓p
                push    bp
                mov     bp, sp
                mov     al, [bp+arg_2]
                sub     ah, ah
                sub     ax, 1           ; switch 15 cases
                cmp     ax, 0Eh
sub_1C3DC       endp


loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                jbe     short loc_1C5DD
                jmp     def_1C5E0       ; jumptable 0001C5E0 default case, cases 3,4,10-13
; ---------------------------------------------------------------------------

loc_1C5DD:                              ; CODE XREF: ovl_2BRAIN:loc_1C5D8↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1C5E0[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1C5E6:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:jpt_1C5E0↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 14h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Fh ; 'o'

loc_1C601:                              ; CODE XREF: ovl_2BRAIN:C627↓j
                                        ; ovl_2BRAIN:C645↓j ...
                push    ax
                call    thk_res_35F0
                add     sp, 4
                jmp     def_1C5E0       ; jumptable 0001C5E0 default case, cases 3,4,10-13
; ---------------------------------------------------------------------------
                align 2

loc_1C60C:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7C4↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 2
                push    ax
                mov     ax, [bp+4]
                add     ax, 13h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Eh ; 'n'
                jmp     short loc_1C601
; ---------------------------------------------------------------------------
                align 2

loc_1C62A:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7CA↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 12h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Dh ; 'm'
                jmp     short loc_1C601
; ---------------------------------------------------------------------------
                align 2

loc_1C648:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7CC↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 6
                push    ax
                mov     ax, [bp+4]
                add     ax, 15h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5

loc_1C65C:                              ; CODE XREF: ovl_2BRAIN:C775↓j
                push    ax
                mov     ax, [bp+4]
                add     ax, 70h ; 'p'
                jmp     short loc_1C601
; ---------------------------------------------------------------------------
                align 2

loc_1C666:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7CE↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 7
                push    ax
                mov     ax, [bp+4]
                add     ax, 10h         ; CODE XREF: seg002:0891↑J
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Bh ; 'k'
                jmp     loc_1C601
; ---------------------------------------------------------------------------

loc_1C684:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7D0↓o
                mov     ax, 1           ; jumptable 0001C5E0 case 8
                push    ax
                mov     ax, [bp+4]
                add     ax, 10h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 11h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 12h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 13h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 14h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 15h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Eh         ; CODE XREF: seg002:0B19↑J
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 27h ; '''
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Bh ; 'k'
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Eh ; 'n'
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Fh ; 'o'
                push    ax
                call    thk_res_35F0    ; CODE XREF: seg002:089D↑J
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 73h ; 's'
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Ch ; 'l'
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Dh ; 'm'
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 1
                jmp     loc_1C65C
; ---------------------------------------------------------------------------

loc_1C778:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7D2↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 9
                push    ax
                mov     ax, [bp+4]
                add     ax, 11h
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 6Ch ; 'l'
                jmp     loc_1C601
; ---------------------------------------------------------------------------

loc_1C796:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7DC↓o
                mov     ax, 0Fh         ; jumptable 0001C5E0 case 14
                push    ax
                mov     ax, [bp+4]
                add     ax, 1Eh
                jmp     loc_1C601
; ---------------------------------------------------------------------------
                align 2

loc_1C7A4:                              ; CODE XREF: ovl_2BRAIN:C5E0↑j
                                        ; DATA XREF: ovl_2BRAIN:C7DE↓o
                mov     ax, 5           ; jumptable 0001C5E0 case 15
                push    ax
                mov     ax, [bp+4]
                add     ax, 27h ; '''
                push    ax
                call    thk_res_35F0
                add     sp, 4
                mov     ax, 5
                push    ax
                mov     ax, [bp+4]
                add     ax, 73h ; 's'
                jmp     loc_1C601
; ---------------------------------------------------------------------------
jpt_1C5E0       dw offset loc_1C5E6     ; DATA XREF: ovl_2BRAIN:C5E0↑r
                                        ; jump table for switch statement
                dw offset loc_1C60C     ; jumptable 0001C5E0 case 2
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset loc_1C62A     ; jumptable 0001C5E0 case 5
                dw offset loc_1C648     ; jumptable 0001C5E0 case 6
                dw offset loc_1C666     ; jumptable 0001C5E0 case 7
                dw offset loc_1C684     ; jumptable 0001C5E0 case 8
                dw offset loc_1C778     ; jumptable 0001C5E0 case 9
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset def_1C5E0     ; jumptable 0001C5E0 default case, cases 3,4,10-13
                dw offset loc_1C796     ; jumptable 0001C5E0 case 14
                dw offset loc_1C7A4     ; jumptable 0001C5E0 case 15
; ---------------------------------------------------------------------------

def_1C5E0:                              ; CODE XREF: ovl_2BRAIN:C5DA↑j
                                        ; ovl_2BRAIN:C5E0↑j ...
                pop     bp              ; jumptable 0001C5E0 default case, cases 3,4,10-13
                retn

; =============== S U B R O U T I N E =======================================

; "you must have 100 gold", "Their secondary skills are gone"
; Attributes: bp-based frame

tavern_retrain_skills proc near         ; CODE XREF: seg002:0849↑J

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

; FUNCTION CHUNK AT C9AE SIZE 0000002A BYTES

                push    bp
                mov     bp, sp
                sub     sp, 8

loc_1C7E8:                              ; CODE XREF: seg002:0B0D↑J
                push    di
                push    si
                mov     [bp+var_6], 1
                cmp     g_map_id, 0
                jz      short loc_1C7FC
                call    sub_1C3DC
                jmp     loc_1C945
; ---------------------------------------------------------------------------

loc_1C7FC:                              ; CODE XREF: tavern_retrain_skills+12↑j
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 40F8h

loc_1C80F:                              ; CODE XREF: tavern_retrain_skills+4A↓j
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
                jl      short loc_1C80F
                mov     [bp+var_4], si
                jmp     short loc_1C839
; ---------------------------------------------------------------------------
                align 2

loc_1C834:                              ; CODE XREF: tavern_retrain_skills+66↓j
                cmp     ax, 4Eh ; 'N'
                jz      short loc_1C84A

loc_1C839:                              ; CODE XREF: tavern_retrain_skills+4F↑j
                call    thk_2PLAY_8282
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1C834

loc_1C84A:                              ; CODE XREF: tavern_retrain_skills+55↑j
                mov     [bp+var_4], si
                cmp     si, 59h ; 'Y'
                jz      short loc_1C855
                jmp     loc_1C945
; ---------------------------------------------------------------------------

loc_1C855:                              ; CODE XREF: tavern_retrain_skills+6E↑j
                mov     bx, word_21946
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+18h], al
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 6
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word_21946
                call    thk_text_puts
                add     sp, 2
                mov     bx, word_21946
                mov     al, [bx+18h]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C8A3
                jmp     loc_1C945
; ---------------------------------------------------------------------------

loc_1C8A3:                              ; CODE XREF: tavern_retrain_skills+BC↑j
                mov     ax, 14h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                mov     ax, 14h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                sub     [bp+var_4], 31h ; '1'
                push    [bp+var_4]
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, [bp+var_4]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1C900
                mov     bx, ax
                cmp     word ptr [bx+68h], 0
                jnz     short loc_1C900
                jb      short loc_1C8F3
                cmp     word ptr [bx+66h], 64h ; 'd'
                jnb     short loc_1C900

loc_1C8F3:                              ; CODE XREF: tavern_retrain_skills+109↑j
                dec     [bp+var_6]
                mov     ax, offset aSorryYouMustHa ; "Sorry, you must have 100 gold."
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C900:                              ; CODE XREF: tavern_retrain_skills+FF↑j
                                        ; tavern_retrain_skills+107↑j ...
                cmp     [bp+var_6], 0
                jz      short loc_1C93E
                mov     bx, [bp+var_2]
                mov     al, [bx+50h]
                sub     ah, ah
                and     ax, 0Fh
                push    ax
                push    bx
                call    loc_1C5CA
                add     sp, 4
                mov     bx, [bp+var_2]
                mov     al, [bx+50h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                push    ax
                push    bx
                call    loc_1C5CA
                add     sp, 4
                mov     ax, offset aTheirSecondary ; "Their secondary skills are gone."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     bx, [bp+var_2]
                mov     byte ptr [bx+50h], 0

loc_1C93E:                              ; CODE XREF: tavern_retrain_skills+122↑j
                                        ; tavern_retrain_skills+161↓j
                call    thk_2PLAY_8282
                or      ax, ax
                jz      short loc_1C93E

loc_1C945:                              ; CODE XREF: tavern_retrain_skills+17↑j
                                        ; tavern_retrain_skills+70↑j ...
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C94E:                              ; CODE XREF: ovl_2BRAIN:CC7A↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     ax, [bp+arg_0]
                cmp     ax, 6           ; switch 7 cases
                ja      short def_1C95F ; jumptable 0001C95F default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1C95F[bx] ; switch jump
; ---------------------------------------------------------------------------

loc_1C964:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:jpt_1C95F↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 0
                add     ax, 6Bh ; 'k'

loc_1C96A:                              ; CODE XREF: tavern_retrain_skills+194↓j
                                        ; tavern_retrain_skills+19C↓j ...
                mov     [bp+var_4], ax
                jmp     short def_1C95F ; jumptable 0001C95F default case
; ---------------------------------------------------------------------------
                align 2

loc_1C970:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:C9A2↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 1
                add     ax, 6Fh ; 'o'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------

loc_1C978:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:C9A4↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 2
                add     ax, 6Dh ; 'm'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------

loc_1C980:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:C9A6↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 3
                add     ax, 6Ch ; 'l'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------

loc_1C988:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:C9A8↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 4
                add     ax, 71h ; 'q'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------

loc_1C990:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; DATA XREF: ovl_2BRAIN:C9AA↓o
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 5
                add     ax, 72h ; 'r'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------

loc_1C998:                              ; CODE XREF: tavern_retrain_skills+17D↑j
                                        ; seg002:07DD↑J
                                        ; DATA XREF: ...
                mov     ax, [bp+arg_2]  ; jumptable 0001C95F case 6
tavern_retrain_skills endp

                add     ax, 6Eh ; 'n'
                jmp     short loc_1C96A
; ---------------------------------------------------------------------------
jpt_1C95F       dw offset loc_1C964     ; DATA XREF: tavern_retrain_skills+17D↑r
                                        ; jump table for switch statement
                dw offset loc_1C970     ; jumptable 0001C95F case 1
                dw offset loc_1C978     ; jumptable 0001C95F case 2
                dw offset loc_1C980     ; jumptable 0001C95F case 3
                dw offset loc_1C988     ; jumptable 0001C95F case 4
                dw offset loc_1C990     ; jumptable 0001C95F case 5
                dw offset loc_1C998     ; jumptable 0001C95F case 6
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR tavern_retrain_skills

def_1C95F:                              ; CODE XREF: tavern_retrain_skills+178↑j
                                        ; tavern_retrain_skills+18B↑j
                mov     bx, [bp+var_4]  ; jumptable 0001C95F default case
                mov     al, [bx]
                mov     byte ptr [bp+var_2], al
                mov     bx, [bp+arg_0]
                mov     al, [bx+4248h]
                add     byte ptr [bp+var_2], al
                mov     al, byte ptr [bp+var_2]
                cmp     [bx+424Eh], al
                jbe     short loc_1C9D4
                cmp     [bx+4248h], al
                ja      short loc_1C9D4
                mov     bx, [bp+var_4]
                mov     [bx], al

loc_1C9D4:                              ; CODE XREF: tavern_retrain_skills+1E5↑j
                                        ; tavern_retrain_skills+1EB↑j
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR tavern_retrain_skills

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

tavern_helper_a proc near               ; CODE XREF: ovl_2BRAIN:CEE6↓p

arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                push    si
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax          ; CODE XREF: seg002:0B01↑J
                shl     bx, 1
                mov     si, [bp+arg_2]
                shl     si, 1
                mov     ax, [bx+si+4A4h]
                mov     bx, [bp+arg_0]
                or      [bx+76h], ax
                pop     si
                pop     bp
                retn
tavern_helper_a endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

tavern_helper_b proc near               ; CODE XREF: ovl_2BRAIN:CB70↓p
                                        ; ovl_2BRAIN:CC55↓p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     si, [bp+arg_0]
                shl     si, 1
                push    word ptr [si+5722h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [si+5724h]
                call    thk_text_puts
                add     sp, 2
                call    thk_monster_anim_step
                pop     si
                pop     bp
                retn
tavern_helper_b endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CA46       proc near               ; CODE XREF: ovl_2BRAIN:loc_1CFD2↓p
                                        ; ovl_2BRAIN:loc_1D054↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, g_era
                shl     bx, 1

loc_1CA52:                              ; CODE XREF: seg002:0A89↑J
                mov     ax, [bx+3A2h]
                mov     [bp+var_2], ax
                cmp     ax, 0B4h
                jnz     short loc_1CA66
                mov     [bp+var_2], 3
                jmp     short loc_1CA87
; ---------------------------------------------------------------------------
                align 2

loc_1CA66:                              ; CODE XREF: sub_1CA46+16↑j
                mov     ax, [bp+var_2]
                cwd
                mov     cx, 1Eh
                idiv    cx
                or      dx, dx
                jnz     short loc_1CA7C
                mov     ax, [bp+var_2]
                cwd
                idiv    cx
                jmp     short loc_1CA84
; ---------------------------------------------------------------------------
                align 2

loc_1CA7C:                              ; CODE XREF: sub_1CA46+2B↑j
                mov     ax, [bp+var_2]
                and     ax, 1
                xor     al, 1

loc_1CA84:                              ; CODE XREF: sub_1CA46+33↑j
                mov     [bp+var_2], ax

loc_1CA87:                              ; CODE XREF: sub_1CA46+1D↑j
                                        ; seg002:0801↑J
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1CA46       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

tavern_helper_c proc near               ; CODE XREF: ovl_2BRAIN:CB27↓p
                                        ; ovl_2BRAIN:CC47↓p ...

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_2]
                mov     dx, [bp+arg_4]
                cmp     [bx+68h], dx
                jb      short loc_1CAAE
                ja      short loc_1CAA9
                cmp     [bx+66h], ax
                jb      short loc_1CAAE

loc_1CAA9:                              ; CODE XREF: tavern_helper_c+14↑j
                mov     ax, 1
                jmp     short loc_1CAB0
; ---------------------------------------------------------------------------

loc_1CAAE:                              ; CODE XREF: tavern_helper_c+12↑j
                                        ; tavern_helper_c+19↑j
                sub     ax, ax

loc_1CAB0:                              ; CODE XREF: tavern_helper_c+1E↑j
                mov     [bp+var_2], ax
                or      ax, ax
                jz      short loc_1CB01
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_2]
                mov     dx, [bp+arg_4]
                sub     [bx+66h], ax
                sbb     [bx+68h], dx
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
                mov     bx, [bp+arg_0]
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                add     sp, 8

loc_1CB01:                              ; CODE XREF: tavern_helper_c+27↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
tavern_helper_c endp

; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: tavern_menu+357↓p
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+4226h]
                cwd
                mov     [bp-4], ax
                mov     [bp-2], dx
                push    dx
                push    ax
                push    word ptr [bp+4]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CB36
                mov     ax, 2
                jmp     short loc_1CB6F
; ---------------------------------------------------------------------------

loc_1CB36:                              ; CODE XREF: ovl_2BRAIN:CB2F↑j
                mov     word ptr [bp-6], 0
                cmp     g_party_size, 0

loc_1CB40:                              ; CODE XREF: seg002:0A65↑J
                jle     short loc_1CB6D
                mov     si, 416h
                mov     cx, g_party_size
                mov     ax, cx
                add     [bp-6], ax

loc_1CB4E:                              ; CODE XREF: ovl_2BRAIN:CB6B↓j
                mov     ax, 82h
                imul    word ptr [si]
                mov     bx, ax
                cmp     byte ptr [bx+7E45h], 28h ; '('
                jnb     short loc_1CB68
                mov     ax, 82h
                imul    word ptr [si]
                mov     bx, ax
                mov     byte ptr [bx+7E45h], 28h ; '('

loc_1CB68:                              ; CODE XREF: ovl_2BRAIN:CB5A↑j
                add     si, 2
                loop    loc_1CB4E

loc_1CB6D:                              ; CODE XREF: ovl_2BRAIN:loc_1CB40↑j
                sub     ax, ax

loc_1CB6F:                              ; CODE XREF: ovl_2BRAIN:CB34↑j
                push    ax
                call    tavern_helper_b
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: tavern_menu+36E↓p
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     word ptr [bp-4], 1
                mov     bx, [bp+4]      ; CODE XREF: seg002:0AF5↑J
                mov     ax, [bx]
                mov     [bp-2], ax
                mov     bx, [bp+6]
                mov     ax, [bx]
                mov     [bp-6], ax
                mov     ax, 16h

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 4254h
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CBB1:                              ; CODE XREF: ovl_2BRAIN:CD46↓j
                cmp     word ptr [bp-4], 0
                jz      short loc_1CC04
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 56C6h
                mov     word ptr [bp-10h], 4230h

loc_1CBCB:                              ; CODE XREF: ovl_2BRAIN:CBFF↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts   ; CODE XREF: seg002:01AD↑J
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, [bp-10h]
                push    word ptr [bx]
                call    thk_text_put_number_pad
                add     sp, 6
                add     di, 2
                add     word ptr [bp-10h], 2
                inc     si
                cmp     si, 6
                jl      short loc_1CBCB
                mov     [bp-0Ch], si

loc_1CC04:                              ; CODE XREF: ovl_2BRAIN:CBB5↑j
                mov     word ptr [bp-4], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp-0Ah], ax
                cmp     ax, 41h ; 'A'
                jnb     short loc_1CC1E
                jmp     loc_1CCD8
; ---------------------------------------------------------------------------

loc_1CC1E:                              ; CODE XREF: ovl_2BRAIN:CC19↑j
                cmp     ax, 46h ; 'F'
                jbe     short loc_1CC26
                jmp     loc_1CCD8
; ---------------------------------------------------------------------------

loc_1CC26:                              ; CODE XREF: ovl_2BRAIN:CC21↑j
                mov     bx, [bp-2]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CC34
                mov     ax, 4
                jmp     short loc_1CC54
; ---------------------------------------------------------------------------

loc_1CC34:                              ; CODE XREF: ovl_2BRAIN:CC2D↑j
                sub     word ptr [bp-0Ah], 41h ; 'A'
                mov     bx, [bp-0Ah]
                shl     bx, 1
                mov     ax, [bx+4230h]
                cwd
                push    dx
                push    ax
                push    word ptr [bp-2]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CC5E
                mov     ax, 2

loc_1CC54:                              ; CODE XREF: ovl_2BRAIN:CC32↑j
                                        ; ovl_2BRAIN:CCD0↓j ...
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1CD40
; ---------------------------------------------------------------------------

loc_1CC5E:                              ; CODE XREF: ovl_2BRAIN:CC4F↑j
                mov     ax, [bp-0Ah]
                shl     ax, 1
                mov     [bp-12h], ax
                mov     bx, ax
                mov     si, ax
                mov     ax, [si+423Ch]
                cmp     [bx+5772h], ax
                jl      short loc_1CC90
                push    word ptr [bp-2]
                push    word ptr [bp-0Ah]
                call    loc_1C94E
                add     sp, 4
                mov     bx, [bp-2]
                cmp     byte ptr [bx+6Eh], 2
                jb      short loc_1CC99
                sub     byte ptr [bx+6Eh], 2
                jmp     short loc_1CC99
; ---------------------------------------------------------------------------
                align 2

loc_1CC90:                              ; CODE XREF: ovl_2BRAIN:CC72↑j
                mov     bx, [bp-0Ah]
                shl     bx, 1
                inc     word ptr [bx+5772h]

loc_1CC99:                              ; CODE XREF: ovl_2BRAIN:CC87↑j
                                        ; ovl_2BRAIN:CC8D↑j
                mov     bx, [bp-2]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_lookup_bracket
                add     sp, 2
                sub     ah, ah
                add     ax, 0Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 2           ; CODE XREF: seg002:08FD↑J
                jnz     short loc_1CCD2
                push    word ptr [bp-2]
                call    thk_char_reset_current_stats
                add     sp, 2
                mov     bx, [bp-2]
                or      byte ptr [bx+26h], 8
                mov     ax, 8
                jmp     short loc_1CC54
; ---------------------------------------------------------------------------

loc_1CCD2:                              ; CODE XREF: ovl_2BRAIN:CCBB↑j
                mov     ax, 6
                jmp     loc_1CC54
; ---------------------------------------------------------------------------

loc_1CCD8:                              ; CODE XREF: ovl_2BRAIN:CC1B↑j
                                        ; ovl_2BRAIN:CC23↑j
                mov     word ptr [bp-4], 0
                cmp     word ptr [bp-0Ah], 47h ; 'G'
                jnz     short loc_1CCFC
                push    word ptr [bp-6]
                call    thk_party_gather_gold
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    word ptr [bp-2]
                call    tavern_helper_c
                add     sp, 6
                jmp     short loc_1CD40
; ---------------------------------------------------------------------------
                align 2

loc_1CCFC:                              ; CODE XREF: ovl_2BRAIN:CCE1↑j
                cmp     word ptr [bp-0Ah], 1Bh
                jz      short loc_1CD40
                mov     ax, [bp-0Ah]
                sub     ax, 31h ; '1'
                mov     [bp-8], ax
                or      ax, ax
                jl      short loc_1CD40
                mov     ax, g_party_size
                cmp     [bp-8], ax
                jge     short loc_1CD40
                mov     ax, [bp-6]
                cmp     [bp-8], ax
                jz      short loc_1CD40
                mov     bx, [bp-8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CD40
                mov     ax, [bp-8]
                mov     [bp-6], ax
                call    loc_1D13C
                push    word ptr [bp-8]
                call    loc_1D0BA
                add     sp, 2
                mov     [bp-2], ax

loc_1CD40:                              ; CODE XREF: ovl_2BRAIN:CC5B↑j
                                        ; ovl_2BRAIN:CCF9↑j ...
                cmp     word ptr [bp-0Ah], 1Bh
                jz      short loc_1CD49
                jmp     loc_1CBB1
; ---------------------------------------------------------------------------

loc_1CD49:                              ; CODE XREF: ovl_2BRAIN:CD44↑j
                mov     bx, [bp+6]
                mov     ax, [bp-6]
                mov     [bx], ax
                mov     bx, [bp+4]
                mov     ax, [bp-2]
                mov     [bx], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: tavern_menu+37E↓p
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     word ptr [bp-2], 1
                mov     bx, [bp+4]
                mov     ax, [bx]
                mov     [bp-4], ax
                mov     bx, [bp+6]
                mov     ax, [bx]
                mov     [bp-6], ax
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBuyAC ; "Buy (A-C)   "
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CD95:                              ; CODE XREF: ovl_2BRAIN:CF5B↓j
                cmp     word ptr [bp-2], 0
                jnz     short loc_1CD9E
                jmp     loc_1CE47
; ---------------------------------------------------------------------------

loc_1CD9E:                              ; CODE XREF: ovl_2BRAIN:CD99↑j
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CDAC:                              ; CODE XREF: ovl_2BRAIN:CE03↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+di+577Eh]
                call    thk_text_puts
                add     sp, 2
                lea     ax, [si+12h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                shl     bx, 1
                push    word ptr [bx+di+5780h]
                call    thk_text_puts
                add     sp, 2
                add     di, 4
                add     si, 2
                cmp     si, 6
                jl      short loc_1CDAC
                mov     [bp-0Ch], si
                mov     word ptr [bp-0Ch], 3
                mov     di, 12h
                sub     si, si

loc_1CE12:                              ; CODE XREF: ovl_2BRAIN:CE45↓j
                push    di
                mov     ax, 22h ; '"'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax

loc_1CE30:                              ; CODE XREF: seg002:0831↑J
                shl     bx, 1
                push    word ptr [bx+si+4208h]
                call    thk_text_put_number_pad
                add     sp, 6
                add     di, 2
                add     si, 2
                cmp     si, 6
                jb      short loc_1CE12

loc_1CE47:                              ; CODE XREF: ovl_2BRAIN:CD9B↑j
                mov     word ptr [bp-2], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp-0Ah], ax
                cmp     ax, 41h ; 'A'
                jnb     short loc_1CE61
                jmp     loc_1CEF2
; ---------------------------------------------------------------------------

loc_1CE61:                              ; CODE XREF: ovl_2BRAIN:CE5C↑j
                cmp     ax, 43h ; 'C'
                jbe     short loc_1CE69
                jmp     loc_1CEF2
; ---------------------------------------------------------------------------

loc_1CE69:                              ; CODE XREF: ovl_2BRAIN:CE64↑j
                mov     bx, [bp-4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CE78
                mov     ax, 4
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------
                align 2

loc_1CE78:                              ; CODE XREF: ovl_2BRAIN:CE70↑j
                sub     word ptr [bp-0Ah], 41h ; 'A'
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                mov     si, [bp-0Ah]
                shl     si, 1
                mov     ax, [bx+si+4208h]
                cwd
                push    dx
                push    ax
                push    word ptr [bp-4]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CEB0
                mov     ax, 2

loc_1CEA5:                              ; CODE XREF: ovl_2BRAIN:CE75↑j
                                        ; ovl_2BRAIN:CEDE↓j ...
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1CF55
; ---------------------------------------------------------------------------
                align 2

loc_1CEB0:                              ; CODE XREF: ovl_2BRAIN:CEA0↑j
                mov     bx, [bp-4]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_lookup_bracket
                add     sp, 2
                sub     ah, ah
                add     ax, 5
                push    ax
                mov     ax, 1

loc_1CEC8:                              ; CODE XREF: seg002:07AD↑J
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 1
                jnz     short loc_1CEE0
                mov     bx, [bp-4]
                or      byte ptr [bx+26h], 4
                mov     ax, 0Ah
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------

loc_1CEE0:                              ; CODE XREF: ovl_2BRAIN:CED2↑j
                push    word ptr [bp-0Ah]
                push    word ptr [bp-4]
                call    tavern_helper_a
                add     sp, 4
                mov     ax, 0Ch
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------
                align 2

loc_1CEF2:                              ; CODE XREF: ovl_2BRAIN:CE5E↑j
                                        ; ovl_2BRAIN:CE66↑j
                mov     word ptr [bp-2], 0
                cmp     word ptr [bp-0Ah], 47h ; 'G'
                jnz     short loc_1CF16
                push    word ptr [bp-6]
                call    thk_party_gather_gold
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    word ptr [bp-4]
                call    tavern_helper_c
                add     sp, 6
                jmp     short loc_1CF55
; ---------------------------------------------------------------------------
                align 2

loc_1CF16:                              ; CODE XREF: ovl_2BRAIN:CEFB↑j
                cmp     word ptr [bp-0Ah], 1Bh
                jz      short loc_1CF55
                mov     ax, [bp-0Ah]
                sub     ax, 31h ; '1'
                mov     [bp-8], ax
                or      ax, ax
                jl      short loc_1CF55
                mov     ax, g_party_size

loc_1CF2C:                              ; CODE XREF: seg002:050D↑J
                cmp     [bp-8], ax
                jge     short loc_1CF55
                mov     ax, [bp-6]
                cmp     [bp-8], ax
                jz      short loc_1CF55
                mov     bx, [bp-8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CF55
                mov     ax, [bp-8]
                mov     [bp-6], ax
                push    ax
                call    loc_1D0BA
                add     sp, 2
                mov     [bp-4], ax

loc_1CF55:                              ; CODE XREF: ovl_2BRAIN:CEAC↑j
                                        ; ovl_2BRAIN:CF13↑j ...
                cmp     word ptr [bp-0Ah], 1Bh
                jz      short loc_1CF5E
                jmp     loc_1CD95       ; CODE XREF: seg002:0AE9↑J
; ---------------------------------------------------------------------------

loc_1CF5E:                              ; CODE XREF: ovl_2BRAIN:CF59↑j
                mov     bx, [bp+6]
                mov     ax, [bp-6]
                mov     [bx], ax
                mov     bx, [bp+4]
                mov     ax, [bp-4]
                mov     [bx], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: tavern_menu+387↓p
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, [bp+4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CF92

loc_1CF84:                              ; CODE XREF: seg002:062D↑J
                mov     ax, 4

loc_1CF87:                              ; CODE XREF: ovl_2BRAIN:CFA8↓j
                                        ; ovl_2BRAIN:CFD0↓j
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1D032
; ---------------------------------------------------------------------------
                align 2

loc_1CF92:                              ; CODE XREF: ovl_2BRAIN:CF82↑j
                mov     ax, 1
                cwd
                push    dx
                push    ax
                push    word ptr [bp+4]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CFAA
                mov     ax, 2
                jmp     short loc_1CF87
; ---------------------------------------------------------------------------

loc_1CFAA:                              ; CODE XREF: ovl_2BRAIN:CFA3↑j
                mov     bx, [bp+4]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_lookup_bracket
                add     sp, 2
                sub     ah, ah
                add     ax, 5
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 1
                jz      short loc_1CFD2
                sub     ax, ax
                jmp     short loc_1CF87
; ---------------------------------------------------------------------------

loc_1CFD2:                              ; CODE XREF: ovl_2BRAIN:CFCC↑j
                call    sub_1CA46
                shl     ax, 1
                mov     [bp-2], ax
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp-2]
                shl     ax, 1
                mov     si, ax
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                push    word ptr [bx+si+5676h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                push    word ptr [bx+si+5678h]
                call    thk_text_puts
                add     sp, 2
                call    thk_monster_anim_step

loc_1D032:                              ; CODE XREF: ovl_2BRAIN:CF8E↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: tavern_menu+38F↓p
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, [bp+4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1D054
                mov     ax, 4
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     short loc_1D0B4
; ---------------------------------------------------------------------------

loc_1D054:                              ; CODE XREF: ovl_2BRAIN:D046↑j
                call    sub_1CA46
                shl     ax, 1
                mov     [bp-2], ax
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp-2]
                shl     ax, 1
                mov     si, ax
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                push    word ptr [bx+si+56D2h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                push    word ptr [bx+si+56D4h]
                call    thk_text_puts
                add     sp, 2
                call    thk_monster_anim_step

loc_1D0B4:                              ; CODE XREF: ovl_2BRAIN:D052↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1D0BA:                              ; CODE XREF: ovl_2BRAIN:CD37↑p
                                        ; ovl_2BRAIN:CF4C↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 2
                push    word ptr [bp+4] ; CODE XREF: seg002:04DD↑J
                call    thk_char_ptr
                add     sp, 2
                mov     [bp-2], ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+4]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word ptr [bp-2]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGold_2 ; "Gold=         "
                push    ax
                call    thk_text_puts
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
                mov     bx, [bp-2]
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                mov     ax, [bp-2]
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1D13C:                              ; CODE XREF: ovl_2BRAIN:CD31↑p
                                        ; tavern_menu+116↓p ...
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                mov     word ptr [bp-2], 6
                sub     ax, ax
                mov     cx, 6
                mov     di, 5772h
                push    ds
                pop     es
                repne stosw
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; "Tavern", G-Gather Gold, #-Other Char, Select (A-E)
; Attributes: bp-based frame

tavern_menu     proc near               ; CODE XREF: seg002:0825↑J

var_16          = word ptr -16h
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
                sub     sp, 16h
                push    di
                push    si
                mov     [bp+var_10], 1
                mov     [bp+var_2], 1
                mov     ax, 1
                push    ax
                call    thk_load_building_text
                add     sp, 2
                mov     [bp+var_C], 5
                mov     [bp+var_E], 4
                mov     [bp+var_12], 0

loc_1D185:                              ; CODE XREF: tavern_menu+48↓j
                mov     si, [bp+var_12]
                add     si, 573Eh
                mov     di, 4

loc_1D18F:                              ; CODE XREF: tavern_menu+3E↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D18F
                add     [bp+var_12], 8
                cmp     [bp+var_12], 28h ; '('
                jl      short loc_1D185
                mov     [bp+var_C], 6
                mov     si, 5766h
                mov     di, 6

loc_1D1AF:                              ; CODE XREF: tavern_menu+5E↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D1AF
                mov     [bp+var_C], 0Eh
                mov     si, 5722h
                mov     di, 0Eh

loc_1D1C5:                              ; CODE XREF: tavern_menu+74↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D1C5
                mov     [bp+var_C], 5
                mov     [bp+var_E], 8
                mov     [bp+var_16], 0

loc_1D1DF:                              ; CODE XREF: tavern_menu+A2↓j
                mov     si, [bp+var_16]
                add     si, 56D2h
                mov     di, 8

loc_1D1E9:                              ; CODE XREF: tavern_menu+98↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D1E9
                add     [bp+var_16], 10h
                cmp     [bp+var_16], 50h ; 'P'
                jl      short loc_1D1DF
                mov     [bp+var_C], 5
                mov     [bp+var_E], 8
                mov     [bp+var_16], 0

loc_1D20D:                              ; CODE XREF: tavern_menu+D0↓j
                mov     si, [bp+var_16]
                add     si, 5676h
                mov     di, 8

loc_1D217:                              ; CODE XREF: tavern_menu+C6↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D217
                add     [bp+var_16], 10h
                cmp     [bp+var_16], 50h ; 'P'
                jl      short loc_1D20D
                mov     [bp+var_C], 6
                mov     si, 56C6h
                mov     di, 6

loc_1D237:                              ; CODE XREF: tavern_menu+E6↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D237
                mov     [bp+var_C], 5
                mov     [bp+var_E], 6
                mov     [bp+var_16], 0

loc_1D251:                              ; CODE XREF: tavern_menu+114↓j
                mov     si, [bp+var_16]
                add     si, 577Eh
                mov     di, 6

loc_1D25B:                              ; CODE XREF: tavern_menu+10A↓j
                call    thk_str_next
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D25B
                add     [bp+var_16], 0Ch
                cmp     [bp+var_16], 3Ch ; '<'
                jl      short loc_1D251
                call    loc_1D13C
                mov     byte_2294F, 0FDh
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                mov     [bp+var_6], 0
                cmp     g_party_size, 0
                jle     short loc_1D2AB
                mov     si, 416h
                mov     dx, g_party_size
                mov     cx, [bp+var_6]

loc_1D2A0:                              ; CODE XREF: tavern_menu:loc_1D4AB↓j
                cmp     word ptr [si], 18h
                jl      short loc_1D2A8
                jmp     loc_1D4A0
; ---------------------------------------------------------------------------

loc_1D2A8:                              ; CODE XREF: tavern_menu+149↑j
                                        ; tavern_menu+34E↓j
                mov     [bp+var_6], cx

loc_1D2AB:                              ; CODE XREF: tavern_menu+13A↑j
                sub     si, si
                sub     di, di

loc_1D2AF:                              ; CODE XREF: tavern_menu+17E↓j
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
                push    word ptr [bx+di+573Eh]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 3
                jl      short loc_1D2AF
                mov     [bp+var_C], si
                cmp     g_map_id, 1
                jz      short loc_1D306
                mov     ax, 16h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bl, g_map_id
                sub     bh, bh
                mov     cl, 3
                shl     bx, cl
                push    word ptr [bx+5744h]
                call    thk_text_puts
                add     sp, 2

loc_1D306:                              ; CODE XREF: tavern_menu+188↑j
                call    thk_2PLAY_946E
                or      byte_1DC80, 6
                cmp     byte_1DC7F, 0
                jnz     short loc_1D318
                jmp     loc_1D50F
; ---------------------------------------------------------------------------

loc_1D318:                              ; CODE XREF: tavern_menu+1B9↑j
                or      byte_1DC80, 1
                call    thk_draw_screen_rows
                call    thk_print_gold_label
                mov     ax, 2
                push    ax
                call    thk_clear_text_preset
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
                mov     ax, offset aTavern ; "   Tavern   "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGGatherGold_0 ; "G-Gather Gold"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOtherChar_0 ; "#-Other Char"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1D388:                              ; CODE XREF: tavern_menu+363↓j
                cmp     [bp+var_10], 0
                jz      short loc_1D39F
                push    [bp+var_6]
                call    loc_1D0BA
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_10], 0

loc_1D39F:                              ; CODE XREF: tavern_menu+232↑j
                cmp     [bp+var_2], 0
                jz      short loc_1D420
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSelectAE ; "Select (A-E)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 7
                push    ax
                call    thk_clear_text_preset
                add     sp, 2
                sub     si, si
                mov     di, 5766h

loc_1D3CC:                              ; CODE XREF: tavern_menu+28F↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 10h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 6
                jl      short loc_1D3CC
                mov     [bp+var_C], si
                mov     ax, 12h
                push    ax
                mov     ax, 21h ; '!'
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                push    word ptr [bx+4226h]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_1D420:                              ; CODE XREF: tavern_menu+249↑j
                mov     [bp+var_2], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_toupper
                add     sp, 2
                mov     [bp+var_A], ax
                cmp     ax, 41h ; 'A'
                jz      short loc_1D4AE
                cmp     ax, 42h ; 'B'
                jnz     short loc_1D43F
                jmp     loc_1D4C0
; ---------------------------------------------------------------------------

loc_1D43F:                              ; CODE XREF: tavern_menu+2E0↑j
                cmp     ax, 43h ; 'C'
                jnz     short loc_1D447
                jmp     loc_1D4D0
; ---------------------------------------------------------------------------

loc_1D447:                              ; CODE XREF: tavern_menu+2E8↑j
                cmp     ax, 44h ; 'D'
                jnz     short loc_1D44F
                jmp     loc_1D4DE
; ---------------------------------------------------------------------------

loc_1D44F:                              ; CODE XREF: tavern_menu+2F0↑j
                cmp     ax, 45h ; 'E'
                jnz     short loc_1D457
                jmp     loc_1D4E6
; ---------------------------------------------------------------------------

loc_1D457:                              ; CODE XREF: tavern_menu+2F8↑j
                cmp     ax, 47h ; 'G'
                jnz     short loc_1D45F
                jmp     loc_1D4EE
; ---------------------------------------------------------------------------

loc_1D45F:                              ; CODE XREF: tavern_menu+300↑j
                mov     [bp+var_2], 0
                cmp     ax, 1Bh
                jz      short loc_1D4B7
                sub     ax, 31h ; '1'
                mov     [bp+var_8], ax
                or      ax, ax
                jl      short loc_1D4B7
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1D4B7
                mov     ax, [bp+var_6]
                cmp     [bp+var_8], ax
                jz      short loc_1D4B7
                mov     bx, [bp+var_8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1D4B7
                call    loc_1D13C
                mov     ax, [bp+var_8]
                mov     [bp+var_6], ax
                mov     [bp+var_10], 1
                jmp     short loc_1D4B7
; ---------------------------------------------------------------------------
                align 2

loc_1D4A0:                              ; CODE XREF: tavern_menu+14B↑j
                add     si, 2
                inc     cx
                cmp     cx, dx
                jl      short loc_1D4AB
                jmp     loc_1D2A8
; ---------------------------------------------------------------------------

loc_1D4AB:                              ; CODE XREF: tavern_menu+34C↑j
                jmp     loc_1D2A0
; ---------------------------------------------------------------------------

loc_1D4AE:                              ; CODE XREF: tavern_menu+2DB↑j
                push    [bp+var_4]
                call    loc_1CB08

loc_1D4B4:                              ; CODE XREF: tavern_menu+38A↓j
                                        ; tavern_menu+392↓j
                add     sp, 2

loc_1D4B7:                              ; CODE XREF: tavern_menu+30D↑j
                                        ; tavern_menu+317↑j ...
                cmp     [bp+var_A], 1Bh
                jz      short loc_1D50C
                jmp     loc_1D388
; ---------------------------------------------------------------------------

loc_1D4C0:                              ; CODE XREF: tavern_menu+2E2↑j
                lea     ax, [bp+var_6]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                call    loc_1CB7C

loc_1D4CB:                              ; CODE XREF: tavern_menu+381↓j
                add     sp, 4
                jmp     short loc_1D4B7
; ---------------------------------------------------------------------------

loc_1D4D0:                              ; CODE XREF: tavern_menu+2EA↑j
                lea     ax, [bp+var_6]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                call    loc_1CD60
                jmp     short loc_1D4CB
; ---------------------------------------------------------------------------
                align 2

loc_1D4DE:                              ; CODE XREF: tavern_menu+2F2↑j
                push    [bp+var_4]
                call    loc_1CF74
                jmp     short loc_1D4B4
; ---------------------------------------------------------------------------

loc_1D4E6:                              ; CODE XREF: tavern_menu+2FA↑j
                push    [bp+var_4]
                call    loc_1D038
                jmp     short loc_1D4B4
; ---------------------------------------------------------------------------

loc_1D4EE:                              ; CODE XREF: tavern_menu+302↑j
                push    [bp+var_6]
                call    thk_party_gather_gold
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_4]
                call    tavern_helper_c
                add     sp, 6
                mov     [bp+var_2], 0
                jmp     short loc_1D4B7
; ---------------------------------------------------------------------------
                align 2

loc_1D50C:                              ; CODE XREF: tavern_menu+361↑j
                call    thk_text_clear_prompt_line

loc_1D50F:                              ; CODE XREF: tavern_menu+1BB↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
tavern_menu     endp

; ---------------------------------------------------------------------------
                align 10h
ovl_2BRAIN      ends

