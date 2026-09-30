; ===========================================================================

; Segment type: Pure code
ovl_2PLAY       segment byte public 'CODE' use16
                assume cs:ovl_2PLAY
                ;org 7E10h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_17E10       proc near               ; CODE XREF: seg002:01C5↑J
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

loc_17E26:                              ; CODE XREF: sub_17E10+468↓j
                call    thk_res_3804
                or      ax, ax
                jz      short loc_17E30
                call    thk_res_3FD8

loc_17E30:                              ; CODE XREF: sub_17E10+1B↑j
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
                call    thk_res_13B6
                add     sp, 0Ch
                cmp     byte_1DC1E, 1
                jnz     short loc_17E82
                call    sub_1B862
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
                call    thk_res_13B6
                add     sp, 0Ch

loc_17E82:                              ; CODE XREF: sub_17E10+50↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     al, byte_1DC1F
                sub     ah, ah
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2

loc_17EAF:                              ; CODE XREF: sub_17E10+25↑j
                                        ; sub_17E10+2C↑j
                cmp     byte_23218, 80h
                jb      short loc_17EB9
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17EB9:                              ; CODE XREF: sub_17E10+A4↑j
                mov     al, byte_231DF
                mov     [bp+var_4], al
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_2], al
                cmp     word_238A0, 0
                jnz     short loc_17EDD
                cmp     byte_1DD59, 0
                jz      short loc_17EE1

loc_17EDD:                              ; CODE XREF: sub_17E10+C4↑j
                mov     [bp+var_2], 0

loc_17EE1:                              ; CODE XREF: sub_17E10+CB↑j
                cmp     [bp+var_2], 1
                jnz     short loc_17F32
                cmp     byte_1DBED, 1
                jnz     short loc_17F36
                cmp     byte_1EF2A, 0Ah
                jnz     short loc_17F36
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte ptr word_1DBE2+1
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
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_4], al
                mov     [bp+var_2], 2
                jmp     short loc_17F36
; ---------------------------------------------------------------------------

loc_17F32:                              ; CODE XREF: sub_17E10+D5↑j
                                        ; sub_17E10+109↑j
                mov     [bp+var_2], 0

loc_17F36:                              ; CODE XREF: sub_17E10+DC↑j
                                        ; sub_17E10+E3↑j ...
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

loc_17F76:                              ; CODE XREF: sub_17E10+175↓j
                mov     bx, cx
                sub     bh, bh
                mov     al, [bp+var_4]
                mov     [bx-6980h], al
                inc     cl

loc_17F83:                              ; CODE XREF: sub_17E10+164↑j
                cmp     cl, dl
                jb      short loc_17F76
                mov     [bp+var_8], cl

loc_17F8A:                              ; CODE XREF: sub_17E10+152↑j
                call    thk_res_3EB2
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17F90:                              ; CODE XREF: sub_17E10+12A↑j
                cmp     byte_1DBED, 1
                jz      short loc_17F9A
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17F9A:                              ; CODE XREF: sub_17E10+185↑j
                cmp     byte_1EF2A, 0Ah
                jnz     short loc_17FA4
                jmp     loc_1802B
; ---------------------------------------------------------------------------

loc_17FA4:                              ; CODE XREF: sub_17E10+18F↑j
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte ptr word_1DBE2+1
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
                call    thk_res_1C88
                add     sp, 4
                cmp     ax, 0Ah
                jge     short loc_1802B
                mov     ax, 14E4h
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_17FF8:                              ; CODE XREF: sub_17E10+1ED↓j
                call    thk_res_1A30
                or      ax, ax
                jz      short loc_17FF8
                mov     al, byte_231E4
                and     al, 0Fh
                mov     byte ptr word_1DBE2+1, al
                mov     al, byte_231E4
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr word_1DBE4, al
                call    sub_1B75E
                call    thk_res_4076
                jmp     short loc_1802B
; ---------------------------------------------------------------------------
                align 2

loc_1801C:                              ; CODE XREF: sub_17E10+220↓j
                cmp     byte_1DC7E, 0
                jz      short loc_18032
                mov     byte_1DC7E, 0
                call    loc_1A8C4

loc_1802B:                              ; CODE XREF: sub_17E10+A6↑j
                                        ; sub_17E10+17D↑j ...
                cmp     byte_23218, 80h
                jnb     short loc_1801C

loc_18032:                              ; CODE XREF: sub_17E10+211↑j
                cmp     byte_1DC84, 0FEh
                jnz     short loc_18040
                inc     byte_1DC84
                call    thk_res_3814

loc_18040:                              ; CODE XREF: sub_17E10+227↑j
                mov     [bp+var_2], 0
                mov     byte ptr word_1DBE4+1, 0
                mov     word_238A0, 0
                call    thk_res_37CA
                or      ax, ax
                jnz     short loc_18059
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18059:                              ; CODE XREF: sub_17E10+244↑j
                mov     byte_1DBEB, 0
                mov     si, [bp+var_C]

loc_18061:                              ; CODE XREF: sub_17E10+26D↓j
                cmp     byte_1DBED, 0
                jnz     short loc_18076
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    sub_18282
                jmp     short loc_18079
; ---------------------------------------------------------------------------

loc_18076:                              ; CODE XREF: sub_17E10+256↑j
                call    thk_res_1A30

loc_18079:                              ; CODE XREF: sub_17E10+264↑j
                mov     si, ax
                or      si, si
                jz      short loc_18061
                mov     [bp+var_C], si
                push    si
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_C], ax
                cmp     byte_1DC80, 0
                jz      short loc_18096
                call    sub_1A580

loc_18096:                              ; CODE XREF: sub_17E10+281↑j
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, [bp+var_C]
                cmp     ax, 4Fh ; 'O'
                jnz     short loc_180AB
                jmp     loc_18190
; ---------------------------------------------------------------------------

loc_180AB:                              ; CODE XREF: sub_17E10+296↑j
                jle     short loc_180B0
                jmp     loc_181F4
; ---------------------------------------------------------------------------

loc_180B0:                              ; CODE XREF: sub_17E10:loc_180AB↑j
                cmp     ax, 11h
                jz      short loc_180E0
                cmp     ax, 42h ; 'B'
                jnz     short loc_180BD
                jmp     loc_18166
; ---------------------------------------------------------------------------

loc_180BD:                              ; CODE XREF: sub_17E10+2A8↑j
                cmp     ax, 43h ; 'C'
                jnz     short loc_180C5
                jmp     loc_1816C
; ---------------------------------------------------------------------------

loc_180C5:                              ; CODE XREF: sub_17E10+2B0↑j
                cmp     ax, 44h ; 'D'
                jnz     short loc_180CD
                jmp     loc_18176
; ---------------------------------------------------------------------------

loc_180CD:                              ; CODE XREF: sub_17E10+2B8↑j
                cmp     ax, 45h ; 'E'
                jnz     short loc_180D5
                jmp     loc_1817C
; ---------------------------------------------------------------------------

loc_180D5:                              ; CODE XREF: sub_17E10+2C0↑j
                cmp     ax, 4Dh ; 'M'
                jnz     short loc_180DD
                jmp     loc_18186
; ---------------------------------------------------------------------------

loc_180DD:                              ; CODE XREF: sub_17E10+2C8↑j
                jmp     loc_18205
; ---------------------------------------------------------------------------

loc_180E0:                              ; CODE XREF: sub_17E10+2A3↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    thk_res_5440
                mov     ax, 14F1h
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_180F6:                              ; CODE XREF: sub_17E10+311↓j
                mov     ax, 79h ; 'y'
                push    ax
                mov     ax, 4Eh ; 'N'
                push    ax
                call    thk_res_2E6E
                add     sp, 4
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_18117
                mov     si, 4Eh ; 'N'

loc_18117:                              ; CODE XREF: sub_17E10+302↑j
                mov     ax, si
                cmp     ax, 59h ; 'Y'
                jz      short loc_18123
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_180F6

loc_18123:                              ; CODE XREF: sub_17E10+30C↑j
                mov     [bp+var_C], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_1812E
                call    thk_res_3FC4

loc_1812E:                              ; CODE XREF: sub_17E10+319↑j
                call    thk_res_35A8
                call    thk_res_421E
                mov     byte_1DBEA, 0
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1813C:                              ; CODE XREF: sub_17E10+43A↓j
                call    thk_res_3FE2
                push    [bp+var_C]
                call    thk_res_43F2
                add     sp, 2
                mov     byte_1DC7E, 1
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18150:                              ; CODE XREF: sub_17E10+447↓j
                push    [bp+var_C]
                call    thk_res_43D6
                add     sp, 2
                mov     ax, 1
                push    ax
                call    thk_res_50CE

loc_18160:                              ; CODE XREF: sub_17E10+39D↓j
                                        ; sub_17E10+3C1↓j
                add     sp, 2
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_18166:                              ; CODE XREF: sub_17E10+2AA↑j
                call    thk_2MISC_C130
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1816C:                              ; CODE XREF: sub_17E10+2B2↑j
                call    thk_res_3FE2
                call    thk_2MISC2_C3F6
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18176:                              ; CODE XREF: sub_17E10+2BA↑j
                call    thk_2MISC2_C130
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1817C:                              ; CODE XREF: sub_17E10+2C2↑j
                call    thk_res_3FE2
                call    thk_2MISC2_C2F8
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18186:                              ; CODE XREF: sub_17E10+2CA↑j
                call    thk_res_3FE2
                call    sub_1BB4E
                jmp     loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_18190:                              ; CODE XREF: sub_17E10+298↑j
                cmp     byte_1DC1E, 1
                jz      short loc_1819A
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_1819A:                              ; CODE XREF: sub_17E10+385↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    thk_res_471E

loc_181A6:                              ; CODE XREF: sub_17E10+3B6↓j
                                        ; sub_17E10+41E↓j
                mov     ax, 1
                push    ax
                call    thk_res_1392
                jmp     short loc_18160
; ---------------------------------------------------------------------------
                align 2

loc_181B0:                              ; CODE XREF: sub_17E10+3EE↓j
                cmp     byte_1DC1E, 0
                jz      short loc_181BA
                jmp     loc_1825C
; ---------------------------------------------------------------------------

loc_181BA:                              ; CODE XREF: sub_17E10+3A5↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    thk_res_47D8
                jmp     short loc_181A6
; ---------------------------------------------------------------------------

loc_181C8:                              ; CODE XREF: sub_17E10+3F3↓j
                call    thk_res_3FE2
                push    [bp+var_C]
                call    thk_res_5B8A
                jmp     short loc_18160
; ---------------------------------------------------------------------------
                align 2

loc_181D4:                              ; CODE XREF: sub_17E10+3E7↓j
                call    thk_2MISC_CF84
                call    thk_res_40E6
                mov     byte_1DC7E, 1
                mov     byte_1DBEA, 1
                jmp     short loc_1825C
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_181E8:                              ; CODE XREF: sub_17E10+42C↓j
                call    thk_res_3814
                jmp     short loc_1825C
; ---------------------------------------------------------------------------
                align 2

loc_181EE:                              ; CODE XREF: sub_17E10+425↓j
                call    thk_2MISC_C242
                jmp     short loc_1825C ; CODE XREF: seg002:01B9↑J
; ---------------------------------------------------------------------------
                align 2

loc_181F4:                              ; CODE XREF: sub_17E10+29D↑j
                cmp     ax, 52h ; 'R'
                jz      short loc_181D4
                jg      short loc_18232
                cmp     ax, 50h ; 'P'
                jz      short loc_181B0
                cmp     ax, 51h ; 'Q'
                jz      short loc_181C8

loc_18205:                              ; CODE XREF: sub_17E10:loc_180DD↑j
                                        ; sub_17E10+42E↓j ...
                call    thk_res_3FE2
                cmp     [bp+var_C], 31h ; '1'
                jb      short loc_1825C
                mov     ax, word_1DC76
                add     ax, 30h ; '0'
                cmp     [bp+var_C], ax
                ja      short loc_1825C
                push    [bp+var_C]
                call    thk_res_5B8A
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    thk_res_4A34
                jmp     loc_181A6
; ---------------------------------------------------------------------------
                align 2

loc_18232:                              ; CODE XREF: sub_17E10+3E9↑j
                cmp     ax, 55h ; 'U'
                jz      short loc_181EE
                jg      short loc_18240
                cmp     ax, 53h ; 'S'
                jz      short loc_181E8
                jmp     short loc_18205
; ---------------------------------------------------------------------------

loc_18240:                              ; CODE XREF: sub_17E10+427↑j
                cmp     ax, 0F0h
                jl      short loc_18205
                cmp     ax, 0F1h
                jg      short loc_1824D
                jmp     loc_1813C
; ---------------------------------------------------------------------------

loc_1824D:                              ; CODE XREF: sub_17E10+438↑j
                cmp     ax, 0F2h
                jl      short loc_18205
                cmp     ax, 0F3h
                jg      short loc_1825A
                jmp     loc_18150
; ---------------------------------------------------------------------------

loc_1825A:                              ; CODE XREF: sub_17E10+445↑j
                jmp     short loc_18205
; ---------------------------------------------------------------------------

loc_1825C:                              ; CODE XREF: sub_17E10+246↑j
                                        ; sub_17E10+329↑j ...
                call    thk_res_37CA
                or      ax, ax
                jz      short loc_18272
                cmp     word_238A0, 0
                jz      short loc_18272
                mov     byte_1DC7E, 1
                call    thk_res_3FE2

loc_18272:                              ; CODE XREF: sub_17E10+451↑j
                                        ; sub_17E10+458↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1827B
                jmp     loc_17E26
; ---------------------------------------------------------------------------

loc_1827B:                              ; CODE XREF: sub_17E10+466↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
sub_17E10       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18282       proc near               ; CODE XREF: seg002:0609↑J
                                        ; sub_17E10+261↑p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    word_1ED84
                inc     word_1ED84
                call    sub_182B2
                add     sp, 2
                cmp     word_1ED84, 4
                jnz     short loc_182A3
                mov     word_1ED84, 1

loc_182A3:                              ; CODE XREF: sub_18282+19↑j
                mov     ax, 8
                push    ax
                call    thk_res_4EFE
                mov     [bp+var_2], ax
                mov     sp, bp
                pop     bp
                retn
sub_18282       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_182B2       proc near               ; CODE XREF: sub_18282+E↑p
                                        ; sub_18744+26C↓p

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

loc_182D8:                              ; CODE XREF: sub_182B2+54↓j
                push    word ptr [si]
                push    word ptr [di]
                mov     bx, [bp+var_8]
                mov     ax, [bx]
                add     ax, [bp+arg_0]
                push    ax
                push    word_1DBB8
                push    word_1DBB6
                call    thk_res_14FE
                add     sp, 0Ah
                add     si, 2
                add     di, 2
                add     [bp+var_8], 2
                inc     [bp+var_2]
                mov     ax, word_1DBF0
                cmp     [bp+var_2], ax
                jle     short loc_182D8

loc_18308:                              ; CODE XREF: sub_182B2+D↑j
                                        ; sub_182B2+19↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_182B2       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1830E       proc near               ; CODE XREF: sub_18558+1D↓p

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

loc_18335:                              ; CODE XREF: sub_1830E+1E↑j
                cmp     [bp+arg_0], 0
                jnz     short loc_1833F
                sub     [bp+var_2], 8

loc_1833F:                              ; CODE XREF: sub_1830E+25↑j
                                        ; sub_1830E+2B↑j
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
sub_1830E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1837A       proc near               ; CODE XREF: sub_185B4+2F↓p

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

loc_1839E:                              ; CODE XREF: sub_1837A+1B↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_183A8
                add     [bp+var_2], 8

loc_183A8:                              ; CODE XREF: sub_1837A+22↑j
                                        ; sub_1837A+28↑j
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
sub_1837A       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_183E0       proc near               ; CODE XREF: sub_1867C+2F↓p

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

loc_18438:                              ; CODE XREF: sub_183E0+4A↑j
                                        ; sub_183E0+51↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_183E0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1843E       proc near               ; CODE XREF: sub_185B4+3A↓p

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

loc_18489:                              ; CODE XREF: sub_1843E+C↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_184A1
                cmp     byte_1DBEC, 0
                jnz     short loc_184A1
                mov     bx, word_1DBF0
                shl     bx, 1
                sub     word ptr [bx+603Ch], 2

loc_184A1:                              ; CODE XREF: sub_1843E+4F↑j
                                        ; sub_1843E+56↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1843E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_184A8       proc near               ; CODE XREF: sub_1867C+3A↓p

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

loc_184FF:                              ; CODE XREF: sub_184A8+4F↑j
                mov     bx, word_1DBF0
                shl     bx, 1
                add     word ptr [bx+6028h], 8

loc_1850A:                              ; CODE XREF: sub_184A8+C↑j
                                        ; sub_184A8+55↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_184A8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18510       proc near               ; CODE XREF: seg002:0771↑J
                                        ; sub_18744+2F↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                cmp     byte ptr word_1DBE2+1, 7
                jbe     short loc_18524
                mov     si, 1
                jmp     short loc_18526
; ---------------------------------------------------------------------------
                align 2

loc_18524:                              ; CODE XREF: sub_18510+C↑j
                sub     si, si

loc_18526:                              ; CODE XREF: sub_18510+11↑j
                mov     bl, byte ptr word_1DBE4
                sub     bh, bh
                shl     bx, 1
                mov     al, [bx+si+59A6h]
                mov     [bp+var_2], al
                mov     bl, byte ptr word_1DBE2+1
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

loc_18550:                              ; CODE XREF: sub_18510+39↑j
                sub     ax, ax

loc_18552:                              ; CODE XREF: sub_18510+3E↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_18510       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18558       proc near               ; CODE XREF: sub_18744+10A↓p
                                        ; sub_18744+17D↓p ...

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
                call    sub_1830E
                add     sp, 2

loc_1857B:                              ; CODE XREF: sub_18558+B↑j
                                        ; sub_18558+11↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_18588
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_1858B
; ---------------------------------------------------------------------------

loc_18588:                              ; CODE XREF: sub_18558+27↑j
                mov     al, [bp+arg_2]

loc_1858B:                              ; CODE XREF: sub_18558+2E↑j
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
                call    thk_res_14FE
                add     sp, 0Ah
                pop     si
                pop     bp
                retn
sub_18558       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_185B4       proc near               ; CODE XREF: sub_18744+AE↓p
                                        ; sub_18744+C5↓p ...

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
                call    sub_1837A
                jmp     short loc_185F1
; ---------------------------------------------------------------------------

loc_185E8:                              ; CODE XREF: sub_185B4+27↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    sub_1843E

loc_185F1:                              ; CODE XREF: sub_185B4+32↑j
                add     sp, 2

loc_185F4:                              ; CODE XREF: sub_185B4+19↑j
                                        ; sub_185B4+1F↑j
                cmp     [bp+var_2], 80h
                jnb     short loc_1862A
                cmp     [bp+arg_0], 2
                jnz     short loc_18608
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_1860B
; ---------------------------------------------------------------------------
                align 2

loc_18608:                              ; CODE XREF: sub_185B4+4A↑j
                mov     al, [bp+arg_2]

loc_1860B:                              ; CODE XREF: sub_185B4+51↑j
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

loc_1862A:                              ; CODE XREF: sub_185B4+44↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_18636
                mov     [bp+arg_0], 10h
                jmp     short loc_1863A
; ---------------------------------------------------------------------------

loc_18636:                              ; CODE XREF: sub_185B4+7A↑j
                mov     [bp+arg_0], 0

loc_1863A:                              ; CODE XREF: sub_185B4+80↑j
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

