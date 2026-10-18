; ---------------------------------------------------------------------------

FILE            struc ; (sizeof=0xC, align=0x2, copyof_1)
_ptr            dd ?                    ; offset
_cnt            dw ?
_base           dd ?                    ; offset
_flag           db ?
_file           db ?
FILE            ends

; ---------------------------------------------------------------------------

AttributeText   struc ; (sizeof=0x4, mappedto_3)
                                        ; XREF: dseg:ATTRIBUTE_TEXT/r
_x              db ?                    ; base 10
_y              db ?                    ; base 10
_text           dw ?                    ; XREF: writeClassesAndAttributes+8E/r ; offset
AttributeText   ends

; ---------------------------------------------------------------------------

WindowAttr      struc ; (sizeof=0x8, mappedto_9) ; XREF: Window/r
_x              dw ?
_y              dw ?
_w              dw ?                    ; XREF: writeCenteredString+1C/r
                                        ; writeCenteredString+46/r
_h              dw ?
WindowAttr      ends

; ---------------------------------------------------------------------------

Window          struc ; (sizeof=0x1C, mappedto_4)
                                        ; XREF: dseg:_windowParty/r
                                        ; dseg:_windowSkills/r ...
_x1             dw ?                    ; base 10
_y1             dw ?                    ; base 10
_x2             dw ?                    ; base 10
_y2             dw ?                    ; base 10
_textX          dw ?                    ; XREF: writeString_1+14/w
                                        ; writeChar+20/r ; base 10
_textY          dw ?                    ; XREF: writeChar+C/r ; base 10
_attr           WindowAttr ?            ; XREF: writeCenteredString+1C/r
                                        ; writeCenteredString+46/r
field_14        dw ?                    ; XREF: sub_1089E+10/w
                                        ; sub_1089E+3F/w ...
_fnFocus        dw ?                    ; XREF: setWindow+22/r
                                        ; setWindow+28/r ; offset
field_18        dw ?                    ; XREF: setWindow+19/w
field_1A        dw ?
Window          ends

; ---------------------------------------------------------------------------

Disk1           struc ; (sizeof=0xEC0, mappedto_5) ; XREF: dseg:_disk1/r
field_0         dw ?                    ; XREF: loadDisk1+4F/r
                                        ; sub_103E0+6/w
field_2         dw ?                    ; XREF: loadDisk1+48/r
                                        ; sub_103E0+C/w
field_4         dw ?
_gfxMode        db ?                    ; XREF: _main+16/r
field_7         db ?                    ; XREF: loadDisk1+1AB/w
field_8         db ?                    ; XREF: loadDisk1+19C/w
field_9         db ?                    ; XREF: loadDisk1+1A1/w
field_A         dw ?                    ; XREF: loadDisk1+1B3/w
field_C         dw ?                    ; XREF: loadDisk1+1B9/w
field_E         dw ?                    ; XREF: loadDisk1:loc_10298/w
field_10        dw ?                    ; XREF: loadDisk1+199/w
field_12        db ?                    ; XREF: loadDisk1+1C4/w
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_1C        db ?                    ; XREF: loadDisk1+1C9/w
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_26        db ?                    ; XREF: loadDisk1+1CE/w
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_30        db ?                    ; XREF: loadDisk1+1BF/w
_partyCount     db ?                    ; XREF: loadDisk1+1D6/w
                                        ; sub_103E0:loc_10415/r ...
_partyIndexes   db 5 dup(?)
field_37        db ?                    ; XREF: loadDisk1+1A6/w
field_38        dw ?                    ; XREF: loadDisk1+1B0/w
_party          PartyMember 5 dup(?)    ; XREF: createNewMember+1B/o
field_6B6       db 250 dup(?)
field_7B0       db ?
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_7B6       dw ?                    ; XREF: loadDisk1+1D3/w
field_7B8       db 1799 dup(?)
field_EBF       db ?
Disk1           ends

; ---------------------------------------------------------------------------

PartyMember     struc ; (sizeof=0x14C, mappedto_6)
                                        ; XREF: createNewMember+15/o
                                        ; Disk1/r
field_0         db 24 dup(?)
_attributes     db 7 dup(?)             ; XREF: createNewMember+2C/w
                                        ; createNewMember+4A/w
field_1F        db ?
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_23        db ?                    ; XREF: createNewMember+5F/w
field_24        db 16 dup(?)            ; XREF: createNewMember+73/w
field_34        db 16 dup(?)            ; XREF: createNewMember+8D/w
_con            dw ?
                db ? ; undefined
                db ? ; undefined
_profession     db ?                    ; XREF: createNewMember+B1/w
field_49        db ?
field_4A        db ?
_array2         db 3 dup(?)
field_4E        db ?
field_4F        db ?
_sex            db ?                    ; XREF: createNewMember+C3/w
field_51        db ?                    ; XREF: createNewMember+BD/w
field_52        db ?
field_53        db ?
field_54        db ?
field_55        db ?
field_56        db ?
field_57        db ?
field_58        db ?                    ; XREF: createNewMember+A7/w
field_59        db ?
field_5A        dw ?
field_5C        dw ?
field_5E        dw ?
field_60        dw ?
_array1         MemberArray1Entry 32 dup(?)
                db ? ; undefined
                db ? ; undefined
field_124       db 16 dup(?)            ; XREF: createNewMember+7C/w
field_134       db 16 dup(?)            ; XREF: createNewMember+96/w
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_14B       db ?
PartyMember     ends

; ---------------------------------------------------------------------------

MemberArray1Entry struc ; (sizeof=0x6, mappedto_7) ; XREF: PartyMember/r
field_0         db ?
field_1         db ?
field_2         db ?
field_3         db ?
field_4         db ?
field_5         db ?
MemberArray1Entry ends

; ---------------------------------------------------------------------------

Profession      struc ; (sizeof=0x80, mappedto_10)
_name           db 16 dup(?)
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
_attributes     db 7 dup(?)             ; XREF: createNewMember+44/r
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
field_33        db ?
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
_activeSkills   db 16 dup(?)            ; XREF: createNewMember+6D/r
_passiveSkills  db 16 dup(?)            ; XREF: createNewMember+87/r
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
field_7B        db ?                    ; XREF: createNewMember+BA/r
                db ? ; undefined
                db ? ; undefined
                db ? ; undefined
field_7F        db ?
Profession      ends


;
; +-------------------------------------------------------------------------+
; |      This file was generated by The Interactive Disassembler (IDA)      |
; |           Copyright (c) 2023 Hex-Rays, <support@hex-rays.com>           |
; |                      License info: 48-337F-7354-97                      |
; |                       Paul Gilbert, ScummVM Team                        |
; +-------------------------------------------------------------------------+
;
; Input SHA256 : 9324CF925EC2E0124E78C4D073823242095323B10D8D7CBE46E55E62867AE6CA
; Input MD5    : 6218D591BCF81A890C4C41D72E00E15B
; Input CRC32  : 16A4F00A

; File Name   : C:\games\fod\FOD.EXE
; Format      : MS-DOS executable (EXE)
; Base Address: 1000h Range: 10000h-194A0h Loaded length: 6F10h
; Entry Point : 1000:1AEC
; OS type         :  MS DOS
; Application type:  Executable 16bit

                .686p
                .mmx
                .model large

; ===========================================================================

; Segment type: Pure code
sg0138          segment byte public 'CODE' use16
                assume cs:sg0138
                ;org 6A0h
                assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing
                db 10h dup(0)

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

; void __cdecl writeCenteredString(const char near *str, int y)
writeCenteredString proc near           ; CODE XREF: displayWelcomePortrait_2+2B↓p
                                        ; displaySkills+3F↓p ...

buffer          = byte ptr -2Ah
str             = word ptr  4
t               = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 2Ah
                push    si
                push    [bp+str]        ; char *
                mov     ax, offset aS   ; "%s"
                push    ax
                lea     ax, [bp+buffer]
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                mov     bx, _currentWindow
                mov     ax, [bx+Window._attr._w]
                cwd
                xor     ax, dx
                sub     ax, dx
                mov     cx, 2
                sar     ax, cl
                xor     ax, dx
                sub     ax, dx
                mov     si, ax
                mov     [bp+si+buffer], 0
                push    [bp+t]          ; t
                lea     ax, [bp+buffer]
                push    ax
                call    sub_132C2
                add     sp, 2
                mov     bx, _currentWindow
                mov     cx, ax
                mov     ax, [bx+Window._attr._w]
                mov     bx, cx
                cwd
                xor     ax, dx
                sub     ax, dx
                mov     cx, 2
                sar     ax, cl
                xor     ax, dx
                sub     ax, dx
                sub     ax, bx
                cwd
                sub     ax, dx
                sar     ax, 1
                push    ax              ; x
                lea     ax, [bp+buffer]
                push    ss
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                pop     si
                mov     sp, bp
                pop     bp
                retn
writeCenteredString endp


; =============== S U B R O U T I N E =======================================

; Attributes: noreturn

error           proc near               ; CODE XREF: fontOpen+1A↓p
                                        ; fontOpen+4F↓p ...
                call    setTextMode
                sub     ax, ax
                push    ax              ; int
                call    _exit
error           endp

; ---------------------------------------------------------------------------
                add     sp, 2
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

fontOpen        proc near               ; CODE XREF: gfxInit+61↓p

var_8           = word ptr -8
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                sub     ax, ax
                push    ax
                mov     ax, offset aFont ; "FONT"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_8], ax
                or      ax, ax
                jnz     short loc_100AD
                call    error
; ---------------------------------------------------------------------------

loc_100AD:                              ; CODE XREF: fontOpen+18↑j
                lea     ax, [bp+var_2]
                push    ss
                push    ax
                mov     ax, 2
                cwd
                push    dx
                push    ax
                push    [bp+var_8]
                call    sub_11902
                add     sp, 0Ah
                mov     ax, [bp+var_2]
                mov     cl, 5
                shl     ax, cl
                push    ax
                call    sub_12FE4
                add     sp, 2
                mov     _fontData, ax
                or      ax, ax
                jnz     short loc_100E2
                push    [bp+var_8]
                call    sub_118F1
                add     sp, 2
                call    error
; ---------------------------------------------------------------------------

loc_100E2:                              ; CODE XREF: fontOpen+44↑j
                push    ds
                push    _fontData
                mov     ax, [bp+var_2]
                cwd
                mov     cl, 5
                call    sub_13BFA
                push    dx
                push    ax
                push    [bp+var_8]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_8]
                call    sub_118F1
                mov     sp, bp
                pop     bp
                retn
fontOpen        endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

loadDisk1       proc near               ; CODE XREF: _main+10↓p

var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                sub     ax, ax
                push    ax
                mov     ax, offset filename ; "DISK1"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                or      ax, ax
                jnz     short loc_10123
                call    error
; ---------------------------------------------------------------------------

loc_10123:                              ; CODE XREF: loadDisk1+19↑j
                push    [bp+var_6]
                call    sub_119BE
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                mov     ax, 231Eh
                push    ds
                push    ax
                push    dx
                push    [bp+var_4]
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                cmp     _disk1.field_2, 0
                jnz     short loc_1015B
                cmp     _disk1.field_0, 0
                jz      short loc_10161

loc_1015B:                              ; CODE XREF: loadDisk1+4D↑j
                mov     ax, 1
                jmp     loc_102E0
; ---------------------------------------------------------------------------

loc_10161:                              ; CODE XREF: loadDisk1+54↑j
                mov     [bp+var_8], 0

loc_10166:                              ; CODE XREF: loadDisk1+190↓j
                mov     ax, 14Ch
                imul    [bp+var_8]
                add     ax, 2358h
                mov     [bp+var_C], ax
                mov     bx, [bp+var_8]
                shl     bx, 1
                mov     [bx+3D88h], ax
                mov     bx, [bp+var_8]
                mov     al, byte ptr [bp+var_8]
                mov     [bx+2350h], al
                mov     bx, [bp+var_C]
                mov     byte ptr [bx], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+52h], 6
                mov     [bp+var_A], 0

loc_10197:                              ; CODE XREF: loadDisk1+D5↓j
                mov     ax, 6
                imul    [bp+var_A]
                mov     si, ax
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+si+62h], 0FFh
                mov     ax, 6
                imul    [bp+var_A]
                mov     si, ax
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+si+64h], 0
                mov     ax, 6
                imul    [bp+var_A]
                mov     si, ax
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+si+63h], 0
                mov     ax, 6
                imul    [bp+var_A]
                mov     si, ax
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+si+65h], 0
                inc     [bp+var_A]
                cmp     [bp+var_A], 20h ; ' '
                jl      short loc_10197
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+62h], 2
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+64h], 9
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+68h], 0Ch
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+6Eh], 0Ch
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+74h], 20h ; ' '
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+7Ah], 29h ; ')'
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+80h], 37h ; '7'
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+4Eh], 1
                mov     bx, [bp+var_C]
                mov     word ptr [bx+5Eh], 5DCh
                mov     word ptr [bx+60h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+4Ah], 0FFh
                mov     [bp+var_A], 0

loc_1022E:                              ; CODE XREF: loadDisk1+13A↓j
                mov     si, [bp+var_A]
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+si+4Bh], 0FFh
                inc     [bp+var_A]
                cmp     [bp+var_A], 3
                jl      short loc_1022E
                mov     bx, [bp+var_C]
                sub     ax, ax
                mov     [bx+5Ch], ax
                mov     [bx+5Ah], ax
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+4Fh], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+50h], 1
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+53h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+54h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+55h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+56h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+57h], 0
                mov     bx, [bp+var_C]
                mov     byte ptr [bx+49h], 0
                mov     bx, [bp+var_8]
                mov     byte ptr [bx+2ACEh], 0
                inc     [bp+var_8]
                cmp     [bp+var_8], 5
                jge     short loc_10298
                jmp     loc_10166
; ---------------------------------------------------------------------------

loc_10298:                              ; CODE XREF: loadDisk1+18E↑j
                mov     _disk1.field_E, 64h ; 'd'
                mov     _disk1.field_10, ax
                mov     _disk1.field_8, 9
                mov     _disk1.field_9, 0
                mov     _disk1.field_37, 1
                mov     _disk1.field_7, 0
                mov     _disk1.field_38, ax
                mov     _disk1.field_A, 0Ah
                mov     _disk1.field_C, 6
                mov     _disk1.field_30, 0
                mov     _disk1.field_12, 1Ch
                mov     _disk1.field_1C, 0Ah
                mov     _disk1.field_26, 6
                mov     _disk1.field_7B6, ax
                mov     _disk1._partyCount, 0

loc_102E0:                              ; CODE XREF: loadDisk1+59↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
loadDisk1       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

loadClasses     proc near               ; CODE XREF: _main+2F↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                sub     ax, ax
                push    ax
                mov     ax, offset aBorders ; "BORDERS"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                sub     ax, ax
                push    ax
                mov     ax, 1388h
                cwd
                push    dx
                push    ax
                push    [bp+var_6]
                call    sub_119A2
                add     sp, 8
                push    ds
                push    dst
                mov     ax, 3E8h
                cwd
                push    dx
                push    ax
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                cmp     [bp+arg_0], 0
                jz      short loc_10333
                jmp     loc_103DC
; ---------------------------------------------------------------------------

loc_10333:                              ; CODE XREF: loadClasses+49↑j
                sub     ax, ax
                push    ax
                mov     ax, offset aArchtype ; "ARCHTYPE"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                or      ax, ax
                jnz     short loc_1034A
                call    error
; ---------------------------------------------------------------------------

loc_1034A:                              ; CODE XREF: loadClasses+60↑j
                push    [bp+var_6]
                call    sub_119BE
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                push    ax
                call    sub_12FE4
                add     sp, 2
                mov     _professionsData, ax
                or      ax, ax
                jnz     short loc_10375
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                call    error
; ---------------------------------------------------------------------------
                jmp     short loc_103DC
; ---------------------------------------------------------------------------

loc_10375:                              ; CODE XREF: loadClasses+80↑j
                push    ds
                push    _professionsData
                push    [bp+var_2]
                push    [bp+var_4]
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                mov     bx, _professionsData
                mov     ax, [bx+4]
                add     ax, bx
                mov     _professions, ax
                sub     ax, ax
                push    ax
                mov     ax, offset aHdspct ; "HDSPCT"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                or      ax, ax
                jnz     short loc_103B5
                call    error
; ---------------------------------------------------------------------------

loc_103B5:                              ; CODE XREF: loadClasses+CB↑j
                mov     [bp+var_4], 49Ch
                mov     [bp+var_2], 0
                mov     ax, 37E8h
                push    ds
                push    ax
                mov     ax, 49Ch
                cwd
                push    dx
                push    ax
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2

loc_103DC:                              ; CODE XREF: loadClasses+4B↑j
                                        ; loadClasses+8E↑j
                mov     sp, bp
                pop     bp
                retn
loadClasses     endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_103E0       proc near               ; CODE XREF: _main+207↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     _disk1.field_0, 1
                mov     _disk1.field_2, 0
                mov     [bp+var_4], 0
                jmp     short loc_10415
; ---------------------------------------------------------------------------

loc_103F9:                              ; CODE XREF: sub_103E0+3D↓j
                mov     ax, 14Ch
                imul    [bp+var_4]
                mov     bx, ax
                mov     al, byte ptr [bp+var_4]
                sub     al, 6
                mov     [bx+23A1h], al
                mov     bx, [bp+var_4]
                mov     byte ptr [bx+2ACEh], 1
                inc     [bp+var_4]

loc_10415:                              ; CODE XREF: sub_103E0+17↑j
                mov     al, _disk1._partyCount
                sub     ah, ah
                cmp     [bp+var_4], ax
                jb      short loc_103F9
                mov     ax, 2
                push    ax
                mov     ax, offset aDisk1_0 ; "DISK1"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_2], ax
                mov     ax, 231Eh
                push    ds
                push    ax
                mov     ax, 0EC0h
                cwd
                push    dx
                push    ax
                push    [bp+var_2]
                call    sub_11952
                add     sp, 0Ah
                push    [bp+var_2]
                call    sub_118F1
                mov     sp, bp
                pop     bp
                retn
sub_103E0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

displayWelcomePortrait proc near        ; CODE XREF: displayWelcomePortrait_2+14↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     _welcomePotraitId, 31Eh
                mov     ax, word_18214
                cmp     [bp+arg_0], ax
                jz      short loc_104E0
                mov     ax, [bp+arg_0]
                mov     word_18214, ax
                mov     ax, 8000h
                push    ax
                mov     ax, offset aGani ; "GANI"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                push    ax
                call    sub_119BE
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                push    word ptr _pictWelcome+2
                push    word ptr _pictWelcome
                push    dx
                push    ax
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                push    word ptr _pictArray2+2
                push    word ptr _pictArray2
                push    word ptr _pictWelcome+2
                push    word ptr _pictWelcome
                call    sub_11AC7
                add     sp, 8
                mov     ax, word ptr _pictArray2
                mov     dx, word ptr _pictArray2+2
                mov     word_16F44, ax
                mov     word_16F46, dx
                les     bx, _pictArray2
                mov     ah, es:[bx+2]
                sub     al, al
                mov     cl, es:[bx+1]
                sub     ch, ch
                add     ax, cx
                mov     word_16F42, ax
                mov     byte_16F4A, 1

loc_104E0:                              ; CODE XREF: displayWelcomePortrait+12↑j
                mov     _welcomeImageLoaded, 1
                mov     sp, bp
                pop     bp
                retn
displayWelcomePortrait endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_104EA       proc near               ; CODE XREF: displayWelcomePortrait_2+1E↓p

var_E           = byte ptr -0Eh
var_C           = dword ptr -0Ch
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                mov     ax, word_16F44
                mov     dx, word_16F46
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx
                add     ax, word_16F42
                mov     word ptr [bp+var_C], ax
                mov     word ptr [bp+var_C+2], dx
                les     bx, [bp+var_C]
                inc     word ptr [bp+var_C]
                mov     al, es:[bx]
                mov     [bp+var_E], al
                mov     ax, _welcomePotraitId
                add     ax, 0Ch
                mov     [bp+var_2], ax
                mov     [bp+var_8], 0
                jmp     short loc_1054B
; ---------------------------------------------------------------------------

loc_10523:                              ; CODE XREF: sub_104EA+69↓j
                mov     bx, [bp+var_2]
                push    word ptr [bx+2]
                mov     ax, [bx]
                shl     ax, 1
                push    ax
                push    [bp+var_4]
                push    [bp+var_6]
                les     bx, [bp+var_C]
                inc     word ptr [bp+var_C]
                mov     al, es:[bx]
                sub     ah, ah
                push    ax
                call    sub_14BDD
                add     sp, 0Ah
                inc     [bp+var_8]

loc_1054B:                              ; CODE XREF: sub_104EA+37↑j
                mov     al, [bp+var_E]
                sub     ah, ah
                cmp     [bp+var_8], ax
                jb      short loc_10523
                cmp     [bp+arg_0], ah
                jnz     short loc_10567
                mov     bx, _welcomePotraitId
                push    word ptr [bx+1Ah]
                call    sub_114D5
                add     sp, 2

loc_10567:                              ; CODE XREF: sub_104EA+6E↑j
                mov     sp, bp
                pop     bp
                retn
sub_104EA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

getKeypress     proc near               ; CODE XREF: sub_10A65:loc_10B15↓p
                                        ; editPartyMember+92↓p ...

result          = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+result], 0
                jmp     short loc_1057A
; ---------------------------------------------------------------------------

loc_10577:                              ; CODE XREF: getKeypress+14↓j
                call    sub_11729

loc_1057A:                              ; CODE XREF: getKeypress+A↑j
                                        ; getKeypress+A2↓j
                call    _kbhit
                or      ax, ax
                jz      short loc_10577
                call    _getch
                mov     [bp+var_4], al
                cbw
                or      ax, ax
                jz      short loc_1059B
                cmp     ax, 0Dh
                jz      short loc_10603
                cmp     ax, 1Bh
                jz      short loc_105FD

loc_10596:                              ; CODE XREF: getKeypress+4A↓j
                mov     [bp+result], al
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------

loc_1059B:                              ; CODE XREF: getKeypress+1F↑j
                call    _getch
                mov     [bp+var_2], al
                cbw
                sub     ax, ';'         ; switch with an invalid jump table
                cmp     ax, 21
                ja      short def_105AD ; jumptable 000105AD default case
                add     ax, ax
                xchg    ax, bx
                jmp     word ptr cs:[bx+5CFh] ; switch jump
; ---------------------------------------------------------------------------
                mov     al, [bp+var_2]
                jmp     short loc_10596
; ---------------------------------------------------------------------------
                mov     [bp+result], 0FDh
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------
                mov     [bp+result], 0FCh
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------
                mov     [bp+result], 0FBh
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------
                mov     [bp+result], 0FAh
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------
                mov     dl, 5
                mov     dl, 5
                mov     dl, 5
                mov     dl, 5
                mov     dl, 5
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                pop     es
                push    es
                mov     bh, 5
                pop     es
                push    es
                pop     es
                push    es
                retn
; ---------------------------------------------------------------------------
                db 5, 7, 6, 0C9h, 5, 7, 6, 7, 6, 0BDh, 5, 0EBh, 0Ah
; ---------------------------------------------------------------------------

loc_105FD:                              ; CODE XREF: getKeypress+29↑j
                mov     [bp+result], 0FFh
                jmp     short def_105AD ; jumptable 000105AD default case
; ---------------------------------------------------------------------------

loc_10603:                              ; CODE XREF: getKeypress+24↑j
                mov     [bp+result], 0FEh

def_105AD:                              ; CODE XREF: getKeypress+2E↑j
                                        ; getKeypress+3D↑j ...
                cmp     [bp+result], 0  ; jumptable 000105AD default case
                jnz     short loc_10610
                jmp     loc_1057A
; ---------------------------------------------------------------------------

loc_10610:                              ; CODE XREF: getKeypress+A0↑j
                mov     al, [bp+result]
                cbw
                mov     sp, bp
                pop     bp
                retn
getKeypress     endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

listParty       proc near               ; CODE XREF: editPartyMember+31↓p
                                        ; editPartyMember+D8↓p ...

var_A           = dword ptr -0Ah
idx             = word ptr -6
win             = word ptr -4
arg_0           = word ptr  4
arg_2           = byte ptr  6
arg_4           = byte ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                mov     ax, _currentWindow
                mov     [bp+win], ax
                sub     ax, ax
                push    ax
                mov     ax, offset _windowParty
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                mov     [bp+idx], 0
                jmp     short loc_106B8
; ---------------------------------------------------------------------------

loc_1063B:                              ; CODE XREF: listParty+AA↓j
                mov     bx, [bp+idx]
                shl     bx, 1
                mov     ax, _party[bx]
                mov     word ptr [bp+var_A], ax
                mov     ax, [bp+idx]
                inc     ax
                push    ax              ; char *
                mov     ax, offset aF1d ; "F%1d>"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                cmp     [bp+arg_2], 0
                jz      short loc_1066D
                mov     ax, [bp+idx]
                cmp     [bp+arg_0], ax
                jnz     short loc_1066D
                mov     _inverseText, 0FFh

loc_1066D:                              ; CODE XREF: listParty+46↑j
                                        ; listParty+4E↑j
                push    [bp+idx]        ; t
                sub     ax, ax
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     _inverseText, 0
                mov     bx, word ptr [bp+var_A]
                mov     al, [bx+48h]
                sub     ah, ah
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                push    ax
                push    bx              ; char *
                mov     ax, offset a1616s1616s ; "%-16.16s - %-16.16s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                push    [bp+idx]        ; t
                mov     ax, 3
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                inc     [bp+idx]

loc_106B8:                              ; CODE XREF: listParty+21↑j
                mov     al, _disk1._partyCount
                sub     ah, ah
                cmp     [bp+idx], ax
                jnb     short loc_106C5
                jmp     loc_1063B
; ---------------------------------------------------------------------------

loc_106C5:                              ; CODE XREF: listParty+A8↑j
                cmp     [bp+arg_4], ah
                jz      short loc_106CF
                call    winField1A
                jmp     short loc_106DC
; ---------------------------------------------------------------------------

loc_106CF:                              ; CODE XREF: listParty+B0↑j
                mov     ax, _currentWindow
                add     ax, 0Ch
                push    ax
                call    sub_114B3
                add     sp, 2

loc_106DC:                              ; CODE XREF: listParty+B5↑j
                sub     ax, ax
                push    ax
                push    [bp+win]        ; win
                call    setWindow
                mov     sp, bp
                pop     bp
                retn
listParty       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_106E9       proc near               ; CODE XREF: editPartyMember+EE↓p
                                        ; _main+177↓p

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     [bp+arg_0], 3Bh ; ';'
                jl      short loc_10715
                mov     al, _disk1._partyCount
                sub     ah, ah
                add     ax, 3Bh ; ';'
                cmp     [bp+arg_0], ax
                jnb     short loc_10715
                mov     ax, [bp+arg_0]
                sub     ax, 3Bh ; ';'
                mov     [bp+var_2], ax
                mov     bx, [bp+arg_2]
                mov     [bx], ax
                mov     ax, 1
                jmp     short loc_10717
; ---------------------------------------------------------------------------

loc_10715:                              ; CODE XREF: sub_106E9+A↑j
                                        ; sub_106E9+17↑j
                sub     ax, ax

loc_10717:                              ; CODE XREF: sub_106E9+2A↑j
                mov     sp, bp
                pop     bp
                retn
sub_106E9       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

displayWelcomePortrait_2 proc near      ; CODE XREF: displaySkills+24↓p
                                        ; _main+D9↓p

arg_0           = word ptr  4
str             = word ptr  6

                push    bp              ; add
                mov     bp, sp
                mov     ax, 1
                push    ax
                mov     ax, 31Eh
                push    ax              ; win
                call    setWindow
                add     sp, 4
                push    [bp+arg_0]
                call    displayWelcomePortrait
                add     sp, 2
                mov     ax, 1
                push    ax
                call    sub_104EA
                add     sp, 2
                mov     ax, 0Bh
                push    ax              ; y
                push    [bp+str]        ; str
                call    writeCenteredString
                add     sp, 4
                call    winField1A
                pop     bp
                retn
displayWelcomePortrait_2 endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_10751       proc near               ; CODE XREF: writeClassesAndAttributes+15B↓p

arg_0           = word ptr  4

                push    bp              ; add
                mov     bp, sp
                sub     ax, ax
                push    ax
                mov     ax, offset _windowSkills
                push    ax              ; win
                call    setWindow
                add     sp, 4
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     bx, _party[bx]
                push    word ptr [bx+44h] ; char *
                mov     ax, offset aConD ; "Con:%d"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                mov     ax, 0Bh
                push    ax              ; t
                mov     ax, 6
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                call    winField1A
                sub     ax, ax
                push    ax
                mov     ax, 2CAh
                push    ax              ; win
                call    setWindow
                add     sp, 4
                pop     bp
                retn
sub_10751       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

displaySkills   proc near               ; CODE XREF: sub_10C58+78↓p
                                        ; sub_10CE5+7B↓p ...

var_C           = byte ptr -0Ch
var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si              ; add
                mov     [bp+var_8], 0
                mov     [bp+var_4], 0
                mov     ax, _profession
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                mov     [bp+var_2], ax
                push    ax
                mov     ax, 33h ; '3'
                push    ax
                call    displayWelcomePortrait_2
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     ax, offset _windowSkills
                push    ax              ; win
                call    setWindow
                add     sp, 4
                sub     ax, ax
                push    ax              ; y
                mov     ax, offset str  ; "Skills"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     ax, 1
                push    ax              ; y
                mov     ax, offset aActivePassive ; "Active      Passive"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     [bp+var_8], 0
                mov     [bp+var_4], 0
                mov     [bp+var_6], 0

