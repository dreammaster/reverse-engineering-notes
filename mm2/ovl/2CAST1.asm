; ===========================================================================

; Segment type: Pure code
ovl_2CAST1      segment byte public 'CODE' use16
                assume cs:ovl_2CAST1
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C130       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 2, 56h, 0E8h, 0Ch, 0Fh, 0Bh, 0C0h
                                        ; DATA XREF: seg002:0038↑o
                db 74h, 70h, 0E8h, 1Bh, 0Eh, 0B8h, 14h, 0, 50h, 0B8h, 1
                db 0, 50h, 0E8h, 0E2h, 0ADh, 83h, 0C4h, 4, 0B8h, 6Ah, 30h
                db 50h, 0E8h, 0FCh, 0ADh, 83h, 0C4h, 2, 2Bh, 0F6h, 8Ah
                db 84h, 7Ah, 30h, 2Ah, 0E4h, 50h, 8Ah, 84h, 74h, 30h, 50h
                db 0E8h, 0C4h, 0ADh, 83h, 0C4h, 4, 8Bh, 0C6h, 5, 41h, 0
                db 50h, 0E8h, 0C4h, 0ADh, 83h, 0C4h, 2, 0B8h, 2Dh, 0, 50h
                db 0E8h, 0BAh, 0ADh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h
                db 1, 0, 50h, 8Bh, 1Eh, 0D6h, 5Dh, 8Ah, 2 dup(40h), 2Ah
                db 0E4h, 50h, 0E8h, 0EAh, 0ADh, 83h, 0C4h, 6, 46h, 83h
                db 0FEh, 6, 7Ch, 0BAh, 89h, 76h, 0FEh, 0E8h, 13h, 0ACh
                db 0Bh, 0C0h, 74h, 0F9h, 0E8h, 0F0h, 0Ch, 5Eh, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 0E8h, 8Fh, 0Eh, 0Bh, 0C0h, 74h, 16h
                db 80h, 3Eh, 0D5h, 3, 0FEh, 73h, 4, 0FEh, 6, 0D5h, 3, 0B0h
                db 1, 0A2h, 9Bh, 3, 0A2h, 95h, 3, 0E8h, 0CDh, 0Ch, 0C3h
                db 0E8h, 71h, 0Eh, 0Bh, 0C0h, 74h, 10h, 0E8h
byte_1C1DA      db 80h, 0Dh, 0E8h, 0D3h, 0B0h, 0C6h, 6, 95h, 3, 1, 0C6h
                                        ; CODE XREF: seg002:08CD↑J
                db 6, 9Bh, 3, 0, 0C3h
sub_1C130       endp ; sp-analysis failed


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1EA       proc near               ; CODE XREF: seg002:07F5↑J
                                        ; sub_1D0C2:loc_1D0E4↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C23A
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_2], al
                cmp     byte_1DC30, 0FAh
                jbe     short loc_1C20D
                mov     byte_1DC30, 0FFh

loc_1C20D:                              ; CODE XREF: sub_1C1EA+1C↑j
                mov     dl, [bp+var_2]
                mov     cl, byte_1DC30
                jmp     short loc_1C21E
; ---------------------------------------------------------------------------

loc_1C216:                              ; CODE XREF: sub_1C1EA+3A↓j
                cmp     cl, 0FAh
                jnb     short loc_1C21E
                add     cl, 5

loc_1C21E:                              ; CODE XREF: sub_1C1EA+2A↑j
                                        ; sub_1C1EA+2F↑j
                mov     al, dl
                dec     dl
                or      al, al
                jnz     short loc_1C216
                mov     [bp+var_2], dl
                mov     byte_1DC30, cl
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

loc_1C23A:                              ; CODE XREF: sub_1C1EA+B↑j
                mov     sp, bp

loc_1C23C:                              ; CODE XREF: seg002:08E5↑J
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1C23E:                              ; CODE XREF: sub_1D0C2:loc_1D0EA↓p
                push    bp
                mov     bp, sp
                sub     sp, 0Ah         ; CODE XREF: seg002:0639↑J
sub_1C1EA       endp

                push    si
                mov     word ptr [bp-8], 0
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C254
                jmp     loc_1C31B
; ---------------------------------------------------------------------------

loc_1C254:                              ; CODE XREF: ovl_2CAST1:C24F↑j
                cmp     byte_1DBED, 1
                jz      short loc_1C286
                mov     si, word_1DBE4
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, byte_1DBE3
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                sub     ah, ah
                mov     cl, byte_23216
                sub     ch, ch
                test    ax, cx
                jnz     short loc_1C286
                mov     al, byte_23218
                and     al, cl
                test    al, 55h
                jz      short loc_1C289

loc_1C286:                              ; CODE XREF: ovl_2CAST1:C259↑j
                                        ; ovl_2CAST1:C27B↑j
                inc     word ptr [bp-8]

loc_1C289:                              ; CODE XREF: ovl_2CAST1:C284↑j
                cmp     word ptr [bp-8], 0
                jnz     short loc_1C2EF
                lea     ax, [bp-4]
                push    ax
                lea     ax, [bp-2]
                push    ax
                call    thk_res_428C
                add     sp, 4
                mov     al, [bp-2]
                add     al, byte_1DBE3
                and     al, 0Fh
                mov     [bp-6], al
                mov     al, [bp-4]
                add     al, byte ptr word_1DBE4
                and     al, 0Fh
                mov     [bp-0Ah], al
                mov     si, [bp-0Ah]
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl

loc_1C2C0:                              ; CODE XREF: seg002:026D↑J
                mov     bl, [bp-6]
                sub     bh, bh
                mov     al, [bx+si+59D6h]
                sub     ah, ah
                mov     cl, byte_23216
                sub     ch, ch
                test    ax, cx
                jnz     short loc_1C2EC
                mov     si, [bp-0Ah]
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     al, [bx+si+5AD6h]
                and     al, byte_23216
                test    al, 55h
                jz      short loc_1C2EF

loc_1C2EC:                              ; CODE XREF: ovl_2CAST1:C2D3↑j
                inc     word ptr [bp-8]

loc_1C2EF:                              ; CODE XREF: ovl_2CAST1:C28D↑j
                                        ; ovl_2CAST1:C2EA↑j
                cmp     word ptr [bp-8], 0
                jz      short loc_1C2FA
                call    near ptr byte_1CEC8+32h

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                jmp     short loc_1C31B
; ---------------------------------------------------------------------------

loc_1C2FA:                              ; CODE XREF: ovl_2CAST1:C2F3↑j
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr word_1DBE4+1, al
                call    near ptr byte_1CE30+6Eh
                mov     al, [bp-6]

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                add     al, [bp-2]
                and     al, 0Fh
                mov     byte_1DBE3, al
                mov     al, [bp-0Ah]
                add     al, [bp-4]
                and     al, 0Fh
                mov     byte ptr word_1DBE4, al

loc_1C31B:                              ; CODE XREF: ovl_2CAST1:C251↑j
                                        ; ovl_2CAST1:loc_1C2F8↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn

; =============== S U B R O U T I N E =======================================


