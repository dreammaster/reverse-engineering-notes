
FILE struc ; (sizeof=0xC, align=0x2, mappedto_1) ; XREF: _sprintf/r
_ptr dd ?               ; offset
_cnt dw ?
_base dd ?              ; offset
_flag db ?              ; XREF: _sprintf+10/w
_file db ?
FILE ends


Window struc ; (sizeof=0x1C, mappedto_3) ; XREF: dseg:_windowInfo/r
                        ; dseg:_window1/r ...
_x1 dw ?                ; XREF: writeStringAt+17/r ; base 10
_y1 dw ?                ; XREF: writeStringAt+26/r ; base 10
_x2 dw ?                ; base 10
_y2 dw ?                ; base 10
_textX dw ?             ; XREF: writeStringAt+1B/w writeChar+20/r ; base 10
_textY dw ?             ; XREF: writeStringAt+2B/w writeChar+C/r ; base 10
_attr dw ?              ; XREF: writeParty+56/o winClearCurrent+4/o ...
field_E dw ?
field_10 dd ?           ; XREF: writeParty+26/r
                        ; writeShortMemberName+1F/r ...
field_14 dw ?           ; XREF: characterInfoStats+148/w
                        ; characterInfoStats+161/w ...
_fnDraw dw ?            ; XREF: setWindow+22/r setWindow+28/r ; base 16
field_18 dw ?           ; XREF: setWindow+19/w
field_1A dw ?           ; XREF: sub_1DBFD+4/r sub_1DC0B/r ...
Window ends


AttributeText struc ; (sizeof=0x4, mappedto_4)
_x db ?                 ; base 10
_y db ?                 ; base 10
_text dw ?              ; offset
AttributeText ends


Bounds struc ; (sizeof=0x8, mappedto_5)
_x dw ?                 ; base 10
_y dw ?                 ; base 10
_w dw ?                 ; base 10
_h dw ?                 ; base 10
Bounds ends


DirtyRegion struc ; (sizeof=0xE, mappedto_6)
_dest dw ?
_x1 dw ?
_src dw ?
_blitRowFn dw ?
_h dw ?
field_A dw ?
_w dw ?
DirtyRegion ends


Disk1 struc ; (sizeof=0xEC0, mappedto_9) ; XREF: sub_101BE+2A/o
                        ; dseg:_disk1/r
field_0 dw ?            ; XREF: sub_18827+26/w
field_2 dw ?            ; XREF: sub_18827+29/w
field_4 dw ?            ; XREF: sub_101BE+48/r sub_101BE+59/r ...
_gfxMode db ?           ; XREF: sub_101BE+42/r
field_7 db ?            ; XREF: sub_16D96+17/w sub_16DB4+19/w ...
_timeHours db ?         ; XREF: writeTime+12/r setTime+21/r ...
_timeMinutes db ?       ; XREF: writeTime+C/r setTime+34/w ...
_mapPosX dw ?           ; XREF: sub_1008C+10D/r sub_1073E+15/r ...
_mapPosY dw ?           ; XREF: sub_1008C+109/r sub_1073E+D/r ...
_cash dd ?              ; XREF: sub_17FA8+11E/w sub_17FA8+122/w ...
_maps db 10 dup(?)      ; XREF: _main+16/r
_mapsX db 10 dup(?)
_mapsY db 10 dup(?)
_mapIndex db ?          ; XREF: _main+10/r sub_18C32+7/r ...
_partyCount db ?        ; XREF: sub_101BE:loc_103AB/r
                        ; sub_107E1:loc_109A0/r ...
_partyIndexes db 5 dup(?)
_timeWeekday db ?       ; XREF: writeTime+38/r setTime+6D/w ...
field_38 dw ?           ; XREF: sub_101BE+1F7/r sub_18827+12/w
_roster PartyMember 5 dup(?)
field_6B6 db 250 dup(?)
field_7B0 db 5 dup(?)
db ? ; undefined
field_7B6 dw ?
field_7B8 db 1799 dup(?)
field_EBF db ?
Disk1 ends


PartyMember struc ; (sizeof=0x14C, mappedto_8) ; XREF: Disk1/r
_name db 24 dup(?)
_attributes db 7 dup(?)
field_1F db ?
db ? ; undefined
db ? ; undefined
db ? ; undefined
_pointsRemaining db ?
_activeSkills db 16 dup(?)
_passiveSkills db 16 dup(?)
_con dw ?               ; XREF: getMemberCon+E/r getMemberCon+14/r ...
_conBase dw ?           ; XREF: characterInfoStats+A9/r
                        ; characterInfoStats+B7/r
_profession db ?        ; XREF: getArmorClass+4F/r
field_49 db ?
field_4A db ?
_array2 db 3 dup(?)     ; XREF: getArmorClass+19/r
_rank db ?              ; XREF: characterInfoStats+36/r
field_4F db ?
_sex db ?
field_51 db ?
field_52 db ?
field_53 db ?
field_54 db ?
field_55 db ?
field_56 db ?
field_57 db ?
_afflicted db ?         ; XREF: writeParty+4D/r
                        ; writeMemberCondition+15/r ...
field_59 db ?
field_5A dw ?
field_5C dw ?
field_5E dw ?
field_60 dw ?
_array1 MemberArray1Entry 32 dup(?) ; XREF: getArmorClass+2C/r
_conTemp dw ?
_activeSkills2 db 16 dup(?)
_passiveSkills2 db 16 dup(?)
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
field_14B db ?
PartyMember ends


SkillName struc ; (sizeof=0x16, mappedto_10)
_text db 22 dup(?)
SkillName ends


MemberArray1Entry struc ; (sizeof=0x6, mappedto_7) ; XREF: PartyMember/r
field_0 db ?            ; XREF: getArmorClass+2C/r
field_1 db ?
field_2 db ?
field_3 db ?
field_4 db ?
field_5 db ?
MemberArray1Entry ends


Profession struc ; (sizeof=0x80, mappedto_11)
_name db 16 dup(?)
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
_attributes db 7 dup(?)
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
_attributePoints db ?
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
_activeSkills db 16 dup(?)
_passiveSkills db 16 dup(?)
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
_unkMin dw ?
_unkMax dw ?
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
db ? ; undefined
field_7A db ?           ; XREF: getArmorClass+5E/r
field_7B db ?
db ? ; undefined
db ? ; undefined
db ? ; undefined
field_7F db ?
Profession ends


;
; +-------------------------------------------------------------------------+
; |      This file was generated by The Interactive Disassembler (IDA)      |
; |           Copyright (c) 2023 Hex-Rays, <support@hex-rays.com>           |
; |                      License info: 48-337F-7354-97                      |
; |                       Paul Gilbert, ScummVM Team                        |
; +-------------------------------------------------------------------------+
;
; Input SHA256 : F805616617609985A81B77E1EAB16BB1DA6D3AF83A08FB1B6DD35EF427675E32
; Input MD5    : C184FEDEBE17399D065610DAC50A4F18
; Input CRC32  : CB5F9E84

; File Name   : C:\games\fod\KEH.EXE
; Format      : MS-DOS executable (EXE)
; Base Address: 1000h Range: 10000h-2F830h Loaded length: 1D0D0h
; Entry Point : 1000:E688
; OS type         :  MS DOS
; Application type:  Executable 16bit

.686p
.mmx
.model large


; Segment type: Pure code
sg01a2 segment byte public 'CODE' use16
assume cs:sg01a2
assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing
db 10h dup(0)


; Attributes: noreturn

error proc near
call    restoreCriticalFn
call    sub_20720
sub     ax, ax
push    ax              ; int
call    _exit
error endp

add     sp, 2
retn


; Attributes: bp-based frame

sub_10022 proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
mov     word_2EC12, 0

loc_1002B:
mov     bx, word_2EC12
shl     bx, 1
mov     word ptr [bx-2438h], 0FFFFh
inc     word_2EC12
cmp     word_2EC12, 3
jl      short loc_1002B
sub     ax, ax
push    ax
mov     ax, 0D944h
push    ax
mov     al, [bp+arg_0]
sub     ah, ah
dec     ax
push    ax
push    word ptr dst+2
push    word ptr dst
push    word_2EBFC
call    sub_103BF
add     sp, 0Ch
les     bx, dst
mov     ax, es:[bx]
mov     word_2EE16, ax
push    word_22C74
push    word_22C72
push    es
push    bx
call    sub_1E5BD
add     sp, 8
mov     ax, word_22C72
mov     dx, word_22C74
mov     word ptr dword_2EE12, ax
mov     word ptr dword_2EE12+2, dx
pop     bp
retn
sub_10022 endp



; Attributes: bp-based frame

sub_1008C proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
push    si
mov     al, byte_2132A
cmp     [bp+arg_0], al
jnz     short loc_1009B
jmp     loc_101BB

loc_1009B:
cbw
push    ax
call    sub_187E5
add     sp, 2
mov     al, [bp+arg_0]
mov     byte_2132A, al
mov     bl, al
sub     bh, bh
mov     al, [bx+125h]
sub     ah, ah
mov     si, ax
cmp     word_2132C, si
jz      short loc_10118
mov     word_2132C, si
mov     ax, 0Ah
imul    si
mov     si, ax
mov     ax, [si+0F6h]
mov     word_2E4C4, ax
mov     ax, [si+0F8h]
mov     word_2EBFC, ax
mov     ax, [si+0FAh]
mov     word_2EC08, ax
mov     ax, [si+0F4h]
mov     word_2DFEC, ax
mov     ax, 1Ch
push    ax
mov     ax, 0D20Ah
push    ax
push    word_2E4C4
call    sub_10444
add     sp, 6
mov     ax, 1Ch
push    ax
mov     ax, 0D944h
push    ax
push    word_2EBFC
call    sub_10444
add     sp, 6
mov     ax, 35h ; '5'
push    ax
mov     ax, 0DAF2h
push    ax
push    word_2EC08
call    sub_10444
add     sp, 6

loc_10118:
sub     ax, ax
push    ax
mov     ax, 0D20Ah
push    ax
mov     al, [bp+arg_0]
sub     ah, ah
dec     ax
push    ax
push    word ptr dword_22C6E+2
push    word ptr dword_22C6E
push    word_2E4C4
call    sub_103BF
add     sp, 0Ch
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    sub_204C4
add     sp, 2
les     bx, dword_22C6E
mov     ax, es:[bx+404h]
add     ax, bx
mov     dx, es
mov     word_2EBF8, ax
mov     word_2EBFA, dx
mov     ax, es:[bx+406h]
add     ax, bx
mov     word ptr dword_2E446, ax
mov     word ptr dword_2E446+2, dx
mov     ax, es:[bx+408h]
add     ax, bx
mov     word_2EC14, ax
mov     word_2EC16, dx
mov     ax, es:[bx+40Ah]
add     ax, bx
mov     word ptr dword_2E41E, ax
mov     word ptr dword_2E41E+2, dx
mov     ax, es:[bx+40Ch]
add     ax, bx
mov     word ptr dword_2EC1A, ax
mov     word ptr dword_2EC1A+2, dx
mov     ax, es:[bx]
mul     _disk1._mapPosY
add     ax, _disk1._mapPosX
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    sub_10022
add     sp, 2

loc_101BB:
pop     si
pop     bp
retn
sub_1008C endp



; Attributes: bp-based frame

sub_101BE proc near

var_A= word ptr -0Ah
var_8= word ptr -8
dst= word ptr -6
var_4= word ptr -4
handle= word ptr -2

push    bp
mov     bp, sp
sub     sp, 0Ah
sub     ax, ax
mov     [bp+var_4], ax
mov     [bp+dst], ax
mov     ax, word_2132C
mov     [bp+var_8], ax
mov     ax, 8000h
push    ax              ; mode
mov     ax, offset aDisk1_1 ; "DISK1"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
mov     ax, offset _disk1
push    ds
push    ax              ; dst
mov     ax, size Disk1
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
mov     al, _disk1._gfxMode
mov     byte_2B166, al
mov     al, byte ptr _disk1.field_4
sub     ah, ah
or      ax, ax
jz      short loc_1022B
cmp     ax, 1
jz      short loc_10269

loc_10214:
call    sub_1D9A9
mov     bl, byte ptr _disk1.field_4
sub     bh, bh
shl     bx, 1
shl     bx, 1
mov     ax, [bx+11Ah]
mov     word_2132C, ax
jmp     loc_102CC

loc_1022B:
mov     [bp+var_A], 1
jmp     short loc_10235

loc_10232:
inc     [bp+var_A]

loc_10235:
cmp     [bp+var_A], 4
jge     short loc_10214
mov     ax, offset aDisk1_2 ; "Disk 1"
push    ax
mov     ax, 0Ah
imul    [bp+var_A]
mov     bx, ax
push    off_21332[bx]   ; char *
call    _strcpy
add     sp, 4
mov     ax, offset aDisk1_3 ; "Disk1"
push    ax
mov     ax, 0Ah
imul    [bp+var_A]
mov     bx, ax
push    word ptr [bx+0F4h] ; char *
call    _strcpy
add     sp, 4
jmp     short loc_10232

loc_10269:
mov     [bp+var_A], 11h

loc_1026E:
mov     bx, [bp+var_A]
mov     byte ptr [bx+126h], 0
inc     [bp+var_A]
cmp     [bp+var_A], 1Bh
jl      short loc_1026E
mov     ax, offset aDisk1_4 ; "Disk 1"
push    ax
push    off_2133C       ; char *
call    _strcpy
add     sp, 4
mov     ax, offset aDisk2_1 ; "Disk 2"
push    ax
push    off_21346       ; char *
call    _strcpy
add     sp, 4
mov     ax, offset aDisk2_2 ; "Disk 2"
push    ax
push    off_21350       ; char *
call    _strcpy
add     sp, 4
mov     ax, offset aDisk1_5 ; "Disk1"
push    ax
push    off_2133E       ; char *
call    _strcpy
add     sp, 4
mov     ax, offset aDisk3_1 ; "Disk3"
push    ax
push    off_21352       ; char *
call    _strcpy
add     sp, 4
jmp     loc_10214

loc_102C8:
inc     word_2132C

loc_102CC:
mov     bl, byte ptr _disk1.field_4
sub     bh, bh
shl     bx, 1
shl     bx, 1
mov     ax, word_2132C
cmp     [bx+11Ch], ax
jle     short loc_1033A
mov     ax, 0Ah
imul    word_2132C
mov     bx, ax
push    word ptr [bx+0F4h]
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
lea     ax, [bp+dst]
push    ss
push    ax              ; dst
mov     ax, 4
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
mov     ax, word_2132E
mov     dx, word_21330
cmp     [bp+var_4], dx
jb      short loc_102C8
ja      short loc_10325
cmp     [bp+dst], ax
jbe     short loc_102C8

loc_10325:
mov     ax, [bp+dst]
mov     dx, [bp+var_4]
mov     word_2132E, ax
mov     word_21330, dx
mov     ax, word_2132C
mov     [bp+var_8], ax
jmp     short loc_102C8

loc_1033A:
mov     ax, [bp+var_8]
mov     word_2132C, ax
add     word_2132E, 1
adc     word_21330, 0
cmp     byte ptr _disk1.field_4, 0
jz      short loc_10384
mov     ax, 0Ah
imul    word_2132C
mov     bx, ax
push    word ptr [bx+0F4h]
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
mov     ax, 0BEEAh
push    ds
push    ax              ; dst
mov     ax, 0EC0h
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2

loc_10384:
mov     word_2132C, 0FFFFh
mov     [bp+var_A], 0
jmp     short loc_103AB

loc_10391:
mov     bx, [bp+var_A]
mov     al, [bx-40E4h]
sub     ah, ah
mov     cx, 14Ch
mul     cx
add     ax, 0BF24h
shl     bx, 1
mov     [bx-2518h], ax
inc     [bp+var_A]

loc_103AB:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A], ax
jb      short loc_10391
mov     ax, _disk1.field_38
mov     word_22C86, ax
mov     sp, bp
pop     bp
retn
sub_101BE endp



; Attributes: bp-based frame

sub_103BF proc near

handle= word ptr -2
arg_0= word ptr  4
dst= dword ptr  6
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch
arg_A= word ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 2
push    si
push    [bp+arg_0]
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
sub     ax, ax
push    ax
mov     bx, [bp+arg_6]
shl     bx, 1
shl     bx, 1
mov     si, [bp+arg_8]
mov     ax, [bx+si]
sub     dx, dx
mov     cl, 4
call    __aNlshl
push    dx
push    ax
push    [bp+handle]
call    sub_1E498
add     sp, 8
cmp     [bp+arg_A], 1
jnz     short loc_10417
push    word ptr [bp+dst+2]
push    word ptr [bp+dst]
mov     si, [bp+arg_6]
shl     si, 1
shl     si, 1
mov     bx, [bp+arg_8]
sub     ax, ax
push    ax
push    word ptr [bx+si+2]
push    [bp+handle]
call    sub_1E448
jmp     short loc_10433

loc_10417:
push    word ptr [bp+dst+2]
push    word ptr [bp+dst] ; dst
mov     si, [bp+arg_6]
shl     si, 1
shl     si, 1
mov     bx, [bp+arg_8]
sub     ax, ax
push    ax
push    word ptr [bx+si+2] ; size
push    [bp+handle]     ; handle
call    fileRead

loc_10433:
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
pop     si
mov     sp, bp
pop     bp
retn
sub_103BF endp



; Attributes: bp-based frame

sub_10444 proc near

handle= word ptr -2
arg_0= word ptr  4
dst= dword ptr  6

push    bp
mov     bp, sp
sub     sp, 2
push    [bp+arg_0]
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
push    ds
push    word ptr [bp+dst] ; dst
mov     ax, word ptr [bp+dst+2]
shl     ax, 1
shl     ax, 1
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
mov     sp, bp
pop     bp
retn
sub_10444 endp



; Attributes: bp-based frame

loadData proc near

handle= word ptr -8
size= dword ptr -6
numChars= word ptr -2

push    bp
mov     bp, sp
sub     sp, 8
sub     ax, ax
push    ax              ; mode
mov     ax, 21Ch
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
push    ax              ; handle
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    word ptr dst+2
push    word ptr dst    ; dst
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
push    word ptr _picDecoded+2
push    word ptr _picDecoded
push    word ptr dst+2
push    word ptr dst
call    sub_1E5BD
add     sp, 8
sub     ax, ax
push    ax              ; mode
mov     ax, offset filename ; "FONT"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
;
lea     ax, [bp+numChars]
push    ss
push    ax              ; dst
mov     ax, 2
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    word ptr _fontData+2
push    word ptr _fontData ; dst
mov     ax, [bp+numChars]
cwd
mov     cl, 5
call    __aNlshl
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
;
sub     ax, ax
push    ax              ; mode
mov     ax, offset aBorders ; "BORDERS"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
push    word ptr _borders+2
push    word ptr _borders ; dst
mov     ax, 5000
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
sub     ax, ax
push    ax              ; mode
mov     ax, offset aPackets ; "PACKETS"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
push    ax              ; handle
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    ax
call    unknown_libname_2 ; MS Quick C v1.0/v2.01 & MSC v5.1 DOS run-time & graphic
add     sp, 2
mov     _packetsSize, ax
or      ax, ax
jnz     short loc_1057F

loc_10570:              ; handle
push    [bp+handle]
call    fileClose
add     sp, 2
call    error
jmp     loc_1073A

loc_1057F:
push    ds
push    _packetsSize    ; dst
push    word ptr [bp+size+2]
push    word ptr [bp+size] ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
sub     ax, ax
push    ax              ; mode
mov     ax, offset aArchtype ; "ARCHTYPE"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
or      ax, ax
jnz     short loc_105B3
call    error

loc_105B3:              ; handle
push    [bp+handle]
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    word ptr _archetypesData+2
push    word ptr _archetypesData ; dst
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
les     bx, _archetypesData
mov     ax, es:[bx+4]
add     ax, bx
mov     dx, es
mov     word ptr _professions, ax
mov     word ptr _professions+2, dx
mov     ax, es:[bx+6]
add     ax, bx
mov     word ptr _enemies, ax
mov     word ptr _enemies+2, dx
sub     ax, ax
push    ax              ; mode
mov     ax, offset aServices ; "SERVICES"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
or      ax, ax
jnz     short loc_10615
jmp     loc_1073A

loc_10615:              ; handle
push    ax
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    word ptr _services+2
push    word ptr _services ; dst
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
sub     ax, ax
push    ax              ; mode
mov     ax, offset aHdspct ; "HDSPCT"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
or      ax, ax
jnz     short loc_10655
call    error

loc_10655:
mov     word ptr [bp+size], 1180
mov     word ptr [bp+size+2], 0
push    word ptr _hdsPct+2
push    word ptr _hdsPct ; dst
mov     ax, 1180
cwd
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
sub     ax, ax
push    ax              ; mode
mov     ax, offset aWeapons ; "WEAPONS"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
or      ax, ax
jnz     short loc_10696
call    error

loc_10696:              ; handle
push    [bp+handle]
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    ax
call    unknown_libname_2 ; MS Quick C v1.0/v2.01 & MSC v5.1 DOS run-time & graphic
add     sp, 2
mov     _weapons, ax
or      ax, ax
jnz     short loc_106BF
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
call    error

loc_106BF:
push    ds
push    _weapons        ; dst
push    word ptr [bp+size+2]
push    word ptr [bp+size] ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
sub     ax, ax
push    ax              ; mode
mov     ax, offset aGlobals ; "GLOBALS"
push    ax              ; filename
call    fileOpen
add     sp, 4
mov     [bp+handle], ax
or      ax, ax
jnz     short loc_106F3
call    error

loc_106F3:              ; handle
push    [bp+handle]
call    fileGetSize
add     sp, 2
mov     word ptr [bp+size], ax
mov     word ptr [bp+size+2], dx
push    ax
call    unknown_libname_2 ; MS Quick C v1.0/v2.01 & MSC v5.1 DOS run-time & graphic
add     sp, 2
mov     _globals, ax
or      ax, ax
jnz     short loc_10713
jmp     loc_10570

loc_10713:
push    ds
push    ax              ; dst
push    word ptr [bp+size+2]
push    word ptr [bp+size] ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
mov     bx, _globals
mov     ax, [bx+4]
imul    word ptr [bx+6]
mov     word_2E426, ax

loc_1073A:
mov     sp, bp
pop     bp
retn
loadData endp



; Attributes: bp-based frame

sub_1073E proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     al, [bp+arg_0]
cbw
mov     si, ax
mov     ax, _disk1._mapPosY
sub     ax, si
mov     [bp+var_8], ax
mov     ax, _disk1._mapPosX
sub     ax, si
mov     [bp+var_6], ax
mov     ax, _disk1._mapPosY
add     ax, si
mov     [bp+var_4], ax
mov     ax, _disk1._mapPosX
add     ax, si
mov     [bp+var_2], ax
mov     al, [bp+arg_2]
cbw
cmp     ax, [bp+var_6]
jl      short loc_1078E
cbw
cmp     ax, [bp+var_2]
jg      short loc_1078E
mov     al, [bp+arg_4]
cbw
cmp     ax, [bp+var_8]
jl      short loc_1078E
cbw
cmp     ax, [bp+var_4]
jg      short loc_1078E
mov     ax, 1
jmp     short loc_10790

loc_1078E:
sub     ax, ax

loc_10790:
pop     si
mov     sp, bp
pop     bp
retn
sub_1073E endp



; Attributes: bp-based frame

sub_10795 proc near

var_2= byte ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 1
jmp     short loc_107A4

loc_107A1:
inc     [bp+var_2]

loc_107A4:
cmp     [bp+var_2], 40h ; '@'
jge     short loc_107C3
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
mov     al, [bp+var_2]
cbw
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_107A1

loc_107C3:
mov     al, [bp+var_2]
cbw
mov     sp, bp
pop     bp
retn
sub_10795 endp




sub_107CB proc near
cmp     byte_2EE06, 0
jz      short locret_107E0
push    _disk1._mapPosY
push    _disk1._mapPosX
call    sub_17FA8
add     sp, 4

locret_107E0:
retn
sub_107CB endp



; Attributes: bp-based frame

sub_107E1 proc near

var_10= word ptr -10h
member= word ptr -0Eh
y= word ptr -0Ah
var_8= word ptr -8
win= dword ptr -6
var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 10h
push    si              ; draw
mov     ax, _currentWindow
mov     word ptr [bp+win+2], ax
sub     ax, ax
push    ax
mov     ax, offset _window1
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     [bp+y], 0
jmp     loc_109A0

loc_10806:
sub     ax, ax

loc_10808:
mov     [bp+var_8], ax
mov     bx, [bp+y]
cmp     byte ptr [bx+197Ah], 0FFh
jnz     short loc_1081D
mov     bx, [bp+member]
mov     al, [bx+4Ah]
jmp     short loc_10824

loc_1081D:
mov     bx, [bp+member]
mov     al, [bx+145h]

loc_10824:
sub     ah, ah
mov     [bp+var_10], ax
cmp     ax, 0FFh
jz      short loc_10844
mov     ax, 6
imul    [bp+var_10]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
jmp     short loc_10847

loc_10844:
mov     ax, 260h

loc_10847:
mov     word ptr [bp+win], ax
mov     ax, [bp+y]
inc     ax
push    ax              ; char *
mov     ax, offset aF1d ; "F%1d>"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
cmp     [bp+arg_2], 0
jz      short loc_10870
mov     ax, [bp+y]
cmp     [bp+arg_0], ax
jnz     short loc_10870
mov     _inverseText, 0FFh

loc_10870:              ; y
push    [bp+y]
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
mov     bx, [bp+y]
cmp     byte ptr [bx+197Ah], 0FFh
jz      short loc_1089D
push    word ptr [bp+win]
push    bx
call    sub_169E8
add     sp, 4
jmp     loc_1099D

loc_1089D:
push    [bp+var_8]
push    [bp+var_2]
push    [bp+member]     ; char *
mov     ax, offset a1212s21d31d ; "%-12.12s %2.1d %3.1d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
push    [bp+y]          ; y
mov     ax, 3
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+member]
push    word ptr [bx+46h] ; char *
mov     ax, offset a31d ; "%3.1d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     bx, [bp+member]
cmp     byte ptr [bx+58h], 0
jz      short loc_108E8
mov     _inverseText, 0FFh

loc_108E8:              ; y
push    [bp+y]
mov     ax, 23
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
push    [bp+member]     ; member
call    getMemberCon
add     sp, 2
mov     [bp+member+2], ax
or      ax, ax
jle     short loc_10932
mov     _inverseText, 0FFh
push    [bp+y]          ; y
mov     ax, 1Bh
push    ax              ; x
mov     bx, [bp+member+2]
shl     bx, 1
push    ds
push    off_21410[bx]   ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
jmp     short loc_10958

loc_10932:
mov     bx, [bp+member]
push    word ptr [bx+44h] ; char *
mov     ax, offset a31d_0 ; "%3.1d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
push    [bp+y]          ; y
mov     ax, 1Bh
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_10958:
cmp     [bp+var_10], 0FFh
jz      short loc_10975
mov     ax, 6
imul    [bp+var_10]
mov     si, ax
mov     bx, [bp+member]
test    byte ptr [bx+si+65h], 1
jz      short loc_10975
mov     _inverseText, 0FFh

loc_10975:              ; char *
push    word ptr [bp+win]
mov     ax, offset a77s ; "%-7.7s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
push    [bp+y]          ; y
mov     ax, 1Fh
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0

loc_1099D:
inc     [bp+y]

loc_109A0:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+y], ax
jnb     short loc_109DB
mov     bx, [bp+y]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
push    ax              ; member
call    getArmorClass
add     sp, 2
mov     [bp+var_2], ax
mov     bx, [bp+member]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_109CC
jmp     loc_10806

loc_109CC:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+64h]
sub     ah, ah
jmp     loc_10808

loc_109DB:
cmp     [bp+arg_4], 0
jz      short loc_109E6
call    sub_1DC28
jmp     short loc_109F3

loc_109E6:
mov     ax, _currentWindow
add     ax, 0Ch
push    ax
call    sub_1D9F5
add     sp, 2

loc_109F3:
sub     ax, ax
push    ax
push    word ptr [bp+win+2] ; win
call    setWindow
add     sp, 4
pop     si
mov     sp, bp
pop     bp
retn
sub_107E1 endp



; Attributes: bp-based frame

writeParty proc near

member= word ptr -6
idx= word ptr -4
win= dword ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 6
push    si              ; draw
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     [bp+idx], 0

loc_10A16:
mov     si, [bp+idx]
shl     si, 1
mov     ax, _party[si]
mov     [bp+member], ax
mov     _inverseText, 0
sub     ax, ax
push    ax
push    _partyNameWindows[si] ; win
call    setWindow
add     sp, 4
;
call    winClearCurrent
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+idx], ax
jnb     short loc_10A64
sub     ax, ax
push    ax              ; int
push    ds
push    [bp+member]     ; member
call    writeShortMemberName
add     sp, 6
mov     bx, [bp+member]
cmp     [bx+PartyMember._afflicted], 0
jz      short loc_10A64
mov     ax, _currentWindow
add     ax, Window._attr
push    ax
call    sub_1E2B8
add     sp, 2

loc_10A64:
cmp     [bp+arg_0], 0
jz      short loc_10A6D
call    sub_1DC28

loc_10A6D:
inc     [bp+idx]
cmp     [bp+idx], 5
jl      short loc_10A16
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
pop     si
mov     sp, bp
pop     bp
retn
writeParty endp



; Attributes: bp-based frame

sub_10A87 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     [bp+var_2], 0FFFFh

loc_10A93:
mov     [bp+var_6], 0
mov     [bp+var_8], 0
call    getKeypress
mov     [bp+var_4], al
cbw
cmp     ax, 0FFFAh
jz      short loc_10B00
cmp     ax, 0FFFBh
jz      short loc_10AF3
cmp     ax, 0FFFCh
jz      short loc_10ADB
cmp     ax, 0FFFDh
jz      short loc_10ACE
cmp     ax, 0FFFEh
jz      short loc_10AC7
cmp     ax, 0FFFFh
jnz     short loc_10B15
mov     ax, 0FFFFh
jmp     short loc_10B40

loc_10AC7:
mov     [bp+var_2], 5
jmp     short loc_10B15

loc_10ACE:
cmp     [bp+arg_2], 0
jle     short loc_10B15
mov     [bp+var_2], 1
jmp     short loc_10B15

loc_10ADB:
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mov     cx, es:[bx+2]
dec     cx
cmp     ax, cx
jnb     short loc_10B15
mov     [bp+var_2], 2
jmp     short loc_10B15

loc_10AF3:
cmp     [bp+arg_0], 0
jle     short loc_10B15
mov     [bp+var_2], 4
jmp     short loc_10B15

loc_10B00:
mov     al, [bp+arg_0]
cbw
les     bx, dword_22C6E
mov     cx, es:[bx]
dec     cx
cmp     ax, cx
jnb     short loc_10B15
mov     [bp+var_2], 3

loc_10B15:
cmp     [bp+var_2], 0FFFFh
jnz     short loc_10B1E
jmp     loc_10A93

loc_10B1E:
mov     al, byte ptr [bp+var_2]
mov     byte_2E441, al
mov     si, [bp+var_2]
shl     si, 1
mov     al, [si+1DCh]
add     al, [bp+arg_0]
mov     byte_2E423, al
mov     al, [si+1DDh]
add     al, [bp+arg_2]
mov     byte_2E428, al
mov     ax, [bp+var_2]

loc_10B40:
pop     si
mov     sp, bp
pop     bp
retn
sub_10A87 endp




sub_10B45 proc near
sub     ax, ax
push    ax
mov     ax, offset _windowInfo
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     ax, 1
push    ax              ; y
push    ax              ; x
mov     ax, offset text ; "Which way?"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
sub     ax, ax
push    ax              ; int
mov     ax, offset member ; "^"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 1
push    ax              ; int
mov     ax, offset aSpace ; "< space >"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 2
push    ax              ; int
mov     ax, offset aV   ; "v"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
call    sub_1DC28
retn
sub_10B45 endp



; Attributes: bp-based frame

sub_10B95 proc near

var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 2
call    sub_1DD4C
call    sub_10B45
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
call    sub_10A87
add     sp, 4
mov     [bp+var_2], ax
call    winClearCurrent
call    sub_1DDFD
cmp     [bp+var_2], 0FFFFh
jz      short loc_10BE0
mov     al, byte_2E423
cbw
mov     cx, ax
mov     al, byte_2E428
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
add     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
jmp     short loc_10BE3

loc_10BE0:
sub     ax, ax
cwd

loc_10BE3:
mov     sp, bp
pop     bp
retn
sub_10B95 endp



; Attributes: bp-based frame

sub_10BE7 proc near

var_6= byte ptr -6
var_4= word ptr -4
var_2= byte ptr -2

push    bp
mov     bp, sp
sub     sp, 6
mov     [bp+var_6], 0
cmp     _disk1._partyCount, 1
jnz     short loc_10C35
sub     ax, ax
jmp     short loc_10C3E

loc_10BFC:
call    getKeypress
mov     [bp+var_2], al
cmp     al, 0FFh
jnz     short loc_10C0B
mov     ax, 0FFFFh
jmp     short loc_10C3E

loc_10C0B:
cmp     [bp+var_2], 3Bh ; ';'
jl      short loc_10C21
cmp     [bp+var_2], 3Fh ; '?'
jg      short loc_10C21
mov     al, [bp+var_2]
cbw
sub     ax, 3Bh ; ';'
mov     [bp+var_4], ax

loc_10C21:
cmp     [bp+var_4], 0
jl      short loc_10C35
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jnb     short loc_10C35
mov     [bp+var_6], 1

loc_10C35:
cmp     [bp+var_6], 0
jz      short loc_10BFC
mov     ax, [bp+var_4]

loc_10C3E:
mov     sp, bp
pop     bp
retn
sub_10BE7 endp



; Attributes: bp-based frame

sub_10C42 proc near

arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
mov     al, [bp+arg_0]
cbw
cmp     ax, _disk1._mapPosX
jnz     short loc_10C5E
mov     al, [bp+arg_2]
cbw
cmp     ax, _disk1._mapPosY
jnz     short loc_10C5E
mov     ax, 1
jmp     short loc_10C60

loc_10C5E:
sub     ax, ax

loc_10C60:
pop     bp
retn
sub_10C42 endp



; Attributes: bp-based frame

sub_10C62 proc near

var_8= byte ptr -8
var_4= word ptr -4
var_2= word ptr -2
arg_0= dword ptr  4
arg_4= dword ptr  8
arg_8= byte ptr  0Ch
arg_A= byte ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     [bp+var_8], 0
mov     ax, 1
push    ax
mov     al, [bp+arg_A]
cbw
push    ax
mov     al, [bp+arg_8]
cbw
push    ax
call    sub_200D8
add     sp, 6
mov     [bp+var_2], ax
mov     [bp+var_4], 0
jmp     short loc_10C93

loc_10C8D:
inc     [bp+var_4]
inc     [bp+var_2]

loc_10C93:
cmp     [bp+var_4], 3
jge     short loc_10CE3
mov     si, [bp+var_2]
mov     cl, 2
shl     si, cl
mov     al, [bp+arg_A]
cbw
add     ax, [si+172h]
push    ax
mov     al, [bp+arg_8]
cbw
add     ax, [si+170h]
push    ax
call    sub_12C0F
add     sp, 4
or      al, al
jz      short loc_10C8D
mov     si, [bp+var_2]
mov     cl, 2
shl     si, cl
mov     al, [bp+arg_A]
cbw
add     ax, [si+172h]
push    ax
mov     al, [bp+arg_8]
cbw
add     ax, [si+170h]
push    ax
call    sub_10C42
add     sp, 4
or      al, al
jnz     short loc_10C8D
mov     [bp+var_8], 1

loc_10CE3:
cmp     [bp+var_8], 0
jz      short loc_10D14
les     bx, [bp+arg_0]
mov     si, [bp+var_2]
shl     si, 1
shl     si, 1
mov     al, [si+170h]
add     al, [bp+arg_8]
mov     es:[bx], al
les     bx, [bp+arg_4]
mov     si, [bp+var_2]
shl     si, 1
shl     si, 1
mov     al, [si+172h]
add     al, [bp+arg_A]
mov     es:[bx], al
call    sub_17E09

loc_10D14:
pop     si
mov     sp, bp
pop     bp
retn
sub_10C62 endp



; Attributes: bp-based frame

sub_10D19 proc near

var_4= dword ptr -4
arg_0= byte ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 4
mov     al, [bp+arg_4]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [bp+arg_2]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
and     byte ptr es:[bx+1], 7
mov     al, [bp+arg_4]
cbw
push    ax
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_10D74
les     bx, [bp+var_4]
mov     ax, [bp+arg_6]
mov     cl, 0Bh
shl     ax, cl
or      es:[bx], ax

loc_10D74:
mov     sp, bp
pop     bp
retn
sub_10D19 endp



; Attributes: bp-based frame

sub_10D78 proc near

var_A= dword ptr -0Ah
var_6= word ptr -6
var_4= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 0Eh
mov     [bp+var_6], 0
jmp     short loc_10DBF

loc_10D85:
mov     ax, 68h ; 'h'
imul    [bp+var_6]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
mov     al, es:[bx+65h]
sub     ah, ah
push    ax
mov     al, es:[bx+5Ch]
cbw
push    ax
mov     al, es:[bx+5Bh]
cbw
push    ax
mov     al, es:[bx+5Fh]
sub     ah, ah
push    ax
call    sub_10D19
add     sp, 8
inc     [bp+var_6]

loc_10DBF:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_6]
ja      short loc_10D85
mov     [bp+var_6], 0
jmp     short loc_10E16

loc_10DD5:
mov     ax, 66h ; 'f'
imul    [bp+var_6]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_A], ax
mov     word ptr [bp+var_A+2], dx
les     bx, [bp+var_A]
test    byte ptr es:[bx+9], 0Fh
jz      short loc_10E13
mov     al, es:[bx+0Ch]
sub     ah, ah
push    ax
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
mov     al, es:[bx+5]
sub     ah, ah
push    ax
call    sub_10D19
add     sp, 8

loc_10E13:
inc     [bp+var_6]

loc_10E16:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_6]
ja      short loc_10DD5
mov     sp, bp
pop     bp
retn
sub_10D78 endp



; Attributes: bp-based frame

sub_10E29 proc near

var_4= byte ptr -4
var_2= byte ptr -2
arg_0= dword ptr  4
arg_4= dword ptr  8
arg_8= byte ptr  0Ch
arg_A= byte ptr  0Eh
arg_C= word ptr  10h
arg_E= word ptr  12h

push    bp
mov     bp, sp
sub     sp, 4
push    si
les     bx, [bp+arg_0]
mov     al, es:[bx]
mov     [bp+var_2], al
les     bx, [bp+arg_4]
mov     al, es:[bx]
mov     [bp+var_4], al
cmp     [bp+arg_C], 0
jg      short loc_10E4B
jmp     loc_10EF2

loc_10E4B:
mov     al, [bp+var_2]
cbw
mov     si, ax
les     bx, dword_22C6E
cmp     es:[bx], ax
ja      short loc_10E5D
jmp     loc_10EF2

loc_10E5D:
mov     al, [bp+var_4]
cbw
push    ax
push    si
call    sub_10795
add     sp, 4
cmp     al, 1
jz      short loc_10EA3
mov     al, [bp+var_4]
cbw
push    ax
mov     al, [bp+var_2]
cbw
push    ax
mov     al, [bp+arg_8]
sub     ah, ah
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_10EA3
mov     al, [bp+var_4]
cbw
push    ax
mov     al, [bp+var_2]
cbw
push    ax
push    word ptr [bp+arg_4+2]
push    word ptr [bp+arg_4]
push    word ptr [bp+arg_0+2]
push    word ptr [bp+arg_0]
call    sub_10C62
add     sp, 0Ch

loc_10EA3:
sub     ax, ax
push    ax
mov     al, [bp+var_4]
cbw
push    ax
mov     al, [bp+var_2]
cbw
push    ax
call    sub_12AB2
add     sp, 6
les     bx, [bp+arg_4]
mov     al, es:[bx]
cbw
push    ax
les     bx, [bp+arg_0]
mov     al, es:[bx]
cbw
push    ax
mov     al, [bp+arg_A]
sub     ah, ah
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_10EF2
push    [bp+arg_E]
les     bx, [bp+arg_4]
mov     al, es:[bx]
cbw
push    ax
les     bx, [bp+arg_0]
mov     al, es:[bx]
cbw
push    ax
call    sub_12AB2
add     sp, 6
call    sub_17E09

loc_10EF2:
pop     si
mov     sp, bp
pop     bp
retn
sub_10E29 endp



; Attributes: bp-based frame

sub_10EF7 proc near

var_10= word ptr -10h
var_C= dword ptr -0Ch
var_8= word ptr -8
var_4= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 14h
push    si
call    sub_154D6
mov     [bp+var_10], ax
mov     [bp+var_8], 0
jmp     short loc_10F54

loc_10F0B:
les     bx, [bp+var_4]
mov     al, es:[bx+65h]
sub     ah, ah
push    ax
push    word ptr es:[bx+44h]
mov     al, es:[bx+5Fh]
push    ax
mov     al, es:[bx+5Eh]
push    ax
mov     ax, bx
mov     dx, es
add     ax, 5Ch ; '\'
push    dx
push    ax
mov     ax, bx
add     ax, 5Bh ; '['
push    dx
push    ax
call    sub_10E29
add     sp, 10h
les     bx, [bp+var_4]
mov     al, byte ptr [bp+var_10]
sub     es:[bx+64h], al

loc_10F43:
les     bx, [bp+var_4]
mov     al, es:[bx+64h]
sub     ah, ah
cmp     ax, [bp+var_10]
jnb     short loc_10F0B

loc_10F51:
inc     [bp+var_8]

loc_10F54:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_8]
jbe     short loc_10F9E
mov     ax, 68h ; 'h'
imul    [bp+var_8]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
cmp     byte ptr es:[bx+5Dh], 79h ; 'y'
jb      short loc_10F51
cmp     byte ptr es:[bx+60h], 0FFh
jz      short loc_10F51
les     bx, _hdsPct
mov     si, es:[bx+3E8h]
les     bx, [bp+var_4]
mov     al, es:[bx+si+18h]
add     es:[bx+64h], al
jmp     short loc_10F43

loc_10F9E:
mov     [bp+var_8], 0
jmp     short loc_10FF0

loc_10FA5:
les     bx, [bp+var_C]
mov     al, es:[bx+0Ch]
sub     ah, ah
push    ax
mov     ax, 1
push    ax
mov     al, es:[bx+5]
sub     ah, ah
push    ax
mov     al, es:[bx+4]
push    ax
mov     ax, bx
mov     dx, es
add     ax, 3
push    dx
push    ax
mov     ax, bx
add     ax, 2
push    dx
push    ax
call    sub_10E29
add     sp, 10h
les     bx, [bp+var_C]
mov     al, byte ptr [bp+var_10]
sub     es:[bx+0Ah], al

loc_10FDF:
les     bx, [bp+var_C]
mov     al, es:[bx+0Ah]
sub     ah, ah
cmp     ax, [bp+var_10]
jnb     short loc_10FA5

loc_10FED:
inc     [bp+var_8]

loc_10FF0:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_8]
jbe     short loc_1102E
mov     ax, 66h ; 'f'
imul    [bp+var_8]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_C], ax
mov     word ptr [bp+var_C+2], dx
les     bx, [bp+var_C]
test    byte ptr es:[bx+9], 0Fh
jz      short loc_10FED
test    byte ptr es:[bx+9], 0F0h
jnz     short loc_10FED
mov     al, es:[bx+0Bh]
add     es:[bx+0Ah], al
jmp     short loc_10FDF

loc_1102E:
pop     si
mov     sp, bp
pop     bp
retn
sub_10EF7 endp



; Attributes: bp-based frame

writeTime proc near

win= dword ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     al, _disk1._timeMinutes
sub     ah, ah
push    ax
mov     al, _disk1._timeHours
push    ax              ; char *
mov     ax, offset a22d2d ; "%2.2d:%.2d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
sub     ax, ax
push    ax
mov     ax, offset _windowTime
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax              ; y
mov     ax, 1
push    ax              ; x
mov     bl, _disk1._timeWeekday
sub     bh, bh
shl     bx, 1
push    ds
push    WEEKDAYS[bx]    ; text
call    writeStringAt
add     sp, 8
mov     ax, 1
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
mov     sp, bp
pop     bp
retn
writeTime endp



; Attributes: bp-based frame

setTime proc near

var_8= byte ptr -8
var_6= word ptr -6
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 8
push    di
push    si
cmp     [bp+arg_0], 0
jz      short loc_110AF
mov     al, 1
jmp     short loc_110B7

loc_110AF:
les     bx, dword_22C6E
mov     al, es:[bx+6]

loc_110B7:
mov     [bp+var_8], al
inc     word_2E204
mov     al, _disk1._timeHours
sub     ah, ah
mov     si, ax
mov     [bp+var_6], si
mov     al, [bp+var_8]
mov     di, ax
mov     cl, 3Ch ; '<'
div     cl
add     _disk1._timeMinutes, ah
mov     ax, di
div     cl
sub     ah, ah
mov     cx, ax
mov     al, _disk1._timeMinutes
mov     dl, 60
div     dl
sub     ah, ah
add     ax, cx
add     ax, si
sub     dx, dx
mov     cx, 24
div     cx
mov     _disk1._timeHours, dl
mov     cl, 60
mov     al, _disk1._timeMinutes
sub     ah, ah
div     cl
mov     _disk1._timeMinutes, ah
mov     al, dl
sub     ah, ah
cmp     si, ax
jbe     short loc_11119
inc     _disk1._timeWeekday
mov     al, _disk1._timeWeekday
mov     cl, 7
div     cl
mov     _disk1._timeWeekday, ah

loc_11119:
cmp     [bp+arg_0], 0
jnz     short loc_11139
call    writeTime
mov     ax, 1CA4h
push    ax
call    sub_1DA17
add     sp, 2
mov     al, _disk1._timeHours
sub     ah, ah
cmp     [bp+var_6], ax
jz      short loc_11139
call    sub_17854

loc_11139:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
setTime endp



; Attributes: bp-based frame

sub_1113F proc near

var_16= dword ptr -16h
var_12= word ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 16h
push    si
mov     [bp+var_4], 0
cmp     [bp+arg_0], 0
jz      short loc_11155
mov     ax, 5
jmp     short loc_11158

loc_11155:
mov     ax, 0Ah

loc_11158:
mov     [bp+var_10], ax
cmp     [bp+arg_0], 0
jz      short loc_11166
mov     ax, 1
jmp     short loc_11170

loc_11166:
les     bx, dword_22C6E
mov     al, es:[bx+6]
sub     ah, ah

loc_11170:
mov     [bp+var_2], ax
jmp     loc_112DB

loc_11176:
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    setTime
add     sp, 2
mov     [bp+var_A], 0
mov     [bp+var_8], 0
jmp     loc_11212

loc_1118F:
mov     ax, 0FFF1h

loc_11192:
mov     [bp+var_E], ax
cmp     [bp+arg_0], 0
jz      short loc_111B4
mov     bx, [bp+var_12]
cmp     byte ptr [bx+1Bh], 0Fh
jbe     short loc_111AF
mov     al, [bx+1Bh]
sub     ah, ah
sub     ax, 0Fh
add     [bp+var_6], ax

loc_111AF:
mov     [bp+var_E], 0

loc_111B4:
mov     ax, [bp+var_E]
cmp     [bp+var_6], ax
jg      short loc_111CB
mov     ax, 1
push    ax
push    [bp+var_12]
call    sub_18FFA
add     sp, 4
jmp     short loc_11203

loc_111CB:
mov     bx, [bp+var_12]
test    byte ptr [bx+58h], 7Fh
jnz     short loc_11203
cmp     byte ptr [bx+54h], 0
jz      short loc_111E0
cmp     byte ptr [bx+21h], 0
jz      short loc_11203

loc_111E0:
mov     si, bx
mov     ax, [si+44h]
inc     ax
cmp     ax, [si+46h]
jle     short loc_111EE
mov     ax, [si+46h]

loc_111EE:
mov     [bx+44h], ax
mov     bx, [bp+var_12]
cmp     word ptr [bx+44h], 1
jnz     short loc_11203
mov     si, bx
mov     ax, [si+122h]
mov     [bx+44h], ax

loc_11203:
mov     bx, [bp+var_12]
cmp     word ptr [bx+44h], 0
jle     short loc_1120F
inc     [bp+var_8]

loc_1120F:
inc     [bp+var_A]

loc_11212:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A], ax
jb      short loc_1121F
jmp     loc_112C3

loc_1121F:
mov     bx, [bp+var_A]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_12], ax
mov     bx, ax
test    byte ptr [bx+58h], 7Fh
jz      short loc_11294
mov     [bp+var_C], 0

loc_11238:
mov     bx, [bp+var_12]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_C]
shl     dx, cl
test    ax, dx
jz      short loc_1128B
mov     ax, 16h
imul    [bp+var_C]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
mov     al, es:[bx+3ECh]
sub     ah, ah
mul     [bp+var_2]
mov     cx, ax
mov     ax, word_2E204
sub     dx, dx
div     cx
or      dx, dx
jnz     short loc_1128B
mov     al, es:[bx+3EDh]
sub     ah, ah
push    ax
push    [bp+var_12]
call    sub_18FFA
add     sp, 4

loc_1128B:
inc     [bp+var_C]
cmp     [bp+var_C], 7
jl      short loc_11238

loc_11294:
mov     ax, word_2E204
sub     dx, dx
div     [bp+var_10]
or      dx, dx
jz      short loc_112A3
jmp     loc_11203

loc_112A3:
mov     bx, [bp+var_12]
mov     ax, [bx+44h]
mov     [bp+var_6], ax
cmp     byte ptr [bx+54h], 0
jnz     short loc_112B5
jmp     loc_1118F

loc_112B5:
cmp     byte ptr [bx+21h], 0
jz      short loc_112BE
jmp     loc_1118F

loc_112BE:
sub     ax, ax
jmp     loc_11192

loc_112C3:
cmp     [bp+var_8], 0
jnz     short loc_112D7
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    sub_12B93
add     sp, 2
jmp     short loc_112DB

loc_112D7:
mov     [bp+var_4], 1

loc_112DB:
cmp     [bp+var_4], 0
jnz     short loc_112E4
jmp     loc_11176

loc_112E4:
pop     si
mov     sp, bp
pop     bp
retn
sub_1113F endp



; Attributes: bp-based frame

sub_112E9 proc near

win= dword ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
sub     ax, ax
push    ax
call    sub_1113F
add     sp, 2
cmp     word_2D128, 0
jle     short loc_11323
mov     ax, word_2E204
sub     dx, dx
div     word_2D128
or      dx, dx
jnz     short loc_11323
call    sub_192D5

loc_11323:
sub     ax, ax
push    ax
call    sub_12B93
add     sp, 2
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
mov     sp, bp
pop     bp
retn
sub_112E9 endp




sub_11339 proc near
call    sub_10EF7
call    sub_10D78
sub     ax, ax
push    ax
call    sub_14F1A
add     sp, 2
mov     word_2F01A, ax
or      ax, ax
jg      short loc_1137F
call    sub_17685
or      al, al
jz      short loc_1136F
call    sub_17E09
mov     ax, 2
push    ax
call    sub_21130
add     sp, 2
mov     ax, offset aUhOhHereComesT ; "\r\rUh oh! Here comes trouble!"
push    ax
call    sub_1B29B
add     sp, 2

loc_1136F:
sub     ax, ax
push    ax
call    sub_14F1A
add     sp, 2
mov     word_2F01A, ax
or      ax, ax
jle     short locret_1138C

loc_1137F:
sub     ax, ax
push    ax
mov     ax, 1
push    ax
call    sub_15691
add     sp, 4

locret_1138C:
retn
sub_11339 endp



; Attributes: bp-based frame

nothing proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
call    sub_112E9
cmp     [bp+arg_0], 0
jnz     short loc_1139C
call    sub_11339

loc_1139C:
call    sub_1DD4C
pop     bp
retn
nothing endp

push    bp
mov     bp, sp
sub     sp, 28h
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     si, [bx-3240h]
mov     bx, si
shl     bx, 1
push    word ptr [bx-2518h]
lea     ax, [si+1]
push    ax
mov     ax, offset aF1d1717s ; "F%1d> %-17.17s"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 2Eh
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
push    ax
push    word ptr [bp+8]
call    sub_20182
add     sp, 4
mov     [bp-4], al
or      al, al
jz      short loc_1140F
mov     ax, 2Ah ; '*'
jmp     short loc_11412

loc_1140F:
mov     ax, 20h ; ' '

loc_11412:
push    ax
push    word ptr [bp+4]
mov     ax, offset a1dC ; "%1d>%c"
push    ax
lea     ax, [bp-2Eh]
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Eh]
push    ss
push    ax
call    writeStringAt
add     sp, 8
mov     ax, 6
imul    word ptr [bp-2]
mov     si, ax
mov     bx, [bp+8]
test    byte ptr [bx+si+65h], 1
jz      short loc_1144D
mov     _inverseText, 0FFh

loc_1144D:
mov     ax, 0FFFFh
push    ax
push    ax
mov     ax, 6
imul    word ptr [bp-2]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 42
push    ds
push    ax
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 2Ah
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     ax, 16h
imul    word ptr [bp-2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 126h
push    dx
push    ax
mov     si, [bp-2]
mov     bx, [bp+8]
mov     al, [bx+si+24h]
sub     ah, ah
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d3d1717fs ; "%1d> %3d %-17.17Fs"
push    ax
lea     ax, [bp-2Ah]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Ah]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 2Ah
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     ax, 16h
imul    word ptr [bp-2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 286h
push    dx
push    ax
mov     si, [bp-2]
mov     bx, [bp+8]
mov     al, [bx+si+34h]
sub     ah, ah
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d3d1717fs_0 ; "%1d> %3d %-17.17Fs"
push    ax
lea     ax, [bp-2Ah]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Ah]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 28h
push    si
mov     ax, 18h
imul    word ptr [bp+6]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 6
push    dx
push    ax
mov     si, [bp+6]
mov     bx, [bp+8]
mov     al, [bx+si+18h]
sub     ah, ah
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d3d1717fs_1 ; "%1d> %3d %-17.17Fs"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 28h
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, 6
mul     word ptr [bx-3240h]
mov     si, ax
mov     bx, [bp+8]
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1717s ; "%1d> %-17.17s"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_115CB proc near

var_6= word ptr -6
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
text= dword ptr  0Ah
arg_A= byte ptr  0Eh
arg_C= byte ptr  10h

push    bp
mov     bp, sp
sub     sp, 2Eh
mov     ax, [bp+arg_2]
add     ax, 8
cmp     ax, [bp+arg_4]
jle     short loc_115E4
mov     ax, [bp+arg_4]
sub     ax, [bp+arg_2]
jmp     short loc_115E7

loc_115E4:
mov     ax, 8

loc_115E7:
mov     [bp+var_2], ax
call    winClearCurrent
cmp     [bp+arg_4], 8
jle     short loc_11620
cmp     [bp+arg_2], 0
jnz     short loc_11602
sub     ax, ax
push    ax
push    ax
mov     ax, 19h
jmp     short loc_11619

loc_11602:
sub     ax, ax
push    ax
push    ax
mov     ax, [bp+arg_2]
add     ax, [bp+var_2]
cmp     ax, [bp+arg_4]
jge     short loc_11616
mov     ax, 1Ah
jmp     short loc_11619

loc_11616:
mov     ax, 18h

loc_11619:
push    ax
call    sub_1DE81
add     sp, 6

loc_11620:
cmp     [bp+arg_A], 0
jz      short loc_1162B
mov     _inverseText, 0FFh

loc_1162B:
sub     ax, ax
push    ax              ; y
mov     ax, 2
push    ax              ; x
push    ds
push    word ptr [bp+text] ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
mov     [bp+var_6], 0
jmp     short loc_11660

loc_11648:
push    [bp+arg_0]
mov     ax, [bp+arg_2]
add     ax, [bp+var_6]
push    ax
mov     ax, [bp+var_6]
inc     ax
push    ax
call    word ptr ss:(aMsRunTimeLibra+4)[bp] ; "un-Time Library - Copyright (c) 1988, M"...
add     sp, 6
inc     [bp+var_6]

loc_11660:
mov     ax, [bp+var_2]
cmp     [bp+var_6], ax
jl      short loc_11648
cmp     byte_21328, 0
jz      short loc_11672
call    sub_1DAF0

loc_11672:
cmp     [bp+arg_C], 0
jz      short loc_1167B
call    sub_1DC28

loc_1167B:
mov     ax, [bp+var_2]
mov     sp, bp
pop     bp
retn
sub_115CB endp



; Attributes: bp-based frame

sub_11682 proc near

var_A= byte ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch
arg_A= byte ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     [bp+var_A], 0
mov     bx, [bp+arg_4]
mov     ax, [bx]
mov     [bp+var_8], ax
mov     ax, [bp+arg_2]
cmp     [bx], ax
jnz     short loc_116A3
or      ax, ax
jle     short loc_116A3

loc_116A0:
sub     word ptr [bx], 8

loc_116A3:
mov     ax, 1
push    ax
mov     al, [bp+arg_A]
sub     ah, ah
push    ax
push    [bp+arg_8]
push    [bp+arg_6]
push    [bp+arg_2]
mov     bx, [bp+arg_4]
push    word ptr [bx]
push    [bp+arg_0]
call    sub_115CB
add     sp, 0Eh
mov     [bp+var_8], ax
jmp     loc_1174F

loc_116CA:
call    getKeypress
mov     [bp+var_4], al
cbw
mov     si, ax
test    byte ptr [si-4365h], 4
jz      short loc_116F2
and     ax, 0Fh
mov     [bp+var_6], ax
or      ax, ax
jle     short loc_1174F
mov     ax, [bp+var_8]
cmp     [bp+var_6], ax
jg      short loc_1174F
mov     [bp+var_A], 1
jmp     short loc_1174F

loc_116F2:
mov     al, [bp+var_4]
cbw
cmp     ax, 0FFFCh
jz      short loc_1171C
jg      short loc_1173B
cmp     ax, 0FFFAh
jl      short loc_1174F
cmp     ax, 0FFFBh
jle     short loc_1172F
jmp     short loc_1174F

loc_11709:
mov     al, [bp+var_4]
cbw
sub     ax, 46h ; 'F'
jmp     short loc_11761

loc_11712:
mov     bx, [bp+arg_4]
cmp     word ptr [bx], 8
jl      short loc_1174F
jmp     short loc_116A0

loc_1171C:
mov     ax, [bp+arg_2]
mov     bx, [bp+arg_4]
sub     ax, [bx]
cmp     ax, [bp+var_8]
jle     short loc_1174F
add     word ptr [bx], 8
jmp     loc_116A3

loc_1172F:
cmp     [bp+arg_A], 0
jz      short loc_1174F

loc_11735:
mov     al, [bp+var_4]
cbw
jmp     short loc_11761

loc_1173B:
cmp     ax, 0FFFDh
jz      short loc_11712
cmp     ax, 0FFFFh
jz      short loc_11735
cmp     ax, 3Bh ; ';'
jl      short loc_1174F
cmp     ax, 3Fh ; '?'
jle     short loc_11709

loc_1174F:
cmp     [bp+var_A], 0
jnz     short loc_11758
jmp     loc_116CA

loc_11758:
mov     bx, [bp+arg_4]
mov     ax, [bx]
add     ax, [bp+var_6]
dec     ax

loc_11761:
pop     si
mov     sp, bp
pop     bp
retn
sub_11682 endp



; Attributes: bp-based frame

sub_11766 proc near

var_4= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 4
push    si
cmp     word_2E418, 8
jz      short loc_11777
jmp     loc_11825

loc_11777:
mov     byte_2ED26, 0
mov     ax, 68h ; 'h'
imul    word_2D100
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
mov     al, es:[bx+5Bh]
mov     es:[bx+60h], al
les     bx, [bp+var_4]
mov     al, es:[bx+5Ch]
mov     es:[bx+61h], al
mov     word_2F01C, 0
push    word ptr [bp+var_4+2]
push    word ptr [bp+var_4]
les     bx, [bp+var_4]
mov     al, es:[bx+63h]
sub     ah, ah
push    ax
mov     ax, 1
push    ax
mov     al, es:[bx+1Ah]
sub     ah, ah
shl     ax, 1
push    ax
sub     ax, ax
push    ax
push    word_2D100
mov     ax, 1
push    ax
sub     ax, ax
push    ax
call    sub_14E61
add     sp, 12h
mov     byte_2E6DA, 0Eh
les     bx, [bp+var_4]
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
call    sub_10795
add     sp, 4
mov     byte_2E6DC, al
mov     word_2F01A, 1
les     bx, _hdsPct
mov     si, es:[bx+3E8h]
les     bx, [bp+var_4]
mov     al, es:[bx+si+18h]
sub     ah, ah
mov     word_2EC0A, ax
mov     ax, 1
push    ax
sub     ax, ax
push    ax
call    sub_15691
add     sp, 4
jmp     short loc_1182F

loc_11825:
cmp     byte_2ED26, 0
jz      short loc_1182F
call    sub_1B452

loc_1182F:
pop     si
mov     sp, bp
pop     bp
retn
sub_11766 endp



; Attributes: bp-based frame

sub_11834 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 4
mov     ax, [bp+arg_4]
mov     word_2E418, ax
mov     ax, [bp+arg_0]
mov     dx, [bp+arg_2]
mov     word_2EBF4, ax
mov     word_2EBF6, dx
mov     ax, [bp+arg_6]
mov     word_2D100, ax
mov     word_2EC10, 0
jmp     short loc_11890

loc_1185B:
cmp     word_2E418, 8
jz      short loc_11897
sub     ax, ax
push    ax
push    word_2D100
push    word_2EC10
push    word_2E418
push    word_2EBF6
push    word_2EBF4
call    doScript
add     sp, 0Ch
cmp     byte_2ED26, 0
jz      short loc_11890
cmp     word_2E418, 0
jnz     short loc_11890
call    sub_1B452

loc_11890:
cmp     word_2E418, 0FFFFh
jnz     short loc_1185B

loc_11897:
call    sub_11766
mov     sp, bp
pop     bp
retn
sub_11834 endp



; Attributes: bp-based frame

sub_1189E proc near

arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= word ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     si, [bx-2518h]
mov     ax, [si+148h]
mov     dx, [si+14Ah]
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
mov     al, [si+145h]
push    ax
push    [bp+arg_0]
push    [bp+arg_4]
mov     ax, word ptr dword_2E4C6
add     ax, 6
push    dx
push    ax
call    doScript
add     sp, 0Ch
or      al, al
jz      short loc_1190A
cmp     word_2E418, 0FFFFh
jz      short loc_118F9
sub     ax, ax
push    ax
push    word_2E418
push    word_2EBF6
push    word_2EBF4
call    sub_11834
add     sp, 8

loc_118F9:
cmp     [bp+arg_6], 0
jnz     short loc_1191D
sub     ax, ax
push    ax
call    nothing
add     sp, 2
jmp     short loc_1191D

loc_1190A:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aThatHasNoEffec ; "\rThat has no effect.\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_1191D:
pop     si
pop     bp
retn
sub_1189E endp



; Attributes: bp-based frame

sub_11920 proc near

var_C= word ptr -0Ch
var_A= dword ptr -0Ah
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= dword ptr  4
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 0Ch
push    si
cmp     [bp+arg_6], 0
jnz     short loc_11933
mov     ax, 6
jmp     loc_11A31

loc_11933:
les     bx, [bp+arg_0]
cmp     word ptr es:[bx+6], 0FFFFh
jnz     short loc_11943

loc_1193D:
mov     ax, 5
jmp     loc_11A31

loc_11943:
les     bx, [bp+arg_0]
mov     bx, es:[bx+6]
shl     bx, 1
les     si, dword_2EE12
mov     ax, es:[bx+si]
add     ax, word_22C72
mov     dx, word_22C74
mov     word ptr [bp+var_A], ax
mov     word ptr [bp+var_A+2], dx
les     bx, [bp+var_A]
test    byte ptr es:[bx], 10h
jz      short loc_1193D
les     bx, [bp+arg_0]
mov     ax, es:[bx+4]
mov     [bp+var_6], ax
or      ax, ax
jnz     short loc_1197E

loc_11978:
mov     ax, 4
jmp     loc_11A31

loc_1197E:
les     bx, [bp+arg_0]
mov     al, es:[bx+3]
sub     ah, ah
mov     [bp+var_4], ax
mov     bx, _globals
mov     al, [bx]
mov     cx, ax
mov     ax, [bp+var_4]
mul     cx
mov     bx, [bp+arg_4]
cmp     ax, [bx+0Ch]
jnb     short loc_11978
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+arg_6], ax
jb      short loc_11A2E
mov     bx, _globals
mov     al, [bx+1]
sub     ah, ah
mul     [bp+var_4]
push    ax
mov     al, [bx]
sub     ah, ah
mov     cx, ax
mov     ax, [bp+var_4]
mul     cx
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+var_2], ax
mov     bx, [bp+arg_4]
push    word ptr [bx+0Ch]
push    word ptr [bx+0Ah]
call    sub_1DEB1
add     sp, 4
mov     [bp+var_C], ax
mov     ax, [bp+var_2]
cmp     [bp+var_C], ax
jle     short loc_119F6
mov     ax, [bp+var_C]
sub     ax, [bp+var_2]
jmp     short loc_119F8

loc_119F6:
sub     ax, ax

loc_119F8:
mov     [bp+var_C], ax
mov     ax, [bp+var_6]
cmp     [bp+var_C], ax
jge     short loc_11A08
sub     ax, [bp+var_C]
jmp     short loc_11A0A

loc_11A08:
sub     ax, ax

loc_11A0A:
mov     [bp+var_6], ax
les     bx, [bp+arg_0]
mov     es:[bx+4], ax
cmp     [bp+var_C], 0
jnz     short loc_11A1E
sub     ax, ax
jmp     short loc_11A31

loc_11A1E:
cmp     [bp+var_6], 0
jnz     short loc_11A29
mov     ax, 1
jmp     short loc_11A31

loc_11A29:
mov     ax, 2
jmp     short loc_11A31

loc_11A2E:
mov     ax, 3

loc_11A31:
pop     si
mov     sp, bp
pop     bp
retn
sub_11920 endp



; Attributes: bp-based frame

; int __cdecl sub_11A36(char *, int)
sub_11A36 proc near

arg_0= dword ptr  4
arg_4= word ptr  8

push    bp
mov     bp, sp
push    [bp+arg_4]
push    word ptr [bp+arg_0+2]
push    word ptr [bp+arg_0] ; char *
mov     ax, offset aSFs ; "\r%s %Fs..."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
pop     bp
retn
sub_11A36 endp



; Attributes: bp-based frame

sub_11A62 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 8
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_8], ax
mov     al, 6
mul     [bp+arg_2]
mov     si, ax
add     si, [bp+var_8]
mov     al, 18h
mul     byte ptr [si+62h]
mov     di, ax
mov     bx, _globals
mov     al, 3Ch ; '<'
mul     byte ptr [bx+di+3Fh]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp+var_6], ax
mov     bx, ax
test    byte ptr [bx+18h], 2
jz      short loc_11B14
test    byte ptr [bx+18h], 1
jz      short loc_11AAC
dec     byte ptr [si+64h]
jmp     short loc_11B14

loc_11AAC:
mov     bx, [bp+var_8]
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_11AC3
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
jmp     short loc_11AC6

loc_11AC3:
mov     ax, 0FFh

loc_11AC6:
mov     [bp+var_2], ax
mov     ax, 0FFh
push    ax
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_2060D
add     sp, 6
cmp     [bp+var_2], 0FFh
jz      short loc_11B14
mov     [bp+var_4], 0
jmp     short loc_11AEF

loc_11AEC:
inc     [bp+var_4]

loc_11AEF:
mov     bx, [bp+var_8]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_4]
jbe     short loc_11B14
mov     ax, 6
imul    [bp+var_4]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
cmp     ax, [bp+var_2]
jnz     short loc_11AEC
mov     al, byte ptr [bp+var_4]
mov     [bx+4Ah], al

loc_11B14:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_11A62 endp




sub_11B1A proc near
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aYouDecideThatI ; "\rYou decide that it wouldn't work.\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
retn
sub_11B1A endp



; Attributes: bp-based frame

sub_11B2E proc near

var_16= dword ptr -16h
var_12= word ptr -12h
var_E= word ptr -0Eh
var_8= word ptr -8
var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 18h
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_16], ax
mov     bx, ax
mov     al, 18h
mul     byte ptr [bx+147h]
mov     si, ax
mov     bx, _globals
mov     al, 3Ch ; '<'
mul     byte ptr [bx+si+3Fh]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp+var_E], ax
mov     bx, ax
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     word ptr [bp+var_16+2], ax
mov     bx, word ptr [bp+var_16]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, 12h
mul     byte ptr [bx+si+63h]
add     ax, word ptr [bp+var_16+2]
mov     [bp+var_12], ax
call    sub_1DD4C
mov     [bp+var_8], 64h ; 'd'
mov     bx, word ptr [bp+var_16]
mov     ax, [bx+148h]
mov     dx, [bx+14Ah]
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
mov     ax, 64h ; 'd'
push    ax
push    [bp+var_12]
push    dx
push    word ptr dword_2E4C6
call    sub_11920
add     sp, 8
mov     [bp+var_2], ax
cmp     ax, 6           ; switch 7 cases
jbe     short loc_11BB6
jmp     def_11BB9       ; jumptable 00011BB9 default case

loc_11BB6:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_11BB9[bx] ; switch jump

loc_11BBE:              ; jumptable 00011BB9 cases 0,3
push    ds
push    [bp+var_E]
push    word ptr [bp+var_16] ; char *
call    sub_11A36
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aAndMisses ; "\rand misses!\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, word ptr [bp+var_16]
mov     al, [bx+145h]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_11A62
add     sp, 4

loc_11BEE:
sub     ax, ax
push    ax
call    nothing
add     sp, 2
jmp     short def_11BB9 ; jumptable 00011BB9 default case

loc_11BF9:              ; jumptable 00011BB9 cases 1,2
push    ds
push    [bp+var_E]
push    word ptr [bp+var_16] ; char *
call    sub_11A36
add     sp, 6
cmp     [bp+var_2], 1
jnz     short loc_11C11
mov     ax, 1
jmp     short loc_11C13

loc_11C11:
sub     ax, ax

loc_11C13:
push    ax
mov     bx, word ptr [bp+var_16]
mov     al, [bx+145h]
sub     ah, ah
push    ax
push    [bp+arg_0]
mov     ax, 4
push    ax
mov     ax, word ptr dword_2E4C6
mov     dx, word ptr dword_2E4C6+2
add     ax, 6
push    dx
push    ax
call    doScript
add     sp, 0Ch
or      al, al
jz      short loc_11C56 ; jumptable 00011BB9 cases 4-6
mov     bx, word ptr [bp+var_16]
mov     al, [bx+145h]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_11A62
add     sp, 4
cmp     [bp+arg_2], 0
jnz     short def_11BB9 ; jumptable 00011BB9 default case
jmp     short loc_11BEE

loc_11C56:              ; jumptable 00011BB9 cases 4-6
call    sub_11B1A
jmp     short def_11BB9 ; jumptable 00011BB9 default case
jpt_11BB9 dw offset loc_11BBE ; jump table for switch statement
dw offset loc_11BF9
dw offset loc_11BF9
dw offset loc_11BBE
dw offset loc_11C56
dw offset loc_11C56
dw offset loc_11C56

def_11BB9:              ; jumptable 00011BB9 default case
pop     si
mov     sp, bp
pop     bp
retn
sub_11B2E endp

push    bp
mov     bp, sp
sub     sp, 4
mov     byte ptr [bp-2], 0

loc_11C78:
cmp     _disk1._partyCount, 1
jbe     short loc_11C93
call    winClearCurrent
sub     ax, ax
push    ax
push    ax
push    ds
push    word ptr [bp+4]
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_11C93:
call    sub_10BE7
mov     [bp-4], ax
mov     bx, ax
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0
jg      short loc_11CD9
call    sub_1DDFD
call    sub_1DDFD
mov     ax, 0FFFFh
push    ax
sub     ax, ax
push    ax
mov     bx, [bp-4]
shl     bx, 1
push    ds
push    _party[bx]
call    writeStringAt
add     sp, 8
mov     ax, 0FFFFh
push    ax
push    ax
mov     ax, 385h
push    ds
push    ax
call    writeStringAt
add     sp, 8
call    getKeypress
jmp     short loc_11CDD

loc_11CD9:
mov     byte ptr [bp-2], 1

loc_11CDD:
cmp     byte ptr [bp-2], 0
jz      short loc_11C78
mov     ax, [bp-4]
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_11CEA proc near

var_4= word ptr -4
arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 4
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_4], ax
mov     bx, ax
mov     al, [bx+144h]
sub     ah, ah
cmp     ax, 0Ah
jz      short loc_11D51
cmp     ax, 0Bh
jz      short loc_11D16
cmp     ax, 0Eh
jz      short loc_11D5C
jmp     short loc_11D65

loc_11D16:
mov     al, 18h
mul     byte ptr [bx+147h]
mov     si, ax
mov     bx, _globals
test    byte ptr [bx+si+3Eh], 1
jz      short loc_11D39
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_11B2E
add     sp, 4
jmp     short loc_11D65

loc_11D39:
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
mov     ax, 2

loc_11D42:
push    ax
sub     ax, ax
push    ax
push    [bp+arg_0]
call    sub_1189E
add     sp, 8
jmp     short loc_11D65

loc_11D51:
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
mov     ax, 3
jmp     short loc_11D42

loc_11D5C:
sub     ax, ax
push    ax
call    nothing
add     sp, 2

loc_11D65:
pop     si
mov     sp, bp
pop     bp
retn
sub_11CEA endp



; Attributes: bp-based frame

sub_11D6A proc near

var_1A= word ptr -1Ah
var_18= word ptr -18h
var_16= word ptr -16h
var_14= byte ptr -14h
var_12= word ptr -12h
var_10= word ptr -10h
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
arg_0= word ptr  4
arg_2= word ptr  6
win= dword ptr  8

push    bp
mov     bp, sp
sub     sp, 1Ah
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_18], ax
mov     [bp+var_14], 0
mov     [bp+var_6], 0
mov     ax, 6
imul    [bp+arg_2]
mov     si, ax
mov     bx, [bp+var_18]
mov     al, 18h
mul     byte ptr [bx+si+62h]
mov     si, ax
mov     bx, _globals
mov     al, [bx+si+3Fh]
sub     ah, ah
mov     [bp+var_1A], ax
mov     ax, 3Ch ; '<'
imul    [bp+var_1A]
mov     si, ax
mov     bx, _weapons
mov     ax, [bx+si+3Ch]
add     ax, bx
mov     [bp+var_16], ax
mov     [bp+var_C], 0
mov     [bp+var_12], 0
jmp     short loc_11E1C

loc_11DC5:
inc     [bp+var_10]

loc_11DC8:
mov     bx, [bp+var_18]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_10]
jbe     short loc_11E15
mov     ax, 6
imul    [bp+var_10]
mov     si, ax
add     si, bx
mov     bx, [bp+var_16]
mov     al, [si+62h]
cmp     [bx+0Eh], al
jnz     short loc_11DC5
mov     al, byte ptr [bp+var_C]
mov     [si+63h], al
mov     bx, [bp+var_16]
mov     al, [bx+0Fh]
mov     cx, ax
mov     ax, 6
imul    [bp+var_10]
mov     di, ax
mov     bx, [bp+var_18]
mov     [bx+di+64h], cl
mov     bx, [bp+var_12]
shl     bx, 1
mov     ax, [bp+var_10]
mov     [bx-3240h], ax
inc     [bp+var_12]

loc_11E15:
inc     [bp+var_C]
add     [bp+var_16], 12h

loc_11E1C:
mov     ax, 3Ch ; '<'
imul    [bp+var_1A]
mov     si, ax
mov     bx, _weapons
mov     ax, [bp+var_C]
cmp     [bx+si+3Ah], ax
jle     short loc_11E37
mov     [bp+var_10], 0
jmp     short loc_11DC8

loc_11E37:
cmp     [bp+var_12], 0
jg      short loc_11E40
jmp     loc_11ECA

loc_11E40:
cmp     [bp+var_12], 1
jle     short loc_11EAC
mov     ax, _currentWindow
cmp     word ptr [bp+win], ax
jz      short loc_11E5D
mov     ax, 1
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
jmp     short loc_11E60

loc_11E5D:
call    winClearCurrent

loc_11E60:
cmp     byte ptr [bp+win+2], 0
jz      short loc_11E70
mov     ax, 1
push    ax
call    sub_12A34
add     sp, 2

loc_11E70:
sub     ax, ax
push    ax
mov     ax, 157Dh
push    ax
mov     ax, offset aLoadWhichAmmo ; "Load Which Ammo?"
push    ax
lea     ax, [bp+var_6]
push    ax
push    [bp+var_12]
push    [bp+var_18]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_A], ax
cmp     byte ptr [bp+win+2], 0
jz      short loc_11EB1
sub     ax, ax
push    ax
call    sub_12A34
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 1
push    ax
call    sub_147D8
add     sp, 4
jmp     short loc_11EB1

loc_11EAC:
mov     [bp+var_A], 0

loc_11EB1:
cmp     [bp+var_A], 0
jl      short loc_11EC2
mov     bx, [bp+var_A]
shl     bx, 1
mov     ax, [bx-3240h]
jmp     short loc_11EC5

loc_11EC2:
mov     ax, 0FFFFh

loc_11EC5:
mov     [bp+var_8], ax
jmp     short loc_11ECF

loc_11ECA:
mov     [bp+var_8], 0FFFEh

loc_11ECF:
mov     ax, [bp+var_8]
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_11D6A endp



; Attributes: bp-based frame

sub_11ED8 proc near

var_6= word ptr -6
var_4= word ptr -4
var_2= dword ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 6
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
mov     bx, [bp+arg_2]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_2], ax
mov     ax, 6
imul    [bp+arg_4]
mov     si, ax
add     si, [bp+var_6]
add     si, 62h ; 'b'
mov     al, [si]
sub     ah, ah
mov     [bp+var_4], ax
mov     bx, word ptr [bp+var_2]
cmp     word ptr [bx+44h], 0
jg      short loc_11F19
jmp     loc_11F9E

loc_11F19:
cmp     byte ptr [bx+52h], 20h ; ' '
jnb     short loc_11F7D
cmp     [bp+arg_6], ah
jnz     short loc_11F78
sub     ax, ax
push    ax
mov     al, 18h
mul     byte ptr [si]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    bx
call    sub_17B02
add     sp, 6
mov     ax, 6
imul    [bp+arg_4]
mov     di, ax
mov     bx, [bp+var_6]
lea     ax, [bx+di+62h]
mov     bx, word ptr [bp+var_2]
mov     cx, ax
mov     al, 6
mul     byte ptr [bx+52h]
mov     di, ax
push    si
lea     di, [bx+di+62h]
mov     si, cx
push    ds
pop     es
assume es:dseg
movsw
movsw
movsw
pop     si
mov     bx, word ptr [bp+var_2]
inc     byte ptr [bx+52h]
mov     ax, 0FFh
push    ax
push    [bp+arg_4]
push    [bp+arg_0]
call    sub_2060D
add     sp, 6

loc_11F78:
mov     ax, 1
jmp     short loc_11FA0

loc_11F7D:
cmp     [bp+arg_6], 0
jz      short loc_11F9E
push    word ptr [bp+var_2] ; char *
mov     ax, offset aSHasTooMuchToC ; "%s\rhas too much to carry already."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2

loc_11F9E:
sub     ax, ax

loc_11FA0:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_11ED8 endp




sub_11FA6 proc near
mov     ax, 3CBh
push    ds
push    ax
sub     ax, ax
push    ax
call    YesNo
add     sp, 6
sub     ah, ah
retn
sub_11FA6 endp



; Attributes: bp-based frame

sub_11FB7 proc near

var_22= byte ptr -22h
var_20= word ptr -20h
var_1C= byte ptr -1Ch
var_1A= word ptr -1Ah
var_18= word ptr -18h
var_16= byte ptr -16h
var_8= byte ptr -8
var_6= byte ptr -6
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= byte ptr  8

; FUNCTION CHUNK AT 2301 SIZE 00000005 BYTES

push    bp
mov     bp, sp
sub     sp, 22h
push    si
mov     [bp+var_22], 0
mov     bx, [bp+arg_0]
shl     bx, 1
mov     si, [bx-2518h]
mov     ax, 6
imul    [bp+arg_2]
mov     bx, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
mov     [bp+var_20], ax
mov     [bp+var_1A], si
mov     [bp+var_18], 0
mov     [bp+var_2], 0
cmp     [bp+arg_4], 0
jz      short loc_11FFE
push    si
call    sub_16F6B
add     sp, 2
jmp     short loc_1200D

loc_11FFE:
mov     bx, [bp+var_1A]
cmp     word ptr [bx+44h], 0
jle     short loc_1200B
mov     al, 1
jmp     short loc_1200D

loc_1200B:
sub     al, al

loc_1200D:
mov     [bp+var_16], al

loc_12010:
cmp     [bp+var_22], 0
jz      short loc_12019
jmp     loc_12301

loc_12019:
mov     [bp+var_1C], 0
mov     [bp+var_6], 0
mov     [bp+var_8], 0
mov     ax, 6
imul    [bp+arg_2]
sub_11FB7 endp ; sp-analysis failed

mov     si, ax



sub_1202D proc near
mov     bx, [bp-1Ah]
mov     al, [bx+si+62h]
mov     [bx+147h], al
mov     ax, 1
push    ax
mov     ax, offset _windowDialog
push    ax              ; win
call    setWindow
add     sp, 4
push    word ptr [bp-20h] ; char *
mov     ax, offset aDoWhatWithYour ; "Do what with your\r%s:\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
sub_1202D endp




sub_12065 proc near

; FUNCTION CHUNK AT 208D SIZE 00000066 BYTES
; FUNCTION CHUNK AT 20F3 SIZE 0000020E BYTES
; FUNCTION CHUNK AT 2301 SIZE 00000005 BYTES
; FUNCTION CHUNK AT 2306 SIZE 00000218 BYTES

cmp     byte ptr [bp-16h], 0
jz      short loc_120E7
cmp     byte ptr [bp+8], 0
jz      short loc_1208D
mov     bx, [bp-20h]
sub_12065 endp ; sp-analysis failed




sub_12074 proc near

; FUNCTION CHUNK AT 208D SIZE 00000066 BYTES

test    byte ptr [bx+14h], 1
jz      short loc_1208D
sub_12074 endp

cmp     byte ptr [bp+8], 0
jz      short loc_1209F
mov     bx, [bp-1Ah]
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+6]
jnz     short loc_1209F
; START OF FUNCTION CHUNK FOR sub_12065
;   ADDITIONAL PARENT FUNCTION sub_12074

loc_1208D:
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aUSe ; "U)se\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1209F:
cmp     byte ptr [bp+8], 0
jnz     short loc_120D5
mov     bx, [bp-20h]
test    byte ptr [bx+14h], 3
jnz     short loc_120BD
mov     al, [bx+14h]
sub     ah, ah
and     ax, 0Ch
shr     ax, 1
shr     ax, 1
mov     [bp-2], ax

loc_120BD:
cmp     word ptr [bp-2], 0
jle     short loc_120D5
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aINgest ; "I)ngest\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_120D5:
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aDRop ; "D)rop\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_120E7:
mov     word ptr [bp-14h], 0
mov     word ptr [bp-0Eh], 0
jmp     short loc_1211C
; END OF FUNCTION CHUNK FOR sub_12065
; START OF FUNCTION CHUNK FOR sub_12065

loc_120F3:
mov     ax, [bp+4]
cmp     [bp-14h], ax
jz      short loc_12119
mov     bx, [bp-14h]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0
jle     short loc_12119
mov     bx, [bp-0Eh]
inc     word ptr [bp-0Eh]
shl     bx, 1
mov     ax, [bp-14h]
mov     [bx-3240h], ax

loc_12119:
inc     word ptr [bp-14h]

loc_1211C:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp-14h], ax
jb      short loc_120F3
cmp     word ptr [bp-0Eh], 0
jle     short loc_1213E
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aTRade ; "T)rade\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1213E:
cmp     byte ptr [bp-16h], 0
jnz     short loc_12147
jmp     loc_1222C

loc_12147:
mov     bx, [bp-20h]
test    byte ptr [bx+14h], 1
jz      short loc_121A8
cmp     byte ptr [bp+8], 0
jnz     short loc_12190
mov     bx, [bp-1Ah]
cmp     byte ptr [bx+144h], 4
jnz     short loc_12165
mov     ax, 1
jmp     short loc_12167

loc_12165:
sub     ax, ax

loc_12167:
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aEQuipUnequipWe ; "E)quip-unequip weapon\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp-1Ah]
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+6]
jnz     short loc_1218C
mov     byte ptr [bp-6], 1
jmp     short loc_12190

loc_1218C:
mov     byte ptr [bp-1Ch], 1

loc_12190:
mov     bx, [bp-20h]
mov     al, 3Ch ; '<'
mul     byte ptr [bx+15h]
mov     si, ax
mov     bx, _weapons
mov     al, [bx+si+1Ah]
and     al, 1
mov     [bp-8], al
jmp     short loc_1220E

loc_121A8:
cmp     byte ptr [bp+8], 0
jnz     short loc_1220E
mov     bx, [bp-20h]
test    byte ptr [bx+14h], 2
jz      short loc_1220E
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aEQuipUnequipAr ; "E)quip-unequip armor\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     word ptr [bp-12h], 0
jmp     short loc_121D3

loc_121D0:
inc     word ptr [bp-12h]

loc_121D3:
cmp     word ptr [bp-12h], 3
jge     short loc_121ED
mov     si, [bp-12h]
mov     bx, [bp-1Ah]
mov     al, [bx+si+4Bh]
sub     ah, ah
cmp     ax, [bp+6]
jnz     short loc_121D0
mov     byte ptr [bp-6], 1

loc_121ED:
cmp     byte ptr [bp-6], 1
sbb     ax, ax
neg     ax
mov     [bp-1Ch], al
or      al, al
jz      short loc_1220E
mov     bx, [bp-20h]
mov     al, [bx+14h]
sub     ah, ah
and     ax, 0Ch
shr     ax, 1
shr     ax, 1
mov     [bp-12h], ax

loc_1220E:
cmp     byte ptr [bp+8], 0
jnz     short loc_1222C
cmp     byte ptr [bp-8], 0
jz      short loc_1222C
mov     ax, 0FFFFh
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aLOadUnjam ; "L)oad/unjam"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1222C:
call    sub_1DC28
call    getKeypress
mov     [bp-0Ah], al
cmp     al, 0FFh
jnz     short loc_1223C
jmp     loc_12301

loc_1223C:
cbw
cmp     ax, 44h ; 'D'
jnz     short loc_12245
jmp     loc_12306

loc_12245:
cmp     ax, 45h ; 'E'
jnz     short loc_1224D
jmp     loc_12405

loc_1224D:
cmp     ax, 49h ; 'I'
jnz     short loc_12255
jmp     loc_122E9

loc_12255:
cmp     ax, 4Ch ; 'L'
jnz     short loc_1225D
jmp     loc_12461

loc_1225D:
cmp     ax, 54h ; 'T'
jnz     short loc_12265
jmp     loc_12339

loc_12265:
cmp     ax, 55h ; 'U'
jz      short loc_1226D
jmp     loc_12010

loc_1226D:
cmp     byte ptr [bp-16h], 0
jnz     short loc_12276
jmp     loc_12010

loc_12276:
mov     ax, 6
imul    word ptr [bp+6]
mov     si, ax
mov     bx, [bp-1Ah]
test    byte ptr [bx+si+65h], 1
jz      short loc_122B4
call    sub_11FA6
or      al, al
jnz     short loc_12291
jmp     loc_12010

loc_12291:
cmp     byte ptr [bp+8], 0
jnz     short loc_122A6
sub     ax, ax
push    ax
push    word ptr [bp+4]
call    sub_16DD4
add     sp, 4
jmp     loc_12010

loc_122A6:
mov     bx, [bp-1Ah]
mov     byte ptr [bx+144h], 6

loc_122AE:
mov     ax, 1
jmp     loc_12519

loc_122B4:
cmp     byte ptr [bp-8], 0
jz      short loc_122DF
mov     ax, 6
imul    word ptr [bp+6]
mov     si, ax
mov     bx, [bp-1Ah]
cmp     byte ptr [bx+si+64h], 0
jnz     short loc_122DF
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 469h
push    ax
call    sub_1738A
add     sp, 6
jmp     loc_12010

loc_122DF:
mov     bx, [bp-1Ah]
mov     byte ptr [bx+144h], 0Bh
jmp     short loc_122AE

loc_122E9:
cmp     word ptr [bp-2], 0
jg      short loc_122F2
jmp     loc_12010

loc_122F2:
push    word ptr [bp+6]
push    word ptr [bp-2]
push    word ptr [bp+4]
call    sub_1C42B

loc_122FE:
add     sp, 6
; END OF FUNCTION CHUNK FOR sub_12065
; START OF FUNCTION CHUNK FOR sub_11FB7
;   ADDITIONAL PARENT FUNCTION sub_12065

loc_12301:
sub     ax, ax
jmp     loc_12519
; END OF FUNCTION CHUNK FOR sub_11FB7
; START OF FUNCTION CHUNK FOR sub_12065

loc_12306:
cmp     byte ptr [bp-16h], 0
jnz     short loc_1230F
jmp     loc_12010

loc_1230F:
cmp     byte ptr [bp+8], 0
jnz     short loc_12329
mov     ax, 0FFh
push    ax
push    word ptr [bp+6]
push    word ptr [bp+4]
call    sub_2060D
add     sp, 6
jmp     short loc_12331

loc_12329:
mov     bx, [bp-1Ah]
mov     byte ptr [bx+144h], 0Dh

loc_12331:
mov     al, [bp+8]
sub     ah, ah
jmp     loc_12519

loc_12339:
cmp     word ptr [bp-0Eh], 0
jg      short loc_12342
jmp     loc_12010

loc_12342:              ; char *
push    word ptr [bp-20h]
mov     ax, offset aWhoWillTakeYou ; "Who will take your\r%-1.19s?"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
sub     ax, ax
push    ax
mov     ax, 13A1h
push    ax
mov     ax, offset sprintf_buffer
push    ax
lea     ax, [bp-18h]
push    ax
push    word ptr [bp-0Eh]
push    word ptr [bp-1Ah]
call    sub_11682
add     sp, 0Ch
mov     [bp-1Eh], ax
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp-0Ch]
push    ax
mov     ax, [bp-1Eh]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jnz     short loc_12394
jmp     loc_12010

loc_12394:
mov     ax, [bp+4]
cmp     [bp-0Ch], ax
jnz     short loc_1239F
jmp     loc_12010

loc_1239F:
mov     bx, [bp-0Ch]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp-4], ax
push    word ptr [bp-0Ch]
call    sub_1CB58
add     sp, 2
or      al, al
jnz     short loc_123BB
jmp     loc_12010

loc_123BB:
mov     ax, 1
push    ax
push    word ptr [bp+6]
push    word ptr [bp-0Ch]
push    word ptr [bp+4]
call    sub_11ED8
add     sp, 8
or      al, al
jnz     short loc_123D5
jmp     loc_12010

loc_123D5:
cmp     byte ptr [bp+8], 0
jz      short loc_123F0
mov     bx, [bp-1Ah]
mov     byte ptr [bx+144h], 0Ch
mov     bx, [bp-1Ah]
mov     al, [bp-0Ch]
mov     [bx+146h], al
jmp     loc_12331

loc_123F0:
sub     ax, ax
push    ax
push    word ptr [bp+6]
push    word ptr [bp-0Ch]
push    word ptr [bp+4]
call    sub_11ED8
add     sp, 8
jmp     loc_12331

loc_12405:
cmp     byte ptr [bp-1Ch], 0
jnz     short loc_12414
cmp     byte ptr [bp-6], 0
jnz     short loc_12414
jmp     loc_12010

loc_12414:
mov     bx, [bp-20h]
test    byte ptr [bx+14h], 1
jz      short loc_12432
cmp     byte ptr [bp-1Ch], 0
jz      short loc_12428
mov     al, [bp+6]
jmp     short loc_1242A

loc_12428:
mov     al, 0FFh

loc_1242A:
mov     bx, [bp-1Ah]
mov     [bx+4Ah], al
jmp     short loc_12448

loc_12432:
cmp     byte ptr [bp-1Ch], 0
jz      short loc_1243D
mov     al, [bp+6]
jmp     short loc_1243F

loc_1243D:
mov     al, 0FFh

loc_1243F:
mov     si, [bp-12h]
mov     bx, [bp-1Ah]
mov     [bx+si+4Bh], al

loc_12448:
cmp     byte ptr [bp-1Ch], 0
jnz     short loc_12451
jmp     loc_12301

loc_12451:
mov     ax, 1
push    ax
push    word ptr [bp-20h]
push    word ptr [bp-1Ah]
call    sub_17B02
jmp     loc_122FE

loc_12461:
cmp     byte ptr [bp+8], 0
jz      short loc_1246A
jmp     loc_12010

loc_1246A:
cmp     byte ptr [bp-8], 0
jnz     short loc_12473
jmp     loc_12010

loc_12473:
mov     ax, 6
imul    word ptr [bp+6]
mov     si, ax
mov     bx, [bp-1Ah]
test    byte ptr [bx+si+65h], 1
jz      short loc_1249D
call    sub_11FA6
or      al, al
jnz     short loc_1248E
jmp     loc_12010

loc_1248E:
sub     ax, ax
push    ax
push    word ptr [bp+4]
call    sub_16DD4
add     sp, 4
jmp     loc_12301

loc_1249D:
sub     ax, ax
push    ax
push    _currentWindow
push    word ptr [bp+6]
push    word ptr [bp+4]
call    sub_11D6A
add     sp, 8
mov     [bp-12h], ax
or      ax, ax
jl      short loc_124FF
mov     ax, 6
imul    word ptr [bp-12h]
mov     si, ax
mov     bx, [bp-1Ah]
mov     al, [bx+si+63h]
mov     cx, ax
mov     ax, 6
imul    word ptr [bp+6]
mov     si, ax
mov     [bx+si+63h], cl
mov     ax, 6
imul    word ptr [bp-12h]
mov     si, ax
mov     bx, [bp-1Ah]
mov     al, [bx+si+64h]
mov     cx, ax
mov     ax, 6
imul    word ptr [bp+6]
mov     si, ax
mov     [bx+si+64h], cl
mov     ax, 0FFh
push    ax
push    word ptr [bp-12h]
push    word ptr [bp+4]
call    sub_2060D
jmp     loc_122FE

loc_124FF:
cmp     word ptr [bp-12h], 0FFFEh
jz      short loc_12508
jmp     loc_12301

loc_12508:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 49Bh
push    ax
call    sub_1738A
jmp     loc_122FE

loc_12519:
pop     si
mov     sp, bp
pop     bp
retn
; END OF FUNCTION CHUNK FOR sub_12065


; Attributes: bp-based frame

sub_1251E proc near

arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
mov     al, [bp+arg_0]
cbw
mov     _disk1._mapPosX, ax
mov     al, [bp+arg_2]
cbw
mov     _disk1._mapPosY, ax
mov     word_2D128, 0
sub     ax, ax
mov     word ptr dword_2EC04+2, ax
mov     word ptr dword_2EC04, ax
pop     bp
retn
sub_1251E endp



; Attributes: bp-based frame

sub_1253F proc near

var_16= dword ptr -16h
var_12= word ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= byte ptr -0Ah
var_4= byte ptr -4
var_2= byte ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 16h
push    di
push    si              ; draw
mov     ax, _party
mov     [bp+var_12], ax
mov     [bp+var_2], 0
sub     ax, ax
push    ax
mov     ax, offset _windowInfo
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
call    sub_1DC28
mov     [bp+var_C], 0
jmp     short loc_1256E

loc_1256B:
inc     [bp+var_C]

loc_1256E:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_C], ax
jnb     short loc_12590
mov     bx, [bp+var_C]
shl     bx, 1
mov     si, [bx-2518h]
cmp     word ptr [si+44h], 0
jle     short loc_1256B
cmp     [si+57h], ah
jz      short loc_1256B
mov     [bp+var_2], 1

loc_12590:
cmp     [bp+arg_2], 0
jnz     short loc_125CA
cmp     [bp+var_2], 0
jz      short loc_125CA
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     ax, 0Ah
ja      short loc_125CA
mov     ax, 4
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+arg_0], al
mov     ax, 4
push    ax
call    sub_1E376
add     sp, 2

loc_125CA:
mov     al, [bp+arg_0]
cbw
mov     si, ax
mov     di, si
shl     di, 1
mov     al, [di+1DCh]
add     al, byte ptr _disk1._mapPosX
mov     [bp+var_4], al
mov     al, [di+1DDh]
add     al, byte ptr _disk1._mapPosY
mov     [bp+var_A], al
push    si
call    sub_1DFC6
add     sp, 2
cmp     [bp+var_4], 0
jl      short loc_12614
mov     al, [bp+var_4]
cbw
les     bx, dword_22C6E
assume es:nothing
cmp     ax, es:[bx]
jnb     short loc_12614
cmp     [bp+var_A], 0
jl      short loc_12614
mov     al, [bp+var_A]
cbw
cmp     ax, es:[bx+2]
jb      short loc_1262A

loc_12614:
les     bx, dword_22C6E
mov     al, es:[bx+0Dh]
sub     ah, ah
push    ax
call    sub_1E376
add     sp, 2
sub     ax, ax
jmp     loc_12755

loc_1262A:
mov     al, byte ptr _disk1._mapPosX
mov     byte_2E423, al
mov     al, byte ptr _disk1._mapPosY
mov     byte_2E428, al
les     bx, dword_22C6E
mov     ax, es:[bx]
mul     _disk1._mapPosY
add     ax, _disk1._mapPosX
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
mov     [bp+var_10], ax
mov     [bp+var_E], dx
mov     byte_2DFEE, 1
mov     byte_2EE06, 0
call    sub_1DFEE
sub     ax, ax
push    ax
mov     ax, 1
push    ax
mov     ax, word ptr dword_2E4C6
mov     dx, word ptr dword_2E4C6+2
add     ax, 6
push    dx
push    ax
call    sub_11834
add     sp, 8
les     bx, dword_22C6E
mov     ax, es:[bx]
mul     _disk1._mapPosY
add     ax, _disk1._mapPosX
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
cmp     [bp+var_10], ax
jnz     short loc_126AE
cmp     [bp+var_E], dx
jz      short loc_126B3

loc_126AE:
mov     byte_2DFEE, 0

loc_126B3:
cmp     byte_2DFEE, 0
jnz     short loc_126BD
jmp     loc_127AC

loc_126BD:
mov     al, [bp+var_4]
mov     byte_2E423, al
mov     al, [bp+var_A]
mov     byte_2E428, al
cbw
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [bp+var_4]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
mov     [bp+var_C], 0
jmp     short loc_126F3

loc_126F0:
inc     [bp+var_C]

loc_126F3:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_C]
jbe     short loc_1275E
mov     ax, 68h ; 'h'
imul    [bp+var_C]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
mov     al, [bp+var_4]
cmp     es:[bx+5Bh], al
jnz     short loc_126F0
mov     al, [bp+var_A]
cmp     es:[bx+5Ch], al
jnz     short loc_126F0
and     byte ptr es:[bx+50h], 7Fh
push    [bp+var_C]
mov     ax, 6
push    ax
mov     ax, 68h ; 'h'
imul    [bp+var_C]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
add     ax, 66h ; 'f'
push    dx
push    ax
call    sub_11834
add     sp, 8
mov     al, [bp+arg_2]
sub     ah, ah

loc_12755:
push    ax
call    nothing
add     sp, 2
jmp     short loc_127C6

loc_1275E:
mov     al, [bp+var_A]
cbw
push    ax
mov     al, [bp+var_4]
cbw
push    ax
call    sub_12C0F
add     sp, 4
or      al, al
jz      short loc_12785
mov     al, [bp+var_A]
cbw
push    ax
mov     al, [bp+var_4]
cbw
push    ax
call    sub_1251E
add     sp, 4
call    sub_1DD4C

loc_12785:
call    sub_112E9
sub     ax, ax
push    ax
push    ax
mov     ax, word ptr dword_2E4C6
mov     dx, word ptr dword_2E4C6+2
add     ax, 6
push    dx
push    ax
call    sub_11834
add     sp, 8
cmp     [bp+arg_2], 0
jnz     short loc_127A7
call    sub_11339

loc_127A7:
call    sub_1DD4C
jmp     short loc_127B8

loc_127AC:
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
call    nothing
add     sp, 2

loc_127B8:
push    _disk1._mapPosY
push    _disk1._mapPosX
call    sub_17FA8
add     sp, 4

loc_127C6:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1253F endp



; Attributes: bp-based frame

sub_127CC proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
call    sub_1B452
cmp     [bp+arg_0], 0FFFFh
jz      short loc_127EC
mov     byte_2EE06, 0
sub     ax, ax
push    ax
push    [bp+arg_0]
call    sub_11CEA
add     sp, 4
call    sub_107CB

loc_127EC:
pop     bp
retn
sub_127CC endp



; Attributes: bp-based frame

sub_127EE proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
jmp     short loc_127FE

loc_127FB:
inc     [bp+var_2]

loc_127FE:
cmp     [bp+var_2], 7D00h
jge     short loc_1280F
call    sub_1DEDB
call    _kbhit
or      ax, ax
jz      short loc_127FB

loc_1280F:
mov     sp, bp
pop     bp
retn
sub_127EE endp



; Attributes: noreturn bp-based frame

; int __cdecl main(int argc, const char **argv, const char **envp)
_main proc near

var_10= word ptr -10h
var_8= word ptr -8
win= dword ptr -6
key= byte ptr -2
argc= word ptr  4
argv= dword ptr  6
envp= dword ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 10h
push    si              ; draw
call    gfxInit
call    loadData
call    sub_101BE
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, _disk1._maps[bx]
sub     ah, ah
push    ax
call    sub_1008C
add     sp, 2
call    drawBordersAndWelcome
sub     ax, ax
push    ax
call    writeParty
add     sp, 2
call    writeTime
call    sub_10D78
call    sub_1DC46
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
mov     word_2D126, 0
mov     [bp+var_8], 0

loc_12860:
mov     bx, [bp+var_8]
shl     bx, 1
mov     word ptr [bx-2438h], 0FFFFh
inc     [bp+var_8]
cmp     [bp+var_8], 3
jl      short loc_12860
mov     word_2E204, 0
mov     word_2D128, 0
sub     ax, ax
mov     word ptr dword_2EC04+2, ax
mov     word ptr dword_2EC04, ax
mov     byte_2ED26, 0
mov     byte_2E4BE, 0
mov     _quitGame, 0

loc_12897:
call    sub_127EE
call    _kbhit
or      ax, ax
jnz     short loc_128A4
jmp     loc_12A0B

loc_128A4:
call    getKeypress
mov     [bp+key], al
call    sub_16D5E
mov     al, [bp+key]
cbw
cmp     ax, -3
jnz     short loc_128B9
jmp     loc_129BB

loc_128B9:
jle     short loc_128BE
jmp     loc_129E0

loc_128BE:
cmp     ax, 0FFFAh
jnz     short loc_128C6
jmp     loc_129CB

loc_128C6:
cmp     ax, 0FFFBh
jnz     short loc_128CE
jmp     loc_129D3

loc_128CE:
cmp     ax, 0FFFCh
jnz     short loc_128D6
jmp     loc_129C3

loc_128D6:
jmp     loc_12A14

loc_128D9:
call    sub_1D5BA

loc_128DC:
mov     [bp+var_10], ax
push    ax
call    sub_127CC
jmp     loc_12A11

loc_128E6:
mov     ax, 1
push    ax
call    sub_14F1A
add     sp, 2
mov     word_2F01A, ax
or      ax, ax
jg      short loc_128FA
jmp     loc_12A14

loc_128FA:
mov     ax, 1
push    ax
push    ax
call    sub_15691

loc_12902:
add     sp, 4
jmp     loc_12A14

loc_12908:
mov     byte ptr [bp+win+2], 0

loc_1290C:
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     ax, 1
push    ax
mov     ax, offset _windowDialog
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset aYourChoice1Qui ; "Your choice?\r\r\r1. Quit (without savi"...
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
call    getKeypress
mov     [bp+key], al
cbw
mov     bx, ax
test    byte_2CEDB[bx], 2
jz      short loc_12948
cbw
sub     ax, ' '
jmp     short loc_1294C

loc_12948:
mov     al, [bp+key]
cbw

loc_1294C:
cmp     ax, '1'
jz      short loc_12967
cmp     ax, 33h ; '3'
jz      short loc_1296E
cmp     ax, 35h ; '5'
jz      short loc_12973
cmp     ax, 4Eh ; 'N'
jz      short loc_12981
cmp     ax, 0FFFFh
jz      short loc_12981
jmp     short loc_12985

loc_12967:
mov     _quitGame, 1
jmp     short loc_12981

loc_1296E:
mov     _quitGame, 1

loc_12973:
mov     al, byte_2132A
cbw
push    ax
call    sub_187E5
add     sp, 2
call    sub_18827

loc_12981:
mov     byte ptr [bp+win+2], 1

loc_12985:
cmp     byte ptr [bp+win+2], 0
jz      short loc_1290C
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
call    sub_1DD4C
jmp     short loc_12A14

loc_1299C:
mov     al, [bp+key]
cbw
mov     si, ax
sub     si, 59
mov     al, _disk1._partyCount
sub     ah, ah
cmp     ax, si
jbe     short loc_12A14
sub     ax, ax
push    ax
push    si
call    characterInfo
add     sp, 4
jmp     loc_128DC

loc_129BB:
sub     ax, ax
push    ax
mov     ax, 1
jmp     short loc_129D9

loc_129C3:
sub     ax, ax
push    ax
mov     ax, 2
jmp     short loc_129D9

loc_129CB:
sub     ax, ax
push    ax
mov     ax, 3
jmp     short loc_129D9

loc_129D3:
sub     ax, ax
push    ax
mov     ax, 4

loc_129D9:
push    ax
call    sub_1253F
jmp     loc_12902

loc_129E0:
cmp     ax, 3Fh ; '?'
jg      short loc_129F1
cmp     ax, 3Bh ; ';'
jge     short loc_1299C
cmp     ax, 0FFFFh
jz      short loc_12A0B
jmp     short loc_12A14

loc_129F1:
cmp     ax, 41h ; 'A'
jnz     short loc_129F9
jmp     loc_128D9

loc_129F9:
cmp     ax, 45h ; 'E'
jnz     short loc_12A01
jmp     loc_128E6

loc_12A01:
cmp     ax, 51h ; 'Q'
jnz     short loc_12A09
jmp     loc_12908

loc_12A09:
jmp     short loc_12A14

loc_12A0B:
sub     ax, ax
push    ax
call    nothing

loc_12A11:
add     sp, 2

loc_12A14:
cmp     _quitGame, 0
jnz     short loc_12A1E
jmp     loc_12897

loc_12A1E:
call    restoreCriticalFn
call    sub_20720
sub     ax, ax
push    ax              ; int
call    _exit
_main endp

add     sp, 2
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_12A34 proc near

var_2= word ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 2
push    si
cmp     [bp+arg_0], 0
jz      short loc_12A46
mov     ax, 18h
jmp     short loc_12A49

loc_12A46:
mov     ax, 30h ; '0'

loc_12A49:
mov     bx, word_22E66
mov     [bx+4], ax
cmp     [bp+arg_0], 0
jz      short loc_12A5B
mov     ax, 18h
jmp     short loc_12A5E

loc_12A5B:
mov     ax, 30h ; '0'

loc_12A5E:
mov     bx, word_22E9E
mov     [bx+4], ax
cmp     [bp+arg_0], 0
jz      short loc_12A70
mov     ax, 84h
jmp     short loc_12A73

loc_12A70:
mov     ax, 6Ch ; 'l'

loc_12A73:
mov     bx, word_22E9E
mov     [bx], ax
mov     al, [bp+arg_0]
mov     byte_21328, al
mov     [bp+var_2], 0
jmp     short loc_12A89

loc_12A86:
inc     [bp+var_2]

loc_12A89:
mov     ax, word_2D126
cmp     [bp+var_2], ax
jge     short loc_12AAD
mov     ax, 0Ch
imul    [bp+var_2]
mov     si, ax
cmp     word ptr [si-413Eh], 1C28h
jnz     short loc_12A86
cmp     [bp+arg_0], 1
sbb     ax, ax
neg     ax
mov     [si-4134h], al

loc_12AAD:
pop     si
mov     sp, bp
pop     bp
retn
sub_12A34 endp



; Attributes: bp-based frame

sub_12AB2 proc near

var_4= dword ptr -4
arg_0= byte ptr  4
arg_2= byte ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
push    si
mov     al, [bp+arg_0]
cbw
mov     si, ax
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
add     ax, si
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
cmp     es:[bx], si
jbe     short loc_12AF7
les     bx, [bp+var_4]
mov     ax, es:[bx]
and     ah, 7
mov     dx, [bp+arg_4]
mov     cl, 0Bh
shl     dx, cl
or      ax, dx
mov     es:[bx], ax

loc_12AF7:
pop     si
mov     sp, bp
pop     bp
retn
sub_12AB2 endp



; Attributes: bp-based frame

; int __cdecl writeShortMemberName(char *member, int)
writeShortMemberName proc near

name= byte ptr -2Ah
member= dword ptr  4
y= word ptr  8

push    bp
mov     bp, sp
sub     sp, 2Ah
push    si
push    word ptr [bp+member+2]
push    word ptr [bp+member] ; char *
mov     ax, offset aFs  ; "%Fs"
push    ax
lea     ax, [bp+name]
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     bx, _currentWindow
mov     ax, word ptr [bx+Window.field_10]
cwd
xor     ax, dx
sub     ax, dx
mov     cx, 2
sar     ax, cl
xor     ax, dx
sub     ax, dx
mov     si, ax
mov     [bp+si+name], 0
push    [bp+y]          ; y
lea     ax, [bp+name]
push    ax              ; char *
call    _strlen
add     sp, 2
mov     bx, _currentWindow
mov     cx, ax
mov     ax, word ptr [bx+Window.field_10]
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
lea     ax, [bp+name]
push    ss
push    ax              ; text
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
writeShortMemberName endp



; Attributes: noreturn bp-based frame

quit proc near
push    bp
mov     bp, sp
mov     word_2D126, 0
mov     ax, offset aFloridaBidsYou ; "Florida bids you a fond farewell!"
push    ax
call    sub_1B29B
add     sp, 2
call    sub_20720
sub     ax, ax
push    ax              ; int
call    _exit
quit endp

add     sp, 2
pop     bp
retn


; Attributes: bp-based frame

sub_12B93 proc near

var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 4
mov     [bp+var_4], 0
mov     [bp+var_2], 0
jmp     short loc_12BBA

loc_12BA5:
mov     bx, [bp+var_4]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0FFF1h
jle     short loc_12BB7
inc     [bp+var_2]

loc_12BB7:
inc     [bp+var_4]

loc_12BBA:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_12BA5
cmp     [bp+var_2], 0
jnz     short loc_12C0B
cmp     [bp+arg_0], ah
jz      short loc_12BFF
mov     [bp+var_4], 0
jmp     short loc_12BE7

loc_12BD6:
mov     bx, [bp+var_4]
shl     bx, 1
mov     bx, [bx-2518h]
mov     word ptr [bx+44h], 0FFBFh
inc     [bp+var_4]

loc_12BE7:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_12BD6
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
call    sub_107E1
add     sp, 6

loc_12BFF:
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
call    quit
add     sp, 2

loc_12C0B:
mov     sp, bp
pop     bp
retn
sub_12B93 endp



; Attributes: bp-based frame

sub_12C0F proc near

var_A= dword ptr -0Ah
var_6= dword ptr -6
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
cmp     [bp+arg_2], 0
jl      short loc_12C39
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
cmp     ax, es:[bx+2]
jnb     short loc_12C39
cmp     [bp+arg_0], 0
jl      short loc_12C39
mov     al, [bp+arg_0]
cbw
cmp     ax, es:[bx]
jb      short loc_12C3E

loc_12C39:
sub     ax, ax
jmp     loc_12CF0

loc_12C3E:
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     si, ax
mov     al, [bp+arg_0]
cbw
add     si, ax
mov     cl, 3
shl     si, cl
test    byte ptr es:[bx+si+416h], 1
jz      short loc_12C39
mov     [bp+var_2], 0
jmp     short loc_12C67

loc_12C64:
inc     [bp+var_2]

loc_12C67:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_2]
jbe     short loc_12CA1
mov     ax, 68h ; 'h'
imul    [bp+var_2]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_A], ax
mov     word ptr [bp+var_A+2], dx
les     bx, [bp+var_A]
mov     al, [bp+arg_0]
cmp     es:[bx+5Bh], al
jnz     short loc_12C64
mov     al, [bp+arg_2]
cmp     es:[bx+5Ch], al
jnz     short loc_12C64
jmp     short loc_12C39

loc_12CA1:
mov     [bp+var_2], 0
jmp     short loc_12CAB

loc_12CA8:
inc     [bp+var_2]

loc_12CAB:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_2]
jbe     short loc_12CED
mov     ax, 66h ; 'f'
imul    [bp+var_2]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_6], ax
mov     word ptr [bp+var_6+2], dx
les     bx, [bp+var_6]
test    byte ptr es:[bx+9], 0Fh
jz      short loc_12CA8
mov     al, [bp+arg_0]
cmp     es:[bx+2], al
jnz     short loc_12CA8
mov     al, [bp+arg_2]
cmp     es:[bx+3], al
jnz     short loc_12CA8
jmp     loc_12C39

loc_12CED:
mov     ax, 1

loc_12CF0:
pop     si
mov     sp, bp
pop     bp
retn
sub_12C0F endp




sub_12CF5 proc near
mov     ax, 576h
mov     word_2EC00, ax
mov     word_2EC02, ds
mov     ax, 580h
mov     word_2E4BA, ax
mov     word_2E4BC, ds
retn
sub_12CF5 endp



; Attributes: bp-based frame

sub_12D0A proc near

var_18= word ptr -18h
var_16= dword ptr -16h
var_12= byte ptr -12h
var_10= byte ptr -10h
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= dword ptr -6
var_2= byte ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 18h
sub     ax, ax
mov     word ptr [bp+var_6+2], ax
mov     word ptr [bp+var_6], ax
mov     [bp+var_2], 0
mov     [bp+var_8], 0FFFFh
mov     [bp+var_A], 0FFFFh
mov     [bp+var_18], 0FFFFh
call    sub_10B45
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [bp+arg_0]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
and     byte ptr es:[bx+2], 0BFh
call    sub_1DD4C
mov     al, [bp+arg_0]
mov     [bp+var_10], al
mov     al, [bp+arg_2]
mov     [bp+var_12], al
jmp     loc_12E4D

loc_12D6E:
mov     al, [bp+var_12]
cbw
push    ax
mov     al, [bp+var_10]
cbw
push    ax
call    sub_10A87
add     sp, 4
mov     [bp+var_C], ax
inc     ax
jnz     short loc_12D87
jmp     loc_12E49

loc_12D87:
cmp     [bp+var_C], 5
jnz     short loc_12D90
jmp     loc_12E49

loc_12D90:
mov     al, [bp+arg_0]
cmp     [bp+var_10], al
jnz     short loc_12DB1
mov     al, [bp+arg_2]
cmp     [bp+var_12], al
jnz     short loc_12DB1
mov     ax, [bp+var_C]
mov     [bp+var_A], ax
mov     bx, ax
shl     bx, 1
mov     ax, [bx+52Ch]
mov     [bp+var_18], ax

loc_12DB1:
mov     ax, [bp+var_A]
cmp     [bp+var_C], ax
jz      short loc_12DC4
mov     ax, [bp+var_18]
cmp     [bp+var_C], ax
jz      short loc_12DC4
jmp     loc_12E4D

loc_12DC4:
mov     al, byte_2E428
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     cx, ax
mov     al, byte_2E423
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr [bp+var_6], ax
mov     word ptr [bp+var_6+2], dx
mov     ax, word ptr [bp+var_16]
mov     dx, word ptr [bp+var_16+2]
cmp     word ptr [bp+var_6], ax
jnz     short loc_12DFA
cmp     word ptr [bp+var_6+2], dx
jz      short loc_12E4D

loc_12DFA:
les     bx, [bp+var_16]
test    byte ptr es:[bx+2], 1
jnz     short loc_12E16
mov     ax, [bp+var_8]
cmp     [bp+var_C], ax
jz      short loc_12E4D
les     bx, [bp+var_6]
test    byte ptr es:[bx+2], 1
jz      short loc_12E4D

loc_12E16:
mov     al, byte_2E423
mov     [bp+var_10], al
mov     al, byte_2E428
mov     [bp+var_12], al
mov     ax, [bp+var_C]
mov     [bp+var_8], ax
les     bx, [bp+var_16]
and     byte ptr es:[bx+2], 0BFh
mov     ax, word ptr [bp+var_6]
mov     dx, word ptr [bp+var_6+2]
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
or      byte ptr es:[bx+2], 40h
call    sub_1DD4C
jmp     short loc_12E4D

loc_12E49:
mov     [bp+var_2], 1

loc_12E4D:
cmp     [bp+var_2], 0
jnz     short loc_12E56
jmp     loc_12D6E

loc_12E56:
les     bx, [bp+var_16]
and     byte ptr es:[bx+2], 0BFh
call    sub_1DD4C
cmp     [bp+var_C], 0FFFFh
jz      short loc_12E6F
mov     ax, word ptr [bp+var_16]
mov     dx, word ptr [bp+var_16+2]
jmp     short loc_12E72

loc_12E6F:
sub     ax, ax
cwd

loc_12E72:
mov     sp, bp
pop     bp
retn
sub_12D0A endp



; Attributes: bp-based frame

sub_12E76 proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
cmp     byte_2E442, 1
jz      short loc_12E8A
cmp     byte_2E442, 2
jnz     short loc_12EAA

loc_12E8A:
mov     [bp+var_2], 0
jmp     short loc_12EA0

loc_12E91:
mov     ax, 3
push    ax
call    sub_21130
add     sp, 2
inc     [bp+var_2]

loc_12EA0:
mov     al, byte_2E443
sub     ah, ah
cmp     [bp+var_2], ax
jb      short loc_12E91

loc_12EAA:
mov     sp, bp
pop     bp
retn
sub_12E76 endp

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     al, [bx-3240h]
mov     [bp-2], al
push    word ptr [bp+4]
mov     ax, offset a1d  ; "%1d> "
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 6
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ds
push    ax
call    writeStringAt
add     sp, 8
cmp     byte ptr [bp-2], 0FFh
jnz     short loc_12F01
mov     ax, [bp+4]
inc     ax
push    ax
mov     ax, 0FFFFh
push    ax
mov     ax, offset aHands_0 ; "Hands"
push    ds
push    ax
call    writeStringAt
add     sp, 8
jmp     short loc_12F3F

loc_12F01:
mov     al, 6
mul     byte ptr [bp-2]
mov     si, ax
mov     bx, [bp+8]
test    byte ptr [bx+si+65h], 1
jz      short loc_12F16
mov     _inverseText, 0FFh

loc_12F16:
mov     ax, [bp+4]
inc     ax
push    ax
mov     ax, 0FFFFh
push    ax
mov     al, 6
mul     byte ptr [bp-2]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ds
push    ax
call    writeStringAt
add     sp, 8
mov     _inverseText, 0

loc_12F3F:
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_12F44 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
push    si
mov     ax, 18h
imul    [bp+arg_0]
mov     si, ax
mov     bx, _globals
mov     al, 3Ch ; '<'
mul     byte ptr [bx+si+3Fh]
add     ax, _weapons
inc     ax
inc     ax
pop     si
pop     bp
retn
sub_12F44 endp



; Attributes: bp-based frame

sub_12F62 proc near

var_4E= dword ptr -4Eh
var_4A= word ptr -4Ah
var_48= word ptr -48h
var_46= word ptr -46h
member= word ptr -44h
var_3E= word ptr -3Eh
var_3C= word ptr -3Ch
var_3A= word ptr -3Ah
var_38= word ptr -38h
var_36= word ptr -36h
var_34= dword ptr -34h
var_30= word ptr -30h
var_2E= word ptr -2Eh
var_2C= word ptr -2Ch
var_2A= word ptr -2Ah
var_26= word ptr -26h
var_24= word ptr -24h
var_22= word ptr -22h
var_20= word ptr -20h
var_1E= dword ptr -1Eh
var_1A= byte ptr -1Ah
var_18= word ptr -18h
var_16= word ptr -16h
var_14= word ptr -14h
var_12= dword ptr -12h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4Eh
push    di
push    si
mov     [bp+var_8], 0
mov     [bp+var_3C], 0
mov     [bp+var_C], 0
mov     [bp+var_4A], 1
sub     ax, ax
mov     word ptr [bp+var_1E+2], ax
mov     word ptr [bp+var_1E], ax
mov     [bp+var_1A], 0
mov     bx, [bp+arg_2]
mov     al, [bx]
sub     ah, ah
or      ax, ax
jz      short loc_12FA8
cmp     ax, 1
jnz     short loc_12F9D
jmp     loc_130A1

loc_12F9D:
cmp     ax, 2
jnz     short loc_12FA5
jmp     loc_1317A

loc_12FA5:
jmp     loc_1307F

loc_12FA8:
mov     bl, [bx+1]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
mov     bx, ax
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_12FCC
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
jmp     short loc_12FCF

loc_12FCC:
mov     ax, 0FFh

loc_12FCF:
mov     [bp+var_3A], ax
cmp     ax, 0FFh
jz      short loc_12FE5
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+63h]
sub     ah, ah
jmp     short loc_12FE7

loc_12FE5:
sub     ax, ax

loc_12FE7:
mov     [bp+var_26], ax
mov     bx, [bp+member]
mov     si, [bx+48h]
and     si, 0FFh
mov     cl, 7
shl     si, cl
les     bx, _professions
mov     ax, es:[bx+si+76h]
mov     [bp+var_18], ax
mov     bx, [bp+member]
mov     si, [bx+48h]
and     si, 0FFh
shl     si, cl
mov     bx, word ptr _professions
mov     ax, es:[bx+si+78h]
mov     [bp+var_24], ax
mov     bx, [bp+member]
mov     al, [bx+18h]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
shl     ax, 1
mov     [bp+var_8], ax
mov     bx, [bp+member]
mov     al, [bx+38h]
sub     ah, ah
shl     ax, 1
mov     cl, [bx+51h]
sub     ch, ch
add     ax, cx
mov     [bp+var_22], ax
mov     al, [bx+1Ah]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
mov     bx, [bp+member]
mov     cl, [bx+1Eh]
sub     ch, ch
push    cx
mov     si, ax
call    sub_16F3A
add     sp, 2
add     ax, si
shl     ax, 1
add     [bp+var_22], ax
mov     ax, [bp+member]
mov     word ptr [bp+var_1E], ax
mov     word ptr [bp+var_1E+2], ds
mov     bx, ax
mov     al, [bx+144h]
mov     byte_2E442, al
mov     al, [bx+146h]
mov     byte_2E443, al

loc_1307F:
mov     bx, [bp+arg_2]
mov     al, [bx+3]
sub     ah, ah
or      ax, ax
jnz     short loc_1308E
jmp     loc_13202

loc_1308E:
cmp     ax, 1
jnz     short loc_13096
jmp     loc_132D2

loc_13096:
cmp     ax, 2
jnz     short loc_1309E
jmp     loc_13351

loc_1309E:
jmp     loc_13250

loc_130A1:
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
les     bx, [bp+var_12]
cmp     byte ptr es:[bx+4Ah], 0FFh
jz      short loc_130CD
mov     si, es:[bx+4Ah]
and     si, 0FFh
mov     al, es:[bx+si+53h]
cbw
jmp     short loc_130D0

loc_130CD:
mov     ax, 0FFh

loc_130D0:
mov     [bp+var_3A], ax
mov     [bp+var_26], 0
mov     si, es:[bx+48h]
and     si, 0FFh
mov     cl, 7
shl     si, cl
les     bx, _professions
mov     ax, es:[bx+si+76h]
mov     [bp+var_18], ax
les     bx, [bp+var_12]
mov     si, es:[bx+48h]
and     si, 0FFh
shl     si, cl
les     bx, _professions
mov     ax, es:[bx+si+78h]
mov     [bp+var_24], ax
les     bx, [bp+var_12]
mov     al, es:[bx+18h]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
shl     ax, 1
mov     [bp+var_8], ax
les     bx, [bp+var_12]
mov     al, es:[bx+38h]
sub     ah, ah
shl     ax, 1
mov     cl, es:[bx+51h]
sub     ch, ch
add     ax, cx
mov     [bp+var_22], ax
mov     al, es:[bx+1Ah]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
les     bx, [bp+var_12]
mov     cl, es:[bx+1Eh]
sub     ch, ch
push    cx
mov     si, ax
call    sub_16F3A
add     sp, 2
add     ax, si
shl     ax, 1
add     [bp+var_22], ax
mov     ax, word ptr [bp+var_12]
mov     dx, word ptr [bp+var_12+2]
mov     word ptr [bp+var_1E], ax
mov     word ptr [bp+var_1E+2], dx
les     bx, [bp+var_12]
mov     al, es:[bx+60h]
cbw
mov     [bp+var_2E], ax
mov     al, es:[bx+61h]
cbw
mov     [bp+var_36], ax
jmp     loc_1307F

loc_1317A:
mov     bx, [bp+arg_2]
mov     al, 16h
mul     byte ptr [bx+2]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [bx+1]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     word ptr [bp+var_34], ax
mov     word ptr [bp+var_34+2], dx
les     bx, [bp+var_34]
mov     al, es:[bx+13h]
sub     ah, ah
mov     [bp+var_3A], ax
mov     al, es:[bx+10h]
cbw
mov     [bp+var_2E], ax
mov     al, es:[bx+11h]
cbw
mov     [bp+var_36], ax
mov     al, es:[bx]
sub     ah, ah
mov     [bp+var_16], ax
mov     [bp+var_26], 0
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word ptr [bp+var_4E], ax
mov     word ptr [bp+var_4E+2], dx
les     bx, [bp+var_4E]
mov     ax, es:[bx+2Ch]
mov     [bp+var_18], ax
mov     ax, es:[bx+2Eh]
mov     [bp+var_24], ax
mov     al, es:[bx+3Ah]
sub     ah, ah
mov     [bp+var_22], ax
mov     al, es:[bx+3Bh]
mov     [bp+var_1A], al
mov     al, es:[bx+3Ch]
mov     [bp+var_4], al
jmp     loc_1307F

loc_13202:
mov     bx, [bp+arg_2]
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
mov     bx, ax
mov     al, [bx+1Eh]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
shl     ax, 1
mov     bx, [bp+arg_2]
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
mov     cx, [bx-2B76h]
mov     bx, [bp+member]
mov     dl, [bx+40h]
sub     dh, dh
shl     dx, 1
add     cx, dx
mov     dl, [bx+43h]
sub     dh, dh
shl     dx, 1
add     cx, dx
add     cx, ax
mov     [bp+var_30], cx
mov     [bp+var_3C], 0FFC4h

loc_13250:
cmp     [bp+var_3A], 0FFh
jnz     short loc_1325A
jmp     loc_13491

loc_1325A:
push    [bp+var_3A]
call    sub_12F44
add     sp, 2
mov     [bp+var_2C], ax
mov     ax, 18h
imul    [bp+var_3A]
mov     si, ax
add     si, _globals
mov     al, [si+40h]
mov     [bp+var_1A], al
mov     al, [si+41h]
mov     [bp+var_4], al
mov     [bp+var_8], 0
mov     bx, [bp+var_2C]
test    byte ptr [bx+18h], 2
jnz     short loc_1328F
jmp     loc_133BB

loc_1328F:
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     [bp+var_46], ax
mov     ax, 12h
imul    [bp+var_26]
add     ax, [bp+var_46]
mov     [bp+var_38], ax
mov     bx, ax
mov     ax, [bx+0Ah]
mov     [bp+var_18], ax
mov     ax, [bx+0Ch]
mov     [bp+var_24], ax
push    [bp+var_36]
push    [bp+var_2E]
call    sub_10795
add     sp, 4
sub     ah, ah
mov     [bp+var_3E], ax
mov     di, ax
mov     bx, [bp+var_38]
mov     al, [bx+di-1]
mov     [bp+var_22], ax
jmp     loc_133F9

loc_132D2:
mov     bx, [bp+arg_2]
mov     al, 68h ; 'h'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
les     bx, [bp+var_12]
cmp     byte ptr es:[bx+4Bh], 0FFh
jz      short loc_13301
mov     si, es:[bx+4Bh]
and     si, 0FFh
mov     al, es:[bx+si+53h]
cbw
jmp     short loc_13304

loc_13301:
mov     ax, 0FFh

loc_13304:
mov     [bp+var_2], ax
mov     si, es:[bx+48h]
and     si, 0FFh
mov     cl, 7
shl     si, cl
les     bx, _professions
mov     al, es:[bx+si+7Ah]
sub     ah, ah
mov     [bp+var_20], ax
les     bx, [bp+var_12]
mov     al, es:[bx+1Eh]
push    ax
call    sub_16F3A
add     sp, 2
les     bx, [bp+var_12]
mov     cl, es:[bx+1Ah]
sub     ch, ch
add     cx, ax
shl     cx, 1
mov     [bp+var_30], cx
mov     al, es:[bx+60h]
cbw
mov     [bp+var_2E], ax
mov     al, es:[bx+61h]

loc_1334A:
cbw
mov     [bp+var_36], ax
jmp     loc_13250

loc_13351:
mov     bx, [bp+arg_2]
mov     al, 16h
mul     byte ptr [bx+5]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     word ptr [bp+var_34], ax
mov     word ptr [bp+var_34+2], dx
mov     [bp+var_2], 0FFh
les     bx, [bp+var_34]
mov     si, es:[bx]
and     si, 0FFh
mov     cl, 6
shl     si, cl
les     bx, _enemies
mov     al, es:[bx+si+39h]
sub     ah, ah
mov     [bp+var_20], ax
les     bx, [bp+var_34]
mov     si, es:[bx]
and     si, 0FFh
shl     si, cl
les     bx, _enemies
mov     al, es:[bx+si+3Dh]
mov     [bp+var_30], ax
les     bx, [bp+var_34]
mov     al, es:[bx+10h]
cbw
mov     [bp+var_2E], ax
mov     al, es:[bx+11h]
jmp     short loc_1334A

loc_133BB:
mov     bx, [bp+var_2C]
mov     ax, [bx+14h]
mov     [bp+var_18], ax
mov     ax, [bx+16h]
mov     [bp+var_24], ax
mov     al, [bx+19h]
sub     ah, ah
mov     [bp+var_22], ax
mov     ax, word ptr [bp+var_1E]
or      ax, word ptr [bp+var_1E+2]
jz      short loc_133F9
les     bx, [bp+var_1E]
mov     al, es:[bx+18h]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
mov     cx, ax
mov     bx, [bp+var_2C]
mov     al, [bx+1Ch]
sub     ah, ah
mul     cx
mov     [bp+var_8], ax

loc_133F9:
mov     ax, word ptr [bp+var_1E]
or      ax, word ptr [bp+var_1E+2]
jz      short loc_13460
les     bx, [bp+var_1E]
mov     al, es:[bx+1Ah]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
mov     cx, ax
mov     bx, [bp+var_2C]
mov     al, [bx+1Eh]
sub     ah, ah
mul     cx
add     [bp+var_22], ax
les     bx, [bp+var_1E]
mov     al, es:[bx+1Eh]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
mov     cx, ax
mov     bx, [bp+var_2C]
mov     al, [bx+22h]
sub     ah, ah
mul     cx
add     [bp+var_22], ax
mov     [bp+var_2A], 0

loc_13444:
mov     si, [bp+var_2A]
mov     bx, [bp+var_2C]
mov     al, [bx+si+28h]
les     bx, [bp+var_1E]
mul     byte ptr es:[bx+si+34h]
add     [bp+var_22], ax
inc     [bp+var_2A]
cmp     [bp+var_2A], 10h
jl      short loc_13444

loc_13460:
mov     bx, [bp+arg_2]
cmp     byte ptr [bx], 2
jnz     short loc_1347C
mov     si, [bp+var_16]
mov     cl, 6
shl     si, cl
les     bx, _enemies
mov     al, es:[bx+si+3Dh]
sub     ah, ah
add     [bp+var_22], ax

loc_1347C:
mov     al, byte_2E442
sub     ah, ah
cmp     ax, 1
jz      short loc_1348D
cmp     ax, 2
jz      short loc_134AB
jmp     short loc_13491

loc_1348D:
add     [bp+var_22], 0Ah

loc_13491:
mov     ax, [bp+var_30]
sub     [bp+var_22], ax
jns     short loc_1349E
mov     [bp+var_22], 0

loc_1349E:
cmp     byte_2E43E, 0
jz      short loc_134B1
mov     ax, [bp+var_22]
jmp     loc_136B2

loc_134AB:
add     [bp+var_22], 14h
jmp     short loc_13491

loc_134B1:
cmp     word_2E43C, 0
jle     short loc_134BD
mov     [bp+var_22], 64h ; 'd'

loc_134BD:
mov     bx, [bp+arg_2]
cmp     byte ptr [bx+3], 0
jnz     short loc_134D1
push    [bp+member]     ; member
call    getArmorClass
add     sp, 2
jmp     short loc_134EE

loc_134D1:
cmp     [bp+var_2], 0FFh
jz      short loc_134EB
mov     ax, 18h
imul    [bp+var_2]
mov     si, ax
mov     bx, _globals
mov     al, [bx+si+3Fh]
sub     ah, ah
jmp     short loc_134EE

loc_134EB:
mov     ax, [bp+var_20]

loc_134EE:
mov     [bp+var_E], ax
mov     bx, _globals
mov     al, [bx]
sub     ah, ah
mov     cx, ax
mov     ax, [bp+var_E]
mul     cx
mov     cx, [bp+var_24]
add     cx, [bp+var_8]
cmp     ax, cx
jb      short loc_1350D
jmp     loc_136AA

loc_1350D:
mov     [bp+var_2A], 0
mov     [bp+var_48], 0
mov     bx, [bp+arg_0]
mov     word ptr [bx], 0
jmp     loc_1361D

loc_13521:
inc     [bp+var_2A]

loc_13524:
cmp     [bp+var_2A], 10h
jge     short loc_13587
mov     si, [bp+var_2A]
mov     bx, [bp+member]
cmp     byte ptr [bx+si+34h], 0
jz      short loc_13521
mov     bx, [bp+var_2C]
cmp     byte ptr [bx+si+28h], 0
jz      short loc_13521
mov     ax, si
add     ax, 10h
push    ax
mov     bx, [bp+arg_2]
mov     al, [bx+1]
sub     ah, ah
push    ax
call    sub_1969F
add     sp, 4
mov     bx, [bp+member]
mov     [bx+22h], al
mov     bx, [bp+member]
mov     al, byte ptr [bp+var_2A]
add     al, 10h
mov     [bx+23h], al
jmp     short loc_13521

loc_13567:
mov     ax, 14h
push    ax
mov     bx, [bp+arg_2]
mov     al, [bx+1]
sub     ah, ah
push    ax
call    sub_1969F
add     sp, 4
mov     bx, [bp+member]
mov     [bx+22h], al
mov     bx, [bp+member]
mov     byte ptr [bx+23h], 14h

loc_13587:
inc     [bp+var_48]
mov     bx, _globals
mov     al, [bx+1]
sub     ah, ah
mul     [bp+var_E]
push    ax
mov     al, [bx]
sub     ah, ah
mov     cx, ax
mov     ax, [bp+var_E]
mul     cx
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+var_6], ax
push    [bp+var_24]
push    [bp+var_18]
call    sub_1DEB1
add     sp, 4
mov     cx, ax
mov     ax, [bp+var_8]
add     ax, cx
sub     dx, dx
div     word_2E43A
sub     ax, [bp+var_6]
mov     [bp+var_A], ax
or      ax, ax
jge     short loc_135D4
mov     [bp+var_A], 0

loc_135D4:
cmp     [bp+var_A], 0
jle     short loc_1361A
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     cl, [bp+var_1A]
sub     ch, ch
cmp     cx, ax
jb      short loc_135FB
mov     bx, [bp+arg_4]
mov     al, [bp+var_4]
inc     al
mov     [bx], al

loc_135FB:
mov     bx, [bp+arg_0]
mov     ax, [bp+var_A]
add     [bx], ax
les     bx, dword_2E42A
mov     ax, es:[bx]
mov     bx, [bp+arg_0]
sub     ax, [bx]
cmp     ax, [bp+var_3C]
jg      short loc_1361A
mov     ax, 4
jmp     loc_136B2

loc_1361A:
inc     [bp+var_2A]

loc_1361D:
mov     al, byte_2E443
sub     ah, ah
cmp     [bp+var_2A], ax
jnb     short loc_13687
mov     bx, [bp+arg_2]
cmp     [bx], ah
jnz     short loc_13657
cmp     [bp+var_3A], 0FFh
jz      short loc_13657
cmp     word_2E43C, 0
jnz     short loc_13657
mov     bx, [bp+member]
mov     al, [bx+4Ah]
push    ax
mov     bx, [bp+arg_2]
mov     al, [bx+1]
push    ax
call    sub_11A62
add     sp, 4
mov     bx, [bp+member]
dec     byte ptr [bx+146h]

loc_13657:
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_22], ax
jb      short loc_1361A
mov     bx, [bp+arg_2]
cmp     byte ptr [bx], 0
jz      short loc_13675
jmp     loc_13587

loc_13675:
cmp     [bp+var_3A], 0FFh
jnz     short loc_1367F
jmp     loc_13567

loc_1367F:
mov     [bp+var_2A], 0
jmp     loc_13524

loc_13687:
cmp     [bp+var_48], 0
jnz     short loc_13694
mov     [bp+var_14], 0
jmp     short loc_136AF

loc_13694:
mov     bx, [bp+arg_0]
cmp     word ptr [bx], 0
jnz     short loc_136A3
mov     [bp+var_14], 1
jmp     short loc_136AF

loc_136A3:
mov     [bp+var_14], 2
jmp     short loc_136AF

loc_136AA:
mov     [bp+var_14], 3

loc_136AF:
mov     ax, [bp+var_14]

loc_136B2:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_12F62 endp



; Attributes: bp-based frame

sub_136B8 proc near

arg_0= dword ptr  4
arg_4= word ptr  8

push    bp
mov     bp, sp
les     bx, [bp+arg_0]
cmp     byte ptr es:[bx+5Dh], 0
jz      short loc_136CC
cmp     byte ptr es:[bx+5Dh], 0FFh
jnz     short loc_136E2

loc_136CC:
mov     ax, 0FFFFh
cwd
push    dx
push    ax
push    [bp+arg_4]
call    __aNNalmul
les     bx, [bp+arg_0]
mov     byte ptr es:[bx+5Dh], 0FFh
jmp     short loc_13714

loc_136E2:
les     bx, [bp+arg_0]
cmp     byte ptr es:[bx+5Dh], 3Dh ; '='
jnb     short loc_13700
mov     bx, [bp+arg_4]
sub     ax, ax
mov     [bx+2], ax
mov     [bx], ax
les     bx, [bp+arg_0]
add     byte ptr es:[bx+5Dh], 3Ch ; '<'
jmp     short loc_13714

loc_13700:
les     bx, [bp+arg_0]
cmp     byte ptr es:[bx+5Dh], 79h ; 'y'
jnb     short loc_13714
mov     bx, [bp+arg_4]
sub     ax, ax
mov     [bx+2], ax
mov     [bx], ax

loc_13714:
pop     bp
retn
sub_136B8 endp



; Attributes: bp-based frame

sub_13716 proc near

var_28= word ptr -28h

push    bp
mov     bp, sp
sub     sp, 34h
push    di
push    si
mov     [bp+var_28], 0
jmp     loc_13885
sub_13716 endp

; START OF FUNCTION CHUNK FOR sub_13838

loc_13726:
mov     di, [bp-34h]
shl     di, 1
les     bx, [bp-32h]
mov     si, es:[bx+di+2]
or      si, si
jle     short loc_13753
mov     di, [bp-4]
shl     di, 1
mov     es:[bx+di+2], si
mov     di, [bp-34h]
les     bx, [bp-32h]
mov     al, es:[bx+di+0Ah]
mov     di, [bp-4]
inc     word ptr [bp-4]
mov     es:[bx+di+0Ah], al

loc_13753:
inc     word ptr [bp-34h]

loc_13756:
les     bx, [bp-32h]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp-34h]
ja      short loc_13726
mov     al, [bp-4]
mov     es:[bx+0Fh], al
cmp     word ptr [bp-4], 0
jnz     short loc_137B0
les     bx, [bp-2Ch]
mov     al, 1
mov     cl, [bp-2Eh]
shl     al, cl
not     al
and     es:[bx+9], al
les     bx, [bp-32h]
cmp     es:[bx+15h], ah
jz      short loc_13799
mov     al, es:[bx+15h]
dec     al
mov     si, [bp-26h]
inc     word ptr [bp-26h]
mov     [bp+si-24h], al

loc_13799:
les     bx, [bp-32h]
cmp     byte ptr es:[bx+13h], 0FFh
jz      short loc_137B0
mov     al, es:[bx+13h]
mov     si, [bp-2]
inc     word ptr [bp-2]
mov     [bp+si-14h], al

loc_137B0:
inc     word ptr [bp-2Eh]

loc_137B3:
les     bx, [bp-2Ch]
mov     al, es:[bx+8]
sub     ah, ah
cmp     ax, [bp-2Eh]
jbe     short loc_137F1
mov     al, es:[bx+9]
mov     dx, 1
mov     cl, [bp-2Eh]
shl     dx, cl
test    ax, dx
jz      short loc_137B0
mov     ax, 16h
imul    word ptr [bp-2Eh]
add     ax, bx
mov     dx, es
add     ax, 0Eh
mov     [bp-32h], ax
mov     [bp-30h], dx
mov     word ptr [bp-34h], 0
mov     word ptr [bp-4], 0
jmp     loc_13756

loc_137F1:
mov     word ptr [bp-2Eh], 0
jmp     short loc_1382B

loc_137F8:
mov     si, [bp-2Eh]
mov     al, [bp+si-14h]
mov     bx, _packetsSize
mov     cx, ax
mov     ax, 6
imul    word ptr [bx+5Eh]
mov     si, ax
mov     [bx+si+60h], cl
les     bx, [bp-2Ch]
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
mov     ax, 17h
push    ax
call    sub_182CA
add     sp, 6
inc     word ptr [bp-2Eh]

loc_1382B:
mov     ax, [bp-2]
cmp     [bp-2Eh], ax
jl      short loc_137F8
mov     word ptr [bp-2Eh], 0
; END OF FUNCTION CHUNK FOR sub_13838



sub_13838 proc near

; FUNCTION CHUNK AT 3726 SIZE 00000112 BYTES

jmp     short loc_1385B

loc_1383A:
les     bx, [bp-2Ch]
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
mov     si, [bp-2Eh]
mov     al, [bp+si-24h]
sub     ah, ah
push    ax
call    sub_182CA
add     sp, 6
inc     word ptr [bp-2Eh]

loc_1385B:
mov     ax, [bp-26h]
cmp     [bp-2Eh], ax
jl      short loc_1383A
les     bx, [bp-2Ch]
test    byte ptr es:[bx+9], 0Fh
jnz     short loc_13882
sub     ax, ax
push    ax
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
call    sub_12AB2
add     sp, 6

loc_13882:
inc     word ptr [bp-28h]

loc_13885:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp-28h]
jbe     short loc_138C4
mov     ax, 66h ; 'f'
imul    word ptr [bp-28h]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp-2Ch], ax
mov     [bp-2Ah], dx
les     bx, [bp-2Ch]
test    byte ptr es:[bx+9], 0Fh
jz      short loc_13882
mov     word ptr [bp-2Eh], 0
mov     word ptr [bp-2], 0
mov     word ptr [bp-26h], 0
jmp     loc_137B3

loc_138C4:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_13838 endp ; sp-analysis failed




sub_138CA proc near
push    word_2DFFE
push    word_2DFFC
push    word_2E4BC
push    word_2E4BA
push    word_2EC02
push    word_2EC00      ; char *
mov     ax, offset aFsFsFs ; "%Fs %Fs %Fs"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 10h
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
retn
sub_138CA endp



; Attributes: bp-based frame

sub_13901 proc near

var_16= word ptr -16h
var_8= word ptr -8

push    bp
mov     bp, sp
sub     sp, 20h
push    si
mov     [bp+var_16], 0FFh
mov     [bp+var_8], 0
mov     word_2E43A, 1
mov     word_2E43C, 0
mov     byte_2E43E, 0
mov     byte_2E442, 0
mov     byte_2E443, 1
mov     byte_2E444, 4
sub_13901 endp ; sp-analysis failed

mov     bx, [bp+4]
mov     al, [bx]
sub     ah, ah
or      ax, ax
jz      short loc_1394F
cmp     ax, 1
jnz     short loc_13945
jmp     loc_13A06

loc_13945:
cmp     ax, 2
jnz     short loc_1394D
jmp     loc_13A71

loc_1394D:
jmp     short loc_139A9

loc_1394F:
mov     bl, [bx+1]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp-1Eh], ax
mov     word_2EC00, ax
mov     word_2EC02, ds
mov     bx, ax
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_1397A
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
jmp     short loc_1397D

loc_1397A:
mov     ax, 0FFh

loc_1397D:
mov     [bp-16h], ax
mov     al, [bx+48h]
sub     ah, ah
mov     cl, 7
shl     ax, cl
add     ax, word ptr _professions
mov     dx, word ptr _professions+2
add     ax, 58h ; 'X'
mov     word_2E4BA, ax
mov     word_2E4BC, dx
mov     al, [bx+144h]
mov     byte_2E442, al
mov     al, [bx+146h]
mov     byte_2E443, al

loc_139A9:
push    word ptr [bp+4]
call    sub_2023E
add     sp, 2
cmp     word ptr [bp-16h], 0FFh
jnz     short loc_139BE
jmp     loc_13B6B

loc_139BE:
mov     ax, 18h
imul    word ptr [bp-16h]
mov     si, ax
mov     bx, _globals
mov     al, 3Ch ; '<'
mul     byte ptr [bx+si+3Fh]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp-0Eh], ax
mov     word_2E4BA, ax
mov     word_2E4BC, ds
mov     bx, [bp+4]
cmp     byte ptr [bx], 0
jz      short loc_139EA
jmp     loc_13ADF

loc_139EA:
mov     bx, [bp-1Eh]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+63h]
sub     ah, ah
mov     [bp-8], ax
mov     al, [bx+144h]
mov     [bp-18h], al
jmp     loc_13AE3

loc_13A06:
mov     bx, [bp+4]
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     [bp-6], ax
mov     [bp-4], dx
mov     word_2EC00, ax
mov     word_2EC02, dx
les     bx, [bp-6]
cmp     byte ptr es:[bx+4Ah], 0FFh
jz      short loc_13A3C
mov     si, es:[bx+4Ah]
and     si, 0FFh
mov     al, es:[bx+si+53h]
cbw
jmp     short loc_13A3F

loc_13A3C:
mov     ax, 0FFh

loc_13A3F:
mov     [bp-16h], ax
mov     al, es:[bx+48h]
sub     ah, ah
mov     cl, 7
shl     ax, cl
add     ax, word ptr _professions
mov     dx, word ptr _professions+2
add     ax, 58h ; 'X'
mov     word_2E4BA, ax
mov     word_2E4BC, dx
mov     ax, 64h ; 'd'
imul    word ptr es:[bx+44h]
sub     dx, dx
div     word ptr es:[bx+46h]

loc_13A6B:
mov     [bp-2], ax
jmp     loc_139A9

loc_13A71:
mov     bx, [bp+4]
mov     al, 16h
mul     byte ptr [bx+2]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [bx+1]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     [bp-12h], ax
mov     [bp-10h], dx
les     bx, [bp-12h]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word_2EC00, ax
mov     word_2EC02, dx
mov     al, es:[bx+13h]
sub     ah, ah
mov     [bp-16h], ax
mov     al, es:[bx]
shl     ax, cl
add     ax, word ptr _enemies
add     ax, 18h
mov     word_2E4BA, ax
mov     word_2E4BC, dx
mov     ax, 32h ; '2'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     cx, 0Ah
mul     cx
jmp     short loc_13A6B

loc_13ADF:
mov     byte ptr [bp-18h], 3

loc_13AE3:
mov     bx, [bp-0Eh]
test    byte ptr [bx+18h], 2
jz      short loc_13B6B
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     [bp-20h], ax
mov     ax, 12h
imul    word ptr [bp-8]
add     ax, [bp-20h]
mov     [bp-14h], ax
cmp     byte ptr [bp-18h], 3
jnz     short loc_13B3B
mov     bx, ax
cmp     byte ptr [bx+10h], 1
jbe     short loc_13B3B
cmp     word ptr [bp-2], 32h ; '2'
jnb     short loc_13B2D
mov     byte_2E442, 2
mov     al, [bx+0Fh]
sub     ah, ah
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
jmp     short loc_13B38

loc_13B2D:
mov     byte_2E442, 1
mov     bx, [bp-14h]
mov     al, [bx+10h]

loc_13B38:
mov     byte_2E443, al

loc_13B3B:
mov     bx, [bp-14h]
mov     al, [bx+11h]
sub     ah, ah
mov     word_2E43C, ax
mov     al, byte_2E442
cmp     ax, 1
jz      short loc_13B55
cmp     ax, 2
jz      short loc_13B66
jmp     short loc_13B6B

loc_13B55:
mov     ax, 5A4h

loc_13B58:
mov     word_2E4BA, ax
mov     word_2E4BC, ds
mov     byte_2E444, 4
jmp     short loc_13B6B

loc_13B66:
mov     ax, 5B8h
jmp     short loc_13B58

loc_13B6B:
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_13B70 proc near

var_8= dword ptr -8
var_4= dword ptr -4
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 10h
mov     bx, [bp+arg_0]
mov     al, [bx+3]
sub     ah, ah
cmp     ax, 1
jz      short loc_13B8A
cmp     ax, 2
jz      short loc_13BBD
jmp     short loc_13BFF

loc_13B8A:
mov     al, 68h ; 'h'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
mov     bl, es:[bx+49h]
sub     bh, bh
or      byte ptr [bx-3A60h], 2
sub     ax, ax
push    ax
les     bx, [bp+var_4]
mov     al, es:[bx+5Ch]
cbw
push    ax
mov     al, es:[bx+5Bh]
jmp     short loc_13BF7

loc_13BBD:
mov     bx, [bp+arg_0]
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
mov     ax, 1
mov     cl, [bx+5]
shl     ax, cl
not     ax
les     bx, [bp+var_8]
mov     cl, es:[bx+9]
sub     ch, ch
test    ax, cx
jnz     short loc_13BFF
sub     ax, ax
push    ax
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]

loc_13BF7:
cbw
push    ax
call    sub_12AB2
add     sp, 6

loc_13BFF:
mov     sp, bp
pop     bp
retn
sub_13B70 endp



; Attributes: bp-based frame

; int __cdecl sub_13C03(char *)
sub_13C03 proc near

arg_0= dword ptr  4

push    bp
mov     bp, sp
push    word ptr [bp+arg_0] ; char *
mov     ax, offset aForDPtsDamage ; " for %d pts damage.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
pop     bp
retn
sub_13C03 endp



; Attributes: bp-based frame

sub_13C29 proc near

var_8= dword ptr -8
var_4= dword ptr -4
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     byte ptr [bp+var_4+2], 0
lea     ax, [bp+var_4+2]
push    ax
push    [bp+arg_0]
lea     ax, [bp+var_8]
push    ax
call    sub_12F62
add     sp, 6
mov     word ptr [bp+var_8+2], ax
cmp     byte_2E43E, 0
jz      short loc_13C55

loc_13C4F:
mov     ax, word ptr [bp+var_8+2]
jmp     loc_13E74

loc_13C55:
mov     ax, word ptr [bp+var_8+2]
or      ax, ax
jz      short loc_13C75
cmp     ax, 1
jz      short loc_13CD4
cmp     ax, 2
jz      short loc_13CE2
cmp     ax, 3
jnz     short loc_13C6E
jmp     loc_13E43

loc_13C6E:
cmp     ax, 4
jz      short loc_13CE2
jmp     short loc_13C4F

loc_13C75:
call    sub_138CA
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aAndMisses_0 ; " and misses.\r\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+3], 0
jnz     short loc_13C4F
mov     al, [bx+4]
sub     ah, ah
mov     si, ax
mov     bx, si
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0
jle     short loc_13C4F
mov     ax, 1Ch
push    ax
push    si
call    sub_1969F
add     sp, 4
or      al, al
jz      short loc_13C4F
mov     ax, 1
push    ax              ; char
mov     ax, 1Ch
push    ax
mov     bx, [bp+arg_0]
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h] ; char *
call    sub_16F9D

loc_13CCE:
add     sp, 6
jmp     loc_13C4F

loc_13CD4:
call    sub_138CA
mov     ax, 0FFFFh
push    ax
push    ax
mov     ax, 5F0h
jmp     loc_13E69

loc_13CE2:
call    sub_138CA
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+3], 0
jz      short loc_13CF1
jmp     loc_13DD6

loc_13CF1:
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_4], ax
push    word ptr [bp+var_8]
push    ax
call    sub_18FFA
add     sp, 4
cmp     word ptr [bp+var_8+2], 4
jnz     short loc_13D3F
mov     bx, word ptr [bp+var_4]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+53Ch] ; char *
mov     ax, offset aAndKillsS ; " and kills %s!\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     loc_13DC8

loc_13D3F:              ; char *
push    word ptr [bp+var_8]
call    sub_13C03
add     sp, 2
cmp     byte ptr [bp+var_4+2], 0
jz      short loc_13DC8
dec     byte ptr [bp+var_4+2]
cmp     byte ptr [bp+var_4+2], 7
jnz     short loc_13D77
mov     ax, 64h ; 'd'
push    ax
push    word ptr [bp+var_4]
call    sub_17AAB
add     sp, 4
push    word ptr [bp+var_4] ; char *
mov     ax, offset aSHasBeenExpose ; "%s has been exposed to mutagens!\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
jmp     short loc_13DB5

loc_13D77:
mov     bx, word ptr [bp+var_4]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_4+2]
shl     dx, cl
test    ax, dx
jnz     short loc_13DC8
mov     al, 1
shl     al, cl
or      [bx+58h], al
mov     al, 16h
mul     byte ptr [bp+var_4+2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 3EEh
push    dx
push    ax
push    word ptr [bp+var_4] ; char *
mov     ax, offset aSHasBecomeFs ; "%s has become %Fs!\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah

loc_13DB5:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DDFD

loc_13DC8:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
call    sub_107E1
jmp     loc_13CCE

loc_13DD6:
mov     bx, [bp+arg_0]
mov     bl, [bx+1]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_4], ax
les     bx, dword_2E42A
mov     ax, word ptr [bp+var_8]
sub     es:[bx], ax
cmp     word ptr [bp+var_8+2], 4
jnz     short loc_13E37
mov     bx, word_2E434
shl     bx, 1
push    word ptr [bx+53Ch] ; char *
mov     ax, offset aAndWastesS ; " and wastes %s!\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, word_2E42E
mov     ax, word_2E430
mov     dx, word_2E432
add     [bx], ax
adc     [bx+2], dx
push    [bp+arg_0]
call    sub_13B70
jmp     short loc_13E3D

loc_13E37:              ; char *
push    word ptr [bp+var_8]
call    sub_13C03

loc_13E3D:
add     sp, 2
jmp     loc_13C4F

loc_13E43:
push    word_2DFFE
push    word_2DFFC
push    word_2EC02
push    word_2EC00      ; char *
mov     ax, offset aFsSWeaponIsUse ; "%Fs 's weapon is useless against %Fs.\r"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ch
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer

loc_13E69:
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     loc_13C4F

loc_13E74:
pop     si
mov     sp, bp
pop     bp
retn
sub_13C29 endp



; Attributes: bp-based frame

sub_13E79 proc near

var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ch
push    si
cmp     [bp+arg_0], 0
jg      short loc_13E89
jmp     loc_13FA5

loc_13E89:
mov     bx, [bp+arg_2]
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h] ; member
call    getMemberCon
add     sp, 2
cmp     ax, 5
jnz     short loc_13EA5
jmp     loc_13FA5

loc_13EA5:
mov     [bp+var_A], 0
jmp     loc_13F57

loc_13EAD:
mov     byte_2E43E, 1
mov     ax, 5
push    ax
call    sub_21130
add     sp, 2
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
mov     [bp+var_C], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_C], ax
jb      short loc_13EE3
mov     word_2E43A, 2

loc_13EE3:
mov     ax, word_2E43A
mov     [bp+var_4], ax
mov     byte_2E43E, 0
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
mov     bx, [bp+arg_2]
mov     al, [bx+4]
mov     [bp+var_2], al
mov     [bp+var_6], 0
jmp     short loc_13F4C

loc_13F07:
mov     ax, 0Ch
imul    [bp+var_6]
add     ax, 0D494h
mov     [bp+var_8], ax
mov     bx, ax
cmp     byte ptr [bx], 0
jnz     short loc_13F49
mov     al, [bp+var_2]
cmp     [bx+1], al
jz      short loc_13F49
mov     bx, [bp+arg_2]
mov     si, [bp+var_8]
mov     al, [si+1]
mov     [bx+4], al
push    [bp+arg_2]
call    sub_13901
add     sp, 2
call    sub_12CF5
mov     ax, [bp+var_4]
mov     word_2E43A, ax
push    [bp+arg_2]
call    sub_13C29
add     sp, 2

loc_13F49:
inc     [bp+var_6]

loc_13F4C:
mov     ax, word_2F01A
cmp     [bp+var_6], ax
jl      short loc_13F07

loc_13F54:
inc     [bp+var_A]

loc_13F57:
mov     bx, [bp+arg_2]
mov     al, [bx+0Ah]
sub     ah, ah
cmp     ax, [bp+var_A]
jbe     short loc_13FA5
lea     ax, [bp+arg_0]
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, ss
push    bx
call    sub_13901
add     sp, 2
mov     bx, [bp+arg_2]
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
add     ax, 44h ; 'D'
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, ds
cmp     word_2E43C, 0
jz      short loc_13F97
jmp     loc_13EAD

loc_13F97:
call    sub_12E76
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
jmp     short loc_13F54

loc_13FA5:
pop     si
mov     sp, bp
pop     bp
retn
sub_13E79 endp



; Attributes: bp-based frame

sub_13FAA proc near

var_E= dword ptr -0Eh
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Eh
push    di
push    si
mov     [bp+var_4], 0
mov     bx, [bp+arg_0]
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp+var_A], ax
mov     [bp+var_8], dx
mov     al, 16h
mul     byte ptr [bx+5]
add     ax, [bp+var_A]
add     ax, 0Eh
mov     word ptr [bp+var_E], ax
mov     word ptr [bp+var_E+2], dx
mov     [bp+var_2], 0
jmp     short loc_13FE7

loc_13FE4:
inc     [bp+var_2]

loc_13FE7:
les     bx, [bp+var_E]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_2]
jbe     short loc_14005
mov     si, [bp+var_2]
shl     si, 1
cmp     word ptr es:[bx+si+2], 0
jle     short loc_13FE4
mov     [bp+var_4], 1

loc_14005:
cmp     [bp+var_4], 0
jnz     short loc_1400E
jmp     loc_140A9

loc_1400E:
mov     [bp+var_6], 0
jmp     short loc_14039

loc_14015:
mov     di, [bp+var_6]
shl     di, 1
les     bx, [bp+var_E]
mov     si, es:[bx+di+2]
or      si, si
jle     short loc_14036
mov     di, [bp+var_2]
shl     di, 1
cmp     es:[bx+di+2], si
jle     short loc_14036
mov     ax, [bp+var_6]
mov     [bp+var_2], ax

loc_14036:
inc     [bp+var_6]

loc_14039:
les     bx, [bp+var_E]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_6]
ja      short loc_14015
mov     ax, [bp+var_2]
mov     word_2E436, ax
shl     ax, 1
add     ax, bx
mov     dx, es
add     ax, 2
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, dx
mov     word_2E434, 0
mov     al, es:[bx+1]
sub     ah, ah
sub     cx, cx
push    cx
push    ax
mov     si, es:[bx]
and     si, 0FFh
mov     cl, 6
shl     si, cl
les     bx, _enemies
push    word ptr es:[bx+si+16h]
push    word ptr es:[bx+si+14h]
call    __aNlmul
mov     word_2E430, ax
mov     word_2E432, dx
les     bx, [bp+var_E]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word_2DFFC, ax
mov     word_2DFFE, dx

loc_140A9:
mov     al, [bp+var_4]
sub     ah, ah
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_13FAA endp



; Attributes: bp-based frame

sub_140B4 proc near

var_4= dword ptr -4
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     word_2E434, 0
sub     ax, ax
mov     word_2E432, ax
mov     word_2E430, ax
mov     bx, [bp+arg_0]
mov     al, [bx+3]
sub     ah, ah
cmp     ax, 1
jz      short loc_140DD
cmp     ax, 2
jz      short loc_14145
jmp     short loc_1415C

loc_140DD:
mov     al, 68h ; 'h'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
add     ax, 44h ; 'D'
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, dx
les     bx, [bp+var_4]
mov     al, es:[bx+4Eh]
sub     ah, ah
sub     cx, cx
push    cx
push    ax
mov     si, es:[bx+48h]
and     si, 0FFh
mov     cl, 7
shl     si, cl
les     bx, _professions
push    word ptr es:[bx+si+6Eh]
push    word ptr es:[bx+si+6Ch]
call    __aNlmul
mov     word_2E430, ax
mov     word_2E432, dx
les     bx, [bp+var_4]
mov     al, es:[bx+50h]
sub     ah, ah
and     ax, 3
mov     word_2E434, ax
mov     ax, bx
mov     dx, es
mov     word_2DFFC, ax
mov     word_2DFFE, dx
jmp     short loc_1415C

loc_14145:
push    [bp+arg_0]
call    sub_13FAA
add     sp, 2
or      al, al
jnz     short loc_1415C
mov     ax, 0D1F8h
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, ds

loc_1415C:
pop     si
mov     sp, bp
pop     bp
retn
sub_140B4 endp



; Attributes: bp-based frame

sub_14161 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 8
push    di
push    si
mov     al, byte_2E440
cbw
mov     si, ax
sub     ax, [bp+arg_0]
mov     [bp+var_8], ax
mov     al, byte_2E43F
cbw
mov     di, ax
sub     ax, [bp+arg_0]
mov     [bp+var_6], ax
mov     ax, [bp+arg_0]
add     ax, si
mov     [bp+var_4], ax
mov     ax, [bp+arg_0]
add     ax, di
mov     [bp+var_2], ax
mov     ax, [bp+arg_2]
cmp     [bp+var_6], ax
jg      short loc_141B0
cmp     [bp+var_2], ax
jl      short loc_141B0
mov     ax, [bp+arg_4]
cmp     [bp+var_8], ax
jg      short loc_141B0
cmp     [bp+var_4], ax
jl      short loc_141B0
mov     ax, 1
jmp     short loc_141B2

loc_141B0:
sub     ax, ax

loc_141B2:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_14161 endp



; Attributes: bp-based frame

sub_141B8 proc near

var_16= dword ptr -16h
var_12= dword ptr -12h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 16h
push    si
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+3], 2
jnz     short loc_14239
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp+var_C], ax
mov     [bp+var_A], dx
mov     al, 16h
mul     byte ptr [bx+5]
add     ax, [bp+var_C]
add     ax, 0Eh
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
mov     [bp+var_2], 0
jmp     short loc_1422B

loc_141F3:
mov     ax, word_2E436
cmp     [bp+var_2], ax
jz      short loc_14228
mov     ax, [bp+var_2]
shl     ax, 1
add     ax, word ptr [bp+var_12]
mov     dx, word ptr [bp+var_12+2]
add     ax, 2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
cmp     word ptr es:[bx], 0
jle     short loc_14228
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, dx
push    [bp+arg_0]
call    sub_13C29
add     sp, 2

loc_14228:
inc     [bp+var_2]

loc_1422B:
les     bx, [bp+var_12]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_2]
ja      short loc_141F3

loc_14239:
mov     [bp+var_8], 1
jmp     loc_14360

loc_14241:
inc     [bp+var_2]

loc_14244:
les     bx, [bp+var_12]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_2]
jbe     short loc_14291
mov     ax, [bp+var_2]
shl     ax, 1
add     ax, bx
mov     dx, es
add     ax, 2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
cmp     word ptr es:[bx], 0
jle     short loc_14241
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, dx
push    [bp+arg_0]
call    sub_13C29
add     sp, 2
jmp     short loc_14241

loc_1427F:
push    [bp+arg_0]
call    sub_140B4
add     sp, 2
push    [bp+arg_0]
call    sub_13C29
add     sp, 2

loc_14291:
inc     [bp+var_E]

loc_14294:
mov     ax, word_2F01A
cmp     [bp+var_E], ax
jl      short loc_1429F
jmp     loc_1435D

loc_1429F:
mov     ax, 0Ch
imul    [bp+var_E]
mov     si, ax
add     ax, 0D494h
cmp     [bp+arg_0], ax
jz      short loc_14291
test    byte ptr [si-2B63h], 1
jnz     short loc_14291
lea     ax, [bp+var_6]
push    ax
lea     ax, [bp+var_4]
push    ax
mov     ax, si
add     ax, 0D494h
push    ax
call    sub_202F2
add     sp, 6
or      ax, ax
jle     short loc_14291
push    [bp+var_6]
push    [bp+var_4]
push    [bp+var_8]
call    sub_14161
add     sp, 6
or      al, al
jz      short loc_14291
mov     ax, 0Ch
imul    [bp+var_E]
mov     si, ax
or      byte ptr [si-2B63h], 1
mov     bx, [bp+arg_0]
mov     al, [si-2B6Ch]
mov     [bx+3], al
mov     ax, 0Ch
imul    [bp+var_E]
mov     bx, ax
mov     al, [bx-2B6Bh]
mov     bx, [bp+arg_0]
mov     [bx+4], al
mov     ax, 0Ch
imul    [bp+var_E]
mov     bx, ax
mov     al, [bx-2B6Ah]
mov     bx, [bp+arg_0]
mov     [bx+5], al
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+3], 2
jz      short loc_1432A
jmp     loc_1427F

loc_1432A:
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp+var_C], ax
mov     [bp+var_A], dx
mov     al, 16h
mul     byte ptr [bx+5]
add     ax, [bp+var_C]
add     ax, 0Eh
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
push    bx
call    sub_13FAA
add     sp, 2
mov     [bp+var_2], 0
jmp     loc_14244

loc_1435D:
inc     [bp+var_8]

loc_14360:
mov     ax, word_2E43C
cmp     [bp+var_8], ax
jg      short loc_14370
mov     [bp+var_E], 0
jmp     loc_14294

loc_14370:
pop     si
mov     sp, bp
pop     bp
retn
sub_141B8 endp



; Attributes: bp-based frame

sub_14375 proc near

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     [bp+var_A], 1
mov     [bp+var_2], 0

loc_14385:
cmp     [bp+var_A], 4
jl      short loc_1438E
jmp     loc_1441F

loc_1438E:
mov     [bp+var_8], 0
jmp     short loc_14398

loc_14395:
inc     [bp+var_8]

loc_14398:
mov     ax, word_2F01A
cmp     [bp+var_8], ax
jge     short loc_14413
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
add     si, 0D494h
cmp     [bp+arg_0], si
jz      short loc_14395
lea     ax, [bp+var_6]
push    ax
lea     ax, [bp+var_4]
push    ax
push    si
call    sub_202F2
add     sp, 6
or      ax, ax
jle     short loc_14395
push    [bp+var_6]
push    [bp+var_4]
push    [bp+var_A]
call    sub_14161
add     sp, 6
or      al, al
jz      short loc_14395
mov     ax, 0Ch
imul    [bp+var_8]
mov     bx, ax
mov     al, [bx-2B6Ch]
mov     bx, [bp+arg_0]
mov     [bx+3], al
mov     ax, 0Ch
imul    [bp+var_8]
mov     bx, ax
mov     al, [bx-2B6Bh]
mov     bx, [bp+arg_0]
mov     [bx+4], al
mov     ax, 0Ch
imul    [bp+var_8]
mov     bx, ax
mov     al, [bx-2B6Ah]
mov     bx, [bp+arg_0]
mov     [bx+5], al
mov     [bp+var_2], 1

loc_14413:
inc     [bp+var_A]
cmp     [bp+var_2], 0
jnz     short loc_1441F
jmp     loc_14385

loc_1441F:
cmp     [bp+var_2], 0
jz      short loc_1442E
push    [bp+arg_0]
call    sub_140B4
add     sp, 2

loc_1442E:
mov     al, [bp+var_2]
sub     ah, ah
pop     si
mov     sp, bp
pop     bp
retn
sub_14375 endp



; Attributes: bp-based frame

sub_14438 proc near

var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
mov     bx, [bp+arg_0]
mov     bl, [bx+1]
sub     bh, bh
shl     bx, 1
mov     bx, [bx-2518h]
cmp     byte ptr [bx+144h], 2
jnz     short loc_1445B
mov     [bp+var_2], 1

loc_1445B:
mov     bx, [bp+arg_0]
mov     al, [bx+3]
sub     ah, ah
cmp     ax, 1
jz      short loc_1447E
cmp     ax, 2
jz      short loc_14471

loc_1446D:
sub     ax, ax
jmp     short loc_14494

loc_14471:
push    [bp+arg_0]
call    sub_13FAA
add     sp, 2
or      al, al
jnz     short loc_14491

loc_1447E:
cmp     [bp+var_2], 0
jz      short loc_1446D
push    [bp+arg_0]
call    sub_14375
add     sp, 2
sub     ah, ah
jmp     short loc_14494

loc_14491:
mov     ax, 1

loc_14494:
mov     sp, bp
pop     bp
retn
sub_14438 endp



; Attributes: bp-based frame

sub_14498 proc near

var_6= dword ptr -6
var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 6
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_6], ax
mov     [bp+var_2], 0
mov     bx, ax
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_144BA
jmp     loc_14595

loc_144BA:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
push    ax
call    sub_12F44
add     sp, 2
mov     word ptr [bp+var_6+2], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     bx, word ptr [bp+var_6+2]
mov     cl, [bx+1Ah]
sub     ch, ch
cmp     cx, ax
jb      short loc_144ED
jmp     loc_14595

loc_144ED:
mov     [bp+var_2], 1
test    byte ptr [bx+18h], 2
jz      short loc_1455C
test    byte ptr [bx+18h], 1
jz      short loc_1452E
mov     bx, word ptr [bp+var_6]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
or      byte ptr [bx+si+65h], 1
push    word ptr [bp+var_6] ; char *
mov     ax, offset aSSWeaponHasJam ; "%s's weapon has jammed!\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     short loc_14595

loc_1452E:
cmp     word_2E43C, 0
jle     short loc_14595
call    sub_138CA
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aButItFailsToDe ; " but it fails to detonate.\r\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, word ptr [bp+var_6]
mov     al, [bx+4Ah]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_11A62
add     sp, 4
jmp     short loc_14595

loc_1455C:              ; char *
push    word ptr [bp+var_6]
mov     ax, offset aSSWeaponHasBro ; "%s's weapon has broken!\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 0FFh
push    ax
mov     bx, word ptr [bp+var_6]
mov     al, [bx+4Ah]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_2060D
add     sp, 6

loc_14595:
mov     al, [bp+var_2]
sub     ah, ah
pop     si
mov     sp, bp
pop     bp
retn
sub_14498 endp



; Attributes: bp-based frame

sub_1459F proc near

var_A= word ptr -0Ah
var_8= dword ptr -8
var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_8], ax
mov     byte ptr [bp+var_8+2], 1
mov     [bp+var_2], 1
push    ax
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_145C8
jmp     loc_14783

loc_145C8:
mov     [bp+var_4], 0
jmp     short loc_145DF

loc_145CF:
mov     ax, 0Ch
imul    [bp+var_4]
mov     bx, ax
and     byte ptr [bx-2B63h], 0FEh
inc     [bp+var_4]

loc_145DF:
mov     ax, word_2F01A
cmp     [bp+var_4], ax
jl      short loc_145CF
push    [bp+arg_2]
call    sub_140B4
add     sp, 2
les     bx, dword_2E42A
cmp     word ptr es:[bx], 0
jg      short loc_145FD
jmp     loc_14761

loc_145FD:
mov     bx, [bp+arg_2]
cmp     byte ptr [bx+3], 1
jnz     short loc_1461F
mov     ax, 0D1F0h
push    ax
mov     al, 68h ; 'h'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
push    dx
push    ax
call    sub_136B8
add     sp, 6

loc_1461F:
push    [bp+arg_2]
call    sub_13901
add     sp, 2
push    [bp+arg_0]
call    sub_14498
add     sp, 2
or      al, al
jz      short loc_14638
jmp     loc_14783

loc_14638:
cmp     word_2E43C, 0
jnz     short loc_146BC
call    sub_12E76
jmp     short loc_14688

loc_14644:
cmp     [bp+var_2], 0
jz      short loc_1468E
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
mov     [bp+var_A], ax
cmp     ax, 4
jnz     short loc_14669
mov     bx, word ptr [bp+var_8]
cmp     byte ptr [bx+146h], 0
jz      short loc_14669
mov     al, 1
jmp     short loc_1466B

loc_14669:
sub     al, al

loc_1466B:
mov     byte ptr [bp+var_8+2], al
or      al, al
jz      short loc_14688
push    [bp+arg_2]
call    sub_14438
add     sp, 2
mov     [bp+var_2], al
mov     ax, 6E6h
mov     word_2E4BA, ax
mov     word_2E4BC, ds

loc_14688:
cmp     byte ptr [bp+var_8+2], 0
jnz     short loc_14644

loc_1468E:
mov     [bp+var_4], 0
jmp     short loc_14698

loc_14695:
inc     [bp+var_4]

loc_14698:
mov     bx, word ptr [bp+var_8]
mov     al, [bx+146h]
sub     ah, ah
cmp     ax, [bp+var_4]
ja      short loc_146A9
jmp     loc_14783

loc_146A9:
mov     al, [bx+4Ah]
push    ax
mov     bx, [bp+arg_2]
mov     al, [bx+1]
push    ax
call    sub_11A62
add     sp, 4
jmp     short loc_14695

loc_146BC:
mov     [bp+var_4], 0
jmp     short loc_146C6

loc_146C3:
inc     [bp+var_4]

loc_146C6:
mov     ax, word_2F01A
cmp     [bp+var_4], ax
jge     short loc_146F9
mov     ax, 0Ch
imul    [bp+var_4]
mov     si, ax
mov     bx, [bp+arg_2]
mov     al, [si-2B6Ch]
cmp     [bx+3], al
jnz     short loc_146C3
mov     al, [si-2B6Bh]
cmp     [bx+4], al
jnz     short loc_146C3
mov     al, [si-2B6Ah]
cmp     [bx+5], al
jnz     short loc_146C3
or      byte ptr [si-2B63h], 1

loc_146F9:
mov     byte_2E43E, 1
mov     ax, 5
push    ax
call    sub_21130
add     sp, 2
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
mov     [bp+var_A], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_A], ax
jb      short loc_1472F
mov     word_2E43A, 2

loc_1472F:
mov     byte_2E43E, 0
push    [bp+arg_2]
call    sub_13C29
add     sp, 2
call    sub_12CF5
push    [bp+arg_2]
call    sub_141B8
add     sp, 2
mov     bx, word ptr [bp+var_8]
mov     al, [bx+4Ah]
sub     ah, ah
push    ax
mov     bx, [bp+arg_2]
mov     al, [bx+1]
push    ax
call    sub_11A62
add     sp, 4
jmp     short loc_14783

loc_14761:
push    ds
push    word ptr [bp+var_8] ; char *
mov     ax, offset aFsManagesToHol ; "%Fs manages to hold back.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_14783:
pop     si
mov     sp, bp
pop     bp
retn
sub_1459F endp



; Attributes: bp-based frame

sub_14788 proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0

loc_14793:
sub     ax, ax
push    ax
mov     ax, 0Ch
imul    [bp+var_2]
mov     bx, ax
push    word_2D102[bx]  ; win
call    setWindow
add     sp, 4
mov     ax, 0Bh
push    ax              ; int
mov     bx, [bp+var_2]
shl     bx, 1
shl     bx, 1
push    word ptr (dword_2DFF0+2)[bx]
push    word ptr dword_2DFF0[bx] ; member
call    writeShortMemberName
add     sp, 6
inc     [bp+var_2]
cmp     [bp+var_2], 3
jl      short loc_14793
sub     ax, ax
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
mov     sp, bp
pop     bp
retn
sub_14788 endp



; Attributes: bp-based frame

sub_147D8 proc near

arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp              ; draw
mov     bp, sp
cmp     [bp+arg_2], 0
jz      short loc_147EB
mov     ax, 1
push    ax
call    sub_18880
add     sp, 2

loc_147EB:
call    sub_1DAFF
mov     ax, 1
push    ax
call    sub_16B44
add     sp, 2
call    sub_14788
cmp     [bp+arg_0], 0
jz      short loc_1480A
mov     ax, 1A6Eh
push    ax
call    sub_1DA17
jmp     short loc_14811

loc_1480A:
mov     ax, 1A6Eh
push    ax
call    sub_1D9F5

loc_14811:
add     sp, 2
sub     ax, ax
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
add     sp, 4
pop     bp
retn
sub_147D8 endp

push    bp
mov     bp, sp
sub     sp, 34h
push    si
mov     word ptr [bp-8], 1
mov     ax, 0Ch
imul    word ptr [bp+6]
mov     bx, ax
mov     al, [bx-2B6Ch]
sub     ah, ah
cmp     ax, 1
jz      short loc_1484C
cmp     ax, 2
jnz     short loc_1484A
jmp     loc_148E1

loc_1484A:
jmp     short loc_14868

loc_1484C:
mov     ax, 0Ch
imul    word ptr [bp+6]
mov     bx, ax
mov     al, 68h ; 'h'
mul     byte ptr [bx-2B6Bh]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2

loc_14862:
mov     [bp-4], ax
mov     [bp-2], dx

loc_14868:
push    word ptr [bp-2]
push    word ptr [bp-4]
mov     ax, offset aFs_0 ; "%Fs"
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 8
cmp     word ptr [bp-8], 1
jle     short loc_148A1
mov     ax, offset sprintf_buffer
push    ax
call    _strlen
add     sp, 2
mov     [bp-6], ax
mov     bx, ax
inc     word ptr [bp-6]
mov     byte ptr [bx-261Ah], 73h ; 's'
mov     bx, [bp-6]
mov     byte ptr [bx-261Ah], 0

loc_148A1:
mov     ax, 0Ch
imul    word ptr [bp+6]
mov     bx, ax
mov     al, [bx-2B64h]
sub     ah, ah
mul     word_2E426
push    ax
mov     ax, offset sprintf_buffer
push    ax
push    word ptr [bp-8]
push    word ptr [bp+4]
mov     ax, offset a1d1d1414s3d ; "%1d> %1d %-14.14s %3d'"
push    ax
lea     ax, [bp-30h]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-30h]
push    ss
push    ax
call    writeStringAt
add     sp, 8
jmp     short loc_14929

loc_148E1:
mov     ax, 0Ch
imul    word ptr [bp+6]
mov     si, ax
mov     al, 16h
mul     byte ptr [si-2B6Ah]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [si-2B6Bh]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp-34h], ax
mov     [bp-32h], dx
les     bx, [bp-34h]
mov     al, es:[bx+1Dh]
sub     ah, ah
mov     [bp-8], ax
mov     al, es:[bx+0Eh]
mov     [bp-6], ax
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
jmp     loc_14862

loc_14929:
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_1492E proc near

var_12= dword ptr -12h
var_E= word ptr -0Eh
var_C= dword ptr -0Ch
var_8= byte ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 12h
push    si              ; draw
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_E], ax
mov     [bp+var_6], 0
mov     [bp+var_2], 0
cmp     word_2DFEA, 1
jg      short loc_14974
cmp     byte_2E6D4, 2
jnz     short loc_149C0
mov     al, 66h ; 'f'
mul     byte_2E6D5
mov     si, ax
mov     al, 16h
mul     byte_2E6D6
add     si, ax
les     bx, dword_2E41E
cmp     byte ptr es:[bx+si+1Dh], 1
jbe     short loc_149C0

loc_14974:
mov     ax, 1
push    ax
mov     ax, offset _window9
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
call    sub_12A34
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 4823h
push    ax
push    [bp+arg_4]
lea     ax, [bp+var_2]
push    ax
push    word_2DFEA
push    [bp+var_E]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_6], ax
sub     ax, ax
push    ax
call    sub_12A34
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 1
push    ax
call    sub_147D8
add     sp, 4

loc_149C0:
cmp     [bp+var_6], 0
jl      short loc_14A44
mov     ax, 0Ch
imul    [bp+var_6]
mov     bx, ax
mov     al, [bx-2B6Ch]
sub     ah, ah
cmp     ax, 1
jz      short loc_149E0
cmp     ax, 2
jz      short loc_14A49
jmp     short loc_14A0D

loc_149E0:
mov     ax, 0Ch
imul    [bp+var_6]
mov     bx, ax
mov     al, 68h ; 'h'
mul     byte ptr [bx-2B6Bh]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
les     bx, [bp+var_12]
mov     al, es:[bx+60h]
mov     [bp+var_4], al
mov     al, es:[bx+61h]

loc_14A0A:
mov     [bp+var_8], al

loc_14A0D:
mov     al, [bp+var_8]
cbw
push    ax
mov     al, [bp+var_4]
cbw
push    ax
push    [bp+arg_2]
call    sub_1073E
add     sp, 6
or      al, al
jnz     short loc_14A44
sub     ax, ax
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
mov     ax, 72Bh
push    ax              ; char *
call    sub_173AC
add     sp, 4
mov     [bp+var_6], 0FFFFh

loc_14A44:
mov     ax, [bp+var_6]
jmp     short loc_14A82

loc_14A49:
mov     ax, 0Ch
imul    [bp+var_6]
mov     si, ax
mov     al, 16h
mul     byte ptr [si-2B6Ah]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [si-2B6Bh]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     word ptr [bp+var_C], ax
mov     word ptr [bp+var_C+2], dx
les     bx, [bp+var_C]
mov     al, es:[bx+10h]
mov     [bp+var_4], al
mov     al, es:[bx+11h]
jmp     short loc_14A0A

loc_14A82:
pop     si
mov     sp, bp
pop     bp
retn
sub_1492E endp

push    bp
mov     bp, sp
mov     bx, [bp+4]
mov     al, [bx+8]
sub     ah, ah
mov     bx, [bp+6]
mov     cl, [bx+8]
sub     ch, ch
sub     ax, cx
pop     bp
retn


; Attributes: bp-based frame

sub_14A9E proc near

var_E= dword ptr -0Eh
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= dword ptr -4
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Eh
push    si
mov     [bp+var_8], 0
mov     bx, [bp+arg_0]
mov     al, [bx]
sub     ah, ah
cmp     ax, 1
jz      short loc_14AC0
cmp     ax, 2
jnz     short loc_14ABE
jmp     loc_14B73

loc_14ABE:
jmp     short loc_14B2D

loc_14AC0:
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
mov     al, [bx+4]
sub     ah, ah
push    ax
les     bx, [bp+var_4]
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
call    sub_200D8
add     sp, 6
mov     [bp+var_6], ax
les     bx, [bp+var_4]
mov     si, ax
shl     si, 1
shl     si, 1
mov     al, [si+170h]
add     es:[bx+60h], al
les     bx, [bp+var_4]
mov     si, [bp+var_6]
shl     si, 1
shl     si, 1
mov     al, [si+172h]
add     es:[bx+61h], al
mov     ax, word ptr [bp+var_4]
mov     dx, word ptr [bp+var_4+2]
mov     word_2EC00, ax
mov     word_2EC02, dx
les     bx, [bp+var_4]
mov     ax, es:[bx+44h]
mov     [bp+var_8], ax

loc_14B2D:
cmp     [bp+var_8], 0
jg      short loc_14B36
jmp     loc_14C3E

loc_14B36:
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+5], 0
jnz     short loc_14B42
jmp     loc_14C3E

loc_14B42:
push    word_2EC02
push    word_2EC00      ; char *
mov     ax, offset aFsMoves ; "%Fs moves "
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+arg_0]
mov     al, [bx+3]
sub     ah, ah
jmp     loc_14C35

loc_14B73:
mov     al, 16h
mul     byte ptr [bx+2]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [bx+1]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     word ptr [bp+var_E], ax
mov     word ptr [bp+var_E+2], dx
mov     ax, 1
push    ax
les     bx, [bp+var_E]
mov     al, es:[bx+11h]
cbw
push    ax
mov     al, es:[bx+10h]
cbw
push    ax
call    sub_200D8
add     sp, 6
mov     [bp+var_6], ax
les     bx, [bp+var_E]
mov     si, ax
shl     si, 1
shl     si, 1
mov     al, [si+170h]
add     es:[bx+10h], al
les     bx, [bp+var_E]
mov     si, [bp+var_6]
shl     si, 1
shl     si, 1
mov     al, [si+172h]
add     es:[bx+11h], al
les     bx, [bp+var_E]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word_2EC00, ax
mov     word_2EC02, dx
mov     [bp+var_A], 0
mov     [bp+var_8], 0
jmp     short loc_14BFD

loc_14BFA:
inc     [bp+var_A]

loc_14BFD:
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_A]
ja      short loc_14C0B
jmp     loc_14B2D

loc_14C0B:
mov     si, [bp+var_A]
shl     si, 1
mov     ax, es:[bx+si+2]
add     [bp+var_8], ax
jmp     short loc_14BFA

loc_14C19:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aToABetterPosti ; "to a better postion.\r\r"

loc_14C21:
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     short loc_14C3E

loc_14C2B:
mov     ax, 0FFFFh
push    ax
push    ax
mov     ax, offset aCloser ; "closer.\r\r"
jmp     short loc_14C21

loc_14C35:
or      ax, ax
jz      short loc_14C19
cmp     ax, 1
jz      short loc_14C2B

loc_14C3E:
pop     si
mov     sp, bp
pop     bp
retn
sub_14A9E endp



; Attributes: bp-based frame

sub_14C43 proc near

var_14= word ptr -14h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= byte ptr -6
var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8
arg_6= dword ptr  0Ah
arg_A= word ptr  0Eh
arg_C= word ptr  10h
arg_E= byte ptr  12h
arg_10= byte ptr  14h
arg_12= byte ptr  16h

push    bp
mov     bp, sp
sub     sp, 14h
push    si
mov     ax, 0Ch
imul    [bp+arg_0]
mov     si, ax
mov     byte ptr [si-2B66h], 0Eh
mov     al, [bp+arg_12]
cbw
push    ax
mov     al, [bp+arg_10]
cbw
push    ax
call    sub_10795
add     sp, 4
mov     [si-2B64h], al
cmp     [bp+arg_E], 3Dh ; '='
jnb     short loc_14C74
jmp     loc_14E5C

loc_14C74:
cmp     [bp+arg_E], 0FFh
jnz     short loc_14C7D
jmp     loc_14E5C

loc_14C7D:
mov     ax, 64h ; 'd'
imul    [bp+arg_A]
cwd
mov     cx, [bp+arg_C]
idiv    cx
mov     [bp+var_6], al
mov     [bp+var_10], 0
jmp     short loc_14C96

loc_14C93:
inc     [bp+var_10]

loc_14C96:
cmp     [bp+var_10], 4
jge     short loc_14CAA
mov     bx, [bp+var_10]
shl     bx, 1
mov     al, [bp+var_6]
cmp     [bx+548h], al
jb      short loc_14C93

loc_14CAA:
cmp     [bp+arg_4], 0FFh
jz      short loc_14CD4
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
push    ax
mov     bl, [bp+arg_2]
sub     bh, bh
les     si, [bp+arg_6]
mov     al, es:[bx+si]
sub     ah, ah
push    ax
call    sub_20000
add     sp, 0Ah
mov     [bp+var_8], ax
jmp     short loc_14CD9

loc_14CD4:
mov     [bp+var_8], 1

loc_14CD9:
mov     [bp+var_C], 0
mov     [bp+var_4], 0
jmp     short loc_14D1D

loc_14CE5:
mov     bx, [bp+var_C]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0FFC4h
jle     short loc_14D1A
mov     al, [bp+arg_12]
cbw
push    ax
mov     al, [bp+arg_10]
cbw
push    ax
push    [bp+var_8]
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_14D1A
mov     bx, [bp+var_4]
shl     bx, 1
mov     ax, [bp+var_C]
mov     [bx-3240h], ax
inc     [bp+var_4]

loc_14D1A:
inc     [bp+var_C]

loc_14D1D:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_C], ax
jb      short loc_14CE5
cmp     [bp+var_4], 0
jg      short loc_14D30
jmp     loc_14E40

loc_14D30:
cmp     [bp+var_8], 1
jle     short loc_14D6C
cmp     [bp+arg_4], 0FFh
jz      short loc_14D6C
sub     ax, ax
push    ax
mov     ax, 0Ch
imul    [bp+arg_0]
mov     bx, ax
mov     al, [bx-2B64h]
sub     ah, ah
dec     ax
push    ax
sub     ax, ax
push    ax
push    ax
mov     bl, [bp+arg_2]
sub     bh, bh
les     si, [bp+arg_6]
mov     al, es:[bx+si]
sub     ah, ah
push    ax
call    sub_20000
add     sp, 0Ah
mov     [bp+var_8], ax

loc_14D6C:
mov     ax, 0Ch
imul    [bp+arg_0]
mov     bx, ax
mov     al, [bx-2B64h]
sub     ah, ah
cmp     ax, [bp+var_8]
jz      short loc_14D9C
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     bx, [bp+var_10]
shl     bx, 1
mov     cl, [bx+549h]
sub     ch, ch
cmp     cx, ax
jb      short loc_14E17

loc_14D9C:
mov     [bp+var_2], 0

loc_14DA0:
mov     ax, [bp+var_4]
dec     ax
push    ax
sub     ax, ax
push    ax
call    sub_1DEB1
add     sp, 4
mov     bx, ax
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp+var_A], ax
mov     si, ax
shl     si, 1
mov     ax, [si-2518h]
mov     [bp+var_14], ax
cmp     word ptr [si-3036h], 0
jle     short loc_14DD0
mov     ax, 32h ; '2'
jmp     short loc_14DD3

loc_14DD0:
mov     ax, 64h ; 'd'

loc_14DD3:
mov     [bp+var_E], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_E], ax
jb      short loc_14DF6
mov     bx, [bp+var_A]
shl     bx, 1
inc     word ptr [bx-3036h]
mov     [bp+var_2], 1

loc_14DF6:
cmp     [bp+var_2], 0
jz      short loc_14DA0
mov     ax, 0Ch
imul    [bp+arg_0]
mov     si, ax
mov     byte ptr [si-2B66h], 4
mov     byte ptr [si-2B69h], 0
mov     al, byte ptr [bp+var_A]
mov     [si-2B68h], al
jmp     short loc_14E5C

loc_14E17:
mov     ax, 0Ch
imul    [bp+arg_0]
mov     si, ax
mov     byte ptr [si-2B66h], 1
mov     byte ptr [si-2B69h], 0
mov     al, [si-2B64h]
sub     ah, ah
cmp     ax, [bp+var_8]
jbe     short loc_14E38
mov     al, 1
jmp     short loc_14E3A

loc_14E38:
sub     al, al

loc_14E3A:
mov     [si-2B68h], al
jmp     short loc_14E57

loc_14E40:
mov     ax, 0Ch
imul    [bp+arg_0]
mov     si, ax
mov     byte ptr [si-2B66h], 1
mov     byte ptr [si-2B69h], 1
mov     byte ptr [si-2B68h], 1

loc_14E57:
mov     byte ptr [si-2B67h], 1

loc_14E5C:
pop     si
mov     sp, bp
pop     bp
retn
sub_14C43 endp



; Attributes: bp-based frame

sub_14E61 proc near

var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8
arg_6= byte ptr  0Ah
arg_8= byte ptr  0Ch
arg_A= byte ptr  0Eh
arg_C= byte ptr  10h
arg_E= word ptr  12h
arg_10= word ptr  14h

push    bp
mov     bp, sp
sub     sp, 6
push    si
mov     [bp+var_4], 0
mov     ax, 0Ch
mul     [bp+arg_0]
mov     si, ax
mov     al, [bp+arg_2]
mov     [si-2B6Ch], al
mov     al, [bp+arg_4]
mov     [si-2B6Bh], al
mov     al, [bp+arg_6]
mov     [si-2B6Ah], al
mov     al, [bp+arg_8]
mov     [si-2B65h], al
mov     al, [bp+arg_A]
mov     [si-2B62h], al
cmp     word_2F01C, 3
jge     short loc_14EC9
mov     bx, word_2F01C
shl     bx, 1
mov     al, [bp+arg_C]
sub     ah, ah
mov     [bx-2220h], ax
mov     bx, word_2F01C
inc     word_2F01C

loc_14EB5:
shl     bx, 1
shl     bx, 1
mov     ax, [bp+arg_E]
mov     dx, [bp+arg_10]
mov     [bx-3250h], ax
mov     [bx-324Eh], dx
jmp     short loc_14F15

loc_14EC9:
mov     [bp+var_6], 0
jmp     short loc_14ED3

loc_14ED0:
inc     [bp+var_6]

loc_14ED3:
cmp     [bp+var_6], 3
jge     short loc_14F15
mov     ax, [bp+var_6]
inc     ax
cwd
mov     cx, 3
idiv    cx
mov     [bp+var_2], dx
mov     bx, [bp+var_6]
shl     bx, 1
mov     si, dx
shl     si, 1
mov     ax, [si-2220h]
cmp     [bx-2220h], ax
jnz     short loc_14ED0
or      dx, dx
jnz     short loc_14F02
mov     [bp+var_2], 2

loc_14F02:
mov     bx, [bp+var_2]
shl     bx, 1
mov     al, [bp+arg_C]
sub     ah, ah
mov     [bx-2220h], ax
mov     bx, [bp+var_2]
jmp     short loc_14EB5

loc_14F15:
pop     si
mov     sp, bp
pop     bp
retn
sub_14E61 endp



; Attributes: bp-based frame

sub_14F1A proc near

var_24= word ptr -24h
var_1C= word ptr -1Ch
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= dword ptr -8
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 28h
push    di
push    si
mov     [bp+var_24], 0
mov     word_2F01C, 0
mov     [bp+var_E], 0

loc_14F32:
mov     bx, [bp+var_E]
shl     bx, 1
mov     word ptr [bx-2220h], 0FFFFh
mov     bx, [bp+var_E]
shl     bx, 1
shl     bx, 1
mov     ax, 779h
mov     [bx-3250h], ax
mov     word ptr [bx-324Eh], ds
inc     [bp+var_E]
cmp     [bp+var_E], 3
jl      short loc_14F32
mov     word_2EC0A, 0
call    sub_203C9
mov     [bp+var_A], ax
mov     [bp+var_E], 0
jmp     loc_15068

loc_14F6E:
mov     ax, 1

loc_14F71:
mov     [bp+var_1C], ax
cmp     [bp+arg_0], 0
jz      short loc_14F7F
mov     ax, [bp+var_A]
jmp     short loc_14F82

loc_14F7F:
mov     ax, [bp+var_1C]

loc_14F82:
mov     [bp+var_C], ax
cmp     [bp+arg_0], 0
jz      short loc_14F99
mov     ax, [bp+var_A]
cmp     [bp+var_1C], ax
jle     short loc_14F99
mov     ax, [bp+var_1C]
mov     [bp+var_C], ax

loc_14F99:
les     bx, [bp+var_8]
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
mov     al, es:[bx+5Fh]
sub     ah, ah
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jnz     short loc_14FBC
jmp     loc_1505D

loc_14FBC:
les     bx, [bp+var_8]
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
push    [bp+var_C]
call    sub_1073E
add     sp, 6
or      al, al
jnz     short loc_14FDB
jmp     loc_1505D

loc_14FDB:
les     bx, _hdsPct
mov     di, es:[bx+3E8h]
les     bx, [bp+var_8]
mov     al, es:[bx+di+18h]
sub     ah, ah
mov     si, ax
cmp     word_2EC0A, si
jnb     short loc_14FF9
mov     word_2EC0A, si

loc_14FF9:
push    es
push    bx
mov     al, es:[bx+63h]
sub     ah, ah
push    ax
mov     ax, 1
push    ax
mov     al, es:[bx+1Ah]
sub     ah, ah
shl     ax, 1
push    ax
sub     ax, ax
push    ax
push    [bp+var_E]
mov     ax, 1
push    ax
push    [bp+var_24]
call    sub_14E61
add     sp, 12h
les     bx, [bp+var_8]
mov     al, es:[bx+4Ah]
sub     ah, ah
mov     si, ax
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
mov     al, es:[bx+5Dh]
sub     ah, ah
push    ax
push    word ptr es:[bx+46h]
push    word ptr es:[bx+44h]
lea     ax, [bx+si+53h]
push    es
push    ax
push    si
push    si
push    [bp+var_24]
call    sub_14C43
add     sp, 14h
inc     [bp+var_24]
jmp     short loc_15065

loc_1505D:
les     bx, [bp+var_8]
mov     byte ptr es:[bx+60h], 0FFh

loc_15065:
inc     [bp+var_E]

loc_15068:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_E]
jbe     short loc_150E4
mov     ax, 68h ; 'h'
imul    [bp+var_E]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
les     bx, [bp+var_8]
cmp     byte ptr es:[bx+5Dh], 3Ch ; '<'
jbe     short loc_15065
cmp     [bp+var_24], 64h ; 'd'
jge     short loc_15065
cmp     byte ptr es:[bx+60h], 0FFh
jnz     short loc_150B5
mov     al, es:[bx+5Bh]
mov     es:[bx+60h], al
les     bx, [bp+var_8]
mov     al, es:[bx+5Ch]
mov     es:[bx+61h], al

loc_150B5:
les     bx, [bp+var_8]
cmp     byte ptr es:[bx+4Ah], 0FFh
jnz     short loc_150C2
jmp     loc_14F6E

loc_150C2:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
push    ax
mov     si, es:[bx+4Ah]
and     si, 0FFh
mov     al, es:[bx+si+53h]
cbw
push    ax
call    sub_20000
add     sp, 0Ah
jmp     loc_14F71

loc_150E4:
mov     [bp+var_E], 0
jmp     loc_152D1
sub_14F1A endp

; START OF FUNCTION CHUNK FOR sub_15220

loc_150EC:
les     bx, [bp-12h]
mov     al, es:[bx+9]
sub     ah, ah
mov     dx, 1
mov     cl, [bp-14h]
shl     dx, cl
test    ax, dx
jz      short loc_15136
mov     ax, 16h
imul    word ptr [bp-14h]
mov     si, ax
mov     al, es:[bx+si+21h]
mov     [bp-28h], al
cmp     al, 0FFh
jz      short loc_15136
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
push    ax
mov     al, [bp-28h]
sub     ah, ah
push    ax
call    sub_20000
add     sp, 0Ah
mov     [bp-0Ch], ax
cmp     ax, [bp-1Ch]
jle     short loc_15136
mov     [bp-1Ch], ax

loc_15136:
inc     word ptr [bp-14h]

loc_15139:
les     bx, [bp-12h]
mov     al, es:[bx+8]
sub     ah, ah
cmp     ax, [bp-14h]
ja      short loc_150EC
mov     word ptr [bp-14h], 0
jmp     loc_15281

loc_1514F:
mov     al, es:[bx+2]
les     bx, [bp-18h]
mov     es:[bx+10h], al
les     bx, [bp-12h]
mov     al, es:[bx+3]
les     bx, [bp-18h]
mov     es:[bx+11h], al
cmp     byte ptr [bp+4], 0
jz      short loc_15173
mov     ax, [bp-0Ah]
jmp     short loc_15176

loc_15173:
mov     ax, [bp-1Ch]

loc_15176:
mov     [bp-0Ch], ax
cmp     byte ptr [bp+4], 0
jz      short loc_1518D
mov     ax, [bp-0Ah]
cmp     [bp-1Ch], ax
jle     short loc_1518D
mov     ax, [bp-1Ch]
mov     [bp-0Ch], ax

loc_1518D:
les     bx, [bp-18h]
mov     al, es:[bx+11h]
cbw
push    ax
mov     al, es:[bx+10h]
cbw
push    ax
les     bx, [bp-12h]
mov     al, es:[bx+5]
sub     ah, ah
push    ax
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_151D0
les     bx, [bp-18h]
mov     al, es:[bx+11h]
cbw
push    ax
mov     al, es:[bx+10h]
cbw
push    ax
push    word ptr [bp-0Ch]
call    sub_1073E
add     sp, 6
or      al, al
jz      short loc_151D0
mov     al, 1
jmp     short loc_151D2

loc_151D0:
sub     al, al

loc_151D2:
mov     [bp-1Eh], al

loc_151D5:
cmp     byte ptr [bp-1Eh], 0
jnz     short loc_151DE
jmp     loc_1527E

loc_151DE:
les     bx, [bp-12h]
mov     al, es:[bx+0Bh]
sub     ah, ah
mov     si, ax
cmp     word_2EC0A, si
jnb     short loc_151F3
mov     word_2EC0A, si

loc_151F3:
mov     al, 1
mov     cl, [bp-14h]
add     cl, 4
shl     al, cl
or      es:[bx+9], al
les     bx, [bp-18h]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     [bp-4], ax
mov     [bp-2], dx
push    dx
push    ax
les     bx, [bp-4]
; END OF FUNCTION CHUNK FOR sub_15220



sub_15220 proc near

; FUNCTION CHUNK AT 50EC SIZE 00000134 BYTES

mov     al, es:[bx+3Eh]
sub     ah, ah
push    ax
mov     al, es:[bx+34h]
push    ax
mov     al, es:[bx+3Dh]
push    ax
push    word ptr [bp-14h]
push    word ptr [bp-0Eh]
mov     ax, 2
push    ax
push    word ptr [bp-24h]
call    sub_14E61
add     sp, 12h
les     bx, [bp-18h]
mov     al, es:[bx+11h]
cbw
push    ax
mov     al, es:[bx+10h]
cbw
push    ax
mov     ax, 79h ; 'y'
push    ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
mov     ax, bx
mov     dx, es
add     ax, 13h
push    dx
push    ax
mov     al, es:[bx+13h]
sub     ah, ah
push    ax
sub     ax, ax
push    ax
push    word ptr [bp-24h]
call    sub_14C43
add     sp, 14h
inc     word ptr [bp-24h]

loc_1527E:
inc     word ptr [bp-14h]

loc_15281:
les     bx, [bp-12h]
mov     al, es:[bx+8]
sub     ah, ah
cmp     ax, [bp-14h]
jbe     short loc_152CE
mov     al, es:[bx+9]
mov     dx, 1
mov     cl, [bp-14h]
shl     dx, cl
test    ax, dx
jz      short loc_1527E
mov     ax, 16h
imul    word ptr [bp-14h]
add     ax, bx
mov     dx, es
add     ax, 0Eh
mov     [bp-18h], ax
mov     [bp-16h], dx
mov     ax, 1
add     cl, 4
shl     ax, cl
mov     cl, es:[bx+9]
sub     ch, ch
test    ax, cx
jnz     short loc_152C7
jmp     loc_1514F

loc_152C7:
mov     byte ptr [bp-1Eh], 1
jmp     loc_151D5

loc_152CE:
inc     word ptr [bp-0Eh]

loc_152D1:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp-0Eh]
jbe     short loc_15311
mov     ax, 66h ; 'f'
imul    word ptr [bp-0Eh]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp-12h], ax
mov     [bp-10h], dx
les     bx, [bp-12h]
test    byte ptr es:[bx+9], 0Fh
jz      short loc_152CE
cmp     word ptr [bp-24h], 64h ; 'd'
jge     short loc_152CE
mov     word ptr [bp-1Ch], 1
mov     word ptr [bp-14h], 0
jmp     loc_15139

loc_15311:
cmp     word ptr [bp-24h], 1
jle     short loc_1532C
mov     ax, 4A87h
push    ax              ; size_t
mov     ax, 0Ch
push    ax              ; size_t
push    word ptr [bp-24h]
mov     ax, 0D494h
push    ax              ; void *
call    _qsort
add     sp, 8

loc_1532C:
mov     ax, [bp-24h]
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_15220 endp ; sp-analysis failed

push    bp
mov     bp, sp
mov     bx, [bp+4]
mov     al, [bx+6]
sub     ah, ah
mov     bx, [bp+6]
mov     cl, [bx+6]
sub     ch, ch
sub     ax, cx
pop     bp
retn
push    bp
mov     bp, sp
mov     bx, [bp+6]
mov     al, [bx+7]
sub     ah, ah
mov     bx, [bp+4]
mov     cl, [bx+7]
sub     ch, ch
sub     ax, cx
pop     bp
retn
push    bp
mov     bp, sp
mov     bx, [bp+4]
mov     al, [bx]
sub     ah, ah
mov     bx, [bp+6]
mov     cl, [bx]
sub     ch, ch
sub     ax, cx
mov     cx, ax
mov     ah, cl
sub     al, al
mov     bx, [bp+4]
mov     cl, [bx+1]
sub     ch, ch
add     ax, cx
mov     bx, [bp+6]
mov     cl, [bx+1]
sub     ax, cx
pop     bp
retn


; Attributes: bp-based frame

sub_15390 proc near

var_18= word ptr -18h
var_14= byte ptr -14h
var_4= word ptr -4

push    bp
mov     bp, sp
sub     sp, 18h
push    si              ; int (__cdecl *)(const void *, const void *)
mov     ax, 5335h
push    ax              ; size_t
mov     ax, 0Ch
push    ax              ; size_t
push    word_2F01A
mov     ax, 0D494h
push    ax              ; void *
call    _qsort
add     sp, 8
call    winClearCurrent
call    sub_1DDFD
mov     [bp+var_4], 0

loc_153B8:
mov     si, [bp+var_4]
mov     [bp+si+var_14], 0
inc     [bp+var_4]
cmp     [bp+var_4], 10h
jl      short loc_153B8
mov     [bp+var_4], 0
jmp     short loc_153E5

loc_153CF:
mov     ax, 0Ch
imul    [bp+var_4]
mov     bx, ax
mov     si, [bx-2B66h]
and     si, 0FFh
inc     [bp+si+var_14]
inc     [bp+var_4]

loc_153E5:
mov     ax, word_2F01A
cmp     [bp+var_4], ax
jl      short loc_153CF
mov     [bp+var_4], 0
mov     [bp+var_18], 0
jmp     short loc_1546D
sub_15390 endp

; START OF FUNCTION CHUNK FOR sub_1542D

loc_153F9:
mov     si, [bp-4]
mov     al, [bp+si-14h]
sub     ah, ah
add     ax, [bp-18h]
cmp     ax, [bp-16h]
jbe     short loc_15428
inc     word ptr [bp-16h]

loc_1540C:
mov     ax, 0Ch
imul    word ptr [bp-16h]
mov     bx, ax
mov     al, [bx-2B65h]
mov     cx, ax
mov     ax, 0Ch
imul    word ptr [bp-2]
mov     bx, ax
cmp     [bx-2B65h], cl
jz      short loc_153F9

loc_15428:
mov     si, [bp-16h]
; END OF FUNCTION CHUNK FOR sub_1542D
db 2Bh, 76h



sub_1542D proc near

; FUNCTION CHUNK AT 53F9 SIZE 00000032 BYTES

inc     byte ptr [bp+di+1FEh]
jle     short loc_15449
mov     ax, 5363h
push    ax              ; size_t
mov     ax, 0Ch
push    ax              ; size_t
push    si
imul    word ptr [bp-2]
add     ax, 0D494h
push    ax              ; void *
call    _qsort
add     sp, 8

loc_15449:
mov     ax, [bp-16h]
mov     [bp-2], ax

loc_1544F:
mov     si, [bp-4]
mov     al, [bp+si-14h]
sub     ah, ah
add     ax, [bp-18h]
cmp     ax, [bp-16h]
ja      short loc_1540C

loc_1545F:
mov     si, [bp-4]
mov     al, [bp+si-14h]
sub     ah, ah
add     [bp-18h], ax

loc_1546A:
inc     word ptr [bp-4]

loc_1546D:
cmp     word ptr [bp-4], 10h
jge     short loc_154AB
mov     si, [bp-4]
cmp     byte ptr [bp+si-14h], 0
jz      short loc_1546A
cmp     byte ptr [bp+si-14h], 1
jbe     short loc_1545F
mov     ax, 534Ch
push    ax              ; size_t
mov     ax, 0Ch
push    ax              ; size_t
mov     al, [bp+si-14h]
sub     ah, ah
push    ax
mov     ax, 0Ch
imul    word ptr [bp-18h]
add     ax, 0D494h
push    ax              ; void *
call    _qsort
add     sp, 8
mov     ax, [bp-18h]
mov     [bp-2], ax
mov     [bp-16h], ax
jmp     short loc_1544F

loc_154AB:
pop     si
mov     sp, bp
pop     bp
retn
sub_1542D endp ; sp-analysis failed



; Attributes: bp-based frame

sub_154B0 proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
jmp     short loc_154C8

loc_154BD:
mov     bx, [bp+var_2]
mov     byte ptr [bx+197Ah], 0FFh
inc     [bp+var_2]

loc_154C8:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_2], ax
jb      short loc_154BD
mov     sp, bp
pop     bp
retn
sub_154B0 endp



; Attributes: bp-based frame

sub_154D6 proc near

var_4= word ptr -4
var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 4
push    si
mov     [bp+var_4], 0
mov     [bp+var_2], 0
jmp     short loc_15503

loc_154E9:
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
cmp     word ptr [si+44h], 0
jle     short loc_15500
mov     al, [si+1Ah]
sub     ah, ah
add     [bp+var_4], ax

loc_15500:
inc     [bp+var_2]

loc_15503:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_2], ax
jb      short loc_154E9
mov     cx, 5
mov     ax, [bp+var_4]
cwd
idiv    cx
mov     [bp+var_4], ax
or      ax, ax
jnz     short loc_15520
inc     [bp+var_4]

loc_15520:
mov     ax, [bp+var_4]
pop     si
mov     sp, bp
pop     bp
retn
sub_154D6 endp



; Attributes: bp-based frame

sub_15528 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     [bp+var_4], 0
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_1553F
jmp     loc_15642

loc_1553F:
mov     bx, [bp+arg_2]
cmp     byte ptr [bx+18h], 0
jnz     short loc_1554B
jmp     loc_15642

loc_1554B:
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     [bp+var_8], ax
mov     bx, [bp+arg_0]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, 12h
mul     byte ptr [bx+si+63h]
add     ax, [bp+var_8]
mov     [bp+var_6], ax
mov     bx, ax
cmp     byte ptr [bx+10h], 1
ja      short loc_15575
jmp     loc_15642

loc_15575:
mov     al, [bx+10h]
mov     bx, [bp+arg_0]
mov     cx, ax
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
cmp     [bx+si+64h], cl
jnb     short loc_1558C
jmp     loc_15642

loc_1558C:
call    winClearCurrent
sub     ax, ax
push    ax              ; int
mov     ax, offset aChooseAFiringM ; "Choose a firing mode:"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 1
push    ax              ; y
mov     ax, 0Eh
push    ax              ; x
mov     ax, offset aSIngleFire ; "S)ingle fire."
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 2
push    ax              ; y
mov     ax, 0Eh
push    ax              ; x
mov     ax, offset aBUrst ; "B)urst."
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 3
push    ax              ; y
mov     ax, 0Eh
push    ax              ; x
mov     ax, offset aFUllAuto ; "F)ull auto."
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_155D9:
cmp     [bp+var_4], 0
jnz     short loc_15642
call    getKeypress
mov     [bp+var_2], al
cbw
cmp     ax, 0FFFFh
jz      short loc_1563E
cmp     ax, 42h ; 'B'
jz      short loc_15610
cmp     ax, 46h ; 'F'
jz      short loc_15623
cmp     ax, 53h ; 'S'
jnz     short loc_155D9
mov     bx, [bp+arg_0]
mov     byte ptr [bx+144h], 0
mov     bx, [bp+arg_0]
mov     byte ptr [bx+146h], 1

loc_1560A:
mov     [bp+var_4], 1
jmp     short loc_155D9

loc_15610:
mov     bx, [bp+arg_0]
mov     byte ptr [bx+144h], 1
mov     bx, [bp+arg_0]
mov     si, [bp+var_6]
mov     al, [si+10h]
jmp     short loc_15638

loc_15623:
mov     bx, [bp+arg_0]
mov     byte ptr [bx+144h], 2
mov     bx, [bp+arg_0]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+64h]

loc_15638:
mov     [bx+146h], al
jmp     short loc_1560A

loc_1563E:
sub     ax, ax
jmp     short loc_15645

loc_15642:
mov     ax, 1

loc_15645:
pop     si
mov     sp, bp
pop     bp
retn
sub_15528 endp



; Attributes: bp-based frame

sub_1564A proc near

arg_0= byte ptr  4

push    bp              ; draw
mov     bp, sp
mov     ax, 1
push    ax
mov     ax, offset _window3
push    ax              ; win
call    setWindow
add     sp, 4
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
sub     ax, ax
push    ax
call    sub_147D8
add     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_14788
call    sub_20772
mov     ax, 1A4Eh
push    ax
call    sub_1D9F5
add     sp, 2
call    sub_208E2
pop     bp
retn
sub_1564A endp



; Attributes: bp-based frame

sub_15691 proc near

var_7A= byte ptr -7Ah
var_74= word ptr -74h
var_72= word ptr -72h
var_70= word ptr -70h
var_6E= dword ptr -6Eh
var_66= word ptr -66h
var_3C= byte ptr -3Ch
var_3A= word ptr -3Ah
var_38= word ptr -38h
var_36= word ptr -36h
var_30= word ptr -30h
var_2E= word ptr -2Eh
var_2A= byte ptr -2Ah
var_28= word ptr -28h
var_22= word ptr -22h
var_20= word ptr -20h
var_C= byte ptr -0Ch
var_A= byte ptr -0Ah
var_8= byte ptr -8
var_6= word ptr -6
arg_0= byte ptr  4

; FUNCTION CHUNK AT 68DF SIZE 00000009 BYTES
; FUNCTION CHUNK AT 68E8 SIZE 00000024 BYTES
; FUNCTION CHUNK AT 690C SIZE 0000004F BYTES

push    bp
mov     bp, sp
sub     sp, 7Eh
push    di
push    si
mov     [bp+var_3C], 0
mov     [bp+var_8], 1
cmp     [bp+arg_0], 0
jz      short loc_156AA
call    sub_1DD4C

loc_156AA:
cmp     word_2F01A, 0
jnz     short loc_156B4
jmp     loc_16955

loc_156B4:
mov     [bp+var_3A], 0

loc_156B9:
mov     si, [bp+var_3A]
shl     si, 1
shl     si, 1
sub     ax, ax
mov     [bp+si+var_20], ax
mov     [bp+si+var_22], ax
inc     [bp+var_3A]
cmp     [bp+var_3A], 5
jl      short loc_156B9
mov     word_2E438, ax
mov     [bp+var_3A], ax
mov     [bp+var_2E], ax
jmp     short loc_156F1

loc_156DC:
mov     bx, [bp+var_3A]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0
jle     short loc_156EE
inc     [bp+var_2E]

loc_156EE:
inc     [bp+var_3A]

loc_156F1:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_3A], ax
jb      short loc_156DC
cmp     [bp+var_2E], 0
jnz     short loc_1570B
mov     ax, 1
push    ax
call    sub_1113F
add     sp, 2

loc_1570B:
mov     ax, word_2F01A
mov     word_2DFEA, ax
mov     ax, 1
push    ax
call    sub_1564A
add     sp, 2
mov     byte_2E4BE, 1
call    sub_17E09
mov     al, byte ptr _disk1.field_4+1
sub     ah, ah
mov     word_22C44, ax
jmp     loc_168E8

loc_1572E:
mov     [bp+var_C], 0
mov     [bp+var_72], 0
mov     ax, word_2F01A
mov     word_2DFEA, ax
jmp     loc_15E66

loc_15740:
mov     ax, word_2D126
mov     [bp+var_70], ax
mov     si, [bp+var_72]
shl     si, 1
mov     ax, [si-2518h]
mov     [bp+var_74], ax
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 0FFh
mov     ax, 0Ch
imul    word_2F01A
mov     di, ax
mov     byte ptr [di-2B66h], 0Eh
mov     al, byte ptr [bp+var_72]
mov     [di-2B6Bh], al
mov     byte ptr [di-2B6Ch], 0
mov     bx, [bp+var_74]
mov     al, [bx+1Ah]
shl     al, 1
mov     [di-2B65h], al
mov     bx, [bp+var_74]
mov     al, [bx+1Ah]
sub     ah, ah
shl     ax, 1
mov     [si-2B76h], ax
push    [bp+var_74]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_1579E
jmp     loc_15E57

loc_1579E:
sub     ax, ax
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     bx, [bp+var_72]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
mov     ax, offset aSChoose ; "%s, choose:"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
sub     ax, ax
push    ax              ; y
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _strlen
add     sp, 2
sub     ax, 26h ; '&'
neg     ax
cwd
sub     ax, dx
sar     ax, 1
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 2
push    ax              ; y
push    ax              ; x
mov     ax, offset aRun ; "Run"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 3
push    ax              ; y
mov     ax, 2
push    ax              ; x
mov     ax, offset aEvade ; "Evade"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 2
push    ax              ; y
mov     ax, 0Fh
push    ax              ; x
mov     ax, offset aAttack ; "Attack"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 3
push    ax              ; y
mov     ax, 0Fh
push    ax              ; x
mov     ax, offset aLoadUnjam ; "Load/unjam"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 2
push    ax              ; y
mov     ax, 1Eh
push    ax              ; x
mov     ax, offset aWeapon ; "Weapon"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 3
push    ax              ; y
mov     ax, 1Eh
push    ax              ; x
mov     ax, offset aUse ; "Use"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, _currentWindow
add     ax, 0Ch
push    ax
call    sub_1D9F5
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 1
push    ax
push    [bp+var_72]
call    sub_107E1
add     sp, 6
call    sub_208E2
call    getKeypress
mov     [bp+var_A], al
cmp     al, 0FFh
jnz     short loc_15893
cmp     [bp+var_72], 0
jle     short loc_15893
dec     word_2F01A
dec     [bp+var_72]
jmp     loc_15E66

loc_15893:
mov     al, [bp+var_A]
cbw
cmp     ax, 41h ; 'A'
jnz     short loc_1589F
jmp     loc_15A2A

loc_1589F:
cmp     ax, 45h ; 'E'
jnz     short loc_158A7
jmp     loc_15A00

loc_158A7:
cmp     ax, 4Ch ; 'L'
jnz     short loc_158AF
jmp     loc_15B62

loc_158AF:
cmp     ax, 52h ; 'R'
jz      short loc_158C7
cmp     ax, 55h ; 'U'
jnz     short loc_158BC
jmp     loc_15D6E

loc_158BC:
cmp     ax, 57h ; 'W'
jnz     short loc_158C4
jmp     loc_15C61

loc_158C4:
jmp     loc_15E66

loc_158C7:
call    winClearCurrent
sub     ax, ax
push    ax              ; int
mov     ax, offset aChooseAMethod ; "Choose a method:"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 2
push    ax              ; int
mov     ax, offset aGEtCloserFLee ; "G)et closer    F)lee"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
call    sub_1DC28
mov     [bp+var_2A], 0

loc_158EE:
cmp     [bp+var_2A], 0
jz      short loc_158F7
jmp     loc_15E66

loc_158F7:
call    getKeypress
mov     [bp+var_A], al
cbw
cmp     ax, 0FFFFh
jz      short loc_1595C
cmp     ax, 46h ; 'F'
jz      short loc_15962
cmp     ax, 47h ; 'G'
jnz     short loc_158EE
mov     bx, [bp+var_74]
mov     byte ptr [bx+144h], 0
mov     ax, word_2DFEA
inc     ax
mov     word_2F01A, ax
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     byte ptr [bx-2B66h], 0
mov     ax, word_2F01A
inc     word_2F01A
mov     cx, 0Ch
imul    cx
mov     bx, ax
mov     byte ptr [bx-2B65h], 0FFh
mov     [bp+var_3A], 0
jmp     short loc_1594F

loc_15944:
mov     bx, [bp+var_3A]
mov     byte ptr [bx+197Ah], 0
inc     [bp+var_3A]

loc_1594F:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_3A], ax
jb      short loc_15944
mov     [bp+var_72], ax

loc_1595C:
mov     [bp+var_2A], 1
jmp     short loc_158EE

loc_15962:
mov     bx, [bp+var_74]
mov     byte ptr [bx+144h], 0Fh
mov     word_2D126, 0
push    [bp+var_72]
call    sub_1CBDF
add     sp, 2
inc     ax
jz      short loc_159EE
mov     ax, word_2DFEA
mov     word_2F01A, ax
mov     [bp+var_3A], 0
jmp     short loc_159CE

loc_15989:
mov     si, [bp+var_3A]
shl     si, 1
mov     bx, [si-2518h]
mov     al, [bx+1Ah]
sub     ah, ah
shl     ax, 1
shl     ax, 1
mov     [si-2B76h], ax
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     al, [si-2B76h]
mov     [bx-2B65h], al
mov     ax, word_2F01A
inc     word_2F01A
mov     cx, 0Ch
imul    cx
mov     bx, ax
mov     byte ptr [bx-2B66h], 0Eh
mov     bx, [bp+var_3A]
mov     byte ptr [bx+197Ah], 0Fh
inc     [bp+var_3A]

loc_159CE:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_3A], ax
jb      short loc_15989
mov     ax, 0Ch
imul    word_2DFEA
mov     bx, ax
mov     byte ptr [bx-2B66h], 0Fh
mov     al, _disk1._partyCount
sub     ah, ah
mov     [bp+var_72], ax

loc_159EE:
mov     ax, [bp+var_70]
mov     word_2D126, ax
sub     ax, ax
push    ax
call    sub_1564A
add     sp, 2
jmp     loc_1595C

loc_15A00:
mov     ax, 0Ch
imul    word_2F01A
mov     si, ax
mov     byte ptr [si-2B66h], 1
mov     di, [bp+var_72]
shl     di, 1
add     di, 0D48Ah
shl     word ptr [di], 1
mov     al, [di]
mov     [si-2B65h], al
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 1
jmp     loc_15B5C

loc_15A2A:
mov     bx, [bp+var_74]
mov     byte ptr [bx+144h], 0
mov     bx, [bp+var_74]
mov     di, bx
mov     al, [di+4Ah]
mov     [bx+145h], al
mov     bx, [bp+var_74]
mov     byte ptr [bx+146h], 1
mov     bx, [bp+var_74]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_15A52
jmp     loc_15AF4

loc_15A52:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     di, ax
mov     al, 18h
mul     byte ptr [bx+di+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
mov     word ptr [bp+var_6E], ax
mov     bx, ax
mov     al, 3Ch ; '<'
mul     byte ptr [bx+15h]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp+var_66], ax
mov     bx, ax
test    byte ptr [bx+18h], 1
jz      short loc_15AC6
mov     bx, [bp+var_74]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
add     si, bx
test    byte ptr [si+65h], 1
jnz     short loc_15A98
cmp     byte ptr [si+64h], 0
jnz     short loc_15AC6

loc_15A98:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
test    byte ptr [bx+si+65h], 1
jz      short loc_15AAA
mov     ax, 817h
jmp     short loc_15AAD

loc_15AAA:
mov     ax, 81Eh

loc_15AAD:              ; char *
push    ax
mov     ax, offset aYourWeaponIsS ; "Your weapon is %s."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 1
push    ax
mov     ax, offset sprintf_buffer
jmp     loc_15C13

loc_15AC6:
mov     bx, [bp+var_74]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
add     si, bx
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     al, [si+64h]
sub     ah, ah
push    ax
mov     al, [si+63h]
push    ax
mov     al, [si+62h]
push    ax
call    sub_20000
add     sp, 0Ah
mov     [bp+var_6], ax
jmp     short loc_15AF9

loc_15AF4:
mov     [bp+var_6], 1

loc_15AF9:
mov     ax, offset aChooseATarget ; "Choose a target.."
push    ax
push    [bp+var_6]
push    [bp+var_72]
call    sub_1492E
add     sp, 6
mov     [bp+var_30], ax
or      ax, ax
jge     short loc_15B13
jmp     loc_15E66

loc_15B13:
push    [bp+var_66]
push    [bp+var_74]
call    sub_15528
add     sp, 4
or      al, al
jnz     short loc_15B26
jmp     loc_15E66

loc_15B26:
mov     ax, 0Ch
imul    word_2F01A
mov     si, ax
mov     ax, 0Ch
imul    [bp+var_30]
mov     di, ax
mov     al, [di-2B6Ch]
mov     [si-2B69h], al
mov     al, [di-2B6Bh]
mov     [si-2B68h], al
mov     al, [di-2B6Ah]
mov     [si-2B67h], al
mov     byte ptr [si-2B66h], 4
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 4

loc_15B5C:
inc     [bp+var_72]
jmp     loc_15E62

loc_15B62:
mov     bx, [bp+var_74]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_15B6E
jmp     loc_15C58

loc_15B6E:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
test    byte ptr [bx+si+65h], 1
jz      short loc_15B93
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 6
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     byte ptr [bx-2B66h], 6
jmp     short loc_15B5C

loc_15B93:
mov     bx, [bp+var_74]
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
mov     word ptr [bp+var_6E], ax
mov     bx, ax
mov     al, 3Ch ; '<'
mul     byte ptr [bx+15h]
mov     si, ax
mov     bx, _weapons
test    byte ptr [bx+si+1Ah], 1
jz      short loc_15C1D
mov     ax, 1
push    ax
mov     ax, offset _window9
push    ax
mov     bx, [bp+var_74]
mov     al, [bx+4Ah]
sub     ah, ah
push    ax
push    [bp+var_72]
call    sub_11D6A
add     sp, 8
mov     [bp+var_38], ax
or      ax, ax
jl      short loc_15C03
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 5
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     byte ptr [bx-2B66h], 5
mov     bx, [bp+var_74]
mov     al, byte ptr [bp+var_38]
mov     [bx+145h], al
jmp     loc_15B5C

loc_15C03:
cmp     [bp+var_38], 0FFFEh
jz      short loc_15C0C
jmp     loc_15E66

loc_15C0C:
mov     ax, 1
push    ax
mov     ax, 849h

loc_15C13:              ; char *
push    ax
call    sub_173AC
add     sp, 4
jmp     loc_15E66

loc_15C1D:
call    winClearCurrent
push    word ptr [bp+var_6E] ; char *
mov     ax, offset aSS  ; "%s's"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 1
push    ax              ; int
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 2
push    ax              ; int
mov     ax, offset aAreNotReloadab ; "are not reloadable."
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
call    sub_1DC28
call    getKeypress
jmp     loc_15E66

loc_15C58:
mov     ax, 1
push    ax
mov     ax, 884h
jmp     short loc_15C13

loc_15C61:
mov     [bp+var_36], 0
mov     bx, [bp+var_74]
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_15C7D
mov     bx, [bp+var_36]
inc     [bp+var_36]
shl     bx, 1
mov     word ptr [bx-3240h], 0FFh

loc_15C7D:
mov     [bp+var_3A], 0
jmp     short loc_15C96

loc_15C84:
mov     bx, [bp+var_36]
inc     [bp+var_36]
shl     bx, 1
mov     ax, [bp+var_3A]
mov     [bx-3240h], ax

loc_15C93:
inc     [bp+var_3A]

loc_15C96:
mov     bx, [bp+var_74]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_3A]
jbe     short loc_15CDB
mov     ax, 6
imul    [bp+var_3A]
mov     si, ax
mov     al, [bx+si+62h]
mov     [bp+var_7A], al
cmp     al, 0FFh
jz      short loc_15C93
mov     al, 18h
mul     [bp+var_7A]
mov     si, ax
mov     bx, _globals
test    byte ptr [bx+si+3Eh], 1
jz      short loc_15C93
mov     bx, [bp+var_74]
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_15C84
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+var_3A]
jz      short loc_15C93
jmp     short loc_15C84

loc_15CDB:
cmp     [bp+var_36], 0
jnz     short loc_15CEB
mov     ax, 1
push    ax
mov     ax, 8A3h
jmp     loc_15C13

loc_15CEB:
mov     ax, 1
push    ax
mov     ax, offset _window9
push    ax              ; win
call    setWindow
add     sp, 4
mov     [bp+var_28], 0
mov     ax, 1
push    ax
call    sub_12A34
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 2EAEh
push    ax
mov     ax, 8BEh
push    ax
lea     ax, [bp+var_28]
push    ax
push    [bp+var_36]
push    [bp+var_74]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_38], ax
sub     ax, ax
push    ax
call    sub_12A34
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 1
push    ax
call    sub_147D8
add     sp, 4
cmp     [bp+var_38], 0
jge     short loc_15D45
jmp     loc_15E66

loc_15D45:
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     byte ptr [bx-2B66h], 7
mov     bx, [bp+var_74]
mov     si, [bp+var_38]
shl     si, 1
mov     al, [si-3240h]
mov     [bx+145h], al
mov     bx, [bp+var_72]
mov     byte ptr [bx+197Ah], 7
jmp     loc_15B5C

loc_15D6E:
mov     word_2D126, 0
mov     ax, 1
push    ax
push    [bp+var_72]
call    characterInfo
add     sp, 4
inc     ax
jnz     short loc_15D87
jmp     loc_15E3E

loc_15D87:
mov     bx, [bp+var_74]
cmp     byte ptr [bx+144h], 0Ah
jnz     short loc_15E0B
cmp     byte ptr [bx+145h], 1Ah
jnz     short loc_15DFC
mov     ax, [bp+var_70]
mov     word_2D126, ax
sub     ax, ax
push    ax
call    sub_1564A
add     sp, 2
mov     ax, offset aChooseATarget_0 ; "Choose a target.."
push    ax
mov     ax, 1
push    ax
push    [bp+var_72]
call    sub_1492E
add     sp, 6
mov     [bp+var_30], ax
or      ax, ax
jl      short loc_15DF2
mov     ax, 0Ch
imul    [bp+var_30]
mov     si, ax
mov     ax, 0Ch
imul    word_2F01A
mov     di, ax
mov     al, [si-2B6Ch]
mov     [di-2B69h], al
mov     al, [si-2B6Bh]
mov     [di-2B68h], al
mov     al, [si-2B6Ah]
mov     [di-2B67h], al
mov     bx, [bp+var_74]
mov     byte ptr [bx+144h], 3
jmp     short loc_15E0B

loc_15DF2:
mov     bx, [bp+var_74]
mov     byte ptr [bx+144h], 0Eh
jmp     short loc_15E0B

loc_15DFC:
mov     bx, [bp+var_74]
cmp     byte ptr [bx+145h], 19h
jnz     short loc_15E0B
mov     byte ptr [bx+144h], 2

loc_15E0B:
mov     ax, 0Ch
imul    word_2F01A
mov     bx, ax
mov     si, [bp+var_74]
mov     al, [si+144h]
mov     [bx-2B66h], al
mov     bx, [bp+var_72]
mov     si, [bp+var_74]
mov     al, [si+144h]
mov     [bx+197Ah], al
mov     bx, [bp+var_74]
cmp     byte ptr [bx+144h], 0Eh
jz      short loc_15E3E
inc     [bp+var_72]
inc     word_2F01A

loc_15E3E:
mov     ax, word_2D126
cmp     [bp+var_70], ax
jz      short loc_15E66
mov     ax, [bp+var_70]
mov     word_2D126, ax
sub     ax, ax
push    ax
call    sub_1564A
add     sp, 2
jmp     short loc_15E66

loc_15E57:
mov     bx, [bp+var_72]
inc     [bp+var_72]
mov     byte ptr [bx+197Ah], 0Eh

loc_15E62:
inc     word_2F01A

loc_15E66:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_72], ax
jnb     short loc_15E73
jmp     loc_15740

loc_15E73:
sub     ax, ax
push    ax
push    ax
push    ax
call    sub_107E1
add     sp, 6
call    winClearCurrent
sub     ax, ax
push    ax              ; int
mov     ax, offset aUseTheseComman ; "Use these commands?"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 2
push    ax
mov     ax, 10h
push    ax
call    sub_19780
add     sp, 4
or      al, al
jnz     short loc_15EA4
jmp     loc_168DF

loc_15EA4:
cmp     [bp+var_3C], 0
jz      short loc_15EAD
jmp     loc_168E8

loc_15EAD:
call    sub_154B0
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
call    sub_107E1
add     sp, 6
call    sub_15390
mov     [bp+var_C], 0
sub     ax, ax
push    ax
mov     ax, offset _window8
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     ax, 3
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aEncounterBegin ; "Encounter begins...\r\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     [bp+var_3A], 0
jmp     loc_1675E
sub_15691 endp

; START OF FUNCTION CHUNK FOR sub_16328

loc_15EEF:              ; jumptable 000167A7 case 0
mov     word ptr [bp-68h], 0
jmp     short loc_15F26

loc_15EF6:
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     si, ax
cmp     byte ptr [si-2B6Ch], 0
jz      short loc_15F23
mov     byte ptr [si-2B66h], 1
mov     byte ptr [si-2B69h], 1
mov     byte ptr [si-2B68h], 1
mov     byte ptr [si-2B67h], 0
add     ax, 0D494h
push    ax
call    sub_14A9E
add     sp, 2

loc_15F23:
inc     word ptr [bp-68h]

loc_15F26:
mov     ax, word_2F01A
cmp     [bp-68h], ax
jl      short loc_15EF6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
cmp     _disk1._partyCount, 1
jbe     short loc_15F3F
mov     ax, offset aYourPartyMoves ; "Your party moves closer.\r\r"
jmp     short loc_15F42

loc_15F3F:
mov     ax, offset aYouMoveCloser ; "You move closer.\r\r"

loc_15F42:
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     word ptr [bp-68h], 0
jmp     short loc_15FA8

loc_15F51:
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     bx, ax
mov     al, 68h ; 'h'
mul     byte ptr [bx-2B6Bh]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     [bp-26h], ax
mov     [bp-24h], dx
les     bx, [bp-26h]
mov     al, es:[bx+4Ah]
sub     ah, ah
mov     si, ax
mov     al, es:[bx+61h]
cbw
push    ax
mov     al, es:[bx+60h]
cbw
push    ax
mov     al, es:[bx+5Dh]
sub     ah, ah
push    ax
push    word ptr es:[bx+46h]
push    word ptr es:[bx+44h]
mov     ax, bx
add     ax, 53h ; 'S'
push    dx
push    ax
push    si
push    si

loc_15F9C:
push    word ptr [bp-68h]
call    sub_14C43
add     sp, 14h

loc_15FA5:
inc     word ptr [bp-68h]

loc_15FA8:
mov     ax, word_2F01A
cmp     [bp-68h], ax
jl      short loc_15FB3
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_15FB3:
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     bx, ax
mov     al, [bx-2B6Ch]
sub     ah, ah
cmp     ax, 1
jz      short loc_15F51
cmp     ax, 2
jnz     short loc_15FA5
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     si, ax
mov     al, 16h
mul     byte ptr [si-2B6Ah]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [si-2B6Bh]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     [bp-6Ch], ax
mov     [bp-6Ah], dx
les     bx, [bp-6Ch]
mov     al, es:[bx+11h]
cbw
push    ax
mov     al, es:[bx+10h]
cbw
push    ax
mov     ax, 79h ; 'y'
push    ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
mov     ax, bx
add     ax, 13h
push    dx
push    ax
mov     al, es:[bx+13h]
sub     ah, ah
push    ax
sub     ax, ax
push    ax
jmp     loc_15F9C

loc_16023:              ; jumptable 000167A7 case 15
call    sub_154D6
mov     [bp-36h], ax
mov     ax, word_2EC0A
cmp     [bp-36h], ax
jnz     short loc_16048
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     ax, 32h ; '2'
ja      short loc_16050

loc_16044:
mov     al, 1
jmp     short loc_16052

loc_16048:
mov     ax, word_2EC0A
cmp     [bp-36h], ax
jg      short loc_16044

loc_16050:
sub     al, al

loc_16052:
mov     [bp-0Ch], al
or      al, al
jnz     short loc_160C4
mov     word ptr [bp-68h], 0
mov     word ptr [bp-36h], 0
jmp     short loc_16098

loc_16065:
mov     bx, [bp-68h]
shl     bx, 1
push    word ptr [bx-2518h]
call    sub_16F6B
add     sp, 2
or      al, al
jz      short loc_16095
mov     bx, [bp-68h]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+1Eh]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
mov     cx, 3
imul    cx
add     [bp-36h], ax

loc_16095:
inc     word ptr [bp-68h]

loc_16098:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp-68h], ax
jb      short loc_16065
cmp     word ptr [bp-36h], 0
jle     short loc_160BF
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp-36h], ax
jb      short loc_160BF
mov     al, 1
jmp     short loc_160C1

loc_160BF:
sub     al, al

loc_160C1:
mov     [bp-0Ch], al

loc_160C4:
cmp     byte ptr [bp-0Ch], 0
jz      short loc_160CF
mov     ax, 930h
jmp     short loc_160D2

loc_160CF:
mov     ax, 938h

loc_160D2:              ; char *
push    ax
mov     ax, offset aYouSInYourAtte ; "You %s in your attempt to run away.\r\r"

loc_160D6:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6

loc_160E1:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer

loc_160E9:
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_160F4:              ; jumptable 000167A7 case 1
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     bx, ax
mov     al, [bx-2B6Ch]
sub     ah, ah
or      ax, ax
jz      short loc_16116
cmp     ax, 1
jnb     short loc_1610E
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1610E:
cmp     ax, 2
jbe     short loc_1613F
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16116:
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_16126
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16126:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
push    ds
push    word ptr [bp-74h] ; text
call    writeStringAt
add     sp, 8
mov     ax, 0FFFFh
push    ax
push    ax
mov     ax, offset aEvades ; " evades.\r\r"
jmp     short loc_160E9

loc_1613F:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
call    sub_14A9E

loc_1614C:
add     sp, 2
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16152:              ; jumptable 000167A7 case 4
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     bx, ax
mov     al, [bx-2B6Ch]
sub     ah, ah
or      ax, ax
jz      short loc_16174
cmp     ax, 1
jz      short loc_161BB
cmp     ax, 2
jnz     short loc_16171
jmp     loc_16248

loc_16171:              ; jumptable 000167A7 default case, case 14
jmp     def_167A7

loc_16174:
mov     si, [bp-72h]
shl     si, 1
shl     si, 1
lea     ax, [bp+si-22h]
mov     word_2E42E, ax
mov     bx, [bp-74h]
mov     byte ptr [bx+22h], 0
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
push    word ptr [bp-72h]
call    sub_1459F
add     sp, 4
mov     bx, [bp-74h]
cmp     byte ptr [bx+22h], 0
jnz     short loc_161A7
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_161A7:
mov     ax, 1
push    ax              ; char
mov     al, [bx+23h]
sub     ah, ah
push    ax
push    bx              ; char *
call    sub_16F9D

loc_161B5:
add     sp, 6
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_161BB:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     bx, ax
mov     al, 68h ; 'h'
mul     byte ptr [bx-2B6Bh]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     [bp-26h], ax
mov     [bp-24h], dx
les     bx, [bp-26h]
cmp     word ptr es:[bx+44h], 0
jle     short loc_16213
test    byte ptr es:[bx+50h], 4
jz      short loc_16213
push    dx
push    ax              ; char *
mov     ax, offset aFsReloads ; "%Fs reloads.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
les     bx, [bp-26h]
and     byte ptr es:[bx+50h], 0FBh
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16213:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
mov     ax, 68h ; 'h'
imul    word ptr [bp-72h]
mov     si, ax
les     bx, dword_2E446
push    word ptr es:[bx+si+44h]
call    sub_13E79
add     sp, 4
cmp     byte_2E442, 2
jz      short loc_1623D
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1623D:
les     bx, [bp-26h]
or      byte ptr es:[bx+50h], 4
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16248:
mov     ax, 66h ; 'f'
imul    word ptr [bp-72h]
mov     cx, ax
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     bx, ax
mov     al, 16h
mul     byte ptr [bx-2B6Ah]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     [bp-6Ch], ax
mov     [bp-6Ah], dx
mov     word ptr [bp-68h], 0
jmp     short loc_162A9

loc_16278:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
mov     si, [bp-68h]
shl     si, 1
les     bx, [bp-6Ch]
push    word ptr es:[bx+si+2]
call    sub_13E79
add     sp, 4
cmp     byte_2E442, 2
jnz     short loc_162A6
mov     si, [bp-68h]
les     bx, [bp-6Ch]
or      byte ptr es:[bx+si+0Ah], 1

loc_162A6:
inc     word ptr [bp-68h]

loc_162A9:
les     bx, [bp-6Ch]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp-68h]
ja      short loc_162BA
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_162BA:
mov     ax, [bp-68h]
shl     ax, 1
add     ax, bx
mov     dx, es
add     ax, 2
mov     [bp-7Eh], ax
mov     [bp-7Ch], dx
les     bx, [bp-7Eh]
cmp     word ptr es:[bx], 0
jle     short loc_16278
mov     si, [bp-68h]
les     bx, [bp-6Ch]
test    byte ptr es:[bx+si+0Ah], 1
jz      short loc_16278
mov     word ptr dword_2E42A, ax
mov     word ptr dword_2E42A+2, dx
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
call    sub_13901
add     sp, 2
push    word_2EC02
push    word_2EC00      ; char *
mov     ax, offset aFsReloads_0 ; "%Fs reloads.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     si, [bp-68h]
les     bx, [bp-6Ch]
; END OF FUNCTION CHUNK FOR sub_16328
db 26h, 80h, 60h



sub_16328 proc near

; FUNCTION CHUNK AT 5EEF SIZE 00000436 BYTES
; FUNCTION CHUNK AT 66E9 SIZE 00000112 BYTES
; FUNCTION CHUNK AT 6854 SIZE 00000013 BYTES
; FUNCTION CHUNK AT 68D9 SIZE 00000006 BYTES
; FUNCTION CHUNK AT 68E8 SIZE 00000024 BYTES

or      bh, dh
jmp     loc_162A6

loc_1632D:              ; jumptable 000167A7 case 6
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_1633D
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1633D:
mov     ax, 1
push    ax
push    word ptr [bp-72h]
call    sub_16DD4
add     sp, 4
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1634D:              ; jumptable 000167A7 case 5
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_1635D
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1635D:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
push    ds
push    word ptr [bp-74h] ; text
call    writeStringAt
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aReloads ; " reloads.\r\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp-74h]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, [bx+si+64h]
mov     cx, ax
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     [bx+si+64h], cl
mov     bx, [bp-74h]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, [bx+si+63h]
mov     cx, ax
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     [bx+si+63h], cl

loc_163B0:
mov     ax, 0FFh
push    ax
mov     bx, [bp-74h]
mov     al, [bx+145h]
sub     ah, ah
push    ax
push    word ptr [bp-72h]
call    sub_2060D
jmp     loc_161B5

loc_163C9:              ; jumptable 000167A7 case 7
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_163D9
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_163D9:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
push    ds
push    word ptr [bp-74h] ; text
call    writeStringAt
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset aSwitches ; " switches.\r\r"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp-74h]
mov     si, bx
mov     al, [si+145h]
mov     [bx+4Ah], al
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16407:              ; jumptable 000167A7 cases 10,11
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_16417
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16417:              ; char *
push    word ptr [bp-74h]
mov     ax, offset aSUses ; "%s uses...\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    getKeypress
call    sub_1B452
mov     ax, 1
push    ax
push    word ptr [bp-72h]
call    sub_11CEA
add     sp, 4
call    getKeypress
mov     ax, [bp-70h]
mov     word_2D126, ax
sub     ax, ax
push    ax
call    sub_1564A
jmp     loc_1614C

loc_1645D:              ; jumptable 000167A7 case 2
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_1646D
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_1646D:
mov     bx, [bp-74h]
mov     al, 0Fh
mul     byte ptr [bx+31h]
mov     [bp-36h], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp-36h], ax
jb      short loc_164BC
mov     ax, [bp-3Ah]
inc     ax
mov     [bp-68h], ax
jmp     short loc_164AB

loc_16494:
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     si, ax
cmp     byte ptr [si-2B6Ch], 0
jz      short loc_164A8
mov     byte ptr [si-2B66h], 0Eh

loc_164A8:
inc     word ptr [bp-68h]

loc_164AB:
mov     ax, word_2F01A
cmp     [bp-68h], ax
jl      short loc_16494
push    word ptr [bp-74h]
mov     ax, 9B2h
jmp     loc_160D6

loc_164BC:
push    word ptr [bp-74h]
mov     ax, 9CDh
jmp     loc_160D6

loc_164C5:              ; jumptable 000167A7 case 3
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_164D5
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_164D5:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
add     ax, 0D494h
push    ax
call    sub_140B4
add     sp, 2
les     bx, dword_2E42A
cmp     word ptr es:[bx], 0
jg      short loc_164F2
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_164F2:
mov     bx, [bp-74h]
mov     al, 0Fh
mul     byte ptr [bx+32h]
mov     [bp-36h], ax
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp-36h], ax
jb      short loc_16567
mov     ax, [bp-3Ah]
inc     ax
mov     [bp-68h], ax
jmp     short loc_1651C

loc_16519:
inc     word ptr [bp-68h]

loc_1651C:
mov     ax, word_2F01A
cmp     [bp-68h], ax
jge     short loc_16557
mov     ax, 0Ch
imul    word ptr [bp-68h]
mov     si, ax
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     di, ax
mov     al, [di-2B69h]
cmp     [si-2B6Ch], al
jnz     short loc_16519
mov     al, [di-2B68h]
cmp     [si-2B6Bh], al
jnz     short loc_16519
mov     al, [di-2B67h]
cmp     [si-2B6Ah], al
jnz     short loc_16519
mov     byte ptr [si-2B66h], 0Eh

loc_16557:
push    word_2DFFE
push    word_2DFFC
push    word ptr [bp-74h]
mov     ax, 9EEh
jmp     short loc_16575

loc_16567:
push    word_2DFFE
push    word_2DFFC
push    word ptr [bp-74h] ; char *
mov     ax, offset aSFailsInAnAtte ; "%s fails in an attempt to stun %Fs!\r\r"

loc_16575:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
jmp     loc_160E1

loc_16583:              ; jumptable 000167A7 case 13
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_16593
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16593:
mov     bx, [bp-74h]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+542h]
push    word ptr [bp-74h] ; char *
mov     ax, offset aSDropsSS ; "%s drops %s %s.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     loc_163B0

loc_165DB:              ; jumptable 000167A7 case 12
mov     bx, [bp-74h]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
mov     [bp-68h], ax
sub     ax, ax
push    ax
mov     al, [bx+145h]
sub     ah, ah
push    ax
mov     al, [bx+146h]
push    ax
push    word ptr [bp-72h]
call    sub_11ED8
add     sp, 8
or      al, al
jz      short loc_1664A
mov     bx, [bp-74h]
mov     bl, [bx+146h]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h]
mov     ax, 18h
imul    word ptr [bp-68h]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
mov     bx, [bp-74h]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+542h]
push    word ptr [bp-74h] ; char *
mov     ax, offset aSTradesSSToS ; "%s trades %s %s to %s.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ch
jmp     loc_160E1

loc_1664A:
push    word ptr [bp-74h]
mov     ax, 0A5Fh
jmp     loc_160D6

loc_16653:              ; jumptable 000167A7 cases 8,9
push    word ptr [bp-74h]
call    sub_16F6B
add     sp, 2
or      al, al
jnz     short loc_16663
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_16663:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    word ptr [bp-72h]
call    sub_1C052
add     sp, 6
or      al, al
jz      short loc_166A0
mov     bx, [bp-74h]
mov     bl, [bx+146h]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h]
mov     bx, [bp-74h]
cmp     byte ptr [bx+144h], 8
jnz     short loc_16695
mov     ax, 0A73h
jmp     short loc_16698

loc_16695:
mov     ax, 0A79h

loc_16698:
push    ax
push    bx
mov     ax, 0A7Fh
jmp     loc_16575

loc_166A0:
mov     bx, [bp-74h]
mov     bl, [bx+146h]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h]
mov     bx, [bp-74h]
cmp     byte ptr [bx+144h], 8
jnz     short loc_166BE
mov     ax, 0A8Bh
jmp     short loc_166C1

loc_166BE:
mov     ax, 0A90h

loc_166C1:
push    ax
push    bx
mov     ax, 0A95h
jmp     loc_16575
sub_16328 endp

jpt_167A7 dw offset loc_15EEF ; jump table for switch statement
dw offset loc_160F4
dw offset loc_1645D
dw offset loc_164C5
dw offset loc_16152
dw offset loc_1634D
dw offset loc_1632D
dw offset loc_163C9
dw offset loc_16653
dw offset loc_16653
dw offset loc_16407
dw offset loc_16407
dw offset loc_165DB
dw offset loc_16583
dw offset def_167A7
dw offset loc_16023
; START OF FUNCTION CHUNK FOR sub_16328

def_167A7:              ; jumptable 000167A7 default case, case 14
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
call    sub_107E1
add     sp, 6
mov     ax, 1
push    ax
call    sub_12B93
add     sp, 2
cmp     byte ptr [bp-0Ch], 0
jz      short loc_1675B

loc_16707:
mov     ax, 1
push    ax
call    sub_1113F
add     sp, 2
mov     word ptr [bp-3Ah], 0

loc_16716:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp-3Ah]
ja      short loc_16728
jmp     loc_167B2

loc_16728:
mov     ax, 68h ; 'h'
imul    word ptr [bp-3Ah]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     [bp-7Eh], ax
mov     [bp-7Ch], dx
les     bx, [bp-7Eh]
cmp     word ptr es:[bx+44h], 1
jge     short loc_167AC
sub     ax, ax
push    ax
mov     ax, 1
push    ax
push    dx
push    bx
call    sub_18607
add     sp, 8
mov     byte ptr [bp-8], 0
jmp     short loc_16716

loc_1675B:
inc     word ptr [bp-3Ah]

loc_1675E:
mov     ax, word_2F01A
cmp     [bp-3Ah], ax
jge     short loc_16707
mov     ax, word_2D126
mov     [bp-70h], ax
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     si, ax
mov     al, [si-2B6Bh]
sub     ah, ah
mov     [bp-72h], ax
cmp     [si-2B6Ch], ah
jnz     short loc_1678E
mov     bx, ax
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp-74h], ax

loc_1678E:
mov     ax, 0Ch
imul    word ptr [bp-3Ah]
mov     bx, ax
mov     al, [bx-2B66h]
sub     ah, ah
cmp     ax, 0Fh         ; switch 16 cases
jbe     short loc_167A4
jmp     def_167A7       ; jumptable 000167A7 default case, case 14

loc_167A4:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_167A7[bx] ; switch jump

loc_167AC:
inc     word ptr [bp-3Ah]
jmp     loc_16716

loc_167B2:
call    sub_13716
call    sub_10EF7
call    sub_10D78
mov     al, [bp+6]
sub     ah, ah
push    ax
call    sub_14F1A
add     sp, 2
mov     word_2F01A, ax
cmp     byte ptr [bp-0Ch], 0
jnz     short loc_167E4
cmp     word_2DFEA, ax
jz      short loc_167E4
mov     word_2DFEA, ax
mov     ax, 1
push    ax
push    ax
call    sub_147D8
add     sp, 4

loc_167E4:
cmp     byte ptr [bp-0Ch], 0
jnz     short loc_167F4
cmp     word_2F01A, 0
jz      short loc_167F4
jmp     loc_168E8

loc_167F4:
mov     word ptr [bp-3Ah], 0
jmp     short loc_16854
; END OF FUNCTION CHUNK FOR sub_16328
; START OF FUNCTION CHUNK FOR sub_16867

loc_167FB:
mov     si, [bp-3Ah]
shl     si, 1
shl     si, 1
push    word ptr [bp+si-20h]
push    word ptr [bp+si-22h]
mov     bx, [bp-3Ah]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
mov     ax, offset aSGainsLdExperi ; "%s gains %ld experience points.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    getKeypress
mov     si, [bp-3Ah]
shl     si, 1
shl     si, 1
mov     ax, [bp+si-22h]
mov     dx, [bp+si-20h]
mov     bx, [bp-3Ah]
shl     bx, 1
mov     bx, [bx-2518h]
add     [bx+5Ah], ax
adc     [bx+5Ch], dx

loc_1684E:
call    sub_1DC28

loc_16851:
inc     word ptr [bp-3Ah]
; END OF FUNCTION CHUNK FOR sub_16867
; START OF FUNCTION CHUNK FOR sub_16328

loc_16854:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp-3Ah], ax
jnb     short loc_168D9
mov     bx, [bp-3Ah]
shl     bx, 1
mov     si, [bx-2518h]
; END OF FUNCTION CHUNK FOR sub_16328



sub_16867 proc near

; FUNCTION CHUNK AT 67FB SIZE 00000059 BYTES

cmp     word ptr [si+44h], 0FFC4h
jle     short loc_16851
mov     di, [bp-3Ah]
shl     di, 1
shl     di, 1
mov     ax, [bp+di-22h]
mov     dx, [bp+di-20h]
mov     [bp-7Eh], ax
mov     [bp-7Ch], dx
or      ax, dx
jz      short loc_16851
or      dx, dx
jl      short loc_1688B
jmp     loc_167FB

loc_1688B:
mov     ax, [bp-7Eh]
neg     ax
adc     dx, 0
neg     dx
push    dx
push    ax
push    si              ; char *
mov     ax, offset aSLosesLdExperi ; "%s loses %ld experience points for kill"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     di, [bp-3Ah]
shl     di, 1
shl     di, 1
mov     ax, [bp+di-22h]
mov     dx, [bp+di-20h]
neg     ax
adc     dx, 0
neg     dx
mov     bx, [bp-74h]
add     [bx+5Eh], ax
adc     [bx+60h], dx
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    getKeypress
jmp     loc_1684E
sub_16867 endp

; START OF FUNCTION CHUNK FOR sub_16328

loc_168D9:
mov     byte ptr [bp-3Ch], 1
jmp     short loc_168E8
; END OF FUNCTION CHUNK FOR sub_16328
; START OF FUNCTION CHUNK FOR sub_15691

loc_168DF:
call    sub_154B0
mov     ax, word_2DFEA
mov     word_2F01A, ax
; END OF FUNCTION CHUNK FOR sub_15691
; START OF FUNCTION CHUNK FOR sub_15691
;   ADDITIONAL PARENT FUNCTION sub_16328

loc_168E8:
cmp     [bp+var_3C], 0
jnz     short loc_168F1
jmp     loc_1572E

loc_168F1:
mov     al, byte ptr word_22C44
mov     byte ptr _disk1.field_4+1, al
mov     word_22C44, 0
call    sub_154B0
call    sub_201C8
mov     [bp+var_3A], 0
jmp     short loc_16920
; END OF FUNCTION CHUNK FOR sub_15691
; START OF FUNCTION CHUNK FOR sub_15691

loc_1690C:
mov     ax, 66h ; 'f'
imul    [bp+var_3A]
mov     si, ax
les     bx, dword_2E41E
and     byte ptr es:[bx+si+9], 0Fh
inc     [bp+var_3A]

loc_16920:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_3A]
ja      short loc_1690C
call    sub_1B452
mov     word_2F01A, 0
mov     byte_2E4BE, 0
call    sub_17B3F
cmp     [bp+var_C], 0
jz      short loc_16955
mov     ax, 1
push    ax
mov     al, byte_2E441
cbw
push    ax
call    sub_1253F
add     sp, 4

loc_16955:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
; END OF FUNCTION CHUNK FOR sub_15691
align 2


; Attributes: bp-based frame

sub_1695C proc near

win= word ptr -2
arg_0= dword ptr  4

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, _currentWindow
mov     [bp+win], ax
mov     ax, 1
push    ax
call    sub_21130
add     sp, 2
cmp     byte_2E4BE, 0
jz      short loc_1698D
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
push    word ptr [bp+arg_0] ; char *
call    sub_1B2F0
add     sp, 6
jmp     short loc_16996

loc_1698D:
push    word ptr [bp+arg_0]
call    sub_1B29B
add     sp, 2

loc_16996:
sub     ax, ax
push    ax
push    [bp+win]        ; win
call    setWindow
mov     sp, bp
pop     bp
retn
sub_1695C endp



; Attributes: bp-based frame

sub_169A3 proc near

var_2= word ptr -2
filename= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
jmp     short loc_169D0

loc_169AB:
mov     ax, 0Ah
imul    word_2132C
mov     bx, ax
push    word ptr [bx+0F2h] ; char *
mov     ax, offset aPleaseInsertSS ; "Please insert %s.\r\r<space bar> when r"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1695C
add     sp, 2

loc_169D0:
mov     ax, 8002h
push    ax              ; mode
push    [bp+filename]   ; filename
call    fileOpen
add     sp, 4
mov     [bp+var_2], ax
or      ax, ax
jz      short loc_169AB
mov     sp, bp
pop     bp
retn
sub_169A3 endp



; Attributes: bp-based frame

sub_169E8 proc near

var_2= word ptr -2
y= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 2
push    si              ; char *
mov     bx, [bp+y]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_2], ax
push    ax              ; char *
mov     ax, offset a1212s ; "%-12.12s "
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     bx, [bp+y]
mov     bl, byte_22BBA[bx]
sub     bh, bh
shl     bx, 1
push    off_21DD0[bx]
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _strcat
add     sp, 4
mov     bx, [bp+y]
mov     al, byte_22BBA[bx]
sub     ah, ah
sub     ax, 3           ; switch 11 cases
cmp     ax, 0Ah
jbe     short loc_16A37
jmp     def_16A3A       ; jumptable 00016A3A default case

loc_16A37:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_16A3A[bx] ; switch jump

loc_16A3F:              ; jumptable 00016A3A case 6
mov     bx, [bp+var_2]
mov     al, 6
mul     byte ptr [bx+4Ah]

loc_16A47:
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'

loc_16A55:
push    ax

loc_16A56:
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _strcat
add     sp, 4
jmp     def_16A3A       ; jumptable 00016A3A default case

loc_16A63:              ; jumptable 00016A3A case 4
mov     ax, [bp+y]
add     ax, word_2DFEA
mov     cx, 0Ch
imul    cx
add     ax, 0D494h
push    ax
call    sub_2023E
add     sp, 2
push    word_2DFFE
push    word_2DFFC      ; char *
mov     ax, 0BE5h

loc_16A86:
push    ax
mov     ax, offset unk_2323A
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 1FFAh
jmp     short loc_16A55

loc_16A96:              ; jumptable 00016A3A case 7
push    [bp+arg_2]
jmp     short loc_16A56

loc_16A9B:              ; jumptable 00016A3A cases 5,11-13
mov     bx, [bp+var_2]
mov     al, 6
mul     byte ptr [bx+145h]
jmp     short loc_16A47

loc_16AA6:              ; jumptable 00016A3A case 10
mov     bx, [bp+var_2]
cmp     byte ptr [bx+145h], 0Ch
jnb     short loc_16AC3
mov     al, 18h
mul     byte ptr [bx+145h]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 6
jmp     short loc_16AD4

loc_16AC3:
mov     al, 16h
mul     byte ptr [bx+145h]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 1Eh

loc_16AD4:
push    dx
push    ax
mov     ax, 0BE9h
jmp     short loc_16A86

loc_16ADB:              ; jumptable 00016A3A case 3
mov     ax, [bp+y]
add     ax, word_2DFEA
mov     cx, 0Ch
imul    cx
add     ax, 0D494h
push    ax
call    sub_2023E
add     sp, 2
push    word_2DFFE
push    word_2DFFC
mov     ax, 0BEDh
jmp     short loc_16A86

loc_16B00:              ; jumptable 00016A3A cases 8,9
mov     bx, [bp+var_2]
mov     bl, [bx+146h]
sub     bh, bh
shl     bx, 1
push    word ptr [bx-2518h]
jmp     loc_16A56
jpt_16A3A dw offset loc_16ADB ; jump table for switch statement
dw offset loc_16A63
dw offset loc_16A9B
dw offset loc_16A3F
dw offset loc_16A96
dw offset loc_16B00
dw offset loc_16B00
dw offset loc_16AA6
dw offset loc_16A9B
dw offset loc_16A9B
dw offset loc_16A9B

def_16A3A:              ; jumptable 00016A3A default case
mov     byte_2EC49, 0
push    [bp+y]          ; y
mov     ax, 3
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
sub_169E8 endp



; Attributes: bp-based frame

sub_16B44 proc near

var_12= byte ptr -12h
var_10= dword ptr -10h
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= dword ptr -8
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 12h
push    si
call    sub_1E08C
mov     [bp+var_2], ax
cmp     [bp+arg_0], 0
jnz     short loc_16B62
mov     ax, word_2E424
cmp     [bp+var_2], ax
jnb     short loc_16B62
jmp     loc_16C5A

loc_16B62:
mov     ax, [bp+var_2]
add     ax, 3
mov     word_2E424, ax
mov     [bp+var_A], 0
jmp     short loc_16BC1

loc_16B72:
mov     bx, [bp+var_4]
push    word ptr [bx+2]
mov     ax, [bx]
shl     ax, 1
push    ax
push    word ptr [bp+var_8+2]
push    word ptr [bp+var_8]
les     bx, [bp+var_10]
inc     word ptr [bp+var_10]
mov     al, es:[bx]
sub     ah, ah
push    ax
call    sub_210DD
add     sp, 0Ah
inc     [bp+var_C]

loc_16B9A:
mov     al, [bp+var_12]
sub     ah, ah
cmp     [bp+var_C], ax
jb      short loc_16B72
cmp     [bp+arg_0], ah
jnz     short loc_16BBE
mov     ax, 0Ch
imul    [bp+var_A]
mov     bx, ax
mov     bx, [bx-413Eh]
push    word ptr [bx+1Ah]
call    sub_1DA17
add     sp, 2

loc_16BBE:
inc     [bp+var_A]

loc_16BC1:
mov     ax, word_2D126
cmp     [bp+var_A], ax
jl      short loc_16BCC
jmp     loc_16C5A

loc_16BCC:
mov     ax, 0Ch
imul    [bp+var_A]
mov     si, ax
mov     ax, [si-413Ah]
mov     dx, [si-4138h]
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
mov     ax, [si-413Ch]
add     ax, word ptr [bp+var_8]
mov     word ptr [bp+var_10], ax
mov     word ptr [bp+var_10+2], dx
les     bx, [bp+var_10]
inc     word ptr [bp+var_10]
mov     al, es:[bx]
mov     [bp+var_12], al
cmp     al, 0FFh
jnz     short loc_16C2C
les     bx, [bp+var_8]
mov     ah, es:[bx+2]
sub     al, al
mov     cl, es:[bx+1]
sub     ch, ch
add     ax, cx
mov     [si-413Ch], ax
add     ax, word ptr [bp+var_8]
mov     dx, word ptr [bp+var_8+2]
mov     word ptr [bp+var_10], ax
mov     word ptr [bp+var_10+2], dx
les     bx, [bp+var_10]
inc     word ptr [bp+var_10]
mov     al, es:[bx]
mov     [bp+var_12], al

loc_16C2C:
mov     ax, 0Ch
imul    [bp+var_A]
mov     si, ax
mov     al, [bp+var_12]
sub     ah, ah
inc     ax
add     [si-413Ch], ax
cmp     byte ptr [si-4134h], 0
jnz     short loc_16C48
jmp     loc_16BBE

loc_16C48:
mov     ax, [si-413Eh]
add     ax, 0Ch
mov     [bp+var_4], ax
mov     [bp+var_C], 0
jmp     loc_16B9A

loc_16C5A:
pop     si
mov     sp, bp
pop     bp
retn
sub_16B44 endp



; Attributes: bp-based frame

; int __cdecl getKeypress()
getKeypress proc near

var_6= byte ptr -6
var_4= byte ptr -4
var_2= byte ptr -2

push    bp
mov     bp, sp
sub     sp, 6
mov     [bp+var_6], 0
jmp     short loc_16C74

loc_16C6B:
sub     ax, ax
push    ax
call    sub_16B44
add     sp, 2

loc_16C74:
call    _kbhit
or      ax, ax
jz      short loc_16C6B
call    _getch
mov     [bp+var_4], al
cbw
or      ax, ax
jz      short loc_16CAC
cmp     ax, 0Dh
jnz     short loc_16C8E
jmp     loc_16D49

loc_16C8E:
cmp     ax, 1Bh
jnz     short loc_16C96
jmp     loc_16D43

loc_16C96:
cmp     ax, 20h ; ' '
jnz     short loc_16C9E
jmp     loc_16D49

loc_16C9E:
cbw
push    ax
call    sub_1FBE2
add     sp, 2

loc_16CA6:
mov     [bp+var_6], al
jmp     def_16CC1       ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CAC:
call    _getch
mov     [bp+var_2], al
cbw
sub     ax, 3Bh ; ';'   ; switch 22 cases
cmp     ax, 15h
jbe     short loc_16CBE
jmp     def_16CC1       ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CBE:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_16CC1[bx] ; switch jump

loc_16CC6:              ; jumptable 00016CC1 cases 59-63
mov     al, [bp+var_2]
jmp     short loc_16CA6

loc_16CCB:              ; jumptable 00016CC1 case 72
mov     [bp+var_6], 0FDh
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CD1:              ; jumptable 00016CC1 case 80
mov     [bp+var_6], 0FCh
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CD7:              ; jumptable 00016CC1 case 75
mov     [bp+var_6], 0FBh
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CDD:              ; jumptable 00016CC1 case 77
mov     [bp+var_6], 0FAh
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16CE3:              ; jumptable 00016CC1 case 68
mov     ax, 1
push    ax
call    sub_21130
add     sp, 2
cmp     byte_2CC1E, 1
sbb     ax, ax
neg     ax
mov     byte_2CC1E, al
mov     ax, 1
push    ax
call    sub_21130
add     sp, 2
mov     ax, 1
push    ax
call    sub_21130
add     sp, 2
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79
jpt_16CC1 dw offset loc_16CC6 ; jump table for switch statement
dw offset loc_16CC6
dw offset loc_16CC6
dw offset loc_16CC6
dw offset loc_16CC6
dw offset def_16CC1
dw offset def_16CC1
dw offset def_16CC1
dw offset def_16CC1
dw offset loc_16CE3
dw offset def_16CC1
dw offset def_16CC1
dw offset def_16CC1
dw offset loc_16CCB
dw offset def_16CC1
dw offset def_16CC1
dw offset loc_16CD7
dw offset def_16CC1
dw offset loc_16CDD
dw offset def_16CC1
dw offset def_16CC1
dw offset loc_16CD1
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16D43:
mov     [bp+var_6], 0FFh
jmp     short def_16CC1 ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79

loc_16D49:
mov     [bp+var_6], 0FEh

def_16CC1:              ; jumptable 00016CC1 default case, cases 64-67,69-71,73,74,76,78,79
cmp     [bp+var_6], 0
jnz     short loc_16D56
jmp     loc_16C74

loc_16D56:
mov     al, [bp+var_6]
cbw
mov     sp, bp
pop     bp
retn
getKeypress endp




sub_16D5E proc near
jmp     short loc_16D63

loc_16D60:
call    _getch

loc_16D63:
call    _kbhit
or      ax, ax
jnz     short loc_16D60
sub_16D5E endp

retn


; Attributes: bp-based frame

sub_16D6B proc near

var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     ax, 18h
imul    [bp+arg_0]
mov     si, ax
mov     bx, _globals
mov     al, [bx+si+3Eh]
mov     [bp+var_2], al
and     al, 1Ch
cmp     al, 1Ch
jnz     short loc_16D8F
mov     ax, 1
jmp     short loc_16D91

loc_16D8F:
sub     ax, ax

loc_16D91:
pop     si
mov     sp, bp
pop     bp
retn
sub_16D6B endp



; Attributes: bp-based frame

sub_16D96 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
push    si
mov     ax, 18h
imul    [bp+arg_0]
mov     si, ax
mov     bx, _globals
mov     cl, [bx+si+41h]
mov     al, 1
shl     al, cl
or      _disk1.field_7, al
pop     si
pop     bp
retn
sub_16D96 endp



; Attributes: bp-based frame

sub_16DB4 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
push    si
mov     ax, 18h
imul    [bp+arg_0]
mov     si, ax
mov     bx, _globals
mov     cl, [bx+si+41h]
mov     al, 1
shl     al, cl
not     al
and     _disk1.field_7, al
pop     si
pop     bp
retn
sub_16DB4 endp



; Attributes: bp-based frame

sub_16DD4 proc near

var_A= dword ptr -0Ah
var_6= word ptr -6
var_4= byte ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6

; FUNCTION CHUNK AT 6E89 SIZE 000000B1 BYTES

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_A], ax
mov     [bp+var_2], 0
mov     [bp+var_4], 0
mov     bx, ax
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
mov     al, [bx+si+62h]
sub     ah, ah
mov     [bp+var_6], ax
mov     al, [bx+3Eh]
mov     word ptr [bp+var_A+2], ax
mov     ax, 14h
imul    word ptr [bp+var_A+2]
mov     word ptr [bp+var_A+2], ax
cmp     ax, 64h ; 'd'
jle     short loc_16E1A
mov     word ptr [bp+var_A+2], 64h ; 'd'

loc_16E1A:
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     word ptr [bp+var_A+2], ax
jb      short loc_16E31
mov     al, 1
jmp     short loc_16E33

loc_16E31:
sub     al, al

loc_16E33:
mov     [bp+var_2], al
or      al, al
jz      short loc_16E89
mov     bx, word ptr [bp+var_A]
mov     al, 6
mul     byte ptr [bx+145h]
mov     si, ax
and     byte ptr [bx+si+65h], 0FEh
mov     ax, 18h
imul    [bp+var_6]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
mov     bx, word ptr [bp+var_A]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+542h]
push    word ptr [bp+var_A] ; char *
mov     ax, offset aSSuccessfullyU ; "%s successfully unjams %s %s.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf

loc_16E74:
add     sp, 0Ah
sub_16DD4 endp ; sp-analysis failed

mov     ax, 1Ah
push    ax
push    word ptr [bp+4]
call    sub_1969F
add     sp, 4
mov     [bp-4], al
jmp     short loc_16ED0
; START OF FUNCTION CHUNK FOR sub_16DD4

loc_16E89:
mov     ax, 18h
imul    [bp+var_6]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
mov     bx, word ptr [bp+var_A]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+542h]
push    word ptr [bp+var_A] ; char *
mov     ax, offset aSTrashesSSWhil ; "%s trashes %s %s while trying to fix it"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     ax, 0FFh
push    ax
mov     bx, word ptr [bp+var_A]
mov     al, [bx+145h]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_2060D
add     sp, 6

loc_16ED0:
cmp     [bp+arg_2], 0
jz      short loc_16F01
cmp     [bp+var_4], 0
jz      short loc_16EEF
mov     ax, 1
push    ax              ; char
mov     ax, 1Ah
push    ax
push    word ptr [bp+var_A] ; char *
call    sub_16F9D
add     sp, 6
jmp     short loc_16F35

loc_16EEF:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
jmp     short loc_16F35

loc_16F01:
call    winClearCurrent
cmp     [bp+var_4], 0
jz      short loc_16F1D
mov     ax, 1
push    ax              ; char
mov     ax, 1Ah
push    ax
push    word ptr [bp+var_A] ; char *
call    sub_16F9D
add     sp, 6
jmp     short loc_16F2F

loc_16F1D:
mov     ax, 1
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_16F2F:
call    sub_1DC28
call    getKeypress

loc_16F35:
pop     si
mov     sp, bp
pop     bp
retn
; END OF FUNCTION CHUNK FOR sub_16DD4


; Attributes: bp-based frame

sub_16F3A proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
cmp     [bp+arg_0], 0
jz      short loc_16F57
sub_16F3A endp ; sp-analysis failed




sub_16F43 proc near
cmp     byte ptr [bp+4], 0Eh
jnb     short loc_16F57
mov     al, [bp+4]
sub     ah, ah
sub     ax, 0Eh
neg     ax
neg     ax
jmp     short loc_16F69

loc_16F57:
cmp     byte ptr [bp+4], 0Fh
jbe     short loc_16F67
mov     al, [bp+4]
sub     ah, ah
sub     ax, 0Fh
jmp     short loc_16F69

loc_16F67:
sub     ax, ax

loc_16F69:
pop     bp
retn
sub_16F43 endp ; sp-analysis failed



; Attributes: bp-based frame

sub_16F6B proc near

var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 4
mov     bx, [bp+arg_0]
mov     ax, [bx+44h]
mov     [bp+var_4], ax
mov     al, [bx+1Bh]
mov     [bp+var_2], al
cmp     al, 0Fh
jbe     short loc_16F8C
sub     ah, ah
sub     ax, 0Fh
add     [bp+var_4], ax

loc_16F8C:
cmp     [bp+var_4], 0
jle     short loc_16F97
mov     ax, 1
jmp     short loc_16F99

loc_16F97:
sub     ax, ax

loc_16F99:
mov     sp, bp
pop     bp
retn
sub_16F6B endp



; Attributes: bp-based frame

; int __cdecl sub_16F9D(char *, char)
sub_16F9D proc near

arg_0= dword ptr  4
arg_4= byte ptr  8

push    bp
mov     bp, sp
push    si              ; draw
mov     si, word ptr [bp+arg_0+2]
mov     bx, word ptr [bp+arg_0]
mov     al, [bx+si+24h]
sub     ah, ah
push    ax
mov     ax, 16h
imul    si
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 126h
push    dx
push    ax
push    bx              ; char *
mov     ax, offset aSNowHasAFsLeve ; "%s now has a %Fs level of %d."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ch
cmp     [bp+arg_4], 0
jz      short loc_16FEC
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DDFD
call    sub_1DDFD
jmp     short loc_17012

loc_16FEC:
sub     ax, ax
push    ax
mov     ax, offset _windowInfo
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_1DDFD
call    sub_1DDFD
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_17012:
pop     si
pop     bp
retn
sub_16F9D endp



; Attributes: bp-based frame

sub_17015 proc near

var_C= byte ptr -0Ch
var_A= word ptr -0Ah
var_8= byte ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ch
push    si
mov     [bp+var_2], 0
mov     [bp+var_4], 0
jmp     short loc_17064

loc_17027:
mov     bx, [bp+var_4]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
mov     bx, ax
cmp     word ptr [bx+44h], 0
jle     short loc_17061
mov     si, [bp+arg_0]
cmp     byte ptr [bx+si+24h], 0
jz      short loc_17061
mov     si, [bp+arg_2]
mov     al, [bx+si+18h]
mov     [bp+var_C], al
mov     al, [bp+var_2]
cmp     [bp+var_C], al
jbe     short loc_17061
mov     al, [bp+var_C]
mov     [bp+var_2], al
mov     ax, [bp+var_4]
mov     [bp+var_A], ax

loc_17061:
inc     [bp+var_4]

loc_17064:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_17027
cmp     [bp+var_2], ah
jz      short loc_170AD
push    [bp+var_A]
call    sub_198E3
add     sp, 2
mov     al, [bp+var_2]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
shl     al, 1
mov     [bp+var_8], al
or      al, al
jz      short loc_170AD
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     cl, [bp+var_8]
sub     ch, ch
cmp     cx, ax
jb      short loc_170AD
mov     ax, 1
jmp     short loc_170AF

loc_170AD:
sub     ax, ax

loc_170AF:
pop     si
mov     sp, bp
pop     bp
retn
sub_17015 endp

push    bp
mov     bp, sp
sub     sp, 28h
mov     ax, 14Ch
imul    word ptr [bp+6]
add     ax, word ptr dword_22C6E
mov     dx, word ptr dword_22C6E+2
add     ax, 20h ; ' '
push    dx
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1919fs ; "%1d> %-19.19Fs"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 0Ah
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_170F2 proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
jmp     short loc_17102

loc_170FF:
inc     [bp+var_2]

loc_17102:
cmp     [bp+var_2], 5
jge     short loc_17117
mov     ax, 14Ch
imul    [bp+var_2]
mov     bx, ax
cmp     byte ptr [bx-40DCh], 0
jnz     short loc_170FF

loc_17117:
mov     ax, [bp+var_2]
mov     sp, bp
pop     bp
retn
sub_170F2 endp




sub_1711E proc near
cmp     byte_2ED26, 0
jz      short loc_17135
mov     ax, 1
push    ax
sub     ax, ax
push    ax
push    ax
call    sub_107E1
add     sp, 6
jmp     short locret_1713F

loc_17135:
mov     ax, 1
push    ax
call    writeParty
add     sp, 2

locret_1713F:
retn
sub_1711E endp



; Attributes: bp-based frame

sub_17140 proc near

var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
win= dword ptr -6
var_2= byte ptr -2

; FUNCTION CHUNK AT 72B4 SIZE 000000D6 BYTES

push    bp
mov     bp, sp
sub     sp, 10h
push    di
push    si              ; draw
mov     [bp+var_10], 0
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     byte ptr [bp+win+2], 0
cmp     byte_2ED26, 1
sbb     ax, ax
neg     ax
push    ax
cmp     byte_2ED26, 0
jz      short loc_1716D
mov     ax, offset _window7
jmp     short loc_17170

loc_1716D:
mov     ax, offset _windowDialog

loc_17170:              ; win
push    ax
call    setWindow
add     sp, 4
jmp     loc_1736F

loc_1717A:
call    winClearCurrent
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset aWhatDoYouWish1 ; "What do you wish?\r\r\r1. Retrieve a me"...
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
call    getKeypress
mov     [bp+var_2], al
cbw
cmp     ax, 31h ; '1'
jz      short loc_171AE
cmp     ax, 32h ; '2'
jnz     short loc_171A3
jmp     loc_172BD

loc_171A3:
cmp     ax, 4Ch ; 'L'
jnz     short loc_171AB
jmp     loc_1736B

loc_171AB:
jmp     loc_1736F

loc_171AE:
les     bx, dword_22C6E
cmp     word ptr es:[bx+1Eh], 0
jnz     short loc_171BF
mov     ax, 0CACh
jmp     loc_172D7

loc_171BF:
cmp     _disk1._partyCount, 5
jnz     short loc_171CC
mov     ax, 0CC2h
jmp     loc_172D7

loc_171CC:
sub     ax, ax
push    ax
mov     ax, 70B4h
push    ax
mov     ax, 0CD7h
push    ax
lea     ax, [bp+var_10]
push    ax
les     bx, dword_22C6E
push    word ptr es:[bx+1Eh]
push    _party
call    sub_11682
add     sp, 0Ch
mov     [bp+var_A], ax
or      ax, ax
jge     short loc_171F7
jmp     loc_1736F

loc_171F7:
call    sub_170F2
mov     [bp+var_8], ax
mov     bl, _disk1._partyCount
sub     bh, bh
mov     al, byte ptr [bp+var_8]
mov     [bx-40E4h], al
mov     ax, 14Ch
imul    [bp+var_8]
mov     si, ax
add     si, 0BF24h
mov     bl, _disk1._partyCount
shl     bx, 1
mov     [bx-2518h], si
mov     ax, 14Ch
imul    [bp+var_A]
add     ax, word ptr dword_22C6E
mov     dx, word ptr dword_22C6E+2
add     ax, 20h ; ' '
push    si
mov     di, si
mov     si, ax
push    ds
pop     es
assume es:dseg
push    ds
mov     ds, dx
mov     cx, 0A6h
repne movsw
pop     ds
pop     si
mov     bl, _disk1._partyCount
inc     word ptr _disk1._partyCount
sub     bh, bh
shl     bx, 1
mov     bx, [bx-2518h]
mov     bl, [bx+49h]
sub     bh, bh
or      byte ptr [bx-3A60h], 1
les     bx, dword_22C6E
sub_17140 endp ; sp-analysis failed

assume es:nothing
db 26h



sub_17261 proc near
mov     ax, [bx+1Eh]
sub_17261 endp

dec     ax
cmp     ax, [bp-0Ah]
jz      short loc_172B0
mov     ax, [bp-0Ah]
mov     [bp-0Eh], ax
jmp     short loc_172A2

loc_17272:
mov     ax, 14Ch
imul    word ptr [bp-0Eh]
add     ax, bx
mov     dx, es
add     ax, 16Ch
mov     cx, ax
mov     ax, 14Ch
mov     bx, dx
imul    word ptr [bp-0Ah]
mov     si, ax
mov     ax, bx
mov     bx, word ptr dword_22C6E
lea     di, [bx+si+20h]
mov     si, cx
push    ds
mov     ds, ax
mov     cx, 0A6h
repne movsw
pop     ds
inc     word ptr [bp-0Eh]

loc_172A2:
les     bx, dword_22C6E
mov     ax, es:[bx+1Eh]
dec     ax
cmp     ax, [bp-0Eh]
ja      short loc_17272

loc_172B0:
dec     word ptr es:[bx+1Eh]
; START OF FUNCTION CHUNK FOR sub_17140

loc_172B4:
call    sub_17E09
call    sub_1711E
jmp     loc_1736F

loc_172BD:
cmp     _disk1._partyCount, 1
jnz     short loc_172C9
mov     ax, offset aWithYouGoneWho ; "\rWith you gone, who's gonna type?"
jmp     short loc_172D7

loc_172C9:
les     bx, dword_22C6E
cmp     word ptr es:[bx+1Eh], 3
jnz     short loc_172E1
mov     ax, offset aWeReFullRightN ; "\rWe're full right now!"

loc_172D7:
push    ax
call    sub_1B29B
add     sp, 2
jmp     loc_1736F

loc_172E1:
mov     [bp+var_E], 0
mov     [bp+var_C], 0
jmp     short loc_17305

loc_172ED:
mov     bx, [bp+var_C]
inc     [bp+var_C]
shl     bx, 1
mov     si, [bp+var_E]
mov     al, [si-40E4h]
sub     ah, ah
mov     [bx-3240h], ax
inc     [bp+var_E]

loc_17305:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_E], ax
jb      short loc_172ED
sub     ax, ax
push    ax
mov     ax, 0D23Bh
push    ax
mov     ax, 0D1Eh
push    ax
lea     ax, [bp+var_10]
push    ax
push    [bp+var_C]
push    _party
call    sub_11682
add     sp, 0Ch
mov     [bp+var_8], ax
or      ax, ax
jl      short loc_1736F
mov     bx, ax
shl     bx, 1
mov     ax, [bx-2518h]
les     bx, dword_22C6E
mov     cx, ax
mov     ax, es:[bx+1Eh]
inc     word ptr es:[bx+1Eh]
mov     bx, 14Ch
mul     bx
mov     si, ax
les     bx, dword_22C6E
lea     di, [bx+si+20h]
mov     si, cx
mov     cx, 0A6h
repne movsw
push    [bp+var_8]
call    sub_2045A
add     sp, 2
jmp     loc_172B4

loc_1736B:
mov     byte ptr [bp+win+2], 1

loc_1736F:
cmp     byte ptr [bp+win+2], 0
jnz     short loc_17378
jmp     loc_1717A

loc_17378:
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
pop     si
pop     di
mov     sp, bp
pop     bp
retn
; END OF FUNCTION CHUNK FOR sub_17140


; Attributes: bp-based frame

sub_1738A proc near

text= dword ptr  4
arg_4= byte ptr  8

push    bp
mov     bp, sp
call    winClearCurrent
mov     al, [bp+arg_4]
cbw
push    ax              ; y
mov     al, byte ptr [bp+text+2]
cbw
push    ax              ; x
push    ds
push    word ptr [bp+text] ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
call    getKeypress
pop     bp
retn
sub_1738A endp



; Attributes: bp-based frame

; int __cdecl sub_173AC(char *)
sub_173AC proc near

member= dword ptr  4

push    bp
mov     bp, sp
call    winClearCurrent
mov     al, byte ptr [bp+member+2]
cbw
push    ax              ; int
push    ds
push    word ptr [bp+member] ; member
call    writeShortMemberName
add     sp, 6
call    sub_1DC28
call    getKeypress
pop     bp
retn
sub_173AC endp



; Attributes: bp-based frame

sub_173C9 proc near

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= dword ptr  8
arg_8= dword ptr  0Ch

push    bp
mov     bp, sp
sub     sp, 0Ah
mov     [bp+var_6], 0
mov     [bp+var_A], 0
jmp     short loc_173ED

loc_173DB:
mov     bx, [bp+var_6]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bx+46h]
add     [bp+var_A], ax
inc     [bp+var_6]

loc_173ED:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_6], ax
jb      short loc_173DB
mov     cx, ax
mov     ax, [bp+var_A]
cwd
idiv    cx
mov     [bp+var_A], ax
imul    [bp+arg_0]
cwd
mov     cx, 64h ; 'd'
idiv    cx
mov     [bp+var_8], ax
mov     ax, [bp+var_A]
sub     ax, [bp+var_8]
cmp     ax, 1
jge     short loc_1741C
mov     ax, 1

loc_1741C:
mov     [bp+var_2], ax
mov     ax, [bp+var_A]
add     ax, [bp+var_8]
cmp     ax, 270Fh
jle     short loc_1742D
mov     ax, 270Fh

loc_1742D:
mov     [bp+var_4], ax
push    ax
push    [bp+var_2]
call    sub_1DEB1
add     sp, 4
les     bx, [bp+arg_4]
mov     es:[bx], ax
les     bx, [bp+arg_4]
mov     ax, es:[bx]
cwd
mov     cx, [bp+arg_2]
idiv    cx
les     bx, [bp+arg_8]
mov     es:[bx], al
les     bx, [bp+arg_8]
cmp     byte ptr es:[bx], 1
jnb     short loc_1745F
mov     byte ptr es:[bx], 1

loc_1745F:
les     bx, [bp+arg_8]
cmp     byte ptr es:[bx], 63h ; 'c'
jbe     short loc_1746C
mov     byte ptr es:[bx], 63h ; 'c'

loc_1746C:
mov     sp, bp
pop     bp
retn
sub_173C9 endp



; Attributes: bp-based frame

sub_17470 proc near

var_C= dword ptr -0Ch
var_8= dword ptr -8
var_4= word ptr -4
var_2= word ptr -2
arg_0= dword ptr  4

push    bp
mov     bp, sp
sub     sp, 0Ch
push    di
push    si
les     bx, [bp+arg_0]
mov     al, es:[bx]
mov     es:[bx+2], al
les     bx, [bp+arg_0]
mov     al, es:[bx+1]
mov     es:[bx+3], al
les     bx, [bp+arg_0]
mov     byte ptr es:[bx+5], 1
les     bx, [bp+arg_0]
mov     byte ptr es:[bx+4], 1
les     bx, [bp+arg_0]
mov     byte ptr es:[bx+0Ah], 0
les     bx, [bp+arg_0]
mov     byte ptr es:[bx+0Bh], 1
mov     [bp+var_2], 0
jmp     loc_1756C

loc_174B5:
les     bx, [bp+var_8]
mov     byte ptr es:[bx+0Fh], 1

loc_174BD:
les     bx, [bp+arg_0]
test    byte ptr es:[bx+6], 38h
jz      short loc_17518
mov     [bp+var_4], 0
jmp     short loc_174D1

loc_174CE:
inc     [bp+var_4]

loc_174D1:
les     bx, [bp+var_8]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_4]
ja      short loc_174E2
jmp     loc_17569

loc_174E2:
mov     si, [bp+var_4]
mov     es:[bx+si+0Ah], ah
mov     ax, word ptr [bp+var_8]
mov     dx, word ptr [bp+var_8+2]
inc     ax
push    dx
push    ax
mov     ax, [bp+var_4]
shl     ax, 1
add     ax, word ptr [bp+var_8]
add     ax, 2
push    dx
push    ax
les     bx, [bp+var_C]
push    word ptr es:[bx+32h]
les     bx, [bp+arg_0]
mov     al, es:[bx+7]
sub     ah, ah
push    ax
call    sub_173C9
add     sp, 0Ch
jmp     short loc_174CE

loc_17518:
mov     [bp+var_4], 0
jmp     short loc_1755B

loc_1751F:
mov     di, [bp+var_4]
les     bx, [bp+var_8]
mov     byte ptr es:[bx+di+0Ah], 0
les     bx, [bp+var_8]
mov     al, es:[bx+1]
sub     ah, ah
mov     si, ax
les     bx, [bp+var_C]
mov     ax, es:[bx+32h]
mul     si
push    ax
mov     ax, es:[bx+30h]
mul     si
push    ax
call    sub_1DEB1
add     sp, 4
mov     di, [bp+var_4]
shl     di, 1
les     bx, [bp+var_8]
mov     es:[bx+di+2], ax
inc     [bp+var_4]

loc_1755B:
les     bx, [bp+var_8]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_4]
ja      short loc_1751F

loc_17569:
inc     [bp+var_2]

loc_1756C:
les     bx, [bp+arg_0]
mov     al, es:[bx+8]
sub     ah, ah
cmp     ax, [bp+var_2]
ja      short loc_1757D
jmp     loc_17657

loc_1757D:
mov     ax, 16h
imul    [bp+var_2]
add     ax, bx
mov     dx, es
add     ax, 0Eh
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
mov     al, 1
mov     cl, byte ptr [bp+var_2]
shl     al, cl
or      es:[bx+9], al
les     bx, [bp+var_8]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word ptr [bp+var_C], ax
mov     word ptr [bp+var_C+2], dx
les     bx, [bp+arg_0]
mov     al, es:[bx+5]
les     bx, [bp+var_C]
cmp     es:[bx+37h], al
jbe     short loc_175D0
mov     al, es:[bx+37h]
les     bx, [bp+arg_0]
mov     es:[bx+5], al

loc_175D0:
les     bx, [bp+arg_0]
mov     al, es:[bx+4]
les     bx, [bp+var_C]
cmp     es:[bx+36h], al
jbe     short loc_175EB
mov     al, es:[bx+36h]
les     bx, [bp+arg_0]
mov     es:[bx+4], al

loc_175EB:
les     bx, [bp+arg_0]
mov     al, es:[bx+0Bh]
les     bx, [bp+var_C]
cmp     es:[bx+35h], al
jbe     short loc_17606
mov     al, es:[bx+35h]
les     bx, [bp+arg_0]
mov     es:[bx+0Bh], al

loc_17606:
les     bx, [bp+var_8]
mov     al, es:[bx+0Eh]
mov     es:[bx+0Fh], al
les     bx, [bp+var_8]
cmp     byte ptr es:[bx+12h], 64h ; 'd'
jb      short loc_1761E
jmp     loc_174BD

loc_1761E:
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
les     bx, [bp+var_8]
mov     cl, es:[bx+12h]
sub     ch, ch
cmp     cx, ax
jnb     short loc_1763C
jmp     loc_174B5

loc_1763C:
mov     al, es:[bx+0Eh]
sub     ah, ah
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
les     bx, [bp+var_8]
mov     es:[bx+0Fh], al
jmp     loc_174BD

loc_17657:
les     bx, [bp+arg_0]
mov     al, es:[bx+0Ch]
sub     ah, ah
push    ax
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
mov     al, es:[bx+5]
sub     ah, ah
push    ax
call    sub_10D19
add     sp, 8
mov     byte_2F01E, 1
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_17470 endp



; Attributes: bp-based frame

sub_17685 proc near

var_1A= dword ptr -1Ah
var_16= word ptr -16h
var_14= byte ptr -14h
var_12= word ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= dword ptr -0Ch
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= byte ptr -2

push    bp
mov     bp, sp
sub     sp, 1Ah
push    di
push    si
mov     [bp+var_2], 0
les     bx, dword_22C6E
cmp     byte ptr es:[bx+8], 0
jnz     short loc_1769F
jmp     loc_17848

loc_1769F:
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
les     bx, dword_22C6E
mov     cl, es:[bx+7]
sub     ch, ch
cmp     cx, ax
jnb     short loc_176BE
jmp     loc_17848

loc_176BE:
mov     ax, 7
push    ax
sub     ax, ax
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+var_12], ax
mov     [bp+var_E], 0
mov     [bp+var_14], 0
jmp     short loc_176E9

loc_176D9:
inc     [bp+var_E]
mov     ax, [bp+var_12]
inc     ax
cwd
mov     cx, 8
idiv    cx
mov     [bp+var_12], dx

loc_176E9:
cmp     [bp+var_E], 8
jge     short loc_17716
mov     si, [bp+var_12]
mov     cl, 2
shl     si, cl
mov     ax, [si+152h]
add     ax, _disk1._mapPosY
push    ax
mov     ax, [si+150h]
add     ax, _disk1._mapPosX
push    ax
call    sub_12C0F
add     sp, 4
or      al, al
jz      short loc_176D9
mov     [bp+var_14], 1

loc_17716:
cmp     [bp+var_14], 0
jnz     short loc_17721
sub     ax, ax
jmp     loc_1784D

loc_17721:
les     bx, dword_2E41E
mov     al, es:[bx+0Dh]
sub     ah, ah
mov     [bp+var_16], ax
mov     [bp+var_6], 0
mov     [bp+var_4], 0
jmp     short loc_1775F

loc_1773A:
mov     ax, 66h ; 'f'
imul    [bp+var_6]
mov     di, ax
les     bx, dword_2E41E
mov     al, es:[bx+di+0Dh]
sub     ah, ah
mov     si, ax
cmp     [bp+var_16], si
jbe     short loc_1775C
mov     [bp+var_16], si
mov     ax, [bp+var_6]
mov     [bp+var_4], ax

loc_1775C:
inc     [bp+var_6]

loc_1775F:
cmp     [bp+var_6], 8
jl      short loc_1773A
mov     ax, [bp+var_4]
mov     [bp+var_6], ax
mov     ax, 66h ; 'f'
imul    [bp+var_6]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_C], ax
mov     word ptr [bp+var_C+2], dx
les     bx, [bp+var_C]
cmp     byte ptr es:[bx+0Dh], 0
jz      short loc_1779E
sub     ax, ax
push    ax
mov     al, es:[bx+3]
cbw
push    ax
mov     al, es:[bx+2]
cbw
push    ax
call    sub_12AB2
add     sp, 6

loc_1779E:
les     bx, dword_22C6E
mov     al, es:[bx+8]
sub     ah, ah
add     ax, 7
push    ax
mov     ax, 8
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+var_10], ax
mov     ax, 66h ; 'f'
imul    [bp+var_10]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
les     bx, [bp+var_C]
mov     di, bx
mov     si, ax
push    ds
mov     ds, dx
mov     cx, 33h ; '3'
repne movsw
pop     ds
mov     si, [bp+var_12]
shl     si, 1
shl     si, 1
mov     al, [si+150h]
add     al, byte ptr _disk1._mapPosX
mov     es:[bx], al
les     bx, [bp+var_C]
mov     si, [bp+var_12]
shl     si, 1
shl     si, 1
mov     al, [si+152h]
add     al, byte ptr _disk1._mapPosY
mov     es:[bx+1], al
push    word ptr [bp+var_C+2]
push    word ptr [bp+var_C]
call    sub_17470
add     sp, 4
mov     [bp+var_8], 0

loc_17810:
mov     ax, 66h ; 'f'
imul    [bp+var_8]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Dh
mov     word ptr [bp+var_1A], ax
mov     word ptr [bp+var_1A+2], dx
les     bx, [bp+var_1A]
cmp     byte ptr es:[bx], 0
jz      short loc_17833
dec     byte ptr es:[bx]

loc_17833:
inc     [bp+var_8]
cmp     [bp+var_8], 8
jl      short loc_17810
les     bx, [bp+var_C]
mov     byte ptr es:[bx+0Dh], 0FFh
mov     [bp+var_2], 1

loc_17848:
mov     al, [bp+var_2]
sub     ah, ah

loc_1784D:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_17685 endp

align 2


; Attributes: bp-based frame

sub_17854 proc near

var_18= word ptr -18h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_6= byte ptr -6

push    bp
mov     bp, sp
sub     sp, 18h
push    si
mov     ax, _currentWindow
mov     [bp+var_C], ax
mov     [bp+var_E], 0
mov     [bp+var_A], 0
jmp     short loc_178EC

loc_1786D:
mov     bx, [bp+var_E]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_18], ax
push    ax              ; member
call    getMemberCon
add     sp, 2
cmp     ax, 5
jz      short loc_178E9
mov     bx, [bp+var_18]
cmp     byte ptr [bx+57h], 0
jz      short loc_17891
dec     byte ptr [bx+57h]

loc_17891:
mov     si, [bp+var_A]
mov     [bp+si+var_6], 0
mov     bx, [bp+var_18]
cmp     byte ptr [bx+21h], 0
jz      short loc_178B2
dec     byte ptr [bx+21h]
cmp     byte ptr [bx+21h], 1
sbb     ax, ax
neg     ax
mov     si, [bp+var_A]
mov     [bp+si+var_6], al

loc_178B2:
mov     si, [bp+var_A]
cmp     [bp+si+var_6], 0
jnz     short loc_178DA
mov     bx, [bp+var_18]
cmp     byte ptr [bx+53h], 0
jnz     short loc_178CA
cmp     byte ptr [bx+54h], 0
jz      short loc_178E9

loc_178CA:
mov     al, _disk1._timeWeekday
cmp     [bx+55h], al
jnz     short loc_178E9
mov     al, _disk1._timeHours
cmp     [bx+56h], al
jnz     short loc_178E9

loc_178DA:
mov     bx, [bp+var_A]
inc     [bp+var_A]
shl     bx, 1
mov     ax, [bp+var_E]
mov     [bx-3240h], ax

loc_178E9:
inc     [bp+var_E]

loc_178EC:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_E], ax
jnb     short loc_178F9
jmp     loc_1786D

loc_178F9:
cmp     [bp+var_A], 0
jg      short loc_17902
jmp     loc_17AA6

loc_17902:
mov     ax, 1
push    ax
cmp     byte_2ED26, 0
jz      short loc_17912
mov     ax, offset _window7
jmp     short loc_17915

loc_17912:
mov     ax, offset _windowDialog

loc_17915:              ; win
push    ax
call    setWindow
add     sp, 4
mov     [bp+var_E], 0
jmp     loc_179B0
sub_17854 endp

; START OF FUNCTION CHUNK FOR sub_17970

loc_17924:
mov     byte ptr [bp-16h], 1

loc_17928:
mov     ax, 3
push    ax
sub     ax, ax
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp-14h], ax
mov     bx, ax
mov     al, [bx+0F5Ch]
sub     ah, ah
mov     si, ax
add     si, [bp-18h]
add     si, 24h ; '$'
cmp     byte ptr [si], 63h ; 'c'
jnb     short loc_17955
inc     byte ptr [si]
jmp     short loc_17955

loc_17951:
mov     byte ptr [bp-16h], 1

loc_17955:
cmp     byte ptr [bp-16h], 0
jz      short loc_1796C
push    word ptr [bp-18h] ; char *
mov     ax, offset aSSMutationsAre ; "%s's mutations are acting up again. Hea"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6

loc_1796C:
sub     ax, ax
push    ax
push    ax
; END OF FUNCTION CHUNK FOR sub_17970



sub_17970 proc near

; FUNCTION CHUNK AT 7924 SIZE 0000004C BYTES

mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_1797E:
call    getKeypress
cmp     al, 0FEh
jnz     short loc_1797E
mov     word_2D126, 0
mov     al, _disk1._timeWeekday
sub     ah, ah
inc     ax
inc     ax
sub     dx, dx
mov     cx, 7
div     cx
mov     bx, [bp-18h]
mov     [bx+55h], dl
mov     bx, [bp-18h]
mov     byte ptr [bx+21h], 0
mov     bx, [bp-18h]
mov     byte ptr [bx+53h], 0
inc     word ptr [bp-0Eh]

loc_179B0:
mov     ax, [bp-0Ah]
cmp     [bp-0Eh], ax
jl      short loc_179BB
jmp     loc_17A88

loc_179BB:
call    winClearCurrent
mov     bx, [bp-0Eh]
shl     bx, 1
mov     bx, [bx-3240h]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp-18h], ax
mov     byte ptr [bp-16h], 0
mov     bx, ax
cmp     byte ptr [bx+53h], 0
jnz     short loc_179DF
jmp     loc_17951

loc_179DF:
mov     al, [bx+53h]
dec     al
mov     [bp-10h], al
mov     al, 1
mov     cl, [bp-10h]
shl     al, cl
mov     [bp-8], al
mov     al, [bx+54h]
sub     ah, ah
mov     cl, [bp-8]
sub     ch, ch
test    ax, cx
jz      short loc_17A02
jmp     loc_17924

loc_17A02:
or      byte ptr [bx+58h], 80h
mov     bx, [bp-18h]
mov     al, [bp-8]
or      [bx+54h], al
mov     bl, [bp-10h]
sub     bh, bh
mov     al, [bx+0F56h]
mov     si, ax
add     si, [bp-18h]
add     si, 18h
cmp     byte ptr [si], 3
jb      short loc_17A28
sub     byte ptr [si], 2

loc_17A28:
mov     bx, [bp-18h]
cmp     byte ptr [bx+1Dh], 1
jbe     short loc_17A34
dec     byte ptr [bx+1Dh]

loc_17A34:
mov     al, [bp-10h]
sub     ah, ah
mov     si, ax
shl     si, 1
push    word ptr [si+0F4Ah]
mov     bx, [bp-18h]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+542h]
push    word ptr [si+0F3Eh]
push    word ptr [bp-18h] ; char *
mov     ax, offset aSSSS ; "%s %s %s %s.\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ch
mov     word_2F01C, 1
mov     al, [bp-10h]
sub     ah, ah
add     ax, 2Dh ; '-'
mov     word_2F020, ax
sub     ax, ax
push    ax
call    sub_18880
add     sp, 2
mov     word_2D102, 1C60h
jmp     loc_17928

loc_17A88:
sub     ax, ax
push    ax
push    word ptr [bp-0Ch] ; win
call    setWindow
add     sp, 4
cmp     byte_2ED26, 0
jz      short loc_17AA3
call    winClearCurrent
call    sub_1DC28
jmp     short loc_17AA6

loc_17AA3:
call    sub_1DD4C

loc_17AA6:
pop     si
mov     sp, bp
pop     bp
retn
sub_17970 endp ; sp-analysis failed



; Attributes: bp-based frame

sub_17AAB proc near

arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     cl, [bp+arg_2]
sub     ch, ch
cmp     cx, ax
jb      short loc_17B00
mov     ax, 6
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     bx, [bp+arg_0]
mov     [bx+53h], al
mov     al, _disk1._timeWeekday
sub     ah, ah
inc     ax
sub     dx, dx
mov     cx, 7
div     cx
mov     bx, [bp+arg_0]
mov     [bx+55h], dl
mov     ax, 0Bh
push    ax
mov     ax, 6
push    ax
call    sub_1DEB1
add     sp, 4
mov     bx, [bp+arg_0]
mov     [bx+56h], al

loc_17B00:
pop     bp
retn
sub_17AAB endp



; Attributes: bp-based frame

sub_17B02 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
push    si
mov     bx, [bp+arg_2]
cmp     byte ptr [bx+16h], 0
jz      short loc_17B3C
cmp     byte ptr [bx+17h], 7
jnz     short loc_17B26
mov     al, [bx+16h]
sub     ah, ah
push    ax
push    [bp+arg_0]
call    sub_17AAB
add     sp, 4
jmp     short loc_17B3C

loc_17B26:
cmp     [bp+arg_4], 0
jz      short loc_17B3C
mov     bx, [bp+arg_0]
mov     al, 1
mov     si, [bp+arg_2]
mov     cl, [si+17h]
shl     al, cl
or      [bx+58h], al

loc_17B3C:
pop     si
pop     bp
retn
sub_17B02 endp



; Attributes: bp-based frame

sub_17B3F proc near

var_16= word ptr -16h
var_14= dword ptr -14h
var_10= dword ptr -10h
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= dword ptr -8
win= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 16h
push    di
push    si
mov     word ptr [bp+var_14], 0
mov     word ptr [bp+var_14+2], 0
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     word ptr [bp+var_10+2], 0
mov     [bp+var_C], 0
jmp     short loc_17B97

loc_17B63:
mov     bx, word ptr [bp+var_10+2]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_10], ax
mov     bx, ax
mov     si, ax
mov     ax, [si+5Eh]
mov     dx, [si+60h]
cmp     [bx+5Ch], dx
jb      short loc_17B94
ja      short loc_17B85
cmp     [bx+5Ah], ax
jb      short loc_17B94

loc_17B85:
mov     bx, [bp+var_C]
inc     [bp+var_C]
shl     bx, 1
mov     ax, word ptr [bp+var_10+2]
mov     [bx-3240h], ax

loc_17B94:
inc     word ptr [bp+var_10+2]

loc_17B97:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     word ptr [bp+var_10+2], ax
jb      short loc_17B63
cmp     [bp+var_C], 0
jg      short loc_17BAA
jmp     loc_17D95

loc_17BAA:
mov     ax, 1
push    ax
cmp     byte_2ED26, 0
jz      short loc_17BBA
mov     ax, offset _window7
jmp     short loc_17BBD

loc_17BBA:
mov     ax, offset _windowDialog

loc_17BBD:              ; win
push    ax
call    setWindow
add     sp, 4
mov     word ptr [bp+var_10+2], 0
jmp     loc_17CA2

loc_17BCC:
mov     bx, word ptr [bp+var_10]
mov     si, bx
mov     si, [si+4Eh]
and     si, 0FFh
shl     si, 1
shl     si, 1
mov     ax, [si+0F06h]
mov     dx, [si+0F08h]
mov     [bx+5Eh], ax
mov     [bx+60h], dx
jmp     loc_17CDD

loc_17BED:
mov     bx, word ptr [bp+var_10]
mov     al, [bx+4Eh]
sub     ah, ah
push    ax
push    [bp+var_16]
push    bx              ; char *
mov     ax, offset aSYouHaveGained ; "%s, you have gained %d hit points and y"...
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2
cmp     word ptr [bp+var_14], 0
jg      short loc_17C1B
jmp     loc_17C9F

loc_17C1B:
jmp     short loc_17C77

loc_17C1D:              ; char *
push    word ptr [bp+var_14]
mov     ax, offset aDistributeDPoi ; "Distribute %d points:"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
sub     ax, ax
push    ax
mov     ax, 152Eh
push    ax
mov     ax, offset sprintf_buffer
push    ax
lea     ax, [bp+var_14+2]
push    ax
mov     ax, 7
push    ax
push    word ptr [bp+var_10]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_A], ax
or      ax, ax
jl      short loc_17C77
mov     si, ax
add     si, word ptr [bp+var_10]
add     si, 18h
mov     di, ax
les     bx, [bp+var_8]
mov     al, [si]
cmp     es:[bx+di+2Ch], al
jbe     short loc_17C6D
inc     byte ptr [si]
dec     word ptr [bp+var_14]
jmp     short loc_17C77

loc_17C6D:
mov     ax, offset aYouHaveAttaine ; "You have attained the highest level for"...
push    ax
call    sub_1B29B
add     sp, 2

loc_17C77:
cmp     word ptr [bp+var_14], 0
jg      short loc_17C1D
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 152Eh
push    ax
mov     ax, 0FF7h
push    ax
mov     ax, 7
push    ax
push    word ptr [bp+var_14+2]
push    word ptr [bp+var_10]
call    sub_115CB
add     sp, 0Eh
call    getKeypress

loc_17C9F:
inc     word ptr [bp+var_10+2]

loc_17CA2:
mov     ax, [bp+var_C]
cmp     word ptr [bp+var_10+2], ax
jl      short loc_17CAD
jmp     loc_17D77

loc_17CAD:
mov     bx, word ptr [bp+var_10+2]
shl     bx, 1
mov     bx, [bx-3240h]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_10], ax
mov     bx, ax
mov     al, [bx+48h]
sub     ah, ah
mov     cl, 7
shl     ax, cl
add     ax, word ptr _professions
mov     dx, word ptr _professions+2
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
mov     [bp+var_16], 0

loc_17CDD:
mov     bx, word ptr [bp+var_10]
mov     si, bx
mov     ax, [si+5Eh]
mov     dx, [si+60h]
cmp     [bx+5Ch], dx
jnb     short loc_17CF0
jmp     loc_17BED

loc_17CF0:
ja      short loc_17CFA
cmp     [bx+5Ah], ax
jnb     short loc_17CFA
jmp     loc_17BED

loc_17CFA:
cmp     byte ptr [bx+4Eh], 63h ; 'c'
jnb     short loc_17D49
inc     byte ptr [bx+4Eh]
les     bx, [bp+var_8]
mov     al, es:[bx+7Dh]
sub     ah, ah
push    ax
mov     al, es:[bx+7Ch]
push    ax
call    sub_1DEB1
add     sp, 4
mov     word ptr [bp+win+2], ax
mov     bx, word ptr [bp+var_10]
mov     ax, [bx+46h]
add     ax, word ptr [bp+win+2]
les     bx, [bp+var_8]
cmp     ax, es:[bx+74h]
jbe     short loc_17D3A
mov     ax, es:[bx+74h]
mov     bx, word ptr [bp+var_10]
sub     ax, [bx+46h]
mov     word ptr [bp+win+2], ax

loc_17D3A:
mov     bx, word ptr [bp+var_10]
mov     ax, word ptr [bp+win+2]
add     [bx+46h], ax
mov     ax, word ptr [bp+win+2]
add     [bp+var_16], ax

loc_17D49:
add     word ptr [bp+var_14], 2
mov     bx, word ptr [bp+var_10]
mov     si, bx
mov     ax, [si+5Eh]
mov     dx, [si+60h]
sub     [bx+5Ah], ax
sbb     [bx+5Ch], dx
mov     bx, word ptr [bp+var_10]
cmp     byte ptr [bx+4Eh], 0Dh
ja      short loc_17D6A
jmp     loc_17BCC

loc_17D6A:
mov     word ptr [bx+5Eh], 0D40h
mov     word ptr [bx+60h], 3
jmp     loc_17CDD

loc_17D77:
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
cmp     byte_2ED26, 0
jz      short loc_17D92
call    winClearCurrent
call    sub_1DC28
jmp     short loc_17D95

loc_17D92:
call    sub_1DD4C

loc_17D95:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_17B3F endp



; Attributes: bp-based frame

; int __cdecl getArmorClass(PartyMember near *member)
getArmorClass proc near

var_6= byte ptr -6
idx= word ptr -4
var_2= word ptr -2
member= word ptr  4

push    bp
mov     bp, sp
sub     sp, 6
push    si
mov     [bp+idx], 0
mov     [bp+var_2], 0
jmp     short loc_17DDB

loc_17DAE:
mov     si, [bp+idx]
mov     bx, [bp+member]
mov     al, [bx+si+PartyMember._array2]
mov     [bp+var_6], al
cmp     al, 0FFh
jz      short loc_17DD8
mov     al, 6
mul     [bp+var_6]
mov     si, ax
mov     al, 24
mul     [bx+si+PartyMember._array1.field_0]
mov     si, ax
mov     bx, _globals
mov     al, [bx+si+3Fh]
sub     ah, ah
add     [bp+var_2], ax

loc_17DD8:
inc     [bp+idx]

loc_17DDB:
cmp     [bp+idx], 3
jl      short loc_17DAE
cmp     [bp+var_2], 0
jnz     short loc_17E01
mov     bx, [bp+member]
mov     si, word ptr [bx+PartyMember._profession]
and     si, 0FFh
mov     cl, 7
shl     si, cl
les     bx, _professions
mov     al, es:[bx+si+Profession.field_7A]
sub     ah, ah
jmp     short loc_17E04

loc_17E01:
mov     ax, [bp+var_2]

loc_17E04:
pop     si
mov     sp, bp
pop     bp
retn
getArmorClass endp




sub_17E09 proc near
les     bx, dword_22C6E
or      byte ptr es:[bx+5], 80h
retn
sub_17E09 endp



; Attributes: bp-based frame

sub_17E13 proc near

arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
push    si
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     si, ax
mov     al, [bp+arg_0]
cbw
add     si, ax
mov     cl, 3
shl     si, cl
or      byte ptr es:[bx+si+416h], 80h
pop     si
pop     bp
retn
sub_17E13 endp



; Attributes: bp-based frame

sub_17E37 proc near

arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
push    si
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     si, ax
mov     al, [bp+arg_0]
cbw
add     si, ax
mov     cl, 3
shl     si, cl
and     byte ptr es:[bx+si+416h], 7Fh
pop     si
pop     bp
retn
sub_17E37 endp

push    bp
mov     bp, sp
sub     sp, 2Eh
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     ax, 6
mul     word ptr [bp-2]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     [bp-2Eh], ax
mov     [bp-2Ch], dx
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
mov     ax, 0FFFFh
push    ax
les     bx, [bp-2Eh]
mov     al, 18h
mul     byte ptr es:[bx+4]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    word ptr es:[bx]
push    word ptr [bp+4]
cmp     byte ptr es:[bx+4], 1
jnz     short loc_17EAE
mov     ax, offset a1d51d1616s ; "%1d>%5.1d %-16.16s"
jmp     short loc_17EB1

loc_17EAE:
mov     ax, offset a1d31d1818s ; "%1d>%3.1d %-18.18s"

loc_17EB1:
push    ax
lea     ax, [bp-2Ah]
push    ax
call    _sprintf
add     sp, 0Eh
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Ah]
push    ss
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_17ED0 proc near

var_8= dword ptr -8
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 8
mov     [bp+var_4], 0
mov     [bp+var_2], 0
jmp     short loc_17F1F

loc_17EE2:
mov     ax, 6
imul    [bp+var_4]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
les     bx, [bp+var_8]
mov     al, [bp+arg_0]
cmp     es:[bx+2], al
jnz     short loc_17F1C
mov     al, es:[bx+3]
and     al, 7Fh
cmp     al, [bp+arg_2]
jnz     short loc_17F1C
mov     bx, [bp+var_2]
shl     bx, 1
mov     ax, [bp+var_4]
mov     [bx-3240h], ax
inc     [bp+var_2]

loc_17F1C:
inc     [bp+var_4]

loc_17F1F:
les     bx, dword_22C6E
mov     ax, es:[bx+40Eh]
cmp     [bp+var_4], ax
jb      short loc_17EE2
mov     ax, [bp+var_2]
mov     sp, bp
pop     bp
retn
sub_17ED0 endp



; Attributes: bp-based frame

sub_17F34 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 8
les     bx, dword_22C6E
dec     word ptr es:[bx+40Eh]
les     bx, dword_22C6E
mov     ax, es:[bx+410h]
cmp     [bp+arg_0], ax
jnb     short loc_17F56
dec     word ptr es:[bx+410h]

loc_17F56:
mov     ax, 6
imul    [bp+arg_0]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     [bp+var_8], ax
mov     [bp+var_6], dx
push    dx
push    ax
add     ax, 6
push    dx
push    ax
call    sub_1858A
add     sp, 8
call    sub_17E09
mov     al, [bp+arg_4]
cbw
push    ax
mov     al, [bp+arg_2]
cbw
push    ax
call    sub_17ED0
add     sp, 4
mov     [bp+var_4], ax
or      ax, ax
jnz     short loc_17FA1
mov     al, [bp+arg_4]
cbw
push    ax
mov     al, [bp+arg_2]
cbw
push    ax
call    sub_17E37
add     sp, 4

loc_17FA1:
mov     ax, [bp+var_4]
mov     sp, bp
pop     bp
retn
sub_17F34 endp



; Attributes: bp-based frame

sub_17FA8 proc near

var_1A= dword ptr -1Ah
var_16= word ptr -16h
var_14= word ptr -14h
var_12= word ptr -12h
var_10= dword ptr -10h
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 1Ah
push    si              ; draw
mov     [bp+var_12], 0
mov     byte ptr [bp+var_10+2], 0
mov     [bp+var_4], 0
mov     al, [bp+arg_2]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [bp+arg_0]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
les     bx, dword_2E4C6
test    byte ptr es:[bx+2], 80h
jnz     short loc_17FF1
jmp     loc_18229

loc_17FF1:
mov     ax, 1
push    ax
call    sub_1BB93
add     sp, 2
mov     [bp+var_A], ax
mov     [bp+var_14], ax
mov     si, ax
shl     si, 1
mov     ax, [si-2518h]
mov     word ptr [bp+var_10], ax
mov     ax, 1
push    ax
push    word ptr [si+1ACEh]
call    sub_1E2B8
add     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _windowDialog
push    ax              ; win
call    setWindow
add     sp, 4
jmp     loc_181C3

loc_1802B:
cmp     byte ptr [bp+var_10+2], 0
jz      short loc_18034
jmp     loc_181DD

loc_18034:
mov     ax, [bp+var_A]
cmp     [bp+var_14], ax
jz      short loc_18073
mov     ax, 1
push    ax
mov     bx, [bp+var_14]
shl     bx, 1
push    word ptr [bx+1ACEh]
call    sub_1E2B8
add     sp, 4
mov     ax, 1
push    ax
mov     bx, [bp+var_A]
shl     bx, 1
push    word ptr [bx+1ACEh]
call    sub_1E2B8
add     sp, 4
mov     ax, [bp+var_A]
mov     [bp+var_14], ax
mov     bx, ax
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_10], ax

loc_18073:
sub     ax, ax
push    ax
mov     ax, 7E5Bh
push    ax
mov     ax, 1028h
push    ax
lea     ax, [bp+var_12]
push    ax
push    [bp+var_6]
push    word ptr [bp+var_10]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_8], ax
or      ax, ax
jge     short loc_18098
jmp     loc_1819E

loc_18098:
mov     bx, ax
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp+var_8], ax
mov     ax, 6
imul    [bp+var_8]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     word ptr [bp+var_1A], ax
mov     word ptr [bp+var_1A+2], dx
les     bx, [bp+var_1A]
cmp     byte ptr es:[bx+4], 1
jnz     short loc_180F2
mov     ax, es:[bx]
sub     dx, dx
add     word ptr _disk1._cash, ax
adc     word ptr _disk1._cash+2, dx

loc_180CE:
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
push    [bp+var_8]
call    sub_17F34
add     sp, 6
mov     [bp+var_6], ax
or      ax, ax
jz      short loc_180EB
jmp     loc_181C3

loc_180EB:
mov     byte ptr [bp+var_10+2], 1
jmp     loc_181C3

loc_180F2:
mov     ax, 6
imul    [bp+var_8]
mov     si, ax
les     bx, dword_2EC1A
mov     al, es:[bx+si+4]
sub     ah, ah
mov     [bp+var_16], ax
sub     ax, ax
push    ax
mov     ax, 18h
imul    [bp+var_16]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    word ptr [bp+var_10]
call    sub_17B02
add     sp, 6
push    [bp+var_16]
call    sub_16D6B
add     sp, 2
or      al, al
jz      short loc_1813A
push    [bp+var_16]
call    sub_16D96

loc_18134:
add     sp, 2
jmp     loc_181C3

loc_1813A:
mov     bx, word ptr [bp+var_10]
cmp     byte ptr [bx+52h], 20h ; ' '
jnb     short loc_18184
mov     ax, 6
imul    [bp+var_8]
mov     si, ax
les     bx, dword_2EC1A
mov     al, es:[bx+si+4]
sub     ah, ah
push    ax
mov     bx, word ptr [bp+var_10]
mov     al, 6
mul     byte ptr [bx+52h]
add     ax, bx
add     ax, 62h ; 'b'
push    ax
call    sub_1D6AA
add     sp, 4
mov     bx, word ptr [bp+var_10]
inc     byte ptr [bx+52h]
mov     ax, 6
imul    [bp+var_8]
mov     bx, ax
les     si, dword_2EC1A
dec     word ptr es:[bx+si]
jnz     short loc_181C3
jmp     loc_180CE

loc_18184:              ; char *
push    word ptr [bp+var_10]
mov     ax, offset aSHasTooMuchToC_0 ; "%s\rhas too much to carry\ralready."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
jmp     short loc_18134

loc_1819E:
cmp     [bp+var_8], 0FFFFh
jnz     short loc_181A7
jmp     loc_180EB

loc_181A7:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+var_A]
push    ax
mov     ax, [bp+var_8]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah

loc_181C3:
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
call    sub_17ED0
add     sp, 4
mov     [bp+var_6], ax
or      ax, ax
jle     short loc_181DD
jmp     loc_1802B

loc_181DD:
mov     ax, 1
push    ax
mov     bx, [bp+var_14]
shl     bx, 1
push    word ptr [bx+1ACEh]
call    sub_1E2B8
add     sp, 4
cmp     [bp+var_6], 0
jnz     short loc_181FA
mov     [bp+var_4], 1

loc_181FA:
sub     ax, ax
push    ax
mov     ax, offset _windowInfo
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_1DD4C
cmp     [bp+var_4], 0
jz      short loc_18229
sub     ax, ax
push    ax
mov     ax, 5
push    ax
mov     ax, word ptr dword_2E4C6
mov     dx, word ptr dword_2E4C6+2
add     ax, 6
push    dx
push    ax
call    sub_11834
add     sp, 8

loc_18229:
pop     si
mov     sp, bp
pop     bp
retn
sub_17FA8 endp



; Attributes: bp-based frame

sub_1822E proc near

var_8= dword ptr -8
var_4= byte ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= word ptr  6
arg_4= byte ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     [bp+var_4], 0
cmp     [bp+arg_2], 0
jz      short loc_182BC
mov     [bp+var_2], 0
jmp     short loc_18269

loc_18246:
mov     ax, 6
imul    [bp+var_2]
mov     bx, ax
les     si, dword_2EC1A
mov     word ptr es:[bx+si], 0FFFFh

loc_18257:
mov     bl, [bp+arg_0]
sub     bh, bh
shl     bx, 1
mov     word ptr [bx-3240h], 0
jmp     short loc_182BC

loc_18266:
inc     [bp+var_2]

loc_18269:
les     bx, dword_22C6E
mov     ax, es:[bx+40Eh]
cmp     [bp+var_2], ax
jnb     short loc_182C0
mov     ax, 6
imul    [bp+var_2]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     word ptr [bp+var_8], ax
mov     word ptr [bp+var_8+2], dx
les     bx, [bp+var_8]
mov     al, [bp+arg_0]
cmp     es:[bx+4], al
jnz     short loc_18266
mov     al, [bp+arg_4]
cmp     es:[bx+2], al
jnz     short loc_18266
mov     al, [bp+arg_6]
cmp     es:[bx+3], al
jnz     short loc_18266
mov     ax, es:[bx]
add     ax, [bp+arg_2]
cmp     ax, 0FFFFh
ja      short loc_18246
mov     ax, [bp+arg_2]
add     es:[bx], ax
jmp     short loc_18257

loc_182BC:
mov     [bp+var_4], 1

loc_182C0:
mov     al, [bp+var_4]
sub     ah, ah
pop     si
mov     sp, bp
pop     bp
retn
sub_1822E endp



; Attributes: bp-based frame

sub_182CA proc near

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 0Ch
push    di
push    si
mov     si, [bp+arg_0]
shl     si, 1
mov     bx, _packetsSize
mov     ax, [bx+si+30h]
mov     [bp+var_6], ax
mov     [bp+var_A], 0

loc_182E6:
mov     bx, [bp+var_A]
shl     bx, 1
mov     word ptr [bx-3240h], 0
inc     [bp+var_A]
cmp     [bp+var_A], 100h
jl      short loc_182E6
mov     ax, [bp+var_6]
mov     [bp+var_A], ax
mov     [bp+var_8], 0
jmp     loc_18390

loc_18309:
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
mov     cx, ax
mov     ax, 6
imul    [bp+var_A]
mov     di, ax
mov     bx, _packetsSize
mov     al, [bx+di+61h]
sub     ah, ah
cmp     ax, cx
jb      short loc_1838D
mov     ax, 6
imul    [bp+var_A]
mov     si, ax
add     si, bx
push    word ptr [si+64h]
push    word ptr [si+62h]
call    sub_1DEB1
add     sp, 4
mov     [bp+var_4], ax
mov     al, [bp+arg_4]
cbw
push    ax
mov     al, [bp+arg_2]
cbw
push    ax
push    [bp+var_4]
mov     ax, 6
imul    [bp+var_A]
mov     di, ax
mov     bx, _packetsSize
mov     al, [bx+di+60h]
sub     ah, ah
push    ax
call    sub_1822E
add     sp, 8
or      al, al
jnz     short loc_1838D
mov     ax, 6
imul    [bp+var_A]
mov     di, ax
mov     bx, _packetsSize
mov     bl, [bx+di+60h]
sub     bh, bh
shl     bx, 1
mov     ax, [bp+var_4]
mov     [bx-3240h], ax
inc     [bp+var_8]

loc_1838D:
inc     [bp+var_A]

loc_18390:
mov     bx, [bp+arg_0]
shl     bx, 1
mov     di, _packetsSize
mov     ax, [bx+di]
add     ax, [bp+var_6]
cmp     ax, [bp+var_A]
jle     short loc_183A6
jmp     loc_18309

loc_183A6:
cmp     [bp+var_8], 0
jg      short loc_183AF
jmp     loc_184A2

loc_183AF:
mov     ax, [bp+var_8]
les     bx, dword_22C6E
sub     ax, es:[bx+412h]
add     ax, es:[bx+40Eh]
mov     [bp+var_2], ax
or      ax, ax
jle     short loc_18402
mov     ax, 6
mul     word ptr es:[bx+410h]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
push    dx
push    ax
mov     ax, [bp+var_2]
add     ax, es:[bx+410h]
mov     cx, 6
mul     cx
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
push    dx
push    ax
call    sub_1858A
add     sp, 8
les     bx, dword_22C6E
mov     ax, [bp+var_2]
sub     es:[bx+40Eh], ax

loc_18402:
mov     [bp+var_A], 0

loc_18407:
mov     bx, [bp+var_A]
shl     bx, 1
cmp     word ptr [bx-3240h], 0
jz      short loc_18485
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+40Eh]
mov     si, ax
les     bx, dword_2EC1A
mov     al, [bp+arg_2]
mov     es:[bx+si+2], al
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+40Eh]
mov     si, ax
les     bx, dword_2EC1A
mov     al, [bp+arg_4]
mov     es:[bx+si+3], al
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+40Eh]
mov     si, ax
les     bx, dword_2EC1A
mov     al, byte ptr [bp+var_A]
mov     es:[bx+si+4], al
mov     bx, [bp+var_A]
shl     bx, 1
mov     ax, [bx-3240h]
les     bx, dword_22C6E
mov     cx, ax
mov     ax, es:[bx+40Eh]
inc     word ptr es:[bx+40Eh]
mov     bx, 6
mul     bx
mov     bx, ax
les     si, dword_2EC1A
mov     es:[bx+si], cx

loc_18485:
inc     [bp+var_A]
cmp     [bp+var_A], 100h
jge     short loc_18492
jmp     loc_18407

loc_18492:
mov     al, [bp+arg_4]
cbw
push    ax
mov     al, [bp+arg_2]
cbw
push    ax
call    sub_17E13
add     sp, 4

loc_184A2:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_182CA endp



; Attributes: bp-based frame

sub_184A8 proc near

var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 6
push    si
mov     [bp+var_6], 0
mov     [bp+var_4], 0
mov     [bp+var_6], 0
jmp     short loc_184E6

loc_184C0:
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
mov     bx, [bp+var_4]
shl     bx, 1
push    word ptr [bx-3240h]
push    [bp+var_4]
call    sub_1822E
add     sp, 8
or      al, al
jnz     short loc_184E3
inc     [bp+var_6]

loc_184E3:
inc     [bp+var_4]

loc_184E6:
cmp     [bp+var_4], 100h
jl      short loc_184C0
mov     [bp+var_4], 0

loc_184F2:
mov     bx, [bp+var_4]
shl     bx, 1
cmp     word ptr [bx-3240h], 0
jz      short loc_18562
les     bx, dword_22C6E
mov     ax, es:[bx+410h]
inc     word ptr es:[bx+410h]
mov     [bp+var_2], ax
mov     ax, 6
imul    [bp+var_2]
mov     si, ax
les     bx, dword_2EC1A
mov     al, [bp+arg_0]
mov     es:[bx+si+2], al
mov     ax, 6
imul    [bp+var_2]
mov     si, ax
les     bx, dword_2EC1A
mov     al, [bp+arg_2]
mov     es:[bx+si+3], al
mov     ax, 6
imul    [bp+var_2]
mov     si, ax
les     bx, dword_2EC1A
mov     al, byte ptr [bp+var_4]
mov     es:[bx+si+4], al
mov     bx, [bp+var_4]
shl     bx, 1
mov     ax, [bx-3240h]
mov     cx, ax
mov     ax, 6
imul    [bp+var_2]
mov     bx, ax
les     si, dword_2EC1A
mov     es:[bx+si], cx

loc_18562:
inc     [bp+var_4]
cmp     [bp+var_4], 100h
jl      short loc_184F2
cmp     [bp+var_6], 0
jle     short loc_18582
mov     al, [bp+arg_2]
cbw
push    ax
mov     al, [bp+arg_0]
cbw
push    ax
call    sub_17E13
add     sp, 4

loc_18582:
mov     ax, [bp+var_6]
pop     si
mov     sp, bp
pop     bp
retn
sub_184A8 endp



; Attributes: bp-based frame

sub_1858A proc near

var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 0Ch
mov     al, byte_2132A
cbw
mov     bx, ax
shl     bx, 1
shl     bx, 1
mov     ax, [bx-2DF8h]
add     ax, word ptr dword_22C6E
mov     dx, word ptr dword_22C6E+2
mov     [bp+var_A], ax
mov     [bp+var_8], dx
mov     ax, [bp+arg_2]
mov     [bp+var_2], ax
mov     ax, [bp+arg_0]
mov     [bp+var_6], ax
mov     ax, [bp+arg_6]
mov     [bp+var_4], ax
mov     ax, [bp+arg_4]
mov     [bp+var_C], ax
mov     ax, [bp+var_A]
sub     ax, [bp+arg_0]
push    ax              ; unsigned int
push    [bp+var_C]      ; unsigned int
push    [bp+var_4]      ; unsigned int
push    [bp+var_6]      ; unsigned int
push    [bp+var_2]      ; unsigned int
call    _movedata
add     sp, 0Ah
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+410h]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     cx, ax
mov     ax, [bp+var_A]
sub     ax, cx
cwd
mov     cx, 6
idiv    cx
mov     es:[bx+412h], ax
mov     sp, bp
pop     bp
retn
sub_1858A endp



; Attributes: bp-based frame

sub_18607 proc near

var_2A= word ptr -2Ah
var_28= word ptr -28h
var_26= word ptr -26h
var_24= word ptr -24h
var_1C= word ptr -1Ch
var_1A= word ptr -1Ah
var_18= byte ptr -18h
var_16= byte ptr -16h
var_14= word ptr -14h
var_12= word ptr -12h
var_10= word ptr -10h
var_E= dword ptr -0Eh
var_A= byte ptr -0Ah
var_8= word ptr -8
var_6= dword ptr -6
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 2Ah
mov     ax, [bp+arg_0]
mov     dx, [bp+arg_2]
mov     [bp+var_26], ax
mov     [bp+var_24], dx
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+410h]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     [bp+var_12], ax
mov     [bp+var_10], dx
mov     [bp+var_1C], 0
mov     [bp+var_A], 0
cmp     [bp+arg_4], 1
jnz     short loc_186A8
dec     byte ptr es:[bx+9]
add     [bp+var_26], 68h ; 'h'
les     bx, dword_22C6E
sub     word ptr es:[bx+408h], 68h ; 'h'
les     bx, dword_22C6E
mov     ax, es:[bx+408h]
add     ax, bx
mov     dx, es
mov     word_2EC14, ax
mov     word_2EC16, dx
sub     word ptr es:[bx+40Ah], 68h ; 'h'
les     bx, dword_22C6E
mov     ax, es:[bx+40Ah]
add     ax, bx
mov     dx, es
mov     word ptr dword_2E41E, ax
mov     word ptr dword_2E41E+2, dx
mov     ax, [bp+arg_0]
mov     dx, [bp+arg_2]
mov     word ptr [bp+var_E], ax
mov     word ptr [bp+var_E+2], dx
add     ax, 52h ; 'R'
mov     word ptr [bp+var_6], ax
mov     word ptr [bp+var_6+2], dx
les     bx, [bp+var_E]
mov     al, es:[bx+5Bh]
mov     [bp+var_16], al
mov     al, es:[bx+5Ch]
mov     [bp+var_18], al

loc_186A8:
cmp     [bp+var_A], 0
jnz     short loc_18709
cmp     [bp+arg_6], 0
jnz     short loc_18709
mov     [bp+var_1A], 0

loc_186B9:
mov     bx, [bp+var_1A]
shl     bx, 1
mov     word ptr [bx-3240h], 0
inc     [bp+var_1A]
cmp     [bp+var_1A], 100h
jl      short loc_186B9
les     bx, [bp+var_E]
mov     ax, es:[bx+14h]
mov     word_2E002, ax
les     bx, [bp+var_6]
inc     word ptr [bp+var_6]
mov     al, es:[bx]
cbw
mov     [bp+var_14], ax
mov     [bp+var_1A], 0
jmp     short loc_18701

loc_186EC:
les     bx, [bp+var_6]
inc     word ptr [bp+var_6]
mov     al, es:[bx]
cbw
mov     bx, ax
shl     bx, 1
inc     word ptr [bx-3240h]
inc     [bp+var_1A]

loc_18701:
mov     ax, [bp+var_14]
cmp     [bp+var_1A], ax
jl      short loc_186EC

loc_18709:
mov     ax, [bp+var_24]
mov     [bp+var_2], ax
mov     ax, [bp+var_26]
mov     [bp+var_28], ax
mov     ax, [bp+arg_2]
mov     [bp+var_8], ax
mov     ax, [bp+arg_0]
mov     [bp+var_2A], ax
cmp     [bp+var_A], 0
jz      short loc_1872A
jmp     loc_187AF

loc_1872A:
cmp     [bp+arg_6], 0
jnz     short loc_187AF
mov     ax, [bp+var_12]
sub     ax, [bp+var_26]
push    ax              ; unsigned int
push    [bp+var_2A]     ; unsigned int
push    [bp+var_8]      ; unsigned int
push    [bp+var_28]     ; unsigned int
push    [bp+var_2]      ; unsigned int
call    _movedata
add     sp, 0Ah
les     bx, dword_22C6E
mov     ax, [bp+var_26]
sub     ax, [bp+arg_0]
sub     es:[bx+40Ch], ax
les     bx, dword_22C6E
mov     ax, es:[bx+40Ch]
add     ax, bx
mov     dx, es
mov     word ptr dword_2EC1A, ax
mov     word ptr dword_2EC1A+2, dx
cmp     [bp+var_14], 0
jle     short loc_18785
mov     al, [bp+var_18]
cbw
push    ax
mov     al, [bp+var_16]
cbw
push    ax
call    sub_184A8
add     sp, 4
mov     [bp+var_1C], ax

loc_18785:
les     bx, dword_22C6E
mov     ax, [bp+var_1C]
add     es:[bx+40Eh], ax
les     bx, dword_22C6E
mov     ax, 6
mul     word ptr es:[bx+410h]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
push    dx
push    ax
push    [bp+var_10]
push    [bp+var_12]
jmp     short loc_187DE

loc_187AF:
les     bx, dword_22C6E
mov     ax, [bp+var_26]
sub     ax, [bp+arg_0]
sub     es:[bx+40Ch], ax
les     bx, dword_22C6E
mov     ax, es:[bx+40Ch]
add     ax, bx
mov     dx, es
mov     word ptr dword_2EC1A, ax
mov     word ptr dword_2EC1A+2, dx
push    [bp+arg_2]
push    [bp+arg_0]
push    [bp+var_24]
push    [bp+var_26]

loc_187DE:
call    sub_1858A
mov     sp, bp
pop     bp
retn
sub_18607 endp



; Attributes: bp-based frame

sub_187E5 proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 2
cmp     [bp+arg_0], 0
jz      short loc_18823
les     bx, dword_22C6E
test    word ptr es:[bx+4], 8000h
jz      short loc_18823
and     byte ptr es:[bx+5], 7Fh
mov     ax, 1
push    ax
mov     ax, 0D20Ah
push    ax
mov     al, [bp+arg_0]
sub     ah, ah
dec     ax
push    ax
push    word ptr dword_22C6E+2
push    word ptr dword_22C6E
push    word_2E4C4
call    sub_103BF
add     sp, 0Ch

loc_18823:
mov     sp, bp
pop     bp
retn
sub_187E5 endp



; Attributes: bp-based frame

sub_18827 proc near

var_6= word ptr -6
var_4= word ptr -4
handle= word ptr -2

push    bp
mov     bp, sp
sub     sp, 6
mov     ax, 0BEEAh
mov     [bp+var_6], ax
mov     [bp+var_4], ds
mov     ax, word_22C86
mov     _disk1.field_38, ax
mov     ax, word_2132E
mov     dx, word_21330
add     word_2132E, 1
adc     word_21330, 0
mov     _disk1.field_0, ax
mov     _disk1.field_2, dx
push    word_2DFEC
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
push    [bp+var_4]
push    [bp+var_6]
mov     ax, 0EC0h
cwd
push    dx
push    ax
push    [bp+handle]
call    sub_1E448
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
mov     sp, bp
pop     bp
retn
sub_18827 endp



; Attributes: bp-based frame

sub_18880 proc near

var_1E= dword ptr -1Eh
var_1A= word ptr -1Ah
var_14= word ptr -14h
var_12= word ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= byte ptr -0Ch
var_8= byte ptr -8
var_4= word ptr -4
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 1Eh
push    di
push    si
mov     [bp+var_4], 0
mov     word_2D126, 0
cmp     [bp+arg_0], 0
jnz     short loc_1889E
inc     [bp+var_4]
jmp     short loc_188DD

loc_1889E:
cmp     word_2F01C, 2
jnz     short loc_188A8
inc     [bp+var_4]

loc_188A8:
mov     ax, word_2F01C
or      ax, ax
jz      short loc_188BB
cmp     ax, 1
jz      short loc_188CB
cmp     ax, 2
jz      short loc_188D1
jmp     short loc_188D7

loc_188BB:
mov     word_2F020, 34h ; '4'
mov     ax, 1060h
mov     word ptr dword_2DFF0, ax
mov     word ptr dword_2DFF0+2, ds

loc_188CB:
mov     word_2F022, 33h ; '3'

loc_188D1:
mov     word_2F024, 33h ; '3'

loc_188D7:
mov     word_2F01C, 3

loc_188DD:
mov     [bp+var_E], 0

loc_188E2:
mov     si, [bp+var_E]
mov     [bp+si+var_C], 0
mov     si, [bp+var_E]
mov     [bp+si+var_8], 1
inc     [bp+var_E]
cmp     [bp+var_E], 3
jl      short loc_188E2
mov     [bp+var_E], 0
jmp     short loc_18934

loc_18900:
inc     [bp+var_12]

loc_18903:
cmp     [bp+var_12], 3
jge     short loc_18931
mov     si, [bp+var_E]
shl     si, 1
mov     bx, [bp+var_12]
shl     bx, 1
mov     ax, [si-2220h]
cmp     [bx-2438h], ax
jnz     short loc_18900
mov     ax, [bp+var_12]
mov     [bp+si+var_1A], ax
mov     di, [bp+var_12]
mov     [bp+di+var_8], 0
mov     di, [bp+var_E]
mov     [bp+di+var_C], 1

loc_18931:
inc     [bp+var_E]

loc_18934:
mov     ax, word_2F01C
cmp     [bp+var_E], ax
jge     short loc_18943
mov     [bp+var_12], 0
jmp     short loc_18903

loc_18943:
mov     [bp+var_E], 0
jmp     loc_189F8

loc_1894B:
mov     [bp+var_10], 0
jmp     short loc_18955

loc_18952:
inc     [bp+var_10]

loc_18955:
cmp     [bp+var_10], 3
jge     short loc_189B0
mov     si, [bp+var_10]
cmp     [bp+si+var_8], 0
jz      short loc_18952
sub     ax, ax
push    ax
mov     ax, 0DAF2h
push    ax
push    [bp+var_14]
push    word_22C48
push    word_22C46
push    word_2EC08
call    sub_103BF
add     sp, 0Ch
mov     bx, [bp+var_10]
shl     bx, 1
shl     bx, 1
push    word ptr [bx+1A0Ch]
push    word ptr [bx+1A0Ah]
push    word_22C48
push    word_22C46
call    sub_1E5BD
add     sp, 8
mov     bx, [bp+var_10]
shl     bx, 1
mov     ax, [bp+var_14]
mov     [bx-2438h], ax
mov     si, [bp+var_10]
mov     [bp+si+var_8], 0

loc_189B0:
mov     bx, [bp+var_10]
shl     bx, 1
shl     bx, 1
mov     ax, [bx+1A0Ah]
mov     dx, [bx+1A0Ch]
mov     word ptr [bp+var_1E], ax
mov     word ptr [bp+var_1E+2], dx
mov     ax, 0Ch
imul    [bp+var_E]
mov     si, ax
mov     ax, word ptr [bp+var_1E]
mov     dx, word ptr [bp+var_1E+2]
mov     [si-413Ah], ax
mov     [si-4138h], dx
les     bx, [bp+var_1E]
mov     ah, es:[bx+2]
sub     al, al
mov     cl, es:[bx+1]
sub     ch, ch
add     ax, cx
mov     [si-413Ch], ax
mov     byte ptr [si-4134h], 1
inc     [bp+var_E]

loc_189F8:
mov     ax, word_2F01C
cmp     [bp+var_E], ax
jge     short loc_18A39
mov     si, [bp+var_E]
shl     si, 1
mov     ax, [si-2220h]
mov     [bp+var_14], ax
mov     ax, 0Ch
imul    [bp+var_E]
mov     bx, ax
mov     di, [bp+var_E]
add     di, [bp+var_4]
shl     di, 1
mov     ax, [di+1E74h]
mov     [bx-413Eh], ax
mov     di, [bp+var_E]
cmp     [bp+di+var_C], 0
jnz     short loc_18A30
jmp     loc_1894B

loc_18A30:
mov     ax, [bp+si+var_1A]
mov     [bp+var_10], ax
jmp     loc_189B0

loc_18A39:
cmp     [bp+arg_0], 0
jz      short loc_18A44
mov     ax, 3
jmp     short loc_18A47

loc_18A44:
mov     ax, 1

loc_18A47:
mov     word_2D126, ax
mov     word_2E424, 0
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_18880 endp



; Attributes: bp-based frame

sub_18A56 proc near

handle= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4

push    bp
mov     bp, sp
sub     sp, 6
cmp     [bp+arg_0], 4
jb      short loc_18A65
jmp     loc_18B03

loc_18A65:
mov     word_2EC12, 0

loc_18A6B:
mov     bx, word_2EC12
shl     bx, 1
mov     word ptr [bx-2438h], 0FFFFh
inc     word_2EC12
cmp     word_2EC12, 3
jl      short loc_18A6B
mov     bl, [bp+arg_0]
sub     bh, bh
shl     bx, 1
push    word ptr [bx+0EFEh]
call    sub_169A3
add     sp, 2
mov     [bp+handle], ax
push    ax              ; handle
call    fileGetSize
add     sp, 2
mov     [bp+var_4], ax
mov     [bp+var_2], dx
push    word ptr dst+2
push    word ptr dst    ; dst
push    dx
push    ax              ; size
push    [bp+handle]     ; handle
call    fileRead
add     sp, 0Ah
push    [bp+handle]     ; handle
call    fileClose
add     sp, 2
push    word ptr _picDecoded_0+2
push    word ptr _picDecoded_0
push    word ptr dst+2
push    word ptr dst
call    sub_1E5BD
add     sp, 8
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
call    getKeypress
cmp     [bp+arg_0], 3
jnz     short loc_18B00
mov     byte_2ED26, 0
mov     word_2D126, 0
mov     ax, offset aSeeYouLater ; "\r\r\r    See you later...!"
push    ax
call    sub_1B29B
add     sp, 2
call    error

loc_18B00:
call    sub_1B452

loc_18B03:
mov     sp, bp
pop     bp
retn
sub_18A56 endp

align 2


; Attributes: bp-based frame

sub_18B08 proc near

var_A= dword ptr -0Ah
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ah
sub     ax, ax
mov     [bp+var_2], ax
mov     [bp+var_4], ax
mov     [bp+var_6], ax
jmp     short loc_18B1E

loc_18B1B:
inc     [bp+var_6]

loc_18B1E:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_6]
jbe     short loc_18B5F
mov     ax, 68h ; 'h'
imul    [bp+var_6]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_A], ax
mov     word ptr [bp+var_A+2], dx
les     bx, [bp+var_A]
mov     al, es:[bx+49h]
sub     ah, ah
cmp     ax, [bp+arg_0]
jnz     short loc_18B1B
mov     ax, bx
mov     [bp+var_4], ax
mov     [bp+var_2], dx
mov     bx, [bp+arg_2]
mov     ax, [bp+var_6]
mov     [bx], ax

loc_18B5F:
mov     ax, [bp+var_4]
mov     dx, [bp+var_2]
mov     sp, bp
pop     bp
retn
sub_18B08 endp



; Attributes: bp-based frame

sub_18B69 proc near

arg_0= dword ptr  4
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
les     bx, [bp+arg_0]
mov     al, byte ptr [bp+arg_4]
and     al, 7Fh
mov     es:[bx+3], al
mov     ax, [bp+arg_4]
and     ax, 80h
mov     cl, 7
shr     ax, cl
les     bx, [bp+arg_0]
mov     cl, es:[bx+2]
and     cl, 0FEh
or      al, cl
mov     es:[bx+2], al
les     bx, [bp+arg_0]
mov     ax, [bp+arg_6]
and     ah, 7Fh
mov     es:[bx+4], ax
les     bx, [bp+arg_0]
mov     al, es:[bx+2]
and     al, 0FDh
mov     dx, [bp+arg_6]
and     dx, 8000h
mov     cl, 0Eh
shr     dx, cl
or      al, dl
mov     es:[bx+2], al
pop     bp
retn
sub_18B69 endp



; Attributes: bp-based frame

sub_18BBB proc near

arg_0= dword ptr  4
arg_4= byte ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch

push    bp
mov     bp, sp
push    si
mov     al, [bp+arg_4]
sub     ah, ah
cmp     ax, 19h
jz      short loc_18BE4
cmp     ax, 1Ah
jz      short loc_18C0E
cmp     ax, 1Bh
jz      short loc_18C22
cmp     ax, 1Ch
jz      short loc_18BE4
cmp     ax, 1Dh
jz      short loc_18C0E
cmp     ax, 1Eh
jz      short loc_18C22
jmp     short loc_18C2C

loc_18BE4:
les     bx, [bp+arg_0]
mov     si, es:[bx]
mov     ax, si
and     ah, 7
cmp     [bp+arg_6], ax
jz      short loc_18C2C
mov     ax, [bp+arg_6]
mov     cx, si
and     cx, 0F800h
or      ax, cx
mov     es:[bx], ax
cmp     byte_2ED26, 0
jnz     short loc_18C2C
call    sub_1DD4C
jmp     short loc_18C2C

loc_18C0E:
push    [bp+arg_8]
push    [bp+arg_6]
push    word ptr [bp+arg_0+2]
push    word ptr [bp+arg_0]
call    sub_18B69
add     sp, 8
jmp     short loc_18C2C

loc_18C22:
les     bx, [bp+arg_0]
mov     ax, [bp+arg_6]
mov     es:[bx+6], ax

loc_18C2C:
call    sub_17E09
pop     si
pop     bp
retn
sub_18BBB endp



; Attributes: bp-based frame

sub_18C32 proc near

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= byte ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8
arg_6= byte ptr  0Ah
arg_8= word ptr  0Ch
arg_A= word ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, [bp+arg_2]
cmp     [bx-4104h], al
jnz     short loc_18C87
mov     al, [bp+arg_6]
cbw
les     bx, dword_22C6E
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [bp+arg_4]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     [bp+var_4], ax
mov     [bp+var_2], dx
push    [bp+arg_A]
push    [bp+arg_8]
mov     al, [bp+arg_0]
sub     ah, ah
push    ax
push    dx
push    [bp+var_4]
call    sub_18BBB
add     sp, 0Ah
jmp     loc_18E1E

loc_18C87:
mov     bx, word_21326
cmp     word ptr [bx], 96h
jnz     short loc_18CAA
mov     ax, 5
push    ax
call    sub_21130
add     sp, 2
mov     ax, offset aFlushingNeeded ; "Flushing Needed!!"
push    ax
call    sub_1B29B
add     sp, 2
jmp     loc_18E1E

loc_18CAA:
mov     [bp+var_A], 0
mov     [bp+var_6], 0
jmp     short loc_18CB8

loc_18CB5:
inc     [bp+var_A]

loc_18CB8:
mov     bx, word_21326
mov     ax, [bx]
cmp     [bp+var_A], ax
jge     short loc_18CEF
mov     ax, 0Ch
imul    [bp+var_A]
mov     si, ax
add     si, bx
mov     al, [bp+arg_2]
cmp     [si+2], al
jnz     short loc_18CB5
mov     al, [bp+arg_4]
cmp     [si+3], al
jnz     short loc_18CB5
mov     al, [bp+arg_6]
cmp     [si+4], al
jnz     short loc_18CB5
mov     [bp+var_6], 1
mov     ax, [bp+var_A]
mov     [bp+var_8], ax

loc_18CEF:
cmp     [bp+var_6], 0
jz      short loc_18CF8
jmp     loc_18D87

loc_18CF8:
mov     ax, [bx]
inc     word ptr [bx]
mov     [bp+var_8], ax
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     al, [bp+arg_2]
mov     [bx+si+2], al
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     al, [bp+arg_4]
mov     [bx+si+3], al
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     al, [bp+arg_6]
mov     [bx+si+4], al
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     byte ptr [bx+si+5], 0
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     byte ptr [bx+si+8], 0
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     word ptr [bx+si+6], 0
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     byte ptr [bx+si+9], 0
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     word ptr [bx+si+0Ah], 0

loc_18D87:
mov     al, [bp+arg_0]
sub     ah, ah
jmp     short loc_18E0C

loc_18D8E:
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
or      byte ptr [bx+si+5], 80h
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     ax, [bp+arg_8]
and     ah, 7
mov     [bx+si+6], ax
jmp     short loc_18E1E

loc_18DB5:
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
or      byte ptr [bx+si+5], 40h
mov     ax, 0Ch
imul    [bp+var_8]
add     ax, word_21326
add     ax, 6
mov     [bp+var_4], ax
mov     [bp+var_2], ds
push    [bp+arg_A]
push    [bp+arg_8]
push    ds
push    ax
call    sub_18B69
add     sp, 8
jmp     short loc_18E1E

loc_18DE8:
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
or      byte ptr [bx+si+5], 20h
mov     ax, 0Ch
imul    [bp+var_8]
mov     si, ax
mov     bx, word_21326
mov     ax, [bp+arg_8]
mov     [bx+si+0Ch], ax
jmp     short loc_18E1E

loc_18E0C:
cmp     ax, 1Ch
jnz     short loc_18E14
jmp     loc_18D8E

loc_18E14:
cmp     ax, 1Dh
jz      short loc_18DB5
cmp     ax, 1Eh
jz      short loc_18DE8

loc_18E1E:
pop     si
mov     sp, bp
pop     bp
retn
sub_18C32 endp



; Attributes: bp-based frame

sub_18E23 proc near

arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
cmp     [bp+arg_0], 0
jnz     short loc_18E31
mov     ax, [bp+arg_2]
jmp     short loc_18E35

loc_18E31:
mov     ax, [bp+arg_0]
dec     ax

loc_18E35:
pop     bp
retn
sub_18E23 endp



; Attributes: bp-based frame

sub_18E37 proc near

var_2= word ptr -2
arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+52h]
sub     ah, ah
dec     ax
mov     [bp+var_2], ax
jmp     short loc_18E91

loc_18E52:
mov     bx, [bp+arg_0]
shl     bx, 1
mov     si, [bx-2518h]
mov     ax, 6
imul    [bp+var_2]
mov     bx, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
mov     si, ax
mov     bx, _globals
mov     al, [bx+si+3Eh]
sub     ah, ah
mov     cl, [bp+arg_2]
sub     ch, ch
test    ax, cx
jz      short loc_18E8E
mov     ax, 0FFh
push    ax
push    [bp+var_2]
push    [bp+arg_0]
call    sub_2060D
add     sp, 6

loc_18E8E:
dec     [bp+var_2]

loc_18E91:
cmp     [bp+var_2], 0
jge     short loc_18E52
pop     si
mov     sp, bp
pop     bp
retn
sub_18E37 endp



; Attributes: bp-based frame

sub_18E9C proc near

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_8], ax
mov     [bp+var_A], 0
jmp     short loc_18F17

loc_18EB6:
dec     [bp+var_4]

loc_18EB9:
cmp     [bp+var_4], 0
jl      short loc_18EEB
mov     bx, [bp+var_6]
mov     al, [bx+0Eh]
mov     cx, ax
mov     ax, 6
imul    [bp+var_4]
mov     si, ax
mov     bx, [bp+var_8]
cmp     [bx+si+62h], cl
jnz     short loc_18EB6
mov     ax, 0FFh
push    ax
push    [bp+var_4]
push    [bp+arg_0]
call    sub_2060D
add     sp, 6
jmp     short loc_18EB6

loc_18EEB:
inc     [bp+var_2]
add     [bp+var_6], 12h

loc_18EF2:
mov     ax, 3Ch ; '<'
imul    [bp+var_A]
mov     si, ax
mov     bx, _weapons
mov     ax, [bp+var_2]
cmp     [bx+si+3Ah], ax
jle     short loc_18F14
mov     bx, [bp+var_8]
mov     al, [bx+52h]
sub     ah, ah
dec     ax
mov     [bp+var_4], ax
jmp     short loc_18EB9

loc_18F14:
inc     [bp+var_A]

loc_18F17:
mov     bx, _weapons
mov     ax, [bx]
cmp     [bp+var_A], ax
jge     short loc_18F39
mov     ax, 3Ch ; '<'
imul    [bp+var_A]
mov     si, ax
mov     ax, [bx+si+3Ch]
add     ax, bx
mov     [bp+var_6], ax
mov     [bp+var_2], 0
jmp     short loc_18EF2

loc_18F39:
pop     si
mov     sp, bp
pop     bp
retn
sub_18E9C endp



; Attributes: bp-based frame

sub_18F3E proc near

arg_0= word ptr  4
arg_2= dword ptr  6

push    bp
mov     bp, sp
jmp     short loc_18F55

loc_18F43:
inc     word ptr [bp+arg_2]

loc_18F46:
les     bx, [bp+arg_2]
cmp     byte ptr es:[bx], 0
jnz     short loc_18F43
inc     word ptr [bp+arg_2]
dec     [bp+arg_0]

loc_18F55:
cmp     [bp+arg_0], 0
jnz     short loc_18F46
mov     ax, word ptr [bp+arg_2]
mov     dx, word ptr [bp+arg_2+2]
pop     bp
retn
sub_18F3E endp



; Attributes: bp-based frame

sub_18F63 proc near

text= dword ptr -4
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
push    [bp+arg_4]
push    [bp+arg_2]
push    [bp+arg_0]
call    sub_18F3E
add     sp, 6
mov     word ptr [bp+text], ax
mov     word ptr [bp+text+2], dx
mov     ax, 1
push    ax
push    _currentWindow  ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax              ; y
push    ax              ; x
push    word ptr [bp+text+2]
push    word ptr [bp+text] ; text
call    writeStringAt
mov     sp, bp
pop     bp
retn
sub_18F63 endp



; Attributes: bp-based frame

showMessage2 proc near

text= dword ptr -4
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
push    si              ; draw
mov     ax, 1
push    ax
cmp     byte_2ED26, 0
jz      short loc_18FB4
mov     ax, offset _window7
jmp     short loc_18FB7

loc_18FB4:
mov     ax, offset _windowDialog

loc_18FB7:              ; win
push    ax
call    setWindow
add     sp, 4
mov     bx, _currentWindow
mov     si, bx
mov     ax, [si+6]
sub     ax, [si+2]
mov     [bx+18h], ax
mov     byte_22A74, 1
push    [bp+arg_4]
push    [bp+arg_2]
push    [bp+arg_0]
call    sub_18F3E
add     sp, 6
mov     word ptr [bp+text], ax
mov     word ptr [bp+text+2], dx
sub     ax, ax
push    ax              ; y
push    ax              ; x
push    dx
push    word ptr [bp+text] ; text
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
showMessage2 endp



; Attributes: bp-based frame

sub_18FFA proc near

arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
push    si
cmp     [bp+arg_2], 0
jle     short loc_19070
mov     bx, [bp+arg_0]
cmp     word ptr [bx+44h], 0
jle     short loc_19045
mov     ax, [bx+44h]
sub     ax, [bp+arg_2]
cmp     ax, 1
jge     short loc_19045
mov     ax, 64h ; 'd'
imul    word ptr [bx+44h]
cwd
mov     cx, [bx+46h]
idiv    cx
cmp     ax, 32h ; '2'
jle     short loc_1903C
mov     ax, cx
cwd
xor     ax, dx
sub     ax, dx
mov     cx, 2
sar     ax, cl
xor     ax, dx
sub     ax, dx
inc     ax
jmp     short loc_19041

loc_1903C:
mov     si, bx
mov     ax, [si+44h]

loc_19041:
mov     [bx+122h], ax

loc_19045:
mov     bx, [bp+arg_0]
mov     bl, [bx+49h]
sub     bh, bh
test    byte ptr [bx-3A60h], 2
jnz     short loc_1905D
mov     bx, [bp+arg_0]
mov     ax, [bp+arg_2]
sub     [bx+44h], ax

loc_1905D:
mov     bx, [bp+arg_0]
cmp     word ptr [bx+44h], 0FFC4h
jg      short loc_19070
mov     bl, [bx+49h]
sub     bh, bh
or      byte ptr [bx-3A60h], 2

loc_19070:
pop     si
pop     bp
retn
sub_18FFA endp



; Attributes: bp-based frame

sub_19073 proc near

var_6= word ptr -6
member= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 6
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
push    [bp+arg_4]
push    [bp+arg_2]
call    sub_1DEB1
add     sp, 4
mov     [bp+var_6], ax
cmp     byte_21329, 0
jz      short loc_1909F
sub     ax, ax
jmp     short loc_190A8

loc_1909F:              ; member
push    [bp+member]
call    getArmorClass
add     sp, 2

loc_190A8:
mov     [bp+var_2], ax
or      ax, ax
jz      short loc_190D1
mov     bx, _globals
mov     al, [bx+1]
sub     ah, ah
mul     [bp+var_2]
push    ax
mov     al, [bx]
sub     ah, ah
mov     cx, ax
mov     ax, [bp+var_2]
mul     cx
push    ax
call    sub_1DEB1
add     sp, 4
mov     [bp+var_2], ax

loc_190D1:
mov     ax, [bp+var_2]
cmp     [bp+var_6], ax
jle     short loc_190EA
sub     [bp+var_6], ax
push    [bp+var_6]
push    [bp+member]
call    sub_18FFA
add     sp, 4
jmp     short loc_190EF

loc_190EA:
mov     [bp+var_6], 0

loc_190EF:
mov     ax, [bp+var_6]
mov     sp, bp
pop     bp
retn
sub_19073 endp



; Attributes: bp-based frame

; void __cdecl getMemberCon(PartyMember near *member)
getMemberCon proc near

result= word ptr -2
member= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+result], 0
mov     bx, [bp+member]
cmp     [bx+PartyMember._con], 1
jge     short loc_19128
cmp     [bx+PartyMember._con], -60
jg      short loc_19115
mov     ax, 5
jmp     short loc_19125

loc_19115:              ; int
push    [bx+PartyMember._con]
call    _abs
add     sp, 2
cwd
mov     cx, 15
idiv    cx
inc     ax

loc_19125:
mov     [bp+result], ax

loc_19128:
mov     ax, [bp+result]
mov     sp, bp
pop     bp
retn
getMemberCon endp



; Attributes: bp-based frame

sub_1912F proc near

var_C= word ptr -0Ch
member= word ptr -0Ah
var_6= word ptr -6
win= dword ptr -4
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch

push    bp
mov     bp, sp
sub     sp, 0Ch
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
push    [bp+arg_8]
push    [bp+arg_6]
push    [bp+arg_0]
call    showMessage2
add     sp, 6
call    sub_1DDFD
call    sub_1DDFD
call    sub_1DC28
mov     [bp+member+2], 0
jmp     short loc_191D9

loc_1915A:
mov     ax, offset aAndHasBeenRend ; " and has been rendered unconscious"

loc_1915D:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 4

loc_19168:
mov     ax, offset asc_22378 ; ".\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _strcat

loc_19173:
add     sp, 4
jmp     short loc_191C3

loc_19178:
mov     bx, word ptr [bp+win+2]
shl     bx, 1
push    word ptr [bx+10B0h]
mov     bx, [bp+member]
mov     bl, [bx+50h]
and     bx, 3
shl     bx, 1
push    word ptr [bx+10AEh] ; char *
mov     ax, offset aAndSConditionH ; " and %s condition has become %s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
jmp     short loc_19168

loc_191A0:
mov     ax, 1129h
jmp     short loc_1915D

loc_191A5:
mov     ax, offset asc_2237C ; ".\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
jmp     short loc_19173

loc_191B2:              ; char *
push    [bp+member]
mov     ax, offset aSSArmorAbsorbs ; "%s's armor absorbs the damage.\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6

loc_191C3:
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_191D6:
inc     [bp+member+2]

loc_191D9:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+member+2], ax
jb      short loc_191E6
jmp     loc_19276

loc_191E6:
mov     bx, [bp+member+2]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
push    ax              ; member
call    getMemberCon
add     sp, 2
mov     [bp+var_6], ax
cmp     ax, 5
jz      short loc_191D6
push    [bp+arg_4]
push    [bp+arg_2]
push    [bp+member+2]
call    sub_19073
add     sp, 6
mov     [bp+var_C], ax
or      ax, ax
jle     short loc_191B2
push    ax
push    [bp+member]     ; char *
mov     ax, offset aSTakesUPtsOfDa ; "%s takes %u pts of damage"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
push    [bp+member]     ; member
call    getMemberCon
add     sp, 2
mov     word ptr [bp+win+2], ax
mov     ax, [bp+var_6]
cmp     word ptr [bp+win+2], ax
jnz     short loc_19250
jmp     loc_191A5

loc_19250:
mov     ax, word ptr [bp+win+2]
cmp     ax, 1
jnz     short loc_1925B
jmp     loc_1915A

loc_1925B:
cmp     ax, 2
jge     short loc_19263
jmp     loc_19168

loc_19263:
cmp     ax, 4
jg      short loc_1926B
jmp     loc_19178

loc_1926B:
cmp     ax, 5
jnz     short loc_19273
jmp     loc_191A0

loc_19273:
jmp     loc_19168

loc_19276:
call    getKeypress
cmp     al, 0FEh
jnz     short loc_19276
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
mov     byte_22A74, 0
sub     ax, ax
push    ax
call    sub_12B93
mov     sp, bp
pop     bp
retn
sub_1912F endp



; Attributes: bp-based frame

sub_19298 proc near

var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_4], ax
push    [bp+arg_4]
push    [bp+arg_2]
call    sub_1DEB1
add     sp, 4
mov     [bp+var_2], ax
mov     bx, [bp+var_4]
mov     si, bx
mov     ax, [si+44h]
add     ax, [bp+var_2]
cmp     ax, [si+46h]
jle     short loc_192CD
mov     ax, [si+46h]

loc_192CD:
mov     [bx+44h], ax
pop     si
mov     sp, bp
pop     bp
retn
sub_19298 endp



; Attributes: bp-based frame

sub_192D5 proc near

var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 2
cmp     word_2E41C, 1
jnz     short loc_192FD
push    word ptr dword_2EC04+2
push    word ptr dword_2EC04
push    word_2F018
push    word_2E206
sub     ax, ax
push    ax
call    sub_1912F
add     sp, 0Ah
jmp     short loc_19331

loc_192FD:
mov     [bp+var_2], 0
jmp     short loc_19327

loc_19304:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
test    byte ptr [bx+58h], 7Fh
jnz     short loc_19324
push    word_2F018
push    word_2E206
push    [bp+var_2]
call    sub_19298
add     sp, 6

loc_19324:
inc     [bp+var_2]

loc_19327:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_2], ax
jb      short loc_19304

loc_19331:
mov     sp, bp
pop     bp
retn
sub_192D5 endp



; Attributes: bp-based frame

sub_19335 proc near

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 8
push    si
sub     ax, ax
mov     [bp+var_4], ax
mov     [bp+var_6], ax
push    [bp+arg_6]
push    [bp+arg_0]
call    sub_18E23
add     sp, 4
mov     [bp+var_2], ax
mov     bx, ax
shl     bx, 1
push    word ptr [bx-2518h] ; member
call    getMemberCon
add     sp, 2
mov     [bp+var_8], ax
mov     ax, [bp+arg_2]
cmp     ax, 9           ; switch 10 cases
jbe     short loc_1936F
jmp     def_19372       ; jumptable 00019372 default case

loc_1936F:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_19372[bx] ; switch jump

loc_19377:              ; jumptable 00019372 case 0
cmp     [bp+var_8], 0
jz      short loc_19380
jmp     def_19372       ; jumptable 00019372 default case

loc_19380:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+4Eh]

loc_1938C:
sub     ah, ah

loc_1938E:
mov     [bp+var_6], ax
mov     [bp+var_4], 0
jmp     def_19372       ; jumptable 00019372 default case

loc_19399:              ; jumptable 00019372 case 1
cmp     [bp+var_8], 0
jz      short loc_193A2
jmp     def_19372       ; jumptable 00019372 default case

loc_193A2:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bx+5Ah]
mov     dx, [bx+5Ch]

loc_193B1:
mov     [bp+var_6], ax
mov     [bp+var_4], dx
jmp     def_19372       ; jumptable 00019372 default case

loc_193BA:              ; jumptable 00019372 case 2
cmp     [bp+var_8], 5
jl      short loc_193C3
jmp     def_19372       ; jumptable 00019372 default case

loc_193C3:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bx+44h]

loc_193CF:
cwd
jmp     short loc_193B1

loc_193D2:              ; jumptable 00019372 case 3
cmp     [bp+var_8], 5
jl      short loc_193DB
jmp     def_19372       ; jumptable 00019372 default case

loc_193DB:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bx+46h]
jmp     short loc_193CF

loc_193E9:              ; jumptable 00019372 case 4
cmp     [bp+var_8], 0
jz      short loc_193F2
jmp     def_19372       ; jumptable 00019372 default case

loc_193F2:
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, [bx+si+18h]
jmp     short loc_1938C

loc_19403:              ; jumptable 00019372 case 5
cmp     [bp+var_8], 0
jnz     short def_19372 ; jumptable 00019372 default case
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, [bx+si+24h]
jmp     loc_1938C

loc_1941B:              ; jumptable 00019372 case 6
cmp     [bp+var_8], 0
jnz     short def_19372 ; jumptable 00019372 default case
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, [bx+si+34h]
jmp     loc_1938C

loc_19433:              ; jumptable 00019372 case 7
mov     ax, word ptr _disk1._cash
mov     dx, word ptr _disk1._cash+2
jmp     loc_193B1

loc_1943D:              ; jumptable 00019372 case 8
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+50h]
sub     ah, ah
and     ax, 3
jmp     loc_1938E

loc_19451:              ; jumptable 00019372 case 9
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+48h]
jmp     loc_1938C
jpt_19372 dw offset loc_19377 ; jump table for switch statement
dw offset loc_19399
dw offset loc_193BA
dw offset loc_193D2
dw offset loc_193E9
dw offset loc_19403
dw offset loc_1941B
dw offset loc_19433
dw offset loc_1943D
dw offset loc_19451

def_19372:              ; jumptable 00019372 default case
mov     ax, [bp+var_6]
mov     dx, [bp+var_4]
pop     si
mov     sp, bp
pop     bp
retn
sub_19335 endp



; Attributes: bp-based frame

sub_1947F proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch

push    bp
mov     bp, sp
mov     bx, [bp+arg_0]
mov     ax, [bp+arg_2]
mov     dx, [bp+arg_4]
cmp     [bx+2], dx
jg      short loc_194A1
jl      short loc_19496
cmp     [bx], ax
jnb     short loc_194A1

loc_19496:
mov     ax, [bp+arg_2]
mov     dx, [bp+arg_4]
mov     [bx], ax
mov     [bx+2], dx

loc_194A1:
mov     bx, [bp+arg_0]
mov     ax, [bp+arg_6]
mov     dx, [bp+arg_8]
cmp     [bx+2], dx
jl      short loc_194C0
jg      short loc_194B5
cmp     [bx], ax
jbe     short loc_194C0

loc_194B5:
mov     ax, [bp+arg_6]
mov     dx, [bp+arg_8]
mov     [bx], ax
mov     [bx+2], dx

loc_194C0:
pop     bp
retn
sub_1947F endp



; Attributes: bp-based frame

sub_194C2 proc near

var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch
arg_A= word ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 4
push    si
push    [bp+arg_A]
push    [bp+arg_0]
call    sub_18E23
add     sp, 4
mov     [bp+var_2], ax
mov     bx, ax
shl     bx, 1
push    word ptr [bx-2518h] ; member
call    getMemberCon
add     sp, 2
mov     [bp+var_4], ax
mov     ax, [bp+arg_2]
cmp     ax, 7           ; switch 8 cases
jbe     short loc_194F4
jmp     def_194F7       ; jumptable 000194F7 default case

loc_194F4:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_194F7[bx] ; switch jump

loc_194FC:              ; jumptable 000194F7 case 0
cmp     [bp+var_4], 0
jz      short loc_19505
jmp     def_194F7       ; jumptable 000194F7 default case

loc_19505:
mov     ax, 63h ; 'c'
cwd
push    dx
push    ax
mov     ax, 1
cwd
push    dx
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, byte ptr [bp+arg_6]
mov     [bx+4Eh], al
jmp     def_194F7       ; jumptable 000194F7 default case

loc_1952D:              ; jumptable 000194F7 case 1
cmp     [bp+var_4], 0
jz      short loc_19536
jmp     def_194F7       ; jumptable 000194F7 default case

loc_19536:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bp+arg_6]
mov     dx, [bp+arg_8]
add     [bx+5Ah], ax
adc     [bx+5Ch], dx
call    sub_17B3F
jmp     def_194F7       ; jumptable 000194F7 default case

loc_19551:              ; jumptable 000194F7 case 2
cmp     [bp+var_4], 5
jnz     short loc_1955A
jmp     def_194F7       ; jumptable 000194F7 default case

loc_1955A:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bx+46h]
cwd
push    dx
push    ax
mov     ax, 0FFC4h
cwd
push    dx
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bp+arg_6]
mov     [bx+44h], ax
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
cmp     word ptr [si+44h], 0FFC4h
jle     short loc_1959A
jmp     def_194F7       ; jumptable 000194F7 default case

loc_1959A:
mov     bl, [si+49h]
sub     bh, bh
or      byte ptr [bx-3A60h], 2
mov     al, byte_2ED26
sub     ah, ah
push    ax
call    sub_12B93
add     sp, 2
jmp     def_194F7       ; jumptable 000194F7 default case

loc_195B3:              ; jumptable 000194F7 case 3
cmp     [bp+var_4], 5
jnz     short loc_195BC
jmp     def_194F7       ; jumptable 000194F7 default case

loc_195BC:
mov     bx, [bp+var_2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     ax, [bp+arg_6]
mov     [bx+46h], ax
jmp     def_194F7       ; jumptable 000194F7 default case

loc_195CE:              ; jumptable 000194F7 case 4
cmp     [bp+var_4], 0
jz      short loc_195D7
jmp     def_194F7       ; jumptable 000194F7 default case

loc_195D7:
mov     ax, 23h ; '#'
cwd
push    dx
push    ax
mov     ax, 1
cwd
push    dx
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, byte ptr [bp+arg_6]
mov     [bx+si+18h], al
jmp     def_194F7       ; jumptable 000194F7 default case

loc_19602:              ; jumptable 000194F7 case 5
cmp     [bp+var_4], 0
jz      short loc_1960B
jmp     def_194F7       ; jumptable 000194F7 default case

loc_1960B:
mov     ax, 23h ; '#'
cwd
push    dx
push    ax
mov     ax, 1
cwd
push    dx
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, byte ptr [bp+arg_6]
mov     [bx+si+24h], al
jmp     short def_194F7 ; jumptable 000194F7 default case

loc_19635:              ; jumptable 000194F7 case 6
cmp     [bp+var_4], 0
jnz     short def_194F7 ; jumptable 000194F7 default case
mov     ax, 23h ; '#'
cwd
push    dx
push    ax
mov     ax, 1
cwd
push    dx
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     bx, [bp+var_2]
shl     bx, 1
mov     si, [bx-2518h]
mov     bx, [bp+arg_4]
mov     al, byte ptr [bp+arg_6]
mov     [bx+si+34h], al
jmp     short def_194F7 ; jumptable 000194F7 default case

loc_19665:              ; jumptable 000194F7 case 7
mov     ax, 423Fh
mov     dx, 0Fh
push    dx
push    ax
sub     ax, ax
push    ax
push    ax
lea     ax, [bp+arg_6]
push    ax
call    sub_1947F
add     sp, 0Ah
mov     ax, [bp+arg_6]
mov     dx, [bp+arg_8]
mov     word ptr _disk1._cash, ax
mov     word ptr _disk1._cash+2, dx
jmp     short def_194F7 ; jumptable 000194F7 default case
jpt_194F7 dw offset loc_194FC ; jump table for switch statement
dw offset loc_1952D
dw offset loc_19551
dw offset loc_195B3
dw offset loc_195CE
dw offset loc_19602
dw offset loc_19635
dw offset loc_19665

def_194F7:              ; jumptable 000194F7 default case
pop     si
mov     sp, bp
pop     bp
retn
sub_194C2 endp



; Attributes: bp-based frame

sub_1969F proc near

var_8= byte ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 8
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
mov     [bp+var_4], 0
cmp     [bp+arg_2], 0Fh
jle     short loc_196C2
mov     ax, 1
jmp     short loc_196C5

loc_196C2:
mov     ax, 4

loc_196C5:
mov     [bp+var_2], ax
mov     di, [bp+arg_2]
mov     bx, [bp+var_6]
inc     byte ptr [bx+di+124h]
mov     si, [bp+arg_2]
add     si, [bp+var_6]
mov     ax, 16h
imul    [bp+arg_2]
mov     di, ax
les     bx, _hdsPct
mov     al, [si+124h]
cmp     es:[bx+di+13Ah], al
ja      short loc_1971F
mov     di, [bp+var_2]
mov     bx, [bp+var_6]
mov     al, [bx+di+18h]
mov     [bp+var_8], al
cmp     al, 0Fh
jbe     short loc_19714
sub     ah, ah
sub     ax, 0Fh
mov     cl, [si+24h]
sub     ch, ch
cmp     ax, cx
jbe     short loc_19714
inc     byte ptr [si+24h]
mov     [bp+var_4], 1

loc_19714:
mov     si, [bp+arg_2]
mov     bx, [bp+var_6]
mov     byte ptr [bx+si+124h], 0

loc_1971F:
mov     al, [bp+var_4]
sub     ah, ah
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1969F endp



; Attributes: bp-based frame

sub_1972A proc near

arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
mov     bx, [bp+arg_4]
mov     ax, [bx]
or      ax, ax
jz      short loc_19742
cmp     ax, 1
jz      short loc_1975B
cmp     ax, 2
jz      short loc_1976D
jmp     short loc_1974A

loc_19742:
mov     bx, [bp+arg_0]
mov     al, [bp+arg_2]

loc_19748:
mov     [bx], al

loc_1974A:
mov     bx, [bp+arg_4]
mov     ax, [bp+arg_6]
mov     [bx], ax
mov     bx, [bp+arg_0]
mov     al, [bx]
sub     ah, ah
jmp     short loc_1977E

loc_1975B:
mov     bx, [bp+arg_0]
cmp     byte ptr [bx], 0
jz      short loc_19777

loc_19763:
cmp     [bp+arg_2], 0
jz      short loc_19777

loc_19769:
mov     al, 1
jmp     short loc_19779

loc_1976D:
mov     bx, [bp+arg_0]
cmp     byte ptr [bx], 0
jnz     short loc_19769
jmp     short loc_19763

loc_19777:
sub     al, al

loc_19779:
mov     bx, [bp+arg_0]
jmp     short loc_19748

loc_1977E:
pop     bp
retn
sub_1972A endp



; Attributes: bp-based frame

sub_19780 proc near

var_6= byte ptr -6
var_4= byte ptr -4
var_2= byte ptr -2
arg_0= byte ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 6
mov     [bp+var_4], 0
mov     al, [bp+arg_2]
cbw
push    ax              ; y
mov     al, [bp+arg_0]
cbw
push    ax              ; x
mov     ax, offset aYEsNO ; "Y)es  N)o"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
jmp     short loc_197C3

loc_197A4:
call    getKeypress
mov     [bp+var_2], al
cbw
cmp     ax, 4Eh ; 'N'
jz      short loc_197BB
cmp     ax, 59h ; 'Y'
jnz     short loc_197C3
mov     [bp+var_6], 1
jmp     short loc_197BF

loc_197BB:
mov     [bp+var_6], 0

loc_197BF:
mov     [bp+var_4], 1

loc_197C3:
cmp     [bp+var_4], 0
jz      short loc_197A4
mov     al, [bp+var_6]
sub     ah, ah
mov     sp, bp
pop     bp
retn
sub_19780 endp



; Attributes: bp-based frame

YesNo proc near

var_4= byte ptr -4
win= dword ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
sub     ax, ax
push    ax
cmp     byte_2ED26, 0
jz      short loc_197ED
mov     ax, offset _window7
jmp     short loc_197F0

loc_197ED:
mov     ax, offset _windowDialog

loc_197F0:              ; win
push    ax
call    setWindow
add     sp, 4
push    [bp+arg_4]
push    [bp+arg_2]
push    [bp+arg_0]
call    sub_18F63
add     sp, 6
call    sub_1DDFD
call    sub_1DDFD
mov     ax, 0FFFFh
push    ax
sub     ax, ax
push    ax
call    sub_19780
add     sp, 4
mov     [bp+var_4], al
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
mov     al, [bp+var_4]
sub     ah, ah
mov     sp, bp
pop     bp
retn
YesNo endp



; Attributes: bp-based frame

sub_1982E proc near

var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+arg_0], ax
jb      short loc_19843

loc_1983F:
sub     ax, ax
jmp     short loc_19875

loc_19843:
mov     [bp+var_2], 0
jmp     short loc_1984D

loc_1984A:
inc     [bp+var_2]

loc_1984D:
mov     bx, [bp+arg_0]
shl     bx, 1
mov     si, [bx-2518h]
mov     al, [si+52h]
sub     ah, ah
cmp     ax, [bp+var_2]
jbe     short loc_1983F
mov     ax, 6
imul    [bp+var_2]
mov     bx, ax
mov     al, [bx+si+62h]
sub     ah, ah
cmp     ax, [bp+arg_2]
jnz     short loc_1984A
mov     ax, 1

loc_19875:
pop     si
mov     sp, bp
pop     bp
retn
sub_1982E endp



; Attributes: bp-based frame

; void __cdecl showMessage(int line, const char far *msg)
showMessage proc near

win= dword ptr -2
line= word ptr  4
msg= dword ptr  6

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
push    word ptr [bp+msg+2]
push    word ptr [bp+msg]
push    [bp+line]
call    showMessage2
add     sp, 6
call    sub_1DC28

loc_19898:
call    getKeypress
cmp     al, 0FEh
jnz     short loc_19898
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
mov     byte_22A74, 0
mov     sp, bp
pop     bp
retn
showMessage endp




sub_198B1 proc near
les     bx, dword_22C6E
mov     ax, es:[bx]
mul     _disk1._mapPosY
add     ax, _disk1._mapPosX
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr dword_2E4C6, ax
mov     word ptr dword_2E4C6+2, dx
add     ax, 6
mov     word_2EBF4, ax
mov     word_2EBF6, dx
mov     word_2E418, 0
retn
sub_198B1 endp



; Attributes: bp-based frame

sub_198E3 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word_22C8A, ax
pop     bp
retn
sub_198E3 endp



; Attributes: bp-based frame

doScript proc near

var_13C= dword ptr -13Ch
var_138= dword ptr -138h
var_134= word ptr -134h
var_132= byte ptr -132h
var_11C= word ptr -11Ch
var_11A= word ptr -11Ah
var_118= word ptr -118h
var_116= word ptr -116h
var_114= word ptr -114h
var_112= word ptr -112h
var_110= word ptr -110h
var_10E= word ptr -10Eh
var_E8= word ptr -0E8h
var_E6= byte ptr -0E6h
var_D2= byte ptr -0D2h
var_D0= word ptr -0D0h
var_CE= word ptr -0CEh
params0= word ptr -0CCh
var_CA= word ptr -0CAh
var_C8= word ptr -0C8h
param3= word ptr -0C6h
var_C4= word ptr -0C4h
var_C0= word ptr -0C0h
var_BE= word ptr -0BEh
var_AA= dword ptr -0AAh
var_A6= word ptr -0A6h
var_A4= word ptr -0A4h
var_A2= word ptr -0A2h
var_A0= word ptr -0A0h
var_9E= word ptr -9Eh
var_9C= word ptr -9Ch
var_9A= word ptr -9Ah
var_74= byte ptr -74h
var_72= byte ptr -72h
win= dword ptr -70h
var_6C= word ptr -6Ch
var_6A= word ptr -6Ah
var_68= word ptr -68h
var_66= byte ptr -66h
var_64= dword ptr -64h
var_60= dword ptr -60h
var_5C= word ptr -5Ch
var_5A= word ptr -5Ah
var_58= byte ptr -58h
var_57= byte ptr -57h
text= byte ptr -56h
var_55= byte ptr -55h
var_4A= word ptr -4Ah
var_48= word ptr -48h
var_46= word ptr -46h
var_44= word ptr -44h
var_42= byte ptr -42h
arg_0= dword ptr  4
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch
arg_A= byte ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 13Ch
push    di
push    si              ; draw
mov     ax, [bp+arg_6]
mov     [bp+var_A2], ax
sub     ax, ax
mov     [bp+var_5A], ax
mov     [bp+var_5C], ax
mov     [bp+var_11C], ax
mov     [bp+var_112], ax
mov     [bp+var_68], ax
mov     [bp+var_134], ax
mov     [bp+var_11A], ax
mov     [bp+var_48], ax
mov     [bp+var_E8], ax
mov     [bp+var_74], 0
mov     [bp+var_D2], 0
mov     [bp+var_44], ax
mov     [bp+var_6C], ax
mov     word_2E418, 0FFFFh
les     bx, [bp+arg_0]
mov     si, es:[bx]
cmp     si, 0FFFFh
jz      short loc_19972
mov     bx, si
shl     bx, 1
les     di, dword_2EE12
mov     ax, es:[bx+di]
add     ax, word_22C72
mov     dx, word_22C74
mov     word ptr [bp+var_138], ax
mov     word ptr [bp+var_138+2], dx
mov     ax, 1
mov     cl, byte ptr [bp+arg_4]
shl     ax, cl
les     bx, [bp+var_138]
test    es:[bx], ax
jnz     short loc_19977

loc_19972:
sub     ax, ax
jmp     loc_1B24A

loc_19977:
call    sub_16D5E
mov     byte_22BB9, 1
push    [bp+var_A2]
call    sub_198E3
add     sp, 2
cmp     byte_2ED26, 0
jnz     short loc_1999F
cmp     [bp+arg_4], 6
jnz     short loc_1999F
push    [bp+arg_8]
call    sub_1D8CD
add     sp, 2

loc_1999F:
cmp     byte_2ED26, 0
jz      short loc_199B4
mov     ax, 1
push    ax
call    sub_1BB93
add     sp, 2
mov     [bp+var_A2], ax

loc_199B4:
mov     [bp+var_132], 0
mov     [bp+var_E6], 0
mov     [bp+text], 0
cmp     [bp+arg_4], 4
jnz     short loc_199CF
mov     al, [bp+arg_A]
mov     [bp+var_132], al

loc_199CF:
mov     ax, word ptr [bp+var_138]
mov     dx, word ptr [bp+var_138+2]
add     ax, 4

loc_199DA:
mov     [bp+var_118], ax
mov     [bp+var_116], dx

def_19A29:              ; jumptable 00019A29 default case, cases 33,51
cmp     [bp+var_D2], 0  ; jumptable 00019A3D default case
jz      short loc_199EC
jmp     loc_1B242

loc_199EC:
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
cmp     ax, [bp+var_118]
ja      short loc_19A01
jmp     loc_1B242

loc_19A01:
push    [bp+arg_4]
lea     ax, [bp+var_D0]
push    ax
lea     ax, [bp+var_118]
push    ax
call    sub_1DF48
add     sp, 6
mov     ax, [bp+var_D0]
cmp     [bp+arg_4], ax
jnz     short def_19A29 ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case
mov     ax, [bp+var_CE]
cmp     ax, 4Bh         ; switch 76 cases
ja      short def_19A29 ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_19A29[bx] ; switch jump
jmp     loc_19972

opcode01:               ; jumptable 00019A29 case 1
mov     ax, [bp+params0]
cmp     ax, 7           ; switch 8 cases
ja      short def_19A29 ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_19A3D[bx] ; switch jump

loc_19A42:              ; jumptable 00019A3D case 0
mov     ax, [bp+var_11C]
cwd
jmp     short loc_19A4F

loc_19A49:              ; jumptable 00019A3D case 1
mov     ax, [bp+var_CA]
sub     dx, dx

loc_19A4F:
add     [bp+var_5C], ax
adc     [bp+var_5A], dx
jmp     short def_19A29 ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19A57:              ; jumptable 00019A3D case 2
mov     ax, [bp+var_11C]
cwd
jmp     short loc_19A64

loc_19A5E:              ; jumptable 00019A3D case 3
mov     ax, [bp+var_CA]
sub     dx, dx

loc_19A64:
sub     [bp+var_5C], ax
sbb     [bp+var_5A], dx
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19A6D:              ; jumptable 00019A3D case 4
cmp     [bp+var_11C], 0
jnz     short loc_19A77
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19A77:
mov     ax, [bp+var_11C]
cwd
push    dx
push    ax
jmp     short loc_19A91

loc_19A80:              ; jumptable 00019A3D case 5
cmp     [bp+var_CA], 0
jnz     short loc_19A8A
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19A8A:
sub     ax, ax
push    ax
push    [bp+var_CA]

loc_19A91:
lea     ax, [bp+var_5C]
push    ax
call    unknown_libname_3 ; MS Quick C v1.0/v2.01 & MSC v5.1 DOS run-time & graphic
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19A9B:              ; jumptable 00019A3D case 6
mov     ax, [bp+var_11C]
cwd
push    dx
push    ax
jmp     short loc_19AAB

loc_19AA4:              ; jumptable 00019A3D case 7
sub     ax, ax
push    ax
push    [bp+var_CA]

loc_19AAB:
lea     ax, [bp+var_5C]
push    ax
call    __aNNalmul
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case
jpt_19A3D dw offset loc_19A42 ; jump table for switch statement
dw offset loc_19A49
dw offset loc_19A57
dw offset loc_19A5E
dw offset loc_19A6D
dw offset loc_19A80
dw offset loc_19A9B
dw offset loc_19AA4
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode02:               ; jumptable 00019A29 case 2
mov     ax, [bp+params0]
or      ax, ax
jz      short loc_19AE7
cmp     ax, 1
jz      short loc_19AF1
cmp     ax, 2
jz      short loc_19AFF
cmp     ax, 3
jz      short loc_19B08
cmp     ax, 4
jz      short loc_19B12
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19AE7:
mov     ax, [bp+var_5C]
mov     [bp+var_11C], ax
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19AF1:
mov     ax, [bp+var_11C]
cwd

loc_19AF6:
mov     [bp+var_5C], ax
mov     [bp+var_5A], dx
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19AFF:
mov     [bp+var_11C], 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19B08:
mov     ax, [bp+var_5C]
add     [bp+var_11C], ax
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19B12:
mov     ax, [bp+var_5C]
sub     [bp+var_11C], ax
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode03:               ; jumptable 00019A29 case 3
mov     ax, [bp+params0]

loc_19B20:
mov     [bp+var_5C], ax
mov     [bp+var_5A], 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode04:               ; jumptable 00019A29 case 4
push    [bp+var_A2]
push    [bp+var_C8]
push    [bp+var_CA]
push    [bp+params0]
call    sub_19335
add     sp, 8
jmp     short loc_19AF6

opcode05:               ; jumptable 00019A29 case 5
push    [bp+var_A2]
push    [bp+var_5A]
push    [bp+var_5C]
push    [bp+var_C8]
push    [bp+var_CA]
push    [bp+params0]
call    sub_194C2

loc_19B5C:
add     sp, 0Ch
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode06:               ; jumptable 00019A29 case 6
push    [bp+var_A2]
push    [bp+var_CA]
push    [bp+params0]
mov     ax, 1
push    ax
call    sub_19335
add     sp, 8
mov     [bp+var_5C], ax
mov     [bp+var_5A], dx
mov     [bp+var_A6], 1
jmp     short loc_19B8A

loc_19B86:
inc     [bp+var_A6]

loc_19B8A:
mov     al, _disk1._partyCount
sub     ah, ah
inc     ax
cmp     [bp+var_A6], ax
jb      short loc_19B99
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19B99:
push    [bp+var_A2]
push    [bp+var_CA]
push    [bp+params0]
push    [bp+var_A6]
call    sub_19335
add     sp, 8
mov     [bp+var_A0], ax
mov     [bp+var_9E], dx
mov     ax, [bp+var_5C]
mov     dx, [bp+var_5A]
cmp     [bp+var_9E], dx
jl      short loc_19B86
jg      short loc_19BCB
cmp     [bp+var_A0], ax
jbe     short loc_19B86

loc_19BCB:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
mov     [bp+var_5C], ax
mov     [bp+var_5A], dx
jmp     short loc_19B86

opcode07:               ; jumptable 00019A29 case 7
push    [bp+var_A2]
push    [bp+var_CA]
push    [bp+params0]
mov     ax, 1
push    ax
call    sub_19335
add     sp, 8
mov     [bp+var_5C], ax
mov     [bp+var_5A], dx
mov     [bp+var_A6], 1
jmp     short loc_19C03

loc_19BFF:
inc     [bp+var_A6]

loc_19C03:
mov     al, _disk1._partyCount
sub     ah, ah
inc     ax
cmp     [bp+var_A6], ax
jb      short loc_19C12
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19C12:
mov     bx, [bp+var_A6]
shl     bx, 1
mov     bx, [bx-251Ah]
cmp     word ptr [bx+44h], 0
jle     short loc_19BFF
push    [bp+var_A2]
push    [bp+var_CA]
push    [bp+params0]
push    [bp+var_A6]
call    sub_19335
add     sp, 8
mov     [bp+var_A0], ax
mov     [bp+var_9E], dx
mov     ax, [bp+var_5C]
mov     dx, [bp+var_5A]
cmp     [bp+var_9E], dx
jg      short loc_19BFF
jl      short loc_19C54
cmp     [bp+var_A0], ax
jnb     short loc_19BFF

loc_19C54:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
mov     [bp+var_5C], ax
mov     [bp+var_5A], dx
jmp     short loc_19BFF

opcode08:               ; jumptable 00019A29 case 8
mov     ax, word ptr _disk1._cash
mov     dx, word ptr _disk1._cash+2
jmp     loc_19AF6

loc_19C6E:              ; jumptable 00019A29 case 56
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
push    [bp+var_CA]
call    sub_16D6B
add     sp, 2
or      al, al
jz      short loc_19C97
push    [bp+var_CA]
call    sub_16D96
jmp     loc_19EAE

loc_19C97:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
cmp     byte ptr [si+52h], 20h ; ' '
jb      short loc_19CA9
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19CA9:
mov     al, 6
mul     byte ptr [si+52h]
mov     bx, ax
mov     al, byte ptr [bp+var_CA]
mov     [bx+si+62h], al
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
push    [bp+var_CA]
mov     al, 6
mul     byte ptr [si+52h]
mov     bx, ax
lea     ax, [bx+si+62h]
push    ax
call    sub_1D6AA
add     sp, 4
mov     bx, [bp+var_4A]
shl     bx, 1
mov     bx, [bx-2518h]
inc     byte ptr [bx+52h]
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode09:               ; jumptable 00019A29 case 9
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
push    [bp+var_CA]
push    ax
call    sub_1982E
add     sp, 4
mov     si, [bp+var_112]
mov     [bp+si+var_132], al
jmp     loc_1AC4E

opcode10:               ; jumptable 00019A29 case 10
push    [bp+params0]
call    sub_16D6B
add     sp, 2
or      al, al
jz      short loc_19D3B
mov     ax, 18h
mul     [bp+params0]
mov     si, ax
mov     bx, _globals
mov     cl, [bx+si+41h]
mov     al, 1
shl     al, cl
and     al, _disk1.field_7

loc_19D31:
mov     si, [bp+var_112]
mov     [bp+si+var_132], al
jmp     short loc_19D6C

loc_19D3B:
mov     [bp+var_A6], 0
jmp     short loc_19D47

loc_19D43:
inc     [bp+var_A6]

loc_19D47:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A6], ax
jnb     short loc_19D6C
push    [bp+params0]
push    [bp+var_A6]
call    sub_1982E
add     sp, 4
mov     si, [bp+var_112]
mov     [bp+si+var_132], al
cmp     al, 1
jnz     short loc_19D43

loc_19D6C:
push    [bp+var_CA]
jmp     loc_1AC52

loc_19D73:              ; jumptable 00019A29 case 66
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
mov     [bp+var_A6], 0
mov     [bp+var_C0], 0
jmp     short loc_19DA9

loc_19D92:
mov     ax, 6
imul    [bp+var_A6]
mov     bx, ax
cmp     byte ptr [bx+si+62h], 0FFh
jz      short loc_19DA5
inc     [bp+var_C0]

loc_19DA5:
inc     [bp+var_A6]

loc_19DA9:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
mov     al, [si+52h]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_19D92
push    [bp+var_CA]
lea     ax, [bp+var_48]
push    ax
cmp     [bp+var_C0], 20h ; ' '
jl      short loc_19DD2
mov     ax, 1
jmp     loc_1AC60

loc_19DD2:
sub     ax, ax
jmp     loc_1AC60

opcode11:               ; jumptable 00019A29 case 11
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
cmp     [bp+var_CA], 4
jb      short loc_19DF2
jmp     loc_19E91

loc_19DF2:
mov     ax, [bp+var_CA]
or      ax, ax
jz      short loc_19E0C
cmp     ax, 1
jz      short loc_19E1C
cmp     ax, 2
jz      short loc_19E31
cmp     ax, 3
jz      short loc_19E36
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19E0C:
mov     ax, 80h

loc_19E0F:
push    ax
push    [bp+var_4A]
call    sub_18E37

loc_19E16:
add     sp, 4
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19E1C:
mov     ax, 1
push    ax
push    [bp+var_4A]
call    sub_18E37
add     sp, 4
push    [bp+var_4A]
call    sub_18E9C
jmp     short loc_19EAE

loc_19E31:
mov     ax, 2
jmp     short loc_19E0F

loc_19E36:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+52h]
sub     ah, ah
dec     ax
mov     [bp+var_A6], ax
jmp     short loc_19E4F

loc_19E4B:
dec     [bp+var_A6]

loc_19E4F:
cmp     [bp+var_A6], 0
jge     short loc_19E59
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19E59:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
mov     ax, 6
imul    [bp+var_A6]
mov     bx, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
mov     si, ax
mov     bx, _globals
test    byte ptr [bx+si+3Eh], 3
jnz     short loc_19E4B
mov     ax, 0FFh
push    ax
push    [bp+var_A6]
push    [bp+var_4A]
call    sub_2060D
add     sp, 6
jmp     short loc_19E4B

loc_19E91:
mov     ax, [bp+var_CA]
sub     ax, 4
push    ax
call    sub_16D6B
add     sp, 2
or      al, al
jz      short loc_19EB4
mov     ax, [bp+var_CA]
sub     ax, 4
push    ax
call    sub_16DB4

loc_19EAE:
add     sp, 2
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19EB4:
mov     [bp+var_A6], 0
jmp     short loc_19EC0

loc_19EBC:
inc     [bp+var_A6]

loc_19EC0:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
mov     al, [si+52h]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_19ED7
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19ED7:
mov     ax, 6
imul    [bp+var_A6]
mov     bx, ax
mov     al, [bx+si+62h]
sub     ah, ah
mov     cx, [bp+var_CA]
sub     cx, 4
cmp     ax, cx
jnz     short loc_19EBC
mov     ax, 0FFh
push    ax

loc_19EF4:
push    [bp+var_A6]
push    [bp+var_4A]
call    sub_2060D

loc_19F00:
add     sp, 6
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19F06:              ; jumptable 00019A29 case 59
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
mov     [bp+var_A6], 0
jmp     short loc_19F23

loc_19F1F:
inc     [bp+var_A6]

loc_19F23:
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
mov     al, [si+52h]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_19F3A
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19F3A:
mov     ax, 6
imul    [bp+var_A6]
mov     bx, ax
mov     al, [bx+si+62h]
sub     ah, ah
cmp     ax, [bp+var_CA]
jnz     short loc_19F1F
push    [bp+var_C8]
jmp     short loc_19EF4

opcode13:               ; jumptable 00019A29 case 13
mov     ax, [bp+var_A2]
mov     [bp+var_11A], ax
mov     [bp+var_A2], 0
sub     ax, ax
push    ax
call    sub_198E3
add     sp, 2
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+var_118]
mov     dx, [bp+var_116]
mov     [bp+si+var_110], ax
mov     [bp+si+var_10E], dx
mov     si, [bp+var_68]
inc     [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+params0]
add     ax, word ptr [bp+var_138]
mov     dx, word ptr [bp+var_138+2]
mov     [bp+si+var_9C], ax
mov     [bp+si+var_9A], dx
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode14:               ; jumptable 00019A29 case 14
mov     al, _disk1._partyCount
sub     ah, ah
dec     ax
cmp     [bp+var_A2], ax
jnb     short loc_19FCF
inc     [bp+var_A2]
push    [bp+var_A2]
call    sub_198E3
add     sp, 2
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+si+var_114]
mov     dx, [bp+si+var_112]
jmp     loc_199DA

loc_19FCF:
mov     ax, [bp+var_11A]
mov     [bp+var_A2], ax
push    ax
call    sub_198E3
add     sp, 2
dec     [bp+var_68]
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_19FE4:              ; jumptable 00019A29 case 67
dec     [bp+var_68]
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+si+var_9C]
mov     dx, [bp+si+var_9A]
jmp     loc_199DA

opcode15:               ; jumptable 00019A29 case 15
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
sub     ax, ax
push    ax
cmp     byte_2ED26, 0
jz      short loc_1A00E
mov     ax, offset _window7
jmp     short loc_1A011

loc_1A00E:
mov     ax, offset _windowDialog

loc_1A011:              ; win
push    ax
call    setWindow
add     sp, 4
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    sub_18F63
add     sp, 6
call    sub_1DDFD
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
mov     ax, offset asc_223AB ; ">"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     [bp+var_A6], 0
mov     [bp+text], 1Bh
mov     [bp+var_55], 0
mov     [bp+var_72], 0
jmp     short loc_1A083

loc_1A057:
call    sub_1E08C
and     al, 7
cmp     al, 4
jnb     short loc_1A064
mov     al, 1Bh
jmp     short loc_1A066

loc_1A064:
mov     al, 20h ; ' '

loc_1A066:
mov     si, [bp+var_A6]
mov     [bp+si+text], al
mov     ax, 0FFFFh
push    ax              ; y
mov     ax, 1
push    ax              ; x
lea     ax, [bp+text]
push    ss
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_1A083:
call    _kbhit
or      ax, ax
jz      short loc_1A057
call    _getch
mov     [bp+var_58], al
or      al, al
jnz     short loc_1A09B
call    _getch
mov     [bp+var_58], 0

loc_1A09B:
cmp     [bp+var_58], 0Dh
jnz     short loc_1A0AF
mov     si, [bp+var_A6]
mov     [bp+si+text], 0
mov     [bp+var_72], 1
jmp     short loc_1A12B

loc_1A0AF:
cmp     [bp+var_58], 8
jnz     short loc_1A0F5
cmp     [bp+var_A6], 0
jle     short loc_1A12B
mov     si, [bp+var_A6]
mov     [bp+si+text], 20h ; ' '
mov     si, [bp+var_A6]
mov     [bp+si+var_57], 1Bh
mov     ax, 0FFFFh
push    ax              ; y
mov     ax, 1
push    ax              ; x
lea     ax, [bp+text]
push    ss
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     si, [bp+var_A6]
mov     [bp+si+text], 0
dec     [bp+var_A6]
mov     si, [bp+var_A6]
mov     [bp+si+text], 1Bh
jmp     short loc_1A12B

loc_1A0F5:
mov     al, [bp+var_58]
cbw
mov     bx, ax
test    byte ptr [bx-4365h], 7
jnz     short loc_1A106
cmp     al, 20h ; ' '
jnz     short loc_1A12B

loc_1A106:
cmp     [bp+var_A6], 0Ah
jge     short loc_1A12B
mov     si, [bp+var_A6]
mov     al, [bp+var_58]
mov     [bp+si+text], al
inc     [bp+var_A6]
mov     si, [bp+var_A6]
mov     [bp+si+text], 1Bh
mov     si, [bp+var_A6]
mov     [bp+si+var_55], 0

loc_1A12B:
cmp     [bp+var_72], 0
jnz     short loc_1A134
jmp     loc_1A083

loc_1A134:
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
jmp     loc_19E16

opcode47:               ; jumptable 00019A29 case 47
inc     [bp+var_134]
mov     si, [bp+var_68]
shl     si, 1
mov     ax, [bp+var_112]
mov     [bp+si+var_BE], ax
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+var_118]
mov     dx, [bp+var_116]
mov     cl, byte_22BF0
sub     ch, ch
sub     ax, cx
sub     ax, 2
mov     [bp+si+var_110], ax
mov     [bp+si+var_10E], dx
mov     si, [bp+var_68]
inc     [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+var_CA]
add     ax, word ptr [bp+var_138]
mov     dx, word ptr [bp+var_138+2]
mov     [bp+si+var_9C], ax
mov     [bp+si+var_9A], dx
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
sub     ax, ax
push    ax
cmp     byte_2ED26, ch
jz      short loc_1A1A5
mov     ax, offset _window7
jmp     short loc_1A1A8

loc_1A1A5:
mov     ax, offset _windowDialog

loc_1A1A8:              ; win
push    ax
call    setWindow
add     sp, 4
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    sub_18F63
add     sp, 6
cmp     byte_2ED26, 0
jz      short loc_1A1E0
mov     ax, 11
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset aHIreATtackUSe ; "H)ire A)ttack U)se"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1A1E0:
mov     ax, 0Bh
push    ax              ; y
cmp     byte_2ED26, 0
jz      short loc_1A1F0
mov     ax, 13h
jmp     short loc_1A1F3

loc_1A1F0:
mov     ax, 1

loc_1A1F3:              ; x
push    ax
mov     ax, offset aLEave ; "L)eave"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
cmp     byte_2ED26, 0
jz      short loc_1A218
mov     ax, 1
push    ax
push    ax
push    [bp+var_A2]
call    sub_107E1
add     sp, 6

loc_1A218:
mov     [bp+var_E8], 0

loc_1A21E:
cmp     [bp+var_E8], 0
jz      short loc_1A228
jmp     loc_1A134

loc_1A228:
call    getKeypress
mov     [bp+var_58], al
cbw
mov     si, ax
test    byte ptr [si-4365h], 4
jz      short loc_1A245
cmp     al, 30h ; '0'
jz      short loc_1A21E
and     ax, 0Fh
mov     [bp+var_E8], ax
jmp     short loc_1A21E

loc_1A245:
mov     al, [bp+var_58]
cbw
cmp     ax, 41h ; 'A'
jnz     short loc_1A251
jmp     loc_1A36B

loc_1A251:
jle     short loc_1A256
jmp     loc_1A414

loc_1A256:
cmp     ax, 0FFFFh
jz      short loc_1A267
cmp     ax, 3Bh ; ';'
jl      short loc_1A21E
cmp     ax, 3Fh ; '?'
jle     short loc_1A2A2
jmp     short loc_1A21E

loc_1A267:
cmp     [bp+var_134], 1
jle     short loc_1A21E
dec     [bp+var_68]
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+si+var_9C]
mov     dx, [bp+si+var_9A]
mov     [bp+var_118], ax
mov     [bp+var_116], dx
mov     si, [bp+var_68]
shl     si, 1
mov     ax, [bp+si+var_BE]
mov     [bp+var_112], ax
mov     [bp+var_E8], 0FFh
dec     [bp+var_134]
jmp     loc_1A21E

loc_1A2A2:
cmp     byte_2ED26, 0
jnz     short loc_1A2AC
jmp     loc_1A21E

loc_1A2AC:
sub     ax, ax
push    ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+var_A2]
push    ax
mov     al, [bp+var_58]
cbw
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jnz     short loc_1A2CB
jmp     loc_1A21E

loc_1A2CB:
push    [bp+var_A2]
call    sub_198E3
add     sp, 2
mov     ax, 1
push    ax
push    ax
push    [bp+var_A2]
call    sub_107E1
add     sp, 6
jmp     loc_1A21E

loc_1A2E7:
cmp     byte_2ED26, 0
jnz     short loc_1A2F1
jmp     loc_1A21E

loc_1A2F1:
sub     ax, ax
push    ax
push    [bp+arg_8]
push    [bp+var_A2]
mov     ax, 7
push    ax
push    word_2EBF6
push    word_2EBF4
call    doScript
add     sp, 0Ch
or      al, al
jz      short loc_1A319

loc_1A311:
mov     [bp+var_D2], 1
jmp     loc_1A40B

loc_1A319:
mov     ax, offset aIAmNotInterest ; "I am not interested in joining up with "...
jmp     loc_1A3FA

loc_1A31F:
cmp     byte_2ED26, 0
jz      short loc_1A311
mov     ax, 68h ; 'h'
imul    [bp+arg_8]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_60], ax
mov     word ptr [bp+var_60+2], dx
les     bx, [bp+var_60]
test    byte ptr es:[bx+50h], 80h
jz      short loc_1A311
sub     ax, ax
push    ax
mov     al, es:[bx+5Ch]
cbw
push    ax
mov     al, es:[bx+5Bh]
cbw
push    ax
call    sub_12AB2
add     sp, 6
les     bx, [bp+var_60]
mov     byte ptr es:[bx+5Bh], 80h
les     bx, [bp+var_60]
mov     byte ptr es:[bx+5Ch], 80h
jmp     short loc_1A311

loc_1A36B:
cmp     byte_2ED26, 0
jnz     short loc_1A375
jmp     loc_1A21E

loc_1A375:
mov     word_2E418, 8
jmp     short loc_1A311

loc_1A37D:
cmp     byte_2ED26, 0
jnz     short loc_1A387
jmp     loc_1A21E

loc_1A387:
mov     word_2D126, 0
sub     ax, ax
push    ax
push    [bp+var_A2]
call    characterInfo
add     sp, 4
mov     [bp+var_4A], ax
mov     word_2D126, 1
cmp     ax, 0FFFFh
jz      short loc_1A3AC
mov     [bp+var_A2], ax

loc_1A3AC:
push    [bp+var_A2]
call    sub_1B3DF
add     sp, 2
cmp     [bp+var_4A], 0FFFFh
jz      short loc_1A40B
mov     bx, [bp+var_4A]
shl     bx, 1
mov     bx, [bx-2518h]
mov     byte ptr [bx+144h], 2
mov     bx, [bp+var_4A]
shl     bx, 1
mov     si, [bx-2518h]
sub     ax, ax
push    ax
mov     al, [si+145h]
sub     ah, ah
push    ax
push    [bp+var_4A]
mov     al, [si+144h]
push    ax
push    word_2EBF6
push    word_2EBF4
call    doScript
add     sp, 0Ch
or      al, al
jnz     short loc_1A40B
mov     ax, offset aThatDoesnTAffe ; "That doesn't affect me!"

loc_1A3FA:
mov     word ptr [bp+var_64], ax
mov     word ptr [bp+var_64+2], ds
push    ds
push    ax              ; msg
sub     ax, ax
push    ax              ; line
call    showMessage
add     sp, 6

loc_1A40B:
mov     [bp+var_E8], 0FFh
jmp     loc_1A21E

loc_1A414:
cmp     ax, 48h ; 'H'
jnz     short loc_1A41C
jmp     loc_1A2E7

loc_1A41C:
cmp     ax, 4Ch ; 'L'
jnz     short loc_1A424
jmp     loc_1A31F

loc_1A424:
cmp     ax, 55h ; 'U'
jnz     short loc_1A42C
jmp     loc_1A37D

loc_1A42C:
jmp     loc_1A21E

opcode48:               ; jumptable 00019A29 case 48
dec     [bp+var_68]
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+si+var_9C]
mov     dx, [bp+si+var_9A]
mov     [bp+var_118], ax
mov     [bp+var_116], dx
mov     si, [bp+var_68]
shl     si, 1
mov     ax, [bp+si+var_BE]
mov     [bp+var_112], ax
dec     [bp+var_134]
cmp     byte_2ED26, 0
jnz     short loc_1A464
call    sub_1DD4C

loc_1A464:
mov     [bp+var_E8], 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode49:               ; jumptable 00019A29 case 49
dec     [bp+var_68]
mov     si, [bp+var_68]
shl     si, 1
shl     si, 1
mov     ax, [bp+si+var_110]
mov     dx, [bp+si+var_10E]
mov     [bp+var_118], ax
mov     [bp+var_116], dx
dec     [bp+var_134]
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode16:               ; jumptable 00019A29 case 16
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    sub_18F3E
add     sp, 6
mov     word ptr [bp+var_64], ax
mov     word ptr [bp+var_64+2], dx
push    dx
push    ax              ; char *
mov     ax, offset aFs_4 ; "%Fs"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
lea     ax, [bp+text]
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _stricmp
add     sp, 4
or      ax, ax
jnz     short loc_1A4DA
mov     si, [bp+var_112]
mov     [bp+si+var_132], 1
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A4DA:
mov     si, [bp+var_112]
mov     [bp+si+var_132], 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A4E6:              ; jumptable 00019A29 case 55
mov     ax, [bp+params0]
cmp     [bp+var_E8], ax
jnz     short loc_1A4F5
mov     al, 1
jmp     loc_1AC6B

loc_1A4F5:
sub     al, al
jmp     loc_1AC6B

opcode17:               ; jumptable 00019A29 cases 17,70
cmp     [bp+var_CE], 46h ; 'F'
jnz     short loc_1A505
mov     al, 1
jmp     short loc_1A507

loc_1A505:
sub     al, al

loc_1A507:
mov     byte_21329, al
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+var_C8]
push    [bp+var_CA]
push    [bp+params0]
call    sub_1912F

loc_1A527:
add     sp, 0Ah
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode00_41:            ; jumptable 00019A29 cases 0,41,71
cmp     [bp+var_CE], 47h ; 'G'
jnz     short loc_1A538
mov     al, 1
jmp     short loc_1A53A

loc_1A538:
sub     al, al

loc_1A53A:
mov     byte_21329, al
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    sub_18F3E
add     sp, 6
mov     word ptr dword_2EC04, ax
mov     word ptr dword_2EC04+2, dx
les     bx, dword_2EC04
cmp     byte ptr es:[bx], 0
jnz     short loc_1A56E
sub     ax, ax
mov     word ptr dword_2EC04+2, ax
mov     word ptr dword_2EC04, ax

loc_1A56E:
mov     ax, [bp+var_CA]
mov     word_2E206, ax
mov     ax, [bp+var_C8]
mov     word_2F018, ax
mov     ax, [bp+param3]
mov     word_2D128, ax
cmp     [bp+var_CE], 1
sbb     ax, ax
inc     ax
mov     word_2E41C, ax
call    sub_192D5
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode18:               ; jumptable 00019A29 case 18
mov     [bp+var_A6], 0
jmp     short loc_1A5A0

loc_1A59C:
inc     [bp+var_A6]

loc_1A5A0:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A6], ax
jb      short loc_1A5AE
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A5AE:
push    [bp+params0]
push    [bp+params0]
push    [bp+var_A6]
call    sub_19298
add     sp, 6
jmp     short loc_1A59C

opcode35:               ; jumptable 00019A29 case 35
mov     bx, [bp+var_A2]
shl     bx, 1
mov     si, [bx-2518h]
mov     ax, 6
imul    [bp+arg_8]
mov     bx, ax
mov     al, [bx+si+62h]
sub     ah, ah
cmp     ax, [bp+params0]
jnz     short loc_1A5F4
jmp     loc_1AF68

opcode36:               ; jumptable 00019A29 case 36
mov     ax, [bp+params0]
cmp     [bp+arg_8], ax
jnz     short loc_1A5F4
cmp     [bp+arg_4], 3
jnz     short loc_1A5F4
jmp     loc_1AF68

loc_1A5F4:
sub     al, al

loc_1A5F6:
mov     [bp+var_66], al

loc_1A5F9:
push    [bp+var_CA]
lea     ax, [bp+var_48]
push    ax
mov     al, [bp+var_66]
jmp     loc_1AC5E

opcode19:               ; jumptable 00019A29 case 19
cmp     [bp+params0], 12
jb      short loc_1A611
jmp     loc_1A6E8

loc_1A611:
cmp     [bp+params0], 6
jnb     short loc_1A61E
mov     ax, [bp+var_C8]
jmp     short loc_1A622

loc_1A61E:
mov     ax, [bp+var_11C]

loc_1A622:
mov     [bp+var_A0], ax
mov     [bp+var_9E], 0
mov     ax, [bp+params0]
sub     dx, dx
mov     cx, 6
div     cx
mov     ax, dx
or      ax, ax
jz      short loc_1A659
cmp     ax, 1
jz      short loc_1A676
cmp     ax, 2
jz      short loc_1A692
cmp     ax, 3
jz      short loc_1A6A8
cmp     ax, 4
jz      short loc_1A6BE
cmp     ax, 5
jz      short loc_1A6D4
jmp     loc_19D6C

loc_1A659:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5C], ax
jz      short loc_1A669
jmp     loc_1A719

loc_1A669:
cmp     [bp+var_5A], dx

loc_1A66C:
jz      short loc_1A671
jmp     loc_1A719

loc_1A671:
mov     al, 1
jmp     loc_19D31

loc_1A676:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5A], dx
jge     short loc_1A686
jmp     loc_1A719

loc_1A686:
jg      short loc_1A671
cmp     [bp+var_5C], ax
ja      short loc_1A690
jmp     loc_1A719

loc_1A690:
jmp     short loc_1A671

loc_1A692:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5A], dx
jg      short loc_1A719
jl      short loc_1A671
cmp     [bp+var_5C], ax
jnb     short loc_1A719
jmp     short loc_1A671

loc_1A6A8:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5A], dx
jl      short loc_1A719
jg      short loc_1A671
cmp     [bp+var_5C], ax
jb      short loc_1A719
jmp     short loc_1A671

loc_1A6BE:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5A], dx
jg      short loc_1A719
jl      short loc_1A671
cmp     [bp+var_5C], ax
ja      short loc_1A719
jmp     short loc_1A671

loc_1A6D4:
mov     ax, [bp+var_A0]
mov     dx, [bp+var_9E]
cmp     [bp+var_5C], ax
jnz     short loc_1A671
cmp     [bp+var_5A], dx
jnz     short loc_1A671
jmp     short loc_1A719

loc_1A6E8:
cmp     [bp+params0], 0Fh
jnb     short loc_1A703
mov     ax, [bp+params0]
sub     ax, 0Ch
sub     dx, dx
cmp     ax, [bp+var_5C]
jnz     short loc_1A719
cmp     dx, [bp+var_5A]
jmp     loc_1A66C

loc_1A703:
mov     ax, [bp+params0]
sub     ax, 0Fh
sub     dx, dx
cmp     ax, [bp+var_5C]
jnz     short loc_1A719
cmp     dx, [bp+var_5A]
jnz     short loc_1A719
jmp     loc_1A671

loc_1A719:
sub     al, al
jmp     loc_19D31

opcode20:               ; jumptable 00019A29 case 20
mov     ax, [bp+params0]
or      ax, ax
jz      short loc_1A742
cmp     ax, 1
jz      short loc_1A761
cmp     ax, 2
jz      short loc_1A742
cmp     ax, 3
jz      short loc_1A761
cmp     ax, 4
jz      short loc_1A742
cmp     ax, 5
jz      short loc_1A761
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A742:
mov     si, [bp+var_112]
cmp     [bp+si+var_132], 0
jnz     short loc_1A76C

loc_1A74D:
mov     [bp+si+var_E6], 1
mov     ax, [bp+var_CA]

loc_1A756:
add     ax, word ptr [bp+var_138]
mov     dx, word ptr [bp+var_138+2]
jmp     loc_199DA

loc_1A761:
mov     si, [bp+var_112]
cmp     [bp+si+var_132], 0
jnz     short loc_1A74D

loc_1A76C:
mov     [bp+si+var_E6], 0

loc_1A771:
inc     [bp+var_112]
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode21:               ; jumptable 00019A29 case 21
mov     si, [bp+var_112]
cmp     [bp+si+var_E6], 0
jnz     short loc_1A771
mov     ax, [bp+params0]
jmp     short loc_1A756

opcode22:               ; jumptable 00019A29 case 22
cmp     [bp+var_112], 0
jg      short loc_1A793
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A793:
dec     [bp+var_112]
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode23:               ; jumptable 00019A29 case 23
call    sub_1DDFD
call    sub_1DDFD
mov     byte_22A74, 1
mov     bx, _currentWindow
mov     word ptr [bx+18h], 0
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    sub_18F3E
add     sp, 6
mov     word ptr [bp+var_64], ax
mov     word ptr [bp+var_64+2], dx
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
push    dx
push    word ptr [bp+var_64] ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28
mov     byte_22A74, 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A7E6:              ; jumptable 00019A29 case 57
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax              ; msg
push    [bp+params0]    ; line
call    showMessage
jmp     loc_19F00

opcode25:               ; jumptable 00019A29 case 25
sub     ax, ax
push    ax
push    [bp+params0]
mov     ax, 19h

loc_1A808:
push    ax
push    word ptr dword_2E4C6+2
push    word ptr dword_2E4C6
call    sub_18BBB
jmp     loc_1A527

opcode26:               ; jumptable 00019A29 case 26
push    [bp+var_CA]
push    [bp+params0]
mov     ax, 1Ah
jmp     short loc_1A808

opcode27:               ; jumptable 00019A29 case 27
sub     ax, ax
push    ax
push    [bp+params0]
mov     ax, 1Bh
jmp     short loc_1A808

opcode24:               ; jumptable 00019A29 case 24
push    [bp+var_C8]
push    [bp+var_CA]
mov     ax, [bp+params0]
dec     ax
push    ax
call    sub_182CA
add     sp, 6
mov     byte_2EE06, 1
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode50:               ; jumptable 00019A29 case 50
mov     [bp+var_A6], 0
mov     [bp+var_C0], 0
jmp     short loc_1A89C

loc_1A85A:
mov     ax, 6
imul    [bp+var_A6]
add     ax, word ptr dword_2EC1A
mov     dx, word ptr dword_2EC1A+2
mov     word ptr [bp+var_13C], ax
mov     word ptr [bp+var_13C+2], dx
les     bx, [bp+var_13C]
mov     al, es:[bx+2]
sub     ah, ah
and     ax, 7Fh
cmp     ax, [bp+params0]
jnz     short loc_1A898
mov     al, es:[bx+3]
cbw
cmp     ax, [bp+var_CA]
jnz     short loc_1A898
and     byte ptr es:[bx+2], 7Fh
inc     [bp+var_C0]

loc_1A898:
inc     [bp+var_A6]

loc_1A89C:
les     bx, dword_22C6E
mov     ax, es:[bx+40Eh]
cmp     [bp+var_A6], ax
jb      short loc_1A85A
cmp     [bp+var_C0], 0
jg      short loc_1A8B5
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A8B5:
mov     byte_2EE06, 1
call    sub_17E09
push    [bp+var_CA]
push    [bp+params0]
call    sub_17E13
jmp     loc_19E16

loc_1A8CB:              ; jumptable 00019A29 case 61
mov     si, [bp+var_112]
mov     [bp+si+var_132], 0
mov     [bp+var_A6], 0
jmp     short loc_1A8E0

loc_1A8DC:
inc     [bp+var_A6]

loc_1A8E0:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_1A8F3
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A8F3:
mov     ax, 66h ; 'f'
imul    [bp+var_A6]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_AA], ax
mov     word ptr [bp+var_AA+2], dx
les     bx, [bp+var_AA]
mov     al, es:[bx]
cbw
cmp     ax, [bp+params0]
jnz     short loc_1A8DC
mov     al, es:[bx+1]
cbw
cmp     ax, [bp+var_CA]
jnz     short loc_1A8DC
mov     al, es:[bx+9]
and     al, 0Fh
jmp     loc_1AC6B

loc_1A92C:              ; jumptable 00019A29 case 53
mov     ax, _disk1._mapPosX
cmp     [bp+params0], ax
jnz     short loc_1A941
mov     ax, _disk1._mapPosY
cmp     [bp+var_CA], ax
jnz     short loc_1A941
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A941:
les     bx, dword_22C6E
cmp     byte ptr es:[bx+7], 0
jz      short loc_1A95E
cmp     byte ptr es:[bx+8], 0
jz      short loc_1A95E
mov     al, es:[bx+8]
sub     ah, ah
add     ax, 8
jmp     short loc_1A960

loc_1A95E:
sub     ax, ax

loc_1A960:
mov     [bp+var_C0], ax
mov     [bp+var_A6], ax
jmp     short loc_1A96E

loc_1A96A:
inc     [bp+var_A6]

loc_1A96E:
les     bx, dword_22C6E
mov     al, es:[bx+0Bh]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_1A981
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1A981:
mov     ax, 66h ; 'f'
imul    [bp+var_A6]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     word ptr [bp+var_AA], ax
mov     word ptr [bp+var_AA+2], dx
les     bx, [bp+var_AA]
test    byte ptr es:[bx+9], 0Fh
jnz     short loc_1A96A
mov     al, es:[bx]
cbw
cmp     ax, [bp+params0]
jnz     short loc_1A96A
mov     al, es:[bx+1]
cbw
cmp     ax, [bp+var_CA]
jnz     short loc_1A96A
push    dx
push    bx
call    sub_17470
jmp     loc_19E16

opcode31:               ; jumptable 00019A29 case 31
push    [bp+params0]
call    sub_21130
jmp     loc_19EAE

opcode29:               ; jumptable 00019A29 case 29
push    [bp+var_C4]
jmp     short loc_1A9D5

opcode28_30:            ; jumptable 00019A29 cases 28,30
sub     ax, ax
push    ax

loc_1A9D5:
push    [bp+param3]
push    [bp+var_CA]
push    [bp+params0]
push    [bp+var_C8]
push    [bp+var_CE]
call    sub_18C32
jmp     loc_19B5C

opcode12_moveParty:     ; jumptable 00019A29 case 12
cmp     [bp+param3], 0
jnz     short loc_1AA06
mov     ax, offset aEnterNewLocati ; "Enter new location?"
push    ds
push    ax
sub     ax, ax
push    ax
call    YesNo
add     sp, 6
jmp     short loc_1AA08

loc_1AA06:
mov     al, 1

loc_1AA08:
mov     [bp+var_66], al
or      al, al
jz      short loc_1AA8B
mov     ax, [bp+var_C8]
or      ax, ax
jz      short loc_1AA51
cmp     ax, 1
jz      short loc_1AA97
call    sub_18827
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, byte ptr _disk1._mapPosX
mov     [bx-40FAh], al
mov     bl, _disk1._mapIndex
mov     al, byte ptr _disk1._mapPosY
mov     [bx-40F0h], al
mov     ax, [bp+var_C8]
dec     ax
mov     [bp+var_A0], ax
mov     [bp+var_9E], 0
mov     [bp+var_A6], 0
mov     [bp+var_66], bh
jmp     short loc_1AAA5

loc_1AA51:
cmp     _disk1._mapIndex, 0
jz      short loc_1AA8B
call    sub_18827
dec     _disk1._mapIndex
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, [bx-4104h]
sub     ah, ah
push    ax

loc_1AA6C:
call    sub_1008C
add     sp, 2
mov     al, _disk1._mapIndex
sub     ah, ah
mov     si, ax
mov     al, [si-40F0h]
cbw
push    ax
mov     al, [si-40FAh]
cbw
push    ax

loc_1AA85:
call    sub_1251E
add     sp, 4

loc_1AA8B:
cmp     [bp+param3], 2
jnz     short loc_1AB0F
call    sub_198B1
jmp     short loc_1AB15

loc_1AA97:
push    [bp+var_CA]
push    [bp+params0]
jmp     short loc_1AA85

loc_1AAA1:
inc     [bp+var_A6]

loc_1AAA5:
mov     al, _disk1._mapIndex
sub     ah, ah
inc     ax
cmp     [bp+var_A6], ax
jnb     short loc_1AACD
mov     bx, [bp+var_A6]
mov     al, [bx-4104h]
sub     ah, ah
sub     dx, dx
cmp     ax, [bp+var_A0]
jnz     short loc_1AAA1
cmp     dx, [bp+var_9E]
jnz     short loc_1AAA1
mov     [bp+var_66], 1

loc_1AACD:
cmp     [bp+var_66], 0
jz      short loc_1AADC
mov     al, byte ptr [bp+var_A6]
mov     _disk1._mapIndex, al
jmp     short loc_1AAEE

loc_1AADC:
inc     _disk1._mapIndex
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, byte ptr [bp+var_A0]
mov     [bx-4104h], al

loc_1AAEE:
mov     bl, _disk1._mapIndex
sub     bh, bh
mov     al, byte ptr [bp+params0]
mov     [bx-40FAh], al
mov     bl, _disk1._mapIndex
mov     al, byte ptr [bp+var_CA]
mov     [bx-40F0h], al
push    [bp+var_A0]
jmp     loc_1AA6C

loc_1AB0F:
mov     word_2E418, 0FFFFh

loc_1AB15:
mov     byte_22BB9, 0
cmp     byte_2ED26, 0
jnz     short loc_1AB24
call    sub_1DD4C

loc_1AB24:
mov     ax, 1
jmp     loc_1B24A

opcode32:               ; jumptable 00019A29 case 32
push    [bp+var_CA]
push    [bp+params0]
call    sub_1DEB1
add     sp, 4
jmp     loc_19B20

opcode34:               ; jumptable 00019A29 case 34
mov     ax, [bp+params0]
mov     [bp+var_A4], ax
cmp     ax, 0Ch
jnb     short loc_1AB4F
mov     [bp+var_46], 4
jmp     short loc_1AB6C

loc_1AB4F:
cmp     [bp+params0], 1Ch
jnb     short loc_1AB62
sub     [bp+var_A4], 0Ch
mov     [bp+var_46], 5
jmp     short loc_1AB6C

loc_1AB62:
sub     [bp+var_A4], 1Ch
mov     [bp+var_46], 6

loc_1AB6C:
cmp     [bp+arg_4], 3
jz      short loc_1AB75
jmp     loc_1AC76

loc_1AB75:
mov     si, [bp+var_112]
mov     [bp+si+var_132], 0
mov     ax, [bp+params0]
cmp     [bp+arg_8], ax
jz      short loc_1AB8A
jmp     loc_1AC4E

loc_1AB8A:
push    [bp+var_A2]
push    [bp+var_A4]
push    [bp+var_46]
sub     ax, ax
push    ax
call    sub_19335
add     sp, 8
mov     [bp+var_6A], ax
mov     ax, [bp+var_CA]
cmp     [bp+var_6A], ax
jb      short loc_1ABE4
cmp     [bp+var_46], 4
jz      short loc_1ABD9
push    [bp+var_A4]
push    [bp+var_A2]
call    sub_1969F
add     sp, 4
or      al, al
jz      short loc_1ABD9
sub     ax, ax
push    ax              ; char
push    [bp+var_A4]
mov     bx, [bp+var_A2]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
call    sub_16F9D
add     sp, 6

loc_1ABD9:
mov     si, [bp+var_112]
mov     [bp+si+var_132], 1
jmp     short loc_1AC4E

loc_1ABE4:
cmp     [bp+var_46], 5
jnz     short loc_1AC4E
mov     bx, [bp+var_A2]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+1Ch]
sub     ah, ah
push    ax
call    sub_16F3A
add     sp, 2
shl     ax, 1
mov     [bp+var_6A], ax
or      ax, ax
jle     short loc_1AC4E
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_6A], ax
jb      short loc_1AC4E
mov     si, [bp+var_112]
mov     [bp+si+var_132], 1
push    [bp+var_A4]
push    [bp+var_A2]
call    sub_1969F
add     sp, 4
or      al, al
jz      short loc_1AC4E
sub     ax, ax
push    ax              ; char
push    [bp+var_A4]
mov     bx, [bp+var_A2]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
call    sub_16F9D
add     sp, 6

loc_1AC4E:
push    [bp+var_C8]

loc_1AC52:
lea     ax, [bp+var_48]
push    ax
mov     si, [bp+var_112]
mov     al, [bp+si+var_132]

loc_1AC5E:
sub     ah, ah

loc_1AC60:
push    ax
lea     ax, [bp+var_74]
push    ax
call    sub_1972A
add     sp, 8

loc_1AC6B:
mov     si, [bp+var_112]
mov     [bp+si+var_132], al
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AC76:
push    [bp+var_A2]
push    [bp+var_A4]
push    [bp+var_46]
mov     ax, 1
push    ax
call    sub_19335
add     sp, 8
mov     [bp+var_6A], ax
sub     ax, ax
push    ax
call    sub_198E3
add     sp, 2
mov     [bp+var_A6], 1
jmp     short loc_1ACE5

loc_1AC9F:
push    [bp+var_A2]
push    [bp+var_A4]
push    [bp+var_46]
mov     ax, [bp+var_A6]
inc     ax
push    ax
call    sub_19335
add     sp, 8
mov     [bp+var_A0], ax
mov     [bp+var_9E], dx
mov     ax, [bp+var_6A]
cwd
cmp     dx, [bp+var_9E]
jg      short loc_1ACE1
jl      short loc_1ACD0
cmp     ax, [bp+var_A0]
jnb     short loc_1ACE1

loc_1ACD0:
mov     ax, [bp+var_A0]
mov     [bp+var_6A], ax
push    [bp+var_A6]
call    sub_198E3
add     sp, 2

loc_1ACE1:
inc     [bp+var_A6]

loc_1ACE5:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A6], ax
jb      short loc_1AC9F
mov     ax, [bp+var_CA]
cmp     [bp+var_6A], ax
jb      short loc_1ACFD
mov     al, 1
jmp     short loc_1ACFF

loc_1ACFD:
sub     al, al

loc_1ACFF:
mov     si, [bp+var_112]
mov     [bp+si+var_132], al
mov     si, [bp+var_112]
cmp     [bp+si+var_132], 0
jnz     short loc_1AD42
cmp     [bp+var_46], 5
jnz     short loc_1AD22
mov     ax, 4
push    ax
push    [bp+var_A4]
jmp     short loc_1AD34

loc_1AD22:
cmp     [bp+var_46], 6
jnz     short loc_1AD42
mov     ax, 1
push    ax
mov     ax, [bp+var_A4]
add     ax, 10h
push    ax

loc_1AD34:
call    sub_17015
add     sp, 4
mov     si, [bp+var_112]
mov     [bp+si+var_132], al

loc_1AD42:
push    [bp+var_C8]
lea     ax, [bp+var_48]
push    ax
mov     di, [bp+var_112]
mov     al, [bp+di+var_132]
sub     ah, ah
push    ax
lea     ax, [bp+var_74]
push    ax
call    sub_1972A
add     sp, 8
mov     di, [bp+var_112]
mov     [bp+di+var_132], al
mov     di, [bp+var_112]
cmp     [bp+di+var_132], 0
jnz     short loc_1AD75
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AD75:
cmp     [bp+var_46], 4
jz      short loc_1AD95
mov     si, [bp+var_44]
shl     si, 1
add     si, bp
mov     al, byte ptr [bp+params0]
sub     al, 0Ch
mov     [si-42h], al
mov     al, byte ptr [bp+var_CA]
mov     [si-41h], al
inc     [bp+var_44]

loc_1AD95:
cmp     [bp+var_C8], 0
jz      short loc_1AD9F
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AD9F:
mov     [bp+var_A6], 0
jmp     short loc_1AE23

loc_1ADA7:
inc     [bp+var_C0]

loc_1ADAB:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_C0], ax
jnb     short loc_1AE1F
mov     si, [bp+var_A6]
shl     si, 1
add     si, bp
push    [bp+var_C0]
mov     al, [si-42h]
push    ax
mov     ax, 5
push    ax
sub     ax, ax
push    ax
call    sub_19335
add     sp, 8
mov     cl, [si-41h]
sub     ch, ch
sub     bx, bx
cmp     bx, dx
ja      short loc_1ADA7
jb      short loc_1ADE4
cmp     cx, ax
ja      short loc_1ADA7

loc_1ADE4:
mov     di, [bp+var_A6]
shl     di, 1
mov     al, [bp+di+var_42]
sub     ah, ah
push    ax
push    [bp+var_C0]
call    sub_1969F
add     sp, 4
or      al, al
jz      short loc_1ADA7
sub     ax, ax
push    ax              ; char
mov     di, [bp+var_A6]
shl     di, 1
mov     al, [bp+di+var_42]
sub     ah, ah
push    ax
mov     bx, [bp+var_C0]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
call    sub_16F9D
add     sp, 6
jmp     short loc_1ADA7

loc_1AE1F:
inc     [bp+var_A6]

loc_1AE23:
mov     ax, [bp+var_44]
cmp     [bp+var_A6], ax
jge     short loc_1AE35
mov     [bp+var_C0], 0
jmp     loc_1ADAB

loc_1AE35:
mov     [bp+var_44], 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode37:               ; jumptable 00019A29 case 37
push    [bp+params0]
call    sub_1BA26
jmp     loc_19EAE

opcode45:               ; jumptable 00019A29 case 45
push    [bp+params0]
call    sub_1BBC8
jmp     loc_19EAE

opcode40:               ; jumptable 00019A29 case 40
mov     al, byte_2E428
cbw
push    ax
mov     al, byte_2E423
cbw
push    ax
call    sub_1251E
jmp     loc_19E16

opcode38:               ; jumptable 00019A29 case 38
mov     byte_2DFEE, 0
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode39:               ; jumptable 00019A29 case 39
les     bx, [bp+var_138]
mov     ax, es:[bx+2]
add     ax, bx
mov     dx, es
push    dx
push    ax
push    [bp+params0]
call    YesNo
add     sp, 6
jmp     loc_1AC6B

opcode42:               ; jumptable 00019A29 case 42
mov     [bp+var_A6], 0
jmp     short loc_1AE90

loc_1AE8C:
inc     [bp+var_A6]

loc_1AE90:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_A6]
ja      short loc_1AEA3
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AEA3:
mov     ax, 68h ; 'h'
imul    [bp+var_A6]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
add     ax, 5Dh ; ']'
mov     word ptr [bp+var_13C], ax
mov     word ptr [bp+var_13C+2], dx
les     bx, [bp+var_13C]
mov     al, es:[bx]
sub     ah, ah
cmp     ax, [bp+params0]
jnz     short loc_1AE8C
mov     al, byte ptr [bp+var_CA]
mov     es:[bx], al
jmp     short loc_1AE8C

loc_1AED5:              ; jumptable 00019A29 case 54
mov     si, [bp+var_112]
mov     [bp+si+var_132], 0
lea     ax, [bp+var_A4]
push    ax
push    [bp+params0]
call    sub_18B08
add     sp, 4
mov     word ptr [bp+var_60], ax
mov     word ptr [bp+var_60+2], dx
or      dx, ax
jz      short loc_1AF0F ; jumptable 00019A29 case 68
les     bx, [bp+var_60]
mov     bl, es:[bx+49h]
sub     bh, bh
test    byte ptr [bx-3A60h], 3
jnz     short loc_1AF0F ; jumptable 00019A29 case 68
push    es
push    ax
call    sub_1D712
add     sp, 4

loc_1AF0F:              ; jumptable 00019A29 case 68
mov     [bp+var_D2], 1
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode43:               ; jumptable 00019A29 case 43
mov     bx, [bp+params0]
mov     al, [bx-3A60h]
and     al, 1
mov     [bp+var_66], al
or      al, al
jnz     short loc_1AF2B
jmp     loc_1A5F9

loc_1AF2B:
mov     [bp+var_A6], 0
jmp     short loc_1AF37

loc_1AF33:
inc     [bp+var_A6]

loc_1AF37:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_A6], ax
jnb     short loc_1AF55
mov     bx, [bp+var_A6]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+49h]
cmp     ax, [bp+params0]
jnz     short loc_1AF33

loc_1AF55:
mov     bx, [bp+var_A6]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 0
jg      short loc_1AF68
jmp     loc_1A5F4

loc_1AF68:
mov     al, 1
jmp     loc_1A5F6

opcode44:               ; jumptable 00019A29 case 44
push    [bp+var_CA]
lea     ax, [bp+var_48]
push    ax
mov     bx, [bp+params0]
mov     al, [bx-3A60h]
sub     ah, ah
and     ax, 2
jmp     loc_1AC60

loc_1AF85:              ; jumptable 00019A29 cases 52,75
lea     ax, [bp+var_A4]
push    ax
push    [bp+params0]
call    sub_18B08
add     sp, 4
mov     word ptr [bp+var_60], ax
mov     word ptr [bp+var_60+2], dx
or      dx, ax
jnz     short loc_1AFA1
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AFA1:
les     bx, [bp+var_60]
mov     bl, es:[bx+49h]
sub     bh, bh
test    byte ptr [bx-3A60h], 2
jz      short loc_1AFB4
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1AFB4:
cmp     [bp+var_CE], 34h ; '4'
jnz     short loc_1B00C
sub     ax, ax
push    ax
mov     bx, word ptr [bp+var_60]
mov     al, es:[bx+5Ch]
cbw
push    ax
mov     al, es:[bx+5Bh]
cbw
push    ax
call    sub_12AB2
add     sp, 6
les     bx, [bp+var_60]
mov     al, byte ptr [bp+var_CA]
mov     es:[bx+5Bh], al
les     bx, [bp+var_60]
mov     al, byte ptr [bp+var_C8]
mov     es:[bx+5Ch], al
les     bx, [bp+var_60]
mov     al, es:[bx+65h]
sub     ah, ah
push    ax
mov     al, es:[bx+5Ch]
cbw
push    ax
mov     al, es:[bx+5Bh]
cbw
push    ax
mov     al, es:[bx+5Fh]
sub     ah, ah
push    ax
call    sub_10D19
add     sp, 8

loc_1B00C:
les     bx, [bp+var_60]
mov     byte ptr es:[bx+60h], 0FFh
call    sub_17E09
cmp     [bp+param3], 1
jz      short loc_1B028
cmp     [bp+var_CE], 4Bh ; 'K'
jz      short loc_1B028
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B028:
cmp     [bp+param3], 1
jnz     short loc_1B037
les     bx, [bp+var_60]
or      byte ptr es:[bx+50h], 80h

loc_1B037:
mov     ax, word ptr [bp+var_60]
mov     dx, word ptr [bp+var_60+2]
add     ax, 66h ; 'f'
mov     word_2EBF4, ax
mov     word_2EBF6, dx
mov     word_2E418, 6
mov     ax, [bp+var_A2]
mov     word_2EC10, ax
mov     ax, [bp+var_A4]
mov     word_2D100, ax
mov     byte_2ED26, 0
jmp     loc_1AF0F       ; jumptable 00019A29 case 68

loc_1B063:              ; jumptable 00019A29 case 58
lea     ax, [bp+var_A4]
push    ax
push    [bp+params0]
call    sub_18B08
add     sp, 4
mov     word ptr [bp+var_60], ax
mov     word ptr [bp+var_60+2], dx
or      dx, ax
jnz     short loc_1B07F
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B07F:
les     bx, [bp+var_60]
mov     ax, [bp+var_CA]
mov     es:[bx+66h], ax
call    sub_17E09
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

opcode46:               ; jumptable 00019A29 case 46
mov     al, 64h ; 'd'
mul     _disk1._timeHours
mov     cl, _disk1._timeMinutes
sub     ch, ch
add     ax, cx
jmp     loc_19B20

loc_1B0A1:              ; jumptable 00019A29 case 60
mov     al, _disk1._timeWeekday
sub     ah, ah
jmp     loc_19B20

loc_1B0A9:              ; jumptable 00019A29 case 62
mov     ax, [bp+params0]
inc     ax
push    ax
call    sub_1DFC6
add     sp, 2
mov     si, [bp+params0]
shl     si, 1
mov     al, [si+1DFh]
cbw
add     ax, _disk1._mapPosY
push    ax
mov     al, [si+1DEh]
cbw
add     ax, _disk1._mapPosX
push    ax
call    sub_1251E
add     sp, 4
cmp     byte_2ED26, 0
jnz     short loc_1B0DF
call    sub_1DD4C

loc_1B0DF:
call    sub_198B1
jmp     loc_1AF0F       ; jumptable 00019A29 case 68

loc_1B0E5:              ; jumptable 00019A29 case 63
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
push    [bp+var_C8]
lea     ax, [bp+var_48]
push    ax
mov     bx, [bp+var_4A]
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_CA]
shl     dx, cl
and     ax, dx
jmp     loc_1AC60

loc_1B11A:              ; jumptable 00019A29 case 64
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
mov     bx, ax
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, 1
mov     cl, byte ptr [bp+var_CA]
shl     al, cl
or      [bx+58h], al
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B141:              ; jumptable 00019A29 case 65
push    [bp+var_A2]
push    [bp+params0]
call    sub_18E23
add     sp, 4
mov     [bp+var_4A], ax
mov     bx, ax
shl     bx, 1
mov     bx, [bx-2518h]
mov     al, 1
mov     cl, byte ptr [bp+var_CA]
shl     al, cl
and     [bx+58h], al
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B168:              ; jumptable 00019A29 case 69
mov     ax, [bp+params0]
mov     word_22C86, ax
push    word_22C88
call    sub_1DFC6
add     sp, 2
call    sub_1DFEE
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B17F:              ; jumptable 00019A29 case 72
call    sub_17140
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B185:              ; jumptable 00019A29 case 73
push    [bp+var_C8]
push    [bp+var_CA]
push    [bp+params0]
call    sub_12AB2
add     sp, 6
call    sub_1DD4C
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B19D:              ; jumptable 00019A29 case 74
push    [bp+params0]
call    sub_18A56
jmp     loc_19EAE
jpt_19A29 dw offset opcode00_41 ; jump table for switch statement
dw offset opcode01
dw offset opcode02
dw offset opcode03
dw offset opcode04
dw offset opcode05
dw offset opcode06
dw offset opcode07
dw offset opcode08
dw offset opcode09
dw offset opcode10
dw offset opcode11
dw offset opcode12_moveParty
dw offset opcode13
dw offset opcode14
dw offset opcode15
dw offset opcode16
dw offset opcode17
dw offset opcode18
dw offset opcode19
dw offset opcode20
dw offset opcode21
dw offset opcode22
dw offset opcode23
dw offset opcode24
dw offset opcode25
dw offset opcode26
dw offset opcode27
dw offset opcode28_30
dw offset opcode29
dw offset opcode28_30
dw offset opcode31
dw offset opcode32
dw offset def_19A29
dw offset opcode34
dw offset opcode35
dw offset opcode36
dw offset opcode37
dw offset opcode38
dw offset opcode39
dw offset opcode40
dw offset opcode00_41
dw offset opcode42
dw offset opcode43
dw offset opcode44
dw offset opcode45
dw offset opcode46
dw offset opcode47
dw offset opcode48
dw offset opcode49
dw offset opcode50
dw offset def_19A29
dw offset loc_1AF85
dw offset loc_1A92C
dw offset loc_1AED5
dw offset loc_1A4E6
dw offset loc_19C6E
dw offset loc_1A7E6
dw offset loc_1B063
dw offset loc_19F06
dw offset loc_1B0A1
dw offset loc_1A8CB
dw offset loc_1B0A9
dw offset loc_1B0E5
dw offset loc_1B11A
dw offset loc_1B141
dw offset loc_19D73
dw offset loc_19FE4
dw offset loc_1AF0F
dw offset loc_1B168
dw offset opcode17
dw offset opcode00_41
dw offset loc_1B17F
dw offset loc_1B185
dw offset loc_1B19D
dw offset loc_1AF85
jmp     def_19A29       ; jumptable 00019A29 default case, cases 33,51
                        ; jumptable 00019A3D default case

loc_1B242:
mov     byte_22BB9, 0
jmp     loc_1AB24

loc_1B24A:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
doScript endp

push    bp
mov     bp, sp
sub     sp, 2
push    di
push    si
mov     di, [bp+8]
mov     bx, [bp+4]
mov     al, [bp+6]
add     [bx+di+18h], al
mov     si, [bp+8]
add     si, [bp+4]
add     si, 18h
mov     bx, [bp+4]
mov     di, [bx+48h]
and     di, 0FFh
mov     cl, 7
shl     di, cl
add     di, [bp+8]
les     bx, _professions
mov     al, es:[bx+di+2Ch]
mov     [bp-2], al
mov     al, [si]
cmp     [bp-2], al
jnb     short loc_1B295
mov     al, [bp-2]
mov     [si], al

loc_1B295:
pop     si
pop     di
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_1B29B proc near

text= dword ptr  4

push    bp
mov     bp, sp
push    si              ; draw
mov     ax, 1
push    ax
cmp     byte_2ED26, 0
jz      short loc_1B2AF
mov     ax, offset _window7
jmp     short loc_1B2B2

loc_1B2AF:
mov     ax, offset _windowDialog

loc_1B2B2:              ; win
push    ax
call    setWindow
add     sp, 4
call    winClearCurrent
mov     bx, _currentWindow
mov     si, bx
mov     ax, [si+6]
sub     ax, [si+2]
mov     [bx+18h], ax
mov     byte_22A74, 1
sub     ax, ax
push    ax              ; y
push    ax              ; x
push    ds
push    word ptr [bp+text] ; text
call    writeStringAt
add     sp, 8
call    sub_1DC28

loc_1B2E1:
call    getKeypress
cmp     al, 0FEh
jnz     short loc_1B2E1
mov     byte_22A74, 0
pop     si
pop     bp
retn
sub_1B29B endp



; Attributes: bp-based frame

; int __cdecl sub_1B2F0(char *, char)
sub_1B2F0 proc near

win= dword ptr -2
text= dword ptr  4
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
sub     ax, ax
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
push    word ptr [bp+text] ; char *
call    _strlen
add     sp, 2
mov     bx, _currentWindow
mov     cx, [bx+4]
sub     cx, [bx]
cmp     cx, ax
jle     short loc_1B333
mov     al, [bp+arg_4]
cbw
push    ax              ; int
push    ds
push    word ptr [bp+text] ; member
call    writeShortMemberName
add     sp, 6
jmp     short loc_1B347

loc_1B333:
mov     al, [bp+arg_4]
cbw
push    ax              ; y
mov     al, byte ptr [bp+text+2]
cbw
push    ax              ; x
push    ds
push    word ptr [bp+text] ; text
call    writeStringAt
add     sp, 8

loc_1B347:
call    sub_1DC28
call    getKeypress
call    winClearCurrent
call    sub_1DC28
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
mov     sp, bp
pop     bp
retn
sub_1B2F0 endp



; Attributes: bp-based frame

; int __cdecl sub_1B360(int, char *)
sub_1B360 proc near

arg_0= word ptr  4
member= dword ptr  6

push    bp              ; draw
mov     bp, sp
mov     word_2F01C, 1
mov     ax, win
mov     word_2D102, ax
mov     ax, [bp+arg_0]
mov     word_2F020, ax
mov     ax, word ptr [bp+member]
mov     dx, word ptr [bp+member+2]
mov     word ptr dword_2DFF0, ax
mov     word ptr dword_2DFF0+2, dx
sub     ax, ax
push    ax
call    sub_18880
add     sp, 2
sub     ax, ax
push    ax
mov     ax, offset _window3
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     ax, 1
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax
push    win             ; win
call    setWindow
add     sp, 4
mov     ax, 0Bh
push    ax              ; int
push    word ptr [bp+member+2]
push    word ptr [bp+member] ; member
call    writeShortMemberName
add     sp, 6
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
sub     ax, ax
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
pop     bp
retn
sub_1B360 endp



; Attributes: bp-based frame

sub_1B3DF proc near

arg_0= word ptr  4

push    bp              ; draw
mov     bp, sp
mov     ax, 1
push    ax
mov     ax, offset _window3
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
push    word_2D102      ; win
call    setWindow
add     sp, 4
mov     ax, 0Bh
push    ax              ; int
push    word ptr dword_2DFF0+2
push    word ptr dword_2DFF0 ; member
call    writeShortMemberName
add     sp, 6
cmp     [bp+arg_0], 0FFFFh
jz      short loc_1B434
sub     ax, ax
push    ax
mov     ax, 1
push    ax
push    [bp+arg_0]
call    sub_107E1
add     sp, 6

loc_1B434:
sub     ax, ax
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_20772
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
pop     bp
retn
sub_1B3DF endp




sub_1B452 proc near
mov     word_2D126, 0
mov     byte_2ED26, 0
sub     ax, ax
push    ax
mov     ax, offset _window3
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     ax, 1
push    ax
mov     ax, offset _windowInfo
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax
call    writeParty
add     sp, 2
call    sub_10D78
call    sub_1DC46
call    writeTime
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
retn
sub_1B452 endp



; Attributes: bp-based frame

; int __cdecl sub_1B498(char *)
sub_1B498 proc near

arg_0= dword ptr  4

push    bp              ; draw
mov     bp, sp
sub     ax, ax
push    ax
mov     ax, offset _window10
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
push    word ptr _disk1._cash+2
push    word ptr _disk1._cash
push    word ptr [bp+arg_0] ; char *
mov     ax, offset aSYouHaveLu ; "%s, you have $%lu"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
push    word ptr [bp+arg_0+2] ; int
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
pop     bp
retn
sub_1B498 endp

push    bp
mov     bp, sp
sub     sp, 6
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     cl, 4
shl     ax, cl
add     ax, word ptr dword_2E200
mov     dx, word ptr dword_2E200+2
mov     [bp-6], ax
mov     [bp-4], dx
les     bx, [bp-6]
push    word ptr es:[bx+34h]
push    word ptr es:[bx+32h]
mov     al, 18h
mul     byte ptr es:[bx+2Ch]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1616s6ld ; "%1d>%-16.16s %6ld"
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ds
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 6
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3200h]
mov     [bp-2], ax
push    ax
push    word ptr [bp+8]
call    sub_20182
add     sp, 4
mov     [bp-4], al
or      al, al
jz      short loc_1B562
mov     ax, 2Ah ; '*'
jmp     short loc_1B565

loc_1B562:
mov     ax, 20h ; ' '

loc_1B565:
push    ax
push    word ptr [bp+4]
mov     ax, offset a1dC_0 ; "%1d>%c"
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ds
push    ax
call    writeStringAt
add     sp, 8
mov     ax, 6
imul    word ptr [bp-2]
mov     si, ax
mov     bx, [bp+8]
test    byte ptr [bx+si+65h], 1
jz      short loc_1B5A1
mov     _inverseText, 0FFh

loc_1B5A1:
mov     bx, [bp+6]
shl     bx, 1
mov     si, [bx-3240h]
mov     cl, 4
shl     si, cl
les     bx, dword_2E200
push    word ptr es:[bx+si+30h]
push    word ptr es:[bx+si+2Eh]
mov     ax, 6
imul    word ptr [bp-2]
mov     si, ax
mov     bx, [bp+8]
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
mov     ax, offset a1515s6ld ; "%-15.15s %6ld"
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 0Ah
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
mov     ax, 0FFFFh
push    ax
mov     ax, offset sprintf_buffer
push    ds
push    ax
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

; int __cdecl sub_1B5FF(char *)
sub_1B5FF proc near

arg_0= dword ptr  4

push    bp              ; draw
mov     bp, sp
mov     ax, 1
push    ax
push    word ptr [bp+arg_0] ; char *
call    sub_1B498
add     sp, 4
call    sub_1DC28
sub     ax, ax
push    ax
mov     ax, offset _window7
push    ax              ; win
call    setWindow
add     sp, 4
pop     bp
retn
sub_1B5FF endp



; Attributes: bp-based frame

sub_1B621 proc near

var_12= dword ptr -12h
var_E= dword ptr -0Eh
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 12h
push    si
mov     [bp+var_A], 0
mov     [bp+var_4], 0
call    winClearCurrent
call    sub_1DC28
jmp     loc_1B81E

loc_1B63A:
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_E], ax
mov     ax, 1
push    ax
push    ax
push    [bp+arg_0]
call    sub_107E1
add     sp, 6
mov     [bp+var_6], 0
mov     [bp+var_8], 0
jmp     short loc_1B684

loc_1B660:
mov     si, [bp+var_6]
mov     cl, 4
shl     si, cl
les     bx, dword_2E200
cmp     byte ptr es:[bx+si+2Dh], 0
jz      short loc_1B681
mov     bx, [bp+var_8]
inc     [bp+var_8]
shl     bx, 1
mov     ax, [bp+var_6]
mov     [bx-3240h], ax

loc_1B681:
inc     [bp+var_6]

loc_1B684:
les     bx, dword_2E200
mov     ax, [bp+var_6]
cmp     es:[bx+2Ah], ax
jg      short loc_1B660
cmp     [bp+var_8], 0
jg      short loc_1B69A
jmp     loc_1B809

loc_1B69A:              ; char *
push    word ptr [bp+var_E]
call    sub_1B5FF
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 0B4D4h
push    ax
mov     ax, 1239h
push    ax
lea     ax, [bp+var_A]
push    ax
push    [bp+var_8]
push    word ptr [bp+var_E]
call    sub_11682
add     sp, 0Ch
mov     word ptr [bp+var_E+2], ax
or      ax, ax
jge     short loc_1B6C8
jmp     loc_1B7DD

loc_1B6C8:
mov     bx, ax
shl     bx, 1
mov     ax, [bx-3240h]
mov     word ptr [bp+var_E+2], ax
mov     si, ax
mov     cl, 4
shl     si, cl
les     bx, dword_2E200
mov     ax, word ptr _disk1._cash
mov     dx, word ptr _disk1._cash+2
cmp     es:[bx+si+34h], dx
jb      short loc_1B706
ja      short loc_1B6F2
cmp     es:[bx+si+32h], ax
jbe     short loc_1B706

loc_1B6F2:
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, 1264h

loc_1B6FC:              ; char *
push    ax
call    sub_1B2F0
add     sp, 6
jmp     loc_1B81E

loc_1B706:
mov     bx, word ptr [bp+var_E]
cmp     byte ptr [bx+52h], 20h ; ' '
jb      short loc_1B712
jmp     loc_1B7D0

loc_1B712:
mov     si, word ptr [bp+var_E+2]
mov     cl, 4
shl     si, cl
les     bx, dword_2E200
mov     al, es:[bx+si+2Ch]
sub     ah, ah
mov     [bp+var_2], ax
mov     ax, 18h
imul    [bp+var_2]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    word ptr [bp+var_E] ; char *
mov     ax, offset aSBuys1S ; "%s buys 1 %s."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    sub_1B2F0
add     sp, 6
mov     bx, word ptr [bp+var_E]
mov     al, 6
mul     byte ptr [bx+52h]
mov     si, ax
mov     al, byte ptr [bp+var_2]
mov     [bx+si+62h], al
mov     bx, word ptr [bp+var_E]
mov     al, 6
mul     byte ptr [bx+52h]
mov     si, ax
mov     byte ptr [bx+si+64h], 0
mov     bx, word ptr [bp+var_E]
mov     al, 6
mul     byte ptr [bx+52h]
mov     si, ax
mov     byte ptr [bx+si+63h], 0
push    [bp+var_2]
mov     bx, word ptr [bp+var_E]
mov     al, 6
mul     byte ptr [bx+52h]
add     ax, bx
add     ax, 62h ; 'b'
push    ax
call    sub_1D6AA
add     sp, 4
mov     bx, word ptr [bp+var_E]
inc     byte ptr [bx+52h]
mov     ax, word ptr [bp+var_E+2]
mov     cl, 4
shl     ax, cl
add     ax, word ptr dword_2E200
mov     dx, word ptr dword_2E200+2
mov     word ptr [bp+var_12], ax
mov     word ptr [bp+var_12+2], dx
les     bx, [bp+var_12]
mov     ax, es:[bx+32h]
mov     dx, es:[bx+34h]
sub     word ptr _disk1._cash, ax
sbb     word ptr _disk1._cash+2, dx
dec     byte ptr es:[bx+2Dh]
call    sub_17E09
jmp     short loc_1B81E

loc_1B7D0:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 1298h
jmp     loc_1B6FC

loc_1B7DD:
sub     ax, ax
push    ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+arg_0]
push    ax
mov     ax, word ptr [bp+var_E+2]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jnz     short loc_1B802
call    winClearCurrent
call    sub_1DC28
jmp     short loc_1B81A

loc_1B802:
mov     [bp+var_A], 0
jmp     short loc_1B81E

loc_1B809:
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, 12B0h
push    ax              ; char *
call    sub_1B2F0
add     sp, 6

loc_1B81A:
mov     [bp+var_4], 1

loc_1B81E:
cmp     [bp+var_4], 0
jnz     short loc_1B827
jmp     loc_1B63A

loc_1B827:
mov     ax, [bp+arg_0]
pop     si
mov     sp, bp
pop     bp
retn
sub_1B621 endp



; Attributes: bp-based frame

sub_1B82F proc near

var_16= dword ptr -16h
var_12= dword ptr -12h
var_E= word ptr -0Eh
var_C= byte ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= byte ptr -4
var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 16h
push    si
mov     [bp+var_E], 0
mov     [bp+var_4], 0
mov     [bp+var_C], 0
call    winClearCurrent
call    sub_1DC28
jmp     loc_1BA15

loc_1B84C:
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_12], ax
mov     ax, 1
push    ax
push    ax
push    [bp+arg_0]
call    sub_107E1
add     sp, 6
mov     [bp+var_6], 0
mov     [bp+var_A], 0
jmp     short loc_1B8DA

loc_1B872:
inc     [bp+var_8]

loc_1B875:
mov     bx, word ptr [bp+var_12]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_8]
jbe     short loc_1B8D7
mov     ax, [bp+var_6]
mov     cl, 4
shl     ax, cl
add     ax, word ptr dword_2E200
mov     dx, word ptr dword_2E200+2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
les     bx, [bp+var_16]
mov     al, es:[bx+2Ch]
mov     cx, ax
mov     ax, 6
imul    [bp+var_8]
mov     si, ax
mov     bx, word ptr [bp+var_12]
cmp     [bx+si+62h], cl
jnz     short loc_1B872
mov     bx, word ptr [bp+var_16]
cmp     byte ptr es:[bx+2Dh], 0FFh
jnb     short loc_1B872
mov     bx, [bp+var_A]
shl     bx, 1
mov     ax, [bp+var_8]
mov     [bx-3200h], ax
mov     bx, [bp+var_A]
inc     [bp+var_A]
shl     bx, 1
mov     ax, [bp+var_6]
mov     [bx-3240h], ax
jmp     short loc_1B872

loc_1B8D7:
inc     [bp+var_6]

loc_1B8DA:
les     bx, dword_2E200
mov     ax, [bp+var_6]
cmp     es:[bx+2Ah], ax
jle     short loc_1B8EE
mov     [bp+var_8], 0
jmp     short loc_1B875

loc_1B8EE:
cmp     [bp+var_A], 0
jg      short loc_1B8F7
jmp     loc_1B9F2

loc_1B8F7:              ; char *
push    word ptr [bp+var_12]
call    sub_1B5FF
add     sp, 2
sub     ax, ax
push    ax
mov     ax, 0B537h
push    ax
mov     ax, 12CEh
push    ax
lea     ax, [bp+var_E]
push    ax
push    [bp+var_A]
push    word ptr [bp+var_12]
call    sub_11682
add     sp, 0Ch
mov     word ptr [bp+var_12+2], ax
or      ax, ax
jge     short loc_1B925
jmp     loc_1B9CE

loc_1B925:
mov     bx, ax
shl     bx, 1
mov     ax, 6
mul     word ptr [bx-3200h]
mov     si, ax
mov     bx, word ptr [bp+var_12]
mov     al, [bx+si+62h]
sub     ah, ah
mov     [bp+var_2], ax
mov     ax, 18h
imul    [bp+var_2]
add     ax, _globals
add     ax, 2Ah ; '*'
push    ax
push    bx              ; char *
mov     ax, offset aSSells1S ; "%s sells 1 %s."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    sub_1B2F0
add     sp, 6
mov     ax, 0FFh
push    ax
mov     bx, word ptr [bp+var_12+2]
shl     bx, 1
push    word ptr [bx-3200h]
push    [bp+arg_0]
call    sub_2060D
add     sp, 6
mov     ax, 1
push    ax
push    ax
push    [bp+arg_0]
call    sub_107E1
add     sp, 6
mov     bx, word ptr [bp+var_12+2]
shl     bx, 1
mov     si, [bx-3240h]
mov     cl, 4
shl     si, cl
les     bx, dword_2E200
inc     byte ptr es:[bx+si+2Dh]
mov     bx, word ptr [bp+var_12+2]
shl     bx, 1
mov     si, [bx-3240h]
shl     si, cl
les     bx, dword_2E200
mov     ax, es:[bx+si+2Eh]
mov     dx, es:[bx+si+30h]
add     word ptr _disk1._cash, ax
adc     word ptr _disk1._cash+2, dx
mov     [bp+var_C], 1
call    sub_17E09
jmp     short loc_1BA15

loc_1B9CE:
sub     ax, ax
push    ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+arg_0]
push    ax
mov     ax, word ptr [bp+var_12+2]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jz      short loc_1BA0B
mov     [bp+var_E], 0
jmp     short loc_1BA15

loc_1B9F2:
cmp     [bp+var_C], 0
jnz     short loc_1BA0B
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, 1309h
push    ax              ; char *
call    sub_1B2F0
add     sp, 6
jmp     short loc_1BA11

loc_1BA0B:
call    winClearCurrent
call    sub_1DC28

loc_1BA11:
mov     [bp+var_4], 1

loc_1BA15:
cmp     [bp+var_4], 0
jnz     short loc_1BA1E
jmp     loc_1B84C

loc_1BA1E:
mov     ax, [bp+arg_0]
pop     si
mov     sp, bp
pop     bp
retn
sub_1B82F endp



; Attributes: bp-based frame

sub_1BA26 proc near

var_C= dword ptr -0Ch
var_8= byte ptr -8
var_6= word ptr -6
var_4= word ptr -4
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 0Ch
mov     [bp+var_8], 0
mov     ax, word_2EBF8
mov     dx, word_2EBFA
mov     word ptr dword_2E200, ax
mov     word ptr dword_2E200+2, dx
mov     word ptr [bp+var_C+2], 0
jmp     short loc_1BA5E

loc_1BA45:
les     bx, dword_2E200
mov     ax, es:[bx+2Ah]
mov     cl, 4
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 2Ch ; ','
mov     word ptr dword_2E200, ax
inc     word ptr [bp+var_C+2]

loc_1BA5E:
mov     ax, [bp+arg_0]
cmp     word ptr [bp+var_C+2], ax
jl      short loc_1BA45
mov     ax, word ptr dword_2E200
mov     dx, word ptr dword_2E200+2
add     ax, 14h
push    dx
push    ax              ; char *
les     bx, dword_2E200
mov     al, es:[bx+28h]
sub     ah, ah
push    ax              ; int
call    sub_1B360
add     sp, 6
mov     ax, 1
push    ax
call    sub_1BB93

loc_1BA8A:
add     sp, 2
mov     [bp+var_4], ax
jmp     loc_1BB20

loc_1BA93:
mov     ax, 1
push    ax
push    ax
push    [bp+var_4]
call    sub_107E1
add     sp, 6
mov     bx, [bp+var_4]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_C], ax
sub     ax, ax
push    ax
push    word ptr [bp+var_C] ; char *
call    sub_1B498
add     sp, 4
mov     ax, 2
push    ax              ; int
mov     ax, offset aBUySEllLEave ; "B)uy    S)ell    L)eave"
push    ds
push    ax              ; member
call    writeShortMemberName
add     sp, 6
call    sub_1DC28
call    getKeypress
cbw
mov     [bp+var_6], ax
cmp     ax, 3Fh ; '?'
jg      short loc_1BB11
cmp     ax, 3Bh ; ';'
jge     short loc_1BAE3
cmp     ax, 0FFFFh
jz      short loc_1BAFA
jmp     short loc_1BB20

loc_1BAE3:
sub     ax, ax
push    ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+var_4]
push    ax
push    [bp+var_6]
call    sub_1CB80
add     sp, 0Ah
jmp     short loc_1BB20

loc_1BAFA:
mov     [bp+var_8], 1
jmp     short loc_1BB20

loc_1BB00:
push    [bp+var_4]
call    sub_1B621
jmp     short loc_1BA8A

loc_1BB08:
push    [bp+var_4]
call    sub_1B82F
jmp     loc_1BA8A

loc_1BB11:
cmp     ax, 42h ; 'B'
jz      short loc_1BB00
cmp     ax, 4Ch ; 'L'
jz      short loc_1BAFA
cmp     ax, 53h ; 'S'
jz      short loc_1BB08

loc_1BB20:
cmp     [bp+var_8], 0
jnz     short loc_1BB29
jmp     loc_1BA93

loc_1BB29:
cmp     byte_2ED26, 0
jnz     short loc_1BB35
call    sub_1B452
jmp     short loc_1BB3F

loc_1BB35:
mov     ax, 0FFFFh
push    ax
call    sub_1B3DF
add     sp, 2

loc_1BB3F:
mov     sp, bp
pop     bp
retn
sub_1BA26 endp

push    bp
mov     bp, sp
sub     sp, 4
mov     ax, [bp+6]
mov     cl, 5
shl     ax, cl
add     ax, word ptr dword_2EC1E
mov     dx, word ptr dword_2EC1E+2
mov     [bp-4], ax
mov     [bp-2], dx
les     bx, [bp-4]
push    word ptr es:[bx+44h]
push    word ptr es:[bx+42h]
add     ax, 2Ch ; ','
push    dx
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1616fs6ld ; "%1d>%-16.16Fs %6ld"
push    ax
mov     ax, offset sprintf_buffer
push    ax
call    _sprintf
add     sp, 0Eh
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ds
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_1BB93 proc near

var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
jmp     short loc_1BBA3

loc_1BBA0:
inc     [bp+var_2]

loc_1BBA3:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_2], ax
jnb     short loc_1BBC1
mov     bx, [bp+var_2]
shl     bx, 1
push    word ptr [bx-2518h] ; member
call    getMemberCon
add     sp, 2
cmp     ax, [bp+arg_0]
jge     short loc_1BBA0

loc_1BBC1:
mov     ax, [bp+var_2]
mov     sp, bp
pop     bp
retn
sub_1BB93 endp



; Attributes: bp-based frame

sub_1BBC8 proc near

var_24= dword ptr -24h
var_20= word ptr -20h
var_1E= word ptr -1Eh
var_1C= word ptr -1Ch
var_1A= word ptr -1Ah
var_18= word ptr -18h
var_16= dword ptr -16h
var_12= byte ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_2= byte ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 24h
push    di
push    si
mov     [bp+var_1E], 0
mov     [bp+var_2], 0
mov     ax, word ptr _services
mov     dx, word ptr _services+2
mov     word ptr dword_2EC1E, ax
mov     word ptr dword_2EC1E+2, dx
mov     [bp+var_10], 0
jmp     short loc_1BC07

loc_1BBEE:
les     bx, dword_2EC1E
mov     ax, es:[bx+2Ah]
mov     cl, 5
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 2Ch ; ','
mov     word ptr dword_2EC1E, ax
inc     [bp+var_10]

loc_1BC07:
mov     ax, [bp+arg_0]
cmp     [bp+var_10], ax
jl      short loc_1BBEE
mov     ax, word ptr dword_2EC1E
mov     dx, word ptr dword_2EC1E+2
add     ax, 14h
push    dx
push    ax              ; char *
les     bx, dword_2EC1E
mov     al, es:[bx+28h]
sub     ah, ah
push    ax              ; int
call    sub_1B360
add     sp, 6
mov     ax, 5
push    ax
call    sub_1BB93
add     sp, 2
mov     [bp+var_6], ax
jmp     loc_1BF30

loc_1BC3C:
mov     ax, 1
push    ax
push    ax
push    [bp+var_6]
call    sub_107E1
add     sp, 6
mov     bx, [bp+var_6]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_1C], ax
push    ax              ; char *
call    sub_1B5FF
add     sp, 2
mov     bx, [bp+var_1C]
mov     al, [bx+48h]
sub     ah, ah
mov     cl, 7
shl     ax, cl
add     ax, word ptr _professions
mov     dx, word ptr _professions+2
mov     word ptr [bp+var_16], ax
mov     word ptr [bp+var_16+2], dx
sub     ax, ax
push    ax
mov     ax, 0BB43h
push    ax
mov     ax, 134Eh
push    ax
lea     ax, [bp+var_1E]
push    ax
les     bx, dword_2EC1E
push    word ptr es:[bx+2Ah]
push    [bp+var_1C]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_20], ax
or      ax, ax
jge     short loc_1BCA1
jmp     loc_1BF05

loc_1BCA1:
mov     cl, 5
shl     ax, cl
add     ax, word ptr dword_2EC1E
mov     dx, word ptr dword_2EC1E+2
mov     word ptr [bp+var_24], ax
mov     word ptr [bp+var_24+2], dx
les     bx, [bp+var_24]
mov     ax, es:[bx+48h]
mov     dx, es:[bx+4Ah]
mov     [bp+var_E], ax
mov     [bp+var_C], dx
mov     al, es:[bx+47h]
sub     ah, ah
mov     [bp+var_1A], ax
mov     al, es:[bx+46h]
cmp     ax, 7           ; switch 8 cases
ja      short def_1BCD9 ; jumptable 0001BCD9 default case
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_1BCD9[bx] ; switch jump

loc_1BCDE:              ; jumptable 0001BCD9 case 0
mov     bx, [bp+var_1C]
mov     al, [bx+4Eh]

loc_1BCE4:
sub     ah, ah
mov     [bp+var_A], ax
mov     [bp+var_8], 0
jmp     short def_1BCD9 ; jumptable 0001BCD9 default case

loc_1BCF0:              ; jumptable 0001BCD9 case 1
mov     bx, [bp+var_1C]
mov     ax, [bx+5Ah]
mov     dx, [bx+5Ch]

loc_1BCF9:
mov     [bp+var_A], ax
mov     [bp+var_8], dx
jmp     short def_1BCD9 ; jumptable 0001BCD9 default case

loc_1BD01:              ; jumptable 0001BCD9 case 2
mov     bx, [bp+var_1C]
mov     ax, [bx+44h]

loc_1BD07:
cwd
jmp     short loc_1BCF9

loc_1BD0A:              ; jumptable 0001BCD9 case 3
mov     bx, [bp+var_1C]
mov     ax, [bx+46h]
jmp     short loc_1BD07

loc_1BD12:              ; jumptable 0001BCD9 case 4
mov     si, [bp+var_1A]
mov     bx, [bp+var_1C]
mov     al, [bx+si+18h]
jmp     short loc_1BCE4

loc_1BD1D:              ; jumptable 0001BCD9 case 5
mov     si, [bp+var_1A]
mov     bx, [bp+var_1C]
mov     al, [bx+si+24h]
jmp     short loc_1BCE4

loc_1BD28:              ; jumptable 0001BCD9 case 6
mov     si, [bp+var_1A]
mov     bx, [bp+var_1C]
mov     al, [bx+si+34h]
jmp     short loc_1BCE4

loc_1BD33:              ; jumptable 0001BCD9 case 7
sub     ax, ax
mov     [bp+var_8], ax
mov     [bp+var_A], ax
jmp     short def_1BCD9 ; jumptable 0001BCD9 default case
jpt_1BCD9 dw offset loc_1BCDE ; jump table for switch statement
dw offset loc_1BCF0
dw offset loc_1BD01
dw offset loc_1BD0A
dw offset loc_1BD12
dw offset loc_1BD1D
dw offset loc_1BD28
dw offset loc_1BD33

def_1BCD9:              ; jumptable 0001BCD9 default case
mov     ax, [bp+var_20]
mov     cl, 5
shl     ax, cl
add     ax, word ptr dword_2EC1E
mov     dx, word ptr dword_2EC1E+2
mov     word ptr [bp+var_24], ax
mov     word ptr [bp+var_24+2], dx
les     bx, [bp+var_24]
mov     ax, word ptr _disk1._cash
mov     dx, word ptr _disk1._cash+2
cmp     es:[bx+44h], dx
jbe     short loc_1BD75
jmp     loc_1BEF9

loc_1BD75:
jb      short loc_1BD80
cmp     es:[bx+42h], ax
jbe     short loc_1BD80
jmp     loc_1BEF9

loc_1BD80:
mov     ax, [bp+var_E]
mov     dx, [bp+var_C]
cmp     [bp+var_8], dx
jge     short loc_1BD8E
jmp     loc_1BEED

loc_1BD8E:
jg      short loc_1BD98
cmp     [bp+var_A], ax
jnb     short loc_1BD98
jmp     loc_1BEED

loc_1BD98:
mov     [bp+var_12], 0
les     bx, [bp+var_24]
mov     al, es:[bx+41h]
sub     ah, ah
mov     [bp+var_18], ax
mov     al, es:[bx+40h]
cmp     ax, 7           ; switch 8 cases
jbe     short loc_1BDB4
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BDB4:
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_1BDB7[bx] ; switch jump

loc_1BDBC:              ; jumptable 0001BDB7 case 0
mov     bx, [bp+var_1C]
cmp     byte ptr [bx+4Eh], 64h ; 'd'
jb      short loc_1BDC8
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BDC8:
inc     byte ptr [bx+4Eh]

loc_1BDCB:
mov     [bp+var_12], 1
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BDD2:              ; jumptable 0001BDB7 case 1
mov     bx, [bp+var_1C]
cmp     word ptr [bx+5Ch], 98h
jbe     short loc_1BDDF
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BDDF:
jb      short loc_1BDEB
cmp     word ptr [bx+5Ah], 967Fh
jb      short loc_1BDEB
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BDEB:
add     word ptr [bx+5Ah], 1
adc     word ptr [bx+5Ch], 0
jmp     short loc_1BDCB

loc_1BDF5:              ; jumptable 0001BDB7 case 2
mov     bx, [bp+var_1C]
mov     si, bx
mov     ax, [si+46h]
cmp     [bx+44h], ax
jl      short loc_1BE05
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BE05:
inc     word ptr [bx+44h]
jmp     short loc_1BDCB

loc_1BE0A:              ; jumptable 0001BDB7 case 3
mov     bx, [bp+var_1C]
les     si, [bp+var_16]
mov     ax, es:[si+74h]
cmp     [bx+46h], ax
jb      short loc_1BE1C
jmp     def_1BDB7       ; jumptable 0001BDB7 default case

loc_1BE1C:
inc     word ptr [bx+46h]
jmp     short loc_1BDCB

loc_1BE21:              ; jumptable 0001BDB7 case 4
mov     si, [bp+var_18]
add     si, [bp+var_1C]
add     si, 18h
mov     di, [bp+var_1A]

loc_1BE2D:
les     bx, [bp+var_16]
mov     al, [si]
cmp     es:[bx+di+2Ch], al
jbe     short def_1BDB7 ; jumptable 0001BDB7 default case
inc     byte ptr [si]
jmp     short loc_1BDCB

loc_1BE3C:              ; jumptable 0001BDB7 case 5
mov     si, [bp+var_18]
add     si, [bp+var_1C]
add     si, 24h ; '$'

loc_1BE45:
les     bx, _hdsPct
mov     di, es:[bx+3E6h]
jmp     short loc_1BE2D

loc_1BE50:              ; jumptable 0001BDB7 case 6
mov     si, [bp+var_18]
add     si, [bp+var_1C]
add     si, 34h ; '4'
jmp     short loc_1BE45

loc_1BE5B:              ; jumptable 0001BDB7 case 7
mov     bx, [bp+var_1C]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_18]
shl     dx, cl
test    ax, dx
jz      short def_1BDB7 ; jumptable 0001BDB7 default case
mov     al, 1
shl     al, cl
not     al
and     [bx+58h], al
cmp     [bp+var_18], 7
jz      short loc_1BE81
jmp     loc_1BDCB

loc_1BE81:
mov     bx, [bp+var_1C]
mov     [bx+21h], ah
mov     bx, [bp+var_1C]
mov     [bx+54h], ah
mov     bx, [bp+var_1C]
mov     [bx+57h], ah
mov     bx, [bp+var_1C]
mov     [bx+53h], ah
jmp     loc_1BDCB
jpt_1BDB7 dw offset loc_1BDBC ; jump table for switch statement
dw offset loc_1BDD2
dw offset loc_1BDF5
dw offset loc_1BE0A
dw offset loc_1BE21
dw offset loc_1BE3C
dw offset loc_1BE50
dw offset loc_1BE5B

def_1BDB7:              ; jumptable 0001BDB7 default case
cmp     [bp+var_12], 0
jz      short loc_1BEDD
mov     si, [bp+var_20]
mov     cl, 5
shl     si, cl
les     bx, dword_2EC1E
mov     ax, es:[bx+si+42h]
mov     dx, es:[bx+si+44h]
sub     word ptr _disk1._cash, ax
sbb     word ptr _disk1._cash+2, dx
mov     ax, 1
push    ax
push    ax
push    [bp+var_6]
call    sub_107E1

loc_1BED8:
add     sp, 6
jmp     short loc_1BF30

loc_1BEDD:
mov     ax, 1
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, 1375h

loc_1BEE7:              ; char *
push    ax
call    sub_1B2F0
jmp     short loc_1BED8

loc_1BEED:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 1398h
jmp     short loc_1BEE7

loc_1BEF9:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 13CBh
jmp     short loc_1BEE7

loc_1BF05:
cmp     [bp+var_20], 0FFFFh
jnz     short loc_1BF17
call    winClearCurrent
call    sub_1DC28
mov     [bp+var_2], 1
jmp     short loc_1BF30

loc_1BF17:
sub     ax, ax
push    ax
push    ax
mov     ax, 5
push    ax
lea     ax, [bp+var_6]
push    ax
mov     ax, [bp+var_20]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah

loc_1BF30:
cmp     [bp+var_2], 0
jnz     short loc_1BF39
jmp     loc_1BC3C

loc_1BF39:
cmp     byte_2ED26, 0
jnz     short loc_1BF45
call    sub_1B452
jmp     short loc_1BF4F

loc_1BF45:
mov     ax, 0FFFFh
push    ax
call    sub_1B3DF
add     sp, 2

loc_1BF4F:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1BBC8 endp

push    bp
mov     bp, sp
sub     sp, 28h
mov     bx, [bp+6]
shl     bx, 1
push    word ptr [bx-2518h]
push    word ptr [bp+4]
mov     ax, offset aF1d1919s ; "F%1d> %-19.19s"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 28h
mov     bx, [bp+6]
shl     bx, 1
mov     ax, 16h
mul     word ptr [bx-3240h]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 3EEh
push    dx
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1919fs_0 ; "%1d> %-19.19Fs"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 0Ah
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

; int __cdecl sub_1BFCE(char *)
sub_1BFCE proc near

arg_0= dword ptr  4

push    bp
mov     bp, sp
push    word ptr [bp+arg_0] ; char *
mov     ax, offset aSIsBeyondYourH ; "%s is beyond your help."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2
pop     bp
retn
sub_1BFCE endp




sub_1BFEE proc near
mov     ax, offset aYourEffortsWer ; "Your efforts were commendable, but unsu"...
push    ax
call    sub_1B29B
add     sp, 2
retn
sub_1BFEE endp



; Attributes: bp-based frame

sub_1BFF9 proc near

var_2= dword ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= byte ptr  0Ch

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_2], ax
push    [bp+arg_2]
push    [bp+arg_0]
call    sub_1969F
add     sp, 4
or      al, al
jz      short loc_1C04D
cmp     [bp+arg_8], 0
jz      short loc_1C04D
push    [bp+arg_6]
push    [bp+arg_4]
mov     si, [bp+arg_2]
mov     bx, word ptr [bp+var_2]
mov     al, [bx+si+24h]
sub     ah, ah
push    ax
push    bx              ; char *
mov     ax, offset aSIsNowALevelDF ; "%s is now a level %d %Fs."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ch
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2

loc_1C04D:
pop     si
mov     sp, bp
pop     bp
retn
sub_1BFF9 endp



; Attributes: bp-based frame

sub_1C052 proc near

var_1A= word ptr -1Ah
var_18= word ptr -18h
var_16= word ptr -16h
var_14= word ptr -14h
var_12= word ptr -12h
var_10= byte ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= byte ptr -6
member= word ptr -4
arg_0= word ptr  4
arg_2= byte ptr  6
arg_4= byte ptr  8

push    bp
mov     bp, sp
sub     sp, 1Ah
push    si              ; draw
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_1A], ax
mov     [bp+var_18], 0
mov     [bp+var_6], 0
mov     bx, ax
mov     al, [bx+145h]
sub     ah, ah
mov     [bp+member+2], ax
mov     si, ax
mov     al, [bx+si+24h]
mov     [bp+var_8], ax
cmp     [bp+arg_2], ah
jz      short loc_1C097
mov     ax, 1
push    ax
mov     ax, offset _windowDialog
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_1DC28

loc_1C097:
cmp     [bp+arg_2], 0
jz      short loc_1C0F2
mov     [bp+var_C], 0
cmp     _disk1._partyCount, 1
jbe     short loc_1C0DB
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 0BF55h
push    ax
cmp     [bp+member+2], 0
jnz     short loc_1C0BF
mov     ax, 147Ah
jmp     short loc_1C0C2

loc_1C0BF:
mov     ax, 1485h

loc_1C0C2:
push    ax
mov     al, _disk1._partyCount
sub     ah, ah
push    ax
sub     ax, ax
push    ax
push    [bp+var_1A]
call    sub_115CB
add     sp, 0Eh
call    sub_10BE7
mov     [bp+var_C], ax

loc_1C0DB:
cmp     [bp+var_C], 0FFFFh
jnz     short loc_1C0E6

loc_1C0E1:
sub     ax, ax
jmp     loc_1C426

loc_1C0E6:
mov     bx, [bp+var_1A]
mov     al, byte ptr [bp+var_C]
mov     [bx+146h], al
jmp     short loc_1C0FE

loc_1C0F2:
mov     bx, [bp+var_1A]
mov     al, [bx+146h]
sub     ah, ah
mov     [bp+var_C], ax

loc_1C0FE:
cmp     [bp+var_C], 0
jl      short loc_1C097
mov     bx, [bp+var_C]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+member], ax
cmp     [bp+member+2], 0
jz      short loc_1C119
jmp     loc_1C24B

loc_1C119:
mov     bx, [bp+var_1A]
mov     byte ptr [bx+144h], 8
push    [bp+member]     ; member
call    getMemberCon
add     sp, 2
mov     [bp+var_A], ax
or      ax, ax
jnz     short loc_1C134
jmp     loc_1C227

loc_1C134:
cmp     ax, 1
jl      short loc_1C0E1
cmp     ax, 4
jle     short loc_1C145
cmp     ax, 5
jz      short loc_1C156
jmp     short loc_1C0E1

loc_1C145:
dec     [bp+var_A]
mov     bx, [bp+var_A]
shl     bx, 1
mov     ax, [bp+var_8]
cmp     [bx+11F0h], ax
jle     short loc_1C168

loc_1C156:
cmp     [bp+arg_2], 0
jz      short loc_1C0E1

loc_1C15C:              ; char *
push    [bp+member]
call    sub_1BFCE

loc_1C162:
add     sp, 2
jmp     loc_1C0E1

loc_1C168:
cmp     [bp+var_8], 14h
jle     short loc_1C175

loc_1C16E:
mov     [bp+var_16], 5Fh ; '_'
jmp     short loc_1C199

loc_1C175:
mov     ax, [bp+var_8]
cwd
mov     cx, 5
idiv    cx
cmp     ax, [bp+var_A]
jg      short loc_1C16E
mov     ax, [bp+var_8]
mov     bx, [bp+var_A]
shl     bx, 1
sub     ax, [bx+11F0h]
mov     cx, 1Eh
imul    cx
add     ax, cx
mov     [bp+var_16], ax

loc_1C199:
cmp     [bp+arg_4], 0
jz      short loc_1C212
mov     bx, [bp+var_1A]
mov     byte ptr [bx+144h], 0Eh
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_16], ax
jb      short loc_1C218
cmp     [bp+arg_2], 0
jz      short loc_1C1DB
push    [bp+member]     ; char *
mov     ax, offset aSuccessSIsGett ; "Success! %s is getting better."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2

loc_1C1DB:
cmp     [bp+var_A], 0
jnz     short loc_1C1EC
mov     bx, [bp+member]
mov     si, bx
mov     ax, [si+122h]
jmp     short loc_1C1F8

loc_1C1EC:
mov     bx, [bp+member]
mov     si, [bp+var_A]
shl     si, 1
mov     ax, [si+11F8h]

loc_1C1F8:
mov     [bx+44h], ax
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
mov     ax, 14AFh
push    ds
push    ax
sub     ax, ax
push    ax

loc_1C209:
push    [bp+arg_0]
call    sub_1BFF9
add     sp, 0Ah

loc_1C212:
mov     ax, 1
jmp     loc_1C426

loc_1C218:
cmp     [bp+arg_2], 0
jnz     short loc_1C221
jmp     loc_1C0E1

loc_1C221:
call    sub_1BFEE
jmp     loc_1C0E1

loc_1C227:
cmp     [bp+arg_2], 0
jnz     short loc_1C230
jmp     loc_1C0E1

loc_1C230:              ; char *
push    [bp+member]
mov     ax, offset aSDoesNotNeedAn ; "%s does not need any attention."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
jmp     loc_1C162

loc_1C24B:
mov     bx, [bp+var_1A]
mov     byte ptr [bx+144h], 9
mov     [bp+var_E], 0
mov     [bp+var_14], 0
jmp     short loc_1C285

loc_1C25F:
mov     bx, [bp+member]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_E]
shl     dx, cl
test    ax, dx
jz      short loc_1C282
mov     bx, [bp+var_14]
inc     [bp+var_14]
shl     bx, 1
mov     ax, [bp+var_E]
mov     [bx-3240h], ax

loc_1C282:
inc     [bp+var_E]

loc_1C285:
cmp     [bp+var_E], 7
jl      short loc_1C25F
mov     bx, [bp+member]
cmp     byte ptr [bx+54h], 0
jz      short loc_1C2A2
mov     bx, [bp+var_14]
inc     [bp+var_14]
shl     bx, 1
mov     word ptr [bx-3240h], 7

loc_1C2A2:
cmp     [bp+var_14], 0
jg      short loc_1C2AB
jmp     loc_1C3FE

loc_1C2AB:
mov     [bp+var_10], 0

loc_1C2AF:
cmp     [bp+var_10], 0
jz      short loc_1C2B8
jmp     loc_1C0E1

loc_1C2B8:
cmp     [bp+arg_2], 0
jz      short loc_1C2F8
sub     ax, ax
push    ax
mov     ax, 0BF8Ah
push    ax
mov     ax, 14D5h
push    ax
lea     ax, [bp+var_18]
push    ax
push    [bp+var_14]
push    [bp+var_1A]
call    sub_11682
add     sp, 0Ch
mov     [bp+var_12], ax
cmp     ax, 0FFFFh
jnz     short loc_1C2E7
mov     [bp+var_10], 1
jmp     short loc_1C304

loc_1C2E7:
cmp     [bp+var_12], 0
jl      short loc_1C304
mov     bx, [bp+var_12]
shl     bx, 1
mov     ax, [bx-3240h]
jmp     short loc_1C301

loc_1C2F8:
mov     bx, [bp+var_1A]
mov     al, [bx+147h]
sub     ah, ah

loc_1C301:
mov     [bp+var_12], ax

loc_1C304:
cmp     [bp+var_12], 0
jl      short loc_1C2AF
cmp     [bp+var_12], 7
jnz     short loc_1C313
jmp     loc_1C15C

loc_1C313:
mov     bx, [bp+var_1A]
mov     al, byte ptr [bp+var_12]
mov     [bx+147h], al
mov     ax, [bp+var_8]
dec     ax
cwd
sub     ax, dx
sar     ax, 1
cmp     ax, [bp+var_12]
jge     short loc_1C340
cmp     [bp+arg_2], 0
jnz     short loc_1C334
jmp     loc_1C2AF

loc_1C334:              ; char *
push    [bp+member]
call    sub_1BFCE
add     sp, 2
jmp     loc_1C2AF

loc_1C340:
cmp     [bp+var_8], 0Eh
jg      short loc_1C35C
mov     ax, [bp+var_8]
dec     ax
cwd
sub     ax, dx
sar     ax, 1
mov     bx, [bp+var_1A]
mov     cl, [bx+147h]
sub     ch, ch
cmp     ax, cx
jbe     short loc_1C363

loc_1C35C:
mov     [bp+var_16], 5Fh ; '_'
jmp     short loc_1C374

loc_1C363:
test    byte ptr [bp+var_8], 1
jz      short loc_1C36E
mov     ax, 3Ch ; '<'
jmp     short loc_1C371

loc_1C36E:
mov     ax, 5Ah ; 'Z'

loc_1C371:
mov     [bp+var_16], ax

loc_1C374:
cmp     [bp+arg_4], 0
jnz     short loc_1C37D
jmp     loc_1C212

loc_1C37D:
mov     bx, [bp+var_1A]
mov     byte ptr [bx+144h], 0Eh
mov     ax, 64h ; 'd'
push    ax
mov     ax, 1
push    ax
call    sub_1DEB1
add     sp, 4
cmp     [bp+var_16], ax
jb      short loc_1C3EB
cmp     [bp+arg_2], 0
jz      short loc_1C3B9
push    [bp+member]     ; char *
mov     ax, offset aSuccessYourEff ; "Success! Your efforts have cured %s."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2

loc_1C3B9:
mov     bx, [bp+member]
mov     al, 1
mov     si, [bp+var_1A]
mov     cl, [si+147h]
shl     al, cl
not     al
and     [bx+58h], al
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
mov     ax, 16h
imul    [bp+member+2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 126h
push    dx
push    ax
push    [bp+member+2]
jmp     loc_1C209

loc_1C3EB:
cmp     [bp+arg_2], 0
jz      short loc_1C3F7
call    sub_1BFEE
jmp     loc_1C212

loc_1C3F7:
mov     [bp+var_10], 1
jmp     loc_1C2AF

loc_1C3FE:
cmp     [bp+arg_2], 0
jz      short loc_1C41F
push    [bp+member]     ; char *
mov     ax, offset aSIsNotAfflicte ; "%s is not afflicted by anything!"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2

loc_1C41F:
mov     [bp+var_6], 1
jmp     loc_1C0E1

loc_1C426:
pop     si
mov     sp, bp
pop     bp
retn
sub_1C052 endp



; Attributes: bp-based frame

sub_1C42B proc near

var_A= word ptr -0Ah
var_6= word ptr -6
var_4= dword ptr -4
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 0Ah
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_A], ax
mov     byte ptr [bp+var_4+2], 0
mov     ax, 1
push    ax
mov     ax, offset _windowDialog
push    ax              ; win
call    setWindow
add     sp, 4
call    sub_1DC28
jmp     loc_1C531

loc_1C455:
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 0BF55h
push    ax
mov     ax, offset aWhoWillIngestT ; "Who will ingest this?"
push    ax
mov     al, _disk1._partyCount
sub     ah, ah
push    ax
sub     ax, ax
push    ax
push    [bp+var_A]
call    sub_115CB
add     sp, 0Eh
call    sub_10BE7
mov     [bp+var_6], ax
cmp     ax, 0FFFFh
jnz     short loc_1C484
jmp     loc_1C53A

loc_1C484:
or      ax, ax
jge     short loc_1C48B
jmp     loc_1C531

loc_1C48B:
mov     bx, ax
shl     bx, 1
mov     ax, [bx-2518h]
mov     word ptr [bp+var_4], ax
push    [bp+var_6]
call    sub_1CB58
add     sp, 2
or      al, al
jnz     short loc_1C4A6
jmp     loc_1C531

loc_1C4A6:
mov     ax, 0FFh
push    ax
push    [bp+arg_4]
push    [bp+arg_0]
call    sub_2060D
add     sp, 6
mov     al, _disk1._timeWeekday
sub     ah, ah
inc     ax
inc     ax
sub     dx, dx
mov     cx, 7
div     cx
mov     bx, word ptr [bp+var_4]
mov     [bx+55h], dl
cmp     [bp+arg_2], 1
jnz     short loc_1C4E1
mov     bx, word ptr [bp+var_4]
mov     byte ptr [bx+57h], 18h
push    word ptr [bp+var_4]
mov     ax, offset aSAlthoughPlagu ; "%s, although plagued by pink elephants,"...
jmp     short loc_1C4E7

loc_1C4E1:              ; char *
push    word ptr [bp+var_4]
mov     ax, offset aSWillNotBeBoth ; "%s will not be bothered by any mutation"...

loc_1C4E7:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2
mov     bx, word ptr [bp+var_4]
test    byte ptr [bx+58h], 80h
jz      short loc_1C526
mov     byte ptr [bx+21h], 30h ; '0'
mov     al, _disk1._timeWeekday
sub     ah, ah
inc     ax
inc     ax
sub     dx, dx
mov     cx, 7
div     cx
mov     bx, word ptr [bp+var_4]
mov     [bx+55h], dl
mov     bx, word ptr [bp+var_4]
mov     al, _disk1._timeHours
mov     [bx+56h], al

loc_1C526:
mov     bx, word ptr [bp+var_4]
mov     byte ptr [bx+53h], 0
mov     byte ptr [bp+var_4+2], 1

loc_1C531:
cmp     byte ptr [bp+var_4+2], 0
jnz     short loc_1C53A
jmp     loc_1C455

loc_1C53A:
mov     sp, bp
pop     bp
retn
sub_1C42B endp

push    bp
mov     bp, sp
sub     sp, 2Ah
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     ax, 16h
imul    word ptr [bp-2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 126h
push    dx
push    ax
mov     si, [bp-2]
mov     bx, [bp+8]
mov     al, [bx+si+24h]
sub     ah, ah
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d2d9fs ; "%1d>%2d %-.9Fs"
push    ax
lea     ax, [bp-2Ah]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Ah]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn
push    bp
mov     bp, sp
sub     sp, 2Ah
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     ax, [bx-3240h]
mov     [bp-2], ax
mov     ax, 16h
imul    word ptr [bp-2]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 286h
push    dx
push    ax
mov     si, [bp-2]
mov     bx, [bp+8]
mov     al, [bx+si+34h]
sub     ah, ah
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d2d9fs_0 ; "%1d>%2d %-.9Fs"
push    ax
lea     ax, [bp-2Ah]
push    ax
call    _sprintf
add     sp, 0Ch
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-2Ah]
push    ss
push    ax
call    writeStringAt
add     sp, 8
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

; void __cdecl characterInfoMemberName(PartyMember near *member, bool highlight)
characterInfoMemberName proc near

member= word ptr  4
highlight= byte ptr  6

push    bp              ; draw
mov     bp, sp
sub     ax, ax
push    ax
mov     ax, offset _windowCharInfoStats
push    ax              ; win
call    setWindow
add     sp, 4
push    [bp+member]     ; char *
mov     ax, offset aS   ; "%s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
cmp     [bp+highlight], 0
jz      short loc_1C620
mov     _inverseText, 0FFh

loc_1C620:
sub     ax, ax
push    ax              ; y
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _strlen
add     sp, 2
sub     ax, 25
neg     ax
cwd
sub     ax, dx
sar     ax, 1
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _inverseText, 0
pop     bp
retn
characterInfoMemberName endp



; Attributes: bp-based frame

; void __cdecl writeMemberCondition(PartyMember near *member, char brackets)
writeMemberCondition proc near

member= word ptr  4
brackets= byte ptr  6

push    bp
mov     bp, sp
cmp     [bp+brackets], 0
jz      short loc_1C658
mov     ax, offset asc_22866 ; ")"
jmp     short loc_1C65B

loc_1C658:
mov     ax, offset unk_22868

loc_1C65B:
push    ax
mov     bx, [bp+member]
cmp     [bx+PartyMember._afflicted], 0
jz      short loc_1C66A
mov     ax, offset aAfflicted ; "Afflicted"
jmp     short loc_1C66D

loc_1C66A:
mov     ax, offset aUnafflicted ; "Unafflicted"

loc_1C66D:
push    ax
cmp     [bp+brackets], 0
jz      short loc_1C679
mov     ax, offset asc_2287F ; "("
jmp     short loc_1C67C

loc_1C679:
mov     ax, offset aCond ; "Cond: "

loc_1C67C:              ; char *
push    ax
mov     ax, offset aSSS_0 ; "%s%-s%-s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 0Ah
pop     bp
retn
writeMemberCondition endp



; Attributes: bp-based frame

; void __cdecl characterInfoStats(PartyMember *member)
characterInfoStats proc near

cond= word ptr -4
ac= dword ptr -2
member= word ptr  4
highlight= word ptr  6

push    bp
mov     bp, sp
sub     sp, 4
push    di
push    si              ; draw
push    [bp+member]     ; member
call    getArmorClass
add     sp, 2
mov     word ptr [bp+ac], ax
sub     ax, ax
push    ax
mov     ax, offset _windowCharInfoStats
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     al, byte ptr [bp+highlight]
sub     ah, ah
push    ax              ; highlight
push    [bp+member]     ; member
call    characterInfoMemberName
add     sp, 4
;
mov     bx, [bp+member]
mov     al, [bx+PartyMember._rank]
sub     ah, ah
push    ax              ; char *
mov     ax, offset aRankU ; "Rank: %u"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 1
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
;
sub     ax, ax
push    ax              ; brackets
push    [bp+member]     ; member
call    writeMemberCondition
add     sp, 4
mov     ax, 2
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+member]
cmp     [bx+PartyMember._afflicted], 0
jz      short loc_1C723
mov     ax, 2
push    ax              ; y
mov     ax, 16
push    ax              ; x
mov     ax, offset aVIew ; "(V)iew"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1C723:              ; member
push    [bp+member]
call    getMemberCon
add     sp, 2
mov     [bp+cond], ax
or      ax, ax
jnz     short loc_1C741
mov     bx, [bp+member]
push    [bx+PartyMember._conBase]
push    [bx+PartyMember._con]
mov     ax, offset aCon3uMaxcon3u ; " Con: %3u  MaxCon:  %3u"
jmp     short loc_1C753

loc_1C741:
mov     bx, [bp+member]
push    [bx+PartyMember._conBase]
mov     bx, [bp+cond]
shl     bx, 1
push    off_21410[bx]   ; char *
mov     ax, offset aCon3sMaxcon3u ; " Con: %3s  MaxCon:  %3u"

loc_1C753:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 3
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
push    word ptr [bp+ac] ; char *
mov     ax, offset aAc3u ; "  AC: %3u"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 4
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+member]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_1C7A1
mov     ax, 169Bh
jmp     short loc_1C7B4

loc_1C7A1:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'

loc_1C7B4:              ; char *
push    ax
mov     ax, offset aWeap17s ; "Weap: %-.17s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 5
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _windowCharInfoStats.field_14, 8
mov     ax, 7
push    ax              ; y
mov     ax, 6
push    ax              ; x
mov     ax, offset aAttributes ; "Attributes"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _windowCharInfoStats.field_14, 0
mov     [bp+cond], 0

loc_1C7F9:
mov     di, [bp+cond]
mov     bx, [bp+member]
mov     al, [bx+di+18h]
sub     ah, ah
push    ax
mov     bx, di
shl     bx, 1
shl     bx, 1
push    word ptr [bx+15FAh] ; char *
mov     ax, offset a3s2u ; "%3s%2u"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     si, [bp+cond]
mov     cl, 2
shl     si, cl
mov     al, [si+15F9h]
cbw
push    ax              ; y
mov     al, [si+15F8h]
cbw
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
inc     [bp+cond]
cmp     [bp+cond], 7
jl      short loc_1C7F9
pop     si
pop     di
mov     sp, bp
pop     bp
retn
characterInfoStats endp



; Attributes: bp-based frame

sub_1C84A proc near

var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= byte ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 6
push    si              ; draw
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
sub     ax, ax
push    ax
mov     ax, offset _window13
push    ax              ; win
call    setWindow
add     sp, 4
mov     [bp+var_4], 0
mov     [bp+var_2], 0
jmp     short loc_1C893

loc_1C876:
mov     si, [bp+var_4]
mov     bx, [bp+var_6]
cmp     byte ptr [bx+si+24h], 0
jz      short loc_1C890
mov     bx, [bp+var_2]
shl     bx, 1
mov     ax, si
mov     [bx-3240h], ax
inc     [bp+var_2]

loc_1C890:
inc     [bp+var_4]

loc_1C893:
les     bx, _hdsPct
mov     ax, [bp+var_4]
cmp     es:[bx+2], ax
jg      short loc_1C876
cmp     [bp+arg_4], 0
jz      short loc_1C8C3
mov     ax, 1
push    ax
mov     ax, 0C53Eh
push    ax
mov     ax, 16C0h
push    ax
push    [bp+arg_2]
push    [bp+var_2]
push    [bp+var_6]
call    sub_11682
add     sp, 0Ch
jmp     short loc_1C8E8

loc_1C8C3:
sub     ax, ax
push    ax
mov     al, [bp+arg_6]
sub     ah, ah
push    ax
mov     ax, 0C53Eh
push    ax
mov     ax, 16C7h
push    ax
push    [bp+var_2]
mov     bx, [bp+arg_2]
push    word ptr [bx]
push    [bp+var_6]
call    sub_115CB
add     sp, 0Eh
mov     ax, 0FFFFh

loc_1C8E8:
pop     si
mov     sp, bp
pop     bp
retn
sub_1C84A endp



; Attributes: bp-based frame

sub_1C8ED proc near

var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= byte ptr  8
arg_6= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 6
push    si              ; draw
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
sub     ax, ax
push    ax
mov     ax, offset _window14
push    ax              ; win
call    setWindow
add     sp, 4
mov     [bp+var_4], 0
mov     [bp+var_2], 0
jmp     short loc_1C936

loc_1C919:
mov     si, [bp+var_4]
mov     bx, [bp+var_6]
cmp     byte ptr [bx+si+34h], 0
jz      short loc_1C933
mov     bx, [bp+var_2]
shl     bx, 1
mov     ax, si
mov     [bx-3240h], ax
inc     [bp+var_2]

loc_1C933:
inc     [bp+var_4]

loc_1C936:
les     bx, _hdsPct
mov     ax, [bp+var_4]
cmp     es:[bx+4], ax
jg      short loc_1C919
cmp     [bp+arg_4], 0
jz      short loc_1C966
mov     ax, 1
push    ax
mov     ax, 0C599h
push    ax
mov     ax, 16CEh
push    ax
push    [bp+arg_2]
push    [bp+var_2]
push    [bp+var_6]
call    sub_11682
add     sp, 0Ch
jmp     short loc_1C98B

loc_1C966:
sub     ax, ax
push    ax
mov     al, [bp+arg_6]
sub     ah, ah
push    ax
mov     ax, 0C599h
push    ax
mov     ax, 16D6h
push    ax
push    [bp+var_2]
mov     bx, [bp+arg_2]
push    word ptr [bx]
push    [bp+var_6]
call    sub_115CB
add     sp, 0Eh
mov     ax, 0FFFFh

loc_1C98B:
pop     si
mov     sp, bp
pop     bp
retn
sub_1C8ED endp



; Attributes: bp-based frame

sub_1C990 proc near

var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
sub     sp, 2
push    di
push    si              ; draw
sub     ax, ax
push    ax
mov     ax, offset _window15
push    ax              ; win
call    setWindow
add     sp, 4
mov     [bp+var_2], 0
jmp     short loc_1C9BB

loc_1C9AC:
mov     bx, [bp+var_2]
shl     bx, 1
mov     ax, [bp+var_2]
mov     [bx-3240h], ax
inc     [bp+var_2]

loc_1C9BB:
mov     bx, [bp+arg_0]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_2]
ja      short loc_1C9AC
mov     bx, [bp+arg_4]
mov     di, [bp+arg_0]
mov     al, [di+52h]
mov     [bx], ax
mov     bx, [bp+arg_4]
mov     si, [bx]
mov     bx, [bp+arg_2]
cmp     [bx], si
jnz     short loc_1C9E6
or      si, si
jle     short loc_1C9E6
sub     word ptr [bx], 8

loc_1C9E6:
sub     ax, ax
push    ax
push    ax
mov     ax, 13E4h
push    ax
mov     ax, 16DEh
push    ax
mov     bx, [bp+arg_4]
push    word ptr [bx]
mov     bx, [bp+arg_2]
push    word ptr [bx]
push    [bp+arg_0]
call    sub_115CB
add     sp, 0Eh
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1C990 endp



; Attributes: bp-based frame

sub_1CA0B proc near

var_4= word ptr -4
y= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _window16
push    ax              ; win
call    setWindow
add     sp, 4
mov     _window16.field_14, 8
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset aConditions ; "Conditions:"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     _window16.field_14, 0
mov     [bp+var_4], 0
mov     [bp+y], 2
jmp     short loc_1CA80

loc_1CA46:
mov     bx, [bp+arg_0]
mov     al, [bx+58h]
sub     ah, ah
mov     dx, 1
mov     cl, byte ptr [bp+var_4]
shl     dx, cl
test    ax, dx
jz      short loc_1CA7D
push    [bp+y]          ; y
inc     [bp+y]
mov     ax, 3
push    ax              ; x
mov     ax, 16h
imul    [bp+var_4]
add     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 3EEh
push    dx
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1CA7D:
inc     [bp+var_4]

loc_1CA80:
cmp     [bp+var_4], 7
jl      short loc_1CA46
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+54h], 0
jz      short loc_1CADC
push    [bp+y]          ; y
inc     [bp+y]
mov     ax, 3
push    ax              ; x
mov     ax, word ptr _hdsPct
mov     dx, word ptr _hdsPct+2
add     ax, 488h
push    dx
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+arg_0]
cmp     byte ptr [bx+21h], 0
jz      short loc_1CADC
mov     al, [bx+21h]
sub     ah, ah
push    ax              ; char *
mov     ax, offset aRemissionExpDH ; "(remission exp:%d hrs.)"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
push    [bp+y]          ; y
inc     [bp+y]
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1CADC:
call    sub_1DC28
call    getKeypress
mov     sp, bp
pop     bp
retn
sub_1CA0B endp



; Attributes: bp-based frame

sub_1CAE6 proc near

var_2= byte ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 2
mov     [bp+var_2], 0
cmp     [bp+arg_0], 0FFFAh
jnz     short loc_1CAFE
mov     bx, [bp+arg_2]
mov     ax, [bp+arg_6]
jmp     short loc_1CB0A

loc_1CAFE:
cmp     [bp+arg_0], 0FFFBh
jnz     short loc_1CB10
mov     bx, [bp+arg_2]
mov     ax, [bp+arg_4]

loc_1CB0A:
mov     [bx], ax
mov     [bp+var_2], 1

loc_1CB10:
mov     al, [bp+var_2]
sub     ah, ah
mov     sp, bp
pop     bp
retn
sub_1CAE6 endp



; Attributes: bp-based frame

sub_1CB19 proc near

arg_0= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
mov     bx, [bp+arg_0]
shl     bx, 1
push    word ptr [bx-2518h] ; char *
mov     ax, offset aSIsIncapacitat ; "%s is incapacitated."
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
cmp     [bp+arg_2], 0
jz      short loc_1CB45
mov     ax, offset sprintf_buffer
push    ax
call    sub_1B29B
add     sp, 2
jmp     short loc_1CB56

loc_1CB45:
mov     ax, 2
push    ax              ; char
sub     ax, ax
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    sub_1B2F0
add     sp, 6

loc_1CB56:
pop     bp
retn
sub_1CB19 endp



; Attributes: bp-based frame

sub_1CB58 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
mov     bx, [bp+arg_0]
shl     bx, 1
mov     bx, [bx-2518h]
cmp     word ptr [bx+44h], 1
jge     short loc_1CB7B
mov     ax, 1
push    ax
push    [bp+arg_0]
call    sub_1CB19
add     sp, 4
sub     ax, ax
jmp     short loc_1CB7E

loc_1CB7B:
mov     ax, 1

loc_1CB7E:
pop     bp
retn
sub_1CB58 endp



; Attributes: bp-based frame

sub_1CB80 proc near

var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= byte ptr  0Ah
arg_8= byte ptr  0Ch

push    bp
mov     bp, sp
sub     sp, 2
cmp     [bp+arg_0], 3Bh ; ';'
jl      short loc_1CBD9
mov     al, _disk1._partyCount
sub     ah, ah
add     ax, 3Bh ; ';'
cmp     [bp+arg_0], ax
jnb     short loc_1CBD9
mov     ax, [bp+arg_0]
sub     ax, 3Bh ; ';'
mov     [bp+var_2], ax
mov     bx, ax
shl     bx, 1
push    word ptr [bx-2518h] ; member
call    getMemberCon
add     sp, 2
cmp     ax, [bp+arg_4]
jl      short loc_1CBC6
mov     al, [bp+arg_8]
sub     ah, ah
push    ax
push    [bp+var_2]
call    sub_1CB19
add     sp, 4
jmp     short loc_1CBD9

loc_1CBC6:
cmp     [bp+arg_6], 0
jnz     short loc_1CBD9
mov     bx, [bp+arg_2]
mov     ax, [bp+var_2]
mov     [bx], ax
mov     ax, 1
jmp     short loc_1CBDB

loc_1CBD9:
sub     ax, ax

loc_1CBDB:
mov     sp, bp
pop     bp
retn
sub_1CB80 endp



; Attributes: bp-based frame

sub_1CBDF proc near

var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_2], ax
cmp     byte_2ED26, 0
jz      short loc_1CBFE

loc_1CBF9:
mov     ax, [bp+arg_0]
jmp     short loc_1CC53

loc_1CBFE:
call    sub_1B452
mov     bx, [bp+var_2]
cmp     byte ptr [bx+144h], 0Bh
jnz     short loc_1CC2A
mov     al, 18h
mul     byte ptr [bx+147h]
mov     si, ax
mov     bx, _globals
test    byte ptr [bx+si+3Eh], 1
jz      short loc_1CC2A
push    _disk1._mapPosY
push    _disk1._mapPosX
call    sub_12D0A
jmp     short loc_1CC35

loc_1CC2A:
push    _disk1._mapPosY
push    _disk1._mapPosX
call    sub_10B95

loc_1CC35:
add     sp, 4
mov     bx, [bp+var_2]
mov     [bx+148h], ax
mov     [bx+14Ah], dx
mov     bx, [bp+var_2]
mov     ax, [bx+148h]
or      ax, [bx+14Ah]
jnz     short loc_1CBF9
mov     ax, 0FFFFh

loc_1CC53:
pop     si
mov     sp, bp
pop     bp
retn
sub_1CBDF endp



; Attributes: bp-based frame

characterInfo proc near

var_20= word ptr -20h
member= word ptr -1Eh
var_1C= word ptr -1Ch
var_1A= word ptr -1Ah
var_18= word ptr -18h
loopedFlag= byte ptr -16h
var_12= word ptr -12h
var_E= word ptr -0Eh
win= dword ptr -0Ch
var_6= word ptr -6
var_4= byte ptr -4
var_2= byte ptr -2
partyNum= word ptr  4
arg_2= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 24h
push    si
mov     [bp+member], 0
mov     [bp+loopedFlag], 0
cmp     [bp+arg_2], 0
jnz     short loc_1CC75
cmp     byte_2ED26, 0
jz      short loc_1CC79

loc_1CC75:
mov     al, 1
jmp     short loc_1CC7B

loc_1CC79:
sub     al, al

loc_1CC7B:
mov     [bp+var_2], al
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     [bp+var_6], 0
jmp     main_loop

loc_1CC8C:
mov     bx, [bp+partyNum]
shl     bx, 1
mov     si, _party[bx]
cmp     [bp+member], si
jz      short loc_1CCAC
mov     [bp+member], si
mov     [bp+var_18], 0
mov     [bp+var_1C], 0
mov     [bp+var_1A], 0

loc_1CCAC:
mov     ax, 1
push    ax
mov     ax, offset _windowCharInfo
push    ax              ; win
call    setWindow
add     sp, 4
cmp     [bp+var_6], 1
sbb     ax, ax
neg     ax
push    ax
push    [bp+member]     ; member
call    characterInfoStats
add     sp, 4
lea     ax, [bp+var_12]
push    ax
lea     ax, [bp+var_18]
push    ax
push    [bp+member]
call    sub_1C990
add     sp, 6
cmp     [bp+var_6], 1
jnz     short loc_1CCE8
mov     ax, 1
jmp     short loc_1CCEA

loc_1CCE8:
sub     ax, ax

loc_1CCEA:
push    ax
sub     ax, ax
push    ax
lea     ax, [bp+var_1C]
push    ax
push    [bp+partyNum]
call    sub_1C84A
add     sp, 8
cmp     [bp+var_6], 2
jnz     short loc_1CD06
mov     ax, 1
jmp     short loc_1CD08

loc_1CD06:
sub     ax, ax

loc_1CD08:
push    ax
sub     ax, ax
push    ax
lea     ax, [bp+var_1A]
push    ax
push    [bp+partyNum]
call    sub_1C8ED
add     sp, 8
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
mov     ax, [bp+var_6]
or      ax, ax
jz      short loc_1CD3D
cmp     ax, 1
jnz     short loc_1CD32
jmp     loc_1CF84

loc_1CD32:
cmp     ax, 2
jnz     short loc_1CD3A
jmp     loc_1D07F

loc_1CD3A:
jmp     main_loop

loc_1CD3D:
lea     ax, [bp+var_12]
push    ax
lea     ax, [bp+var_18]
push    ax
push    [bp+member]
call    sub_1C990
add     sp, 6
mov     [bp+var_E], ax
call    getKeypress
mov     [bp+var_4], al
cmp     al, 0FFh
jnz     short loc_1CD5E
jmp     loc_1D079

loc_1CD5E:
mov     bx, [bp+member]
mov     byte ptr [bx+144h], 0Ah
mov     al, [bp+var_4]
cbw
cmp     ax, 41h ; 'A'
jz      short loc_1CDC5 ; jumptable 0001CF52 cases 67,68,73,76,83,87
jle     short loc_1CD74
jmp     loc_1CF47

loc_1CD74:
cmp     ax, 0FFFCh
jnz     short loc_1CD7C
jmp     loc_1CE7C

loc_1CD7C:
jle     short loc_1CD81
jmp     loc_1CEE4

loc_1CD81:
cmp     ax, 0FFFAh
jge     short loc_1CD89
jmp     def_1CF52       ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85

loc_1CD89:
cmp     ax, 0FFFBh
jle     short loc_1CDB1
jmp     def_1CF52       ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85

loc_1CD91:
mov     ax, 1
push    ax
mov     al, [bp+var_2]
sub     ah, ah
push    ax
mov     ax, 6
push    ax
lea     ax, [bp+partyNum]
push    ax
mov     al, [bp+var_4]
cbw
push    ax
call    sub_1CB80
add     sp, 0Ah
jmp     main_loop

loc_1CDB1:
mov     ax, 1
push    ax
mov     ax, 2
push    ax
lea     ax, [bp+var_6]
push    ax
mov     al, [bp+var_4]
cbw
push    ax
jmp     loc_1D0CB

loc_1CDC5:              ; jumptable 0001CF52 cases 67,68,73,76,83,87
mov     al, [bp+var_4]
cbw
cmp     ax, 44h ; 'D'
jz      short loc_1CE07
jg      short loc_1CE39
cmp     ax, 41h ; 'A'
jz      short loc_1CE1B
cmp     ax, 43h ; 'C'
jz      short loc_1CE25
jmp     short loc_1CDE4

loc_1CDDC:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 0

loc_1CDE4:
push    [bp+partyNum]
call    sub_1CB58
add     sp, 2
or      al, al
jz      short loc_1CE4F

loc_1CDF1:
push    [bp+partyNum]
call    sub_1CBDF
add     sp, 2
jmp     loc_1D0E9

loc_1CDFD:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 1
jmp     short loc_1CDE4

loc_1CE07:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 2
jmp     short loc_1CDE4

loc_1CE11:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 3
jmp     short loc_1CDE4

loc_1CE1B:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 4
jmp     short loc_1CDE4

loc_1CE25:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 5
jmp     short loc_1CDE4

loc_1CE2F:
mov     bx, [bp+member]
mov     byte ptr [bx+145h], 6
jmp     short loc_1CDE4

loc_1CE39:
cmp     ax, 49h ; 'I'
jz      short loc_1CDFD
cmp     ax, 4Ch ; 'L'
jz      short loc_1CE2F
cmp     ax, 53h ; 'S'
jz      short loc_1CDDC
cmp     ax, 57h ; 'W'
jz      short loc_1CE11
jmp     short loc_1CDE4

loc_1CE4F:
mov     [bp+member], 0
jmp     main_loop

loc_1CE57:              ; jumptable 0001CF52 case 86
mov     bx, [bp+member]
cmp     byte ptr [bx+58h], 0
jnz     short loc_1CE63
jmp     main_loop

loc_1CE63:
push    bx
call    sub_1CA0B
add     sp, 2
jmp     main_loop

loc_1CE6D:
cmp     [bp+var_18], 8
jge     short loc_1CE76
jmp     main_loop

loc_1CE76:
sub     [bp+var_18], 8
jmp     short loc_1CE8E

loc_1CE7C:
mov     ax, [bp+var_12]
sub     ax, [bp+var_18]
cmp     ax, [bp+var_E]
jg      short loc_1CE8A
jmp     main_loop

loc_1CE8A:
add     [bp+var_18], 8

loc_1CE8E:
lea     ax, [bp+var_12]
push    ax
lea     ax, [bp+var_18]
push    ax
push    [bp+member]
call    sub_1C990
add     sp, 6
mov     [bp+var_E], ax
jmp     main_loop

loc_1CEA5:
mov     al, [bp+arg_2]
sub     ah, ah
push    ax
push    [bp+var_20]
push    [bp+partyNum]
call    sub_11FB7
add     sp, 6
or      al, al
jnz     short loc_1CEBE
jmp     main_loop

loc_1CEBE:
cmp     [bp+arg_2], 0
jz      short loc_1CED5
mov     bx, [bp+member]
cmp     byte ptr [bx+144h], 0Dh
jz      short loc_1CF41
cmp     byte ptr [bx+144h], 0Ch
jz      short loc_1CF41

loc_1CED5:
mov     bx, [bp+member]
cmp     byte ptr [bx+144h], 6

loc_1CEDD:
jz      short loc_1CEE2
jmp     loc_1CDF1

loc_1CEE2:
jmp     short loc_1CF41

loc_1CEE4:
cmp     ax, 0FFFDh
jz      short loc_1CE6D
cmp     ax, 3Bh ; ';'
jl      short def_1CF52 ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85
cmp     ax, 3Fh ; '?'
jg      short def_1CF52 ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85
jmp     loc_1CD91

def_1CF52:              ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85
mov     al, [bp+var_4]
cbw
mov     si, ax
test    byte ptr [si-4365h], 4
jnz     short loc_1CF06
jmp     main_loop

loc_1CF06:
and     ax, 0Fh
mov     [bp+var_20], ax
or      ax, ax
jg      short loc_1CF13
jmp     main_loop

loc_1CF13:
mov     ax, [bp+var_E]
cmp     [bp+var_20], ax
jle     short loc_1CF1E
jmp     main_loop

loc_1CF1E:
mov     bx, [bp+var_20]
add     bx, [bp+var_18]
shl     bx, 1
mov     ax, [bx-3242h]
mov     [bp+var_20], ax
mov     bx, [bp+member]
mov     al, byte ptr [bp+var_20]
mov     [bx+145h], al
cmp     byte_2ED26, 0
jnz     short loc_1CF41
jmp     loc_1CEA5

loc_1CF41:
mov     ax, [bp+partyNum]
jmp     loc_1D0E9

loc_1CF47:              ; switch 21 cases
sub     ax, 43h ; 'C'
cmp     ax, 14h
ja      short def_1CF52 ; jumptable 0001CF52 default case, cases 69-72,74,75,77-82,84,85
add     ax, ax
xchg    ax, bx
jmp     cs:jpt_1CF52[bx] ; switch jump
jpt_1CF52 dw offset loc_1CDC5 ; jump table for switch statement
dw offset loc_1CDC5
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset loc_1CDC5
dw offset def_1CF52
dw offset def_1CF52
dw offset loc_1CDC5
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset def_1CF52
dw offset loc_1CDC5
dw offset def_1CF52
dw offset def_1CF52
dw offset loc_1CE57
dw offset loc_1CDC5
jmp     main_loop

loc_1CF84:
mov     ax, 1
push    ax
push    ax
lea     ax, [bp+var_1C]
push    ax
push    [bp+partyNum]
call    sub_1C84A
add     sp, 8
mov     [bp+var_20], ax
mov     ax, 1
push    ax
mov     al, [bp+var_2]
sub     ah, ah
push    ax
mov     ax, 6
push    ax
lea     ax, [bp+partyNum]
push    ax
mov     ax, [bp+var_20]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jz      short loc_1CFBF
jmp     main_loop

loc_1CFBF:
mov     ax, 2
push    ax
sub     ax, ax
push    ax
lea     ax, [bp+var_6]
push    ax
push    [bp+var_20]
call    sub_1CAE6
add     sp, 8
or      al, al
jz      short loc_1CFDA
jmp     main_loop

loc_1CFDA:
cmp     [bp+var_20], 0
jge     short loc_1CFE3
jmp     loc_1D073

loc_1CFE3:
push    [bp+partyNum]
call    sub_1CB58
add     sp, 2
or      al, al
jnz     short loc_1CFF3
jmp     main_loop

loc_1CFF3:
mov     bx, [bp+member]
mov     si, [bp+var_20]
shl     si, 1
mov     al, [si-3240h]
mov     [bx+145h], al
mov     bx, [bp+member]
cmp     byte ptr [bx+145h], 0
jz      short loc_1D01B
cmp     byte ptr [bx+145h], 0Bh
jz      short loc_1D01B
cmp     byte ptr [bx+145h], 4
jnz     short loc_1D045

loc_1D01B:
cmp     byte_2ED26, 0
jz      short loc_1D025
jmp     main_loop

loc_1D025:
cmp     [bp+arg_2], 1
sbb     ax, ax
neg     ax
push    ax
mov     ax, 1
push    ax
push    [bp+partyNum]
call    sub_1C052
add     sp, 6
or      al, al
jz      short loc_1D042
jmp     loc_1CF41

loc_1D042:
jmp     main_loop

loc_1D045:
mov     bx, [bp+member]
mov     byte ptr [bx+144h], 0Ah
mov     bx, [bp+member]
add     byte ptr [bx+145h], 0Ch
cmp     [bp+arg_2], 0
jnz     short loc_1D05E
jmp     loc_1CDF1

loc_1D05E:
mov     bx, [bp+member]
cmp     byte ptr [bx+145h], 1Ah
jnz     short loc_1D06B
jmp     loc_1CF41

loc_1D06B:
cmp     byte ptr [bx+145h], 19h
jmp     loc_1CEDD

loc_1D073:
cmp     [bp+var_20], 0FFFFh
jnz     short main_loop

loc_1D079:
mov     [bp+loopedFlag], 1
jmp     short main_loop

loc_1D07F:
mov     ax, 1
push    ax
push    ax
lea     ax, [bp+var_1A]
push    ax
push    [bp+partyNum]
call    sub_1C8ED
add     sp, 8
mov     [bp+var_20], ax
mov     ax, 1
push    ax
mov     al, [bp+var_2]
sub     ah, ah
push    ax
mov     ax, 6
push    ax
lea     ax, [bp+partyNum]
push    ax
mov     ax, [bp+var_20]
add     ax, 46h ; 'F'
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jnz     short main_loop
cmp     [bp+var_20], 0FFFFh
jz      short loc_1D079
sub     ax, ax
push    ax
mov     ax, 1
push    ax
lea     ax, [bp+var_6]
push    ax
push    [bp+var_20]

loc_1D0CB:
call    sub_1CAE6
add     sp, 8

main_loop:
cmp     [bp+loopedFlag], 0
jnz     short loc_1D0DA
jmp     loc_1CC8C

loc_1D0DA:
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
mov     ax, 0FFFFh

loc_1D0E9:
pop     si
mov     sp, bp
pop     bp
retn
characterInfo endp



; Attributes: bp-based frame

sub_1D0EE proc near

member= word ptr -6
var_4= word ptr -4
var_2= dword ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 6
push    si              ; draw
mov     si, [bp+arg_0]
shl     si, 1
mov     ax, _party[si]
mov     [bp+member], ax
mov     ax, 1
push    ax
push    word_230BC[si]  ; win
call    setWindow
add     sp, 4
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+arg_0], ax
jb      short loc_1D11C
jmp     loc_1D236

loc_1D11C:              ; member
push    [bp+member]
call    getArmorClass
add     sp, 2
mov     word ptr [bp+var_2], ax
push    [bp+member]
mov     ax, [bp+arg_0]
inc     ax
push    ax              ; char *
mov     ax, offset aF1d1919s_0 ; "F%1d>%-19.19s"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 1
push    ax              ; brackets
push    [bp+member]     ; member
call    writeMemberCondition
add     sp, 4
sub     ax, ax
push    ax              ; y
mov     ax, 19h
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
push    [bp+member]     ; member
call    getMemberCon
add     sp, 2
mov     [bp+var_4], ax
or      ax, ax
jnz     short loc_1D18A
mov     bx, [bp+member]
push    word ptr [bx+46h]
push    word ptr [bx+44h]
mov     ax, 1735h
jmp     short loc_1D19C

loc_1D18A:
mov     bx, [bp+member]
push    word ptr [bx+46h]
mov     bx, [bp+var_4]
shl     bx, 1
push    word ptr [bx+1D0h] ; char *
mov     ax, offset aCon3s3u ; "Con:%3s /%3u"

loc_1D19C:
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 1
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
push    word ptr [bp+var_2] ; char *
mov     ax, offset aAc2d ; "AC: %2d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
mov     ax, 1
push    ax              ; y
mov     ax, 10h
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     bx, [bp+member]
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_1D1EA
sub     ax, ax
jmp     short loc_1D1F6

loc_1D1EA:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, [bx+si+64h]
sub     ah, ah

loc_1D1F6:
push    ax
cmp     byte ptr [bx+4Ah], 0FFh
jnz     short loc_1D202
mov     ax, 1757h
jmp     short loc_1D215

loc_1D202:
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
mov     al, 18h
mul     byte ptr [bx+si+62h]
add     ax, _globals
add     ax, 2Ah ; '*'

loc_1D215:              ; char *
push    ax
mov     ax, offset aWeapon1919sAmm ; "Weapon: %-19.19s   Ammo: %2d"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
mov     ax, 2
push    ax              ; y
sub     ax, ax
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8

loc_1D236:
pop     si
mov     sp, bp
pop     bp
retn
sub_1D0EE endp

push    bp
mov     bp, sp
sub     sp, 28h
push    si
mov     bx, [bp+6]
shl     bx, 1
mov     si, [bx-3240h]
cmp     si, 0FFh
jz      short loc_1D27E
mov     ax, 14Ch
mul     si
add     ax, 0BF24h
push    ax
push    word ptr [bp+4]
mov     ax, offset a1d1919s ; "%1d> %-19.19s"
push    ax
lea     ax, [bp-28h]
push    ax
call    _sprintf
add     sp, 8
mov     ax, [bp+4]
inc     ax
push    ax
sub     ax, ax
push    ax
lea     ax, [bp-28h]
push    ss
push    ax
call    writeStringAt
add     sp, 8

loc_1D27E:
pop     si
mov     sp, bp
pop     bp
retn


; Attributes: bp-based frame

sub_1D283 proc near

arg_0= byte ptr  4

push    bp              ; draw
mov     bp, sp
mov     ax, 1
push    ax
mov     ax, offset _window18
push    ax              ; win
call    setWindow
add     sp, 4
mov     al, _disk1.field_7
sub     ah, ah
push    ax
call    sub_1E045
add     sp, 2
push    word ptr _disk1._cash+2
push    word ptr _disk1._cash ; char *
mov     ax, offset aCashLu ; "Cash: $%lu"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 8
sub     ax, ax
push    ax              ; y
mov     ax, 16h
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 1
push    ax              ; y
mov     ax, 13h
push    ax              ; x
mov     ax, offset aREorderRoster ; "(R)eorder roster"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 2
push    ax              ; y
mov     ax, 13h
push    ax              ; x
mov     ax, offset aBAnishLEave ; "(B)anish   (L)eave"
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
cmp     [bp+arg_0], 0
jz      short loc_1D2F7
call    sub_1DC28

loc_1D2F7:
pop     bp
retn
sub_1D283 endp



; Attributes: bp-based frame

sub_1D2F9 proc near

var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= byte ptr -0Ah
var_8= byte ptr -8
var_6= byte ptr -6

push    bp
mov     bp, sp
sub     sp, 10h
push    di
push    si
mov     [bp+var_10], 0
mov     [bp+var_A], 0
cmp     _disk1._partyCount, 2
jnb     short loc_1D314
jmp     loc_1D4B9

loc_1D314:
mov     [bp+var_E], 0
jmp     short loc_1D330

loc_1D31B:
mov     bx, [bp+var_E]
shl     bx, 1
mov     si, [bp+var_E]
mov     al, [si-40E4h]
sub     ah, ah
mov     [bx-3240h], ax
inc     [bp+var_E]

loc_1D330:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_E], ax
jb      short loc_1D31B
mov     [bp+var_8], al
mov     _disk1._partyCount, ah
mov     [bp+var_E], 0

loc_1D346:
push    [bp+var_E]
call    sub_1D0EE
add     sp, 2
inc     [bp+var_E]
cmp     [bp+var_E], 5
jl      short loc_1D346
mov     ax, 1
push    ax
mov     ax, offset _window18
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _window19
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2

loc_1D37E:
cmp     [bp+var_A], 0
jz      short loc_1D387
jmp     loc_1D4B9

loc_1D387:
mov     [bp+var_10], 0
sub     ax, ax
push    ax
mov     ax, offset _window19
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax
mov     ax, 0D23Bh
push    ax
mov     ax, 17B7h
push    ax
lea     ax, [bp+var_10]
push    ax
mov     al, [bp+var_8]
sub     ah, ah
push    ax
push    _party
call    sub_11682
add     sp, 0Ch
mov     [bp+var_C], ax
cmp     ax, 0FFFFh
jnz     short loc_1D3F7
mov     al, [bp+var_8]
mov     _disk1._partyCount, al
mov     [bp+var_E], 0
jmp     short loc_1D3E7

loc_1D3CD:
mov     bx, [bp+var_E]
mov     al, [bx-40E4h]
sub     ah, ah
mov     cx, 14Ch
mul     cx
add     ax, 0BF24h
shl     bx, 1
mov     [bx-2518h], ax
inc     [bp+var_E]

loc_1D3E7:
mov     al, [bp+var_8]
sub     ah, ah
cmp     [bp+var_E], ax
jb      short loc_1D3CD

loc_1D3F1:
mov     [bp+var_A], 1
jmp     short loc_1D37E

loc_1D3F7:
cmp     [bp+var_C], 0
jl      short loc_1D37E
mov     si, [bp+var_C]
shl     si, 1
add     si, 0CDC0h
cmp     word ptr [si], 0FFh
jnz     short loc_1D40F
jmp     loc_1D37E

loc_1D40F:
mov     ax, 14Ch
mul     word ptr [si]
add     ax, 0BF24h
mov     bl, _disk1._partyCount
sub     bh, bh
shl     bx, 1
mov     [bx-2518h], ax
mov     al, _disk1._partyCount
inc     _disk1._partyCount
sub     ah, ah
mov     di, ax
mov     al, [si]
mov     [bp+di+var_6], al
mov     word ptr [si], 0FFh
mov     al, _disk1._partyCount
dec     ax
push    ax
call    sub_1D0EE
add     sp, 2
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
mov     al, [bp+var_8]
sub     ah, ah
dec     ax
mov     cl, _disk1._partyCount
sub     ch, ch
cmp     ax, cx
jz      short loc_1D45F
jmp     loc_1D37E

loc_1D45F:
mov     [bp+var_E], 0
jmp     short loc_1D4AC

loc_1D466:
mov     bx, [bp+var_E]
shl     bx, 1
mov     si, [bx-3240h]
cmp     si, 0FFh
jz      short loc_1D485
mov     al, _disk1._partyCount
inc     _disk1._partyCount
sub     ah, ah
mov     di, ax
mov     ax, si
mov     [bp+di+var_6], al

loc_1D485:
mov     si, [bp+var_E]
mov     al, [bp+si+var_6]
mov     bx, si
mov     [bx-40E4h], al
mov     si, [bp+var_E]
mov     al, [bp+si+var_6]
sub     ah, ah
mov     cx, 14Ch
mul     cx
add     ax, 0BF24h
mov     bx, si
shl     bx, 1
mov     [bx-2518h], ax
inc     [bp+var_E]

loc_1D4AC:
mov     al, [bp+var_8]
sub     ah, ah
cmp     [bp+var_E], ax
jb      short loc_1D466
jmp     loc_1D3F1

loc_1D4B9:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1D2F9 endp



; Attributes: bp-based frame

sub_1D4BF proc near

var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 6
push    si              ; draw
mov     [bp+var_6], 0
cmp     _disk1._partyCount, 1
jnz     short loc_1D4F0
sub     ax, ax
push    ax
mov     ax, offset _window18
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
mov     ax, 17C6h
push    ax              ; char *
call    sub_173AC
add     sp, 4
jmp     loc_1D5B5

loc_1D4F0:
mov     [bp+var_4], 0
jmp     short loc_1D50C

loc_1D4F7:
mov     bx, [bp+var_4]
shl     bx, 1
mov     si, [bp+var_4]
mov     al, [si-40E4h]
sub     ah, ah
mov     [bx-3240h], ax
inc     [bp+var_4]

loc_1D50C:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_1D4F7
mov     ax, 1
push    ax
mov     ax, offset _window18
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1
push    ax
mov     ax, offset _window19
push    ax              ; win
call    setWindow
add     sp, 4
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
sub     ax, ax
push    ax
mov     ax, offset _window19
push    ax              ; win
call    setWindow
add     sp, 4
sub     ax, ax
push    ax
mov     ax, 0D23Bh
push    ax
mov     ax, 17E0h
push    ax
lea     ax, [bp+var_6]
push    ax
mov     al, _disk1._partyCount
sub     ah, ah
push    ax
push    _party
call    sub_11682
add     sp, 0Ch
mov     [bp+var_2], ax
or      ax, ax
jl      short loc_1D5B5
call    winClearCurrent
mov     bx, [bp+var_2]
shl     bx, 1
push    _party[bx]      ; char *
mov     ax, offset aReallyBanishS ; "Really banish\r%s?\r\r"
push    ax
mov     ax, offset sprintf_buffer
push    ax              ; char *
call    _sprintf
add     sp, 6
sub     ax, ax
push    ax              ; y
push    ax              ; x
mov     ax, offset sprintf_buffer
push    ds
push    ax              ; text
call    writeStringAt
add     sp, 8
mov     ax, 0FFFFh
push    ax
mov     ax, 3
push    ax
call    sub_19780
add     sp, 4
or      al, al
jz      short loc_1D5B5
push    [bp+var_2]
call    sub_2045A
add     sp, 2

loc_1D5B5:
pop     si
mov     sp, bp
pop     bp
retn
sub_1D4BF endp



; Attributes: bp-based frame

sub_1D5BA proc near

var_E= word ptr -0Eh
var_C= byte ptr -0Ch
var_8= word ptr -8
var_6= byte ptr -6
win= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 0Eh
mov     [bp+var_E], 0
mov     ax, _currentWindow
mov     word ptr [bp+win], ax
mov     [bp+var_6], 0
mov     [bp+var_C], 1
sub     ax, ax
push    ax
mov     ax, offset _window3
push    ax              ; win
call    setWindow
add     sp, 4
call    winClearCurrent
mov     ax, 1
push    ax
mov     ax, offset _window4
push    ax              ; win
call    setWindow
add     sp, 4

loc_1D5F1:
cmp     [bp+var_6], 0
jz      short loc_1D5FA
jmp     loc_1D697

loc_1D5FA:
cmp     [bp+var_C], 0
jz      short loc_1D62E
mov     [bp+var_8], 0

loc_1D605:
push    [bp+var_8]
call    sub_1D0EE
add     sp, 2
inc     [bp+var_8]
cmp     [bp+var_8], 5
jl      short loc_1D605
sub     ax, ax
push    ax
call    sub_1D283
add     sp, 2
mov     ax, 1A4Eh
push    ax
call    sub_1DA17
add     sp, 2
mov     [bp+var_C], 0

loc_1D62E:
call    getKeypress
mov     byte ptr [bp+win+2], al
cmp     al, 0FFh
jz      short loc_1D63C
cmp     al, 4Ch ; 'L'
jnz     short loc_1D642

loc_1D63C:
mov     [bp+var_6], 1
jmp     short loc_1D5F1

loc_1D642:
mov     al, byte ptr [bp+win+2]
cbw
cmp     ax, 3Bh ; ';'
jl      short loc_1D5F1
cmp     ax, 3Fh ; '?'
jle     short loc_1D65C
cmp     ax, 42h ; 'B'
jz      short loc_1D692
cmp     ax, 52h ; 'R'
jz      short loc_1D688
jmp     short loc_1D5F1

loc_1D65C:
sub     ax, ax
push    ax
push    ax
mov     ax, 6
push    ax
lea     ax, [bp+var_E]
push    ax
mov     al, byte ptr [bp+win+2]
cbw
push    ax
call    sub_1CB80
add     sp, 0Ah
or      al, al
jnz     short loc_1D67A
jmp     loc_1D5F1

loc_1D67A:
sub     ax, ax
push    ax
push    [bp+var_E]
call    characterInfo
add     sp, 4
jmp     short loc_1D6A6

loc_1D688:
call    sub_1D2F9

loc_1D68B:
mov     [bp+var_C], 1
jmp     loc_1D5F1

loc_1D692:
call    sub_1D4BF
jmp     short loc_1D68B

loc_1D697:
sub     ax, ax
push    ax
push    word ptr [bp+win] ; win
call    setWindow
add     sp, 4
mov     ax, 0FFFFh

loc_1D6A6:
mov     sp, bp
pop     bp
retn
sub_1D5BA endp



; Attributes: bp-based frame

sub_1D6AA proc near

var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  4
arg_2= word ptr  6

push    bp
mov     bp, sp
sub     sp, 4
push    di
push    si
mov     bx, [bp+arg_0]
mov     al, byte ptr [bp+arg_2]
mov     [bx], al
mov     bx, [bp+arg_0]
mov     byte ptr [bx+1], 0
mov     bx, [bp+arg_0]
mov     byte ptr [bx+2], 0
mov     bx, [bp+arg_0]
mov     byte ptr [bx+3], 0
mov     ax, 18h
imul    [bp+arg_2]
mov     si, ax
add     si, _globals
test    byte ptr [si+3Eh], 1
jz      short loc_1D70C
mov     al, 3Ch ; '<'
mul     byte ptr [si+3Fh]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp+var_2], ax
mov     bx, ax
test    byte ptr [bx+18h], 1
jz      short loc_1D70C
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     [bp+var_4], ax
mov     bx, [bp+arg_0]
mov     di, ax
mov     al, [di+0Fh]
mov     [bx+2], al

loc_1D70C:
pop     si
pop     di
mov     sp, bp
pop     bp
retn
sub_1D6AA endp



; Attributes: bp-based frame

sub_1D712 proc near

var_12= word ptr -12h
var_10= word ptr -10h
var_E= word ptr -0Eh
var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= dword ptr  4

push    bp
mov     bp, sp
sub     sp, 12h
push    si
cmp     _disk1._partyCount, 5
jnz     short loc_1D734
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     ax, 1801h
push    ax
call    sub_1738A
add     sp, 6
jmp     loc_1D8C8

loc_1D734:
call    sub_170F2
mov     [bp+var_A], ax
mov     ax, 14Ch
imul    [bp+var_A]
add     ax, 0BF24h
mov     [bp+var_8], ax
mov     [bp+var_6], ds
mov     ax, word ptr [bp+arg_0+2]
mov     [bp+var_2], ax
mov     ax, word ptr [bp+arg_0]
mov     [bp+var_10], ax
mov     ax, ds
mov     [bp+var_4], ax
mov     ax, [bp+var_8]
mov     [bp+var_12], ax
mov     ax, 54h ; 'T'
push    ax              ; unsigned int
push    [bp+var_12]     ; unsigned int
push    [bp+var_4]      ; unsigned int
push    [bp+var_10]     ; unsigned int
push    [bp+var_2]      ; unsigned int
call    _movedata
add     sp, 0Ah
mov     bl, _disk1._partyCount
sub     bh, bh
mov     al, byte ptr [bp+var_A]
mov     [bx-40E4h], al
mov     ax, 14Ch
imul    [bp+var_A]
add     ax, 0BF24h
mov     bl, _disk1._partyCount
shl     bx, 1
mov     [bx-2518h], ax
mov     ax, 14Ch
imul    [bp+var_A]
add     ax, 0BF24h
mov     [bp+var_E], ax
mov     bx, ax
les     si, [bp+arg_0]
mov     al, es:[si+52h]
mov     [bx+52h], al
mov     [bp+var_C], 0
jmp     short loc_1D7D6

loc_1D7B5:
mov     si, [bp+var_C]
les     bx, [bp+arg_0]
mov     al, es:[bx+si+53h]
cbw
push    ax
mov     ax, 6
imul    si
add     ax, [bp+var_E]
add     ax, 62h ; 'b'
push    ax
call    sub_1D6AA
add     sp, 4
inc     [bp+var_C]

loc_1D7D6:
les     bx, [bp+arg_0]
mov     al, es:[bx+52h]
sub     ah, ah
cmp     ax, [bp+var_C]
ja      short loc_1D7B5
mov     [bp+var_C], 0

loc_1D7E9:
mov     si, [bp+var_C]
mov     bx, [bp+var_E]
mov     byte ptr [bx+si+124h], 0
mov     si, [bp+var_C]
mov     bx, [bp+var_E]
mov     byte ptr [bx+si+134h], 0
inc     [bp+var_C]
cmp     [bp+var_C], 10h
jl      short loc_1D7E9
mov     bx, [bp+var_E]
sub     ax, ax
mov     [bx+5Ch], ax
mov     [bx+5Ah], ax
mov     bx, [bp+var_E]
cmp     byte ptr [bx+4Eh], 0Dh
jbe     short loc_1D828
mov     word ptr [bx+5Eh], 0D40h
mov     word ptr [bx+60h], 3
jmp     short loc_1D846

loc_1D828:
mov     bx, [bp+var_E]
mov     si, bx
mov     si, [si+4Eh]
and     si, 0FFh
shl     si, 1
shl     si, 1
mov     ax, [si+0F06h]
mov     dx, [si+0F08h]
mov     [bx+5Eh], ax
mov     [bx+60h], dx

loc_1D846:
mov     bx, [bp+var_E]
mov     byte ptr [bx+58h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+53h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+54h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+21h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+57h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+56h], 0
mov     bx, [bp+var_E]
mov     byte ptr [bx+55h], 0
mov     bx, [bp+var_E]
mov     ax, [bx+14h]
mov     dx, [bx+16h]
add     word ptr _disk1._cash, ax
adc     word ptr _disk1._cash+2, dx
inc     _disk1._partyCount
les     bx, [bp+arg_0]
mov     bl, es:[bx+49h]
sub     bh, bh
or      byte ptr [bx-3A60h], 1
sub     ax, ax
push    ax
les     bx, [bp+arg_0]
mov     al, es:[bx+5Ch]
and     ax, 7Fh
push    ax
mov     al, es:[bx+5Bh]
cbw
push    ax
call    sub_12AB2
add     sp, 6
mov     ax, 1
push    ax
push    ax
push    word ptr [bp+arg_0+2]
push    word ptr [bp+arg_0]
call    sub_18607
add     sp, 8
call    sub_17E09

loc_1D8C8:
pop     si
mov     sp, bp
pop     bp
retn
sub_1D712 endp



; Attributes: bp-based frame

sub_1D8CD proc near

var_2= word ptr -2
arg_0= word ptr  4

push    bp
mov     bp, sp
sub     sp, 2
mov     ax, 1
push    ax
call    sub_1BB93
add     sp, 2
mov     [bp+var_2], ax
mov     ax, 68h ; 'h'
imul    [bp+arg_0]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr dword_2E4C0, ax
mov     word ptr dword_2E4C0+2, dx
push    dx
push    ax              ; char *
les     bx, dword_2E4C0
mov     al, es:[bx+63h]
sub     ah, ah
push    ax              ; int
call    sub_1B360
add     sp, 6
mov     ax, 1
push    ax
push    ax
push    [bp+var_2]
call    sub_107E1
mov     byte_2ED26, 1
mov     sp, bp
pop     bp
retn
sub_1D8CD endp

; START OF FUNCTION CHUNK FOR gfxInit

loc_1D91C:
cli
mov     al, 3
mov     ah, 0
int     10h             ; - VIDEO - SET VIDEO MODE
                        ; AL = mode
mov     dx, offset aYouDoNotHaveEn_0 ; "You do not have enough memory to run Fo"...
mov     ah, 9
int     21h             ; DOS - PRINT STRING
                        ; DS:DX -> string terminated by "$"
mov     al, 0
mov     ah, 4Ch
int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
; END OF FUNCTION CHUNK FOR gfxInit ; AL = exit code
retn



gfxInit proc near

; FUNCTION CHUNK AT D91C SIZE 00000014 BYTES

push    si
push    di
push    bx
push    cx
push    dx
push    ds
push    es
push    bp
mov     ah, 48h ; 'H'
mov     bx, 45F5h
int     21h             ; DOS - 2+ - ALLOCATE MEMORY
                        ; BX = number of 16-byte paragraphs desired
jb      short loc_1D91C
mov     word ptr _picDecoded+2, ax
add     ax, 8000
mov     word ptr _services+2, ax
add     ax, 47
mov     word ptr _archetypesData+2, ax
add     ax, 0A5h
mov     word ptr _hdsPct+2, ax
add     ax, 74
mov     word ptr _fontData+2, ax
add     ax, 419
mov     word ptr _borders+2, ax
add     ax, 313
mov     word_22C74, ax
add     ax, 2944
mov     word ptr dword_22C6E+2, ax
add     ax, 2250
lea     di, dst
mov     [di+2], ax
add     ax, 43Ah
mov     [di+6], ax
add     ax, 43Ah
mov     [di+0Ah], ax
add     ax, 43Ah
mov     word_22C48, ax
mov     ax, 20FAh
shr     ax, 1
shr     ax, 1
shr     ax, 1
shr     ax, 1
add     ax, 1
mov     bx, ds
add     ax, bx
mov     word ptr _picDecoded_0+2, ax
pop     bp
pop     es
pop     ds
pop     dx
pop     cx
pop     bx
pop     di
pop     si
retn
gfxInit endp




sub_1D9A9 proc near
push    si
push    di
push    bx
push    cx
push    dx
push    ds
push    es
push    bp
call    sub_20734
call    setCriticalFn
mov     cx, 3E80h
xor     di, di
mov     es, word ptr _picDecoded_0+2
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
sub_1D9A9 endp




drawBordersAndWelcome proc near
push    si
push    di
push    bx
push    cx
push    dx
push    ds
push    es
push    bp
lea     si, _windowInfo
mov     _currentWindow, si
call    drawBorders
mov     ax, ds
mov     es, ax          ; str
assume es:dseg
mov     di, offset str  ; "Welcome to the beautiful island of Flor"...
call    writeString2
pop     bp
pop     es
assume es:nothing
pop     ds
pop     dx
pop     cx
pop     bx
pop     di
pop     si
retn
drawBordersAndWelcome endp



; Attributes: bp-based frame

sub_1D9F5 proc near

arg_0= word ptr  4

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
call    sub_2077F
pop     bp
pop     es
pop     ds
pop     di
pop     si
pop     bp
retn
sub_1D9F5 endp



; Attributes: bp-based frame

sub_1DA17 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
push    si
push    di
push    ds
push    es
push    bp
mov     si, [bp+arg_0]
push    si
call    sub_1D9F5
add     sp, 2
call    sub_208E2
pop     bp
pop     es
pop     ds
pop     di
pop     si
pop     bp
retn
sub_1DA17 endp

push    si
push    di
push    bp
call    sub_208E2
pop     bp
pop     di
pop     si
retn



; void __usercall drawBorders_1(int dataOffset@<ax>)
drawBorders_1 proc near
push    di
push    si
push    es
mov     si, ax
mov     ax, _textX
push    ax
mov     ax, _textY
push    ax
mov     _textY, 0

loc_1DA54:
mov     _textX, 0

loc_1DA5A:
push    ds
mov     ds, word ptr _borders+2
lodsb
pop     ds
or      al, al
jz      short loc_1DA6A
push    si
call    writeBorderCell
pop     si

loc_1DA6A:
inc     _textX
cmp     _textX, 40
jnz     short loc_1DA5A
inc     _textY
cmp     _textY, 25
jnz     short loc_1DA54
pop     ax
mov     _textY, ax
pop     ax
mov     _textX, ax
pop     es
pop     si
pop     di
retn
drawBorders_1 endp




sub_1DA8C proc near
lea     di, unk_22BA4   ; bytes
call    drawBorderVert
lea     di, unk_22BAE   ; bytes
call    drawBorderVert
retn
sub_1DA8C endp




sub_1DA9B proc near
lea     di, unk_22B87   ; bytes
call    drawBorderHoriz
retn
sub_1DA9B endp




drawBorders2000 proc near
mov     ax, 2000        ; dataOffset
call    drawBorders_1
retn
drawBorders2000 endp




drawBorders proc near
xor     ax, ax          ; dataOffset
call    drawBorders_1
retn
drawBorders endp




windowCharStats_draw proc near
mov     ax, 1000        ; dataOffset
call    drawBorders_1
retn
windowCharStats_draw endp




drawBorders4000 proc near
mov     ax, 4000        ; dataOffset
call    drawBorders_1
lea     di, unk_22B6F
call    sub_1DDC5
retn
drawBorders4000 endp




drawBorders3000 proc near
mov     ax, 3000        ; dataOffset
call    drawBorders_1
lea     di, unk_22B6F
call    sub_1DDC5
retn
drawBorders3000 endp




dialogDrawBorder proc near
lea     di, _dialogBorderTop ; bytes
call    drawBorderHoriz
lea     di, _dialogBorderLeft ; bytes
call    drawBorderVert
lea     di, _dialogBorderRight ; bytes
call    drawBorderVert
lea     di, _dialogBorderBottom ; bytes
call    drawBorderHoriz
retn
dialogDrawBorder endp




sub_1DAF0 proc near
lea     di, unk_22B33   ; bytes
call    drawBorderVert
lea     di, unk_22B42   ; bytes
call    drawBorderVert
retn
sub_1DAF0 endp




sub_1DAFF proc near
push    si
push    di
push    ds
push    es
push    bp
lea     di, unk_22B51   ; bytes
call    drawBorderVert
lea     di, unk_22B60   ; bytes
call    drawBorderVert
pop     bp
pop     es
pop     ds
pop     di
pop     si
retn
sub_1DAFF endp



; Attributes: bp-based frame

; void __cdecl setWindow(Window *win, bool draw)
setWindow proc near

win= dword ptr  4
draw= byte ptr  8

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
jz      short loc_1DB43
push    si
mov     [si+Window.field_18], 0
call    winClearCurrent
pop     si
cmp     [si+Window._fnDraw], 0
jz      short loc_1DB43
call    [si+Window._fnDraw]

loc_1DB43:
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
setWindow endp




winClearCurrent proc near
mov     si, _currentWindow
add     si, Window._attr
call    winClear
retn
winClearCurrent endp




sub_1DB58 proc near
push    ds
mov     si, di
lea     di, unk_2323A
push    ds
push    es
pop     ds
pop     es
push    di

loc_1DB64:
lodsb
or      al, al
jz      short loc_1DB87
cmp     al, '^'
jnz     short loc_1DB84
push    si
push    ds
mov     ax, seg dseg
mov     ds, ax
mov     si, word_22C8A

loc_1DB78:
lodsb
or      al, al
jz      short loc_1DB80
stosb
jmp     short loc_1DB78

loc_1DB80:
pop     ds
pop     si
jmp     short loc_1DB64

loc_1DB84:
stosb
jmp     short loc_1DB64

loc_1DB87:
stosb
pop     di
pop     ds
retn
sub_1DB58 endp




writeString proc near
cmp     byte_22BB9, 0
jz      short writeString2
call    sub_1DB58

writeString2:
mov     _writeStringPtr1, di
mov     _writeStringPtr2, di
mov     _writeStringPtr3, di

loc_1DBA1:
mov     si, _currentWindow
mov     al, es:[di]
or      al, al
jz      short sub_1DBF9
cmp     al, 0Dh
jnz     short loc_1DBBB
call    sub_1DBF9
push    di
call    sub_1DDFD
pop     di
inc     di              ; str
jmp     short writeString2

loc_1DBBB:
push    ax
mov     cx, di
sub     cx, _writeStringPtr1
mov     ax, [si+8]
add     ax, cx
cmp     ax, [si+4]
jg      short loc_1DBDF
pop     ax
cmp     al, 20h ; ' '
jnz     short loc_1DBDC
mov     _writeStringPtr3, di
inc     di
mov     _writeStringPtr2, di
jmp     short loc_1DBA1

loc_1DBDC:
inc     di
jmp     short loc_1DBA1

loc_1DBDF:
pop     ax
cmp     al, 20h ; ' '
jnz     short loc_1DBED
mov     _writeStringPtr3, di
inc     di
mov     _writeStringPtr2, di

loc_1DBED:
call    sub_1DBFD
call    sub_1DDFD
mov     di, _writeStringPtr2 ; str
jmp     short writeString2
writeString endp




sub_1DBF9 proc near
mov     _writeStringPtr3, di
sub_1DBF9 endp




sub_1DBFD proc near
mov     cx, _writeStringPtr3
sub     cx, _writeStringPtr1
jz      short locret_1DC0A
call    sub_1DC0B

locret_1DC0A:
retn
sub_1DBFD endp




sub_1DC0B proc near
mov     di, _writeStringPtr1

loc_1DC0F:
push    cx
mov     al, es:[di]
push    di
mov     si, _currentWindow
call    writeChar
mov     si, _currentWindow
inc     word ptr [si+8]
pop     di
inc     di
pop     cx
loop    loc_1DC0F
retn
sub_1DC0B endp




sub_1DC28 proc near
push    si
push    di
push    bp
mov     si, _currentWindow
cmp     [si+Window.field_1A], 0
lea     di, [si+Window._attr]
jz      short loc_1DC3B
mov     di, [si+Window.field_1A]

loc_1DC3B:
push    di
call    sub_1DA17
add     sp, 2
pop     bp
pop     di
pop     si
retn
sub_1DC28 endp




sub_1DC46 proc near
mov     ax, word ptr dword_22C6E+2
mov     es, ax
xor     si, si
mov     ax, es:[si+4]
and     ax, 7FFFh
mov     word_230E2, ax
mov     ax, _disk1._mapPosX
sub     ax, 9
mov     word_230DA, ax
mov     ax, _disk1._mapPosY
sub     ax, 4
mov     word_230DC, ax
lea     di, unk_230E4
mov     word_230D6, 0
mov     word_230D8, 0

loc_1DC79:
mov     ax, word_230D6
add     ax, word_230DA
mov     word_230DE, ax
cmp     ax, es:[si]
jge     short loc_1DCE6
mov     ax, word_230D8
add     ax, word_230DC
mov     word_230E0, ax
cmp     ax, es:[si+2]
jge     short loc_1DCE6
mov     ax, 8000h
test    word_230DE, ax
jnz     short loc_1DCE6
test    word_230E0, ax
jnz     short loc_1DCE6
push    si
mov     cx, es:[si]
mov     ax, word_230E0
mul     cx
add     ax, word_230DE
shl     ax, 1
shl     ax, 1
shl     ax, 1
add     si, 414h
add     si, ax
mov     ax, es:[si]
test    byte ptr es:[si+2], 40h
jz      short loc_1DCD5
and     ax, 7FFh
or      ax, 1800h
jmp     short loc_1DCE2
db 90h

loc_1DCD3:
jmp     short loc_1DC79

loc_1DCD5:
test    byte ptr es:[si+2], 80h
jz      short loc_1DCE2
and     ax, 7FFh
or      ax, 1000h

loc_1DCE2:
pop     si
jmp     short loc_1DCE9
align 2

loc_1DCE6:
mov     ax, word_230E2

loc_1DCE9:
mov     [di], ax
inc     di
inc     di
inc     word_230D6
cmp     word_230D6, 13h
jnz     short loc_1DC79
mov     word_230D6, 0
inc     word_230D8
cmp     word_230D8, 9
jnz     short loc_1DCD3
lea     di, word_2318E
and     word ptr [di], 7FFh
or      word ptr [di], 800h
call    sub_1DD19
retn
sub_1DC46 endp




sub_1DD19 proc near
push    di
push    si
push    ds
push    bp
lea     si, unk_230E4
mov     word_230D8, 0

loc_1DD27:
mov     word_230D6, 0

loc_1DD2D:
lodsw
call    sub_1E098
inc     word_230D6
cmp     word_230D6, 13h
jnz     short loc_1DD2D
inc     word_230D8
cmp     word_230D8, 9
jnz     short loc_1DD27
pop     bp
pop     ds
pop     si
pop     di
retn
sub_1DD19 endp




sub_1DD4C proc near
push    bp
push    si
call    sub_1DC46
lea     si, unk_22C9E
push    si
call    sub_1DA17
add     sp, 2
pop     si
pop     bp
retn
sub_1DD4C endp




; void __usercall drawBorderHoriz(const void near *bytes@<di>)
drawBorderHoriz proc near
mov     ax, _textX
push    ax
mov     ax, _textY
push    ax
xor     ax, ax
mov     al, [di]
mov     _textX, ax
mov     al, [di+1]
mov     _textY, ax
add     di, 2

loc_1DD77:
mov     al, [di]
cmp     al, 0FFh
jz      short loc_1DD89
push    di
call    writeBorderCell
inc     _textX
pop     di
inc     di
jnz     short loc_1DD77

loc_1DD89:
pop     ax
mov     _textY, ax
pop     ax
mov     _textX, ax
retn
drawBorderHoriz endp




; void __usercall drawBorderVert(const void near *bytes@<di>)
drawBorderVert proc near
mov     ax, _textX
push    ax
mov     ax, _textY
push    ax
xor     ax, ax
mov     al, [di]
mov     _textX, ax
mov     al, [di+1]
mov     _textY, ax
add     di, 2

loc_1DDAA:
mov     al, [di]
cmp     al, 0FFh
jz      short loc_1DDBC
push    di
call    writeBorderCell
inc     _textY
pop     di
inc     di
jnz     short loc_1DDAA

loc_1DDBC:
pop     ax
mov     _textY, ax
pop     ax
mov     _textX, ax
retn
drawBorderVert endp




sub_1DDC5 proc near
mov     si, offset _window3
push    word ptr [si+14h]
mov     word ptr [si+14h], 4
xor     ax, ax
mov     al, [di]
mov     [si+8], ax
mov     al, [di+1]
mov     [si+0Ah], ax
add     di, 2

loc_1DDE0:
mov     al, [di]
cmp     al, 0FFh
jz      short loc_1DDF9
push    di
or      al, al
jz      short loc_1DDEE
call    writeChar

loc_1DDEE:
inc     word ptr [si+8]
inc     _textX
pop     di
inc     di
jnz     short loc_1DDE0

loc_1DDF9:
pop     word ptr [si+14h]
retn
sub_1DDC5 endp




sub_1DDFD proc near
push    es
mov     si, _currentWindow
mov     ax, [si]
mov     [si+8], ax
inc     word ptr [si+0Ah]
mov     ax, [si+0Ah]
cmp     ax, [si+6]
jle     short loc_1DE3D
mov     ax, [si+6]
mov     [si+0Ah], ax
push    si
cmp     byte_22A74, 0
jz      short loc_1DE39
mov     ax, [si+6]
sub     ax, [si+2]
inc     word ptr [si+18h]
cmp     [si+18h], ax
jle     short loc_1DE39
mov     word ptr [si+18h], 0
call    sub_1DC28
call    getKeypress

loc_1DE39:
pop     si
call    sub_1E18D

loc_1DE3D:
pop     es
retn
sub_1DDFD endp

retn


; Attributes: bp-based frame

; int __cdecl writeStringAt(const char far *text, int x, int y)
writeStringAt proc near

text= dword ptr  4
x= word ptr  8
y= word ptr  0Ah

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
jz      short loc_1DE5E
mov     ax, [si+Window._x1]
add     ax, bx
mov     [si+Window._textX], ax

loc_1DE5E:
mov     bx, [bp+y]
cmp     bx, 0FFFFh
jz      short loc_1DE6E
mov     ax, [si+Window._y1]
add     ax, bx
mov     [si+Window._textY], ax

loc_1DE6E:
mov     es, word ptr [bp+text+2]
mov     di, word ptr [bp+text]
call    writeString
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
writeStringAt endp



; Attributes: bp-based frame

sub_1DE81 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

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
mov     ax, [si]
add     ax, [bp+arg_2]
mov     [si+8], ax
mov     ax, [si+2]
add     ax, [bp+arg_4]
mov     [si+0Ah], ax
mov     ax, [bp+arg_0]
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
sub_1DE81 endp



; Attributes: bp-based frame

sub_1DEB1 proc near

arg_0= word ptr  4
arg_2= word ptr  6

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
jge     short loc_1DED1

loc_1DEC4:
call    sub_1DEE7
cmp     ax, [bp+arg_0]
jl      short loc_1DEC4
cmp     ax, [bp+arg_2]
jg      short loc_1DEC4

loc_1DED1:
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
sub_1DEB1 endp




sub_1DEDB proc near
push    si
push    bx
push    cx
push    dx
call    sub_1DEE7
pop     dx
pop     cx
pop     bx
pop     si
retn
sub_1DEDB endp




sub_1DEE7 proc near
lea     si, unk_22A64
mov     cx, 0Fh
mov     dl, [si+0Fh]
mov     bx, 0Eh
clc

loc_1DEF5:
mov     al, [bx+si]
adc     dl, al
mov     [bx+si], dl
dec     bx
loop    loc_1DEF5
mov     cx, 10h
mov     bx, 10h

loc_1DF04:
dec     bx
inc     byte ptr [bx+si]
loopne  loc_1DF04
mov     al, [si]
retn
sub_1DEE7 endp

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
pop     es
pop     di
pop     si
pop     bx
retn



sub_1DF22 proc near
xor     ax, ax
mov     al, es:[di]
clc
add     di, 1
jnb     short locret_1DF35
mov     cx, es
add     cx, 1000h
mov     es, cx
assume es:nothing

locret_1DF35:
retn
sub_1DF22 endp

mov     ax, es:[di]
clc
add     di, 2
jnb     short locret_1DF47
mov     cx, es
add     cx, 1000h
mov     es, cx
assume es:nothing

locret_1DF47:
retn


; Attributes: bp-based frame

sub_1DF48 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8

push    bp
mov     bp, sp
push    cx
push    bx
push    di
push    si
push    es
push    bp
mov     si, [bp+arg_0]
mov     di, [si]
mov     es, word ptr [si+2]
assume es:nothing
call    sub_1DF22
mov     bx, ax
and     al, 0Fh
mov     si, [bp+arg_2]
mov     [si], ax
add     si, 2
cmp     ax, [bp+arg_4]
jnz     short loc_1DF99
call    sub_1DF22
mov     [si], ax
add     si, 2
shr     bx, 1
shr     bx, 1
shr     bx, 1
mov     dx, si
lea     si, off_22C2E
add     si, bx
mov     si, [si]

loc_1DF85:
lodsw
or      ax, ax
jz      short loc_1DFB6
call    ax
push    si
mov     si, dx
mov     [si], ax
add     si, 2
mov     dx, si
pop     si
jmp     short loc_1DF85

loc_1DF99:
xor     ah, ah
mov     al, es:[di]
lea     si, unk_22BC1
add     si, ax
xor     ah, ah
mov     al, [si]
inc     ax
clc
add     di, ax
jnb     short loc_1DFB6
mov     cx, es
add     cx, 1000h
mov     es, cx
assume es:nothing

loc_1DFB6:
mov     si, [bp+arg_0]
mov     [si], di
mov     word ptr [si+2], es
pop     bp
pop     es
assume es:nothing
pop     si
pop     di
pop     bx
pop     cx
pop     bp
retn
sub_1DF48 endp



; Attributes: bp-based frame

sub_1DFC6 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
push    si
push    bp
mov     si, [bp+arg_0]
cmp     si, 5
jz      short loc_1DFEA
mov     word_22C88, si
dec     si
mov     ax, word_22C86
shl     ax, 1
shl     ax, 1
add     si, ax
shl     si, 1
mov     ax, [si+1A36h]
mov     word_2B05C, ax

loc_1DFEA:
pop     bp
pop     si
pop     bp
retn
sub_1DFC6 endp




sub_1DFEE proc near
push    di
push    si
push    ds
push    bp
mov     word_230D6, 9
mov     word_230D8, 4
mov     ax, word_2318E
or      ax, 800h
call    sub_1E098
lea     si, unk_22CBE
push    si
call    sub_1DA17
add     sp, 2
pop     bp
pop     ds
pop     si
pop     di
retn
sub_1DFEE endp




sub_1E017 proc near
push    ds
mov     es, word ptr _picDecoded_0+2
mov     si, _currentWindow
mov     di, [si+0Eh]
add     di, 4
shl     di, 1
mov     di, [di-5F3Fh]
add     di, word_230D6
mov     ds, bx
xor     si, si
mov     bx, 10h

loc_1E037:
mov     cx, 4
rep movsw
add     di, 98h
dec     bx
jnz     short loc_1E037
pop     ds
retn
sub_1E017 endp



; Attributes: bp-based frame

sub_1E045 proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
push    bx
push    cx
push    di
push    si
push    ds
push    es
push    bp
mov     word_230D6, 0Ah
mov     bx, word ptr _picDecoded+2
add     bx, 1EA8h
mov     cx, 4
mov     ax, 1

loc_1E063:
push    ax
push    cx
push    bx
test    [bp+arg_0], al
jz      short loc_1E079
call    sub_1E017
pop     bx
pop     cx
pop     ax
add     word_230D6, 10h
jmp     short loc_1E07C
db 90h

loc_1E079:
pop     bx
pop     cx
pop     ax

loc_1E07C:
add     bx, 8
shl     ax, 1
loop    loc_1E063
pop     bp
pop     es
pop     ds
pop     si
pop     di
pop     cx
pop     bx
pop     bp
retn
sub_1E045 endp




sub_1E08C proc near
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
sub_1E08C endp

align 2



sub_1E098 proc near
push    si
push    di
push    ds
push    es
push    ax
xor     bx, bx
and     ax, 7FFh
cmp     ax, 200h
jl      short loc_1E0AD
sub     ax, 200h
mov     bx, 1000h

loc_1E0AD:
push    bx
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
mov     si, ax
mov     es, word ptr _picDecoded_0+2
mov     bx, word_230D8
shl     bx, 1
mov     di, [bx-61BCh]
mov     di, [di-5F3Fh]
mov     bx, word_230D6
shl     bx, 1
add     di, [bx-61AAh]
pop     cx
pop     bx
xor     dx, dx
and     bx, 0F800h
jz      short loc_1E0F2
xchg    bh, bl
shr     bx, 1
shr     bx, 1
mov     dx, [bx-61E6h]
add     dx, word ptr _picDecoded+2

loc_1E0F2:
mov     ax, word ptr _picDecoded+2
add     ax, cx
mov     ds, ax
push    di
mov     bx, 10h

loc_1E0FD:
mov     cx, 4
rep movsw
add     di, 98h
dec     bx
jnz     short loc_1E0FD
pop     di
or      dx, dx
jz      short loc_1E13C
mov     ds, dx
mov     si, 0
mov     bx, 10h

loc_1E116:
mov     cx, 8

loc_1E119:
mov     al, [si]
and     al, 0F0h
jz      short loc_1E126
and     byte ptr es:[di], 0Fh
or      es:[di], al

loc_1E126:
lodsb
and     al, 0Fh
jz      short loc_1E132
and     byte ptr es:[di], 0F0h
or      es:[di], al

loc_1E132:
inc     di
loop    loc_1E119
add     di, 98h
dec     bx
jnz     short loc_1E116

loc_1E13C:
pop     es
pop     ds
pop     di
pop     si
retn
sub_1E098 endp




writeBorderCell proc near
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
mov     di, _scanlinePtrs[di]
mov     ax, _textX
shl     ax, 1
shl     ax, 1
add     di, ax
mov     es, word ptr _picDecoded_0+2
pop     ax
xor     ah, ah
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
mov     ds, word ptr _fontData+2
mov     si, ax
mov     bx, 8

loc_1E17B:
mov     cx, 2
rep movsw
add     di, 9Ch
dec     bx
jnz     short loc_1E17B
pop     bp
pop     es
pop     ds
pop     di
pop     si
retn
writeBorderCell endp




sub_1E18D proc near
push    si
mov     ax, [si+0Eh]
shl     ax, 1
mov     di, ax
mov     di, [di-5F3Fh]
add     di, [si+0Ch]
mov     cx, [si+12h]
mov     bx, [si+10h]
mov     si, di
cmp     word_22C44, 5
jge     short loc_1E1BA
sub     cx, 1
shr     bx, 1
mov     dx, 8
add     si, 0A0h
jmp     short loc_1E1DA
align 2

loc_1E1BA:
cmp     word_22C44, 7
jge     short loc_1E1CE
sub     cx, 2
mov     dx, 4
add     si, 140h
jmp     short loc_1E1DA
align 2

loc_1E1CE:
sub     cx, 4
mov     dx, 2
shl     bx, 1
add     si, 280h

loc_1E1DA:
mov     es, word ptr _picDecoded_0+2

loc_1E1DE:
push    dx
push    di
push    si
push    cx
push    ds
mov     ax, es
mov     ds, ax

loc_1E1E7:
push    cx
push    di
push    si
mov     cx, bx
rep movsw
mov     ax, 0A0h
sub     ax, bx
pop     si
pop     di
add     di, 0A0h
add     si, 0A0h
pop     cx
loop    loc_1E1E7
pop     ds
xor     ax, ax
mov     cx, bx
rep stosw
push    bx
mov     si, _currentWindow
mov     ax, [si+0Ch]
mov     di, [si+10h]
mov     cx, [si+12h]
mov     si, [si+0Eh]
call    sub_2077F
call    sub_208E2
mov     si, word_22C44
or      si, si
jz      short loc_1E27D
dec     si
shl     si, 1
mov     cx, [si-61F6h]

loc_1E231:
push    cx
push    es
xor     ax, ax
push    ax
call    sub_16B44
add     sp, 2
pop     es
pop     cx
loop    loc_1E231
mov     dl, 0FFh
mov     ah, 6
int     21h             ; DOS - DIRECT CONSOLE I/O CHARACTER OUTPUT
                        ; DL = character <> FFh
                        ;  Return: ZF set = no character
                        ;   ZF clear = character recieved, AL = character
jz      short loc_1E27D
or      al, al
jnz     short loc_1E252
mov     dl, 0FFh
mov     ah, 6
int     21h             ; DOS - DIRECT CONSOLE I/O CHARACTER OUTPUT
                        ; DL = character <> FFh
                        ;  Return: ZF set = no character
                        ;   ZF clear = character recieved, AL = character

loc_1E252:
cmp     al, 2Ch ; ','
jz      short loc_1E25A
cmp     al, 3Ch ; '<'
jnz     short loc_1E269

loc_1E25A:
cmp     word_22C44, 1
jz      short loc_1E27D
sub     word_22C44, 1
jmp     short loc_1E27D
db 90h

loc_1E269:
cmp     al, 2Eh ; '.'
jz      short loc_1E271
cmp     al, 3Eh ; '>'
jnz     short loc_1E27D

loc_1E271:
cmp     word_22C44, 8
jz      short loc_1E27D
add     word_22C44, 1

loc_1E27D:
pop     bx
pop     cx
pop     si
pop     di
pop     dx
dec     dx
jz      short loc_1E288
jmp     loc_1E1DE

loc_1E288:
pop     si
retn
sub_1E18D endp




winClear proc near
push    si
push    di
push    es
push    bp
mov     es, word ptr _picDecoded_0+2
mov     di, [si+2]
shl     di, 1
mov     di, [di-5F3Fh]
add     di, [si]
xor     ax, ax
mov     cx, [si+6]

loc_1E2A2:
push    cx
push    di
mov     cx, [si+4]
shr     cx, 1
rep stosw
pop     di
add     di, 0A0h
pop     cx
loop    loc_1E2A2
pop     bp
pop     es
pop     di
pop     si
retn
winClear endp



; Attributes: bp-based frame

sub_1E2B8 proc near

arg_0= word ptr  4
arg_2= byte ptr  6

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
mov     si, [bp+arg_0]
mov     es, word ptr _picDecoded_0+2
mov     di, [si+2]
shl     di, 1
mov     di, [di-5F3Fh]
add     di, [si]
mov     ax, 0FFFFh
mov     cx, [si+6]

loc_1E2DB:
push    cx
push    di
mov     cx, [si+4]
shr     cx, 1

loc_1E2E2:
xor     es:[di], ax
inc     di
inc     di
loop    loc_1E2E2
pop     di
add     di, 160
pop     cx
loop    loc_1E2DB
cmp     [bp+arg_2], 0
jz      short loc_1E2FE
push    si
call    sub_1DA17
add     sp, 2

loc_1E2FE:
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
sub_1E2B8 endp




writeChar proc near
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
shl     ax, 1
shl     ax, 1
shl     ax, 1
mov     di, ax
add     di, [si+Window.field_14]
mov     di, _scanlinePtrs[di]
mov     ax, [si+Window._textX]
shl     ax, 1
shl     ax, 1
add     di, ax
mov     es, word ptr _picDecoded_0+2
pop     ax
xor     ah, ah
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
shl     ax, 1
mov     ds, word ptr _fontData+2
mov     si, ax
mov     cx, 8
or      bl, bl
jz      short loc_1E363
mov     bx, 0FFFFh

loc_1E352:
lodsw
xor     ax, bx
stosw
lodsw
xor     ax, bx
stosw
add     di, 9Ch
loop    loc_1E352
jmp     short loc_1E36D
db 90h

loc_1E363:
lodsw
stosw
lodsw
stosw
add     di, 9Ch
loop    loc_1E363

loc_1E36D:
pop     bp
pop     es
pop     ds
pop     di
pop     si
pop     cx
pop     bx
retn
writeChar endp

align 2


; Attributes: bp-based frame

sub_1E376 proc near

arg_0= byte ptr  4

push    bp
mov     bp, sp
push    si
push    di
push    ds
push    es
push    bp
mov     ax, ds
mov     es, ax
assume es:dseg
mov     di, 0A834h
xor     al, al
mov     bl, [bp+arg_0]
or      bl, bl
jz      short loc_1E3A7
jmp     short loc_1E396
db 90h

loc_1E391:
mov     cx, 100h
repne scasb

loc_1E396:
dec     bl
jnz     short loc_1E391
mov     ax, 0FFFFh
push    ax              ; y
push    ax              ; x
push    ds
push    di              ; text
call    writeStringAt
add     sp, 8

loc_1E3A7:
pop     bp
pop     es
assume es:nothing
pop     ds
pop     di
pop     si
pop     bp
retn
sub_1E376 endp



; Attributes: bp-based frame

; int __cdecl fileOpen(const char near *filename, char mode)
fileOpen proc near

filename= word ptr  4
mode= byte ptr  6

push    bp
mov     bp, sp
mov     ah, 3Dh ; '='
mov     al, [bp+mode]
mov     dx, [bp+filename]
int     21h             ; DOS - 2+ - OPEN DISK FILE WITH HANDLE
                        ; DS:DX -> ASCIZ filename
                        ; AL = access mode
                        ; 0 - read, 1 - write, 2 - read & write
jnb     short loc_1E3BF
xor     ax, ax

loc_1E3BF:
pop     bp
retn
fileOpen endp

push    bp
mov     bp, sp
mov     ah, 3Ch ; '<'
xor     ch, ch
mov     cl, [bp+6]
mov     dx, [bp+4]
int     21h             ; DOS - 2+ - CREATE A FILE WITH HANDLE (CREAT)
                        ; CX = attributes for file
                        ; DS:DX -> ASCIZ filename (may include drive and path)
jnb     short loc_1E3D4
xor     ax, ax

loc_1E3D4:
pop     bp
retn
push    bp
mov     bp, sp
mov     ah, 41h ; 'A'
mov     dx, [bp+4]
int     21h             ; DOS - 2+ - DELETE A FILE (UNLINK)
                        ; DS:DX -> ASCIZ pathname of file to delete (no wildcards allowed)
xor     ax, ax
jb      short loc_1E3E5
inc     ax

loc_1E3E5:
pop     bp
retn


; Attributes: bp-based frame

; void __cdecl fileClose(int handle)
fileClose proc near

handle= word ptr  4

push    bp
mov     bp, sp
mov     ah, 3Eh ; '>'
mov     bx, [bp+handle]
int     21h             ; DOS - 2+ - CLOSE A FILE WITH HANDLE
                        ; BX = file handle
xor     ax, ax
jb      short loc_1E3F6
inc     ax

loc_1E3F6:
pop     bp
retn
fileClose endp



; Attributes: bp-based frame

; int __cdecl fileRead(int handle, __int32 size, void far *dst)
fileRead proc near

handle= word ptr  4
size= dword ptr  6
dst= dword ptr  0Ah

push    bp
mov     bp, sp
push    ds
mov     bx, [bp+handle]
mov     ax, word ptr [bp+dst+2]
mov     ds, ax
mov     dx, word ptr [bp+dst]

loc_1E407:
mov     cx, word ptr [bp+size]
cmp     word ptr [bp+size+2], 0
jz      short loc_1E413
mov     cx, 0FFFFh

loc_1E413:
mov     ah, 3Fh
int     21h             ; DOS - 2+ - READ FROM FILE WITH HANDLE
                        ; BX = file handle, CX = number of bytes to read
                        ; DS:DX -> buffer
jb      short loc_1E443
cmp     cx, ax
jnz     short loc_1E443
clc
sub     word ptr [bp+size], cx
sbb     word ptr [bp+size+2], 0
clc
add     dx, cx
jnb     short loc_1E431
mov     ax, ds
add     ax, 1000h
mov     ds, ax
assume ds:nothing

loc_1E431:
cmp     word ptr [bp+size+2], 0
jnz     short loc_1E407
cmp     word ptr [bp+size], 0
jnz     short loc_1E407
mov     ax, 1
pop     ds
assume ds:dseg
pop     bp
retn

loc_1E443:
xor     ax, ax
pop     ds
pop     bp
retn
fileRead endp



; Attributes: bp-based frame

sub_1E448 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= word ptr  0Ah
arg_8= word ptr  0Ch

push    bp
mov     bp, sp
push    ds
mov     bx, [bp+arg_0]
mov     ax, [bp+arg_8]
mov     ds, ax
mov     dx, [bp+arg_6]

loc_1E457:
mov     cx, [bp+arg_2]
cmp     [bp+arg_4], 0
jz      short loc_1E463
mov     cx, 0FFF0h

loc_1E463:
mov     ah, 40h
int     21h             ; DOS - 2+ - WRITE TO FILE WITH HANDLE
                        ; BX = file handle, CX = number of bytes to write, DS:DX -> buffer
jb      short loc_1E493
cmp     cx, ax
jnz     short loc_1E493
clc
sub     [bp+arg_2], cx
sbb     [bp+arg_4], 0
clc
add     dx, cx
jnb     short loc_1E481
mov     ax, ds
add     ax, 1000h
mov     ds, ax
assume ds:nothing

loc_1E481:
cmp     [bp+arg_4], 0
jnz     short loc_1E457
cmp     [bp+arg_2], 0
jnz     short loc_1E457
mov     ax, 1
pop     ds
assume ds:dseg
pop     bp
retn

loc_1E493:
xor     ax, ax
pop     ds
pop     bp
retn
sub_1E448 endp



; Attributes: bp-based frame

sub_1E498 proc near

arg_0= word ptr  4
arg_2= word ptr  6
arg_4= word ptr  8
arg_6= byte ptr  0Ah

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
jnb     short loc_1E4B2
mov     ax, 0FFFFh
mov     dx, ax

loc_1E4B2:
pop     bp
retn
sub_1E498 endp



; Attributes: bp-based frame

; int __cdecl fileGetSize(int handle)
fileGetSize proc near

handle= word ptr  4

push    bp
mov     bp, sp
mov     bx, [bp+handle]
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
fileGetSize endp




sub_1E4D0 proc near
cld
mov     ax, seg dseg
mov     es, ax
assume es:dseg
push    ds
mov     ds, word_2BB18
xor     si, si
mov     bx, [si]
mov     dx, [si+2]
pop     ds
mov     word_2BB14, bx
mov     word_2BB16, dx
mov     di, 0A8DDh
mov     cx, 808h
mov     ax, 2020h
rep stosw
mov     si, 4
xor     di, di
mov     es, word_2BB1A
assume es:nothing
mov     cx, word_2BB14
mov     dx, word_2BB16
mov     word_2BB10, cx
mov     word_2BB12, dx
mov     bx, 0FEEh
mov     byte_2BB1C, 1

loc_1E517:
cmp     si, 40h ; '@'
jb      short loc_1E524
sub     si, 40h ; '@'
add     word_2BB18, 4

loc_1E524:
cmp     di, 40h ; '@'
jb      short loc_1E535
sub     di, 40h ; '@'
add     word_2BB1A, 4
mov     es, word_2BB1A

loc_1E535:
dec     byte_2BB1C
jnz     short loc_1E549
push    ds
mov     ds, word_2BB18
lodsb
mov     ah, al
pop     ds
mov     byte_2BB1C, 8

loc_1E549:
shr     ah, 1
jnb     short loc_1E56E
push    ds
mov     ds, word_2BB18
lodsb
stosb
pop     ds
sub     cx, 1
sbb     dx, 0
or      dx, dx
jnz     short loc_1E563
or      cx, cx
jz      short locret_1E5BC

loc_1E563:
mov     [bx-5723h], al
inc     bx
and     bx, 0FFFh
jmp     short loc_1E517

loc_1E56E:
mov     bp, cx
push    ds
mov     ds, word_2BB18
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

loc_1E58F:
mov     al, [si-5723h]
stosb
inc     si
and     si, 0FFFh
sub     bp, 1
sbb     dx, 0
or      dx, dx
jnz     short loc_1E5AB
or      bp, bp
jnz     short loc_1E5AB
pop     si
jmp     short locret_1E5BC
db 90h

loc_1E5AB:
mov     [bx-5723h], al
inc     bx
and     bx, 0FFFh
loop    loc_1E58F
mov     cx, bp
pop     si
jmp     loc_1E517

locret_1E5BC:
retn
sub_1E4D0 endp



; Attributes: bp-based frame

sub_1E5BD proc near

arg_2= word ptr  6
arg_6= word ptr  0Ah

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
mov     word_2BB18, ax
mov     ax, [bp+arg_6]
mov     word_2BB1A, ax
call    sub_1E4D0
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
sub_1E5BD endp

align 2



sub_1E5E2 proc near
push    dx
call    sub_1695C
add     sp, 2
retn
sub_1E5E2 endp




criticalFn proc far
sti
mov     al, 3
push    bx
push    cx
push    dx
push    si
push    di
push    es
push    ds
mov     dx, seg dseg
mov     ds, dx
and     di, 0FFh
mov     word_2CB55, di
shl     di, 1
mov     dx, [di-46E5h]
call    sub_1E5E2
cmp     dx, 0B9D4h
jz      short loc_1E616
cmp     dx, 0B9D9h
jnz     short loc_1E62B

loc_1E616:
call    restoreCriticalFn
call    sub_20720
pop     ds
pop     es
pop     di
pop     si
pop     dx
pop     cx
pop     bx
mov     al, 0
mov     ah, 4Ch
int     21h             ; DOS - 2+ - QUIT WITH EXIT CODE (EXIT)
                        ; AL = exit code

loc_1E62B:
mov     al, 1
mov     byte_2CB54, 0FFh
cmp     byte ptr _dosVersion, 2
jle     short loc_1E647
cmp     byte ptr _dosVersion, 3
jg      short loc_1E649
cmp     byte ptr _dosVersion+1, 2
jnz     short loc_1E649

loc_1E647:
xor     al, al

loc_1E649:
pop     ds
pop     es
pop     di
pop     si
pop     dx
pop     cx
pop     bx
iret
criticalFn endp




setCriticalFn proc near
mov     ax, 3524h
int     21h             ; DOS - 2+ - GET INTERRUPT VECTOR
                        ; AL = interrupt number
                        ; Return: ES:BX = value of interrupt vector
mov     word ptr _savedCricialFn, es
mov     word ptr _savedCricialFn+2, bx
push    ds
mov     dx, offset criticalFn
mov     ax, cs
mov     ds, ax
assume ds:sg01a2
mov     ax, 2524h
int     21h             ; DOS - SET INTERRUPT VECTOR
                        ; AL = interrupt number
                        ; DS:DX = new vector to be used for specified interrupt
pop     ds
assume ds:dseg
mov     ah, 30h
int     21h             ; DOS - GET DOS VERSION
                        ; Return: AL = major version number (00h for DOS 1.x)
mov     _dosVersion, ax
retn
setCriticalFn endp




restoreCriticalFn proc near
cli
mov     dx, word ptr _savedCricialFn+2
push    ds
mov     ax, word ptr _savedCricialFn
mov     ds, ax
mov     ax, 2524h
int     21h             ; DOS - SET INTERRUPT VECTOR
                        ; AL = interrupt number
                        ; DS:DX = new vector to be used for specified interrupt
pop     ds
sti
retn
restoreCriticalFn endp

align 2
assume ss:seg003, ds:nothing
; [000000B2 BYTES: COLLAPSED FUNCTION start. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000C4 BYTES: COLLAPSED FUNCTION __cinit. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000017 BYTES: COLLAPSED FUNCTION _exit. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000045 BYTES: COLLAPSED FUNCTION __exit. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000002D BYTES: COLLAPSED FUNCTION __ctermsub. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000000F BYTES: COLLAPSED FUNCTION sub_1E887. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000013 BYTES: COLLAPSED FUNCTION sub_1E896. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000020 BYTES: COLLAPSED FUNCTION __FF_MSGBANNER. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000006 BYTES: COLLAPSED FUNCTION __fptrap. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000016 BYTES: COLLAPSED FUNCTION __chkstk. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000022 BYTES: COLLAPSED FUNCTION __nullcheck. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000018E BYTES: COLLAPSED FUNCTION __setargv. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000006E BYTES: COLLAPSED FUNCTION __setenvp. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000002B BYTES: COLLAPSED FUNCTION __NMSG_TEXT. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000029 BYTES: COLLAPSED FUNCTION __NMSG_WRITE. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000042 BYTES: COLLAPSED FUNCTION __myalloc. PRESS CTRL-NUMPAD+ TO EXPAND]

__dosret0:
jb      short loc_1EBAF

loc_1EB9C:
xor     ax, ax
mov     sp, bp
pop     bp
retn

__dosreturn:
jnb     short loc_1EB9C
push    ax
call    sub_1EBC0
pop     ax
mov     sp, bp
pop     bp
retn
; [0000000D BYTES: COLLAPSED CHUNK OF FUNCTION _lseek. PRESS CTRL-NUMPAD+ TO EXPAND]

__maperror:
xor     ah, ah
call    sub_1EBC0
retn



sub_1EBC0 proc near
mov     byte_2CD3E, al
or      ah, ah
jnz     short loc_1EBEA
cmp     byte ptr word_2CD3B, 3
jb      short loc_1EBDB
cmp     al, 22h ; '"'
jnb     short loc_1EBDF
cmp     al, 20h ; ' '
jb      short loc_1EBDB
mov     al, 5
jmp     short loc_1EBE1
db 90h

loc_1EBDB:
cmp     al, 13h
jbe     short loc_1EBE1

loc_1EBDF:
mov     al, 13h

loc_1EBE1:
mov     bx, 0BB30h
xlat

loc_1EBE5:
cbw
mov     word_2CD33, ax
retn

loc_1EBEA:
mov     al, ah
jmp     short loc_1EBE5
sub_1EBC0 endp

; [00000032 BYTES: COLLAPSED FUNCTION _flushall. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000155 BYTES: COLLAPSED FUNCTION __flsbuf. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000006C BYTES: COLLAPSED FUNCTION __getbuf. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000006E BYTES: COLLAPSED FUNCTION _fflush. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000031D BYTES: COLLAPSED FUNCTION __output. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000135 BYTES: COLLAPSED FUNCTION iprint. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [000000E8 BYTES: COLLAPSED FUNCTION sprint. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000057 BYTES: COLLAPSED FUNCTION fprint. PRESS CTRL-NUMPAD+ TO EXPAND]
add     sp, 0Ah
cmp     byte ptr [bp-4], 0
jz      short loc_1F3FE
cmp     word_2D0CC, 0
jnz     short loc_1F3FE
push    word_2D0EA
call    off_2CECC
add     sp, 2

loc_1F3FE:
cmp     word_2D0CC, 0
jz      short loc_1F417
cmp     word_2D0E6, 0
jnz     short loc_1F417
push    word_2D0EA
call    off_2CED0
add     sp, 2

loc_1F417:
add     word_2D0DA, 8
mov     word_2D0EE, 0
mov     ax, word_2D0D6
or      ax, word_2D0DC
jz      short loc_1F43E
push    word ptr [bp-2]
call    off_2CED2
add     sp, 2
or      ax, ax
jz      short loc_1F43E
mov     ax, 1
jmp     short loc_1F440

loc_1F43E:
sub     ax, ax

loc_1F440:
push    ax
call    _out
mov     sp, bp
pop     bp
retn
; [0000003D BYTES: COLLAPSED FUNCTION _outc. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000005E BYTES: COLLAPSED FUNCTION putpad. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000068 BYTES: COLLAPSED FUNCTION putbuf. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000E2 BYTES: COLLAPSED FUNCTION _out. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000017 BYTES: COLLAPSED FUNCTION putsign. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000029 BYTES: COLLAPSED FUNCTION putprefix. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000007F BYTES: COLLAPSED FUNCTION getnum. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000027 BYTES: COLLAPSED FUNCTION flagchar. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000007A BYTES: COLLAPSED FUNCTION _lseek. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000A6 BYTES: COLLAPSED FUNCTION _write. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000004E BYTES: COLLAPSED FUNCTION sub_1F838. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000034 BYTES: COLLAPSED CHUNK OF FUNCTION _write. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000012 BYTES: COLLAPSED FUNCTION _stackavail. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000012 BYTES: COLLAPSED FUNCTION unknown_libname_1. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000046 BYTES: COLLAPSED FUNCTION unknown_libname_2. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000003 BYTES: COLLAPSED CHUNK OF FUNCTION __amalloc. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000E3 BYTES: COLLAPSED FUNCTION __amalloc. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000003A BYTES: COLLAPSED FUNCTION __amexpand. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000022 BYTES: COLLAPSED FUNCTION __amlink. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001F BYTES: COLLAPSED FUNCTION __amallocbrk. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000006E BYTES: COLLAPSED FUNCTION _brkctl. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000056 BYTES: COLLAPSED FUNCTION sub_1FAF4. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000003F BYTES: COLLAPSED FUNCTION _strcat. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000032 BYTES: COLLAPSED FUNCTION _strcpy. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001B BYTES: COLLAPSED FUNCTION _strlen. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [0000000A BYTES: COLLAPSED FUNCTION _ultoa. PRESS CTRL-NUMPAD+ TO EXPAND]


; Attributes: bp-based frame

sub_1FBE2 proc near

arg_0= word ptr  4

push    bp
mov     bp, sp
mov     bx, [bp+arg_0]
test    byte ptr [bx-4365h], 2
jz      short loc_1FBF6
mov     ax, bx
sub     ax, 20h ; ' '
jmp     short loc_1FBF8

loc_1FBF6:
mov     ax, bx

loc_1FBF8:
pop     bp
retn
sub_1FBE2 endp

; [00000023 BYTES: COLLAPSED FUNCTION _isatty. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000010 BYTES: COLLAPSED FUNCTION _kbhit. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000004 BYTES: COLLAPSED FUNCTION _getche. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000017 BYTES: COLLAPSED FUNCTION _getch. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000056 BYTES: COLLAPSED FUNCTION _sprintf. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000001E BYTES: COLLAPSED FUNCTION _movedata. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000042 BYTES: COLLAPSED FUNCTION _stricmp. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000005F BYTES: COLLAPSED CHUNK OF FUNCTION _ultoa. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000015 BYTES: COLLAPSED FUNCTION _abs. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000070 BYTES: COLLAPSED FUNCTION _qsort. PRESS CTRL-NUMPAD+ TO EXPAND]
; [000000B8 BYTES: COLLAPSED FUNCTION quicksort. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000038 BYTES: COLLAPSED FUNCTION swap. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000009C BYTES: COLLAPSED FUNCTION __aNldiv. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000034 BYTES: COLLAPSED FUNCTION __aNlmul. PRESS CTRL-NUMPAD+ TO EXPAND]
; [0000000B BYTES: COLLAPSED FUNCTION __aNlshl. PRESS CTRL-NUMPAD+ TO EXPAND]
align 2
; [00000022 BYTES: COLLAPSED FUNCTION unknown_libname_3. PRESS CTRL-NUMPAD+ TO EXPAND]
; [00000022 BYTES: COLLAPSED FUNCTION __aNNalmul. PRESS CTRL-NUMPAD+ TO EXPAND]
align 10h
sg01a2 ends


; Segment type: Pure code
sg11a2 segment byte public 'CODE' use16
assume cs:sg11a2
assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing


; Attributes: bp-based frame

sub_20000 proc far

var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  6
arg_2= word ptr  8
arg_4= word ptr  0Ah
arg_6= word ptr  0Ch
arg_8= byte ptr  0Eh

push    bp
mov     bp, sp
sub     sp, 0Ah
push    si
mov     [bp+var_6], 0
mov     ax, 18h
imul    [bp+arg_0]
mov     si, ax
mov     bx, _globals
mov     al, 3Ch ; '<'
mul     byte ptr [bx+si+3Fh]
add     ax, _weapons
inc     ax
inc     ax
mov     [bp+var_4], ax
mov     bx, ax
test    byte ptr [bx+18h], 2
jnz     short loc_20040
test    byte ptr [bx+18h], 1
jnz     short loc_20037
jmp     loc_200CF

loc_20037:
cmp     [bp+arg_4], 0
jg      short loc_20040
jmp     loc_200CF

loc_20040:
mov     ax, [bx+3Ah]
add     ax, _weapons
mov     [bp+var_A], ax
test    byte ptr [bx+18h], 1
jz      short loc_2005B
mov     ax, 12h
imul    [bp+arg_2]
add     ax, [bp+var_A]
jmp     short loc_2005E

loc_2005B:
mov     ax, [bp+var_A]

loc_2005E:
mov     [bp+var_8], ax
cmp     [bp+arg_8], 0
jz      short loc_20091
mov     [bp+var_2], 0
jmp     short loc_20071

loc_2006E:
inc     [bp+var_2]

loc_20071:
mov     bx, _globals
mov     ax, [bp+var_2]
cmp     [bx+2], ax
jle     short loc_200CF
mov     bx, ax
mov     si, [bp+var_8]
cmp     byte ptr [bx+si], 0
jz      short loc_2006E
cmp     [bp+var_6], ax
jge     short loc_2006E
mov     [bp+var_6], ax
jmp     short loc_2006E

loc_20091:
mov     [bp+var_2], 0
jmp     short loc_200AF

loc_20098:
mov     bx, [bp+var_6]
mov     si, [bp+var_8]
mov     al, [bx+si]
mov     bx, [bp+var_2]
cmp     [bx+si], al
jb      short loc_200AC
mov     ax, bx
mov     [bp+var_6], ax

loc_200AC:
inc     [bp+var_2]

loc_200AF:
mov     bx, _globals
mov     ax, [bp+var_2]
cmp     [bx+2], ax
jg      short loc_20098
mov     bx, [bp+var_6]
mov     si, [bp+var_8]
mov     al, [bx+si]
mov     bx, [bp+arg_6]
cmp     [bx+si], al
jnz     short loc_200CF
mov     ax, bx
mov     [bp+var_6], ax

loc_200CF:
mov     ax, [bp+var_6]
inc     ax
pop     si
mov     sp, bp
pop     bp
retf
sub_20000 endp



; Attributes: bp-based frame

sub_200D8 proc far

var_A= byte ptr -0Ah
var_8= byte ptr -8
var_6= byte ptr -6
var_4= word ptr -4
var_2= byte ptr -2
arg_0= byte ptr  6
arg_2= byte ptr  8
arg_4= byte ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 0Ah
cmp     [bp+arg_4], 0
jz      short loc_200FB
mov     al, byte ptr _disk1._mapPosX
mov     [bp+var_8], al
mov     al, byte ptr _disk1._mapPosY
mov     [bp+var_A], al
mov     al, [bp+arg_0]
mov     [bp+var_2], al
mov     al, [bp+arg_2]
jmp     short loc_20110

loc_200FB:
mov     al, [bp+arg_0]
mov     [bp+var_8], al
mov     al, [bp+arg_2]
mov     [bp+var_A], al
mov     al, byte ptr _disk1._mapPosX
mov     [bp+var_2], al
mov     al, byte ptr _disk1._mapPosY

loc_20110:
mov     [bp+var_6], al
mov     al, [bp+var_2]
cmp     [bp+var_8], al
jnz     short loc_2012F
mov     al, [bp+var_6]
cmp     [bp+var_A], al
jge     short loc_20127
sub     ax, ax
jmp     short loc_2012A

loc_20127:
mov     ax, 3

loc_2012A:
mov     [bp+var_4], ax
jmp     short loc_2017B

loc_2012F:
mov     al, [bp+var_2]
cmp     [bp+var_8], al
jle     short loc_2015A
mov     [bp+var_4], 6
mov     al, [bp+var_6]
cmp     [bp+var_A], al
jle     short loc_2014B
mov     [bp+var_4], 9
jmp     short loc_2017B

loc_2014B:
mov     al, [bp+var_6]
cmp     [bp+var_A], al
jge     short loc_2017B
mov     [bp+var_4], 0Ch
jmp     short loc_2017B

loc_2015A:
mov     [bp+var_4], 0Fh
mov     al, [bp+var_6]
cmp     [bp+var_A], al
jle     short loc_2016E
mov     [bp+var_4], 12h
jmp     short loc_2017B

loc_2016E:
mov     al, [bp+var_6]
cmp     [bp+var_A], al
jge     short loc_2017B
mov     [bp+var_4], 15h

loc_2017B:
mov     ax, [bp+var_4]
mov     sp, bp
pop     bp
retf
sub_200D8 endp



; Attributes: bp-based frame

sub_20182 proc far

var_4= word ptr -4
var_2= byte ptr -2
arg_0= word ptr  6
arg_2= word ptr  8

push    bp
mov     bp, sp
sub     sp, 4
push    si
mov     [bp+var_2], 0
mov     bx, [bp+arg_0]
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+arg_2]
jz      short loc_201BA
mov     [bp+var_4], 0
jmp     short loc_201A4

loc_201A1:
inc     [bp+var_4]

loc_201A4:
cmp     [bp+var_4], 3
jge     short loc_201BE
mov     si, [bp+var_4]
mov     bx, [bp+arg_0]
mov     al, [bx+si+4Bh]
sub     ah, ah
cmp     ax, [bp+arg_2]
jnz     short loc_201A1

loc_201BA:
mov     [bp+var_2], 1

loc_201BE:
mov     al, [bp+var_2]
sub     ah, ah
pop     si
mov     sp, bp
pop     bp
retf
sub_20182 endp



; Attributes: bp-based frame

sub_201C8 proc far

var_6= word ptr -6
var_4= dword ptr -4

push    bp
mov     bp, sp
sub     sp, 6
mov     [bp+var_6], 0
jmp     short loc_201FF

loc_201D5:
les     bx, [bp+var_4]
cmp     byte ptr es:[bx+5Dh], 3Ch ; '<'
jbe     short loc_201FC
cmp     byte ptr es:[bx+5Dh], 51h ; 'Q'
jnb     short loc_201ED
sub     byte ptr es:[bx+5Dh], 3Ch ; '<'
jmp     short loc_201FC

loc_201ED:
les     bx, [bp+var_4]
cmp     byte ptr es:[bx+5Dh], 79h ; 'y'
jnb     short loc_201FC
add     byte ptr es:[bx+5Dh], 3Ch ; '<'

loc_201FC:
inc     [bp+var_6]

loc_201FF:
les     bx, dword_22C6E
mov     al, es:[bx+9]
sub     ah, ah
cmp     ax, [bp+var_6]
jbe     short loc_2023A
mov     ax, 68h ; 'h'
imul    [bp+var_6]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
les     bx, [bp+var_4]
cmp     byte ptr es:[bx+5Dh], 0FFh
jz      short loc_20233
cmp     byte ptr es:[bx+5Dh], 0
jnz     short loc_201D5

loc_20233:
mov     byte ptr es:[bx+5Dh], 0
jmp     short loc_201FC

loc_2023A:
mov     sp, bp
pop     bp
retf
sub_201C8 endp



; Attributes: bp-based frame

sub_2023E proc far

var_12= word ptr -12h
var_C= dword ptr -0Ch
var_4= dword ptr -4
arg_0= word ptr  6

push    bp
mov     bp, sp
sub     sp, 12h
mov     bx, [bp+arg_0]
mov     al, [bx+3]
sub     ah, ah
or      ax, ax
jz      short loc_2025D
cmp     ax, 1
jz      short loc_20274
cmp     ax, 2
jz      short loc_202A4
jmp     loc_202EE

loc_2025D:
mov     bl, [bx+4]
sub     bh, bh
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_12], ax
mov     word_2DFFC, ax
mov     word_2DFFE, ds
jmp     short loc_202EE

loc_20274:
mov     bx, [bp+arg_0]
mov     al, 68h ; 'h'
mul     byte ptr [bx+4]
add     ax, word ptr dword_2E446
mov     dx, word ptr dword_2E446+2
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
mov     word_2DFFC, ax
mov     word_2DFFE, dx
les     bx, [bp+var_4]
mov     al, es:[bx+60h]
mov     byte_2E43F, al
mov     al, es:[bx+61h]

loc_2029F:
mov     byte_2E440, al
jmp     short loc_202EE

loc_202A4:
mov     bx, [bp+arg_0]
mov     al, 16h
mul     byte ptr [bx+5]
mov     cx, ax
mov     al, 66h ; 'f'
mul     byte ptr [bx+4]
add     ax, cx
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
add     ax, 0Eh
mov     word ptr [bp+var_C], ax
mov     word ptr [bp+var_C+2], dx
les     bx, [bp+var_C]
mov     al, es:[bx]
sub     ah, ah
mov     cl, 6
shl     ax, cl
add     ax, word ptr _enemies
mov     dx, word ptr _enemies+2
mov     word_2DFFC, ax
mov     word_2DFFE, dx
mov     al, es:[bx+10h]
mov     byte_2E43F, al
mov     al, es:[bx+11h]
jmp     short loc_2029F

loc_202EE:
mov     sp, bp
pop     bp
retf
sub_2023E endp



; Attributes: bp-based frame

sub_202F2 proc far

var_E= dword ptr -0Eh
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
arg_0= word ptr  6
arg_2= word ptr  8
arg_4= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 12h
push    di
push    si
mov     bx, [bp+arg_0]
mov     al, [bx]
sub     ah, ah
or      ax, ax
jz      short loc_2030F
cmp     ax, 1
jz      short loc_20314
cmp     ax, 2
jz      short loc_20358

loc_2030F:
sub     ax, ax
jmp     loc_203C3

loc_20314:
mov     bx, [bp+arg_0]
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
mov     si, ax
les     bx, dword_2E446
mov     al, es:[bx+si+60h]
cbw
mov     bx, [bp+arg_2]
mov     [bx], ax
mov     bx, [bp+arg_0]
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
mov     si, ax
les     bx, dword_2E446
mov     al, es:[bx+si+61h]
cbw
mov     bx, [bp+arg_4]
mov     [bx], ax
mov     bx, [bp+arg_0]
mov     al, 68h ; 'h'
mul     byte ptr [bx+1]
mov     si, ax
les     bx, dword_2E446
mov     ax, es:[bx+si+44h]
jmp     short loc_203C3

loc_20358:
mov     bx, [bp+arg_0]
mov     al, 66h ; 'f'
mul     byte ptr [bx+1]
add     ax, word ptr dword_2E41E
mov     dx, word ptr dword_2E41E+2
mov     [bp+var_A], ax
mov     [bp+var_8], dx
mov     al, 16h
mul     byte ptr [bx+2]
add     ax, [bp+var_A]
add     ax, 0Eh
mov     word ptr [bp+var_E], ax
mov     word ptr [bp+var_E+2], dx
les     bx, [bp+var_E]
mov     al, es:[bx+10h]
cbw
mov     bx, [bp+arg_2]
mov     [bx], ax
les     bx, [bp+var_E]
mov     al, es:[bx+11h]
cbw
mov     bx, [bp+arg_4]
mov     [bx], ax
mov     [bp+var_6], 0
jmp     short loc_203A3

loc_203A0:
inc     [bp+var_6]

loc_203A3:
les     bx, [bp+var_E]
mov     al, es:[bx+0Fh]
sub     ah, ah
cmp     ax, [bp+var_6]
ja      short loc_203B4
jmp     loc_2030F

loc_203B4:
mov     di, [bp+var_6]
shl     di, 1
mov     si, es:[bx+di+2]
or      si, si
jle     short loc_203A0
mov     ax, si

loc_203C3:
pop     si
pop     di
mov     sp, bp
pop     bp
retf
sub_202F2 endp



; Attributes: bp-based frame

sub_203C9 proc far

var_8= word ptr -8
var_6= word ptr -6
var_4= word ptr -4
var_2= word ptr -2

push    bp
mov     bp, sp
sub     sp, 8
push    si
mov     [bp+var_2], 1
mov     [bp+var_4], 0
jmp     short loc_203EA

loc_203DC:
mov     bx, [bp+var_4]
shl     bx, 1
mov     word ptr [bx-3036h], 0
inc     [bp+var_4]

loc_203EA:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_203DC
mov     [bp+var_2], 1
mov     [bp+var_4], 0
jmp     short loc_20448

loc_20400:
mov     bx, [bp+var_4]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_8], ax
mov     bx, ax
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_20445
mov     al, 6
mul     byte ptr [bx+4Ah]
mov     si, ax
add     si, bx
mov     ax, 1
push    ax
sub     ax, ax
push    ax
mov     al, [si+64h]
sub     ah, ah
push    ax
mov     al, [si+63h]
push    ax
mov     al, [si+62h]
push    ax
call    sub_20000
add     sp, 0Ah
mov     [bp+var_6], ax
cmp     ax, [bp+var_2]
jle     short loc_20445
mov     [bp+var_2], ax

loc_20445:
inc     [bp+var_4]

loc_20448:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_4], ax
jb      short loc_20400
mov     ax, [bp+var_2]
pop     si
mov     sp, bp
pop     bp
retf
sub_203C9 endp



; Attributes: bp-based frame

sub_2045A proc far

var_4= word ptr -4
var_2= word ptr -2
arg_0= word ptr  6

push    bp
mov     bp, sp
sub     sp, 4
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_4], ax
mov     bx, ax
mov     bl, [bx+49h]
sub     bh, bh
and     byte ptr [bx-3A60h], 0FEh
mov     bx, [bp+var_4]
mov     byte ptr [bx], 0
dec     _disk1._partyCount
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+arg_0], ax
jz      short loc_204BD
mov     ax, [bp+arg_0]
mov     [bp+var_2], ax
jmp     short loc_204B3

loc_20496:
mov     bx, [bp+var_2]
mov     di, bx
mov     al, [di-40E3h]
mov     [bx-40E4h], al
mov     si, [bp+var_2]
shl     si, 1
mov     ax, [si-2516h]
mov     [si-2518h], ax
inc     [bp+var_2]

loc_204B3:
mov     al, _disk1._partyCount
sub     ah, ah
cmp     [bp+var_2], ax
jb      short loc_20496

loc_204BD:
pop     si
pop     di
mov     sp, bp
pop     bp
retf
sub_2045A endp

align 2


; Attributes: bp-based frame

sub_204C4 proc far

var_C= word ptr -0Ch
var_A= word ptr -0Ah
var_8= word ptr -8
var_6= word ptr -6
var_4= dword ptr -4
arg_0= byte ptr  6

push    bp
mov     bp, sp
sub     sp, 0Ch
push    di
push    si
mov     [bp+var_8], 0
mov     [bp+var_A], 0
jmp     short loc_20507

loc_204D8:
mov     ax, 0Ch
imul    [bp+var_C]
mov     si, ax
add     si, word_21326
push    si
lea     di, [si+2]
lea     si, [si+0Eh]
push    ds
pop     es
assume es:dseg
mov     cx, 6
repne movsw
pop     si
inc     [bp+var_C]

loc_204F6:
mov     bx, word_21326
mov     ax, [bx]
sub     ax, [bp+var_A]
cmp     ax, [bp+var_C]
jg      short loc_204D8

loc_20504:
inc     [bp+var_8]

loc_20507:
mov     bx, word_21326
mov     ax, [bx]
cmp     [bp+var_8], ax
jl      short loc_20515
jmp     loc_205F3

loc_20515:
mov     ax, [bp+var_8]
sub     ax, [bp+var_A]
mov     [bp+var_6], ax
mov     ax, 0Ch
imul    [bp+var_6]
mov     si, ax
add     si, bx
mov     al, [bp+arg_0]
cmp     [si+2], al
jnz     short loc_20504
mov     al, [si+4]
cbw
les     bx, dword_22C6E
assume es:nothing
mul     word ptr es:[bx]
mov     cx, ax
mov     al, [si+3]
cbw
add     cx, ax
mov     ax, cx
mov     cl, 3
shl     ax, cl
add     ax, bx
mov     dx, es
add     ax, 414h
mov     word ptr [bp+var_4], ax
mov     word ptr [bp+var_4+2], dx
test    byte ptr [si+5], 80h
jz      short loc_2056B
les     bx, [bp+var_4]
mov     ax, es:[bx]
and     ax, 0F800h
or      ax, [si+6]
mov     es:[bx], ax

loc_2056B:
mov     ax, 0Ch
imul    [bp+var_6]
mov     si, ax
add     si, word_21326
test    byte ptr [si+5], 40h
jz      short loc_205BC
les     bx, [bp+var_4]
mov     al, [si+9]
mov     es:[bx+3], al
mov     ax, 0Ch
imul    [bp+var_6]
mov     di, ax
mov     bx, word_21326
mov     ax, [bx+di+0Ah]
les     bx, [bp+var_4]
mov     es:[bx+4], ax
mov     ax, 0Ch
imul    [bp+var_6]
mov     di, ax
mov     bx, word_21326
mov     al, [bx+di+8]
les     bx, [bp+var_4]
mov     cl, es:[bx+2]
and     cl, 80h
or      al, cl
mov     es:[bx+2], al

loc_205BC:
mov     ax, 0Ch
imul    [bp+var_6]
mov     si, ax
add     si, word_21326
test    byte ptr [si+5], 20h
jz      short loc_205D8
les     bx, [bp+var_4]
mov     ax, [si+0Ch]
mov     es:[bx+6], ax

loc_205D8:
inc     [bp+var_A]
mov     bx, word_21326
mov     ax, [bx]
dec     ax
cmp     ax, [bp+var_6]
jnz     short loc_205EA
jmp     loc_20504

loc_205EA:
mov     ax, [bp+var_6]
mov     [bp+var_C], ax
jmp     loc_204F6

loc_205F3:
cmp     [bp+var_A], 0
jle     short loc_20607
mov     ax, [bp+var_A]
sub     [bx], ax
les     bx, dword_22C6E
or      byte ptr es:[bx+5], 80h

loc_20607:
pop     si
pop     di
mov     sp, bp
pop     bp
retf
sub_204C4 endp



; Attributes: bp-based frame

sub_2060D proc far

var_6= word ptr -6
var_4= word ptr -4
arg_0= word ptr  6
arg_2= word ptr  8
arg_4= word ptr  0Ah

push    bp
mov     bp, sp
sub     sp, 6
push    di
push    si
mov     bx, [bp+arg_0]
shl     bx, 1
mov     ax, [bx-2518h]
mov     [bp+var_6], ax
mov     bx, ax
cmp     byte ptr [bx+4Ah], 0FFh
jz      short loc_20650
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+arg_2]
jnz     short loc_20639
mov     byte ptr [bx+4Ah], 0FFh
jmp     short loc_20650

loc_20639:
mov     bx, [bp+var_6]
mov     al, [bx+4Ah]
sub     ah, ah
cmp     ax, [bp+arg_2]
jbe     short loc_20650
cmp     [bp+arg_4], 0FFh
jnz     short loc_20650
dec     byte ptr [bx+4Ah]

loc_20650:
mov     [bp+var_4], 0
jmp     short loc_20675

loc_20657:
mov     si, [bp+var_4]
add     si, [bp+var_6]
add     si, 4Bh ; 'K'
mov     al, [si]
sub     ah, ah
cmp     [bp+arg_2], ax
jnb     short loc_20672
cmp     [bp+arg_4], 0FFh
jnz     short loc_20672
dec     byte ptr [si]

loc_20672:
inc     [bp+var_4]

loc_20675:
cmp     [bp+var_4], 3
jge     short loc_20697
mov     si, [bp+var_4]
add     si, [bp+var_6]
add     si, 4Bh ; 'K'
cmp     byte ptr [si], 0FFh
jz      short loc_20672
mov     al, [si]
sub     ah, ah
cmp     [bp+arg_2], ax
jnz     short loc_20657
mov     byte ptr [si], 0FFh
jmp     short loc_20672

loc_20697:
cmp     [bp+arg_4], 0FFh
jnz     short loc_206E3
mov     bx, [bp+var_6]
dec     byte ptr [bx+52h]
mov     bx, [bp+var_6]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+arg_2]
jz      short loc_20712
mov     ax, [bp+arg_2]
mov     [bp+var_4], ax
jmp     short loc_206D4

loc_206B9:
mov     ax, 6
imul    [bp+var_4]
mov     si, ax
add     si, [bp+var_6]
push    si
lea     di, [si+62h]
lea     si, [si+68h]
push    ds
pop     es
assume es:dseg
movsw
movsw
movsw
pop     si
inc     [bp+var_4]

loc_206D4:
mov     bx, [bp+var_6]
mov     al, [bx+52h]
sub     ah, ah
cmp     ax, [bp+var_4]
ja      short loc_206B9
jmp     short loc_20712

loc_206E3:
mov     ax, 6
imul    [bp+arg_2]
mov     si, ax
mov     bx, [bp+var_6]
mov     al, byte ptr [bp+arg_4]
mov     [bx+si+62h], al
mov     ax, 6
imul    [bp+arg_2]
mov     si, ax
mov     bx, [bp+var_6]
mov     byte ptr [bx+si+64h], 0
mov     ax, 6
imul    [bp+arg_2]
mov     si, ax
mov     bx, [bp+var_6]
mov     byte ptr [bx+si+65h], 0

loc_20712:
pop     si
pop     di
mov     sp, bp
pop     bp
retf
sub_2060D endp

align 10h



sub_20720 proc far
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
assume es:nothing
pop     ds
pop     dx
pop     cx
pop     bx
pop     ax
retf
sub_20720 endp




sub_20734 proc far
push    ax
push    bx
push    cx
push    dx
push    si
push    di
push    ds
push    es
mov     word_2BA10, 0A251h
mov     word_2BA0E, 0
mov     ax, seg dseg
mov     es, ax
assume es:dseg
mov     di, 0A0C1h
xor     ax, ax
mov     cx, 0C8h

loc_20755:
stosw
add     ax, 0A0h
loop    loc_20755
xor     bx, bx
mov     bl, byte_2B166
shl     bx, 1
mov     bx, [bx-60E2h]
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
sub_20734 endp




sub_20772 proc far
mov     word_2BA10, 0A251h
mov     word_2BA0E, 0
retf
sub_20772 endp




sub_2077F proc far
mov     bx, word_2BA10
add     word_2BA10, 0Eh
inc     word_2BA0E
xor     ah, ah
test    ax, 1
jz      short loc_20794
inc     di

loc_20794:
test    di, 1
jz      short loc_2079B
inc     di

loc_2079B:
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
mov     di, 9F31h
add     di, si
mov     [bx], di
mov     di, 0A0C1h
add     di, si
mov     si, [di]
shl     ax, 1
add     si, ax
mov     [bx+4], si
mov     di, [bx+0Ah]
shl     di, 1
mov     ax, [di-6184h]
mov     [bx+6], ax
retf
sub_2077F endp

push    es
mov     word ptr cs:loc_208E7+1, 12h
mov     ax, seg dseg
mov     es, ax
assume es:dseg
mov     di, 9F31h
xor     ax, ax
mov     cx, 32h ; '2'

loc_207EC:
stosw
add     ax, 2000h
stosw
add     ax, 2000h
stosw
add     ax, 2000h
stosw
sub     ax, 5F60h
loop    loc_207EC
pop     es
assume es:nothing
retn
mov     word ptr cs:loc_208E7+1, 67h ; 'g'
mov     cx, 0C8h
mov     di, 9F31h
mov     ax, seg dseg
mov     es, ax
assume es:dseg
mov     ax, 4000h

loc_20815:
stosw
add     ax, 50h ; 'P'
loop    loc_20815
mov     di, 9E7Ch
mov     cx, 51h ; 'Q'
mov     ax, 0EACh

loc_20824:
stosw
sub     ax, 0Fh
loop    loc_20824
mov     cx, 0Fh
mov     ax, cs
mov     ds, ax
assume ds:sg11a2
mov     es, ax
assume es:sg11a2
mov     si, 847h
mov     di, 9FCh
rep movsb
mov     cx, 4A1h
mov     si, 9FCh
rep movsb
mov     al, 0C3h
stosb
retn
lodsw
mov     bx, ax
shr     bx, 1
shr     bx, 1
shr     bx, 1
shr     bx, 1
mov     ah, es:[bx]
stosb
mov     word ptr cs:loc_208E7+1, 0B9h
mov     ax, seg dseg
mov     es, ax
assume es:dseg
mov     si, 0A7D4h
mov     cx, 10h
xor     bx, bx

loc_2086A:
mov     di, 0A804h
lodsb
mov     dh, al
lodsw
push    cx
mov     cx, 10h

loc_20875:
mov     es:[di], dh
inc     di
stosw
loop    loc_20875
mov     ax, 1012h
mov     cx, 10h
mov     dx, 0A804h
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
add     bx, 10h
pop     cx
loop    loc_2086A
mov     di, 9F31h
mov     cx, 0C8h
xor     ax, ax

loc_2089B:
stosw
add     ax, 140h
loop    loc_2089B
mov     di, 9E7Ch
mov     cx, 51h ; 'Q'
mov     ax, 108Ch

loc_208AA:
stosw
sub     ax, 15h
loop    loc_208AA
mov     ax, cs
mov     ds, ax
mov     es, ax
assume es:sg11a2
mov     si, 8CDh
mov     di, 9FCh
mov     cx, 15h
rep movsb
mov     cx, 67Bh
mov     si, 9FCh
rep movsb
mov     al, 0C3h
stosb
retn
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



sub_208E2 proc far
push    si
push    di
push    es
push    ds
push    bp

loc_208E7:
call    sub_208FC
pop     bp
pop     ds
assume ds:dseg
pop     es
assume es:nothing
pop     di
pop     si
mov     word_2BA10, 0A251h
mov     word_2BA0E, 0
retf
sub_208E2 endp




sub_208FC proc near
mov     word_2BA12, 0A251h
mov     ax, 0B800h
mov     es, ax
assume es:nothing

loc_20907:
cmp     word_2BA0E, 0
jnz     short loc_2090F
retn

loc_2090F:
dec     word_2BA0E
mov     bx, word_2BA12
mov     ax, [bx+8]
mov     byte_2BA0D, al
mov     ax, [bx+2]
shl     ax, 1
mov     si, [bx+4]
mov     dx, [bx+0Ch]
mov     bp, [bx+0Ah]
mov     bx, [bx]

loc_2092D:
mov     di, [bx]
add     bx, 2
add     di, ax
mov     cx, bp
mov     ds, word ptr _picDecoded_0+2
rep movsw
mov     cx, seg dseg
mov     ds, cx
add     si, dx
dec     byte_2BA0D
jnz     short loc_2092D
add     word_2BA12, 0Eh
jmp     short loc_20907
sub_208FC endp

retn
mov     word_2BA12, 0A251h
mov     ax, 0A000h
mov     es, ax
assume es:nothing

loc_2095C:
cmp     word_2BA0E, 0
jnz     short loc_20964
retn

loc_20964:
dec     word_2BA0E
mov     bx, word_2BA12
mov     si, [bx+4]
mov     dx, [bx+0Ch]
mov     cx, [bx+8]
mov     ax, [bx+6]
mov     word_2BA09, ax
mov     ax, [bx+2]
mov     bx, [bx]
mov     bp, [bx]
add     bp, ax

loc_20984:
mov     di, bp
add     bp, 50h ; 'P'
mov     ax, word_2BA09
mov     ds, word ptr _picDecoded_0+2
call    ax ; word_2BA09
mov     ax, seg dseg
mov     ds, ax
add     si, dx
loop    loc_20984
add     word_2BA12, 0Eh
jmp     short loc_2095C
retn
mov     word_2BA12, 0A251h
mov     ax, 0A000h
mov     es, ax

loc_209AE:
cmp     word_2BA0E, 0
jnz     short loc_209B6
retn

loc_209B6:
dec     word_2BA0E
mov     bx, word_2BA12
mov     si, [bx+4]
mov     dx, [bx+0Ch]
mov     cx, [bx+8]
mov     ax, [bx+6]
mov     word_2BA09, ax
mov     ax, [bx+2]
shl     ax, 1
shl     ax, 1
mov     bx, [bx]
mov     bp, [bx]
add     bp, ax

loc_209DA:
mov     di, bp
add     bp, 140h
mov     ax, word_2BA09
mov     ds, word ptr _picDecoded_0+2
call    ax ; word_2BA09
mov     ax, seg dseg
mov     ds, ax
add     si, dx
dec     cl
jnz     short loc_209DA
add     word_2BA12, 0Eh
jmp     short loc_209AE
retn
db 691h dup(0), 0C3h, 2 dup(0)


; Attributes: bp-based frame

sub_21090 proc far

arg_0= word ptr  6
arg_2= word ptr  8
arg_4= word ptr  0Ah
arg_6= word ptr  0Ch

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
mov     es, word ptr _picDecoded_0+2
assume es:nothing
mov     ax, [bp+arg_6]
shl     ax, 1
mov     si, ax
mov     di, [si-5F3Fh]
mov     ax, [bp+arg_4]
shr     ax, 1
add     di, ax
mov     ds, [bp+arg_2]
mov     si, [bp+arg_0]
xor     ax, ax
mov     dx, ax
mov     al, [si]
mov     dl, [si+1]
mov     bx, 0A0h
sub     bx, ax
shr     ax, 1
add     si, 4

loc_210CA:
mov     cx, ax
rep movsw
add     di, bx
dec     dx
jnz     short loc_210CA
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
sub_21090 endp



; Attributes: bp-based frame

sub_210DD proc far

arg_0= word ptr  6
arg_2= word ptr  8
arg_4= word ptr  0Ah
arg_6= word ptr  0Ch
arg_8= word ptr  0Eh

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
call    sub_21090
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
sub_210DD endp

align 10h


; Attributes: bp-based frame

sub_21130 proc far

arg_0= word ptr  6

push    bp
mov     bp, sp
push    bx
push    cx
push    dx
push    si
push    di
push    es
push    ds
push    bp
mov     ax, seg dseg
mov     ds, ax
cmp     byte_2CC1E, 0
jz      short loc_21194
mov     si, [bp+arg_0]
cmp     si, 6
jg      short loc_21194
shl     si, 1
mov     ax, [si-458Ch]
cli
mov     word_2CC2B, ax
mov     word_2CC2D, ax
mov     al, 0
mov     byte_2CC20, al
mov     byte_2CC22, al
in      al, 61h         ; PC/XT PPI port B bits:
                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                        ; 3: 1=read high switches
                        ; 4: 0=enable RAM parity checking
                        ; 5: 0=enable I/O channel check
                        ; 6: 0=hold keyboard clock low
                        ; 7: 0=enable kbrd
or      al, 3
out     61h, al         ; PC/XT PPI port B bits:
                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                        ; 3: 1=read high switches
                        ; 4: 0=enable RAM parity checking
                        ; 5: 0=enable I/O channel check
                        ; 6: 0=hold keyboard clock low
                        ; 7: 0=enable kbrd
mov     al, 0B6h
out     43h, al         ; Timer 8253-5 (AT: 8254.2).
mov     ax, 2
out     42h, al         ; Timer 8253-5 (AT: 8254.2).
mov     al, ah
out     42h, al         ; Timer 8253-5 (AT: 8254.2).
mov     byte_2CC1F, 1
sti

loc_2117D:
mov     dx, 3DAh

loc_21180:              ; Video status bits:
in      al, dx          ; 0: retrace.  1=display is in vert or horiz retrace.
                        ; 1: 1=light pen is triggered; 0=armed
                        ; 2: 1=light pen switch is open; 0=closed
                        ; 3: 1=vertical sync pulse is occurring.
and     al, 8
jnz     short loc_21180

loc_21185:              ; Video status bits:
in      al, dx          ; 0: retrace.  1=display is in vert or horiz retrace.
                        ; 1: 1=light pen is triggered; 0=armed
                        ; 2: 1=light pen switch is open; 0=closed
                        ; 3: 1=vertical sync pulse is occurring.
and     al, 8
jz      short loc_21185
call    sub_2119E
cmp     byte_2CC1F, 0
jnz     short loc_2117D

loc_21194:
pop     bp
pop     ds
pop     es
pop     di
pop     si
pop     dx
pop     cx
pop     bx
pop     bp
retf
sub_21130 endp




sub_2119E proc near
cmp     byte_2CC1F, 1
jz      short loc_211A8
jmp     locret_21233

loc_211A8:
cmp     byte_2CC20, 0
jz      short loc_211B2
jmp     short loc_21215
align 2

loc_211B2:
cmp     byte_2CC22, 0
jz      short loc_211BC
jmp     short loc_21202
align 2

loc_211BC:
mov     si, word_2CC2D

loc_211C0:
xor     ah, ah
lodsb
cmp     al, 84h
jz      short loc_211D1
cmp     al, 83h
jz      short loc_211DF
cmp     al, 81h
jz      short loc_211FC
jmp     short loc_211C0

loc_211D1:              ; PC/XT PPI port B bits:
in      al, 61h         ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                        ; 3: 1=read high switches
                        ; 4: 0=enable RAM parity checking
                        ; 5: 0=enable I/O channel check
                        ; 6: 0=hold keyboard clock low
                        ; 7: 0=enable kbrd
and     al, 0FCh
out     61h, al         ; PC/XT PPI port B bits:
                        ; 0: Tmr 2 gate ═╦═► OR 03H=spkr ON
                        ; 1: Tmr 2 data ═╝  AND 0fcH=spkr OFF
                        ; 3: 1=read high switches
                        ; 4: 0=enable RAM parity checking
                        ; 5: 0=enable I/O channel check
                        ; 6: 0=hold keyboard clock low
                        ; 7: 0=enable kbrd
mov     byte_2CC1F, 0
jmp     short locret_21233
db 90h

loc_211DF:
lodsb
mov     byte_2CC20, al
mov     byte_2CC21, al
lodsw
mov     word_2CC23, ax
mov     word_2CC25, ax
lodsw
mov     word_2CC27, ax
lodsw
mov     word_2CC29, ax
mov     word_2CC2D, si
jmp     short loc_21228
align 2

loc_211FC:
lodsb
mov     byte_2CC22, al
jmp     short loc_211C0

loc_21202:
dec     byte_2CC22
mov     ax, word_2CC23
mov     word_2CC25, ax
mov     al, byte_2CC21
mov     byte_2CC20, al
jmp     short loc_21228
db 90h

loc_21215:
mov     ax, word_2CC27
add     word_2CC25, ax
mov     bx, word_2CC29
add     word_2CC27, bx
dec     byte_2CC20

loc_21228:
xor     ah, ah
mov     ax, word_2CC25
out     42h, al         ; Timer 8253-5 (AT: 8254.2).
mov     al, ah
out     42h, al         ; Timer 8253-5 (AT: 8254.2).

locret_21233:
retn
sub_2119E endp

align 10h
sg11a2 ends

; 12c6h

; Segment type: Pure data
dseg segment para public 'DATA' use16
assume cs:dseg
db    0
db    0
word_21242 dw 0
align 8
aMsRunTimeLibra db 'MS Run-Time Library - Copyright (c)'
db ' 1988, Microsoft Corp'
db  11h
db    0
aDisk1 db 'Disk 1',0
aDisk1_0 db 'Disk1',0
aKmap_0 db 'KMAP',0
aKscr_0 db 'KSCR',0
aKani_0 db 'KANI',0
aDisk2 db 'Disk 2',0
aDisk2_0 db 'Disk2',0
aKmap db 'KMAP',0
aKscr db 'KSCR',0
aKani db 'KANI',0
aDisk3 db 'Disk 3',0
aDisk3_0 db 'Disk3',0
aFmap db 'FMAP',0
aFscr db 'FSCR',0
aFani db 'FANI',0
aDisk4 db 'Disk 4',0
aDisk4_0 db 'Disk4',0
aDmap db 'DMAP',0
aDscr db 'DSCR',0
aDani db 'DANI',0
aSun db 'Sun',0
aMon db 'Mon',0
aTue db 'Tue',0
aWed db 'Wed',0
aThu db 'Thu',0
aFri db 'Fri',0
aSat db 'Sat',0
aOk db 'OK ',0
aUnc db 'UNC',0
aSer db 'SER',0
aCrt db 'CRT',0
aCom db 'COM',0
aDed db 'DED',0
word_21326 dw 0C6A0h
byte_21328 db 0
byte_21329 db 0
byte_2132A db 0
align 2
word_2132C dw 0
word_2132E dw 0
word_21330 dw 0
off_21332 dw offset aDisk1 ; "Disk 1"
dw offset aDisk1_0      ; "Disk1"
dw offset aKmap_0       ; "KMAP"
dw offset aKscr_0       ; "KSCR"
dw offset aKani_0       ; "KANI"
; char *off_2133C
off_2133C dw offset aDisk2 ; "Disk 2"
; char *off_2133E
off_2133E dw offset aDisk2_0 ; "Disk2"
dw offset aKmap         ; "KMAP"
dw offset aKscr         ; "KSCR"
dw offset aKani         ; "KANI"
; char *off_21346
off_21346 dw offset aDisk3 ; "Disk 3"
dw offset aDisk3_0      ; "Disk3"
dw offset aFmap         ; "FMAP"
dw offset aFscr         ; "FSCR"
dw offset aFani         ; "FANI"
; char *off_21350
off_21350 dw offset aDisk4 ; "Disk 4"
; char *off_21352
off_21352 dw offset aDisk4_0 ; "Disk4"
dw offset aDmap         ; "DMAP"
dw offset aDscr         ; "DSCR"
dw offset aDani         ; "DANI"
db    0
db    0
db    0
db    0
db    1
db    0
db    3
db    0
db    1
db    0
db    4
db    0
db    2
db    2
db    2
db    2
db    3
db    3
db    3
db    3
db    3
db    3
db    3
db    2
db    3
db    2
db    3
db    3
db    3
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
db    2
WEEKDAYS dw offset aSun ; "Sun"
dw offset aMon          ; "Mon"
dw offset aTue          ; "Tue"
dw offset aWed          ; "Wed"
dw offset aThu          ; "Thu"
dw offset aFri          ; "Fri"
dw offset aSat          ; "Sat"
db    0
db    0
db 0FFh
db 0FFh
db    0
db    0
db    1
db    0
db 0FFh
db 0FFh
db    0
db    0
db    1
db    0
db    0
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    1
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    1
db    0
db    1
db    0
db    1
db    0
db    0
db    0
db 0FFh
db 0FFh
db    1
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    0
db    0
db    1
db    0
db 0FFh
db 0FFh
db    1
db    0
db    1
db    0
db    1
db    0
db    1
db    0
db    0
db    0
db    1
db    0
db    1
db    0
db    1
db    0
db 0FFh
db 0FFh
db    1
db    0
db    1
db    0
db    0
db    0
db    1
db    0
db    1
db    0
db    0
db    0
db    1
db    0
db 0FFh
db 0FFh
db    1
db    0
db    0
db    0
db    0
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    0
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    1
db    0
db 0FFh
db 0FFh
db    1
db    0
db    0
db    0
db    1
db    0
db 0FFh
db 0FFh
db    0
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    0
db    0
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db    0
db    0
off_21410 dw offset aOk ; "OK "
dw offset aUnc          ; "UNC"
dw offset aSer          ; "SER"
dw offset aCrt          ; "CRT"
dw offset aCom          ; "COM"
dw offset aDed          ; "DED"
db    0
db    0
db    0
db 0FFh
db    0
db    1
db    1
db    0
db 0FFh
db    0
db    0
db    0
aDisk1_1 db 'DISK1',0
aDisk1_2 db 'Disk 1',0
aDisk1_3 db 'Disk1',0
aDisk1_4 db 'Disk 1',0
aDisk2_1 db 'Disk 2',0
aDisk2_2 db 'Disk 2',0
aDisk1_5 db 'Disk1',0
aDisk3_1 db 'Disk3',0
aTiles db 'TILES',0
; char filename[]
filename db 'FONT',0
aBorders db 'BORDERS',0
aPackets db 'PACKETS',0
aArchtype db 'ARCHTYPE',0
aServices db 'SERVICES',0
aHdspct db 'HDSPCT',0
aWeapons db 'WEAPONS',0
aGlobals db 'GLOBALS',0
aHands db 'Hands',0
aF1d db 'F%1d>',0
a1212s21d31d db '%-12.12s %2.1d %3.1d',0
a31d db '%3.1d',0
a31d_0 db '%3.1d',0
a77s db '%-7.7s',0
; char text[]
text db 'Which way?',0
; char member[]
member db '^',0
; char aSpace[]
aSpace db '< space >',0
; char aV[]
aV db 'v',0
a22d2d db '%2.2d:%.2d',0
aUhOhHereComesT db 0Dh,0Dh,'Uh oh! Here comes trouble!',0
aF1d1717s db 'F%1d> %-17.17s',0
a1dC db '%1d>%c',0
a1d3d1717fs db '%1d> %3d %-17.17Fs',0
a1d3d1717fs_0 db '%1d> %3d %-17.17Fs',0
a1d3d1717fs_1 db '%1d> %3d %-17.17Fs',0
a1d1717s db '%1d> %-17.17s',0
; char aThatHasNoEffec[]
aThatHasNoEffec db 0Dh,'That has no effect.',0Dh,0
aSFs db 0Dh,'%s %Fs...',0
; char aYouDecideThatI[]
aYouDecideThatI db 0Dh,'You decide that it wouldn',27h,'t'
db ' work.',0Dh,0
; char aAndMisses[]
aAndMisses db 0Dh,'and misses!',0Dh,0
aIsIncapacitate db ' is',0Dh,'incapacitated.',0
aLoadWhichAmmo db 'Load Which Ammo?',0
aSHasTooMuchToC db '%s',0Dh,'has too much to carry alre'
db 'ady.',0
aThisWeaponIsJa db 'This weapon is jammed.',0Dh,0Dh,'Do'
db ' you wish to fix it?',0
aDoWhatWithYour db 'Do what with your',0Dh,'%s:',0Dh,0Dh
db 0
; char aUSe[]
aUSe db 'U)se',0Dh,0
; char aINgest[]
aINgest db 'I)ngest',0Dh,0
; char aDRop[]
aDRop db 'D)rop',0Dh,0
; char aTRade[]
aTRade db 'T)rade',0Dh,0
; char aEQuipUnequipWe[]
aEQuipUnequipWe db 'E)quip-unequip weapon',0Dh,0
; char aEQuipUnequipAr[]
aEQuipUnequipAr db 'E)quip-unequip armor',0Dh,0
; char aLOadUnjam[]
aLOadUnjam db 'L)oad/unjam',0
aThisWeaponIsEm db 'This weapon is empty.',0
aWhoWillTakeYou db 'Who will take your',0Dh,'%-1.19s?',0
aYouHaveNoAmmoF db 'You have no ammo for',0Dh,'this Wea'
db 'pon.',0
; char aYourChoice1Qui[]
aYourChoice1Qui db 'Your choice?',0Dh,0Dh,0Dh,'1. Quit '
db '(without saving)',0Dh,'3. Quit (sav'
db 'e game)',0Dh,'5. Just save!',0Dh,0Dh
db 'N)evermind',0
db    0
aIt db 'it',0
aHim db 'him',0
aHer db 'her',0
aIts db 'its',0
aHis db 'his',0
aHer_0 db 'her',0
db    0
db    0
db    0
db    2
db    0
db    1
db    0
db    4
db    0
db    3
db    0
db    0
db    2
db    1
db    4
db    3
db    0
dw offset aIt           ; "it"
dw offset aHim          ; "him"
dw offset aHer          ; "her"
dw offset aIts          ; "its"
dw offset aHis          ; "his"
dw offset aHer_0        ; "her"
db  19h
aDabzD db 'dABZ!d',0
aFs db '%Fs',0
aFloridaBidsYou db 'Florida bids you a fond farewell!',0
aTheBlast db 'The blast',0
aAlsoPounds db 'also pounds',0
a1d db '%1d> ',0
aHands_0 db 'Hands',0
aFsFsFs db '%Fs %Fs %Fs',0
aTriggersABurst db 'triggers a burst at',0
aRipsOffAClipAt db 'rips off a clip at',0
aForDPtsDamage db ' for %d pts damage.',0Dh,0Dh,0
; char aAndMisses_0[]
aAndMisses_0 db ' and misses.',0Dh,0Dh,0
aButFailsToInfl db ' but fails to inflict any damage.',0Dh
db 0Dh,0
aAndKillsS db ' and kills %s!',0Dh,0Dh,0
aSHasBeenExpose db '%s has been exposed to mutagens!',0Dh
db 0
aSHasBecomeFs db '%s has become %Fs!',0Dh,0
aAndWastesS db ' and wastes %s!',0Dh,0Dh,0
aFsSWeaponIsUse db '%Fs ',27h,'s weapon is useless agai'
db 'nst %Fs.',0Dh,0Dh,0
aSSWeaponHasJam db '%s',27h,'s weapon has jammed!',0Dh,0Dh
db 0
; char aButItFailsToDe[]
aButItFailsToDe db ' but it fails to detonate.',0Dh,0Dh
db 0
aSSWeaponHasBro db '%s',27h,'s weapon has broken!',0Dh,0Dh
db 0
aAlsoFiresAt db 'also fires at',0
aFsManagesToHol db '%Fs manages to hold back.',0Dh,0Dh,0
aFs_0 db '%Fs',0
a1d1d1414s3d db '%1d> %1d %-14.14s %3d',27h,0
aThatTargetIsOu db 'That target is out of your range.',0
aFsMoves db '%Fs moves ',0
aToABetterPosti db 'to a better postion.',0Dh,0Dh,0
aCloser db 'closer.',0Dh,0Dh,0
db '            ',0
; char aChooseAFiringM[]
aChooseAFiringM db 'Choose a firing mode:',0
aSIngleFire db 'S)ingle fire.',0
aBUrst db 'B)urst.',0
aFUllAuto db 'F)ull auto.',0
aSChoose db '%s, choose:',0
; char aRun[]
aRun db 'Run',0
; char aEvade[]
aEvade db 'Evade',0
; char aAttack[]
aAttack db 'Attack',0
; char aLoadUnjam[]
aLoadUnjam db 'Load/unjam',0
; char aWeapon[]
aWeapon db 'Weapon',0
; char aUse[]
aUse db 'Use',0
; char aChooseAMethod[]
aChooseAMethod db 'Choose a method:',0
; char aGEtCloserFLee[]
aGEtCloserFLee db 'G)et closer    F)lee',0
aJammed db 'jammed',0
aEmpty db 'empty',0
aYourWeaponIsS db 'Your weapon is %s.',0
aChooseATarget db 'Choose a target..',0
aYouHaveNoAmmoF_0 db 'You have no ammo for this weapon.',0
aSS db '%s',27h,'s',0
; char aAreNotReloadab[]
aAreNotReloadab db 'are not reloadable.',0
aYourWeaponIsNo db 'Your weapon is not reloadable.',0
aYouHaveNoOther db 'You have no other weapons.',0
aWeapons_0 db 'Weapons',0
aChooseATarget_0 db 'Choose a target..',0
; char aUseTheseComman[]
aUseTheseComman db 'Use these commands?',0
; char aEncounterBegin[]
aEncounterBegin db 'Encounter begins...',0Dh,0Dh,0
aYourPartyMoves db 'Your party moves closer.',0Dh,0Dh,0
aYouMoveCloser db 'You move closer.',0Dh,0Dh,0
aSucceed db 'succeed',0
aFail db 'fail',0
aYouSInYourAtte db 'You %s in your attempt to run away.'
db 0Dh,0Dh,0
aEvades db ' evades.',0Dh,0Dh,0
aFsReloads db '%Fs reloads.',0Dh,0Dh,0
aFsReloads_0 db '%Fs reloads.',0Dh,0Dh,0
aReloads db ' reloads.',0Dh,0Dh,0
aSwitches db ' switches.',0Dh,0Dh,0
aSUses db '%s uses...',0Dh,0Dh,0
aSShrieksAtTheE db '%s shrieks at the enemy!',0Dh,0Dh,0
aSShrieksButNoO db '%s shrieks but no one notices!',0Dh
db 0Dh,0
aSReachesOutAnd db '%s reaches out and stuns %Fs!',0Dh,0Dh
db 0
aSFailsInAnAtte db '%s fails in an attempt to stun %Fs!'
db 0Dh,0Dh,0
aSDropsSS db '%s drops %s %s.',0Dh,0Dh,0
aSTradesSSToS db '%s trades %s %s to %s.',0Dh,0Dh,0
aSSTradeFails db '%s',27h,'s trade fails.',0Dh,0Dh,0
aHeals db 'heals',0
aCures db 'cures',0
aSSS db '%s %s %s.',0Dh,0Dh,0
aHeal db 'heal',0
aCure db 'cure',0
aSFailsToSS db '%s fails to %s %s.',0Dh,0Dh,0
aSLosesLdExperi db '%s loses %ld experience points for '
db 'killing innocent bystanders.',0Dh,0Dh
db 0
aSGainsLdExperi db '%s gains %ld experience points.',0Dh
db 0Dh,0
aGetCloser db 'Get closer',0
aEvade_0 db 'Evade',0
aUseShriek db 'Use shriek',0
aStun db 'Stun ',0
aAttack_0 db 'Attack ',0
aReloadWith db 'Reload with ',0
aUnjam db 'Unjam ',0
aEquip db 'Equip ',0
aHeal_0 db 'Heal ',0
aCure_0 db 'Cure ',0
aUse_0 db 'Use ',0
aUse_1 db 'Use ',0
aTrade db 'Trade ',0
aDrop db 'Drop ',0
aIncapacitated db '(Incapacitated)',0
aRunAway db 'Run away',0
db    0
off_21DD0 dw offset aGetCloser ; "Get closer"
dw offset aEvade_0      ; "Evade"
dw offset aUseShriek    ; "Use shriek"
dw offset aStun         ; "Stun "
dw offset aAttack_0     ; "Attack "
dw offset aReloadWith   ; "Reload with "
dw offset aUnjam        ; "Unjam "
dw offset aEquip        ; "Equip "
dw offset aHeal_0       ; "Heal "
dw offset aCure_0       ; "Cure "
dw offset aUse_0        ; "Use "
dw offset aUse_1        ; "Use "
dw offset aTrade        ; "Trade "
dw offset aDrop         ; "Drop "
dw offset aIncapacitated ; "(Incapacitated)"
dw offset aRunAway      ; "Run away"
aPleaseInsertSS db 'Please insert %s.',0Dh,0Dh,'<space '
db 'bar> when ready.',0
a1212s db '%-12.12s ',0
aFs_1 db '%Fs',0
aFs_2 db '%Fs',0
aFs_3 db '%Fs',0
aSSuccessfullyU db '%s successfully unjams %s %s.',0Dh,0Dh
db 0
aSTrashesSSWhil db '%s trashes %s %s while trying to fi'
db 'x it.',0Dh,0Dh,0
aSNowHasAFsLeve db '%s now has a %Fs level of %d.',0
a1d1919fs db '%1d> %-19.19Fs',0
aWhatDoYouWish1 db 'What do you wish?',0Dh,0Dh,0Dh,'1. '
db 'Retrieve a member',0Dh,'2. Store a '
db 'member',0Dh,0Dh,'L)eave',0
aThereSNoOneHer db 0Dh,'There',27h,'s no one here!',0
aYourPartyIsFul db 0Dh,'Your party is full!',0
aPickUpWhom db 'Pick up whom?',0
aWithYouGoneWho db 0Dh,'With you gone, who',27h,'s gonn'
db 'a type?',0
aWeReFullRightN db 0Dh,'We',27h,'re full right now!',0
aWhoWillStay db 'Who will stay?',0
db    0
aFpict db 'FPICT',0
aDpict db 'DPICT',0
aKpict db 'KPICT',0
aEpict db 'EPICT',0
aFeelsAStrongTi db 'feels a strong tingling sensation i'
db 'n',0
aBlinksBackTear db 'blinks back tears from a very painf'
db 'ul burning of',0
aSneezesRapidly db 'sneezes rapidly and notices a purpl'
db 'ish mist radiating from',0
aBecomesMomenta db 'becomes momentarily dizzy as a tric'
db 'kle of blood seeps from both',0
aSomewhatUninte db '(somewhat unintelligibly), complain'
db 's about a sudden horrible spasming '
db 'of',0
aReelsAndStagge db 'reels and staggers as a sudden weak'
db 'ness envelops',0
aHands_1 db 'hands',0
aEyes db 'eyes',0
aNose db 'nose',0
aEars db 'ears',0
aTongue db 'tongue',0
aLegs db 'legs',0
aSSSS db '%s %s %s %s.',0Dh,0
aSSMutationsAre db '%s',27h,'s mutations are acting up '
db 'again. Healing ability impaired.',0
dw offset aFpict        ; "FPICT"
dw offset aDpict        ; "DPICT"
dw offset aKpict        ; "KPICT"
dw offset aEpict        ; "EPICT"
db    0
db    0
db    0
db    0
db 0DCh
db    5
db    0
db    0
db 0E8h
db    3
db    0
db    0
db 0DCh
db    5
db    0
db    0
db  94h
db  11h
db    0
db    0
db  64h ; d
db  19h
db    0
db    0
db  1Ch
db  25h ; %
db    0
db    0
db 0ECh
db  2Ch ; ,
db    0
db    0
db 0D8h
db  59h ; Y
db    0
db    0
db    0
db  7Dh ; }
db    0
db    0
db 0E0h
db 0ABh
db    0
db    0
db 0A8h
db 0DEh
db    0
db    0
db 0A8h
db  5Bh ; [
db    1
db    0
db 0D8h
db 0D0h
db    1
db    0
db  46h ; F
db  0Dh
db  6Bh ; k
db  0Dh
db  9Ch
db  0Dh
db 0D7h
db  0Dh
db  17h
db  0Eh
db  60h ; `
db  0Eh
db  91h
db  0Eh
db  97h
db  0Eh
db  9Ch
db  0Eh
db 0A1h
db  0Eh
db 0A6h
db  0Eh
db 0ADh
db  0Eh
db    2
db    1
db    3
db    4
db    6
db    0
db  0Ch
db  0Dh
db  0Eh
db  1Fh
aSYouHaveGained db '%s, you have gained %d hit points a'
db 'nd you are elevated to the rank of '
db '%d',0
aDistributeDPoi db 'Distribute %d points:',0
aYouHaveAttaine db 'You have attained the highest level'
db ' for that attribute.',0
aGoodWork db 'Good work!',0
a1d51d1616s db '%1d>%5.1d %-16.16s',0
a1d31d1818s db '%1d>%3.1d %-18.18s',0
aYouFindTheseIt db 'You find these items:',0
aSHasTooMuchToC_0 db '%s',0Dh,'has too much to carry',0Dh
db 'already.',0
aVictory db '  Victory!  ',0
aSeeYouLater db 0Dh,0Dh,0Dh,'    See you later...!',0
aItS db 'it',27h,'s',0
aHis_0 db 'his',0
aHer_1 db 'her',0
aSerious db 'serious',0
aCritical db 'critical',0
aComatose db 'comatose',0
db    0
dw offset aItS          ; "it's"
dw offset aHis_0        ; "his"
dw offset aHer_1        ; "her"
dw offset aSerious      ; "serious"
dw offset aCritical     ; "critical"
dw offset aComatose     ; "comatose"
aFlushingNeeded db 'Flushing Needed!!',0
aSTakesUPtsOfDa db '%s takes %u pts of damage',0
aAndHasBeenRend db ' and has been rendered unconscious',0
aAndSConditionH db ' and %s condition has become %s',0
aAndIsKilled db ' and is killed',0
asc_22378 db '.',0Dh,0Dh,0
asc_2237C db '.',0Dh,0Dh,0
aSSArmorAbsorbs db '%s',27h,'s armor absorbs the damage'
db '.',0Dh,0Dh,0
aYEsNO db 'Y)es  N)o',0
asc_223AB db '>',0
aHIreATtackUSe db 'H)ire A)ttack U)se',0
aLEave db 'L)eave',0
aIAmNotInterest db 'I am not interested in joining up w'
db 'ith the likes of you',0
aThatDoesnTAffe db 'That doesn',27h,'t affect me!',0
aFs_4 db '%Fs',0
aEnterNewLocati db 'Enter new location?',0
db    0
db    1
db    0
db    4
db    0
db    7
db    0
db  0Ah
db    0
db  0Fh
db  27h ; '
db 0F9h
db 0FFh
db 0EAh
db 0FFh
db 0DBh
db 0FFh
aSYouHaveLu db '%s, you have $%lu',0
a1d1616s6ld db '%1d>%-16.16s %6ld',0
a1dC_0 db '%1d>%c',0
a1515s6ld db '%-15.15s %6ld',0
aBuyWhichItemPr db 'Buy which item:',0Dh,0Dh,'         '
db '           Price',0
aYouDoNotHaveEn db 'You do not have enough cash for tha'
db 't.',0
aSBuys1S db '%s buys 1 %s.',0
aYourInventoryI db 'Your inventory is full.',0
aWeHaveNothingM db 'We have nothing more to sell.',0
aSellWhichItemP db 'Sell which item?',0Dh,0Dh,'        '
db '            Price',0
aSSells1S db '%s sells 1 %s.',0
aYouHaveNothing db 'You have nothing we want.',0
; char aBUySEllLEave[]
aBUySEllLEave db 'B)uy    S)ell    L)eave',0
a1d1616fs6ld db '%1d>%-16.16Fs %6ld',0
aWhichServiceSe db 'Which service?',0Dh,'  Service     '
db '    Price',0
aYouHaveNoNeedF db 'You have no need for this service.',0
aThatServiceIsN db 'That service is not available to yo'
db 'u at this time.',0
aYouDoNotHaveTh db 'You do not have the funds for that.'
db 0
aF1d1919s db 'F%1d> %-19.19s',0
a1d1919fs_0 db '%1d> %-19.19Fs',0
aSIsBeyondYourH db '%s is beyond your help.',0
aYourEffortsWer db 'Your efforts were commendable, but '
db 'unsuccessful this time.',0
aSIsNowALevelDF db '%s is now a level %d %Fs.',0
aHealWhom db 'Heal whom?',0
aCureWhom db 'Cure whom?',0
aSuccessSIsGett db 'Success! %s is getting better.',0
aMedic db 'Medic',0
aSDoesNotNeedAn db '%s does not need any attention.',0
aCureWhat db 'Cure what?',0
aSuccessYourEff db 'Success! Your efforts have cured %s'
db '.',0
aSIsNotAfflicte db '%s is not afflicted by anything!',0
aWhoWillIngestT db 'Who will ingest this?',0
aSAlthoughPlagu db '%s, although plagued by pink elepha'
db 'nts, will not be bothered by any mu'
db 'tations for 2 days.',0
aSWillNotBeBoth db '%s will not be bothered by any muta'
db 'tions for 2 days.',0
db    0
aSt db 'ST:',0
aIq db 'IQ:',0
aDx db 'DX:',0
aWp db 'WP:',0
aAp db 'AP:',0
aCh db 'CH:',0
aLk db 'LK:',0
a1d2d9fs db '%1d>%2d %-.9Fs',0
db    0
db    1
db    9
dw offset aSt           ; "ST:"
dw 911h
dw offset aIq           ; "IQ:"
dw 0A01h
dw offset aDx           ; "DX:"
dw 0A09h
dw offset aWp           ; "WP:"
dw 0A11h
dw offset aAp           ; "AP:"
dw 0B01h
dw offset aCh           ; "CH:"
dw 0B11h
dw offset aLk           ; "LK:"
a1d2d9fs_0 db '%1d>%2d %-.9Fs',0
aS db '%s',0
asc_22866 db ')',0
unk_22868 db    0
aAfflicted db 'Afflicted',0
aUnafflicted db 'Unafflicted',0
asc_2287F db '(',0
aCond db 'Cond: ',0
aSSS_0 db '%s%-s%-s',0
aRankU db 'Rank: %u',0
aVIew db '(V)iew',0
aCon3uMaxcon3u db ' Con: %3u  MaxCon:  %3u',0
aCon3sMaxcon3u db ' Con: %3s  MaxCon:  %3u',0
aAc3u db '  AC: %3u',0
aHands_2 db 'Hands',0
aWeap17s db 'Weap: %-.17s',0
aAttributes db 'Attributes',0
a3s2u db '%3s%2u',0
aActive db 'Active',0
aActive_0 db 'Active',0
aPassive db 'Passive',0
aPassive_0 db 'Passive',0
aInventoryItems db 'Inventory Items',0
aConditions db 'Conditions:',0
aRemissionExpDH db '(remission exp:%d hrs.)',0
aSIsIncapacitat db '%s is incapacitated.',0
aF1d1919s_0 db 'F%1d>%-19.19s',0
aCon3u3u db 'Con:%3u /%3u',0
aCon3s3u db 'Con:%3s /%3u',0
aAc2d db 'AC: %2d',0
aHands_3 db 'Hands',0
aWeapon1919sAmm db 'Weapon: %-19.19s   Ammo: %2d',0
a1d1919s db '%1d> %-19.19s',0
aCashLu db 'Cash: $%lu',0
aREorderRoster db '(R)eorder roster',0
aBAnishLEave db '(B)anish   (L)eave',0
aPickAPlayer db 'Pick a Player:',0
aHeyIMAllYouVeG db 'Hey! I',27h,'m all you',27h,'ve got'
db '!!',0
aBanishWhom db 'Banish whom:',0
aReallyBanishS db 'Really banish',0Dh,'%s?',0Dh,0Dh,0
aThereIsNoRoomI db 'There is no room in the party.',0
db  13h
db  28h ; (
db 0FFh
db 0FFh
unk_22A64 db  64h ; d
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
byte_22A74 db 0
; char str[]
str db 'Welcome to the beautiful island of '
db 'Florida!',0Dh,0
aYouDoNotHaveEn_0 db 'You do not have enough memory to ru'
db 'n Fountain of Dreams.$'
_dialogBorderTop db 7, 5, 84h, 80h, 16h dup(81h), 80h, 84h
db 0FFh
_dialogBorderLeft db 7, 6, 82h, 0Ah dup(0B5h), 82h, 0FFh
_dialogBorderRight db 20h, 6, 82h, 0Ah dup(0B5h), 82h, 0FFh
_dialogBorderBottom db 7, 12h, 84h, 80h, 16h dup(81h), 80h, 84h
db 0FFh
unk_22B33 db    7
db    1
db 0ADh
db  82h
db  82h
db 0B3h
db 0B6h
db 0B7h
db 0B8h
db  82h
db  8Ah
db  8Ah
db  8Ah
db  82h
db 0FFh
unk_22B42 db  20h
db    1
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0ABh
db 0AFh
db  82h
db  82h
db 0B9h
db 0FFh
unk_22B51 db  0Dh
db    1
db 0ADh
db  82h
db  82h
db 0B3h
db 0B6h
db 0B7h
db 0B8h
db  82h
db  8Ah
db  8Ah
db  8Ah
db  82h
db 0FFh
unk_22B60 db  1Ah
db    1
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0AEh
db 0ABh
db 0AFh
db  82h
db  82h
db 0B9h
db 0FFh
unk_22B6F db  11h
db  12h
aAc db 'AC',0
aAmm db 'AMM',0
aMax db 'MAX',0
aCon db 'CON',0
aWeapon_0 db 'WEAPON'
db 0FFh
unk_22B87 db    7
db  14h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db  81h
db 0FFh
unk_22BA4 db    7
db  11h
db  82h
db  83h
db  83h
db  83h
db  83h
db  83h
db  82h
db 0FFh
unk_22BAE db  20h
db  11h
db  82h
db  83h
db  83h
db  83h
db  83h
db  83h
db  82h
db 0FFh
_inverseText db 0
byte_22BB9 db 0
byte_22BBA db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
db 0FFh
unk_22BC1 db    4
db    2
db    1
db    1
db    3
db    3
db    2
db    2
db    0
db    3
db    2
db    2
db    4
db    2
db    0
db    1
db    1
db    3
db    1
db    4
db    3
db    2
db    0
db    1
db    3
db    2
db    4
db    2
db    5
db    7
db    5
db    1
db    2
db    0
db    3
db    2
db    2
db    1
db    0
db    1
db    0
db    4
db    2
db    2
db    2
db    1
db    0
byte_22BF0 db 3
db    0
db    0
db    2
db    0
db    4
db    2
db    1
db    1
db    2
db    1
db    3
db    3
db    0
db    2
db    1
db    3
db    2
db    2
db    2
db    0
db    0
db    1
db    3
db    4
db    0
db    3
db    1
db    1
unk_22C0D db  22h ; "
db 0DFh
unk_22C0F db  22h ; "
db 0DFh
unk_22C11 db  22h ; "
db 0DFh
unk_22C13 db  22h ; "
db 0DFh
unk_22C15 db    0
db    0
unk_22C17 db  22h ; "
db 0DFh
db  22h ; "
db 0DFh
db  22h ; "
db 0DFh
unk_22C1D db  36h ; 6
db 0DFh
unk_22C1F db  36h ; 6
db 0DFh
db    0
db    0
word_22C23 dw 0DF22h
word_22C25 dw 0DF22h
word_22C27 dw 0DF22h
dw 0DF36h
dw 0
db    0
off_22C2E dw offset unk_22C15
dw offset unk_22C13
dw offset unk_22C11
dw offset unk_22C1F
dw offset unk_22C0F
dw offset word_22C27
dw offset unk_22C0D
dw offset word_22C25
dw offset unk_22C1D
dw offset word_22C23
dw offset unk_22C17
word_22C44 dw 0
word_22C46 dw 0
word_22C48 dw 0
; void far *dst
dst dd 0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
_picDecoded dd 0
; void far *services
_services dd 0
; void far *archetypesData
_archetypesData dd 0
; void far *hdsPct
_hdsPct dd 0
; void far *fontData
_fontData dd 0
; void far *borders
_borders dd 0
dword_22C6E dd 0
word_22C72 dw 0
word_22C74 dw 0
db  70h ; p
db  1Eh
db  68h ; h
db  1Eh
db  78h ; x
db  1Eh
db  80h
db  1Eh
db  90h
db  1Eh
db  88h
db  1Eh
db  98h
db  1Eh
db 0A0h
db  1Eh
word_22C86 dw 0
word_22C88 dw 0
word_22C8A dw 0
; Window *currentWindow
_currentWindow dw 0
db    0
db    0
db    0
db    0
db 0A0h
db    0
db 0C8h
db    0
db  1Ch
db    0
db    8
db    0
db  68h ; h
db    0
db  60h ; `
db    0
unk_22C9E db    4
db    0
db  18h
db    0
db  98h
db    0
db  90h
db    0
db  1Ch
db    0
db  28h ; (
db    0
db  68h ; h
db    0
db  70h ; p
db    0
db    4
db    0
db    8
db    0
db  98h
db    0
db  60h ; `
db    0
db  48h ; H
db    0
db 0A8h
db    0
db  14h
db    0
db    8
db    0
unk_22CBE db  4Ch ; L
db    0
db  58h ; X
db    0
db    8
db    0
db  10h
db    0
db    4
db    0
db    8
db    0
db  30h ; 0
db    0
db  60h ; `
db    0
db  38h ; 8
db    0
db    8
db    0
db  30h ; 0
db    0
db  60h ; `
db    0
db  6Ch ; l
db    0
db    8
db    0
db  30h ; 0
db    0
db  60h ; `
db    0
db  48h ; H
db    0
db  60h ; `
db    0
db  10h
db    0
db  20h
db    0
db  0Ch
db    0
db    0
db    0
db    8
db    0
db    8
db    0
db  28h ; (
db    0
db    0
db    0
db    8
db    0
db    8
db    0
db  44h ; D
db    0
db    0
db    0
db    8
db    0
db    8
db    0
db  60h ; `
db    0
db    0
db    0
db    8
db    0
db    8
db    0
db  7Ch ; |
db    0
db    0
db    0
db    8
db    0
db    8
db    0
db 0A6h
db  1Ah
db 0AEh
db  1Ah
db 0B6h
db  1Ah
db 0BEh
db  1Ah
db 0C6h
db  1Ah
; Window windowInfo
_windowInfo Window <7, 22, 39, 24, 7, 22, 1Ch, 0B0h,\
        180084h, 0, 0DAAAh, 0, 0>
; Window window1
_window1 Window <1, 19, 38, 23, 1, 19, 4, 9Ch, \
        280098h, 8, 0, 0, 0>
; Window windowDialog
_windowDialog Window <8, 6, 31, 17, 8, 6, 20h, 30h, \
        600060h, 0, 0DAD3h, 0, 1A66h>
; Window windowCharInfoStats
_windowCharInfoStats Window <1, 1, 23, 13, 1, 1, 4, 8, \
        68005Ch, 0, 0, 0, 0>
; Window window16
_window16 Window <1, 2, 23, 13, 1, 2, 4, 10h, \
        60005Ch, 8, 0, 0, 0>
; Window window15
_window15 Window <1, 14, 23, 23, 1, 14, 4, 70h, \
        50005Ch, 0, 0, 0, 0>
; Window window13
_window13 Window <25, 1, 38, 11, 25, 1, 64h, 8, \
        580038h, 0, 0, 0, 0>
; Window window14
_window14 Window <25, 13, 38, 23, 25, 13, 64h, \
        68h, 580038h, 0, 0, 0, 0>
; Window window9
_window9 Window <8, 1, 31, 12, 8, 1, 20h, 8, \
        600060h, 0, 0DAF0h, 0, 1A56h>
; Window window8
_window8 Window <0, 14, 39, 17, 0, 14, 0, 70h, \
        2000A0h, 0, 0DAB7h, 0, 0>
db    1
db    0
db    1
db    0
db  26h ; &
db    0
db  0Bh
db    0
db    1
db    0
db    1
db    0
db    4
db    0
db    8
db    0
db  98h
db    0
db  60h ; `
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
unk_22E4C db    1
db    0
db    1
db    0
db  0Ch
db    0
db  0Bh
db    0
db    1
db    0
db    1
db    0
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
db    0
db    0
word_22E66 dw 1A86h
db  0Eh
db    0
db    1
db    0
db  19h
db    0
db  0Bh
db    0
db  0Eh
db    0
db    1
db    0
db  38h ; 8
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
db    0
db    0
db  8Eh
db  1Ah
db  1Bh
db    0
db    1
db    0
db  26h ; &
db    0
db  0Bh
db    0
db  1Bh
db    0
db    1
db    0
db  6Ch ; l
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
db    0
db    0
word_22E9E dw 1A96h
db  12h
db    0
db  0Ch
db    0
db  18h
db    0
db  10h
db    0
db  12h
db    0
db  0Ch
db    0
db  48h ; H
db    0
db  60h ; `
db    0
db  10h
db    0
db  20h
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db  9Eh
db  1Ah
; Window window3
_window3 Window <0, 0, 39, 24, 0, 0, 0, 0, \
        0C800A0h, 0, 0, 0, 0>
; Window windowTime
_windowTime Window <1, 22, 5, 23, 1, 22, 4, 0B0h, \
        100014h, 0, 0, 0, 0>
; Window windowCharInfo
_windowCharInfo Window <0, 0, 39, 24, 0, 0, 0, 0, \
        0C800A0h, 0, 0DAB0h, 0, 0>
; Window window7
_window7 Window <14, 1, 38, 12, 14, 1, 38h, 8, \
        600064h, 0, 0, 0, 0>
; Window window10
_window10 Window <0, 14, 39, 17, 0, 14, 0, 70h, \
        2000A0h, 0, 0DAC5h, 0, 0>
; Window window4
_window4 Window <0, 0, 39, 24, 0, 0, 0, 0, \
        0C800A0h, 0, 0DAA3h, 0, 0>
db    1
db    0
db    1
db    0
db  26h ; &
db    0
db    3
db    0
db    1
db    0
db    1
db    0
db    4
db    0
db    8
db    0
db  98h
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
db    1
db    0
db    5
db    0
db  26h ; &
db    0
db    7
db    0
db    1
db    0
db    5
db    0
db    4
db    0
db  28h ; (
db    0
db  98h
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
db    1
db    0
db    9
db    0
db  26h ; &
db    0
db  0Bh
db    0
db    1
db    0
db    9
db    0
db    4
db    0
db  48h ; H
db    0
db  98h
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
db    1
db    0
db  0Dh
db    0
db  26h ; &
db    0
db  0Fh
db    0
db    1
db    0
db  0Dh
db    0
db    4
db    0
db  68h ; h
db    0
db  98h
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
db    1
db    0
db  11h
db    0
db  26h ; &
db    0
db  13h
db    0
db    1
db    0
db  11h
db    0
db    4
db    0
db  88h
db    0
db  98h
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
; Window window18
_window18 Window <1, 21, 38, 23, 1, 21, 4, 0A8h, \
        180098h, 0, 0DA9Bh, 0, 0>
; Window window19
_window19 Window <8, 17, 31, 23, 1, 17, 20h, 88h, \
        380060h, 0, 0DA8Ch, 0, 0>
_partyWindow0 Window <1, 1, 6, 1, 1, 1, 4, 8, 80018h, \
        0, 0, 0, 0>
_partyWindow1 Window <8, 1, 13, 1, 8, 1, 20h, 8, \
        80018h, 0, 0, 0, 0>
_partyWindow2 Window <15, 1, 20, 1, 15, 1, 3Ch, 8, \
        80018h, 0, 0, 0, 0>
_partyWindow3 Window <22, 1, 27, 1, 22, 1, 58h, 8, \
        80018h, 0, 0, 0, 0>
_partyWindow4 Window <29, 1, 34, 1, 29, 1, 74h, 8, \
        80018h, 0, 0, 0, 0>
dw 1C28h
; Window *win
win dw offset unk_22E4C
db  44h ; D
db  1Ch
db  28h ; (
db  1Ch
word_230BC dw 1D24h
db  40h ; @
db  1Dh
db  5Ch ; \
db  1Dh
db  78h ; x
db  1Dh
db  94h
db  1Dh
_partyNameWindows dw offset _partyWindow0
dw offset _partyWindow1
dw offset _partyWindow2
dw offset _partyWindow3
dw offset _partyWindow4
_writeStringPtr1 dw 0
; char *writeStringPtr2
_writeStringPtr2 dw 0
_writeStringPtr3 dw 0
word_230D6 dw 0
word_230D8 dw 0
word_230DA dw 0
word_230DC dw 0
word_230DE dw 0
word_230E0 dw 0
word_230E2 dw 0
unk_230E4 db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
word_2318E dw 0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
unk_2323A db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db  60h ; `
db    9
db 0E8h
db    3
db 0F4h
db    1
db    1
db    0
db  64h ; d
db    0
db  0Fh
db    0
db 0FAh
db    0
db    5
db    0
db    0
db    0
word_2B05C dw 1E68h
db  60h ; `
db  1Eh
db  38h ; 8
db  1Fh
db  48h ; H
db  1Eh
db  50h ; P
db  1Eh
db  58h ; X
db  1Eh
db 0C8h
db  1Eh
db 0D0h
db  1Eh
db 0D8h
db  1Eh
db 0E0h
db  1Eh
db 0E8h
db  1Eh
db 0F0h
db  1Eh
db 0F8h
db  1Eh
db    0
db  1Fh
db    8
db  1Fh
db  10h
db  1Fh
db  18h
db  1Fh
db  20h
db  1Fh
db  28h ; (
db  1Fh
db  30h ; 0
db  1Fh
db  30h ; 0
db    0
db  50h ; P
db    0
db  70h ; p
db    0
db  90h
db    0
db 0B0h
db    0
db 0D0h
db    0
db 0F0h
db    0
db  10h
db    1
db  30h ; 0
db    1
db    4
db    0
db  0Ch
db    0
db  14h
db    0
db  1Ch
db    0
db  24h ; $
db    0
db  2Ch ; ,
db    0
db  34h ; 4
db    0
db  3Ch ; <
db    0
db  44h ; D
db    0
db  4Ch ; L
db    0
db  54h ; T
db    0
db  5Ch ; \
db    0
db  64h ; d
db    0
db  6Ch ; l
db    0
db  74h ; t
db    0
db  7Ch ; |
db    0
db  84h
db    0
db  8Ch
db    0
db  94h
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    8
db 0D7h
db    7
db    0
db    8
db  56h ; V
db    8
byte_2B166 db 2
db    0
db    0
db    0
db    0
db    0
db    0
_picDecoded_0 dd 0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
_scanlinePtrs dw 384h dup(0)
word_2BA09 dw 0
db    0
db    0
byte_2BA0D db 0
word_2BA0E dw 0
word_2BA10 dw 0A251h
word_2BA12 dw 0
db    0
db    0
db    0
db    0
db    0
db  27h ; '
db    3
db  25h ; %
db    3
db    0
db  1Bh
db  1Bh
db  20h
db    0
db    0
db  1Bh
db    0
db  1Bh
db  20h
db  1Bh
db    5
db  26h ; &
db  26h ; &
db  26h ; &
db  10h
db  10h
db  10h
db    0
db    0
db  38h ; 8
db    0
db  3Fh ; ?
db    0
db    0
db  3Fh ; ?
db  3Fh ; ?
db  3Ah ; :
db  20h
db  10h
db  3Fh ; ?
db    0
db  3Fh ; ?
db  3Fh ; ?
db  3Fh ; ?
db    0
db  3Fh ; ?
db  3Fh ; ?
db  3Fh ; ?
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
aYouSeeMilesOfI db 0Dh,'You see miles of impassable oce'
db 'an and you decide to turn back.',0Dh
db 0
aThereAreRoadsA db 0Dh,'There are roads around here som'
db 'ewhere!',0Dh,0
aTryUsingTheRoa db 0Dh,'Try using the road, friend.',0Dh
db 0
aHic db 0Dh,'            hic!',0Dh,0
db    0
word_2BB10 dw 0
word_2BB12 dw 0
word_2BB14 dw 0
word_2BB16 dw 0
word_2BB18 dw 0
word_2BB1A dw 0
byte_2BB1C db 0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
aDisk1_6 db 'DISK 1',0
aDisk2_3 db 'DISK 2',0
aDisk3_2 db 'DISK 3',0
aDisk4_1 db 'DISK 4',0
db 0EEh
db 0B8h
db 0F5h
db 0B8h
db 0FCh
db 0B8h
db    3
db 0B9h
_dosVersion dw 0
byte_2CB54 db 0
word_2CB55 dw 0
_savedCricialFn dd 0
db  35h ; 5
db 0B9h
db  6Ch ; l
db 0B9h
db  7Fh ; 
db 0B9h
db 0D9h
db 0B9h
db 0D4h
db 0B9h
db 0D9h
db 0B9h
db 0D4h
db 0B9h
db 0C1h
db 0B9h
db 0D4h
db 0B9h
db 0D9h
db 0B9h
db 0D4h
db 0B9h
db 0D4h
db 0B9h
db 0D9h
db 0B9h
aThisDiskIsWrit db 'This disk is write protected.',0Dh,0Dh
db '<space bar> when ready.',0
aUnknownDriveNa db 'Unknown drive name',0
aPleaseCloseThe db 'Please close the door on the disk d'
db 'rive.',0Dh,0Dh,'<space bar> when re'
db 'ady.',0
aUnknownMediaTy db 'Unknown media type',0
aErr5 db 'err5',0
aErr6 db 'err6',0
byte_2CC1E db 0FFh
byte_2CC1F db 0
byte_2CC20 db 0
byte_2CC21 db 0
byte_2CC22 db 0
word_2CC23 dw 0
word_2CC25 dw 0
word_2CC27 dw 0
word_2CC29 dw 0
word_2CC2B dw 0
word_2CC2D dw 0
db  81h
db    0
db  83h
db  0Ch
db 0BCh
db  1Bh
db  70h ; p
db  0Ch
db  30h ; 0
db    6
db  84h
db  81h
db    0
db  83h
db    2
db 0BCh
db  1Fh
db  70h ; p
db    1
db  30h ; 0
db    6
db  84h
db  81h
db    0
db  83h
db    8
db 0BCh
db    0
db 0C0h
db    0
db  60h ; `
db    0
db  84h
db  81h
db    0
db  83h
db  1Eh
db 0FFh
db    1
db    1
db    0
db    0
db    0
db  83h
db    8
db 0FFh
db  0Fh
db  37h ; 7
db    0
db  30h ; 0
db    6
db  83h
db  14h
db 0BCh
db  1Bh
db  70h ; p
db  0Ch
db  30h ; 0
db    6
db  84h
db  81h
db    0
db  83h
db    8
db    0
db  12h
db  80h
db  10h
db    0
db    0
db  84h
db  81h
db    0
db  83h
db    5
db 0C0h
db    1
db 0FFh
db 0FFh
db    0
db    0
db  83h
db  0Ah
db 0C0h
db    5
db    0
db    0
db    0
db    0
db  83h
db    5
db 0C0h
db    1
db    0
db    0
db    0
db    0
db  83h
db  0Ah
db 0C0h
db    5
db    0
db    0
db    0
db    0
db  83h
db    5
db 0C0h
db    1
db    0
db    0
db    0
db    0
db  83h
db  0Ah
db 0C0h
db    5
db    0
db    0
db    0
db    0
db  84h
db  81h
db    0
db  83h
db    3
db 0CDh
db  10h
db  0Ah
db    0
db    0
db    0
db  84h
db  2Bh ; +
db 0BAh
db  69h ; i
db 0BAh
db  36h ; 6
db 0BAh
db 0EFh
db 0B9h
db    5
db 0BAh
db  10h
db 0BAh
db 0FAh
db 0B9h
word_2CCC2 dw 0
word_2CCC4 dw 0
off_2CCC6 dw offset __exit
word_2CCC8 dw 0
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
word_2CD18 dw 0BA88h
aCFileInfo db ';C_FILE_INFO',0
dword_2CD27 dd 0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
db    0
word_2CD33 dw 0
db    0
db    0
db    0
db    0
word_2CD39 dw 0
word_2CD3B dw 0
align 2
byte_2CD3E db 0
align 2
word_2CD40 dw 14h
db  81h
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
argc dw 0
; char **argv
argv dw 0
word_2CD5A dw 0
word_2CD5C dw 0BB20h
seg_2CD5E dw seg dseg
db  43h ; C
db    0
db    0
db    0
byte_2CD64 db 0
byte_2CD65 db 0
dword_2CD66 dd 0
word_2CD6A dw 0
word_2CD6C dw 0DEF0h
word_2CD6E dw 0
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
word_2CD84 dw 0
db  8Ah
db 0D2h
db    0
db    0
db  8Ah
db 0D2h
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
word_2CE9E dw 0BBDEh
aNull db '(null)',0
aNull_0 db '(null)',0
db '+- #',0
align 2
word_2CEB4 dw 0
word_2CEB6 dw 0
db    0
db    0
word_2CEBA dw 0
db    0
db    0
word_2CEBE dw 0
db    0
db    0
word_2CEC2 dw 0
word_2CEC4 dw 2000h
align 4
byte_2CEC8 db 0
align 2
off_2CECA dw offset __fptrap
off_2CECC dw offset __fptrap
db 0CAh
db 0E8h
off_2CED0 dw offset __fptrap
off_2CED2 dw offset __fptrap
db    0
db    0
db    0
db    0
db    0
db    0
db    0
byte_2CEDB db 20h
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
word_2CFDC dw 0FFFFh
word_2CFDE dw 0
word_2CFE0 dw 0
align 4
unk_2CFE4 db    0
db    0
word_2CFE6 dw 0
dword_2CFE8 dd 0
dword_2CFEC dd 0
db 0EEh
db 0EBh
aNmsg db '<<NMSG>>',0
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
db '- not enough space for environment',0Dh
db 0Ah,0
db 0FCh
db    0
db  0Dh
db  0Ah
db    0
db 0FFh
db    0
aRunTimeError db 'run-time error ',0
db    2
db    0
aR6002FloatingP db 'R6002',0Dh,0Ah
db '- floating point not loaded',0Dh,0Ah
db 0
db    1
db    0
aR6001NullPoint db 'R6001',0Dh,0Ah
db '- null pointer assignment',0Dh,0Ah,0
db 0FFh
db 0FFh
db 0FFh
db    0
word_2D0CC dw 0
; FILE *word_2D0CE
word_2D0CE dw 0
word_2D0D0 dw ?
word_2D0D2 dw ?
word_2D0D4 dw ?
word_2D0D6 dw ?
word_2D0D8 dw ?
word_2D0DA dw ?
word_2D0DC dw ?
word_2D0DE dw ?
word_2D0E0 dw ?
word_2D0E2 dw ?
word_2D0E4 dw ?
word_2D0E6 dw ?
word_2D0E8 dw ?
; char *word_2D0EA
word_2D0EA dw ?
word_2D0EC dw ?
word_2D0EE dw ?
; int word_2D0F0
word_2D0F0 dw ?
align 10h
word_2D100 dw ?
; Window *word_2D102
word_2D102 dw ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
word_2D126 dw ?
word_2D128 dw ?
_disk1 Disk1 <?>
word_2DFEA dw ?
word_2DFEC dw ?
byte_2DFEE db ?
align 2
; char *dword_2DFF0
dword_2DFF0 dd 3 dup(?)
; char *word_2DFFC
word_2DFFC dw ?
word_2DFFE dw ?
db    ? ;
db    ? ;
word_2E002 dw ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
dword_2E200 dd ?
word_2E204 dw ?
word_2E206 dw ?
; void far *weapons
_weapons dw ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
_textX dw ?
_textY dw ?
word_2E418 dw ?
_globals dw ?
word_2E41C dw ?
dword_2E41E dd ?
_quitGame db ?
byte_2E423 db ?
word_2E424 dw ?
word_2E426 dw ?
byte_2E428 db ?
align 2
dword_2E42A dd ?
word_2E42E dw ?
word_2E430 dw ?
word_2E432 dw ?
word_2E434 dw ?
word_2E436 dw ?
word_2E438 dw ?
word_2E43A dw ?
word_2E43C dw ?
byte_2E43E db ?
byte_2E43F db ?
byte_2E440 db ?
byte_2E441 db ?
byte_2E442 db ?
byte_2E443 db ?
byte_2E444 db ?
align 2
dword_2E446 dd ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
word_2E4BA dw ?
word_2E4BC dw ?
byte_2E4BE db ?
align 2
dword_2E4C0 dd ?
word_2E4C4 dw ?
dword_2E4C6 dd ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
byte_2E6D4 db ?
byte_2E6D5 db ?
byte_2E6D6 db ?
db    ? ;
db    ? ;
db    ? ;
byte_2E6DA db ?
align 2
byte_2E6DC db ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
word_2EBF4 dw ?
word_2EBF6 dw ?
word_2EBF8 dw ?
word_2EBFA dw ?
word_2EBFC dw ?
align 4
; char *word_2EC00
word_2EC00 dw ?
word_2EC02 dw ?
dword_2EC04 dd ?
word_2EC08 dw ?
word_2EC0A dw ?
_professions dd ?
word_2EC10 dw ?
word_2EC12 dw ?
word_2EC14 dw ?
word_2EC16 dw ?
; void far *packetsSize
_packetsSize dw ?
dword_2EC1A dd ?
dword_2EC1E dd ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
; char sprintf_buffer[]
sprintf_buffer db 23h dup(?)
byte_2EC49 db ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
byte_2ED26 db ?
align 2
_party dw 5 dup(?)
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
byte_2EE06 db ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
_enemies dd ?
dword_2EE12 dd ?
word_2EE16 dw ?
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
db    ? ;
word_2F018 dw ?
word_2F01A dw ?
word_2F01C dw ?
byte_2F01E db ?
align 2
word_2F020 dw ?
word_2F022 dw ?
word_2F024 dw ?
align 10h
dseg ends


; Segment type: Uninitialized
seg003 segment byte stack 'STACK' use16
assume cs:seg003
assume es:nothing, ss:nothing, ds:dseg, fs:nothing, gs:nothing
db 800h dup(?)
seg003 ends


end start