loc_10802:                              ; CODE XREF: displaySkills+F6↓j
                mov     si, [bp+var_6]
                mov     bx, [bp+var_2]
                mov     al, [bx+si+38h]
                mov     [bp+var_C], al
                or      al, al
                jz      short loc_10847
                mov     ax, 16h
                imul    si
                add     ax, 390Eh
                push    ax
                mov     al, [bp+var_C]
                sub     ah, ah
                push    ax              ; char *
                mov     ax, offset a1d1010s ; "%1d %-10.10s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                mov     ax, [bp+var_8]
                add     ax, 3
                push    ax              ; t
                sub     ax, ax
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                inc     [bp+var_8]

loc_10847:                              ; CODE XREF: displaySkills+70↑j
                mov     si, [bp+var_6]
                mov     bx, [bp+var_2]
                mov     al, [bx+si+48h]
                mov     [bp+var_A], al
                or      al, al
                jz      short loc_1088D
                mov     ax, 16h
                imul    si
                add     ax, offset unk_1869E
                push    ax
                mov     al, [bp+var_A]
                sub     ah, ah
                push    ax              ; char *
                mov     ax, offset a1d1010s_0 ; "%1d %-10.10s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                mov     ax, [bp+var_4]
                add     ax, 3
                push    ax              ; t
                mov     ax, 13
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                inc     [bp+var_4]

loc_1088D:                              ; CODE XREF: displaySkills+B5↑j
                inc     [bp+var_6]
                cmp     [bp+var_6], 10h
                jge     short loc_10899
                jmp     loc_10802
; ---------------------------------------------------------------------------

loc_10899:                              ; CODE XREF: displaySkills+F4↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
displaySkills   endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1089E       proc near               ; CODE XREF: writeClassesAndAttributes+164↓p

arg_0           = word ptr  4

                push    bp              ; add
                mov     bp, sp
                sub     ax, ax
                push    ax
                mov     ax, 302h
                push    ax              ; win
                call    setWindow
                add     sp, 4
                mov     _windowMain.field_14, 8
                mov     bx, [bp+arg_0]
                mov     al, [bx+23h]
                sub     ah, ah
                push    ax              ; char *
                mov     ax, offset a22d ; "%2.2d->"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                sub     ax, ax
                push    ax              ; t
                mov     ax, 24h ; '$'
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     _windowMain.field_14, 0
                call    winField1A
                sub     ax, ax
                push    ax
                mov     ax, 2CAh
                push    ax              ; win
                call    setWindow
                add     sp, 4
                pop     bp
                retn
sub_1089E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

; void __cdecl writeClassesAndAttributes(int partyNum, int hiliteAtrib)
writeClassesAndAttributes proc near     ; CODE XREF: sub_10C58+81↓p
                                        ; sub_10CE5+84↓p ...

var_4           = word ptr -4
var_2           = word ptr -2
partyNum        = word ptr  4
hiliteAtrib     = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si              ; add
                mov     bx, [bp+partyNum]
                shl     bx, 1
                mov     ax, _party[bx]
                mov     [bp+var_4], ax
                sub     ax, ax
                push    ax
                mov     ax, offset _windowMain
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                mov     [bp+var_2], 0

loc_1091E:                              ; CODE XREF: writeClassesAndAttributes+62↓j
                mov     ax, [bp+var_2]
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                push    ax
                mov     ax, [bp+var_2]
                inc     ax
                push    ax              ; char *
                mov     ax, offset a1d1313s ; "%1d)%-13.13s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                mov     ax, [bp+var_2]
                inc     ax
                push    ax              ; t
                sub     ax, ax
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                inc     [bp+var_2]
                cmp     [bp+var_2], 5
                jl      short loc_1091E
                mov     _windowMain.field_14, 8
                sub     ax, ax
                push    ax              ; t
                mov     ax, 13h
                push    ax              ; x
                mov     ax, 133h
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     _windowMain.field_14, 0
                mov     [bp+var_2], 0

loc_1097C:                              ; CODE XREF: writeClassesAndAttributes+114↓j
                mov     bx, [bp+var_2]
                shl     bx, 1
                shl     bx, 1
                push    ATTRIBUTE_TEXT._text[bx] ; char *
                mov     ax, offset a3s  ; "%3s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                mov     si, [bp+var_2]
                mov     cl, 2
                shl     si, cl
                mov     al, [si+79h]
                cbw
                push    ax              ; t
                mov     al, [si+78h]
                cbw
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     di, [bp+var_2]
                mov     bx, [bp+var_4]
                mov     al, [bx+di+18h]
                sub     ah, ah
                push    ax              ; char *
                mov     ax, offset a2u  ; "%2u"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                mov     ax, [bp+var_2]
                cmp     [bp+hiliteAtrib], ax
                jnz     short loc_109DA
                mov     _inverseText, 0FFh

loc_109DA:                              ; CODE XREF: writeClassesAndAttributes+DE↑j
                mov     si, [bp+var_2]
                mov     cl, 2
                shl     si, cl
                mov     al, [si+79h]
                cbw
                push    ax              ; t
                mov     al, [si+78h]
                cbw
                add     ax, 3
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     _inverseText, 0
                inc     [bp+var_2]
                cmp     [bp+var_2], 7
                jge     short loc_10A0C
                jmp     loc_1097C
; ---------------------------------------------------------------------------

loc_10A0C:                              ; CODE XREF: writeClassesAndAttributes+112↑j
                mov     ax, 5
                push    ax              ; t
                mov     ax, 1Dh
                push    ax              ; x
                mov     ax, 14Dh
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                cmp     [bp+hiliteAtrib], 7
                jnz     short loc_10A2A
                mov     _inverseText, 0FFh

loc_10A2A:                              ; CODE XREF: writeClassesAndAttributes+12E↑j
                mov     ax, 5
                push    ax              ; t
                mov     ax, 22h ; '"'
                push    ax              ; x
                mov     bx, [bp+var_4]
                mov     bl, [bx+50h]
                and     bx, 3
                shl     bx, 1
                push    ds
                push    word ptr [bx+94h] ; str
                call    writeStringAt
                add     sp, 8
                mov     _inverseText, 0
                push    [bp+partyNum]
                call    sub_10751
                add     sp, 2
                push    [bp+var_4]
                call    sub_1089E
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
writeClassesAndAttributes endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_10A65       proc near               ; CODE XREF: addPartyMember:loc_1102C↓p

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     [bp+var_4], 0
                sub     ax, ax
                push    ax
                mov     ax, offset _windowMain
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                mov     ax, 1
                push    ax              ; y
                mov     ax, offset aChooseAProfess ; "Choose a Profession:"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     [bp+var_6], 0

loc_10A92:                              ; CODE XREF: sub_10A65+A9↓j
                mov     ax, [bp+var_6]
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                push    ax
                mov     ax, [bp+var_6]
                inc     ax
                push    ax              ; char *
                mov     ax, offset a1d1313s_0 ; "%1d)%-13.13s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                mov     ax, [bp+var_6]
                add     ax, 3
                push    ax              ; t
                mov     ax, 4
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                cmp     [bp+var_6], 2
                jge     short loc_10B07
                mov     ax, [bp+var_6]
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                add     ax, 180h
                push    ax
                mov     ax, [bp+var_6]
                add     ax, 4
                push    ax              ; char *
                mov     ax, offset a1d1313s_1 ; "%1d)%-13.13s"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 8
                mov     ax, [bp+var_6]
                add     ax, 3
                push    ax              ; t
                mov     ax, 17h
                push    ax              ; x
                mov     ax, offset sprint_buffer
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8

loc_10B07:                              ; CODE XREF: sub_10A65+66↑j
                inc     [bp+var_6]
                cmp     [bp+var_6], 3
                jl      short loc_10A92
                call    winField1A
                jmp     short loc_10B37
; ---------------------------------------------------------------------------

loc_10B15:                              ; CODE XREF: sub_10A65+D6↓j
                call    getKeypress
                mov     [bp+var_2], al
                cmp     al, 0FFh
                jnz     short loc_10B22
                cbw
                jmp     short loc_10B3D
; ---------------------------------------------------------------------------

loc_10B22:                              ; CODE XREF: sub_10A65+B8↑j
                cmp     [bp+var_2], 30h ; '0'
                jle     short loc_10B37
                cmp     [bp+var_2], 36h ; '6'
                jge     short loc_10B37
                mov     al, [bp+var_2]
                cbw
                sub     ax, 31h ; '1'
                jmp     short loc_10B3D
; ---------------------------------------------------------------------------

loc_10B37:                              ; CODE XREF: sub_10A65+AE↑j
                                        ; sub_10A65+C1↑j ...
                cmp     [bp+var_4], 0
                jz      short loc_10B15

loc_10B3D:                              ; CODE XREF: sub_10A65+BB↑j
                                        ; sub_10A65+D0↑j
                mov     sp, bp
                pop     bp
                retn
sub_10A65       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_10B41       proc near               ; CODE XREF: editPartyMember+1CC↓p
                                        ; addPartyMember+3F↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     ax, _profession
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                mov     [bp+var_2], ax
                mov     bx, ax
                push    word ptr [bx+72h]
                push    word ptr [bx+70h]
                call    sub_116FF
                add     sp, 4
                mov     bx, [bp+arg_0]
                mov     [bx+46h], ax
                mov     bx, [bp+arg_0]
                mov     si, bx
                mov     ax, [si+46h]
                mov     [bx+44h], ax
                mov     bx, [bp+arg_0]
                mov     si, bx
                mov     ax, [si+44h]
                mov     [bx+122h], ax
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_10B41       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

createNewMember proc near               ; CODE XREF: editPartyMember+1BA↓p
                                        ; addPartyMember+2D↓p

memberP         = word ptr -6
idx             = word ptr -4
prof            = word ptr -2
partyNum        = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     ax, _profession
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                mov     [bp+prof], ax
                mov     ax, size PartyMember
                imul    [bp+partyNum]
                add     ax, offset _disk1._party
                mov     [bp+memberP], ax
                mov     [bp+idx], 0

loc_10BAC:                              ; CODE XREF: createNewMember+37↓j
                mov     si, [bp+idx]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember._attributes], 0
                inc     [bp+idx]
                cmp     [bp+idx], 0Ch
                jl      short loc_10BAC
                mov     [bp+idx], 0

loc_10BC4:                              ; CODE XREF: createNewMember+54↓j
                mov     si, [bp+idx]
                mov     bx, [bp+prof]
                mov     al, [bx+si+Profession._attributes]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember._attributes], al
                inc     [bp+idx]
                cmp     [bp+idx], 7
                jl      short loc_10BC4
                mov     bx, [bp+memberP]
                mov     si, [bp+prof]
                mov     al, [si+33h]
                mov     [bx+PartyMember.field_23], al
                mov     [bp+idx], 0

loc_10BED:                              ; CODE XREF: createNewMember+A2↓j
                mov     si, [bp+idx]
                mov     bx, [bp+prof]
                mov     al, [bx+si+Profession._activeSkills]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember.field_24], al
                mov     si, [bp+idx]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember.field_124], 0
                mov     si, [bp+idx]
                mov     bx, [bp+prof]
                mov     al, [bx+si+Profession._passiveSkills]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember.field_34], al
                mov     si, [bp+idx]
                mov     bx, [bp+memberP]
                mov     [bx+si+PartyMember.field_134], 0
                inc     [bp+idx]
                cmp     [bp+idx], 16
                jl      short loc_10BED
                mov     bx, [bp+memberP]
                mov     [bx+PartyMember.field_58], 0
                mov     bx, [bp+memberP]
                mov     al, byte ptr _profession
                mov     [bx+PartyMember._profession], al
                mov     bx, [bp+memberP]
                mov     si, [bp+prof]
                mov     al, [si+Profession.field_7B]
                mov     [bx+PartyMember.field_51], al
                mov     bx, [bp+memberP]
                mov     [bx+PartyMember._sex], 1
                mov     word_14CA6, 0
                pop     si
                mov     sp, bp
                pop     bp
                retn
createNewMember endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_10C58       proc near               ; CODE XREF: editPartyMember+176↓p

var_4           = word ptr -4
var_2           = word ptr -2
partyNum        = word ptr  4
hiliteAtrib     = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     bx, [bp+partyNum]
                shl     bx, 1
                mov     ax, [bx+3D88h]
                mov     [bp+var_4], ax
                mov     ax, _profession
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                mov     [bp+var_2], ax
                cmp     [bp+hiliteAtrib], 7
                jge     short loc_10CA7
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+23h], 0
                jz      short loc_10CDF
                mov     si, [bp+hiliteAtrib]
                add     si, bx
                add     si, 18h
                mov     di, [bp+hiliteAtrib]
                mov     bx, ax
                mov     al, [si]
                cmp     [bx+di+20h], al
                jbe     short loc_10CDF
                inc     byte ptr [si]
                mov     bx, [bp+var_4]
                dec     byte ptr [bx+23h]
                jmp     short loc_10CD3
; ---------------------------------------------------------------------------

loc_10CA7:                              ; CODE XREF: sub_10C58+26↑j
                mov     bx, [bp+var_4]
                inc     byte ptr [bx+50h]
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+50h], 2
                jbe     short loc_10CBA
                mov     byte ptr [bx+50h], 1

loc_10CBA:                              ; CODE XREF: sub_10C58+5C↑j
                mov     bx, [bp+var_4]
                mov     al, [bx+50h]
                and     al, 3
                cmp     al, 1
                jnz     short loc_10CCA
                sub     ax, ax
                jmp     short loc_10CCD
; ---------------------------------------------------------------------------

loc_10CCA:                              ; CODE XREF: sub_10C58+6C↑j
                mov     ax, 5

loc_10CCD:                              ; CODE XREF: sub_10C58+70↑j
                mov     word_14CA6, ax
                call    displaySkills

loc_10CD3:                              ; CODE XREF: sub_10C58+4D↑j
                push    [bp+hiliteAtrib] ; hiliteAtrib
                push    [bp+partyNum]   ; partyNum
                call    writeClassesAndAttributes
                add     sp, 4

loc_10CDF:                              ; CODE XREF: sub_10C58+2F↑j
                                        ; sub_10C58+43↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_10C58       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_10CE5       proc near               ; CODE XREF: editPartyMember+181↓p

var_4           = word ptr -4
var_2           = word ptr -2
partyNum        = word ptr  4
hiliteAtrib     = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     bx, [bp+partyNum]
                shl     bx, 1
                mov     ax, [bx+3D88h]
                mov     [bp+var_4], ax
                mov     ax, _profession
                mov     cl, 7
                shl     ax, cl
                add     ax, _professions
                mov     [bp+var_2], ax
                cmp     [bp+hiliteAtrib], 7
                jge     short loc_10D39
                mov     bx, [bp+var_4]
                mov     di, ax
                mov     al, [di+33h]
                cmp     [bx+23h], al
                jnb     short loc_10D6F
                mov     si, [bp+hiliteAtrib]
                add     si, bx
                add     si, 18h
                mov     di, [bp+hiliteAtrib]
                mov     bx, [bp+var_2]
                mov     al, [si]
                cmp     [bx+di+14h], al
                jnb     short loc_10D6F
                dec     byte ptr [si]
                mov     bx, [bp+var_4]
                inc     byte ptr [bx+23h]
                jmp     short loc_10D63
; ---------------------------------------------------------------------------

loc_10D39:                              ; CODE XREF: sub_10CE5+26↑j
                mov     bx, [bp+var_4]
                mov     si, bx
                mov     al, [si+50h]
                add     al, 2
                and     al, 1
                inc     al
                mov     [bx+50h], al
                mov     bx, [bp+var_4]
                mov     al, [bx+50h]
                and     al, 3
                cmp     al, 1
                jnz     short loc_10D5A
                sub     ax, ax
                jmp     short loc_10D5D
; ---------------------------------------------------------------------------

loc_10D5A:                              ; CODE XREF: sub_10CE5+6F↑j
                mov     ax, 5

loc_10D5D:                              ; CODE XREF: sub_10CE5+73↑j
                mov     word_14CA6, ax
                call    displaySkills

loc_10D63:                              ; CODE XREF: sub_10CE5+52↑j
                push    [bp+hiliteAtrib] ; hiliteAtrib
                push    [bp+partyNum]   ; partyNum
                call    writeClassesAndAttributes
                add     sp, 4

loc_10D6F:                              ; CODE XREF: sub_10CE5+33↑j
                                        ; sub_10CE5+48↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_10CE5       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

editPartyMember proc near               ; CODE XREF: addPartyMember+5B↓p
                                        ; _main+1AA↓p

hiliteAtrib     = word ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
str             = dword ptr -2
t               = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     [bp+var_6], 0
                mov     bx, [bp+t]
                shl     bx, 1
                mov     ax, [bx+3D88h]
                mov     word ptr [bp+str], ax
                mov     [bp+hiliteAtrib], 0
                sub     ax, ax
                push    ax              ; hiliteAtrib
                push    [bp+t]          ; partyNum
                call    writeClassesAndAttributes
                add     sp, 4
                mov     ax, 1
                push    ax
                push    ax
                push    [bp+t]
                call    listParty
                add     sp, 6
                push    word ptr [bp+str]
                call    sub_132C2
                add     sp, 2
                mov     [bp+var_8], ax
                mov     bx, ax

loc_10DBA:                              ; CODE XREF: editPartyMember+27E↓j
                mov     si, word ptr [bp+str]
                mov     byte ptr [bx+si], 1Bh
                mov     si, [bp+var_8]
                mov     bx, word ptr [bp+str]
                mov     byte ptr [bx+si+1], 0

loc_10DCA:                              ; CODE XREF: editPartyMember+C5↓j
                                        ; editPartyMember+E2↓j ...
                cmp     [bp+var_6], 0
                jz      short loc_10DD3
                jmp     loc_11012
; ---------------------------------------------------------------------------

loc_10DD3:                              ; CODE XREF: editPartyMember+59↑j
                jmp     short loc_10E00
; ---------------------------------------------------------------------------

loc_10DD5:                              ; CODE XREF: editPartyMember+90↓j
                call    sub_1185F
                and     al, 7
                cmp     al, 4
                jnb     short loc_10DE2
                mov     al, 1Bh
                jmp     short loc_10DE4
; ---------------------------------------------------------------------------

loc_10DE2:                              ; CODE XREF: editPartyMember+67↑j
                mov     al, 20h ; ' '

loc_10DE4:                              ; CODE XREF: editPartyMember+6B↑j
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     [bx+si], al
                push    [bp+t]          ; t
                mov     ax, 3
                push    ax              ; x
                push    ds
                push    word ptr [bp+str] ; str
                call    writeStringAt
                add     sp, 8
                call    winField1A

loc_10E00:                              ; CODE XREF: editPartyMember:loc_10DD3↑j
                call    _kbhit
                or      ax, ax
                jz      short loc_10DD5
                call    getKeypress
                mov     [bp+var_4], al
                cbw
                cmp     ax, 0FFFDh
                jnz     short loc_10E16
                jmp     loc_10EFB
; ---------------------------------------------------------------------------

loc_10E16:                              ; CODE XREF: editPartyMember+9C↑j
                jle     short loc_10E1B
                jmp     loc_10FB6
; ---------------------------------------------------------------------------

loc_10E1B:                              ; CODE XREF: editPartyMember:loc_10E16↑j
                cmp     ax, 0FFFAh
                jnz     short loc_10E23
                jmp     loc_10EE5
; ---------------------------------------------------------------------------

loc_10E23:                              ; CODE XREF: editPartyMember+A9↑j
                cmp     ax, 0FFFBh
                jnz     short loc_10E2B
                jmp     loc_10EF0
; ---------------------------------------------------------------------------

loc_10E2B:                              ; CODE XREF: editPartyMember+B1↑j
                cmp     ax, 0FFFCh
                jnz     short loc_10E33
                jmp     loc_10F13
; ---------------------------------------------------------------------------

loc_10E33:                              ; CODE XREF: editPartyMember+B9↑j
                jmp     loc_10FC5
; ---------------------------------------------------------------------------

loc_10E36:                              ; CODE XREF: editPartyMember+24D↓j
                cmp     [bp+var_8], 0
                jle     short loc_10DCA
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     byte ptr [bx+si], 0
                mov     ax, 1
                push    ax
                push    ax
                push    [bp+t]
                call    listParty
                add     sp, 6
                mov     [bp+var_6], 1
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10E5A:                              ; CODE XREF: editPartyMember+298↓j
                lea     ax, [bp+t]
                push    ax
                mov     al, [bp+var_4]
                cbw
                push    ax
                call    sub_106E9
                add     sp, 4
                or      al, al
                jnz     short loc_10E70
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10E70:                              ; CODE XREF: editPartyMember+F6↑j
                mov     bx, [bp+t]
                shl     bx, 1
                mov     si, [bx+3D88h]
                mov     al, [si+48h]
                sub     ah, ah
                mov     _profession, ax
                mov     al, [si+50h]
                and     al, 3
                cmp     al, 1
                jnz     short loc_10E8E
                sub     ax, ax
                jmp     short loc_10E91
; ---------------------------------------------------------------------------

loc_10E8E:                              ; CODE XREF: editPartyMember+113↑j
                mov     ax, 5

loc_10E91:                              ; CODE XREF: editPartyMember+117↑j
                mov     word_14CA6, ax
                call    displaySkills
                push    [bp+hiliteAtrib] ; hiliteAtrib
                push    [bp+t]          ; partyNum
                call    writeClassesAndAttributes
                add     sp, 4
                mov     bx, [bp+var_8]
                mov     di, word ptr [bp+str]
                mov     byte ptr [bx+di], 0
                mov     ax, 1
                push    ax
                push    ax
                push    [bp+t]
                call    listParty
                add     sp, 6
                mov     bx, [bp+t]
                shl     bx, 1
                mov     ax, [bx+3D88h]
                mov     word ptr [bp+str], ax
                push    ax
                call    sub_132C2
                add     sp, 2
                mov     [bp+var_8], ax
                mov     bx, ax
                mov     di, word ptr [bp+str]
                mov     byte ptr [bx+di], 1Bh
                mov     di, [bp+var_8]
                mov     bx, word ptr [bp+str]
                mov     byte ptr [bx+di+1], 0
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10EE5:                              ; CODE XREF: editPartyMember+AB↑j
                push    [bp+hiliteAtrib]
                push    [bp+t]
                call    sub_10C58
                jmp     short loc_10F6B
; ---------------------------------------------------------------------------

loc_10EF0:                              ; CODE XREF: editPartyMember+B3↑j
                push    [bp+hiliteAtrib]
                push    [bp+t]
                call    sub_10CE5
                jmp     short loc_10F6B
; ---------------------------------------------------------------------------

loc_10EFB:                              ; CODE XREF: editPartyMember+9E↑j
                mov     ax, [bp+hiliteAtrib]
                add     ax, 7

loc_10F01:                              ; CODE XREF: editPartyMember+1A2↓j
                cwd
                mov     cx, 8
                idiv    cx
                mov     [bp+hiliteAtrib], dx
                push    dx              ; hiliteAtrib
                push    [bp+t]          ; partyNum
                call    writeClassesAndAttributes
                jmp     short loc_10F6B
; ---------------------------------------------------------------------------

loc_10F13:                              ; CODE XREF: editPartyMember+BB↑j
                mov     ax, [bp+hiliteAtrib]
                inc     ax
                jmp     short loc_10F01
; ---------------------------------------------------------------------------

loc_10F19:                              ; CODE XREF: editPartyMember+28B↓j
                mov     al, [bp+var_4]
                cbw
                mov     si, ax
                sub     si, 31h ; '1'
                cmp     _profession, si
                jz      short loc_10F38
                mov     _profession, si
                push    [bp+t]
                call    createNewMember
                add     sp, 2
                call    displaySkills

loc_10F38:                              ; CODE XREF: editPartyMember+1B1↑j
                mov     bx, [bp+t]
                shl     bx, 1
                push    word ptr [bx+3D88h]
                call    sub_10B41
                add     sp, 2
                push    [bp+hiliteAtrib] ; hiliteAtrib
                push    [bp+t]          ; partyNum
                call    writeClassesAndAttributes
                add     sp, 4
                mov     ax, 1
                push    ax
                push    ax
                push    [bp+t]
                call    listParty
                add     sp, 6
                sub     ax, ax
                push    ax
                mov     ax, 2CAh
                push    ax              ; win
                call    setWindow

loc_10F6B:                              ; CODE XREF: editPartyMember+179↑j
                                        ; editPartyMember+184↑j ...
                add     sp, 4
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10F71:                              ; CODE XREF: editPartyMember+244↓j
                cmp     [bp+var_8], 0
                jg      short loc_10F7A
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10F7A:                              ; CODE XREF: editPartyMember+200↑j
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     byte ptr [bx+si], 20h ; ' '
                mov     si, [bp+var_8]
                mov     bx, word ptr [bp+str]
                mov     byte ptr [bx+si-1], 1Bh
                push    [bp+t]          ; t
                mov     ax, 3
                push    ax              ; x
                push    ds
                push    word ptr [bp+str] ; str
                call    writeStringAt
                add     sp, 8
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     byte ptr [bx+si], 0
                dec     [bp+var_8]
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     byte ptr [bx+si], 1Bh
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10FB6:                              ; CODE XREF: editPartyMember+A3↑j
                cmp     ax, 8
                jz      short loc_10F71
                jg      short loc_10FF6
                cmp     ax, 0FFFFh
                jnz     short loc_10FC5
                jmp     loc_10E36
; ---------------------------------------------------------------------------

loc_10FC5:                              ; CODE XREF: editPartyMember:loc_10E33↑j
                                        ; editPartyMember+24B↑j ...
                mov     al, [bp+var_4]
                cbw
                mov     bx, ax
                test    byte ptr [bx+2027h], 7
                jnz     short loc_10FD9
                cmp     al, 20h ; ' '
                jz      short loc_10FD9
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10FD9:                              ; CODE XREF: editPartyMember+25B↑j
                                        ; editPartyMember+25F↑j
                cmp     [bp+var_8], 0Ch
                jl      short loc_10FE2
                jmp     loc_10DCA
; ---------------------------------------------------------------------------

loc_10FE2:                              ; CODE XREF: editPartyMember+268↑j
                mov     bx, [bp+var_8]
                mov     si, word ptr [bp+str]
                mov     al, [bp+var_4]
                mov     [bx+si], al
                inc     [bp+var_8]
                mov     bx, [bp+var_8]
                jmp     loc_10DBA
; ---------------------------------------------------------------------------

loc_10FF6:                              ; CODE XREF: editPartyMember+246↑j
                cmp     ax, 31h ; '1'
                jl      short loc_10FC5
                cmp     ax, 35h ; '5'
                jg      short loc_11003
                jmp     loc_10F19
; ---------------------------------------------------------------------------

loc_11003:                              ; CODE XREF: editPartyMember+289↑j
                cmp     ax, 3Bh ; ';'
                jl      short loc_10FC5
                cmp     ax, 3Dh ; '='
                jg      short loc_11010
                jmp     loc_10E5A
; ---------------------------------------------------------------------------

loc_11010:                              ; CODE XREF: editPartyMember+296↑j
                jmp     short loc_10FC5
; ---------------------------------------------------------------------------

loc_11012:                              ; CODE XREF: editPartyMember+5B↑j
                mov     ax, [bp+t]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
editPartyMember endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

addPartyMember  proc near               ; CODE XREF: _main:loc_11347↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                cmp     _disk1._partyCount, 3
                jnz     short loc_1102C

loc_11028:                              ; CODE XREF: addPartyMember+18↓j
                sub     ax, ax
                jmp     short loc_1107F
; ---------------------------------------------------------------------------

loc_1102C:                              ; CODE XREF: addPartyMember+B↑j
                call    sub_10A65
                mov     [bp+var_2], ax
                inc     ax
                jz      short loc_11028
                mov     ax, [bp+var_2]
                mov     _profession, ax
                mov     al, _disk1._partyCount
                inc     _disk1._partyCount
                sub     ah, ah
                mov     [bp+var_4], ax
                push    ax
                call    createNewMember
                add     sp, 2
                call    displaySkills
                mov     bx, [bp+var_4]
                shl     bx, 1
                push    word ptr [bx+3D88h]
                call    sub_10B41
                add     sp, 2
                mov     ax, 181h
                push    ax
                mov     bx, [bp+var_4]
                shl     bx, 1
                push    word ptr [bx+3D88h] ; char *
                call    _strcpy
                add     sp, 4
                push    [bp+var_4]
                call    editPartyMember
                add     sp, 2
                mov     ax, [bp+var_4]

loc_1107F:                              ; CODE XREF: addPartyMember+F↑j
                mov     sp, bp
                pop     bp
                retn
addPartyMember  endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

removePartyMember proc near             ; CODE XREF: _main+1C5↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si              ; add
                mov     bx, [bp+arg_0]
                shl     bx, 1
                mov     ax, [bx+3D88h]
                mov     [bp+var_6], ax
                sub     ax, ax
                push    ax
                mov     ax, offset _windowMain
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                mov     ax, 1
                push    ax              ; y
                mov     ax, offset aReallyRemoveTh ; "Really remove this member?"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     ax, 3
                push    ax              ; y
                mov     ax, offset aYEsNO ; "Y)es  N)o"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                call    winField1A
                call    getKeypress
                mov     [bp+var_2], al
                cbw
                mov     bx, ax
                test    ascii_flags[bx], 2
                jz      short loc_110DC
                cbw
                sub     ax, 20h ; ' '
                jmp     short loc_110E0
; ---------------------------------------------------------------------------

loc_110DC:                              ; CODE XREF: removePartyMember+51↑j
                mov     al, [bp+var_2]
                cbw

loc_110E0:                              ; CODE XREF: removePartyMember+57↑j
                cmp     ax, 59h ; 'Y'
                jnz     short loc_1114E
                mov     al, _disk1._partyCount
                sub     ah, ah
                cmp     [bp+arg_0], ax
                jnb     short loc_1111E
                mov     ax, [bp+arg_0]
                mov     [bp+var_4], ax
                jmp     short loc_11113
; ---------------------------------------------------------------------------