sub_1C320       proc near               ; CODE XREF: sub_1D0C2:loc_1D0F0↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C33F
                cmp     byte_1DC28, 0FFh
                jnb     short loc_1C332
                inc     byte_1DC28

loc_1C332:                              ; CODE XREF: sub_1C320+C↑j
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

locret_1C33F:                           ; CODE XREF: sub_1C320+5↑j
                retn
sub_1C320       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C340       proc near               ; CODE XREF: sub_1D0C2:loc_1D0F6↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     [bp+var_2], 0
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C355
                jmp     loc_1C3E9
; ---------------------------------------------------------------------------

loc_1C355:                              ; CODE XREF: sub_1C340+10↑j
                test    byte_231F0, 40h
                jz      short loc_1C362

loc_1C35C:                              ; CODE XREF: sub_1C340+5A↓j
                inc     [bp+var_2]
                jmp     short loc_1C3DA
; ---------------------------------------------------------------------------
                align 2

loc_1C362:                              ; CODE XREF: sub_1C340+1A↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h
                push    ax

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3080h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1C35C
                cmp     ax, 31h ; '1'
                jnz     short loc_1C3B8
                mov     al, byte_1DBE2
                mov     byte_1DC38, al
                mov     al, byte ptr word_1DBE4
                mov     cl, 4
                shl     al, cl
                add     al, byte_1DBE3
                mov     byte_1DC39, al
                jmp     short loc_1C3DA
; ---------------------------------------------------------------------------
                align 2

loc_1C3B8:                              ; CODE XREF: sub_1C340+5F↑j
                mov     al, byte_1DC38
                mov     byte_1DBE2, al
                mov     al, byte_1DC39
                and     al, 0Fh
                mov     byte_1DBE3, al
                mov     al, byte_1DC39
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr word_1DBE4, al
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr word_1DBE4+1, al

loc_1C3DA:                              ; CODE XREF: sub_1C340+1F↑j
                                        ; sub_1C340+75↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C3E6
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C3E9
; ---------------------------------------------------------------------------
                align 2

loc_1C3E6:                              ; CODE XREF: sub_1C340+9E↑j
                call    near ptr byte_1CE30+6Eh

loc_1C3E9:                              ; CODE XREF: sub_1C340+12↑j
                                        ; sub_1C340+A3↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C340       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C3EE       proc near               ; CODE XREF: sub_1D0C2:loc_1D102↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si

loc_1C3F6:                              ; CODE XREF: seg002:047D↑J
                mov     [bp+var_2], 0
                mov     [bp+var_8], 0
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C40A
                jmp     loc_1C4F5
; ---------------------------------------------------------------------------

loc_1C40A:                              ; CODE XREF: sub_1C3EE+17↑j
                test    byte_231F0, 40h
                jz      short loc_1C418

loc_1C411:                              ; CODE XREF: sub_1C3EE+8D↓j
                                        ; sub_1C3EE+C6↓j
                inc     [bp+var_2]
                jmp     loc_1C4E7
; ---------------------------------------------------------------------------
                align 2

loc_1C418:                              ; CODE XREF: sub_1C3EE+21↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 4
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 30A7h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C43A:                              ; CODE XREF: sub_1C3EE+82↓j
                call    thk_res_1A30
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     di, ax
                cmp     di, 1Bh
                jnz     short loc_1C450
                mov     ax, 1
                jmp     short loc_1C452
; ---------------------------------------------------------------------------

loc_1C450:                              ; CODE XREF: sub_1C3EE+5B↑j
                sub     ax, ax

loc_1C452:                              ; CODE XREF: sub_1C3EE+60↑j
                mov     si, ax
                or      si, si
                jnz     short loc_1C46E
                mov     ax, di
                cmp     ax, 41h ; 'A'
                jb      short loc_1C46A
                cmp     ax, 45h ; 'E'

loc_1C462:                              ; CODE XREF: seg002:086D↑J
                ja      short loc_1C46A
                mov     ax, 1
                jmp     short loc_1C46C
; ---------------------------------------------------------------------------
                align 2

loc_1C46A:                              ; CODE XREF: sub_1C3EE+6F↑j
                                        ; sub_1C3EE:loc_1C462↑j
                sub     ax, ax

loc_1C46C:                              ; CODE XREF: sub_1C3EE+79↑j
                mov     si, ax

loc_1C46E:                              ; CODE XREF: sub_1C3EE+68↑j
                or      si, si
                jz      short loc_1C43A
                mov     [bp+var_6], di
                mov     [bp+var_8], si
                cmp     di, 1Bh
                jz      short loc_1C411
                push    di
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_res_1676    ; CODE XREF: seg002:0B31↑J
                add     sp, 4
                mov     ax, 30B5h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C4B7
                jmp     loc_1C411
; ---------------------------------------------------------------------------

loc_1C4B7:                              ; CODE XREF: sub_1C3EE+C4↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2
                sub     [bp+var_4], 31h ; '1'
                sub     [bp+var_6], 41h ; 'A'
                mov     al, 0FFh
                mov     byte ptr word_1DBE4, al
                mov     byte_1DBE3, al
                mov     si, [bp+var_6]
                shl     si, 1
                shl     si, 1
                mov     bx, [bp+var_4]
                mov     al, [bx+si+30BCh]
                mov     byte_1DBE2, al
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr word_1DBE4+1, al

loc_1C4E7:                              ; CODE XREF: sub_1C3EE+26↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C4F2
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C4F5
; ---------------------------------------------------------------------------

loc_1C4F2:                              ; CODE XREF: sub_1C3EE+FD↑j
                call    near ptr byte_1CE30+6Eh

loc_1C4F5:                              ; CODE XREF: sub_1C3EE+19↑j
                                        ; sub_1C3EE+102↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C3EE       endp

; ---------------------------------------------------------------------------
                align 2

loc_1C4FC:                              ; CODE XREF: sub_1D0C2:loc_1D108↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C54C
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp-2], al
                cmp     byte_1DC31, 0FAh
                jbe     short loc_1C51F
                mov     byte_1DC31, 0FFh

loc_1C51F:                              ; CODE XREF: ovl_2CAST1:C518↑j
                mov     dl, [bp-2]
                mov     cl, byte_1DC31
                jmp     short loc_1C530
; ---------------------------------------------------------------------------

loc_1C528:                              ; CODE XREF: ovl_2CAST1:C536↓j
                cmp     cl, 0FAh
                jnb     short loc_1C530 ; CODE XREF: seg002:0879↑J
                add     cl, 5

loc_1C530:                              ; CODE XREF: ovl_2CAST1:C526↑j
                                        ; ovl_2CAST1:C52B↑j
                mov     al, dl
                dec     dl
                or      al, al
                jnz     short loc_1C528
                mov     [bp-2], dl
                mov     byte_1DC31, cl
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

loc_1C54C:                              ; CODE XREF: ovl_2CAST1:C507↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                call    loc_1D046       ; CODE XREF: sub_1D0C2:loc_1D10E↓p
                or      ax, ax
                jz      short locret_1C56F
                cmp     byte_1DC2A, 0FFh
                jnb     short loc_1C562
                inc     byte_1DC2A

