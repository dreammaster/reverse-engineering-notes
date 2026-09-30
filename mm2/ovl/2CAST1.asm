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
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

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
                cmp     g_outdoors, 1
                jz      short loc_1C286
                mov     si, g_party_y
                and     si, 0FFh
                mov     cl, 4
                shl     si, cl
                mov     bl, g_party_x
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
                call    thk_facing_delta
                add     sp, 4
                mov     al, [bp-2]
                add     al, g_party_x
                and     al, 0Fh
                mov     [bp-6], al
                mov     al, [bp-4]
                add     al, byte ptr g_party_y
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
                call    sub_1CEFA

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                jmp     short loc_1C31B
; ---------------------------------------------------------------------------

loc_1C2FA:                              ; CODE XREF: ovl_2CAST1:C2F3↑j
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al
                call    sub_1CE9E
                mov     al, [bp-6]

loc_1C308:                              ; CODE XREF: seg002:08F1↑J
                add     al, [bp-2]
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, [bp-0Ah]
                add     al, [bp-4]
                and     al, 0Fh
                mov     byte ptr g_party_y, al

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
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aBeacon1SetNew2 ; "Beacon: 1) Set new 2) Teleport to last"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1C35C
                cmp     ax, 31h ; '1'
                jnz     short loc_1C3B8
                mov     al, g_map_id
                mov     byte_1DC38, al
                mov     al, byte ptr g_party_y
                mov     cl, 4
                shl     al, cl
                add     al, g_party_x
                mov     byte_1DC39, al
                jmp     short loc_1C3DA
; ---------------------------------------------------------------------------
                align 2

loc_1C3B8:                              ; CODE XREF: sub_1C340+5F↑j
                mov     al, byte_1DC38
                mov     g_map_id, al
                mov     al, byte_1DC39
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, byte_1DC39
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr g_party_y, al
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al

loc_1C3DA:                              ; CODE XREF: sub_1C340+1F↑j
                                        ; sub_1C340+75↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C3E6
                call    sub_1CEFA
                jmp     short loc_1C3E9
; ---------------------------------------------------------------------------
                align 2

loc_1C3E6:                              ; CODE XREF: sub_1C340+9E↑j
                call    sub_1CE9E

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
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aFlyToAE ; "Fly to (A-E)?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1C43A:                              ; CODE XREF: sub_1C3EE+82↓j
                call    thk_kbd_poll
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
                call    thk_text_putc
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:0B31↑J
                add     sp, 4
                mov     ax, offset a14  ; "(1-4)"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 34h ; '4'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jnz     short loc_1C4B7
                jmp     loc_1C411
; ---------------------------------------------------------------------------

loc_1C4B7:                              ; CODE XREF: sub_1C3EE+C4↑j
                push    ax
                call    thk_text_putc
                add     sp, 2
                sub     [bp+var_4], 31h ; '1'
                sub     [bp+var_6], 41h ; 'A'
                mov     al, 0FFh
                mov     byte ptr g_party_y, al
                mov     g_party_x, al
                mov     si, [bp+var_6]
                shl     si, 1
                shl     si, 1
                mov     bx, [bp+var_4]
                mov     al, [bx+si+30BCh]
                mov     g_map_id, al
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al

loc_1C4E7:                              ; CODE XREF: sub_1C3EE+26↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C4F2
                call    sub_1CEFA
                jmp     short loc_1C4F5
; ---------------------------------------------------------------------------

loc_1C4F2:                              ; CODE XREF: sub_1C3EE+FD↑j
                call    sub_1CE9E

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

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C4FC       proc near               ; CODE XREF: sub_1D0C2:loc_1D108↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C54C
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_2], al
                cmp     byte_1DC31, 0FAh
                jbe     short loc_1C51F
                mov     byte_1DC31, 0FFh

loc_1C51F:                              ; CODE XREF: sub_1C4FC+1C↑j
                mov     dl, [bp+var_2]
                mov     cl, byte_1DC31
                jmp     short loc_1C530
; ---------------------------------------------------------------------------

loc_1C528:                              ; CODE XREF: sub_1C4FC+3A↓j
                cmp     cl, 0FAh
                jnb     short loc_1C530 ; CODE XREF: seg002:0879↑J
                add     cl, 5

loc_1C530:                              ; CODE XREF: sub_1C4FC+2A↑j
                                        ; sub_1C4FC+2F↑j
                mov     al, dl
                dec     dl
                or      al, al
                jnz     short loc_1C528
                mov     [bp+var_2], dl
                mov     byte_1DC31, cl
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

loc_1C54C:                              ; CODE XREF: sub_1C4FC+B↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C4FC       endp


; =============== S U B R O U T I N E =======================================


sub_1C550       proc near               ; CODE XREF: sub_1D0C2:loc_1D10E↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C56F
                cmp     byte_1DC2A, 0FFh
                jnb     short loc_1C562
                inc     byte_1DC2A

loc_1C562:                              ; CODE XREF: sub_1C550+C↑j
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

locret_1C56F:                           ; CODE XREF: sub_1C550+5↑j
                retn
sub_1C550       endp


; =============== S U B R O U T I N E =======================================


sub_1C570       proc near               ; CODE XREF: sub_1D0C2:loc_1D114↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C58F
                cmp     byte_1DC64, 0FFh
                jnb     short loc_1C582
                inc     byte_1DC64

loc_1C582:                              ; CODE XREF: sub_1C570+C↑j
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

locret_1C58F:                           ; CODE XREF: sub_1C570+5↑j
                retn
sub_1C570       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C590       proc near               ; CODE XREF: sub_1D0C2:loc_1D11A↓p

var_8           = word ptr -8
var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     [bp+var_6], 0
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C5A6
                jmp     loc_1C643
; ---------------------------------------------------------------------------

loc_1C5A6:                              ; CODE XREF: sub_1C590+11↑j
                test    byte_231F0, 10h
                jz      short loc_1C5B4 ; CODE XREF: seg002:0885↑J

loc_1C5AD:                              ; CODE XREF: sub_1C590+5C↓j
                inc     [bp+var_6]
                jmp     loc_1C634
; ---------------------------------------------------------------------------
                align 2