loc_110F7:                              ; CODE XREF: removePartyMember+99↓j
                mov     ax, 14Ch
                imul    [bp+var_4]
                mov     si, ax
                push    si
                lea     di, [si+2358h]
                lea     si, [si+24A4h]
                push    ds
                pop     es
                assume es:dseg
                mov     cx, 0A6h
                repne movsw
                pop     si
                inc     [bp+var_4]

loc_11113:                              ; CODE XREF: removePartyMember+72↑j
                mov     al, _disk1._partyCount
                sub     ah, ah
                dec     ax
                cmp     [bp+var_4], ax
                jb      short loc_110F7

loc_1111E:                              ; CODE XREF: removePartyMember+6A↑j
                dec     _disk1._partyCount
                mov     al, _disk1._partyCount
                sub     ah, ah
                mov     cx, 14Ch
                mul     cx
                mov     bx, ax
                mov     byte ptr [bx+2358h], 0
                mov     al, _disk1._partyCount
                sub     ah, ah
                cmp     [bp+arg_0], ax
                jnz     short loc_11140
                dec     [bp+arg_0]

loc_11140:                              ; CODE XREF: removePartyMember+B8↑j
                mov     ax, 1
                push    ax
                push    ax
                push    [bp+arg_0]
                call    listParty
                add     sp, 6

loc_1114E:                              ; CODE XREF: removePartyMember+60↑j
                sub     ax, ax
                push    ax
                mov     ax, 2CAh
                push    ax              ; win
                call    setWindow
                add     sp, 4
                mov     ax, [bp+arg_0]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
removePartyMember endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

noOne           proc near               ; CODE XREF: _main+1CE↓p

arg_0           = dword ptr  4

                push    bp
                mov     bp, sp
                call    winClearCurrent
                push    word ptr [bp+arg_0] ; char *
                mov     ax, offset aThereSNoOneToS ; "There's no one to %s!"
                push    ax
                mov     ax, offset sprint_buffer
                push    ax              ; char *
                call    _sprintf
                add     sp, 6
                mov     ax, 2
                push    ax              ; y
                mov     ax, offset sprint_buffer
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                call    winField1A
                call    getKeypress
                pop     bp
                retn
noOne           endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

; int __cdecl main(int argc, const char **argv, const char **envp)
_main           proc near               ; CODE XREF: start+8D↓p

var_10          = word ptr -10h
var_E           = byte ptr -0Eh
var_A           = byte ptr -0Ah
var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
argc            = word ptr  4
argv            = dword ptr  6
envp            = dword ptr  0Ah

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    si              ; add
                mov     [bp+var_10], 0
                mov     [bp+var_A], 0
                call    loadDisk1
                mov     [bp+var_E], al
                mov     al, _disk1._gfxMode
                mov     _gfxMode, al
                mov     ax, 3E8h
                push    ax
                call    sub_12FE4
                add     sp, 2
                mov     dst, ax
                mov     al, [bp+var_E]
                sub     ah, ah
                push    ax
                call    loadClasses
                add     sp, 2
                call    gfxInit
                mov     _welcomeImageLoaded, 0
                mov     word_18214, 0FFFFh
                mov     _profession, 0
                mov     ax, 8000h
                push    ax
                mov     ax, offset aTpict ; "TPICT"
                push    ax
                call    fileOpen
                add     sp, 4
                mov     [bp+var_6], ax
                push    ax
                call    sub_119BE
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                push    word ptr _pictData+2
                push    word ptr _pictData
                push    dx
                push    ax
                push    [bp+var_6]
                call    sub_11902
                add     sp, 0Ah
                push    [bp+var_6]
                call    sub_118F1
                add     sp, 2
                cmp     [bp+var_E], 0
                jz      short loc_11222

loc_1121B:                              ; CODE XREF: _main+20A↓j
                mov     [bp+var_A], 1
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_11222:                              ; CODE XREF: _main+88↑j
                push    word ptr _pictArray1+2
                push    word ptr _pictArray1
                push    word ptr _pictData+2
                push    word ptr _pictData
                call    sub_11AC7
                add     sp, 8
                mov     ax, offset word_14ECE
                push    ax
                call    sub_114D5
                add     sp, 2
                call    getKeypress
                call    sub_11548
                mov     ax, offset word_14ECE
                push    ax
                call    sub_114D5
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, offset _windowMain
                push    ax              ; win
                call    setWindow
                add     sp, 4
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_11262:                              ; CODE XREF: _main+24C↓j
                mov     ax, offset aWelcome ; "Welcome"
                push    ax
                mov     ax, 51
                push    ax
                call    displayWelcomePortrait_2
                add     sp, 4
                sub     ax, ax
                push    ax
                mov     ax, offset _windowSkills
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                call    winField1A
                sub     ax, ax
                push    ax
                mov     ax, offset _windowMain
                push    ax              ; win
                call    setWindow
                add     sp, 4
                call    winClearCurrent
                mov     ax, 1
                push    ax              ; y
                mov     ax, offset aChooseAFunctio ; "Choose a function:"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     ax, 3
                push    ax              ; t
                mov     ax, 5
                push    ax              ; x
                mov     ax, offset text ; "A)dd member      R)emove member"
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                mov     ax, 4
                push    ax              ; t
                mov     ax, 5
                push    ax              ; x
                mov     ax, offset aEDitMemberPLay ; "E)dit member     P)lay the game"
                push    ds
                push    ax              ; str
                call    writeStringAt
                add     sp, 8
                call    winField1A
                call    getKeypress
                mov     [bp+var_8], al
                cbw
                mov     bx, ax
                test    byte ptr [bx+2027h], 2
                jz      short loc_112DE
                sub     al, 20h ; ' '
                jmp     short loc_112E1
; ---------------------------------------------------------------------------

loc_112DE:                              ; CODE XREF: _main+147↑j
                mov     al, [bp+var_8]

loc_112E1:                              ; CODE XREF: _main+14B↑j
                mov     [bp+var_8], al
                cbw
                cmp     ax, 41h ; 'A'
                jz      short loc_11347
                jle     short loc_112EF
                jmp     loc_113C5
; ---------------------------------------------------------------------------

loc_112EF:                              ; CODE XREF: _main+159↑j
                cmp     ax, 3Bh ; ';'
                jge     short loc_112F7
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_112F7:                              ; CODE XREF: _main+161↑j
                cmp     ax, 3Dh ; '='
                jle     short loc_112FF
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_112FF:                              ; CODE XREF: _main+169↑j
                lea     ax, [bp+var_10]
                push    ax
                mov     al, [bp+var_8]
                cbw
                push    ax
                call    sub_106E9
                add     sp, 4
                or      al, al
                jnz     short loc_11315
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_11315:                              ; CODE XREF: _main+17F↑j
                mov     bx, [bp+var_10]
                shl     bx, 1
                mov     si, [bx+3D88h]
                mov     al, [si+48h]
                sub     ah, ah
                mov     _profession, ax
                mov     al, [si+50h]
                and     al, 3
                cmp     al, 1
                jz      short loc_11388

loc_1132F:                              ; CODE XREF: _main+1F5↓j
                mov     ax, 5

loc_11332:                              ; CODE XREF: _main+1F9↓j
                mov     word_14CA6, ax
                call    displaySkills
                push    [bp+var_10]
                call    editPartyMember

loc_1133E:                              ; CODE XREF: _main+1C8↓j
                add     sp, 2

loc_11341:                              ; CODE XREF: _main+1B9↓j
                mov     [bp+var_10], ax
                jmp     loc_113D7
; ---------------------------------------------------------------------------

loc_11347:                              ; CODE XREF: _main+157↑j
                call    addPartyMember
                jmp     short loc_11341
; ---------------------------------------------------------------------------

loc_1134C:                              ; CODE XREF: _main+243↓j
                cmp     _disk1._partyCount, 0
                jz      short loc_1135B
                push    [bp+var_10]
                call    removePartyMember
                jmp     short loc_1133E
; ---------------------------------------------------------------------------

loc_1135B:                              ; CODE XREF: _main+1C0↑j
                mov     ax, 227h

loc_1135E:                              ; CODE XREF: _main+1FE↓j
                push    ax
                call    noOne
                add     sp, 2
                jmp     short loc_113D7
; ---------------------------------------------------------------------------

loc_11367:                              ; CODE XREF: _main+237↓j
                cmp     _disk1._partyCount, 0
                jz      short loc_1138C
                mov     bx, [bp+var_10]
                shl     bx, 1
                mov     si, [bx+3D88h]
                mov     al, [si+48h]
                sub     ah, ah
                mov     _profession, ax
                mov     al, [si+50h]
                and     al, 3
                cmp     al, 1
                jnz     short loc_1132F

loc_11388:                              ; CODE XREF: _main+19C↑j
                sub     ax, ax
                jmp     short loc_11332
; ---------------------------------------------------------------------------

loc_1138C:                              ; CODE XREF: _main+1DB↑j
                mov     ax, 22Eh
                jmp     short loc_1135E
; ---------------------------------------------------------------------------

loc_11391:                              ; CODE XREF: _main+23C↓j
                cmp     _disk1._partyCount, 0
                jz      short loc_1139E
                call    sub_103E0
                jmp     loc_1121B
; ---------------------------------------------------------------------------

loc_1139E:                              ; CODE XREF: _main+205↑j
                call    winClearCurrent
                mov     ax, 1
                push    ax              ; y
                mov     ax, offset aItSToughOutThe ; "It's tough out there!"
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                mov     ax, 3
                push    ax              ; y
                mov     ax, offset aYouShouldTakeS ; "You should take somebody with you."
                push    ax              ; str
                call    writeCenteredString
                add     sp, 4
                call    winField1A
                call    getKeypress
                jmp     short loc_113D7
; ---------------------------------------------------------------------------

loc_113C5:                              ; CODE XREF: _main+15B↑j
                cmp     ax, 45h ; 'E'
                jz      short loc_11367
                cmp     ax, 50h ; 'P'
                jz      short loc_11391
                cmp     ax, 52h ; 'R'
                jnz     short loc_113D7
                jmp     loc_1134C
; ---------------------------------------------------------------------------

loc_113D7:                              ; CODE XREF: _main+8E↑j
                                        ; _main+CE↑j ...
                cmp     [bp+var_A], 0
                jnz     short loc_113E0
                jmp     loc_11262
; ---------------------------------------------------------------------------

loc_113E0:                              ; CODE XREF: _main+24A↑j
                push    word ptr _pictArray1+2
                push    word ptr _pictArray1
                push    word ptr _pictData+2
                push    word ptr _pictData
                call    sub_11AC7
                add     sp, 8
                mov     ax, 29Eh
                push    ax
                call    sub_114D5
                add     sp, 2
                call    sub_1141A
                sub     ax, ax
                push    ax
                mov     ax, 26Ch
                push    ax
                mov     ax, 274h
                push    ax
                call    sub_139FE
                add     sp, 6
                pop     si
                mov     sp, bp
                pop     bp
                retn
_main           endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1141A       proc near               ; CODE XREF: _main+26F↑p
                push    es
                mov     ax, word ptr _pictArray1+2
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                assume es:nothing
                retn
sub_1141A       endp

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR gfxInit

loc_11424:                              ; CODE XREF: gfxInit+1E↓j
                                        ; gfxInit+31↓j ...
                cli
                mov     al, 3
                mov     ah, 0
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                mov     dx, offset aYouDoNotHaveEn ; "You do not have enough memory to run Fo"...
                mov     ah, 9
                int     21h             ; DOS - PRINT STRING
                                        ; DS:DX -> string terminated by "$"
                mov     al, 0
                mov     ah, 4Ch
                int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
; END OF FUNCTION CHUNK FOR gfxInit     ; AL = exit code
; ---------------------------------------------------------------------------
                retn

; =============== S U B R O U T I N E =======================================


gfxInit         proc near               ; CODE XREF: _main+35↑p

; FUNCTION CHUNK AT 1AC4 SIZE 00000014 BYTES

                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     ah, 48h ; 'H'
                mov     bx, 0FFFFh
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                cmp     _gfxMode, 1
                jnz     short loc_11453
                sub     bx, 7D1h

loc_11453:                              ; CODE XREF: gfxInit+14↑j
                mov     ah, 48h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                jb      short loc_11424
                mov     word ptr _pictData+2, ax
                cmp     _gfxMode, 1
                jnz     short loc_1146C
                mov     ah, 48h ; 'H'
                mov     bx, 7D0h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                jb      short loc_11424

loc_1146C:                              ; CODE XREF: gfxInit+28↑j
                push    es
                mov     ax, word ptr _pictData+2
                mov     es, ax
                mov     ah, 49h
                int     21h             ; DOS - 2+ - FREE MEMORY
                                        ; ES = segment address of area to be freed
                pop     es
                mov     ah, 48h ; 'H'
                mov     bx, 45F5h
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                jb      short loc_11424
                mov     word ptr _pictArray1+2, ax
                add     ax, 7D0h
                mov     word ptr _pictArray2+2, ax
                add     ax, 43Ah
                mov     word ptr _pictWelcome+2, ax
                add     ax, 1CAh
                mov     word ptr _pictData+2, ax
                call    gfxScanlinesSetup
                call    fontOpen
                mov     cx, 3E80h
                xor     di, di
                mov     es, word ptr _pictArray1+2
                xor     ax, ax
                rep stosw
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                retn
gfxInit         endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_114B3       proc near               ; CODE XREF: listParty+BE↑p
                                        ; sub_114D5+C↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                push    es
                push    bp
                mov     si, [bp+arg_0]
                mov     ax, [si]
                mov     di, [si+4]
                mov     cx, [si+6]
                mov     si, [si+2]
                call    sub_13CC0
                pop     bp
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                retn
sub_114B3       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_114D5       proc near               ; CODE XREF: sub_104EA+77↑p
                                        ; _main+AB↑p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                push    es
                push    bp
                mov     si, [bp+arg_0]
                push    si
                call    sub_114B3
                add     sp, 2
                call    sub_13F5D
                pop     bp
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                retn
sub_114D5       endp

; ---------------------------------------------------------------------------
                push    si
                push    di
                push    bp
                call    sub_13F5D
                pop     bp
                pop     di
                pop     si
                retn

; =============== S U B R O U T I N E =======================================


writeString?    proc near               ; CODE XREF: sub_11548+A↓p
                push    di
                push    si
                push    es
                mov     si, dst
                add     si, ax
                mov     ax, _xPos
                push    ax
                mov     ax, _textY
                push    ax
                mov     _textY, 0

loc_11516:                              ; CODE XREF: writeString?+3B↓j
                mov     _xPos, 0

loc_1151C:                              ; CODE XREF: writeString?+30↓j
                lodsb
                or      al, al
                jz      short loc_11526
                push    si
                call    writeChar?
                pop     si

loc_11526:                              ; CODE XREF: writeString?+20↑j
                inc     _xPos
                cmp     _xPos, 40
                jnz     short loc_1151C
                inc     _textY
                cmp     _textY, 19h
                jnz     short loc_11516
                pop     ax
                mov     _textY, ax
                pop     ax
                mov     _xPos, ax
                pop     es
                pop     si
                pop     di
                retn
writeString?    endp


; =============== S U B R O U T I N E =======================================


sub_11548       proc near               ; CODE XREF: _main+B4↑p
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                xor     ax, ax
                call    writeString?
                pop     bp
                pop     es
                pop     ds
                assume ds:nothing
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                retn
sub_11548       endp

                assume ds:dseg

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

; void __cdecl setWindow(Window *win, bool add)
setWindow       proc near               ; CODE XREF: listParty+13↑p
                                        ; listParty+CA↑p ...

win             = dword ptr  4
add             = byte ptr  8

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     si, word ptr [bp+win]
                mov     _currentWindow, si
                cmp     word ptr [bp+win+2], 0
                jz      short loc_11589
                push    si
                mov     [si+Window.field_18], 0
                call    winClearCurrent
                pop     si
                cmp     [si+Window._fnFocus], 0
                jz      short loc_11589
                call    [si+Window._fnFocus]

loc_11589:                              ; CODE XREF: setWindow+16↑j
                                        ; setWindow+26↑j
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retn
setWindow       endp


; =============== S U B R O U T I N E =======================================


winClearCurrent proc near               ; CODE XREF: listParty+19↑p
                                        ; writeClassesAndAttributes+21↑p ...
                mov     si, _currentWindow
                add     si, 0Ch
                call    sub_117C4
                retn
winClearCurrent endp


; =============== S U B R O U T I N E =======================================


writeString     proc near               ; CODE XREF: writeString+24↓j
                                        ; writeString+62↓j ...
                mov     _writeStringPtr1, di
                mov     _writeStringPtr2, di
                mov     _writeStringPtr3, di

loc_115AA:                              ; CODE XREF: writeString+45↓j
                                        ; writeString+48↓j
                mov     si, _currentWindow
                mov     al, es:[di]
                or      al, al
                jz      short sub_11602
                cmp     al, 0Dh
                jnz     short loc_115C4
                call    sub_11602
                push    di
                call    sub_1164F
                pop     di
                inc     di
                jmp     short writeString
; ---------------------------------------------------------------------------

loc_115C4:                              ; CODE XREF: writeString+19↑j
                push    ax
                mov     cx, di
                sub     cx, _writeStringPtr1
                mov     ax, [si+8]
                add     ax, cx
                cmp     ax, [si+4]
                jg      short loc_115E8
                pop     ax
                cmp     al, 20h ; ' '

loc_115D8:                              ; CODE XREF: sub_11D34-19A↓p
                                        ; DATA XREF: dseg:off_16A40↓o
                jnz     short loc_115E5
                mov     _writeStringPtr3, di
                inc     di
                mov     _writeStringPtr2, di
                jmp     short loc_115AA
; ---------------------------------------------------------------------------

loc_115E5:                              ; CODE XREF: writeString:loc_115D8↑j
                inc     di
                jmp     short loc_115AA
; ---------------------------------------------------------------------------

loc_115E8:                              ; CODE XREF: writeString+35↑j
                pop     ax
                cmp     al, 20h ; ' '
                jnz     short loc_115F6
                mov     _writeStringPtr3, di
                inc     di
                mov     _writeStringPtr2, di

loc_115F6:                              ; CODE XREF: writeString+4D↑j
                call    sub_11606
                call    sub_1164F
                mov     di, _writeStringPtr2
                jmp     short writeString
writeString     endp


; =============== S U B R O U T I N E =======================================


sub_11602       proc near               ; CODE XREF: writeString+15↑j
                                        ; writeString+1B↑p
                mov     _writeStringPtr3, di
sub_11602       endp


; =============== S U B R O U T I N E =======================================


sub_11606       proc near               ; CODE XREF: writeString:loc_115F6↑p
                mov     cx, _writeStringPtr3
                sub     cx, _writeStringPtr1
                jz      short locret_11613
                call    writeString_1

locret_11613:                           ; CODE XREF: sub_11606+8↑j
                retn
sub_11606       endp


; =============== S U B R O U T I N E =======================================


writeString_1   proc near               ; CODE XREF: sub_11606+A↑p
                mov     di, _writeStringPtr1

loc_11618:                              ; CODE XREF: writeString_1+1A↓j
                push    cx
                mov     al, es:[di]     ; c
                push    di
                mov     si, _currentWindow ; win
                call    writeChar
                mov     si, _currentWindow
                inc     [si+Window._textX]
                pop     di
                inc     di
                pop     cx
                loop    loc_11618
                retn
writeString_1   endp


; =============== S U B R O U T I N E =======================================


winField1A      proc near               ; CODE XREF: listParty+B2↑p
                                        ; displayWelcomePortrait_2+31↑p ...
                push    si
                push    di
                push    bp
                mov     si, _currentWindow
                cmp     word ptr [si+1Ah], 0
                lea     di, [si+0Ch]
                jz      short loc_11644
                mov     di, [si+1Ah]

loc_11644:                              ; CODE XREF: winField1A+E↑j
                push    di
                call    sub_114D5
                add     sp, 2
                pop     bp
                pop     di
                pop     si
                retn
winField1A      endp


; =============== S U B R O U T I N E =======================================


sub_1164F       proc near               ; CODE XREF: writeString+1F↑p
                                        ; writeString+5B↑p
                push    es
                mov     si, _currentWindow
                mov     ax, [si]
                mov     [si+8], ax
                inc     word ptr [si+0Ah]
                mov     ax, [si+0Ah]
                cmp     ax, [si+6]
                jle     short loc_1168C
                mov     ax, [si+6]
                mov     [si+0Ah], ax
                push    si
                cmp     byte_14EBE, 0
                jz      short loc_1168B
                mov     ax, [si+6]
                sub     ax, [si+2]
                inc     word ptr [si+18h]
                cmp     [si+18h], ax
                jle     short loc_1168B
                mov     word ptr [si+18h], 0
                call    winField1A
                call    getKeypress

loc_1168B:                              ; CODE XREF: sub_1164F+21↑j
                                        ; sub_1164F+2F↑j
                pop     si

loc_1168C:                              ; CODE XREF: sub_1164F+13↑j
                pop     es
                retn
sub_1164F       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

; void __cdecl writeStringAt(const char far *str, int x, int t)
writeStringAt   proc near               ; CODE XREF: writeCenteredString+66↑p
                                        ; listParty+60↑p ...

str             = dword ptr  4
x               = word ptr  8
t               = word ptr  0Ah

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     si, _currentWindow
                mov     bx, [bp+x]
                cmp     bx, 0FFFFh
                jz      short loc_116AC
                mov     ax, [si]
                add     ax, bx
                mov     [si+8], ax

loc_116AC:                              ; CODE XREF: writeStringAt+15↑j
                mov     bx, [bp+t]
                cmp     bx, 0FFFFh
                jz      short loc_116BC
                mov     ax, [si+2]
                add     ax, bx
                mov     [si+0Ah], ax

loc_116BC:                              ; CODE XREF: writeStringAt+24↑j
                mov     es, word ptr [bp+str+2]
                mov     di, word ptr [bp+str]
                call    writeString
                pop     bp
                pop     es
                pop     ds
                assume ds:nothing
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retn
writeStringAt   endp

; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     si, ds:29Ch
                mov     ax, [si]
                add     ax, [bp+6]
                mov     [si+8], ax
                mov     ax, [si+2]
                add     ax, [bp+8]
                mov     [si+0Ah], ax
                mov     ax, [bp+4]
                call    writeChar
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_116FF       proc near               ; CODE XREF: sub_10B41+1D↑p

arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     ax, [bp+arg_0]
                cmp     ax, [bp+arg_2]
                jge     short loc_1171F

loc_11712:                              ; CODE XREF: sub_116FF+19↓j
                                        ; sub_116FF+1E↓j
                call    sub_1173D
                cmp     ax, [bp+arg_0]
                jl      short loc_11712
                cmp     ax, [bp+arg_2]
                jg      short loc_11712

loc_1171F:                              ; CODE XREF: sub_116FF+11↑j
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retn
sub_116FF       endp


; =============== S U B R O U T I N E =======================================


sub_11729       proc near               ; CODE XREF: getKeypress:loc_10577↑p
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                call    sub_1173D
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                retn
sub_11729       endp


; =============== S U B R O U T I N E =======================================


sub_1173D       proc near               ; CODE XREF: sub_116FF:loc_11712↑p
                                        ; sub_11729+8↑p
                lea     si, ds:27Eh
                mov     cx, 0Fh
                mov     dl, [si+0Fh]
                mov     bx, 0Eh
                clc

loc_1174B:                              ; CODE XREF: sub_1173D+15↓j
                mov     al, [bx+si]
                adc     dl, al
                mov     [bx+si], dl
                dec     bx
                loop    loc_1174B
                mov     cx, 10h
                mov     bx, 10h

loc_1175A:                              ; CODE XREF: sub_1173D+20↓j
                dec     bx
                inc     byte ptr [bx+si]
                loopne  loc_1175A
                mov     al, [si]
                retn
sub_1173D       endp

; ---------------------------------------------------------------------------
                push    bx
                push    si
                push    di
                push    es
                push    ds
                push    bp
                mov     ah, 48h ; 'H'
                mov     bx, 0FFFFh
                int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                                        ; BX = number of 16-byte paragraphs desired
                mov     ax, bx
                pop     bp
                pop     ds
                assume ds:dseg
                pop     es
                pop     di
                pop     si
                pop     bx
                retn

; =============== S U B R O U T I N E =======================================


writeChar?      proc near               ; CODE XREF: writeString?+23↑p
                push    si
                push    di
                push    ds
                push    es
                push    bp
                push    ax
                mov     ax, _textY
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                mov     di, ax
                mov     di, _scanlinePtrs?[di]
                mov     ax, _xPos
                shl     ax, 1
                shl     ax, 1
                add     di, ax
                mov     es, word ptr _pictArray1+2
                pop     ax
                xor     ah, ah
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                mov     si, _fontData
                add     si, ax
                mov     bx, 8

loc_117B2:                              ; CODE XREF: writeChar?+44↓j
                mov     cx, 2
                rep movsw
                add     di, 156
                dec     bx
                jnz     short loc_117B2
                pop     bp
                pop     es
                pop     ds
                pop     di
                pop     si
                retn
writeChar?      endp


; =============== S U B R O U T I N E =======================================


sub_117C4       proc near               ; CODE XREF: winClearCurrent+7↑p
                push    si
                push    di
                push    es
                push    bp
                mov     es, word ptr _pictArray1+2
                mov     di, [si+2]
                shl     di, 1
                mov     di, [di+5BFh]
                add     di, [si]
                xor     ax, ax
                mov     cx, [si+6]

loc_117DC:                              ; CODE XREF: sub_117C4+27↓j
                push    cx

loc_117DD:                              ; DATA XREF: sub_13A14+110↓w
                push    di
                mov     cx, [si+4]
                sar     cx, 1
                rep stosw
                pop     di

loc_117E6:                              ; DATA XREF: sub_13A14+116↓w
                add     di, 0A0h
                pop     cx
                loop    loc_117DC
                pop     bp
                pop     es
                pop     di
                pop     si
                retn
sub_117C4       endp


; =============== S U B R O U T I N E =======================================


; void __usercall writeChar(Window near *win@<si>, char c@<al>)
writeChar       proc near               ; CODE XREF: writeString_1+D↑p
                                        ; sg0138:1D92↑p
                push    bx
                push    cx
                push    si
                push    di
                push    ds
                push    es
                push    bp
                push    ax
                mov     bl, _inverseText
                mov     ax, [si+Window._textY]
                shl     ax, 1

loc_11803:                              ; DATA XREF: sub_139FE+3↓r
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                mov     di, ax
                add     di, [si+Window.field_14]
                mov     di, _scanlinePtrs?[di]
                mov     ax, [si+Window._textX]
                shl     ax, 1
                shl     ax, 1
                add     di, ax
                mov     es, word ptr _pictArray1+2
                pop     ax
                xor     ah, ah
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                shl     ax, 1
                mov     si, _fontData
                add     si, ax
                mov     cx, 8
                or      bl, bl
                jz      short loc_1184D
                mov     bx, 0FFFFh

loc_1183C:                              ; CODE XREF: writeChar+56↓j
                lodsw
                xor     ax, bx
                stosw
                lodsw
                xor     ax, bx
                stosw
                add     di, 156
                loop    loc_1183C
                jmp     short loc_11857
; ---------------------------------------------------------------------------
                db 90h
; ---------------------------------------------------------------------------

loc_1184D:                              ; CODE XREF: writeChar+45↑j
                                        ; writeChar+63↓j
                lodsw
                stosw
                lodsw
                stosw
                add     di, 156
                loop    loc_1184D

loc_11857:                              ; CODE XREF: writeChar+58↑j
                pop     bp
                pop     es
                pop     ds
                assume ds:nothing
                pop     di
                pop     si
                pop     cx
                pop     bx
                retn
writeChar       endp


; =============== S U B R O U T I N E =======================================


sub_1185F       proc near               ; CODE XREF: editPartyMember:loc_10DD5↑p
                push    cx
                push    dx
                xor     ah, ah
                int     1Ah             ; CLOCK - GET TIME OF DAY
                                        ; Return: CX:DX = clock count
                                        ; AL = 00h if clock was read or written (via AH=0,1) since the previous
                                        ; midnight
                                        ; Otherwise, AL > 0
                mov     ax, dx
                pop     dx
                pop     cx
                retn
sub_1185F       endp

; ---------------------------------------------------------------------------
                push    cx
                push    bx
                push    si
                push    di
                push    ds
                push    es
                push    bp
                mov     ah, 2
                mov     dl, 7
                int     21h             ; DOS - DISPLAY OUTPUT
                                        ; DL = character to send to standard output
                pop     bp
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bx
                pop     cx
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                push    es
                push    bp
                mov     ax, ds
                mov     es, ax
                mov     di, 0D52h
                xor     al, al
                mov     bl, [bp+4]
                or      bl, bl
                jz      short loc_118B1
                jmp     short loc_118A0
; ---------------------------------------------------------------------------
                db 90h
; ---------------------------------------------------------------------------

loc_1189B:                              ; CODE XREF: sg0138:1F42↓j
                mov     cx, 100h
                repne scasb

loc_118A0:                              ; CODE XREF: sg0138:1F38↑j
                dec     bl
                jnz     short loc_1189B
                mov     ax, 0FFFFh
                push    ax
                push    ax
                push    ds
                push    di
                call    writeStringAt
                add     sp, 8

loc_118B1:                              ; CODE XREF: sg0138:1F36↑j
                pop     bp
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

fileOpen        proc near               ; CODE XREF: fontOpen+D↑p
                                        ; loadDisk1+E↑p ...

arg_0           = word ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                mov     ah, 3Dh ; '='
                mov     al, [bp+arg_2]
                mov     dx, [bp+arg_0]
                int     21h             ; DOS - 2+ - OPEN DISK FILE WITH HANDLE
                                        ; DS:DX -> ASCIZ filename
                                        ; AL = access mode
                                        ; 0 - read, 1 - write, 2 - read & write
                jnb     short loc_118C9
                xor     ax, ax