loc_1C562:                              ; CODE XREF: ovl_2CAST1:C55C↑j
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

locret_1C56F:                           ; CODE XREF: ovl_2CAST1:C555↑j
                retn
; ---------------------------------------------------------------------------
                call    loc_1D046       ; CODE XREF: sub_1D0C2:loc_1D114↓p
                or      ax, ax
                jz      short locret_1C58F
                cmp     byte_1DC64, 0FFh
                jnb     short loc_1C582
                inc     byte_1DC64

loc_1C582:                              ; CODE XREF: ovl_2CAST1:C57C↑j
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

locret_1C58F:                           ; CODE XREF: ovl_2CAST1:C575↑j
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: sub_1D0C2:loc_1D11A↓p
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     word ptr [bp-6], 0
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C5A6
                jmp     loc_1C643
; ---------------------------------------------------------------------------

loc_1C5A6:                              ; CODE XREF: ovl_2CAST1:C5A1↑j
                test    byte_231F0, 10h
                jz      short loc_1C5B4 ; CODE XREF: seg002:0885↑J

loc_1C5AD:                              ; CODE XREF: ovl_2CAST1:C5EC↓j
                inc     word ptr [bp-6]
                jmp     loc_1C634
; ---------------------------------------------------------------------------
                align 2

loc_1C5B4:                              ; CODE XREF: ovl_2CAST1:C5AB↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h         ; CODE XREF: seg002:029D↑J
                push    ax
                mov     ax, 0Dh
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 30D0h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 39h ; '9'   ; CODE XREF: seg002:01A1↑J
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp-8], ax
                cmp     ax, 1Bh
                jz      short loc_1C5AD
                sub     word ptr [bp-8], 30h ; '0'
                lea     ax, [bp-4]
                push    ax
                lea     ax, [bp-2]
                push    ax
                call    thk_res_428C
                add     sp, 4
                mov     si, [bp-8]
                mov     dl, byte_1DBE3
                mov     cl, byte ptr word_1DBE4
                jmp     short loc_1C61A
; ---------------------------------------------------------------------------
                align 2

loc_1C60E:                              ; CODE XREF: ovl_2CAST1:C61F↓j
                add     dl, [bp-2]
                add     cl, [bp-4]
                and     dl, 0Fh
                and     cl, 0Fh

loc_1C61A:                              ; CODE XREF: ovl_2CAST1:C60B↑j
                mov     ax, si
                dec     si
                or      ax, ax
                jnz     short loc_1C60E
                mov     [bp-8], si
                mov     byte_1DBE3, dl
                mov     byte ptr word_1DBE4, cl
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr word_1DBE4+1, al

loc_1C634:                              ; CODE XREF: ovl_2CAST1:C5B0↑j
                cmp     word ptr [bp-6], 0
                jnz     short loc_1C640
                call    near ptr byte_1CE30+6Eh
                jmp     short loc_1C643
; ---------------------------------------------------------------------------
                align 2

loc_1C640:                              ; CODE XREF: ovl_2CAST1:C638↑j
                call    near ptr byte_1CEC8+32h

loc_1C643:                              ; CODE XREF: ovl_2CAST1:C5A3↑j
                                        ; ovl_2CAST1:C63D↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: sub_1D0C2:loc_1D120↓p
                mov     bp, sp
                sub     sp, 2
                push    si
                call    near ptr byte_1CB40+8
                mov     [bp-2], ax
                cmp     ax, 1Bh
                jz      short loc_1C687
                mov     si, ax
                mov     bx, word_23626
                cmp     byte ptr [bx+si+40h], 0
                jnz     short loc_1C66C
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C687
; ---------------------------------------------------------------------------
                align 2

loc_1C66C:                              ; CODE XREF: ovl_2CAST1:C664↑j
                                        ; seg002:0891↑J
                mov     ax, 6
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     si, [bp-2]
                mov     bx, word_23626
                add     [bx+si+40h], al
                call    near ptr byte_1CE30+6Eh

loc_1C687:                              ; CODE XREF: ovl_2CAST1:C658↑j
                                        ; ovl_2CAST1:C669↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: sub_1D0C2:loc_1D126↓p
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     word ptr [bp-2], 0
                call    near ptr byte_1CB40+8
                mov     [bp-6], ax
                cmp     ax, 1Bh
                jz      short loc_1C702
                mov     si, ax
                mov     bx, word_23626
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C6B6

loc_1C6B0:                              ; CODE XREF: ovl_2CAST1:C6CE↓j
                                        ; ovl_2CAST1:C6E7↓j
                inc     word ptr [bp-2]
                jmp     short loc_1C702
; ---------------------------------------------------------------------------
                align 2

loc_1C6B6:                              ; CODE XREF: ovl_2CAST1:C6AE↑j
                sub     cx, cx
                mov     dx, word_23626

loc_1C6BC:                              ; CODE XREF: ovl_2CAST1:C6D6↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C6D0

loc_1C6C6:                              ; CODE XREF: ovl_2CAST1:C6D4↓j
                mov     [bp-4], cx
                cmp     cx, 6
                jnz     short loc_1C6D8
                jmp     short loc_1C6B0
; ---------------------------------------------------------------------------

loc_1C6D0:                              ; CODE XREF: ovl_2CAST1:C6C4↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C6C6
                jmp     short loc_1C6BC
; ---------------------------------------------------------------------------

loc_1C6D8:                              ; CODE XREF: ovl_2CAST1:C6CC↑j
                mov     si, [bp-6]
                mov     bx, word_23626
                mov     al, [bx+si+3Ah]
                mov     [bp-8], al
                cmp     al, 0D0h
                jnb     short loc_1C6B0
                mov     si, [bp-4]
                add     si, bx
                mov     [si+3Ah], al
                mov     di, [bp-6]      ; CODE XREF: seg002:0B19↑J
                add     di, bx
                mov     al, [di+40h]
                mov     [si+40h], al
                mov     al, [di+46h]
                mov     [si+46h], al

loc_1C702:                              ; CODE XREF: ovl_2CAST1:C6A2↑j
                                        ; ovl_2CAST1:C6B3↑j
                cmp     word ptr [bp-2], 0
                jz      short loc_1C70E
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C71C
; ---------------------------------------------------------------------------
                align 2

loc_1C70E:                              ; CODE XREF: ovl_2CAST1:C706↑j
                cmp     word ptr [bp-6], 1Bh
                jz      short loc_1C71C
                call    near ptr byte_1CE30+6Eh
                mov     byte_1DBE7, 1

loc_1C71C:                              ; CODE XREF: ovl_2CAST1:C70B↑j
                                        ; ovl_2CAST1:C712↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp              ; CODE XREF: sub_1D0C2:loc_1D12C↓p
                mov     bp, sp
                sub     sp, 4
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C76F
                test    byte_231F0, 20h
                jz      short loc_1C73C
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C76F ; CODE XREF: seg002:089D↑J
; ---------------------------------------------------------------------------
                align 2