loc_1865A:                              ; CODE XREF: sub_185B4+73↑j
                mov     [bp+var_8], ax
                push    ax
                push    [bp+var_6]
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    word_1DBD0
                push    word_1DBCE
                call    thk_res_14FE
                add     sp, 0Ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_185B4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1867C       proc near               ; CODE XREF: sub_18744+DC↓p
                                        ; sub_18744+F3↓p ...

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
                call    sub_183E0
                jmp     short loc_186B9
; ---------------------------------------------------------------------------

loc_186B0:                              ; CODE XREF: sub_1867C+27↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                push    ax
                call    sub_184A8

loc_186B9:                              ; CODE XREF: sub_1867C+32↑j
                add     sp, 2

loc_186BC:                              ; CODE XREF: sub_1867C+19↑j
                                        ; sub_1867C+1F↑j
                cmp     [bp+var_2], 80h
                jnb     short loc_186F2
                cmp     [bp+arg_0], 2
                jnz     short loc_186D0
                mov     al, [bp+arg_2]
                add     al, 10h
                jmp     short loc_186D3
; ---------------------------------------------------------------------------
                align 2

loc_186D0:                              ; CODE XREF: sub_1867C+4A↑j
                mov     al, [bp+arg_2]

loc_186D3:                              ; CODE XREF: sub_1867C+51↑j
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

loc_186F2:                              ; CODE XREF: sub_1867C+44↑j
                cmp     [bp+arg_0], 2
                jnz     short loc_186FE
                mov     [bp+arg_0], 10h
                jmp     short loc_18702
; ---------------------------------------------------------------------------

loc_186FE:                              ; CODE XREF: sub_1867C+7A↑j
                mov     [bp+arg_0], 0

loc_18702:                              ; CODE XREF: sub_1867C+80↑j
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

loc_18722:                              ; CODE XREF: sub_1867C+73↑j
                mov     [bp+var_8], ax
                push    ax
                push    [bp+var_6]
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    word_1DBD0
                push    word_1DBCE
                call    thk_res_14FE
                add     sp, 0Ah
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1867C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18744       proc near               ; CODE XREF: seg002:0765↑J

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     word_1DBF0, 0FFFFh
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, 44h ; 'D'
                push    ax
                mov     ax, 8
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBB4
                push    word_1DBB2
                call    thk_res_14FE
                add     sp, 0Ah
                call    sub_18510
                mov     [bp+var_2], ax
                mov     ax, 8
                push    ax
                push    ax
                push    [bp+var_2]
                push    word_1DBD4
                push    word_1DBD2
                call    thk_res_14FE
                add     sp, 0Ah
                cmp     word_1DC1C, 80h
                jl      short loc_187A0
                cmp     [bp+var_2], 0
                jnz     short loc_187A0
                call    thk_res_4FB2

loc_187A0:                              ; CODE XREF: sub_18744+51↑j
                                        ; sub_18744+57↑j
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
                call    sub_185B4
                add     sp, 4

loc_187F8:                              ; CODE XREF: sub_18744+A2↑j
                cmp     byte_27836, 0
                jz      short loc_1880F
                mov     ax, 84h
                push    ax
                mov     al, byte_27836
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_1880F:                              ; CODE XREF: sub_18744+B9↑j
                cmp     byte_2782A, 0
                jz      short loc_18826
                mov     ax, 4
                push    ax
                mov     al, byte_2782A
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_18826:                              ; CODE XREF: sub_18744+D0↑j
                cmp     byte_27832, 0
                jz      short loc_1883D
                mov     ax, 84h
                push    ax
                mov     al, byte_27832
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_1883D:                              ; CODE XREF: sub_18744+E7↑j
                cmp     byte_2782E, 0
                jz      short loc_18854
                mov     ax, 4
                push    ax
                mov     al, byte_2782E
                sub     ah, ah
                push    ax
                call    sub_18558
                add     sp, 4

loc_18854:                              ; CODE XREF: sub_18744+FE↑j
                cmp     byte_27839, 0
                jz      short loc_1886B
                mov     ax, 3
                push    ax
                mov     al, byte_27839
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_1886B:                              ; CODE XREF: sub_18744+115↑j
                cmp     byte_27835, 0
                jz      short loc_18882
                mov     ax, 83h
                push    ax
                mov     al, byte_27835
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_18882:                              ; CODE XREF: sub_18744+12C↑j
                cmp     byte_27829, 0
                jz      short loc_18899
                mov     ax, 3
                push    ax
                mov     al, byte_27829
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_18899:                              ; CODE XREF: sub_18744+143↑j
                cmp     byte_27831, 0
                jz      short loc_188B0
                mov     ax, 83h
                push    ax
                mov     al, byte_27831
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_188B0:                              ; CODE XREF: sub_18744+15A↑j
                cmp     byte_2782D, 0
                jz      short loc_188C7
                mov     ax, 3
                push    ax
                mov     al, byte_2782D
                sub     ah, ah
                push    ax
                call    sub_18558
                add     sp, 4

loc_188C7:                              ; CODE XREF: sub_18744+171↑j
                cmp     byte_27838, 0
                jz      short loc_188DE
                mov     ax, 2
                push    ax
                mov     al, byte_27838
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_188DE:                              ; CODE XREF: sub_18744+188↑j
                cmp     byte_27834, 0
                jz      short loc_188F5
                mov     ax, 82h
                push    ax
                mov     al, byte_27834
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_188F5:                              ; CODE XREF: sub_18744+19F↑j
                cmp     byte_27828, 0
                jz      short loc_1890C
                mov     ax, 2
                push    ax
                mov     al, byte_27828
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_1890C:                              ; CODE XREF: sub_18744+1B6↑j
                cmp     byte_27830, 0
                jz      short loc_18923
                mov     ax, 82h
                push    ax
                mov     al, byte_27830
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_18923:                              ; CODE XREF: sub_18744+1CD↑j
                cmp     byte_2782C, 0
                jz      short loc_1893A
                mov     ax, 2
                push    ax
                mov     al, byte_2782C
                sub     ah, ah
                push    ax
                call    sub_18558
                add     sp, 4

loc_1893A:                              ; CODE XREF: sub_18744+1E4↑j
                cmp     byte_27837, 0
                jz      short loc_18951
                mov     ax, 1
                push    ax
                mov     al, byte_27837
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_18951:                              ; CODE XREF: sub_18744+1FB↑j
                cmp     byte_27833, 0
                jz      short loc_18968
                mov     ax, 81h
                push    ax
                mov     al, byte_27833
                sub     ah, ah
                push    ax
                call    sub_185B4
                add     sp, 4

loc_18968:                              ; CODE XREF: sub_18744+212↑j
                cmp     byte_27827, 0
                jz      short loc_1897F
                mov     ax, 1
                push    ax
                mov     al, byte_27827
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_1897F:                              ; CODE XREF: sub_18744+229↑j
                cmp     byte_2782F, 0
                jz      short loc_18996
                mov     ax, 81h
                push    ax
                mov     al, byte_2782F
                sub     ah, ah
                push    ax
                call    sub_1867C
                add     sp, 4

loc_18996:                              ; CODE XREF: sub_18744+240↑j
                cmp     byte_2782B, 0
                jz      short loc_189AD
                mov     ax, 1
                push    ax
                mov     al, byte_2782B
                sub     ah, ah
                push    ax
                call    sub_18558
                add     sp, 4

loc_189AD:                              ; CODE XREF: sub_18744+257↑j
                sub     ax, ax
                push    ax
                call    sub_182B2
                mov     sp, bp
                pop     bp
                retn
sub_18744       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_189B8       proc near               ; CODE XREF: sub_18D6C:loc_18DB6↓p

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
                call    thk_res_14FE
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
                call    thk_res_14FE
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
                call    thk_res_14FE
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
                call    thk_res_14FE
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
                call    thk_res_14FE
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
                call    thk_res_14FE
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
                call    thk_res_14FE
                add     sp, 0Ah

loc_18CC1:                              ; CODE XREF: sub_18BEC+46↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_18BEC       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18CC6       proc near               ; CODE XREF: sub_18D6C+4D↓p

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


sub_18D6C       proc near               ; CODE XREF: seg002:0789↑J
                call    thk_res_5F54
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, 44h ; 'D'   ; CODE XREF: seg002:04A1↑J
                push    ax
                mov     ax, 8
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBB4
                push    word_1DBB2
                call    thk_res_14FE
                add     sp, 0Ah
                cmp     word_1DC1C, 80h
                jl      short loc_18DA0
                call    thk_res_4FB2
                jmp     short loc_18DB6
; ---------------------------------------------------------------------------
                align 2

loc_18DA0:                              ; CODE XREF: sub_18D6C+2C↑j
                mov     ax, 8
                push    ax
                push    ax
                sub     ax, ax
                push    ax
                push    word_1DBD4
                push    word_1DBD2
                call    thk_res_14FE
                add     sp, 0Ah

loc_18DB6:                              ; CODE XREF: sub_18D6C+31↑j
                call    sub_189B8
                call    sub_18CC6
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                retn
sub_18D6C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_18DC8       proc near               ; CODE XREF: sub_18DD8+6↓p
                                        ; sub_18DD8+E↓p ...
                mov     bx, word_1DC7A
                inc     word_1DC7A
                mov     al, [bx+6052h]
                sub     ah, ah
                retn
sub_18DC8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18DD8       proc near               ; CODE XREF: sub_18DF8+6↓p
                                        ; sub_1A01E+6↓p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                sub     ah, ah
                mov     [bp+var_2], ax
                call    sub_18DC8
                mov     ch, al
                sub     cl, cl
                add     [bp+var_2], cx
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_18DD8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18DF8       proc near               ; CODE XREF: sub_19E40+6B↓p
                                        ; sub_1A1A0+7↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_18DD8
                mov     [bp+var_4], ax
                mov     [bp+var_2], 0
                call    sub_18DC8
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
sub_18DF8       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18E22       proc near               ; CODE XREF: ovl_2PLAY:9B26↓p
                                        ; sub_19C1A+18↓p

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

loc_18E3B:                              ; CODE XREF: sub_18E22+7A↓j
                                        ; sub_18E22+90↓j ...
                mov     [bp+var_2], ax
                jmp     loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E42:                              ; CODE XREF: sub_18E22+F↑j
                cmp     [bp+arg_0], 23h ; '#'
                jnz     short loc_18E50
                mov     [bp+var_2], 3D8h
                jmp     loc_18EDF
; ---------------------------------------------------------------------------

loc_18E50:                              ; CODE XREF: sub_18E22+24↑j
                cmp     [bp+arg_0], 2Bh ; '+'
                jnz     short loc_18E5E
                mov     [bp+var_2], 3E0h
                jmp     loc_18EDF
; ---------------------------------------------------------------------------