loc_118C9:                              ; CODE XREF: fileOpen+D↑j
                pop     bp
                retn
fileOpen        endp

; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                mov     ah, 3Ch ; '<'
                xor     ch, ch
                mov     cl, [bp+6]
                mov     dx, [bp+4]
                int     21h             ; DOS - 2+ - CREATE A FILE WITH HANDLE (CREAT)
                                        ; CX = attributes for file
                                        ; DS:DX -> ASCIZ filename (may include drive and path)
                jnb     short loc_118DE
                xor     ax, ax

loc_118DE:                              ; CODE XREF: sg0138:1F7A↑j
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                mov     ah, 41h ; 'A'
                mov     dx, [bp+4]
                int     21h             ; DOS - 2+ - DELETE A FILE (UNLINK)
                                        ; DS:DX -> ASCIZ pathname of file to delete (no wildcards allowed)
                xor     ax, ax
                jb      short loc_118EF
                inc     ax

loc_118EF:                              ; CODE XREF: sg0138:1F8C↑j
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_118F1       proc near               ; CODE XREF: fontOpen+49↑p
                                        ; fontOpen+6E↑p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ah, 3Eh ; '>'
                mov     bx, [bp+arg_0]
                int     21h             ; DOS - 2+ - CLOSE A FILE WITH HANDLE
                                        ; BX = file handle
                xor     ax, ax
                jb      short loc_11900
                inc     ax

loc_11900:                              ; CODE XREF: sub_118F1+C↑j
                pop     bp
                retn
sub_118F1       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11902       proc near               ; CODE XREF: fontOpen+2B↑p
                                        ; fontOpen+65↑p ...

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8
arg_6           = word ptr  0Ah
arg_8           = word ptr  0Ch

                push    bp
                mov     bp, sp
                push    ds
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_8]
                mov     ds, ax
                mov     dx, [bp+arg_6]

loc_11911:                              ; CODE XREF: sub_11902+3D↓j
                                        ; sub_11902+43↓j
                mov     cx, [bp+arg_2]
                cmp     [bp+arg_4], 0
                jz      short loc_1191D
                mov     cx, 0FFFFh

loc_1191D:                              ; CODE XREF: sub_11902+16↑j
                mov     ah, 3Fh
                int     21h             ; DOS - 2+ - READ FROM FILE WITH HANDLE
                                        ; BX = file handle, CX = number of bytes to read
                                        ; DS:DX -> buffer
                jb      short loc_1194D
                cmp     cx, ax
                jnz     short loc_1194D
                clc
                sub     [bp+arg_2], cx
                sbb     [bp+arg_4], 0
                clc
                add     dx, cx
                jnb     short loc_1193B
                mov     ax, ds
                add     ax, 1000h
                mov     ds, ax
                assume ds:nothing

loc_1193B:                              ; CODE XREF: sub_11902+30↑j
                cmp     [bp+arg_4], 0
                jnz     short loc_11911
                cmp     [bp+arg_2], 0
                jnz     short loc_11911
                mov     ax, 1
                pop     ds
                assume ds:dseg
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1194D:                              ; CODE XREF: sub_11902+1F↑j
                                        ; sub_11902+23↑j
                xor     ax, ax
                pop     ds
                pop     bp
                retn
sub_11902       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11952       proc near               ; CODE XREF: sub_103E0+5E↑p

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8
arg_6           = word ptr  0Ah
arg_8           = word ptr  0Ch

                push    bp
                mov     bp, sp
                push    ds
                mov     bx, [bp+arg_0]
                mov     ax, [bp+arg_8]
                mov     ds, ax
                assume ds:nothing
                mov     dx, [bp+arg_6]

loc_11961:                              ; CODE XREF: sub_11952+3D↓j
                                        ; sub_11952+43↓j
                mov     cx, [bp+arg_2]
                cmp     [bp+arg_4], 0
                jz      short loc_1196D
                mov     cx, 0FFF0h

loc_1196D:                              ; CODE XREF: sub_11952+16↑j
                mov     ah, 40h
                int     21h             ; DOS - 2+ - WRITE TO FILE WITH HANDLE
                                        ; BX = file handle, CX = number of bytes to write, DS:DX -> buffer
                jb      short loc_1199D
                cmp     cx, ax
                jnz     short loc_1199D
                clc
                sub     [bp+arg_2], cx
                sbb     [bp+arg_4], 0
                clc
                add     dx, cx
                jnb     short loc_1198B
                mov     ax, ds
                add     ax, 1000h
                mov     ds, ax
                assume ds:nothing

loc_1198B:                              ; CODE XREF: sub_11952+30↑j
                cmp     [bp+arg_4], 0
                jnz     short loc_11961
                cmp     [bp+arg_2], 0
                jnz     short loc_11961
                mov     ax, 1
                pop     ds
                assume ds:dseg
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1199D:                              ; CODE XREF: sub_11952+1F↑j
                                        ; sub_11952+23↑j
                xor     ax, ax
                pop     ds
                pop     bp
                retn
sub_11952       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_119A2       proc near               ; CODE XREF: loadClasses+22↑p

arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8
arg_6           = byte ptr  0Ah

                push    bp
                mov     bp, sp
                mov     ah, 42h ; 'B'
                mov     al, [bp+arg_6]
                mov     bx, [bp+arg_0]
                mov     cx, [bp+arg_4]
                mov     dx, [bp+arg_2]
                int     21h             ; DOS - 2+ - MOVE FILE READ/WRITE POINTER (LSEEK)
                                        ; AL = method:
                                        ; 0-from beginnig,1-from current,2-from end
                jnb     short loc_119BC
                mov     ax, 0FFFFh
                mov     dx, ax

loc_119BC:                              ; CODE XREF: sub_119A2+13↑j
                pop     bp
                retn
sub_119A2       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_119BE       proc near               ; CODE XREF: loadDisk1+21↑p
                                        ; loadClasses+68↑p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     bx, [bp+arg_0]
                sub     cx, cx
                sub     dx, dx
                mov     ax, 4202h
                int     21h             ; DOS - 2+ - MOVE FILE READ/WRITE POINTER (LSEEK)
                                        ; AL = method: offset from end of file
                push    dx
                push    ax
                sub     dx, dx
                mov     ax, 4200h
                int     21h             ; DOS - 2+ - MOVE FILE READ/WRITE POINTER (LSEEK)
                                        ; AL = method: offset from beginning of file
                pop     ax
                pop     dx
                pop     bp
                retn
sub_119BE       endp


; =============== S U B R O U T I N E =======================================


sub_119DA       proc near               ; CODE XREF: sub_11AC7+17↓p
                cld
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                push    ds
                mov     ds, _pictPtr
                assume ds:nothing
                xor     si, si
                mov     bx, [si]
                mov     dx, [si+2]
                pop     ds
                assume ds:dseg
                mov     word_15A22, bx
                mov     word_15A24, dx
                mov     di, 0DFBh
                mov     cx, 808h
                mov     ax, 2020h
                rep stosw
                mov     si, 4
                xor     di, di
                mov     es, word_15A28
                assume es:nothing
                mov     cx, word_15A22
                mov     dx, word_15A24
                mov     word_15A1E, cx
                mov     word_15A20, dx
                mov     bx, 0FEEh
                mov     byte_15A2A, 1

loc_11A21:                              ; CODE XREF: sub_119DA+9C↓j
                                        ; sub_119DA+E9↓j
                cmp     si, 40h ; '@'
                jb      short loc_11A2E
                sub     si, 40h ; '@'
                add     _pictPtr, 4

loc_11A2E:                              ; CODE XREF: sub_119DA+4A↑j
                cmp     di, 40h ; '@'
                jb      short loc_11A3F
                sub     di, 40h ; '@'
                add     word_15A28, 4
                mov     es, word_15A28

loc_11A3F:                              ; CODE XREF: sub_119DA+57↑j
                dec     byte_15A2A
                jnz     short loc_11A53
                push    ds
                mov     ds, _pictPtr
                assume ds:nothing
                lodsb
                mov     ah, al
                pop     ds
                assume ds:dseg
                mov     byte_15A2A, 8

loc_11A53:                              ; CODE XREF: sub_119DA+69↑j
                shr     ah, 1
                jnb     short loc_11A78
                push    ds
                mov     ds, _pictPtr
                assume ds:nothing
                lodsb
                stosb
                pop     ds
                assume ds:dseg
                sub     cx, 1
                sbb     dx, 0
                or      dx, dx
                jnz     short loc_11A6D
                or      cx, cx
                jz      short locret_11AC6

loc_11A6D:                              ; CODE XREF: sub_119DA+8D↑j
                mov     byte_15A2B[bx], al
                inc     bx
                and     bx, 0FFFh
                jmp     short loc_11A21
; ---------------------------------------------------------------------------

loc_11A78:                              ; CODE XREF: sub_119DA+7B↑j
                mov     bp, cx
                push    ds
                mov     ds, _pictPtr
                assume ds:nothing
                lodsb
                mov     cl, al
                lodsb
                pop     ds
                push    si
                mov     ch, al
                and     al, 0Fh
                add     al, 3
                shr     ch, 1
                shr     ch, 1
                shr     ch, 1
                shr     ch, 1
                mov     si, cx
                mov     cl, al
                xor     ch, ch

loc_11A99:                              ; CODE XREF: sub_119DA+E4↓j
                mov     al, [si+0DFBh]
                stosb
                inc     si
                and     si, 0FFFh
                sub     bp, 1
                sbb     dx, 0
                or      dx, dx
                jnz     short loc_11AB5
                or      bp, bp
                jnz     short loc_11AB5
                pop     si
                jmp     short locret_11AC6
; ---------------------------------------------------------------------------
                db 90h
; ---------------------------------------------------------------------------

loc_11AB5:                              ; CODE XREF: sub_119DA+D1↑j
                                        ; sub_119DA+D5↑j
                mov     [bx+0DFBh], al
                inc     bx
                and     bx, 0FFFh
                loop    loc_11A99
                mov     cx, bp
                pop     si
                jmp     loc_11A21
; ---------------------------------------------------------------------------

locret_11AC6:                           ; CODE XREF: sub_119DA+91↑j
                                        ; sub_119DA+D8↑j
                retn
sub_119DA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11AC7       proc near               ; CODE XREF: displayWelcomePortrait+64↑p
                                        ; _main+A1↑p ...

arg_2           = word ptr  6
arg_6           = word ptr  0Ah

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     ax, [bp+arg_2]
                mov     ds:0DF6h, ax
                mov     ax, [bp+arg_6]
                mov     ds:0DF8h, ax
                call    sub_119DA
                pop     bp
                pop     es
                assume es:nothing
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retn
sub_11AC7       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: library function noreturn fuzzy-sp

start           proc near
                mov     ah, 30h
                int     21h             ; DOS - GET DOS VERSION
                                        ; Return: AL = major version number (00h for DOS 1.x)
                cmp     al, 2
                jnb     short loc_11AF6
                int     20h             ; DOS - PROGRAM TERMINATION
                                        ; returns to DOS--identical to INT 21/AH=00h
; ---------------------------------------------------------------------------

loc_11AF6:                              ; CODE XREF: start+6↑j
                mov     di, seg dseg
                mov     si, ds:2
                sub     si, di
                cmp     si, 1000h
                jb      short loc_11B08
                mov     si, 1000h

loc_11B08:                              ; CODE XREF: start+17↑j
                cli
                mov     ss, di
                assume ss:dseg
                add     sp, 406Eh
                sti
                jnb     short loc_11B22
                push    ss
                pop     ds
                assume ds:dseg
                call    __FF_MSGBANNER
                xor     ax, ax
                push    ax
                call    sub_11F93
                mov     ax, 4CFFh
                int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
                                        ; AL = exit code
; ---------------------------------------------------------------------------

loc_11B22:                              ; CODE XREF: start+24↑j
                and     sp, 0FFFEh
                mov     ss:word_16A42, sp
                mov     ss:word_16A3E, sp
                mov     ax, si
                mov     cl, 4
                shl     ax, cl
                dec     ax
                mov     ss:word_16A3C, ax
                add     si, di
                mov     word_14C31+1, si
                mov     bx, es
                sub     bx, si
                neg     bx
                mov     ah, 4Ah
                int     21h             ; DOS - 2+ - ADJUST MEMORY BLOCK SIZE (SETBLOCK)
                                        ; ES = segment address of block to change
                                        ; BX = new size in paragraphs
                mov     ss:word_16AB3, ds
                push    ss
                pop     es
                assume es:dseg
                cld
                mov     di, 22DCh
                mov     cx, 4070h
                sub     cx, di
                xor     ax, ax
                rep stosb
                push    ss
                pop     ds
                call    sub_11B9E
                push    ss
                pop     ds
                call    sub_11EFA
                call    loc_11D6C
                xor     bp, bp
                push    word_16AD4
                push    argv            ; argv
                push    argc            ; argc
                call    _main
                push    ax              ; int
                call    _exit
start           endp ; sp-analysis failed

; ---------------------------------------------------------------------------
                mov     ax, seg dseg
                mov     ds, ax
                mov     ax, 3
                mov     ss:off_16A40, 1C62h
; [0000000F BYTES: COLLAPSED CHUNK OF FUNCTION sub_11D34. PRESS CTRL-NUMPAD+ TO EXPAND]

; =============== S U B R O U T I N E =======================================


sub_11B9E       proc near               ; CODE XREF: start+74↑p

; FUNCTION CHUNK AT 222F SIZE 0000000F BYTES
; FUNCTION CHUNK AT 23CE SIZE 00000006 BYTES

                mov     ah, 30h
                int     21h             ; DOS - GET DOS VERSION
                                        ; Return: AL = major version number (00h for DOS 1.x)
                mov     word_16AB5, ax
                mov     ax, 3500h
                int     21h             ; DOS - 2+ - GET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; Return: ES:BX = value of interrupt vector
                mov     word ptr dword_16AA1, bx
                mov     word ptr dword_16AA1+2, es
                push    cs
                pop     ds
                assume ds:sg0138
                mov     ax, 2500h
                mov     dx, 1B80h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                push    ss
                pop     ds
                assume ds:dseg
                mov     cx, word_16D82
                jcxz    short loc_11BF2
                mov     es, word_16AB3
                assume es:nothing
                mov     si, es:2Ch
                lds     ax, dword_16D84
                mov     dx, ds
                xor     bx, bx
                call    dword ptr ss:unk_16D80
                jnb     short loc_11BE1
                push    ss
                pop     ds
                jmp     loc_11D2E
; ---------------------------------------------------------------------------

loc_11BE1:                              ; CODE XREF: sub_11B9E+3C↑j
                lds     ax, ss:dword_16D88
                assume ds:nothing
                mov     dx, ds
                mov     bx, 3
                call    dword ptr ss:unk_16D80
                push    ss
                pop     ds
                assume ds:dseg

loc_11BF2:                              ; CODE XREF: sub_11B9E+24↑j
                mov     es, word_16AB3
                mov     cx, es:2Ch
                jcxz    short loc_11C33
                mov     es, cx
                xor     di, di

loc_11C01:                              ; CODE XREF: sub_11B9E+7C↓j
                cmp     byte ptr es:[di], 0
                jz      short loc_11C33
                mov     cx, 0Ch
                mov     si, 1E64h
                repe cmpsb
                jz      short loc_11C1C
                mov     cx, 7FFFh
                xor     ax, ax
                repne scasb
                jnz     short loc_11C33
                jmp     short loc_11C01
; ---------------------------------------------------------------------------

loc_11C1C:                              ; CODE XREF: sub_11B9E+71↑j
                push    es
                push    ds
                pop     es
                assume es:dseg
                pop     ds
                assume ds:nothing
                mov     si, di
                mov     di, 1E8Ch
                lodsb
                cbw
                xchg    ax, cx

loc_11C28:                              ; CODE XREF: sub_11B9E+91↓j
                lodsb
                inc     al
                jz      short loc_11C2E
                dec     ax

loc_11C2E:                              ; CODE XREF: sub_11B9E+8D↑j
                stosb
                loop    loc_11C28
                push    ss
                pop     ds
                assume ds:dseg

loc_11C33:                              ; CODE XREF: sub_11B9E+5D↑j
                                        ; sub_11B9E+67↑j ...
                mov     bx, 4

loc_11C36:                              ; CODE XREF: sub_11B9E+AF↓j
                and     byte_16ABC[bx], 0BFh
                mov     ax, 4400h
                int     21h             ; DOS - 2+ - IOCTL - GET DEVICE INFORMATION
                                        ; BX = file or device handle
                jb      short loc_11C4C
                test    dl, 80h
                jz      short loc_11C4C
                or      byte ptr [bx+1E8Ch], 40h

loc_11C4C:                              ; CODE XREF: sub_11B9E+A2↑j
                                        ; sub_11B9E+A7↑j
                dec     bx
                jns     short loc_11C36
                mov     si, 215Ch
                mov     di, 215Ch
                call    sub_11CFA
                mov     si, 215Ch
                mov     di, 215Ch
                call    sub_11CEB
                retn
sub_11B9E       endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; Attributes: library function noreturn bp-based frame

; void __cdecl exit(int)
_exit           proc near               ; CODE XREF: error+8↑p
                                        ; start+91↑p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     si, 2302h
                mov     di, 2302h
                call    sub_11CEB
                mov     si, 215Ch
                mov     di, 215Eh
                call    sub_11CEB
                jmp     short loc_11C7C
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp

loc_11C7C:                              ; CODE XREF: _exit+15↑j
                mov     si, 215Eh
                mov     di, 215Eh
                call    sub_11CEB
                mov     si, 215Eh
                mov     di, 215Eh
                call    sub_11CFA
                call    __nullcheck
                or      ax, ax
                jz      short loc_11CA0
                cmp     [bp+arg_0], 0
                jnz     short loc_11CA0
                mov     [bp+arg_0], 0FFh

loc_11CA0:                              ; CODE XREF: _exit+31↑j
                                        ; _exit+37↑j
                mov     cx, 0Fh
                mov     bx, 5

loc_11CA6:                              ; CODE XREF: _exit+50↓j
                test    byte_16ABC[bx], 1
                jz      short loc_11CB1
                mov     ah, 3Eh
                int     21h             ; DOS - 2+ - CLOSE A FILE WITH HANDLE
                                        ; BX = file handle

loc_11CB1:                              ; CODE XREF: _exit+49↑j
                inc     bx
                loop    loc_11CA6
                call    __ctermsub
                mov     ax, [bp+arg_0]
                mov     ah, 4Ch
                int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
_exit           endp                    ; AL = exit code


; =============== S U B R O U T I N E =======================================

; Attributes: info_from_lumina

__ctermsub      proc near               ; CODE XREF: _exit+52↑p
                                        ; __doexec:loc_1380A↓p
                mov     cx, word_16D82
                jcxz    short loc_11CCB
                mov     bx, 2
                call    dword ptr unk_16D80

loc_11CCB:                              ; CODE XREF: __ctermsub+4↑j
                push    ds
                lds     dx, dword_16AA1
                assume ds:nothing
                mov     ax, 2500h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                pop     ds
                assume ds:dseg
                cmp     byte_16ADE, 0
                jz      short locret_11CEA
                push    ds
                mov     al, byte_16ADF
                lds     dx, dword_16AE0
                assume ds:nothing
                mov     ah, 25h
                int     21h             ; DOS - SET INTERRUPT VECTOR
                                        ; AL = interrupt number
                                        ; DS:DX = new vector to be used for specified interrupt
                pop     ds
                assume ds:dseg

locret_11CEA:                           ; CODE XREF: __ctermsub+1D↑j
                retn
__ctermsub      endp


; =============== S U B R O U T I N E =======================================


sub_11CEB       proc near               ; CODE XREF: sub_11B9E+C0↑p
                                        ; _exit+9↑p ...
                cmp     si, di
                jnb     short locret_11CF9
                dec     di
                dec     di
                mov     cx, [di]
                jcxz    short sub_11CEB
                call    cx
                jmp     short sub_11CEB
; ---------------------------------------------------------------------------

locret_11CF9:                           ; CODE XREF: sub_11CEB+2↑j
                retn
sub_11CEB       endp


; =============== S U B R O U T I N E =======================================


sub_11CFA       proc near               ; CODE XREF: sub_11B9E+B7↑p
                                        ; _exit+29↑p ...
                cmp     si, di
                jnb     short locret_11D0C
                sub     di, 4
                mov     ax, [di]
                or      ax, [di+2]
                jz      short sub_11CFA
                call    dword ptr [di]
                jmp     short sub_11CFA
; ---------------------------------------------------------------------------

locret_11D0C:                           ; CODE XREF: sub_11CFA+2↑j
                retn
sub_11CFA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame info_from_lumina

__FF_MSGBANNER  proc near               ; CODE XREF: start+28↑p
                                        ; sub_11D34-1A4↑p ...
                push    bp
                mov     bp, sp
                mov     ax, 0FCh
                push    ax
                call    sub_11F93
                cmp     word_16AE4, 0
                jz      short loc_11D23
                call    word_16AE4

loc_11D23:                              ; CODE XREF: __FF_MSGBANNER+F↑j
                mov     ax, 0FFh
                push    ax
                call    sub_11F93
                mov     sp, bp
                pop     bp
                retn
__FF_MSGBANNER  endp

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_11B9E

loc_11D2E:                              ; CODE XREF: sub_11B9E+40↑j
                mov     ax, 2
                jmp     loc_11B8F
; END OF FUNCTION CHUNK FOR sub_11B9E

; =============== S U B R O U T I N E =======================================


sub_11D34       proc near               ; CODE XREF: sub_122B4+6↓p
                                        ; sub_13A14+6↓p

; FUNCTION CHUNK AT 222F SIZE 0000000F BYTES

                pop     cx
                mov     bx, sp
                sub     bx, ax
                jb      short loc_11D45
                cmp     bx, word_16AE6
                jb      short loc_11D45
                mov     sp, bx
                jmp     cx
; ---------------------------------------------------------------------------

loc_11D45:                              ; CODE XREF: sub_11D34+5↑j
                                        ; sub_11D34+B↑j
                xor     ax, ax
                jmp     loc_11B8F
sub_11D34       endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; Attributes: info_from_lumina

__nullcheck     proc near               ; CODE XREF: _exit+2C↑p
                push    si
                xor     si, si
                mov     cx, 42h ; 'B'
                xor     ah, ah
                cld

loc_11D53:                              ; CODE XREF: __nullcheck+C↓j
                lodsb
                xor     ah, al
                loop    loc_11D53
                xor     ah, 55h
                jz      short loc_11D6A
                call    __FF_MSGBANNER
                mov     ax, 1
                push    ax
                call    sub_11F93
                mov     ax, 1

loc_11D6A:                              ; CODE XREF: __nullcheck+11↑j
                pop     si
                retn
__nullcheck     endp

; ---------------------------------------------------------------------------

loc_11D6C:                              ; CODE XREF: start+7C↑p
                pop     word_16AE8
                mov     dx, 2
                cmp     byte ptr word_16AB5, dl
                jz      short loc_11DA2
                mov     es, word_16AB3
                assume es:nothing
                mov     es, word ptr es:2Ch
                mov     seg_16AD8, es
                xor     ax, ax
                cwd
                mov     cx, 8000h
                xor     di, di

loc_11D8E:                              ; CODE XREF: sg0138:2431↓j
                repne scasb
                scasb
                jnz     short loc_11D8E
                inc     di
                inc     di
                mov     word_16AD6, di
                mov     cx, 0FFFFh
                repne scasb
                not     cx
                mov     dx, cx

loc_11DA2:                              ; CODE XREF: sg0138:2417↑j
                mov     di, 1
                mov     si, 81h
                mov     ds, word_16AB3
                assume ds:nothing

loc_11DAC:                              ; CODE XREF: sg0138:244F↓j
                                        ; sg0138:2453↓j ...
                lodsb
                cmp     al, 20h ; ' '
                jz      short loc_11DAC
                cmp     al, 9
                jz      short loc_11DAC
                cmp     al, 0Dh
                jz      short loc_11E28
                or      al, al
                jz      short loc_11E28
                inc     di

loc_11DBE:                              ; CODE XREF: sg0138:2489↓j
                dec     si

loc_11DBF:                              ; CODE XREF: sg0138:2479↓j
                                        ; sg0138:2493↓j ...
                lodsb
                cmp     al, 20h ; ' '
                jz      short loc_11DAC
                cmp     al, 9
                jz      short loc_11DAC
                cmp     al, 0Dh
                jz      short loc_11E28
                or      al, al
                jz      short loc_11E28
                cmp     al, 22h ; '"'
                jz      short loc_11DF8
                cmp     al, 5Ch ; '\'
                jz      short loc_11DDB
                inc     dx
                jmp     short loc_11DBF
; ---------------------------------------------------------------------------

loc_11DDB:                              ; CODE XREF: sg0138:2476↑j
                xor     cx, cx

loc_11DDD:                              ; CODE XREF: sg0138:2481↓j
                inc     cx
                lodsb
                cmp     al, 5Ch ; '\'
                jz      short loc_11DDD
                cmp     al, 22h ; '"'
                jz      short loc_11DEB
                add     dx, cx
                jmp     short loc_11DBE
; ---------------------------------------------------------------------------

loc_11DEB:                              ; CODE XREF: sg0138:2485↑j
                mov     ax, cx
                shr     cx, 1
                adc     dx, cx
                test    al, 1
                jnz     short loc_11DBF
                jmp     short loc_11DF8
; ---------------------------------------------------------------------------

loc_11DF7:                              ; CODE XREF: sg0138:24BA↓j
                dec     si

loc_11DF8:                              ; CODE XREF: sg0138:2472↑j
                                        ; sg0138:2495↑j ...
                lodsb
                cmp     al, 0Dh
                jz      short loc_11E28
                or      al, al
                jz      short loc_11E28
                cmp     al, 22h ; '"'
                jz      short loc_11DBF
                cmp     al, 5Ch ; '\'
                jz      short loc_11E0C
                inc     dx
                jmp     short loc_11DF8
; ---------------------------------------------------------------------------

loc_11E0C:                              ; CODE XREF: sg0138:24A7↑j
                xor     cx, cx

loc_11E0E:                              ; CODE XREF: sg0138:24B2↓j
                inc     cx
                lodsb
                cmp     al, 5Ch ; '\'
                jz      short loc_11E0E
                cmp     al, 22h ; '"'
                jz      short loc_11E1C
                add     dx, cx
                jmp     short loc_11DF7
; ---------------------------------------------------------------------------

loc_11E1C:                              ; CODE XREF: sg0138:24B6↑j
                mov     ax, cx
                shr     cx, 1
                adc     dx, cx
                test    al, 1
                jnz     short loc_11DF8
                jmp     short loc_11DBF
; ---------------------------------------------------------------------------

loc_11E28:                              ; CODE XREF: sg0138:2457↑j
                                        ; sg0138:245B↑j ...
                push    ss
                pop     ds
                assume ds:dseg
                mov     argc, di
                add     dx, di
                inc     di
                shl     di, 1
                add     dx, di
                and     dl, 0FEh
                sub     sp, dx
                mov     ax, sp
                mov     argv, ax
                mov     bx, ax
                add     di, bx
                push    ss
                pop     es
                assume es:dseg
                mov     ss:[bx], di
                inc     bx
                inc     bx
                lds     si, dword ptr word_16AD6
                assume ds:nothing

loc_11E4E:                              ; CODE XREF: sg0138:24F2↓j
                lodsb
                stosb
                or      al, al
                jnz     short loc_11E4E
                mov     si, 81h
                mov     ds, ss:word_16AB3
                jmp     short loc_11E61
; ---------------------------------------------------------------------------

loc_11E5E:                              ; CODE XREF: sg0138:2521↓j
                                        ; sg0138:2525↓j
                xor     ax, ax
                stosb

loc_11E61:                              ; CODE XREF: sg0138:24FC↑j
                                        ; sg0138:2504↓j ...
                lodsb
                cmp     al, 20h ; ' '
                jz      short loc_11E61
                cmp     al, 9
                jz      short loc_11E61
                cmp     al, 0Dh
                jnz     short loc_11E71
                jmp     loc_11EF0
; ---------------------------------------------------------------------------

loc_11E71:                              ; CODE XREF: sg0138:250C↑j
                or      al, al
                jnz     short loc_11E78
                jmp     short loc_11EF0
; ---------------------------------------------------------------------------
                align 2

loc_11E78:                              ; CODE XREF: sg0138:2513↑j
                mov     ss:[bx], di
                inc     bx
                inc     bx

loc_11E7D:                              ; CODE XREF: sg0138:254A↓j
                dec     si

loc_11E7E:                              ; CODE XREF: sg0138:2538↓j
                                        ; sg0138:2557↓j ...
                lodsb
                cmp     al, 20h ; ' '
                jz      short loc_11E5E
                cmp     al, 9
                jz      short loc_11E5E
                cmp     al, 0Dh
                jz      short loc_11EED
                or      al, al
                jz      short loc_11EED
                cmp     al, 22h ; '"'
                jz      short loc_11EBA
                cmp     al, 5Ch ; '\'
                jz      short loc_11E9A
                stosb
                jmp     short loc_11E7E
; ---------------------------------------------------------------------------

loc_11E9A:                              ; CODE XREF: sg0138:2535↑j
                xor     cx, cx

loc_11E9C:                              ; CODE XREF: sg0138:2540↓j
                inc     cx
                lodsb
                cmp     al, 5Ch ; '\'
                jz      short loc_11E9C
                cmp     al, 22h ; '"'
                jz      short loc_11EAC
                mov     al, 5Ch ; '\'
                rep stosb
                jmp     short loc_11E7D
; ---------------------------------------------------------------------------

loc_11EAC:                              ; CODE XREF: sg0138:2544↑j
                mov     al, 5Ch ; '\'
                shr     cx, 1
                rep stosb
                jnb     short loc_11EBA
                mov     al, 22h ; '"'
                stosb
                jmp     short loc_11E7E