loc_1C73C:                              ; CODE XREF: ovl_2CAST1:C734↑j
                lea     ax, [bp-4]
                push    ax
                lea     ax, [bp-2]
                push    ax
                call    thk_res_428C
                add     sp, 4
                mov     al, [bp-2]
                add     byte_1DBE3, al
                mov     al, [bp-4]
                add     byte ptr word_1DBE4, al
                and     byte_1DBE3, 0Fh
                and     byte ptr word_1DBE4, 0Fh
                mov     byte ptr word_1DBE4+1, 1
                mov     byte_1DBEB, 0
                call    near ptr byte_1CE30+6Eh

loc_1C76F:                              ; CODE XREF: ovl_2CAST1:C72D↑j
                                        ; ovl_2CAST1:C739↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp              ; CODE XREF: sub_1D0C2:loc_1D132↓p
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     word ptr [bp-4], 0
                call    near ptr byte_1CB40+8
                mov     [bp-8], ax
                cmp     ax, 1Bh
                jz      short loc_1C7D5
                mov     si, ax
                mov     bx, word_23626
                mov     al, [bx+si+46h]
                mov     [bp-0Ah], al
                and     al, 0C0h
                mov     [bp-6], al
                mov     al, [bp-0Ah]
                and     al, 3Fh
                mov     [bp-2], al
                mov     al, 32h ; '2'
                mul     byte ptr [bp-2]
                mov     word_2765C, ax
                cmp     [bx+58h], ax
                jnb     short loc_1C7B6
                call    near ptr byte_1CEC8+32h
                jmp     short loc_1C7D5
; ---------------------------------------------------------------------------

loc_1C7B6:                              ; CODE XREF: ovl_2CAST1:C7AF↑j
                cmp     byte ptr [bp-2], 3Fh ; '?'
                ja      short loc_1C7BF
                inc     byte ptr [bp-2]

loc_1C7BF:                              ; CODE XREF: ovl_2CAST1:C7BA↑j
                mov     al, [bp-6]
                or      [bp-2], al
                mov     si, [bp-8]
                mov     bx, word_23626
                mov     al, [bp-2]
                mov     [bx+si+46h], al
                call    near ptr byte_1CE30+6Eh

loc_1C7D5:                              ; CODE XREF: ovl_2CAST1:C789↑j
                                        ; ovl_2CAST1:C7B4↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db  55h ; U             ; CODE XREF: sub_1D0C2:loc_1D156↓p
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    8
                db  56h ; V
                db 0C7h
byte_1C7E2      db 46h, 0FCh, 2 dup(0), 0C7h, 46h
                                        ; CODE XREF: seg002:0849↑J
byte_1C7E8      db 0FAh, 0Ch, 0, 0E8h, 58h, 8, 0Bh, 0C0h, 75h, 3, 0E9h
                                        ; CODE XREF: seg002:0B0D↑J
                db 82h, 0, 0F6h, 6, 0A0h, 59h, 40h, 74h, 6, 0FFh, 46h
                db 0FCh, 0EBh, 5Ch, 90h, 83h, 3Eh, 0CAh, 3, 9, 75h, 0F3h
                db 0A1h, 0B4h, 3, 89h, 46h, 0FEh, 0F6h, 46h, 0FEh, 1, 74h
                db 0Eh, 2Bh, 0C9h, 0BEh, 0E0h, 30h, 8Bh, 0D0h, 39h, 14h
                db 7Eh, 48h, 89h, 4Eh, 0FAh, 81h, 7Eh, 0FEh, 96h, 0, 7Ch
                db 0Bh, 0C7h, 6, 0CAh, 3, 8, 0, 0C7h, 46h, 0FAh, 0Bh, 0
                db 8Bh, 5Eh, 0FAh, 8Ah, 87h, 0FAh, 30h, 0A2h, 92h, 3, 8Ah
                db 87h, 8, 31h, 24h, 0Fh, 0A2h, 93h, 3, 8Ah, 87h, 8, 31h
                db 2Ah, 0E4h, 0B1h, 4, 0D3h, 0E8h, 0A2h, 94h, 3, 0B0h
                db 1, 0A2h, 30h, 4, 0A2h, 95h, 3, 83h, 7Eh, 0FCh, 0, 74h
                db 11h, 0E8h, 94h, 6, 0EBh, 0Fh, 83h, 0C6h, 2, 41h, 83h
                db 0F9h, 0Dh, 7Dh, 0AFh, 0EBh, 0A9h, 90h, 0E8h, 27h, 6
                db 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 0E8h, 0C7h, 7, 0Bh, 0C0h
                db 74h, 1Ch, 8Bh, 1Eh, 0D6h, 5Dh, 80h, 7Fh, 25h, 28h, 73h
                db 0Fh, 80h, 47h, 25h, 8, 0E8h, 0Ah, 6, 0C6h, 6, 97h, 3
                db 1, 0EBh, 4, 90h, 0E8h, 5Bh, 6, 0C3h, 0E8h, 0A3h, 7
                db 0Bh, 0C0h, 74h, 1Fh, 80h, 3Eh, 0D5h, 3, 0EBh, 76h, 8
                db 0C6h, 6, 0D5h, 3, 0FFh, 0EBh, 6, 90h, 80h, 6, 0D5h
                db 3, 14h, 0B0h, 1, 0A2h, 9Bh, 3, 0A2h, 95h, 3, 0E8h, 0D8h
                db 5, 0C3h, 90h, 0E8h, 7Bh, 7, 0Bh, 0C0h, 74h, 10h, 0B0h
                db 1, 0A2h, 0D9h, 3, 0A2h, 95h, 3, 0C6h, 6, 9Bh, 3, 0
                db 0E8h, 0BFh, 5, 0C3h, 0E8h, 63h, 7, 0Bh, 0C0h, 74h, 8
                db 0C6h, 6, 0DDh, 3, 1, 0E8h, 0AFh, 5, 0C3h, 55h, 8Bh
                db 0ECh, 83h, 0ECh, 4, 56h, 0E8h, 92h, 6, 89h, 46h, 0FCh
                db 3Dh, 1Bh, 0, 74h, 23h, 50h, 0E8h, 5Ch, 0A8h, 83h, 0C4h
                db 2, 89h, 46h, 0FEh, 8Bh, 0D8h, 8Bh, 0F0h, 8Ah, 44h, 0Dh
                db 88h, 47h, 6Ah, 0E8h, 85h, 5, 0A1h, 0D6h, 5Dh, 3Bh, 0F0h
                db 75h, 5, 0C6h, 6, 97h, 3, 1, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h
                db 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 0C7h, 46h, 0FEh, 2 dup(0)
                db 0E8h, 0Eh, 7, 0Bh, 0C0h, 74h, 43h, 0F6h, 6, 0A0h, 59h
                db 40h, 74h, 5, 0FFh, 46h, 0FEh, 0EBh, 29h, 80h, 3Eh, 9Ch
                db 59h, 0, 74h, 0F4h, 0A0h, 9Ch, 59h, 24h, 0Fh, 0A2h, 93h
                db 3, 0A0h, 9Ch, 59h, 2Ah, 0E4h, 0B1h, 4, 0D3h, 0E8h, 0A2h
                db 94h, 3, 0A0h, 9Eh, 59h, 0A2h, 92h, 3, 0B0h, 1, 0A2h
                db 9Bh, 3, 0A2h, 95h, 3, 83h, 7Eh, 0FEh, 0, 74h, 5, 0E8h
                db 80h, 5, 0EBh, 3, 0E8h, 1Fh, 5, 8Bh, 0E5h, 5Dh, 0C3h
                db 90h, 0E8h, 0BFh, 6, 0Bh, 0C0h, 74h, 8, 0C6h, 6, 0DFh
                db 3, 1, 0E8h, 0Bh, 5, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 6