loc_1C5B4:                              ; CODE XREF: sub_1C590+1B↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h         ; CODE XREF: seg002:029D↑J
                push    ax
                mov     ax, 0Dh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 30D0h
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 39h ; '9'   ; CODE XREF: seg002:01A1↑J
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_8], ax
                cmp     ax, 1Bh
                jz      short loc_1C5AD
                sub     [bp+var_8], 30h ; '0'
                lea     ax, [bp+var_4]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                call    thk_facing_delta
                add     sp, 4
                mov     si, [bp+var_8]
                mov     dl, g_party_x
                mov     cl, byte ptr g_party_y
                jmp     short loc_1C61A
; ---------------------------------------------------------------------------
                align 2

loc_1C60E:                              ; CODE XREF: sub_1C590+8F↓j
                add     dl, [bp+var_2]
                add     cl, [bp+var_4]
                and     dl, 0Fh
                and     cl, 0Fh

loc_1C61A:                              ; CODE XREF: sub_1C590+7B↑j
                mov     ax, si
                dec     si
                or      ax, ax
                jnz     short loc_1C60E
                mov     [bp+var_8], si
                mov     g_party_x, dl
                mov     byte ptr g_party_y, cl
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al

loc_1C634:                              ; CODE XREF: sub_1C590+20↑j
                cmp     [bp+var_6], 0
                jnz     short loc_1C640
                call    sub_1CE9E
                jmp     short loc_1C643
; ---------------------------------------------------------------------------
                align 2

loc_1C640:                              ; CODE XREF: sub_1C590+A8↑j
                call    sub_1CEFA

loc_1C643:                              ; CODE XREF: sub_1C590+13↑j
                                        ; sub_1C590+AD↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C590       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C648       proc near               ; CODE XREF: sub_1D0C2:loc_1D120↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    sub_1CB48
                mov     [bp+var_2], ax
                cmp     ax, 1Bh
                jz      short loc_1C687
                mov     si, ax
                mov     bx, word_23626
                cmp     byte ptr [bx+si+40h], 0
                jnz     short loc_1C66C
                call    sub_1CEFA
                jmp     short loc_1C687
; ---------------------------------------------------------------------------
                align 2

loc_1C66C:                              ; CODE XREF: sub_1C648+1C↑j
                                        ; seg002:0891↑J
                mov     ax, 6
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     si, [bp+var_2]
                mov     bx, word_23626
                add     [bx+si+40h], al
                call    sub_1CE9E

loc_1C687:                              ; CODE XREF: sub_1C648+10↑j
                                        ; sub_1C648+21↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C648       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C68C       proc near               ; CODE XREF: sub_1D0C2:loc_1D126↓p

var_8           = byte ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    di
                push    si
                mov     [bp+var_2], 0
                call    sub_1CB48
                mov     [bp+var_6], ax
                cmp     ax, 1Bh
                jz      short loc_1C702
                mov     si, ax
                mov     bx, word_23626
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C6B6

loc_1C6B0:                              ; CODE XREF: sub_1C68C+42↓j
                                        ; sub_1C68C+5B↓j
                inc     [bp+var_2]
                jmp     short loc_1C702
; ---------------------------------------------------------------------------
                align 2

loc_1C6B6:                              ; CODE XREF: sub_1C68C+22↑j
                sub     cx, cx
                mov     dx, word_23626

loc_1C6BC:                              ; CODE XREF: sub_1C68C+4A↓j
                mov     si, cx
                mov     bx, dx
                cmp     byte ptr [bx+si+3Ah], 0
                jnz     short loc_1C6D0

loc_1C6C6:                              ; CODE XREF: sub_1C68C+48↓j
                mov     [bp+var_4], cx
                cmp     cx, 6
                jnz     short loc_1C6D8
                jmp     short loc_1C6B0
; ---------------------------------------------------------------------------

loc_1C6D0:                              ; CODE XREF: sub_1C68C+38↑j
                inc     cx
                cmp     cx, 6
                jge     short loc_1C6C6
                jmp     short loc_1C6BC
; ---------------------------------------------------------------------------

loc_1C6D8:                              ; CODE XREF: sub_1C68C+40↑j
                mov     si, [bp+var_6]
                mov     bx, word_23626
                mov     al, [bx+si+3Ah]
                mov     [bp+var_8], al
                cmp     al, 0D0h
                jnb     short loc_1C6B0
                mov     si, [bp+var_4]
                add     si, bx
                mov     [si+3Ah], al
                mov     di, [bp+var_6]  ; CODE XREF: seg002:0B19↑J
                add     di, bx
                mov     al, [di+40h]
                mov     [si+40h], al
                mov     al, [di+46h]
                mov     [si+46h], al

loc_1C702:                              ; CODE XREF: sub_1C68C+16↑j
                                        ; sub_1C68C+27↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C70E
                call    sub_1CEFA
                jmp     short loc_1C71C
; ---------------------------------------------------------------------------
                align 2

loc_1C70E:                              ; CODE XREF: sub_1C68C+7A↑j
                cmp     [bp+var_6], 1Bh
                jz      short loc_1C71C
                call    sub_1CE9E
                mov     byte_1DBE7, 1

loc_1C71C:                              ; CODE XREF: sub_1C68C+7F↑j
                                        ; sub_1C68C+86↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1C68C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C722       proc near               ; CODE XREF: sub_1D0C2:loc_1D12C↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C76F
                test    byte_231F0, 20h
                jz      short loc_1C73C
                call    sub_1CEFA
                jmp     short loc_1C76F ; CODE XREF: seg002:089D↑J
; ---------------------------------------------------------------------------
                align 2

loc_1C73C:                              ; CODE XREF: sub_1C722+12↑j
                lea     ax, [bp+var_4]
                push    ax
                lea     ax, [bp+var_2]
                push    ax
                call    thk_facing_delta
                add     sp, 4
                mov     al, [bp+var_2]
                add     g_party_x, al
                mov     al, [bp+var_4]
                add     byte ptr g_party_y, al
                and     g_party_x, 0Fh
                and     byte ptr g_party_y, 0Fh
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

loc_1C76F:                              ; CODE XREF: sub_1C722+B↑j
                                        ; sub_1C722+17↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C722       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C774       proc near               ; CODE XREF: sub_1D0C2:loc_1D132↓p