; ---------------------------------------------------------------------------
                dec     si

loc_11EBA:                              ; CODE XREF: sg0138:2531↑j
                                        ; sg0138:2552↑j ...
                lodsb
                cmp     al, 0Dh
                jz      short loc_11EED
                or      al, al
                jz      short loc_11EED
                cmp     al, 22h ; '"'
                jz      short loc_11E7E
                cmp     al, 5Ch ; '\'
                jz      short loc_11ECE
                stosb
                jmp     short loc_11EBA
; ---------------------------------------------------------------------------

loc_11ECE:                              ; CODE XREF: sg0138:2569↑j
                xor     cx, cx

loc_11ED0:                              ; CODE XREF: sg0138:2574↓j
                inc     cx
                lodsb
                cmp     al, 5Ch ; '\'
                jz      short loc_11ED0
                cmp     al, 22h ; '"'
                jz      short loc_11EE0
                mov     al, 5Ch ; '\'
                rep stosb
; ---------------------------------------------------------------------------
jpt_12429       dw 0D9EBh               ; DATA XREF: sub_122B4+175↓r
                                        ; jump table for switch statement
; ---------------------------------------------------------------------------

loc_11EE0:                              ; CODE XREF: sg0138:2578↑j
                mov     al, 5Ch ; '\'
                shr     cx, 1
                rep stosb
                jnb     short loc_11E7E
                mov     al, 22h ; '"'
                stosb
                jmp     short loc_11EBA
; ---------------------------------------------------------------------------

loc_11EED:                              ; CODE XREF: sg0138:2529↑j
                                        ; sg0138:252D↑j ...
                xor     ax, ax
                stosb

loc_11EF0:                              ; CODE XREF: sg0138:250E↑j
                                        ; sg0138:2515↑j
                push    ss
                pop     ds
                assume ds:dseg
                mov     word ptr [bx], 0
                jmp     word_16AE8

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11EFA       proc near               ; CODE XREF: start+79↑p

var_s0          = word ptr  0

                push    bp
                mov     bp, sp
                push    bp
                mov     ds, word_16AB3
                assume ds:nothing
                xor     cx, cx
                mov     ax, cx
                mov     bp, cx
                mov     di, cx
                dec     cx
                mov     si, ds:2Ch
                or      si, si
                jz      short loc_11F23
                mov     es, si
                assume es:nothing
                cmp     byte ptr es:0, 0
                jz      short loc_11F23

loc_11F1D:                              ; CODE XREF: sub_11EFA+27↓j
                repne scasb
                inc     bp
                scasb
                jnz     short loc_11F1D

loc_11F23:                              ; CODE XREF: sub_11EFA+17↑j
                                        ; sub_11EFA+21↑j
                inc     bp
                xchg    ax, di
                inc     ax
                and     al, 0FEh
                mov     di, bp
                shl     bp, 1
                add     ax, bp
                push    ss
                pop     ds
                assume ds:dseg
                push    di
                mov     di, 9
                call    sub_11FBC
                pop     di
                mov     cx, di
                mov     di, bp
                add     di, ax
                mov     word_16AD4, bp
                push    ds
                pop     es
                assume es:dseg
                mov     ds, si
                assume ds:nothing
                xor     si, si
                dec     cx
                jcxz    short loc_11F5E

loc_11F4B:                              ; CODE XREF: sub_11EFA+62↓j
                cmp     word ptr [si], 433Bh
                jz      short loc_11F56
                mov     [bp+var_s0], di
                inc     bp
                inc     bp

loc_11F56:                              ; CODE XREF: sub_11EFA+55↑j
                                        ; sub_11EFA+60↓j
                lodsb
                stosb
                or      al, al
                jnz     short loc_11F56
                loop    loc_11F4B

loc_11F5E:                              ; CODE XREF: sub_11EFA+4F↑j
                mov     [bp+var_s0], cx
                push    ss
                pop     ds
                assume ds:dseg
                pop     bp
                mov     sp, bp
                pop     bp
                retn
sub_11EFA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11F68       proc near               ; CODE XREF: sub_11F93+7↓p
                                        ; __doexec+5D↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    ds
                pop     es
                mov     dx, [bp+arg_0]
                mov     si, 2168h

loc_11F75:                              ; CODE XREF: sub_11F68+20↓j
                lodsw
                cmp     ax, dx
                jz      short loc_11F8A
                inc     ax
                xchg    ax, si
                jz      short loc_11F8A
                xchg    ax, di
                xor     ax, ax
                mov     cx, 0FFFFh
                repne scasb
                mov     si, di
                jmp     short loc_11F75
; ---------------------------------------------------------------------------

loc_11F8A:                              ; CODE XREF: sub_11F68+10↑j
                                        ; sub_11F68+14↑j
                xchg    ax, si
                pop     di
                pop     si
                mov     sp, bp
                pop     bp
                retn    2
sub_11F68       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_11F93       proc near               ; CODE XREF: start+2E↑p
                                        ; sub_11D34-1A1↑p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    di
                push    [bp+arg_0]
                call    sub_11F68
                or      ax, ax
                jz      short loc_11FB5
                xchg    ax, dx
                mov     di, dx
                xor     ax, ax
                mov     cx, 0FFFFh
                repne scasb
                not     cx
                dec     cx
                mov     bx, 2
                mov     ah, 40h
                int     21h             ; DOS - 2+ - WRITE TO FILE WITH HANDLE
                                        ; BX = file handle, CX = number of bytes to write, DS:DX -> buffer

loc_11FB5:                              ; CODE XREF: sub_11F93+C↑j
                pop     di
                mov     sp, bp
                pop     bp
                retn    2
sub_11F93       endp


; =============== S U B R O U T I N E =======================================


sub_11FBC       proc near               ; CODE XREF: sub_11EFA+3A↑p

; FUNCTION CHUNK AT 222F SIZE 0000000F BYTES

                mov     dx, ax
                add     ax, word_16A42
                jb      short loc_11FF9
                cmp     word_16A3C, ax
                jnb     short loc_11FEF
                add     ax, 0Fh
                push    ax
                rcr     ax, 1
                mov     cl, 3
                shr     ax, cl
                mov     cx, ds
                mov     bx, word_16AB3
                sub     cx, bx
                add     ax, cx
                mov     es, bx
                assume es:nothing
                mov     bx, ax
                mov     ah, 4Ah
                int     21h             ; DOS - 2+ - ADJUST MEMORY BLOCK SIZE (SETBLOCK)
                                        ; ES = segment address of block to change
                                        ; BX = new size in paragraphs
                pop     ax
                jb      short loc_11FF9
                and     al, 0F0h
                dec     ax
                mov     word_16A3C, ax

loc_11FEF:                              ; CODE XREF: sub_11FBC+C↑j
                xchg    ax, bp
                mov     bp, word_16A42
                add     word_16A42, dx
                retn
; ---------------------------------------------------------------------------

loc_11FF9:                              ; CODE XREF: sub_11FBC+6↑j
                                        ; sub_11FBC+2B↑j
                mov     ax, di
                jmp     loc_11B8F
sub_11FBC       endp ; sp-analysis failed

; ---------------------------------------------------------------------------
; START OF FUNCTION CHUNK FOR sub_12B7C

loc_11FFE:                              ; CODE XREF: sub_12B7C:loc_12B99↓j
                jb      short loc_12013