byte_1C99A      db 0E8h, 0EFh, 5, 89h, 46h, 0FAh, 3Dh, 1Bh, 0, 74h, 57h
                                        ; CODE XREF: seg002:07DD↑J
                db 50h, 0E8h, 0B9h, 0A7h, 83h, 0C4h, 2, 89h, 46h, 0FEh
                db 0B8h, 0Ah, 0, 50h, 0B8h, 1, 0, 50h, 0E8h, 0BCh, 0A5h
                db 83h, 0C4h, 4, 88h, 46h, 0FCh, 0B8h, 64h, 0, 50h, 0B8h
                db 1, 0, 50h, 0E8h, 0ABh, 0A5h, 83h, 0C4h, 4, 3Dh, 32h
                db 0, 7Dh, 9, 8Bh, 5Eh, 0FEh, 80h, 7Fh, 21h, 12h, 73h
                db 14h, 8Ah, 46h, 0FCh, 2Ah, 0E4h, 50h, 0FFh, 76h, 0FEh
                db 0E8h
byte_1C9E6      db 0D2h, 0ACh, 83h, 0C4h, 4, 0E8h, 0Ch, 5, 0EBh, 0Ch, 8Bh
                                        ; CODE XREF: seg002:0B01↑J
                db 5Eh, 0FEh, 8Ah, 46h, 0FCh, 28h, 47h, 21h, 0E8h, 0A2h
                db 4, 8Bh, 0E5h, 5Dh, 0C3h, 0E8h, 43h, 6, 0Bh, 0C0h, 74h
                db 8, 0C6h, 6, 0DCh, 3, 1, 0E8h, 8Fh, 4, 0C3h, 0E8h, 33h
                db 6, 0Bh, 0C0h, 74h, 8, 0C6h, 6, 0DEh, 3, 1, 0E8h, 7Fh
                db 4, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0C7h, 46h, 0FEh
                db 2 dup(0), 0E8h, 18h, 6, 0Bh, 0C0h, 74h, 6Dh, 0F6h, 6
                db 0A0h, 59h, 40h, 74h, 5, 0FFh, 46h, 0FEh, 0EBh, 52h
                db 0B8h, 1, 0, 50h, 0E8h, 0B1h, 0A3h, 83h, 0C4h, 2, 0B8h
                db 15h, 0, 50h, 0B8h, 0Ah, 0, 50h, 0E8h, 0DBh
byte_1CA52      db 0A4h, 83h, 0C4h, 4, 0B8h, 15h, 31h, 50h, 0E8h, 0F5h
                                        ; CODE XREF: seg002:0A89↑J
                db 0A4h, 83h, 0C4h, 2, 0B8h, 35h, 0, 50h, 0B8h, 31h, 0
                db 50h, 0E8h, 57h, 0A4h, 83h, 0C4h, 4, 2Ah, 0E4h, 89h
                db 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 0C1h, 8Ah, 46h, 0FCh, 2Ch
                db 31h, 0A2h, 92h, 3, 0B0h, 0FFh, 0A2h, 94h, 3, 0A2h, 93h
                db 3
byte_1CA88      db 0B0h, 1, 0A2h, 9Bh, 3, 0A2h, 95h, 3, 83h, 7Eh, 0FEh
                                        ; CODE XREF: seg002:0801↑J
                db 0, 74h, 6, 0E8h, 61h, 4, 0EBh, 4, 90h, 0E8h, 0FFh, 3
                db 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 6, 56h, 0E8h, 0DEh, 4, 89h, 46h, 0FAh, 3Dh, 1Bh, 0
                db 74h, 55h, 50h, 0E8h, 0A8h, 0A6h, 83h, 0C4h, 2, 89h
                db 46h, 0FCh, 8Bh, 0D8h, 80h, 7Fh, 26h, 80h, 73h, 6, 0E8h
                db 2Fh, 4, 0EBh, 3Eh, 90h, 0B8h, 1, 0, 50h, 0FFh, 36h
                db 0D6h, 5Dh, 0E8h, 0E1h, 0ABh, 83h, 0C4h, 4, 0B8h, 5
                db 0, 50h, 0FFh, 76h, 0FCh, 0E8h, 0D4h, 0ABh, 83h, 0C4h
                db 4, 8Bh, 5Eh, 0FCh, 8Ah, 47h, 27h, 88h, 46h, 0FEh, 0Ah
                db 0C0h, 74h, 0D2h, 0FEh, 4Eh, 0FEh, 8Bh, 0F3h, 8Ah, 46h
                db 0FEh, 88h, 44h, 73h, 88h, 47h, 27h, 0C6h, 47h, 26h
                db 0, 0E8h, 93h, 3, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh
                db 0ECh, 83h, 0ECh, 2, 56h, 0E8h, 2Eh, 0, 89h, 46h, 0FEh
                db 3Dh, 1Bh, 0, 74h, 20h, 8Bh, 0F0h, 8Bh, 1Eh, 0D6h, 5Dh
                db 80h, 78h, 40h, 0FFh, 75h, 6, 0E8h, 0C9h, 3, 0EBh, 0Fh
                db 90h, 8Bh, 76h, 0FEh, 8Bh, 1Eh, 0D6h, 5Dh, 0C6h, 2 dup(40h)
                db 1, 0E8h
byte_1CB40      db 5Ch, 3, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh
                                        ; CODE XREF: seg002:0A65↑J
                                        ; ovl_2CAST1:C64F↑p ...
                db 83h, 0ECh, 4, 57h, 56h, 0E8h, 9, 4, 0B8h, 15h, 0, 50h
                db 0B8h, 18h, 0, 50h, 0E8h, 0D0h, 0A3h, 83h, 0C4h, 4, 0B8h
                db 21h, 31h, 50h, 0E8h, 0EAh, 0A3h, 83h, 0C4h, 2, 0E8h
                db 4Ch, 0A2h, 50h, 0E8h, 0B0h, 0A3h, 83h, 0C4h, 2, 8Bh
                db 0F8h, 83h, 0FFh, 1Bh, 75h, 6, 0B8h, 1, 0, 0EBh, 3, 90h
                db 2Bh, 0C0h, 8Bh, 0F0h, 0Bh, 0F6h, 75h, 27h
byte_1CB8A      db 8Bh, 0C7h, 3Dh, 41h, 0, 72h, 0Bh, 3Dh, 46h, 0, 77h
                                        ; CODE XREF: seg002:0AF5↑J
                db 6, 0B8h, 1, 0, 0EBh, 3, 90h