var_A           = byte ptr -0Ah
var_8           = word ptr -8
var_6           = byte ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     [bp+var_4], 0
                call    sub_1CB48
                mov     [bp+var_8], ax
                cmp     ax, 1Bh
                jz      short loc_1C7D5
                mov     si, ax
                mov     bx, word_23626
                mov     al, [bx+si+46h]
                mov     [bp+var_A], al
                and     al, 0C0h
                mov     [bp+var_6], al
                mov     al, [bp+var_A]
                and     al, 3Fh
                mov     [bp+var_2], al
                mov     al, 32h ; '2'
                mul     [bp+var_2]
                mov     word_2765C, ax
                cmp     [bx+58h], ax
                jnb     short loc_1C7B6
                call    sub_1CEFA
                jmp     short loc_1C7D5
; ---------------------------------------------------------------------------

loc_1C7B6:                              ; CODE XREF: sub_1C774+3B↑j
                cmp     [bp+var_2], 3Fh ; '?'
                ja      short loc_1C7BF
                inc     [bp+var_2]

loc_1C7BF:                              ; CODE XREF: sub_1C774+46↑j
                mov     al, [bp+var_6]
                or      [bp+var_2], al
                mov     si, [bp+var_8]
                mov     bx, word_23626
                mov     al, [bp+var_2]
                mov     [bx+si+46h], al
                call    sub_1CE9E

loc_1C7D5:                              ; CODE XREF: sub_1C774+15↑j
                                        ; sub_1C774+40↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C774       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C7DA       proc near               ; CODE XREF: sub_1D0C2:loc_1D156↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 8
                push    si
                mov     [bp+var_4], 0   ; CODE XREF: seg002:0849↑J
                mov     [bp+var_6], 0Ch ; CODE XREF: seg002:0B0D↑J
                call    loc_1D046
                or      ax, ax
                jnz     short loc_1C7F5
                jmp     loc_1C877
; ---------------------------------------------------------------------------

loc_1C7F5:                              ; CODE XREF: sub_1C7DA+16↑j
                test    byte_231F0, 40h
                jz      short loc_1C802

loc_1C7FC:                              ; CODE XREF: sub_1C7DA+2D↓j
                inc     [bp+var_4]
                jmp     short loc_1C85D
; ---------------------------------------------------------------------------
                align 2

loc_1C802:                              ; CODE XREF: sub_1C7DA+20↑j
                cmp     g_era, 9
                jnz     short loc_1C7FC
                mov     ax, word_1DC04
                mov     [bp+var_2], ax
                test    byte ptr [bp+var_2], 1
                jz      short loc_1C823
                sub     cx, cx
                mov     si, 30E0h
                mov     dx, ax

loc_1C81C:                              ; CODE XREF: sub_1C7DA+97↓j
                cmp     [si], dx
                jle     short loc_1C868

loc_1C820:                              ; CODE XREF: sub_1C7DA+95↓j
                mov     [bp+var_6], cx

loc_1C823:                              ; CODE XREF: sub_1C7DA+39↑j
                cmp     [bp+var_2], 96h
                jl      short loc_1C835
                mov     g_era, 8
                mov     [bp+var_6], 0Bh

loc_1C835:                              ; CODE XREF: sub_1C7DA+4E↑j
                mov     bx, [bp+var_6]
                mov     al, [bx+30FAh]
                mov     g_map_id, al
                mov     al, [bx+3108h]
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, [bx+3108h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr g_party_y, al
                mov     al, 1
                mov     byte_1DC80, al
                mov     byte ptr g_party_y+1, al

loc_1C85D:                              ; CODE XREF: sub_1C7DA+25↑j
                cmp     [bp+var_4], 0
                jz      short loc_1C874
                call    sub_1CEFA
                jmp     short loc_1C877
; ---------------------------------------------------------------------------

loc_1C868:                              ; CODE XREF: sub_1C7DA+44↑j
                add     si, 2
                inc     cx
                cmp     cx, 0Dh
                jge     short loc_1C820
                jmp     short loc_1C81C
; ---------------------------------------------------------------------------
                align 2

loc_1C874:                              ; CODE XREF: sub_1C7DA+87↑j
                call    sub_1CE9E

loc_1C877:                              ; CODE XREF: sub_1C7DA+18↑j
                                        ; sub_1C7DA+8C↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C7DA       endp


; =============== S U B R O U T I N E =======================================


sub_1C87C       proc near               ; CODE XREF: sub_1D0C2:loc_1D162↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C89F
                mov     bx, word_23626
                cmp     byte ptr [bx+25h], 28h ; '('
                jnb     short loc_1C89C
                add     byte ptr [bx+25h], 8
                call    sub_1CE9E
                mov     byte_1DBE7, 1
                jmp     short locret_1C89F
; ---------------------------------------------------------------------------
                align 2

loc_1C89C:                              ; CODE XREF: sub_1C87C+F↑j
                call    sub_1CEFA

locret_1C89F:                           ; CODE XREF: sub_1C87C+5↑j
                                        ; sub_1C87C+1D↑j
                retn
sub_1C87C       endp


; =============== S U B R O U T I N E =======================================


sub_1C8A0       proc near               ; CODE XREF: sub_1D0C2:loc_1D16E↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C8C6
                cmp     byte_1DC25, 0EBh
                jbe     short loc_1C8B6
                mov     byte_1DC25, 0FFh
                jmp     short loc_1C8BB
; ---------------------------------------------------------------------------
                align 2

loc_1C8B6:                              ; CODE XREF: sub_1C8A0+C↑j
                add     byte_1DC25, 14h

loc_1C8BB:                              ; CODE XREF: sub_1C8A0+13↑j
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al
                call    sub_1CE9E

locret_1C8C6:                           ; CODE XREF: sub_1C8A0+5↑j
                retn
sub_1C8A0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C8C8       proc near               ; CODE XREF: sub_1D0C2:loc_1D174↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C8DF
                mov     al, 1
                mov     byte_1DC29, al
                mov     byte ptr g_party_y+1, al
                mov     byte_1DBEB, 0
                call    sub_1CE9E

locret_1C8DF:                           ; CODE XREF: sub_1C8C8+5↑j
                retn
sub_1C8C8       endp


; =============== S U B R O U T I N E =======================================


sub_1C8E0       proc near               ; CODE XREF: sub_1D0C2:loc_1D17A↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C8EF
                mov     byte_1DC2D, 1
                call    sub_1CE9E

locret_1C8EF:                           ; CODE XREF: sub_1C8E0+5↑j
                retn
sub_1C8E0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C8F0       proc near               ; CODE XREF: sub_1D0C2:loc_1D186↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1C925
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                mov     si, ax
                mov     al, [si+0Dh]
                mov     [bx+6Ah], al
                call    sub_1CE9E
                mov     ax, word_23626
                cmp     si, ax
                jnz     short loc_1C925
                mov     byte_1DBE7, 1

loc_1C925:                              ; CODE XREF: sub_1C8F0+10↑j
                                        ; sub_1C8F0+2E↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1C8F0       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C92A       proc near               ; CODE XREF: sub_1D0C2:loc_1D18C↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     [bp+var_2], 0
                call    loc_1D046
                or      ax, ax
                jz      short loc_1C97F
                test    byte_231F0, 40h
                jz      short loc_1C948

loc_1C943:                              ; CODE XREF: sub_1C92A+23↓j
                inc     [bp+var_2]
                jmp     short loc_1C971
; ---------------------------------------------------------------------------

loc_1C948:                              ; CODE XREF: sub_1C92A+17↑j
                cmp     byte_231EC, 0
                jz      short loc_1C943
                mov     al, byte_231EC
                and     al, 0Fh
                mov     g_party_x, al
                mov     al, byte_231EC
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                mov     byte ptr g_party_y, al
                mov     al, byte_231EE
                mov     g_map_id, al
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al

loc_1C971:                              ; CODE XREF: sub_1C92A+1C↑j
                cmp     [bp+var_2], 0
                jz      short loc_1C97C
                call    sub_1CEFA
                jmp     short loc_1C97F
; ---------------------------------------------------------------------------

loc_1C97C:                              ; CODE XREF: sub_1C92A+4B↑j
                call    sub_1CE9E

loc_1C97F:                              ; CODE XREF: sub_1C92A+10↑j
                                        ; sub_1C92A+50↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C92A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C984       proc near               ; CODE XREF: sub_1D0C2:loc_1D198↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1C993
                mov     byte_1DC2F, 1
                call    sub_1CE9E

locret_1C993:                           ; CODE XREF: sub_1C984+5↑j
                retn
sub_1C984       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C994       proc near               ; CODE XREF: sub_1D0C2:loc_1D19E↓p

var_6           = word ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6

loc_1C99A:                              ; CODE XREF: seg002:07DD↑J
                call    loc_1CF8C
                mov     [bp+var_6], ax
                cmp     ax, 1Bh
                jz      short loc_1C9FC
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     ax, 0Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], al
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                cmp     ax, 32h ; '2'
                jge     short loc_1C9DC
                mov     bx, [bp+var_2]
                cmp     byte ptr [bx+21h], 12h
                jnb     short loc_1C9F0

