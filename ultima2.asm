; ---------------------------------------------------------------------------

FCB             struc ; (sizeof=0x25, mappedto_1)
                                        ; XREF: sg08e3:_picData/r
drive           db ?                    ; XREF: access_file:loc_152E7/w
filename        db 8 dup(?)             ; XREF: access_file+30/w
                                        ; access_file+7F/r
extension       db 3 dup(?)             ; XREF: access_file+36/w
                                        ; access_file+3B/w ...
current_block   dw ?
record_size     dw ?                    ; XREF: access_file+9A/w
file_size       dd ?
date            dw ?
time            dw ?
reserved        db 8 dup(?)
current_record  db ?
random_record   dd ?
FCB             ends

; ---------------------------------------------------------------------------

Savegame        struc ; (sizeof=0x100, mappedto_2)
                                        ; XREF: canMoveToTile-18B0/r
                                        ; canMoveToTile-1729/w ...
_name           db 13 dup(?)            ; XREF: sg01a2:07AA/r
                                        ; write_player_name:loc_1507D/r ...
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
_sex            db ?                    ; XREF: sg01a2:5751/w
_class          db ?                    ; XREF: sg01a2:080D/r
                                        ; sg01a2:5894/w ...
_race           db ?                    ; XREF: sg01a2:57BF/w
                                        ; sg01a2:587A/r ...
_mapNum1        db ?                    ; XREF: load_map+1/r
                                        ; sg01a2:2306/r ...
_mapNum2        db ?                    ; XREF: canMoveToTile-1BB0/r
                                        ; canMoveToTile-19E0/r ...
_strength       db ?                    ; XREF: sg01a2:56C1/w
                                        ; sg01a2:5760/r ...
_agility        db ?                    ; XREF: sg01a2:56D9/w
                                        ; sg01a2:57FB/r ...
_stamina        db ?                    ; XREF: sg01a2:56F1/w
_charisma       db ?                    ; XREF: sg01a2:5709/w
                                        ; sg01a2:578F/r ...
_wisdom         db ?                    ; XREF: sg01a2:5721/w
                                        ; sg01a2:585B/r ...
_intelligence   db ?                    ; XREF: sg01a2:5739/w
                                        ; sg01a2:57CF/r ...
_hp             db 2 dup(?)             ; XREF: canMoveToTile-183B/r
                                        ; canMoveToTile-1831/w ...
_food           db 2 dup(?)             ; XREF: canMoveToTile-1B0B/r
                                        ; canMoveToTile-1B03/w ...
_foodTurnCtr    db ?                    ; XREF: canMoveToTile-1B16/r
                                        ; canMoveToTile-1B0E/w ...
_experience     db 2 dup(?)             ; XREF: sub_10F8E-31B/w
                                        ; sub_10F8E-318/w ...
_gold           db 2 dup(?)             ; XREF: sub_10F8E-315/w
                                        ; sub_10F8E-312/w ...
_mapX           db ?                    ; XREF: sg01a2:0826/r
                                        ; sg01a2:2323/w ...
_mapY           db ?                    ; XREF: sg01a2:082C/r
                                        ; sg01a2:2329/w ...
                db ? ; undefined
_lastKeypress   db ?                    ; XREF: canMoveToTile:overworld_keypress/w
                                        ; canMoveToTile-1AEA/r
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_33        db ?                    ; XREF: sg01a2:0832/r
field_34        db ?                    ; XREF: sg01a2:0839/r
                                        ; canMoveToTile+37/r
field_35        db ?                    ; XREF: sg01a2:083F/r
                                        ; canMoveToTile+3D/r
                db ? ; undefined
_disableSave    db ?                    ; XREF: sg01a2:save_game/r
                                        ; canMoveToTile+30/r ...
field_38        db ?                    ; XREF: normal_movement+142/r
                                        ; normal_movement+161/r ...
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
_gameSpeed?     db ?                    ; XREF: set_player_game_speed/w
                                        ; setup_speed_array:loc_106F0/r ...
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
_hasRing        db ?                    ; XREF: canMoveToTile+92/r
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_FF        db ?
Savegame        ends


;
; +-------------------------------------------------------------------------+
; |      This file was generated by The Interactive Disassembler (IDA)      |
; |           Copyright (c) 2023 Hex-Rays, <support@hex-rays.com>           |
; |                      License info: 48-337F-7354-97                      |
; |                       Paul Gilbert, ScummVM Team                        |
; +-------------------------------------------------------------------------+
;
; Input SHA256 : 781D0A8F48FD7F6F0DE9EFE7DAC0FAD1F5D8C8CE7A35D1C5CCDD5478C2781AEF
; Input MD5    : D6727DACBE7EF7DE20498A32B8F171EA
; Input CRC32  : E1AA48FB

; File Name   : C:\GOG Games\Ultima 2\ULTIMAII.EXE
; Format      : MS-DOS executable (EXE)
; Base Address: 1000h Range: 10000h-1A044h Loaded length: 8FE0h
; Entry Point : 18AC:0

                .686p
                .mmx
                .model large

; ===========================================================================

; Segment type: Pure code
sg01a2          segment byte public 'CODE' use16
                assume cs:sg01a2
                assume es:nothing, ss:nothing, ds:sg08e3, fs:nothing, gs:nothing
byte_10000      db 0FFh dup(     0)
                db    0

; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

start_          proc near               ; CODE XREF: sg01a2:loc_15A61↓p
                                        ; DATA XREF: start+D↓o
                mov     ax, seg sg08e3
                mov     ds, ax
                add     ax, 310h
                mov     ss, ax
                assume ss:nothing
                mov     sp, 100h
                mov     ax, 40h ; '@'
                mov     es, ax
                assume es:nothing
                mov     al, es:6Ch      ; Tick counter low byte
                mov     _timer1, al
                mov     _timer2, al
                mov     _timer3, al
                mov     _timer4, al
                mov     _timer5, al
                mov     _timer6, al
                inc     _timer1
                inc     _timer2
                call    set_cga_mode
                mov     al, 0
                mov     _demoMode, al
                call    set_player_game_speed
                call    setup_speed_array
                nop
                call    set_cga_mode
                mov     di, 16          ; x
                mov     si, 10          ; y
                call    set_text_pos
                call    write_string    ; ORIGIN
; ---------------------------------------------------------------------------
aOrigin         db 'ORIGIN',0
; ---------------------------------------------------------------------------
                mov     di, 0Bh         ; x
                mov     si, 0Ch         ; y
                call    set_text_pos
                call    write_string    ; PROUDLY PRESENTS
; ---------------------------------------------------------------------------
aProudlyPresent db 'PROUDLY PRESENTS',0
; ---------------------------------------------------------------------------
                call    pauseScreen
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_10180
                jmp     loc_10242
; ---------------------------------------------------------------------------

loc_10180:                              ; CODE XREF: start_+7B↑j
                nop
                call    set_cga_mode
                mov     di, 10h         ; x
                mov     si, 8           ; y
                call    set_text_pos
                call    write_string    ; PART ][
; ---------------------------------------------------------------------------
aPart           db 'PART ][',0
; ---------------------------------------------------------------------------
                mov     di, 9           ; x
                mov     si, 0Ah         ; y
                call    set_text_pos
                call    write_string    ; OF THE #1 BEST SELLING
; ---------------------------------------------------------------------------
aOfThe1BestSell db 'OF THE #1 BEST SELLING',0
; ---------------------------------------------------------------------------
                mov     di, 7           ; x
                mov     si, 0Ch         ; y
                call    set_text_pos
                call    write_string    ; FANTASY ROLE-PLAYING GAME
; ---------------------------------------------------------------------------
aFantasyRolePla db 'FANTASY ROLE-PLAYING GAME',0
; ---------------------------------------------------------------------------
                mov     di, 0Ch         ; x
                mov     si, 0Eh         ; y
                call    set_text_pos
                call    write_string    ; BY LORD BRITISH
; ---------------------------------------------------------------------------
aByLordBritish  db 'BY LORD BRITISH',0
; ---------------------------------------------------------------------------
                call    pauseScreen
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_1020B
                jmp     short loc_10242
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1020B:                              ; CODE XREF: start_+106↑j
                nop
                call    setPalette
                mov     ah, 27h ; '''
                mov     cx, 4000h
                mov     dx, 0
                call    access_file
                push    ax
                dec     cx
                inc     bx
                inc     sp
                push    dx
                inc     cx
                and     [bx+si], ah
                call    pauseScreen
                mov     si, 5

loc_10228:                              ; CODE XREF: start_+135↓j
                mov     di, 2C00h

loc_1022B:                              ; CODE XREF: start_+12C↓j
                dec     di
                jnz     short loc_1022B
                mov     bx, 8
                call    delayFrames
                dec     si
                jnz     short loc_10228
                nop
                cmp     _demoMode, 0
                jz      short loc_10242
                jmp     demo
; ---------------------------------------------------------------------------

loc_10242:                              ; CODE XREF: start_+7D↑j
                                        ; start_+108↑j ...
                nop
                call    set_cga_mode
                mov     text_y, 1
                mov     al, 0Fh
                mov     text_x, al
                call    write_string    ; ULTIMA ][
; ---------------------------------------------------------------------------
aUltima         db 'ULTIMA ][',0
; ---------------------------------------------------------------------------
                mov     text_y, 3
                mov     al, 10h
                mov     text_x, al
                call    write_string    ; REVENGE
; ---------------------------------------------------------------------------
aRevenge        db 'REVENGE',0
; ---------------------------------------------------------------------------
                mov     text_y, 5
                mov     al, 10h
                mov     text_x, al
                call    write_string    ; OF  THE
; ---------------------------------------------------------------------------
aOfThe          db 'OF  THE',0
; ---------------------------------------------------------------------------
                mov     text_y, 7
                mov     al, 0Eh
                mov     text_x, al
                call    write_string    ; ENCHANTRESS
; ---------------------------------------------------------------------------
aEnchantress    db 'ENCHANTRESS',0
; ---------------------------------------------------------------------------
                mov     al, 16h
                mov     text_y, al
                mov     al, 6
                mov     text_x, al
                call    write_string
; ---------------------------------------------------------------------------
                db  28h ; (
                db  43h ; C
                db  29h ; )
                db  2Dh ; -
                db  31h ; 1
                db  39h ; 9
                db  38h ; 8
                db  33h ; 3
                db  2Ch ; ,
                db  31h ; 1
                db  39h ; 9
                db  38h ; 8
                db  39h ; 9
                db  20h
                db  42h ; B
                db  59h ; Y
                db  20h
                db  4Ch ; L
                db  4Fh ; O
                db  52h ; R
                db  44h ; D
                db  20h
                db  42h ; B
                db  52h ; R
                db  49h ; I
                db  54h ; T
                db  49h ; I
                db  53h ; S
                db  48h ; H
                db    0
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  17h
                db 0B0h
                db  0Fh
                db 0A2h
                db  2Eh ; .
                db    0
                db 0E8h                 ; AND ORIGIN
                db    7
                db  4Dh ; M
aAndOrigin      db 'AND ORIGIN',0
; ---------------------------------------------------------------------------
                mov     al, 0Bh
                mov     text_y, al
                mov     al, 5
                mov     text_x, al
                call    write_string
; ---------------------------------------------------------------------------
                db  54h ; T
                db  59h ; Y
                db  50h ; P
                db  45h ; E
                db  20h
                db  2Dh ; -
                db    0
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  0Dh
                db 0B0h
                db    6
                db 0A2h
                db  2Eh ; .
                db    0
                db 0E8h                 ; 'D' - FOR A DEMONSTRATION
                db 0DBh
                db  4Ch ; L
aDForADemonstra db 27h,'D',27h,' - FOR A DEMONSTRATION',0
; ---------------------------------------------------------------------------
                mov     text_y, 0Fh
                mov     al, 6
                mov     text_x, al
                call    write_string    ; 'P' - PLAY A GAME OF ULTIMA ][
; ---------------------------------------------------------------------------
aPPlayAGameOfUl db 27h,'P',27h,' - PLAY A GAME OF ULTIMA ][',0
; ---------------------------------------------------------------------------
                mov     text_y, 11h
                mov     al, 6
                mov     text_x, al
                call    write_string    ; 'C' - CREATE A NEW CHARACTER
; ---------------------------------------------------------------------------
aCCreateANewCha db 27h,'C',27h,' - CREATE A NEW CHARACTER',0
; ---------------------------------------------------------------------------
                mov     text_y, 13h
                mov     al, 5
                mov     text_x, al
                call    write_string    ; CHOICE:
; ---------------------------------------------------------------------------
aChoice         db 'CHOICE:',0
; ---------------------------------------------------------------------------
                mov     di, 30h ; '0'

loc_1038C:                              ; CODE XREF: start_+292↓j
                call    keypress_check
                cmp     ah, 0FFh
                jnz     short loc_1038C
                call    write_character
                cmp     al, 'D'
                jnz     short loc_1039E
                jmp     short demo
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1039E:                              ; CODE XREF: start_+299↑j
                cmp     al, 'P'
                jz      short loc_103AC
                cmp     al, 'C'
                jz      short loc_103A9
                jmp     loc_10242
; ---------------------------------------------------------------------------

loc_103A9:                              ; CODE XREF: start_+2A4↑j
                call    create_character

loc_103AC:                              ; CODE XREF: start_+2A0↑j
                call    play_game

demo:                                   ; CODE XREF: start_+13F↑j
                                        ; start_+29B↑j
                nop
                call    setPalette
                mov     al, 1
                mov     _demoMode, al
                mov     ah, 27h ; '''
                mov     cx, 4000h
                mov     dx, 0
                call    access_file
                push    ax
                dec     cx
                inc     bx
                dec     di
                push    bp
                push    sp
                and     [bx+si], ah
                mov     text_y, 17h
                mov     text_x, 0
                call    write_string    ;         BATTLE STRANGE CREATURES
; ---------------------------------------------------------------------------
aBattleStrangeC db '        BATTLE STRANGE CREATURES',0
; ---------------------------------------------------------------------------
                mov     text_x, 0
                mov     text_y, 18h
                call    write_string    ;       ACROSS THE FACE OF THE EARTH
start_          endp ; sp-analysis failed

; ---------------------------------------------------------------------------
aAcrossTheFaceO db '      ACROSS THE FACE OF THE EARTH',0
; ---------------------------------------------------------------------------
                call    sub_1068F
                call    sub_1538F
                call    sub_14B4E
                mov     ah, 27h ; '''
                mov     cx, 4000h
                mov     dx, 0
                call    access_file
                push    ax
                dec     cx
                inc     bx
                push    sp
                push    di
                dec     si
                and     [bx+si], ah
                mov     text_x, 0
                mov     text_y, 17h
                call    write_string    ;    SEARCH FOR CLUES IN CARELESS WORDS
; ---------------------------------------------------------------------------
aSearchForClues db '   SEARCH FOR CLUES IN CARELESS WORDS',0
; ---------------------------------------------------------------------------
                mov     text_x, 2
                mov     text_y, 18h
                call    write_string    ;       SPOKEN AT THE LOCAL PUB
; ---------------------------------------------------------------------------
aSpokenAtTheLoc db '      SPOKEN AT THE LOCAL PUB',0
; ---------------------------------------------------------------------------
                call    sub_1068F
                jmp     short loc_10519
; ---------------------------------------------------------------------------
                db  90h
                db 0E8h
                db 0E3h
                db  4Eh ; N
                db 0E8h
                db  9Fh
                db  46h ; F
                db 0B4h
                db  27h ; '
                db 0B9h
                db    0
                db  40h ; @
                db 0BAh
                db    0
                db    0
                db 0E8h
                db  10h
                db  4Eh ; N
                db  50h ; P
                db  49h ; I
                db  43h ; C
                db  43h ; C
                db  41h ; A
                db  53h ; S
                db  20h
                db  20h
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    0
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  17h
                db 0E8h
                db  10h
                db  4Bh ; K
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  20h
                db  57h ; W
                db  49h ; I
                db  54h ; T
                db  48h ; H
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  44h ; D
                db  49h ; I
                db  45h ; E
                db  56h ; V
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  4Bh ; K
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  53h ; S
                db    0
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    4
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  18h
                db 0E8h
                db 0E1h
                db  4Ah ; J
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  49h ; I
                db  53h ; S
                db  54h ; T
                db  41h ; A
                db  4Eh ; N
                db  43h ; C
                db  45h ; E
                db    0
                db 0E8h
                db  76h ; v
                db    1
; ---------------------------------------------------------------------------

loc_10519:                              ; CODE XREF: sg01a2:04A6↑j
                call    sub_1538F
                call    sub_14B4E
                mov     ah, 27h ; '''
                mov     cx, 4000h
                mov     dx, 0
                call    access_file
                push    ax
                dec     cx
                inc     bx
                inc     sp
                dec     si
                inc     di
                and     [bx+si], ah
                mov     text_x, 0
                mov     text_y, 17h
                call    write_string
; ---------------------------------------------------------------------------
                db  20h
                db  20h
                db  20h
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db  20h
                db  44h ; D
                db  45h ; E
                db  45h ; E
                db  50h ; P
                db  20h
                db  44h ; D
                db  41h ; A
                db  52h ; R
                db  4Bh ; K
                db  20h
                db  44h ; D
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db  44h ; D
                db  55h ; U
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db    0
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    1
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  18h
                db 0E8h
                db  6Dh ; m
                db  4Ah ; J
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  54h ; T
                db  41h ; A
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  52h ; R
                db  49h ; I
                db  46h ; F
                db  59h ; Y
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db  53h ; S
                db    0
                db 0E8h
                db 0F9h
                db    0
                db 0E8h
                db 0F6h
                db  4Dh ; M
                db 0E8h
                db 0B2h
                db  45h ; E
                db 0B4h
                db  27h ; '
                db 0B9h
                db    0
                db  40h ; @
                db 0BAh
                db    0
                db    0
                db 0E8h
                db  23h ; #
                db  4Dh ; M
                db  50h ; P
                db  49h ; I
                db  43h ; C
                db  53h ; S
                db  50h ; P
                db  41h ; A
                db  20h
                db  20h
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    0
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  17h
                db 0E8h
                db  23h ; #
                db  4Ah ; J
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  48h ; H
                db  52h ; R
                db  4Fh ; O
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  4Fh ; O
                db  55h ; U
                db  54h ; T
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  47h ; G
                db  41h ; A
                db  4Ch ; L
                db  41h ; A
                db  58h ; X
                db  59h ; Y
                db    0
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    1
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  18h
                db 0E8h
                db 0F3h
                db  49h ; I
                db  20h
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  54h ; T
                db  53h ; S
                db  20h
                db  4Fh ; O
                db  46h ; F
                db  20h
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  53h ; S
                db  4Fh ; O
                db  4Ch ; L
                db  41h ; A
                db  52h ; R
                db  20h
                db  53h ; S
                db  59h ; Y
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  4Dh ; M
                db    0
                db 0E8h
                db  7Bh ; {
                db    0
                db 0E8h
                db  78h ; x
                db  4Dh ; M
                db 0E8h
                db  34h ; 4
                db  45h ; E
                db 0B4h
                db  27h ; '
                db 0B9h
                db    0
                db  40h ; @
                db 0BAh
                db    0
                db    0
                db 0E8h
                db 0A5h
                db  4Ch ; L
                db  50h ; P
                db  49h ; I
                db  43h ; C
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  20h
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    0
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  17h
                db 0E8h
                db 0A5h
                db  49h ; I
                db  20h
                db  20h
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Eh ; N
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  52h ; R
                db  20h
                db  54h ; T
                db  49h ; I
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  49h ; I
                db  54h ; T
                db  53h ; S
                db  45h ; E
                db  4Ch ; L
                db  46h ; F
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  42h ; B
                db  41h ; A
                db  54h ; T
                db  54h ; T
                db  4Ch ; L
                db  45h ; E
                db    0
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    8
                db 0C6h
                db    6
                db  2Fh ; /
                db    0
                db  18h
                db 0E8h
                db  73h ; s
                db  49h ; I
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  41h ; A
                db  58h ; X
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  45h ; E
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  54h ; T
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  53h ; S
                db    0
                db 0E8h
                db  0Ah
                db    0
                db 0E9h
                db 0B6h
                db 0FAh

; =============== S U B R O U T I N E =======================================


pauseScreen     proc near               ; CODE XREF: start_+72↑p
                                        ; start_+FD↑p ...
                mov     dx, 6000h
                call    delayMilli?
                retn
pauseScreen     endp


; =============== S U B R O U T I N E =======================================


sub_1068F       proc near               ; CODE XREF: sg01a2:0429↑p
                                        ; sg01a2:04A3↑p
                mov     dx, 0A000h
                call    delayMilli?
                retn
sub_1068F       endp


; =============== S U B R O U T I N E =======================================


delayMilli?     proc near               ; CODE XREF: pauseScreen+3↑p
                                        ; sub_1068F+3↑p ...
                mov     bx, 10
                call    delayFrames
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_106A8
                pop     ax
                jmp     loc_10242
; ---------------------------------------------------------------------------

loc_106A8:                              ; CODE XREF: delayMilli?+C↑j
                dec     dx
                jnz     short delayMilli?
                retn
delayMilli?     endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================


set_player_game_speed proc near         ; CODE XREF: start_+38↑p
                                        ; sg01a2:07ED↓p
                mov     word ptr player._gameSpeed?, 0FFh
                retn
set_player_game_speed endp

; ---------------------------------------------------------------------------
                db    5
                db  89h
                db  16h
                db  50h ; P
                db    5
                db 0C7h
                db    6
                db  74h ; t
                db    0
                db    0
                db    0
                db 0B4h
                db    0
                db 0CDh
                db  1Ah
                db  3Ch ; <
                db    0
                db  75h ; u
                db 0E6h
                db  3Bh ; ;
                db  16h
                db  50h ; P
                db    5
                db  74h ; t
                db 0F2h
                db 0B4h
                db    0
                db 0CDh
                db  1Ah
                db  3Ch ; <
                db    0
                db  75h ; u
                db 0D8h
                db 0B8h
                db  64h ; d
                db    0
                db  48h ; H
                db  75h ; u
                db 0FDh
                db  83h
                db    6
                db  74h ; t
                db    0
                db    1
                db  2Bh ; +
                db  16h
                db  50h ; P
                db    5
                db  2Bh ; +
                db  0Eh
                db  52h ; R
                db    5
                db  83h
                db 0FAh
                db  14h
                db  72h ; r
                db 0E0h
                db 0C3h

; =============== S U B R O U T I N E =======================================


setup_speed_array proc near             ; CODE XREF: start_+3B↑p
                                        ; sg01a2:loc_107F0↓p ...
                mov     bx, 38

loc_106F0:                              ; CODE XREF: setup_speed_array+21↓j
                mov     ax, word ptr player._gameSpeed?
                sub     ax, speed_divisor
                jnb     short loc_106FF
                mov     ax, 0
                jmp     short loc_10707
; ---------------------------------------------------------------------------
                nop

loc_106FF:                              ; CODE XREF: setup_speed_array+A↑j
                mul     array1[bx]
                div     speed_divisor

loc_10707:                              ; CODE XREF: setup_speed_array+F↑j
                mov     array2[bx], ax
                sub     bx, 2
                jnb     short loc_106F0
                retn
setup_speed_array endp


; =============== S U B R O U T I N E =======================================


delayFrames     proc near               ; CODE XREF: start_+131↑p
                                        ; delayMilli?+3↑p ...
                mov     bx, [bx+580h]

loc_10715:                              ; CODE XREF: delayFrames+D↓j
                dec     bx
                js      short locret_10720
                mov     ax, 0Ch

loc_1071B:                              ; CODE XREF: delayFrames+B↓j
                dec     ax
                jnz     short loc_1071B
                jmp     short loc_10715
; ---------------------------------------------------------------------------

locret_10720:                           ; CODE XREF: delayFrames+5↑j
                retn
delayFrames     endp

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
                db    0
                db    0
                db  8Fh
                db  2Bh ; +
                db  6Fh ; o
                db  2Fh ; /
                db  0Bh
                db  31h ; 1
                db  4Dh ; M
                db  33h ; 3
                db  89h
                db  33h ; 3
                db 0C1h
                db  35h ; 5
                db  3Bh ; ;
                db  36h ; 6
                db  82h
                db  37h ; 7
                db  94h
                db  37h ; 7
                db 0DCh
                db  37h ; 7
                db 0F1h
                db  37h ; 7
                db 0B0h
                db  38h ; 8
                db  92h
                db  3Bh ; ;
                db 0CEh
                db  3Bh ; ;
                db  2Eh ; .
                db  3Ch ; <
                db 0BCh
                db  3Dh ; =
                db 0C7h
                db  3Dh ; =
                db  50h ; P
                db  3Eh ; >
                db  25h ; %
                db  3Fh ; ?
                db  50h ; P
                db  40h ; @
                db 0F1h
                db  42h ; B
                db  7Eh ; ~
                db  43h ; C
                db 0F3h
                db  43h ; C
                db 0D4h
                db  44h ; D
                db  42h ; B
                db  45h ; E
                db  68h ; h
                db  45h ; E
; ---------------------------------------------------------------------------

play_game:                              ; CODE XREF: start_:loc_103AC↑p
                                        ; sg01a2:07E3↓j
                call    set_cga_mode
                mov     bh, 0
                mov     bl, 0Ch
                mov     si, bx
                mov     bh, 0
                mov     bl, 0Ah
                mov     di, bx
                call    set_text_pos
                jmp     short loc_10796
; ---------------------------------------------------------------------------
                db  48h ; H
aInsertPlayerDi db '(INSERT PLAYER DISK)',0
; ---------------------------------------------------------------------------

loc_1078E:                              ; CODE XREF: sg01a2:0794↓j
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_1078E

loc_10796:                              ; CODE XREF: sg01a2:0776↑j
                mov     cx, 100h
                lea     dx, player
                mov     ah, 27h
                call    access_file
; ---------------------------------------------------------------------------
aPlayer_0       db 'PLAYER  '
; ---------------------------------------------------------------------------
                mov     al, player._name
                or      al, al
                jnz     short loc_107E6
                call    set_cga_mode
                mov     bh, 0
                mov     bl, 0Ch
                mov     si, bx
                mov     bh, 0
                mov     bl, 0Ah
                mov     di, bx
                call    set_text_pos
                call    write_string
; ---------------------------------------------------------------------------
aNoCharacterOnD db 'NO CHARACTER ON DISK',0
; ---------------------------------------------------------------------------

loc_107DB:                              ; CODE XREF: sg01a2:07E1↓j
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_107DB
                jmp     play_game
; ---------------------------------------------------------------------------

loc_107E6:                              ; CODE XREF: sg01a2:07AF↑j
                cmp     word ptr player._gameSpeed?, 0
                jnz     short loc_107F0
                call    set_player_game_speed

loc_107F0:                              ; CODE XREF: sg01a2:07EB↑j
                call    setup_speed_array
                mov     cx, 800h
                mov     dx, monsters_ptr
                mov     ah, 27h
                call    access_file
; ---------------------------------------------------------------------------
aMonsters       db 'MONSTERS'
; ---------------------------------------------------------------------------
                call    load_map
                call    setPalette
                mov     al, player._class
                add     al, al
                clc
                adc     al, 120
                mov     _playerTileId, al
                mov     al, 0
                mov     _flag1, al
                mov     player_paralyzedFlag, al
                mov     _flag3, al
                mov     _sleepFlag, al
                mov     al, player._mapX
                mov     _mapX, al
                mov     al, player._mapY
                mov     _mapY, al
;
                mov     al, player.field_33
                or      al, al
                jz      short loc_10850
                mov     al, player.field_34
                mov     _playerX, al
                mov     al, player.field_35
                mov     _playerY, al
                call    get_player_tile
                mov     al, 80
                mov     bx, current_tile_ptr
                mov     [bx+si], al

loc_10850:                              ; CODE XREF: sg01a2:0837↑j
                nop
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx
                mov     al, 0FFh

loc_10859:                              ; CODE XREF: sg01a2:0862↓j
                dec     di
                mov     _mapTileIds[di], al
                mov     _priorMapTileIds[di], al
                jnz     short loc_10859
                call    draw_map
                mov     al, 0FFh
                mov     _outsideMapTile, al
                call    write_stats
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_1086F:                              ; CODE XREF: canMoveToTile:loc_10C5F↓j
                                        ; canMoveToTile-114B↓j
                nop
                mov     sp, 100h
                mov     al, player._mapNum2
                cmp     al, 4
                jb      short overworld_render
                jmp     dungeon_render
; ---------------------------------------------------------------------------

overworld_render:                       ; CODE XREF: canMoveToTile-1BAB↑j
                nop
                call    write_string
; ---------------------------------------------------------------------------
aCmd            db 'CMD: ',0
; ---------------------------------------------------------------------------
                call    animate_water
                call    animate_water
                call    animate_water
                call    animate_water
                call    animate_forcefield
                mov     al, 0
                mov     _commandWaitCtr, al
                mov     al, 0C0h
                mov     byte_17432, al
                mov     al, _sleepFlag
                or      al, al
                jz      short loc_108C8
                mov     al, 20
                mov     _sleepFlag2?, al

loc_108AC:                              ; CODE XREF: canMoveToTile-1B60↓j
                call    write_string
; ---------------------------------------------------------------------------
aZ              db 'Z',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx

loc_108B7:                              ; CODE XREF: canMoveToTile-1B66↓j
                dec     di
                nop
                nop
                nop
                nop
                nop
                jnz     short loc_108B7
                dec     _sleepFlag2?
                jnz     short loc_108AC
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_108C8:                              ; CODE XREF: canMoveToTile-1B7E↑j
                                        ; canMoveToTile-1B49↓j ...
                mov     bx, 0
                call    delayFrames
                call    keypress_check
                cmp     ah, 0FFh
                jz      short overworld_keypress
                inc     _commandWaitCtr
                jnz     short loc_108C8
                mov     al, _playerTileId
                cmp     al, 38
                jnz     short loc_108E6
                call    sub_15EC7

loc_108E6:                              ; CODE XREF: canMoveToTile-1B42↑j
                nop
                call    draw_map
                call    animate_water
                call    animate_water
                call    animate_water
                call    animate_water
                call    animate_forcefield
                inc     byte_17432
                jnz     short loc_108C8
                mov     al, 0
                mov     byte_17436, al
                call    write_string
; ---------------------------------------------------------------------------
aPass_0         db 'PASS',0
; ---------------------------------------------------------------------------

end_of_turn:                            ; CODE XREF: canMoveToTile-1AD6↓j
                stc
                mov     al, player._foodTurnCtr
                cmc
                sbb     al, 10
                das
                cmc
                mov     player._foodTurnCtr, al
                mov     al, player._food+1
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food+1, al
                mov     al, player._food
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food, al
                jb      short not_dead
                jmp     dead2
; ---------------------------------------------------------------------------

not_dead:                               ; CODE XREF: canMoveToTile-1AF5↑j
                jmp     loc_10A33
; ---------------------------------------------------------------------------

overworld_keypress:                     ; CODE XREF: canMoveToTile-1B4F↑j
                mov     player._lastKeypress, al
                mov     al, player._lastKeypress
                cmp     al, ' '
                jnz     short not_pass
                mov     al, 0
                mov     byte_17436, al
                call    write_string
; ---------------------------------------------------------------------------
aPass           db 'PASS',0
; ---------------------------------------------------------------------------
                jmp     short end_of_turn
; ---------------------------------------------------------------------------

not_pass:                               ; CODE XREF: canMoveToTile-1AE5↑j
                cmp     al, NORTH_KEYCODE
                jnz     short not_north
                dec     _mapY
                call    write_string
; ---------------------------------------------------------------------------
aNorth          db 'NORTH',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerUp
                call    canMoveToTile
                cmp     al, 0
                jz      short loc_10973
                inc     _mapY
                jmp     loc_10A02
; ---------------------------------------------------------------------------

loc_10973:                              ; CODE XREF: canMoveToTile-1AB9↑j
                jmp     short loc_109E9
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

not_north:                              ; CODE XREF: canMoveToTile-1AD0↑j
                cmp     al, byte_1767F
                jnz     short loc_1099D
                inc     _mapY
                call    write_string
; ---------------------------------------------------------------------------
aSouth          db 'SOUTH',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerDown
                call    canMoveToTile
                cmp     al, 0
                jz      short loc_1099A
                dec     _mapY
                jmp     short loc_10A02
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_1099A:                              ; CODE XREF: canMoveToTile-1A92↑j
                jmp     short loc_109E9
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_1099D:                              ; CODE XREF: canMoveToTile-1AA9↑j
                cmp     al, byte_17681
                jnz     short loc_109C3
                dec     _mapX
                call    write_string    ; WEST
; ---------------------------------------------------------------------------
aWest           db 'WEST',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerLeft
                call    canMoveToTile
                cmp     al, 0
                jz      short loc_109C0
                inc     _mapX
                jmp     short loc_10A02
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_109C0:                              ; CODE XREF: canMoveToTile-1A6C↑j
                jmp     short loc_109E9
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_109C3:                              ; CODE XREF: canMoveToTile-1A82↑j
                cmp     al, byte_17680
                jnz     short loc_109F4
                inc     _mapX
                call    write_string    ; EAST
; ---------------------------------------------------------------------------
aEast           db 'EAST',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerRight
                call    canMoveToTile
                cmp     al, 0
                jz      short loc_109E6
                dec     _mapX
                jmp     short loc_10A02
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_109E6:                              ; CODE XREF: canMoveToTile-1A46↑j
                jmp     short loc_109E9
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_109E9:                              ; CODE XREF: canMoveToTile:loc_10973↑j
                                        ; canMoveToTile:loc_1099A↑j ...
                mov     al, byte_17436
                xor     al, 0FFh
                mov     byte_17436, al
                jmp     short loc_10A33
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_109F4:                              ; CODE XREF: canMoveToTile-1A5C↑j
                push    ax
                mov     al, 0
                mov     byte_17436, al
                pop     ax

loc_109FB:                              ; CODE XREF: canMoveToTile:loc_1285E↓j
                cmp     al, 41h ; 'A'
                jnb     short loc_10A18
                jmp     loc_12275
; ---------------------------------------------------------------------------

loc_10A02:                              ; CODE XREF: canMoveToTile-1AB3↑j
                                        ; canMoveToTile-1A8C↑j ...
                call    write_string    ; --INVALID MOVE!
; ---------------------------------------------------------------------------
aInvalidMove    db '--INVALID MOVE!',0
; ---------------------------------------------------------------------------
                jmp     short loc_10A30
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10A18:                              ; CODE XREF: canMoveToTile-1A26↑j
                cmp     al, 5Ah ; 'Z'
                jbe     short loc_10A1F
                jmp     loc_12275
; ---------------------------------------------------------------------------

loc_10A1F:                              ; CODE XREF: canMoveToTile-1A09↑j
                stc
                cmc
                sbb     al, 41h ; 'A'
                add     al, al
                mov     ah, 0
                mov     si, ax
                mov     bx, cs:[si+730h]
                jmp     bx
; ---------------------------------------------------------------------------

loc_10A30:                              ; CODE XREF: canMoveToTile-1A0E↑j
                                        ; sub_12052:loc_120A4↓j ...
                call    sub_15C95

loc_10A33:                              ; CODE XREF: canMoveToTile-1B5E↑j
                                        ; canMoveToTile:not_dead↑j ...
                mov     al, 0Dh
                call    sub_153F4
                mov     al, 0
                mov     byte_17432, al
                mov     byte_17430, al
                mov     _commandWaitCtr, al
                mov     al, player._mapNum2
                or      al, al
                jz      short loc_10A6D
                cmp     al, 1
                jnz     short loc_10A51
                jmp     loc_11013
; ---------------------------------------------------------------------------

loc_10A51:                              ; CODE XREF: canMoveToTile-19D7↑j
                cmp     al, 2
                jnz     short loc_10A58
                jmp     loc_11014
; ---------------------------------------------------------------------------

loc_10A58:                              ; CODE XREF: canMoveToTile-19D0↑j
                cmp     al, 3
                jnz     short loc_10A5F
                jmp     loc_11015
; ---------------------------------------------------------------------------

loc_10A5F:                              ; CODE XREF: canMoveToTile-19C9↑j
                cmp     al, 4
                jnz     short loc_10A66
                jmp     loc_1115F
; ---------------------------------------------------------------------------

loc_10A66:                              ; CODE XREF: canMoveToTile-19C2↑j
                cmp     al, 5
                jnz     short loc_10A6D
                jmp     loc_1115E
; ---------------------------------------------------------------------------

loc_10A6D:                              ; CODE XREF: canMoveToTile-19DB↑j
                                        ; canMoveToTile-19BB↑j
                nop
                mov     bh, 0
                mov     bl, 1Fh
                mov     di, bx

loc_10A74:                              ; CODE XREF: canMoveToTile-1981↓j
                mov     al, [di+197h]
                or      al, al
                jz      short loc_10AA1
                mov     al, _playerTileId
                cmp     al, 22h ; '"'
                jnz     short loc_10AA7
                mov     al, byte_17436
                or      al, al
                jns     short loc_10AA7
                mov     al, [di+197h]
                cmp     al, 48h ; 'H'
                jz      short loc_10AA7
                cmp     al, 3Ch ; '<'
                jz      short loc_10AA7
                cmp     al, 2Ch ; ','
                jz      short loc_10AA7
                mov     al, _playerTileId
                cmp     al, 22h ; '"'
                jnz     short loc_10AA7

loc_10AA1:                              ; CODE XREF: canMoveToTile-19A9↑j
                                        ; canMoveToTile-1877↓j ...
                dec     di
                jnz     short loc_10A74
                jmp     loc_10C9C
; ---------------------------------------------------------------------------

loc_10AA7:                              ; CODE XREF: canMoveToTile-19A2↑j
                                        ; canMoveToTile-199B↑j ...
                call    sub_14F36
                mov     bx, di
                mov     points_to_distrubte, bl
                mov     al, _circleDeltaX
                call    sub_150EF
                cmp     al, 3
                jnb     short loc_10AFC
                mov     al, _circleDeltaY
                call    sub_150EF
                cmp     al, 3
                jnb     short loc_10AFC
                call    sub_15217
                mov     bh, 0
                mov     bl, points_to_distrubte
                mov     di, bx
                cmp     al, 20h ; ' '
                jnb     short loc_10AFC
                mov     al, [di+197h]
                cmp     al, 34h ; '4'
                jnz     short loc_10AE1
                call    sub_10E70
; ---------------------------------------------------------------------------
                jmp     short loc_10AFC
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10AE1:                              ; CODE XREF: canMoveToTile-194A↑j
                cmp     al, 38h ; '8'
                jnz     short loc_10AEB
                call    sub_10EC2
; ---------------------------------------------------------------------------
                jmp     short loc_10AFC
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10AEB:                              ; CODE XREF: canMoveToTile-1940↑j
                cmp     al, 0F8h
                jnz     short loc_10AF5
                call    sub_10F12
; ---------------------------------------------------------------------------
                jmp     short loc_10AFC
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10AF5:                              ; CODE XREF: canMoveToTile-1936↑j
                cmp     al, 3Ch ; '<'
                jnz     short loc_10AFC
                call    near ptr sub_10F48
; ---------------------------------------------------------------------------

loc_10AFC:                              ; CODE XREF: canMoveToTile-196B↑j
                                        ; canMoveToTile-1961↑j ...
                mov     bh, 0
                mov     bl, points_to_distrubte
                mov     di, bx
                call    sub_14F11
                mov     al, [di+177h]
                cmp     al, 0Fh
                jnb     short loc_10B25
                stc
                mov     al, 0
                cmc
                sbb     al, _circleDeltaX
                mov     _circleDeltaX, al
                stc
                mov     al, 0
                cmc
                sbb     al, _circleDeltaY
                mov     _circleDeltaY, al

loc_10B25:                              ; CODE XREF: canMoveToTile-1916↑j
                clc
                mov     al, [di+137h]
                adc     al, _circleDeltaX
                and     al, 3Fh
                cmp     al, _mapX
                jz      short loc_10B39
                jmp     loc_12122
; ---------------------------------------------------------------------------

loc_10B39:                              ; CODE XREF: canMoveToTile-18EF↑j
                clc
                mov     al, [di+157h]
                adc     al, _circleDeltaY
                and     al, 3Fh
                cmp     al, _mapY
                jz      short loc_10B4D
                jmp     loc_12122
; ---------------------------------------------------------------------------

loc_10B4D:                              ; CODE XREF: canMoveToTile-18DB↑j
                inc     byte_17430
                mov     bx, di
                mov     _sleepFlag2?, bl
                mov     al, [di+197h]
                cmp     al, 0FCh
                jnz     short loc_10B62
                call    sub_10FCD

loc_10B62:                              ; CODE XREF: canMoveToTile-18C6↑j
                call    sub_15217
                mov     bh, 0
                mov     bl, _sleepFlag2?
                mov     di, bx
                cmp     al, 0
                js      short loc_10BA0
                and     al, 7
                cmp     al, byte ptr player+2Ch
                jb      short loc_10BA0
                inc     _commandWaitCtr
                mov     al, [di+177h]
                clc
                rcr     al, 1
                clc
                rcr     al, 1
                stc
                adc     al, byte_17432
                mov     bh, 0
                mov     bl, _tilePlayerCenter
                mov     si, bx
                mov     bx, si
                cmp     bl, 6
                jnz     short loc_10B9D
                add     al, al

loc_10B9D:                              ; CODE XREF: canMoveToTile-188A↑j
                mov     byte_17432, al

loc_10BA0:                              ; CODE XREF: canMoveToTile-18B4↑j
                                        ; canMoveToTile-18AC↑j
                nop
                mov     al, player._mapNum2
                or      al, al
                jz      short loc_10BAB
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_10BAB:                              ; CODE XREF: canMoveToTile-187D↑j
                nop
                jmp     loc_10AA1
; ---------------------------------------------------------------------------

loc_10BAF:                              ; CODE XREF: canMoveToTile-1776↓j
                                        ; canMoveToTile-1766↓j ...
                call    draw_map

loc_10BB2:                              ; CODE XREF: canMoveToTile-185F↓j
                                        ; canMoveToTile-1854↓j
                mov     al, byte_17430
                or      al, al
                jz      short loc_10BD1
                call    sub_15EAC
                dec     byte_17430
                dec     _commandWaitCtr
                js      short loc_10BB2
                call    xorSpriteDrawCenter
                call    pause?
                call    xorSpriteDrawCenter
                jmp     short loc_10BB2
; ---------------------------------------------------------------------------

loc_10BD1:                              ; CODE XREF: canMoveToTile-186C↑j
                mov     al, byte_17432
                or      al, al
                jz      short loc_10C1C
                call    sub_15217
                and     al, byte_17432
                and     al, 77h
                clc
                adc     al, 1
                mov     byte_17432, al
                stc
                mov     al, player._hp+1
                cmc
                sbb     al, byte_17432
                das
                cmc
                mov     player._hp+1, al
                mov     al, player._hp
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._hp, al
                jnb     short dead
                mov     al, player._mapNum2
                or      al, al
                jz      short loc_10C1C
                mov     byte_17432, al
                stc
                mov     al, player._hp
                cmc
                sbb     al, byte_17432
                das
                cmc
                mov     player._hp, al
                jnb     short dead

loc_10C1C:                              ; CODE XREF: canMoveToTile-184D↑j
                                        ; canMoveToTile-181C↑j
                call    write_stats
; ---------------------------------------------------------------------------
                mov     al, 4
                mov     _outsideMapTile, al
                mov     al, player._mapNum2
                or      al, al
                jnz     short loc_10C33
                mov     al, 0FFh
                mov     _outsideMapTile, al
                call    sub_126F9

loc_10C33:                              ; CODE XREF: canMoveToTile-17FA↑j
                mov     al, player_paralyzedFlag
                or      al, al
                jz      short loc_10C3E
                dec     player_paralyzedFlag

loc_10C3E:                              ; CODE XREF: canMoveToTile-17EB↑j
                mov     al, _flag3
                or      al, al
                jz      short loc_10C49
                dec     _flag3

loc_10C49:                              ; CODE XREF: canMoveToTile-17E0↑j
                mov     al, _sleepFlag
                or      al, al
                jz      short loc_10C54
                dec     _sleepFlag

loc_10C54:                              ; CODE XREF: canMoveToTile-17D5↑j
                mov     al, _flag1
                or      al, al
                jz      short loc_10C5F
                dec     _flag1

loc_10C5F:                              ; CODE XREF: canMoveToTile-17CA↑j
                jmp     loc_1086F
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_10F8E
;   ADDITIONAL PARENT FUNCTION sub_112DB
;   ADDITIONAL PARENT FUNCTION canMoveToTile

dead:                                   ; CODE XREF: canMoveToTile-1823↑j
                                        ; canMoveToTile-1809↑j ...
                mov     al, 0
                mov     player._hp, al
                mov     player._hp+1, al
                mov     player._food, al
                mov     player._food+1, al
                mov     player._foodTurnCtr, al
                mov     player._experience, al
                mov     player._experience+1, al
                mov     player._gold, al
                mov     player._gold+1, al
                call    write_stats
; ---------------------------------------------------------------------------
                call    write_string
; ---------------------------------------------------------------------------
                db 8Dh,8Dh,8Dh,0
; ---------------------------------------------------------------------------
                call    write_player_name
                call    write_string
; ---------------------------------------------------------------------------
aIsDead         db ' IS DEAD!'
                db  8Dh
                db    0
; ---------------------------------------------------------------------------

halt:                                   ; CODE XREF: sub_10F8E:halt↓j
                jmp     short halt
; END OF FUNCTION CHUNK FOR sub_10F8E
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10C9C:                              ; CODE XREF: canMoveToTile-197F↑j
                mov     bh, 0
                mov     bl, 1Fh
                mov     di, bx

loc_10CA2:                              ; CODE XREF: canMoveToTile-1778↓j
                mov     al, [di+197h]
                or      al, al
                jz      short loc_10CB0
                dec     di
                jnz     short loc_10CA2
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10CB0:                              ; CODE XREF: canMoveToTile-177B↑j
                mov     bx, di
                mov     byte_1742F, bl
                call    sub_15217
                cmp     al, 3Fh ; '?'
                jb      short loc_10CC0
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10CC0:                              ; CODE XREF: canMoveToTile-1768↑j
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                mov     [di+137h], al
                mov     _playerX, al
                call    sub_15217
                js      short loc_10CD9
                and     al, 3Fh
                jmp     short loc_10CDC
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10CD9:                              ; CODE XREF: canMoveToTile-1751↑j
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10CDC:                              ; CODE XREF: canMoveToTile-174D↑j
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                mov     [di+157h], al
                mov     _playerY, al
                call    get_player_tile
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                mov     [di+1B7h], al
                inc     byte ptr player+2Ah
                mov     al, byte ptr player+2Ah
                clc
                rcr     al, 1
                jnb     short loc_10D09
                jmp     short loc_10D44
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D09:                              ; CODE XREF: canMoveToTile-171F↑j
                clc
                rcr     al, 1
                jnb     short loc_10D11
                jmp     short loc_10D64
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D11:                              ; CODE XREF: canMoveToTile-1717↑j
                clc
                rcr     al, 1
                jnb     short loc_10D19
                jmp     short loc_10D84
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D19:                              ; CODE XREF: canMoveToTile-170F↑j
                clc
                rcr     al, 1
                jnb     short loc_10D21
                jmp     loc_10DA4
; ---------------------------------------------------------------------------

loc_10D21:                              ; CODE XREF: canMoveToTile-1707↑j
                clc
                rcr     al, 1
                jnb     short loc_10D29
                jmp     loc_10DD4
; ---------------------------------------------------------------------------

loc_10D29:                              ; CODE XREF: canMoveToTile-16FF↑j
                clc
                rcr     al, 1
                jnb     short loc_10D31
                jmp     loc_10DCB
; ---------------------------------------------------------------------------

loc_10D31:                              ; CODE XREF: canMoveToTile-16F7↑j
                clc
                rcr     al, 1
                jnb     short loc_10D39
                jmp     loc_10DCE
; ---------------------------------------------------------------------------

loc_10D39:                              ; CODE XREF: canMoveToTile-16EF↑j
                clc
                rcr     al, 1
                jnb     short loc_10D41
                jmp     loc_10DD1
; ---------------------------------------------------------------------------

loc_10D41:                              ; CODE XREF: canMoveToTile-16E7↑j
                jmp     loc_10E50
; ---------------------------------------------------------------------------

loc_10D44:                              ; CODE XREF: canMoveToTile-171D↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10D4F
                jmp     short loc_10DC4
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D4F:                              ; CODE XREF: canMoveToTile-16D9↑j
                mov     al, 30h ; '0'
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 10h
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10D64:                              ; CODE XREF: canMoveToTile-1715↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10D6F
                jmp     short loc_10DC4
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D6F:                              ; CODE XREF: canMoveToTile-16B9↑j
                mov     al, 0FCh
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 20h ; ' '
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10D84:                              ; CODE XREF: canMoveToTile-170D↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10D8F
                jmp     short loc_10DC4
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10D8F:                              ; CODE XREF: canMoveToTile-1699↑j
                mov     al, 34h ; '4'
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 40h ; '@'
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10DA4:                              ; CODE XREF: canMoveToTile-1705↑j
                mov     al, [di+1B7h]
                cmp     al, 0
                jz      short loc_10DAF
                jmp     short loc_10DC4
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10DAF:                              ; CODE XREF: canMoveToTile-1679↑j
                mov     al, 2Ch ; ','
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 40h ; '@'
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10DC4:                              ; CODE XREF: canMoveToTile-16D7↑j
                                        ; canMoveToTile-16B7↑j ...
                dec     byte ptr player+2Ah
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10DCB:                              ; CODE XREF: canMoveToTile-16F5↑j
                jmp     short loc_10DF3
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10DCE:                              ; CODE XREF: canMoveToTile-16ED↑j
                jmp     short loc_10E12
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10DD1:                              ; CODE XREF: canMoveToTile-16E5↑j
                jmp     short loc_10E31
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_10DD4:                              ; CODE XREF: canMoveToTile-16FD↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10DDE
                jmp     short loc_10DC4
; ---------------------------------------------------------------------------

loc_10DDE:                              ; CODE XREF: canMoveToTile-1649↑j
                mov     al, 0F0h
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 80h
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10DF3:                              ; CODE XREF: canMoveToTile:loc_10DCB↑j
                mov     al, [di+1B7h]
                cmp     al, 0
                jz      short loc_10DFD
                jmp     short loc_10DC4
; ---------------------------------------------------------------------------

loc_10DFD:                              ; CODE XREF: canMoveToTile-162A↑j
                mov     al, 48h ; 'H'
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 0A0h
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10E12:                              ; CODE XREF: canMoveToTile:loc_10DCE↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10E1C
                jmp     short loc_10DC4
; ---------------------------------------------------------------------------

loc_10E1C:                              ; CODE XREF: canMoveToTile-160B↑j
                mov     al, 38h ; '8'
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 0C0h
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10E31:                              ; CODE XREF: canMoveToTile:loc_10DD1↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10E3B
                jmp     short loc_10DC4
; ---------------------------------------------------------------------------

loc_10E3B:                              ; CODE XREF: canMoveToTile-15EC↑j
                mov     al, 0F8h
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 0E0h
                mov     [di+177h], al
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_10E50:                              ; CODE XREF: canMoveToTile:loc_10D41↑j
                mov     al, [di+1B7h]
                cmp     al, 8
                jz      short loc_10E5B
                jmp     loc_10DC4
; ---------------------------------------------------------------------------

loc_10E5B:                              ; CODE XREF: canMoveToTile-15CD↑j
                mov     al, 3Ch ; '<'
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     [di+197h], al
                mov     al, 0FFh
                mov     [di+177h], al
                jmp     loc_10BAF
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

sub_10E70       proc near               ; CODE XREF: canMoveToTile-1948↑p

; FUNCTION CHUNK AT 0EB8 SIZE 0000000A BYTES

                call    write_string    ; LEGS PARALIZED!
; ---------------------------------------------------------------------------
aLegsParalized  db 'LEGS PARALIZED!',0Dh,0
; ---------------------------------------------------------------------------
                call    sub_14B26
                call    sub_15FC3
                call    sub_14B26
                cmp     byte ptr player+0A3h, 0
                jz      short loc_10EB8
                call    sub_15217
                cmp     al, 40h ; '@'
                jb      short loc_10EB8
                call    write_string    ; SAVED BY MAGICAL BOOTS!
; ---------------------------------------------------------------------------
aSavedByMagical db 'SAVED BY MAGICAL BOOTS!',8Dh,0
sub_10E70       endp

; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_10E70

loc_10EB8:                              ; CODE XREF: sub_10E70+22↑j
                                        ; sub_10E70+29↑j
                nop
                call    sub_15217
                and     al, 0Fh
                mov     player_paralyzedFlag, al
                retn
; END OF FUNCTION CHUNK FOR sub_10E70

; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

sub_10EC2       proc near               ; CODE XREF: canMoveToTile-193E↑p

; FUNCTION CHUNK AT 0F09 SIZE 00000009 BYTES

                call    write_string    ; ARMS PARALIZED!
; ---------------------------------------------------------------------------
aArmsParalized  db 'ARMS PARALIZED!',0Dh,0
; ---------------------------------------------------------------------------
                call    sub_14B26
                call    sub_15FC3
                call    sub_14B26
                mov     al, byte ptr player+0A4h
                or      al, al
                jz      short loc_10F09
                call    sub_15217
                cmp     al, 40h ; '@'
                jb      short loc_10F09
                call    write_string    ; SAVED BY MAGICAL CLOAK
; ---------------------------------------------------------------------------
aSavedByMagical_0 db 'SAVED BY MAGICAL CLOAK',8Dh,0
sub_10EC2       endp

; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_10EC2

loc_10F09:                              ; CODE XREF: sub_10EC2+22↑j
                                        ; sub_10EC2+29↑j
                call    sub_15217
                and     al, 0Fh
                mov     _flag3, al
                retn
; END OF FUNCTION CHUNK FOR sub_10EC2

; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

sub_10F12       proc near               ; CODE XREF: canMoveToTile-1934↑p
                call    write_string    ; MAGIC MISSILE!
; ---------------------------------------------------------------------------
aMagicMissile   db 'MAGIC MISSILE!',0Dh,0
; ---------------------------------------------------------------------------
                call    sub_14B26
                call    sub_15FC3
                call    sub_14B26
                inc     byte_17430
                inc     byte_17430
                inc     _commandWaitCtr
                inc     _commandWaitCtr
                clc
                mov     al, byte_17432
                adc     al, 40h ; '@'
                mov     byte_17432, al
                retn
sub_10F12       endp


; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

sub_10F48       proc far                ; CODE XREF: canMoveToTile-192A↑p
                call    write_string    ; SLEEP SPELL!
; ---------------------------------------------------------------------------
aSleepSpell     db 'SLEEP SPELL!',0Dh,0
sub_10F48       endp

; ---------------------------------------------------------------------------
                call    sub_14B26
                call    sub_15FC3
                call    sub_14B26
                mov     al, byte ptr player+0AEh
                or      al, al
                jz      short loc_10F84
                call    sub_15217
                cmp     al, 40h ; '@'
                jb      short loc_10F84
                call    write_string
; ---------------------------------------------------------------------------
                db  53h ; S
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  44h ; D
                db  20h
                db  42h ; B
                db  59h ; Y
                db  20h
                db  49h ; I
                db  44h ; D
                db  4Fh ; O
                db  4Ch ; L
                db  21h ; !
                db  8Dh
                db    0
                db 0C3h
; ---------------------------------------------------------------------------

loc_10F84:                              ; CODE XREF: sg01a2:0F67↑j
                                        ; sg01a2:0F6E↑j
                nop
                call    sub_15217
                and     al, 0Fh
                mov     _sleepFlag, al
                retn

; =============== S U B R O U T I N E =======================================


sub_10F8E       proc near               ; CODE XREF: canMoveToTile-13B9↓p

; FUNCTION CHUNK AT 0C62 SIZE 0000003A BYTES

                nop
                call    write_string    ; MINAX CRIES: DIE FOOL!
; ---------------------------------------------------------------------------
aMinaxCriesDieF db 'MINAX CRIES: DIE FOOL!',8Dh,0
; ---------------------------------------------------------------------------

loc_10FAA:                              ; CODE XREF: sub_10F8E+1↑j
                mov     al, 0
                mov     _circleDeltaX, al
                mov     _circleDeltaY, al
                call    xorDrawCircle
                call    pause?
                call    xorDrawCircle
                stc
                mov     al, player._hp
                cmc
                sbb     al, 1
                das
                cmc
                mov     player._hp, al
                jb      short locret_10FCC
                jmp     dead
; ---------------------------------------------------------------------------

locret_10FCC:                           ; CODE XREF: sub_10F8E+39↑j
                retn
sub_10F8E       endp


; =============== S U B R O U T I N E =======================================


sub_10FCD       proc near               ; CODE XREF: canMoveToTile-18C4↑p
                nop
                call    sub_15217
                cmp     al, 40h ; '@'
                jb      short loc_10FD6
                retn
; ---------------------------------------------------------------------------

loc_10FD6:                              ; CODE XREF: sub_10FCD+6↑j
                nop
                call    sub_15217
                and     ax, 0Fh
                mov     di, ax
                mov     al, [di+0D6h]
                or      al, al
                jz      short nullsub_1
                stc
                mov     al, [di+0D6h]
                cmc
                sbb     al, 1
                das
                cmc
                mov     [di+0D6h], al
                call    write_string    ; A THIEF STOLE SOMETHING!
sub_10FCD       endp

; ---------------------------------------------------------------------------
aAThiefStoleSom db 'A THIEF STOLE SOMETHING!',8Dh,0
; [00000001 BYTES: COLLAPSED FUNCTION nullsub_1. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_11013:                              ; CODE XREF: canMoveToTile-19D5↑j
                nop

loc_11014:                              ; CODE XREF: canMoveToTile-19CE↑j
                nop

loc_11015:                              ; CODE XREF: canMoveToTile-19C7↑j
                nop
                mov     bh, 0
                mov     bl, 1Fh
                mov     di, bx

loc_1101C:                              ; CODE XREF: canMoveToTile-13AD↓j
                nop
                mov     bx, di
                mov     byte_17435, bl
                mov     al, [di+197h]
                or      al, al
                jz      short loc_11040
                mov     al, [di+1D7h]
                or      al, al
                jz      short loc_11040
                js      short loc_11040
                cmp     al, 3
                jnb     short loc_11086
                cmp     al, 2
                jz      short loc_1107B
                jmp     short loc_11082
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_11040:                              ; CODE XREF: canMoveToTile-187B↑j
                                        ; canMoveToTile-13FA↑j ...
                mov     al, [di+197h]
                cmp     al, 40h ; '@'
                jnz     short loc_1106D
                mov     al, [di+137h]
                stc
                cmc
                sbb     al, _mapX
                call    sub_150EF
                cmp     al, 4
                jnb     short loc_1106D
                mov     al, [di+157h]
                stc
                cmc
                sbb     al, _mapY
                call    sub_150EF
                cmp     al, 4
                jnb     short loc_1106D
                call    sub_10F8E

loc_1106D:                              ; CODE XREF: canMoveToTile-13DD↑j
                                        ; canMoveToTile-13CC↑j ...
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                dec     di
                jns     short loc_1101C
                jmp     loc_10BAF
; ---------------------------------------------------------------------------

loc_1107B:                              ; CODE XREF: canMoveToTile-13E8↑j
                nop
                call    sub_14F11
                jmp     loc_12122
; ---------------------------------------------------------------------------

loc_11082:                              ; CODE XREF: canMoveToTile-13E6↑j
                nop
                jmp     loc_10AA7
; ---------------------------------------------------------------------------

loc_11086:                              ; CODE XREF: canMoveToTile-13EC↑j
                nop
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                mov     al, [di+1F7h]
                mov     _circleDeltaX, al
                mov     al, [di+217h]
                mov     _circleDeltaY, al
                call    sub_15217
                cmp     al, 40h ; '@'
                jnb     short loc_110CE
                call    sub_15217
                call    sub_150D8
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                mov     [di+1F7h], al
                mov     _circleDeltaX, al
                call    sub_15217
                call    sub_150D8
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                mov     [di+217h], al
                mov     _circleDeltaY, al

loc_110CE:                              ; CODE XREF: canMoveToTile-1381↑j
                nop
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                clc
                mov     al, [di+137h]
                adc     al, _circleDeltaX
                and     al, 3Fh
                mov     _playerX, al
                cmp     al, 4
                jnb     short loc_110EC
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_110EC:                              ; CODE XREF: canMoveToTile-133C↑j
                cmp     al, 3Ch ; '<'
                jb      short loc_110F3
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_110F3:                              ; CODE XREF: canMoveToTile-1335↑j
                cmp     al, _mapX
                jnz     short loc_110FC
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_110FC:                              ; CODE XREF: canMoveToTile-132C↑j
                clc
                mov     al, [di+157h]
                adc     al, _circleDeltaY
                and     al, 3Fh
                mov     _playerY, al
                cmp     al, 4
                jnb     short loc_11111
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_11111:                              ; CODE XREF: canMoveToTile-1317↑j
                cmp     al, 3Ch ; '<'
                jb      short loc_11118
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_11118:                              ; CODE XREF: canMoveToTile-1310↑j
                cmp     al, _mapY
                jnz     short loc_11121
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_11121:                              ; CODE XREF: canMoveToTile-1307↑j
                call    get_player_tile
                cmp     al, 70h ; 'p'
                jz      short loc_1112F
                cmp     al, 8
                jz      short loc_1112F
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_1112F:                              ; CODE XREF: canMoveToTile-12FD↑j
                                        ; canMoveToTile-12F9↑j
                mov     bh, 0
                mov     bl, byte_17435
                mov     di, bx
                jmp     loc_12201
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db    1
                db 0C3h
                db 0B7h
                db    0
                db 0B3h
                db    7
                db  8Bh
                db 0FBh
                db 0B0h
                db    1
                db  88h
                db  85h
                db 0D7h
                db    1
                db  4Fh ; O
                db  79h ; y
                db 0F9h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0FBh
                db  88h
                db  85h
                db 0D7h
                db    1
                db 0C3h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_1115E:                              ; CODE XREF: canMoveToTile-19B9↑j
                nop

loc_1115F:                              ; CODE XREF: canMoveToTile-19C0↑j
                nop
                call    sub_1147F
                nop
                mov     bh, 0
                mov     bl, 1Fh
                mov     di, bx
                mov     bx, di
                mov     byte_1742F, bl

loc_11170:                              ; CODE XREF: canMoveToTile-129D↓j
                mov     al, [di+197h]
                or      al, al
                jnz     short loc_1118B

loc_11178:                              ; CODE XREF: canMoveToTile:loc_11229↓j
                                        ; canMoveToTile-11EF↓j ...
                dec     byte_1742F
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                or      di, di
                jnz     short loc_11170
                jmp     loc_11291
; ---------------------------------------------------------------------------

loc_1118B:                              ; CODE XREF: canMoveToTile-12AD↑j
                nop
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                call    sub_1152E
                mov     al, [di+177h]
                cmp     al, 0Fh
                jnb     short loc_111B5
                stc
                mov     al, 0
                cmc
                sbb     al, _circleDeltaX
                mov     _circleDeltaX, al
                stc
                mov     al, 0
                cmc
                sbb     al, _circleDeltaY
                mov     _circleDeltaY, al

loc_111B5:                              ; CODE XREF: canMoveToTile-1286↑j
                nop
                mov     al, _circleDeltaX
                or      al, al
                jz      short loc_111EF
                clc
                adc     al, [di+137h]
                mov     _playerX, al
                mov     al, [di+157h]
                mov     _playerY, al
                mov     al, _playerX
                cmp     al, _mapX
                jnz     short loc_111E1
                mov     al, _playerY
                cmp     al, _mapY
                jnz     short loc_111E1
                jmp     loc_11308
; ---------------------------------------------------------------------------

loc_111E1:                              ; CODE XREF: canMoveToTile-1250↑j
                                        ; canMoveToTile-1247↑j
                nop
                call    sub_1143C
                jz      short loc_1122C
                cmp     al, 0FFh
                jz      short loc_111EF
                and     al, 7Fh
                jnz     short loc_1122C

loc_111EF:                              ; CODE XREF: canMoveToTile-1268↑j
                                        ; canMoveToTile-123A↑j
                nop
                mov     al, _circleDeltaY
                or      al, al
                jz      short loc_11229
                clc
                adc     al, [di+157h]
                mov     _playerY, al
                mov     al, [di+137h]
                mov     _playerX, al
                mov     al, _playerX
                cmp     al, _mapX
                jnz     short loc_1121B
                mov     al, _playerY
                cmp     al, _mapY
                jnz     short loc_1121B
                jmp     loc_11308
; ---------------------------------------------------------------------------

loc_1121B:                              ; CODE XREF: canMoveToTile-1216↑j
                                        ; canMoveToTile-120D↑j
                nop
                call    sub_1143C
                jz      short loc_1122C
                cmp     al, 0FFh
                jz      short loc_11229
                and     al, 7Fh
                jnz     short loc_1122C

loc_11229:                              ; CODE XREF: canMoveToTile-122E↑j
                                        ; canMoveToTile-1200↑j
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_1122C:                              ; CODE XREF: canMoveToTile-123E↑j
                                        ; canMoveToTile-1236↑j ...
                nop
                call    map_get_monster_at?
                and     al, 7
                jz      short loc_11237
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_11237:                              ; CODE XREF: canMoveToTile-11F1↑j
                nop
                mov     al, _playerX
                mov     [di+1F7h], al
                mov     al, _playerY
                mov     [di+217h], al
                mov     al, [di+137h]
                mov     _playerX, al
                mov     al, [di+157h]
                mov     _playerY, al
                call    map_get_monster_at?
                mov     bx, word_17418
                mov     al, [bx+si]
                and     al, 0F0h
                mov     bx, word_17418
                mov     [bx+si], al
                mov     al, [di+1F7h]
                mov     _playerX, al
                mov     [di+137h], al
                mov     al, [di+217h]
                mov     _playerY, al
                mov     [di+157h], al
                call    map_get_monster_at?
                mov     bx, word_17418
                mov     al, [bx+si]
                or      al, [di+197h]
                mov     bx, word_17418
                mov     [bx+si], al
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_11291:                              ; CODE XREF: canMoveToTile-129B↑j
                nop
                mov     al, byte_17436
                or      al, al
                jnz     short near ptr loc_112AC+2
                call    sub_14B4E
                call    write_string    ; IT'S DARK!
; ---------------------------------------------------------------------------
aItSDark        db 'IT',27h,'S DARK!',8Dh,0
                db 0EBh
; ---------------------------------------------------------------------------

loc_112AC:                              ; CODE XREF: canMoveToTile-118C↑j
                and     [bx+si+0EFEh], dl
                add     es:[di+19h], dh
                call    write_string    ; TORCH BURNED OUT!
; ---------------------------------------------------------------------------
aTorchBurnedOut db 'TORCH BURNED OUT!',8Dh,0
; ---------------------------------------------------------------------------
                call    sub_15C95
                call    sub_16000
                mov     al, 10h
                call    sub_112DB
                call    write_stats
; ---------------------------------------------------------------------------
                jmp     loc_1086F
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


sub_112DB       proc near               ; CODE XREF: canMoveToTile-1151↑p

; FUNCTION CHUNK AT 0C62 SIZE 0000003A BYTES

                mov     byte_17430, al
                stc
                mov     al, player._foodTurnCtr
                cmc
                sbb     al, byte_17430
                das
                cmc
                mov     player._foodTurnCtr, al
                mov     al, player._food+1
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food+1, al
                mov     al, player._food
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food, al
                jb      short locret_11307
                jmp     dead
; ---------------------------------------------------------------------------

locret_11307:                           ; CODE XREF: sub_112DB+27↑j
                retn
sub_112DB       endp ; sp-analysis failed

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_11308:                              ; CODE XREF: canMoveToTile-1245↑j
                                        ; canMoveToTile-120B↑j
                nop
                call    sub_15217
                cmp     al, 40h ; '@'
                jnb     short loc_1133E
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                mov     al, byte_17435
                cmp     al, [di+1B7h]
                jz      short loc_11324
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_11324:                              ; CODE XREF: canMoveToTile-1104↑j
                nop
                mov     al, [di+197h]
                cmp     al, 2
                jnz     short loc_11330
                jmp     short loc_113AB
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_11330:                              ; CODE XREF: canMoveToTile-10F8↑j
                cmp     al, 5
                jnz     short loc_11337
                jmp     loc_113DA
; ---------------------------------------------------------------------------

loc_11337:                              ; CODE XREF: canMoveToTile-10F1↑j
                cmp     al, 7
                jnz     short loc_1133E
                jmp     loc_11410
; ---------------------------------------------------------------------------

loc_1133E:                              ; CODE XREF: canMoveToTile-1115↑j
                                        ; canMoveToTile-10EA↑j
                nop
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                mov     al, byte_17435
                cmp     al, [di+1B7h]
                jz      short loc_11353
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_11353:                              ; CODE XREF: canMoveToTile-10D5↑j
                nop
                call    sub_15EAC
                call    sub_15217
                js      short loc_11364
                and     al, 7
                cmp     al, byte ptr player+2Ch
                jnb     short loc_11367

loc_11364:                              ; CODE XREF: canMoveToTile-10C9↑j
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_11367:                              ; CODE XREF: canMoveToTile-10C1↑j
                nop
                call    sub_14B26
                call    pause?
                call    sub_14B26
                call    sub_15217
                and     al, 77h
                mov     byte_17430, al
                mov     al, byte_17435
                add     al, al
                add     al, al
                and     al, 77h
                adc     al, byte_17430
                mov     byte_17430, al
                stc
                mov     al, player._hp+1
                cmc
                sbb     al, byte_17430
                das
                cmc
                mov     player._hp+1, al
                mov     al, player._hp
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._hp, al
                jb      short loc_113A7
                jmp     dead
; ---------------------------------------------------------------------------

loc_113A7:                              ; CODE XREF: canMoveToTile-1081↑j
                nop
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_113AB:                              ; CODE XREF: canMoveToTile-10F6↑j
                mov     al, byte_17436
                or      al, al
                jz      short loc_113D7
                call    write_string    ; YOUR TORCH IS BLOWN OUT!
; ---------------------------------------------------------------------------
aYourTorchIsBlo db 'YOUR TORCH IS BLOWN OUT!',8Dh,0
; ---------------------------------------------------------------------------
                call    sub_15C95
; END OF FUNCTION CHUNK FOR canMoveToTile
                mov     al, 0
                mov     byte_17436, al
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_113D7:                              ; CODE XREF: canMoveToTile-1073↑j
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_113DA:                              ; CODE XREF: canMoveToTile-10EF↑j
                call    write_string    ; A GREMLIN STOLE SOME FOOD!
; ---------------------------------------------------------------------------
aAGremlinStoleS db 'A GREMLIN STOLE SOME FOOD!',8Dh,0
; ---------------------------------------------------------------------------
                call    sub_15C95
                stc
                mov     al, player._food
                cmc
                sbb     al, 1
                das
                cmc
                mov     player._food, al
                jnb     short loc_1140D
                jmp     loc_11178
; ---------------------------------------------------------------------------

loc_1140D:                              ; CODE XREF: canMoveToTile-101B↑j
                jmp     dead
; ---------------------------------------------------------------------------

loc_11410:                              ; CODE XREF: canMoveToTile-10E8↑j
                nop
                call    write_string    ; YOU FEEL A STRONG MAGIC!
; ---------------------------------------------------------------------------
aYouFeelAStrong db 'YOU FEEL A STRONG MAGIC!',8Dh,0
; ---------------------------------------------------------------------------
                call    sub_15C95
                call    sub_15217
                and     al, 7
                mov     _sleepFlag, al
                jmp     loc_11178
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


sub_1143C       proc near               ; CODE XREF: canMoveToTile-1241↑p
                                        ; canMoveToTile-1207↑p
                nop
                mov     al, byte_17435
                cmp     al, [di+1B7h]
                jz      short loc_1144B
                mov     al, 0FFh
                or      al, al
                retn
; ---------------------------------------------------------------------------

loc_1144B:                              ; CODE XREF: sub_1143C+8↑j
                nop
sub_1143C       endp


; =============== S U B R O U T I N E =======================================


map_get_monster_at? proc near           ; CODE XREF: canMoveToTile-11F6↑p
                                        ; canMoveToTile-11CF↑p ...
                nop
                mov     al, (_mapMonsters+80h)[di]
                nop
                mov     ah, al
                mov     al, 0
                add     ax, map_ptr
                mov     byte ptr word_17418+1, ah
                mov     al, _playerY
                add     al, al
                add     al, al
                add     al, al
                add     al, al
                adc     al, _playerX
                mov     byte ptr word_17418, al
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     bx, word_17418
                mov     al, [bx+si]
                or      al, al
                retn
map_get_monster_at? endp


; =============== S U B R O U T I N E =======================================


sub_1147F       proc near               ; CODE XREF: canMoveToTile-12C3↑p
                nop
                mov     bh, 0
                mov     bl, 1Fh
                mov     di, bx

loc_11486:                              ; CODE XREF: sub_1147F+1E↓j
                mov     bx, di
                mov     byte_1742F, bl
                mov     al, [di+197h]
                or      al, al
                jz      short loc_114A0

loc_11494:                              ; CODE XREF: sub_1147F+5E↓j
                                        ; sub_1147F:loc_114FA↓j ...
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                dec     di
                jnz     short loc_11486
                retn
; ---------------------------------------------------------------------------

loc_114A0:                              ; CODE XREF: sub_1147F+13↑j
                nop
                call    sub_15217
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                and     al, 7
                add     al, al
                or      al, 1
                mov     [di+137h], al
                mov     _playerX, al
                call    sub_15217
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                and     al, 7
                add     al, al
                or      al, 1
                mov     [di+157h], al
                mov     _playerY, al
                mov     ax, di
                clc
                rcr     al, 1
                mov     [di+1B7h], al
                call    map_get_monster_at?
                jnz     short loc_11494
                call    sub_15217
                mov     bh, 0
                mov     bl, byte_1742F
                mov     di, bx
                and     al, 7
                cmp     al, 7
                jz      short loc_114FA
                add     al, al
                cmp     al, [di+1B7h]
                jz      short loc_114FC
                jb      short loc_114FC

loc_114FA:                              ; CODE XREF: sub_1147F+6F↑j
                jmp     short loc_11494
; ---------------------------------------------------------------------------

loc_114FC:                              ; CODE XREF: sub_1147F+77↑j
                                        ; sub_1147F+79↑j
                nop
                clc
                rcr     al, 1
                adc     al, 1
                mov     [di+197h], al
                add     al, al
                adc     al, [di+1B7h]
                add     al, al
                add     al, al
                add     al, al
                or      al, 10h
                mov     [di+177h], al
                call    map_get_monster_at?
                mov     bx, word_17418
                mov     al, [bx+si]
                or      al, [di+197h]
                mov     bx, word_17418
                mov     [bx+si], al
                jmp     loc_11494
sub_1147F       endp


; =============== S U B R O U T I N E =======================================


sub_1152E       proc near               ; CODE XREF: canMoveToTile-128F↑p
                nop
                mov     al, _mapX
                stc
                cmc
                sbb     al, [di+137h]
                call    sub_150D8
                mov     _circleDeltaX, al
                mov     al, _mapY
                stc
                cmc
                sbb     al, [di+157h]
                call    sub_150D8
                mov     _circleDeltaY, al
                retn
sub_1152E       endp

; ---------------------------------------------------------------------------
                db  90h
                db    6
                db 0B4h
                db    0
                db  8Bh
                db 0F0h
                db  8Bh
                db  3Eh ; >
                db  76h ; v
                db    4
                db  1Eh
                db    7
                db 0EBh
                db    8
                db  90h
                db    6
                db  0Eh
                db    7
                db  8Bh
                db  3Eh ; >
                db  27h ; '
                db    0
                db  26h ; &
                db  8Ah
                db    5
                db  3Ch ; <
                db    0
                db  74h ; t
                db    3
                db  47h ; G
                db 0EBh
                db 0F6h
                db  4Eh ; N
                db  74h ; t
                db    2
                db 0EBh
                db 0F8h
                db  47h ; G
                db  26h ; &
                db  8Ah
                db    5
                db  3Ch ; <
                db    0
                db  74h ; t
                db    5
                db 0E8h
                db  76h ; v
                db  3Eh ; >
                db 0EBh
                db 0F3h
                db    7
                db 0C3h
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
                db    0
                db  90h
                db 0A0h
                db    0
                db    0
                db  3Ch ; <
                db  20h
                db  72h ; r
                db  0Ah
                db 0A0h
                db    1
                db    0
                db  3Ch ; <
                db  20h
                db  72h ; r
                db  0Dh
                db 0EBh
                db  27h ; '
                db  90h
                db 0A0h
                db    1
                db    0
                db  3Ch ; <
                db  20h
                db  72h ; r
                db  11h
                db 0EBh
                db  2Bh ; +
                db  90h
                db  90h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    2
                db  75h ; u
                db    3
                db 0E9h
                db  66h ; f
                db    7
                db 0E9h
                db 0DFh
                db    1
                db  90h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    2
                db  75h ; u
                db    3
                db 0E9h
                db  8Ah
                db    8
                db 0E9h
                db  18h
                db    5
                db  90h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    2
                db  75h ; u
                db    3
                db 0E9h
                db 0A2h
                db    9
                db 0E9h
                db  6Bh ; k
                db    2
                db  90h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db    4
                db  90h
                db 0E9h
                db 0CBh
                db    3
                db  90h
                db 0E8h
                db 0F7h
                db  39h ; 9
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  50h ; P
                db  55h ; U
                db  42h ; B
                db  21h ; !
                db  0Dh
                db  31h ; 1
                db  2Dh ; -
                db  42h ; B
                db  55h ; U
                db  59h ; Y
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  54h ; T
                db  49h ; I
                db  50h ; P
                db  20h
                db  2Dh ; -
                db  2Dh ; -
                db  2Dh ; -
                db  20h
                db    0
                db 0E8h
                db  17h
                db  0Ah
                db  3Ch ; <
                db    1
                db  74h ; t
                db    7
                db  3Ch ; <
                db    2
                db  74h ; t
                db  38h ; 8
                db 0E9h
                db  17h
                db 0F4h
                db  90h
                db 0B0h
                db    5
                db 0A2h
                db  28h ; (
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  27h ; '
                db    0
                db 0E8h
                db  28h ; (
                db  0Ah
                db 0E8h
                db 0B2h
                db  39h ; 9
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  2Ch ; ,
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  41h ; A
                db  20h
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  4Fh ; O
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db  21h ; !
                db  0Dh
                db    0
                db 0E8h
                db 0CDh
                db  3Bh ; ;
                db  24h ; $
                db    3
                db 0B0h
                db    1
                db 0EBh
                db  36h ; 6
                db  90h
                db  90h
                db 0E8h
                db  8Ah
                db  39h ; 9
                db  54h ; T
                db  49h ; I
                db  50h ; P
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  43h ; C
                db  48h ; H
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0C1h
                db    9
                db 0A2h
                db  25h ; %
                db    0
                db 0A2h
                db  28h ; (
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  27h ; '
                db    0
                db 0E8h
                db 0DDh
                db    9
                db 0E8h
                db  9Fh
                db  3Bh ; ;
                db 0E8h
                db  5Dh ; ]
                db  3Ah ; :
                db 0F8h
                db  12h
                db    6
                db  25h ; %
                db    0
                db  3Ch ; <
                db  0Ah
                db  72h ; r
                db    2
                db 0B0h
                db    0
                db  90h
                db 0A2h
                db  25h ; %
                db    0
                db 0E8h
                db  52h ; R
                db  39h ; 9
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  42h ; B
                db  41h ; A
                db  52h ; R
                db  4Bh ; K
                db  45h ; E
                db  45h ; E
                db  50h ; P
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0B4h
                db    0
                db  8Bh
                db 0F0h
                db  46h ; F
                db  2Eh ; .
                db  8Dh
                db    6
                db 0BCh
                db  16h
                db 0A3h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0A4h
                db 0FEh
                db 0E9h
                db  77h ; w
                db 0F3h
                db    0
                db  43h ; C
                db  41h ; A
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db  49h ; I
                db  53h ; S
                db  4Eh ; N
                db  27h ; '
                db  54h ; T
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  53h ; S
                db  20h
                db  41h ; A
                db  20h
                db  47h ; G
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  54h ; T
                db  20h
                db  47h ; G
                db  41h ; A
                db  4Dh ; M
                db  45h ; E
                db  3Fh ; ?
                db    0
                db  48h ; H
                db  48h ; H
                db  4Dh ; M
                db  4Dh ; M
                db  4Dh ; M
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db  53h ; S
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  46h ; F
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  53h ; S
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  4Dh ; M
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  43h ; C
                db  20h
                db  48h ; H
                db  45h ; E
                db  4Ch ; L
                db  4Dh ; M
                db  53h ; S
                db  21h ; !
                db    0
                db  41h ; A
                db  56h ; V
                db  49h ; I
                db  41h ; A
                db  54h ; T
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  20h
                db  55h ; U
                db  53h ; S
                db  45h ; E
                db  20h
                db  53h ; S
                db  4Bh ; K
                db  55h ; U
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db  53h ; S
                db  21h ; !
                db    0
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  4Ch ; L
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  42h ; B
                db  4Ch ; L
                db  55h ; U
                db  45h ; E
                db  20h
                db  54h ; T
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  4Ch ; L
                db  45h ; E
                db  53h ; S
                db  21h ; !
                db    0
                db  4Dh ; M
                db  41h ; A
                db  47h ; G
                db  45h ; E
                db  53h ; S
                db  20h
                db  43h ; C
                db  41h ; A
                db  52h ; R
                db  52h ; R
                db  59h ; Y
                db  20h
                db  57h ; W
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  53h ; S
                db  20h
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  53h ; S
                db  54h ; T
                db  41h ; A
                db  46h ; F
                db  46h ; F
                db  53h ; S
                db  21h ; !
                db    0
                db  47h ; G
                db  55h ; U
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db  53h ; S
                db  20h
                db  43h ; C
                db  41h ; A
                db  52h ; R
                db  52h ; R
                db  59h ; Y
                db  20h
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db  53h ; S
                db  21h ; !
                db    0
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  48h ; H
                db  53h ; S
                db  20h
                db  4Fh ; O
                db  50h ; P
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  53h ; S
                db  50h ; P
                db  41h ; A
                db  43h ; C
                db  45h ; E
                db  21h ; !
                db    0
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  53h ; S
                db  20h
                db  4Eh ; N
                db  45h ; E
                db  45h ; E
                db  44h ; D
                db  20h
                db  42h ; B
                db  52h ; R
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  20h
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  54h ; T
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  21h ; !
                db    0
                db  90h
                db 0E8h
                db  42h ; B
                db  38h ; 8
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  4Fh ; O
                db  44h ; D
                db  20h
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  53h ; S
                db  54h ; T
                db  53h ; S
                db  20h
                db    0
                db 0B0h
                db    3
                db 0E8h
                db 0F4h
                db    8
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0E0h
                db  39h ; 9
                db 0E8h
                db  17h
                db  38h ; 8
                db  8Dh
                db  50h ; P
                db  45h ; E
                db  52h ; R
                db  20h
                db  31h ; 1
                db  30h ; 0
                db  30h ; 0
                db  2Ch ; ,
                db  20h
                db  57h ; W
                db  41h ; A
                db  4Eh ; N
                db  54h ; T
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  6Dh ; m
                db  37h ; 7
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db    9
                db 0E8h
                db 0F3h
                db  37h ; 7
                db  4Eh ; N
                db  4Fh ; O
                db    0
                db 0E9h
                db  41h ; A
                db 0F2h
                db 0E8h
                db 0EAh
                db  37h ; 7
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  2Ch ; ,
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  49h ; I
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  47h ; G
                db  4Fh ; O
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  53h ; S
                db  45h ; E
                db  21h ; !
                db  8Dh
                db    0
                db 0E8h
                db  38h ; 8
                db    8
                db 0F8h
                db 0A0h
                db  53h ; S
                db    0
                db  14h
                db    1
                db  27h ; '
                db 0A2h
                db  53h ; S
                db    0
                db 0E8h
                db 0B8h
                db  37h ; 7
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  2Ch ; ,
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  41h ; A
                db  47h ; G
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db  21h ; !
                db    0
                db 0E9h
                db 0F2h
                db 0F1h
                db  90h
                db 0A0h
                db  47h ; G
                db    0
                db  3Ch ; <
                db    1
                db  74h ; t
                db  20h
                db 0E8h
                db  2Eh ; .
                db  38h ; 8
                db 0E8h
                db  90h
                db  37h ; 7
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  41h ; A
                db  20h
                db  43h ; C
                db  4Ch ; L
                db  45h ; E
                db  52h ; R
                db  49h ; I
                db  43h ; C
                db  21h ; !
                db    0
                db 0E9h
                db 0CAh
                db 0F1h
                db  90h
                db 0E8h
                db  72h ; r
                db  37h ; 7
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  20h
                db    0
                db 0A0h
                db  46h ; F
                db    0
                db  3Ch ; <
                db    1
                db  74h ; t
                db  0Fh
                db 0E8h
                db  5Fh ; _
                db  37h ; 7
                db  42h ; B
                db  52h ; R
                db  4Fh ; O
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  20h
                db    0
                db 0EBh
                db  0Ch
                db  90h
                db 0E8h
                db  50h ; P
                db  37h ; 7
                db  53h ; S
                db  49h ; I
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  20h
                db    0
                db 0E8h
                db 0E0h
                db  37h ; 7
                db 0E8h
                db  42h ; B
                db  37h ; 7
                db  8Dh
                db  31h ; 1
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Ch ; L
                db  2Eh ; .
                db  44h ; D
                db  2Eh ; .
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  4Ch ; L
                db  2Eh ; .
                db  55h ; U
                db  2Eh ; .
                db  2Ch ; ,
                db  8Dh
                db  34h ; 4
                db  2Dh ; -
                db  50h ; P
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  20h
                db  35h ; 5
                db  2Dh ; -
                db  53h ; S
                db  55h ; U
                db  52h ; R
                db  46h ; F
                db  41h ; A
                db  43h ; C
                db  45h ; E
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  50h ; P
                db  52h ; R
                db  41h ; A
                db  59h ; Y
                db  45h ; E
                db  52h ; R
                db  2Eh ; .
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  42h ; B
                db    7
                db  74h ; t
                db    7
                db  3Ch ; <
                db    7
                db  73h ; s
                db    3
                db 0EBh
                db  21h ; !
                db  90h
                db 0E8h
                db 0EDh
                db  36h ; 6
                db  46h ; F
                db  4Fh ; O
                db  4Ch ; L
                db  4Ch ; L
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  4Ch ; L
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  46h ; F
                db  52h ; R
                db  49h ; I
                db  45h ; E
                db  4Eh ; N
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  24h ; $
                db 0F1h
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db 0CAh
                db  36h ; 6
                db  46h ; F
                db  49h ; I
                db  56h ; V
                db  45h ; E
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db  68h ; h
                db  37h ; 7
                db 0E8h
                db 0B8h
                db  36h ; 6
                db  53h ; S
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0E8h
                db  77h ; w
                db    7
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db  63h ; c
                db  38h ; 8
                db 0E8h
                db  9Ah
                db  36h ; 6
                db  8Dh
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  2Ch ; ,
                db  20h
                db  46h ; F
                db  52h ; R
                db  49h ; I
                db  45h ; E
                db  4Eh ; N
                db  44h ; D
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0F6h
                db  35h ; 5
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db  1Eh
                db 0E8h
                db  7Ch ; |
                db  36h ; 6
                db  4Eh ; N
                db  4Fh ; O
                db  8Dh
                db  49h ; I
                db  27h ; '
                db  4Dh ; M
                db  20h
                db  53h ; S
                db  4Fh ; O
                db  52h ; R
                db  52h ; R
                db  59h ; Y
                db  2Ch ; ,
                db  20h
                db  47h ; G
                db  4Fh ; O
                db  4Fh ; O
                db  44h ; D
                db  20h
                db  44h ; D
                db  41h ; A
                db  59h ; Y
                db  2Eh ; .
                db    0
                db 0E9h
                db 0B5h
                db 0F0h
                db 0E8h
                db  5Eh ; ^
                db  36h ; 6
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  2Ch ; ,
                db  8Dh
                db  49h ; I
                db  20h
                db  57h ; W
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  41h ; A
                db  4Bh ; K
                db  45h ; E
                db  20h
                db  35h ; 5
                db  21h ; !
                db    0
                db 0E8h
                db 0BAh
                db    6
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db 0F8h
                db  14h
                db    5
                db  27h ; '
                db  88h
                db  85h
                db 0B6h
                db    0
                db 0E9h
                db  84h
                db 0F0h
                db  90h
                db 0E8h
                db 0C7h
                db  36h ; 6
                db 0A0h
                db  47h ; G
                db    0
                db  3Ch ; <
                db    2
                db  74h ; t
                db  1Dh
                db 0E8h
                db  22h ; "
                db  36h ; 6
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  41h ; A
                db  20h
                db  57h ; W
                db  49h ; I
                db  5Ah ; Z
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  5Ch ; \
                db 0F0h
                db  90h
                db 0E8h
                db    4
                db  36h ; 6
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  53h ; S
                db  45h ; E
                db  52h ; R
                db  52h ; R
                db  45h ; E
                db  46h ; F
                db  20h
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  55h ; U
                db  4Dh ; M
                db  21h ; !
                db  8Dh
                db  31h ; 1
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Ch ; L
                db  2Eh ; .
                db  44h ; D
                db  2Eh ; .
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  4Ch ; L
                db  2Eh ; .
                db  55h ; U
                db  2Eh ; .
                db  2Ch ; ,
                db  8Dh
                db  34h ; 4
                db  2Dh ; -
                db  4Dh ; M
                db  2Eh ; .
                db  4Dh ; M
                db  2Eh ; .
                db  2Ch ; ,
                db  20h
                db  35h ; 5
                db  2Dh ; -
                db  42h ; B
                db  4Ch ; L
                db  49h ; I
                db  4Eh ; N
                db  4Bh ; K
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  4Bh ; K
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  2Ch ; ,
                db  8Dh
                db  42h ; B
                db  49h ; I
                db  52h ; R
                db  20h
                db  49h ; I
                db  4Bh ; K
                db  49h ; I
                db  20h
                db  55h ; U
                db  43h ; C
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0F7h
                db    5
                db  74h ; t
                db    7
                db  3Ch ; <
                db    7
                db  73h ; s
                db    3
                db 0EBh
                db  16h
                db  90h
                db 0E8h
                db 0A2h
                db  35h ; 5
                db  8Dh
                db  55h ; U
                db  47h ; G
                db  55h ; U
                db  52h ; R
                db  4Ch ; L
                db  41h ; A
                db  20h
                db  4Fh ; O
                db  4Ch ; L
                db  53h ; S
                db  55h ; U
                db  4Eh ; N
                db  21h ; !
                db    0
                db 0E9h
                db 0E4h
                db 0EFh
                db  3Ch ; <
                db    4
                db  72h ; r
                db    3
                db 0F8h
                db  14h
                db    3
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db  83h
                db  35h ; 5
                db  46h ; F
                db  49h ; I
                db  56h ; V
                db  45h ; E
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db  21h ; !
                db  36h ; 6
                db 0E8h
                db  71h ; q
                db  35h ; 5
                db  53h ; S
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0E8h
                db  30h ; 0
                db    6
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db  1Ch
                db  37h ; 7
                db 0E8h
                db  53h ; S
                db  35h ; 5
                db  8Dh
                db  4Ch ; L
                db  55h ; U
                db  54h ; T
                db  46h ; F
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  45h ; E
                db  56h ; V
                db  45h ; E
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0AFh
                db  34h ; 4
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db    9
                db 0E8h
                db  35h ; 5
                db  35h ; 5
                db  4Eh ; N
                db  4Fh ; O
                db  21h ; !
                db    0
                db 0EBh
                db  8Ah
                db 0E8h
                db  2Ch ; ,
                db  35h ; 5
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  8Dh
                db    0
                db 0E8h
                db  97h
                db    5
                db 0F8h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db  14h
                db    5
                db  27h ; '
                db  88h
                db  85h
                db 0B6h
                db    0
                db 0E8h
                db  0Dh
                db  35h ; 5
                db  47h ; G
                db  55h ; U
                db  4Ch ; L
                db  45h ; E
                db  20h
                db  47h ; G
                db  55h ; U
                db  4Ch ; L
                db  45h ; E
                db  21h ; !
                db    0
                db 0E9h
                db  53h ; S
                db 0EFh
                db  90h
                db 0E8h
                db 0FBh
                db  34h ; 4
                db  54h ; T
                db  49h ; I
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  4Ch ; L
                db  20h
                db  4Dh ; M
                db  41h ; A
                db  44h ; D
                db  45h ; E
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  57h ; W
                db  49h ; I
                db  53h ; S
                db  45h ; E
                db  2Ch ; ,
                db  0Dh
                db  50h ; P
                db  52h ; R
                db  45h ; E
                db  43h ; C
                db  49h ; I
                db  4Fh ; O
                db  55h ; U
                db  53h ; S
                db  20h
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  2Ch ; ,
                db  20h
                db  41h ; A
                db  20h
                db  43h ; C
                db  4Ch ; L
                db  55h ; U
                db  45h ; E
                db  20h
                db  49h ; I
                db  54h ; T
                db  20h
                db  42h ; B
                db  55h ; U
                db  59h ; Y
                db  53h ; S
                db  21h ; !
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  43h ; C
                db  48h ; H
                db  20h
                db  57h ; W
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  50h ; P
                db  41h ; A
                db  59h ; Y
                db  20h
                db  2Ah ; *
                db  31h ; 1
                db  30h ; 0
                db  30h ; 0
                db  3Fh ; ?
                db    0
                db 0E8h
                db 0E9h
                db    4
                db 0A2h
                db  1Fh
                db    0
                db 0A2h
                db  27h ; '
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  28h ; (
                db    0
                db 0E8h
                db    5
                db    5
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0F3h
                db  46h ; F
                db  2Eh ; .
                db  8Dh
                db    6
                db  6Ah ; j
                db  1Bh
                db 0A3h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0F6h
                db 0F9h
                db 0E9h
                db 0C9h
                db 0EEh
                db    0
                db  41h ; A
                db  53h ; S
                db  4Bh ; K
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  49h ; I
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  2Ch ; ,
                db  8Dh
                db  49h ; I
                db  27h ; '
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  4Ch ; L
                db  49h ; I
                db  45h ; E
                db  53h ; S
                db  2Eh ; .
                db    0
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  4Bh ; K
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  8Dh
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  4Bh ; K
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  41h ; A
                db  20h
                db  53h ; S
                db  50h ; P
                db  59h ; Y
                db  2Eh ; .
                db    0
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  45h ; E
                db  56h ; V
                db  49h ; I
                db  4Ch ; L
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  44h ; D
                db  4Fh ; O
                db  8Dh
                db  49h ; I
                db  53h ; S
                db  20h
                db  41h ; A
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  52h ; R
                db  52h ; R
                db  49h ; I
                db  42h ; B
                db  4Ch ; L
                db  45h ; E
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  2Eh ; .
                db    0
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  20h
                db  44h ; D
                db  49h ; I
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  41h ; A
                db  4Eh ; N
                db  53h ; S
                db  8Dh
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  53h ; S
                db  54h ; T
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  52h ; R
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  2Eh ; .
                db    0
                db  4Ah ; J
                db  55h ; U
                db  53h ; S
                db  54h ; T
                db  20h
                db  57h ; W
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  49h ; I
                db  54h ; T
                db  20h
                db  49h ; I
                db  53h ; S
                db  2Ch ; ,
                db  8Dh
                db  49h ; I
                db  20h
                db  43h ; C
                db  41h ; A
                db  4Eh ; N
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  2Eh ; .
                db    0
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  20h
                db  49h ; I
                db  27h ; '
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  52h ; R
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  57h ; W
                db  2Ch ; ,
                db  8Dh
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  4Fh ; O
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  20h
                db  57h ; W
                db  41h ; A
                db  59h ; Y
                db  2Eh ; .
                db    0
                db  49h ; I
                db  20h
                db  48h ; H
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  41h ; A
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  8Dh
                db  57h ; W
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  57h ; W
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  20h
                db  52h ; R
                db  55h ; U
                db  4Eh ; N
                db  53h ; S
                db  20h
                db  46h ; F
                db  52h ; R
                db  45h ; E
                db  45h ; E
                db  2Eh ; .
                db    0
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  20h
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  20h
                db  4Dh ; M
                db  41h ; A
                db  4Eh ; N
                db  8Dh
                db  4Ch ; L
                db  49h ; I
                db  56h ; V
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  55h ; U
                db  4Eh ; N
                db  44h ; D
                db  45h ; E
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  54h ; T
                db  52h ; R
                db  45h ; E
                db  45h ; E
                db  2Eh ; .
                db    0
                db  48h ; H
                db  45h ; E
                db  20h
                db  48h ; H
                db  41h ; A
                db  53h ; S
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  41h ; A
                db  20h
                db  4Eh ; N
                db  41h ; A
                db  4Dh ; M
                db  45h ; E
                db  8Dh
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  20h
                db  48h ; H
                db  45h ; E
                db  20h
                db  44h ; D
                db  4Fh ; O
                db  45h ; E
                db  53h ; S
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  41h ; A
                db  20h
                db  43h ; C
                db  4Ch ; L
                db  55h ; U
                db  45h ; E
                db    0
                db  4Dh ; M
                db  41h ; A
                db  59h ; Y
                db  48h ; H
                db  41h ; A
                db  50h ; P
                db  53h ; S
                db  20h
                db  49h ; I
                db  46h ; F
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  46h ; F
                db  49h ; I
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  48h ; H
                db  49h ; I
                db  4Dh ; M
                db  2Ch ; ,
                db  8Dh
                db  48h ; H
                db  45h ; E
                db  27h ; '
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  54h ; T
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  49h ; I
                db  54h ; T
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  21h ; !
                db    0
                db  90h
                db 0E8h
                db 0BEh
                db  32h ; 2
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  53h ; S
                db  48h ; H
                db  4Fh ; O
                db  50h ; P
                db  50h ; P
                db  45h ; E
                db  3Ah ; :
                db  0Dh
                db  31h ; 1
                db  2Dh ; -
                db  43h ; C
                db  4Ch ; L
                db  4Fh ; O
                db  54h ; T
                db  48h ; H
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db  2Ch ; ,
                db  0Dh
                db  34h ; 4
                db  2Dh ; -
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  2Ch ; ,
                db  20h
                db  35h ; 5
                db  2Dh ; -
                db  52h ; R
                db  45h ; E
                db  46h ; F
                db  4Ch ; L
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  50h ; P
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db  2Ch ; ,
                db  0Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0A2h
                db    2
                db  74h ; t
                db    7
                db  3Ch ; <
                db    7
                db  73h ; s
                db    3
                db 0EBh
                db  1Fh
                db  90h
                db 0E8h
                db  4Dh ; M
                db  32h ; 2
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  53h ; S
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  42h ; B
                db  59h ; Y
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db 0E9h
                db  86h
                db 0ECh
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db  2Ch ; ,
                db  32h ; 2
                db  41h ; A
                db  48h ; H
                db  21h ; !
                db  20h
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  21h ; !
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0F8h
                db  14h
                db  1Dh
                db 0E8h
                db 0C6h
                db  32h ; 2
                db 0E8h
                db  16h
                db  32h ; 2
                db  0Dh
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db    2
                db 0C0h
                db 0E8h
                db 0CBh
                db    2
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0B7h
                db  33h ; 3
                db 0E8h
                db 0EEh
                db  31h ; 1
                db  0Dh
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  27h ; '
                db  42h ; B
                db  4Fh ; O
                db  55h ; U
                db  54h ; T
                db  20h
                db  49h ; I
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  49h ; I
                db  31h ; 1
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db 0A2h
                db  21h ; !
                db    0
                db 0E8h
                db 0E5h
                db  35h ; 5
                db 0B0h
                db  8Dh
                db 0E8h
                db 0E0h
                db  35h ; 5
                db 0A0h
                db  21h ; !
                db    0
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db  10h
                db 0E8h
                db 0C1h
                db  31h ; 1
                db  4Fh ; O
                db  48h ; H
                db  2Ch ; ,
                db  20h
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  2Eh ; .
                db    0
                db 0E9h
                db    8
                db 0ECh
                db  90h
                db 0E8h
                db  23h ; #
                db    2
                db 0E8h
                db 0ADh
                db  31h ; 1
                db  53h ; S
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  21h ; !
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  96h
                db    0
                db 0F8h
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  96h
                db    0
                db 0E9h
                db 0E4h
                db 0EBh
                db  90h
                db 0E8h
                db  8Ch
                db  31h ; 1
                db  20h
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  20h
                db  53h ; S
                db  48h ; H
                db  4Fh ; O
                db  50h ; P
                db  50h ; P
                db  45h ; E
                db  3Ah ; :
                db  0Dh
                db  31h ; 1
                db  2Dh ; -
                db  44h ; D
                db  41h ; A
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Dh ; M
                db  41h ; A
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  41h ; A
                db  58h ; X
                db  2Ch ; ,
                db  20h
                db  34h ; 4
                db  2Dh ; -
                db  42h ; B
                db  4Fh ; O
                db  2Ch ; ,
                db  0Dh
                db  35h ; 5
                db  2Dh ; -
                db  53h ; S
                db  57h ; W
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  47h ; G
                db  52h ; R
                db  2Ch ; ,
                db  20h
                db  37h ; 7
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  2Ch ; ,
                db  20h
                db  38h ; 8
                db  2Dh ; -
                db  50h ; P
                db  48h ; H
                db  2Eh ; .
                db  0Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  7Ch ; |
                db    1
                db  74h ; t
                db    7
                db  3Ch ; <
                db    9
                db  74h ; t
                db    3
                db 0EBh
                db  1Fh
                db  90h
                db 0E8h
                db  27h ; '
                db  31h ; 1
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  53h ; S
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  42h ; B
                db  59h ; Y
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db 0E9h
                db  60h ; `
                db 0EBh
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db    6
                db  31h ; 1
                db  41h ; A
                db  48h ; H
                db  21h ; !
                db  20h
                db  59h ; Y
                db  45h ; E
                db  53h ; S
                db  21h ; !
                db  20h
                db  41h ; A
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0F8h
                db  14h
                db  13h
                db 0E8h
                db  9Eh
                db  31h ; 1
                db 0E8h
                db 0EEh
                db  30h ; 0
                db  0Dh
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0E8h
                db 0A5h
                db    1
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db  91h
                db  32h ; 2
                db 0E8h
                db 0C8h
                db  30h ; 0
                db  0Dh
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  27h ; '
                db  42h ; B
                db  4Fh ; O
                db  55h ; U
                db  54h ; T
                db  20h
                db  49h ; I
                db  54h ; T
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  23h ; #
                db  30h ; 0
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db 0A2h
                db  21h ; !
                db    0
                db 0E8h
                db 0BFh
                db  34h ; 4
                db 0B0h
                db  8Dh
                db 0E8h
                db 0BAh
                db  34h ; 4
                db 0A0h
                db  21h ; !
                db    0
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db  10h
                db 0E8h
                db  9Bh
                db  30h ; 0
                db  4Fh ; O
                db  48h ; H
                db  2Ch ; ,
                db  20h
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  2Eh ; .
                db    0
                db 0E9h
                db 0E2h
                db 0EAh
                db  90h
                db 0E8h
                db 0FDh
                db    0
                db 0E8h
                db  87h
                db  30h ; 0
                db  53h ; S
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  21h ; !
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db 0F8h
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  76h ; v
                db    0
                db 0E9h
                db 0BEh
                db 0EAh
                db  90h
                db 0E8h
                db  66h ; f
                db  30h ; 0
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  2Ch ; ,
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  53h ; S
                db  50h ; P
                db  4Fh ; O
                db  52h ; R
                db  54h ; T
                db  0Dh
                db  53h ; S
                db  48h ; H
                db  4Fh ; O
                db  50h ; P
                db  50h ; P
                db  45h ; E
                db  2Eh ; .
                db  20h
                db  49h ; I
                db  20h
                db  53h ; S
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db  20h
                db    0
                db 0B0h
                db    4
                db 0E8h
                db 0F9h
                db    0
                db 0A0h
                db  27h ; '
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0E5h
                db  31h ; 1
                db 0E8h
                db  1Ch
                db  30h ; 0
                db  0Dh
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  2Ch ; ,
                db  20h
                db  44h ; D
                db  45h ; E
                db  41h ; A
                db  4Ch ; L
                db  2Ch ; ,
                db  20h
                db  4Fh ; O
                db  4Bh ; K
                db  3Fh ; ?
                db  20h
                db  2Dh ; -
                db  2Dh ; -
                db  2Dh ; -
                db  20h
                db    0
                db 0E8h
                db  71h ; q
                db  2Fh ; /
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db  50h ; P
                db 0E8h
                db  0Fh
                db  34h ; 4
                db 0B0h
                db  8Dh
                db 0E8h
                db  0Ah
                db  34h ; 4
                db  58h ; X
                db  3Ch ; <
                db  59h ; Y
                db  74h ; t
                db  18h
                db 0E8h
                db 0EDh
                db  2Fh ; /
                db  0Dh
                db  4Fh ; O
                db  2Eh ; .
                db  4Bh ; K
                db  2Eh ; .
                db  20h
                db  42h ; B
                db  59h ; Y
                db  45h ; E
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  4Eh ; N
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db 0E9h
                db  2Ch ; ,
                db 0EAh
                db 0E8h
                db  48h ; H
                db    0
                db 0E8h
                db 0D2h
                db  2Fh ; /
                db  8Dh
                db  52h ; R
                db  49h ; I
                db  44h ; D
                db  45h ; E
                db  20h
                db  53h ; S
                db  57h ; W
                db  49h ; I
                db  46h ; F
                db  54h ; T
                db  4Ch ; L
                db  59h ; Y
                db  21h ; !
                db    0
                db 0B0h
                db  22h ; "
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db  0Fh
                db 0EAh
                db  90h
                db 0E9h
                db  0Bh
                db 0EAh
                db  90h
                db 0E8h
                db  21h ; !
                db  2Fh ; /
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F7h
                db  3Ch ; <
                db  30h ; 0
                db  72h ; r
                db 0F3h
                db  3Ch ; <
                db  39h ; 9
                db  77h ; w
                db 0EFh
                db 0F9h
                db 0F5h
                db  1Ch
                db  30h ; 0
                db 0F5h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db  14h
                db  30h ; 0
                db 0E8h
                db 0ACh
                db  33h ; 3
                db 0B0h
                db  8Dh
                db 0E8h
                db 0A7h
                db  33h ; 3
                db  8Bh
                db 0C7h
                db  3Ch ; <
                db    0
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_12052       proc near               ; CODE XREF: sg01a2:428B↓p
                nop
                stc
                mov     al, player._gold+1
                cmc
                sbb     al, byte_17438
                das
                cmc
                mov     player._gold+1, al
                mov     al, player._gold
                cmc
                sbb     al, _sleepFlag2?
                das
                cmc
                mov     player._gold, al
                jb      short loc_120A7
                clc
                mov     al, player._gold+1
                adc     al, byte_17438
                daa
                mov     player._gold+1, al
                mov     al, player._gold
                adc     al, _sleepFlag2?
                daa
                mov     player._gold, al
                pop     ax
                call    write_string    ; YOU DONT HAVE THAT MUCH!
; ---------------------------------------------------------------------------
aYouDontHaveTha db 'YOU DONT HAVE THAT MUCH!',0
; ---------------------------------------------------------------------------

loc_120A4:                              ; CODE XREF: sub_12052+36↑j
                jmp     loc_10A30
; ---------------------------------------------------------------------------

loc_120A7:                              ; CODE XREF: sub_12052+1C↑j
                nop
                mov     al, 0
                retn
sub_12052       endp ; sp-analysis failed

; ---------------------------------------------------------------------------
                db  90h
                db 0F8h
                db  14h
                db    8
                db 0A2h
                db  20h
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  27h ; '
                db    0
                db 0A2h
                db  29h ; )
                db    0
                db 0B0h
                db    4
                db 0A2h
                db  28h ; (
                db    0
                db 0A2h
                db  2Ah ; *
                db    0
                db 0A0h
                db  50h ; P
                db    0
                db  12h
                db    6
                db  4Eh ; N
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  46h ; F
                db 0F8h
                db 0D0h
                db 0D8h
                db  0Ah
                db 0C0h
                db  75h ; u
                db 0F8h
                db 0A0h
                db  20h
                db    0
                db  8Bh
                db 0DEh
                db  88h
                db  1Eh
                db  20h
                db    0
                db 0F9h
                db 0F5h
                db  1Ah
                db    6
                db  20h
                db    0
                db 0F5h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db 0A0h
                db  2Ah ; *
                db    0
                db  12h
                db    6
                db  28h ; (
                db    0
                db  27h ; '
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  28h ; (
                db    0
                db  8Bh
                db 0F3h
                db  8Bh
                db 0DEh
                db  88h
                db  1Eh
                db  2Ah ; *
                db    0
                db 0A2h
                db  28h ; (
                db    0
                db 0A0h
                db  29h ; )
                db    0
                db  12h
                db    6
                db  27h ; '
                db    0
                db  27h ; '
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  27h ; '
                db    0
                db  8Bh
                db 0F3h
                db  8Bh
                db 0DEh
                db  88h
                db  1Eh
                db  29h ; )
                db    0
                db 0A2h
                db  27h ; '
                db    0
                db  4Fh ; O
                db  75h ; u
                db 0CAh
                db 0C3h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_12122:                              ; CODE XREF: canMoveToTile-18ED↑j
                                        ; canMoveToTile-18D9↑j ...
                mov     al, [di+177h]
                cmp     al, 10h
                jnb     short loc_1212E
                inc     byte ptr [di+177h]

loc_1212E:                              ; CODE XREF: canMoveToTile-2FB↑j
                clc
                mov     al, [di+137h]
                adc     al, _circleDeltaX
                and     al, 3Fh
                mov     _playerX, al
                clc
                mov     al, [di+157h]
                adc     al, _circleDeltaY
                and     al, 3Fh
                mov     _playerY, al
                call    get_player_tile
                call    sub_12199
                jnz     short loc_12155
                jmp     loc_12201
; ---------------------------------------------------------------------------

loc_12155:                              ; CODE XREF: canMoveToTile-2D3↑j
                mov     al, _circleDeltaY
                cmp     al, 0
                jz      short loc_1216E
                mov     al, [di+137h]
                mov     _playerX, al
                call    get_player_tile
                call    sub_12199
                jnz     short loc_1216E
                jmp     loc_12201
; ---------------------------------------------------------------------------

loc_1216E:                              ; CODE XREF: canMoveToTile-2C9↑j
                                        ; canMoveToTile-2BA↑j
                clc
                mov     al, [di+137h]
                adc     al, _circleDeltaX
                and     al, 3Fh
                mov     _playerX, al
                mov     al, [di+157h]
                mov     _playerY, al
                call    get_player_tile
                call    sub_12199
                jz      short loc_12201

loc_1218B:                              ; CODE XREF: canMoveToTile-210↓j
                mov     al, player._mapNum2
                cmp     al, 0
                jz      short loc_12195
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_12195:                              ; CODE XREF: canMoveToTile-293↑j
                nop
                jmp     loc_10AA1
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


sub_12199       proc near               ; CODE XREF: canMoveToTile-2D6↑p
                                        ; canMoveToTile-2BD↑p ...
                mov     byte_1742F, al
                mov     al, _flag1
                cmp     al, 0
                jz      short loc_121A6
                jmp     short loc_121FC
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_121A6:                              ; CODE XREF: sub_12199+8↑j
                mov     al, [di+197h]
                cmp     al, 2Ch ; ','
                jz      short loc_121D8
                cmp     al, 48h ; 'H'
                jz      short loc_121D8
                cmp     al, 78h ; 'x'
                jnb     short loc_121BD
                mov     al, byte_1742F
                cmp     al, 5Ch ; '\'
                jz      short loc_121E2

loc_121BD:                              ; CODE XREF: sub_12199+1B↑j
                nop
                mov     al, byte_1742F
                cmp     al, 10h
                jz      short loc_121FC
                cmp     al, 4
                jz      short loc_121FC
                cmp     al, 0
                jz      short loc_121FC
                cmp     al, 14h
                jb      short loc_121E2
                cmp     al, 70h ; 'p'
                jz      short loc_121E2
                jmp     short loc_121FC
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_121D8:                              ; CODE XREF: sub_12199+13↑j
                                        ; sub_12199+17↑j
                mov     al, byte_1742F
                cmp     al, 0
                jnz     short loc_121FC
                jmp     short loc_121E2
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_121E2:                              ; CODE XREF: sub_12199+22↑j
                                        ; sub_12199+36↑j ...
                mov     al, player._mapNum2
                cmp     al, 0
                jz      short loc_121F7
                mov     al, _playerX
                cmp     al, 0
                jz      short loc_121FC
                mov     al, _playerY
                cmp     al, 0
                jz      short loc_121FC

loc_121F7:                              ; CODE XREF: sub_12199+4E↑j
                mov     al, 0
                cmp     al, 0
                retn
; ---------------------------------------------------------------------------

loc_121FC:                              ; CODE XREF: sub_12199+A↑j
                                        ; sub_12199+2A↑j ...
                mov     al, 0FFh
                cmp     al, 0
                retn
sub_12199       endp

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_12201:                              ; CODE XREF: canMoveToTile-12EC↑j
                                        ; canMoveToTile-2D1↑j ...
                mov     al, _mapX
                cmp     al, _playerX
                jnz     short loc_12216
                mov     al, _mapY
                cmp     al, _playerY
                jnz     short loc_12216
                jmp     loc_1218B
; ---------------------------------------------------------------------------

loc_12216:                              ; CODE XREF: canMoveToTile-21B↑j
                                        ; canMoveToTile-212↑j
                mov     al, _playerX
                mov     _sleepFlag2?, al
                mov     al, _playerY
                mov     byte_17438, al
                mov     al, [di+137h]
                mov     _playerX, al
                mov     al, [di+157h]
                mov     _playerY, al
                call    get_player_tile
                mov     al, [di+1B7h]
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     al, _sleepFlag2?
                mov     _playerX, al
                mov     [di+137h], al
                mov     al, byte_17438
                mov     _playerY, al
                mov     [di+157h], al
                call    get_player_tile
                mov     [di+1B7h], al
                mov     al, [di+197h]
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     al, player._mapNum2
                cmp     al, 0
                jz      short loc_12272
                jmp     loc_11040
; ---------------------------------------------------------------------------

loc_12272:                              ; CODE XREF: canMoveToTile-1B6↑j
                jmp     loc_10AA1
; ---------------------------------------------------------------------------

loc_12275:                              ; CODE XREF: canMoveToTile-1A24↑j
                                        ; canMoveToTile-1A07↑j
                call    write_string    ; -ILLEGAL COMMAND!
; ---------------------------------------------------------------------------
aIllegalCommand db '-ILLEGAL COMMAND!',0
; ---------------------------------------------------------------------------
                jmp     loc_10A30
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile
;   ADDITIONAL PARENT FUNCTION normal_movement
;   ADDITIONAL PARENT FUNCTION canMoveToTileEnd

dead2:                                  ; CODE XREF: canMoveToTile-1AF3↑j
                                        ; normal_movement+48↓j ...
                jmp     dead
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


load_map        proc near               ; CODE XREF: sg01a2:0807↑p
                                        ; canMoveToTile+2D↓p ...
                clc
                mov     al, player._mapNum1
                adc     al, 30h ; '0'
                mov     byte ptr cs:aMapxff+4, al ; "FF  "
                mov     cs:byte_122C9+4, al
                mov     al, player._mapNum2
                adc     al, 30h ; '0'
                mov     byte ptr cs:aMapxff+5, al ; "F  "
                mov     cs:byte_122C9+5, al
                mov     ah, 27h
                mov     cx, 1000h
                mov     dx, map_ptr
                call    access_file
; ---------------------------------------------------------------------------
aMapxff         db 'MAPXFF  '           ; DATA XREF: load_map+6↑w
                                        ; load_map+13↑w
; ---------------------------------------------------------------------------
                mov     cx, 100h
                lea     dx, _mapMonsters
                call    access_file
; ---------------------------------------------------------------------------
byte_122C9      db    'M',   'O',   'N',   'X',   'F',   'F',   ' ',   ' '
                                        ; DATA XREF: load_map+A↑w
                                        ; load_map+17↑w
; ---------------------------------------------------------------------------
                call    keypress_check
                retn
load_map        endp

; ---------------------------------------------------------------------------
                db 0F8h
                db 0A0h
                db  49h ; I
                db    0
                db  14h
                db  30h ; 0
                db  2Eh ; .
                db 0A2h
                db 0F8h
                db  22h ; "
                db 0A0h
                db  4Ah ; J
                db    0
                db  14h
                db  30h ; 0
                db  2Eh ; .
                db 0A2h
                db 0F9h
                db  22h ; "
                db 0B4h
                db  27h ; '
                db 0B9h
                db    0
                db    1
                db  8Bh
                db  16h
                db  76h ; v
                db    4
                db 0E8h
                db 0D6h
                db  2Fh ; /
                db  54h ; T
                db  4Ch ; L
                db  4Bh ; K
                db  58h ; X
                db  46h ; F
                db  46h ; F
                db  20h
                db  20h
                db 0C3h
; ---------------------------------------------------------------------------

save_game:                              ; CODE XREF: normal_movement+13F↓p
                mov     al, player._disableSave
                or      al, al
                jz      short save_game1
                retn
; ---------------------------------------------------------------------------

save_game1:                             ; CODE XREF: sg01a2:2302↑j
                clc
                mov     al, player._mapNum1
                adc     al, '0'
                mov     byte ptr cs:aMapxff_0+4, al ; "FF  "
                mov     byte ptr cs:aMonxff+4, al ; "FF  "
                mov     al, player._mapNum2
                adc     al, '0'
                mov     byte ptr cs:aMapxff_0+5, al ; "F  "
                mov     byte ptr cs:aMonxff+5, al ; "F  "
                mov     al, _mapX
                mov     player._mapX, al
                mov     al, _mapY
                mov     player._mapY, al
                mov     ah, 40
                mov     cx, 1000h
                mov     dx, map_ptr
                call    access_file
; ---------------------------------------------------------------------------
aMapxff_0       db 'MAPXFF  '           ; DATA XREF: sg01a2:230B↑w
                                        ; sg01a2:2318↑w
; ---------------------------------------------------------------------------
                mov     cx, 100h
                lea     dx, _mapMonsters
                call    access_file
; ---------------------------------------------------------------------------
aMonxff         db 'MONXFF  '           ; DATA XREF: sg01a2:230F↑w
                                        ; sg01a2:231C↑w
; ---------------------------------------------------------------------------
                lea     dx, player
                call    access_file
; ---------------------------------------------------------------------------
aPlayer_1       db 'PLAYER  '
; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
                mov     al, 0
                mov     byte_1742F, al
                mov     byte_17430, al
                mov     _circleDeltaX, al
                mov     _circleDeltaY, al

loc_12370:                              ; CODE XREF: sg01a2:2381↓j
                                        ; sg01a2:2387↓j ...
                call    keypress_check
                cmp     ah, 0FFh
                jz      short near ptr loc_12394+1
                nop
                nop
                nop
                nop
                nop
                inc     byte_1742F
                jnz     short loc_12370
                inc     byte_17430
                jnz     short loc_12370
                pop     ax
                call    write_string    ; PASS
; ---------------------------------------------------------------------------
aPass_1         db 'PASS',0
unk_12392       db 0E9h                 ; CODE XREF: sg01a2:238A↑j
; ---------------------------------------------------------------------------
                wait

loc_12394:                              ; CODE XREF: sg01a2:2376↑j
                out     90h, al
                cmp     al, NORTH_KEYCODE
                jz      short loc_123B0
                cmp     al, byte_1767F
                jz      short loc_123BE
                cmp     al, byte_17680
                jz      short loc_123CC
                cmp     al, byte_17681
                jz      short loc_123D9
                jmp     short loc_12370
; ---------------------------------------------------------------------------

loc_123B0:                              ; CODE XREF: sg01a2:239A↑j
                call    write_string    ; NORTH
; ---------------------------------------------------------------------------
aNorth_0        db 'NORTH',0
; ---------------------------------------------------------------------------

loc_123B9:                              ; CODE XREF: sg01a2:loc_123B0↑j
                dec     _circleDeltaY
                retn
; ---------------------------------------------------------------------------

loc_123BE:                              ; CODE XREF: sg01a2:23A0↑j
                call    write_string    ; SOUTH
; ---------------------------------------------------------------------------
aSouth_0        db 'SOUTH',0
; ---------------------------------------------------------------------------

loc_123C7:                              ; CODE XREF: sg01a2:loc_123BE↑j
                inc     _circleDeltaY
                retn
; ---------------------------------------------------------------------------

loc_123CC:                              ; CODE XREF: sg01a2:23A6↑j
                call    write_string    ; EAST
; ---------------------------------------------------------------------------
aEast_0         db 'EAST',0
; ---------------------------------------------------------------------------

loc_123D4:                              ; CODE XREF: sg01a2:loc_123CC↑j
                inc     _circleDeltaX
                retn
; ---------------------------------------------------------------------------

loc_123D9:                              ; CODE XREF: sg01a2:23AC↑j
                call    write_string    ; WEST
; ---------------------------------------------------------------------------
aWest_0         db 'WEST',0
; ---------------------------------------------------------------------------

loc_123E1:                              ; CODE XREF: sg01a2:loc_123D9↑j
                dec     _circleDeltaX
                retn
; ---------------------------------------------------------------------------
                db 0F8h
                db 0A0h
                db    0
                db    0
                db  12h
                db    6
                db  19h
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0F8h
                db 0A0h
                db    1
                db    0
                db  12h
                db    6
                db  1Ah
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  1Fh
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  97h
                db    1
                db  3Ch ; <
                db    0
                db  74h ; t
                db  15h
                db  8Ah
                db  85h
                db  37h ; 7
                db    1
                db  3Ah ; :
                db    6
                db  23h ; #
                db    0
                db  75h ; u
                db  0Bh
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db  3Ah ; :
                db    6
                db  24h ; $
                db    0
                db  75h ; u
                db    1
                db 0C3h
                db  4Fh ; O
                db  75h ; u
                db 0E0h
                db 0C3h

; =============== S U B R O U T I N E =======================================


canMoveToTile   proc near               ; CODE XREF: canMoveToTile-1ABE↑p
                                        ; canMoveToTile-1A97↑p ...

; FUNCTION CHUNK AT 086F SIZE 00000106 BYTES
; FUNCTION CHUNK AT 0976 SIZE 00000023 BYTES
; FUNCTION CHUNK AT 099A SIZE 00000002 BYTES
; FUNCTION CHUNK AT 099D SIZE 00000022 BYTES
; FUNCTION CHUNK AT 09C0 SIZE 00000002 BYTES
; FUNCTION CHUNK AT 09C3 SIZE 00000022 BYTES
; FUNCTION CHUNK AT 09E6 SIZE 00000002 BYTES
; FUNCTION CHUNK AT 09E9 SIZE 0000000A BYTES
; FUNCTION CHUNK AT 09F4 SIZE 00000023 BYTES
; FUNCTION CHUNK AT 0A18 SIZE 000000C8 BYTES
; FUNCTION CHUNK AT 0AE1 SIZE 00000009 BYTES
; FUNCTION CHUNK AT 0AEB SIZE 00000009 BYTES
; FUNCTION CHUNK AT 0AF5 SIZE 0000016D BYTES
; FUNCTION CHUNK AT 0C62 SIZE 0000003A BYTES
; FUNCTION CHUNK AT 0C9C SIZE 0000003C BYTES
; FUNCTION CHUNK AT 0CD9 SIZE 0000002F BYTES
; FUNCTION CHUNK AT 0D09 SIZE 00000007 BYTES
; FUNCTION CHUNK AT 0D11 SIZE 00000007 BYTES
; FUNCTION CHUNK AT 0D19 SIZE 00000035 BYTES
; FUNCTION CHUNK AT 0D4F SIZE 0000001F BYTES
; FUNCTION CHUNK AT 0D6F SIZE 0000001F BYTES
; FUNCTION CHUNK AT 0D8F SIZE 0000001F BYTES
; FUNCTION CHUNK AT 0DAF SIZE 0000001E BYTES
; FUNCTION CHUNK AT 0DCE SIZE 00000002 BYTES
; FUNCTION CHUNK AT 0DD1 SIZE 00000002 BYTES
; FUNCTION CHUNK AT 0DD4 SIZE 0000009C BYTES
; FUNCTION CHUNK AT 1013 SIZE 0000002C BYTES
; FUNCTION CHUNK AT 1040 SIZE 000000FA BYTES
; FUNCTION CHUNK AT 115E SIZE 0000017D BYTES
; FUNCTION CHUNK AT 1308 SIZE 00000027 BYTES
; FUNCTION CHUNK AT 1330 SIZE 000000A2 BYTES
; FUNCTION CHUNK AT 13D7 SIZE 00000065 BYTES
; FUNCTION CHUNK AT 2122 SIZE 00000077 BYTES
; FUNCTION CHUNK AT 2201 SIZE 0000008C BYTES
; FUNCTION CHUNK AT 228D SIZE 00000003 BYTES
; FUNCTION CHUNK AT 25FB SIZE 00000004 BYTES
; FUNCTION CHUNK AT 2604 SIZE 00000002 BYTES
; FUNCTION CHUNK AT 2607 SIZE 0000001F BYTES
; FUNCTION CHUNK AT 27C0 SIZE 0000007B BYTES
; FUNCTION CHUNK AT 283C SIZE 0000018C BYTES

                mov     points_to_distrubte, al
                mov     al, player._mapNum2
                cmp     al, 0
                jz      short within_map
                mov     al, _mapX
                cmp     al, 64
                jnb     short loc_1243E
                mov     al, _mapY
                cmp     al, 64
                jnb     short loc_1243E
                jmp     short within_map
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1243E:                              ; CODE XREF: canMoveToTile+F↑j
                                        ; canMoveToTile+16↑j
                nop
                mov     al, player._mapX
                mov     _mapX, al
                mov     al, player._mapY
                mov     _mapY, al
                mov     al, 0
                mov     player._mapNum2, al
                call    load_map
                mov     al, player._disableSave
                cmp     al, 0
                jz      short loc_12474
                mov     al, player.field_34
                mov     _playerX, al
                mov     al, player.field_35
                mov     _playerY, al
                call    get_player_tile
                mov     al, 80
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12474:                              ; CODE XREF: canMoveToTile+35↑j
                mov     ah, 40
                mov     cx, 100h
                lea     dx, player
                call    access_file
                push    ax
                dec     sp
                inc     cx
                pop     cx
                inc     bp
                push    dx
                and     [bx+si], ah
                jmp     loc_10A33
; ---------------------------------------------------------------------------

within_map:                             ; CODE XREF: canMoveToTile+8↑j
                                        ; canMoveToTile+18↑j
                nop
                mov     al, points_to_distrubte
                and     al, 7Fh
                cmp     al, 2
                jnz     short loc_124B1
                stc
                mov     al, player._hp+1
                cmc
                sbb     al, 5
                das
                cmc
                mov     player._hp+1, al
                mov     al, player._hp
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._hp, al
                jb      short loc_124B1
                jmp     dead
; ---------------------------------------------------------------------------

loc_124B1:                              ; CODE XREF: canMoveToTile+70↑j
                                        ; canMoveToTile+89↑j
                cmp     al, 46
                jnz     short check_paralyzed
                mov     al, player._hasRing
                cmp     al, 0
                jz      short loc_124E0
                call    write_string
; ---------------------------------------------------------------------------
                db 8Dh,'RING PROTECTS FROM FIELD!',0
; ---------------------------------------------------------------------------
                call    pause?
                jmp     short check_paralyzed
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_124E0:                              ; CODE XREF: canMoveToTile+97↑j
                call    write_string
; ---------------------------------------------------------------------------
                db 8Dh,'FIELD CAUSES 1000 DAMAGE!',0
; ---------------------------------------------------------------------------
                mov     al, 0
                mov     _circleDeltaX, al
                mov     _circleDeltaY, al
                call    xorDrawCircle
                call    pause?
                call    xorDrawCircle
                mov     al, player._hp
                stc
                cmc
                sbb     al, 10h
                das
                cmc
                mov     player._hp, al
                jb      short check_paralyzed
                jmp     dead
; ---------------------------------------------------------------------------

check_paralyzed:                        ; CODE XREF: canMoveToTile+90↑j
                                        ; canMoveToTile+BA↑j ...
                mov     al, player_paralyzedFlag
                cmp     al, 0
                jz      short loc_1253B
                call    write_string    ; --PARALIZED!
; ---------------------------------------------------------------------------
aParalized      db '--PARALIZED!',0Dh,0
; ---------------------------------------------------------------------------
                mov     al, 0FFh
                retn
; ---------------------------------------------------------------------------

loc_1253B:                              ; CODE XREF: canMoveToTile+102↑j
                mov     al, _playerTileId
                cmp     al, 28h ; '('
                jnz     short loc_12545
                jmp     rocket_return
; ---------------------------------------------------------------------------

loc_12545:                              ; CODE XREF: canMoveToTile+11D↑j
                cmp     al, 38
                jnz     short loc_1254C
                jmp     loc_12607
; ---------------------------------------------------------------------------

loc_1254C:                              ; CODE XREF: canMoveToTile+124↑j
                cmp     al, 36
                jnz     short loc_12553
                jmp     ship
; ---------------------------------------------------------------------------

loc_12553:                              ; CODE XREF: canMoveToTile+12B↑j
                cmp     al, 34
                jnz     short normal_movement
                call    sub_15EC7
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx

loc_12560:                              ; CODE XREF: canMoveToTile+14D↓j
                mov     bh, 0
                mov     bl, 20h ; ' '
                mov     si, bx

loc_12566:                              ; CODE XREF: canMoveToTile+144↓j
                dec     si
                jnz     short loc_12566
                mov     bx, 10h
                call    delayFrames
                dec     di
                jnz     short loc_12560
                jmp     canMoveToTileEnd
canMoveToTile   endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================


normal_movement proc near               ; CODE XREF: canMoveToTile+132↑j
                                        ; canMoveToTileEnd:loc_1264E↓p

; FUNCTION CHUNK AT 228D SIZE 00000003 BYTES
; FUNCTION CHUNK AT 2604 SIZE 00000002 BYTES
; FUNCTION CHUNK AT 2664 SIZE 00000095 BYTES

                call    sub_15EC7
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx

loc_1257E:                              ; CODE XREF: normal_movement+19↓j
                mov     bh, 0
                mov     bl, 20h ; ' '
                mov     si, bx

loc_12584:                              ; CODE XREF: normal_movement+10↓j
                dec     si
                jnz     short loc_12584
                mov     bx, 10h
                call    delayFrames
                dec     di
                jnz     short loc_1257E
                call    sub_15EC7
                mov     bx, 12h
                call    delayFrames
                stc
                mov     al, player._foodTurnCtr
                cmc
                sbb     al, 25
                das
                cmc
                mov     player._foodTurnCtr, al
                mov     al, player._food+1
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food+1, al
                mov     al, player._food
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food, al
                jb      short loc_125C0
                jmp     dead2
; ---------------------------------------------------------------------------

loc_125C0:                              ; CODE XREF: normal_movement+46↑j
                mov     al, points_to_distrubte
                and     al, 7Fh
                cmp     al, 60h ; '`'
                jnz     short loc_125CC
                jmp     loc_12677
; ---------------------------------------------------------------------------

loc_125CC:                              ; CODE XREF: normal_movement+52↑j
                cmp     al, 0
                jz      short return_true
                cmp     al, 8
                jz      short return_true
                cmp     al, 22h ; '"'
                jz      short update_map_xy_
                cmp     al, 24h ; '$'
                jz      short update_map_xy_
                cmp     al, 26h ; '&'
                jz      short update_map_xy_
                cmp     al, 28h ; '('
                jz      short update_map_xy_
                cmp     al, 2Ah ; '*'
                jz      short update_map_xy_
                cmp     al, 2Ch ; ','
                jz      short update_map_xy_
                cmp     al, 2Eh ; '.'
                jz      short update_map_xy_
                cmp     al, 38h ; '8'
                jz      short update_map_xy_
                cmp     al, 16h
                jnb     short return_true
                jmp     short update_map_xy_
normal_movement endp ; sp-analysis failed

; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

rocket_return:                          ; CODE XREF: canMoveToTile+11F↑j
                call    return_true
                retn
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


return_true     proc near               ; CODE XREF: normal_movement+59↑j
                                        ; normal_movement+5D↑j ...
                mov     al, 0FFh
                cmp     al, 0
                retn
return_true     endp

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR normal_movement
;   ADDITIONAL PARENT FUNCTION canMoveToTile
;   ADDITIONAL PARENT FUNCTION canMoveToTileEnd

update_map_xy_:                         ; CODE XREF: normal_movement+61↑j
                                        ; normal_movement+65↑j ...
                jmp     short update_map_xy
; END OF FUNCTION CHUNK FOR normal_movement
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_12607:                              ; CODE XREF: canMoveToTile+126↑j
                call    canMoveToTileEnd
                cmp     al, 0FFh
                jz      short return_true
                mov     al, points_to_distrubte
                and     al, 7Fh
                cmp     al, 6
                jz      short return_true
                jmp     short update_map_xy_
; ---------------------------------------------------------------------------

ship:                                   ; CODE XREF: canMoveToTile+12D↑j
                mov     al, points_to_distrubte
                cmp     al, 0
                jz      short update_map_xy_
                cmp     al, 128
                jz      short update_map_xy_
                jmp     short return_true
; END OF FUNCTION CHUNK FOR canMoveToTile

; =============== S U B R O U T I N E =======================================


canMoveToTileEnd proc near              ; CODE XREF: canMoveToTile+14F↑j
                                        ; canMoveToTile:loc_12607↑p

; FUNCTION CHUNK AT 228D SIZE 00000003 BYTES
; FUNCTION CHUNK AT 2604 SIZE 00000002 BYTES

                nop
                stc
                mov     al, player._foodTurnCtr
                cmc
                sbb     al, 25
                das
                cmc
                mov     player._foodTurnCtr, al
                mov     al, player._food+1
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food+1, al
                mov     al, player._food
                cmc
                sbb     al, 0
                das
                cmc
                mov     player._food, al
                jb      short loc_1264E
                jmp     dead2
; ---------------------------------------------------------------------------

loc_1264E:                              ; CODE XREF: canMoveToTileEnd+23↑j
                call    normal_movement
                cmp     al, 0FFh
                jz      short return_true
                mov     al, points_to_distrubte
                and     al, 7Fh
                cmp     al, 2
                jz      short return_true
                cmp     al, 96
                jz      short return_true
                jmp     short update_map_xy_
canMoveToTileEnd endp

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR normal_movement

update_map_xy:                          ; CODE XREF: normal_movement:update_map_xy_↑j
                mov     al, _mapX
                and     al, 63
                mov     _mapX, al
                mov     al, _mapY
                and     al, 63
                mov     _mapY, al
                mov     al, 0
                retn
; ---------------------------------------------------------------------------

loc_12677:                              ; CODE XREF: normal_movement+54↑j
                mov     al, _playerTileId
                mov     byte_1742F, al
                mov     al, _mapX
                mov     _playerX, al
                mov     al, _mapY
                mov     _playerY, al
                call    get_player_tile
                mov     al, byte ptr player+3Ah
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                clc
                clc
                rcr     al, 1
                mov     _playerTileId, al
                call    draw_map
                mov     al, byte_1742F
                mov     _playerTileId, al
                call    sub_15FA6
                mov     al, _mapX
                mov     player._mapX, al
                mov     al, _mapY
                mov     player._mapY, al
                call    save_game
                mov     al, player.field_38
                clc
                rcr     al, 1
                cmp     al, player._mapNum1
                jb      short loc_126C6
                clc
                adc     al, 1

loc_126C6:                              ; CODE XREF: normal_movement+14C↑j
                mov     player._mapNum1, al
                call    load_map
                clc
                mov     al, player._mapNum1
                add     al, al
                add     al, al
                add     al, al
                adc     al, player.field_38
                mov     ah, 0
                mov     di, ax
                mov     al, cs:[di+2798h]
                mov     _mapX, al
                inc     di
                mov     al, cs:[di+2798h]
                mov     _mapY, al
                call    draw_map
                call    sub_15FA6
                pop     ax
                jmp     loc_10A33
; END OF FUNCTION CHUNK FOR normal_movement

; =============== S U B R O U T I N E =======================================


sub_126F9       proc near               ; CODE XREF: canMoveToTile-17F3↑p
                mov     al, player._disableSave
                cmp     al, 0
                jz      short loc_12701
                retn
; ---------------------------------------------------------------------------

loc_12701:                              ; CODE XREF: sub_126F9+5↑j
                dec     byte ptr player+39h
                jz      short loc_12708
                retn
; ---------------------------------------------------------------------------

loc_12708:                              ; CODE XREF: sub_126F9+C↑j
                clc
                mov     al, player._mapNum1
                add     al, al
                add     al, al
                add     al, al
                mov     byte_1742F, al
                adc     al, player.field_38
                mov     ah, 0
                mov     di, ax
                mov     al, cs:[di+2798h]
                mov     _playerX, al
                inc     di
                mov     al, cs:[di+2798h]
                mov     _playerY, al
                call    get_player_tile
                cmp     al, 0C0h
                jnz     short loc_12744
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     al, byte ptr player+3Ah
                mov     bx, current_tile_ptr
                mov     [bx+si], al

loc_12744:                              ; CODE XREF: sub_126F9+3A↑j
                mov     al, 8
                mov     byte ptr player+39h, al
                inc     player.field_38
                inc     player.field_38
                mov     al, player.field_38
                and     al, 7
                mov     player.field_38, al
                clc
                adc     al, byte_1742F
                mov     ah, 0
                mov     di, ax
                mov     al, cs:[di+2798h]
                mov     _playerX, al
                inc     di
                mov     al, cs:[di+2798h]
                mov     _playerY, al
                call    get_player_tile
                cmp     al, 14h
                jb      short loc_1277B
                retn
; ---------------------------------------------------------------------------

loc_1277B:                              ; CODE XREF: sub_126F9+7F↑j
                mov     byte ptr player+3Ah, al
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     al, 0C0h
                mov     bx, current_tile_ptr
                mov     [bx+si], al
                mov     al, _playerX
                mov     bh, 0
                mov     bl, _playerY
                mov     di, bx
                retn
sub_126F9       endp

; ---------------------------------------------------------------------------
                db  1Dh
                db  38h ; 8
                db  1Fh
                db  38h ; 8
                db  21h ; !
                db  38h ; 8
                db  23h ; #
                db  38h ; 8
                db  22h ; "
                db    8
                db  2Fh ; /
                db  1Ch
                db  24h ; $
                db  38h ; 8
                db  14h
                db  25h ; %
                db  10h
                db  10h
                db  30h ; 0
                db  18h
                db  2Ah ; *
                db  18h
                db  13h
                db  34h ; 4
                db  38h ; 8
                db  34h ; 4
                db  1Ch
                db  0Ch
                db  21h ; !
                db  17h
                db  14h
                db  34h ; 4
                db  12h
                db  17h
                db  32h ; 2
                db  22h ; "
                db  34h ; 4
                db  18h
                db    8
                db  0Bh
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

dungeon_render:                         ; CODE XREF: canMoveToTile-1BA9↑j
                nop
                mov     al, _sleepFlag
                or      al, al
                jz      short loc_127F1
                call    write_string
; ---------------------------------------------------------------------------
aCmd_0          db 'CMD: ',0
; ---------------------------------------------------------------------------
                mov     al, 14h
; ---------------------------------------------------------------------------
                db 0A2h
                db  27h ; '
; ---------------------------------------------------------------------------

loc_127D5:                              ; CODE XREF: canMoveToTile+3C5↓j
                add     al, ch
                push    es
                sub     [bp+si+0], bl
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx

loc_127E1:                              ; CODE XREF: canMoveToTile+3BF↓j
                dec     di
                jnz     short loc_127E1
                dec     _sleepFlag2?
                jnz     short near ptr loc_127D5+1
                dec     _sleepFlag
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_127F1:                              ; CODE XREF: canMoveToTile+3A3↑j
                nop
                call    write_string
; ---------------------------------------------------------------------------
aCmd_1          db 'CMD: ',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 0FFh
                mov     di, bx
                mov     bh, 0
                mov     bl, 2
; ---------------------------------------------------------------------------
                db  8Bh
; ---------------------------------------------------------------------------

loc_12806:                              ; CODE XREF: canMoveToTile+3FF↓j
                                        ; canMoveToTile+402↓j
                rep mov bx, 0
                call    delayFrames
                call    keypress_check
                cmp     ah, 0FFh
                jz      short loc_12832
                push    ax
                pop     ax
                push    ax
                pop     ax
                push    ax
                pop     ax
                push    ax
                pop     ax
                push    ax
                pop     ax
                push    ax
                pop     ax
                dec     di
                jnz     short near ptr loc_12806+1
                dec     si
                jnz     short near ptr loc_12806+1
                call    write_string    ; PASS
; ---------------------------------------------------------------------------
aPass_2         db 'PASS',0
; ---------------------------------------------------------------------------
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12832:                              ; CODE XREF: canMoveToTile+3F0↑j
                nop
                cmp     al, NORTH_KEYCODE
                jnz     short loc_1283C
                jmp     short loc_12861
; END OF FUNCTION CHUNK FOR canMoveToTile
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR canMoveToTile

loc_1283C:                              ; CODE XREF: canMoveToTile+414↑j
                cmp     al, byte_17681
                jnz     short loc_12845
                jmp     loc_12912
; ---------------------------------------------------------------------------

loc_12845:                              ; CODE XREF: canMoveToTile+41D↑j
                cmp     al, byte_17680
                jnz     short loc_1284E
                jmp     loc_12944
; ---------------------------------------------------------------------------

loc_1284E:                              ; CODE XREF: canMoveToTile+426↑j
                cmp     al, byte_1767F
                jnz     short loc_12857
                jmp     loc_12977
; ---------------------------------------------------------------------------

loc_12857:                              ; CODE XREF: canMoveToTile+42F↑j
                cmp     al, 20h ; ' '
                jnz     short loc_1285E
                jmp     loc_129BC
; ---------------------------------------------------------------------------

loc_1285E:                              ; CODE XREF: canMoveToTile+436↑j
                jmp     loc_109FB
; ---------------------------------------------------------------------------

loc_12861:                              ; CODE XREF: canMoveToTile+416↑j
                nop
                call    write_string    ; ADVANCE
; ---------------------------------------------------------------------------
aAdvance        db 'ADVANCE',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerUp
                and     al, 0Fh
                jz      short loc_12884

loc_12874:                              ; CODE XREF: canMoveToTile+466↓j
                call    write_string    ; -BLOCKED!
; ---------------------------------------------------------------------------
aBlocked        db '-BLOCKED!',0
; ---------------------------------------------------------------------------
                jmp     loc_10A30
; ---------------------------------------------------------------------------

loc_12884:                              ; CODE XREF: canMoveToTile+44F↑j
                mov     al, _tilePlayerUp
                cmp     al, 80h
                jz      short loc_12874
                clc
                mov     al, _mapX
                adc     al, _mapLeft
                and     al, 3Fh
                mov     _mapX, al
                clc
                mov     al, _mapY
                adc     al, _mapTop
                and     al, 3Fh
                mov     _mapY, al
                call    sub_15217
                cmp     al, byte_17435
                jb      short loc_128B1
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_128B1:                              ; CODE XREF: canMoveToTile+489↑j
                nop
                call    sub_14B4E
                mov     al, 0
                mov     byte_17436, al
                call    write_string
; ---------------------------------------------------------------------------
                db 8Dh,'ARGH! A TRAP!',8Dh,0
; ---------------------------------------------------------------------------
                call    sub_15E3B
                call    pause?
                call    pause?
                call    pause?
                call    pause?
                mov     al, byte ptr player+30h
                or      al, al
                jnz     short loc_128E6
                jmp     dead
; ---------------------------------------------------------------------------

loc_128E6:                              ; CODE XREF: canMoveToTile+4BE↑j
                call    write_string    ; ESCAPED! BY USE OF TOOLS!
; ---------------------------------------------------------------------------
aEscapedByUseOf db 'ESCAPED! BY USE OF TOOLS!',0
; ---------------------------------------------------------------------------
                stc
                mov     al, byte ptr player+30h
                cmc
                sbb     al, 1
                das
                cmc
                mov     byte ptr player+30h, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12912:                              ; CODE XREF: canMoveToTile+41F↑j
                nop
                call    write_string    ; TURN LEFT
; ---------------------------------------------------------------------------
aTurnLeft       db 'TURN LEFT',0
; ---------------------------------------------------------------------------
                mov     al, _mapTop
                cmp     al, 0
                jz      short loc_12932
                mov     _mapLeft, al
                mov     al, 0
                mov     _mapTop, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12932:                              ; CODE XREF: canMoveToTile+502↑j
                stc
                cmc
                sbb     al, _mapLeft
                cmc
                mov     _mapTop, al
                mov     al, 0
                mov     _mapLeft, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12944:                              ; CODE XREF: canMoveToTile+428↑j
                nop
                call    write_string    ; TURN RIGHT
; ---------------------------------------------------------------------------
aTurnRight      db 'TURN RIGHT',0
; ---------------------------------------------------------------------------
                mov     al, _mapLeft
                or      al, al
                jz      short loc_12965
                mov     _mapTop, al
                mov     al, 0
                mov     _mapLeft, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12965:                              ; CODE XREF: canMoveToTile+535↑j
                stc
                cmc
                sbb     al, _mapTop
                cmc
                mov     _mapLeft, al
                mov     al, 0
                mov     _mapTop, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_12977:                              ; CODE XREF: canMoveToTile+431↑j
                nop
                call    write_string    ; RETREAT
; ---------------------------------------------------------------------------
aRetreat        db 'RETREAT',0
; ---------------------------------------------------------------------------
                mov     al, _tilePlayerDown
                and     al, 8Fh
                jz      short loc_1299A
                call    write_string    ; -BLOCKED!
; ---------------------------------------------------------------------------
aBlocked_0      db '-BLOCKED!',0
; ---------------------------------------------------------------------------
                jmp     loc_10A30
; ---------------------------------------------------------------------------

loc_1299A:                              ; CODE XREF: canMoveToTile+565↑j
                nop
                mov     al, _mapX
                stc
                cmc
                sbb     al, _mapLeft
                cmc
                and     al, 3Fh
                mov     _mapX, al
                mov     al, _mapY
                stc
                cmc
                sbb     al, _mapTop
                cmc
                and     al, 3Fh
                mov     _mapY, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------

loc_129BC:                              ; CODE XREF: canMoveToTile+438↑j
                call    write_string    ; PASS
; ---------------------------------------------------------------------------
aPass_3         db 'PASS',0
; ---------------------------------------------------------------------------
                jmp     loc_10A33
; ---------------------------------------------------------------------------
                db    0
; END OF FUNCTION CHUNK FOR canMoveToTile
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
                db  90h
                db 0B0h
                db    0
                db  2Eh ; .
                db 0A2h
                db 0D0h
                db  29h ; )
                db  2Eh ; .
                db 0A2h
                db 0D1h
                db  29h ; )
                db 0A2h
                db  19h
                db    0
                db 0A2h
                db  1Ah
                db    0
                db 0E8h
                db  68h ; h
                db  21h ; !
                db  2Eh ; .
                db 0A0h
                db 0D0h
                db  29h ; )
                db 0F8h
                db  12h
                db    6
                db  19h
                db    0
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db  23h ; #
                db    0
                db  2Eh ; .
                db 0A0h
                db 0D1h
                db  29h ; )
                db 0F8h
                db  12h
                db    6
                db  1Ah
                db    0
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db 0AEh
                db  26h ; &
                db  75h ; u
                db    3
                db 0E9h
                db  3Ch ; <
                db    1
                db  3Ch ; <
                db  10h
                db  74h ; t
                db  21h ; !
                db  3Ch ; <
                db  78h ; x
                db  72h ; r
                db    4
                db  3Ch ; <
                db 0F0h
                db  72h ; r
                db  19h
                db  3Ch ; <
                db    8
                db  74h ; t
                db  6Fh ; o
                db  3Ch ; <
                db  0Ch
                db  74h ; t
                db  4Dh ; M
                db  3Ch ; <
                db    4
                db  74h ; t
                db    7
                db  3Ch ; <
                db  70h ; p
                db  74h ; t
                db    6
                db 0E9h
                db 0E1h
                db    0
                db 0EBh
                db  7Fh ; 
                db  90h
                db 0E9h
                db  9Ch
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  27h ; '
                db    1
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db  18h
                db    1
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db    9
                db    1
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db 0FAh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db 0EBh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    3
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db 0DCh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db 0CDh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    3
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db 0BEh
                db    0
                db 0E9h
                db  9Ch
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db 0ACh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    3
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  9Dh
                db    0
                db 0EBh
                db  7Ch ; |
                db  90h
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  8Bh
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db  7Ch ; |
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  6Dh ; m
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db  5Eh ; ^
                db    0
                db 0EBh
                db  3Dh ; =
                db  90h
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  4Ch ; L
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db 0E8h
                db  3Dh ; =
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db  2Eh ; .
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0F3h
                db 0E8h
                db  1Fh
                db    0
                db 0FEh
                db    6
                db  19h
                db    0
                db 0A0h
                db  19h
                db    0
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db  19h
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db    3
                db 0E9h
                db  8Dh
                db 0FEh
                db 0FEh
                db    6
                db  1Ah
                db    0
                db 0A0h
                db  1Ah
                db    0
                db  3Ch ; <
                db  40h ; @
                db  72h ; r
                db 0F2h
                db 0C3h
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  79h ; y
                db    4
                db  8Bh
                db 0DEh
                db  88h
                db  1Eh
                db  7Ah ; z
                db    4
                db 0A0h
                db  19h
                db    0
                db    2
                db 0C0h
                db    2
                db 0C0h
                db  12h
                db    6
                db  79h ; y
                db    4
                db 0A2h
                db  79h ; y
                db    4
                db 0A0h
                db  1Ah
                db    0
                db    2
                db 0C0h
                db  12h
                db    6
                db  7Ah ; z
                db    4
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db 0E8h
                db  1Fh
                db 0C3h
                db 0E8h
                db  4Dh ; M
                db  24h ; $
                db  41h ; A
                db  54h ; T
                db  54h ; T
                db  41h ; A
                db  43h ; C
                db  4Bh ; K
                db  2Dh ; -
                db  2Dh ; -
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  72h ; r
                db    3
                db 0E9h
                db  71h ; q
                db    2
                db  90h
                db 0A0h
                db  6Ch ; l
                db    2
                db  0Ah
                db 0C0h
                db  74h ; t
                db  11h
                db 0E8h
                db  2Fh ; /
                db  24h ; $
                db  50h ; P
                db  41h ; A
                db  52h ; R
                db  41h ; A
                db  4Ch ; L
                db  49h ; I
                db  5Ah ; Z
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  72h ; r
                db 0DEh
                db 0E8h
                db 0A1h
                db 0F7h
                db 0E8h
                db  1Ch
                db  34h ; 4
                db 0E8h
                db  1Fh
                db 0F8h
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db    0
                db  75h ; u
                db    3
                db 0E9h
                db  32h ; 2
                db    2
                db 0E8h
                db  3Dh ; =
                db  26h ; &
                db 0F8h
                db 0D0h
                db 0D8h
                db  3Ah ; :
                db    6
                db  4Ch ; L
                db    0
                db  72h ; r
                db    3
                db 0E9h
                db  23h ; #
                db    2
                db 0E8h
                db  55h ; U
                db  26h ; &
                db 0E8h
                db  4Eh ; N
                db 0E5h
                db 0E8h
                db 0F0h
                db  23h ; #
                db  2Dh ; -
                db  2Dh ; -
                db  48h ; H
                db  49h ; I
                db  54h ; T
                db  21h ; !
                db  21h ; !
                db  21h ; !
                db    0
                db 0E8h
                db 0DCh
                db  32h ; 2
                db 0A0h
                db  61h ; a
                db    0
                db    2
                db 0C0h
                db    2
                db 0C0h
                db    2
                db 0C0h
                db  12h
                db    6
                db  4Bh ; K
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0A2h
                db  1Fh
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0FBh
                db 0F9h
                db  8Ah
                db  85h
                db  77h ; w
                db    1
                db 0F5h
                db  1Ah
                db    6
                db  1Fh
                db    0
                db 0F5h
                db  73h ; s
                db  7Eh ; ~
                db  88h
                db  85h
                db  77h ; w
                db    1
                db 0E8h
                db  11h
                db  26h ; &
                db  8Ah
                db  85h
                db  97h
                db    1
                db  3Ch ; <
                db  40h ; @
                db  74h ; t
                db    3
                db 0E9h
                db 0FBh
                db 0DDh
                db 0E8h
                db 0EBh
                db  1Eh
                db 0E8h
                db  68h ; h
                db  33h ; 3
                db 0E8h
                db 0E5h
                db  1Eh
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  37h ; 7
                db    1
                db 0A2h
                db  23h ; #
                db    0
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  59h ; Y
                db  24h ; $
                db  8Ah
                db  85h
                db 0B7h
                db    1
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0A0h
                db  23h ; #
                db    0
                db  88h
                db  85h
                db  57h ; W
                db    1
                db 0A0h
                db  24h ; $
                db    0
                db  88h
                db  85h
                db  37h ; 7
                db    1
                db 0A2h
                db  23h ; #
                db    0
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  34h ; 4
                db  24h ; $
                db  8Ah
                db  85h
                db  97h
                db    1
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0B0h
                db  84h
                db  88h
                db  85h
                db 0D7h
                db    1
                db 0E8h
                db  4Dh ; M
                db  23h ; #
                db  8Dh
                db  53h ; S
                db  48h ; H
                db  45h ; E
                db  27h ; '
                db  53h ; S
                db  20h
                db  47h ; G
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db  21h ; !
                db  21h ; !
                db  21h ; !
                db    0
                db 0E9h
                db  8Fh
                db 0DDh
                db  8Ah
                db  85h
                db  37h ; 7
                db    1
                db 0A2h
                db  23h ; #
                db    0
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db 0FEh
                db  23h ; #
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  8Ah
                db  85h
                db 0B7h
                db    1
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db  8Ah
                db  85h
                db  97h
                db    1
                db  3Ch ; <
                db  40h ; @
                db  75h ; u
                db    6
                db 0E8h
                db 0D0h
                db  45h ; E
                db 0E9h
                db  60h ; `
                db 0DDh
                db  90h
                db  3Ch ; <
                db  60h ; `
                db  75h ; u
                db  0Dh
                db 0F8h
                db 0A0h
                db  65h ; e
                db    0
                db  14h
                db    2
                db  27h ; '
                db 0A2h
                db  65h ; e
                db    0
                db 0E9h
                db 0A7h
                db    0
                db  3Ch ; <
                db 0FCh
                db  75h ; u
                db  39h ; 9
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db 0E8h
                db  25h ; %
                db  25h ; %
                db  3Ch ; <
                db  40h ; @
                db  73h ; s
                db  0Ah
                db 0F8h
                db 0A0h
                db  66h ; f
                db    0
                db  14h
                db    1
                db  27h ; '
                db 0A2h
                db  66h ; f
                db    0
                db 0E8h
                db  14h
                db  25h ; %
                db  24h ; $
                db  0Fh
                db  74h ; t
                db  10h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db  8Ah
                db  85h
                db 0D6h
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db 0D6h
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db 0EBh
                db  6Bh ; k
                db  90h
                db  3Ch ; <
                db 0F0h
                db  75h ; u
                db  3Ah ; :
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db 0E8h
                db 0E8h
                db  24h ; $
                db  3Ch ; <
                db  40h ; @
                db  73h ; s
                db  0Ah
                db 0F8h
                db 0A0h
                db 0DBh
                db    0
                db  14h
                db    1
                db  27h ; '
                db 0A2h
                db 0DBh
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db 0CFh
                db  24h ; $
                db  24h ; $
                db    3
                db  14h
                db    1
                db 0F8h
                db  12h
                db    6
                db  64h ; d
                db    0
                db  27h ; '
                db 0A2h
                db  64h ; d
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db 0EBh
                db  2Dh ; -
                db  90h
                db  3Ch ; <
                db 0F8h
                db  75h ; u
                db  27h ; '
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db 0E8h
                db 0AAh
                db  24h ; $
                db  24h ; $
                db    1
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db  47h ; G
                db 0F8h
                db  8Ah
                db  85h
                db 0D6h
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db 0D6h
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db 0EBh
                db    2
                db  90h
                db  90h
                db 0B0h
                db    0
                db  88h
                db  85h
                db 0B7h
                db    1
                db  88h
                db  85h
                db  77h ; w
                db    1
                db  88h
                db  85h
                db  37h ; 7
                db    1
                db  88h
                db  85h
                db  57h ; W
                db    1
                db  88h
                db  85h
                db  97h
                db    1
                db 0E8h
                db  3Ah ; :
                db  22h ; "
                db  0Dh
                db  4Bh ; K
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  45h ; E
                db  44h ; D
                db  2Dh ; -
                db  2Dh ; -
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  2Bh ; +
                db    0
                db 0E8h
                db  60h ; `
                db  24h ; $
                db  24h ; $
                db  17h
                db  0Ch
                db    1
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db 0F4h
                db  25h ; %
                db 0F8h
                db 0A0h
                db  59h ; Y
                db    0
                db  12h
                db    6
                db  1Fh
                db    0
                db  27h ; '
                db 0A2h
                db  59h ; Y
                db    0
                db 0A0h
                db  58h ; X
                db    0
                db  14h
                db    0
                db  27h ; '
                db 0A2h
                db  58h ; X
                db    0
                db 0E8h
                db    6
                db  22h ; "
                db  2Dh ; -
                db  2Dh ; -
                db  45h ; E
                db  58h ; X
                db  50h ; P
                db  2Eh ; .
                db  2Bh ; +
                db    0
                db 0E8h
                db  33h ; 3
                db  24h ; $
                db  24h ; $
                db    3
                db  14h
                db    1
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db 0C7h
                db  25h ; %
                db 0F8h
                db 0A0h
                db  57h ; W
                db    0
                db  12h
                db    6
                db  1Fh
                db    0
                db  27h ; '
                db 0A2h
                db  57h ; W
                db    0
                db 0A0h
                db  56h ; V
                db    0
                db  14h
                db    0
                db  27h ; '
                db 0A2h
                db  56h ; V
                db    0
                db 0E8h
                db  38h ; 8
                db  24h ; $
                db 0E9h
                db  2Ah ; *
                db 0DCh
                db 0E8h
                db 0D3h
                db  21h ; !
                db  2Dh ; -
                db  2Dh ; -
                db  4Dh ; M
                db  49h ; I
                db  53h ; S
                db  53h ; S
                db    0
                db 0E9h
                db  1Dh
                db 0DCh
                db  90h
                db 0E8h
                db 0C6h
                db  31h ; 1
                db 0A0h
                db    0
                db    0
                db 0F8h
                db  12h
                db    6
                db    2
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0F8h
                db  12h
                db    6
                db    3
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0E8h
                db  1Bh
                db 0E6h
                db  24h ; $
                db    7
                db  75h ; u
                db    3
                db 0E9h
                db  27h ; '
                db    1
                db 0B7h
                db    0
                db 0B3h
                db  1Fh
                db  8Bh
                db 0FBh
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db  8Ah
                db  85h
                db  37h ; 7
                db    1
                db  3Ah ; :
                db    6
                db  23h ; #
                db    0
                db  75h ; u
                db  17h
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db  3Ah ; :
                db    6
                db  24h ; $
                db    0
                db  75h ; u
                db  0Dh
                db  8Ah
                db  85h
                db 0B7h
                db    1
                db  3Ah ; :
                db    6
                db  25h ; %
                db    0
                db  75h ; u
                db    3
                db 0EBh
                db  14h
                db  90h
                db 0FEh
                db  0Eh
                db  1Fh
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  0Ah
                db 0C0h
                db  75h ; u
                db 0CFh
                db 0E9h
                db 0E7h
                db    0
                db  90h
                db 0E8h
                db  96h
                db  23h ; #
                db 0F8h
                db 0D0h
                db 0D8h
                db  3Ah ; :
                db    6
                db  4Ch ; L
                db    0
                db  72h ; r
                db    3
                db 0E9h
                db 0D7h
                db    0
                db  90h
                db 0E8h
                db  4Eh ; N
                db  21h ; !
                db  48h ; H
                db  49h ; I
                db  54h ; T
                db  21h ; !
                db    0
                db 0E8h
                db 0A5h
                db  23h ; #
                db 0E8h
                db  3Bh ; ;
                db  30h ; 0
                db 0E8h
                db  9Fh
                db  23h ; #
                db 0A0h
                db  61h ; a
                db    0
                db    2
                db 0C0h
                db    2
                db 0C0h
                db    2
                db 0C0h
                db  12h
                db    6
                db  4Bh ; K
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db 0A2h
                db  20h
                db    0
                db  8Ah
                db  85h
                db  77h ; w
                db    1
                db 0F9h
                db 0F5h
                db  1Ah
                db    6
                db  20h
                db    0
                db 0F5h
                db  73h ; s
                db    7
                db  88h
                db  85h
                db  77h ; w
                db    1
                db 0E9h
                db  62h ; b
                db 0DBh
                db  90h
                db 0B0h
                db    0
                db  88h
                db  85h
                db  77h ; w
                db    1
                db  88h
                db  85h
                db  97h
                db    1
                db  88h
                db  85h
                db 0B7h
                db    1
                db  8Ah
                db  85h
                db  37h ; 7
                db    1
                db 0A2h
                db  23h ; #
                db    0
                db  8Ah
                db  85h
                db  57h ; W
                db    1
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0E8h
                db  5Dh ; ]
                db 0E5h
                db  8Bh
                db  1Eh
                db    8
                db    0
                db  8Ah
                db    0
                db  24h ; $
                db 0F0h
                db  8Bh
                db  1Eh
                db    8
                db    0
                db  88h
                db    0
                db 0E8h
                db 0DAh
                db  20h
                db  8Dh
                db  4Bh ; K
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  45h ; E
                db  44h ; D
                db  2Dh ; -
                db  2Dh ; -
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  2Bh ; +
                db    0
                db 0E8h
                db    0
                db  23h ; #
                db  24h ; $
                db  17h
                db  0Ch
                db    1
                db 0A2h
                db  20h
                db    0
                db 0E8h
                db  94h
                db  24h ; $
                db 0F8h
                db 0A0h
                db  59h ; Y
                db    0
                db  12h
                db    6
                db  20h
                db    0
                db  27h ; '
                db 0A2h
                db  59h ; Y
                db    0
                db 0A0h
                db  58h ; X
                db    0
                db  14h
                db    0
                db  27h ; '
                db 0A2h
                db  58h ; X
                db    0
                db 0E8h
                db 0A6h
                db  20h
                db  2Dh ; -
                db  2Dh ; -
                db  45h ; E
                db  58h ; X
                db  50h ; P
                db  2Eh ; .
                db  2Bh ; +
                db    0
                db 0E8h
                db 0D3h
                db  22h ; "
                db  24h ; $
                db    7
                db 0A2h
                db  20h
                db    0
                db 0E8h
                db  69h ; i
                db  24h ; $
                db 0F8h
                db 0A0h
                db  57h ; W
                db    0
                db  12h
                db    6
                db  20h
                db    0
                db  27h ; '
                db 0A2h
                db  57h ; W
                db    0
                db 0A0h
                db  56h ; V
                db    0
                db  14h
                db    0
                db  27h ; '
                db 0A2h
                db  56h ; V
                db    0
                db 0E9h
                db 0CFh
                db 0DAh
                db 0E8h
                db  78h ; x
                db  20h
                db  4Dh ; M
                db  49h ; I
                db  53h ; S
                db  53h ; S
                db    0
                db 0E9h
                db 0C4h
                db 0DAh
                db 0E8h
                db  6Dh ; m
                db  20h
                db  42h ; B
                db  4Fh ; O
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  72h ; r
                db  17h
                db 0E8h
                db  5Dh ; ]
                db  20h
                db  8Dh
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  4Eh ; N
                db  4Bh ; K
                db  20h
                db  41h ; A
                db  47h ; G
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db  20h
                db    0
                db 0E8h
                db 0E7h
                db  20h
                db 0E9h
                db  9Ah
                db 0DAh
                db  90h
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  78h ; x
                db  72h ; r
                db 0E1h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db    6
                db  21h ; !
                db 0A0h
                db  14h
                db    0
                db  3Ch ; <
                db  22h ; "
                db  74h ; t
                db  22h ; "
                db  3Ch ; <
                db  24h ; $
                db  74h ; t
                db  15h
                db  3Ch ; <
                db  26h ; &
                db  74h ; t
                db  14h
                db  3Ch ; <
                db  28h ; (
                db  74h ; t
                db  13h
                db 0E8h
                db  1Ch
                db  20h
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  63h ; c
                db 0DAh
                db 0EBh
                db  27h ; '
                db  90h
                db 0E9h
                db  86h
                db    0
                db 0E9h
                db 0D3h
                db    0
                db 0E8h
                db    6
                db  20h
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db    0
                db 0B0h
                db    8
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db  8Bh
                db  9Dh
                db    6
                db    0
                db  88h
                db    7
                db 0B0h
                db  22h ; "
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db  3Dh ; =
                db 0DAh
                db  90h
                db 0A0h
                db 0E2h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  3Ah ; :
                db 0E8h
                db 0DEh
                db  1Fh
                db  20h
                db  53h ; S
                db  48h ; H
                db  49h ; I
                db  50h ; P
                db  8Dh
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  43h ; C
                db  52h ; R
                db  45h ; E
                db  57h ; W
                db  20h
                db  4Fh ; O
                db  46h ; F
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  53h ; S
                db  20h
                db  53h ; S
                db  48h ; H
                db  49h ; I
                db  50h ; P
                db  8Dh
                db  57h ; W
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  4Ch ; L
                db  45h ; E
                db  54h ; T
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  42h ; B
                db  4Fh ; O
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db 0F8h
                db 0D9h
                db  90h
                db 0E8h
                db 0A3h
                db  1Fh
                db  20h
                db  46h ; F
                db  52h ; R
                db  49h ; I
                db  47h ; G
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db    0
                db 0B0h
                db    0
                db 0B4h
                db    0
                db  8Bh
                db 0F0h
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0B0h
                db  24h ; $
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db 0DAh
                db 0D9h
                db  90h
                db 0A0h
                db 0DFh
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  27h ; '
                db 0E8h
                db  7Bh ; {
                db  1Fh
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  8Dh
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  43h ; C
                db  41h ; A
                db  4Eh ; N
                db  27h ; '
                db  54h ; T
                db  20h
                db  47h ; G
                db  45h ; E
                db  54h ; T
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  21h ; !
                db    0
                db 0E9h
                db 0A8h
                db 0D9h
                db  90h
                db 0E8h
                db  53h ; S
                db  1Fh
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db    0
                db 0B0h
                db    8
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0B0h
                db  26h ; &
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db  8Ah
                db 0D9h
                db  90h
                db 0E8h
                db  32h ; 2
                db  1Fh
                db  20h
                db  52h ; R
                db  4Fh ; O
                db  43h ; C
                db  4Bh ; K
                db  45h ; E
                db  54h ; T
                db    0
                db 0A0h
                db 0DDh
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  38h ; 8
                db 0E8h
                db  20h
                db  1Fh
                db  8Dh
                db  41h ; A
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  54h ; T
                db  41h ; A
                db  4Ch ; L
                db  49h ; I
                db  43h ; C
                db  20h
                db  56h ; V
                db  4Fh ; O
                db  49h ; I
                db  43h ; C
                db  45h ; E
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  4Dh ; M
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  53h ; S
                db  3Ah ; :
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  53h ; S
                db  54h ; T
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  48h ; H
                db  21h ; !
                db    0
                db 0E9h
                db  3Ch ; <
                db 0D9h
                db  90h
                db 0B0h
                db    8
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0B0h
                db  28h ; (
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db  28h ; (
                db 0D9h
                db 0E8h
                db 0D1h
                db  1Eh
                db  43h ; C
                db  41h ; A
                db  53h ; S
                db  54h ; T
                db  2Dh ; -
                db    0
                db 0EBh
                db  3Dh ; =
                db  90h
                db 0B8h
                db    0
                db 0B8h
                db 0E8h
                db  1Ch
                db    0
                db 0B8h
                db    0
                db 0BAh
                db 0E8h
                db  16h
                db    0
                db 0B4h
                db  28h ; (
                db 0B9h
                db    0
                db  40h ; @
                db 0BAh
                db    0
                db    0
                db 0E8h
                db  9Ch
                db  21h ; !
                db  50h ; P
                db  49h ; I
                db  43h ; C
                db  58h ; X
                db  58h ; X
                db  58h ; X
                db  20h
                db  20h
                db 0E9h
                db 0FAh
                db 0D8h
                db  90h
                db  53h ; S
                db    6
                db  8Eh
                db 0C0h
                db 0B8h
                db    0
                db    0
                db 0BBh
                db    0
                db  19h
                db  26h ; &
                db  89h
                db    7
                db  83h
                db 0C3h
                db    2
                db  81h
                db 0FBh
                db  40h ; @
                db  1Fh
                db  75h ; u
                db 0F4h
                db    7
                db  5Bh ; [
                db 0C3h
                db  90h
                db 0A0h
                db  63h ; c
                db    0
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db  2Fh ; /
                db  1Fh
                db 0A0h
                db 0D7h
                db    0
                db 0F8h
                db  12h
                db    6
                db 0D8h
                db    0
                db  75h ; u
                db  1Eh
                db 0E8h
                db  75h ; u
                db  1Eh
                db  8Dh
                db  4Eh ; N
                db  45h ; E
                db  45h ; E
                db  44h ; D
                db  20h
                db  57h ; W
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  53h ; S
                db  54h ; T
                db  41h ; A
                db  46h ; F
                db  46h ; F
                db  21h ; !
                db    0
                db 0E8h
                db    1
                db  2Dh ; -
                db 0E9h
                db 0AEh
                db 0D8h
                db  90h
                db 0A0h
                db  63h ; c
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db    3
                db 0E9h
                db 0A3h
                db 0D8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  14h
                db 0E8h
                db  40h ; @
                db  1Eh
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  21h ; !
                db    0
                db 0E8h
                db 0D6h
                db  2Ch ; ,
                db 0E9h
                db  83h
                db 0D8h
                db 0E8h
                db  10h
                db  2Eh ; .
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  63h ; c
                db    0
                db  8Bh
                db 0FBh
                db 0F9h
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db  88h
                db  85h
                db 0B6h
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  73h ; s
                db  12h
                db 0E8h
                db  0Ch
                db  1Eh
                db  2Dh ; -
                db  46h ; F
                db  41h ; A
                db  49h ; I
                db  4Ch ; L
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E8h
                db 0A4h
                db  2Ch ; ,
                db 0E9h
                db  51h ; Q
                db 0D8h
                db  90h
                db 0A0h
                db  63h ; c
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db    3
                db 0E9h
                db  46h ; F
                db 0D8h
                db  3Ch ; <
                db    1
                db  74h ; t
                db  2Bh ; +
                db  3Ch ; <
                db    2
                db  74h ; t
                db  2Fh ; /
                db  3Ch ; <
                db    3
                db  74h ; t
                db  5Ah ; Z
                db  3Ch ; <
                db    4
                db  74h ; t
                db  75h ; u
                db  3Ch ; <
                db    5
                db  74h ; t
                db  0Fh
                db  3Ch ; <
                db    6
                db  74h ; t
                db  0Eh
                db  3Ch ; <
                db    7
                db  74h ; t
                db  0Dh
                db  3Ch ; <
                db    8
                db  75h ; u
                db  0Ch
                db 0E9h
                db 0C2h
                db    0
                db 0E9h
                db  8Fh
                db    0
                db 0E9h
                db  8Fh
                db    0
                db 0E9h
                db  94h
                db    0
                db 0E9h
                db 0E5h
                db    0
                db 0B0h
                db  96h
                db 0A2h
                db  26h ; &
                db    0
                db 0E9h
                db  0Fh
                db 0D8h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  74h ; t
                db  2Fh ; /
                db 0A0h
                db  25h ; %
                db    0
                db  3Ch ; <
                db  0Fh
                db  72h ; r
                db  12h
                db 0E8h
                db 0AAh
                db  1Dh
                db  2Dh ; -
                db  46h ; F
                db  41h ; A
                db  49h ; I
                db  4Ch ; L
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E8h
                db  42h ; B
                db  2Ch ; ,
                db 0E9h
                db 0EFh
                db 0D7h
                db 0F8h
                db  14h
                db    1
                db 0E8h
                db    7
                db 0E2h
                db  75h ; u
                db 0E6h
                db 0FEh
                db    6
                db  25h ; %
                db    0
                db 0E9h
                db 0D7h
                db    5
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  74h ; t
                db 0D1h
                db 0A0h
                db  25h ; %
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  41h ; A
                db 0F9h
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db 0E8h
                db 0E8h
                db 0E1h
                db  75h ; u
                db 0C7h
                db 0FEh
                db  0Eh
                db  25h ; %
                db    0
                db 0E9h
                db 0B8h
                db    5
                db 0F8h
                db 0A0h
                db    0
                db    0
                db  12h
                db    6
                db    2
                db    0
                db  24h ; $
                db  0Fh
                db  74h ; t
                db 0B4h
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db  12h
                db    6
                db    3
                db    0
                db  24h ; $
                db  0Fh
                db  74h ; t
                db 0A6h
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0E8h
                db 0BCh
                db 0E1h
                db  79h ; y
                db  9Bh
                db 0B0h
                db    0
                db  8Bh
                db  1Eh
                db    8
                db    0
                db  88h
                db    0
                db 0E9h
                db  91h
                db 0D7h
                db 0E9h
                db  9Fh
                db    5
                db 0E8h
                db  6Fh ; o
                db  1Fh
                db  78h ; x
                db  88h
                db 0EBh
                db  55h ; U
                db  90h
                db 0E8h
                db  5Ch ; \
                db    0
                db  74h ; t
                db  80h
                db 0A0h
                db  56h ; V
                db    0
                db    2
                db 0C0h
                db  14h
                db  1Eh
                db 0A2h
                db  20h
                db    0
                db  8Ah
                db  85h
                db  77h ; w
                db    1
                db 0F5h
                db  1Ah
                db    6
                db  20h
                db    0
                db 0F5h
                db  88h
                db  85h
                db  77h ; w
                db    1
                db  72h ; r
                db    3
                db 0E9h
                db    2
                db 0FCh
                db 0E9h
                db  61h ; a
                db 0D7h
                db 0E8h
                db  42h ; B
                db  1Fh
                db  24h ; $
                db  0Fh
                db  0Ch
                db    1
                db 0A2h
                db  23h ; #
                db    0
                db 0E8h
                db  38h ; 8
                db  1Fh
                db  24h ; $
                db  0Fh
                db  0Ch
                db    1
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0E8h
                db  65h ; e
                db 0E1h
                db  74h ; t
                db    3
                db 0E9h
                db  41h ; A
                db 0FFh
                db  90h
                db 0A0h
                db  23h ; #
                db    0
                db 0A2h
                db    0
                db    0
                db 0A0h
                db  24h ; $
                db    0
                db 0A2h
                db    1
                db    0
                db 0E9h
                db  32h ; 2
                db 0D7h
                db 0E8h
                db    8
                db    0
                db  75h ; u
                db    3
                db 0E9h
                db  29h ; )
                db 0FFh
                db 0E9h
                db 0C5h
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  1Fh
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  97h
                db    1
                db  0Ah
                db 0C0h
                db  75h ; u
                db    8
                db  4Fh ; O
                db  75h ; u
                db 0F5h
                db 0B0h
                db    0
                db  0Ah
                db 0C0h
                db 0C3h
                db  8Ah
                db  85h
                db 0B7h
                db    1
                db  3Ah ; :
                db    6
                db  25h ; %
                db    0
                db  75h ; u
                db 0EEh
                db 0A0h
                db    0
                db    0
                db 0F8h
                db  12h
                db    6
                db    2
                db    0
                db  3Ah ; :
                db  85h
                db  37h ; 7
                db    1
                db  75h ; u
                db 0E0h
                db 0A0h
                db    1
                db    0
                db 0F8h
                db  12h
                db    6
                db    3
                db    0
                db  3Ah ; :
                db  85h
                db  57h ; W
                db    1
                db  75h ; u
                db 0D2h
                db  8Bh
                db 0C7h
                db  0Ah
                db 0C0h
                db 0C3h
                db 0E8h
                db  8Fh
                db  1Ch
                db  44h ; D
                db  45h ; E
                db  53h ; S
                db  43h ; C
                db  45h ; E
                db  4Eh ; N
                db  44h ; D
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    5
                db  74h ; t
                db  11h
                db  3Ch ; <
                db    4
                db  74h ; t
                db  0Dh
                db 0E8h
                db  79h ; y
                db  1Ch
                db  2Dh ; -
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db 0C0h
                db 0D6h
                db 0A0h
                db  14h
                db    0
                db  24h ; $
                db  20h
                db  74h ; t
                db 0ECh
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  75h ; u
                db    3
                db 0E9h
                db 0A2h
                db    4
                db  90h
                db 0FEh
                db    6
                db  25h ; %
                db    0
                db 0E9h
                db 0A1h
                db    4
                db 0E8h
                db  53h ; S
                db  1Ch
                db  45h ; E
                db  4Eh ; N
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  0Dh
                db 0E8h
                db  43h ; C
                db  1Ch
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  8Dh
                db 0D6h
                db  90h
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  78h ; x
                db  73h ; s
                db  15h
                db 0E8h
                db  2Eh ; .
                db  1Ch
                db  2Dh ; -
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  4Fh ; O
                db  54h ; T
                db  21h ; !
                db    0
                db 0E9h
                db  6Dh ; m
                db 0D6h
                db  90h
                db 0A0h
                db  14h
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F9h
                db 0F5h
                db  1Ch
                db    5
                db 0F5h
                db  75h ; u
                db    3
                db 0EBh
                db  3Ch ; <
                db  90h
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db  75h ; u
                db    3
                db 0EBh
                db  6Bh ; k
                db  90h
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db  75h ; u
                db    3
                db 0E9h
                db  96h
                db    0
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db  75h ; u
                db    3
                db 0E9h
                db 0C6h
                db    0
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db  75h ; u
                db    3
                db 0E9h
                db 0F4h
                db    0
                db 0F5h
                db  1Ch
                db    1
                db 0F5h
                db  75h ; u
                db    3
                db 0E9h
                db  24h ; $
                db    1
                db  90h
                db 0E8h
                db 0DAh
                db  1Bh
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  24h ; $
                db 0D6h
                db 0E8h
                db 0CDh
                db  1Bh
                db  2Dh ; -
                db  56h ; V
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  41h ; A
                db  47h ; G
                db  45h ; E
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  5Ah ; Z
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  5Bh ; [
                db    0
                db 0E8h
                db 0D3h
                db 0EEh
                db 0B0h
                db  1Fh
                db 0A2h
                db    0
                db    0
                db 0B0h
                db  3Eh ; >
                db 0A2h
                db    1
                db    0
                db 0B0h
                db    1
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db  54h ; T
                db 0EEh
                db 0E8h
                db  96h
                db 0EEh
                db 0B0h
                db    4
                db 0A2h
                db  12h
                db    0
                db 0E9h
                db 0ECh
                db 0D5h
                db 0E8h
                db  95h
                db  1Bh
                db  2Dh ; -
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  5Ah ; Z
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  5Bh ; [
                db    0
                db 0E8h
                db  9Eh
                db 0EEh
                db 0B0h
                db  1Fh
                db 0A2h
                db    0
                db    0
                db 0B0h
                db  3Eh ; >
                db 0A2h
                db    1
                db    0
                db 0B0h
                db    2
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db  1Fh
                db 0EEh
                db 0E8h
                db  61h ; a
                db 0EEh
                db 0B0h
                db    4
                db 0A2h
                db  12h
                db    0
                db 0E9h
                db 0B7h
                db 0D5h
                db 0E8h
                db  60h ; `
                db  1Bh
                db  2Dh ; -
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  5Ah ; Z
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  5Bh ; [
                db    0
                db 0E8h
                db  68h ; h
                db 0EEh
                db 0B0h
                db    0
                db 0A2h
                db  25h ; %
                db    0
                db 0A2h
                db    3
                db    0
                db 0B0h
                db    1
                db 0A2h
                db    2
                db    0
                db 0B0h
                db    5
                db 0A2h
                db    0
                db    0
                db 0A2h
                db    1
                db    0
                db 0B0h
                db    4
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db 0DEh
                db 0EDh
                db 0E9h
                db  7Eh ; ~
                db 0D5h
                db 0E8h
                db  27h ; '
                db  1Bh
                db  2Dh ; -
                db  43h ; C
                db  41h ; A
                db  53h ; S
                db  54h ; T
                db  4Ch ; L
                db  45h ; E
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  5Ah ; Z
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  5Bh ; [
                db    0
                db 0E8h
                db  2Eh ; .
                db 0EEh
                db 0B0h
                db  1Fh
                db 0A2h
                db    0
                db    0
                db 0B0h
                db  3Eh ; >
                db 0A2h
                db    1
                db    0
                db 0B0h
                db    3
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db 0AFh
                db 0EDh
                db 0E8h
                db 0F1h
                db 0EDh
                db 0B0h
                db    4
                db 0A2h
                db  12h
                db    0
                db 0E9h
                db  47h ; G
                db 0D5h
                db 0E8h
                db 0F0h
                db  1Ah
                db  2Dh ; -
                db  44h ; D
                db  55h ; U
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  4Fh ; O
                db  4Eh ; N
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  5Ah ; Z
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  5Bh ; [
                db    0
                db 0E8h
                db 0F6h
                db 0EDh
                db 0B0h
                db    0
                db 0A2h
                db  25h ; %
                db    0
                db 0A2h
                db    3
                db    0
                db 0B0h
                db    1
                db 0A2h
                db    2
                db    0
                db 0B0h
                db    5
                db 0A2h
                db    0
                db    0
                db 0A2h
                db    1
                db    0
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db  6Eh ; n
                db 0EDh
                db 0E9h
                db  0Eh
                db 0D5h
                db 0E8h
                db 0B7h
                db  1Ah
                db  2Dh ; -
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  53h ; S
                db  49h ; I
                db  47h ; G
                db  4Eh ; N
                db  20h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  49h ; I
                db    0
                db  8Bh
                db 0FBh
                db  0Bh
                db 0FFh
                db  74h ; t
                db  1Fh
                db  4Fh ; O
                db  74h ; t
                db  31h ; 1
                db  4Fh ; O
                db  74h ; t
                db  49h ; I
                db  4Fh ; O
                db  74h ; t
                db  5Ch ; \
                db 0E8h
                db  8Dh
                db  1Ah
                db  41h ; A
                db  4Eh ; N
                db  4Fh ; O
                db  53h ; S
                db  3Ah ; :
                db  20h
                db  32h ; 2
                db  31h ; 1
                db  31h ; 1
                db  32h ; 2
                db  20h
                db  41h ; A
                db  2Eh ; .
                db  44h ; D
                db  2Eh ; .
                db    0
                db 0E9h
                db 0CEh
                db 0D4h
                db 0E8h
                db  77h ; w
                db  1Ah
                db  41h ; A
                db  4Eh ; N
                db  4Fh ; O
                db  53h ; S
                db  3Ah ; :
                db  20h
                db  4Ch ; L
                db  45h ; E
                db  47h ; G
                db  45h ; E
                db  4Eh ; N
                db  44h ; D
                db  53h ; S
                db  21h ; !
                db    0
                db 0E9h
                db 0B9h
                db 0D4h
                db 0E8h
                db  62h ; b
                db  1Ah
                db  41h ; A
                db  4Eh ; N
                db  4Fh ; O
                db  53h ; S
                db  3Ah ; :
                db  20h
                db  39h ; 9
                db  2Ch ; ,
                db  30h ; 0
                db  30h ; 0
                db  30h ; 0
                db  2Ch ; ,
                db  30h ; 0
                db  30h ; 0
                db  30h ; 0
                db  20h
                db  42h ; B
                db  2Eh ; .
                db  43h ; C
                db  2Eh ; .
                db    0
                db 0E9h
                db  9Eh
                db 0D4h
                db 0E8h
                db  47h ; G
                db  1Ah
                db  41h ; A
                db  4Eh ; N
                db  4Fh ; O
                db  53h ; S
                db  3Ah ; :
                db  20h
                db  31h ; 1
                db  34h ; 4
                db  32h ; 2
                db  33h ; 3
                db  20h
                db  42h ; B
                db  2Eh ; .
                db  43h ; C
                db  2Eh ; .
                db    0
                db 0E9h
                db  88h
                db 0D4h
                db 0E8h
                db  31h ; 1
                db  1Ah
                db  41h ; A
                db  4Eh ; N
                db  4Fh ; O
                db  53h ; S
                db  3Ah ; :
                db  20h
                db  31h ; 1
                db  39h ; 9
                db  39h ; 9
                db  30h ; 0
                db  20h
                db  41h ; A
                db  2Eh ; .
                db  44h ; D
                db  2Eh ; .
                db    0
                db 0E9h
                db  72h ; r
                db 0D4h
                db 0E8h
                db  1Bh
                db  1Ah
                db  46h ; F
                db  49h ; I
                db  52h ; R
                db  45h ; E
                db    0
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  24h ; $
                db  74h ; t
                db  0Dh
                db 0E8h
                db  0Ch
                db  1Ah
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  53h ; S
                db 0D4h
                db  90h
                db 0E8h
                db 0FEh
                db  19h
                db  20h
                db  44h ; D
                db  49h ; I
                db  52h ; R
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  2Dh ; -
                db    0
                db 0E8h
                db  75h ; u
                db 0EDh
                db 0E8h
                db 0BCh
                db  28h ; (
                db 0E8h
                db 0F3h
                db 0EDh
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db    0
                db  75h ; u
                db    3
                db 0E9h
                db    6
                db 0F8h
                db 0E8h
                db  11h
                db  1Ch
                db  0Ch
                db  20h
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db  30h ; 0
                db  1Ch
                db 0E8h
                db 0C6h
                db  28h ; (
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0FBh
                db 0E9h
                db 0FDh
                db 0F5h
                db  80h
                db  3Eh ; >
                db  4Ah ; J
                db    0
                db    4
                db  73h ; s
                db  17h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  81h
                db  1Ah
                db 0B0h
                db    8
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0C3h
                db 0E8h
                db 0A1h
                db  19h
                db  47h ; G
                db  45h ; E
                db  54h ; T
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  72h ; r
                db    3
                db 0EBh
                db  78h ; x
                db  90h
                db  90h
                db 0A0h
                db  14h
                db    0
                db  3Ch ; <
                db  2Ah ; *
                db  74h ; t
                db  47h ; G
                db  3Ch ; <
                db  2Ch ; ,
                db  74h ; t
                db  0Dh
                db 0E8h
                db  84h
                db  19h
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db 0CEh
                db 0D3h
                db 0E8h
                db  77h ; w
                db  19h
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db    0
                db 0E8h
                db 0A9h
                db 0FFh
                db 0E8h
                db 0A1h
                db  1Bh
                db  24h ; $
                db    7
                db  74h ; t
                db  13h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  76h ; v
                db    0
                db 0E9h
                db 0A6h
                db 0D3h
                db 0E8h
                db  4Fh ; O
                db  19h
                db  20h
                db  45h ; E
                db  4Dh ; M
                db  50h ; P
                db  54h ; T
                db  59h ; Y
                db  21h ; !
                db    0
                db 0E9h
                db  98h
                db 0D3h
                db 0E8h
                db  41h ; A
                db  19h
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db    0
                db 0E8h
                db  73h ; s
                db 0FFh
                db 0E8h
                db  6Bh ; k
                db  1Bh
                db  24h ; $
                db    3
                db  74h ; t
                db 0DDh
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db  8Ah
                db  85h
                db  96h
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  96h
                db    0
                db 0E9h
                db  70h ; p
                db 0D3h
                db  90h
                db 0A0h
                db  14h
                db    0
                db  3Ch ; <
                db  40h ; @
                db  74h ; t
                db  0Dh
                db 0E8h
                db  11h
                db  19h
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  5Bh ; [
                db 0D3h
                db  90h
                db 0E8h
                db    3
                db  19h
                db  20h
                db  43h ; C
                db  48h ; H
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  21h ; !
                db  8Dh
                db  49h ; I
                db  54h ; T
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Eh ; N
                db  54h ; T
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db  53h ; S
                db  20h
                db    0
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db 0E8h
                db  4Eh ; N
                db 0DDh
                db 0B0h
                db    0
                db  8Bh
                db  1Eh
                db    8
                db    0
                db  88h
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db  3Ch ; <
                db  0Fh
                db  74h ; t
                db  48h ; H
                db 0E8h
                db    2
                db  1Bh
                db  3Ch ; <
                db  40h ; @
                db  72h ; r
                db  2Dh ; -
                db 0E8h
                db 0C3h
                db  18h
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  21h ; !
                db    0
                db 0E8h
                db 0F2h
                db  1Ah
                db  24h ; $
                db  1Fh
                db  12h
                db    6
                db  25h ; %
                db    0
                db  12h
                db    6
                db  25h ; %
                db    0
                db  24h ; $
                db  77h ; w
                db 0F8h
                db  12h
                db    6
                db  59h ; Y
                db    0
                db  27h ; '
                db 0A2h
                db  59h ; Y
                db    0
                db 0A0h
                db  58h ; X
                db    0
                db  14h
                db    0
                db  27h ; '
                db 0A2h
                db  58h ; X
                db    0
                db 0E9h
                db 0EDh
                db 0D2h
                db 0A0h
                db  25h ; %
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db  73h ; s
                db    3
                db 0E9h
                db  4Ah ; J
                db 0FFh
                db 0E8h
                db  8Bh
                db  18h
                db  41h ; A
                db  20h
                db    0
                db 0E9h
                db  0Bh
                db 0FFh
                db 0E8h
                db  82h
                db  18h
                db  54h ; T
                db  52h ; R
                db  49h ; I
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  55h ; U
                db  4Dh ; M
                db  21h ; !
                db    0
                db 0F8h
                db 0A0h
                db 0E5h
                db    0
                db  14h
                db    1
                db  27h ; '
                db 0A2h
                db 0E5h
                db    0
                db 0E9h
                db 0BCh
                db 0D2h
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
                db 0E8h
                db  5Ah ; Z
                db  18h
                db  48h ; H
                db  59h ; Y
                db  50h ; P
                db  45h ; E
                db  52h ; R
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  9Fh
                db 0D2h
                db 0E8h
                db  48h ; H
                db  18h
                db  49h ; I
                db  47h ; G
                db  4Eh ; N
                db  49h ; I
                db  54h ; T
                db  45h ; E
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  52h ; R
                db  43h ; C
                db  48h ; H
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  73h ; s
                db    3
                db 0E9h
                db  85h
                db 0D2h
                db 0A0h
                db  64h ; d
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  13h
                db 0E8h
                db  27h ; '
                db  18h
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db  20h
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  68h ; h
                db 0D2h
                db 0F9h
                db 0A0h
                db  64h ; d
                db    0
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db  64h ; d
                db    0
                db 0B0h
                db  96h
                db 0A2h
                db  26h ; &
                db    0
                db 0E9h
                db  57h ; W
                db 0D2h
                db 0E8h
                db    0
                db  18h
                db  4Ah ; J
                db  55h ; U
                db  4Dh ; M
                db  50h ; P
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db  57h ; W
                db  48h ; H
                db  45h ; E
                db  45h ; E
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db 0E9h
                db  42h ; B
                db 0D2h
                db 0E8h
                db 0EBh
                db  17h
                db  4Bh ; K
                db  4Ch ; L
                db  49h ; I
                db  4Dh ; M
                db  42h ; B
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    5
                db  74h ; t
                db  11h
                db  3Ch ; <
                db    4
                db  74h ; t
                db  0Dh
                db 0E8h
                db 0D7h
                db  17h
                db  2Dh ; -
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  21h ; !
                db 0D2h
                db 0A0h
                db  14h
                db    0
                db  24h ; $
                db  10h
                db  74h ; t
                db 0ECh
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  75h ; u
                db    3
                db 0E9h
                db  5Eh ; ^
                db 0FBh
                db  90h
                db 0FEh
                db  0Eh
                db  25h ; %
                db    0
                db  78h ; x
                db  1Ah
                db 0E8h
                db 0B2h
                db  17h
                db  8Dh
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  4Ch ; L
                db  45h ; E
                db  56h ; V
                db  45h ; E
                db  4Ch ; L
                db  20h
                db    0
                db 0A0h
                db  25h ; %
                db    0
                db    4
                db    0
                db  27h ; '
                db 0E8h
                db  74h ; t
                db  1Bh
                db 0E9h
                db 0EFh
                db 0D1h
                db  90h
                db 0A0h
                db  5Ah ; Z
                db    0
                db 0A2h
                db    0
                db    0
                db 0A0h
                db  5Bh ; [
                db    0
                db 0A2h
                db    1
                db    0
                db 0BBh
                db 0FEh
                db    0
                db 0B8h
                db 0FFh
                db 0FFh
                db  89h
                db  87h
                db  72h ; r
                db    2
                db  89h
                db  87h
                db  72h ; r
                db    3
                db  83h
                db 0EBh
                db    2
                db  79h ; y
                db 0F3h
                db 0B0h
                db    0
                db 0A2h
                db  4Ah ; J
                db    0
                db 0E8h
                db  24h ; $
                db 0EAh
                db 0A0h
                db  5Ah ; Z
                db    0
                db 0A2h
                db    0
                db    0
                db 0A0h
                db  5Bh ; [
                db    0
                db 0A2h
                db    1
                db    0
                db 0A0h
                db  6Dh ; m
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  1Ah
                db 0A0h
                db  6Ah ; j
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db  6Bh ; k
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  25h ; %
                db  18h
                db 0B0h
                db  50h ; P
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0E9h
                db  9Ah
                db 0D1h
                db  8Dh
                db  16h
                db  36h ; 6
                db    0
                db 0B4h
                db  28h ; (
                db 0B9h
                db    0
                db    1
                db 0E8h
                db  25h ; %
                db  1Ah
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  59h ; Y
                db  45h ; E
                db  52h ; R
                db  20h
                db  20h
                db 0E9h
                db  83h
                db 0D1h
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  26h ; &
                db  75h ; u
                db    3
                db 0E9h
                db  40h ; @
                db    1
                db  90h
                db  3Ch ; <
                db  28h ; (
                db  74h ; t
                db  13h
                db 0E8h
                db  1Dh
                db  17h
                db  4Ch ; L
                db  41h ; A
                db  55h ; U
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  61h ; a
                db 0D1h
                db 0E8h
                db  0Ah
                db  17h
                db  4Ch ; L
                db  41h ; A
                db  55h ; U
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  2Dh ; -
                db  2Dh ; -
                db  52h ; R
                db  4Fh ; O
                db  43h ; C
                db  4Bh ; K
                db  45h ; E
                db  54h ; T
                db    0
                db 0A0h
                db 0E5h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  38h ; 8
                db 0E8h
                db 0F1h
                db  16h
                db  8Dh
                db  41h ; A
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  54h ; T
                db  41h ; A
                db  4Ch ; L
                db  4Ch ; L
                db  49h ; I
                db  43h ; C
                db  20h
                db  56h ; V
                db  4Fh ; O
                db  49h ; I
                db  43h ; C
                db  45h ; E
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  8Dh
                db  53h ; S
                db  48h ; H
                db  49h ; I
                db  50h ; P
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  43h ; C
                db  41h ; A
                db  50h ; P
                db  41h ; A
                db  42h ; B
                db  4Ch ; L
                db  45h ; E
                db  20h
                db  4Fh ; O
                db  46h ; F
                db  20h
                db  4Ch ; L
                db  41h ; A
                db  55h ; U
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  21h ; !
                db    0
                db 0E9h
                db  10h
                db 0D1h
                db 0E8h
                db 0B9h
                db  16h
                db  8Dh
                db  50h ; P
                db  52h ; R
                db  45h ; E
                db  50h ; P
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  4Ch ; L
                db  41h ; A
                db  55h ; U
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  21h ; !
                db    0
                db 0A0h
                db  6Dh ; m
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  1Bh
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  14h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  6Ah ; j
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  6Bh ; k
                db    0
                db 0B0h
                db    1
                db 0A2h
                db  69h ; i
                db    0
                db 0E8h
                db 0A0h
                db 0E9h
                db  90h
                db 0E8h
                db  38h ; 8
                db  30h ; 0
                db 0E8h
                db  2Ch ; ,
                db 0E9h
                db 0BFh
                db 0FFh
                db    0
                db 0B0h
                db 0FFh
                db  88h
                db  85h
                db  72h ; r
                db    2
                db  88h
                db  85h
                db  72h ; r
                db    3
                db  4Fh ; O
                db  79h ; y
                db 0F5h
                db 0C6h
                db    6
                db    0
                db    0
                db    0
                db 0C6h
                db    6
                db    1
                db    0
                db    0
                db 0A2h
                db  12h
                db    0
                db 0E8h
                db  5Bh ; [
                db  16h
                db  28h ; (
                db  50h ; P
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  53h ; S
                db  20h
                db  41h ; A
                db  4Eh ; N
                db  59h ; Y
                db  20h
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db  29h ; )
                db    0
                db 0A0h
                db    1
                db    0
                db 0F8h
                db  14h
                db    2
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    1
                db    0
                db  24h ; $
                db    7
                db  75h ; u
                db  0Bh
                db 0A0h
                db    0
                db    0
                db 0F8h
                db  14h
                db    1
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    0
                db    0
                db 0E8h
                db    0
                db  21h ; !
                db 0BBh
                db  1Ch
                db    0
                db 0E8h
                db  5Ah ; Z
                db 0CDh
                db 0E8h
                db  93h
                db  15h
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0D5h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db 0E5h
                db  16h
                db  24h ; $
                db  7Fh ; 
                db  3Ch ; <
                db    8
                db  74h ; t
                db    3
                db 0E9h
                db  8Bh
                db 0D2h
                db 0A0h
                db  6Dh ; m
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db    8
                db 0B0h
                db    0
                db 0A2h
                db  69h ; i
                db    0
                db 0E9h
                db  4Dh ; M
                db 0D0h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  6Ah ; j
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  6Bh ; k
                db    0
                db 0B0h
                db    1
                db 0A2h
                db  69h ; i
                db    0
                db 0E9h
                db  39h ; 9
                db 0D0h
                db 0E8h
                db 0E2h
                db  15h
                db  4Ch ; L
                db  41h ; A
                db  55h ; U
                db  4Eh ; N
                db  43h ; C
                db  48h ; H
                db  2Dh ; -
                db  2Dh ; -
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db    0
                db 0A0h
                db 0E1h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  33h ; 3
                db 0E8h
                db 0CAh
                db  15h
                db  8Dh
                db  46h ; F
                db  55h ; U
                db  4Eh ; N
                db  4Eh ; N
                db  59h ; Y
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  53h ; S
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  20h
                db  49h ; I
                db  53h ; S
                db  8Dh
                db  4Dh ; M
                db  49h ; I
                db  53h ; S
                db  53h ; S
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  41h ; A
                db  20h
                db  42h ; B
                db  52h ; R
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  20h
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  54h ; T
                db  4Fh ; O
                db  4Eh ; N
                db  21h ; !
                db    0
                db 0E9h
                db 0EBh
                db 0CFh
                db  90h
                db 0B0h
                db    0
                db 0A2h
                db  21h ; !
                db    0
                db 0B0h
                db 0FFh
                db 0A2h
                db  22h ; "
                db    0
                db 0E8h
                db  8Ch
                db  15h
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0C6h
                db    6
                db  4Eh ; N
                db    5
                db 0FFh
                db 0E8h
                db  65h ; e
                db  24h ; $
                db 0BBh
                db    2
                db    0
                db 0E8h
                db 0A9h
                db 0CCh
                db 0E8h
                db 0E2h
                db  14h
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db  2Bh ; +
                db  3Ah ; :
                db    6
                db  6Eh ; n
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  63h ; c
                db  90h
                db  3Ah ; :
                db    6
                db  6Fh ; o
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  75h ; u
                db  90h
                db  3Ah ; :
                db    6
                db  70h ; p
                db    2
                db  75h ; u
                db    3
                db 0E9h
                db  86h
                db    0
                db  3Ah ; :
                db    6
                db  71h ; q
                db    2
                db  75h ; u
                db    3
                db 0E9h
                db  98h
                db    0
                db  3Ch ; <
                db  4Ch ; L
                db  75h ; u
                db    3
                db 0E9h
                db 0ACh
                db    0
                db 0F8h
                db 0A0h
                db    0
                db    0
                db  12h
                db    6
                db  21h ; !
                db    0
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    0
                db    0
                db 0F8h
                db 0A0h
                db    1
                db    0
                db  12h
                db    6
                db  22h ; "
                db    0
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    1
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  19h
                db 0A0h
                db    0
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  0Ah
                db 0A0h
                db    1
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db    3
                db 0EBh
                db    9
                db  90h
                db 0C6h
                db    6
                db  4Eh ; N
                db    5
                db    0
                db 0E9h
                db  69h ; i
                db 0E9h
                db  90h
                db 0E8h
                db 0D8h
                db  1Fh
                db 0EBh
                db  84h
                db 0B0h
                db    0
                db 0A2h
                db  21h ; !
                db    0
                db 0B0h
                db 0FFh
                db 0A2h
                db  22h ; "
                db    0
                db 0E8h
                db 0F7h
                db  14h
                db  4Eh ; N
                db  4Fh ; O
                db  52h ; R
                db  54h ; T
                db  48h ; H
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0EBh
                db 0A5h
                db 0B0h
                db    0
                db 0A2h
                db  21h ; !
                db    0
                db 0B0h
                db    1
                db 0A2h
                db  22h ; "
                db    0
                db 0E8h
                db 0DCh
                db  14h
                db  53h ; S
                db  4Fh ; O
                db  55h ; U
                db  54h ; T
                db  48h ; H
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0EBh
                db  8Ah
                db 0B0h
                db    1
                db 0A2h
                db  21h ; !
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  22h ; "
                db    0
                db 0E8h
                db 0C1h
                db  14h
                db  45h ; E
                db  41h ; A
                db  53h ; S
                db  54h ; T
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0E9h
                db  6Fh ; o
                db 0FFh
                db 0B0h
                db 0FFh
                db 0A2h
                db  21h ; !
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  22h ; "
                db    0
                db 0E8h
                db 0A6h
                db  14h
                db  57h ; W
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0E9h
                db  54h ; T
                db 0FFh
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  5Dh ; ]
                db  15h
                db  3Ch ; <
                db    8
                db  75h ; u
                db  17h
                db 0E8h
                db  82h
                db  14h
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  2Eh ; .
                db    0
                db 0C6h
                db    6
                db  4Eh ; N
                db    5
                db    0
                db 0E9h
                db 0C2h
                db 0CEh
                db  90h
                db 0E8h
                db  6Ah ; j
                db  14h
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  2Dh ; -
                db  2Dh ; -
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  21h ; !
                db  21h ; !
                db  21h ; !
                db  21h ; !
                db  21h ; !
                db  0Dh
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0E9h
                db    9
                db 0FFh
                db 0E8h
                db  4Ah ; J
                db  14h
                db  4Dh ; M
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  43h ; C
                db  20h
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  59h ; Y
                db  20h
                db  23h ; #
                db    0
                db 0E8h
                db  7Ch ; |
                db 0E4h
                db 0A2h
                db  63h ; c
                db    0
                db 0E8h
                db  2Dh ; -
                db  14h
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  59h ; Y
                db  20h
                db  3Dh ; =
                db  3Eh ; >
                db  20h
                db    0
                db 0A0h
                db  63h ; c
                db    0
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db 0C1h
                db  14h
                db 0E9h
                db  65h ; e
                db 0CEh
                db 0E8h
                db  0Eh
                db  14h
                db  4Eh ; N
                db  45h ; E
                db  47h ; G
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  20h
                db  54h ; T
                db  49h ; I
                db  4Dh ; M
                db  45h ; E
                db    0
                db 0A0h
                db 0E3h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  20h
                db 0E8h
                db 0F8h
                db  13h
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  3Fh ; ?
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  27h ; '
                db  52h ; R
                db  45h ; E
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  45h ; E
                db  49h ; I
                db  4Eh ; N
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  49h ; I
                db  4Eh ; N
                db    0
                db 0E9h
                db  2Fh ; /
                db 0CEh
                db 0F9h
                db 0A0h
                db 0E3h
                db    0
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db 0E3h
                db    0
                db 0E8h
                db 0CCh
                db  13h
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  52h ; R
                db  55h ; U
                db  42h ; B
                db  20h
                db  41h ; A
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  49h ; I
                db  4Eh ; N
                db  2Eh ; .
                db  2Eh ; .
                db  2Eh ; .
                db    0
                db 0B0h
                db  14h
                db 0A2h
                db  6Ah ; j
                db    2
                db 0E9h
                db    5
                db 0CEh
                db 0E8h
                db 0AEh
                db  13h
                db  4Fh ; O
                db  46h ; F
                db  46h ; F
                db  45h ; E
                db  52h ; R
                db  20h
                db  47h ; G
                db  4Fh ; O
                db  4Ch ; L
                db  44h ; D
                db  20h
                db  44h ; D
                db  49h ; I
                db  52h ; R
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  2Dh ; -
                db    0
                db 0E8h
                db  1Bh
                db 0E7h
                db 0E8h
                db  9Ch
                db 0E7h
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db 0B0h
                db  8Dh
                db 0E8h
                db  9Fh
                db  17h
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db    0
                db  75h ; u
                db  15h
                db 0E8h
                db  80h
                db  13h
                db  4Fh ; O
                db  46h ; F
                db  46h ; F
                db  45h ; E
                db  52h ; R
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  57h ; W
                db  48h ; H
                db  4Fh ; O
                db  4Dh ; M
                db  3Fh ; ?
                db    0
                db 0E9h
                db 0C2h
                db 0CDh
                db 0E8h
                db  6Bh ; k
                db  13h
                db  48h ; H
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  43h ; C
                db  48h ; H
                db  20h
                db  28h ; (
                db  2Ah ; *
                db  31h ; 1
                db  30h ; 0
                db  30h ; 0
                db  29h ; )
                db  20h
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  9Eh
                db 0E3h
                db 0A2h
                db  27h ; '
                db    0
                db 0B0h
                db    0
                db 0A2h
                db  28h ; (
                db    0
                db 0E8h
                db 0BDh
                db 0E3h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0D7h
                db    1
                db  0Ah
                db 0C0h
                db  79h ; y
                db  11h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    2
                db  75h ; u
                db  0Ah
                db 0A0h
                db  49h ; I
                db    0
                db  3Ch ; <
                db    3
                db  75h ; u
                db    3
                db 0EBh
                db  1Dh
                db  90h
                db  90h
                db 0E8h
                db  25h ; %
                db  13h
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  56h ; V
                db  45h ; E
                db  52h ; R
                db  59h ; Y
                db  20h
                db  4Dh ; M
                db  55h ; U
                db  43h ; C
                db  48h ; H
                db  21h ; !
                db    0
                db 0E9h
                db  61h ; a
                db 0CDh
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0D7h
                db    1
                db  3Ch ; <
                db  81h
                db  74h ; t
                db  0Bh
                db  3Ch ; <
                db  82h
                db  74h ; t
                db  2Ah ; *
                db  3Ch ; <
                db  83h
                db  74h ; t
                db  68h ; h
                db 0E9h
                db  91h
                db    0
                db 0A0h
                db  27h ; '
                db    0
                db  3Ch ; <
                db    5
                db  72h ; r
                db 0C2h
                db 0E8h
                db 0E8h
                db  12h
                db  45h ; E
                db  4Eh ; N
                db  49h ; I
                db  4Ch ; L
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  53h ; S
                db  21h ; !
                db    0
                db 0B0h
                db    1
                db 0A2h
                db  7Fh ; 
                db    0
                db 0E9h
                db  23h ; #
                db 0CDh
                db 0A0h
                db  27h ; '
                db    0
                db  3Ch ; <
                db    5
                db  72h ; r
                db  9Fh
                db 0A0h
                db  6Ch ; l
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  15h
                db 0E8h
                db 0BEh
                db  12h
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  4Eh ; N
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  52h ; R
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  21h ; !
                db    0
                db 0E9h
                db    0
                db 0CDh
                db  90h
                db 0E8h
                db 0A8h
                db  12h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  52h ; R
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  53h ; S
                db  21h ; !
                db    0
                db 0B0h
                db    1
                db 0A2h
                db 0D6h
                db    0
                db 0E9h
                db 0E1h
                db 0CCh
                db 0E8h
                db 0C2h
                db  14h
                db  24h ; $
                db    7
                db 0F8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db  47h ; G
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  76h ; v
                db    0
                db 0E8h
                db  74h ; t
                db  12h
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  20h
                db  54h ; T
                db  41h ; A
                db  4Bh ; K
                db  45h ; E
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  53h ; S
                db  21h ; !
                db    0
                db 0E9h
                db 0B5h
                db 0CCh
                db 0E8h
                db  96h
                db  14h
                db  24h ; $
                db    7
                db  3Ch ; <
                db    6
                db  72h ; r
                db    3
                db 0E9h
                db  2Ch ; ,
                db 0FFh
                db  90h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0F8h
                db  8Ah
                db  85h
                db  4Bh ; K
                db    0
                db  12h
                db    6
                db  27h ; '
                db    0
                db  27h ; '
                db  12h
                db    6
                db  27h ; '
                db    0
                db  27h ; '
                db  12h
                db    6
                db  27h ; '
                db    0
                db  27h ; '
                db  12h
                db    6
                db  27h ; '
                db    0
                db  27h ; '
                db  88h
                db  85h
                db  4Bh ; K
                db    0
                db 0E8h
                db  30h ; 0
                db  12h
                db  41h ; A
                db  4Ch ; L
                db  41h ; A
                db  4Bh ; K
                db  41h ; A
                db  5Ah ; Z
                db  41h ; A
                db  4Dh ; M
                db  21h ; !
                db    0
                db 0E9h
                db  77h ; w
                db 0CCh
                db 0E8h
                db  20h
                db  12h
                db  50h ; P
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db    0
                db 0E9h
                db  6Ch ; l
                db 0CCh
                db 0E8h
                db  15h
                db  12h
                db  51h ; Q
                db  55h ; U
                db  49h ; I
                db  54h ; T
                db  20h
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  53h ; S
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  47h ; G
                db  41h ; A
                db  4Dh ; M
                db  45h ; E
                db  2Eh ; .
                db    0
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  16h
                db 0E8h
                db 0F8h
                db  11h
                db  8Dh
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db  4Fh ; O
                db  55h ; U
                db  54h ; T
                db  44h ; D
                db  4Fh ; O
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  21h ; !
                db    0
                db 0E9h
                db  36h ; 6
                db 0CCh
                db 0A0h
                db  6Dh ; m
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  16h
                db 0E8h
                db 0DBh
                db  11h
                db  8Dh
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  20h
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  54h ; T
                db  48h ; H
                db  21h ; !
                db    0
                db 0E9h
                db  19h
                db 0CCh
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  78h ; x
                db  73h ; s
                db  15h
                db 0E8h
                db 0BEh
                db  11h
                db  8Dh
                db  4Fh ; O
                db  4Eh ; N
                db  4Ch ; L
                db  59h ; Y
                db  20h
                db  4Fh ; O
                db  4Eh ; N
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  4Fh ; O
                db  54h ; T
                db  21h ; !
                db    0
                db 0E9h
                db 0FDh
                db 0CBh
                db 0E8h
                db 0A9h
                db  11h
                db  8Dh
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db  20h
                db  4Dh ; M
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  4Eh ; N
                db  54h ; T
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  53h ; S
                db  45h ; E
                db  21h ; !
                db    0
                db 0E8h
                db 0B0h
                db 0E4h
                db 0E9h
                db 0E3h
                db 0CBh
                db 0E8h
                db  8Ch
                db  11h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  59h ; Y
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  3Ah ; :
                db  0Dh
                db  31h ; 1
                db  2Dh ; -
                db  44h ; D
                db  41h ; A
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Dh ; M
                db  41h ; A
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  41h ; A
                db  58h ; X
                db  2Ch ; ,
                db  20h
                db  34h ; 4
                db  2Dh ; -
                db  42h ; B
                db  4Fh ; O
                db  2Ch ; ,
                db  0Dh
                db  35h ; 5
                db  2Dh ; -
                db  53h ; S
                db  57h ; W
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  47h ; G
                db  52h ; R
                db  2Ch ; ,
                db  20h
                db  37h ; 7
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  2Ch ; ,
                db  20h
                db  38h ; 8
                db  2Dh ; -
                db  50h ; P
                db  48h ; H
                db  2Eh ; .
                db  0Dh
                db  39h ; 9
                db  2Dh ; -
                db  51h ; Q
                db  55h ; U
                db  2Ch ; ,
                db  20h
                db  57h ; W
                db  48h ; H
                db  49h ; I
                db  43h ; C
                db  48h ; H
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db  86h
                db 0E1h
                db 0A2h
                db  1Fh
                db    0
                db 0F8h
                db  14h
                db  13h
                db 0E8h
                db 0E1h
                db  11h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  19h
                db 0A0h
                db  1Fh
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  12h
                db 0E8h
                db  1Ah
                db  11h
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  5Ch ; \
                db 0CBh
                db 0A0h
                db  1Fh
                db    0
                db    2
                db 0C0h
                db    2
                db 0C0h
                db    2
                db 0C0h
                db  3Ah ; :
                db    6
                db  4Ch ; L
                db    0
                db  72h ; r
                db  2Eh ; .
                db 0E8h
                db 0F9h
                db  10h
                db  20h
                db  3Ch ; <
                db  2Dh ; -
                db  54h ; T
                db  48h ; H
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  54h ; T
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  8Dh
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  4Ch ; L
                db  45h ; E
                db  20h
                db  45h ; E
                db  4Eh ; N
                db  4Fh ; O
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  57h ; W
                db  49h ; I
                db  45h ; E
                db  4Ch ; L
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db  22h ; "
                db 0CBh
                db 0E8h
                db 0CBh
                db  10h
                db  20h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  59h ; Y
                db  2Eh ; .
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0A2h
                db  61h ; a
                db    0
                db 0E9h
                db  0Eh
                db 0CBh
                db 0E8h
                db 0B7h
                db  10h
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  44h ; D
                db  49h ; I
                db  52h ; R
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  2Dh ; -
                db    0
                db 0E8h
                db  29h ; )
                db 0E4h
                db 0F8h
                db 0A0h
                db    0
                db    0
                db  12h
                db    6
                db  19h
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0F8h
                db 0A0h
                db    1
                db    0
                db  12h
                db    6
                db  1Ah
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0B0h
                db  8Dh
                db 0E8h
                db 0A0h
                db  14h
                db 0E8h
                db  5Ch ; \
                db  11h
                db  78h ; x
                db    4
                db  3Ch ; <
                db  7Ch ; |
                db  75h ; u
                db  21h ; !
                db  90h
                db 0F8h
                db 0A0h
                db  23h ; #
                db    0
                db  12h
                db    6
                db  19h
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0F8h
                db 0A0h
                db  24h ; $
                db    0
                db  12h
                db    6
                db  1Ah
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  3Ch ; <
                db  11h
                db  3Ch ; <
                db  68h ; h
                db  75h ; u
                db    3
                db 0EBh
                db  1Bh
                db  90h
                db 0E8h
                db  5Eh ; ^
                db  10h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  4Ch ; L
                db  55h ; U
                db  43h ; C
                db  4Bh ; K
                db  21h ; !
                db    0
                db 0E8h
                db  8Ah
                db  12h
                db  24h ; $
                db    7
                db  75h ; u
                db    3
                db 0E8h
                db 0A6h
                db 0D1h
                db  90h
                db 0E9h
                db  9Bh
                db 0CAh
                db  90h
                db 0A0h
                db  47h ; G
                db    0
                db  3Ch ; <
                db    3
                db  74h ; t
                db    5
                db 0E8h
                db  74h ; t
                db  12h
                db  78h ; x
                db 0D9h
                db  90h
                db 0E8h
                db  6Eh ; n
                db  12h
                db  78h ; x
                db 0D3h
                db 0A0h
                db 0D7h
                db    1
                db  3Ch ; <
                db    1
                db  74h ; t
                db 0CCh
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    1
                db  74h ; t
                db  15h
                db  3Ch ; <
                db    2
                db  75h ; u
                db 0C1h
                db 0A0h
                db    1
                db    0
                db  3Ch ; <
                db  20h
                db  73h ; s
                db 0BAh
                db 0A0h
                db    0
                db    0
                db  3Ch ; <
                db  20h
                db  73h ; s
                db  2Eh ; .
                db 0EBh
                db  57h ; W
                db  90h
                db 0A0h
                db    1
                db    0
                db  3Ch ; <
                db  20h
                db  73h ; s
                db 0A9h
                db 0A0h
                db    0
                db    0
                db  3Ch ; <
                db  20h
                db  72h ; r
                db 0A2h
                db  90h
                db 0E8h
                db 0FFh
                db  0Fh
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  4Fh ; O
                db  44h ; D
                db  21h ; !
                db    0
                db 0F8h
                db 0A0h
                db  53h ; S
                db    0
                db  14h
                db    1
                db  27h ; '
                db 0A2h
                db  53h ; S
                db    0
                db 0E9h
                db  3Ah ; :
                db 0CAh
                db  90h
                db 0E8h
                db 0E2h
                db  0Fh
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  21h ; !
                db    0
                db 0E8h
                db    9
                db  12h
                db  24h ; $
                db    3
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db  47h ; G
                db 0F8h
                db  8Ah
                db  85h
                db  96h
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  96h
                db    0
                db 0E9h
                db  0Fh
                db 0CAh
                db  90h
                db 0E8h
                db 0B7h
                db  0Fh
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  21h ; !
                db    0
                db 0E8h
                db 0DDh
                db  11h
                db  24h ; $
                db    7
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db  47h ; G
                db 0F8h
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db  14h
                db    1
                db  27h ; '
                db  88h
                db  85h
                db  76h ; v
                db    0
                db 0E9h
                db 0E3h
                db 0C9h
                db 0E8h
                db  8Ch
                db  0Fh
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  53h ; S
                db  41h ; A
                db  43h ; C
                db  54h ; T
                db  2Dh ; -
                db    0
                db 0E8h
                db    2
                db 0E3h
                db 0E8h
                db  83h
                db 0E3h
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  20h
                db    0
                db 0B0h
                db  8Dh
                db 0E8h
                db  86h
                db  13h
                db 0A0h
                db  4Ah ; J
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  31h ; 1
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db    0
                db  75h ; u
                db  44h ; D
                db 0E8h
                db  34h ; 4
                db  10h
                db  78h ; x
                db    4
                db  3Ch ; <
                db  7Ch ; |
                db  75h ; u
                db  21h ; !
                db  90h
                db 0F8h
                db 0A0h
                db  23h ; #
                db    0
                db  12h
                db    6
                db  19h
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0F8h
                db 0A0h
                db  24h ; $
                db    0
                db  12h
                db    6
                db  1Ah
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db  14h
                db  10h
                db  3Ch ; <
                db  68h ; h
                db  75h ; u
                db    3
                db 0E9h
                db 0EAh
                db 0D4h
                db 0E8h
                db  36h ; 6
                db  0Fh
                db  46h ; F
                db  55h ; U
                db  4Eh ; N
                db  4Eh ; N
                db  59h ; Y
                db  2Ch ; ,
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  52h ; R
                db  45h ; E
                db  53h ; S
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  45h ; E
                db  21h ; !
                db    0
                db 0E9h
                db  73h ; s
                db 0C9h
                db  8Ah
                db  85h
                db 0D7h
                db    1
                db  3Ch ; <
                db  80h
                db  72h ; r
                db    3
                db 0E9h
                db  3Eh ; >
                db    1
                db  90h
                db  8Ah
                db  85h
                db  97h
                db    1
                db  3Ch ; <
                db  6Ch ; l
                db  75h ; u
                db    3
                db 0E9h
                db  72h ; r
                db    1
                db  90h
                db  8Ah
                db  85h
                db  97h
                db    1
                db  3Ch ; <
                db  60h ; `
                db  75h ; u
                db  24h ; $
                db 0E8h
                db 0FCh
                db  0Eh
                db  41h ; A
                db  20h
                db  47h ; G
                db  55h ; U
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  50h ; P
                db  41h ; A
                db  59h ; Y
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  20h
                db  54h ; T
                db  41h ; A
                db  58h ; X
                db  45h ; E
                db  53h ; S
                db  21h ; !
                db    0
                db 0E9h
                db  2Fh ; /
                db 0C9h
                db  3Ch ; <
                db  64h ; d
                db  75h ; u
                db  20h
                db 0E8h
                db 0D4h
                db  0Eh
                db  41h ; A
                db  20h
                db  4Ah ; J
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  20h
                db  53h ; S
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  48h ; H
                db  4Fh ; O
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  21h ; !
                db    0
                db 0E9h
                db  0Bh
                db 0C9h
                db  3Ch ; <
                db  68h ; h
                db  75h ; u
                db  2Fh ; /
                db 0E8h
                db 0B0h
                db  0Eh
                db  41h ; A
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  52h ; R
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  54h ; T
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  57h ; W
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  42h ; B
                db  55h ; U
                db  59h ; Y
                db  20h
                db  4Dh ; M
                db  59h ; Y
                db  20h
                db  41h ; A
                db  50h ; P
                db  50h ; P
                db  4Ch ; L
                db  45h ; E
                db  53h ; S
                db  3Fh ; ?
                db    0
                db 0E9h
                db 0D8h
                db 0C8h
                db  3Ch ; <
                db 0F0h
                db  75h ; u
                db  25h ; %
                db 0E8h
                db  7Dh ; }
                db  0Eh
                db  41h ; A
                db  20h
                db  46h ; F
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  2Ch ; ,
                db  20h
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  21h ; !
                db    0
                db 0E9h
                db 0AFh
                db 0C8h
                db  3Ch ; <
                db 0F4h
                db  75h ; u
                db  1Eh
                db 0E8h
                db  54h ; T
                db  0Eh
                db  41h ; A
                db  20h
                db  43h ; C
                db  4Ch ; L
                db  45h ; E
                db  52h ; R
                db  49h ; I
                db  43h ; C
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  42h ; B
                db  45h ; E
                db  4Ch ; L
                db  49h ; I
                db  45h ; E
                db  56h ; V
                db  45h ; E
                db  21h ; !
                db    0
                db 0E9h
                db  8Dh
                db 0C8h
                db  3Ch ; <
                db 0F8h
                db  75h ; u
                db  2Bh ; +
                db 0E8h
                db  32h ; 2
                db  0Eh
                db  41h ; A
                db  20h
                db  57h ; W
                db  49h ; I
                db  5Ah ; Z
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  48h ; H
                db  45h ; E
                db  58h ; X
                db  2Dh ; -
                db  45h ; E
                db  2Dh ; -
                db  50h ; P
                db  4Fh ; O
                db  4Fh ; O
                db  2Dh ; -
                db  48h ; H
                db  45h ; E
                db  58h ; X
                db  2Dh ; -
                db  4Fh ; O
                db  4Eh ; N
                db  2Dh ; -
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  21h ; !
                db    0
                db 0E9h
                db  5Eh ; ^
                db 0C8h
                db  3Ch ; <
                db 0FCh
                db  75h ; u
                db  2Dh ; -
                db 0E8h
                db    3
                db  0Eh
                db  41h ; A
                db  20h
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  45h ; E
                db  46h ; F
                db  20h
                db  53h ; S
                db  41h ; A
                db  59h ; Y
                db  53h ; S
                db  3Ah ; :
                db  0Dh
                db  50h ; P
                db  53h ; S
                db  53h ; S
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  57h ; W
                db  41h ; A
                db  4Eh ; N
                db  4Eh ; N
                db  41h ; A
                db  20h
                db  42h ; B
                db  55h ; U
                db  59h ; Y
                db  20h
                db  41h ; A
                db  20h
                db  57h ; W
                db  41h ; A
                db  54h ; T
                db  43h ; C
                db  48h ; H
                db  3Fh ; ?
                db    0
                db 0E9h
                db  2Dh ; -
                db 0C8h
                db 0E9h
                db  9Dh
                db 0FEh
                db 0F9h
                db 0F5h
                db  1Ch
                db  80h
                db 0F5h
                db 0E8h
                db  3Dh ; =
                db 0D3h
                db 0BFh
                db    3
                db    0
                db 0BEh
                db 0FFh
                db 0FFh
                db  4Eh ; N
                db  75h ; u
                db 0FDh
                db 0BBh
                db    4
                db    0
                db 0E8h
                db 0F1h
                db 0C4h
                db  4Fh ; O
                db  75h ; u
                db 0F1h
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  20h
                db    0
                db  8Bh
                db 0FBh
                db 0A0h
                db  6Dh ; m
                db    0
                db  3Ch ; <
                db    9
                db  75h ; u
                db  14h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    3
                db  75h ; u
                db  0Dh
                db  8Ah
                db  85h
                db 0D7h
                db    1
                db  3Ch ; <
                db  81h
                db  75h ; u
                db    5
                db 0B0h
                db    1
                db 0A2h
                db  6Ch ; l
                db    0
                db 0E9h
                db 0EAh
                db 0C7h
                db  90h
                db 0E8h
                db  92h
                db  0Dh
                db  57h ; W
                db  45h ; E
                db  4Ch ; L
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  45h ; E
                db  20h
                db  4Dh ; M
                db  59h ; Y
                db  20h
                db  43h ; C
                db  48h ; H
                db  49h ; I
                db  4Ch ; L
                db  44h ; D
                db  20h
                db    0
; ---------------------------------------------------------------------------
                call    write_player_name
                call    write_string
; ---------------------------------------------------------------------------
                db 8Dh,'FIRST MY 50 G.P. TRIBUTE!',8Dh,0
; ---------------------------------------------------------------------------

loc_14281:                              ; CODE XREF: sg01a2:4262↑j
                mov     al, 50h ; 'P'
                mov     byte_17438, al
                mov     al, 0
                mov     _sleepFlag2?, al
                call    sub_12052
                call    write_string    ; AND FOR IT I RAISE THEE
; ---------------------------------------------------------------------------
aAndForItIRaise db 'AND FOR IT I RAISE THEE ',0
; ---------------------------------------------------------------------------
                mov     al, player._hp
                mov     bh, 0
                mov     bl, 3
                mov     di, bx
                cmp     al, 50h ; 'P'
                jb      short loc_142D1
                mov     bh, 0
                mov     bl, 2
                mov     di, bx
                cmp     al, 75h ; 'u'
                jb      short loc_142D1
                mov     bh, 0
                mov     bl, 1
                mov     di, bx
                cmp     al, 99h
                jb      short loc_142D1
                mov     bh, 0
                mov     bl, 0
                mov     di, bx

loc_142D1:                              ; CODE XREF: sg01a2:42B5↑j
                                        ; sg01a2:42BF↑j ...
                mov     bx, di
                mov     byte_1742F, bl
                mov     ax, di
                mov     bh, 0
                mov     bl, 0
                mov     di, bx
                call    write_two_numbers
                clc
                mov     al, player._hp
                adc     al, byte_1742F
                daa
                mov     player._hp, al
                jmp     loc_10A33
; ---------------------------------------------------------------------------
                db 0E8h
                db 0EBh
                db  0Ch
                db  55h ; U
                db  4Eh ; N
                db  4Ch ; L
                db  4Fh ; O
                db  43h ; C
                db  4Bh ; K
                db  20h
                db  44h ; D
                db  49h ; I
                db  52h ; R
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  49h ; I
                db  4Fh ; O
                db  4Eh ; N
                db  2Dh ; -
                db    0
                db 0E8h
                db  59h ; Y
                db 0E0h
                db 0A0h
                db  19h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  16h
                db 0E8h
                db 0CCh
                db  0Ch
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  44h ; D
                db  4Fh ; O
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  21h ; !
                db    0
                db 0E9h
                db  0Ah
                db 0C7h
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0A0h
                db    0
                db    0
                db 0F8h
                db  12h
                db    6
                db  19h
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0E8h
                db  79h ; y
                db  0Dh
                db  3Ch ; <
                db 0A0h
                db  75h ; u
                db 0D2h
                db 0A0h
                db  65h ; e
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  19h
                db 0E8h
                db  97h
                db  0Ch
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db  53h ; S
                db  20h
                db  54h ; T
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  20h
                db  46h ; F
                db  49h ; I
                db  54h ; T
                db  21h ; !
                db    0
                db 0E9h
                db 0D2h
                db 0C6h
                db 0F9h
                db 0A0h
                db  65h ; e
                db    0
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db  65h ; e
                db    0
                db 0A0h
                db  14h
                db    0
                db    2
                db 0C0h
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db  8Bh
                db  9Dh
                db    6
                db    0
                db  88h
                db    7
                db 0E9h
                db 0B5h
                db 0C6h
                db 0A0h
                db 0DBh
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  11h
                db 0E8h
                db  57h ; W
                db  0Ch
                db  56h ; V
                db  49h ; I
                db  45h ; E
                db  57h ; W
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  9Dh
                db 0C6h
                db 0A0h
                db  4Ah ; J
                db    0
                db  3Ch ; <
                db    4
                db  73h ; s
                db 0E8h
                db 0E8h
                db  3Fh ; ?
                db  0Ch
                db  56h ; V
                db  49h ; I
                db  45h ; E
                db  57h ; W
                db  8Dh
                db  57h ; W
                db  49h ; I
                db  54h ; T
                db  48h ; H
                db  20h
                db  4Dh ; M
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  43h ; C
                db  41h ; A
                db  4Ch ; L
                db  20h
                db  48h ; H
                db  45h ; E
                db  4Ch ; L
                db  4Dh ; M
                db  21h ; !
                db    0
                db 0F9h
                db 0A0h
                db 0DBh
                db    0
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db 0DBh
                db    0
                db 0B0h
                db  20h
                db 0A2h
                db  23h ; #
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db    3
                db 0E6h
                db 0E8h
                db  7Bh ; {
                db  0Bh
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db 0B7h
                db    0
                db 0B3h
                db 0FFh
                db  8Bh
                db 0FBh
                db 0B0h
                db 0FFh
                db  88h
                db  85h
                db  72h ; r
                db    2
                db  88h
                db  85h
                db  72h ; r
                db    3
                db  4Fh ; O
                db  75h ; u
                db 0F5h
                db 0A2h
                db  72h ; r
                db    2
                db 0A2h
                db  72h ; r
                db    3
                db 0E9h
                db  40h ; @
                db 0C6h
                db 0E8h
                db 0E9h
                db  0Bh
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  3Ah ; :
                db  0Dh
                db  31h ; 1
                db  2Dh ; -
                db  43h ; C
                db  4Ch ; L
                db  4Fh ; O
                db  54h ; T
                db  48h ; H
                db  2Ch ; ,
                db  20h
                db  32h ; 2
                db  2Dh ; -
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  2Ch ; ,
                db  20h
                db  33h ; 3
                db  2Dh ; -
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db  2Ch ; ,
                db  0Dh
                db  34h ; 4
                db  2Dh ; -
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  2Ch ; ,
                db  20h
                db  35h ; 5
                db  2Dh ; -
                db  52h ; R
                db  45h ; E
                db  46h ; F
                db  4Ch ; L
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db  2Ch ; ,
                db  20h
                db  36h ; 6
                db  2Dh ; -
                db  50h ; P
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db  2Ch ; ,
                db  0Dh
                db  57h ; W
                db  48h ; H
                db  49h ; I
                db  43h ; C
                db  48h ; H
                db  3Fh ; ?
                db  20h
                db    0
                db 0E8h
                db 0E0h
                db 0DBh
                db 0A2h
                db  1Fh
                db    0
                db  3Ch ; <
                db    7
                db  72h ; r
                db    5
                db 0B0h
                db    0
                db 0A2h
                db  1Fh
                db    0
                db  90h
                db 0F8h
                db  14h
                db  1Dh
                db 0E8h
                db  31h ; 1
                db  0Ch
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  96h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  19h
                db 0A0h
                db  1Fh
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  12h
                db 0E8h
                db  6Ah ; j
                db  0Bh
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db    0
                db 0E9h
                db 0AFh
                db 0C5h
                db 0A0h
                db  1Fh
                db    0
                db    2
                db 0C0h
                db    2
                db 0C0h
                db    2
                db 0C0h
                db  3Ah ; :
                db    6
                db  4Bh ; K
                db    0
                db  72h ; r
                db  2Dh ; -
                db 0E8h
                db  49h ; I
                db  0Bh
                db  20h
                db  3Ch ; <
                db  2Dh ; -
                db  54h ; T
                db  48h ; H
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  54h ; T
                db  20h
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  0Dh
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  4Fh ; O
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  45h ; E
                db  4Eh ; N
                db  4Fh ; O
                db  55h ; U
                db  47h ; G
                db  48h ; H
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  21h ; !
                db    0
                db 0E9h
                db  73h ; s
                db 0C5h
                db 0E8h
                db  1Ch
                db  0Bh
                db  20h
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  59h ; Y
                db  2Eh ; .
                db    0
                db 0A0h
                db  1Fh
                db    0
                db 0A2h
                db  62h ; b
                db    0
                db 0E9h
                db  5Fh ; _
                db 0C5h
                db 0E8h
                db    8
                db  0Bh
                db  58h ; X
                db  2Dh ; -
                db  49h ; I
                db  54h ; T
                db    0
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  78h ; x
                db  72h ; r
                db  0Dh
                db 0E8h
                db 0F9h
                db  0Ah
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db    0
                db 0E9h
                db  40h ; @
                db 0C5h
                db  90h
                db 0A0h
                db    0
                db    0
                db 0A2h
                db  23h ; #
                db    0
                db 0A0h
                db    1
                db    0
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db 0B3h
                db  0Bh
                db 0A0h
                db  14h
                db    0
                db  3Ch ; <
                db    4
                db  74h ; t
                db  1Ch
                db  3Ch ; <
                db    0
                db  75h ; u
                db    7
                db 0A0h
                db  13h
                db    0
                db  3Ch ; <
                db  24h ; $
                db  74h ; t
                db  11h
                db 0E8h
                db 0CAh
                db  0Ah
                db  2Dh ; -
                db  4Eh ; N
                db  4Fh ; O
                db  54h ; T
                db  20h
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  45h ; E
                db  21h ; !
                db    0
                db 0E9h
                db  0Dh
                db 0C5h
                db 0A0h
                db  13h
                db    0
                db    2
                db 0C0h
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0A0h
                db  47h ; G
                db    0
                db    2
                db 0C0h
                db 0F8h
                db  14h
                db  78h ; x
                db 0A2h
                db  13h
                db    0
                db 0E9h
                db 0F1h
                db 0C4h
                db 0E8h
                db  9Ah
                db  0Ah
                db  59h ; Y
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  57h ; W
                db  48h ; H
                db  41h ; A
                db  54h ; T
                db  3Fh ; ?
                db  8Dh
                db    0
                db 0E8h
                db 0F9h
                db    9
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db  3Ch ; <
                db  0Dh
                db  74h ; t
                db    5
                db 0E8h
                db  94h
                db  0Eh
                db 0EBh
                db 0EFh
                db 0E8h
                db  8Fh
                db  0Eh
                db 0E9h
                db 0CBh
                db 0C4h
                db 0E8h
                db 0B1h
                db    4
                db 0C6h
                db    6
                db  4Fh ; O
                db    5
                db    1
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    1
                db 0C6h
                db    6
                db  31h ; 1
                db    0
                db  26h ; &
                db 0E8h
                db  80h
                db  0Ch
                db 0E8h
                db 0FAh
                db  0Ah
                db 0E8h
                db  5Ch ; \
                db  0Ah
                db  8Dh
                db  41h ; A
                db  20h
                db  4Ch ; L
                db  45h ; E
                db  56h ; V
                db  45h ; E
                db  4Ch ; L
                db  20h
                db    0
                db 0A0h
                db  56h ; V
                db    0
                db 0E8h
                db  22h ; "
                db  0Eh
                db 0B0h
                db  20h
                db 0E8h
                db  5Ch ; \
                db  0Eh
                db 0A0h
                db  46h ; F
                db    0
                db 0E8h
                db 0C1h
                db  0Ah
                db 0B0h
                db  20h
                db 0E8h
                db  51h ; Q
                db  0Eh
                db 0A0h
                db  48h ; H
                db    0
                db 0F8h
                db  14h
                db  44h ; D
                db 0E8h
                db 0E0h
                db  0Ah
                db 0B0h
                db  20h
                db 0E8h
                db  43h ; C
                db  0Eh
                db 0A0h
                db  47h ; G
                db    0
                db 0F8h
                db  14h
                db  48h ; H
                db 0E8h
                db 0D2h
                db  0Ah
                db 0B0h
                db  0Dh
                db 0E8h
                db  35h ; 5
                db  0Eh
                db 0B0h
                db  0Dh
                db 0E8h
                db  30h ; 0
                db  0Eh
                db 0E8h
                db  18h
                db  0Ah
                db  20h
                db  20h
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  2Dh ; -
                db    0
                db 0A0h
                db  61h ; a
                db    0
                db 0F8h
                db  14h
                db  13h
                db 0E8h
                db 0B2h
                db  0Ah
                db 0B7h
                db    0
                db 0B3h
                db  1Dh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    3
                db  8Bh
                db 0F3h
                db 0E8h
                db    2
                db  0Ch
                db 0E8h
                db 0F3h
                db    9
                db  54h ; T
                db  4Fh ; O
                db  52h ; R
                db  43h ; C
                db  48h ; H
                db  45h ; E
                db  53h ; S
                db  2Dh ; -
                db    0
                db 0A0h
                db  64h ; d
                db    0
                db 0E8h
                db 0BAh
                db  0Dh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    4
                db  8Bh
                db 0F3h
                db 0E8h
                db 0E1h
                db  0Bh
                db 0E8h
                db 0D2h
                db    9
                db  20h
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  2Dh ; -
                db    0
                db 0A0h
                db  62h ; b
                db    0
                db 0F8h
                db  14h
                db  1Dh
                db 0E8h
                db  6Ch ; l
                db  0Ah
                db 0B7h
                db    0
                db 0B3h
                db  20h
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    4
                db  8Bh
                db 0F3h
                db 0E8h
                db 0BCh
                db  0Bh
                db 0E8h
                db 0ADh
                db    9
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db  53h ; S
                db  2Dh ; -
                db    0
                db 0A0h
                db  65h ; e
                db    0
                db 0E8h
                db  77h ; w
                db  0Dh
                db 0B7h
                db    0
                db 0B3h
                db    4
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    5
                db  8Bh
                db 0F3h
                db 0E8h
                db  9Eh
                db  0Bh
                db 0E8h
                db  8Fh
                db    9
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  2Dh ; -
                db    0
                db 0A0h
                db  63h ; c
                db    0
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db  2Ch ; ,
                db  0Ah
                db 0B7h
                db    0
                db 0B3h
                db  1Fh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    5
                db  8Bh
                db 0F3h
                db 0E8h
                db  7Ch ; |
                db  0Bh
                db 0E8h
                db  6Dh ; m
                db    9
                db  54h ; T
                db  4Fh ; O
                db  4Fh ; O
                db  4Ch ; L
                db  53h ; S
                db  2Dh ; -
                db    0
                db 0A0h
                db  66h ; f
                db    0
                db 0E8h
                db  36h ; 6
                db  0Dh
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    6
                db  8Bh
                db 0F3h
                db 0E8h
                db  5Dh ; ]
                db  0Bh
                db 0B0h
                db  3Eh ; >
                db 0E8h
                db 0F9h
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db  5Ch ; \
                db  0Dh
                db 0A0h
                db  4Bh ; K
                db    0
                db 0E8h
                db  17h
                db  0Dh
                db 0B7h
                db    0
                db 0B3h
                db    2
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    7
                db  8Bh
                db 0F3h
                db 0E8h
                db  3Eh ; >
                db  0Bh
                db 0B0h
                db  3Fh ; ?
                db 0E8h
                db 0DAh
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db  3Dh ; =
                db  0Dh
                db 0A0h
                db  4Ch ; L
                db    0
                db 0E8h
                db 0F8h
                db  0Ch
                db 0B7h
                db    0
                db 0B3h
                db  10h
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    6
                db  8Bh
                db 0F3h
                db 0E8h
                db  1Fh
                db  0Bh
                db 0B0h
                db  40h ; @
                db 0E8h
                db 0BBh
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db  1Eh
                db  0Dh
                db 0A0h
                db  4Dh ; M
                db    0
                db 0E8h
                db 0D9h
                db  0Ch
                db 0B7h
                db    0
                db 0B3h
                db  0Fh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    7
                db  8Bh
                db 0F3h
                db 0E8h
                db    0
                db  0Bh
                db 0B0h
                db  41h ; A
                db 0E8h
                db  9Ch
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db 0FFh
                db  0Ch
                db 0A0h
                db  4Eh ; N
                db    0
                db 0E8h
                db 0BAh
                db  0Ch
                db 0B7h
                db    0
                db 0B3h
                db  1Eh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    6
                db  8Bh
                db 0F3h
                db 0E8h
                db 0E1h
                db  0Ah
                db 0B0h
                db  42h ; B
                db 0E8h
                db  7Dh ; }
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db 0E0h
                db  0Ch
                db 0A0h
                db  4Fh ; O
                db    0
                db 0E8h
                db  9Bh
                db  0Ch
                db 0B7h
                db    0
                db 0B3h
                db  1Dh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    7
                db  8Bh
                db 0F3h
                db 0E8h
                db 0C2h
                db  0Ah
                db 0B0h
                db  43h ; C
                db 0E8h
                db  5Eh ; ^
                db    9
                db 0B0h
                db  2Dh ; -
                db 0E8h
                db 0C1h
                db  0Ch
                db 0A0h
                db  50h ; P
                db    0
                db 0E8h
                db  7Ch ; |
                db  0Ch
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db    9
                db  8Bh
                db 0F3h
                db 0E8h
                db 0A3h
                db  0Ah
                db 0E8h
                db  94h
                db    8
                db  57h ; W
                db  45h ; E
                db  41h ; A
                db  50h ; P
                db  4Fh ; O
                db  4Eh ; N
                db  53h ; S
                db  3Ah ; :
                db  20h
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  28h ; (
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0C7h
                db 0F8h
                db  14h
                db  13h
                db 0E8h
                db  1Bh
                db    9
                db 0E8h
                db  6Bh ; k
                db    8
                db  53h ; S
                db  2Dh ; -
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  76h ; v
                db    0
                db 0E8h
                db  2Fh ; /
                db  0Ch
                db 0B0h
                db  20h
                db 0E8h
                db  69h ; i
                db  0Ch
                db  47h ; G
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db  0Ah
                db  72h ; r
                db 0C8h
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  0Ch
                db  8Bh
                db 0F3h
                db 0E8h
                db  49h ; I
                db  0Ah
                db 0E8h
                db  3Ah ; :
                db    8
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  3Ah ; :
                db  20h
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  96h
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  27h ; '
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0C7h
                db 0F8h
                db  14h
                db  1Dh
                db 0E8h
                db 0C2h
                db    8
                db 0E8h
                db  12h
                db    8
                db  2Dh ; -
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db  96h
                db    0
                db 0E8h
                db 0D7h
                db  0Bh
                db 0B0h
                db  20h
                db 0E8h
                db  11h
                db  0Ch
                db  47h ; G
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db    7
                db  72h ; r
                db 0C9h
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  0Eh
                db  8Bh
                db 0F3h
                db 0E8h
                db 0F1h
                db    9
                db 0E8h
                db 0E2h
                db    7
                db  53h ; S
                db  50h ; P
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  53h ; S
                db  3Ah ; :
                db  20h
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  28h ; (
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0C7h
                db 0F8h
                db  14h
                db  24h ; $
                db 0E8h
                db  6Ah ; j
                db    8
                db 0E8h
                db 0BAh
                db    7
                db  53h ; S
                db  2Dh ; -
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0B6h
                db    0
                db 0E8h
                db  7Eh ; ~
                db  0Bh
                db 0B0h
                db  20h
                db 0E8h
                db 0B8h
                db  0Bh
                db  47h ; G
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db  0Ah
                db  72h ; r
                db 0C8h
                db 0B7h
                db    0
                db 0B3h
                db    1
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  12h
                db  8Bh
                db 0F3h
                db 0E8h
                db  98h
                db    9
                db 0E8h
                db  89h
                db    7
                db  49h ; I
                db  54h ; T
                db  45h ; E
                db  4Dh ; M
                db  53h ; S
                db  3Ah ; :
                db  20h
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0D6h
                db    0
                db  0Ah
                db 0C0h
                db  74h ; t
                db  33h ; 3
                db  8Bh
                db 0DFh
                db  88h
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0C7h
                db 0F8h
                db  14h
                db  2Eh ; .
                db 0E8h
                db  12h
                db    8
                db  80h
                db  3Eh ; >
                db  1Fh
                db    0
                db    3
                db  74h ; t
                db    5
                db 0E8h
                db  5Bh ; [
                db    7
                db  53h ; S
                db    0
                db 0E8h
                db  56h ; V
                db    7
                db  2Dh ; -
                db    0
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  1Fh
                db    0
                db  8Bh
                db 0FBh
                db  8Ah
                db  85h
                db 0D6h
                db    0
                db 0E8h
                db  1Bh
                db  0Bh
                db 0B0h
                db  20h
                db 0E8h
                db  55h ; U
                db  0Bh
                db  47h ; G
                db  8Bh
                db 0DFh
                db  80h
                db 0FBh
                db  10h
                db  72h ; r
                db 0BDh
                db 0C6h
                db    6
                db  2Eh ; .
                db    0
                db    0
                db 0C6h
                db    6
                db  4Fh ; O
                db    5
                db    0
                db 0C6h
                db    6
                db  31h ; 1
                db    0
                db  28h ; (
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0E8h
                db  8Eh
                db    6
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0F8h
                db 0E8h
                db  77h ; w
                db    1
                db 0E8h
                db  2Fh ; /
                db    8
                db 0E9h
                db  66h ; f
                db 0C1h
                db    0
                db    0
                db    0
                db 0C0h
                db  30h ; 0
                db  0Ch
                db    3
                db 0FFh
                db  11h
                db  88h
                db 0CCh
                db  33h ; 3
                db    0
                db    0
                db    0
screen_rows     dw 0B800h,0BA00h,0B805h,0BA05h,0B80Ah,0BA0Ah,0B80Fh,0BA0Fh
                                        ; DATA XREF: draw_tile:loc_14A89↓r
                                        ; xorSpriteDraw:loc_14C31↓r
                dw 0B814h,0BA14h,0B819h,0BA19h,0B81Eh,0BA1Eh,0B823h,0BA23h
                dw 0B828h,0BA28h,0B82Dh,0BA2Dh,0B832h,0BA32h,0B837h,0BA37h
                dw 0B83Ch,0BA3Ch,0B841h,0BA41h,0B846h,0BA46h,0B84Bh,0BA4Bh
                dw 0B850h,0BA50h,0B855h,0BA55h,0B85Ah,0BA5Ah,0B85Fh,0BA5Fh
                dw 0B864h,0BA64h,0B869h,0BA69h,0B86Eh,0BA6Eh,0B873h,0BA73h
                dw 0B878h,0BA78h,0B87Dh,0BA7Dh,0B882h,0BA82h,0B887h,0BA87h
                dw 0B88Ch,0BA8Ch,0B891h,0BA91h,0B896h,0BA96h,0B89Bh,0BA9Bh
                dw 0B8A0h,0BAA0h,0B8A5h,0BAA5h,0B8AAh,0BAAAh,0B8AFh,0BAAFh
                dw 0B8B4h,0BAB4h,0B8B9h,0BAB9h,0B8BEh,0BABEh,0B8C3h,0BAC3h
                dw 0B8C8h,0BAC8h,0B8CDh,0BACDh,0B8D2h,0BAD2h,0B8D7h,0BAD7h
                dw 0B8DCh,0BADCh,0B8E1h,0BAE1h,0B8E6h,0BAE6h,0B8EBh,0BAEBh
                dw 0B8F0h,0BAF0h,0B8F5h,0BAF5h,0B8FAh,0BAFAh,0B8FFh,0BAFFh
                dw 0B904h,0BB04h,0B909h,0BB09h,0B90Eh,0BB0Eh,0B913h,0BB13h
                dw 0B918h,0BB18h,0B91Dh,0BB1Dh,0B922h,0BB22h,0B927h,0BB27h
                dw 0B92Ch,0BB2Ch,0B931h,0BB31h,0B936h,0BB36h,0B93Bh,0BB3Bh
                dw 0B940h,0BB40h,0B945h,0BB45h,0B94Ah,0BB4Ah,0B94Fh,0BB4Fh
                dw 0B954h,0BB54h,0B959h,0BB59h,0B95Eh,0BB5Eh,0B963h,0BB63h
                dw 0B968h,0BB68h,0B96Dh,0BB6Dh,0B972h,0BB72h,0B977h,0BB77h
                dw 0B97Ch,0BB7Ch,0B981h,0BB81h,0B986h,0BB86h,0B98Bh,0BB8Bh
; [00000022 BYTES: COLLAPSED FUNCTION set_cga_mode. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000036 BYTES: COLLAPSED FUNCTION setPalette. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000039 BYTES: COLLAPSED FUNCTION draw_tile. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000039 BYTES: COLLAPSED FUNCTION draw_map_content. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000000B BYTES: COLLAPSED FUNCTION animate_water. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000000B BYTES: COLLAPSED FUNCTION animate_forcefield. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000002A BYTES: COLLAPSED FUNCTION animate_tile. PRESS CTRL-NUMPAD+ TO EXPAND]

; =============== S U B R O U T I N E =======================================


sub_14B26       proc near               ; CODE XREF: sub_10E70+14↑p
                                        ; sub_10E70+1A↑p ...
                push    ax
                mov     ax, 0B800h
                call    sub_14B35
                mov     ax, 0BA00h
                call    sub_14B35
                pop     ax
                retn
sub_14B26       endp


; =============== S U B R O U T I N E =======================================


sub_14B35       proc near               ; CODE XREF: sub_14B26+4↑p
                                        ; sub_14B26+A↑p
                push    bx
                push    es
                mov     es, ax
                mov     ax, 0FFFFh
                mov     bx, 0

loc_14B3F:                              ; CODE XREF: sub_14B35+14↓j
                xor     es:[bx], ax
                add     bx, 2
                cmp     bx, 1900h
                jnz     short loc_14B3F
                pop     es
                pop     bx
                retn
sub_14B35       endp


; =============== S U B R O U T I N E =======================================


sub_14B4E       proc near               ; CODE XREF: sg01a2:042F↑p
                                        ; sg01a2:051C↑p ...
                push    ax
                mov     ax, 0B800h
                call    sub_14B5D
                mov     ax, 0BA00h
                call    sub_14B5D
                pop     ax
                retn
sub_14B4E       endp


; =============== S U B R O U T I N E =======================================


sub_14B5D       proc near               ; CODE XREF: sub_14B4E+4↑p
                                        ; sub_14B4E+A↑p
                push    bx
                push    es
                mov     es, ax
                mov     ax, 0
                mov     bx, 0

loc_14B67:                              ; CODE XREF: sub_14B5D+14↓j
                mov     es:[bx], ax
                add     bx, 2
                cmp     bx, 1900h
                jnz     short loc_14B67
                pop     es
                pop     bx
                retn
sub_14B5D       endp


; =============== S U B R O U T I N E =======================================


sub_14B76       proc near               ; CODE XREF: sub_167B3+A3↓p
                                        ; sub_167B3+E6↓p
                push    es
                mov     ah, 0
                mov     al, byte_17889
                add     ax, word_178CF
                mov     di, ax
                mov     ah, 0
                mov     al, byte_1788A
                add     ax, word_178D1
                mov     si, ax
                mov     bx, di
                and     bx, 3
                shr     di, 1
                shr     di, 1
                shl     si, 1
                mov     ax, cs:[si+48DCh]
                mov     es, ax
                mov     al, cs:[bx+48D0h]
                or      es:[di], al
                pop     es
                retn
sub_14B76       endp

; ---------------------------------------------------------------------------
                db    6
                db 0B4h
                db    0
                db 0A0h
                db  79h ; y
                db    4
                db    3
                db    6
                db 0BFh
                db    4
                db  8Bh
                db 0F8h
                db 0B4h
                db    0
                db 0A0h
                db  7Ah ; z
                db    4
                db    3
                db    6
                db 0C1h
                db    4
                db  8Bh
                db 0F0h
                db  8Bh
                db 0DFh
                db  81h
                db 0E3h
                db    3
                db    0
                db 0D1h
                db 0EFh
                db 0D1h
                db 0EFh
                db 0D1h
                db 0E6h
                db  2Eh ; .
                db  8Bh
                db  84h
                db 0DCh
                db  48h ; H
                db  8Eh
                db 0C0h
                db  2Eh ; .
                db  8Ah
                db  87h
                db 0D0h
                db  48h ; H
                db  34h ; 4
                db 0FFh
                db  26h ; &
                db  20h
                db    5
                db    7
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_14BE0       proc near               ; CODE XREF: sub_168A3+19↓p
                push    ax
                push    bx
                push    bp
                push    cx
                push    di
                push    es
                add     ax, word_178CF
                shr     ax, 1
                shr     ax, 1
                mov     di, ax
                mov     cl, cs:[bp+48D4h]
                add     bx, word_178D1
                shl     bx, 1
                mov     ax, cs:[bx+48DCh]
                mov     es, ax
                mov     es:[di], cl
                mov     es:[di+50h], cl
                mov     ax, cs:[bx+48DEh]
                mov     es, ax
                mov     es:[di], cl
                mov     es:[di+50h], cl
                pop     es
                pop     di
                pop     cx
                pop     bp
                pop     bx
                pop     ax
                retn
sub_14BE0       endp


; =============== S U B R O U T I N E =======================================


xorSpriteDraw   proc near               ; CODE XREF: xorDrawCircleAt+5↓p
                push    dx
                push    di
                push    si
                push    es
                mov     si, cx
                shr     ax, 1
                shr     ax, 1
                mov     di, ax
                shl     bx, 1
                mov     dl, 10h
                add     si, 2

loc_14C31:                              ; CODE XREF: xorSpriteDraw+30↓j
                mov     ax, cs:screen_rows[bx]
                mov     es, ax
                mov     ax, cs:[si]
                xor     es:[di], ax
                mov     ax, cs:[si+2]
                xor     es:[di+2], ax
                add     si, 4
                add     bx, 2
                dec     dl
                jnz     short loc_14C31
                pop     es
                pop     si
                pop     di
                pop     dx
                retn
xorSpriteDraw   endp

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
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  57h ; W
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  2Eh ; .
                db    0
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  41h ; A
                db  20h
                db  4Dh ; M
                db  41h ; A
                db  52h ; R
                db  53h ; S
                db  48h ; H
                db  2Eh ; .
                db    0
                db  4Fh ; O
                db  4Eh ; N
                db  20h
                db  47h ; G
                db  52h ; R
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  2Eh ; .
                db    0
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  57h ; W
                db  4Fh ; O
                db  4Fh ; O
                db  44h ; D
                db  53h ; S
                db  2Eh ; .
                db    0
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  4Dh ; M
                db  54h ; T
                db  53h ; S
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  56h ; V
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db  41h ; A
                db  47h ; G
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  43h ; C
                db  41h ; A
                db  53h ; S
                db  54h ; T
                db  4Ch ; L
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  44h ; D
                db  55h ; U
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  4Fh ; O
                db  4Eh ; N
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  53h ; S
                db  49h ; I
                db  47h ; G
                db  4Eh ; N
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  46h ; F
                db  52h ; R
                db  49h ; I
                db  47h ; G
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  52h ; R
                db  4Fh ; O
                db  43h ; C
                db  4Bh ; K
                db  45h ; E
                db  54h ; T
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  52h ; R
                db  4Dh ; M
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  2Eh ; .
                db    0
                db  4Eh ; N
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  20h
                db  41h ; A
                db  20h
                db  48h ; H
                db  4Fh ; O
                db  4Ch ; L
                db  45h ; E
                db  2Eh ; .
                db    0
                db  4Fh ; O
                db  4Eh ; N
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  42h ; B
                db  42h ; B
                db  4Ch ; L
                db  45h ; E
                db  2Eh ; .
                db    0
                db  48h ; H
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  53h ; S
                db    0
                db  44h ; D
                db  41h ; A
                db  47h ; G
                db  47h ; G
                db  45h ; E
                db  52h ; R
                db    0
                db  4Dh ; M
                db  41h ; A
                db  43h ; C
                db  45h ; E
                db    0
                db  41h ; A
                db  58h ; X
                db  45h ; E
                db    0
                db  42h ; B
                db  4Fh ; O
                db  57h ; W
                db    0
                db  53h ; S
                db  57h ; W
                db  4Fh ; O
                db  52h ; R
                db  44h ; D
                db    0
                db  47h ; G
                db  52h ; R
                db  45h ; E
                db  41h ; A
                db  54h ; T
                db  20h
                db  53h ; S
                db  57h ; W
                db  4Fh ; O
                db  52h ; R
                db  44h ; D
                db    0
                db  4Ch ; L
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  20h
                db  53h ; S
                db  57h ; W
                db  4Fh ; O
                db  52h ; R
                db  44h ; D
                db    0
                db  50h ; P
                db  48h ; H
                db  41h ; A
                db  53h ; S
                db  45h ; E
                db  52h ; R
                db    0
                db  51h ; Q
                db  55h ; U
                db  49h ; I
                db  43h ; C
                db  4Bh ; K
                db  20h
                db  53h ; S
                db  57h ; W
                db  4Fh ; O
                db  52h ; R
                db  44h ; D
                db    0
                db  53h ; S
                db  4Bh ; K
                db  49h ; I
                db  4Eh ; N
                db    0
                db  43h ; C
                db  4Ch ; L
                db  4Fh ; O
                db  54h ; T
                db  48h ; H
                db    0
                db  4Ch ; L
                db  45h ; E
                db  41h ; A
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db    0
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  49h ; I
                db  4Eh ; N
                db    0
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  54h ; T
                db  45h ; E
                db    0
                db  52h ; R
                db  45h ; E
                db  46h ; F
                db  4Ch ; L
                db  45h ; E
                db  43h ; C
                db  54h ; T
                db    0
                db  50h ; P
                db  4Fh ; O
                db  57h ; W
                db  45h ; E
                db  52h ; R
                db    0
                db  4Eh ; N
                db  4Fh ; O
                db  4Eh ; N
                db  45h ; E
                db    0
                db  4Ch ; L
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db    0
                db  44h ; D
                db  4Fh ; O
                db  57h ; W
                db  4Eh ; N
                db  20h
                db  4Ch ; L
                db  41h ; A
                db  44h ; D
                db  44h ; D
                db  45h ; E
                db  52h ; R
                db    0
                db  55h ; U
                db  50h ; P
                db  20h
                db  4Ch ; L
                db  41h ; A
                db  44h ; D
                db  44h ; D
                db  45h ; E
                db  52h ; R
                db    0
                db  50h ; P
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  57h ; W
                db  41h ; A
                db  4Ch ; L
                db  4Ch ; L
                db    0
                db  53h ; S
                db  55h ; U
                db  52h ; R
                db  46h ; F
                db  41h ; A
                db  43h ; C
                db  45h ; E
                db    0
                db  50h ; P
                db  52h ; R
                db  41h ; A
                db  59h ; Y
                db  45h ; E
                db  52h ; R
                db    0
                db  4Dh ; M
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  43h ; C
                db  20h
                db  4Dh ; M
                db  49h ; I
                db  53h ; S
                db  53h ; S
                db  49h ; I
                db  4Ch ; L
                db  45h ; E
                db    0
                db  42h ; B
                db  4Ch ; L
                db  49h ; I
                db  4Eh ; N
                db  4Bh ; K
                db    0
                db  4Bh ; K
                db  49h ; I
                db  4Ch ; L
                db  4Ch ; L
                db    0
                db  52h ; R
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db    0
                db  57h ; W
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db    0
                db  53h ; S
                db  54h ; T
                db  41h ; A
                db  46h ; F
                db  46h ; F
                db    0
                db  42h ; B
                db  4Fh ; O
                db  4Fh ; O
                db  54h ; T
                db  53h ; S
                db    0
                db  43h ; C
                db  4Ch ; L
                db  4Fh ; O
                db  41h ; A
                db  4Bh ; K
                db    0
                db  48h ; H
                db  45h ; E
                db  4Ch ; L
                db  4Dh ; M
                db    0
                db  47h ; G
                db  45h ; E
                db  4Dh ; M
                db    0
                db  41h ; A
                db  4Eh ; N
                db  4Bh ; K
                db  48h ; H
                db    0
                db  52h ; R
                db  45h ; E
                db  44h ; D
                db  20h
                db  47h ; G
                db  45h ; E
                db  4Dh ; M
                db    0
                db  53h ; S
                db  4Bh ; K
                db  55h ; U
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  4Bh ; K
                db  45h ; E
                db  59h ; Y
                db    0
                db  47h ; G
                db  52h ; R
                db  45h ; E
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  47h ; G
                db  45h ; E
                db  4Dh ; M
                db    0
                db  42h ; B
                db  52h ; R
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  20h
                db  42h ; B
                db  55h ; U
                db  54h ; T
                db  54h ; T
                db  4Fh ; O
                db  4Eh ; N
                db    0
                db  42h ; B
                db  4Ch ; L
                db  55h ; U
                db  45h ; E
                db  20h
                db  54h ; T
                db  41h ; A
                db  53h ; S
                db  53h ; S
                db  4Ch ; L
                db  45h ; E
                db    0
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  49h ; I
                db  4Eh ; N
                db    0
                db  47h ; G
                db  52h ; R
                db  45h ; E
                db  45h ; E
                db  4Eh ; N
                db  20h
                db  49h ; I
                db  44h ; D
                db  4Fh ; O
                db  4Ch ; L
                db    0
                db  54h ; T
                db  52h ; R
                db  49h ; I
                db  2Dh ; -
                db  4Ch ; L
                db  49h ; I
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  55h ; U
                db  4Dh ; M
                db    0
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  45h ; E
                db  4Eh ; N
                db  47h ; G
                db  54h ; T
                db  48h ; H
                db    0
                db  41h ; A
                db  47h ; G
                db  49h ; I
                db  4Ch ; L
                db  49h ; I
                db  54h ; T
                db  59h ; Y
                db    0
                db  53h ; S
                db  54h ; T
                db  41h ; A
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  41h ; A
                db    0
                db  43h ; C
                db  48h ; H
                db  41h ; A
                db  52h ; R
                db  49h ; I
                db  53h ; S
                db  4Dh ; M
                db  41h ; A
                db    0
                db  57h ; W
                db  49h ; I
                db  53h ; S
                db  44h ; D
                db  4Fh ; O
                db  4Dh ; M
                db    0
                db  49h ; I
                db  4Eh ; N
                db  54h ; T
                db  45h ; E
                db  4Ch ; L
                db  4Ch ; L
                db  2Eh ; .
                db    0
                db  48h ; H
                db  55h ; U
                db  4Dh ; M
                db  41h ; A
                db  4Eh ; N
                db    0
                db  45h ; E
                db  4Ch ; L
                db  46h ; F
                db    0
                db  44h ; D
                db  57h ; W
                db  41h ; A
                db  52h ; R
                db  46h ; F
                db    0
                db  48h ; H
                db  4Fh ; O
                db  42h ; B
                db  42h ; B
                db  49h ; I
                db  54h ; T
                db    0
                db  46h ; F
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db    0
                db  43h ; C
                db  4Ch ; L
                db  45h ; E
                db  52h ; R
                db  49h ; I
                db  43h ; C
                db    0
                db  57h ; W
                db  49h ; I
                db  5Ah ; Z
                db  41h ; A
                db  52h ; R
                db  44h ; D
                db    0
                db  54h ; T
                db  48h ; H
                db  49h ; I
                db  45h ; E
                db  46h ; F
                db    0
_picture_int21_function db 0            ; DATA XREF: access_file+B↓w
                                        ; access_file+A0↓r
_picture_int21_cx dw 0                  ; DATA XREF: access_file+10↓w
                                        ; access_file+95↓r
_picture_int21_dx dw 0                  ; DATA XREF: access_file+15↓w
                                        ; access_file:loc_15343↓r
KEYCODES        db 48h                  ; DATA XREF: keypress_check:loc_14FBE↓r
byte_14F0A      db 0Eh                  ; DATA XREF: keypress_check:loc_14FCC↓r
                db  50h ; P
                db  19h
                db  4Dh ; M
                db    5
                db  4Bh ; K
                db  17h

; =============== S U B R O U T I N E =======================================


sub_14F11       proc near               ; CODE XREF: canMoveToTile-191F↑p
                                        ; canMoveToTile-13A7↑p
                stc
                mov     al, _mapX
                sub     al, [di+137h]
                add     al, al
                add     al, al
                call    sub_150D8
                mov     _circleDeltaX, al
                stc
                mov     al, _mapY
                sub     al, [di+157h]
                add     al, al
                add     al, al
                call    sub_150D8
                mov     _circleDeltaY, al
                retn
sub_14F11       endp


; =============== S U B R O U T I N E =======================================


sub_14F36       proc near               ; CODE XREF: canMoveToTile:loc_10AA7↑p
                stc
                mov     al, _mapX
                sub     al, [di+137h]
                mov     _circleDeltaX, al
                stc
                mov     al, _mapY
                sub     al, [di+157h]
                mov     _circleDeltaY, al
                retn
sub_14F36       endp


; =============== S U B R O U T I N E =======================================


keypress_check  proc near               ; CODE XREF: start_+75↑p
                                        ; start_+100↑p ...
                push    bx
                mov     ah, 1
                int     16h             ; KEYBOARD - CHECK BUFFER, DO NOT CLEAR
                                        ; Return: ZF clear if character in buffer
                                        ; AH = scan code, AL = character
                                        ; ZF set if no character in buffer
                mov     ah, 0
                jz      short loc_14FD3
                int     16h             ; KEYBOARD - READ CHAR FROM BUFFER, WAIT IF EMPTY
                                        ; Return: AH = scan code, AL = character
                cmp     al, 5
                jz      short loc_14F64
                cmp     al, 14
                jz      short loc_14F64
                cmp     al, 23
                jnz     short loc_14F69

loc_14F64:                              ; CODE XREF: keypress_check+D↑j
                                        ; keypress_check+11↑j
                mov     al, 0
                jmp     short loc_14FD1
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_14F69:                              ; CODE XREF: keypress_check+15↑j
                push    ax

loc_14F6A:                              ; CODE XREF: keypress_check+27↓j
                mov     ah, 1
                int     16h             ; KEYBOARD - CHECK BUFFER, DO NOT CLEAR
                                        ; Return: ZF clear if character in buffer
                                        ; AH = scan code, AL = character
                                        ; ZF set if no character in buffer
                jz      short loc_14F76
                mov     ah, 0
                int     16h             ; KEYBOARD - READ CHAR FROM BUFFER, WAIT IF EMPTY
                                        ; Return: AH = scan code, AL = character
                jmp     short loc_14F6A
; ---------------------------------------------------------------------------

loc_14F76:                              ; CODE XREF: keypress_check+21↑j
                pop     ax
                cmp     al, 13h
                jnz     short loc_14F85
                xor     byte_1795D, 0FFh
                mov     ah, 0
                jmp     short loc_14FDD
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_14F85:                              ; CODE XREF: keypress_check+2C↑j
                cmp     al, 0
                jnz     short loc_14FD1
                cmp     ax, 7400h
                jnz     short loc_14FA0
; Ctrl-Right: Speed Up Game
                mov     ax, word ptr player._gameSpeed?
                sub     ax, 200h
                cmp     ax, speed_divisor
                jb      short loc_14FB6
                mov     word ptr player._gameSpeed?, ax
                jmp     short loc_14FB3
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_14FA0:                              ; CODE XREF: keypress_check+3F↑j
                cmp     ax, 7300h
                jnz     short loc_14FBB
; Ctrl+Left: Slow Down Game
                mov     ax, word ptr player._gameSpeed?
                add     ax, 200h
                cmp     ax, 3A00h
                ja      short loc_14FB6
                mov     word ptr player._gameSpeed?, ax

loc_14FB3:                              ; CODE XREF: keypress_check+50↑j
                call    setup_speed_array

loc_14FB6:                              ; CODE XREF: keypress_check+4B↑j
                                        ; keypress_check+61↑j
                mov     ah, 0
                jmp     short loc_14FDD
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_14FBB:                              ; CODE XREF: keypress_check+56↑j
                mov     bx, 6

loc_14FBE:                              ; CODE XREF: keypress_check+7D↓j
                cmp     ah, cs:KEYCODES[bx]
                jz      short loc_14FCC
                sub     bx, 2
                js      short loc_14FD1
                jmp     short loc_14FBE
; ---------------------------------------------------------------------------

loc_14FCC:                              ; CODE XREF: keypress_check+76↑j
                mov     al, cs:byte_14F0A[bx]

loc_14FD1:                              ; CODE XREF: keypress_check+19↑j
                                        ; keypress_check+3A↑j ...
                mov     ah, 0FFh

loc_14FD3:                              ; CODE XREF: keypress_check+7↑j
                cmp     al, 'a'
                jb      short loc_14FDD
                cmp     al, 'z'
                ja      short loc_14FDD
                and     al, not 20h

loc_14FDD:                              ; CODE XREF: keypress_check+35↑j
                                        ; keypress_check+6B↑j ...
                pop     bx
                retn
keypress_check  endp


; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

write_string    proc near               ; CODE XREF: start_+4B↑p
                                        ; start_+5E↑p ...
                call    set_cursor_position
                pop     ax
                mov     word_1743B, ax
                mov     si, 0
                jmp     short loc_14FF0
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_14FEC:                              ; CODE XREF: write_string+1F↓j
                inc     word_1743B

loc_14FF0:                              ; CODE XREF: write_string+A↑j
                mov     bx, word_1743B
                mov     al, cs:[bx+si]
                cmp     al, 0
                jz      short loc_15000
                call    write_character
                jmp     short loc_14FEC
; ---------------------------------------------------------------------------

loc_15000:                              ; CODE XREF: write_string+1A↑j
                inc     bx
                push    bx
                retn
write_string    endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================


write_character proc near               ; CODE XREF: start_+294↑p
                                        ; write_string+1C↑p ...
                push    bx
                push    cx
                push    dx
                push    ax
                and     al, 7Fh
                cmp     al, 0Dh
                jnz     short loc_15016

loc_1500D:                              ; CODE XREF: write_character+52↓j
                mov     al, byte_1795F
                mov     text_x, al
                jmp     short loc_1501A
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15016:                              ; CODE XREF: write_character+8↑j
                cmp     al, 0Ah
                jnz     short loc_1503D

loc_1501A:                              ; CODE XREF: write_character+10↑j
                inc     text_y
                cmp     text_y, 18h
                jnz     short loc_15057
                mov     al, 1
                mov     cx, 1400h
                mov     dh, 17h
                mov     dl, text_width?
                mov     bh, 0
                mov     ah, 6
                int     10h             ; - VIDEO - SCROLL PAGE UP
                                        ; AL = number of lines to scroll window (0 = blank whole window)
                                        ; BH = attributes to be used on blanked lines
                                        ; CH,CL = row,column of upper left corner of window to scroll
                                        ; DH,DL = row,column of lower right corner of window
                dec     text_y
                jmp     short loc_15057
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1503D:                              ; CODE XREF: write_character+15↑j
                mov     ah, 9
                mov     cx, 1
                mov     bl, text_color
                mov     bh, 0
                int     10h             ; - VIDEO - WRITE ATTRIBUTES/CHARACTERS AT CURSOR POSITION
                                        ; AL = character, BH = display page
                                        ; BL = attributes of character (alpha modes) or color (graphics modes)
                                        ; CX = number of times to write character
                inc     text_x
                mov     al, text_width?
                cmp     al, text_x
                jb      short loc_1500D

loc_15057:                              ; CODE XREF: write_character+20↑j
                                        ; write_character+37↑j
                call    set_cursor_position
                pop     ax
                pop     dx
                pop     cx
                pop     bx
                retn
write_character endp

; ---------------------------------------------------------------------------
                db 0A0h
                db  46h ; F
                db    0
                db  3Ch ; <
                db  4Dh ; M
                db  75h ; u
                db    9
                db 0E8h
                db  76h ; v
                db 0FFh
                db  4Dh ; M
                db  41h ; A
                db  4Ch ; L
                db  45h ; E
                db    0
                db 0C3h
                db 0E8h
                db  6Dh ; m
                db 0FFh
                db  46h ; F
                db  45h ; E
                db  4Dh ; M
                db  41h ; A
                db  4Ch ; L
                db  45h ; E
                db    0
                db 0C3h

; =============== S U B R O U T I N E =======================================


write_player_name proc near             ; CODE XREF: sub_10F8E-305↑p
                                        ; sg01a2:425F↑p
                mov     bx, 0

loc_1507D:                              ; CODE XREF: write_player_name+F↓j
                mov     al, byte ptr player._name[bx]
                cmp     al, 0
                jz      short locret_1508B
                call    write_character
                inc     bx
                jmp     short loc_1507D
; ---------------------------------------------------------------------------

locret_1508B:                           ; CODE XREF: write_player_name+9↑j
                retn
write_player_name endp

; ---------------------------------------------------------------------------
                db 0B4h
                db    0
                db  8Bh
                db 0F0h
                db 0BFh
                db    0
                db    0
                db  2Eh ; .
                db  80h
                db 0BDh
                db  60h ; `
                db  4Ch ; L
                db    0
                db  74h ; t
                db    3
                db  47h ; G
                db 0EBh
                db 0F5h
                db  4Eh ; N
                db  74h ; t
                db    2
                db 0EBh
                db 0F8h
                db  47h ; G
                db  2Eh ; .
                db  8Ah
                db  85h
                db  60h ; `
                db  4Ch ; L
                db  3Ch ; <
                db    0
                db  74h ; t
                db    5
                db 0E8h
                db  53h ; S
                db 0FFh
                db 0EBh
                db 0F1h
                db 0C3h

; =============== S U B R O U T I N E =======================================


get_player_tile proc near               ; CODE XREF: sg01a2:0845↑p
                                        ; canMoveToTile-1738↑p ...
                push    bx
                push    cx
                mov     ah, 0
                mov     al, _playerY
                mov     cl, 6
                shl     ax, cl
                mov     bh, 0
                mov     bl, _playerX
                add     bx, ax
                add     bx, map_ptr
                mov     current_tile_ptr, bx
                mov     al, [bx]
                mov     si, 0
                pop     cx
                pop     bx
                or      al, al
                retn
get_player_tile endp


; =============== S U B R O U T I N E =======================================


sub_150D8       proc near               ; CODE XREF: canMoveToTile-137C↑p
                                        ; canMoveToTile-1367↑p ...
                cmp     al, 0
                jz      short locret_150E3
                jns     short loc_150E1
                mov     al, 0FFh
                retn
; ---------------------------------------------------------------------------

loc_150E1:                              ; CODE XREF: sub_150D8+4↑j
                mov     al, 1

locret_150E3:                           ; CODE XREF: sub_150D8+2↑j
                                        ; sub_150EF+2↓j
                retn
sub_150D8       endp

; ---------------------------------------------------------------------------
                db  3Ch ; <
                db    0
                db  74h ; t
                db 0FBh
                db  3Ch ; <
                db  20h
                db  72h ; r
                db 0F5h
                db 0B0h
                db 0FFh
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_150EF       proc near               ; CODE XREF: canMoveToTile-1970↑p
                                        ; canMoveToTile-1966↑p ...
                cmp     al, 80h
                jb      short locret_150E3
                xor     al, 0FFh
                clc
                adc     al, 1
                retn
sub_150EF       endp


; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

write_stats     proc near               ; CODE XREF: sg01a2:086C↑p
                                        ; canMoveToTile:loc_10C1C↑p ...
                mov     al, 40
                mov     text_width?, al
                mov     text_x, 30
                mov     al, 20
                mov     text_y, al
                call    write_string    ; H.P.=
; ---------------------------------------------------------------------------
aHP             db 'H.P.=',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, player._hp+1
                mov     di, bx
                mov     al, player._hp
                or      al, al
                jnz     short loc_15123
                call    set_reverse_text_color

loc_15123:                              ; CODE XREF: write_stats+25↑j
                call    write_two_numbers
                call    set_normal_text_color
                mov     al, 1Eh
                mov     text_x, al
                mov     text_y, 15h
                call    write_string    ; FOOD=
; ---------------------------------------------------------------------------
aFood           db 'FOOD=',0
write_stats     endp

; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, player._food+1
                mov     di, bx
                mov     al, player._food
                or      al, al
                jnz     short loc_1514E
                call    set_reverse_text_color

loc_1514E:                              ; CODE XREF: sg01a2:5149↑j
                call    write_two_numbers
                call    set_normal_text_color
                mov     al, 1Eh
                mov     text_x, al
                mov     text_y, 16h
                call    write_string    ; EXP.=
; ---------------------------------------------------------------------------
aExp            db 'EXP.=',0
; ---------------------------------------------------------------------------
                mov     al, player._experience
                mov     bh, 0
                mov     bl, player._experience+1
                mov     di, bx
                call    write_two_numbers
                mov     al, 1Eh
                mov     text_x, al
                mov     text_y, 17h
                call    write_string    ; GOLD=
; ---------------------------------------------------------------------------
aGold           db 'GOLD=',0
; ---------------------------------------------------------------------------
                mov     al, player._gold
                mov     bh, 0
                mov     bl, player._gold+1
                mov     di, bx
                call    write_two_numbers
                mov     al, 1Dh
                mov     text_width?, al
                mov     di, 0
                mov     si, 17h
                call    set_text_pos
                retn

; =============== S U B R O U T I N E =======================================


write_two_numbers proc near             ; CODE XREF: sg01a2:42DF↑p
                                        ; write_stats:loc_15123↑p ...
                push    ax
                call    write_number
                mov     ax, di
                call    write_number
                pop     ax
                retn
write_two_numbers endp

; ---------------------------------------------------------------------------

get_number:                             ; CODE XREF: sg01a2:51B6↓j
                                        ; sg01a2:51BA↓j ...
                call    keypress_check
                cmp     ah, 0FFh
                jnz     short get_number
                cmp     al, 30h ; '0'
                jb      short get_number
                cmp     al, 39h ; '9'
                ja      short get_number
                call    write_character
                sub     al, 30h ; '0'
                add     al, al
                add     al, al
                add     al, al
                add     al, al
                mov     _sleepFlag2?, al

loc_151D0:                              ; CODE XREF: sg01a2:51D6↓j
                                        ; sg01a2:51DA↓j ...
                call    keypress_check
                cmp     ah, 0FFh
                jnz     short loc_151D0
                cmp     al, 30h ; '0'
                jb      short loc_151D0
                cmp     al, 39h ; '9'
                ja      short loc_151D0
; ---------------------------------------------------------------------------
                db 0E8h
byte_151E1      db 20h                  ; DATA XREF: seg002:003B↓r
                db 0FEh
                db  2Ch ; ,
                db  30h ; 0
                db 0F8h
                db  12h
                db    6
                db  27h ; '
                db    0
                db 0C3h

; =============== S U B R O U T I N E =======================================


; void __usercall set_text_pos(int x@<di>, int y@<si>)
set_text_pos    proc near               ; CODE XREF: start_+48↑p
                                        ; start_+5B↑p ...
                push    bx
                mov     bx, di
                mov     text_x, bl
                mov     bx, si
                mov     text_y, bl
                call    set_cursor_position
                pop     bx
                retn
set_text_pos    endp


; =============== S U B R O U T I N E =======================================


set_cursor_position proc near           ; CODE XREF: set_cga_mode+10↑p
                                        ; write_string↑p ...
                push    ax
                push    bx
                push    cx
                push    dx
                mov     dh, text_y
                mov     dl, text_x
                mov     cx, 1
                mov     bh, 0
                mov     ah, 2
                int     10h             ; - VIDEO - SET CURSOR POSITION
                                        ; DH,DL = row, column (0,0 = upper left)
                                        ; BH = page number
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
set_cursor_position endp


; =============== S U B R O U T I N E =======================================


sub_15217       proc near               ; CODE XREF: canMoveToTile-195F↑p
                                        ; canMoveToTile:loc_10B62↑p ...
                push    ax
                push    bx
                stc
                mov     al, _timer2
                adc     al, _timer5
                adc     al, _timer6
                mov     _timer1, al
                mov     bx, 4

loc_1522B:                              ; CODE XREF: sub_15217+1D↓j
                mov     al, [bx+23Fh]
                mov     [bx+240h], al
                dec     bx
                jns     short loc_1522B
                pop     bx
                pop     ax
                mov     al, _timer1
                or      al, al
                retn
sub_15217       endp


; =============== S U B R O U T I N E =======================================


xorDrawCircle   proc near               ; CODE XREF: sub_10F8E+24↑p
                                        ; sub_10F8E+2A↑p ...
                nop
                mov     al, _circleDeltaY
                cmp     al, 1
                jz      short xorSpriteDrawDown
                cmp     al, 0FFh
                jz      short xorSpriteDrawUp
                mov     al, _circleDeltaX
                cmp     al, 1
                jz      short xorSpriteDrawRight
                cmp     al, 0FFh
                jz      short xorSpriteDrawLeft
xorDrawCircle   endp


; =============== S U B R O U T I N E =======================================


xorSpriteDrawCenter proc near           ; CODE XREF: canMoveToTile-185D↑p
                                        ; canMoveToTile-1857↑p
                mov     ax, 144
                mov     bx, 80
                jmp     short xorDrawCircleAt
xorSpriteDrawCenter endp

; ---------------------------------------------------------------------------
                db  90h

; =============== S U B R O U T I N E =======================================


xorSpriteDrawDown proc near             ; CODE XREF: xorDrawCircle+6↑j
                mov     ax, 144
                mov     bx, 96
                jmp     short xorDrawCircleAt
xorSpriteDrawDown endp

; ---------------------------------------------------------------------------
                db  90h

; =============== S U B R O U T I N E =======================================


xorSpriteDrawUp proc near               ; CODE XREF: xorDrawCircle+A↑j
                mov     ax, 144
                mov     bx, 64
                jmp     short xorDrawCircleAt
xorSpriteDrawUp endp

; ---------------------------------------------------------------------------
                db  90h

; =============== S U B R O U T I N E =======================================


xorSpriteDrawRight proc near            ; CODE XREF: xorDrawCircle+11↑j
                mov     ax, 160
                mov     bx, 80
                jmp     short xorDrawCircleAt
xorSpriteDrawRight endp

; ---------------------------------------------------------------------------
                db  90h

; =============== S U B R O U T I N E =======================================


xorSpriteDrawLeft proc near             ; CODE XREF: xorDrawCircle+15↑j
                mov     ax, 128
                mov     bx, 80
xorSpriteDrawLeft endp


; =============== S U B R O U T I N E =======================================


xorDrawCircleAt proc near               ; CODE XREF: xorSpriteDrawCenter+6↑j
                                        ; xorSpriteDrawDown+6↑j ...
                db      2Eh
                lea     cx, spriteCircle
                call    xorSpriteDraw
                retn
xorDrawCircleAt endp

; ---------------------------------------------------------------------------
spriteCircle    db      4,   10h,     0,   0Fh,  0F0h,2 dup(     0),2 dup(  0FFh),     0
                                        ; DATA XREF: xorDrawCircleAt↑o
                db      3,2 dup(  0FFh),  0C0h,   0Fh,2 dup(  0FFh),  0F0h,   0Fh,2 dup(  0FFh)
                db   0F0h,   3Fh,2 dup(  0FFh),  0FCh,   3Fh,2 dup(  0FFh),  0FCh,   3Fh
                db 2 dup(  0FFh),  0FCh,   3Fh,2 dup(  0FFh),  0FCh,   3Fh,2 dup(  0FFh),  0FCh
                db    3Fh,2 dup(  0FFh),  0FCh,   0Fh,2 dup(  0FFh),  0F0h,   0Fh,2 dup(  0FFh)
                db   0F0h,     3,2 dup(  0FFh),  0C0h,     0,2 dup(  0FFh),2 dup(     0),   0Fh
                db   0F0h,     0

; =============== S U B R O U T I N E =======================================


access_file     proc near               ; CODE XREF: start_+117↑p
                                        ; start_+2C0↑p ...
                pop     bx
                add     bx, 8           ; new return address
                push    bx
                push    ax
                push    cx
                push    dx
                push    si
                push    di
                push    es
                mov     cs:_picture_int21_function, ah
                mov     cs:_picture_int21_cx, cx
                mov     cs:_picture_int21_dx, dx
                mov     si, 36

loc_152E7:                              ; CODE XREF: access_file+23↓j
                mov     byte ptr _picData.drive[si], 0
                dec     si
                jns     short loc_152E7
                mov     si, bx
                mov     bx, 8
                sub     si, bx

loc_152F6:                              ; CODE XREF: access_file+34↓j
                dec     bx
                mov     al, cs:[bx+si]
                mov     _picData.filename[bx], al
                jnz     short loc_152F6
                mov     _picData.extension, 20h ; ' '
                mov     _picData.extension+1, 20h ; ' '
                mov     _picData.extension+2, 20h ; ' '

loc_1530F:                              ; CODE XREF: access_file+77↓j
                mov     ah, 0Fh
                lea     dx, _picData
                int     21h             ; DOS - OPEN DISK FILE
                                        ; DS:DX -> FCB
                                        ; Return: AL = 00h file found, FFh file not found
                cmp     al, 0
                jz      short loc_15343
                call    write_string    ;                WRONG DISK
; ---------------------------------------------------------------------------
aWrongDisk      db 0Dh,'               WRONG DISK',0
; ---------------------------------------------------------------------------

loc_15339:                              ; CODE XREF: access_file+51↑j
                                        ; access_file+75↓j
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_15339
                jmp     short loc_1530F
; ---------------------------------------------------------------------------

loc_15343:                              ; CODE XREF: access_file+4F↑j
                mov     dx, cs:_picture_int21_dx
                push    ds
                cmp     word ptr _picData.filename, 'IP'
                jnz     short loc_15356
                mov     ax, 0B800h
                mov     ds, ax
                assume ds:nothing

loc_15356:                              ; CODE XREF: access_file+85↑j
                mov     ah, 1Ah
                int     21h             ; DOS - SET DISK TRANSFER AREA ADDRESS
                                        ; DS:DX -> disk transfer buffer
                pop     ds
                assume ds:sg08e3
                lea     dx, _picData
                mov     cx, cs:_picture_int21_cx
                mov     _picData.record_size, 1
                mov     ah, cs:_picture_int21_function
                int     21h             ; DOS -
;
                lea     dx, _picData
                mov     ah, 10h
                int     21h             ; DOS - CLOSE DISK FILE
                                        ; DS:DX -> FCB
                                        ; Return: AL = 00h directory update successful
                                        ; FFh file not found in directory
                pop     es
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     ax
                retn
access_file     endp

; ---------------------------------------------------------------------------
                push    ax
                push    bx

loc_15382:                              ; CODE XREF: sg01a2:538A↓j
                mov     bx, 100h

loc_15385:                              ; CODE XREF: sg01a2:5386↓j
                dec     bx
                jnz     short loc_15385
                dec     al
                jnz     short loc_15382
                pop     bx
                pop     ax
                retn

; =============== S U B R O U T I N E =======================================


sub_1538F       proc near               ; CODE XREF: sg01a2:042C↑p
                                        ; sg01a2:loc_10519↑p
                mov     text_y, 17h
                call    sub_153A0
                mov     text_y, 18h
                call    sub_153A0
                retn
sub_1538F       endp


; =============== S U B R O U T I N E =======================================


sub_153A0       proc near               ; CODE XREF: sub_1538F+5↑p
                                        ; sub_1538F+D↑p
                push    ax
                push    cx
                mov     text_x, 0
                mov     al, 20h ; ' '
                mov     cx, 28h ; '('

loc_153AC:                              ; CODE XREF: sub_153A0+10↓j
                call    write_character
                dec     cx
                jnz     short loc_153AC
                pop     cx
                pop     ax
                retn
sub_153A0       endp


; =============== S U B R O U T I N E =======================================


write_number    proc near               ; CODE XREF: write_two_numbers+1↑p
                                        ; write_two_numbers+6↑p ...
                push    ax
                mov     ah, al
                shr     al, 1
                shr     al, 1
                shr     al, 1
                shr     al, 1
                or      al, 30h
                call    write_character
                mov     al, ah
                and     al, 0Fh
                or      al, 30h
                call    write_character
                pop     ax
                retn
write_number    endp


; =============== S U B R O U T I N E =======================================


get_character   proc near               ; CODE XREF: get_character+6↓j
                                        ; sg01a2:loc_1574E↓p ...
                call    keypress_check
                cmp     ah, 0FFh
                jnz     short get_character
                retn
get_character   endp


; =============== S U B R O U T I N E =======================================


set_normal_text_color proc near         ; CODE XREF: set_cga_mode+1D↑p
                                        ; setPalette+1D↑p ...
                mov     text_color, 15
                retn
set_normal_text_color endp


; =============== S U B R O U T I N E =======================================


set_reverse_text_color proc near        ; CODE XREF: write_stats+27↑p
                                        ; sg01a2:514B↑p ...
                cmp     text_mode?, 0
                jz      short loc_153EE
                mov     text_color, 2
                jmp     short locret_153F3
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_153EE:                              ; CODE XREF: set_reverse_text_color+5↑j
                mov     text_color, 70h ; 'p'

locret_153F3:                           ; CODE XREF: set_reverse_text_color+C↑j
                retn
set_reverse_text_color endp


; =============== S U B R O U T I N E =======================================


sub_153F4       proc near               ; CODE XREF: canMoveToTile-19EE↑p
                call    write_character
                retn
sub_153F4       endp

; ---------------------------------------------------------------------------
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
; ---------------------------------------------------------------------------

create_character:                       ; CODE XREF: start_:loc_103A9↑p
                call    set_cga_mode
                mov     bh, 0
                mov     bl, 6
                mov     di, bx
                mov     bh, 0
                mov     bl, 12
                mov     si, bx
                call    set_text_pos
                jmp     short loc_1543B
; ---------------------------------------------------------------------------
                db   0FBh,   28h,   49h,   4Eh,   53h,   45h,   52h,   54h
                db    20h,   41h,   20h,   42h,   4Ch,   41h,   4Eh,   4Bh
                db    20h,   50h,   4Ch,   41h,   59h,   45h,   52h,   20h
                db    44h,   49h,   53h,   4Bh,   29h,   0Dh,     0,  0E8h
                db    17h,  0FBh,   3Dh,   1Bh,  0FFh,   75h,  0F8h
; ---------------------------------------------------------------------------

loc_1543B:                              ; CODE XREF: sg01a2:5412↑j
                mov     ah, 27h
                mov     cx, size Savegame
                lea     dx, player
                call    access_file
; ---------------------------------------------------------------------------
aPlayer_2       db 'PLAYER  '
; ---------------------------------------------------------------------------
                cmp     player._name, 0
                jz      short loc_15498
                call    set_cga_mode
                mov     bh, 0
                mov     bl, 0Bh
                mov     si, bx
                mov     bh, 0
                mov     bl, 0Eh
                mov     di, bx
                call    set_text_pos
                call    write_string
; ---------------------------------------------------------------------------
aNotABlank      db 'NOT A BLANK',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 0Dh
                mov     si, bx
                mov     bh, 0
                mov     bl, 0Eh
                mov     di, bx
                call    set_text_pos
                call    write_string
; ---------------------------------------------------------------------------
aPlayerDisk     db 'PLAYER DISK',0
; ---------------------------------------------------------------------------
                jmp     loc_15A64
; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR update_points_remaining

loc_15498:                              ; CODE XREF: sg01a2:5454↑j
                                        ; sg01a2:59D5↓j ...
                call    set_cga_mode
                call    write_string
; END OF FUNCTION CHUNK FOR update_points_remaining
; ---------------------------------------------------------------------------
aPlayerGenerati db '          PLAYER GENERATION :',0
; ---------------------------------------------------------------------------
                call    write_string
; ---------------------------------------------------------------------------
aPointsLeftToDi db 0Dh,0Ah
                db '     POINTS LEFT TO DISTRIBUTE : ',0
; ---------------------------------------------------------------------------
                call    set_reverse_text_color
                mov     al, 90h
                mov     points_to_distrubte, al
                call    write_number
                call    set_normal_text_color
                call    write_string
; ---------------------------------------------------------------------------
aStrengthAgilit db 0Dh,0Ah
                db '           STRENGTH.......',0Dh,'           AGILITY........',0Dh,' '
                db '          STAMINA........',0Dh,'           CHARISMA.......',0Dh,' '
                db '          WISDOM.........',0Dh,'           INTELLIGENCE...',0Dh,0Ah
                db '                M/F-',0Dh,'               RACE-',0Dh,'           '
                db '    TYPE-',0Dh,'               NAME-',0Dh,0Ah
                db '          SATISFACTORY (Y/N)-',0Dh,0Ah
                db '        RACES:         TYPES:',0Dh,'         1-HUMAN        1-FIG'
                db 'HTER',0Dh,'         2-ELF          2-CLERIC',0Dh,'         3-DWAR'
                db 'F        3-WIZARD',0Dh,'         4-HOBBIT       4-THIEF',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 4
                mov     si, bx
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                call    set_text_pos
                call    get_number
                mov     player._strength, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 5
                mov     si, bx
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                call    set_text_pos
                call    get_number
                mov     player._agility, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 6
                mov     si, bx
                call    set_text_pos
                call    get_number
                mov     player._stamina, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 7
                mov     si, bx
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                call    set_text_pos
                call    get_number
                mov     player._charisma, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 8
                mov     si, bx
                call    set_text_pos
                call    get_number
                mov     player._wisdom, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 1Ah
                mov     di, bx
                mov     bh, 0
                mov     bl, 9
                mov     si, bx
                call    set_text_pos
                call    get_number
                mov     player._intelligence, al
                call    update_points_remaining
                mov     bh, 0
                mov     bl, 20
                mov     di, bx
                mov     bh, 0
                mov     bl, 11
                mov     si, bx
                call    set_text_pos

loc_1574E:                              ; CODE XREF: sg01a2:5783↓j
                call    get_character
                mov     player._sex, al
                cmp     al, 'M'
                jnz     short loc_15781
                call    write_string
; ---------------------------------------------------------------------------
aMale           db 'MALE',0
; ---------------------------------------------------------------------------
                mov     al, player._strength
                add     al, 5
                daa
                mov     player._strength, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 4
                mov     si, bx
                call    set_text_pos
                mov     al, player._strength
                call    write_number
                jmp     short loc_157AD
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15781:                              ; CODE XREF: sg01a2:5756↑j
                cmp     al, 'F'
                jnz     short loc_1574E
                call    write_string
; ---------------------------------------------------------------------------
aFemale         db 'FEMALE',0
; ---------------------------------------------------------------------------
                mov     al, player._charisma
                add     al, 16
                daa
                mov     player._charisma, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 7
                mov     si, bx
                call    set_text_pos
                mov     al, player._charisma
                call    write_number

loc_157AD:                              ; CODE XREF: sg01a2:577E↑j
                                        ; sg01a2:584E↓j
                mov     bh, 0
                mov     bl, 20
                mov     di, bx
                mov     bh, 0
                mov     bl, 12
                mov     si, bx
                call    set_text_pos
                call    get_character
                mov     player._race, al
                cmp     al, '1'
                jnz     short loc_157F0
                call    write_string
; ---------------------------------------------------------------------------
aHuman          db 'HUMAN',0
; ---------------------------------------------------------------------------
                mov     al, player._intelligence
                add     al, 5
                daa
                mov     player._intelligence, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 9
                mov     si, bx
                call    set_text_pos
                mov     al, player._intelligence
                call    write_number
                jmp     loc_15879
; ---------------------------------------------------------------------------

loc_157F0:                              ; CODE XREF: sg01a2:57C4↑j
                cmp     al, '2'
                jnz     short loc_1581C
                call    write_string
; ---------------------------------------------------------------------------
aElf            db 'ELF',0
; ---------------------------------------------------------------------------
                mov     al, player._agility
                add     al, 5
                daa
                mov     player._agility, al
                mov     bh, 0
                mov     bl, 1Ah
                mov     di, bx
                mov     bh, 0
                mov     bl, 5
                mov     si, bx
                call    set_text_pos
                mov     al, player._agility
                call    write_number
                jmp     short loc_15879
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1581C:                              ; CODE XREF: sg01a2:57F2↑j
                cmp     al, '3'
                jnz     short loc_1584A
                call    write_string
; ---------------------------------------------------------------------------
aDwarf          db 'DWARF',0
; ---------------------------------------------------------------------------
                mov     al, player._strength
                add     al, 5
                daa
                mov     player._strength, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 4
                mov     si, bx
                call    set_text_pos
                mov     al, player._strength
                call    write_number
                jmp     short loc_15879
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1584A:                              ; CODE XREF: sg01a2:581E↑j
                cmp     al, '4'
                jz      short loc_15851
                jmp     loc_157AD
; ---------------------------------------------------------------------------

loc_15851:                              ; CODE XREF: sg01a2:584C↑j
                call    write_string
; ---------------------------------------------------------------------------
aHobbit         db 'HOBBIT',0
; ---------------------------------------------------------------------------
                mov     al, player._wisdom
                add     al, 16
                daa
                mov     player._wisdom, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 8
                mov     si, bx
                call    set_text_pos
                mov     al, player._wisdom
                call    write_number

loc_15879:                              ; CODE XREF: sg01a2:57ED↑j
                                        ; sg01a2:5819↑j ...
                clc
                mov     al, player._race
                sub     al, '1'
                mov     player._race, al
                mov     bh, 0
                mov     bl, 13
                mov     si, bx
                mov     bh, 0
                mov     bl, 20
                mov     di, bx
                call    set_text_pos
                call    get_character
                mov     player._class, al
                cmp     al, '1'
                jnz     short loc_158C7
                call    write_string
; ---------------------------------------------------------------------------
aFighter        db 'FIGHTER',0
; ---------------------------------------------------------------------------
                mov     al, player._strength
                add     al, 21
                daa
                mov     player._strength, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 4
                mov     si, bx
                call    set_text_pos
                mov     al, player._strength
                call    write_number
                jmp     loc_15953
; ---------------------------------------------------------------------------

loc_158C7:                              ; CODE XREF: sg01a2:5899↑j
                cmp     al, '2'
                jnz     short loc_158F6
                call    write_string
; ---------------------------------------------------------------------------
aCleric         db 'CLERIC',0
; ---------------------------------------------------------------------------
                mov     al, player._wisdom
                add     al, 16
                daa
                mov     player._wisdom, al
                mov     bh, 0
                mov     bl, 26
                mov     di, bx
                mov     bh, 0
                mov     bl, 8
                mov     si, bx
                call    set_text_pos
                mov     al, player._wisdom
                call    write_number
                jmp     short loc_15953
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_158F6:                              ; CODE XREF: sg01a2:58C9↑j
                cmp     al, '3'
                jnz     short loc_15925
                call    write_string
; ---------------------------------------------------------------------------
aWizard         db 'WIZARD',0
; ---------------------------------------------------------------------------
                mov     al, player._intelligence
                add     al, 16
                daa
                mov     player._intelligence, al
                mov     bh, 0
                mov     bl, 1Ah
                mov     di, bx
                mov     bh, 0
                mov     bl, 9
                mov     si, bx
                call    set_text_pos
                mov     al, player._intelligence
                call    write_number
                jmp     short loc_15953
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15925:                              ; CODE XREF: sg01a2:58F8↑j
                cmp     al, '4'
                jz      short loc_1592C
                jmp     loc_15879
; ---------------------------------------------------------------------------

loc_1592C:                              ; CODE XREF: sg01a2:5927↑j
                call    write_string
; ---------------------------------------------------------------------------
aThief          db 'THIEF',0
; ---------------------------------------------------------------------------
                mov     al, player._agility
                add     al, 16
                daa
                mov     player._agility, al
                mov     bh, 0
                mov     bl, 1Ah
                mov     di, bx
                mov     bh, 0
                mov     bl, 5
                mov     si, bx
                call    set_text_pos
                mov     al, player._agility
                call    write_number

loc_15953:                              ; CODE XREF: sg01a2:58C4↑j
                                        ; sg01a2:58F3↑j ...
                clc
                mov     al, player._class
                sub     al, '1'
                mov     player._class, al

loc_1595C:                              ; CODE XREF: sg01a2:59A7↓j
                mov     bh, 0
                mov     bl, 14
                mov     si, bx
                mov     bh, 0
                mov     bl, 20
                mov     di, bx
                call    set_text_pos
                call    write_string
; ---------------------------------------------------------------------------
                db '               ',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 20
                mov     di, bx
                mov     bh, 0
                mov     bl, 14
                mov     si, bx
                call    set_text_pos
                mov     bh, 0
                mov     bl, 14
                mov     di, bx
                mov     al, 0

loc_15995:                              ; CODE XREF: sg01a2:599A↓j
                mov     byte ptr player._name[di], al
                dec     di
                jnz     short loc_15995
                mov     bh, 0
                mov     bl, 0
                mov     di, bx

loc_159A2:                              ; CODE XREF: sg01a2:59BA↓j
                call    get_character
                cmp     al, 8
                jz      short loc_1595C
                cmp     al, 13
                jz      short loc_159BC
                call    write_character
                mov     byte ptr player._name[di], al
                inc     di
                mov     bx, di
                cmp     bl, 12
                jb      short loc_159A2

loc_159BC:                              ; CODE XREF: sg01a2:59AB↑j
                                        ; sg01a2:59DA↓j
                mov     bh, 0
                mov     bl, 16
                mov     si, bx
                mov     bh, 0
                mov     bl, 1Dh
                mov     di, bx
                call    set_text_pos
                call    get_character
                call    write_character
                cmp     al, 'N'
                jnz     short loc_159D8
                jmp     loc_15498
; ---------------------------------------------------------------------------

loc_159D8:                              ; CODE XREF: sg01a2:59D3↑j
                cmp     al, 'Y'
                jnz     short loc_159BC
                mov     al, 20
                mov     player._mapX, al
                mov     al, 20
                mov     player._mapY, al
                mov     al, 2
                mov     player._mapNum1, al
                mov     al, 4
                mov     player._hp, al
                mov     player._food, al
                mov     player._gold, al
                mov     ah, 28h         ; Write PLAYER file
                mov     cx, 100h
                lea     dx, player
                call    access_file
; ---------------------------------------------------------------------------
aPlayer         db 'PLAYER  '
; ---------------------------------------------------------------------------
                call    set_cga_mode
                mov     bh, 0
                mov     bl, 11
                mov     si, bx
                mov     bh, 0
                mov     bl, 11
                mov     di, bx
                call    set_text_pos
                jmp     short loc_15A61
; ---------------------------------------------------------------------------
                db 0F5h
aToPlayUltima   db 'TO PLAY ULTIMA ][',0
; ---------------------------------------------------------------------------
                mov     bh, 0
                mov     bl, 13
                mov     si, bx
                mov     bh, 0
                mov     bl, 9
                mov     di, bx
                call    set_text_pos
                call    write_string
; ---------------------------------------------------------------------------
aInsertProgramM db 'INSERT PROGRAM MASTER',0
; ---------------------------------------------------------------------------

loc_15A59:                              ; CODE XREF: sg01a2:5A5F↓j
                call    keypress_check
                cmp     ax, 0FF1Bh
                jnz     short loc_15A59

loc_15A61:                              ; CODE XREF: sg01a2:5A1C↑j
                call    start_
; ---------------------------------------------------------------------------

loc_15A64:                              ; CODE XREF: sg01a2:5495↑j
                                        ; sg01a2:loc_15A64↓j
                jmp     short loc_15A64

; =============== S U B R O U T I N E =======================================


update_points_remaining proc near       ; CODE XREF: sg01a2:56C4↑p
                                        ; sg01a2:56DC↑p ...

; FUNCTION CHUNK AT 5498 SIZE 00000006 BYTES

                mov     _attribPoints, al
                cmp     al, 10
                jb      short loc_15A96
                mov     al, points_to_distrubte
                sub     al, _attribPoints
                das
                jb      short loc_15A96
                call    set_reverse_text_color
                mov     points_to_distrubte, al
                mov     bh, 0
                mov     bl, 2
                mov     si, bx          ; y
                mov     bh, 0
                mov     bl, 33
                mov     di, bx          ; x
                call    set_text_pos
                mov     al, points_to_distrubte
                call    write_number
                call    set_normal_text_color
                retn
; ---------------------------------------------------------------------------

loc_15A96:                              ; CODE XREF: update_points_remaining+5↑j
                                        ; update_points_remaining+F↑j
                call    write_string
; ---------------------------------------------------------------------------
                db 7,7,7,0
; ---------------------------------------------------------------------------
                pop     ax
                jmp     loc_15498
update_points_remaining endp

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
                db    0
                db    0
_mapDrawX       db 0                    ; DATA XREF: draw_map+11B↓w
                                        ; draw_map+13C↓r

; =============== S U B R O U T I N E =======================================


draw_map        proc near               ; CODE XREF: sg01a2:0864↑p
                                        ; canMoveToTile-1B3C↑p ...
                cmp     _outsideMapTile, -1
                jnz     short nonwrapping_draw
                jmp     normal_draw
; ---------------------------------------------------------------------------

nonwrapping_draw:                       ; CODE XREF: draw_map+5↑j
                stc
                mov     al, _mapY
                cmc
                sbb     al, 5
                cmc
                mov     _mapTop, al
                stc
                mov     al, _mapX
                cmc
                sbb     al, 9
                cmc
                mov     _mapLeft, al
                mov     al, 0
                mov     _mapOffsetY, al
                mov     _mapOffsetX, al
                mov     ah, 0
                mov     si, ax
                mov     ah, 0
                mov     di, ax

loc_15AE1:                              ; CODE XREF: draw_map+AA↓j
                                        ; draw_map+BD↓j
                clc
                mov     al, _mapOffsetX
                adc     al, _mapLeft
                cmp     al, 64
                jnb     short loc_15AF9
                clc
                mov     al, _mapOffsetY
                adc     al, _mapTop
                cmp     al, 64
                jb      short loc_15AFF

loc_15AF9:                              ; CODE XREF: draw_map+3A↑j
                mov     al, _outsideMapTile
                jmp     short loc_15B33
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15AFF:                              ; CODE XREF: draw_map+46↑j
                mov     byte ptr current_tile_ptr+1, al
                mov     al, 0
                clc
                rcr     byte ptr current_tile_ptr+1, 1
                rcr     al, 1
                clc
                rcr     byte ptr current_tile_ptr+1, 1
                rcr     al, 1
                adc     al, _mapOffsetX
                adc     al, _mapLeft
                mov     byte ptr current_tile_ptr, al
                clc
                mov     al, byte ptr current_tile_ptr+1
                adc     al, byte ptr map_ptr+1
                mov     byte ptr current_tile_ptr+1, al
                mov     bx, current_tile_ptr
                mov     al, [bx+si]
                clc
                rcr     al, 1
                and     al, 0FEh

loc_15B33:                              ; CODE XREF: draw_map+4B↑j
                cmp     al, _priorMapTileIds[di]
                jnz     short loc_15B4D
                cmp     al, 0
                jnz     short loc_15B47
                cmp     map_freezeAnimation, 0FFh
                jnz     short loc_15B4D
                jmp     short loc_15B4B
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15B47:                              ; CODE XREF: draw_map+8A↑j
                cmp     al, 46
                jz      short loc_15B4D

loc_15B4B:                              ; CODE XREF: draw_map+93↑j
                or      al, 80h

loc_15B4D:                              ; CODE XREF: draw_map+86↑j
                                        ; draw_map+91↑j ...
                mov     _mapTileIds[di], al
                inc     di
                inc     _mapOffsetX
                mov     al, _mapOffsetX
                cmp     al, 20
                jnz     short loc_15AE1
                mov     bx, si
                mov     _mapOffsetX, bl
                inc     _mapOffsetY
                mov     al, _mapOffsetY
                cmp     al, 10
                jz      short loc_15B71
                jmp     loc_15AE1
; ---------------------------------------------------------------------------

loc_15B71:                              ; CODE XREF: draw_map+BB↑j
                mov     al, _mapTileIds+6Dh
                mov     _tilePlayerCenter, al
                mov     al, _mapTileIds+59h
                mov     _tilePlayerUp, al
                mov     al, _mapTileIds+81h
                mov     _tilePlayerDown, al
                mov     al, _mapTileIds+6Ch
                mov     _tilePlayerLeft, al
                mov     al, _mapTileIds+6Eh
                mov     _tilePlayerRight, al
                mov     al, _playerTileId
                mov     _mapTileIds+6Dh, al
                jmp     loc_15C6D
; ---------------------------------------------------------------------------

normal_draw:                            ; CODE XREF: draw_map+7↑j
                stc
                mov     al, _mapY
                cmc
                sbb     al, 5
                cmc
                and     al, 63
                mov     _mapTop, al
                stc
                mov     al, _mapX
                cmc
                sbb     al, 9
                cmc
                and     al, 63
                mov     _mapLeft, al
                mov     al, 0
                mov     _mapOffsetY, al
                mov     _mapOffsetX, al
                mov     ah, 0
                mov     si, ax
                mov     ah, 0
                mov     di, ax

loc_15BC2:                              ; CODE XREF: draw_map+182↓j
                                        ; draw_map+195↓j
                clc
                mov     al, _mapOffsetX
                adc     al, _mapLeft
                and     al, 63
                mov     cs:_mapDrawX, al
                clc
                mov     al, _mapOffsetY
                adc     al, _mapTop
                and     al, 63
                mov     byte ptr current_tile_ptr+1, al
                mov     al, 0
                clc
                rcr     byte ptr current_tile_ptr+1, 1
                rcr     al, 1
                clc
                rcr     byte ptr current_tile_ptr+1, 1
                rcr     al, 1
                adc     al, cs:_mapDrawX
                mov     byte ptr current_tile_ptr, al
                clc
                mov     al, byte ptr current_tile_ptr+1
                adc     al, byte ptr map_ptr+1
                mov     byte ptr current_tile_ptr+1, al
                mov     bx, current_tile_ptr
                mov     al, [bx+si]     ; Get tile at x,y
                clc
                rcr     al, 1
                and     al, 0FEh
                cmp     al, _priorMapTileIds[di]
                jnz     short loc_15C25
                cmp     al, 0
                jnz     short loc_15C1F
                cmp     map_freezeAnimation, 0FFh
                jnz     short loc_15C25
                jmp     short loc_15C23
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15C1F:                              ; CODE XREF: draw_map+162↑j
                cmp     al, 46
                jz      short loc_15C25

loc_15C23:                              ; CODE XREF: draw_map+16B↑j
                or      al, 80h

loc_15C25:                              ; CODE XREF: draw_map+15E↑j
                                        ; draw_map+169↑j ...
                mov     _mapTileIds[di], al
                inc     di
                inc     _mapOffsetX
                mov     al, _mapOffsetX
                cmp     al, 20
                jnz     short loc_15BC2
                mov     bx, si
                mov     _mapOffsetX, bl
                inc     _mapOffsetY
                mov     al, _mapOffsetY
                cmp     al, 10
                jz      short loc_15C49
                jmp     loc_15BC2
; ---------------------------------------------------------------------------

loc_15C49:                              ; CODE XREF: draw_map+193↑j
                mov     al, _mapTileIds+6Dh
                mov     _tilePlayerCenter, al
                mov     al, _mapTileIds+59h
                mov     _tilePlayerUp, al
                mov     al, _mapTileIds+81h
                mov     _tilePlayerDown, al
                mov     al, _mapTileIds+6Ch
                mov     _tilePlayerLeft, al
                mov     al, _mapTileIds+6Eh
                mov     _tilePlayerRight, al
                mov     al, _playerTileId
                mov     _mapTileIds+6Dh, al

loc_15C6D:                              ; CODE XREF: draw_map+E4↑j
                call    draw_map_content
                mov     bh, 0
                mov     bl, 0
                mov     di, bx

loc_15C76:                              ; CODE XREF: draw_map+1D5↓j
                mov     al, _mapTileIds[di]
                and     al, 7Fh
                mov     _priorMapTileIds[di], al
                inc     di
                mov     bx, di
                cmp     bl, 240
                jnz     short loc_15C76
                retn
draw_map        endp

; ---------------------------------------------------------------------------
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
byte_15C90      db 0                    ; DATA XREF: sub_15CB2+F↓w
                                        ; sub_15CE4+1↓r
                db    0
                db    0
                db    0
                db    0

; =============== S U B R O U T I N E =======================================


sub_15C95       proc near               ; CODE XREF: canMoveToTile:loc_10A30↑p
                                        ; canMoveToTile-1159↑p ...
                cmp     byte_1795D, 0
                jnz     short locret_15CB1
                push    ax
                push    bx
                push    cx
                mov     bx, 500h
                call    sub_15CB2
                mov     cx, 4000h
                call    sub_15CD2
                call    sub_15CE4
                pop     cx
                pop     bx
                pop     ax

locret_15CB1:                           ; CODE XREF: sub_15C95+5↑j
                retn
sub_15C95       endp


; =============== S U B R O U T I N E =======================================


sub_15CB2       proc near               ; CODE XREF: sub_15C95+D↑p
                                        ; sub_15D09+16↓p ...
                push    ax
                mov     al, 0B6h
                out     43h, al         ; Timer 8253-5 (AT: 8254.2).
                mov     ax, bx
                out     42h, al         ; Timer 8253-5 (AT: 8254.2).
                mov     al, ah
                out     42h, al         ; Timer 8253-5 (AT: 8254.2).
                in      al, 61h         ; PC/XT PPI port B bits:
                                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                                        ; 3: 1=read high switches
                                        ; 4: 0=enable RAM parity checking
                                        ; 5: 0=enable I/O channel check
                                        ; 6: 0=hold keyboard clock low
                                        ; 7: 0=enable kbrd
                mov     cs:byte_15C90, al
                cmp     byte_1795D, 0
                jnz     short loc_15CD0
                or      al, 3
                out     61h, al         ; PC/XT PPI port B bits:
                                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                                        ; 3: 1=read high switches
                                        ; 4: 0=enable RAM parity checking
                                        ; 5: 0=enable I/O channel check
                                        ; 6: 0=hold keyboard clock low
                                        ; 7: 0=enable kbrd

loc_15CD0:                              ; CODE XREF: sub_15CB2+18↑j
                pop     ax
                retn
sub_15CB2       endp


; =============== S U B R O U T I N E =======================================


sub_15CD2       proc near               ; CODE XREF: sub_15C95+13↑p
                                        ; sub_15D09+2E↓p ...
                mov     ax, bx
                out     42h, al         ; Timer 8253-5 (AT: 8254.2).
                mov     al, ah
                out     42h, al         ; Timer 8253-5 (AT: 8254.2).

loc_15CDA:                              ; CODE XREF: sub_15CD2+F↓j
                mov     ax, array2+0Ch

loc_15CDD:                              ; CODE XREF: sub_15CD2+C↓j
                dec     ax
                jns     short loc_15CDD
                dec     cx
                jnz     short loc_15CDA
                retn
sub_15CD2       endp


; =============== S U B R O U T I N E =======================================


sub_15CE4       proc near               ; CODE XREF: sub_15C95+16↑p
                                        ; sub_15D09:loc_15D3D↓p ...
                push    ax
                mov     al, cs:byte_15C90
                out     61h, al         ; PC/XT PPI port B bits:
                                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                                        ; 3: 1=read high switches
                                        ; 4: 0=enable RAM parity checking
                                        ; 5: 0=enable I/O channel check
                                        ; 6: 0=hold keyboard clock low
                                        ; 7: 0=enable kbrd
                pop     ax
                retn
sub_15CE4       endp

; ---------------------------------------------------------------------------
                db 0B0h
                db 0B6h
                db 0E6h
                db  43h ; C
                db  8Bh
                db 0C3h
                db 0E6h
                db  42h ; B
                db  8Ah
                db 0C4h
                db 0E6h
                db  42h ; B
                db 0E4h
                db  61h ; a
                db  8Ah
                db 0E0h
                db  0Ch
                db    3
                db 0E6h
                db  61h ; a
                db  49h ; I
                db  75h ; u
                db 0FDh
                db  8Ah
                db 0C4h
                db 0E6h
                db  61h ; a
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_15D09       proc near               ; CODE XREF: sub_15EC7+B↓p
                push    ax
                push    bx
                push    cx
                push    bp
                cmp     byte_1795D, 0
                jnz     short loc_15D58
                shl     bx, 1
                mov     bp, cs:off_15D9B[bx]
                mov     bx, cs:[bp+1]
                call    sub_15CB2

loc_15D22:                              ; CODE XREF: sub_15D09+4A↓j
                mov     cl, 0
                mov     ch, cs:[bp+0]
                cmp     ch, 0
                jz      short loc_15D55
                mov     bx, cs:[bp+1]
                cmp     bx, 0FFFFh
                jz      short loc_15D3D
                call    sub_15CD2
                jmp     short loc_15D50
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15D3D:                              ; CODE XREF: sub_15D09+2C↑j
                call    sub_15CE4

loc_15D40:                              ; CODE XREF: sub_15D09+3E↓j
                mov     ax, array2+0Ch

loc_15D43:                              ; CODE XREF: sub_15D09+3B↓j
                dec     ax
                jns     short loc_15D43
                dec     cx
                jnz     short loc_15D40
                mov     bx, cs:[bp+4]
                call    sub_15CB2

loc_15D50:                              ; CODE XREF: sub_15D09+31↑j
                add     bp, 3
                jmp     short loc_15D22
; ---------------------------------------------------------------------------

loc_15D55:                              ; CODE XREF: sub_15D09+22↑j
                call    sub_15CE4

loc_15D58:                              ; CODE XREF: sub_15D09+9↑j
                pop     bp
                pop     cx
                pop     bx
                pop     ax
                retn
sub_15D09       endp

; ---------------------------------------------------------------------------
unk_15D5D       db    5                 ; DATA XREF: sg01a2:off_15D9B↓o
                db    0
                db  40h ; @
                db    0
unk_15D61       db    1                 ; DATA XREF: sg01a2:5D9D↓o
                db    0
                db    1
                db    0
unk_15D65       db    1                 ; DATA XREF: sg01a2:5D9F↓o
                db    0
                db    1
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
off_15D9B       dw offset unk_15D5D     ; DATA XREF: sub_15D09+D↑r
                dw offset unk_15D61
                dw offset unk_15D65
word_15DA1      dw 0                    ; DATA XREF: sub_15DA9+11↓w
                                        ; sub_15DA9:loc_15DDE↓r ...
word_15DA3      dw 0                    ; DATA XREF: sub_15DA9+15↓w
                                        ; sub_15DA9+57↓r ...
word_15DA5      dw 0                    ; DATA XREF: sub_15DA9+1A↓w
                                        ; sub_15DA9+27↓r ...
word_15DA7      dw 0                    ; DATA XREF: sub_15DA9+39↓w
                                        ; sub_15DA9:loc_15DE6↓r ...

; =============== S U B R O U T I N E =======================================


sub_15DA9       proc near               ; CODE XREF: sub_15E3B+13↓p
                                        ; sub_15EAC+13↓p ...
                cmp     byte_1795D, 0
                jz      short loc_15DB3
                jmp     locret_15E3A
; ---------------------------------------------------------------------------

loc_15DB3:                              ; CODE XREF: sub_15DA9+5↑j
                push    ax
                push    bx
                push    cx
                push    dx
                push    bp
                shr     bp, 1
                mov     cs:word_15DA1, ax
                mov     cs:word_15DA3, bx
                mov     cs:word_15DA5, cx
                mov     bx, ax
                call    sub_15CB2
                mov     ax, 0
                sub     ax, cs:word_15DA5
                js      short loc_15DDE
                mov     cs:word_15DA5, ax
                jmp     short loc_15E07
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_15DDE:                              ; CODE XREF: sub_15DA9+2C↑j
                                        ; sub_15DA9+87↓j
                mov     ax, cs:word_15DA1
                mov     cs:word_15DA7, ax

loc_15DE6:                              ; CODE XREF: sub_15DA9+5C↓j
                mov     bx, cs:word_15DA7
                mov     cx, bp
                call    sub_15CD2
                dec     dx
                jz      short loc_15E32
                mov     ax, cs:word_15DA7
                add     ax, cs:word_15DA5
                mov     cs:word_15DA7, ax
                cmp     ax, cs:word_15DA3
                jb      short loc_15DE6

loc_15E07:                              ; CODE XREF: sub_15DA9+32↑j
                mov     ax, cs:word_15DA3
                mov     cs:word_15DA7, ax

loc_15E0F:                              ; CODE XREF: sub_15DA9+85↓j
                mov     bx, cs:word_15DA7
                mov     cx, bp
                call    sub_15CD2
                dec     dx
                jz      short loc_15E32
                mov     ax, cs:word_15DA7
                sub     ax, cs:word_15DA5
                mov     cs:word_15DA7, ax
                cmp     ax, cs:word_15DA1
                ja      short loc_15E0F
                jmp     short loc_15DDE
; ---------------------------------------------------------------------------

loc_15E32:                              ; CODE XREF: sub_15DA9+48↑j
                                        ; sub_15DA9+71↑j
                call    sub_15CE4
                pop     bp
                pop     dx
                pop     cx
                pop     bx
                pop     ax

locret_15E3A:                           ; CODE XREF: sub_15DA9+7↑j
                retn
sub_15DA9       endp


; =============== S U B R O U T I N E =======================================


sub_15E3B       proc near               ; CODE XREF: canMoveToTile+4AA↑p
                push    ax
                push    bx
                push    cx
                push    dx
                mov     ax, 100h
                mov     bx, 4000h
                mov     cx, 1
                mov     dx, 1000h
                mov     bp, 80h
                call    sub_15DA9
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
sub_15E3B       endp

; ---------------------------------------------------------------------------
                db  80h
                db  3Eh ; >
                db  4Dh ; M
                db    5
                db    0
                db  75h ; u
                db  25h ; %
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db 0BAh
                db    0
                db    4
                db 0BBh
                db    0
                db    6
                db 0E8h
                db  48h ; H
                db 0FEh
                db  8Bh
                db 0CAh
                db 0E8h
                db  63h ; c
                db 0FEh
                db  83h
                db 0C3h
                db  40h ; @
                db  83h
                db 0EAh
                db  20h
                db  81h
                db 0FAh
                db    0
                db    1
                db  77h ; w
                db 0EFh
                db 0E8h
                db  66h ; f
                db 0FEh
                db  5Ah ; Z
                db  59h ; Y
                db  5Bh ; [
                db  58h ; X
                db 0C3h
                db  80h
                db  3Eh ; >
                db  4Dh ; M
                db    5
                db    0
                db  75h ; u
                db  21h ; !
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db 0BBh
                db    0
                db    8
                db 0E8h
                db  1Eh
                db 0FEh
                db 0BAh
                db    0
                db    1
                db  8Bh
                db 0CAh
                db 0E8h
                db  36h ; 6
                db 0FEh
                db  83h
                db 0EAh
                db    2
                db  83h
                db 0EBh
                db  40h ; @
                db  79h ; y
                db 0F3h
                db 0E8h
                db  3Dh ; =
                db 0FEh
                db  5Ah ; Z
                db  59h ; Y
                db  5Bh ; [
                db  58h ; X
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_15EAC       proc near               ; CODE XREF: canMoveToTile-186A↑p
                                        ; canMoveToTile-10CF↑p
                push    ax
                push    bx
                push    cx
                push    dx
                mov     ax, 300h
                mov     bx, 1000h
                mov     cx, 2
                mov     dx, 200h
                mov     bp, 8
                call    sub_15DA9
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
sub_15EAC       endp


; =============== S U B R O U T I N E =======================================


sub_15EC7       proc near               ; CODE XREF: canMoveToTile-1B40↑p
                                        ; canMoveToTile+134↑p ...
                cmp     byte_1795D, 0
                jnz     short locret_15ED6
                push    bx
                mov     bx, 0
                call    sub_15D09
                pop     bx

locret_15ED6:                           ; CODE XREF: sub_15EC7+5↑j
                retn
sub_15EC7       endp

; ---------------------------------------------------------------------------
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db 0B8h
                db    0
                db    2
                db 0BBh
                db    0
                db  30h ; 0
                db 0B9h
                db  20h
                db    0
                db 0BAh
                db    0
                db  0Eh
                db 0BDh
                db    8
                db    0
                db 0E8h
                db 0BCh
                db 0FEh
                db  5Ah ; Z
                db  59h ; Y
                db  5Bh ; [
                db  58h ; X
                db 0C3h

; =============== S U B R O U T I N E =======================================


pause?          proc near               ; CODE XREF: canMoveToTile-185A↑p
                                        ; sub_10F8E+27↑p ...
                push    ax
                push    bx
                push    cx
                push    dx
                mov     ax, 800h
                mov     bx, 1800h
                mov     cx, 47h ; 'G'
                mov     dx, 800h
                mov     bp, 8
                call    sub_15DA9
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
pause?          endp

; ---------------------------------------------------------------------------
                db  80h
                db  3Eh ; >
                db  4Dh ; M
                db    5
                db    0
                db  74h ; t
                db    3
                db 0E9h
                db  8Eh
                db    0
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db  55h ; U
                db  2Eh ; .
                db  89h
                db  1Eh
                db  91h
                db  5Ch ; \
                db  8Bh
                db 0C1h
                db 0F7h
                db  26h ; &
                db  74h ; t
                db    0
                db 0F7h
                db  36h ; 6
                db  54h ; T
                db    5
                db 0F7h
                db  26h ; &
                db  74h ; t
                db    0
                db 0F7h
                db  36h ; 6
                db  54h ; T
                db    5
                db  2Eh ; .
                db 0A3h
                db  93h
                db  5Ch ; \
                db  81h
                db  3Eh ; >
                db  74h ; t
                db    0
                db    0
                db  18h
                db  72h ; r
                db  17h
                db  8Bh
                db 0D0h
                db 0B8h
                db    0
                db    0
                db 0BBh
                db  88h
                db  13h
                db 0F7h
                db 0F3h
                db 0BBh
                db  1Eh
                db    0
                db  89h
                db  87h
                db  80h
                db    5
                db 0E8h
                db 0BEh
                db 0A7h
                db 0EBh
                db  4Bh ; K
                db  90h
                db 0BDh
                db    1
                db    0
                db  2Eh ; .
                db  8Bh
                db  16h
                db  93h
                db  5Ch ; \
                db  8Bh
                db 0CDh
                db 0BBh
                db    0
                db  30h ; 0
                db 0E8h
                db  87h
                db 0FDh
                db 0B8h
                db  0Ah
                db    0
                db  2Eh ; .
                db    3
                db    6
                db  91h
                db  5Ch ; \
                db  2Bh ; +
                db 0C5h
                db  48h ; H
                db  75h ; u
                db 0FDh
                db  4Ah ; J
                db  75h ; u
                db 0E8h
                db  83h
                db 0C5h
                db    1
                db  83h
                db 0FDh
                db  0Ah
                db  76h ; v
                db 0DBh
                db  2Eh ; .
                db  8Bh
                db  16h
                db  93h
                db  5Ch ; \
                db  8Bh
                db 0CDh
                db 0BBh
                db    0
                db  30h ; 0
                db 0E8h
                db  62h ; b
                db 0FDh
                db 0B8h
                db  0Ah
                db    0
                db  2Eh ; .
                db    3
                db    6
                db  91h
                db  5Ch ; \
                db  2Bh ; +
                db 0C5h
                db  48h ; H
                db  75h ; u
                db 0FDh
                db  4Ah ; J
                db  75h ; u
                db 0E8h
                db  83h
                db 0EDh
                db    1
                db  75h ; u
                db 0DEh
                db  5Dh ; ]
                db  5Ah ; Z
                db  59h ; Y
                db  5Bh ; [
                db  58h ; X
                db 0C3h

; =============== S U B R O U T I N E =======================================


sub_15FA6       proc near               ; CODE XREF: normal_movement+130↑p
                                        ; normal_movement+17D↑p
                push    ax
                push    bx
                push    cx
                push    dx
                push    bp
                mov     ax, 2F8h
                mov     bx, 320h
                mov     cx, 0FFFFh
                mov     dx, 50h ; 'P'
                mov     bp, 0F00h
                call    sub_15DA9
                pop     bp
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
sub_15FA6       endp


; =============== S U B R O U T I N E =======================================


sub_15FC3       proc near               ; CODE XREF: sub_10E70+17↑p
                                        ; sub_10EC2+17↑p ...
                push    ax
                push    bx
                push    cx
                push    dx
                push    bp
                mov     ax, 492h
                mov     bx, 4C0h
                mov     cx, 0FFFFh
                mov     dx, 5Ch ; '\'
                mov     bp, 0B00h
                call    sub_15DA9
                pop     bp
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retn
sub_15FC3       endp

; ---------------------------------------------------------------------------
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db  55h ; U
                db 0B8h
                db    0
                db    4
                db 0BBh
                db    0
                db  10h
                db 0B9h
                db    1
                db    0
                db 0BAh
                db    0
                db    2
                db 0BDh
                db  80h
                db    0
                db 0E8h
                db 0B2h
                db 0FDh
                db  5Dh ; ]
                db  5Ah ; Z
                db  59h ; Y
                db  5Bh ; [
                db  58h ; X
                db 0C3h
                db    0
                db    0
                db    0

; =============== S U B R O U T I N E =======================================


sub_16000       proc near               ; CODE XREF: canMoveToTile-1156↑p
                nop
                mov     al, _mapLeft
                mov     byte_17893, al
                mov     al, _mapTop
                mov     byte_17894, al
                cmp     byte_17436, 0
                jz      short loc_1601D
                call    sub_16078
                call    sub_16144
                call    sub_168ED

loc_1601D:                              ; CODE XREF: sub_16000+12↑j
                mov     al, _mapLeft
                mov     byte_17893, al
                mov     al, _mapTop
                mov     byte_17894, al
                mov     al, _mapX
                mov     _playerX, al
                mov     al, _mapY
                mov     _playerY, al
                call    sub_168C0
                mov     _tilePlayerCenter, al
                mov     al, _mapX
                clc
                adc     al, byte_17893
                mov     _playerX, al
                mov     al, _mapY
                clc
                adc     al, byte_17894
                mov     _playerY, al
                call    sub_168C0
                mov     _tilePlayerUp, al
                mov     al, _mapX
                stc
                cmc
                sbb     al, byte_17893
                cmc
                mov     _playerX, al
                mov     al, _mapY
                stc
                cmc
                sbb     al, byte_17894
                cmc
                mov     _playerY, al
                call    sub_168C0
                mov     _tilePlayerDown, al
                retn
sub_16000       endp


; =============== S U B R O U T I N E =======================================


sub_16078       proc near               ; CODE XREF: sub_16000+14↑p
                clc
                mov     al, _mapX
                mov     byte_1788B, al
                adc     al, byte_17894
                mov     byte_17889, al
                stc
                mov     al, _mapY
                mov     byte_1788C, al
                cmc
                sbb     al, byte_17893
                cmc
                mov     byte_1788A, al
                stc
                mov     al, _mapX
                cmc
                sbb     al, byte_17894
                cmc
                mov     byte_1788D, al
                clc
                mov     al, _mapY
                adc     al, byte_17893
                mov     byte_1788E, al
                mov     bh, 0
                mov     bl, 0
                mov     di, bx

loc_160B4:                              ; CODE XREF: sub_16078+C8↓j
                mov     al, byte_17889
                and     al, 3Fh
                mov     _playerX, al
                clc
                adc     al, byte_17893
                mov     byte_17889, al
                mov     al, byte_1788A
                and     al, 3Fh
                mov     _playerY, al
                clc
                adc     al, byte_17894
                mov     byte_1788A, al
                call    sub_168C0
                and     al, 0F0h
                mov     [di+48Fh], al
                mov     al, byte_1788B
                and     al, 3Fh
                mov     _playerX, al
                clc
                adc     al, byte_17893
                mov     byte_1788B, al
                mov     al, byte_1788C
                and     al, 3Fh
                mov     _playerY, al
                clc
                adc     al, byte_17894
                mov     byte_1788C, al
                call    sub_168C0
                and     al, 0F0h
                mov     [di+497h], al
                call    sub_168C0
                and     al, 7
                mov     [di+4AFh], al
                mov     al, byte_1788D
                and     al, 3Fh
                mov     _playerX, al
                clc
                adc     al, byte_17893
                mov     byte_1788D, al
                mov     al, byte_1788E
                and     al, 3Fh
                mov     _playerY, al
                clc
                adc     al, byte_17894
                mov     byte_1788E, al
                call    sub_168C0
                and     al, 0F0h
                mov     [di+49Fh], al
                inc     di
                mov     bx, di
                cmp     bl, 8
                jnb     short locret_16143
                jmp     loc_160B4
; ---------------------------------------------------------------------------

locret_16143:                           ; CODE XREF: sub_16078+C6↑j
                retn
sub_16078       endp


; =============== S U B R O U T I N E =======================================


sub_16144       proc near               ; CODE XREF: sub_16000+17↑p
                call    sub_14B4E
                mov     al, 0
                mov     byte_17892, al
                call    sub_16425

loc_1614F:                              ; CODE XREF: sub_16144+4C↓j
                                        ; sub_16144+C5↓j
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, [di+497h]
                or      al, al
                jns     short loc_16192
                call    sub_16425
                mov     al, [di+497h]
                and     al, 40h
                jnz     short loc_1616D
                jmp     locret_1620C
; ---------------------------------------------------------------------------

loc_1616D:                              ; CODE XREF: sub_16144+24↑j
                mov     al, [di+497h]
                and     al, 20h
                jz      short loc_16178
                jmp     locret_1620C
; ---------------------------------------------------------------------------

loc_16178:                              ; CODE XREF: sub_16144+2F↑j
                call    sub_1648D
                mov     al, byte_17892
                or      al, al
                jz      short loc_16185
                jmp     locret_1620C
; ---------------------------------------------------------------------------

loc_16185:                              ; CODE XREF: sub_16144+3C↑j
                inc     byte_17892
                mov     al, byte_17892
                cmp     al, 8
                jnb     short locret_1620C
                jmp     short loc_1614F
; ---------------------------------------------------------------------------

loc_16192:                              ; CODE XREF: sub_16144+19↑j
                mov     al, [di+497h]
                and     al, 20h
                jz      short loc_1619D
                call    sub_16594

loc_1619D:                              ; CODE XREF: sub_16144+54↑j
                mov     al, [di+497h]
                and     al, 10h
                jz      short loc_161AB
                call    sub_1660C
                jmp     short loc_161B6
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_161AB:                              ; CODE XREF: sub_16144+5F↑j
                mov     al, [di+497h]
                and     al, 40h
                jz      short loc_161B6
                call    sub_166CB

loc_161B6:                              ; CODE XREF: sub_16144+64↑j
                                        ; sub_16144+6D↑j
                mov     al, [di+48Fh]
                or      al, al
                jns     short loc_161D7
                call    sub_16377
                mov     al, [di+48Fh]
                and     al, 40h
                jz      short loc_161DA
                mov     al, [di+48Fh]
                and     al, 20h
                jnz     short loc_161DA
                call    sub_164E2
                jmp     short loc_161DA
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_161D7:                              ; CODE XREF: sub_16144+78↑j
                call    sub_16291

loc_161DA:                              ; CODE XREF: sub_16144+83↑j
                                        ; sub_16144+8B↑j ...
                mov     al, [di+49Fh]
                or      al, al
                jns     short loc_161FB
                call    sub_163CE
                mov     al, [di+49Fh]
                and     al, 40h
                jz      short loc_161FE
                mov     al, [di+49Fh]
                and     al, 20h
                jnz     short loc_161FE
                call    sub_1653F
                jmp     short loc_161FE
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_161FB:                              ; CODE XREF: sub_16144+9C↑j
                call    sub_16304

loc_161FE:                              ; CODE XREF: sub_16144+A7↑j
                                        ; sub_16144+AF↑j ...
                inc     byte_17892
                mov     al, byte_17892
                cmp     al, 7
                jnb     short locret_1620C
                jmp     loc_1614F
; ---------------------------------------------------------------------------

locret_1620C:                           ; CODE XREF: sub_16144+26↑j
                                        ; sub_16144+31↑j ...
                retn
sub_16144       endp

; ---------------------------------------------------------------------------
                db    0
                db  40h ; @
                db  60h ; `
                db  70h ; p
                db  78h ; x
                db  7Ch ; |
                db  7Eh ; ~
                db  7Fh ; 
                db  7Fh ; 
                db  7Fh ; 
                db 0FFh
                db 0BFh
                db  9Fh
                db  8Fh
                db  87h
                db  83h
                db  81h
                db  80h
                db  80h
                db  80h
                db    0
                db  20h
                db  30h ; 0
                db  38h ; 8
                db  3Ch ; <
                db  3Eh ; >
                db  3Fh ; ?
                db  3Fh ; ?
                db  7Fh ; 
                db  5Fh ; _
                db  4Fh ; O
                db  47h ; G
                db  43h ; C
                db  41h ; A
                db  40h ; @
                db  40h ; @
                db  10h
                db  48h ; H
                db  64h ; d
                db  72h ; r
                db  79h ; y
                db  7Ch ; |
                db  7Fh ; 
                db  7Fh ; 
                db  30h ; 0
                db  58h ; X
                db  6Ch ; l
                db  76h ; v
                db  7Bh ; {
                db  7Ch ; |
                db  7Fh ; 
                db  7Fh ; 
                db 0EFh
                db 0B7h
                db  9Bh
                db  8Dh
                db  86h
                db  83h
                db  80h
                db  80h
                db 0CFh
                db 0A7h
                db  93h
                db  89h
                db  84h
                db  83h
                db  80h
                db  80h
                db  77h ; w
                db  5Bh ; [
                db  4Dh ; M
                db  46h ; F
                db  42h ; B
                db  40h ; @
                db  40h ; @
                db  40h ; @
                db  67h ; g
                db  53h ; S
                db  49h ; I
                db  44h ; D
                db  42h ; B
                db  40h ; @
                db  40h ; @
                db  40h ; @
                db  2Ah ; *
                db  35h ; 5
                db  3Bh ; ;
                db  3Dh ; =
                db  3Eh ; >
                db  3Fh ; ?
                db  3Fh ; ?
                db  3Fh ; ?
                db    8
                db  24h ; $
                db  32h ; 2
                db  39h ; 9
                db  3Ch ; <
                db  3Eh ; >
                db  3Fh ; ?
                db  3Fh ; ?
                db  18h
                db  2Ch ; ,
                db  36h ; 6
                db  3Ah ; :
                db  3Dh ; =
                db  3Eh ; >
                db  3Fh ; ?
                db  3Fh ; ?
                db  77h ; w
                db  5Bh ; [
                db  4Dh ; M
                db  46h ; F
                db  43h ; C
                db  41h ; A
                db  40h ; @
                db  40h ; @
                db  67h ; g
                db  53h ; S
                db  49h ; I
                db  45h ; E
                db  42h ; B
                db  41h ; A
                db  40h ; @
                db  40h ; @
                db  6Fh ; o
                db  57h ; W
                db  4Bh ; K
                db  45h ; E
                db  42h ; B
                db  40h ; @
                db  40h ; @
                db  40h ; @

; =============== S U B R O U T I N E =======================================


sub_16291       proc near               ; CODE XREF: sub_16144:loc_161D7↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6221h]
                mov     byte_1788A, al
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6222h]
                mov     byte_1788A, al
                mov     byte_1788C, al
                mov     al, cs:[di+620Eh]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+622Ah]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_16291       endp


; =============== S U B R O U T I N E =======================================


sub_16304       proc near               ; CODE XREF: sub_16144:loc_161FB↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6217h]
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6221h]
                mov     byte_1788A, al
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6222h]
                mov     byte_1788A, al
                mov     byte_1788C, al
                mov     al, cs:[di+6218h]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+622Ah]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6217h]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_16304       endp


; =============== S U B R O U T I N E =======================================


sub_16377       proc near               ; CODE XREF: sub_16144+7A↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_17889, al
                mov     al, cs:[di+620Eh]
                mov     byte_1788B, al
                mov     al, cs:[di+6221h]
                mov     byte_1788A, al
                mov     al, cs:[di+6222h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_1788B, al
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                mov     al, cs:[di+622Ah]
                mov     byte_1788A, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_16377       endp


; =============== S U B R O U T I N E =======================================


sub_163CE       proc near               ; CODE XREF: sub_16144+9E↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6217h]
                mov     byte_17889, al
                mov     al, cs:[di+6218h]
                mov     byte_1788B, al
                mov     al, cs:[di+6221h]
                mov     byte_1788A, al
                mov     al, cs:[di+6222h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6217h]
                mov     byte_1788B, al
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                mov     al, cs:[di+622Ah]
                mov     byte_1788A, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_163CE       endp


; =============== S U B R O U T I N E =======================================


sub_16425       proc near               ; CODE XREF: sub_16144+8↑p
                                        ; sub_16144+1B↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_17889, al
                mov     al, cs:[di+6221h]
                mov     byte_1788A, al
                mov     byte_1788C, al
                mov     al, cs:[di+6217h]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Dh]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6221h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_16425       endp


; =============== S U B R O U T I N E =======================================


sub_1648D       proc near               ; CODE XREF: sub_16144:loc_16178↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Eh]
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6229h]
                mov     byte_1788A, al
                mov     al, cs:[di+6222h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6218h]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6229h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_1648D       endp


; =============== S U B R O U T I N E =======================================


sub_164E2       proc near               ; CODE XREF: sub_16144+8D↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6231h]
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6251h]
                mov     byte_1788A, al
                mov     al, cs:[di+6222h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6239h]
                mov     byte_1788B, al
                mov     al, cs:[di+6261h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6259h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_164E2       endp


; =============== S U B R O U T I N E =======================================


sub_1653F       proc near               ; CODE XREF: sub_16144+B1↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6241h]
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6251h]
                mov     byte_1788A, al
                mov     al, cs:[di+6222h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6249h]
                mov     byte_1788B, al
                mov     al, cs:[di+6261h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6259h]
                mov     byte_1788C, al
                call    sub_167B3
                retn
sub_1653F       endp


; =============== S U B R O U T I N E =======================================


sub_16594       proc near               ; CODE XREF: sub_16144+56↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6232h]
                mov     byte_17889, al
                mov     al, cs:[di+623Ah]
                mov     byte_1788B, al
                mov     al, cs:[di+6279h]
                mov     byte_1788A, al
                mov     al, cs:[di+6281h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+624Ah]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6242h]
                mov     byte_1788B, al
                mov     al, cs:[di+6279h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6232h]
                mov     byte_1788B, al
                call    sub_167B3
                call    sub_16684
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_16594       endp


; =============== S U B R O U T I N E =======================================


sub_1660C       proc near               ; CODE XREF: sub_16144+61↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6232h]
                mov     byte_17889, al
                mov     al, cs:[di+623Ah]
                mov     byte_1788B, al
                mov     al, cs:[di+6269h]
                mov     byte_1788A, al
                mov     al, cs:[di+6271h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+624Ah]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6242h]
                mov     byte_1788B, al
                mov     al, cs:[di+6269h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6232h]
                mov     byte_1788B, al
                call    sub_167B3
                call    sub_16684
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_1660C       endp


; =============== S U B R O U T I N E =======================================


sub_16684       proc near               ; CODE XREF: sub_16594+6C↑p
                                        ; sub_1660C+6C↑p
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, 7Fh
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6269h]
                mov     byte_1788A, al
                mov     al, cs:[di+6279h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, 80h
                mov     byte_17889, al
                mov     byte_1788B, al
                mov     al, cs:[di+6269h]
                mov     byte_1788A, al
                mov     al, cs:[di+6279h]
                mov     byte_1788C, al
                call    sub_167B3
                retn
sub_16684       endp


; =============== S U B R O U T I N E =======================================


sub_166CB       proc near               ; CODE XREF: sub_16144+6F↑p
                nop
                mov     al, cs:[di+620Fh]
                mov     byte_17889, al
                mov     al, cs:[di+6289h]
                mov     byte_1788A, al
                mov     byte_1788C, al
                mov     al, cs:[di+621Ah]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6251h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+620Fh]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6289h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6210h]
                mov     byte_1788B, al
                mov     al, cs:[di+6259h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6219h]
                mov     byte_1788B, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+6289h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+621Ah]
                mov     byte_1788B, al
                mov     al, cs:[di+6251h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                mov     al, cs:[di+621Ah]
                mov     byte_17889, al
                mov     al, cs:[di+6289h]
                mov     byte_1788A, al
                mov     al, cs:[di+6219h]
                mov     byte_1788B, al
                mov     al, cs:[di+6259h]
                mov     byte_1788C, al
                call    sub_167B3
                mov     bh, 0
                mov     bl, byte_17892
                mov     di, bx
                retn
sub_166CB       endp


; =============== S U B R O U T I N E =======================================


sub_167B3       proc near               ; CODE XREF: sub_16291+23↑p
                                        ; sub_16291+41↑p ...
                nop
                mov     al, byte_1788B
                cmp     al, byte_17889
                jnz     short loc_167C7
                mov     al, byte_1788C
                cmp     al, byte_1788A
                jnz     short loc_167C7
                retn
; ---------------------------------------------------------------------------

loc_167C7:                              ; CODE XREF: sub_167B3+8↑j
                                        ; sub_167B3+11↑j
                stc
                mov     al, byte_1788B
                cmc
                sbb     al, byte_17889
                cmc
                mov     byte_17893, al
                jb      short loc_167E7
                xor     al, 0FFh
                mov     byte_17893, al
                inc     byte_17893
                mov     al, 0FFh
                mov     byte_1788D, al
                jmp     short loc_167EC
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_167E7:                              ; CODE XREF: sub_167B3+21↑j
                mov     al, 1
                mov     byte_1788D, al

loc_167EC:                              ; CODE XREF: sub_167B3+31↑j
                stc
                mov     al, byte_1788C
                cmc
                sbb     al, byte_1788A
                cmc
                mov     byte_17894, al
                jb      short loc_1680C
                xor     al, 0FFh
                mov     byte_17894, al
                inc     byte_17894
                mov     al, 0FFh
                mov     byte_1788E, al
                jmp     short loc_16811
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1680C:                              ; CODE XREF: sub_167B3+46↑j
                mov     al, 1
                mov     byte_1788E, al

loc_16811:                              ; CODE XREF: sub_167B3+56↑j
                mov     al, byte_17893
                cmp     al, byte_17894
                jnb     short loc_1681D
                jmp     short loc_16860
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1681D:                              ; CODE XREF: sub_167B3+65↑j
                mov     al, byte_17893
                mov     byte_17890, al
                clc
                rcr     al, 1
                mov     byte_17891, al

loc_16829:                              ; CODE XREF: sub_167B3+AA↓j
                clc
                mov     al, byte_17891
                adc     al, byte_17894
                mov     byte_17891, al
                stc
                cmc
                sbb     al, byte_17893
                cmc
                jnb     short loc_1684B
                mov     byte_17891, al
                clc
                mov     al, byte_1788A
                adc     al, byte_1788E
                mov     byte_1788A, al

loc_1684B:                              ; CODE XREF: sub_167B3+88↑j
                clc
                mov     al, byte_17889
                adc     al, byte_1788D
                mov     byte_17889, al
                call    sub_14B76
                dec     byte_17890
                jnz     short loc_16829
                retn
; ---------------------------------------------------------------------------

loc_16860:                              ; CODE XREF: sub_167B3+67↑j
                mov     al, byte_17894
                mov     byte_17890, al
                clc
                rcr     al, 1
                mov     byte_17891, al

loc_1686C:                              ; CODE XREF: sub_167B3+ED↓j
                clc
                mov     al, byte_17891
                adc     al, byte_17893
                mov     byte_17891, al
                stc
                cmc
                sbb     al, byte_17894
                cmc
                jnb     short loc_1688E
                mov     byte_17891, al
                clc
                mov     al, byte_17889
                adc     al, byte_1788D
                mov     byte_17889, al

loc_1688E:                              ; CODE XREF: sub_167B3+CB↑j
                clc
                mov     al, byte_1788A
                adc     al, byte_1788E
                mov     byte_1788A, al
                call    sub_14B76
                dec     byte_17890
                jnz     short loc_1686C
                retn
sub_167B3       endp


; =============== S U B R O U T I N E =======================================


sub_168A3       proc near               ; CODE XREF: sub_168ED+8C↓p
                nop
                mov     ah, 0
                mov     al, byte_1788F
                mov     bp, ax
                mov     al, _mapOffsetX
                shl     ax, 1
                shl     ax, 1
                mov     bh, 0
                mov     bl, _mapOffsetY
                shl     bx, 1
                shl     bx, 1
                call    sub_14BE0
                retn
sub_168A3       endp


; =============== S U B R O U T I N E =======================================


sub_168C0       proc near               ; CODE XREF: sub_16000+35↑p
                                        ; sub_16000+51↑p ...
                mov     ah, byte_17435
                mov     al, 0
                add     ax, map_ptr
                mov     byte ptr word_17897+1, ah
                mov     al, _playerY
                add     al, al
                add     al, al
                add     al, al
                add     al, al
                adc     al, _playerX
                mov     byte ptr word_17897, al
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     bx, word_17897
                mov     al, [bx+si]
                retn
sub_168C0       endp


; =============== S U B R O U T I N E =======================================


sub_168ED       proc near               ; CODE XREF: sub_16000+1A↑p
                nop
                inc     byte_17892
                mov     bh, 0
                mov     bl, 0
                mov     di, bx

loc_168F8:                              ; CODE XREF: sub_168ED+1B↓j
                inc     di
                mov     bx, di
                cmp     bl, byte_17892
                jb      short loc_16902
                retn
; ---------------------------------------------------------------------------

loc_16902:                              ; CODE XREF: sub_168ED+12↑j
                mov     ah, [di+4AFh]
                or      ah, ah
                jz      short loc_168F8
                cmp     di, 1
                jz      short loc_1692C
                mov     ax, monsters_ptr
                mov     byte ptr word_17899+1, ah
                mov     al, cs:[di+697Eh]
                mov     byte ptr word_17899, al
                mov     al, byte ptr word_17899+1
                mov     bh, 0
                mov     bl, byte ptr word_17899
                mov     di, bx
                jmp     short loc_1693A
; ---------------------------------------------------------------------------
                db  90h
; ---------------------------------------------------------------------------

loc_1692C:                              ; CODE XREF: sub_168ED+20↑j
                clc
                add     ax, monsters_ptr
                mov     byte ptr word_17899+1, ah
                mov     byte ptr word_17899, 0

loc_1693A:                              ; CODE XREF: sub_168ED+3C↑j
                                        ; sub_168ED+8F↓j
                mov     bh, 0
                mov     bl, 0
                mov     si, bx
                mov     bx, word_17899
                mov     al, [bx+si]
                or      al, al
                jnz     short loc_1694B
                retn
; ---------------------------------------------------------------------------

loc_1694B:                              ; CODE XREF: sub_168ED+5B↑j
                mov     _mapOffsetX, al
                inc     word_17899
                mov     bx, word_17899
                mov     al, [bx+si]
                and     al, 1Fh
                mov     _mapOffsetY, al
                mov     bx, word_17899
                mov     al, [bx+si]
                clc
                rcr     al, 1
                clc
                rcr     al, 1
                clc
                rcr     al, 1
                clc
                rcr     al, 1
                clc
                rcr     al, 1
                mov     byte_1788F, al
                inc     word_17899
                call    sub_168A3
                jmp     short loc_1693A
sub_168ED       endp

; ---------------------------------------------------------------------------
                db    0
                db    0
                db  80h
                db 0C0h
                db 0E0h
                db 0F0h
                db 0F8h
                db 0FCh
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
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  90h
                db 0E8h
                db 0A1h
                db 0E0h
                db  2Eh ; .
                db 0C6h
                db    6
                db  98h
                db  69h ; i
                db 0FFh
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db  6Dh ; m
                db    0
                db  8Bh
                db 0FBh
                db  2Eh ; .
                db  8Ah
                db  85h
                db  12h
                db  70h ; p
                db 0A2h
                db 0CAh
                db    4
                db  2Eh ; .
                db  8Ah
                db  85h
                db  1Ch
                db  70h ; p
                db 0A2h
                db 0CBh
                db    4
                db  2Eh ; .
                db  8Ah
                db  85h
                db  26h ; &
                db  70h ; p
                db 0A2h
                db 0CCh
                db    4
                db 0A0h
                db 0E5h
                db    0
                db 0F9h
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db 0E5h
                db    0
                db 0B0h
                db  28h ; (
                db 0A2h
                db  31h ; 1
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  1Eh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  14h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0FEh
                db 0E7h
                db 0E8h
                db 0EFh
                db 0E5h
                db  20h
                db  20h
                db  46h ; F
                db  55h ; U
                db  45h ; E
                db  4Ch ; L
                db  3Dh ; =
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  1Eh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  15h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0E4h
                db 0E7h
                db 0E8h
                db 0D5h
                db 0E5h
                db  20h
                db  20h
                db  58h ; X
                db  45h ; E
                db  4Eh ; N
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  1Eh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  16h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0CAh
                db 0E7h
                db 0E8h
                db 0BBh
                db 0E5h
                db  20h
                db  20h
                db  59h ; Y
                db  41h ; A
                db  4Bh ; K
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  1Eh
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  17h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0B0h
                db 0E7h
                db 0E8h
                db 0A1h
                db 0E5h
                db  20h
                db  20h
                db  5Ah ; Z
                db  41h ; A
                db  42h ; B
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0B0h
                db  20h
                db 0A2h
                db  31h ; 1
                db    0
                db 0E8h
                db  55h ; U
                db    5
                db 0E8h
                db 0EDh
                db    2
                db 0E8h
                db 0A9h
                db    3
                db 0A0h
                db  62h ; b
                db    0
                db  3Ch ; <
                db    5
                db  73h ; s
                db  1Eh
                db 0E8h
                db  81h
                db 0E5h
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  45h ; E
                db  58h ; X
                db  50h ; P
                db  4Ch ; L
                db  4Fh ; O
                db  44h ; D
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db  8Dh
                db  8Dh
                db    0
                db 0E8h
                db  12h
                db    8
                db 0EBh
                db 0FEh
                db  90h
                db 0E8h
                db  26h ; &
                db    5
                db 0E8h
                db  5Fh ; _
                db 0E5h
                db  43h ; C
                db  4Dh ; M
                db  44h ; D
                db  3Ah ; :
                db  20h
                db    0
                db 0E8h
                db 0B4h
                db    0
                db 0BBh
                db  14h
                db    0
                db 0E8h
                db  82h
                db  9Ch
                db 0E8h
                db 0BBh
                db 0E4h
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0EFh
                db  3Ah ; :
                db    6
                db  71h ; q
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  39h ; 9
                db  90h
                db  3Ah ; :
                db    6
                db  70h ; p
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  48h ; H
                db  90h
                db  3Ah ; :
                db    6
                db  6Eh ; n
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  59h ; Y
                db  90h
                db  3Ah ; :
                db    6
                db  6Fh ; o
                db    2
                db  75h ; u
                db    3
                db 0EBh
                db  6Ah ; j
                db  90h
                db  3Ch ; <
                db  48h ; H
                db  75h ; u
                db    3
                db 0E9h
                db    3
                db    4
                db  3Ch ; <
                db  4Ch ; L
                db  75h ; u
                db    3
                db 0E9h
                db 0BCh
                db    6
                db 0E8h
                db 0F8h
                db    1
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0EBh
                db 0AEh
                db 0E8h
                db    4
                db 0E5h
                db  4Ch ; L
                db  45h ; E
                db  46h ; F
                db  54h ; T
                db  8Dh
                db    0
                db 0E8h
                db 0E0h
                db    1
                db 0B0h
                db  20h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0EBh
                db  89h
                db 0E8h
                db 0ECh
                db 0E4h
                db  52h ; R
                db  49h ; I
                db  47h ; G
                db  48h ; H
                db  54h ; T
                db  8Dh
                db    0
                db 0E8h
                db 0C7h
                db    1
                db 0B0h
                db 0DFh
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0E9h
                db  6Fh ; o
                db 0FFh
                db 0E8h
                db 0D2h
                db 0E4h
                db  43h ; C
                db  4Ch ; L
                db  49h ; I
                db  4Dh ; M
                db  42h ; B
                db  8Dh
                db    0
                db 0E8h
                db 0ADh
                db    1
                db 0B0h
                db  10h
                db 0A2h
                db  7Ch ; |
                db    4
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0E9h
                db  55h ; U
                db 0FFh
                db 0E8h
                db 0B8h
                db 0E4h
                db  44h ; D
                db  49h ; I
                db  56h ; V
                db  45h ; E
                db  8Dh
                db    0
                db 0E8h
                db  94h
                db    1
                db 0B0h
                db  6Fh ; o
                db 0A2h
                db  7Ch ; |
                db    4
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0E9h
                db  3Ch ; <
                db 0FFh
                db  90h
                db 0E8h
                db  42h ; B
                db    2
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0F3h
                db  8Ah
                db  84h
                db 0CDh
                db    4
                db 0A2h
                db 0C3h
                db    4
                db 0F9h
                db 0F5h
                db  1Ah
                db    6
                db  7Bh ; {
                db    4
                db 0F5h
                db  72h ; r
                db  28h ; (
                db  34h ; 4
                db 0FFh
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0A0h
                db 0C3h
                db    4
                db 0F9h
                db 0F5h
                db  2Eh ; .
                db  1Ah
                db  85h
                db 0ACh
                db  6Ch ; l
                db 0F5h
                db  72h ; r
                db    3
                db 0E9h
                db  13h
                db    1
                db 0A2h
                db 0C5h
                db    4
                db 0EBh
                db  22h ; "
                db  90h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0A0h
                db 0C3h
                db    4
                db 0F8h
                db  2Eh ; .
                db  12h
                db  85h
                db 0ACh
                db  6Ch ; l
                db 0A2h
                db 0C5h
                db    4
                db  73h ; s
                db    3
                db 0E9h
                db 0ECh
                db    0
                db  8Ah
                db  84h
                db  0Dh
                db    5
                db 0A2h
                db 0C4h
                db    4
                db 0F9h
                db 0F5h
                db  1Ah
                db    6
                db  7Ch ; |
                db    4
                db 0F5h
                db  72h ; r
                db  25h ; %
                db  34h ; 4
                db 0FFh
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0A0h
                db 0C4h
                db    4
                db 0F9h
                db 0F5h
                db  2Eh ; .
                db  1Ah
                db  85h
                db 0BCh
                db  6Ch ; l
                db 0F5h
                db 0A2h
                db 0C6h
                db    4
                db  79h ; y
                db  24h ; $
                db 0E9h
                db 0B7h
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0F8h
                db 0D0h
                db 0D8h
                db 0B4h
                db    0
                db  8Bh
                db 0F8h
                db 0A0h
                db 0C4h
                db    4
                db 0F8h
                db  2Eh ; .
                db  12h
                db  85h
                db 0BCh
                db  6Ch ; l
                db  79h ; y
                db    3
                db 0E9h
                db  99h
                db    0
                db 0A2h
                db 0C6h
                db    4
                db  8Bh
                db 0DEh
                db  88h
                db  1Eh
                db 0C9h
                db    4
                db 0A0h
                db 0C7h
                db    4
                db  0Ah
                db 0C0h
                db  74h ; t
                db  11h
                db  8Ah
                db  84h
                db 0CDh
                db    4
                db 0A2h
                db  79h ; y
                db    4
                db  8Ah
                db  84h
                db  0Dh
                db    5
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db  96h
                db 0DFh
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db 0C9h
                db    4
                db  8Bh
                db 0F3h
                db 0A0h
                db 0C5h
                db    4
                db 0A2h
                db  79h ; y
                db    4
                db  88h
                db  84h
                db 0CDh
                db    4
                db 0A0h
                db 0C6h
                db    4
                db 0A2h
                db  7Ah ; z
                db    4
                db  88h
                db  84h
                db  0Dh
                db    5
                db 0E8h
                db  43h ; C
                db 0DFh
                db  80h
                db  3Eh ; >
                db 0C7h
                db    4
                db    2
                db  73h ; s
                db  18h
                db  2Eh ; .
                db  8Bh
                db  1Eh
                db  90h
                db  69h ; i
                db  2Eh ; .
                db    3
                db  1Eh
                db  92h
                db  69h ; i
                db  2Eh ; .
                db  89h
                db  1Eh
                db  90h
                db  69h ; i
                db 0B9h
                db    1
                db    0
                db 0E8h
                db  83h
                db 0F0h
                db 0EBh
                db  0Bh
                db  90h
                db  2Eh ; .
                db 0FFh
                db  0Eh
                db  94h
                db  69h ; i
                db  75h ; u
                db    3
                db 0E8h
                db 0E6h
                db    5
                db 0B7h
                db    0
                db  8Ah
                db  1Eh
                db 0C9h
                db    4
                db  8Bh
                db 0F3h
                db  46h ; F
                db  8Bh
                db 0DEh
                db  80h
                db 0FBh
                db  40h ; @
                db  74h ; t
                db  0Ah
                db 0A0h
                db 0C7h
                db    4
                db  3Ch ; <
                db    2
                db  73h ; s
                db    0
                db 0E9h
                db 0D1h
                db 0FEh
                db  90h
                db 0A0h
                db 0C7h
                db    4
                db  0Ah
                db 0C0h
                db  74h ; t
                db  0Dh
                db 0B4h
                db    0
                db  8Bh
                db 0C8h
                db 0BBh
                db  10h
                db    0
                db  4Bh ; K
                db  75h ; u
                db 0FDh
                db  49h ; I
                db  75h ; u
                db 0F7h
                db 0C3h
                db  90h
                db  8Ah
                db  84h
                db 0CDh
                db    4
                db 0A2h
                db 0C3h
                db    4
                db  8Ah
                db  84h
                db  0Dh
                db    5
                db 0A2h
                db 0C4h
                db    4
                db 0E8h
                db 0CAh
                db    0
                db 0A2h
                db 0C5h
                db    4
                db 0E8h
                db 0C4h
                db    0
                db  24h ; $
                db  7Fh ; 
                db 0A2h
                db 0C6h
                db    4
                db 0E9h
                db  4Ah ; J
                db 0FFh
                db    1
                db    2
                db    3
                db    4
                db    5
                db    6
                db    7
                db    8
                db    9
                db  0Ah
                db  0Bh
                db  0Ch
                db  0Dh
                db  0Eh
                db  0Fh
                db  10h
                db    1
                db    2
                db    3
                db    4
                db    5
                db    6
                db    7
                db    8
                db 0A0h
                db  7Bh ; {
                db    4
                db 0A2h
                db  79h ; y
                db    4
                db 0A0h
                db  7Ch ; |
                db    4
                db 0F9h
                db 0F5h
                db  1Ch
                db    3
                db 0F5h
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db 0D2h
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0CBh
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0C4h
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0BDh
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0B6h
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0AFh
                db 0DEh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0A8h
                db 0DEh
                db 0A0h
                db  7Bh ; {
                db    4
                db 0F9h
                db 0F5h
                db  1Ch
                db    3
                db 0F5h
                db 0A2h
                db  79h ; y
                db    4
                db 0A0h
                db  7Ch ; |
                db    4
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db  94h
                db 0DEh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  8Dh
                db 0DEh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  86h
                db 0DEh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  7Bh ; {
                db 0DEh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  74h ; t
                db 0DEh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  6Dh ; m
                db 0DEh
                db 0C3h
                db 0E8h
                db  1Ah
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  3Fh ; ?
                db  8Bh
                db 0F3h
                db 0E8h
                db  1Eh
                db    0
                db  88h
                db  84h
                db 0CDh
                db    4
                db 0E8h
                db  17h
                db    0
                db  24h ; $
                db  7Fh ; 
                db  88h
                db  84h
                db  0Dh
                db    5
                db  4Eh ; N
                db  79h ; y
                db 0EDh
                db 0C3h
                db  2Eh ; .
                db 0C6h
                db    6
                db  96h
                db  69h ; i
                db  3Bh ; ;
                db  2Eh ; .
                db 0C6h
                db    6
                db  97h
                db  69h ; i
                db  67h ; g
                db 0C3h
                db 0F8h
                db  2Eh ; .
                db 0A0h
                db  96h
                db  69h ; i
                db  14h
                db    9
                db  2Eh ; .
                db  12h
                db    6
                db  97h
                db  69h ; i
                db  2Eh ; .
                db  8Ah
                db  26h ; &
                db  96h
                db  69h ; i
                db  2Eh ; .
                db 0A2h
                db  96h
                db  69h ; i
                db  2Eh ; .
                db  88h
                db  26h ; &
                db  97h
                db  69h ; i
                db 0C3h
                db 0A0h
                db  7Bh ; {
                db    4
                db 0A2h
                db  79h ; y
                db    4
                db 0A0h
                db  7Ch ; |
                db    4
                db 0F9h
                db 0F5h
                db  1Ch
                db    3
                db 0F5h
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db 0DFh
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0D8h
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0D1h
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0CAh
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0C3h
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0BCh
                db 0DDh
                db 0FEh
                db    6
                db  7Ah ; z
                db    4
                db 0E8h
                db 0B5h
                db 0DDh
                db 0A0h
                db  7Bh ; {
                db    4
                db 0F9h
                db 0F5h
                db  1Ch
                db    3
                db 0F5h
                db 0A2h
                db  79h ; y
                db    4
                db 0A0h
                db  7Ch ; |
                db    4
                db 0A2h
                db  7Ah ; z
                db    4
                db 0E8h
                db 0A1h
                db 0DDh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  9Ah
                db 0DDh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  93h
                db 0DDh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  88h
                db 0DDh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  81h
                db 0DDh
                db 0FEh
                db    6
                db  79h ; y
                db    4
                db 0E8h
                db  7Ah ; z
                db 0DDh
                db 0C3h
                db 0E8h
                db 0DFh
                db 0E1h
                db  48h ; H
                db  59h ; Y
                db  50h ; P
                db  45h ; E
                db  52h ; R
                db  57h ; W
                db  41h ; A
                db  52h ; R
                db  50h ; P
                db  20h
                db  45h ; E
                db  4Eh ; N
                db  47h ; G
                db  41h ; A
                db  47h ; G
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db  8Dh
                db    0
                db 0B0h
                db  80h
                db 0A2h
                db 0C8h
                db    4
                db 0B0h
                db    0
                db 0A2h
                db 0C7h
                db    4
                db 0E8h
                db  3Ah ; :
                db 0FFh
                db 0BBh
                db    0
                db  22h ; "
                db  2Eh ; .
                db  89h
                db  1Eh
                db  90h
                db  69h ; i
                db  2Eh ; .
                db 0C7h
                db    6
                db  92h
                db  69h ; i
                db 0FFh
                db 0FFh
                db 0E8h
                db  43h ; C
                db    4
                db 0E8h
                db    7
                db 0FDh
                db 0BBh
                db  16h
                db    0
                db 0E8h
                db 0D5h
                db  98h
                db 0FEh
                db  0Eh
                db 0C8h
                db    4
                db  75h ; u
                db 0F1h
                db 0B0h
                db  80h
                db 0A2h
                db 0C8h
                db    4
                db 0B0h
                db    1
                db 0A2h
                db 0C7h
                db    4
                db 0E8h
                db  54h ; T
                db    1
                db 0E8h
                db    9
                db 0FFh
                db  2Eh ; .
                db 0C7h
                db    6
                db  92h
                db  69h ; i
                db    1
                db    0
                db 0E8h
                db 0E1h
                db 0FCh
                db 0BBh
                db  16h
                db    0
                db 0E8h
                db 0AFh
                db  98h
                db 0FEh
                db  0Eh
                db 0C8h
                db    4
                db  75h ; u
                db 0F1h
                db 0FEh
                db    6
                db 0C7h
                db    4
                db  2Eh ; .
                db 0C7h
                db    6
                db  94h
                db  69h ; i
                db    1
                db    0
                db 0E8h
                db 0C7h
                db 0FCh
                db 0BBh
                db  18h
                db    0
                db 0E8h
                db  95h
                db  98h
                db 0FEh
                db    6
                db 0C7h
                db    4
                db 0FEh
                db    6
                db 0C7h
                db    4
                db  79h ; y
                db 0EDh
                db 0A0h
                db 0E5h
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db  73h ; s
                db  32h ; 2
                db 0F8h
                db 0D0h
                db 0D8h
                db  73h ; s
                db  2Dh ; -
                db 0E8h
                db  49h ; I
                db 0E1h
                db  53h ; S
                db  48h ; H
                db  49h ; I
                db  50h ; P
                db  20h
                db  4Fh ; O
                db  46h ; F
                db  46h ; F
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  55h ; U
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db  21h ; !
                db  8Dh
                db    0
                db 0E8h
                db 0BDh
                db 0FEh
                db  24h ; $
                db    7
                db 0A2h
                db 0CAh
                db    4
                db 0E8h
                db 0B5h
                db 0FEh
                db  24h ; $
                db    7
                db 0A2h
                db 0CBh
                db    4
                db 0E8h
                db 0ADh
                db 0FEh
                db  24h ; $
                db    7
                db 0A2h
                db 0CCh
                db    4
                db  90h
                db 0E8h
                db  6Ch ; l
                db    1
                db 0C3h
                db 0A0h
                db 0E5h
                db    0
                db  0Ah
                db 0C0h
                db  75h ; u
                db  10h
                db 0E8h
                db  10h
                db 0E1h
                db  4Eh ; N
                db  4Fh ; O
                db  20h
                db  46h ; F
                db  55h ; U
                db  45h ; E
                db  4Ch ; L
                db  21h ; !
                db  8Dh
                db    0
                db 0E9h
                db  9Dh
                db 0FBh
                db 0F9h
                db 0F5h
                db  1Ch
                db    1
                db  2Fh ; /
                db 0F5h
                db 0A2h
                db 0E5h
                db    0
                db 0E8h
                db 0DCh
                db 0FDh
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0E8h
                db 0EAh
                db 0E0h
                db  48h ; H
                db  59h ; Y
                db  50h ; P
                db  45h ; E
                db  52h ; R
                db  57h ; W
                db  41h ; A
                db  52h ; R
                db  50h ; P
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  3Ah ; :
                db  8Dh
                db  58h ; X
                db  45h ; E
                db  4Eh ; N
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0E8h
                db  73h ; s
                db    0
                db 0A2h
                db 0CAh
                db    4
                db 0E8h
                db 0CDh
                db 0E0h
                db  20h
                db  59h ; Y
                db  41h ; A
                db  4Bh ; K
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0E8h
                db  63h ; c
                db    0
                db 0A2h
                db 0CBh
                db    4
                db 0E8h
                db 0BDh
                db 0E0h
                db  20h
                db  5Ah ; Z
                db  41h ; A
                db  42h ; B
                db  4Fh ; O
                db  3Dh ; =
                db    0
                db 0E8h
                db  53h ; S
                db    0
                db 0A2h
                db 0CCh
                db    4
                db 0E8h
                db 0ADh
                db 0E0h
                db  8Dh
                db  50h ; P
                db  52h ; R
                db  45h ; E
                db  50h ; P
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  20h
                db  48h ; H
                db  59h ; Y
                db  50h ; P
                db  45h ; E
                db  52h ; R
                db  57h ; W
                db  41h ; A
                db  52h ; R
                db  50h ; P
                db  21h ; !
                db  8Dh
                db    0
                db 0E8h
                db  0Dh
                db 0FEh
                db 0E8h
                db 0ECh
                db 0FBh
                db 0BBh
                db  1Ah
                db    0
                db 0E8h
                db 0BAh
                db  97h
                db 0FEh
                db  0Eh
                db 0C7h
                db    4
                db 0A0h
                db 0C7h
                db    4
                db  3Ch ; <
                db    2
                db  75h ; u
                db 0ECh
                db 0B0h
                db  40h ; @
                db 0A2h
                db 0C8h
                db    4
                db 0E8h
                db 0F1h
                db 0FDh
                db 0E8h
                db 0D0h
                db 0FBh
                db 0BBh
                db  1Ah
                db    0
                db 0E8h
                db  9Eh
                db  97h
                db 0FEh
                db  0Eh
                db 0C8h
                db    4
                db  75h ; u
                db 0F1h
                db 0E8h
                db  81h
                db 0FEh
                db 0E9h
                db 0FAh
                db 0FAh
                db 0E8h
                db 0BBh
                db 0FBh
                db 0BBh
                db  14h
                db    0
                db 0E8h
                db  89h
                db  97h
                db 0E8h
                db 0C2h
                db 0DFh
                db  80h
                db 0FCh
                db 0FFh
                db  75h ; u
                db 0EFh
                db  3Ch ; <
                db  30h ; 0
                db  72h ; r
                db 0EBh
                db  3Ch ; <
                db  39h ; 9
                db  77h ; w
                db 0E7h
                db  50h ; P
                db 0E8h
                db  58h ; X
                db 0E4h
                db  58h ; X
                db 0F9h
                db 0F5h
                db  1Ch
                db  30h ; 0
                db 0F5h
                db 0C3h
                db  90h
                db 0B0h
                db  28h ; (
                db 0A2h
                db  31h ; 1
                db    0
                db 0B7h
                db    0
                db 0B3h
                db  25h ; %
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  14h
                db  8Bh
                db 0F3h
                db 0E8h
                db  33h ; 3
                db 0E2h
                db 0A0h
                db 0E5h
                db    0
                db 0E8h
                db 0F7h
                db 0E3h
                db 0B7h
                db    0
                db 0B3h
                db  25h ; %
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  15h
                db  8Bh
                db 0F3h
                db 0E8h
                db  1Eh
                db 0E2h
                db 0A0h
                db 0CAh
                db    4
                db 0E8h
                db 0E2h
                db 0E3h
                db 0B7h
                db    0
                db 0B3h
                db  25h ; %
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  16h
                db  8Bh
                db 0F3h
                db 0E8h
                db    9
                db 0E2h
                db 0A0h
                db 0CBh
                db    4
                db 0E8h
                db 0CDh
                db 0E3h
                db 0B7h
                db    0
                db 0B3h
                db  25h ; %
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  17h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0F4h
                db 0E1h
                db 0A0h
                db 0CCh
                db    4
                db 0E8h
                db 0B8h
                db 0E3h
                db 0B0h
                db  1Fh
                db 0A2h
                db  31h ; 1
                db    0
                db 0B7h
                db    0
                db 0B3h
                db    0
                db  8Bh
                db 0FBh
                db 0B7h
                db    0
                db 0B3h
                db  17h
                db  8Bh
                db 0F3h
                db 0E8h
                db 0DAh
                db 0E1h
                db 0C3h
                db    6
                db    5
                db    3
                db    6
                db    1
                db    2
                db    9
                db    4
                db    0
                db    9
                db    6
                db    4
                db    3
                db    2
                db    3
                db    8
                db    4
                db    0
                db    1
                db    9
                db    6
                db    5
                db    4
                db    3
                db    4
                db    5
                db    6
                db    5
                db    4
                db    9
                db  90h
                db 0A0h
                db 0CAh
                db    4
                db  3Ch ; <
                db    4
                db  75h ; u
                db  2Bh ; +
                db 0A0h
                db 0CBh
                db    4
                db  3Ch ; <
                db    4
                db  75h ; u
                db  24h ; $
                db 0A0h
                db 0CCh
                db    4
                db  3Ch ; <
                db    4
                db  75h ; u
                db  1Dh
                db 0E8h
                db  96h
                db 0DFh
                db  8Dh
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  48h ; H
                db  49h ; I
                db  54h ; T
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  53h ; S
                db  55h ; U
                db  4Eh ; N
                db  21h ; !
                db  8Dh
                db    0
                db 0E8h
                db 0EEh
                db 0DAh
                db 0E9h
                db  11h
                db 0FAh
                db  90h
                db 0B7h
                db    0
                db 0B3h
                db    9
                db  8Bh
                db 0FBh
                db  2Eh ; .
                db  8Ah
                db  85h
                db  12h
                db  70h ; p
                db  3Ah ; :
                db    6
                db 0CAh
                db    4
                db  75h ; u
                db  16h
                db  2Eh ; .
                db  8Ah
                db  85h
                db  1Ch
                db  70h ; p
                db  3Ah ; :
                db    6
                db 0CBh
                db    4
                db  75h ; u
                db  0Bh
                db  2Eh ; .
                db  8Ah
                db  85h
                db  26h ; &
                db  70h ; p
                db  3Ah ; :
                db    6
                db 0CCh
                db    4
                db  74h ; t
                db  26h ; &
                db  4Fh ; O
                db  79h ; y
                db 0DCh
                db 0E8h
                db  4Eh ; N
                db 0DFh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  44h ; D
                db  45h ; E
                db  45h ; E
                db  50h ; P
                db  20h
                db  53h ; S
                db  50h ; P
                db  41h ; A
                db  43h ; C
                db  45h ; E
                db  2Eh ; .
                db  8Dh
                db    0
                db 0B0h
                db  0Ah
                db 0A2h
                db  6Dh ; m
                db    0
                db 0E9h
                db 0D3h
                db    0
                db  8Bh
                db 0C7h
                db 0A2h
                db  6Dh ; m
                db    0
                db 0E8h
                db  26h ; &
                db 0DFh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  41h ; A
                db  52h ; R
                db  45h ; E
                db  20h
                db  4Fh ; O
                db  52h ; R
                db  42h ; B
                db  49h ; I
                db  54h ; T
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db    0
                db 0A0h
                db  6Dh ; m
                db    0
                db  3Ch ; <
                db    0
                db  74h ; t
                db  23h ; #
                db  3Ch ; <
                db    1
                db  74h ; t
                db  2Dh ; -
                db  3Ch ; <
                db    2
                db  74h ; t
                db  39h ; 9
                db  3Ch ; <
                db    3
                db  74h ; t
                db  43h ; C
                db  3Ch ; <
                db    4
                db  74h ; t
                db  4Ch ; L
                db  3Ch ; <
                db    5
                db  74h ; t
                db  58h ; X
                db  3Ch ; <
                db    6
                db  74h ; t
                db  63h ; c
                db  3Ch ; <
                db    7
                db  74h ; t
                db  6Eh ; n
                db  3Ch ; <
                db    8
                db  74h ; t
                db  7Ah ; z
                db 0E9h
                db  85h
                db    0
                db 0E8h
                db 0E7h
                db 0DEh
                db  45h ; E
                db  41h ; A
                db  52h ; R
                db  54h ; T
                db  48h ; H
                db  2Eh ; .
                db  8Dh
                db    0
                db 0E9h
                db  81h
                db    0
                db 0E8h
                db 0D9h
                db 0DEh
                db  4Dh ; M
                db  45h ; E
                db  52h ; R
                db  43h ; C
                db  55h ; U
                db  52h ; R
                db  59h ; Y
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  72h ; r
                db  90h
                db 0E8h
                db 0C9h
                db 0DEh
                db  56h ; V
                db  45h ; E
                db  4Eh ; N
                db  55h ; U
                db  53h ; S
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  64h ; d
                db  90h
                db 0E8h
                db 0BBh
                db 0DEh
                db  4Dh ; M
                db  41h ; A
                db  52h ; R
                db  53h ; S
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  57h ; W
                db  90h
                db 0E8h
                db 0AEh
                db 0DEh
                db  4Ah ; J
                db  55h ; U
                db  50h ; P
                db  49h ; I
                db  54h ; T
                db  45h ; E
                db  52h ; R
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  47h ; G
                db  90h
                db 0E8h
                db  9Eh
                db 0DEh
                db  53h ; S
                db  41h ; A
                db  54h ; T
                db  55h ; U
                db  52h ; R
                db  4Eh ; N
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  38h ; 8
                db  90h
                db 0E8h
                db  8Fh
                db 0DEh
                db  55h ; U
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  55h ; U
                db  53h ; S
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  29h ; )
                db  90h
                db 0E8h
                db  80h
                db 0DEh
                db  4Eh ; N
                db  45h ; E
                db  50h ; P
                db  54h ; T
                db  55h ; U
                db  4Eh ; N
                db  45h ; E
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  19h
                db  90h
                db 0E8h
                db  70h ; p
                db 0DEh
                db  50h ; P
                db  4Ch ; L
                db  55h ; U
                db  54h ; T
                db  4Fh ; O
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db  0Bh
                db  90h
                db 0E8h
                db  62h ; b
                db 0DEh
                db  58h ; X
                db  2Eh ; .
                db  8Dh
                db    0
                db 0EBh
                db    1
                db  90h
                db 0C3h
                db 0E8h
                db  3Ch ; <
                db 0FBh
                db 0B0h
                db  80h
                db 0A2h
                db  7Bh ; {
                db    4
                db 0B0h
                db  40h ; @
                db 0A2h
                db  7Ch ; |
                db    4
                db 0E8h
                db  4Ah ; J
                db 0DEh
                db  4Ch ; L
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  49h ; I
                db  4Eh ; N
                db  47h ; G
                db  20h
                db  52h ; R
                db  45h ; E
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db  8Dh
                db    0
                db 0E8h
                db  84h
                db 0FEh
                db 0A0h
                db  6Dh ; m
                db    0
                db  3Ch ; <
                db  0Ah
                db  75h ; u
                db  17h
                db 0E8h
                db  29h ; )
                db 0DEh
                db  52h ; R
                db  45h ; E
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  53h ; S
                db  54h ; T
                db  20h
                db  44h ; D
                db  45h ; E
                db  4Eh ; N
                db  49h ; I
                db  45h ; E
                db  44h ; D
                db  21h ; !
                db  8Dh
                db    0
                db 0E9h
                db 0AFh
                db 0F8h
                db  90h
                db  3Ch ; <
                db    0
                db  75h ; u
                db  38h ; 8
                db 0EBh
                db  17h
                db 0DEh
                db  28h ; (
                db  49h ; I
                db  4Eh ; N
                db  53h ; S
                db  45h ; E
                db  52h ; R
                db  54h ; T
                db  20h
                db  50h ; P
                db  4Ch ; L
                db  41h ; A
                db  59h ; Y
                db  45h ; E
                db  52h ; R
                db  20h
                db  44h ; D
                db  49h ; I
                db  53h ; S
                db  4Bh ; K
                db  29h ; )
                db  8Dh
                db    0
                db 0E8h
                db  52h ; R
                db 0F9h
                db 0BBh
                db  14h
                db    0
                db 0E8h
                db  20h
                db  95h
                db 0B0h
                db  58h ; X
                db 0E8h
                db  15h
                db    0
                db  90h
                db  90h
                db  90h
                db 0B0h
                db    0
                db 0A2h
                db  4Ah ; J
                db    0
                db 0B0h
                db    4
                db 0A2h
                db  49h ; I
                db    0
                db 0E8h
                db  83h
                db    0
                db 0C3h
                db 0EBh
                db  19h
                db 0DDh
                db  28h ; (
                db  2Eh ; .
                db 0A2h
                db 0BAh
                db  22h ; "
                db  2Eh ; .
                db 0A2h
                db 0CCh
                db  22h ; "
                db  2Eh ; .
                db 0A2h
                db 0F7h
                db  22h ; "
                db 0C3h
                db    0
                db  2Dh ; -
                db  55h ; U
                db  44h ; D
                db  49h ; I
                db  43h ; C
                db  2Dh ; -
                db  29h ; )
                db  8Dh
                db    0
                db 0E8h
                db  18h
                db 0F9h
                db 0BBh
                db  14h
                db    0
                db 0E8h
                db 0E6h
                db  94h
                db 0B0h
                db  47h ; G
                db 0E8h
                db 0DBh
                db 0FFh
                db  90h
                db  90h
                db  90h
                db 0B0h
                db    0
                db 0A2h
                db  4Ah ; J
                db    0
                db 0A0h
                db  6Dh ; m
                db    0
                db 0A2h
                db  49h ; I
                db    0
                db 0E8h
                db  48h ; H
                db    0
                db 0C3h
                db  90h
                db  2Eh ; .
                db  80h
                db  3Eh ; >
                db  98h
                db  69h ; i
                db 0FFh
                db  74h ; t
                db  0Dh
                db 0E8h
                db  3Bh ; ;
                db    0
                db  2Eh ; .
                db 0C7h
                db    6
                db  94h
                db  69h ; i
                db 0E0h
                db    0
                db 0EBh
                db  1Eh
                db  90h
                db 0E8h
                db 0BCh
                db 0DFh
                db  8Ah
                db 0E0h
                db  25h ; %
                db    0
                db  1Fh
                db    5
                db    0
                db    1
                db  8Bh
                db 0D8h
                db 0B9h
                db    1
                db    0
                db 0E8h
                db  0Bh
                db    0
                db 0E8h
                db  64h ; d
                db 0EAh
                db  2Eh ; .
                db 0C7h
                db    6
                db  94h
                db  69h ; i
                db  18h
                db    0
                db 0C3h
                db  90h
                db  2Eh ; .
                db  80h
                db  3Eh ; >
                db  98h
                db  69h ; i
                db 0FFh
                db  75h ; u
                db    9
                db 0E8h
                db  30h ; 0
                db 0EAh
                db  2Eh ; .
                db 0C6h
                db    6
                db  98h
                db  69h ; i
                db    0
                db 0C3h
                db  90h
                db  2Eh ; .
                db  80h
                db  3Eh ; >
                db  98h
                db  69h ; i
                db    0
                db  75h ; u
                db    9
                db 0E8h
                db  4Fh ; O
                db 0EAh
                db  2Eh ; .
                db 0C6h
                db    6
                db  98h
                db  69h ; i
                db 0FFh
                db 0C3h
                db    0
                db    0
                db    0
                db    0
                db  90h
                db 0C6h
                db    6
                db  31h ; 1
                db    0
                db  28h ; (
                db 0E8h
                db  36h ; 6
                db 0DDh
                db  8Dh
                db  8Dh
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  4Dh ; M
                db  49h ; I
                db  4Eh ; N
                db  41h ; A
                db  58h ; X
                db  20h
                db  49h ; I
                db  53h ; S
                db  20h
                db  44h ; D
                db  45h ; E
                db  41h ; A
                db  44h ; D
                db  21h ; !
                db  21h ; !
                db  8Dh
                db  41h ; A
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  48h ; H
                db  45h ; E
                db  52h ; R
                db  20h
                db  57h ; W
                db  4Fh ; O
                db  52h ; R
                db  4Bh ; K
                db  53h ; S
                db  20h
                db  53h ; S
                db  48h ; H
                db  41h ; A
                db  4Ch ; L
                db  4Ch ; L
                db  20h
                db  44h ; D
                db  49h ; I
                db  45h ; E
                db  21h ; !
                db  8Dh
                db    0
                db 0B0h
                db  40h ; @
                db 0A2h
                db  1Fh
                db    0
                db 0E8h
                db 0F5h
                db 0EBh
                db 0B0h
                db  40h ; @
                db 0A2h
                db  20h
                db    0
                db 0E8h
                db  2Dh ; -
                db 0DFh
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db  23h ; #
                db    0
                db 0E8h
                db  25h ; %
                db 0DFh
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db  24h ; $
                db    0
                db 0E8h
                db 0B9h
                db 0DDh
                db 0B0h
                db  74h ; t
                db  8Bh
                db  1Eh
                db    6
                db    0
                db  88h
                db    0
                db 0FEh
                db  0Eh
                db  20h
                db    0
                db  75h ; u
                db 0DFh
                db 0E8h
                db 0A6h
                db 0E7h
                db 0BBh
                db  0Eh
                db    0
                db 0E8h
                db    0
                db  94h
                db 0FEh
                db  0Eh
                db  1Fh
                db    0
                db  75h ; u
                db 0C8h
                db 0E8h
                db 0C5h
                db 0DCh
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  46h ; F
                db  45h ; E
                db  45h ; E
                db  4Ch ; L
                db  20h
                db  41h ; A
                db  20h
                db  53h ; S
                db  54h ; T
                db  52h ; R
                db  41h ; A
                db  4Eh ; N
                db  47h ; G
                db  45h ; E
                db  20h
                db  46h ; F
                db  4Fh ; O
                db  52h ; R
                db  43h ; C
                db  45h ; E
                db  21h ; !
                db    0
                db 0E8h
                db  6Eh ; n
                db 0ECh
                db 0B4h
                db  27h ; '
                db 0B9h
                db    0
                db  10h
                db  8Bh
                db  16h
                db  72h ; r
                db    4
                db 0E8h
                db  86h
                db 0DFh
                db  4Dh ; M
                db  41h ; A
                db  50h ; P
                db  58h ; X
                db  33h ; 3
                db  30h ; 0
                db  20h
                db  20h
                db 0E8h
                db  57h ; W
                db 0ECh
                db 0E8h
                db  8Dh
                db 0DCh
                db  8Dh
                db  8Dh
                db  59h ; Y
                db  4Fh ; O
                db  55h ; U
                db  20h
                db  48h ; H
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  20h
                db  53h ; S
                db  41h ; A
                db  56h ; V
                db  45h ; E
                db  44h ; D
                db  20h
                db  54h ; T
                db  48h ; H
                db  45h ; E
                db  20h
                db  55h ; U
                db  4Eh ; N
                db  49h ; I
                db  56h ; V
                db  45h ; E
                db  52h ; R
                db  53h ; S
                db  45h ; E
                db  2Ch ; ,
                db  8Dh
                db  41h ; A
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Dh ; M
                db  50h ; P
                db  4Ch ; L
                db  45h ; E
                db  54h ; T
                db  45h ; E
                db  44h ; D
                db  20h
                db  55h ; U
                db  4Ch ; L
                db  54h ; T
                db  49h ; I
                db  4Dh ; M
                db  41h ; A
                db  20h
                db  5Dh ; ]
                db  5Bh ; [
                db  21h ; !
                db  20h
                db  53h ; S
                db  45h ; E
                db  45h ; E
                db  4Bh ; K
                db  8Dh
                db  4Eh ; N
                db  4Fh ; O
                db  57h ; W
                db  20h
                db  54h ; T
                db  4Fh ; O
                db  20h
                db  43h ; C
                db  4Fh ; O
                db  4Eh ; N
                db  51h ; Q
                db  55h ; U
                db  45h ; E
                db  52h ; R
                db  20h
                db  57h ; W
                db  49h ; I
                db  43h ; C
                db  4Bh ; K
                db  45h ; E
                db  44h ; D
                db  20h
                db  45h ; E
                db  58h ; X
                db  4Fh ; O
                db  44h ; D
                db  55h ; U
                db  53h ; S
                db  2Ch ; ,
                db  8Dh
                db    0
                db 0E8h
                db  2Eh ; .
                db 0DCh
                db  46h ; F
                db  4Fh ; O
                db  55h ; U
                db  4Eh ; N
                db  44h ; D
                db  20h
                db  49h ; I
                db  4Eh ; N
                db  20h
                db  55h ; U
                db  4Ch ; L
                db  54h ; T
                db  49h ; I
                db  4Dh ; M
                db  41h ; A
                db  20h
                db  5Dh ; ]
                db  49h ; I
                db  5Bh ; [
                db  2Dh ; -
                db  44h ; D
                db  20h
                db  5Dh ; ]
                db  49h ; I
                db  49h ; I
                db  5Bh ; [
                db  2Dh ; -
                db  50h ; P
                db  21h ; !
                db    0
                db 0B0h
                db 0FFh
                db 0A2h
                db  12h
                db    0
                db 0B0h
                db  28h ; (
                db 0A2h
                db  13h
                db    0
                db 0B0h
                db    0
                db 0A2h
                db    0
                db    0
                db 0A2h
                db    1
                db    0
                db 0F8h
                db 0A0h
                db    1
                db    0
                db  14h
                db    1
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    1
                db    0
                db 0F8h
                db 0D0h
                db 0D8h
                db  72h ; r
                db  0Fh
                db 0F8h
                db 0D0h
                db 0D8h
                db  72h ; r
                db  0Ah
                db 0A0h
                db    0
                db    0
                db  14h
                db    1
                db  24h ; $
                db  3Fh ; ?
                db 0A2h
                db    0
                db    0
                db 0E8h
                db 0AEh
                db 0E6h
                db 0BBh
                db    6
                db    0
                db 0E8h
                db    8
                db  93h
                db 0EBh
                db 0D6h
                db    0
                db    0
                db    0
                db    0
                db    0
sg01a2          ends

; ===========================================================================

; Segment type: Regular
sg08e3          segment byte public 'UNK' use16
                assume cs:sg08e3
                assume es:nothing, ss:nothing, ds:nothing, fs:nothing, gs:nothing
_mapX           db 0                    ; DATA XREF: sg01a2:0829↑w
                                        ; canMoveToTile-1A80↑w ...
_mapY           db 0                    ; DATA XREF: sg01a2:082F↑w
                                        ; canMoveToTile-1ACE↑w ...
_mapLeft        db 0                    ; DATA XREF: canMoveToTile+46C↑r
                                        ; canMoveToTile+504↑w ...
_mapTop         db 0                    ; DATA XREF: canMoveToTile+479↑r
                                        ; canMoveToTile+4FD↑r ...
_mapOffsetX     db 0                    ; DATA XREF: draw_map+25↑w
                                        ; draw_map+31↑r ...
_mapOffsetY     db 0                    ; DATA XREF: draw_map+22↑w
                                        ; draw_map+3D↑r ...
current_tile_ptr dw 0                   ; DATA XREF: sg01a2:084A↑r
                                        ; canMoveToTile-16D2↑r ...
word_17418      dw 0                    ; DATA XREF: canMoveToTile-11CC↑r
                                        ; canMoveToTile-11C4↑r ...
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
_outsideMapTile db 0                    ; DATA XREF: sg01a2:0869↑w
                                        ; canMoveToTile-1802↑w ...
_playerTileId   db 0                    ; DATA XREF: sg01a2:0815↑w
                                        ; canMoveToTile-1B47↑r ...
_tilePlayerCenter db 0                  ; DATA XREF: canMoveToTile-1895↑r
                                        ; draw_map+C3↑w ...
_tilePlayerUp   db 0                    ; DATA XREF: canMoveToTile-1AC1↑r
                                        ; canMoveToTile+44A↑r ...
_tilePlayerDown db 0                    ; DATA XREF: canMoveToTile-1A9A↑r
                                        ; canMoveToTile+560↑r ...
_tilePlayerRight db 0                   ; DATA XREF: canMoveToTile-1A4E↑r
                                        ; draw_map+DB↑w ...
_tilePlayerLeft db 0                    ; DATA XREF: canMoveToTile-1A74↑r
                                        ; draw_map+D5↑w ...
_circleDeltaX   db 0                    ; DATA XREF: canMoveToTile-1973↑r
                                        ; canMoveToTile-1910↑r ...
_circleDeltaY   db 0                    ; DATA XREF: canMoveToTile-1969↑r
                                        ; canMoveToTile-1905↑r ...
                db    0
                db    0
                db    0
                db    0
byte_1742F      db 0                    ; DATA XREF: canMoveToTile-1771↑w
                                        ; canMoveToTile-1761↑r ...
byte_17430      db 0                    ; DATA XREF: canMoveToTile-19E6↑w
                                        ; canMoveToTile:loc_10B4D↑w ...
_commandWaitCtr db 0                    ; DATA XREF: canMoveToTile-1B8B↑w
                                        ; canMoveToTile-1B4D↑w ...
byte_17432      db 0                    ; DATA XREF: canMoveToTile-1B86↑w
                                        ; canMoveToTile-1B2A↑w ...
_playerX        db 0                    ; DATA XREF: sg01a2:083C↑w
                                        ; canMoveToTile-1757↑w ...
_playerY        db 0                    ; DATA XREF: sg01a2:0842↑w
                                        ; canMoveToTile-173B↑w ...
byte_17435      db 0                    ; DATA XREF: canMoveToTile-1404↑w
                                        ; canMoveToTile-13B4↑r ...
byte_17436      db 0                    ; DATA XREF: canMoveToTile-1B22↑w
                                        ; canMoveToTile-1AE1↑w ...
_sleepFlag2?    db 0                    ; DATA XREF: canMoveToTile-1B7A↑w
                                        ; canMoveToTile-1B64↑w ...
byte_17438      db 0                    ; DATA XREF: sub_12052+6↑r
                                        ; sub_12052+22↑r ...
points_to_distrubte db 0                ; DATA XREF: canMoveToTile-1977↑w
                                        ; canMoveToTile-195A↑r ...
_attribPoints   db 0                    ; DATA XREF: update_points_remaining↑w
                                        ; update_points_remaining+A↑r
word_1743B      dw 0                    ; DATA XREF: write_string+4↑w
                                        ; write_string:loc_14FEC↑w ...
                db    0
text_x          db 0                    ; DATA XREF: start_+14D↑w
                                        ; start_+164↑w ...
text_y          db 0                    ; DATA XREF: start_+146↑w
                                        ; start_+15D↑w ...
                db    0
text_width?     db 0                    ; DATA XREF: set_cga_mode+13↑w
                                        ; write_character+29↑r ...
                db    0
                db    0
                db    0
                db    0
player          Savegame <0>            ; DATA XREF: sg01a2:0799↑o
                                        ; sg01a2:07AA↑r ...
                db    0
_mapMonsters    db 0BCh dup(     0)     ; DATA XREF: load_map+32↑o
                                        ; sg01a2:2343↑o
unk_17603       db    0                 ; DATA XREF: seg002:0221↓o
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
                db    0
                db    0
                db    0
                db    0
_demoMode       db 0                    ; DATA XREF: start_+35↑w
                                        ; start_+138↑r ...
                db    0
                db    0
                db    0
                db    0
text_color      db 0Fh                  ; DATA XREF: write_character+3F↑r
                                        ; set_normal_text_color↑w ...
_timer1         db 0                    ; DATA XREF: start_+16↑w
                                        ; start_+28↑w ...
_timer2         db 0                    ; DATA XREF: start_+19↑w
                                        ; start_+2C↑w ...
_timer3         db 0                    ; DATA XREF: start_+1C↑w
_timer4         db 0                    ; DATA XREF: start_+1F↑w
_timer5         db 0                    ; DATA XREF: start_+22↑w
                                        ; sub_15217+6↑r
_timer6         db 0                    ; DATA XREF: start_+25↑w
                                        ; sub_15217+A↑r
_picData        FCB <0>                 ; DATA XREF: access_file:loc_152E7↑w
                                        ; access_file+47↑o ...
_flag1          db 0                    ; DATA XREF: sg01a2:081A↑w
                                        ; canMoveToTile:loc_10C54↑r ...
player_paralyzedFlag db 0               ; DATA XREF: sg01a2:081D↑w
                                        ; canMoveToTile:loc_10C33↑r ...
_flag3          db 0                    ; DATA XREF: sg01a2:0820↑w
                                        ; canMoveToTile:loc_10C3E↑r ...
_sleepFlag      db 0                    ; DATA XREF: sg01a2:0823↑w
                                        ; canMoveToTile-1B83↑r ...
NORTH_KEYCODE   db 0Eh                  ; DATA XREF: canMoveToTile:not_pass↑r
                                        ; sg01a2:2396↑r ...
byte_1767F      db 19h                  ; DATA XREF: canMoveToTile:not_north↑r
                                        ; sg01a2:239C↑r ...
byte_17680      db 5                    ; DATA XREF: canMoveToTile:loc_109C3↑r
                                        ; sg01a2:23A2↑r ...
byte_17681      db 17h                  ; DATA XREF: canMoveToTile:loc_1099D↑r
                                        ; sg01a2:23A8↑r ...
_mapTileIds     db 100h dup(     0)     ; DATA XREF: sg01a2:085A↑w
                                        ; setPalette:loc_14A64↑w ...
_priorMapTileIds db 100h dup(     0)    ; DATA XREF: sg01a2:085E↑w
                                        ; setPalette+2A↑w ...
map_ptr         dw 1800h                ; DATA XREF: map_get_monster_at?+A↑r
                                        ; load_map+20↑r ...
monsters_ptr    dw 2900h                ; DATA XREF: sg01a2:07F6↑r
                                        ; sub_168ED+22↑r ...
                db    0
                db  28h ; (
text_mode?      db 0                    ; DATA XREF: set_cga_mode+18↑w
                                        ; setPalette+18↑w ...
byte_17889      db 0                    ; DATA XREF: sub_14B76+3↑r
                                        ; sub_16078+B↑w ...
byte_1788A      db 0                    ; DATA XREF: sub_14B76+E↑r
                                        ; sub_16078+1B↑w ...
byte_1788B      db 0                    ; DATA XREF: sub_16078+4↑w
                                        ; sub_16078+65↑r ...
byte_1788C      db 0                    ; DATA XREF: sub_16078+12↑w
                                        ; sub_16078+75↑r ...
byte_1788D      db 0                    ; DATA XREF: sub_16078+28↑w
                                        ; sub_16078+97↑r ...
byte_1788E      db 0                    ; DATA XREF: sub_16078+33↑w
                                        ; sub_16078+A7↑r ...
byte_1788F      db 0                    ; DATA XREF: sub_168A3+3↑r
                                        ; sub_168ED+85↑w
byte_17890      db 0                    ; DATA XREF: sub_167B3+6D↑w
                                        ; sub_167B3+A6↑w ...
byte_17891      db 0                    ; DATA XREF: sub_167B3+73↑w
                                        ; sub_167B3+77↑r ...
byte_17892      db 0                    ; DATA XREF: sub_16144+5↑w
                                        ; sub_16144+D↑r ...
byte_17893      db 0                    ; DATA XREF: sub_16000+4↑w
                                        ; sub_16000+20↑w ...
byte_17894      db 0                    ; DATA XREF: sub_16000+A↑w
                                        ; sub_16000+26↑w ...
                db    0
                db    0
word_17897      dw 0                    ; DATA XREF: sub_168C0+1D↑w
                                        ; sub_168C0+26↑r ...
word_17899      dw 0                    ; DATA XREF: sub_168ED+2E↑w
                                        ; sub_168ED+36↑r ...
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
word_178CF      dw 10h                  ; DATA XREF: sub_14B76+6↑r
                                        ; sub_14BE0+6↑r
word_178D1      dw 10h                  ; DATA XREF: sub_14B76+11↑r
                                        ; sub_14BE0+15↑r
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
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
byte_1795D      db 0                    ; DATA XREF: keypress_check+2E↑w
                                        ; sub_15C95↑r ...
map_freezeAnimation db 0                ; DATA XREF: draw_map+8C↑r
                                        ; draw_map+164↑r
byte_1795F      db 0                    ; DATA XREF: write_character:loc_1500D↑r
                db    0
                db    0
                db    0
                db    0
speed_divisor   dw 1674                 ; DATA XREF: setup_speed_array+6↑r
                                        ; setup_speed_array+16↑r ...
                db    0
                db    0
array1          dw      6,  2DEh, 2000h,  500h,     3,     3,     3,  500h
                                        ; DATA XREF: setup_speed_array:loc_106FF↑r
                dw    0Ah, 1000h,  1E0h,  140h,  240h,  200h,  800h,     1
                dw      0,     0,     0,     0
array2          dw 14h dup(     0)      ; DATA XREF: setup_speed_array:loc_10707↑w
                                        ; sub_15CD2:loc_15CDA↑r ...
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
TILE_OFFSETS    dw offset byte_17A40,offset byte_17A82,offset byte_17AC4,offset byte_17B06,offset unk_17B48,offset unk_17B8A,offset unk_17BCC,offset unk_17C0E
                                        ; DATA XREF: draw_map_content+1A↑r
                                        ; animate_water+1↑r
                dw offset unk_17C50,offset unk_17C92,offset unk_17CD4,offset unk_17D16,offset unk_17D58,offset unk_17D9A,offset unk_17DDC,offset unk_17E1E
                dw offset unk_17E60,offset unk_17EA2,offset unk_17EE4,offset unk_17F26,offset unk_17F68,offset unk_17FAA,offset unk_17FEC,offset unk_1802E
                dw offset unk_18070,offset unk_180B2,offset unk_180F4,offset unk_18136,offset unk_18178,offset unk_181BA,offset unk_181FC,offset unk_1823E
                dw offset unk_18280,offset unk_182C2,offset unk_18304,offset unk_18346,offset unk_18388,offset unk_183CA,offset unk_1840C,offset unk_1844E
                dw offset unk_18490,offset unk_184D2,offset unk_18514,offset unk_18556,offset unk_18598,offset unk_185DA,offset unk_1861C,offset unk_1865E
                dw offset unk_186A0,offset unk_186E2,offset unk_18724,offset unk_18766,offset unk_187A8,offset unk_187EA,offset unk_1882C,offset unk_1886E
                dw offset unk_188B0,offset unk_188F2,offset unk_18934,offset unk_18976,offset unk_189B8,offset unk_189FA,offset unk_18A3C,offset unk_18A7E
byte_17A40      db      4,   10h,8 dup(     0),   22h,     0,   20h,     2,     0
                                        ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db    22h,     2,   20h,9 dup(     0),   20h,     2,2 dup(   22h),     2
                db    20h,9 dup(     0),   20h,     2,   22h,     0,     2,   20h
                db      0,   22h,9 dup(     0),   22h,     2,   20h,   22h,     0
                db    20h,     2
byte_17A82      db      4,   10h,     0,   11h,6 dup(     0),     3,     0,     1
                                        ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db    10h,4 dup(     0),     1,   10h,     0,   30h,5 dup(     0),   11h
                db      0,     3,5 dup(     0),     3,     0,   11h,4 dup(     0),     1
                db    10h,     0,   30h,4 dup(     0),     3,     0,   11h,5 dup(     0)
                db    11h,     0,   30h,5 dup(     0)
byte_17AC4      db      4,   10h,     0,     1,4 dup(     0),     1,     0,     1
                                        ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db 0Ah dup(     0),     1,     0,   10h,9 dup(     0),   10h,     0,     1
                db 6 dup(     0),     1,5 dup(     0),     1,8 dup(     0),   10h,     0,   10h
                db 6 dup(     0)
byte_17B06      db      4,   10h,     1,   50h,2 dup(     0),   15h,   55h,2 dup(     0)
                                        ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db    15h,   55h,2 dup(     0),   15h,   55h,2 dup(     0),     1,   50h
                db 4 dup(     0),     1,   50h,2 dup(     0),   15h,   55h,2 dup(     0),   15h
                db    55h,2 dup(     0),   15h,   55h,2 dup(     0),     1,   50h,     1
                db    50h,2 dup(     0),   15h,   55h,2 dup(     0),   15h,   55h,2 dup(     0)
                db    15h,   55h,2 dup(     0),     1,   50h,6 dup(     0)
unk_17B48       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0C0h
                db 0C0h
                db  0Ch
                db    0
                db  33h ; 3
                db  30h ; 0
                db  33h ; 3
                db    3
                db  0Ch
                db  0Ch
                db 0C0h
                db 0CCh
                db  30h ; 0
                db    3
                db    0
                db  30h ; 0
                db 0C0h
                db  0Ch
                db    0
                db  0Ch
                db    0
                db  30h ; 0
                db    0
                db    3
                db 0C0h
                db 0C0h
                db 0C0h
                db  0Ch
                db  33h ; 3
                db    3
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  0Ch
                db 0C0h
                db    3
                db  30h ; 0
                db    3
                db    3
                db 0C0h
                db 0C0h
                db    0
                db 0CCh
                db  33h ; 3
                db    0
                db    0
                db  30h ; 0
                db  0Ch
                db    0
                db    0
                db 0C0h
                db    3
                db    3
                db    3
                db    0
                db    0
                db 0CCh
                db 0CCh
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
unk_17B8A       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db  0Fh
                db    0
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db 0C0h
                db    3
                db 0C0h
                db  30h ; 0
                db 0C0h
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db  30h ; 0
                db 0C0h
                db    0
                db    0
                db  30h ; 0
                db 0C0h
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    3
                db 0C0h
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17BCC       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0FCh
                db  3Fh ; ?
                db 0F0h
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Fh
                db 0FCh
                db  3Fh ; ?
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17C0E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
unk_17C50       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0FCh
                db  3Fh ; ?
                db 0C0h
                db  0Fh
                db 0FCh
                db  3Fh ; ?
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17C92       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0C0h
                db 0C0h
                db  0Ch
                db    0
                db  33h ; 3
                db  30h ; 0
                db  33h ; 3
                db    3
                db  0Ch
                db  0Ch
                db 0C0h
                db 0CCh
                db  30h ; 0
                db    3
                db    0
                db  30h ; 0
                db 0C0h
                db  0Ch
                db    0
                db  0Ch
                db    0
                db  30h ; 0
                db    0
                db    3
                db 0C0h
                db 0C0h
                db 0C0h
                db  0Ch
                db  33h ; 3
                db    3
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db    0
                db    0
                db 0C0h
                db    3
                db  0Fh
                db 0F0h
                db    3
                db 0C0h
                db  3Ch ; <
                db  3Ch ; <
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db 0C0h
                db    3
                db  30h ; 0
                db  0Ch
                db    3
                db 0C0h
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db  30h ; 0
unk_17CD4       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
unk_17D16       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  22h ; "
                db    0
                db  20h
                db    2
                db    0
                db  22h ; "
                db    2
                db  20h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db  20h
                db  3Fh ; ?
                db 0FCh
                db  22h ; "
                db    0
                db 0C3h
                db 0FCh
                db    0
                db    0
                db  0Ch
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db  20h
                db    0
                db    0
                db  3Ch ; <
                db    0
                db 0FFh
                db 0C0h
                db  3Ch ; <
                db    3
                db 0FFh
                db 0F0h
                db  3Ch ; <
                db  0Fh
                db 0FFh
                db 0FCh
                db  3Ch ; <
                db    0
                db  22h ; "
                db    2
                db  20h
                db  22h ; "
                db    2
                db  22h ; "
                db    2
unk_17D58       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  30h ; 0
                db  0Ch
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db  0Fh
                db  0Fh
                db 0F0h
                db 0F0h
                db  0Fh
                db  0Fh
                db 0F0h
                db 0F0h
                db  0Fh
                db  3Fh ; ?
                db 0FCh
                db 0F0h
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17D9A       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db  30h ; 0
                db  3Fh ; ?
                db 0FCh
                db  0Ch
                db  0Ch
                db 0CFh
                db 0F3h
                db  30h ; 0
                db    3
                db  0Fh
                db 0F0h
                db 0C0h
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17DDC       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db  80h
                db    0
                db 0C3h
                db    8
                db    8
                db    0
                db 0FFh
                db    8
                db  88h
                db    0
                db 0FFh
                db    3
                db 0F0h
                db    0
                db  3Ch ; <
                db    0
                db 0C0h
                db    3
                db 0FFh
                db 0C0h
                db 0C0h
                db  0Ch
                db 0FFh
                db  33h ; 3
                db 0C0h
                db  0Ch
                db  3Ch ; <
                db  0Ch
                db 0C0h
                db  0Ch
                db  3Ch ; <
                db    0
                db 0C0h
                db  0Ch
                db 0FFh
                db    0
                db 0C0h
                db    0
                db 0FFh
                db    0
                db 0C0h
                db    0
                db 0C3h
                db    0
                db 0C0h
                db    0
                db 0C3h
                db    0
                db 0C0h
                db    0
                db 0C3h
                db    0
                db 0C0h
                db    3
                db 0C3h
                db 0C0h
                db 0C0h
                db    0
                db    0
                db    0
                db    0
unk_17E1E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    3
                db  0Fh
                db 0F0h
                db 0C0h
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db  3Fh ; ?
                db 0FFh
                db 0FFh
                db 0FCh
                db  3Fh ; ?
                db 0FFh
                db 0FFh
                db 0FCh
                db  3Fh ; ?
                db 0CFh
                db 0F3h
                db 0FCh
                db  3Fh ; ?
                db 0CFh
                db 0F3h
                db 0FCh
                db  3Fh ; ?
                db 0C3h
                db 0C3h
                db 0FCh
                db  3Fh ; ?
                db 0CFh
                db 0F3h
                db 0FCh
                db  3Fh ; ?
                db  0Fh
                db 0F0h
                db 0FCh
                db  3Fh ; ?
                db  0Ch
                db  30h ; 0
                db 0FCh
                db  3Ch ; <
                db  0Ch
                db  30h ; 0
                db  3Ch ; <
                db  30h ; 0
                db  3Ch ; <
                db  3Ch ; <
                db  0Ch
                db    0
                db    0
                db    0
                db    0
unk_17E60       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db  3Fh ; ?
                db 0FFh
                db 0FFh
                db 0F0h
                db  0Fh
                db 0FFh
                db 0FFh
                db 0C0h
                db  0Fh
                db 0CFh
                db 0CFh
                db 0C0h
                db  0Fh
                db 0C3h
                db  0Fh
                db 0C0h
                db  0Fh
                db  0Fh
                db 0C3h
                db 0C0h
                db  0Fh
                db  0Fh
                db 0C3h
                db 0C0h
                db  0Ch
                db  3Fh ; ?
                db 0F0h
                db 0C0h
                db  0Ch
                db  3Fh ; ?
                db 0F0h
                db 0C0h
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
unk_17EA2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
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
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  0Ch
                db 0FFh
                db 0FFh
                db 0F0h
                db    0
                db  3Fh ; ?
                db 0FFh
                db 0FCh
                db    0
                db 0FFh
                db 0FFh
                db 0FCh
                db    0
                db 0FFh
                db 0FFh
                db 0F0h
                db    3
                db  30h ; 0
                db  0Fh
                db  3Ch ; <
                db  0Ch
                db  30h ; 0
                db    3
                db  0Ch
                db  0Ch
                db  30h ; 0
                db    3
                db  0Ch
                db    0
                db 0C0h
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db    0
                db    0
unk_17EE4       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db 0C3h
                db  0Ch
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db  0Fh
                db 0F0h
                db 0FFh
                db 0C0h
                db  0Ch
                db  30h ; 0
                db 0C0h
                db 0C0h
                db  0Ch
                db  30h ; 0
                db 0C0h
                db 0C0h
                db  0Fh
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db 0C3h
                db  0Ch
                db    0
                db  3Fh ; ?
                db 0FFh
                db 0FFh
                db 0FCh
                db  3Fh ; ?
                db 0CFh
                db  3Ch ; <
                db 0F0h
                db  0Fh
                db 0FFh
                db 0FFh
                db 0C0h
                db    3
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17F26       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    3
                db 0C0h
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  0Fh
                db 0FCh
                db  3Fh ; ?
                db 0F0h
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  30h ; 0
                db    0
                db    3
                db 0FCh
                db  3Fh ; ?
                db 0C0h
                db  0Ch
                db    3
                db 0C0h
                db  30h ; 0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db    3
                db 0C0h
                db    0
unk_17F68       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    3
                db  30h ; 0
                db    0
                db    0
                db    3
                db  30h ; 0
                db    0
                db    0
                db    3
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db  30h ; 0
                db 0C3h
                db    0
                db    0
                db 0C3h
                db 0F0h
                db 0C0h
                db    3
                db  0Ch
                db 0CCh
                db  30h ; 0
                db    3
                db 0F0h
                db 0C3h
                db 0F0h
unk_17FAA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db  0Ch
                db    0
                db    0
                db  30h ; 0
                db  0Ch
                db  0Fh
                db 0FCh
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db  3Ch ; <
                db  30h ; 0
                db  0Ch
                db  30h ; 0
                db    0
                db  30h ; 0
                db  0Ch
                db  0Fh
                db 0F0h
                db  30h ; 0
                db  0Ch
                db    0
                db  0Ch
                db  30h ; 0
                db  0Ch
                db    3
                db 0F0h
                db  30h ; 0
                db  0Ch
                db  0Ch
                db    0
                db  30h ; 0
                db  0Ch
                db    3
                db 0C0h
                db  30h ; 0
                db    3
                db    0
                db  30h ; 0
                db 0C0h
                db    3
                db    3
                db 0C0h
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
unk_17FEC       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
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
                db  30h ; 0
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db  0Ch
                db    0
                db    0
                db    0
                db  30h ; 0
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db  0Ch
                db  0Ch
                db    0
                db    0
                db    3
                db  30h ; 0
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    3
                db  30h ; 0
                db    0
                db    0
                db  0Ch
                db  0Ch
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
unk_1802E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db  88h
                db  88h
                db  88h
                db  88h
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db  88h
                db  88h
                db  88h
                db  88h
                db  88h
                db  88h
                db  88h
                db  88h
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  33h ; 3
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db  11h
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db 0CCh
                db  88h
                db  88h
                db  88h
                db  88h
unk_18070       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db  0Fh
                db 0FFh
                db 0FFh
                db 0F0h
                db  3Fh ; ?
                db  3Fh ; ?
                db 0FCh
                db 0FCh
                db  3Ch ; <
                db  0Fh
                db 0F0h
                db  3Ch ; <
                db  3Ch ; <
                db    3
                db 0C0h
                db  3Ch ; <
                db  3Ch ; <
                db    3
                db 0C0h
                db  3Ch ; <
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  3Fh ; ?
                db    0
unk_180B2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db  0Ch
                db  0Fh
                db 0C0h
                db 0C0h
                db  0Ch
                db  0Fh
                db 0C0h
                db 0C0h
                db  0Fh
                db  0Fh
                db 0C3h
                db    0
                db    3
                db 0C3h
                db  0Fh
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  3Ch ; <
                db 0F0h
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    3
                db 0C0h
                db  0Fh
                db    0
                db  0Fh
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    0
                db    0
unk_180F4       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    3
                db  3Fh ; ?
                db 0F3h
                db    0
                db    3
                db  0Fh
                db 0C3h
                db    0
                db    3
                db    3
                db    3
                db    0
                db    3
                db  0Fh
                db 0C3h
                db    0
                db    3
                db  0Fh
                db 0C3h
                db    0
                db    0
                db  3Ch ; <
                db 0F0h
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db    0
                db    0
                db    0
unk_18136       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db  0Fh
                db 0CFh
                db 0F3h
                db 0F0h
                db  0Fh
                db    3
                db 0C0h
                db 0F0h
                db  0Fh
                db  0Fh
                db 0F0h
                db 0F0h
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
unk_18178       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  88h
                db  88h
                db  88h
                db  88h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db  88h
                db  88h
                db  88h
                db  88h
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  88h
                db  88h
                db  88h
                db  88h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db    0
                db  80h
                db  88h
                db  88h
                db  88h
                db  88h
unk_181BA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
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
unk_181FC       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1823E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
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
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18280       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FCh
                db    0
                db    0
                db    3
                db 0CFh
                db 0C0h
                db    0
                db  0Fh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_182C2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Fh ; ?
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db 0C0h
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18304       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0C0h
                db    0
                db    3
                db 0CFh
                db    0
                db    0
                db  0Fh
                db  0Fh
                db    0
                db    0
                db  3Fh ; ?
                db    3
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db 0FCh
                db    0
                db    0
                db    0
                db 0FCh
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db    3
                db    0
                db    0
                db  0Fh
                db    3
                db    0
                db    0
                db    3
                db 0CFh
                db    0
                db    0
                db    0
                db 0FCh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18346       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db 0F0h
                db 0F0h
                db    0
                db    3
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db 0F0h
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18388       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0C0h
                db    0
                db    3
                db 0CFh
                db    0
                db    0
                db  0Fh
                db    3
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db  3Ch ; <
                db  0Ch
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    0
                db 0FCh
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  0Ch
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db  0Fh
                db    3
                db    0
                db    0
                db    3
                db 0CFh
                db    0
                db    0
                db    0
                db 0FCh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_183CA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db  30h ; 0
                db    0
                db    0
                db  3Fh ; ?
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db  30h ; 0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db 0FFh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1840C       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db    0
                db    0
                db  0Fh
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  0Ch
                db    0
                db    0
                db 0F0h
                db    0
                db    0
                db    3
                db 0F0h
                db    0
                db    0
                db    3
                db 0F0h
                db 0FCh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db 0C0h
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  0Fh
                db  3Ch ; <
                db    0
                db    0
                db    3
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1844E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db 0FFh
                db    0
                db    0
                db 0F0h
                db    3
                db    0
                db    0
                db 0F0h
                db    0
                db    0
                db    0
                db 0F3h
                db 0C0h
                db    0
                db    0
                db 0FFh
                db 0F0h
                db    0
                db    0
                db 0F0h
                db 0FCh
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  30h ; 0
                db    0
                db    3
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18490       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_184D2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0FFh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db    0
                db  0Fh
                db    0
                db    0
                db 0C0h
                db  0Fh
                db    0
                db    0
                db 0C0h
                db  0Fh
                db    0
                db    0
                db 0C0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0FCh
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18514       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db  30h ; 0
                db    0
                db    0
                db  3Ch ; <
                db 0FCh
                db    0
                db    0
                db  3Fh ; ?
                db 0CFh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Fh ; ?
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db 0FCh
                db    0
                db    0
                db  3Ch ; <
                db  3Fh ; ?
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db 0FFh
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18556       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db    3
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Fh ; ?
                db    0
                db    0
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18598       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db 0F3h
                db 0CFh
                db    0
                db    3
                db 0C3h
                db 0C3h
                db 0C0h
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db  0Fh
                db    3
                db 0C0h
                db 0F0h
                db  3Fh ; ?
                db    3
                db 0C0h
                db 0FCh
                db  3Fh ; ?
                db    3
                db 0C0h
                db 0FCh
                db  0Fh
                db    3
                db 0C0h
                db 0F0h
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db    3
                db 0C3h
                db 0C3h
                db 0C0h
                db    0
                db 0C3h
                db 0C3h
                db    0
                db  0Fh
                db 0CFh
                db 0F3h
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_185DA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db 0F0h
                db 0F0h
                db    0
                db    3
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  30h ; 0
                db    0
                db    3
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1861C       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db 0C0h
                db    3
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  3Fh ; ?
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1865E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db 0C0h
                db    0
                db  3Ch ; <
                db  0Fh
                db 0C0h
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db 0F0h
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db  3Ch ; <
                db    0
                db    0
                db    0
                db 0FFh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_186A0       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FCh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FCh
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
                db 0F3h
                db  33h ; 3
                db  33h ; 3
                db  3Ch ; <
unk_186E2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db 0F0h
                db 0F0h
                db    0
                db    3
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  3Fh ; ?
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F3h
                db 0F0h
                db    0
                db    0
                db 0F0h
                db  30h ; 0
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0FCh
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18724       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db    0
                db    0
                db  0Fh
                db  3Ch ; <
                db    0
                db    0
                db  3Ch ; <
                db  0Ch
                db    0
                db    0
                db 0FFh
                db    0
                db    0
                db    0
                db 0FFh
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    3
                db 0FCh
                db    0
                db    0
                db    0
                db 0FFh
                db    0
                db    0
                db 0C0h
                db 0FFh
                db    0
                db    0
                db 0F3h
                db 0F0h
                db    0
                db    0
                db 0FFh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18766       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db 0F3h
                db 0CFh
                db    0
                db    0
                db 0C3h
                db 0C3h
                db    0
                db    0
                db 0C3h
                db 0C3h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db    3
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_187A8       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FCh
                db  0Fh
                db 0C0h
                db    0
                db  0Ch
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db 0C0h
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_187EA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0F0h
                db 0F0h
                db    0
                db    0
                db  30h ; 0
                db  3Ch ; <
                db    0
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db  0Fh
                db 0C0h
                db    3
                db 0F0h
                db  0Fh
                db 0C0h
                db    3
                db 0F0h
                db    3
                db 0C0h
                db    3
                db 0C0h
                db    3
                db 0F0h
                db  0Fh
                db 0C0h
                db    0
                db 0F0h
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  3Ch ; <
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1882C       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db    0
                db 0C3h
                db 0C3h
                db    0
                db    3
                db 0C3h
                db 0C3h
                db 0C0h
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db  0Fh
                db    3
                db 0C0h
                db 0F0h
                db  3Fh ; ?
                db    3
                db 0C0h
                db 0FCh
                db  3Fh ; ?
                db    3
                db 0C0h
                db 0FCh
                db  0Fh
                db    3
                db 0C0h
                db 0F0h
                db  0Fh
                db 0C3h
                db 0C3h
                db 0F0h
                db    3
                db 0C3h
                db 0C3h
                db 0C0h
                db    0
                db 0F3h
                db 0CFh
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_1886E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0F0h
                db  3Fh ; ?
                db 0C0h
                db    3
                db 0C0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0FCh
                db 0F0h
                db    0
                db    0
                db 0FFh
                db 0C0h
                db    0
                db    0
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0F0h
                db    0
                db    0
                db  3Fh ; ?
                db 0FCh
                db    0
                db    0
                db 0F0h
                db 0FCh
                db    0
                db    3
                db 0C0h
                db  3Fh ; ?
                db    0
                db  0Fh
                db    0
                db  0Fh
                db    0
                db  0Fh
                db 0C0h
                db  3Fh ; ?
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_188B0       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Ch ; <
                db  3Fh ; ?
                db 0C0h
                db    0
                db  0Ch
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db    0
                db    3
                db 0F0h
                db  0Fh
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    0
                db  3Ch ; <
                db  0Fh
                db    0
                db    0
                db  0Fh
                db 0FFh
                db    0
                db    0
                db 0C0h
                db  0Fh
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db 0FFh
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_188F2       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db 0FFh
                db    0
                db    3
                db 0C0h
                db 0FCh
                db    0
                db    3
                db    0
                db 0FCh
                db    0
                db    0
                db    3
                db 0F0h
                db    0
                db    0
                db    3
                db 0F0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  3Fh ; ?
                db    3
                db    0
                db    0
                db  3Fh ; ?
                db    3
                db    0
                db    0
                db 0FCh
                db    3
                db    0
                db    0
                db 0FCh
                db  0Fh
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18934       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db    3
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_18976       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0C0h
                db    0
                db    0
                db    0
                db 0FFh
                db 0FFh
                db 0FFh
                db 0FFh
unk_189B8       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db    3
                db    0
                db    0
                db    0
                db 0FFh
                db 0FCh
                db    0
                db    3
                db  3Fh ; ?
                db 0F3h
                db    0
                db  0Ch
                db  0Fh
                db 0C0h
                db 0C0h
                db    3
                db    3
                db    3
                db    0
                db    0
                db 0CFh
                db 0CCh
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  3Ch ; <
                db 0F0h
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
                db    0
                db  30h ; 0
                db  30h ; 0
                db    0
                db    0
                db 0F0h
                db  3Ch ; <
                db    0
                db    0
                db    0
                db    0
                db    0
unk_189FA       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db 0C0h
                db    0
                db  3Fh ; ?
                db    0
                db 0C0h
                db    0
                db  3Fh ; ?
                db  0Fh
                db 0FCh
                db    0
                db  0Ch
                db    0
                db 0C0h
                db    3
                db 0FFh
                db 0F0h
                db 0C0h
                db  0Ch
                db 0FFh
                db 0CCh
                db 0C0h
                db  0Ch
                db  3Fh ; ?
                db    3
                db 0C0h
                db  0Ch
                db  0Ch
                db    0
                db 0C0h
                db  0Ch
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db 0F3h
                db 0C0h
                db    0
                db    0
                db 0C0h
                db 0C0h
                db    0
                db    0
                db 0C0h
                db 0C0h
                db    0
                db    0
                db 0C0h
                db 0C0h
                db    0
                db    3
                db 0C0h
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
unk_18A3C       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  0Fh
                db 0C0h
                db    0
                db    0
                db  0Fh
                db 0C0h
                db  0Ch
                db    0
                db  0Fh
                db 0C0h
                db  0Ch
                db    0
                db    3
                db    0
                db  0Ch
                db    0
                db 0FFh
                db 0FCh
                db  0Ch
                db  33h ; 3
                db  3Fh ; ?
                db 0F3h
                db  3Ch ; <
                db  0Ch
                db  0Fh
                db 0C0h
                db 0CCh
                db    0
                db    3
                db    0
                db  0Ch
                db    0
                db  0Fh
                db 0C0h
                db  0Ch
                db    0
                db  3Ch ; <
                db 0F0h
                db  0Ch
                db    0
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db    0
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db    0
                db  30h ; 0
                db  30h ; 0
                db  0Ch
                db    0
                db 0F0h
                db  3Ch ; <
                db  0Ch
                db    0
                db    0
                db    0
                db    0
unk_18A7E       db    4                 ; DATA XREF: sg08e3:TILE_OFFSETS↑o
                db  10h
                db    0
                db    0
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db  3Fh ; ?
                db    0
                db    0
                db    0
                db  0Ch
                db    0
                db    0
                db    3
                db 0FFh
                db 0FCh
                db    0
                db  0Ch
                db 0FFh
                db 0CCh
                db  0Ch
                db  0Ch
                db  3Fh ; ?
                db  0Ch
                db  30h ; 0
                db  0Ch
                db  0Ch
                db  0Ch
                db 0C0h
                db  0Ch
                db  3Fh ; ?
                db    3
                db    0
                db  0Ch
                db  3Fh ; ?
                db  0Ch
                db 0C0h
                db    0
                db 0F3h
                db 0C0h
                db    0
                db    0
                db 0C0h
                db 0C0h
                db    0
                db    0
                db 0C0h
                db 0C0h
                db    0
                db    3
                db 0C0h
                db 0F0h
                db    0
                db    0
                db    0
                db    0
                db    0
sg08e3          ends

; ===========================================================================

; Segment type: Pure code
seg002          segment byte public 'CODE' use16
                assume cs:seg002
                assume es:nothing, ss:seg003, ds:nothing, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================


                public start
start           proc far
                mov     ax, ds
                mov     es, ax
                assume es:nothing
                add     ax, 10h
                mov     ss, ax
                assume ss:sg01a2
                mov     sp, 0
                push    ax
                mov     ax, offset start_
                push    ax
                xor     ax, ax
                mov     bx, ax
                mov     cx, ax
                mov     dx, ax
                mov     bp, ax
                mov     si, ax
                mov     di, ax
                retf
start           endp

; ---------------------------------------------------------------------------
                db      2Eh
                inc     bx
                fld     dword ptr [bp+di+16BBh]
                push    bx
                shr     word ptr [bx+2753h], cl
                push    ax
                mov     dx, 5394h
                mov     dx, 5221h
                loope   loc_18B47

loc_18AF4:                              ; CODE XREF: seg002:loc_18B0D↓j
                mov     bx, 5366h
                shr     word ptr [bx+2753h], cl
                mov     al, ds:byte_151E1
                mov     bx, 5378h
                shr     word ptr [bx+2753h], cl
                mov     dx, 45DEh
                pop     ds
                assume ds:sg08e3
                push    dx
                out     5Ah, ax
                sahf

loc_18B0D:
                jb      short near ptr loc_18AF4+2
                push    bx

loc_18B10:                              ; CODE XREF: seg002:0076↓j
                sahf
                inc     bp
                loope   loc_18B67
                mov     bx, 5346h

loc_18B17:                              ; CODE XREF: seg002:007E↓j
                shr     word ptr [bx+2753h], cl
                and     cl, 52h

loc_18B1E:                              ; CODE XREF: seg002:008B↓j
                mov     bx, 5358h

loc_18B21:                              ; CODE XREF: seg002:loc_18B47↓j
                shr     word ptr [bx+2753h], cl
                call    far ptr 0BA53h:0DEBAh

loc_18B2A:                              ; CODE XREF: seg002:008D↓j
                ficom   dword ptr [bp+di-15h]
                push    bx
                push    bx
                fst     qword ptr [bp+di+53E5h]
                call    near ptr 53A1h
                jnz     short loc_18B10
                push    sp
                fcomp   dword ptr [bp+di+29E8h]
                push    bx
                jnz     short near ptr loc_18B17+1
                push    sp
                fst     qword ptr [bp+di-7528h]
                jcxz    short loc_18B97

loc_18B47:                              ; CODE XREF: seg002:0032↑j
                jnz     short near ptr loc_18B21+3
                adc     al, 50h ; 'P'
                jcxz    short near ptr loc_18B1E+2
                jnz     short loc_18B2A
                adc     al, 56h ; 'V'
                fistp   word ptr [bx+si-7723h]
                fst     qword ptr [bx+si+53E8h]
                push    bx
                jmp     far ptr 0E202h:5356h
; ---------------------------------------------------------------------------
                db  51h ; Q
                db 0E6h
                db  55h ; U
                db 0E3h
                db  57h ; W
                db 0E7h
                db  57h ; W
                db  9Eh
; ---------------------------------------------------------------------------

loc_18B67:                              ; CODE XREF: seg002:0052↑j
                inc     ax
                or      dl, bl
                scasw
                push    bx
                daa
                push    si
                mov     cl, 0BEh
                mov     ax, 0C310h
                jmp     short loc_18BC8
; ---------------------------------------------------------------------------
                db  53h ; S
                db 0DDh
                db  93h
                db 0E8h
                db  2Bh ; +
                db  53h ; S
                db  75h ; u
                db 0D8h
                db  54h ; T
                db 0D8h
                db  9Bh
                db 0E8h
                db  29h ; )
                db  53h ; S
                db  75h ; u
                db 0D8h
                db  54h ; T
                db 0DDh
                db  93h
                db 0D8h
                db  8Ah
                db 0E3h
                db  51h ; Q
                db  75h ; u
                db 0DBh
                db  14h
                db  50h ; P
                db 0E3h
                db  49h ; I
                db  75h ; u
                db 0DBh
                db  14h
                db  56h ; V
                db 0DFh
; ---------------------------------------------------------------------------

loc_18B97:                              ; CODE XREF: seg002:0085↑j
                cbw
                fst     qword ptr [bx+si+53E5h]
                call    near ptr 5432h
                jmp     far ptr 0E202h:5356h
; ---------------------------------------------------------------------------
                db 0E8h
                db 0E6h
                db  55h ; U
                db 0E3h
                db  57h ; W
                db 0E7h
                db  57h ; W
; ---------------------------------------------------------------------------

loc_18BAB:                              ; CODE XREF: seg002:0101↓j
                sahf

loc_18BAC:                              ; CODE XREF: seg002:0112↓j
                inc     ax
                or      dl, bl
                scasw
                push    bx
                daa
                push    cx
                mov     cl, 0BEh
                nop

loc_18BB6:                              ; CODE XREF: seg002:0119↓j
                fistp   word ptr [bx+si-7723h]

loc_18BBA:                              ; CODE XREF: seg002:011F↓j
                fiadd   word ptr [di+74h]
                push    dx
                out     5Ah, ax
                sahf
                jb      short loc_18BAB
                push    bx
                push    bx
; ---------------------------------------------------------------------------
                db 0DDh
                db  90h
                db 0E8h
; ---------------------------------------------------------------------------

loc_18BC8:                              ; CODE XREF: seg002:00B3↑j
                sub     dx, [bp+di+75h]
                fcom    dword ptr [si-28h]
                wait
                call    near ptr 543Bh
                jnz     short loc_18BAC
                push    sp
                fst     qword ptr [bp+di+51E3h]
                jnz     short loc_18BB6
                adc     al, 51h ; 'Q'
                jcxz    short loc_18C58
                jnz     short near ptr loc_18BBA+2
                adc     al, 56h ; 'V'
                out     1Fh, ax
                sahf
                jb      short loc_18C46
                pop     cx
                pop     si
                pop     cx
                add     bx, [bx]
                push    ss
                adc     al, [bx+si]
                push    ss
                jnb     short loc_18BF9
                add     ds:byte_18FE0+0A3h, dl
                add     [bp+si], bx

loc_18BF9:                              ; CODE XREF: seg002:0131↑j
                adc     al, 1Ah
                sbb     ax, 1F12h
                jnb     short near ptr loc_18C16+1
                sbb     al, [bx+si]
                sbb     [bp+di+1Ch], dh
                sbb     ax, 0A1Fh
                jge     short near ptr loc_18C66+2
                pop     cx
                ja      short near ptr loc_18C6A+1
                pop     cx
                pop     si
                pop     cx
                add     bx, [bx]
                push    ss
                adc     al, [bx+si]
                push    ss

loc_18C16:                              ; CODE XREF: seg002:013E↑j
                jnb     short loc_18C32
                sbb     ax, 1600h
                add     [bx], ax
                jnb     short loc_18C3B
                add     [bp+si], bx
                adc     al, 1Ah
                sbb     ax, 1F12h
                jnb     short near ptr loc_18C3E+1
                sbb     al, [bx+si]
                sbb     [bp+di+1Ah], dh
                sbb     ax, 1773h
                add     [bp+si], bx

loc_18C32:                              ; CODE XREF: seg002:loc_18C16↑j
                add     ax, 7316h
                adc     dh, [bp+di+1Ch]
                add     [bp+di+11h], si

loc_18C3B:                              ; CODE XREF: seg002:015D↑j
                pop     si
                pop     cx
                pop     es
                assume es:nothing

loc_18C3E:                              ; CODE XREF: seg002:0166↑j
                sbb     dx, ds:731Dh
                add     ax, [bx+di]

loc_18C44:                              ; CODE XREF: seg002:01A3↓j
                push    ss
; ---------------------------------------------------------------------------
                db    0
; ---------------------------------------------------------------------------

loc_18C46:                              ; CODE XREF: seg002:0126↑j
                add     [bp+di+12h], dh
                sbb     ax, 730Ah
                sbb     ds:730Ah, dl
                pop     es
                sbb     al, 73h ; 's'
                adc     [si], bl
                sbb     ax, 1A07h

loc_18C58:                              ; CODE XREF: seg002:011D↑j
                sbb     ax, 1606h
                jnb     short loc_18CDA
                jnb     short near ptr loc_18CDA+2
                jnb     short loc_18CDE
                pop     si
                pop     cx
                ja      short loc_18C44
                cbw

loc_18C66:                              ; CODE XREF: seg002:0148↑j
                fisttp  qword ptr [bx+si-6F23h]

loc_18C6A:                              ; CODE XREF: seg002:014B↑j
                fimul   word ptr [di+4Bh]
                push    ax
                jmp     far ptr 0E202h:5356h
; ---------------------------------------------------------------------------
                db  52h ; R
                db 0E6h
                db  55h ; U
                db 0E3h
                db  52h ; R
                db 0E7h
                db  51h ; Q
                db  9Eh
                db  40h ; @
                db  0Ah
                db 0D3h
                db 0AFh
                db  53h ; S
                db  27h ; '
                db  56h ; V
                db 0B1h
                db 0BEh
                db 0BAh
                db  7Ch ; |
                db 0ACh
                db  54h ; T
                db  4Ch ; L
                db 0D8h
                db 0A0h
                db 0D2h
                db  95h
                db  53h ; S
                db  52h ; R
                db  7Dh ; }
                db  50h ; P
                db  65h ; e
                db  51h ; Q
                db  53h ; S
                db  4Dh ; M
                db 0DFh
                db  88h
                db 0D0h
                db  90h
                db  43h ; C
                db 0DFh
                db  9Bh
                db 0DDh
                db  93h
                db 0DDh
                db  8Bh
                db 0ECh
                db  5Bh ; [
                db  53h ; S
                db 0FEh
                db 0F8h
                db 0FEh
                db  50h ; P
                db  90h
                db 0F8h
                db 0FEh
                db 0D8h
                db  9Bh
                db 0FEh
                db  50h ; P
                db  90h
                db    8
                db  0Dh
                db  5Dh ; ]
                db  9Dh
                db 0E8h
                db  1Ah
                db    0
                db  8Eh
                db 0DBh
                db  8Eh
                db 0C3h
                db  8Eh
                db 0D0h
                db  8Bh
                db 0E1h
                db  8Bh
                db 0C5h
                db  8Bh
                db 0CEh
                db 0BBh
                db    0
                db    0
                db  8Bh
                db 0D3h
                db  8Bh
                db 0EAh
                db  8Bh
                db 0F2h
                db  8Bh
                db 0FAh
                db 0E9h
                db 0F7h
                db 0FDh
                db  9Ch
                db  1Eh
                db    6
                db  50h ; P
                db  53h ; S
                db  51h ; Q
                db  52h ; R
                db  8Ch
                db 0CBh
                db  8Eh
; ---------------------------------------------------------------------------

loc_18CDA:                              ; CODE XREF: seg002:019B↑j
                                        ; seg002:019D↑j
                fisttp  dword ptr [bp-723Dh]

loc_18CDE:                              ; CODE XREF: seg002:019F↑j
                push    ds
                adc     al, [bx+si]
                lea     cx, unk_17603
                sub     cx, 2
                sub     cx, bx

loc_18CEA:                              ; CODE XREF: seg002:0231↓j
                mov     al, [bx]
                xor     al, 53h
                mov     [bx], al
                inc     bx
                loop    loc_18CEA
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                pop     es
                pop     ds
                assume ds:nothing
                popf
                retn
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
                db  9Ch
                db  51h ; Q
                db  52h ; R
                db  50h ; P
                db 0E8h
                db 0B1h
                db    0
                db  52h ; R
                db  51h ; Q
                db  3Ch ; <
                db    0
                db  74h ; t
                db  0Ch
                db  81h
                db 0C2h
                db 0B0h
                db    0
                db  73h ; s
                db    1
                db  41h ; A
                db  83h
                db 0C1h
                db  18h
                db  72h ; r
                db  2Ah ; *
                db  2Eh ; .
                db  2Bh ; +
                db  16h
                db  16h
                db    3
                db  73h ; s
                db    1
                db  49h ; I
                db  2Eh ; .
                db  2Bh ; +
                db  0Eh
                db  14h
                db    3
                db  72h ; r
                db  1Bh
                db  2Eh ; .
                db  89h
                db  0Eh
                db  9Bh
                db    2
                db  2Eh ; .
                db  89h
                db  16h
                db  9Dh
                db    2
                db  59h ; Y
                db  5Ah ; Z
                db  2Eh ; .
                db  89h
                db  0Eh
                db  14h
                db    3
                db  2Eh ; .
                db  89h
                db  16h
                db  16h
                db    3
                db  58h ; X
                db  5Ah ; Z
                db  59h ; Y
                db  9Dh
                db 0C3h
                db 0E9h
                db  67h ; g
                db 0FEh
                db    6
                db    0
                db  40h ; @
                db    0
                db  9Ch
                db  51h ; Q
                db  52h ; R
                db  2Eh ; .
                db  8Bh
                db  0Eh
                db  9Bh
                db    2
                db  83h
                db 0F9h
                db    0
                db  75h ; u
                db 0ECh
                db  2Eh ; .
                db  8Bh
                db  0Eh
                db  9Dh
                db    2
                db  2Eh ; .
                db  3Bh ; ;
                db  0Eh
                db 0BCh
                db    2
                db  73h ; s
                db 0E0h
                db  5Ah ; Z
                db  59h ; Y
                db  9Dh
                db 0C3h
                db  90h
                db    0
                db 0E8h
                db  4Ch ; L
                db    0
                db  2Eh ; .
                db  89h
                db  0Eh
                db  14h
                db    3
                db  2Eh ; .
                db  89h
                db  16h
                db  16h
                db    3
                db 0B8h
                db    0
                db    0
                db  8Eh
                db 0C0h
                db  26h ; &
                db 0A1h
                db  4Eh ; N
                db    0
                db  3Dh ; =
                db 0FFh
                db 0BFh
                db  7Eh ; ~
                db    4
                db  26h ; &
                db 0A1h
                db  12h
                db    0
                db  8Bh
                db 0C8h
                db  26h ; &
                db 0A1h
                db    6
                db    0
                db  3Ch ; <
                db 0F0h
                db  7Dh ; }
                db    7
                db  3Bh ; ;
                db 0C8h
                db  74h ; t
                db    3
                db 0E9h
                db  14h
                db 0FEh
                db  26h ; &
                db 0A1h
                db  0Eh
                db    0
                db  3Ch ; <
                db 0F0h
                db  7Dh ; }
                db    7
                db  3Bh ; ;
                db 0C1h
                db  74h ; t
                db    3
                db 0E9h
                db    5
                db 0FEh
                db  8Dh
                db  1Eh
                db  3Ah ; :
                db    2
                db 0B8h
                db 0C3h
                db    0
                db  89h
                db    7
                db  8Ch
                db 0CBh
                db  8Eh
                db 0DBh
                db  8Eh
                db 0C3h
                db 0C3h
                db 0B4h
                db    0
                db  9Ch
                db 0CDh
                db  1Ah
                db  9Dh
                db 0C3h
                db  20h
                db    0
                db    0
                db    2
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
byte_18FE0      db 1000h dup(?)
seg002          ends

; ===========================================================================

; Segment type: Uninitialized
seg003          segment byte stack 'STACK' use16
                assume cs:seg003
                assume es:nothing, ss:nothing, ds:nothing, fs:nothing, gs:nothing
byte_19FE0      db 64h dup(?)
seg003          ends


                end start