byte_1CB9C      db 2Bh, 0C0h, 8Bh, 0F0h, 0Bh, 0F6h, 74h, 0Dh, 8Bh, 1Eh
                                        ; CODE XREF: seg002:080D↑J
                db 0D6h, 5Dh, 80h, 79h, 0F9h, 1, 1Bh, 0C0h, 40h, 8Bh, 0F0h
                db 0Bh, 0F6h, 74h, 0B6h, 89h, 7Eh, 0FCh, 89h, 76h, 0FEh
                db 83h, 0FFh, 1Bh, 74h, 0Ch, 83h, 6Eh, 0FCh, 41h, 0C6h
                db 6, 28h, 4, 1, 0EBh, 9, 90h, 2Bh, 0C0h, 0A3h, 0Ch, 9Eh
                db 0A3h, 46h, 9Eh, 0F6h, 6, 0C8h, 59h, 2, 74h, 8, 0C7h
byte_1CBDC      db 46h, 0FCh, 1Bh, 0, 0E8h, 17h, 3, 8Bh, 46h, 0FCh, 5Eh
                                        ; CODE XREF: seg002:01AD↑J
                db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 6, 57h, 56h, 0E8h, 4Fh, 4, 0Bh, 0C0h, 74h, 39h, 2Bh
                db 0F6h, 8Bh, 7Eh, 0FCh, 0EBh, 1Eh, 56h, 0E8h, 5Ch, 0A5h
                db 83h, 0C4h, 2, 8Bh, 0F8h, 8Ah, 45h, 26h, 88h, 46h, 0FEh
                db 3Ch, 80h, 73h, 4, 80h, 66h, 0FEh, 6Fh, 8Ah, 46h, 0FEh
                db 88h, 45h, 26h, 46h, 3Bh, 36h, 26h, 4, 7Ch, 0DCh, 89h
                db 7Eh, 0FCh, 89h, 76h, 0FAh, 0C6h, 6, 97h, 3, 1, 0E8h
                db 6Ah, 2, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 0E8h, 9, 4
                db 0Bh, 0C0h, 74h, 19h, 8Bh, 1Eh, 0D6h, 5Dh, 8Ah, 47h
                db 71h, 4, 0Ah, 0A2h, 0D6h, 3, 0C6h, 6, 95h, 3, 1, 0C6h
                db 6, 9Bh, 3, 0, 0E8h, 44h, 2, 0C3h, 90h, 0B8h, 8, 0, 50h
                db 0E8h, 0E3h, 1, 83h, 0C4h, 2, 0C3h, 90h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 4, 56h, 2Bh, 0F6h, 8Bh, 1Eh, 0D6h, 5Dh, 8Ah
                db 47h, 71h, 88h, 46h, 0FEh, 0EBh, 11h, 90h, 0B8h, 0Ah
                db 0, 50h, 0B8h, 1, 0, 50h, 0E8h, 0EDh, 0A2h, 83h, 0C4h
                db 4, 3, 0F0h, 8Ah, 46h, 2 dup(0FEh), 4Eh, 0FEh, 0Ah, 0C0h
                db 75h, 0E6h, 89h, 76h, 0FCh, 56h, 0E8h, 0A7h, 1, 83h
                db 0C4h, 2, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 0B8h, 0Fh
                db 0, 50h, 0E8h, 97h, 1, 83h, 0C4h, 2, 0C3h, 90h, 0E8h
                db 8Fh, 3, 0Bh, 0C0h, 74h
byte_1CCBA      db 19h, 8Bh, 1Eh, 0D6h, 5Dh, 8Ah, 47h, 71h, 4, 14h, 0A2h
                                        ; CODE XREF: seg002:08FD↑J
                db 0D7h, 3, 0C6h, 6, 95h, 3, 1, 0C6h, 6, 9Bh, 3, 0, 0E8h
                db 0CAh, 1, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h
                db 0ADh, 2, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 2Ah, 50h
                db 0E8h, 77h, 0A4h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh
                db 0D8h, 80h, 7Fh, 26h, 80h, 73h, 15h, 80h, 67h, 26h, 77h
                db 0A1h, 0D6h, 5Dh, 3Bh, 0D8h, 75h, 5, 0C6h, 6, 97h, 3
                db 1, 0E8h, 92h, 1, 0EBh, 3, 0E8h, 0E9h, 1, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h
                db 6Dh, 2, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 2Ah, 50h
                db 0E8h, 37h, 0A4h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh
                db 0D8h, 80h, 7Fh, 26h, 80h, 73h, 15h, 80h, 67h, 26h, 7Bh
                db 0A1h, 0D6h, 5Dh, 3Bh, 0D8h, 75h, 5, 0C6h, 6, 97h, 3
                db 1, 0E8h, 52h, 1, 0EBh, 3, 0E8h, 0A9h, 1, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h
                db 2Dh, 2, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 2Ah, 50h
                db 0E8h, 0F7h, 0A3h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh
                db 0D8h, 80h, 7Fh, 26h, 80h, 73h, 15h, 0C6h, 47h, 26h
                db 0, 0A1h, 0D6h, 5Dh, 3Bh, 0D8h, 75h, 5, 0C6h, 6, 97h
                db 3, 1, 0E8h, 12h, 1, 0EBh, 3, 0E8h, 69h, 1, 8Bh, 0E5h
                db 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h
                db 0EDh, 1, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 1Eh, 50h
                db 0E8h, 0B7h, 0A3h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh
                db 0D8h, 80h, 7Fh, 26h, 82h, 75h, 9, 0C6h, 47h, 26h, 0
                db 0E8h, 0DEh, 0, 0EBh, 3, 0E8h, 35h, 1, 8Bh, 0E5h, 5Dh
                db 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h, 0B9h
                db 1, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 66h, 50h, 0E8h
                db 83h, 0A3h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh, 0D8h
                db 80h, 7Fh, 26h, 81h, 74h, 5, 0E8h, 0Ah, 1, 0EBh, 4Fh
                db 0B8h, 1, 0, 50h, 0FFh, 36h, 0D6h, 5Dh, 0E8h, 0BDh, 0A8h
                db 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 0FFh, 76h, 0FEh, 0E8h
                db 0B0h, 0A8h, 83h, 0C4h, 4, 0B8h, 64h, 0, 50h, 0B8h, 1
                db 0, 50h, 0E8h, 5Eh, 0A1h, 83h, 0C4h, 4, 89h, 46h, 0FCh
                db 3Dh, 0Bh, 0, 7Dh, 0Fh, 3Dh, 0Ah, 0, 75h, 0C5h, 8Bh
                db 5Eh, 0FEh, 0C6h, 47h, 26h, 0FFh, 0EBh