loc_1C9DC:                              ; CODE XREF: sub_1C994+3D↑j
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                push    [bp+var_2]
                call    thk_char_add_age ; CODE XREF: seg002:0B01↑J
                add     sp, 4
                call    sub_1CEFA
                jmp     short loc_1C9FC
; ---------------------------------------------------------------------------

loc_1C9F0:                              ; CODE XREF: sub_1C994+46↑j
                mov     bx, [bp+var_2]
                mov     al, [bp+var_4]
                sub     [bx+21h], al
                call    sub_1CE9E

loc_1C9FC:                              ; CODE XREF: sub_1C994+F↑j
                                        ; sub_1C994+5A↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C994       endp


; =============== S U B R O U T I N E =======================================


sub_1CA00       proc near               ; CODE XREF: sub_1D0C2:loc_1D1AA↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1CA0F
                mov     byte_1DC2C, 1
                call    sub_1CE9E

locret_1CA0F:                           ; CODE XREF: sub_1CA00+5↑j
                retn
sub_1CA00       endp


; =============== S U B R O U T I N E =======================================


sub_1CA10       proc near               ; CODE XREF: sub_1D0C2:loc_1D1B6↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1CA1F
                mov     byte_1DC2E, 1
                call    sub_1CE9E

locret_1CA1F:                           ; CODE XREF: sub_1CA10+5↑j
                retn
sub_1CA10       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CA20       proc near               ; CODE XREF: sub_1D0C2:loc_1D1BC↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     [bp+var_2], 0
                call    loc_1D046
                or      ax, ax
                jz      short loc_1CA9F
                test    byte_231F0, 40h
                jz      short loc_1CA3E

loc_1CA39:                              ; CODE XREF: sub_1CA20+56↓j
                inc     [bp+var_2]
                jmp     short loc_1CA90
; ---------------------------------------------------------------------------

loc_1CA3E:                              ; CODE XREF: sub_1CA20+17↑j
                mov     ax, 1
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:0A89↑J
                add     sp, 4
                mov     ax, offset aTown15 ; "Town (1-5)?"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 35h ; '5'
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CA39
                mov     al, byte ptr [bp+var_4]
                sub     al, 31h ; '1'
                mov     g_map_id, al
                mov     al, 0FFh
                mov     byte ptr g_party_y, al
                mov     g_party_x, al

loc_1CA88:                              ; CODE XREF: seg002:0801↑J
                mov     al, 1
                mov     byte_1DBEB, al
                mov     byte ptr g_party_y+1, al

loc_1CA90:                              ; CODE XREF: sub_1CA20+1C↑j
                cmp     [bp+var_2], 0
                jz      short loc_1CA9C
                call    sub_1CEFA
                jmp     short loc_1CA9F
; ---------------------------------------------------------------------------
                align 2

loc_1CA9C:                              ; CODE XREF: sub_1CA20+74↑j
                call    sub_1CE9E

loc_1CA9F:                              ; CODE XREF: sub_1CA20+10↑j
                                        ; sub_1CA20+79↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CA20       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CAA4       proc near               ; CODE XREF: sub_1D0C2:loc_1D1C2↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                call    loc_1CF8C
                mov     [bp+var_6], ax
                cmp     ax, 1Bh
                jz      short loc_1CB0B
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_4], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CACE

loc_1CAC8:                              ; CODE XREF: sub_1CAA4+50↓j
                call    sub_1CEFA
                jmp     short loc_1CB0B
; ---------------------------------------------------------------------------
                align 2