loc_12000:                              ; CODE XREF: sg0138:26A6↓j
                xor     ax, ax
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_12B7C
; ---------------------------------------------------------------------------
                jnb     short loc_12000
                push    ax
                call    sub_12024
                pop     ax
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
; [0000000D BYTES: COLLAPSED CHUNK OF FUNCTION sub_12B9C. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
                xor     ah, ah
                call    sub_12024
                retn

; =============== S U B R O U T I N E =======================================


sub_12024       proc near               ; CODE XREF: sg0138:26A9↑p
                                        ; sub_12B9C:loc_12013↑p ...
                mov     byte ptr word_16AB8, al
                or      ah, ah
                jnz     short loc_1204E
                cmp     byte ptr word_16AB5, 3
                jb      short loc_1203F
                cmp     al, 22h ; '"'
                jnb     short loc_12043
                cmp     al, 20h ; ' '
                jb      short loc_1203F
                mov     al, 5
                jmp     short loc_12045
; ---------------------------------------------------------------------------
                db 90h
; ---------------------------------------------------------------------------

loc_1203F:                              ; CODE XREF: sub_12024+C↑j
                                        ; sub_12024+14↑j
                cmp     al, 13h
                jbe     short loc_12045

loc_12043:                              ; CODE XREF: sub_12024+10↑j
                mov     al, 13h

loc_12045:                              ; CODE XREF: sub_12024+18↑j
                                        ; sub_12024+1D↑j
                mov     bx, 1EBAh
                xlat

loc_12049:                              ; CODE XREF: sub_12024+2C↓j
                cbw
                mov     word_16AAD, ax
                retn
; ---------------------------------------------------------------------------

loc_1204E:                              ; CODE XREF: sub_12024+5↑j
                mov     al, ah
                jmp     short loc_12049
sub_12024       endp

; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     si, 1ED0h
                sub     di, di
                jmp     short loc_12076
; ---------------------------------------------------------------------------
                align 2

loc_12062:                              ; CODE XREF: sg0138:271A↓j
                test    byte ptr [si+6], 83h
                jz      short loc_12073
                push    si
                call    sub_12246
                add     sp, 2
                inc     ax
                jz      short loc_12073
                inc     di

loc_12073:                              ; CODE XREF: sg0138:2706↑j
                                        ; sg0138:2710↑j
                add     si, 8

loc_12076:                              ; CODE XREF: sg0138:26FF↑j
                cmp     word_16C18, si
                jnb     short loc_12062
                mov     ax, di
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12084       proc near               ; CODE XREF: sub_128AC+26↓p
                                        ; sub_128EA+20↓p ...

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
arg_0           = byte ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     si, [bp+arg_2]
                mov     al, [si+7]
                cbw
                mov     [bp+var_6], ax
                mov     ax, si
                sub     ax, 1ED0h
                mov     cl, 3
                sar     ax, cl
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, 1F70h
                mov     [bp+var_8], ax
                test    byte ptr [si+6], 83h
                jz      short loc_120B9
                test    byte ptr [si+6], 40h
                jz      short loc_120C4

loc_120B9:                              ; CODE XREF: sub_12084+2D↑j
                                        ; sub_12084+44↓j ...
                or      byte ptr [si+6], 20h
                mov     ax, 0FFFFh
                jmp     loc_121D3
; ---------------------------------------------------------------------------
                align 2

loc_120C4:                              ; CODE XREF: sub_12084+33↑j
                test    byte ptr [si+6], 1
                jnz     short loc_120B9
                or      byte ptr [si+6], 2
                and     byte ptr [si+6], 0EFh
                sub     ax, ax
                mov     [si+2], ax
                mov     di, ax
                mov     [bp+var_4], di
                test    byte ptr [si+6], 0Ch
                jnz     short loc_12141
                mov     bx, si
                sub     bx, 1ED0h
                mov     cl, 3
                sar     bx, cl
                mov     ax, bx
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                test    byte ptr [bx+1F70h], 1
                jnz     short loc_12141
                cmp     si, 1ED8h
                jz      short loc_12107
                cmp     si, 1EE0h
                jnz     short loc_1213A

loc_12107:                              ; CODE XREF: sub_12084+7B↑j
                push    [bp+var_6]      ; int
                call    _isatty
                add     sp, 2
                or      ax, ax
                jnz     short loc_12141
                inc     word_16AFE
                cmp     si, 1ED8h
                jnz     short loc_12124
                mov     ax, 33E0h
                jmp     short loc_12127
; ---------------------------------------------------------------------------
                align 2

loc_12124:                              ; CODE XREF: sub_12084+98↑j
                mov     ax, 3E68h

loc_12127:                              ; CODE XREF: sub_12084+9D↑j
                mov     [si+4], ax
                mov     [si], ax
                mov     bx, [bp+var_8]
                mov     word ptr [bx+2], 200h
                mov     byte ptr [bx], 1
                jmp     short loc_12141
; ---------------------------------------------------------------------------
                align 2

loc_1213A:                              ; CODE XREF: sub_12084+81↑j
                push    si
                call    sub_121DA
                add     sp, 2

loc_12141:                              ; CODE XREF: sub_12084+5C↑j
                                        ; sub_12084+75↑j ...
                test    byte ptr [si+6], 8
                jnz     short loc_12160
                mov     bx, si
                sub     bx, 1ED0h
                mov     cl, 3
                sar     bx, cl
                mov     ax, bx
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                test    byte ptr [bx+1F70h], 1
                jz      short loc_121B0

loc_12160:                              ; CODE XREF: sub_12084+C1↑j
                mov     di, [si]
                sub     di, [si+4]
                mov     ax, [si+4]
                inc     ax
                mov     [si], ax
                mov     bx, [bp+var_8]
                mov     ax, [bx+2]
                dec     ax
                mov     [si+2], ax
                or      di, di
                jle     short loc_1218C
                push    di
                push    word ptr [si+4]
                push    [bp+var_6]
                call    sub_12E98
                add     sp, 6
                mov     [bp+var_4], ax
                jmp     short loc_121A5
; ---------------------------------------------------------------------------
                align 2

loc_1218C:                              ; CODE XREF: sub_12084+F3↑j
                mov     bx, [bp+var_6]
                test    byte ptr [bx+1E8Ch], 20h
                jz      short loc_121A5
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                push    bx
                call    sub_12B9C
                add     sp, 8

loc_121A5:                              ; CODE XREF: sub_12084+105↑j
                                        ; sub_12084+110↑j
                mov     bx, [si+4]
                mov     al, [bp+arg_0]
                mov     [bx], al
                jmp     short loc_121C6
; ---------------------------------------------------------------------------
                align 2

loc_121B0:                              ; CODE XREF: sub_12084+DA↑j
                mov     di, 1
                mov     ax, di
                push    ax
                lea     ax, [bp+arg_0]
                push    ax
                push    [bp+var_6]
                call    sub_12E98
                add     sp, 6
                mov     [bp+var_4], ax

loc_121C6:                              ; CODE XREF: sub_12084+129↑j
                cmp     [bp+var_4], di
                jz      short loc_121CE
                jmp     loc_120B9
; ---------------------------------------------------------------------------

loc_121CE:                              ; CODE XREF: sub_12084+145↑j
                mov     al, [bp+arg_0]
                sub     ah, ah

loc_121D3:                              ; CODE XREF: sub_12084+3C↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_12084       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_121DA       proc near               ; CODE XREF: sub_12084+B7↑p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     ax, [bp+arg_0]
                sub     ax, 1ED0h
                mov     cl, 3
                sar     ax, cl
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, 1F70h
                mov     [bp+var_2], ax
                mov     ax, 200h
                push    ax
                call    sub_12FE4
                add     sp, 2
                mov     bx, [bp+arg_0]
                mov     [bx+4], ax
                or      ax, ax
                jz      short loc_1221C
                or      byte ptr [bx+6], 8
                mov     bx, [bp+var_2]
                mov     word ptr [bx+2], 200h
                jmp     short loc_12232
; ---------------------------------------------------------------------------
                align 2

loc_1221C:                              ; CODE XREF: sub_121DA+31↑j
                mov     bx, [bp+arg_0]
                or      byte ptr [bx+6], 4
                mov     ax, [bp+var_2]
                inc     ax
                mov     [bx+4], ax
                mov     bx, [bp+var_2]
                mov     word ptr [bx+2], 1

loc_12232:                              ; CODE XREF: sub_121DA+3F↑j
                mov     bx, [bp+arg_0]
                mov     si, bx
                mov     ax, [si+4]
                mov     [bx], ax
                mov     word ptr [bx+2], 0
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_121DA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12246       proc near               ; CODE XREF: sg0138:2709↑p

var_4           = word ptr -4
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     si, [bp+arg_0]
                sub     di, di
                mov     al, [si+6]
                and     al, 3
                cmp     al, 2
                jnz     short loc_122A2
                test    byte ptr [si+6], 8
                jnz     short loc_1227B
                mov     bx, si
                sub     bx, 1ED0h
                mov     cl, 3
                sar     bx, cl
                mov     ax, bx
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                test    byte ptr [bx+1F70h], 1
                jz      short loc_122A2

loc_1227B:                              ; CODE XREF: sub_12246+1A↑j
                mov     ax, [si]
                sub     ax, [si+4]
                mov     [bp+var_4], ax
                or      ax, ax
                jle     short loc_122A2
                push    ax
                push    word ptr [si+4]
                mov     al, [si+7]
                cbw
                push    ax
                call    sub_12E98
                add     sp, 6
                cmp     ax, [bp+var_4]
                jz      short loc_122A2
                or      byte ptr [si+6], 20h
                mov     di, 0FFFFh

loc_122A2:                              ; CODE XREF: sub_12246+14↑j
                                        ; sub_12246+33↑j ...
                mov     ax, [si+4]
                mov     [si], ax
                mov     word ptr [si+2], 0
                mov     ax, di
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_12246       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_122B4       proc near               ; CODE XREF: _sprintf+2C↓p

var_164         = word ptr -164h
var_162         = byte ptr -162h
var_4           = word ptr -4
arg_0           = word ptr  4
arg_2           = word ptr  6
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                mov     ax, 164h
                call    sub_11D34
                push    di
                push    si
                mov     si, [bp+arg_2]
                lea     ax, [bp+var_162]
                mov     word_16F2A, ax
                mov     ax, [bp+arg_4]
                mov     word_16F1A, ax
                mov     ax, [bp+arg_0]
                mov     word_16F0E, ax
                mov     word_16F24, 0
                mov     word_16F22, 0
                jmp     loc_12560
; ---------------------------------------------------------------------------

loc_122E4:                              ; CODE XREF: sub_122B4+2B1↓j
                cmp     byte ptr [si], 25h ; '%'
                jz      short loc_122EC
                jmp     loc_12544
; ---------------------------------------------------------------------------

loc_122EC:                              ; CODE XREF: sub_122B4+33↑j
                mov     t, 1
                sub     ax, ax
                mov     word_16F16, ax
                mov     word_16F12, ax
                mov     word_16F20, ax
                mov     word_16F14, ax
                mov     word_16F1E, ax
                mov     word_16F1C, ax
                mov     word_16F10, ax
                mov     word_16F0C, ax
                mov     word_16F18, ax
                mov     word_16F30, 20h ; ' '
                cmp     byte ptr [si+1], 30h ; '0'
                jnz     short loc_12357
                inc     si
                mov     word_16F30, 30h ; '0'
                jmp     short loc_12357
; ---------------------------------------------------------------------------

loc_12324:                              ; CODE XREF: sub_122B4+9D↓j
                cmp     byte ptr [si], 2Bh ; '+'
                jnz     short loc_12336
                inc     word_16F16
                mov     word_16F1C, 0
                jmp     short loc_12357
; ---------------------------------------------------------------------------
                align 2

loc_12336:                              ; CODE XREF: sub_122B4+73↑j
                cmp     byte ptr [si], 20h ; ' '
                jnz     short loc_12348
                cmp     word_16F16, 0
                jnz     short loc_12357
                inc     word_16F1C
                jmp     short loc_12357
; ---------------------------------------------------------------------------

loc_12348:                              ; CODE XREF: sub_122B4+85↑j
                inc     word_16F0C
                jmp     short loc_12357
; ---------------------------------------------------------------------------

loc_1234E:                              ; CODE XREF: sub_122B4+B0↓j
                cmp     byte ptr [si], 2Dh ; '-'
                jnz     short loc_12324
                inc     word_16F18

loc_12357:                              ; CODE XREF: sub_122B4+65↑j
                                        ; sub_122B4+6E↑j ...
                inc     si
                mov     al, [si]
                cbw
                push    ax
                call    sub_12B54
                add     sp, 2
                or      ax, ax
                jnz     short loc_1234E
                push    si
                mov     ax, 22FCh
                push    ax
                call    sub_12AD4
                add     sp, 4
                mov     si, ax
                cmp     word_16F2C, 0
                jge     short loc_12386
                inc     word_16F18
                mov     ax, word_16F2C
                neg     ax
                mov     word_16F2C, ax

loc_12386:                              ; CODE XREF: sub_122B4+C4↑j
                cmp     byte ptr [si], 2Eh ; '.'
                jnz     short loc_123AE
                inc     word_16F1E
                inc     si
                push    si
                mov     ax, 22F6h
                push    ax
                call    sub_12AD4
                add     sp, 4
                mov     si, ax
                cmp     t, 0
                jge     short loc_123AE
                mov     t, 1
                dec     word_16F1E

loc_123AE:                              ; CODE XREF: sub_122B4+D5↑j
                                        ; sub_122B4+EE↑j
                mov     al, [si]
                cbw
                cmp     ax, 46h ; 'F'
                jz      short loc_123E8
                cmp     ax, 4Eh ; 'N'
                jz      short loc_123F0
                cmp     ax, 68h ; 'h'
                jz      short loc_123E0
                cmp     ax, 6Ch ; 'l'
                jnz     short loc_123CB
                mov     word_16F14, 2

loc_123CB:                              ; CODE XREF: sub_122B4+10F↑j
                                        ; sub_122B4+132↓j ...
                cmp     word_16F14, 0
                jnz     short loc_123D7
                cmp     byte ptr [si], 4Ch ; 'L'
                jnz     short loc_123D8

loc_123D7:                              ; CODE XREF: sub_122B4+11C↑j
                inc     si

loc_123D8:                              ; CODE XREF: sub_122B4+121↑j
                cmp     byte ptr [si], 0
                jnz     short loc_123F8
                jmp     loc_12568
; ---------------------------------------------------------------------------

loc_123E0:                              ; CODE XREF: sub_122B4+10A↑j
                mov     word_16F14, 1
                jmp     short loc_123CB
; ---------------------------------------------------------------------------

loc_123E8:                              ; CODE XREF: sub_122B4+100↑j
                mov     word_16F14, 10h
                jmp     short loc_123CB
; ---------------------------------------------------------------------------

loc_123F0:                              ; CODE XREF: sub_122B4+105↑j
                mov     word_16F14, 8
                jmp     short loc_123CB
; ---------------------------------------------------------------------------

loc_123F8:                              ; CODE XREF: sub_122B4+127↑j
                mov     al, [si]
                cbw
                mov     [bp+var_164], ax
                cmp     ax, 45h ; 'E'
                jz      short loc_1240E
                cmp     ax, 47h ; 'G'
                jz      short loc_1240E
                cmp     ax, 58h ; 'X'
                jnz     short loc_12417

loc_1240E:                              ; CODE XREF: sub_122B4+14E↑j
                                        ; sub_122B4+153↑j
                inc     word_16F12
                add     [bp+var_164], 20h ; ' '

loc_12417:                              ; CODE XREF: sub_122B4+158↑j
                mov     ax, [bp+var_164]
                sub     ax, 63h ; 'c'   ; switch with an invalid jump table
                cmp     ax, 15h
                jbe     short loc_12426
                jmp     def_12429       ; jumptable 00012429 default case
; ---------------------------------------------------------------------------

loc_12426:                              ; CODE XREF: sub_122B4+16D↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_12429[bx] ; switch jump
; ---------------------------------------------------------------------------
                mov     bx, word_16F1A
                mov     bx, [bx]
                mov     ax, word_16F22
                mov     [bx], ax

loc_12439:                              ; CODE XREF: sub_122B4+263↓j
                add     word_16F1A, 2
                jmp     loc_125AA
; ---------------------------------------------------------------------------
                align 2
                inc     word_16F20
                mov     word_16F0C, 0
                mov     ax, 0Ah

loc_1244F:                              ; CODE XREF: sub_122B4+1A9↓j
                                        ; sub_122B4+269↓j
                push    ax
                call    sub_125D2

loc_12453:                              ; CODE XREF: sub_122B4+272↓j
                                        ; sub_122B4+283↓j
                add     sp, 2
                jmp     loc_125AA
; ---------------------------------------------------------------------------
                align 2
                mov     ax, 8
                jmp     short loc_1244F
; ---------------------------------------------------------------------------
                align 2
                inc     word_16F10
                inc     word_16F12
                cmp     word_16F1E, 0
                jnz     short loc_12478
                mov     word_16F28, 1
                jmp     short loc_1247E
; ---------------------------------------------------------------------------
                align 2

loc_12478:                              ; CODE XREF: sub_122B4+1B9↑j
                mov     word_16F28, 0

loc_1247E:                              ; CODE XREF: sub_122B4+1C1↑j
                inc     word_16F1E
                mov     t, 4
                cmp     word_16F14, 8
                jnz     short loc_12492
                jmp     loc_1251A
; ---------------------------------------------------------------------------

loc_12492:                              ; CODE XREF: sub_122B4+1D9↑j
                sub     ax, ax
                mov     word_16F14, ax
                mov     [bp+var_4], ax
                cmp     word_16F2C, ax
                jz      short loc_124C7
                mov     ax, word_16F2C
                mov     [bp+var_4], ax
                cmp     word_16F18, 0
                jz      short loc_124B6
                mov     word_16F2C, 0
                jmp     short loc_124C7
; ---------------------------------------------------------------------------
                align 2

loc_124B6:                              ; CODE XREF: sub_122B4+1F7↑j
                sub     word_16F2C, 5
                mov     ax, word_16F2C
                or      ax, ax
                jge     short loc_124C4
                sub     ax, ax

loc_124C4:                              ; CODE XREF: sub_122B4+20C↑j
                mov     word_16F2C, ax

loc_124C7:                              ; CODE XREF: sub_122B4+1EA↑j
                                        ; sub_122B4+1FF↑j
                add     word_16F1A, 2
                mov     ax, 10h
                push    ax
                call    sub_125D2
                add     sp, 2
                mov     ax, 3Ah ; ':'
                push    ax
                call    sub_128AC
                add     sp, 2
                cmp     [bp+var_4], 0
                jz      short loc_12508
                cmp     word_16F18, 0
                jz      short loc_12502
                mov     ax, [bp+var_4]
                sub     ax, 5
                mov     word_16F2C, ax
                or      ax, ax
                jge     short loc_124FC
                sub     ax, ax

loc_124FC:                              ; CODE XREF: sub_122B4+244↑j
                mov     word_16F2C, ax
                jmp     short loc_12508
; ---------------------------------------------------------------------------
                align 2

loc_12502:                              ; CODE XREF: sub_122B4+237↑j
                mov     word_16F2C, 0

loc_12508:                              ; CODE XREF: sub_122B4+230↑j
                                        ; sub_122B4+24B↑j
                sub     word_16F1A, 4
                mov     ax, 10h
                push    ax
                call    sub_125D2
                add     sp, 2
                jmp     loc_12439
; ---------------------------------------------------------------------------

loc_1251A:                              ; CODE XREF: sub_122B4+1DB↑j
                mov     ax, 10h
                jmp     loc_1244F
; ---------------------------------------------------------------------------
                sub     ax, ax

loc_12522:                              ; CODE XREF: sub_122B4+279↓j
                push    ax
                call    sub_12708
                jmp     loc_12453
; ---------------------------------------------------------------------------
                align 2
                mov     ax, 1
                jmp     short loc_12522
; ---------------------------------------------------------------------------
                align 2
                push    [bp+var_164]
                call    sub_127F0
                jmp     loc_12453
; ---------------------------------------------------------------------------

def_12429:                              ; CODE XREF: sub_122B4+16F↑j
                cmp     word_16F14, 0   ; jumptable 00012429 default case
                jz      short loc_12544
                mov     ax, si
                dec     si

loc_12544:                              ; CODE XREF: sub_122B4+35↑j
                                        ; sub_122B4+28B↑j
                mov     di, si

loc_12546:                              ; CODE XREF: sub_122B4+29B↓j
                inc     di
                cmp     byte ptr [di], 0
                jz      short loc_12551
                cmp     byte ptr [di], 25h ; '%'
                jnz     short loc_12546

loc_12551:                              ; CODE XREF: sub_122B4+296↑j
                mov     ax, di
                sub     ax, si
                push    ax
                push    ds
                push    si
                call    sub_12948
                add     sp, 6
                mov     si, di

loc_12560:                              ; CODE XREF: sub_122B4+2D↑j
                                        ; sub_122B4+311↓j
                cmp     byte ptr [si], 0
                jz      short loc_12568
                jmp     loc_122E4
; ---------------------------------------------------------------------------

loc_12568:                              ; CODE XREF: sub_122B4+129↑j
                                        ; sub_122B4+2AF↑j
                cmp     word_16F22, 0
                jnz     short loc_125C8
                mov     bx, word_16F0E
                test    byte ptr [bx+6], 20h
                jz      short loc_125C8

loc_12579:                              ; CODE XREF: sub_122B4+30C↓j
                mov     ax, 0FFFFh
                jmp     short loc_125CB
; ---------------------------------------------------------------------------
                sub     ah, [di]
                inc     si
                and     al, 30h
                and     ax, 2530h
                xor     [di], ah
                cmp     ah, [di]
                inc     si
                and     al, 3Ah
                and     ax, 253Ah
                cmp     ah, [di]
                cmp     ah, [di]
                db      2Eh
                and     al, 5Ah
                and     al, 60h
                and     al, 3Ah
                and     ax, 253Ah
                and     [di], ah
                cmp     ah, [di]
                inc     dx
                and     al, 3Ah
                and     ax, 253Ah
                sbb     ah, [di]

loc_125AA:                              ; CODE XREF: sub_122B4+18A↑j
                                        ; sub_122B4+1A2↑j
                cmp     word_16F24, 0
                jz      short loc_125C4
                cmp     word_16F22, 0
                jnz     short loc_125C8
                mov     bx, word_16F0E
                test    byte ptr [bx+6], 20h
                jnz     short loc_12579
                jmp     short loc_125C8
; ---------------------------------------------------------------------------

loc_125C4:                              ; CODE XREF: sub_122B4+2FB↑j
                inc     si
                jmp     short loc_12560
; ---------------------------------------------------------------------------
                align 2

loc_125C8:                              ; CODE XREF: sub_122B4+2B9↑j
                                        ; sub_122B4+2C3↑j ...
                mov     ax, word_16F22

loc_125CB:                              ; CODE XREF: sub_122B4+2C8↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_122B4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_125D2       proc near               ; CODE XREF: sub_122B4+19C↑p
                                        ; sub_122B4+21C↑p ...

var_18          = byte ptr -18h
var_C           = word ptr -0Ch
var_8           = word ptr -8
var_4           = dword ptr -4
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 18h
                push    di
                push    si              ; int
                cmp     [bp+arg_0], 0Ah
                jz      short loc_125E4
                inc     word_16F20

loc_125E4:                              ; CODE XREF: sub_125D2+C↑j
                cmp     word_16F14, 2
                jz      short loc_125F2
                cmp     word_16F14, 10h
                jnz     short loc_12608

loc_125F2:                              ; CODE XREF: sub_125D2+17↑j
                mov     bx, word_16F1A
                mov     ax, [bx]
                mov     dx, [bx+2]
                mov     word ptr [bp+var_4], ax
                mov     word ptr [bp+var_4+2], dx
                add     word_16F1A, 4
                jmp     short loc_12632
; ---------------------------------------------------------------------------

loc_12608:                              ; CODE XREF: sub_125D2+1E↑j
                cmp     word_16F20, 0
                jz      short loc_12620
                mov     bx, word_16F1A
                mov     ax, [bx]
                mov     word ptr [bp+var_4], ax
                mov     word ptr [bp+var_4+2], 0
                jmp     short loc_1262D
; ---------------------------------------------------------------------------
                align 2

loc_12620:                              ; CODE XREF: sub_125D2+3B↑j
                mov     bx, word_16F1A
                mov     ax, [bx]
                cwd
                mov     word ptr [bp+var_4], ax
                mov     word ptr [bp+var_4+2], dx

loc_1262D:                              ; CODE XREF: sub_125D2+4B↑j
                add     word_16F1A, 2

loc_12632:                              ; CODE XREF: sub_125D2+34↑j
                cmp     word_16F0C, 0
                jz      short loc_12646
                mov     ax, word ptr [bp+var_4]
                or      ax, word ptr [bp+var_4+2]
                jz      short loc_12646
                mov     ax, [bp+arg_0]
                jmp     short loc_12648
; ---------------------------------------------------------------------------

loc_12646:                              ; CODE XREF: sub_125D2+65↑j
                                        ; sub_125D2+6D↑j
                sub     ax, ax

loc_12648:                              ; CODE XREF: sub_125D2+72↑j
                mov     word_16F2E, ax
                mov     si, word_16F2A
                cmp     word_16F20, 0
                jnz     short loc_12680
                cmp     word ptr [bp+var_4+2], 0
                jge     short loc_12680
                cmp     [bp+arg_0], 0Ah
                jnz     short loc_12679
                mov     byte ptr [si], 2Dh ; '-'
                inc     si
                mov     ax, word ptr [bp+var_4]
                mov     dx, word ptr [bp+var_4+2]
                neg     ax
                adc     dx, 0
                neg     dx
                mov     word ptr [bp+var_4], ax
                mov     word ptr [bp+var_4+2], dx

loc_12679:                              ; CODE XREF: sub_125D2+8E↑j
                mov     [bp+var_8], 1
                jmp     short loc_12685
; ---------------------------------------------------------------------------

loc_12680:                              ; CODE XREF: sub_125D2+82↑j
                                        ; sub_125D2+88↑j
                mov     [bp+var_8], 0

loc_12685:                              ; CODE XREF: sub_125D2+AC↑j
                lea     ax, [bp+var_18]
                mov     di, ax
                push    [bp+arg_0]
                push    di              ; char *
                push    word ptr [bp+var_4+2]
                push    word ptr [bp+var_4] ; unsigned __int32
                call    _ultoa
                add     sp, 8
                cmp     word_16F1E, 0
                jz      short loc_126C2
                push    di
                call    sub_132C2
                add     sp, 2
                mov     cx, t
                sub     cx, ax
                mov     [bp+var_C], cx
                jmp     short loc_126B8
; ---------------------------------------------------------------------------
                align 2

loc_126B4:                              ; CODE XREF: sub_125D2+EB↓j
                mov     byte ptr [si], 30h ; '0'
                inc     si

loc_126B8:                              ; CODE XREF: sub_125D2+DF↑j
                mov     ax, cx
                dec     cx
                or      ax, ax
                jg      short loc_126B4
                mov     [bp+var_C], cx

loc_126C2:                              ; CODE XREF: sub_125D2+CD↑j
                mov     cx, word_16F12

loc_126C6:                              ; CODE XREF: sub_125D2+109↓j
                mov     al, [di]
                mov     [si], al
                or      cx, cx
                jz      short loc_126D5
                cmp     al, 61h ; 'a'
                jl      short loc_126D5
                sub     byte ptr [si], 20h ; ' '

loc_126D5:                              ; CODE XREF: sub_125D2+FA↑j
                                        ; sub_125D2+FE↑j
                inc     si
                inc     di
                cmp     byte ptr [di-1], 0
                jnz     short loc_126C6
                cmp     word_16F20, 0
                jnz     short loc_126F8
                mov     ax, word_16F16
                or      ax, word_16F1C
                jz      short loc_126F8
                cmp     [bp+var_8], 0
                jnz     short loc_126F8
                mov     ax, 1
                jmp     short loc_126FA
; ---------------------------------------------------------------------------

loc_126F8:                              ; CODE XREF: sub_125D2+110↑j
                                        ; sub_125D2+119↑j ...
                sub     ax, ax

loc_126FA:                              ; CODE XREF: sub_125D2+124↑j
                push    ax
                call    sub_129B0
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_125D2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12708       proc near               ; CODE XREF: sub_122B4+26F↑p

var_E           = dword ptr -0Eh
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                cmp     [bp+arg_0], 0
                jz      short loc_1272A
                mov     si, 1
                mov     ax, word_16F1A
                mov     [bp+var_8], ax
                mov     [bp+var_6], ds
                add     word_16F1A, 2
                jmp     loc_127BB
; ---------------------------------------------------------------------------

loc_1272A:                              ; CODE XREF: sub_12708+C↑j
                cmp     word_16F14, 10h
                jnz     short loc_12748
                mov     bx, word_16F1A
                mov     ax, [bx]
                mov     dx, [bx+2]
                mov     [bp+var_8], ax
                mov     [bp+var_6], dx
                add     word_16F1A, 4
                jmp     short loc_1275C
; ---------------------------------------------------------------------------
                align 2

loc_12748:                              ; CODE XREF: sub_12708+27↑j
                mov     bx, word_16F1A
                mov     ax, [bx]
                mov     [bp+var_4], ax
                mov     [bp+var_8], ax
                mov     [bp+var_6], ds
                add     word_16F1A, 2

loc_1275C:                              ; CODE XREF: sub_12708+3D↑j
                cmp     word_16F14, 10h
                jnz     short loc_12770
                mov     ax, [bp+var_8]
                or      ax, [bp+var_6]
                jnz     short loc_1277F
                mov     ax, 1FEAh
                jmp     short loc_12779
; ---------------------------------------------------------------------------

loc_12770:                              ; CODE XREF: sub_12708+59↑j
                cmp     [bp+var_4], 0
                jnz     short loc_1277F
                mov     ax, 1FF1h

loc_12779:                              ; CODE XREF: sub_12708+66↑j
                mov     [bp+var_8], ax
                mov     [bp+var_6], ds

loc_1277F:                              ; CODE XREF: sub_12708+61↑j
                                        ; sub_12708+6C↑j
                mov     ax, [bp+var_8]
                mov     dx, [bp+var_6]
                mov     word ptr [bp+var_E], ax
                mov     word ptr [bp+var_E+2], dx
                sub     si, si
                cmp     word_16F1E, si
                jz      short loc_127AF
                mov     cx, t
                jmp     short loc_127A7
; ---------------------------------------------------------------------------
                align 2

loc_1279A:                              ; CODE XREF: sub_12708+A3↓j
                les     bx, [bp+var_E]
                inc     word ptr [bp+var_E]
                cmp     byte ptr es:[bx], 0
                jz      short loc_127BB
                inc     si

loc_127A7:                              ; CODE XREF: sub_12708+8F↑j
                cmp     cx, si
                jle     short loc_127BB
                jmp     short loc_1279A
; ---------------------------------------------------------------------------
                align 2

loc_127AE:                              ; CODE XREF: sub_12708+B1↓j
                inc     si

loc_127AF:                              ; CODE XREF: sub_12708+89↑j
                les     bx, [bp+var_E]
                inc     word ptr [bp+var_E]
                cmp     byte ptr es:[bx], 0
                jnz     short loc_127AE

loc_127BB:                              ; CODE XREF: sub_12708+1F↑j
                                        ; sub_12708+9C↑j ...
                mov     di, word_16F2C
                sub     di, si
                cmp     word_16F18, 0
                jnz     short loc_127CF
                push    di
                call    sub_128EA
                add     sp, 2

loc_127CF:                              ; CODE XREF: sub_12708+BE↑j
                push    si
                push    [bp+var_6]
                push    [bp+var_8]
                call    sub_12948
                add     sp, 6
                cmp     word_16F18, 0
                jz      short loc_127EA
                push    di
                call    sub_128EA
                add     sp, 2

loc_127EA:                              ; CODE XREF: sub_12708+D9↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_12708       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_127F0       proc near               ; CODE XREF: sub_122B4+280↑p

var_4           = byte ptr -4
str             = dword ptr -2
x               = word ptr  4

                push    bp              ; t
                mov     bp, sp
                sub     sp, 4
                mov     ax, word_16F1A
                mov     word ptr [bp+str], ax
                cmp     [bp+x], 67h ; 'g'
                jz      short loc_12808
                cmp     [bp+x], 47h ; 'G'
                jnz     short loc_1280C

loc_12808:                              ; CODE XREF: sub_127F0+10↑j
                mov     al, 1
                jmp     short loc_1280E
; ---------------------------------------------------------------------------

loc_1280C:                              ; CODE XREF: sub_127F0+16↑j
                sub     al, al

loc_1280E:                              ; CODE XREF: sub_127F0+1A↑j
                mov     [bp+var_4], al
                cmp     word_16F1E, 0
                jnz     short loc_1281E
                mov     t, 6

loc_1281E:                              ; CODE XREF: sub_127F0+26↑j
                cmp     [bp+var_4], 0
                jz      short loc_12831
                cmp     t, 0
                jnz     short loc_12831
                mov     t, 1

loc_12831:                              ; CODE XREF: sub_127F0+32↑j
                                        ; sub_127F0+39↑j
                push    word_16F12
                push    t               ; t
                push    [bp+x]          ; x
                push    word_16F2A
                push    word ptr [bp+str] ; str
                call    off_16C46
                add     sp, 0Ah
                cmp     [bp+var_4], 0
                jz      short loc_12862
                cmp     word_16F0C, 0
                jnz     short loc_12862
                push    word_16F2A      ; str
                call    off_16C48
                add     sp, 2

loc_12862:                              ; CODE XREF: sub_127F0+5E↑j
                                        ; sub_127F0+65↑j
                cmp     word_16F0C, 0
                jz      short loc_1287B
                cmp     t, 0
                jnz     short loc_1287B
                push    word_16F2A      ; str
                call    off_16C4C
                add     sp, 2

loc_1287B:                              ; CODE XREF: sub_127F0+77↑j
                                        ; sub_127F0+7E↑j
                add     word_16F1A, 8
                mov     word_16F2E, 0
                mov     ax, word_16F16
                or      ax, word_16F1C
                jz      short loc_128A2
                push    word ptr [bp+str] ; str
                call    off_16C4E
                add     sp, 2
                or      ax, ax
                jz      short loc_128A2
                mov     ax, 1
                jmp     short loc_128A4
; ---------------------------------------------------------------------------

loc_128A2:                              ; CODE XREF: sub_127F0+9D↑j
                                        ; sub_127F0+AB↑j
                sub     ax, ax

loc_128A4:                              ; CODE XREF: sub_127F0+B0↑j
                push    ax
                call    sub_129B0
                mov     sp, bp
                pop     bp
                retn
sub_127F0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_128AC       proc near               ; CODE XREF: sub_122B4+226↑p
                                        ; sub_129B0+5C↓p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                cmp     word_16F24, 0
                jnz     short loc_128E6
                mov     bx, word_16F0E
                dec     word ptr [bx+2]
                js      short loc_128CE
                mov     al, byte ptr [bp+arg_0]
                mov     si, [bx]
                inc     word ptr [bx]
                mov     [si], al
                sub     ah, ah
                jmp     short loc_128D8
; ---------------------------------------------------------------------------
                align 2

loc_128CE:                              ; CODE XREF: sub_128AC+12↑j
                push    bx
                push    [bp+arg_0]
                call    sub_12084
                add     sp, 4

loc_128D8:                              ; CODE XREF: sub_128AC+1F↑j
                inc     ax
                jnz     short loc_128E2
                inc     word_16F24
                jmp     short loc_128E6
; ---------------------------------------------------------------------------
                align 2

loc_128E2:                              ; CODE XREF: sub_128AC+2D↑j
                inc     word_16F22

loc_128E6:                              ; CODE XREF: sub_128AC+9↑j
                                        ; sub_128AC+33↑j
                pop     si
                pop     bp
                retn
sub_128AC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_128EA       proc near               ; CODE XREF: sub_12708+C1↑p
                                        ; sub_12708+DC↑p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                cmp     word_16F24, 0
                jnz     short loc_12942
                mov     si, [bp+arg_0]
                or      si, si
                jle     short loc_12942
                jmp     short loc_12917
; ---------------------------------------------------------------------------

loc_12902:                              ; CODE XREF: sub_128EA+3B↓j
                push    word_16F0E
                push    word_16F30
                call    sub_12084
                add     sp, 4

loc_12910:                              ; CODE XREF: sub_128EA+48↓j
                inc     ax
                jnz     short loc_12917
                inc     word_16F24

loc_12917:                              ; CODE XREF: sub_128EA+16↑j
                                        ; sub_128EA+27↑j
                mov     ax, si
                dec     si
                or      ax, ax
                jle     short loc_12934
                mov     bx, word_16F0E
                dec     word ptr [bx+2]
                js      short loc_12902
                mov     al, byte ptr word_16F30
                mov     di, [bx]
                inc     word ptr [bx]
                mov     [di], al
                sub     ah, ah
                jmp     short loc_12910
; ---------------------------------------------------------------------------

loc_12934:                              ; CODE XREF: sub_128EA+32↑j
                cmp     word_16F24, 0
                jnz     short loc_12942
                mov     ax, [bp+arg_0]
                add     word_16F22, ax

loc_12942:                              ; CODE XREF: sub_128EA+D↑j
                                        ; sub_128EA+14↑j ...
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_128EA       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12948       proc near               ; CODE XREF: sub_122B4+2A4↑p
                                        ; sub_12708+CE↑p ...

arg_0           = dword ptr  4
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    di
                push    si
                mov     si, [bp+arg_4]
                cmp     word_16F24, 0
                jnz     short loc_129AA
                jmp     short loc_12978
; ---------------------------------------------------------------------------

loc_1295C:                              ; CODE XREF: sub_12948+3E↓j
                push    word_16F0E
                les     bx, [bp+arg_0]
                mov     al, es:[bx]
                cbw
                push    ax
                call    sub_12084
                add     sp, 4

loc_1296E:                              ; CODE XREF: sub_12948+52↓j
                inc     ax
                jnz     short loc_12975
                inc     word_16F24

loc_12975:                              ; CODE XREF: sub_12948+27↑j
                inc     word ptr [bp+arg_0]

loc_12978:                              ; CODE XREF: sub_12948+12↑j
                mov     ax, si
                dec     si
                or      ax, ax
                jz      short loc_1299C
                mov     bx, word_16F0E
                dec     word ptr [bx+2]
                js      short loc_1295C
                les     bx, [bp+arg_0]
                mov     al, es:[bx]
                mov     bx, word_16F0E
                mov     di, [bx]
                inc     word ptr [bx]
                mov     [di], al
                sub     ah, ah
                jmp     short loc_1296E
; ---------------------------------------------------------------------------

loc_1299C:                              ; CODE XREF: sub_12948+35↑j
                cmp     word_16F24, 0
                jnz     short loc_129AA
                mov     ax, [bp+arg_4]
                add     word_16F22, ax

loc_129AA:                              ; CODE XREF: sub_12948+10↑j
                                        ; sub_12948+59↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_12948       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_129B0       proc near               ; CODE XREF: sub_125D2+129↑p
                                        ; sub_127F0+B5↑p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    di
                push    si
                mov     si, word_16F2A
                sub     ax, ax
                mov     [bp+var_4], ax
                mov     [bp+var_6], ax
                cmp     word_16F30, 30h ; '0'
                jnz     short loc_129E3
                cmp     word_16F1E, ax
                jz      short loc_129E3
                cmp     word_16F10, ax
                jz      short loc_129DD
                cmp     word_16F28, ax
                jnz     short loc_129E3

loc_129DD:                              ; CODE XREF: sub_129B0+25↑j
                mov     word_16F30, 20h ; ' '

loc_129E3:                              ; CODE XREF: sub_129B0+19↑j
                                        ; sub_129B0+1F↑j ...
                mov     di, word_16F2C
                push    si
                call    sub_132C2
                add     sp, 2
                mov     [bp+var_8], ax
                sub     di, ax
                sub     di, [bp+arg_0]
                cmp     word_16F18, 0
                jnz     short loc_12A15
                cmp     byte ptr [si], 2Dh ; '-'
                jnz     short loc_12A15
                cmp     word_16F30, 30h ; '0'
                jnz     short loc_12A15
                lodsb
                cbw
                push    ax
                call    sub_128AC
                add     sp, 2
                dec     [bp+var_8]

loc_12A15:                              ; CODE XREF: sub_129B0+4B↑j
                                        ; sub_129B0+50↑j ...
                cmp     word_16F30, 30h ; '0'
                jz      short loc_12A27
                or      di, di
                jle     short loc_12A27
                cmp     word_16F18, 0
                jz      short loc_12A40

loc_12A27:                              ; CODE XREF: sub_129B0+6A↑j
                                        ; sub_129B0+6E↑j
                cmp     [bp+arg_0], 0
                jz      short loc_12A33
                inc     [bp+var_6]
                call    sub_12A92

loc_12A33:                              ; CODE XREF: sub_129B0+7B↑j
                cmp     word_16F2E, 0
                jz      short loc_12A40
                inc     [bp+var_4]
                call    sub_12AAA

loc_12A40:                              ; CODE XREF: sub_129B0+75↑j
                                        ; sub_129B0+88↑j
                cmp     word_16F18, 0
                jnz     short loc_12A6D
                push    di
                call    sub_128EA
                add     sp, 2
                cmp     [bp+arg_0], 0
                jz      short loc_12A5D
                cmp     [bp+var_6], 0
                jnz     short loc_12A5D
                call    sub_12A92

loc_12A5D:                              ; CODE XREF: sub_129B0+A2↑j
                                        ; sub_129B0+A8↑j
                cmp     word_16F2E, 0
                jz      short loc_12A6D
                cmp     [bp+var_4], 0
                jnz     short loc_12A6D
                call    sub_12AAA

loc_12A6D:                              ; CODE XREF: sub_129B0+95↑j
                                        ; sub_129B0+B2↑j ...
                push    [bp+var_8]
                push    ds
                push    si
                call    sub_12948
                add     sp, 6
                cmp     word_16F18, 0
                jz      short loc_12A8C
                mov     word_16F30, 20h ; ' '
                push    di
                call    sub_128EA
                add     sp, 2

loc_12A8C:                              ; CODE XREF: sub_129B0+CD↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_129B0       endp


; =============== S U B R O U T I N E =======================================


sub_12A92       proc near               ; CODE XREF: sub_129B0+80↑p
                                        ; sub_129B0+AA↑p
                cmp     word_16F16, 0
                jz      short loc_12A9E
                mov     ax, 2Bh ; '+'
                jmp     short loc_12AA1
; ---------------------------------------------------------------------------

loc_12A9E:                              ; CODE XREF: sub_12A92+5↑j
                mov     ax, 20h ; ' '

loc_12AA1:                              ; CODE XREF: sub_12A92+A↑j
                push    ax
                call    sub_128AC
                add     sp, 2
                retn
sub_12A92       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_12AAA       proc near               ; CODE XREF: sub_129B0+8D↑p
                                        ; sub_129B0+BA↑p
                mov     ax, 30h ; '0'
                push    ax
                call    sub_128AC
                add     sp, 2
                cmp     word_16F2E, 10h
                jnz     short locret_12AD2
                cmp     word_16F12, 0
                jz      short loc_12AC8
                mov     ax, 58h ; 'X'
                jmp     short loc_12ACB
; ---------------------------------------------------------------------------
                align 2

loc_12AC8:                              ; CODE XREF: sub_12AAA+16↑j
                mov     ax, 78h ; 'x'

loc_12ACB:                              ; CODE XREF: sub_12AAA+1B↑j
                push    ax
                call    sub_128AC
                add     sp, 2

locret_12AD2:                           ; CODE XREF: sub_12AAA+F↑j
                retn
sub_12AAA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12AD4       proc near               ; CODE XREF: sub_122B4+B7↑p
                                        ; sub_122B4+E1↑p

var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     si, [bp+arg_2]
                mov     [bp+var_2], 1
                cmp     byte ptr [si], 2Ah ; '*'
                jnz     short loc_12AF8
                mov     bx, word_16F1A
                mov     di, [bx]
                add     word_16F1A, 2
                inc     si
                jmp     short loc_12B3F
; ---------------------------------------------------------------------------
                align 2

loc_12AF8:                              ; CODE XREF: sub_12AD4+13↑j
                cmp     byte ptr [si], 2Dh ; '-'
                jnz     short loc_12B03
                mov     [bp+var_2], 0FFFFh
                inc     si

loc_12B03:                              ; CODE XREF: sub_12AD4+27↑j
                sub     di, di
                cmp     byte ptr [si], 30h ; '0'
                jl      short loc_12B3F
                cmp     byte ptr [si], 39h ; '9'
                jg      short loc_12B3F
                cmp     word_16F1E, di
                jnz     short loc_12B20
                cmp     byte ptr [si], 30h ; '0'
                jnz     short loc_12B20
                mov     word_16F30, 30h ; '0'

loc_12B20:                              ; CODE XREF: sub_12AD4+3F↑j
                                        ; sub_12AD4+44↑j ...
                mov     al, [si]
                cbw
                mov     cx, di
                shl     cx, 1
                shl     cx, 1
                add     cx, di
                shl     cx, 1
                add     cx, ax
                sub     cx, 30h ; '0'
                mov     di, cx
                inc     si
                cmp     byte ptr [si], 30h ; '0'
                jl      short loc_12B3F
                cmp     byte ptr [si], 39h ; '9'
                jle     short loc_12B20

loc_12B3F:                              ; CODE XREF: sub_12AD4+21↑j
                                        ; sub_12AD4+34↑j ...
                mov     ax, [bp+var_2]
                imul    di
                mov     di, ax
                mov     bx, [bp+arg_0]
                mov     [bx], di
                mov     ax, si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_12AD4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12B54       proc near               ; CODE XREF: sub_122B4+A8↑p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     si, 1FF8h
                mov     cl, [bp+arg_0]
                jmp     short loc_12B65
; ---------------------------------------------------------------------------
                align 2

loc_12B64:                              ; CODE XREF: sub_12B54+18↓j
                inc     si

loc_12B65:                              ; CODE XREF: sub_12B54+D↑j
                cmp     byte ptr [si], 0
                jz      short loc_12B74
                cmp     cl, [si]
                jnz     short loc_12B64
                mov     ax, 1
                jmp     short loc_12B76
; ---------------------------------------------------------------------------
                align 2

loc_12B74:                              ; CODE XREF: sub_12B54+14↑j
                sub     ax, ax

loc_12B76:                              ; CODE XREF: sub_12B54+1D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_12B54       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_12B7C       proc near               ; CODE XREF: sub_13A14+10A↓p
                                        ; sub_13A14+150↓p

arg_0           = word ptr  4

; FUNCTION CHUNK AT 269E SIZE 00000008 BYTES

                push    bp
                mov     bp, sp
                mov     bx, [bp+arg_0]
                cmp     bx, word_16ABA
                jb      short loc_12B8E
                mov     ax, 900h
                stc
                jmp     short loc_12B99
; ---------------------------------------------------------------------------

loc_12B8E:                              ; CODE XREF: sub_12B7C+A↑j
                mov     ah, 3Eh
                int     21h             ; DOS - 2+ - CLOSE A FILE WITH HANDLE
                                        ; BX = file handle
                jb      short loc_12B99
                mov     byte ptr [bx+1E8Ch], 0

loc_12B99:                              ; CODE XREF: sub_12B7C+10↑j
                                        ; sub_12B7C+16↑j
                jmp     loc_11FFE
sub_12B7C       endp

; [0000007A BYTES: COLLAPSED FUNCTION sub_12B9C. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                xor     bh, bh
; [00000093 BYTES: COLLAPSED CHUNK OF FUNCTION _sopen. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [00000048 BYTES: COLLAPSED CHUNK OF FUNCTION _sopen. PRESS CTRL-NUMPAD+ TO EXPAND]
                db 90h
; ---------------------------------------------------------------------------
; [000000AE BYTES: COLLAPSED CHUNK OF FUNCTION _sopen. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000011 BYTES: COLLAPSED FUNCTION sub_12DA9. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000DD BYTES: COLLAPSED FUNCTION sub_12DBA. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [000000A6 BYTES: COLLAPSED FUNCTION sub_12E98. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000004E BYTES: COLLAPSED FUNCTION sub_12F3E. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
; [00000034 BYTES: COLLAPSED CHUNK OF FUNCTION sub_12E98. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000012 BYTES: COLLAPSED FUNCTION sub_12FC0. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000012 BYTES: COLLAPSED FUNCTION sub_12FD2. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000046 BYTES: COLLAPSED FUNCTION sub_12FE4. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
; [00000003 BYTES: COLLAPSED CHUNK OF FUNCTION sub_1302D. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000E3 BYTES: COLLAPSED FUNCTION sub_1302D. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000003A BYTES: COLLAPSED FUNCTION sub_13110. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000022 BYTES: COLLAPSED FUNCTION sub_1314A. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001F BYTES: COLLAPSED FUNCTION sub_1316C. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [0000006E BYTES: COLLAPSED FUNCTION sub_1318C. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000056 BYTES: COLLAPSED FUNCTION sub_131FA. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000003F BYTES: COLLAPSED FUNCTION _strcat. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [00000032 BYTES: COLLAPSED FUNCTION _strcpy. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001B BYTES: COLLAPSED FUNCTION sub_132C2. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [0000000A BYTES: COLLAPSED FUNCTION _ultoa. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000023 BYTES: COLLAPSED FUNCTION _isatty. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [00000010 BYTES: COLLAPSED FUNCTION _kbhit. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
                mov     dh, 1
                jmp     short loc_13322
; [00000017 BYTES: COLLAPSED FUNCTION _getch. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [00000056 BYTES: COLLAPSED FUNCTION _sprintf. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001B BYTES: COLLAPSED FUNCTION _sopen. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [0000002A BYTES: COLLAPSED FUNCTION _strchr. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000002B BYTES: COLLAPSED FUNCTION _strrchr. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [0000005F BYTES: COLLAPSED CHUNK OF FUNCTION _ultoa. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 2
; [00000225 BYTES: COLLAPSED FUNCTION __cenvarg. PRESS CTRL-NUMPAD+ TO EXPAND]
                align 4
; [000002AB BYTES: COLLAPSED FUNCTION __doexec. PRESS CTRL-NUMPAD+ TO EXPAND]
; ---------------------------------------------------------------------------
                push    cs
                pop     ds
                assume ds:sg0138
                int     21h             ; DOS -
                jnb     short loc_13977
                mov     bx, 1
                cmp     al, 1
                jz      short loc_1394E
                mov     bl, 2
                cmp     al, 0Ah
                jz      short loc_1394E
                mov     bl, 1
                cmp     al, 0Bh
                jz      short loc_1394E
                xor     bx, bx

loc_1394E:                              ; CODE XREF: sg0138:3FDE↑j
                                        ; sg0138:3FE4↑j ...
                push    cs
                pop     es
                assume es:sg0138
                xor     ax, ax
                mov     cx, 0FFFFh
                mov     di, 5Ch ; '\'
                nop
                or      bx, bx
                jz      short loc_1396C
                jmp     short loc_13965
; ---------------------------------------------------------------------------
                db 6 dup(0)
; ---------------------------------------------------------------------------

loc_13965:                              ; CODE XREF: sg0138:3FFD↑j
                                        ; sg0138:400A↓j
                repne scasb
                inc     di
                inc     di
                dec     bx
                jnz     short loc_13965

loc_1396C:                              ; CODE XREF: sg0138:3FFB↑j
                mov     dx, di
                mov     ah, 9
                int     21h             ; DOS - PRINT STRING
                                        ; DS:DX -> string terminated by "$"
                mov     ax, 4CFFh
                int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
                                        ; AL = exit code
; ---------------------------------------------------------------------------

loc_13977:                              ; CODE XREF: sg0138:3FD7↑j
                pop     di
                pop     si
                lodsw
                add     ax, di
                xchg    ax, dx
                lodsw
                cli
                mov     ss, dx
                mov     sp, ax
                sti
                lodsw
                add     di, ax
                lodsw
                push    di
                push    ax
                push    es
                pop     ds
                mov     ax, bp
                retf
; ---------------------------------------------------------------------------
                push    cs
                pop     ds
                int     21h             ; DOS -
                jnb     short loc_139D3
                mov     bx, 1
                cmp     al, 1
                jz      short loc_139AA
                mov     bl, 2
                cmp     al, 0Ah
                jz      short loc_139AA
                mov     bl, 1
                cmp     al, 0Bh
                jz      short loc_139AA
                xor     bx, bx

loc_139AA:                              ; CODE XREF: sg0138:403A↑j
                                        ; sg0138:4040↑j ...
                push    cs
                pop     es
                xor     ax, ax
                mov     cx, 0FFFFh
                mov     di, 6Fh ; 'o'
                nop
                or      bx, bx
                jz      short loc_139C8
                jmp     short loc_139C1
; ---------------------------------------------------------------------------
                db 6 dup(0)
; ---------------------------------------------------------------------------

loc_139C1:                              ; CODE XREF: sg0138:4059↑j
                                        ; sg0138:4066↓j
                repne scasb
                inc     di
                inc     di
                dec     bx
                jnz     short loc_139C1

loc_139C8:                              ; CODE XREF: sg0138:4057↑j
                mov     dx, di
                mov     ah, 9
                int     21h             ; DOS - PRINT STRING
                                        ; DS:DX -> string terminated by "$"
                mov     ax, 4CFFh
                int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
                                        ; AL = exit code
; ---------------------------------------------------------------------------

loc_139D3:                              ; CODE XREF: sg0138:4033↑j
                pop     di
                mov     bx, cs
                mov     ax, es
                sub     bx, ax
                mov     cl, 4
                test    bx, 0F000h
                jz      short loc_139E5
                mov     bx, 1000h

loc_139E5:                              ; CODE XREF: sg0138:4080↑j
                shl     bx, cl
                dec     bx
                dec     bx
                cli
                mov     ss, ax
                assume ss:sg0138
                mov     sp, bx
                sti
                mov     word ptr ss:[bx], 0
                push    es
                mov     ax, 100h
                push    ax
                push    es
                pop     ds
                mov     ax, bp
                retf

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_139FE       proc near               ; CODE XREF: _main+27D↑p

arg_0           = word ptr  4
arg_2           = byte ptr  6

                push    bp
                mov     bp, sp
                push    word ptr loc_11803+1
                lea     ax, [bp+arg_2]
                push    ax
                push    [bp+arg_0]
                call    sub_13A14
                add     sp, 6
                pop     bp
                retn
sub_139FE       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_13A14       proc near               ; CODE XREF: sub_139FE+E↑p

var_B2          = byte ptr -0B2h
var_32          = word ptr -32h
var_30          = word ptr -30h
var_2E          = word ptr -2Eh
var_2C          = word ptr -2Ch
var_2A          = word ptr -2Ah
var_28          = word ptr -28h
var_24          = word ptr -24h
var_20          = word ptr -20h
var_1E          = word ptr -1Eh
var_1C          = word ptr -1Ch
var_18          = word ptr -18h
var_16          = word ptr -16h
var_12          = word ptr -12h
var_10          = word ptr -10h
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
arg_0           = dword ptr  4
arg_4           = word ptr  8

                push    bp
                mov     bp, sp
                mov     ax, 0B2h
                call    sub_11D34
                push    di
                push    si              ; int
                mov     [bp+var_2A], 1
                mov     ax, 5Ch ; '\'
                push    ax
                push    word ptr [bp+arg_0] ; char *
                call    _strrchr
                add     sp, 4
                mov     si, ax
                mov     ax, 2Fh ; '/'
                push    ax
                push    word ptr [bp+arg_0] ; char *
                call    _strrchr
                add     sp, 4
                mov     di, ax
                or      di, di
                jnz     short loc_13A50
                or      si, si
                jnz     short loc_13A5A
                mov     si, word ptr [bp+arg_0]
                jmp     short loc_13A5A
; ---------------------------------------------------------------------------
                align 2

loc_13A50:                              ; CODE XREF: sub_13A14+30↑j
                or      si, si
                jz      short loc_13A58
                cmp     di, si
                jbe     short loc_13A5A

loc_13A58:                              ; CODE XREF: sub_13A14+3E↑j
                mov     si, di

loc_13A5A:                              ; CODE XREF: sub_13A14+34↑j
                                        ; sub_13A14+39↑j ...
                sub     di, di
                mov     ax, 2Eh ; '.'
                push    ax
                push    si              ; char *
                call    _strchr
                add     sp, 4
                or      ax, ax
                jz      short loc_13A8C
                mov     ax, 20h ; ' '
                push    ax              ; int
                mov     ax, 8000h
                push    ax
                push    word ptr [bp+arg_0] ; char *
                call    _sopen
                add     sp, 6
                mov     [bp+var_2C], ax
                inc     ax
                jz      short loc_13A85
                jmp     loc_13B07
; ---------------------------------------------------------------------------

loc_13A85:                              ; CODE XREF: sub_13A14+6C↑j
                                        ; sub_13A14+8F↓j ...
                mov     ax, 0FFFFh
                jmp     loc_13BF4
; ---------------------------------------------------------------------------
                align 2

loc_13A8C:                              ; CODE XREF: sub_13A14+55↑j
                push    word ptr [bp+arg_0]
                call    sub_132C2
                add     sp, 2
                add     ax, 5
                push    ax
                call    sub_12FE4
                add     sp, 2
                mov     di, ax
                or      di, di
                jz      short loc_13A85
                mov     ax, 2144h
                push    ax              ; char *
                push    word ptr [bp+arg_0]
                push    di              ; char *
                call    _strcpy
                add     sp, 4
                push    ax              ; char *
                call    _strcat
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax              ; int
                mov     ax, 8000h
                push    ax
                push    di              ; char *
                call    _sopen
                add     sp, 6
                mov     [bp+var_2C], ax
                inc     ax
                jnz     short loc_13B04
                mov     ax, 2149h
                push    ax              ; int
                mov     ax, 2Eh ; '.'
                push    ax
                push    di              ; char *
                call    _strrchr
                add     sp, 4
                push    ax              ; char *
                call    _strcpy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax              ; int
                mov     ax, 8000h
                push    ax
                push    di              ; char *
                call    _sopen
                add     sp, 6
                mov     [bp+var_2C], ax
                inc     ax
                jnz     short loc_13B04

loc_13AFA:                              ; CODE XREF: sub_13A14:loc_13B37↓j
                push    di
                call    sub_12FD2
                add     sp, 2
                jmp     short loc_13A85
; ---------------------------------------------------------------------------
                align 2

loc_13B04:                              ; CODE XREF: sub_13A14+B9↑j
                                        ; sub_13A14+E4↑j
                mov     word ptr [bp+arg_0], di

loc_13B07:                              ; CODE XREF: sub_13A14+6E↑j
                mov     ax, 18h
                push    ax
                lea     ax, [bp+var_20]
                push    ax
                push    [bp+var_2C]
                call    sub_12DBA
                add     sp, 6
                inc     ax
                jnz     short loc_13B3A
                push    [bp+var_2C]
                call    sub_12B7C
                add     sp, 2
                mov     word ptr loc_117DD, 8
                mov     word ptr loc_117E6+2, 0Bh

loc_13B30:                              ; CODE XREF: sub_13A14+187↓j
                                        ; sub_13A14+1DD↓j
                or      di, di
                jnz     short loc_13B37
                jmp     loc_13A85
; ---------------------------------------------------------------------------

loc_13B37:                              ; CODE XREF: sub_13A14+11E↑j
                jmp     short loc_13AFA
; ---------------------------------------------------------------------------
                align 2

loc_13B3A:                              ; CODE XREF: sub_13A14+105↑j
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_2C]
                call    sub_12B9C
                add     sp, 8
                add     ax, 0Fh
                adc     dx, 0
                mov     cl, 4

loc_13B53:                              ; CODE XREF: sub_13A14+145↓j
                sar     dx, 1
                rcr     ax, 1
                dec     cl
                jnz     short loc_13B53
                mov     [bp+var_30], ax
                mov     [bp+var_2E], dx
                push    [bp+var_2C]
                call    sub_12B7C
                add     sp, 2
                cmp     [bp+var_20], 4D5Ah
                jz      short loc_13B78
                cmp     [bp+var_20], 5A4Dh
                jnz     short loc_13B7B

loc_13B78:                              ; CODE XREF: sub_13A14+15B↑j
                dec     [bp+var_2A]

loc_13B7B:                              ; CODE XREF: sub_13A14+162↑j
                push    word ptr [bp+arg_0]
                lea     ax, [bp+var_B2]
                push    ax
                lea     ax, [bp+var_24]
                push    ax
                lea     ax, [bp+var_32]
                push    ax
                push    [bp+arg_4]
                push    word ptr [bp+arg_0+2]
                call    __cenvarg
                add     sp, 0Ch
                mov     [bp+var_28], ax
                inc     ax
                jz      short loc_13B30
                push    [bp+var_30]
                push    [bp+var_C]
                push    [bp+var_A]
                push    [bp+var_10]
                push    [bp+var_12]
                mov     ax, 200h
                sub     ax, [bp+var_1E]
                mov     cl, 4
                shr     ax, cl
                mov     dx, [bp+var_1C]
                mov     cl, 5
                shl     dx, cl
                add     ax, dx
                add     ax, [bp+var_16]
                sub     ax, [bp+var_18]
                push    ax
                push    [bp+var_28]
                push    [bp+var_24]
                lea     ax, [bp+var_B2]
                push    ax
                push    word ptr [bp+arg_0]
                call    sub_132C2
                add     sp, 2
                inc     ax
                push    ax
                push    word ptr [bp+arg_0]
                push    [bp+var_2A]
                call    __doexec
                add     sp, 18h
                push    [bp+var_32]
                call    sub_12FD2
                add     sp, 2
                jmp     loc_13B30
; ---------------------------------------------------------------------------

loc_13BF4:                              ; CODE XREF: sub_13A14+74↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_13A14       endp


; =============== S U B R O U T I N E =======================================


sub_13BFA       proc near               ; CODE XREF: fontOpen+5D↑p
                xor     ch, ch
                jcxz    short locret_13C04

loc_13BFE:                              ; CODE XREF: sub_13BFA+8↓j
                shl     ax, 1
                rcl     dx, 1
                loop    loc_13BFE

locret_13C04:                           ; CODE XREF: sub_13BFA+2↑j
                retn
sub_13BFA       endp

; ---------------------------------------------------------------------------
                align 10h
sg0138          ends

; ===========================================================================

; Segment type: Pure code
sg0563          segment byte public 'CODE' use16
                assume cs:sg0563
                assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================


setTextMode     proc far                ; CODE XREF: error↑P
                push    ax
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                cli
                mov     al, 3
                mov     ah, 0
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retf
setTextMode     endp


; =============== S U B R O U T I N E =======================================


gfxScanlinesSetup proc far              ; CODE XREF: gfxInit+5C↑P
                push    ax
                push    bx
                push    cx
                push    dx
                push    si
                push    di
                push    ds
                push    es
                mov     word_158FE, 1871
                mov     word_158FC, 0
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                mov     di, offset _scanlinePtrs?
                xor     ax, ax
                mov     cx, 200

loc_13C45:                              ; CODE XREF: gfxScanlinesSetup+25↓j
                stosw
                add     ax, 160
                loop    loc_13C45
                xor     bx, bx
                mov     bl, _gfxMode
                shl     bx, 1
                mov     bx, VIDEO_MODE_FNS[bx]
                call    bx
                pop     es
                assume es:nothing
                pop     ds
                pop     di
                pop     si
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                retf
gfxScanlinesSetup endp


; =============== S U B R O U T I N E =======================================


sub_13C62       proc near               ; CODE XREF: sub_13D18+5↓p
                mov     di, 0B76h
                mov     bx, 0D32h
                mov     cl, 10h

loc_13C6A:                              ; CODE XREF: sub_13C62+4E↓j
                mov     si, 0D32h
                mov     ch, 10h

loc_13C6F:                              ; CODE XREF: sub_13C62+47↓j
                mov     ah, [bx]
                and     ah, 0Ch
                lodsb
                and     al, 3
                or      al, ah
                mov     cs:[di+100h], al
                shl     al, 1
                shl     al, 1
                shl     al, 1
                shl     al, 1
                mov     cs:[di], al
                mov     ah, [bx+1]
                and     ah, 0Ch
                lodsb
                and     al, 3
                or      al, ah
                mov     cs:[di+300h], al
                shl     al, 1
                shl     al, 1
                shl     al, 1
                shl     al, 1
                mov     cs:[di+200h], al
                inc     di
                dec     ch
                jnz     short loc_13C6F
                add     bx, 2
                dec     cl
                jnz     short loc_13C6A
                retn
sub_13C62       endp

; ---------------------------------------------------------------------------
                mov     word_158FE, 74Fh
                mov     word_158FC, 0
                retf

; =============== S U B R O U T I N E =======================================


sub_13CC0       proc far                ; CODE XREF: sub_114B3+16↑P
                mov     bx, word_158FE
                add     word_158FE, 0Eh
                inc     word_158FC
                xor     ah, ah
                test    ax, 1
                jz      short loc_13CD5
                inc     di

loc_13CD5:                              ; CODE XREF: sub_13CC0+12↑j
                test    di, 1
                jz      short loc_13CDC
                inc     di

loc_13CDC:                              ; CODE XREF: sub_13CC0+19↑j
                mov     dx, 0A0h
                sub     dx, di
                mov     [bx+0Ch], dx
                and     ax, 0FEh
                sar     ax, 1
                sar     di, 1
                mov     [bx+8], cx
                mov     [bx+0Ah], di
                mov     [bx+2], ax
                shl     si, 1
                mov     di, 42Fh
                add     di, si
                mov     [bx], di
                mov     di, 5BFh
                add     di, si
                mov     si, [di]
                shl     ax, 1
                add     si, ax
                mov     [bx+4], si
                mov     di, [bx+0Ah]
                shl     di, 1
                mov     ax, [di+37Ah]
                mov     [bx+6], ax
                retf
sub_13CC0       endp


; =============== S U B R O U T I N E =======================================


sub_13D18       proc near               ; CODE XREF: gfxScanlinesSetup+33↑p
                                        ; DATA XREF: dseg:VIDEO_MODE_FNS↓o
                push    ax
                push    di
                push    cx
                push    ds
                push    es
                call    sub_13C62
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                xor     ax, ax
                mov     cx, 100
                mov     di, offset unk_1505F

loc_13D2D:                              ; CODE XREF: sub_13D18+1D↓j
                stosw
                add     ax, 2000h
                stosw
                add     ax, 0E050h
                loop    loc_13D2D
                mov     ax, 40h ; '@'
                mov     es, ax
                assume es:nothing
                cli
                mov     ax, es:10h
                and     al, 0CFh
                or      al, 20h
                mov     es:10h, ax
                sti
                mov     ax, 4
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                mov     ah, 0Bh
                mov     bx, 101h
                int     10h             ; - VIDEO - SET COLOR PALETTE
                                        ; BH = 00h, BL = border color
                                        ; BH = 01h, BL = palette (0-3)
                mov     ah, 0Bh
                xor     bx, bx
                int     10h             ; - VIDEO - SET COLOR PALETTE
                                        ; BH = 00h, BL = border color
                                        ; BH = 01h, BL = palette (0-3)
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                mov     di, 37Ah
                mov     cx, 51h ; 'Q'
                mov     ax, 944h

loc_13D6A:                              ; CODE XREF: sub_13D18+56↓j
                stosw
                sub     ax, 0Eh
                loop    loc_13D6A
                mov     cx, 0Eh
                mov     ax, cs
                mov     ds, ax
                assume ds:sg0563
                mov     es, ax
                assume es:sg0563
                mov     si, 182h
                mov     di, 4E4h
                rep movsb
                mov     cx, 452h
                mov     si, 4E4h
                rep movsb
                mov     al, 0C3h
                stosb
                pop     es
                assume es:nothing
                pop     ds
                assume ds:dseg
                pop     cx
                pop     di
                pop     ax
                retn
sub_13D18       endp

; ---------------------------------------------------------------------------
                lodsw
                xlat    byte ptr cs:[bx]
                xchg    al, ah
                inc     bh
                xlat    byte ptr cs:[bx]
                or      al, ah
                stosb
                dec     bh

; =============== S U B R O U T I N E =======================================


sub_13DA0       proc near               ; CODE XREF: gfxScanlinesSetup+33↑p
                                        ; DATA XREF: dseg:VIDEO_MODE_FNS↓o
                push    es
                mov     ax, 9
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                mov     word ptr cs:self_modifying_1+1, 8Fh
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                mov     di, 42Fh
                xor     ax, ax
                mov     cx, 32h ; '2'

loc_13DBA:                              ; CODE XREF: sub_13DA0+2A↓j
                stosw
                add     ax, 2000h
                stosw
                add     ax, 2000h
                stosw
                add     ax, 2000h
                stosw
                sub     ax, 5F60h
                loop    loc_13DBA
                pop     es
                assume es:nothing
                retn
sub_13DA0       endp


; =============== S U B R O U T I N E =======================================


sub_13DCE       proc near               ; CODE XREF: gfxScanlinesSetup+33↑p
                                        ; DATA XREF: dseg:VIDEO_MODE_FNS↓o
                mov     word ptr cs:self_modifying_1+1, 0E4h
                cli
                mov     ax, 40h ; '@'
                mov     es, ax
                assume es:nothing
                mov     ax, es:10h
                and     al, 0CFh
                or      al, 20h
                mov     es:10h, ax
                sti
                mov     ax, 0Eh
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                mov     ax, 501h
                int     10h             ; - VIDEO - SELECT DISPLAY PAGE
                                        ; AL = display page, 0-7  for modes 0 & 1, 0-3  for modes 2 & 3
                mov     dx, 3CEh
                mov     al, 5
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; mode register.Data bits:
                                        ; 0-1: Write mode 0-2
                                        ; 2: test condition
                                        ; 3: read mode: 1=color compare, 0=direct
                                        ; 4: 1=use odd/even RAM addressing
                                        ; 5: 1=use CGA mid-res map (2-bits/pixel)
                inc     dx
                mov     al, 2
                out     dx, al          ; EGA port: graphics controller data register
                mov     dx, 3C4h
                mov     al, 2
                out     dx, al          ; EGA: sequencer address reg
                                        ; map mask: data bits 0-3 enable writes to bit planes 0-3
                inc     dx
                mov     al, 0Fh
                out     dx, al          ; EGA port: sequencer data register
                mov     ax, 0A000h
                mov     es, ax
                assume es:nothing
                xor     bx, bx
                mov     cx, bx
                mov     si, bx
                mov     bp, 1000h

loc_13E14:                              ; CODE XREF: sub_13DCE+78↓j
                                        ; sub_13DCE+81↓j ...
                mov     dx, 3CEh
                mov     al, 8
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; bit mask
                                        ; Bits 0-7 select bits to be masked in all planes
                inc     dx
                mov     al, 0Ch
                out     dx, al          ; EGA port: graphics controller data register
                mov     ah, es:[si]
                mov     es:[si], bl
                dec     dx
                mov     al, 8
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; bit mask
                                        ; Bits 0-7 select bits to be masked in all planes
                inc     dx
                mov     al, 3
                out     dx, al          ; EGA port: graphics controller data register
                mov     ah, es:[si]
                mov     es:[si], cl
                dec     dx
                mov     al, 8
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; bit mask
                                        ; Bits 0-7 select bits to be masked in all planes
                inc     dx
                mov     al, 0C0h
                out     dx, al          ; EGA port: graphics controller data register
                mov     ah, es:[si]
                mov     es:[si], bh
                inc     si
                inc     bh
                cmp     bh, 10h
                jl      short loc_13E14
                xor     bh, bh
                inc     cl
                cmp     cl, 10h
                jl      short loc_13E14
                xor     cl, cl
                inc     bl
                cmp     bl, 10h
                jl      short loc_13E14
                dec     dx
                mov     al, 8
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; bit mask
                                        ; Bits 0-7 select bits to be masked in all planes
                inc     dx
                mov     al, 30h ; '0'
                out     dx, al          ; EGA port: graphics controller data register
                dec     dx
                mov     al, 3
                out     dx, al          ; EGA: graph 1 and 2 addr reg:
                                        ; data rotate and function select for write mode 00. Bits:
                                        ; 0-2: set rotate count for write mode 00
                                        ; 3-4: fn for write modes 00 and 02
                                        ;      00=no change; 01=AND; 10=OR; 11=XOR
                inc     dx
                mov     al, 0
                out     dx, al          ; EGA port: graphics controller data register
                mov     cx, 0C8h
                mov     di, 42Fh
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                mov     ax, 4000h

loc_13E78:                              ; CODE XREF: sub_13DCE+AE↓j
                stosw
                add     ax, 50h ; 'P'
                loop    loc_13E78
                mov     di, 37Ah
                mov     cx, 51h ; 'Q'
                mov     ax, 994h

loc_13E87:                              ; CODE XREF: sub_13DCE+BD↓j
                stosw
                sub     ax, 0Fh
                loop    loc_13E87
                mov     cx, 0Fh
                mov     ax, cs
                mov     ds, ax
                assume ds:sg0563
                mov     es, ax
                assume es:sg0563
                mov     si, 29Ah
                mov     di, 4E4h
                rep movsb
                mov     cx, 4A1h
                mov     si, 4E4h
                rep movsb
                mov     al, 0C3h
                stosb
                retn
sub_13DCE       endp

; ---------------------------------------------------------------------------
                lodsw
                mov     bx, ax
                shr     bx, 1
                shr     bx, 1
                shr     bx, 1
                shr     bx, 1
                mov     ah, es:[bx]
                stosb
                assume ds:dseg

; =============== S U B R O U T I N E =======================================


videoMode3      proc near               ; CODE XREF: gfxScanlinesSetup+33↑p
                                        ; DATA XREF: dseg:VIDEO_MODE_FNS↓o
                mov     word ptr cs:self_modifying_1+1, 136h
                mov     ax, 13h
                int     10h             ; - VIDEO - SET VIDEO MODE
                                        ; AL = mode
                cli
                mov     ax, 40h ; '@'
                mov     es, ax
                assume es:nothing
                mov     ax, es:10h
                and     al, not 30h
                or      al, 20h
                mov     es:10h, ax      ; 01=40-clm color
                sti
;
                mov     ax, seg dseg
                mov     es, ax
                assume es:dseg
                mov     si, offset MASTER_PALETTE
                mov     cx, 16
                xor     bx, bx

loc_13EE5:                              ; CODE XREF: videoMode3+53↓j
                mov     di, offset _tempPalette
                lodsb
                mov     dh, al
                lodsw
                push    cx
                mov     cx, 10h

loc_13EF0:                              ; CODE XREF: videoMode3+3C↓j
                mov     es:[di], dh
                inc     di
                stosw
                loop    loc_13EF0
                mov     ax, 1012h
                mov     cx, 16
                mov     dx, offset _tempPalette
                push    bx
                push    si
                push    di
                int     10h             ; - VIDEO - SET BLOCK OF DAC REGISTERS (EGA, VGA/MCGA)
                                        ; BX = starting color register, CX = number of registers to set
                                        ; ES:DX -> table of 3*CX bytes where each 3 byte group represents one
                                        ; byte each of red, green and blue (0-63)
                pop     di
                pop     si
                pop     bx
                add     bx, 16
                pop     cx
                loop    loc_13EE5
                mov     di, offset unk_1505F
                mov     cx, 200
                xor     ax, ax

loc_13F16:                              ; CODE XREF: videoMode3+61↓j
                stosw
                add     ax, 320
                loop    loc_13F16
                mov     di, offset unk_14FAA
                mov     cx, 81
                mov     ax, 0B74h

loc_13F25:                              ; CODE XREF: videoMode3+70↓j
                stosw
                sub     ax, 21
                loop    loc_13F25
                mov     ax, cs
                mov     ds, ax
                assume ds:sg0563
                mov     es, ax
                assume es:sg0563
                mov     si, offset sub_13F48
                mov     di, offset byte_140F4
                mov     cx, 21
                rep movsb
                mov     cx, 1659
                mov     si, offset byte_140F4
                rep movsb
                mov     al, 0C3h
                stosb
                retn
videoMode3      endp


; =============== S U B R O U T I N E =======================================


sub_13F48       proc near               ; DATA XREF: videoMode3+78↑o
                lodsw
                mov     ch, ah
                mov     bx, ax
                rol     bx, 1
                rol     bx, 1
                rol     bx, 1
                rol     bx, 1
                mov     ah, bl
                stosw
                mov     al, ch
                mov     ah, bh
                stosw
sub_13F48       endp


; =============== S U B R O U T I N E =======================================


sub_13F5D       proc far                ; CODE XREF: sub_114D5+12↑P
                                        ; sg0138:1B96↑P
                push    si
                push    di
                push    es
                push    ds
                push    bp

self_modifying_1:                       ; DATA XREF: sub_13DA0+6↑w
                                        ; sub_13DCE↑w ...
                call    sub_13F77       ; Self-changing code sets to video3Fn1
                pop     bp
                pop     ds
                assume ds:dseg
                pop     es
                assume es:nothing
                pop     di
                pop     si
                mov     word_158FE, 74Fh
                mov     word_158FC, 0
                retf
sub_13F5D       endp


; =============== S U B R O U T I N E =======================================


sub_13F77       proc near               ; CODE XREF: sub_13F5D:self_modifying_1↑p
                mov     word_15900, 74Fh
                mov     ax, 0B800h
                mov     es, ax
                assume es:nothing

loc_13F82:                              ; CODE XREF: sub_13F77+7A↓j
                cmp     word_158FC, 0
                jnz     short loc_13F8A
                retn
; ---------------------------------------------------------------------------

loc_13F8A:                              ; CODE XREF: sub_13F77+10↑j
                dec     word_158FC
                mov     bx, word_15900
                mov     ax, [bx+2]
                shr     ax, 1
                mov     word_158F9, ax
                mov     bp, [bx]
                mov     si, [bx+4]
                mov     dx, [bx+0Ch]
                mov     cx, [bx+8]
                mov     ax, [bx+6]
                mov     word_158F7, ax

loc_13FAB:                              ; CODE XREF: sub_13F77+73↓j
                mov     di, ds:[bp+0]
                add     bp, 2
                add     di, word_158F9
                mov     ax, word_158F7
                mov     ds, word ptr _pictArray1+2
                mov     bx, 0B76h
                call    ax ; word_158F7
                mov     ax, seg dseg
                mov     ds, ax
                add     si, dx
                dec     cx
                jz      short loc_13FEC
                mov     di, ds:[bp+0]
                add     bp, 2
                add     di, word_158F9
                mov     ax, word_158F7
                mov     ds, word ptr _pictArray1+2
                mov     bx, 0D76h
                call    ax ; word_158F7
                mov     ax, seg dseg
                mov     ds, ax
                add     si, dx
                loop    loc_13FAB

loc_13FEC:                              ; CODE XREF: sub_13F77+53↑j
                add     word_15900, 0Eh
                jmp     short loc_13F82
sub_13F77       endp

; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
                mov     word_15900, 74Fh
                mov     ax, 0B800h
                mov     es, ax

loc_13FFF:                              ; CODE XREF: sg0563:0436↓j
                cmp     word_158FC, 0
                jnz     short loc_14007
                retn
; ---------------------------------------------------------------------------

loc_14007:                              ; CODE XREF: sg0563:03F4↑j
                dec     word_158FC
                mov     bx, word_15900
                mov     ax, [bx+8]
                mov     byte_158FB, al
                mov     ax, [bx+2]
                shl     ax, 1
                mov     si, [bx+4]
                mov     dx, [bx+0Ch]
                mov     bp, [bx+0Ah]
                mov     bx, [bx]

loc_14025:                              ; CODE XREF: sg0563:042F↓j
                mov     di, [bx]
                add     bx, 2
                add     di, ax
                mov     cx, bp
                mov     ds, word ptr _pictArray1+2
                rep movsw
                mov     cx, seg dseg
                mov     ds, cx
                add     si, dx
                dec     byte_158FB
                jnz     short loc_14025
                add     word_15900, 0Eh
                jmp     short loc_13FFF
; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
                mov     word_15900, 74Fh
                mov     ax, 0A000h
                mov     es, ax
                assume es:nothing

loc_14054:                              ; CODE XREF: sg0563:0488↓j
                cmp     word_158FC, 0
                jnz     short loc_1405C
                retn
; ---------------------------------------------------------------------------

loc_1405C:                              ; CODE XREF: sg0563:0449↑j
                dec     word_158FC
                mov     bx, word_15900
                mov     si, [bx+4]
                mov     dx, [bx+0Ch]
                mov     cx, [bx+8]
                mov     ax, [bx+6]
                mov     word_158F7, ax
                mov     ax, [bx+2]
                mov     bx, [bx]
                mov     bp, [bx]
                add     bp, ax

loc_1407C:                              ; CODE XREF: sg0563:0481↓j
                mov     di, bp
                add     bp, 50h ; 'P'
                mov     ax, word_158F7
                mov     ds, word ptr _pictArray1+2
                call    ax ; word_158F7
                mov     ax, seg dseg
                mov     ds, ax
                add     si, dx
                loop    loc_1407C
                add     word_15900, 0Eh
                jmp     short loc_14054
; ---------------------------------------------------------------------------
                retn

; =============== S U B R O U T I N E =======================================


video3Fn1       proc near
                mov     word_15900, 74Fh
                mov     ax, 0A000h
                mov     es, ax

loc_140A6:                              ; CODE XREF: video3Fn1+56↓j
                cmp     word_158FC, 0
                jnz     short loc_140AE
                retn
; ---------------------------------------------------------------------------

loc_140AE:                              ; CODE XREF: video3Fn1+10↑j
                dec     word_158FC
                mov     bx, word_15900
                mov     si, [bx+4]
                mov     dx, [bx+0Ch]
                mov     cx, [bx+8]
                mov     ax, [bx+6]
                mov     word_158F7, ax
                mov     ax, [bx+2]
                shl     ax, 1
                shl     ax, 1
                mov     bx, [bx]
                mov     bp, [bx]
                add     bp, ax

loc_140D2:                              ; CODE XREF: video3Fn1+4F↓j
                mov     di, bp
                add     bp, 140h
                mov     ax, word_158F7
                mov     ds, word ptr _pictArray1+2
                call    ax ; word_158F7
                mov     ax, seg dseg
                mov     ds, ax
                add     si, dx
                dec     cl
                jnz     short loc_140D2
                add     word_15900, 0Eh
                jmp     short loc_140A6
video3Fn1       endp

; ---------------------------------------------------------------------------
                retn
; ---------------------------------------------------------------------------
byte_140F4      db 691h dup(0), 0C3h, 40Ah dup(0)
                                        ; DATA XREF: videoMode3+7B↑o
                                        ; videoMode3+86↑o

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_14B90       proc far                ; CODE XREF: sub_14BDD+32↓P

arg_0           = word ptr  6
arg_2           = word ptr  8
arg_4           = word ptr  0Ah
arg_6           = word ptr  0Ch

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     es, word ptr _pictArray1+2
                assume es:nothing
                mov     ax, [bp+arg_6]
                shl     ax, 1
                mov     si, ax
                mov     di, [si+5BFh]
                mov     ax, [bp+arg_4]
                sar     ax, 1
                add     di, ax
                mov     ds, [bp+arg_2]
                mov     si, [bp+arg_0]
                xor     ax, ax
                mov     dx, ax
                mov     al, [si]
                mov     dl, [si+1]
                mov     bx, 0A0h
                sub     bx, ax
                sar     ax, 1
                add     si, 4

loc_14BCA:                              ; CODE XREF: sub_14B90+41↓j
                mov     cx, ax
                rep movsw
                add     di, bx
                dec     dx
                jnz     short loc_14BCA
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retf
sub_14B90       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_14BDD       proc far                ; CODE XREF: sub_104EA+56↑P

arg_0           = word ptr  6
arg_2           = word ptr  8
arg_4           = word ptr  0Ah
arg_6           = word ptr  0Ch
arg_8           = word ptr  0Eh

                push    bp
                mov     bp, sp
                push    si
                push    di
                push    bx
                push    cx
                push    dx
                push    ds
                push    es
                push    bp
                mov     es, [bp+arg_4]
                mov     di, [bp+arg_2]
                mov     bx, [bp+arg_0]
                shl     bx, 1
                add     bx, 3
                add     di, es:[bx+di]
                xor     ax, ax
                mov     al, es:[di+3]
                add     ax, [bp+arg_8]
                push    ax
                xor     ax, ax
                mov     al, es:[di+2]
                add     ax, [bp+arg_6]
                push    ax
                push    es
                push    di
                call    sub_14B90
                add     sp, 8
                pop     bp
                pop     es
                pop     ds
                pop     dx
                pop     cx
                pop     bx
                pop     di
                pop     si
                pop     bp
                retf
sub_14BDD       endp

; ---------------------------------------------------------------------------
                align 10h
sg0563          ends

; ===========================================================================

; Segment type: Pure data
dseg            segment para public 'DATA' use16
                assume cs:dseg
                db 0
word_14C31      dw 0                    ; DATA XREF: start+50↑w
                dw 0
                db    0
                db    0
                db    0
aMsRunTimeLibra db 'MS Run-Time Library - Copyright (c) 1988, Microsoft Corp'
                db  11h
                db    0
aSt             db 'ST:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aIq             db 'IQ:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aDx             db 'DX:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aWp             db 'WP:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aAp             db 'AP:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aCh             db 'CH:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aLk             db 'LK:',0              ; DATA XREF: dseg:ATTRIBUTE_TEXT↓o
aOther          db 'Other',0            ; DATA XREF: dseg:SEX_NAMES↓o
aMale           db 'Male',0             ; DATA XREF: dseg:SEX_NAMES↓o
aFemale         db 'Female',0           ; DATA XREF: dseg:SEX_NAMES↓o
aS              db '%s',0               ; DATA XREF: writeCenteredString+A↑o
                db    0
                db 0D4h
                db  2Ah ; *
word_14CA6      dw 0                    ; DATA XREF: createNewMember+C7↑w
                                        ; sub_10C58:loc_10CCD↑w ...
ATTRIBUTE_TEXT  AttributeText <21, 2, offset aSt>
                                        ; DATA XREF: writeClassesAndAttributes+8E↑r ; "ST:"
                AttributeText <21, 3, offset aIq> ; "IQ:"
                AttributeText <21, 4, offset aDx> ; "DX:"
                AttributeText <21, 5, offset aWp> ; "WP:"
                AttributeText <30, 2, offset aAp> ; "AP:"
                AttributeText <30, 3, offset aCh> ; "CH:"
                AttributeText <30, 4, offset aLk> ; "LK:"
SEX_NAMES       dw offset aOther        ; "Other"
                dw offset aMale         ; "Male"
                dw offset aFemale       ; "Female"
aFont           db 'FONT',0             ; DATA XREF: fontOpen+9↑o
; char filename[]
filename        db 'DISK1',0            ; DATA XREF: loadDisk1+A↑o
aBorders        db 'BORDERS',0          ; DATA XREF: loadClasses+9↑o
aArchtype       db 'ARCHTYPE',0         ; DATA XREF: loadClasses+51↑o
aHdspct         db 'HDSPCT',0           ; DATA XREF: loadClasses+BC↑o
aDisk1_0        db 'DISK1',0            ; DATA XREF: sub_103E0+43↑o
aGani           db 'GANI',0             ; DATA XREF: displayWelcomePortrait+1E↑o
aF1d            db 'F%1d>',0            ; DATA XREF: listParty+34↑o
a1616s1616s     db '%-16.16s - %-16.16s',0 ; DATA XREF: listParty+7D↑o
aConD           db 'Con:%d',0           ; DATA XREF: sub_10751+1C↑o
; char str[]
str             db 'Skills',0           ; DATA XREF: displaySkills+3B↑o
; char aActivePassive[]
aActivePassive  db 'Active      Passive',0
                                        ; DATA XREF: displaySkills+49↑o
a1d1010s        db '%1d %-10.10s',0     ; DATA XREF: displaySkills+81↑o
a1d1010s_0      db '%1d %-10.10s',0     ; DATA XREF: displaySkills+C6↑o
a22d            db '%2.2d->',0          ; DATA XREF: sub_1089E+1F↑o
a1d1313s        db '%1d)%-13.13s',0     ; DATA XREF: writeClassesAndAttributes+3A↑o
aAttributePts   db 'Attribute Pts: <-',0
a3s             db '%3s',0              ; DATA XREF: writeClassesAndAttributes+92↑o
a2u             db '%2u',0              ; DATA XREF: writeClassesAndAttributes+CA↑o
aSex            db 'Sex:',0
; char aChooseAProfess[]
aChooseAProfess db 'Choose a Profession:',0 ; DATA XREF: sub_10A65+1E↑o
a1d1313s_0      db '%1d)%-13.13s',0     ; DATA XREF: sub_10A65+3E↑o
a1d1313s_1      db '%1d)%-13.13s',0     ; DATA XREF: sub_10A65+7E↑o
aOjnabBob       db 'Ojnab Bob',0
; char aReallyRemoveTh[]
aReallyRemoveTh db 'Really remove this member?',0
                                        ; DATA XREF: removePartyMember+28↑o
; char aYEsNO[]
aYEsNO          db 'Y)es  N)o',0        ; DATA XREF: removePartyMember+36↑o
aThereSNoOneToS db 'There',27h,'s no one to %s!',0
                                        ; DATA XREF: noOne+9↑o
aTpict          db 'TPICT',0            ; DATA XREF: _main+4E↑o
aWelcome        db 'Welcome',0          ; DATA XREF: _main:loc_11262↑o
; char aChooseAFunctio[]
aChooseAFunctio db 'Choose a function:',0 ; DATA XREF: _main+106↑o
; char text[]
text            db 'A)dd member      R)emove member',0
                                        ; DATA XREF: _main+118↑o
; char aEDitMemberPLay[]
aEDitMemberPLay db 'E)dit member     P)lay the game',0
                                        ; DATA XREF: _main+12B↑o
aRemove         db 'remove',0
aEdit           db 'edit',0
; char aItSToughOutThe[]
aItSToughOutThe db 'It',27h,'s tough out there!',0
                                        ; DATA XREF: _main+214↑o
; char aYouShouldTakeS[]
aYouShouldTakeS db 'You should take somebody with you.',0
                                        ; DATA XREF: _main+222↑o
aKehExe         db 'KEH.EXE',0
aKehExe_0       db 'KEH.EXE',0
                db 0FFh
                db 0FFh
                db  64h ; d
                db  76h ; v
                db  85h
                db  54h ; T
                db 0F6h
                db  5Ch ; \
                db  76h ; v
                db  1Fh
                db 0E7h
                db  12h
                db 0A7h
                db  6Bh ; k
                db  93h
                db 0C4h
                db  6Eh ; n
                db  1Bh
byte_14EBE      db 0                    ; DATA XREF: sub_1164F+1C↑r
_inverseText    db 0                    ; DATA XREF: listParty+50↑w
                                        ; listParty+66↑w ...
; void far *pictWelcome
_pictWelcome    dd 0                    ; DATA XREF: displayWelcomePortrait+3C↑r
                                        ; displayWelcomePortrait+60↑r ...
; void far *pictData
_pictData       dd 0                    ; DATA XREF: _main+6C↑r
                                        ; _main+9D↑r ...
_pictArray2     dd 0                    ; DATA XREF: displayWelcomePortrait+58↑r
                                        ; displayWelcomePortrait+6A↑r ...
; Window *currentWindow
_currentWindow  dw 0                    ; DATA XREF: writeCenteredString+18↑r
                                        ; writeCenteredString+40↑r ...
word_14ECE      dw 0                    ; DATA XREF: _main+A7↑o
                                        ; _main+B7↑o
                dw 0
                dw 0A0h
                dw 0C8h
                db    4
                db    0
                db    8
                db    0
                db  30h ; 0
                db    0
                db  60h ; `
                db    0
                db    0
                db    0
                db    0
                db    0
                db  27h ; '
                db    0
                db  18h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db 0A0h
                db    0
                db 0C8h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
; Window windowParty
_windowParty    Window <1, 21, 38, 23, 1, 21, <4, 0ACh, 98h, 18h>, 8, 0, 0, 0>
                                        ; DATA XREF: listParty+F↑o
; Window windowSkills
_windowSkills   Window <14, 1, 38, 12, 14, 1, <38h, 8, 64h, 60h>, 0, 0, 0, 0>
                                        ; DATA XREF: sub_10751+6↑o
                                        ; displaySkills+2E↑o ...
; Window windowMain
_windowMain     Window <0, 14, 39, 19, 0, 14, <0, 70h, 0A0h, 30h>, 0, 0, 0, 0>
                                        ; DATA XREF: writeClassesAndAttributes+17↑o
                                        ; sub_10A65+D↑o ...
; Window windowPortrait
_windowPortrait Window <1, 1, 12, 11, 1, 1, <4, 8, 30h, 60h>, 0, 0, 0, 2A6h>
_writeStringPtr1 dw 0                   ; DATA XREF: writeString↑w
                                        ; writeString+29↑r ...
; char far *writeStringPtr2
_writeStringPtr2 dw 0                   ; DATA XREF: writeString+4↑w
                                        ; writeString+41↑w ...
_writeStringPtr3 dw 0                   ; DATA XREF: writeString+8↑w
                                        ; writeString+3C↑w ...
aYouDoNotHaveEn db 'You do not have enough memory to run Fountain of Dreams.$',0
                                        ; DATA XREF: gfxInit-E↑o
unk_14FAA       db    0                 ; DATA XREF: videoMode3+63↑o
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
VIDEO_MODE_FNS  dw offset sub_13D18     ; DATA XREF: gfxScanlinesSetup+2F↑r
                dw offset sub_13DA0
                dw offset sub_13DCE
                dw offset videoMode3
_gfxMode        db 2                    ; DATA XREF: _main+19↑w
                                        ; gfxInit+F↑r ...
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
_pictArray1     dd 0                    ; DATA XREF: _main+95↑r
                                        ; _main+253↑r ...
unk_1505F       db    0                 ; DATA XREF: sub_13D18+12↑o
                                        ; videoMode3+55↑o
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
_scanlinePtrs?  dw 0C8h dup(0)          ; DATA XREF: writeChar?+13↑r
                                        ; writeChar+1C↑r ...
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
word_158F7      dw 0                    ; DATA XREF: sub_13F77+31↑w
                                        ; sub_13F77+3F↑r ...
word_158F9      dw 0                    ; DATA XREF: sub_13F77+20↑w
                                        ; sub_13F77+3B↑r ...
byte_158FB      db 0                    ; DATA XREF: sg0563:0402↑w
                                        ; sg0563:042B↑w
word_158FC      dw 0                    ; DATA XREF: gfxScanlinesSetup+E↑w
                                        ; sg0563:00A9↑w ...
word_158FE      dw 74Fh                 ; DATA XREF: gfxScanlinesSetup+8↑w
                                        ; sg0563:00A3↑w ...
word_15900      dw 0                    ; DATA XREF: sub_13F77↑w
                                        ; sub_13F77+17↑r ...
MASTER_PALETTE  db 0, 0, 0              ; DATA XREF: videoMode3+24↑o
                db 0, 0, 27h
                db 3, 25h, 3
                db 0, 1Bh, 1Bh
                db 20h, 0, 0
                db 1Bh, 0, 1Bh
                db 20h, 1Bh, 5
                db 26h, 26h, 26h
                db 10h, 10h, 10h
                db 0, 0, 38h
                db 0, 3Fh, 0
                db 0, 3Fh, 3Fh
                db 3Ah, 20h, 10h
                db 3Fh, 0, 3Fh
                db 3Fh, 3Fh, 0
                db 3Fh, 3Fh, 3Fh
_tempPalette    db 30h dup(0)           ; DATA XREF: videoMode3:loc_13EE5↑o
                                        ; videoMode3+44↑o
                db    0
                db    0
                db    1
                db    4
                db    4
                db    1
                db    4
                db    1
                db    8
                db    2
                db    2
                db    8
                db    8
                db    2
                db  0Ch
                db    3
                db    0
                db    0
                db  0Dh
                db    7
                db    5
                db    5
                db  0Dh
                db    7
                db  0Ah
                db  0Ah
                db  0Eh
                db  0Bh
                db  0Eh
                db  0Bh
                db  0Fh
                db  0Fh
aYouSeeMilesOfI db 0Dh,'You see miles of impassable ocean and you decide to turn back'
                db '.',0Dh,0
aThereAreRoadsA db 0Dh,'There are roads around here somewhere!',0Dh,0
aTryUsingTheRoa db 0Dh,'Try using the road, friend.',0Dh,0
aHic            db 0Dh,'            hic!',0Dh,0
                db    0
word_15A1E      dw 0                    ; DATA XREF: sub_119DA+37↑w
word_15A20      dw 0                    ; DATA XREF: sub_119DA+3B↑w
word_15A22      dw 0                    ; DATA XREF: sub_119DA+13↑w
                                        ; sub_119DA+2F↑r
word_15A24      dw 0                    ; DATA XREF: sub_119DA+17↑w
                                        ; sub_119DA+33↑r
_pictPtr        dw 0                    ; DATA XREF: sub_119DA+7↑r
                                        ; sub_119DA+4F↑w ...
word_15A28      dw 0                    ; DATA XREF: sub_119DA+2B↑r
                                        ; sub_119DA+5C↑w ...
byte_15A2A      db 0                    ; DATA XREF: sub_119DA+42↑w
                                        ; sub_119DA:loc_11A3F↑w ...
byte_15A2B      db 0                    ; DATA XREF: sub_119DA:loc_11A6D↑w
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
word_16A3C      dw 0                    ; DATA XREF: start+4A↑w
                                        ; sub_11FBC+8↑r ...
word_16A3E      dw 0                    ; DATA XREF: start+3E↑w
off_16A40       dw offset loc_115D8+1   ; DATA XREF: sg0138:2228↑w
                                        ; sub_11D34-19A↑r
word_16A42      dw 0                    ; DATA XREF: start+39↑w
                                        ; sub_11FBC+2↑r ...
                dw seg dseg
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
word_16A92      dw 1E12h                ; DATA XREF: sub_1318C:loc_131AB↑r
                                        ; sub_1318C+61↑w
aCFileInfo      db ';C_FILE_INFO',0
dword_16AA1     dd 0                    ; DATA XREF: sub_11B9E+C↑w
                                        ; __ctermsub+E↑r ...
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
word_16AAD      dw 0                    ; DATA XREF: sub_12024+26↑w
                                        ; __cenvarg+89↑w ...
word_16AAF      dw 0                    ; DATA XREF: sub_12DA9↑r
                db    0
                db    0
word_16AB3      dw 0                    ; DATA XREF: start+5E↑w
                                        ; sub_11B9E+26↑r ...
word_16AB5      dw 0                    ; DATA XREF: sub_11B9E+4↑w
                                        ; sg0138:2413↑r ...
                align 2
word_16AB8      dw 0                    ; DATA XREF: sub_12024↑w
                                        ; __cenvarg+8F↑w ...
word_16ABA      dw 14h                  ; DATA XREF: sub_12B7C+6↑r
                                        ; sub_12B9C+9↑r ...
byte_16ABC      db 81h                  ; DATA XREF: sub_11B9E:loc_11C36↑w
                                        ; _exit:loc_11CA6↑r
                db  81h
                db  81h
                db    1
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
; int argc
argc            dw 0                    ; DATA XREF: start+89↑r
                                        ; sg0138:24CA↑w
; char **argv
argv            dw 0                    ; DATA XREF: start+85↑r
                                        ; sg0138:24DC↑w
; char **word_16AD4
word_16AD4      dw 0                    ; DATA XREF: start+81↑r
                                        ; sub_11EFA+44↑w ...
word_16AD6      dw 1EAAh                ; DATA XREF: sg0138:2435↑w
                                        ; sg0138:24EA↑r
seg_16AD8       dw seg dseg             ; DATA XREF: sg0138:2422↑w
                db  43h ; C
                db    0
                db    0
                db    0
byte_16ADE      db 0                    ; DATA XREF: __ctermsub+18↑r
byte_16ADF      db 0                    ; DATA XREF: __ctermsub+20↑r
dword_16AE0     dd 0                    ; DATA XREF: __ctermsub+23↑r
word_16AE4      dw 0                    ; DATA XREF: __FF_MSGBANNER+A↑r
                                        ; __FF_MSGBANNER+11↑r
word_16AE6      dw 4170h                ; DATA XREF: sub_11D34+7↑r
                                        ; sub_12FC0+1↑r
word_16AE8      dw 0                    ; DATA XREF: sg0138:loc_11D6C↑w
                                        ; sg0138:2596↑r
                db    0
                db  16h
                db    2
                db    2
                db  18h
                db  0Dh
                db    9
                db  0Ch
                db  0Ch
                db  0Ch
                db    7
                db    8
                db  16h
                db  16h
                db 0FFh
                db  12h
                db  0Dh
                db  12h
                db    2
                db 0FFh
word_16AFE      dw 0                    ; DATA XREF: sub_12084+90↑w
                db 0E6h
                db  35h ; 5
                db    0
                db    0
                db 0E6h
                db  35h ; 5
                db    1
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    2
                db    1
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    2
                db    2
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db  84h
                db    3
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    2
                db    4
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    1
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
word_16C18      dw 1F68h                ; DATA XREF: sg0138:loc_12076↑r
aNull           db '(null)',0
aNull_0         db '(null)',0
                db '+- #',0
                db    0
                db    0
byte_16C2F      db 0                    ; DATA XREF: _sopen-75A↑r
word_16C30      dw 0                    ; DATA XREF: sub_12FE4+1F↑w
word_16C32      dw 0                    ; DATA XREF: sub_12FE4+22↑w
                db    0
                db    0
word_16C36      dw 0                    ; DATA XREF: sub_12FE4+32↑w
                db    0
                db    0
word_16C3A      dw 0                    ; DATA XREF: sub_1302D+A2↑r
                db    0
                db    0
word_16C3E      dw 0                    ; DATA XREF: sub_1302D+4F↑w
                                        ; sub_1302D+D6↑w
word_16C40      dw 2000h                ; DATA XREF: sub_13110:loc_13120↑r
                                        ; __cenvarg:loc_134FC↑r ...
                align 4
byte_16C44      db 0                    ; DATA XREF: sub_1302D:loc_13085↑w
                                        ; sub_1302D:loc_130C0↑w
                align 2
off_16C46       dw offset writeStringAt ; DATA XREF: sub_127F0+53↑r
off_16C48       dw offset writeStringAt ; DATA XREF: sub_127F0+6B↑r
                db  2Eh ; .
                db  1Dh
off_16C4C       dw offset writeStringAt ; DATA XREF: sub_127F0+84↑r
off_16C4E       dw offset writeStringAt ; DATA XREF: sub_127F0+A2↑r
word_16C50      dw 0                    ; DATA XREF: __doexec+16C↑r
                                        ; __doexec+178↑r
word_16C52      dw 0                    ; DATA XREF: __doexec+174↑r
                db    0
                db    0
                db    0
ascii_flags     db 20h                  ; DATA XREF: removePartyMember+4C↑r
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  28h ; (
                db  28h ; (
                db  28h ; (
                db  28h ; (
                db  28h ; (
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  20h
                db  48h ; H
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  84h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  81h
                db  81h
                db  81h
                db  81h
                db  81h
                db  81h
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db    1
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  10h
                db  82h
                db  82h
                db  82h
                db  82h
                db  82h
                db  82h
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db    2
                db  10h
                db  10h
                db  10h
                db  10h
                db  20h
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
word_16D58      dw 0FFFFh               ; DATA XREF: _kbhit↑r
                                        ; _getch:loc_13322↑r ...
aCFileInfo_0    db ';C_FILE_INFO',0
                align 2
                db    2
                db    0
word_16D6A      dw 0                    ; DATA XREF: __doexec+40↑w
                                        ; __doexec+10F↑r
word_16D6C      dw 0                    ; DATA XREF: __doexec+60↑w
                                        ; __doexec:loc_13851↑r ...
word_16D6E      dw 0                    ; DATA XREF: __doexec+78↑w
                                        ; __doexec+20B↑r
unk_16D70       db    0                 ; DATA XREF: __doexec+2A7↑r
                db    0
word_16D72      dw 0                    ; DATA XREF: __doexec+1E5↑w
aCom            db '.com',0
aExe            db '.exe',0
                align 4
unk_16D80       db    0                 ; DATA XREF: sub_11B9E+37↑r
                                        ; sub_11B9E+4D↑r ...
                db    0
word_16D82      dw 0                    ; DATA XREF: sub_11B9E+20↑r
                                        ; __ctermsub↑r
dword_16D84     dd 0                    ; DATA XREF: sub_11B9E+2F↑r
dword_16D88     dd 0                    ; DATA XREF: sub_11B9E:loc_11BE1↑r
                db  52h ; R
                db  20h
                db  14h
aNmsg           db ':<<NMSG>>',0
                db    0
aR6000StackOver db 'R6000',0Dh,0Ah
                db '- stack overflow',0Dh,0Ah,0
                db    3
                db    0
aR6003IntegerDi db 'R6003',0Dh,0Ah
                db '- integer divide by 0',0Dh,0Ah,0
                db    9
                db    0
aR6009NotEnough db 'R6009',0Dh,0Ah
                db '- not enough space for environment',0Dh,0Ah,0
                db 0FCh
                db    0
                db  0Dh
                db  0Ah
                db    0
                db 0FFh
                db    0
aRunTimeError   db 'run-time error ',0
                db    2
                db    0
aR6002FloatingP db 'R6002',0Dh,0Ah
                db '- floating point not loaded',0Dh,0Ah,0
                db    1
                db    0
aR6001NullPoint db 'R6001',0Dh,0Ah
                db '- null pointer assignment',0Dh,0Ah,0
                db    5
                db    0
aRunTimeErrorR6 db 0Dh,0Ah
                db 'run-time error R6005',0Dh,0Ah
                db '- not enough memory on exec',0Dh,0Ah
                db '$',0
                db    6
                db    0
aRunTimeErrorR6_0 db 0Dh,0Ah
                db 'run-time error R6006',0Dh,0Ah
                db '- bad format on exec',0Dh,0Ah
                db '$',0
                db    7
                db    0
aRunTimeErrorR6_1 db 0Dh,0Ah
                db 'run-time error R6007',0Dh,0Ah
                db '- bad environment on exec',0Dh,0Ah
                db '$',0
                db 0FFh
                db 0FFh
                db 0FFh
                db    0
word_16F0C      dw 0                    ; DATA XREF: sub_122B4+55↑w
                                        ; sub_122B4:loc_12348↑w ...
; FILE *word_16F0E
word_16F0E      dw 0                    ; DATA XREF: sub_122B4+1E↑w
                                        ; sub_122B4+2BB↑r ...
word_16F10      dw ?                    ; DATA XREF: sub_122B4+52↑w
                                        ; sub_122B4+1AC↑w ...
word_16F12      dw ?                    ; DATA XREF: sub_122B4+43↑w
                                        ; sub_122B4:loc_1240E↑w ...
word_16F14      dw ?                    ; DATA XREF: sub_122B4+49↑w
                                        ; sub_122B4+111↑w ...
word_16F16      dw ?                    ; DATA XREF: sub_122B4+40↑w
                                        ; sub_122B4+75↑w ...
word_16F18      dw ?                    ; DATA XREF: sub_122B4+58↑w
                                        ; sub_122B4+9F↑w ...
word_16F1A      dw ?                    ; DATA XREF: sub_122B4+18↑w
                                        ; sub_122B4+17A↑r ...
word_16F1C      dw ?                    ; DATA XREF: sub_122B4+4F↑w
                                        ; sub_122B4+79↑w ...
word_16F1E      dw ?                    ; DATA XREF: sub_122B4+4C↑w
                                        ; sub_122B4+D7↑w ...
word_16F20      dw ?                    ; DATA XREF: sub_122B4+46↑w
                                        ; sub_122B4+18E↑w ...
word_16F22      dw ?                    ; DATA XREF: sub_122B4+27↑w
                                        ; sub_122B4+180↑r ...
word_16F24      dw ?                    ; DATA XREF: sub_122B4+21↑w
                                        ; sub_122B4:loc_125AA↑r ...
; int t
t               dw ?                    ; DATA XREF: sub_122B4:loc_122EC↑w
                                        ; sub_122B4+E9↑r ...
word_16F28      dw ?                    ; DATA XREF: sub_122B4+1BB↑w
                                        ; sub_122B4:loc_12478↑w ...
; char *word_16F2A
word_16F2A      dw ?                    ; DATA XREF: sub_122B4+12↑w
                                        ; sub_125D2+79↑r ...
word_16F2C      dw ?                    ; DATA XREF: sub_122B4+BF↑r
                                        ; sub_122B4+CA↑r ...
word_16F2E      dw ?                    ; DATA XREF: sub_125D2:loc_12648↑w
                                        ; sub_127F0+90↑w ...
; int word_16F30
word_16F30      dw ?                    ; DATA XREF: sub_122B4+5B↑w
                                        ; sub_122B4+68↑w ...
                align 10h
_welcomePotraitId dw ?                  ; DATA XREF: displayWelcomePortrait+6↑w
                                        ; sub_104EA+29↑r ...
word_16F42      dw ?                    ; DATA XREF: displayWelcomePortrait+8A↑w
                                        ; sub_104EA+13↑r
word_16F44      dw ?                    ; DATA XREF: displayWelcomePortrait+71↑w
                                        ; sub_104EA+6↑r
word_16F46      dw ?                    ; DATA XREF: displayWelcomePortrait+74↑w
                                        ; sub_104EA+9↑r
                db    ? ;
                db    ? ;
byte_16F4A      db ?                    ; DATA XREF: displayWelcomePortrait+8D↑w
                align 2
_welcomeImageLoaded dw ?                ; DATA XREF: displayWelcomePortrait:loc_104E0↑w
                                        ; _main+38↑w
_disk1          Disk1 <?>               ; DATA XREF: loadDisk1+4F↑r
                                        ; sub_103E0+6↑w ...
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
_profession     dw ?                    ; DATA XREF: displaySkills+11↑r
                                        ; sub_10B41+7↑r ...
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
_xPos           dw ?                    ; DATA XREF: writeString?+9↑r
                                        ; writeString?:loc_11516↑w ...
_textY          dw ?                    ; DATA XREF: writeString?+D↑r
                                        ; writeString?+11↑w ...
word_18214      dw ?                    ; DATA XREF: displayWelcomePortrait+C↑r
                                        ; displayWelcomePortrait+17↑w ...
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
; void far *professionsData
_professionsData dw ?                   ; DATA XREF: loadClasses+7B↑w
                                        ; loadClasses+91↑r ...
                db 126h dup(?)
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
unk_1869E       db    ? ;               ; DATA XREF: displaySkills+BC↑o
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
; Profession *professions
_professions    dw ?                    ; DATA XREF: loadClasses+B6↑w
                                        ; listParty+77↑r ...
; void far *fontData
_fontData       dw ?                    ; DATA XREF: fontOpen+3F↑w
                                        ; fontOpen+53↑r ...
; char sprint_buffer[]
sprint_buffer   db 100h dup(?)          ; DATA XREF: listParty+38↑o
                                        ; listParty+5B↑o ...
_party          dw 5 dup(?)             ; DATA XREF: listParty+28↑r
                                        ; sub_10751+15↑r ...
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
; void far *dst
dst             dw ?                    ; DATA XREF: loadClasses+29↑r
                                        ; _main+26↑w ...
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
                db    ? ;
dseg            ends

; ===========================================================================

; Segment type: Uninitialized
seg003          segment byte stack 'STACK' use16
                assume cs:seg003
                assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing
                db 800h dup(?)
seg003          ends


                end start