byte_1CE30      db 0BCh, 90h, 8Bh, 5Eh, 0FEh, 0C6h, 47h, 26h, 0, 0C7h
                                        ; CODE XREF: seg002:0831↑J
                db 47h, 5Eh, 1, 0, 0E8h, 5Dh, 0, 8Bh, 0E5h, 5Dh, 0C3h
                db 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 56h, 0E8h, 3Ch, 1
                db 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 41h, 50h, 0E8h, 6
                db 0A3h, 83h, 0C4h, 2, 89h, 46h, 0FEh, 8Bh, 0D8h, 80h
                db 7Fh, 26h, 80h, 72h, 6, 0E8h, 8Dh, 0, 0EBh, 2Ah, 90h
                db 8Bh, 5Eh, 0FEh, 80h, 67h, 26h, 2Fh, 8Bh, 46h, 4, 1
                db 47h, 5Eh, 8Bh, 0F3h, 8Bh, 44h, 74h, 39h, 47h, 5Eh, 76h
                db 3, 89h, 47h, 5Eh, 0A1h, 0D6h, 5Dh, 3Bh, 0D8h, 75h, 5
                db 0C6h, 6, 97h, 3, 1, 0E8h, 5, 0, 5Eh, 8Bh, 0E5h, 5Dh
                db 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 56h, 0C6h, 46h
                db 0FEh, 14h, 80h, 3Eh, 0CEh, 3, 2, 75h, 4, 0C6h, 46h
                db 0FEh, 0Fh, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 8Bh, 0F0h, 8Dh
                db 44h, 2, 50h, 0B8h, 26h, 0, 50h, 56h, 0B8h, 1, 0, 50h
byte_1CEC8      db 0E8h, 33h, 0A0h, 83h, 0C4h, 8, 8Dh, 44h, 1, 50h, 0B8h
                                        ; CODE XREF: seg002:07AD↑J
                db 11h, 0, 50h, 0E8h, 55h, 0A0h, 83h, 0C4h, 4, 0B8h, 32h
                db 31h, 50h, 0E8h, 6Fh, 0A0h, 83h, 0C4h, 2, 0B8h, 32h
                db 0, 50h, 0E8h, 15h, 0A2h, 83h, 0C4h, 2, 0C6h, 6, 28h
                db 4, 1, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h
                db 0ECh, 2, 56h, 0C6h, 46h, 0FEh, 14h, 80h, 3Eh, 0CEh
                db 3, 2, 75h, 4, 0C6h, 46h, 0FEh, 0Fh, 8Ah, 46h, 0FEh
                db 2Ah, 0E4h, 8Bh, 0F0h, 8Dh, 44h, 2, 50h, 0B8h, 26h, 0
                db 50h, 56h, 0B8h, 1, 0, 50h, 0E8h, 0D7h, 9Fh, 83h, 0C4h
                db 8, 8Dh, 44h
byte_1CF2C      db 1, 50h, 0B8h, 0Ch, 0, 50h, 0E8h, 0F9h, 9Fh, 83h, 0C4h
                                        ; CODE XREF: seg002:050D↑J
                db 4, 0B8h, 3Bh, 31h, 50h, 0E8h, 13h, 0A0h, 83h, 0C4h
                db 2, 0B8h, 9, 0, 50h, 0E8h, 4Dh, 0A1h, 83h, 0C4h, 2, 0B8h
                db 32h, 0, 50h, 0E8h, 0AFh, 0A1h, 83h, 0C4h, 2, 5Eh, 8Bh
                db 0E5h, 5Dh, 0C3h, 90h

; =============== S U B R O U T I N E =======================================


sub_1CF5C       proc near               ; CODE XREF: seg002:0AE9↑J
                cmp     word_1DB9A, 0
                jz      short locret_1CF8B
                mov     ax, word_1DB9A
                mov     word_22196, ax
                push    ax
                call    thk_res_0FF2
                add     sp, 2
                mov     word_1DB9A, 0
                mov     ax, word_1DB98
                mov     word_22196, ax
                mov     ax, word_1DB9C
                mov     word_22194, ax
                sub     ax, ax

loc_1CF84:                              ; CODE XREF: seg002:062D↑J
                push    ax
                call    thk_res_3FA0
                add     sp, 2

locret_1CF8B:                           ; CODE XREF: sub_1CF5C+5↑j
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     word ptr [bp-2], 14h
                mov     byte_1DC78, 0
                cmp     byte_1DC1E, 2
                jnz     short loc_1CFA8
                mov     word ptr [bp-2], 0Fh

loc_1CFA8:                              ; CODE XREF: sub_1CF5C+45↑j
                cmp     word_1DC76, 1
                jnz     short loc_1CFC6
                call    loc_1D046
                or      ax, ax
                jz      short loc_1CFBE
                mov     word ptr [bp-4], 31h ; '1'
                jmp     short loc_1D004
; ---------------------------------------------------------------------------
                align 2

loc_1CFBE:                              ; CODE XREF: sub_1CF5C+58↑j
                mov     word ptr [bp-4], 1Bh
                jmp     short loc_1D004
; ---------------------------------------------------------------------------
                align 2

loc_1CFC6:                              ; CODE XREF: sub_1CF5C+51↑j
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, byte ptr word_1DC76
                add     al, 30h ; '0'
                mov     [bx+0Bh], al
                mov     ax, [bp-2]
                inc     ax
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_res_1676
                add     sp, 4
                push    word ptr aL1ReturnToCast ; "L1'Return' to cast"
                call    thk_res_1726
                add     sp, 2
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, [bx+0Bh]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp-4], ax

loc_1D004:                              ; CODE XREF: sub_1CF5C+5F↑j
                                        ; sub_1CF5C+67↑j
                cmp     byte_1DC1E, 2
                jnz     short loc_1D00E
                call    thk_res_35A8

loc_1D00E:                              ; CODE XREF: sub_1CF5C+AD↑j
                cmp     word ptr [bp-4], 1Bh
                jz      short loc_1D020
                sub     word ptr [bp-4], 31h ; '1'
                mov     byte_1DC78, 1
                jmp     short loc_1D028
; ---------------------------------------------------------------------------
                align 2

loc_1D020:                              ; CODE XREF: sub_1CF5C+B6↑j
                sub     ax, ax
                mov     word_2765C, ax
                mov     word_27696, ax

loc_1D028:                              ; CODE XREF: sub_1CF5C+C1↑j
                cmp     byte_1DC78, 0
                jz      short loc_1D03E
                test    byte_23218, 2
                jz      short loc_1D03E
                mov     word ptr [bp-4], 1Bh
                call    near ptr byte_1CEC8+32h

loc_1D03E:                              ; CODE XREF: sub_1CF5C+D1↑j
                                        ; sub_1CF5C+D8↑j
                mov     ax, [bp-4]
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1D046:                              ; CODE XREF: sub_1C1EA+6↑p
                                        ; ovl_2CAST1:C24A↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     word ptr [bp-2], 15h
                cmp     byte_2419E, 0
                jz      short loc_1D060
                mov     word ptr [bp-4], 1
                jmp     short loc_1D0A6
; ---------------------------------------------------------------------------
                align 2

loc_1D060:                              ; CODE XREF: sub_1CF5C+FA↑j
                cmp     byte_1DC1E, 2
                jnz     short loc_1D06C
                mov     word ptr [bp-2], 0Fh