loc_1CACE:                              ; CODE XREF: sub_1CAA4+22↑j
                mov     ax, 1
                push    ax
                push    word_23626
                call    thk_char_add_age
                add     sp, 4
                mov     ax, 5
                push    ax
                push    [bp+var_4]
                call    thk_char_add_age
                add     sp, 4
                mov     bx, [bp+var_4]
                mov     al, [bx+27h]
                mov     [bp+var_2], al
                or      al, al
                jz      short loc_1CAC8
                dec     [bp+var_2]
                mov     si, bx
                mov     al, [bp+var_2]
                mov     [si+73h], al
                mov     [bx+27h], al
                mov     byte ptr [bx+26h], 0
                call    sub_1CE9E

loc_1CB0B:                              ; CODE XREF: sub_1CAA4+10↑j
                                        ; sub_1CAA4+27↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CAA4       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB10       proc near               ; CODE XREF: sub_1D0C2:loc_1D1C8↓p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    sub_1CB48
                mov     [bp+var_2], ax
                cmp     ax, 1Bh
                jz      short loc_1CB42
                mov     si, ax
                mov     bx, word_23626
                cmp     byte ptr [bx+si+40h], 0FFh
                jnz     short loc_1CB34
                call    sub_1CEFA
                jmp     short loc_1CB42
; ---------------------------------------------------------------------------
                align 2

loc_1CB34:                              ; CODE XREF: sub_1CB10+1C↑j
                mov     si, [bp+var_2]
                mov     bx, word_23626
                mov     byte ptr [bx+si+40h], 1
                call    sub_1CE9E       ; CODE XREF: seg002:0A65↑J

loc_1CB42:                              ; CODE XREF: sub_1CB10+10↑j
                                        ; sub_1CB10+21↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CB10       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB48       proc near               ; CODE XREF: sub_1C648+7↑p
                                        ; sub_1C68C+D↑p ...

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                call    sub_1CF5C
                mov     ax, 15h
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOnWhichAF ; "On which (A-F)?"
                push    ax
                call    thk_text_puts
                add     sp, 2

loc_1CB6B:                              ; CODE XREF: sub_1CB48+6B↓j
                call    thk_kbd_poll
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     di, ax
                cmp     di, 1Bh
                jnz     short loc_1CB82
                mov     ax, 1
                jmp     short loc_1CB84
; ---------------------------------------------------------------------------
                align 2

loc_1CB82:                              ; CODE XREF: sub_1CB48+32↑j
                sub     ax, ax

loc_1CB84:                              ; CODE XREF: sub_1CB48+37↑j
                mov     si, ax
                or      si, si
                jnz     short loc_1CBB1

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                mov     ax, di
                cmp     ax, 41h ; 'A'
                jb      short loc_1CB9C
                cmp     ax, 46h ; 'F'
                ja      short loc_1CB9C
                mov     ax, 1
                jmp     short loc_1CB9E
; ---------------------------------------------------------------------------
                align 2

loc_1CB9C:                              ; CODE XREF: seg002:080D↑J
                                        ; sub_1CB48+47↑j ...
                sub     ax, ax

loc_1CB9E:                              ; CODE XREF: sub_1CB48+51↑j
                mov     si, ax
                or      si, si
                jz      short loc_1CBB1
                mov     bx, word_23626
                cmp     byte ptr [bx+di-7], 1
                sbb     ax, ax
                inc     ax
                mov     si, ax

loc_1CBB1:                              ; CODE XREF: sub_1CB48+40↑j
                                        ; sub_1CB48+5A↑j
                or      si, si
                jz      short loc_1CB6B
                mov     [bp+var_4], di
                mov     [bp+var_2], si
                cmp     di, 1Bh
                jz      short loc_1CBCC
                sub     [bp+var_4], 41h ; 'A'
                mov     byte_1DC78, 1
                jmp     short loc_1CBD4
; ---------------------------------------------------------------------------
                align 2

loc_1CBCC:                              ; CODE XREF: sub_1CB48+76↑j
                sub     ax, ax
                mov     word_2765C, ax
                mov     word_27696, ax

loc_1CBD4:                              ; CODE XREF: sub_1CB48+81↑j
                test    byte_23218, 2
                jz      short loc_1CBE3
                mov     [bp+var_4], 1Bh ; CODE XREF: seg002:01AD↑J
                call    sub_1CEFA

loc_1CBE3:                              ; CODE XREF: sub_1CB48+91↑j
                mov     ax, [bp+var_4]
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CB48       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CBEC       proc near               ; CODE XREF: sub_1D0C2:loc_1D138↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                push    si
                call    loc_1D046
                or      ax, ax
                jz      short loc_1CC34
                sub     si, si
                mov     di, [bp+var_4]
                jmp     short loc_1CC20
; ---------------------------------------------------------------------------

loc_1CC02:                              ; CODE XREF: sub_1CBEC+38↓j
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     di, ax
                mov     al, [di+26h]
                mov     [bp+var_2], al
                cmp     al, 80h
                jnb     short loc_1CC19
                and     [bp+var_2], 6Fh

loc_1CC19:                              ; CODE XREF: sub_1CBEC+27↑j
                mov     al, [bp+var_2]
                mov     [di+26h], al
                inc     si

loc_1CC20:                              ; CODE XREF: sub_1CBEC+14↑j
                cmp     si, g_party_size
                jl      short loc_1CC02
                mov     [bp+var_4], di
                mov     [bp+var_6], si
                mov     byte_1DBE7, 1
                call    sub_1CE9E

loc_1CC34:                              ; CODE XREF: sub_1CBEC+D↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CBEC       endp


; =============== S U B R O U T I N E =======================================


sub_1CC3A       proc near               ; CODE XREF: sub_1D0C2:loc_1D0FC↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1CC5A
                mov     bx, word_23626
                mov     al, [bx+71h]
                add     al, 0Ah
                mov     byte_1DC26, al
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

locret_1CC5A:                           ; CODE XREF: sub_1CC3A+5↑j
                retn
sub_1CC3A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CC5C       proc near               ; CODE XREF: sub_1D0C2:loc_1D13E↓p
                mov     ax, 8
                push    ax
                call    sub_1CE46
                add     sp, 2
                retn
sub_1CC5C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CC68       proc near               ; CODE XREF: sub_1D0C2:loc_1D14A↓p

var_4           = word ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                sub     si, si
                mov     bx, word_23626
                mov     al, [bx+71h]
                mov     [bp+var_2], al
                jmp     short loc_1CC8E
; ---------------------------------------------------------------------------
                align 2