loc_18E5E:                              ; CODE XREF: sub_18E22+32↑j
                cmp     [bp+arg_0], 2Ch ; ','
                jnz     short loc_18E6C
                mov     [bp+var_2], 3E1h
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E6C:                              ; CODE XREF: sub_18E22+40↑j
                cmp     [bp+arg_0], 32h ; '2'
                jnz     short loc_18E7A
                mov     [bp+var_2], 3EAh
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E7A:                              ; CODE XREF: sub_18E22+4E↑j
                cmp     [bp+arg_0], 33h ; '3'
                jnz     short loc_18E88
                mov     [bp+var_2], 3F1h
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18E88:                              ; CODE XREF: sub_18E22+5C↑j
                cmp     [bp+arg_0], 27h ; '''
                jb      short loc_18E9E
                cmp     [bp+arg_0], 2Ah ; '*'
                ja      short loc_18E9E
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3B5h
                jmp     short loc_18E3B
; ---------------------------------------------------------------------------

loc_18E9E:                              ; CODE XREF: sub_18E22+6A↑j
                                        ; sub_18E22+70↑j
                cmp     [bp+arg_0], 3Bh ; ';'
                jb      short loc_18EB4
                cmp     [bp+arg_0], 3Eh ; '>'
                ja      short loc_18EB4
                mov     al, [bp+arg_0]
                sub     ah, ah
                add     ax, 3B7h
                jmp     short loc_18E3B
; ---------------------------------------------------------------------------

loc_18EB4:                              ; CODE XREF: sub_18E22+80↑j
                                        ; sub_18E22+86↑j
                cmp     [bp+arg_0], 84h
                jnz     short loc_18EC2
                mov     [bp+var_2], 3CAh
                jmp     short loc_18EDF
; ---------------------------------------------------------------------------
                align 2

loc_18EC2:                              ; CODE XREF: sub_18E22+96↑j
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

loc_18EDA:                              ; CODE XREF: sub_18E22+A4↑j
                                        ; sub_18E22+AA↑j
                or      byte_1DC80, 1

loc_18EDF:                              ; CODE XREF: sub_18E22+1C↑j
                                        ; sub_18E22+2B↑j ...
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_18E22       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18EE6       proc near               ; CODE XREF: sub_1947E+18↓p

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

loc_18EF8:                              ; CODE XREF: sub_18EE6+A↑j
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

loc_18F0E:                              ; CODE XREF: sub_18EE6+22↑j
                                        ; DATA XREF: sub_18EE6:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 0
                sub     bh, bh
                mov     al, [bx+164Ch]

loc_18F17:                              ; CODE XREF: sub_18EE6+3F↓j
                                        ; sub_18EE6+4B↓j ...
                mov     [bp+var_2], al
                jmp     short def_18F08 ; jumptable 00018F08 default case
; ---------------------------------------------------------------------------

loc_18F1C:                              ; CODE XREF: sub_18EE6+22↑j
                                        ; DATA XREF: sub_18EE6:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 3
                sub     bh, bh
                mov     al, [bx+1662h]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F28:                              ; CODE XREF: sub_18EE6+22↑j
                                        ; DATA XREF: sub_18EE6:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 case 1
                sub     bh, bh
                mov     al, [bx+167Ch]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F34:                              ; CODE XREF: sub_18EE6+22↑j
                                        ; DATA XREF: sub_18EE6:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 cases 4,6
                sub     bh, bh
                mov     al, [bx+1694h]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2

loc_18F40:                              ; CODE XREF: sub_18EE6+22↑j
                                        ; DATA XREF: sub_18EE6:jpt_18F08↓o
                mov     bl, [bp+arg_0]  ; jumptable 00018F08 cases 2,5
                sub     bh, bh
                mov     al, [bx+16ACh]
                jmp     short loc_18F17
; ---------------------------------------------------------------------------
                align 2
jpt_18F08       dw offset loc_18F0E     ; DATA XREF: sub_18EE6+22↑r
                dw offset loc_18F28     ; jump table for switch statement
                dw offset loc_18F40
                dw offset loc_18F1C
                dw offset loc_18F34
                dw offset loc_18F40
                dw offset loc_18F34
; ---------------------------------------------------------------------------

def_18F08:                              ; CODE XREF: sub_18EE6+1D↑j
                                        ; sub_18EE6+34↑j
                mov     al, [bp+var_2]  ; jumptable 00018F08 default case
                sub     ah, ah

loc_18F5F:                              ; CODE XREF: sub_18EE6+F↑j
                mov     sp, bp
                pop     bp
                retn
sub_18EE6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18F64       proc near               ; CODE XREF: sub_198D2+16↓p
                                        ; sub_198F2+16↓p ...

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     si, word_1DC7A
                mov     cl, [bp+arg_0]
                jmp     short loc_18F7E
; ---------------------------------------------------------------------------
                align 2

loc_18F72:                              ; CODE XREF: sub_18F64+20↓j
                mov     bl, [si+6052h]
                sub     bh, bh
                shl     bx, 1
                add     si, [bx+15E6h]

loc_18F7E:                              ; CODE XREF: sub_18F64+B↑j
                mov     al, cl
                dec     cl
                or      al, al
                jnz     short loc_18F72
                mov     word_1DC7A, si
                mov     [bp+arg_0], cl
                pop     si
                pop     bp
                retn
sub_18F64       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18F90       proc near               ; CODE XREF: sub_19160+56↓p

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

loc_18F9E:                              ; CODE XREF: sub_18F90+2F↓j
                inc     cl
                inc     dl
                mov     bx, cx
                sub     bh, bh
                cmp     byte ptr [bx+54D0h], 0Ah
                jz      short loc_18FB2
                cmp     dl, 14h
                jnz     short loc_18FB7

loc_18FB2:                              ; CODE XREF: sub_18F90+1B↑j
                inc     [bp+var_2]

loc_18FB5:                              ; CODE XREF: sub_18F90+C↑j
                sub     dl, dl

loc_18FB7:                              ; CODE XREF: sub_18F90+20↑j
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
sub_18F90       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_18FD0       proc near               ; CODE XREF: sub_1905E+5↓p
                                        ; sub_19074+11↓p ...

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                call    sub_18DC8
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

loc_18FFB:                              ; CODE XREF: sub_18FD0+39↓j
                mov     si, di

loc_18FFD:                              ; CODE XREF: sub_18FD0+35↓j
                mov     bx, si
                inc     si
                cmp     byte ptr [bx+6052h], 0FFh
                jnz     short loc_18FFD
                mov     di, si
                loop    loc_18FFB
                mov     word_22D10, di

loc_1900F:                              ; CODE XREF: sub_18FD0+1C↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_18FD0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19016       proc near               ; CODE XREF: sub_1905E+8↓p
                                        ; sub_19074+14↓p ...

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     di, di
                mov     si, word_22D10

loc_19024:                              ; CODE XREF: sub_19016+38↓j
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

loc_19040:                              ; CODE XREF: sub_19016+18↑j
                mov     [bp+var_2], 0

loc_19044:                              ; CODE XREF: sub_19016+22↑j
                                        ; sub_19016+28↑j
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
sub_19016       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1905E       proc near               ; CODE XREF: sub_1A606:loc_1A67C↓p
                or      byte_1DC80, 1
                call    sub_18FD0
                call    sub_19016
                mov     ax, 54D0h
                push    ax
                call    thk_res_410A
                add     sp, 2
                retn
sub_1905E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19074       proc near               ; CODE XREF: sub_190F2+16↓p
                                        ; sub_1A606+80↓p

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
                call    sub_18FD0
                call    sub_19016
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
                call    thk_res_3292
                add     sp, 8
                mov     ax, [bp+var_A]
                mov     [bp+var_8], ax

loc_190AE:                              ; CODE XREF: sub_19074+70↓j
                mov     ax, di
                add     ax, [bp+var_8]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4

loc_190BE:                              ; CODE XREF: sub_19074+69↓j
                mov     al, [si+54D0h]
                mov     [bp+var_2], al
                inc     si
                or      al, al
                jz      short loc_190D3
                sub     ah, ah
                push    ax
                call    thk_res_0D22
                add     sp, 2

loc_190D3:                              ; CODE XREF: sub_19074+54↑j
                cmp     [bp+var_2], 0Ah
                jz      short loc_190DF
                cmp     [bp+var_2], 0
                jnz     short loc_190BE

loc_190DF:                              ; CODE XREF: sub_19074+63↑j
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
sub_19074       endp


; =============== S U B R O U T I N E =======================================


sub_190F2       proc near               ; CODE XREF: sub_1A606:loc_1A690↓p
                or      byte_1DC80, 3
                call    thk_res_34BA
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 11h
                push    ax
                call    sub_19074
                add     sp, 2
                retn
sub_190F2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_19110       proc near               ; CODE XREF: sub_1A606:loc_1A696↓p
                push    di
                call    sub_18FD0
                call    sub_19016
                cmp     byte_1DBEE, 0
                jnz     short loc_1915E
                mov     ax, 0FFh
                push    ax
                call    thk_res_1610
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
                call    thk_res_1676
                add     sp, 4
                mov     ax, 54D0h
                push    ax
                call    thk_res_1726
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_1610
                add     sp, 2

loc_1915E:                              ; CODE XREF: sub_19110+C↑j
                pop     di
                retn
sub_19110       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19160       proc near               ; CODE XREF: sub_1A606:loc_1A69C↓p

var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_18FD0
                call    sub_19016
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
                call    thk_res_0B0E
                add     sp, 8
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     byte ptr [bx+8], 80h
                push    ax
                call    thk_res_0F8A
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_15CA
                add     sp, 2
                mov     ax, 0FFh
                push    ax
                call    thk_res_1610
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_res_165C
                add     sp, 2
                call    sub_18F90
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
                call    thk_res_1676
                add     sp, 4

loc_191D5:                              ; CODE XREF: sub_19160+62↑j
                mov     ax, 54D0h
                push    ax
                call    thk_res_1726
                add     sp, 2
                push    [bp+var_2]
                call    thk_res_0FF2
                add     sp, 2

loc_191E8:                              ; CODE XREF: sub_19160+11↑j
                mov     sp, bp
                pop     bp
                retn
sub_19160       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_191EC       proc near               ; CODE XREF: sub_1A606:loc_1A6A2↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     [bp+var_6], 0
                call    sub_18FD0
                call    sub_19016
                mov     si, 54D0h

loc_19201:                              ; CODE XREF: sub_191EC+21↓j
                cmp     byte ptr [si], 2Dh ; '-'
                jnz     short loc_19209
                mov     byte ptr [si], 7Bh ; '{'

loc_19209:                              ; CODE XREF: sub_191EC+18↑j
                inc     si
                cmp     byte ptr [si], 0
                jnz     short loc_19201
                mov     [bp+var_2], si
                cmp     byte_1DBEE, 0
                jz      short loc_1921C
                jmp     loc_193B2
; ---------------------------------------------------------------------------

loc_1921C:                              ; CODE XREF: sub_191EC+2B↑j
                mov     ax, 9
                push    ax
                mov     ax, 12h
                push    ax
                mov     ax, 8
                push    ax
                push    ax
                call    thk_res_0B0E
                add     sp, 8
                mov     [bp+var_4], ax
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 7
                push    ax
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 10h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     [bp+var_6], 0Bh
                mov     si, 0Bh

loc_1925B:                              ; CODE XREF: sub_191EC+7A↓j
                mov     ax, 0Eh
                push    ax
                call    thk_res_0D22
                add     sp, 2
                dec     si
                jnz     short loc_1925B
                mov     ax, 11h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 8
                push    ax
                mov     ax, 7
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 14h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                push    word_1EF20
                call    thk_res_1726
                add     sp, 2
                mov     ax, 15h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 9
                push    ax
                mov     ax, 7
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 14h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                push    word_1EF20
                call    thk_res_1726
                add     sp, 2
                mov     ax, 15h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 0Ah
                push    ax
                mov     ax, 7
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 12h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     [bp+var_6], 0Bh
                mov     si, 0Bh

loc_192EA:                              ; CODE XREF: sub_191EC+109↓j
                mov     ax, 0Fh
                push    ax
                call    thk_res_0D22
                add     sp, 2
                dec     si
                jnz     short loc_192EA
                mov     ax, 13h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 0FFh
                push    ax
                call    thk_res_1610
                add     sp, 2
                sub     si, si

loc_1930D:                              ; CODE XREF: sub_191EC+151↓j
                lea     ax, [si+0Bh]
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 15h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 7Eh ; '~'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 14h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                inc     si
                cmp     si, 3
                jl      short loc_1930D
                mov     [bp+var_6], si
                mov     ax, 0Eh
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 13h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 7Eh ; '~'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 12h
                push    ax
                call    thk_res_0D22
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_1610
                add     sp, 2
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                push    [bp+var_4]
                call    thk_res_0F8A
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_15CA
                add     sp, 2
                mov     ax, 2
                push    ax
                call    thk_res_165C
                add     sp, 2
                mov     ax, 54D0h
                push    ax
                call    thk_res_1726
                add     sp, 2
                push    [bp+var_4]
                call    thk_res_0FF2
                add     sp, 2

loc_193B2:                              ; CODE XREF: sub_191EC+2D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_191EC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_193B8       proc near               ; CODE XREF: sub_1940E+9↓p
                                        ; sub_1A606+A5↓p

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    thk_res_5426
                mov     si, [bp+var_2]

loc_193C5:                              ; CODE XREF: sub_193B8+2B↓j
                                        ; sub_193B8+30↓j ...
                cmp     [bp+arg_0], 0
                jz      short loc_193D0
                call    thk_res_56C6
                jmp     short loc_193DF
; ---------------------------------------------------------------------------

loc_193D0:                              ; CODE XREF: sub_193B8+11↑j
                cmp     byte_1DBED, 0
                jnz     short loc_193DC
                call    sub_18282
                jmp     short loc_193DF
; ---------------------------------------------------------------------------

loc_193DC:                              ; CODE XREF: sub_193B8+1D↑j
                call    thk_res_1A30

loc_193DF:                              ; CODE XREF: sub_193B8+16↑j
                                        ; sub_193B8+22↑j
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
                call    thk_res_35A8
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_193B8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1940E       proc near               ; CODE XREF: sub_1A606:loc_1A6B0↓p
                mov     byte_2294F, 0FDh
                mov     ax, 1
                push    ax
                call    sub_193B8
                add     sp, 2
                retn
sub_1940E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1941E       proc near               ; CODE XREF: seg002:07B9↑J
                                        ; sub_1946E+9↓p ...

var_2           = word ptr -2
arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     byte_1DC7F, 0
                mov     si, [bp+var_2]

loc_1942D:                              ; CODE XREF: sub_1941E+3C↓j
                cmp     [bp+arg_0], 0
                jz      short loc_19438
                call    thk_res_56C6
                jmp     short loc_19447
; ---------------------------------------------------------------------------

loc_19438:                              ; CODE XREF: sub_1941E+13↑j
                cmp     byte_1DBED, 0
                jnz     short loc_19444
                call    sub_18282
                jmp     short loc_19447
; ---------------------------------------------------------------------------

loc_19444:                              ; CODE XREF: sub_1941E+1F↑j
                call    thk_res_1A30

loc_19447:                              ; CODE XREF: sub_1941E+18↑j
                                        ; sub_1941E+24↑j
                mov     si, ax
                push    si
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 59h ; 'Y'
                jz      short loc_1945C
                cmp     ax, 4Eh ; 'N'
                jnz     short loc_1942D

loc_1945C:                              ; CODE XREF: sub_1941E+37↑j
                mov     [bp+var_2], si
                cmp     si, 59h ; 'Y'
                jnz     short loc_19468
                inc     byte_1DC7F

loc_19468:                              ; CODE XREF: sub_1941E+44↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1941E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1946E       proc near               ; CODE XREF: seg002:07C5↑J
                                        ; sub_1A606+B8↓p
                mov     byte_2294F, 0FDh
                mov     ax, 1
                push    ax
                call    sub_1941E
                add     sp, 2
                retn
sub_1946E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1947E       proc near               ; CODE XREF: sub_1A606+BE↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_18DC8
                mov     [bp+var_2], al
                call    sub_18DC8
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_18EE6
                add     sp, 2
                mov     byte_277D4, al
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_res_2734
                add     sp, 6
                mov     al, byte_277D4
                sub     ah, ah
                push    ax
                call    thk_res_264A
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                call    thk_res_2734
                or      byte_1DC80, 4
                mov     sp, bp
                pop     bp
                retn
sub_1947E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_194D4       proc near               ; CODE XREF: sub_1A606+C4↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_18DC8
                mov     [bp+var_2], al
                call    sub_18DC8
                mov     [bp+var_4], al
                test    [bp+var_2], 40h
                jz      short loc_1950B
                mov     ax, 14h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                add     al, 5
                mov     [bp+var_2], al
                cmp     al, 11h
                jb      short loc_19507
                add     [bp+var_2], 10h

loc_19507:                              ; CODE XREF: sub_194D4+2D↑j
                or      [bp+var_2], 80h

loc_1950B:                              ; CODE XREF: sub_194D4+16↑j
                cmp     [bp+var_2], 80h
                jb      short loc_19522
                mov     ax, 0FFh
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_4], al

loc_19522:                              ; CODE XREF: sub_194D4+3B↑j
                mov     al, [bp+var_4]
                and     al, 0Fh
                mov     byte ptr word_1DBE2+1, al
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr word_1DBE4, al
                and     [bp+var_2], 3Fh
                sub     ah, ah
                push    ax
                mov     al, byte ptr word_1DBE2+1
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    sub_1B5EA
                add     sp, 6
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    sub_198C8
                mov     byte_1DC7E, 1
                mov     sp, bp
                pop     bp
                retn
sub_194D4       endp


; =============== S U B R O U T I N E =======================================


sub_19560       proc near               ; CODE XREF: sub_1A606+CA↓p
                call    sub_18DC8
                sub     ah, ah
                push    ax
                call    thk_res_57E0
                add     sp, 2
                retn
sub_19560       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1956E       proc near               ; CODE XREF: sub_19716+142↓p

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
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_19598
                mov     [bp+var_2], 3Ch ; '<'
                sub     [bp+arg_0], 8
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19598:                              ; CODE XREF: sub_1956E+1C↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 37h ; '7'
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_195BC
                mov     [bp+var_2], 3Dh ; '='
                sub     [bp+arg_0], 10h
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_195BC:                              ; CODE XREF: sub_1956E+40↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 4Bh ; 'K'
                push    ax
                mov     ax, 38h ; '8'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_195E0
                mov     [bp+var_2], 3Eh ; '>'
                sub     [bp+arg_0], 37h ; '7'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_195E0:                              ; CODE XREF: sub_1956E+64↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 54h ; 'T'
                push    ax
                mov     ax, 4Ch ; 'L'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_19604
                mov     [bp+var_2], 3Fh ; '?'
                sub     [bp+arg_0], 4Bh ; 'K'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19604:                              ; CODE XREF: sub_1956E+88↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 5Bh ; '['
                push    ax
                mov     ax, 56h ; 'V'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_19628
                mov     [bp+var_2], 40h ; '@'
                sub     [bp+arg_0], 55h ; 'U'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19628:                              ; CODE XREF: sub_1956E+AC↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 5Eh ; '^'
                push    ax
                mov     ax, 5Ch ; '\'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_1964C
                mov     [bp+var_2], 41h ; 'A'
                sub     [bp+arg_0], 5Bh ; '['
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_1964C:                              ; CODE XREF: sub_1956E+D0↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 69h ; 'i'
                push    ax
                mov     ax, 65h ; 'e'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_19670
                mov     [bp+var_2], 42h ; 'B'
                sub     [bp+arg_0], 64h ; 'd'
                jmp     loc_196F6
; ---------------------------------------------------------------------------
                align 2

loc_19670:                              ; CODE XREF: sub_1956E+F4↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 7Ch ; '|'
                push    ax
                mov     ax, 6Ah ; 'j'
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_19692
                mov     [bp+var_2], 43h ; 'C'
                sub     [bp+arg_0], 69h ; 'i'
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_19692:                              ; CODE XREF: sub_1956E+118↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 98h
                push    ax
                mov     ax, 97h
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_196B4
                mov     [bp+var_2], 44h ; 'D'
                sub     [bp+arg_0], 96h
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_196B4:                              ; CODE XREF: sub_1956E+13A↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 0F3h
                push    ax
                mov     ax, 0E3h
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_196D6
                mov     [bp+var_2], 45h ; 'E'
                sub     [bp+arg_0], 0E2h
                jmp     short loc_196F6
; ---------------------------------------------------------------------------

loc_196D6:                              ; CODE XREF: sub_1956E+15C↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                push    ax
                mov     ax, 0FBh
                push    ax
                mov     ax, 0F4h
                push    ax
                call    thk_res_4C2E
                add     sp, 6
                or      ax, ax
                jz      short loc_196F6
                mov     [bp+var_2], 46h ; 'F'
                sub     [bp+arg_0], 0F3h

loc_196F6:                              ; CODE XREF: sub_1956E+26↑j
                                        ; sub_1956E+4A↑j ...
                mov     al, byte ptr word_1DBE2
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                mov     byte ptr word_1DBE2, al
                call    thk_res_6008
                mov     al, [bp+var_4]
                mov     byte ptr word_1DBE2, al
                mov     al, [bp+arg_0]
                mov     byte_22D13, al
                mov     sp, bp
                pop     bp
                retn
sub_1956E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19716       proc near               ; CODE XREF: sub_1A606+D0↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     byte ptr word_1DBE4+1, 1
                call    sub_18DC8
                mov     [bp+var_2], al
                mov     word_1DC8A, 2
                sub     ah, ah
                cmp     ax, 80h
                jz      short loc_197A2
                jbe     short loc_19739
                jmp     loc_1985E
; ---------------------------------------------------------------------------

loc_19739:                              ; CODE XREF: sub_19716+1E↑j
                cmp     ax, 5
                jz      short loc_19778
                jbe     short loc_19743
                jmp     loc_19822
; ---------------------------------------------------------------------------

loc_19743:                              ; CODE XREF: sub_19716+28↑j
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

loc_1975A:                              ; CODE XREF: sub_19716+30↑j
                mov     word_1DC8A, 1
                call    thk_1RETINN_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19766:                              ; CODE XREF: sub_19716+35↑j
                call    thk_2MISC2_CE30
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1976C:                              ; CODE XREF: sub_19716+3A↑j
                call    thk_2BRAIN_D15A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19772:                              ; CODE XREF: sub_19716+3F↑j
                call    thk_2TEMPLE_CA88
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19778:                              ; CODE XREF: sub_19716+26↑j
                call    thk_2TEMPLE_CB9C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1977E:                              ; CODE XREF: sub_19716+111↓j
                call    thk_2SMITH_CCBA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19784:                              ; CODE XREF: sub_19716+119↓j
                call    thk_2BRAIN_C7E2
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1978A:                              ; CODE XREF: sub_19716+121↓j
                call    thk_2BRAIN_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19790:                              ; CODE XREF: sub_19716+129↓j
                call    thk_2CAVES_C99A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19796:                              ; CODE XREF: sub_19716+131↓j
                call    thk_2CAVES_C130
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1979C:                              ; CODE XREF: sub_19716+139↓j
                call    thk_2CAVES_C1DA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197A2:                              ; CODE XREF: sub_19716+1C↑j
                call    thk_2CAVES_C23C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197A8:                              ; CODE XREF: sub_19716+157↓j
                sub     ax, ax

loc_197AA:                              ; CODE XREF: sub_19716+A1↓j
                                        ; sub_19716+A7↓j
                push    ax
                call    thk_2CAVES_C308

loc_197AE:                              ; CODE XREF: sub_19716+145↓j
                add     sp, 2
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197B4:                              ; CODE XREF: sub_19716+15F↓j
                mov     ax, 1
                jmp     short loc_197AA
; ---------------------------------------------------------------------------
                align 2

loc_197BA:                              ; CODE XREF: sub_19716+167↓j
                mov     ax, 2
                jmp     short loc_197AA
; ---------------------------------------------------------------------------
                align 2

loc_197C0:                              ; CODE XREF: sub_19716+16F↓j
                call    thk_2CAVES_D5DE
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197C6:                              ; CODE XREF: sub_19716+177↓j
                call    thk_2CAVES_D5E8
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197CC:                              ; CODE XREF: sub_19716+14D↓j
                call    thk_2CAVES_C462
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197D2:                              ; CODE XREF: sub_19716+181↓j
                call    thk_2CAVES_C52C
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197D8:                              ; CODE XREF: sub_19716+189↓j
                call    thk_2CAVES_C5AC
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197DE:                              ; CODE XREF: sub_19716+191↓j
                call    thk_2CAVES_C66E
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197E4:                              ; CODE XREF: sub_19716+199↓j
                call    thk_2CAVES_C73A
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197EA:                              ; CODE XREF: sub_19716+1A1↓j
                call    thk_2CAVES_D5F4
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_197F0:                              ; CODE XREF: sub_19716+1A9↓j
                call    thk_2SMITH_CEC8
                cmp     byte ptr word_1DBE4+1, 2
                jnz     short loc_1980C
                mov     al, byte_1DC24
                mov     byte ptr word_1DBE2, al
                mov     word_1DC8A, 1
                call    thk_1RETINN_C1EA
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_1980C:                              ; CODE XREF: sub_19716+E2↑j
                cmp     byte ptr word_1DBE4+1, 3
                jnz     short loc_1981A
                call    thk_res_3FD8
                jmp     loc_198C4
; ---------------------------------------------------------------------------
                align 2

loc_1981A:                              ; CODE XREF: sub_19716+FB↑j
                mov     byte ptr word_1DBE4+1, 1
                jmp     loc_198C4
; ---------------------------------------------------------------------------

loc_19822:                              ; CODE XREF: sub_19716+2A↑j
                cmp     ax, 6
                jnz     short loc_1982A
                jmp     loc_1977E
; ---------------------------------------------------------------------------

loc_1982A:                              ; CODE XREF: sub_19716+10F↑j
                cmp     ax, 7
                jnz     short loc_19832
                jmp     loc_19784
; ---------------------------------------------------------------------------

loc_19832:                              ; CODE XREF: sub_19716+117↑j
                cmp     ax, 8
                jnz     short loc_1983A
                jmp     loc_1978A
; ---------------------------------------------------------------------------

loc_1983A:                              ; CODE XREF: sub_19716+11F↑j
                cmp     ax, 64h ; 'd'
                jnz     short loc_19842
                jmp     loc_19790
; ---------------------------------------------------------------------------

loc_19842:                              ; CODE XREF: sub_19716+127↑j
                cmp     ax, 7Eh ; '~'
                jnz     short loc_1984A
                jmp     loc_19796
; ---------------------------------------------------------------------------

loc_1984A:                              ; CODE XREF: sub_19716+12F↑j
                cmp     ax, 7Fh
                jnz     short loc_19852
                jmp     loc_1979C
; ---------------------------------------------------------------------------

loc_19852:                              ; CODE XREF: sub_19716+41↑j
                                        ; sub_19716+137↑j ...
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_1956E
                jmp     loc_197AE
; ---------------------------------------------------------------------------

loc_1985E:                              ; CODE XREF: sub_19716+20↑j
                cmp     ax, 0CBh
                jnz     short loc_19866
                jmp     loc_197CC
; ---------------------------------------------------------------------------

loc_19866:                              ; CODE XREF: sub_19716+14B↑j
                ja      short loc_19892
                cmp     ax, 81h
                jnz     short loc_19870
                jmp     loc_197A8
; ---------------------------------------------------------------------------

loc_19870:                              ; CODE XREF: sub_19716+155↑j
                cmp     ax, 82h
                jnz     short loc_19878
                jmp     loc_197B4
; ---------------------------------------------------------------------------

loc_19878:                              ; CODE XREF: sub_19716+15D↑j
                cmp     ax, 83h
                jnz     short loc_19880
                jmp     loc_197BA
; ---------------------------------------------------------------------------

loc_19880:                              ; CODE XREF: sub_19716+165↑j
                cmp     ax, 0C9h
                jnz     short loc_19888
                jmp     loc_197C0
; ---------------------------------------------------------------------------

loc_19888:                              ; CODE XREF: sub_19716+16D↑j
                cmp     ax, 0CAh
                jnz     short loc_19890
                jmp     loc_197C6
; ---------------------------------------------------------------------------

loc_19890:                              ; CODE XREF: sub_19716+175↑j
                jmp     short loc_19852
; ---------------------------------------------------------------------------

loc_19892:                              ; CODE XREF: sub_19716:loc_19866↑j
                cmp     ax, 0CCh
                jnz     short loc_1989A
                jmp     loc_197D2
; ---------------------------------------------------------------------------

loc_1989A:                              ; CODE XREF: sub_19716+17F↑j
                cmp     ax, 0CDh
                jnz     short loc_198A2
                jmp     loc_197D8
; ---------------------------------------------------------------------------

loc_198A2:                              ; CODE XREF: sub_19716+187↑j
                cmp     ax, 0CEh
                jnz     short loc_198AA
                jmp     loc_197DE
; ---------------------------------------------------------------------------

loc_198AA:                              ; CODE XREF: sub_19716+18F↑j
                cmp     ax, 0CFh
                jnz     short loc_198B2
                jmp     loc_197E4
; ---------------------------------------------------------------------------

loc_198B2:                              ; CODE XREF: sub_19716+197↑j
                cmp     ax, 0E2h
                jnz     short loc_198BA
                jmp     loc_197EA
; ---------------------------------------------------------------------------

loc_198BA:                              ; CODE XREF: sub_19716+19F↑j
                cmp     ax, 0FDh
                jnz     short loc_198C2
                jmp     loc_197F0
; ---------------------------------------------------------------------------

loc_198C2:                              ; CODE XREF: sub_19716+1A7↑j
                jmp     short loc_19852
; ---------------------------------------------------------------------------

loc_198C4:                              ; CODE XREF: sub_19716+4D↑j
                                        ; sub_19716+53↑j ...
                mov     sp, bp
                pop     bp
                retn
sub_19716       endp


; =============== S U B R O U T I N E =======================================


sub_198C8       proc near               ; CODE XREF: sub_194D4+80↑p
                                        ; sub_1A606+D6↓p
                mov     byte ptr word_1DBE4+1, 1
                call    sub_1A580
                retn
sub_198C8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_198D2       proc near               ; CODE XREF: sub_1A606+DC↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     byte_1DC7F, 0
                jz      short loc_198EE
                sub     ah, ah
                push    ax
                call    sub_18F64
                add     sp, 2

loc_198EE:                              ; CODE XREF: sub_198D2+11↑j
                mov     sp, bp
                pop     bp
                retn
sub_198D2       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_198F2       proc near

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     byte_1DC7F, 0
                jnz     short loc_1990E
                sub     ah, ah
                push    ax
                call    sub_18F64
                add     sp, 2

loc_1990E:                              ; CODE XREF: sub_198F2+11↑j
                mov     sp, bp
                pop     bp
                retn
sub_198F2       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19912       proc near               ; CODE XREF: ovl_2PLAY:9988↓p

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

loc_1992F:                              ; CODE XREF: sub_19912+16↑j
                sub     si, si

loc_19931:                              ; CODE XREF: sub_19912+2A↓j
                call    sub_18DC8
                mov     [si-6980h], al
                inc     si
                cmp     si, 0Ah
                jl      short loc_19931
                mov     [bp+var_2], si
                cmp     byte_1DC65, 80h
                jnz     short loc_19954
                call    sub_18DC8
                mov     byte_26EDA, al
                call    sub_18DC8
                jmp     short loc_19959
; ---------------------------------------------------------------------------
                align 2

loc_19954:                              ; CODE XREF: sub_19912+34↑j
                sub     al, al
                mov     byte_26EDA, al

loc_19959:                              ; CODE XREF: sub_19912+3F↑j
                mov     byte_1DD58, al
                call    thk_res_3EB2
                mov     byte ptr word_1DBE4+1, 1
                call    thk_res_37CA
                or      ax, ax
                jz      short loc_1997E
                cmp     byte_1DD59, 0
                jnz     short loc_19979
                cmp     word_238A0, 0
                jz      short loc_1997E

loc_19979:                              ; CODE XREF: sub_19912+5E↑j
                mov     byte_1DC7E, 1

loc_1997E:                              ; CODE XREF: sub_19912+57↑j
                                        ; sub_19912+65↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_19912       endp

; ---------------------------------------------------------------------------
                align 2
                mov     ax, 1
                push    ax
                call    sub_19912
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2
                push    si
                mov     byte ptr word_1DBE4+1, 1
                call    sub_1A580
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte ptr word_1DBE2+1
                sub     bh, bh
                and     byte ptr [bx+si+5AD6h], 7Fh
                and     byte_23218, 7Fh
                pop     si
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_199B8       proc near               ; CODE XREF: sub_19A02+76↓p

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
                call    thk_res_37B6
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

sub_19A02       proc near               ; CODE XREF: ovl_2PLAY:9B3C↓p

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
                call    sub_18DC8
                mov     [bp+var_2], al
                call    sub_18DC8
                mov     byte_22D0C, al
                call    sub_18DC8
                mov     byte_22D0F, al
                cmp     [bp+arg_0], 0
                jz      short loc_19A38
                call    sub_18DC8
                mov     byte_22D0D, al

loc_19A38:                              ; CODE XREF: sub_19A02+2E↑j
                cmp     [bp+var_2], 80h
                jb      short loc_19A44
                mov     al, byte_22D12
                mov     byte_22D0D, al

loc_19A44:                              ; CODE XREF: sub_19A02+3A↑j
                and     [bp+var_2], 7Fh
                jz      short loc_19A5D
                cmp     [bp+var_2], 9
                jz      short loc_19A5D
                mov     al, [bp+var_2]
                cmp     byte ptr word_1DC76, al
                jnb     short loc_19A5D
                mov     [bp+var_2], 1

loc_19A5D:                              ; CODE XREF: sub_19A02+46↑j
                                        ; sub_19A02+4C↑j ...
                cmp     [bp+var_2], 0
                jnz     short loc_19A6C
                mov     al, byte ptr word_1DC76
                mov     [bp+var_4], al
                mov     [bp+var_2], al

loc_19A6C:                              ; CODE XREF: sub_19A02+5F↑j
                                        ; sub_19A02+B4↓j
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

loc_19A9E:                              ; CODE XREF: sub_19A02+83↑j
                cmp     byte_22D0F, 0
                jz      short loc_19AAB
                mov     al, byte_22D0F
                and     [bp+var_6], al

loc_19AAB:                              ; CODE XREF: sub_19A02+A1↑j
                mov     al, [bp+var_6]
                or      byte_1DC7F, al

loc_19AB2:                              ; CODE XREF: sub_19A02+9A↑j
                cmp     [bp+var_4], 0
                jnz     short loc_19A6C
                mov     sp, bp
                pop     bp
                retn
sub_19A02       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19ABC       proc near

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
                call    sub_18DC8
                mov     [bp+var_8], al
                call    sub_18DC8
                mov     [bp+var_8], al
                sub     di, di
                jmp     short loc_19ADB
; ---------------------------------------------------------------------------
                align 2

loc_19ADA:                              ; CODE XREF: sub_19ABC+59↓j
                inc     di

loc_19ADB:                              ; CODE XREF: sub_19ABC+1B↑j
                cmp     di, word_1DC76
                jge     short loc_19B17
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_2], ax
                mov     [bp+var_6], 0
                mov     si, ax
                mov     dl, [bp+var_8]
                sub     cx, cx

loc_19AF7:                              ; CODE XREF: sub_19ABC+4F↓j
                mov     bx, cx
                cmp     [bx+si+3Ah], dl
                jz      short loc_19B03
                cmp     [bx+si+28h], dl
                jnz     short loc_19B07

loc_19B03:                              ; CODE XREF: sub_19ABC+40↑j
                inc     byte_1DC7F

loc_19B07:                              ; CODE XREF: sub_19ABC+45↑j
                inc     cx
                cmp     cx, 6
                jl      short loc_19AF7
                mov     [bp+var_6], cx
                cmp     byte_1DC7F, 0
                jz      short loc_19ADA

loc_19B17:                              ; CODE XREF: sub_19ABC+23↑j
                mov     [bp+var_4], di
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_19ABC       endp

; ---------------------------------------------------------------------------
                call    sub_18DC8
                sub     ah, ah
                push    ax
                call    sub_18E22
                add     sp, 2
                mov     bx, ax
                mov     al, [bx]
                mov     byte_1DC7F, al
                call    sub_18DC8
                retn
; ---------------------------------------------------------------------------
                align 2
                mov     ax, 1
                push    ax
                call    sub_19A02
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19B44       proc near

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
                call    sub_18DC8
                mov     [bp+var_6], al
                call    sub_18DC8
                mov     [bp+var_E], al
                call    sub_18DC8
                mov     [bp+var_C], al
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     [bp+var_6], 80h
                jb      short loc_19B70
                mov     al, byte_1DC7F
                mov     [bp+var_E], al

loc_19B70:                              ; CODE XREF: sub_19B44+24↑j
                mov     byte_1DC7F, 0
                sub     di, di
                jmp     short loc_19B8B
; ---------------------------------------------------------------------------
                align 2

loc_19B7A:                              ; CODE XREF: sub_19B44+69↓j
                cmp     byte_1DC7F, 0
                jnz     short loc_19BCB
                inc     cx
                cmp     cx, 6
                jge     short loc_19BCB
                jmp     short loc_19BA7
; ---------------------------------------------------------------------------
                align 2

loc_19B8A:                              ; CODE XREF: sub_19B44+8F↓j
                inc     di

loc_19B8B:                              ; CODE XREF: sub_19B44+33↑j
                cmp     di, word_1DC76
                jge     short loc_19BD5
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_4], ax
                mov     [bp+var_A], 0
                mov     si, ax
                mov     dx, [bp+var_10]
                sub     cx, cx

loc_19BA7:                              ; CODE XREF: sub_19B44+43↑j
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

loc_19BCB:                              ; CODE XREF: sub_19B44+3B↑j
                                        ; sub_19B44+41↑j
                mov     [bp+var_A], cx
                cmp     byte_1DC7F, 0
                jz      short loc_19B8A

loc_19BD5:                              ; CODE XREF: sub_19B44+4B↑j
                mov     [bp+var_8], di
                cmp     byte_1DC7F, 0
                jnz     short loc_19C13
                sub     si, si

loc_19BE1:                              ; CODE XREF: sub_19B44+B0↓j
                cmp     byte ptr [si+6950h], 0
                jnz     short loc_19BEE

loc_19BE8:                              ; CODE XREF: sub_19B44+AE↓j
                mov     [bp+var_8], si
                jmp     short loc_19BF6
; ---------------------------------------------------------------------------
                align 2

loc_19BEE:                              ; CODE XREF: sub_19B44+A2↑j
                inc     si
                cmp     si, 2
                jge     short loc_19BE8
                jmp     short loc_19BE1
; ---------------------------------------------------------------------------

loc_19BF6:                              ; CODE XREF: sub_19B44+A7↑j
                mov     bx, [bp+var_8]
                mov     al, [bp+var_E]
                mov     [bx+6950h], al
                mov     al, [bp+var_2]
                mov     [bx+6953h], al
                mov     al, [bp+var_C]
                mov     [bx+6956h], al
                mov     byte_1DC84, 0FFh

loc_19C13:                              ; CODE XREF: sub_19B44+99↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_19B44       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19C1A       proc near

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_18DC8
                mov     [bp+var_2], al
                call    sub_18DC8
                mov     [bp+var_4], al
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                call    sub_18E22
                mov     bx, ax
                mov     al, [bp+var_4]
                mov     [bx], al
                mov     sp, bp
                pop     bp
                retn
sub_19C1A       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19C40       proc near

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                mov     [bp+var_2], al
                mov     al, byte_1DC7F
                cmp     [bp+var_2], al
                jbe     short loc_19C59
                mov     byte_1DC7F, 0

loc_19C59:                              ; CODE XREF: sub_19C40+12↑j
                mov     sp, bp
                pop     bp
                retn
sub_19C40       endp

; ---------------------------------------------------------------------------
                align 2
                call    sub_18DC8
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     byte_1DC7F, al
                retn
; ---------------------------------------------------------------------------
                call    sub_18DC8
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                inc     ax
                push    ax
                call    thk_res_4EFE
                add     sp, 2
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19C8A       proc near

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                mov     [bp+var_2], al
                jmp     short loc_19CA9
; ---------------------------------------------------------------------------

loc_19C98:                              ; CODE XREF: sub_19C8A+27↓j
                mov     ax, 0Ah
                push    ax
                call    thk_res_1C66
                add     sp, 2
                call    thk_res_1A30
                or      ax, ax
                jnz     short loc_19CB3

loc_19CA9:                              ; CODE XREF: sub_19C8A+C↑j
                mov     al, [bp+var_2]
                dec     [bp+var_2]
                or      al, al
                jnz     short loc_19C98

loc_19CB3:                              ; CODE XREF: sub_19C8A+1D↑j
                mov     sp, bp
                pop     bp
                retn
sub_19C8A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19CB8       proc near               ; CODE XREF: sub_19E40+9D↓p
                                        ; sub_19E40+D7↓p

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
                call    thk_res_37B6
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

sub_19E40       proc near               ; CODE XREF: ovl_2PLAY:9F3C↓p

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
                call    sub_18DC8
                mov     [bp+var_6], al
                mov     [bp+var_4], al
                cmp     al, 80h
                jb      short loc_19E6F
                and     [bp+var_4], 7Fh
                and     [bp+var_6], 7Fh
                mov     al, byte_1DC7F
                mov     byte ptr [bp+var_A], al

loc_19E6F:                              ; CODE XREF: sub_19E40+1F↑j
                cmp     [bp+var_6], 9
                jnz     short loc_19E85
                mov     al, byte_22D0E
                mov     [bp+var_6], al
                or      al, al
                jnz     short loc_19E85
                mov     al, byte_1DC7F
                mov     [bp+var_6], al

loc_19E85:                              ; CODE XREF: sub_19E40+33↑j
                                        ; sub_19E40+3D↑j
                mov     byte_1DC7F, 1
                mov     al, [bp+var_6]
                cmp     byte ptr word_1DC76, al
                jnb     short loc_19E96
                jmp     loc_19F2C
; ---------------------------------------------------------------------------

loc_19E96:                              ; CODE XREF: sub_19E40+51↑j
                mov     byte_22D12, al
                call    sub_18DC8
                mov     byte_22D0C, al
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     byte ptr [bp+var_A], 0
                jnz     short loc_19EB6
                call    sub_18DF8
                mov     [bp+var_A], ax
                mov     [bp+var_8], dx
                jmp     short loc_19EBB
; ---------------------------------------------------------------------------

loc_19EB6:                              ; CODE XREF: sub_19E40+69↑j
                add     word_1DC7A, 3

loc_19EBB:                              ; CODE XREF: sub_19E40+74↑j
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

loc_19EE6:                              ; CODE XREF: sub_19E40+80↑j
                mov     [bp+var_C], 1
                cmp     byte ptr word_1DC76, 1
                jb      short loc_19F31
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_2]
                mov     di, ax
                mov     al, byte ptr [bp+var_A]
                mov     [bp+var_12], ax

loc_19F03:                              ; CODE XREF: sub_19E40+E7↓j
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
                cmp     byte ptr word_1DC76, al
                jnb     short loc_19F03
                jmp     short loc_19F31
; ---------------------------------------------------------------------------
                align 2

loc_19F2C:                              ; CODE XREF: sub_19E40+53↑j
                add     word_1DC7A, 5

loc_19F31:                              ; CODE XREF: sub_19E40+A3↑j
                                        ; sub_19E40+AF↑j ...
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_19E40       endp

; ---------------------------------------------------------------------------
                align 2
                mov     ax, 1
                push    ax
                call    sub_19E40
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19F44       proc near

var_8           = byte ptr -8
var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                call    sub_18DC8
                mov     [bp+var_6], al
                call    sub_18DC8
                mov     [bp+var_4], al
                call    sub_18DC8
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
sub_19F44       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19F90       proc near

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     al, byte ptr word_1DC1A
                mov     [bp+var_6], al
                call    sub_18DC8
                mov     [bp+var_4], al
                call    sub_18DC8
                mov     [bp+var_2], al
                mov     byte_1DC7F, 0
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                jb      short loc_19FC2
                mov     al, [bp+var_2]
                cmp     [bp+var_6], al
                ja      short loc_19FC2
                mov     byte_1DC7F, 1

loc_19FC2:                              ; CODE XREF: sub_19F90+23↑j
                                        ; sub_19F90+2B↑j
                mov     sp, bp
                pop     bp
                retn
sub_19F90       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_19FC6       proc near

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                mov     byte_1DC7F, 0
                mov     bx, word_1DC1A
                shl     bx, 1
                mov     al, [bx+3A2h]
                mov     [bp+var_6], al
                call    sub_18DC8
                mov     [bp+var_4], al
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     [bp+var_4], 0B5h
                jnz     short loc_19FF8
                test    [bp+var_6], 1
                jz      short loc_1A01A
                jmp     short loc_1A016
; ---------------------------------------------------------------------------

loc_19FF8:                              ; CODE XREF: sub_19FC6+28↑j
                cmp     [bp+var_4], 0B6h
                jnz     short loc_1A006
                test    [bp+var_6], 1
                jnz     short loc_1A01A
                jmp     short loc_1A016
; ---------------------------------------------------------------------------

loc_1A006:                              ; CODE XREF: sub_19FC6+36↑j
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                jb      short loc_1A01A
                mov     al, [bp+var_2]
                cmp     [bp+var_6], al
                ja      short loc_1A01A

loc_1A016:                              ; CODE XREF: sub_19FC6+30↑j
                                        ; sub_19FC6+3E↑j
                inc     byte_1DC7F

loc_1A01A:                              ; CODE XREF: sub_19FC6+2E↑j
                                        ; sub_19FC6+3C↑j ...
                mov     sp, bp
                pop     bp
                retn
sub_19FC6       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A01E       proc near

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DD8
                mov     [bp+var_2], ax
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk_res_5188
                add     sp, 4
                or      ax, ax
                jz      short loc_1A042
                mov     byte_1DC7F, 1
                jmp     short loc_1A047
; ---------------------------------------------------------------------------
                align 2

loc_1A042:                              ; CODE XREF: sub_1A01E+1A↑j
                mov     byte_1DC7F, 0

loc_1A047:                              ; CODE XREF: sub_1A01E+21↑j
                mov     sp, bp
                pop     bp
                retn
sub_1A01E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A04C       proc near

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    sub_18DC8
                sub     ah, ah
                mov     si, ax
                call    sub_18DC8
                mov     ch, al
                sub     cl, cl
                add     cx, si
                mov     [bp+var_2], cx
                push    cx
                call    thk_res_5262
                add     sp, 2
                or      ax, ax
                jz      short loc_1A078
                mov     byte_1DC7F, 1
                jmp     short loc_1A07D
; ---------------------------------------------------------------------------

loc_1A078:                              ; CODE XREF: sub_1A04C+23↑j
                mov     byte_1DC7F, 0

loc_1A07D:                              ; CODE XREF: sub_1A04C+2A↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1A04C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A082       proc near

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
                mov     ax, word_1DC76
                add     ax, 30h ; '0'
                mov     [bp+var_8], ax
                mov     di, [bp+var_2]
                mov     si, [bp+var_6]

loc_1A099:                              ; CODE XREF: sub_1A082+7B↓j
                cmp     [bp+arg_0], 0
                jz      short loc_1A0B2
                cmp     byte_1DBED, 0
                jnz     short loc_1A0AC
                call    sub_18282
                jmp     short loc_1A0B5
; ---------------------------------------------------------------------------
                align 2

loc_1A0AC:                              ; CODE XREF: sub_1A082+22↑j
                call    thk_res_1A30
                jmp     short loc_1A0B5
; ---------------------------------------------------------------------------
                align 2

loc_1A0B2:                              ; CODE XREF: sub_1A082+1B↑j
                call    thk_res_56C6

loc_1A0B5:                              ; CODE XREF: sub_1A082+27↑j
                                        ; sub_1A082+2D↑j
                mov     si, ax
                cmp     si, 1Bh
                jnz     short loc_1A0C2
                mov     ax, 1
                jmp     short loc_1A0C4
; ---------------------------------------------------------------------------
                align 2

loc_1A0C2:                              ; CODE XREF: sub_1A082+38↑j
                sub     ax, ax

loc_1A0C4:                              ; CODE XREF: sub_1A082+3D↑j
                mov     [bp+var_4], ax
                or      ax, ax
                jnz     short loc_1A0F9
                mov     ax, si
                sub     ax, 30h ; '0'
                mov     di, ax
                cmp     di, 1
                jl      short loc_1A0F4
                cmp     di, word_1DC76
                jg      short loc_1A0F4
                lea     ax, [di-1]
                push    ax
                call    thk_res_37B6
                add     sp, 2
                mov     bx, ax
                cmp     byte ptr [bx+26h], 81h
                jnb     short loc_1A0F4
                mov     ax, 1
                jmp     short loc_1A0F6
; ---------------------------------------------------------------------------

loc_1A0F4:                              ; CODE XREF: sub_1A082+53↑j
                                        ; sub_1A082+59↑j ...
                sub     ax, ax

loc_1A0F6:                              ; CODE XREF: sub_1A082+70↑j
                mov     [bp+var_4], ax

loc_1A0F9:                              ; CODE XREF: sub_1A082+47↑j
                cmp     [bp+var_4], 0
                jz      short loc_1A099
                mov     [bp+var_2], di
                mov     [bp+var_6], si
                cmp     si, 1Bh
                jnz     short loc_1A114
                mov     byte ptr word_1DBE4+1, 1
                call    sub_1A580
                jmp     short loc_1A120
; ---------------------------------------------------------------------------

loc_1A114:                              ; CODE XREF: sub_1A082+86↑j
                mov     al, byte ptr [bp+var_2]
                mov     byte_1DC7F, al
                mov     byte_22D0E, al
                mov     byte_22D12, al

loc_1A120:                              ; CODE XREF: sub_1A082+90↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1A082       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A126       proc near

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
                call    sub_18DC8
                mov     [bp+var_8], al
                call    sub_18DC8
                mov     [bp+var_8], al
                mov     [bp+var_4], 0
                jmp     short loc_1A158
; ---------------------------------------------------------------------------

loc_1A146:                              ; CODE XREF: sub_1A126+67↓j
                inc     si
                cmp     si, 6
                jge     short loc_1A18F
                jmp     short loc_1A170
; ---------------------------------------------------------------------------

loc_1A14E:                              ; CODE XREF: sub_1A126+6C↓j
                cmp     byte_1DC7F, 0
                jnz     short loc_1A194
                inc     [bp+var_4]

loc_1A158:                              ; CODE XREF: sub_1A126+1E↑j
                mov     ax, word_1DC76
                cmp     [bp+var_4], ax
                jge     short loc_1A194
                push    [bp+var_4]
                call    thk_res_37B6
                add     sp, 2
                mov     [bp+var_2], ax
                sub     si, si
                mov     di, ax

loc_1A170:                              ; CODE XREF: sub_1A126+26↑j
                mov     bx, si
                add     bx, di
                mov     al, [bp+var_8]
                cmp     [bx+3Ah], al
                jnz     short loc_1A188
                inc     byte_1DC7F
                push    si
                push    di
                call    thk_res_3766
                add     sp, 4

loc_1A188:                              ; CODE XREF: sub_1A126+54↑j
                cmp     byte_1DC7F, 0
                jz      short loc_1A146

loc_1A18F:                              ; CODE XREF: sub_1A126+24↑j
                mov     [bp+var_6], si
                jmp     short loc_1A14E
; ---------------------------------------------------------------------------

loc_1A194:                              ; CODE XREF: sub_1A126+2D↑j
                                        ; sub_1A126+38↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1A126       endp

; ---------------------------------------------------------------------------
                mov     byte ptr word_1DBE4+1, 1
                retn

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A1A0       proc near

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    sub_18DF8
                mov     word_241AC, ax
                mov     word_241AE, dx
                call    sub_18DD8
                mov     word_241AA, ax
                sub     si, si

loc_1A1B9:                              ; CODE XREF: sub_1A1A0+32↓j
                call    sub_18DC8
                mov     [si+6950h], al
                call    sub_18DC8
                mov     [si+6956h], al
                call    sub_18DC8
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
sub_1A1A0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A1E2       proc near

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                mov     [bp+var_2], al
                cmp     byte_1DD59, 0
                jz      short loc_1A1FE
                sub     ah, ah
                push    ax
                call    sub_18F64
                add     sp, 2

loc_1A1FE:                              ; CODE XREF: sub_1A1E2+11↑j
                mov     sp, bp
                pop     bp
                retn
sub_1A1E2       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A202       proc near

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_18DC8
                sub     ah, ah
                mov     [bp+var_2], ax
                add     word_1DC18, ax
                or      byte_1DC80, 1
                mov     sp, bp
                pop     bp
                retn
sub_1A202       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A21E       proc near

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
                call    sub_18DC8
                mov     byte ptr [bp+var_4], al
                call    sub_18DC8
                mov     byte ptr [bp+var_2], al
                test    byte ptr [bp+var_4], 80h
                jz      short loc_1A250
                inc     byte ptr [bp+var_E]

loc_1A250:                              ; CODE XREF: sub_1A21E+2D↑j
                test    byte ptr [bp+var_4], 40h
                jz      short loc_1A259
                inc     [bp+var_18]

loc_1A259:                              ; CODE XREF: sub_1A21E+36↑j
                cmp     [bp+var_18], 0
                jnz     short loc_1A265
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A268

loc_1A265:                              ; CODE XREF: sub_1A21E+3F↑j
                dec     byte ptr [bp+var_6]

loc_1A268:                              ; CODE XREF: sub_1A21E+45↑j
                test    byte ptr [bp+var_4], 20h
                jz      short loc_1A271
                inc     [bp+var_12]

loc_1A271:                              ; CODE XREF: sub_1A21E+4E↑j
                mov     al, byte ptr [bp+var_4]
                and     al, 0Fh
                mov     [bp+var_14], al
                mov     byte ptr [bp+var_10], al
                test    byte ptr [bp+var_4], 0E0h
                jnz     short loc_1A28A
                mov     al, byte ptr [bp+var_2]
                and     al, 0Fh
                mov     [bp+var_14], al

loc_1A28A:                              ; CODE XREF: sub_1A21E+62↑j
                mov     byte_1DC7F, 0
                mov     [bp+var_C], 0
                mov     di, [bp+var_A]
                mov     si, [bp+var_16]
                jmp     loc_1A349
; ---------------------------------------------------------------------------
                align 2

loc_1A29E:                              ; CODE XREF: sub_1A21E+14F↓j
                sub     ax, ax

loc_1A2A0:                              ; CODE XREF: sub_1A21E+155↓j
                mov     si, ax

loc_1A2A2:                              ; CODE XREF: sub_1A21E+144↓j
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

loc_1A2B6:                              ; CODE XREF: sub_1A21E+90↑j
                sub     ax, ax

loc_1A2B8:                              ; CODE XREF: sub_1A21E+95↑j
                mov     si, ax

loc_1A2BA:                              ; CODE XREF: sub_1A21E:loc_1A2A6↑j
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A2D2
                mov     al, byte ptr [bp+var_10]
                cmp     [di+0Eh], al
                jnz     short loc_1A2CE
                mov     ax, 1
                jmp     short loc_1A2D0
; ---------------------------------------------------------------------------
                align 2

loc_1A2CE:                              ; CODE XREF: sub_1A21E+A8↑j
                sub     ax, ax

loc_1A2D0:                              ; CODE XREF: sub_1A21E+AD↑j
                mov     si, ax

loc_1A2D2:                              ; CODE XREF: sub_1A21E+A0↑j
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

loc_1A2EA:                              ; CODE XREF: sub_1A21E+C4↑j
                sub     ax, ax

loc_1A2EC:                              ; CODE XREF: sub_1A21E+C9↑j
                mov     si, ax

loc_1A2EE:                              ; CODE XREF: sub_1A21E+BC↑j
                cmp     [bp+var_18], 0
                jz      short loc_1A306
                mov     al, [bp+var_14]
                cmp     [di+0Ch], al
                jnz     short loc_1A302
                mov     ax, 1
                jmp     short loc_1A304
; ---------------------------------------------------------------------------
                align 2

loc_1A302:                              ; CODE XREF: sub_1A21E+DC↑j
                sub     ax, ax

loc_1A304:                              ; CODE XREF: sub_1A21E+E1↑j
                mov     si, ax

loc_1A306:                              ; CODE XREF: sub_1A21E+D4↑j
                cmp     byte ptr [bp+var_E], 0
                jz      short loc_1A31E
                mov     al, [bp+var_14]
                cmp     [di+0Eh], al
                jnz     short loc_1A31A
                mov     ax, 1
                jmp     short loc_1A31C
; ---------------------------------------------------------------------------
                align 2

loc_1A31A:                              ; CODE XREF: sub_1A21E+F4↑j
                sub     ax, ax

loc_1A31C:                              ; CODE XREF: sub_1A21E+F9↑j
                mov     si, ax

loc_1A31E:                              ; CODE XREF: sub_1A21E+B6↑j
                                        ; sub_1A21E+EC↑j
                cmp     [bp+var_12], 0
                jnz     short loc_1A32B
                or      si, si
                jnz     short loc_1A32B
                inc     [bp+var_8]

loc_1A32B:                              ; CODE XREF: sub_1A21E+104↑j
                                        ; sub_1A21E+108↑j
                cmp     [bp+var_12], 0
                jz      short loc_1A338
                or      si, si
                jz      short loc_1A338
                inc     [bp+var_8]

loc_1A338:                              ; CODE XREF: sub_1A21E+111↑j
                                        ; sub_1A21E+115↑j
                cmp     [bp+var_8], 0
                jz      short loc_1A346

loc_1A33E:                              ; CODE XREF: sub_1A21E+131↓j
                mov     [bp+var_A], di
                mov     [bp+var_16], si
                jmp     short loc_1A376
; ---------------------------------------------------------------------------

loc_1A346:                              ; CODE XREF: sub_1A21E+11E↑j
                inc     [bp+var_C]

loc_1A349:                              ; CODE XREF: sub_1A21E+7C↑j
                mov     ax, word_1DC76
                cmp     [bp+var_C], ax
                jge     short loc_1A33E
                push    [bp+var_C]
                call    thk_res_37B6
                add     sp, 2
                mov     di, ax
                cmp     byte ptr [bp+var_6], 0
                jnz     short loc_1A365
                jmp     loc_1A2A2
; ---------------------------------------------------------------------------

loc_1A365:                              ; CODE XREF: sub_1A21E+142↑j
                mov     al, byte ptr [bp+var_10]
                cmp     [di+0Fh], al
                jz      short loc_1A370
                jmp     loc_1A29E
; ---------------------------------------------------------------------------

loc_1A370:                              ; CODE XREF: sub_1A21E+14D↑j
                mov     ax, 1
                jmp     loc_1A2A0
; ---------------------------------------------------------------------------

loc_1A376:                              ; CODE XREF: sub_1A21E+126↑j
                cmp     [bp+var_16], 0
                jz      short loc_1A380
                inc     byte_1DC7F

loc_1A380:                              ; CODE XREF: sub_1A21E+15C↑j
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
                call    sub_18DC8
                mov     byte ptr [bp+var_6], al
                call    sub_18DC8
                mov     byte ptr [bp+var_2], al
                mov     byte ptr [bp+var_A], 4
                mov     byte ptr [bp+var_C], 2
                cmp     byte ptr [bp+var_6], 80h
                jb      short loc_1A3B4
                mov     byte ptr [bp+var_A], 3
                mov     byte ptr [bp+var_C], 1
                and     byte ptr [bp+var_6], 7Fh

loc_1A3B4:                              ; CODE XREF: sub_1A21E+188↑j
                sub     byte ptr [bp+var_6], 6Eh ; 'n'
                mov     [bp+var_8], 0
                cmp     word_1DC76, 0
                jle     short loc_1A3FE
                mov     al, byte ptr [bp+var_6]
                sub     ah, ah
                mov     [bp+var_E], ax
                mov     di, [bp+var_8]

loc_1A3CF:                              ; CODE XREF: sub_1A21E+1D8↓j
                push    di
                call    thk_res_37B6
                add     sp, 2
                mov     si, ax
                mov     al, byte ptr [bp+var_A]
                cmp     [si+0Fh], al
                jz      short loc_1A3E8
                mov     al, byte ptr [bp+var_C]
                cmp     [si+0Fh], al
                jnz     short loc_1A3F1

loc_1A3E8:                              ; CODE XREF: sub_1A21E+1C0↑j
                mov     bx, [bp+var_E]
                mov     al, byte ptr [bp+var_2]
                or      [bx+si+51h], al

loc_1A3F1:                              ; CODE XREF: sub_1A21E+1C8↑j
                inc     di
                cmp     di, word_1DC76
                jl      short loc_1A3CF
                mov     [bp+var_8], di
                mov     [bp+var_4], si

loc_1A3FE:                              ; CODE XREF: sub_1A21E+1A4↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_res_0D22
                add     sp, 2

loc_1A416:                              ; CODE XREF: sub_1A21E+20A↓j
                mov     ax, 0Ah
                push    ax
                mov     ax, 54C4h
                push    ax
                call    thk_res_2E98
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

loc_1A44F:                              ; CODE XREF: sub_1A21E+212↑j
                mov     byte_22D1E, 0
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 10h
                push    di
                push    si
                sub     si, si

loc_1A464:                              ; CODE XREF: sub_1A21E+250↓j
                call    sub_18DC8
                mov     byte ptr [bp+si+var_C], al
                inc     si
                cmp     si, 0Ah
                jl      short loc_1A464
                mov     [bp+var_E], si
                sub     si, si

loc_1A475:                              ; CODE XREF: sub_1A21E+290↓j
                mov     al, [si+54C4h]
                sub     ah, ah
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_10], ax
                mov     al, byte ptr [bp+si+var_C]
                sub     ah, ah
                sub     ax, 11Ah
                neg     ax
                mov     di, ax
                cmp     [bp+var_10], di
                jz      short loc_1A4A8

loc_1A496:                              ; CODE XREF: sub_1A21E+28E↓j
                mov     [bp+var_2], di
                mov     [bp+var_E], si
                cmp     si, 0Ah
                jnz     short loc_1A4B0
                mov     byte_1DC7F, 1
                jmp     short loc_1A4B5
; ---------------------------------------------------------------------------

loc_1A4A8:                              ; CODE XREF: sub_1A21E+276↑j
                inc     si
                cmp     si, 0Ah
                jge     short loc_1A496
                jmp     short loc_1A475
; ---------------------------------------------------------------------------

loc_1A4B0:                              ; CODE XREF: sub_1A21E+281↑j
                mov     byte_1DC7F, 0

loc_1A4B5:                              ; CODE XREF: sub_1A21E+288↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     byte ptr [bp+var_4], 1
                mov     byte ptr [bp+var_2], 0
                or      byte_1DC80, 2
                call    sub_18DC8
                mov     byte ptr [bp+var_8], al
                call    sub_18DD8
                mov     [bp+var_6], ax
                cmp     byte ptr [bp+var_8], 80h
                jb      short loc_1A4EE
                and     byte ptr [bp+var_8], 7Fh
                mov     al, byte_1DC7F
                sub     ah, ah
                mov     [bp+var_6], ax

loc_1A4EE:                              ; CODE XREF: sub_1A21E+2C2↑j
                cmp     byte ptr [bp+var_8], 0
                jnz     short loc_1A4FC
                mov     al, byte ptr word_1DC76
                mov     byte ptr [bp+var_4], al
                jmp     short loc_1A52D
; ---------------------------------------------------------------------------

loc_1A4FC:                              ; CODE XREF: sub_1A21E+2D4↑j
                dec     byte ptr [bp+var_8]
                cmp     byte ptr [bp+var_8], 8
                jnz     short loc_1A520
                mov     al, byte_22D0E
                mov     byte ptr [bp+var_8], al
                or      al, al
                jnz     short loc_1A515
                mov     al, byte_1DC7F
                mov     byte ptr [bp+var_8], al

loc_1A515:                              ; CODE XREF: sub_1A21E+2EF↑j
                cmp     byte ptr [bp+var_8], 0
                jz      short loc_1A52D
                dec     byte ptr [bp+var_8]
                jmp     short loc_1A52D
; ---------------------------------------------------------------------------

loc_1A520:                              ; CODE XREF: sub_1A21E+2E5↑j
                mov     al, byte ptr [bp+var_8]
                cmp     byte ptr word_1DC76, al
                ja      short loc_1A52D
                mov     byte ptr [bp+var_8], 0

loc_1A52D:                              ; CODE XREF: sub_1A21E+2DC↑j
                                        ; sub_1A21E+2FB↑j ...
                mov     si, [bp+var_6]
                jmp     short loc_1A555
; ---------------------------------------------------------------------------

loc_1A532:                              ; CODE XREF: sub_1A21E+33F↓j
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
                call    thk_res_37B6
                add     sp, 2
                push    ax
                call    thk_res_3928
                add     sp, 0Ah

loc_1A555:                              ; CODE XREF: sub_1A21E+312↑j
                mov     al, byte ptr [bp+var_4]
                dec     byte ptr [bp+var_4]
                or      al, al
                jnz     short loc_1A532
                call    thk_res_3804
                or      ax, ax
                jz      short loc_1A56B
                mov     byte ptr word_1DBE4+1, 1

loc_1A56B:                              ; CODE XREF: sub_1A21E+346↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                call    sub_18DC8
                sub     ah, ah
                push    ax
                call    thk_res_36A6
                add     sp, 2
                mov     byte_1DC7F, al
                retn
sub_1A21E       endp


; =============== S U B R O U T I N E =======================================


sub_1A580       proc near               ; CODE XREF: seg002:06ED↑J
                                        ; sub_17E10+283↑p ...
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0FFFFh
                push    ax
                call    thk_res_2734
                add     sp, 6
                mov     byte_277D4, 0FFh
                call    thk_res_3E82
                mov     al, byte_1DC80
                and     al, 3
                cmp     al, 3
                jnz     short loc_1A5C8
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 27h ; '''
                push    ax
                sub     ax, ax
                push    ax
                call    thk_res_3026
                add     sp, 6
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2