loc_1D06C:                              ; CODE XREF: sub_1CF5C+109↑j
                push    word ptr [bp-2]
                mov     ax, 16h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 315Eh
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp-4], ax
                cmp     ax, 0Dh
                jnz     short loc_1D09C
                mov     al, 1
                jmp     short loc_1D09E
; ---------------------------------------------------------------------------

loc_1D09C:                              ; CODE XREF: sub_1CF5C+13A↑j
                sub     al, al

loc_1D09E:                              ; CODE XREF: sub_1CF5C+13E↑j
                mov     byte_1DC78, al
                sub     ah, ah
                mov     [bp-4], ax

loc_1D0A6:                              ; CODE XREF: sub_1CF5C+101↑j
                cmp     word ptr [bp-4], 0
                jz      short loc_1D0BB
                test    byte_23218, 2
                jz      short loc_1D0BB
                mov     word ptr [bp-4], 0
                call    near ptr byte_1CEC8+32h

loc_1D0BB:                              ; CODE XREF: sub_1CF5C+14E↑j
                                        ; sub_1CF5C+155↑j
                mov     ax, [bp-4]
                mov     sp, bp
                pop     bp
                retn
sub_1CF5C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D0C2       proc near               ; CODE XREF: seg002:04DD↑J

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ax, [bp+arg_0]
                cmp     ax, 5Fh         ; switch 96 cases
                jbe     short loc_1D0D0
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0D0:                              ; CODE XREF: sub_1D0C2+9↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1D0D3[bx] ; switch jump
; ---------------------------------------------------------------------------

loc_1D0D8:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    sub_1C130       ; jumptable 0001D0D3 case 1
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0DE:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C132+0A0h ; jumptable 0001D0D3 case 5
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0E4:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    sub_1C1EA       ; jumptable 0001D0D3 case 7
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0EA:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C23E       ; jumptable 0001D0D3 case 10
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0F0:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    sub_1C320       ; jumptable 0001D0D3 case 11
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0F6:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    sub_1C340       ; jumptable 0001D0D3 case 12
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0FC:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+5Eh ; jumptable 0001D0D3 case 13
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D102:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    sub_1C3EE       ; jumptable 0001D0D3 case 15
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D108:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C4FC       ; jumptable 0001D0D3 case 19
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D10E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C550       ; jumptable 0001D0D3 case 23
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D114:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C570       ; jumptable 0001D0D3 case 29
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D11A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C590       ; jumptable 0001D0D3 case 30
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D120:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C648       ; jumptable 0001D0D3 case 34
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D126:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C68C       ; jumptable 0001D0D3 case 37
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D12C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C722       ; jumptable 0001D0D3 case 38
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D132:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    loc_1C774       ; jumptable 0001D0D3 case 47
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D138:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+10h ; jumptable 0001D0D3 cases 0,49
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D13E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+80h ; jumptable 0001D0D3 case 51
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D144:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C132+82h ; jumptable 0001D0D3 cases 4,52
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D14A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+8Ch ; jumptable 0001D0D3 case 53
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D150:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+0CCh ; jumptable 0001D0D3 case 55
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D156:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr unk_1C7DA ; jumptable 0001D0D3 case 57
                jmp     def_1D0D3       ; CODE XREF: seg002:0825↑J
                                        ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D15C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CBDC+0D8h ; jumptable 0001D0D3 case 59
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D162:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+94h ; jumptable 0001D0D3 case 63
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D168:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CCBA+1Ch ; jumptable 0001D0D3 case 64
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D16E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+0B8h ; jumptable 0001D0D3 case 66
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D174:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+0E0h ; jumptable 0001D0D3 case 67
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D17A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+0F8h ; jumptable 0001D0D3 case 69
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D180:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CCBA+5Ch ; jumptable 0001D0D3 case 70
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D186:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+108h ; jumptable 0001D0D3 case 71
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D18C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+142h ; jumptable 0001D0D3 case 72
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D192:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CCBA+9Ch ; jumptable 0001D0D3 case 78
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D198:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+19Ch ; jumptable 0001D0D3 case 79
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D19E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C7E8+1ACh ; jumptable 0001D0D3 case 80
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1A4:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CCBA+0DCh ; jumptable 0001D0D3 case 81
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1AA:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C9E6+1Ah ; jumptable 0001D0D3 case 83
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1B0:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CCBA+110h ; jumptable 0001D0D3 case 87
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1B6:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C9E6+2Ah ; jumptable 0001D0D3 case 89
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1BC:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1C9E6+3Ah ; jumptable 0001D0D3 case 91
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1C2:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CA88+1Ch ; jumptable 0001D0D3 case 94
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1C8:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o
                call    near ptr byte_1CA88+88h ; jumptable 0001D0D3 case 95
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------
jpt_1D0D3       dw offset loc_1D138, offset loc_1D0D8, offset def_1D0D3
                                        ; DATA XREF: sub_1D0C2+11↑r
                dw offset def_1D0D3, offset loc_1D144, offset loc_1D0DE ; jump table for switch statement
                dw offset def_1D0D3, offset loc_1D0E4, offset def_1D0D3
                dw offset def_1D0D3, offset loc_1D0EA, offset loc_1D0F0
                dw offset loc_1D0F6, offset loc_1D0FC, offset def_1D0D3
                dw offset loc_1D102, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset loc_1D108, offset def_1D0D3
                dw offset def_1D0D3, offset def_1D0D3, offset loc_1D10E
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset def_1D0D3, offset loc_1D114
                dw offset loc_1D11A, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset loc_1D120, offset def_1D0D3
                dw offset def_1D0D3, offset loc_1D126, offset loc_1D12C
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset def_1D0D3, offset loc_1D132
                dw offset def_1D0D3, offset loc_1D138, offset def_1D0D3
                dw offset loc_1D13E, offset loc_1D144, offset loc_1D14A
                dw offset def_1D0D3, offset loc_1D150, offset def_1D0D3
                dw offset loc_1D156, offset def_1D0D3, offset loc_1D15C
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset loc_1D162, offset loc_1D168, offset def_1D0D3
                dw offset loc_1D16E, offset loc_1D174, offset def_1D0D3
                dw offset loc_1D17A, offset loc_1D180, offset loc_1D186
                dw offset loc_1D18C, offset def_1D0D3, offset def_1D0D3
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset loc_1D192, offset loc_1D198, offset loc_1D19E
                dw offset loc_1D1A4, offset def_1D0D3, offset loc_1D1AA
                dw offset def_1D0D3, offset def_1D0D3, offset def_1D0D3
                dw offset loc_1D1B0, offset def_1D0D3, offset loc_1D1B6
                dw offset def_1D0D3, offset loc_1D1BC, offset def_1D0D3
                dw offset def_1D0D3, offset loc_1D1C2, offset loc_1D1C8
; ---------------------------------------------------------------------------

def_1D0D3:                              ; CODE XREF: sub_1D0C2+B↑j
                                        ; sub_1D0C2+11↑j ...
                pop     bp              ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                retn
sub_1D0C2       endp

ovl_2CAST1      ends