loc_1CC7E:                              ; CODE XREF: sub_1CC68+2E↓j
                mov     ax, 0Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                add     si, ax

loc_1CC8E:                              ; CODE XREF: sub_1CC68+13↑j
                mov     al, [bp+var_2]
                dec     [bp+var_2]
                or      al, al
                jnz     short loc_1CC7E
                mov     [bp+var_4], si
                push    si
                call    sub_1CE46
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CC68       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CCA8       proc near               ; CODE XREF: sub_1D0C2:loc_1D150↓p
                mov     ax, 0Fh
                push    ax
                call    sub_1CE46
                add     sp, 2
                retn
sub_1CCA8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CCB4       proc near               ; CODE XREF: sub_1D0C2:loc_1D15C↓p
                call    loc_1D046
                or      ax, ax
                jz      short locret_1CCD4 ; CODE XREF: seg002:08FD↑J
                mov     bx, word_23626
                mov     al, [bx+71h]
                add     al, 14h
                mov     byte_1DC27, al
                mov     byte ptr g_party_y+1, 1
                mov     byte_1DBEB, 0
                call    sub_1CE9E

locret_1CCD4:                           ; CODE XREF: sub_1CCB4+5↑j
                retn
sub_1CCB4       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CCD6       proc near               ; CODE XREF: sub_1D0C2:loc_1D168↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CD11
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CD0E
                and     byte ptr [bx+26h], 77h
                mov     ax, word_23626
                cmp     bx, ax
                jnz     short loc_1CD09
                mov     byte_1DBE7, 1

loc_1CD09:                              ; CODE XREF: sub_1CCD6+2C↑j
                call    sub_1CE9E
                jmp     short loc_1CD11
; ---------------------------------------------------------------------------

loc_1CD0E:                              ; CODE XREF: sub_1CCD6+21↑j
                call    sub_1CEFA

loc_1CD11:                              ; CODE XREF: sub_1CCD6+F↑j
                                        ; sub_1CCD6+36↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CCD6       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD16       proc near               ; CODE XREF: sub_1D0C2:loc_1D180↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CD51
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CD4E
                and     byte ptr [bx+26h], 7Bh
                mov     ax, word_23626
                cmp     bx, ax
                jnz     short loc_1CD49
                mov     byte_1DBE7, 1

loc_1CD49:                              ; CODE XREF: sub_1CD16+2C↑j
                call    sub_1CE9E
                jmp     short loc_1CD51
; ---------------------------------------------------------------------------

loc_1CD4E:                              ; CODE XREF: sub_1CD16+21↑j
                call    sub_1CEFA

loc_1CD51:                              ; CODE XREF: sub_1CD16+F↑j
                                        ; sub_1CD16+36↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CD16       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD56       proc near               ; CODE XREF: sub_1D0C2:loc_1D192↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CD91
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jnb     short loc_1CD8E
                mov     byte ptr [bx+26h], 0
                mov     ax, word_23626
                cmp     bx, ax
                jnz     short loc_1CD89
                mov     byte_1DBE7, 1

loc_1CD89:                              ; CODE XREF: sub_1CD56+2C↑j
                call    sub_1CE9E
                jmp     short loc_1CD91
; ---------------------------------------------------------------------------

loc_1CD8E:                              ; CODE XREF: sub_1CD56+21↑j
                call    sub_1CEFA

loc_1CD91:                              ; CODE XREF: sub_1CD56+F↑j
                                        ; sub_1CD56+36↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CD56       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD96       proc near               ; CODE XREF: sub_1D0C2:loc_1D1A4↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CDC5
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 82h
                jnz     short loc_1CDC2
                mov     byte ptr [bx+26h], 0
                call    sub_1CE9E
                jmp     short loc_1CDC5
; ---------------------------------------------------------------------------

loc_1CDC2:                              ; CODE XREF: sub_1CD96+21↑j
                call    sub_1CEFA

loc_1CDC5:                              ; CODE XREF: sub_1CD96+F↑j
                                        ; sub_1CD96+2A↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CD96       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CDCA       proc near               ; CODE XREF: sub_1D0C2:loc_1D1B0↓p

var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CE41
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 81h
                jz      short loc_1CDF2

loc_1CDED:                              ; CODE XREF: sub_1CDCA+5C↓j
                                        ; sub_1CDCA+65↓j
                call    sub_1CEFA
                jmp     short loc_1CE41
; ---------------------------------------------------------------------------

loc_1CDF2:                              ; CODE XREF: sub_1CDCA+21↑j
                mov     ax, 1
                push    ax
                push    word_23626
                call    thk_char_add_age
                add     sp, 4
                mov     ax, 1
                push    ax
                push    [bp+var_2]
                call    thk_char_add_age
                add     sp, 4
                mov     ax, 64h ; 'd'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                mov     [bp+var_4], ax
                cmp     ax, 0Bh
                jge     short loc_1CE32
                cmp     ax, 0Ah
                jnz     short loc_1CDED
                mov     bx, [bp+var_2]
                mov     byte ptr [bx+26h], 0FFh
                jmp     short loc_1CDED ; CODE XREF: seg002:0831↑J
; ---------------------------------------------------------------------------
                align 2

loc_1CE32:                              ; CODE XREF: sub_1CDCA+57↑j
                mov     bx, [bp+var_2]
                mov     byte ptr [bx+26h], 0
                mov     word ptr [bx+5Eh], 1
                call    sub_1CE9E

loc_1CE41:                              ; CODE XREF: sub_1CDCA+F↑j
                                        ; sub_1CDCA+26↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CDCA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CE46       proc near               ; CODE XREF: sub_1CC5C+4↑p
                                        ; sub_1CC68+34↑p ...

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                call    loc_1CF8C
                mov     [bp+var_4], ax
                cmp     ax, 1Bh
                jz      short loc_1CE99
                push    ax
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     bx, ax
                cmp     byte ptr [bx+26h], 80h
                jb      short loc_1CE70
                call    sub_1CEFA
                jmp     short loc_1CE99
; ---------------------------------------------------------------------------
                align 2

loc_1CE70:                              ; CODE XREF: sub_1CE46+22↑j
                mov     bx, [bp+var_2]
                and     byte ptr [bx+26h], 2Fh
                mov     ax, [bp+arg_0]
                add     [bx+5Eh], ax
                mov     si, bx
                mov     ax, [si+74h]
                cmp     [bx+5Eh], ax
                jbe     short loc_1CE8A
                mov     [bx+5Eh], ax