loc_1A5C8:                              ; CODE XREF: sub_1A580+1D↑j
                test    byte_1DC80, 1
                jz      short loc_1A5D2
                call    thk_res_421E

loc_1A5D2:                              ; CODE XREF: sub_1A580+4D↑j
                test    byte_1DC80, 2
                jz      short loc_1A5DC
                call    thk_res_4A34

loc_1A5DC:                              ; CODE XREF: sub_1A580+57↑j
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
                call    thk_res_13B6
                add     sp, 0Ch

loc_1A600:                              ; CODE XREF: sub_1A580+61↑j
                mov     byte_1DC80, 0
                retn
sub_1A580       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1A606       proc near               ; CODE XREF: sub_1A606+34B↓p
                                        ; sub_1A606+395↓p

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
                mov     byte ptr word_1DBE4+1, 0
                mov     byte ptr [bp+var_4], 0
                cmp     byte ptr [bp+arg_0], 0
                jz      short loc_1A62F
                mov     al, byte ptr [bp+arg_0]
                cbw
                mov     si, ax
                add     byte ptr [bp+var_4], al

loc_1A625:                              ; CODE XREF: sub_1A606+24↓j
                                        ; sub_1A606+27↓j
                call    sub_18DC8
                cmp     al, 0FFh
                jnz     short loc_1A625
                dec     si
                jnz     short loc_1A625