loc_1CE8A:                              ; CODE XREF: sub_1CE46+3F↑j
                mov     ax, word_23626
                cmp     bx, ax
                jnz     short loc_1CE96
                mov     byte_1DBE7, 1

loc_1CE96:                              ; CODE XREF: sub_1CE46+49↑j
                call    sub_1CE9E

loc_1CE99:                              ; CODE XREF: sub_1CE46+10↑j
                                        ; sub_1CE46+27↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CE46       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CE9E       proc near               ; CODE XREF: sub_1C1EA+4D↑p
                                        ; ovl_2CAST1:C302↑p ...

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     [bp+var_2], 14h
                cmp     g_view_mode, 2
                jnz     short loc_1CEB4
                mov     [bp+var_2], 0Fh

loc_1CEB4:                              ; CODE XREF: sub_1CE9E+10↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+2]
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                push    si
                mov     ax, 1
                push    ax

loc_1CEC8:                              ; CODE XREF: seg002:07AD↑J
                call    thk_clear_text_rect
                add     sp, 8
                lea     ax, [si+1]
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aDone ; "* Done *"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     byte_1DC78, 1
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CE9E       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CEFA       proc near               ; CODE XREF: ovl_2CAST1:C2F5↑p
                                        ; sub_1C340+A0↑p ...

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     [bp+var_2], 14h
                cmp     g_view_mode, 2
                jnz     short loc_1CF10
                mov     [bp+var_2], 0Fh

loc_1CF10:                              ; CODE XREF: sub_1CEFA+10↑j
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                lea     ax, [si+2]
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                push    si
                mov     ax, 1
                push    ax
                call    thk_clear_text_rect
                add     sp, 8
                lea     ax, [si+1]      ; CODE XREF: seg002:050D↑J
                push    ax
                mov     ax, 0Ch
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSpellFailed_0 ; "* Spell Failed *"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 9
                push    ax
                call    thk_play_sound_effect
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CEFA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1CF5C       proc near               ; CODE XREF: seg002:0AE9↑J
                                        ; sub_1CB48+8↑p
                cmp     word_1DB9A, 0
                jz      short locret_1CF8B
                mov     ax, word_1DB9A
                mov     g_cur_text_win, ax
                push    ax
                call    thk_text_window_close
                add     sp, 2
                mov     word_1DB9A, 0
                mov     ax, g_main_text_win
                mov     g_cur_text_win, ax
                mov     ax, g_main_gfx_win
                mov     g_cur_gfx_win, ax
                sub     ax, ax

loc_1CF84:                              ; CODE XREF: seg002:062D↑J
                push    ax
                call    thk_res_3FA0
                add     sp, 2

locret_1CF8B:                           ; CODE XREF: sub_1CF5C+5↑j
                retn
; ---------------------------------------------------------------------------

loc_1CF8C:                              ; CODE XREF: sub_1C8F0+7↑p
                                        ; sub_1C994:loc_1C99A↑p ...
                push    bp
                mov     bp, sp
                sub     sp, 4
                mov     word ptr [bp-2], 14h
                mov     byte_1DC78, 0
                cmp     g_view_mode, 2
                jnz     short loc_1CFA8
                mov     word ptr [bp-2], 0Fh

loc_1CFA8:                              ; CODE XREF: sub_1CF5C+45↑j
                cmp     g_party_size, 1
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
                mov     al, byte ptr g_party_size
                add     al, 30h ; '0'
                mov     [bx+0Bh], al
                mov     ax, [bp-2]
                inc     ax
                push    ax
                mov     ax, 18h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr aL1ReturnToCast ; "L1'Return' to cast"
                call    thk_text_puts
                add     sp, 2
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, [bx+0Bh]
                sub     ah, ah
                push    ax
                mov     ax, 31h ; '1'
                push    ax
                call    thk_get_key_in_range_nowait
                add     sp, 4
                sub     ah, ah
                mov     [bp-4], ax

loc_1D004:                              ; CODE XREF: sub_1CF5C+5F↑j
                                        ; sub_1CF5C+67↑j
                cmp     g_view_mode, 2
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
                call    sub_1CEFA

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
                cmp     g_view_mode, 2
                jnz     short loc_1D06C
                mov     word ptr [bp-2], 0Fh

loc_1D06C:                              ; CODE XREF: sub_1CF5C+109↑j
                push    word ptr [bp-2]
                mov     ax, 16h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 315Eh
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_get_key_in_range_nowait
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
                call    sub_1CEFA

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
                                        ; DATA XREF: sub_1D0C2+10E↓o
                call    sub_1C130       ; jumptable 0001D0D3 case 1
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0DE:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+116↓o
                call    near ptr byte_1C132+0A0h ; jumptable 0001D0D3 case 5
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0E4:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+11A↓o
                call    sub_1C1EA       ; jumptable 0001D0D3 case 7
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0EA:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+120↓o
                call    loc_1C23E       ; jumptable 0001D0D3 case 10
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0F0:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+122↓o
                call    sub_1C320       ; jumptable 0001D0D3 case 11
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0F6:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+124↓o
                call    sub_1C340       ; jumptable 0001D0D3 case 12
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D0FC:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+126↓o
                call    sub_1CC3A       ; jumptable 0001D0D3 case 13
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D102:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+12A↓o
                call    sub_1C3EE       ; jumptable 0001D0D3 case 15
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D108:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+132↓o
                call    sub_1C4FC       ; jumptable 0001D0D3 case 19
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D10E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+13A↓o
                call    sub_1C550       ; jumptable 0001D0D3 case 23
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D114:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+146↓o
                call    sub_1C570       ; jumptable 0001D0D3 case 29
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D11A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+148↓o
                call    sub_1C590       ; jumptable 0001D0D3 case 30
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D120:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+150↓o
                call    sub_1C648       ; jumptable 0001D0D3 case 34
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D126:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+156↓o
                call    sub_1C68C       ; jumptable 0001D0D3 case 37
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D12C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+158↓o
                call    sub_1C722       ; jumptable 0001D0D3 case 38
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D132:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+16A↓o
                call    sub_1C774       ; jumptable 0001D0D3 case 47
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D138:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2:jpt_1D0D3↓o ...
                call    sub_1CBEC       ; jumptable 0001D0D3 cases 0,49
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D13E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+172↓o
                call    sub_1CC5C       ; jumptable 0001D0D3 case 51
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D144:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+114↓o ...
                call    near ptr byte_1C132+82h ; jumptable 0001D0D3 cases 4,52
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D14A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+176↓o
                call    sub_1CC68       ; jumptable 0001D0D3 case 53
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D150:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+17A↓o
                call    sub_1CCA8       ; jumptable 0001D0D3 case 55
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D156:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+17E↓o
                call    sub_1C7DA       ; jumptable 0001D0D3 case 57
                jmp     def_1D0D3       ; CODE XREF: seg002:0825↑J
                                        ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D15C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+182↓o
                call    sub_1CCB4       ; jumptable 0001D0D3 case 59
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D162:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+18A↓o
                call    sub_1C87C       ; jumptable 0001D0D3 case 63
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D168:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+18C↓o
                call    sub_1CCD6       ; jumptable 0001D0D3 case 64
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D16E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+190↓o
                call    sub_1C8A0       ; jumptable 0001D0D3 case 66
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D174:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+192↓o
                call    sub_1C8C8       ; jumptable 0001D0D3 case 67
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D17A:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+196↓o
                call    sub_1C8E0       ; jumptable 0001D0D3 case 69
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D180:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+198↓o
                call    sub_1CD16       ; jumptable 0001D0D3 case 70
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D186:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+19A↓o
                call    sub_1C8F0       ; jumptable 0001D0D3 case 71
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D18C:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+19C↓o
                call    sub_1C92A       ; jumptable 0001D0D3 case 72
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D192:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1A8↓o
                call    sub_1CD56       ; jumptable 0001D0D3 case 78
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D198:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1AA↓o
                call    sub_1C984       ; jumptable 0001D0D3 case 79
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D19E:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1AC↓o
                call    sub_1C994       ; jumptable 0001D0D3 case 80
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1A4:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1AE↓o
                call    sub_1CD96       ; jumptable 0001D0D3 case 81
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1AA:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1B2↓o
                call    sub_1CA00       ; jumptable 0001D0D3 case 83
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1B0:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1BA↓o
                call    sub_1CDCA       ; jumptable 0001D0D3 case 87
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1B6:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1BE↓o
                call    sub_1CA10       ; jumptable 0001D0D3 case 89
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1BC:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1C2↓o
                call    sub_1CA20       ; jumptable 0001D0D3 case 91
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1C2:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1C8↓o
                call    sub_1CAA4       ; jumptable 0001D0D3 case 94
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------

loc_1D1C8:                              ; CODE XREF: sub_1D0C2+11↑j
                                        ; DATA XREF: sub_1D0C2+1CA↓o
                call    sub_1CB10       ; jumptable 0001D0D3 case 95
                jmp     def_1D0D3       ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
; ---------------------------------------------------------------------------
jpt_1D0D3       dw offset loc_1D138     ; DATA XREF: sub_1D0C2+11↑r
                                        ; jump table for switch statement
                dw offset loc_1D0D8     ; jumptable 0001D0D3 case 1
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D144     ; jumptable 0001D0D3 cases 4,52
                dw offset loc_1D0DE     ; jumptable 0001D0D3 case 5
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D0E4     ; jumptable 0001D0D3 case 7
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D0EA     ; jumptable 0001D0D3 case 10
                dw offset loc_1D0F0     ; jumptable 0001D0D3 case 11
                dw offset loc_1D0F6     ; jumptable 0001D0D3 case 12
                dw offset loc_1D0FC     ; jumptable 0001D0D3 case 13
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D102     ; jumptable 0001D0D3 case 15
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D108     ; jumptable 0001D0D3 case 19
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D10E     ; jumptable 0001D0D3 case 23
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D114     ; jumptable 0001D0D3 case 29
                dw offset loc_1D11A     ; jumptable 0001D0D3 case 30
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D120     ; jumptable 0001D0D3 case 34
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D126     ; jumptable 0001D0D3 case 37
                dw offset loc_1D12C     ; jumptable 0001D0D3 case 38
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D132     ; jumptable 0001D0D3 case 47
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D138     ; jumptable 0001D0D3 cases 0,49
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D13E     ; jumptable 0001D0D3 case 51
                dw offset loc_1D144     ; jumptable 0001D0D3 cases 4,52
                dw offset loc_1D14A     ; jumptable 0001D0D3 case 53
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D150     ; jumptable 0001D0D3 case 55
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D156     ; jumptable 0001D0D3 case 57
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D15C     ; jumptable 0001D0D3 case 59
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D162     ; jumptable 0001D0D3 case 63
                dw offset loc_1D168     ; jumptable 0001D0D3 case 64
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D16E     ; jumptable 0001D0D3 case 66
                dw offset loc_1D174     ; jumptable 0001D0D3 case 67
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D17A     ; jumptable 0001D0D3 case 69
                dw offset loc_1D180     ; jumptable 0001D0D3 case 70
                dw offset loc_1D186     ; jumptable 0001D0D3 case 71
                dw offset loc_1D18C     ; jumptable 0001D0D3 case 72
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D192     ; jumptable 0001D0D3 case 78
                dw offset loc_1D198     ; jumptable 0001D0D3 case 79
                dw offset loc_1D19E     ; jumptable 0001D0D3 case 80
                dw offset loc_1D1A4     ; jumptable 0001D0D3 case 81
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D1AA     ; jumptable 0001D0D3 case 83
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D1B0     ; jumptable 0001D0D3 case 87
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D1B6     ; jumptable 0001D0D3 case 89
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D1BC     ; jumptable 0001D0D3 case 91
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset def_1D0D3     ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                dw offset loc_1D1C2     ; jumptable 0001D0D3 case 94
                dw offset loc_1D1C8     ; jumptable 0001D0D3 case 95
; ---------------------------------------------------------------------------

def_1D0D3:                              ; CODE XREF: sub_1D0C2+B↑j
                                        ; sub_1D0C2+11↑j ...
                pop     bp              ; jumptable 0001D0D3 default case, cases 2,3,6,8,9,14,16-18,20-22,24-28,31-33,35,36,39-46,48,50,54,56,58,60-62,65,68,73-77,82,84-86,88,90,92,93
                retn
sub_1D0C2       endp

ovl_2CAST1      ends