loc_1A62F:                              ; CODE XREF: sub_1A606+14↑j
                mov     ax, word_1DC7C
                cmp     word_1DC7A, ax
                jl      short loc_1A63B
                jmp     loc_1A841
; ---------------------------------------------------------------------------

loc_1A63B:                              ; CODE XREF: sub_1A606+30↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                call    sub_18DC8
                mov     byte ptr [bp+var_4], al
                dec     word_1DC7A
                cmp     al, 22h ; '"'
                jz      short loc_1A663
                mov     al, byte ptr word_1DC1A
                mov     byte ptr [bp+var_2], al
                mov     al, byte_231E5
                cmp     byte ptr [bp+var_2], al
                jz      short loc_1A663
                jmp     loc_1A856
; ---------------------------------------------------------------------------

loc_1A663:                              ; CODE XREF: sub_1A606+4A↑j
                                        ; sub_1A606+58↑j ...
                call    sub_18DC8
                sub     ah, ah
                sub     ax, 1           ; switch 8 cases
                cmp     ax, 31h
                jbe     short loc_1A673
                jmp     def_1A676       ; jumptable 0001A676 default case
; ---------------------------------------------------------------------------

loc_1A673:                              ; CODE XREF: sub_1A606+68↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1A676[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1A67C:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_1905E       ; jumptable 0001A676 case 1
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A682:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                mov     ax, 13h         ; jumptable 0001A676 case 2
                push    ax
                call    sub_19074

loc_1A689:                              ; CODE XREF: sub_1A606+A8↓j
                                        ; sub_1A606+B6↓j
                add     sp, 2
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                align 2

loc_1A690:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_190F2       ; jumptable 0001A676 case 3
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A696:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_19110       ; jumptable 0001A676 case 4
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A69C:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_19160       ; jumptable 0001A676 case 5
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6A2:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_191EC       ; jumptable 0001A676 case 6
                jmp     loc_1A82C
; ---------------------------------------------------------------------------

loc_1A6A8:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                sub     ax, ax          ; jumptable 0001A676 case 7
                push    ax
                call    sub_193B8
                jmp     short loc_1A689
; ---------------------------------------------------------------------------

loc_1A6B0:                              ; CODE XREF: sub_1A606+70↑j
                                        ; DATA XREF: sub_1A606:jpt_1A676↓o
                call    sub_1940E       ; jumptable 0001A676 case 8
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                sub     ax, ax
                push    ax
                call    sub_1941E
                jmp     short loc_1A689
; ---------------------------------------------------------------------------
                call    sub_1946E
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_1947E
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_194D4
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_19560
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_19716
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_198C8
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                call    sub_198D2
                jmp     loc_1A82C
; ---------------------------------------------------------------------------
                db 0E8h
                db    7
                db 0F2h
                db 0E9h
                db  3Eh ; >
                db    1
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0E8h
                db  1Eh
                db 0F2h
                db 0EBh
                db  93h
                db 0E8h
                db  8Bh
                db 0F2h
                db 0E9h
                db  30h ; 0
                db    1
                db 0E8h
                db  91h
                db 0F2h
                db 0E9h
                db  2Ah ; *
                db    1
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0E8h
                db 0FAh
                db 0F2h
                db 0E9h
                db  7Eh ; ~
                db 0FFh
                db  90h
                db 0E8h
                db 0ADh
                db 0F3h
                db 0E9h
                db  1Ah
                db    1
                db 0E8h
                db  0Bh
                db 0F4h
                db 0E9h
                db  14h
                db    1
                db 0E8h
                db  1Dh
                db 0F4h
                db 0E9h
                db  0Eh
                db    1
                db 0E8h
                db  23h ; #
                db 0F4h
                db 0E9h
                db    8
                db    1
                db 0E8h
                db 0F3h
                db 0F4h
                db 0E9h
                db    2
                db    1
                db 0E8h
                db  13h
                db 0F5h
                db 0E9h
                db 0FCh
                db    0
                db 0E8h
                db  2Bh ; +
                db 0F5h
                db 0E9h
                db 0F6h
                db    0
                db 0E8h
                db  39h ; 9
                db 0F5h
                db 0E9h
                db 0F0h
                db    0
                db 0E8h
                db  4Bh ; K
                db 0F5h
                db 0E9h
                db 0EAh
                db    0
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0E8h
                db 0F8h
                db 0F6h
                db 0E9h
                db  3Eh ; >
                db 0FFh
                db  90h
                db 0E8h
                db 0E9h
                db 0F7h
                db 0E9h
                db 0DAh
                db    0
                db 0E8h
                db 0EFh
                db 0F7h
                db 0E9h
                db 0D4h
                db    0
                db 0E8h
                db  35h ; 5
                db 0F8h
                db 0E9h
                db 0CEh
                db    0
                db 0E8h
                db  65h ; e
                db 0F8h
                db 0E9h
                db 0C8h
                db    0
                db 0E8h
                db 0B7h
                db 0F8h
                db 0E9h
                db 0C2h
                db    0
                db 0E8h
                db 0DFh
                db 0F8h
                db 0E9h
                db 0BCh
                db    0
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db  0Bh
                db 0F9h
                db 0E9h
                db  0Fh
                db 0FFh
                db  2Bh ; +
                db 0C0h
                db 0EBh
                db 0F5h
                db 0E8h
                db 0A5h
                db 0F9h
                db 0E9h
                db 0A8h
                db    0
                db 0E8h
                db  13h
                db 0FAh
                db 0E9h
                db 0A2h
                db    0
                db 0E8h
                db  13h
                db 0FAh
                db 0E9h
                db  9Ch
                db    0
                db 0E8h
                db  4Fh ; O
                db 0FAh
                db 0E9h
                db  96h
                db    0
                db 0E8h
                db  69h ; i
                db 0FAh
                db 0E9h
                db  90h
                db    0
                db 0E8h
                db  7Fh ; 
                db 0FAh
                db 0E9h
                db  8Ah
                db    0
                db 0E8h
                db 0E1h
                db 0FBh
                db 0E9h
                db  84h
                db    0
                db 0E8h
                db  59h ; Y
                db 0FCh
                db 0EBh
                db  7Fh ; 
                db  90h
                db 0E8h
                db 0A9h
                db 0FCh
                db 0EBh
                db  79h ; y
                db  90h
                db 0E8h
                db    5
                db 0FDh
                db 0EBh
                db  73h ; s
                db  90h
                db 0E8h
                db 0B3h
                db 0FDh
                db 0EBh
                db  6Dh ; m
                db  90h
; ---------------------------------------------------------------------------

def_1A676:                              ; CODE XREF: sub_1A606+6A↑j
                mov     byte ptr word_1DBE4+1, 1 ; jumptable 0001A676 default case
                jmp     short loc_1A82C
; ---------------------------------------------------------------------------
                align 2
jpt_1A676       dw offset loc_1A67C     ; DATA XREF: sub_1A606+70↑r
                dw offset loc_1A682     ; jump table for switch statement
                dw offset loc_1A690
                dw offset loc_1A696
                dw offset loc_1A69C
                dw offset loc_1A6A2
                dw offset loc_1A6A8
                dw offset loc_1A6B0
byte_1A7D8      db 0B6h, 0A6h, 0BEh, 0A6h, 0C4h, 0A6h, 0CAh, 0A6h, 0D0h
                                        ; CODE XREF: seg002:0495↑J
                db 0A6h, 0D6h, 0A6h, 0DCh, 0A6h, 0E2h, 0A6h, 0E8h, 0A6h
byte_1A7EA      db 0EEh, 0A6h, 0F6h, 0A6h, 0FCh, 0A6h, 2, 0A7h, 0Ch, 0A7h
                                        ; CODE XREF: seg002:05B5↑J
                db 12h, 0A7h, 18h, 0A7h, 1Eh, 0A7h, 24h, 0A7h, 2Ah, 0A7h
                db 30h, 0A7h, 36h, 0A7h, 3Ch, 0A7h, 42h, 0A7h, 4Ch, 0A7h
                db 52h, 0A7h, 58h, 0A7h, 5Eh, 0A7h, 64h, 0A7h, 6Ah, 0A7h
                db 70h, 0A7h, 7Ah, 0A7h, 7Eh, 0A7h, 84h, 0A7h, 8Ah, 0A7h
                db 90h, 0A7h, 96h, 0A7h, 9Ch, 0A7h, 0A2h, 0A7h, 0A8h, 0A7h
                db 0AEh, 0A7h, 0B4h, 0A7h, 0BAh, 0A7h
; ---------------------------------------------------------------------------

loc_1A82C:                              ; CODE XREF: seg002:05C1↑J
                                        ; sub_1A606+79↑j ...
                mov     bx, word_1DC7A
                cmp     byte ptr [bx+6052h], 0FFh
                jz      short loc_1A841
                cmp     byte ptr word_1DBE4+1, 0
                jnz     short loc_1A841
                jmp     loc_1A663
; ---------------------------------------------------------------------------

loc_1A841:                              ; CODE XREF: sub_1A606+32↑j
                                        ; sub_1A606+22F↑j ...
                mov     bx, word_1DC7A
                cmp     byte ptr [bx+6052h], 0FFh
                jnz     short loc_1A856
                cmp     byte ptr word_1DBE4+1, 0
                jnz     short loc_1A856
                call    sub_1A580

loc_1A856:                              ; CODE XREF: sub_1A606+5A↑j
                                        ; sub_1A606+244↑j ...
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1A85C:                              ; CODE XREF: sub_1A606+2FB↓p
                                        ; sub_1A606+39E↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ch
                push    si
                sub     si, si

loc_1A865:                              ; CODE XREF: sub_1A606+273↓j
                                        ; sub_1A606+277↓j ...
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

loc_1A8C4:                              ; CODE XREF: sub_17E10+218↑p
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
                mov     al, byte ptr word_1DBE4
                mov     cl, 4
                shl     al, cl
                add     al, byte ptr word_1DBE2+1
                mov     [bp+var_E], al
                cmp     word_1DC7C, 0FFFFh
                jnz     short loc_1A904
                call    loc_1A85C

loc_1A904:                              ; CODE XREF: sub_1A606+2F9↑j
                mov     ax, word_22E0C
                mov     word_1DC7A, ax
                sub     al, al
                mov     byte_1DC80, al
                mov     byte ptr word_1DBE4+1, al
                mov     byte_277D4, 0FFh
                mov     al, [bp+var_A]
                sub     ah, ah
                mov     di, ax
                mov     si, [bp+var_8]

loc_1A921:                              ; CODE XREF: sub_1A606+360↓j
                                        ; sub_1A606+366↓j ...
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
                call    sub_1A606
                add     sp, 2
                sub     al, al
                mov     [bp+var_6], al
                mov     byte ptr [bp+var_4], al
                mov     byte ptr [bp+var_2], al

loc_1A962:                              ; CODE XREF: sub_1A606+339↑j
                                        ; sub_1A606+345↑j
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
                call    sub_1A606
                add     sp, 2
                call    thk_res_6008
                call    loc_1A85C

loc_1A9A7:                              ; CODE XREF: sub_1A606+376↑j
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
                call    thk_res_3EB2
                mov     al, byte ptr word_1DBE4
                sub     ah, ah
                mov     si, ax
                mov     cl, 4
                shl     si, cl
                mov     al, byte ptr word_1DBE2+1
                add     si, ax
                mov     al, [bp+var_E]
                cmp     ax, si
                jnz     short loc_1A9F4
                and     byte_23218, 7Fh
                and     byte ptr [si+5AD6h], 7Fh
                jmp     short loc_1A9F9
; ---------------------------------------------------------------------------
                align 2

loc_1A9F4:                              ; CODE XREF: sub_1A606+3DF↑j
                mov     byte_1DC7E, 1

loc_1A9F9:                              ; CODE XREF: sub_1A606+3A5↑j
                                        ; sub_1A606+3EB↑j
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

loc_1AA18:                              ; CODE XREF: sub_1A606+40A↑j
                mov     byte_27841, 4

loc_1AA1D:                              ; CODE XREF: sub_1A606+410↑j
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

loc_1AA41:                              ; CODE XREF: sub_1A606+41B↑j
                                        ; sub_1A606+421↑j ...
                mov     byte_27841, 2

loc_1AA46:                              ; CODE XREF: sub_1A606+439↑j
                mov     al, [bp+arg_2]
                sub     ah, ah
                cmp     ax, 7Fh         ; switch 128 cases
                jbe     short loc_1AA53
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA53:                              ; CODE XREF: sub_1A606+448↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1AA56[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1AA5C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                push    [bp+arg_0]      ; jumptable 0001AA56 case 0
                call    loc_1B0B2
                add     sp, 2
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA68:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                push    [bp+arg_0]      ; jumptable 0001AA56 case 1
                call    loc_1B0B2
                add     sp, 2
                cmp     byte_27840, 18h
                jnb     short loc_1AA7B
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------

loc_1AA7B:                              ; CODE XREF: sub_1A606+470↑j
                mov     byte_27840, 80h
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------
                align 2

loc_1AA84:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 2

loc_1AA87:                              ; CODE XREF: sub_1A606+48C↓j
                                        ; sub_1A606+494↓j ...
                mov     word_27842, ax
                jmp     def_1AA56       ; jumptable 0001AA56 default case
; ---------------------------------------------------------------------------
                align 2

loc_1AA8E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 3
                inc     ax
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AA94:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 4
                add     ax, 2
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AA9C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 5
                add     ax, 3
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAA4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 6
                add     ax, 4
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAAC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 7
                add     ax, 5
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAB4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 8
                add     ax, 6
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AABC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 9
                add     ax, 7
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAC4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 10
                add     ax, 8
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AACC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 11
                add     ax, 9
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAD4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 12
                add     ax, 0Ch
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AADC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 13
                add     ax, 0Dh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAE4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 14
                add     ax, 0Eh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAEC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 15
                add     ax, 0Fh
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAF4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 16
                add     ax, 10h
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AAFC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 17
                add     ax, 11h
                jmp     short loc_1AA87
; ---------------------------------------------------------------------------

loc_1AB04:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 18
                add     ax, 12h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB0E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 19
                add     ax, 13h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB18:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 20
                add     ax, 14h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB22:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 22
                add     ax, 16h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB2C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 23
                add     ax, 17h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB36:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 24
                add     ax, 18h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB40:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 25
                add     ax, 19h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB4A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 26
                add     ax, 1Ah
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB54:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 27
                add     ax, 1Bh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB5E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 28
                add     ax, 1Ch
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB68:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 29
                add     ax, 1Dh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB72:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 30
                add     ax, 1Eh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB7C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 31
                add     ax, 1Fh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB86:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 32,33
                add     ax, 74h ; 't'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB90:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 34
                add     ax, 6Bh ; 'k'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AB9A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 35
                add     ax, 6Eh ; 'n'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABA4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 36
                add     ax, 6Fh ; 'o'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABAE:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 37
                add     ax, 6Ah ; 'j'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABB8:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 38
                add     ax, 71h ; 'q'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABC2:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 39
                add     ax, 72h ; 'r'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABCC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 40,41
                add     ax, 58h ; 'X'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABD6:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 42
                add     ax, 73h ; 's'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABE0:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 43
                add     ax, 6Ch ; 'l'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABEA:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 44
                add     ax, 6Dh ; 'm'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABF4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 45
                add     ax, 70h ; 'p'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ABFE:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 21,46
                add     ax, 15h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC08:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 47
                add     ax, 21h ; '!'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC12:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 48
                add     ax, 22h ; '"'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC1C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 49-52
                add     ax, 62h ; 'b'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC26:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 53,54
                add     ax, 5Ah ; 'Z'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC30:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 55
                add     ax, 23h ; '#'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC3A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 56,57
                add     ax, 5Ch ; '\'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC44:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 58,59
                add     ax, 5Eh ; '^'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC4E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 60,61
                add     ax, 60h ; '`'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC58:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 62-64
                add     ax, 66h ; 'f'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC62:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 65
                add     ax, 24h ; '$'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC6C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 66
                add     ax, 25h ; '%'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC76:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 67
                add     ax, 26h ; '&'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC80:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 68
                add     ax, 27h ; '''
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC8A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 69
                add     ax, 28h ; '('
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC94:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 70
                add     ax, 29h ; ')'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AC9E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 71
                add     ax, 2Ah ; '*'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACA8:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 72
                add     ax, 2Bh ; '+'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACB2:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 73
                add     ax, 2Ch ; ','
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACBC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 74
                add     ax, 2Dh ; '-'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACC6:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 75
                add     ax, 3Ah ; ':'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACD0:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 76
                add     ax, 3Bh ; ';'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACDA:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 77
                add     ax, 3Ch ; '<'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACE4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 78
                add     ax, 3Dh ; '='
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACEE:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 79
                add     ax, 3Eh ; '>'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ACF8:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 80
                add     ax, 3Fh ; '?'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD02:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 81
                add     ax, 2Eh ; '.'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD0C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 82
                add     ax, 2Fh ; '/'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD16:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 83
                add     ax, 30h ; '0'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD20:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 84
                add     ax, 31h ; '1'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD2A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 85
                add     ax, 32h ; '2'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD34:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 86
                add     ax, 33h ; '3'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD3E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 87
                add     ax, 40h ; '@'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD48:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 88
                add     ax, 41h ; 'A'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD52:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 89
                add     ax, 42h ; 'B'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD5C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 90
                add     ax, 43h ; 'C'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD66:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 91
                add     ax, 44h ; 'D'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD70:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 92
                add     ax, 45h ; 'E'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD7A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 93
                add     ax, 34h ; '4'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD84:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 94
                add     ax, 35h ; '5'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD8E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 95
                add     ax, 36h ; '6'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AD98:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 96
                add     ax, 37h ; '7'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADA2:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 97
                add     ax, 38h ; '8'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADAC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 98
                add     ax, 39h ; '9'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADB6:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 99
                add     ax, 46h ; 'F'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADC0:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 100
                add     ax, 47h ; 'G'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADCA:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 101
                add     ax, 48h ; 'H'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADD4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 102
                add     ax, 49h ; 'I'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADDE:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 103
                add     ax, 4Ah ; 'J'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADE8:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 104
                add     ax, 4Bh ; 'K'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADF2:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 105
                add     ax, 4Ch ; 'L'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1ADFC:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 106
                add     ax, 4Dh ; 'M'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE06:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 107
                add     ax, 4Eh ; 'N'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE10:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 108
                add     ax, 4Fh ; 'O'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE1A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 109
                add     ax, 50h ; 'P'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE24:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 110
                add     ax, 51h ; 'Q'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE2E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 111
                add     ax, 52h ; 'R'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE38:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 112
                add     ax, 53h ; 'S'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE42:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 113
                add     ax, 54h ; 'T'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE4C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 114
                add     ax, 55h ; 'U'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE56:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 115
                add     ax, 56h ; 'V'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE60:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 116
                add     ax, 79h ; 'y'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE6A:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 117
                add     ax, 7Ah ; 'z'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE74:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 118
                add     ax, 7Bh ; '{'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE7E:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 cases 119,120
                add     ax, 76h ; 'v'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE88:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 121
                add     ax, 78h ; 'x'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE92:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 122
                add     ax, 7Ch ; '|'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AE9C:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 123
                add     ax, 7Dh ; '}'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEA6:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 124
                add     ax, 7Eh ; '~'
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEB0:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 125
                add     ax, 7Fh
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEBA:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 126
                add     ax, 80h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2

loc_1AEC4:                              ; CODE XREF: sub_1A606+450↑j
                                        ; DATA XREF: sub_1A606:jpt_1AA56↓o
                mov     ax, [bp+arg_0]  ; jumptable 0001AA56 case 127
                add     ax, 81h
                jmp     loc_1AA87
; ---------------------------------------------------------------------------
                align 2
jpt_1AA56       dw offset loc_1AA5C, offset loc_1AA68, offset loc_1AA84
                                        ; DATA XREF: sub_1A606+450↑r
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

def_1AA56:                              ; CODE XREF: sub_1A606+44A↑j
                                        ; sub_1A606+45F↑j ...
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

loc_1B00A:                              ; CODE XREF: sub_1A606+9D2↑j
                                        ; sub_1A606+9D8↑j ...
                inc     word_27842

loc_1B00E:                              ; CODE XREF: sub_1A606+A02↑j
                cmp     [bp+arg_2], 33h ; '3'
                jz      short loc_1B01A
                cmp     [bp+arg_2], 40h ; '@'
                jnz     short loc_1B01F

loc_1B01A:                              ; CODE XREF: sub_1A606+A0C↑j
                add     word_27842, 2

loc_1B01F:                              ; CODE XREF: sub_1A606+A12↑j
                cmp     [bp+arg_2], 34h ; '4'
                jnz     short loc_1B02A
                add     word_27842, 3

loc_1B02A:                              ; CODE XREF: sub_1A606+A1D↑j
                mov     ax, word_27842
                cmp     [bp+var_2], ax
                jz      short loc_1B037
                mov     byte_27841, 1

loc_1B037:                              ; CODE XREF: sub_1A606+A2A↑j
                cmp     byte_27841, 1
                jnz     short loc_1B04A
                mov     bx, word_27842
                mov     al, [bx]
                mov     byte_27840, al
                jmp     short loc_1B0AD
; ---------------------------------------------------------------------------
                align 2

loc_1B04A:                              ; CODE XREF: sub_1A606+A36↑j
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

loc_1B066:                              ; CODE XREF: sub_1A606+A49↑j
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

loc_1B08D:                              ; CODE XREF: sub_1A606+A8D↓j
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

loc_1B0AD:                              ; CODE XREF: sub_1A606+A41↑j
                                        ; sub_1A606+A5D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1B0B2:                              ; CODE XREF: sub_1A606+459↑p
                                        ; sub_1A606+465↑p
                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                mov     dl, 0FFh
                sub     cx, cx
                mov     si, 7E20h

loc_1B0C1:                              ; CODE XREF: sub_1A606+AE2↓j
                mov     di, si
                cmp     [bp+arg_0], di
                jnz     short loc_1B0CC
                mov     ax, cx
                mov     dl, al

loc_1B0CC:                              ; CODE XREF: sub_1A606+AC0↑j
                cmp     dl, 0FFh
                jz      short loc_1B0DE

loc_1B0D1:                              ; CODE XREF: sub_1A606+AE0↓j
                mov     [bp+var_2], di
                mov     byte_27840, dl
                mov     [bp+var_4], cx
                jmp     short loc_1B0EA
; ---------------------------------------------------------------------------
                align 2

loc_1B0DE:                              ; CODE XREF: sub_1A606+AC9↑j
                add     si, 82h
                inc     cx
                cmp     cx, 30h ; '0'
                jge     short loc_1B0D1
                jmp     short loc_1B0C1
; ---------------------------------------------------------------------------

loc_1B0EA:                              ; CODE XREF: sub_1A606+AD5↑j
                mov     word_27842, 9FF0h
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1A606       endp


; =============== S U B R O U T I N E =======================================


sub_1B0F6       proc near               ; CODE XREF: seg002:0909↑J
                                        ; sub_1B5EA+98↓p
                mov     ax, word_1DBBE
                or      ax, word_1DBC0
                jz      short loc_1B129
                push    word_1DBC8
                push    word_1DBC6
                call    thk_res_14CA
                add     sp, 4
                push    word_1DBC4
                push    word_1DBC2
                call    thk_res_14CA
                add     sp, 4
                push    word_1DBC0
                push    word_1DBBE
                call    thk_res_14CA
                add     sp, 4

loc_1B129:                              ; CODE XREF: sub_1B0F6+7↑j
                mov     ax, word_1DBCE
                or      ax, word_1DBD0
                jz      short loc_1B14E
                push    word_1DBB8
                push    word_1DBB6
                call    thk_res_14CA
                add     sp, 4
                push    word_1DBD0
                push    word_1DBCE
                call    thk_res_14CA
                add     sp, 4

loc_1B14E:                              ; CODE XREF: sub_1B0F6+3A↑j
                mov     ax, word_1DBB2
                or      ax, word_1DBB4
                jz      short loc_1B165
                push    word_1DBB4
                push    word_1DBB2
                call    thk_res_14CA
                add     sp, 4

loc_1B165:                              ; CODE XREF: sub_1B0F6+5F↑j
                mov     ax, word_1DBBA
                or      ax, word_1DBBC
                jz      short loc_1B17C
                push    word_1DBBC
                push    word_1DBBA
                call    thk_res_14CA
                add     sp, 4

loc_1B17C:                              ; CODE XREF: sub_1B0F6+76↑j
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
sub_1B0F6       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B1B0       proc near               ; CODE XREF: sub_1B1D4+93↓p
                                        ; sub_1B288+62↓p ...

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     si, [bp+arg_0]

loc_1B1BA:                              ; CODE XREF: sub_1B1B0+19↓j
                push    si
                call    thk_res_1492
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
sub_1B1B0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B1D4       proc near               ; CODE XREF: seg002:0921↑J
                                        ; sub_1B5EA+CA↓p

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
                call    thk_res_14CA
                add     sp, 4

loc_1B20C:                              ; CODE XREF: sub_1B1D4+28↑j
                cmp     byte ptr word_1DBE2, 29h ; ')'
                jb      short loc_1B227
                cmp     byte ptr word_1DBE2, 2Ch ; ','
                ja      short loc_1B227
                mov     bl, byte ptr word_1DBE2
                sub     bh, bh
                mov     al, [bx+16B3h]
                mov     [bp+var_8], al

loc_1B227:                              ; CODE XREF: sub_1B1D4+3D↑j
                                        ; sub_1B1D4+44↑j
                cmp     [bp+var_8], 9
                jnz     short loc_1B234
                mov     ax, word_1DD48
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1B234:                              ; CODE XREF: sub_1B1D4+57↑j
                cmp     [bp+var_8], 0Bh
                jnz     short loc_1B240
                mov     ax, word_1DD4E
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                align 2

loc_1B240:                              ; CODE XREF: sub_1B1D4+64↑j
                cmp     [bp+var_8], 0Ch
                jnz     short loc_1B24C
                mov     ax, word_1DD4C
                jmp     short loc_1B255
; ---------------------------------------------------------------------------
                align 2

loc_1B24C:                              ; CODE XREF: sub_1B1D4+70↑j
                cmp     [bp+var_8], 0Ah
                jnz     short loc_1B258
                mov     ax, word_1DD4A

loc_1B255:                              ; CODE XREF: sub_1B1D4+5C↑j
                                        ; sub_1B1D4+69↑j ...
                mov     [bp+var_2], ax

loc_1B258:                              ; CODE XREF: sub_1B1D4+7C↑j
                mov     al, [bp+var_8]
                mov     byte_1EF2A, al
                cmp     [bp+var_2], 0
                jz      short loc_1B27D
                push    [bp+var_2]
                call    sub_1B1B0
                add     sp, 2
                jmp     short loc_1B277
; ---------------------------------------------------------------------------
                align 2

loc_1B270:                              ; CODE XREF: sub_1B1D4+1F↑j
                mov     ax, word_1DBCA
                mov     dx, word_1DBCC

loc_1B277:                              ; CODE XREF: sub_1B1D4+99↑j
                mov     [bp+var_6], ax
                mov     [bp+var_4], dx

loc_1B27D:                              ; CODE XREF: sub_1B1D4+8E↑j
                mov     ax, [bp+var_6]
                mov     dx, [bp+var_4]
                mov     sp, bp
                pop     bp
                retn
sub_1B1D4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B288       proc near               ; CODE XREF: seg002:092D↑J
                                        ; sub_1B5EA+B5↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                cmp     byte_1DBED, 0
                jnz     short loc_1B2B6
                mov     ax, word_1DBCA
                or      ax, word_1DBCC
                jz      short loc_1B2B6
                push    word_1DBCC
                push    word_1DBCA
                call    thk_res_14CA
                add     sp, 4
                sub     ax, ax
                mov     word_1DBCC, ax
                mov     word_1DBCA, ax
                mov     byte_1EF2A, 0FFh

loc_1B2B6:                              ; CODE XREF: sub_1B288+8↑j
                                        ; sub_1B288+11↑j
                cmp     byte_1DBED, 0
                jnz     short loc_1B2CC
                cmp     byte_1DB96, 0Fh
                jnz     short loc_1B2CC
                mov     word_1DC8A, 1
                jmp     short loc_1B2D2
; ---------------------------------------------------------------------------

loc_1B2CC:                              ; CODE XREF: sub_1B288+33↑j
                                        ; sub_1B288+3A↑j
                mov     word_1DC8A, 2

loc_1B2D2:                              ; CODE XREF: sub_1B288+42↑j
                mov     ax, [bp+arg_0]
                cmp     ax, 6           ; switch 7 cases
                jbe     short loc_1B2DD
                jmp     def_1B2E0       ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------

loc_1B2DD:                              ; CODE XREF: sub_1B288+50↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1B2E0[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1B2E6:                              ; CODE XREF: sub_1B288+58↑j
                                        ; DATA XREF: sub_1B288:jpt_1B2E0↓o
                push    word_1DD2A      ; jumptable 0001B2E0 case 0
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD26
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD28
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD2C

loc_1B31D:                              ; CODE XREF: sub_1B288+DD↓j
                                        ; sub_1B288+117↓j
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBCE, ax
                mov     word_1DBD0, dx
                jmp     def_1B2E0       ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------
                align 2

loc_1B32E:                              ; CODE XREF: sub_1B288+58↑j
                                        ; DATA XREF: sub_1B288:jpt_1B2E0↓o
                push    word_1DD32      ; jumptable 0001B2E0 case 1
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD2E
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD30
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD34
                jmp     short loc_1B31D
; ---------------------------------------------------------------------------
                align 2

loc_1B368:                              ; CODE XREF: sub_1B288+58↑j
                                        ; DATA XREF: sub_1B288:jpt_1B2E0↓o
                push    word_1DD3A      ; jumptable 0001B2E0 cases 2,5
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD36
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD38
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB6, ax
                mov     word_1DBB8, dx
                push    word_1DD3C
                jmp     loc_1B31D
; ---------------------------------------------------------------------------

loc_1B3A2:                              ; CODE XREF: sub_1B288+58↑j
                                        ; DATA XREF: sub_1B288:jpt_1B2E0↓o
                push    word_1DD44      ; jumptable 0001B2E0 cases 3,4,6
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBBA, ax
                mov     word_1DBBC, dx
                push    word_1DD46
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBB2, ax
                mov     word_1DBB4, dx
                push    word_1DD3E
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBBE, ax
                mov     word_1DBC0, dx
                push    word_1DD40
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBC2, ax
                mov     word_1DBC4, dx
                push    word_1DD42
                call    sub_1B1B0
                add     sp, 2
                mov     word_1DBC6, ax
                mov     word_1DBC8, dx
                jmp     short def_1B2E0 ; jumptable 0001B2E0 default case
; ---------------------------------------------------------------------------
                align 2
jpt_1B2E0       dw offset loc_1B2E6     ; DATA XREF: sub_1B288+58↑r
                dw offset loc_1B32E     ; jump table for switch statement
                dw offset loc_1B368
                dw offset loc_1B3A2
                dw offset loc_1B3A2
                dw offset loc_1B368
                dw offset loc_1B3A2
; ---------------------------------------------------------------------------

def_1B2E0:                              ; CODE XREF: sub_1B288+52↑j
                                        ; sub_1B288+A2↑j ...
                mov     word_1DC8A, 2   ; jumptable 0001B2E0 default case
                pop     bp
                retn
sub_1B288       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B410       proc near               ; CODE XREF: sub_1B5EA+22↓p

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

loc_1B41E:                              ; CODE XREF: sub_1B410+32↓j
                cmp     [si+16E8h], dl
                ja      short loc_1B42E
                cmp     [si+16F0h], dl
                jb      short loc_1B42E
                mov     cl, [si+16E0h]

loc_1B42E:                              ; CODE XREF: sub_1B410+12↑j
                                        ; sub_1B410+18↑j
                cmp     cl, 7
                jz      short loc_1B43C

loc_1B433:                              ; CODE XREF: sub_1B410+30↓j
                mov     [bp+var_4], si
                mov     [bp+var_2], cl
                jmp     short loc_1B444
; ---------------------------------------------------------------------------
                align 2

loc_1B43C:                              ; CODE XREF: sub_1B410+21↑j
                inc     si
                cmp     si, 7
                jge     short loc_1B433
                jmp     short loc_1B41E
; ---------------------------------------------------------------------------

loc_1B444:                              ; CODE XREF: sub_1B410+29↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1B410       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B44E       proc near               ; CODE XREF: sub_1B4E0+9A↓p
                                        ; sub_1B4E0+C0↓p ...

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

loc_1B461:                              ; CODE XREF: sub_1B44E+83↓j
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

loc_1B482:                              ; CODE XREF: sub_1B44E+26↑j
                                        ; sub_1B44E+2C↑j
                cmp     [bp+var_6], 0FCh
                jbe     short loc_1B48B
                mov     dx, 5ED8h

loc_1B48B:                              ; CODE XREF: sub_1B44E+31↑j
                                        ; sub_1B44E+38↑j
                cmp     [bp+var_8], 0Fh
                jbe     short loc_1B49C
                cmp     [bp+var_8], 13h
                jnb     short loc_1B49C
                mov     dx, 5BD6h
                jmp     short loc_1B4A5
; ---------------------------------------------------------------------------

loc_1B49C:                              ; CODE XREF: sub_1B44E+41↑j
                                        ; sub_1B44E+47↑j
                cmp     [bp+var_8], 0FCh
                jbe     short loc_1B4A5
                mov     dx, 5CD6h

loc_1B4A5:                              ; CODE XREF: sub_1B44E+4C↑j
                                        ; sub_1B44E+52↑j
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
sub_1B44E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B4E0       proc near               ; CODE XREF: sub_1B5EA+135↓p
                                        ; sub_1B75E:loc_1B7C0↓p

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
                cmp     byte_1DC1F, 4Eh ; 'N'
                jnz     short loc_1B4F6
                mov     [bp+var_E], 16F8h
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B4F6:                              ; CODE XREF: sub_1B4E0+D↑j
                cmp     byte_1DC1F, 53h ; 'S'
                jnz     short loc_1B504
                mov     [bp+var_E], 16FEh
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B504:                              ; CODE XREF: sub_1B4E0+1B↑j
                cmp     byte_1DC1F, 45h ; 'E'
                jnz     short loc_1B512
                mov     [bp+var_E], 1704h
                jmp     short loc_1B51E
; ---------------------------------------------------------------------------

loc_1B512:                              ; CODE XREF: sub_1B4E0+29↑j
                cmp     byte_1DC1F, 57h ; 'W'
                jnz     short loc_1B51E
                mov     [bp+var_E], 170Ah

loc_1B51E:                              ; CODE XREF: sub_1B4E0+14↑j
                                        ; sub_1B4E0+22↑j ...
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
                mov     al, byte ptr word_1DBE4
                push    ax
                mov     al, byte ptr word_1DBE2+1
                push    ax
                mov     ax, 59CAh
                push    ax
                call    sub_1B44E
                add     sp, 0Ah
                push    di
                push    si
                mov     al, [bp+var_8]
                sub     ah, ah
                mov     cl, byte ptr word_1DBE4
                sub     ch, ch
                add     ax, cx
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                mov     cl, byte ptr word_1DBE2+1
                add     ax, cx
                push    ax
                mov     ax, 59CEh
                push    ax
                call    sub_1B44E
                add     sp, 0Ah
                push    di
                push    si
                mov     al, [bp+var_C]
                sub     ah, ah
                mov     cl, byte ptr word_1DBE4
                sub     ch, ch
                add     ax, cx
                push    ax
                mov     al, [bp+var_A]
                sub     ah, ah
                mov     cl, byte ptr word_1DBE2+1
                add     ax, cx
                push    ax
                mov     ax, 59D2h
                push    ax
                call    sub_1B44E
                add     sp, 0Ah
                mov     bl, byte ptr word_1DBE4
                sub     bh, bh
                mov     cl, 4
                shl     bx, cl
                mov     al, byte ptr word_1DBE2+1
                sub     ah, ah
                add     bx, ax
                mov     al, [bx+5AD6h]
                mov     byte_23218, al
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1B4E0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B5EA       proc near               ; CODE XREF: seg002:02F1↑J
                                        ; sub_194D4+71↑p ...

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
                call    sub_1B410
                add     sp, 2
                mov     [bp+var_8], al
                cmp     al, 3
                jz      short loc_1B621
                cmp     al, 6
                jz      short loc_1B621
                cmp     al, 4
                jnz     short loc_1B635

loc_1B621:                              ; CODE XREF: sub_1B5EA+2D↑j
                                        ; sub_1B5EA+31↑j
                mov     [bp+var_6], 1
                cmp     byte_1DBED, 1
                jnz     short loc_1B632
                inc     [bp+var_2]
                jmp     short loc_1B635
; ---------------------------------------------------------------------------
                align 2

loc_1B632:                              ; CODE XREF: sub_1B5EA+40↑j
                inc     [bp+var_A]

loc_1B635:                              ; CODE XREF: sub_1B5EA+35↑j
                                        ; sub_1B5EA+45↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1B646
                mov     al, [bp+var_C]
                cmp     [bp+var_8], al
                jz      short loc_1B646
                inc     [bp+var_A]

loc_1B646:                              ; CODE XREF: sub_1B5EA+4F↑j
                                        ; sub_1B5EA+57↑j
                cmp     [bp+var_A], 0
                jz      short loc_1B64F
                inc     [bp+var_4]

loc_1B64F:                              ; CODE XREF: sub_1B5EA+60↑j
                sub     ax, ax
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     al, byte ptr word_1DBE2
                cmp     [bp+arg_0], al
                jz      short loc_1B6DC
                mov     al, [bp+var_6]
                mov     byte_1DBED, al
                mov     al, [bp+arg_0]
                mov     byte ptr word_1DBE2, al
                cmp     [bp+var_4], 0
                jz      short loc_1B67C
                mov     ax, 1710h
                push    ax
                call    thk_res_410A
                add     sp, 2

loc_1B67C:                              ; CODE XREF: sub_1B5EA+86↑j
                cmp     [bp+var_A], 0
                jz      short loc_1B6A5
                call    sub_1B0F6
                cmp     byte_1DB96, 0Fh
                jnz     short loc_1B699
                cmp     byte_1DBED, 0
                jnz     short loc_1B699
                mov     word_1DC8A, 1

loc_1B699:                              ; CODE XREF: sub_1B5EA+A0↑j
                                        ; sub_1B5EA+A7↑j
                mov     al, [bp+var_8]
                sub     ah, ah
                push    ax
                call    sub_1B288
                add     sp, 2

loc_1B6A5:                              ; CODE XREF: sub_1B5EA+96↑j
                call    thk_res_6164
                cmp     [bp+var_6], 1
                jnz     short loc_1B6C4
                mov     word_1DC8A, 2
                call    sub_1B1D4
                mov     word_1DBCA, ax
                mov     word_1DBCC, dx
                mov     word_1DBF0, 0FFFFh

loc_1B6C4:                              ; CODE XREF: sub_1B5EA+C2↑j
                mov     al, [bp+var_8]
                mov     byte_1DBEC, al
                call    thk_res_6008
                mov     word_1DC7C, 0FFFFh
                mov     word_1DC8A, 2
                call    sub_1BE24

loc_1B6DC:                              ; CODE XREF: sub_1B5EA+74↑j
                cmp     [bp+arg_2], 0FFh
                jnz     short loc_1B6F6
                mov     al, byte_231E4
                and     al, 0Fh
                mov     byte ptr word_1DBE2+1, al
                mov     al, byte_231E4
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                jmp     short loc_1B6FF
; ---------------------------------------------------------------------------
                align 2

loc_1B6F6:                              ; CODE XREF: sub_1B5EA+F6↑j
                mov     al, [bp+arg_2]
                mov     byte ptr word_1DBE2+1, al
                mov     al, [bp+arg_4]

loc_1B6FF:                              ; CODE XREF: sub_1B5EA+109↑j
                mov     byte ptr word_1DBE4, al
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                call    thk_res_49E2
                call    thk_res_421E
                call    thk_res_4A34
                call    sub_1B4E0
                call    thk_res_3FFC
                cmp     byte_1DC1E, 1
                jnz     short loc_1B732
                call    thk_res_47D8
                jmp     short loc_1B735
; ---------------------------------------------------------------------------
                align 2

loc_1B732:                              ; CODE XREF: sub_1B5EA+140↑j
                call    thk_res_471E

loc_1B735:                              ; CODE XREF: sub_1B5EA+145↑j
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_142A
                add     sp, 4
                mov     byte_1DC80, 0
                mov     byte_1DC7E, 1
                mov     ax, 1
                push    ax
                call    thk_res_1392
                mov     word_1DC8A, 2
                mov     sp, bp
                pop     bp
                retn
sub_1B5EA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B75E       proc near               ; CODE XREF: seg002:06BD↑J
                                        ; sub_17E10+203↑p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0FFh
                cmp     byte ptr word_1DBE2+1, 10h
                jnz     short loc_1B774
                mov     al, byte_231DC
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B774:                              ; CODE XREF: sub_1B75E+F↑j
                cmp     byte ptr word_1DBE2+1, 0FFh
                jnz     short loc_1B780
                mov     al, byte_231DE
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B780:                              ; CODE XREF: sub_1B75E+1B↑j
                cmp     byte ptr word_1DBE4, 10h
                jnz     short loc_1B78C
                mov     al, byte_231DB
                jmp     short loc_1B796
; ---------------------------------------------------------------------------

loc_1B78C:                              ; CODE XREF: sub_1B75E+27↑j
                cmp     byte ptr word_1DBE4, 0FFh
                jnz     short loc_1B799
                mov     al, byte_231DD

loc_1B796:                              ; CODE XREF: sub_1B75E+14↑j
                                        ; sub_1B75E+20↑j ...
                mov     [bp+var_2], al

loc_1B799:                              ; CODE XREF: sub_1B75E+33↑j
                cmp     [bp+var_2], 0FFh
                jz      short loc_1B7C0
                and     byte ptr word_1DBE2+1, 0Fh
                and     byte ptr word_1DBE4, 0Fh
                mov     al, byte ptr word_1DBE4
                sub     ah, ah
                push    ax
                mov     al, byte ptr word_1DBE2+1
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    sub_1B5EA
                add     sp, 6
                jmp     short loc_1B7C3
; ---------------------------------------------------------------------------
                align 2

loc_1B7C0:                              ; CODE XREF: sub_1B75E+3F↑j
                call    sub_1B4E0

loc_1B7C3:                              ; CODE XREF: sub_1B75E+5F↑j
                mov     sp, bp
                pop     bp
                retn
sub_1B75E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B7C8       proc near               ; CODE XREF: sub_1B862+87↓p

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
                add     al, byte ptr word_1DBE2+1
                sub     al, 2
                mov     [bp+var_4], al
                mov     al, [bp+arg_2]
                add     al, byte ptr word_1DBE4
                sub     al, 2
                mov     [bp+var_6], al
                cmp     byte_1DBED, 0
                jnz     short loc_1B802
                cmp     [bp+var_4], 0Fh
                ja      short loc_1B7FD
                cmp     al, 0Fh
                jbe     short loc_1B802

loc_1B7FD:                              ; CODE XREF: sub_1B7C8+2F↑j
                sub     ax, ax
                jmp     short loc_1B85C
; ---------------------------------------------------------------------------
                align 2

loc_1B802:                              ; CODE XREF: sub_1B7C8+29↑j
                                        ; sub_1B7C8+33↑j
                cmp     [bp+var_4], 0Fh
                jbe     short loc_1B816
                cmp     [bp+var_4], 13h
                jnb     short loc_1B816
                mov     [bp+var_2], 5DD8h
                jmp     short loc_1B821
; ---------------------------------------------------------------------------
                align 2

loc_1B816:                              ; CODE XREF: sub_1B7C8+3E↑j
                                        ; sub_1B7C8+44↑j
                cmp     [bp+var_4], 0FCh
                jbe     short loc_1B821
                mov     [bp+var_2], 5ED8h

loc_1B821:                              ; CODE XREF: sub_1B7C8+4B↑j
                                        ; sub_1B7C8+52↑j
                cmp     [bp+var_6], 0Fh
                jbe     short loc_1B834
                cmp     [bp+var_6], 13h
                jnb     short loc_1B834
                mov     [bp+var_2], 5BD6h
                jmp     short loc_1B83F
; ---------------------------------------------------------------------------

loc_1B834:                              ; CODE XREF: sub_1B7C8+5D↑j
                                        ; sub_1B7C8+63↑j
                cmp     [bp+var_6], 0FCh
                jbe     short loc_1B83F
                mov     [bp+var_2], 5CD6h

loc_1B83F:                              ; CODE XREF: sub_1B7C8+6A↑j
                                        ; sub_1B7C8+70↑j
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

loc_1B85C:                              ; CODE XREF: sub_1B7C8+37↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1B7C8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1B862       proc near               ; CODE XREF: seg002:065D↑J
                                        ; sub_17E10+52↑p

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
                call    thk_res_3292
                add     sp, 8
                mov     cx, 19h
                lea     di, [bp+var_1A]
                mov     ax, ss
                mov     es, ax
                assume es:nothing
                mov     ax, 1Eh
                repne stosb
                mov     [bp+var_20], 3E1h
                cmp     byte_1DBED, 1
                jnz     short loc_1B8AA
                mov     [bp+var_20], 3E0h

loc_1B8AA:                              ; CODE XREF: sub_1B862+41↑j
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jz      short loc_1B92C
                mov     [bp+var_28], 0
                jmp     short loc_1B91F
; ---------------------------------------------------------------------------
                db  90h
                align 2

loc_1B8BA:                              ; CODE XREF: sub_1B862+95↓j
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

loc_1B8D4:                              ; CODE XREF: sub_1B862+B7↓j
                inc     [bp+var_24]

loc_1B8D7:                              ; CODE XREF: sub_1B862+C7↓j
                cmp     [bp+var_24], 5
                jnb     short loc_1B91C
                mov     al, [bp+var_24]
                sub     ah, ah
                mov     si, ax
                mov     al, [bp+var_28]
                push    ax
                push    si
                call    sub_1B7C8
                add     sp, 4
                mov     [bp+var_1C], al
                cmp     byte_1DBED, 0
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

loc_1B91C:                              ; CODE XREF: sub_1B862+79↑j
                inc     [bp+var_28]

loc_1B91F:                              ; CODE XREF: sub_1B862+54↑j
                cmp     [bp+var_28], 5
                jnb     short loc_1B92C
                mov     [bp+var_24], 0
                jmp     short loc_1B8D7
; ---------------------------------------------------------------------------
                align 2

loc_1B92C:                              ; CODE XREF: sub_1B862+4E↑j
                                        ; sub_1B862+C1↑j
                mov     [bp+var_28], 0
                jmp     short loc_1B987
; ---------------------------------------------------------------------------

loc_1B932:                              ; CODE XREF: sub_1B862+120↓j
                inc     [bp+var_24]

loc_1B935:                              ; CODE XREF: sub_1B862+12F↓j
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
                call    thk_res_14FE
                add     sp, 0Ah
                jmp     short loc_1B932
; ---------------------------------------------------------------------------

loc_1B984:                              ; CODE XREF: sub_1B862+D7↑j
                inc     [bp+var_28]

loc_1B987:                              ; CODE XREF: sub_1B862+CE↑j
                cmp     [bp+var_28], 5
                jnb     short loc_1B994
                mov     [bp+var_24], 0
                jmp     short loc_1B935
; ---------------------------------------------------------------------------
                align 2

loc_1B994:                              ; CODE XREF: sub_1B862+129↑j
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jnz     short loc_1B99F
                jmp     loc_1BA89
; ---------------------------------------------------------------------------

loc_1B99F:                              ; CODE XREF: sub_1B862+138↑j
                cmp     byte ptr word_1DBE2+1, 3
                jb      short loc_1B9A9
                jmp     loc_1BA39
; ---------------------------------------------------------------------------

loc_1B9A9:                              ; CODE XREF: sub_1B862+142↑j
                cmp     byte_1DBED, 0
                jz      short loc_1B9B3
                jmp     loc_1BA39
; ---------------------------------------------------------------------------

loc_1B9B3:                              ; CODE XREF: sub_1B862+14C↑j
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_0BAA
                add     sp, 2
                cmp     byte ptr word_1DBE4, 0Dh
                jbe     short loc_1B9DD
                mov     al, byte ptr word_1DBE4
                sub     ah, ah
                mov     cx, ax
                shl     ax, 1
                shl     ax, 1
                add     ax, cx
                shl     ax, 1
                add     ax, cx
                sub     ax, 8Fh
                add     [bp+var_22], ax

loc_1B9DD:                              ; CODE XREF: sub_1B862+162↑j
                cmp     byte ptr word_1DBE4, 3
                jnb     short loc_1B9FD
                mov     al, byte ptr word_1DBE4
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

loc_1B9FD:                              ; CODE XREF: sub_1B862+180↑j
                mov     al, byte ptr word_1DBE2+1
                sub     ah, ah
                sub     ax, 2
                neg     ax
                mov     cl, 4
                shl     ax, cl
                add     ax, 0E0h
                mov     [bp+var_1E], ax
                mov     [bp+var_24], 0

loc_1BA15:                              ; CODE XREF: sub_1B862+1CC↓j
                push    [bp+var_26]
                push    [bp+var_22]
                push    [bp+var_1E]
                inc     [bp+var_1E]
                call    thk_res_1126
                add     sp, 6
                inc     [bp+var_24]
                cmp     [bp+var_24], 3
                jb      short loc_1BA15
                sub     ax, ax
                push    ax
                call    thk_res_0BAA
                add     sp, 2

loc_1BA39:                              ; CODE XREF: sub_1B862+144↑j
                                        ; sub_1B862+14E↑j
                cmp     byte_1DC1F, 4Eh ; 'N'
                jnz     short loc_1BA48
                mov     [bp+var_2A], 20h ; ' '
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------
                align 2

loc_1BA48:                              ; CODE XREF: sub_1B862+1DC↑j
                cmp     byte_1DC1F, 53h ; 'S'
                jnz     short loc_1BA56
                mov     [bp+var_2A], 21h ; '!'
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------

loc_1BA56:                              ; CODE XREF: sub_1B862+1EB↑j
                cmp     byte_1DC1F, 45h ; 'E'
                jnz     short loc_1BA64
                mov     [bp+var_2A], 22h ; '"'
                jmp     short loc_1BA70
; ---------------------------------------------------------------------------

loc_1BA64:                              ; CODE XREF: sub_1B862+1F9↑j
                cmp     byte_1DC1F, 57h ; 'W'
                jnz     short loc_1BA70
                mov     [bp+var_2A], 23h ; '#'

loc_1BA70:                              ; CODE XREF: sub_1B862+1E3↑j
                                        ; sub_1B862+1F2↑j ...
                mov     ax, 27h ; '''
                push    ax
                mov     ax, 100h
                push    ax
                push    [bp+var_2A]
                push    word_1DBBC
                push    word_1DBBA
                call    thk_res_14FE
                add     sp, 0Ah

loc_1BA89:                              ; CODE XREF: sub_1B862+13A↑j
                mov     ax, 1
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     bx, [bp+var_20]
                cmp     byte ptr [bx], 0
                jnz     short loc_1BAA4
                mov     ax, 1783h
                jmp     short loc_1BAB3
; ---------------------------------------------------------------------------

loc_1BAA4:                              ; CODE XREF: sub_1B862+23B↑j
                cmp     byte_1DBED, 0
                jnz     short loc_1BAB0
                mov     ax, 178Eh
                jmp     short loc_1BAB3
; ---------------------------------------------------------------------------

loc_1BAB0:                              ; CODE XREF: sub_1B862+247↑j
                mov     ax, 1799h

loc_1BAB3:                              ; CODE XREF: sub_1B862+240↑j
                                        ; sub_1B862+24C↑j
                push    ax
                call    thk_res_1726
                add     sp, 2
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1B862       endp


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
                mov     bl, byte ptr word_1DBE2
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
                cmp     byte_1DBED, 0
                jnz     short loc_1BB40
                mov     bl, al
                sub     bh, bh
                shr     bx, 1
                shr     bx, 1
                mov     al, [bx+1720h]
                mov     [bp+var_2], al
                cmp     byte ptr word_1DBE2, 29h ; ')'
                jnz     short loc_1BB1E
                mov     [bp+var_2], 8
                jmp     short loc_1BB44
; ---------------------------------------------------------------------------
                align 2

loc_1BB1E:                              ; CODE XREF: sub_1BAC0+55↑j
                cmp     byte ptr word_1DBE2, 2Ah ; '*'
                jz      short loc_1BB2C
                cmp     byte ptr word_1DBE2, 2Bh ; '+'
                jnz     short loc_1BB32

loc_1BB2C:                              ; CODE XREF: sub_1BAC0+63↑j
                mov     [bp+var_2], 4
                jmp     short loc_1BB44
; ---------------------------------------------------------------------------

loc_1BB32:                              ; CODE XREF: sub_1BAC0+6A↑j
                cmp     byte ptr word_1DBE2, 2Ch ; ','
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
                                        ; sub_17E10+379↑p

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
                call    thk_res_1392
                add     sp, 2

loc_1BB6D:                              ; CODE XREF: sub_1BB4E+13↑j
                mov     ax, 4
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                call    thk_res_3204
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_1600
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 28h ; '('
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte ptr word_1DBE2+1
                sub     ah, ah
                push    ax
                call    thk_res_1940
                add     sp, 6
                mov     ax, 2Ch ; ','
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte ptr word_1DBE4
                sub     ah, ah
                push    ax
                call    thk_res_1940
                add     sp, 6
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                call    thk_res_5440
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_res_0BAA
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
                call    thk_res_14FE
                add     sp, 0Ah
                mov     al, [si+59D6h]
                and     al, 3
                mov     [bp+var_104], al
                or      al, al
                jz      short loc_1BCC5
                cmp     byte_1DBED, 0
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
                call    thk_res_14FE
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
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     al, byte ptr word_1DBE2+1
                sub     ah, ah
                add     si, ax
                mov     al, [bp+si+var_100]
                mov     [bp+var_102], al
                cmp     byte_1DC1F, 4Eh ; 'N'
                jnz     short loc_1BD0E
                mov     [bp+var_10E], 20h ; ' '
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD0E:                              ; CODE XREF: sub_1BB4E+1B5↑j
                cmp     byte_1DC1F, 53h ; 'S'
                jnz     short loc_1BD1E
                mov     [bp+var_10E], 21h ; '!'
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD1E:                              ; CODE XREF: sub_1BB4E+1C5↑j
                cmp     byte_1DC1F, 45h ; 'E'
                jnz     short loc_1BD2E
                mov     [bp+var_10E], 22h ; '"'
                jmp     short loc_1BD3B
; ---------------------------------------------------------------------------
                align 2

loc_1BD2E:                              ; CODE XREF: sub_1BB4E+1D5↑j
                cmp     byte_1DC1F, 57h ; 'W'
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
                call    thk_res_1392
                add     sp, 2

loc_1BD58:                              ; CODE XREF: sub_1BB4E+1F2↑j
                mov     al, byte ptr word_1DBE2+1
                sub     ah, ah
                mov     cl, 4
                shl     ax, cl
                add     ax, 20h ; ' '
                mov     [bp+var_10A], ax
                mov     al, byte ptr word_1DBE4
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
                call    thk_res_0BAA
                add     sp, 2
                mov     ax, [bp+var_10C]
                add     ax, 0Ah
                push    ax
                mov     ax, [bp+var_10A]
                add     ax, 0Fh
                push    ax
                push    [bp+var_10C]
                push    [bp+var_10A]
                call    thk_res_0C16
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
                call    thk_res_14FE
                add     sp, 0Ah

loc_1BDE5:                              ; CODE XREF: sub_1BB4E+278↑j
                xor     byte ptr [bp+var_112], 1
                mov     ax, 0FAh
                push    ax
                call    thk_res_1C66
                add     sp, 2
                call    thk_res_1A30
                mov     [bp+var_110], ax
                cmp     ax, 1Bh
                jnz     short loc_1BD82
                cmp     byte_1DBE6, 0
                jnz     short loc_1BE1E
                mov     ax, 1
                push    ax
                call    thk_res_1392
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_142A
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


sub_1BE24       proc near               ; CODE XREF: sub_1B5EA+EF↑p
                mov     al, byte_1DBE9
                cmp     byte ptr word_1DBE2, al
                jz      short locret_1BE90
                mov     al, byte ptr word_1DBE2
                mov     byte_1DBE9, al
                sub     ah, ah
                push    ax
                mov     ax, 5AD6h
                push    ax
                call    thk_res_60D0
                add     sp, 4
                mov     al, byte ptr word_1DBE2
                sub     ah, ah
                push    ax
                mov     ax, 59D6h
                push    ax
                call    thk_res_60B4
                add     sp, 4
                mov     al, byte_231DB
                sub     ah, ah
                push    ax
                mov     ax, 5BD6h
                push    ax
                call    thk_res_60B4
                add     sp, 4
                mov     al, byte_231DD
                sub     ah, ah
                push    ax
                mov     ax, 5CD6h
                push    ax
                call    thk_res_60B4
                add     sp, 4
                mov     al, byte_231DC
                sub     ah, ah
                push    ax
                mov     ax, 5DD8h
                push    ax
                call    thk_res_60B4
                add     sp, 4
                mov     al, byte_231DE
                sub     ah, ah
                push    ax
                mov     ax, 5ED8h
                push    ax
                call    thk_res_60B4
                add     sp, 4

locret_1BE90:                           ; CODE XREF: sub_1BE24+7↑j
                retn
sub_1BE24       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1BE92       proc near               ; CODE XREF: seg002:0969↑J
                push    si
                mov     bl, byte ptr word_1DBE2+1
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+1760h]
                mov     si, word_1DBE2
                and     si, 0FFh
                mov     cl, 5
                shl     si, cl
                mov     bl, byte ptr word_1DBE4
                sub     bh, bh
                shl     bx, 1
                or      [bx+si-6974h], ax
                pop     si
                retn
sub_1BE92       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1BEBA       proc near               ; CODE XREF: sub_18744+9A↑p
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

