; ===========================================================================

; Segment type: Pure code
ovl_2CAST2      segment byte public 'CODE' use16
                assume cs:ovl_2CAST2
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C130       proc near               ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 2, 0E8h, 75h, 11h, 88h, 46h, 0FEh
                                        ; DATA XREF: seg002:0038↑o
                db 3Ch, 1Bh, 74h, 26h, 0B8h, 1, 0, 50h, 0B8h, 5, 0, 50h
                db 0E8h, 5Fh, 0B0h, 83h, 0C4h, 4, 2Bh, 0C0h, 50h, 8Ah
                db 46h, 0FEh, 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h, 0E8h, 0ECh
                db 0AFh, 83h, 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh, 0E5h, 5Dh
                db 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 0E8h, 3Bh, 11h
                db 88h, 46h, 0FEh, 3Ch, 1Bh, 74h, 2Dh, 0B8h, 5, 0, 50h
                db 0B8h, 1, 0, 50h, 0E8h, 0F1h, 0ADh, 83h, 0C4h, 4, 5
                db 3, 0, 0A3h, 0C6h, 9Fh, 0B8h, 1, 0, 50h, 8Ah, 46h, 0FEh
                db 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h, 0E8h, 0ABh, 0AFh, 83h
                db 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh, 0E5h, 5Dh, 0C3h, 90h
                db 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 0E8h, 0F9h, 10h, 88h
                db 46h, 0FCh, 3Ch, 1Bh, 74h, 29h, 0E8h, 0DFh, 0AFh, 88h
                db 46h, 0FEh, 0C6h, 6, 0C2h, 9Fh, 4, 0C6h, 6, 0C3h, 9Fh
                db 1, 0B8h, 5, 0, 50h, 8Ah, 46h, 0FCh, 2Ah, 0E4h, 50h
                db 8Ah, 46h, 0FEh, 50h
sub_1C130       endp ; sp-analysis failed

byte_1C1DA      db 0E8h, 6Dh, 0AFh, 83h, 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh
                                        ; CODE XREF: seg002:08CD↑J
                db 0E5h, 5Dh, 0C3h, 90h

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C1EA       proc near               ; CODE XREF: seg002:07F5↑J
                                        ; sub_1CF2C:loc_1CF58↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C227
                mov     ax, 9
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                add     ax, 7
                mov     word_27816, ax
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C227:                              ; CODE XREF: sub_1C1EA+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C1EA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C22C       proc near               ; CODE XREF: sub_1CF2C:loc_1CF5E↓p

var_2           = byte ptr -2

; FUNCTION CHUNK AT C40D SIZE 00000004 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jnz     short loc_1C240 ; CODE XREF: seg002:08E5↑J
                jmp     loc_1C3D0
; ---------------------------------------------------------------------------

loc_1C240:                              ; CODE XREF: sub_1C22C+F↑j
                sub     ax, ax

loc_1C242:                              ; CODE XREF: seg002:0639↑J
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                mov     al, [bp+var_2]
                sub     ah, ah
                mov     si, ax
                push    si
                call    thk_res_3B80
                add     sp, 2
                mov     ax, 23h ; '#'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, [si-6980h]
                sub     ah, ah
                push    ax
                call    thk_res_1940
                add     sp, 6
                mov     ax, 20h ; ' '
                push    ax
                call    thk_res_0D22
                add     sp, 2
                call    thk_res_3E76
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3170h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     bx, si
                shl     bx, 1
                push    word ptr [bx-6056h]
                call    thk_res_1940
                add     sp, 6
                mov     ax, 10h
                push    ax
                mov     ax, 0Ah         ; CODE XREF: seg002:026D↑J
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3176h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                mov     al, byte_2767C
                sub     ah, ah
                push    ax
                call    thk_res_1940
                add     sp, 6
                mov     ax, 11h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 317Ch

loc_1C2F8:                              ; CODE XREF: seg002:0651↑J
                push    ax
                call    thk_res_1726
                add     sp, 2
                cmp     byte_27683, 0
                jz      short loc_1C30C
                mov     ax, 59h ; 'Y'   ; CODE XREF: seg002:08F1↑J
                jmp     short loc_1C30F
; ---------------------------------------------------------------------------
                align 2

loc_1C30C:                              ; CODE XREF: sub_1C22C+D8↑j
                mov     ax, 4Eh ; 'N'

loc_1C30F:                              ; CODE XREF: sub_1C22C+DD↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 0Fh
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3185h
                push    ax
                call    thk_res_1726
                add     sp, 2
                cmp     byte_27676, 0
                jz      short loc_1C344
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C347
; ---------------------------------------------------------------------------

loc_1C344:                              ; CODE XREF: sub_1C22C+111↑j
                mov     ax, 4Eh ; 'N'

loc_1C347:                              ; CODE XREF: sub_1C22C+116↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 3195h
                push    ax
                call    thk_res_1726
                add     sp, 2

loc_1C370:                              ; CODE XREF: seg002:0471↑J
                cmp     byte_27677, 0
                jz      short loc_1C37C
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C37F
; ---------------------------------------------------------------------------

loc_1C37C:                              ; CODE XREF: sub_1C22C+149↑j
                mov     ax, 4Eh ; 'N'

loc_1C37F:                              ; CODE XREF: sub_1C22C+14E↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 11h
                push    ax
                mov     ax, 13h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 31A6h
                push    ax
                call    thk_res_1726
                add     sp, 2
                cmp     byte_27681, 0
                jz      short loc_1C3B4
                mov     ax, 59h ; 'Y'
                jmp     short loc_1C3B7
; ---------------------------------------------------------------------------

loc_1C3B4:                              ; CODE XREF: sub_1C22C+181↑j
                mov     ax, 4Eh ; 'N'

loc_1C3B7:                              ; CODE XREF: sub_1C22C+186↑j
                push    ax
                call    thk_res_0D22
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_res_0D22
                add     sp, 2
                call    thk_res_56C6
                mov     byte_1DC78, 1

loc_1C3D0:                              ; CODE XREF: sub_1C22C+11↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1C3D6:                              ; CODE XREF: sub_1CF2C:loc_1CF64↓p
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C40D
                mov     ax, 3
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 4           ; CODE XREF: seg002:047D↑J
sub_1C22C       endp

                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1
; START OF FUNCTION CHUNK FOR sub_1C22C

loc_1C40D:                              ; CODE XREF: sub_1C22C+1B8↑j
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1C22C
; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C412       proc near               ; CODE XREF: sub_1CF2C:loc_1CF6A↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C42C
                cmp     byte_1DC34, 0FFh
                jnb     short loc_1C424
                inc     byte_1DC34

loc_1C424:                              ; CODE XREF: sub_1C412+C↑j
                call    near ptr byte_1D0C2+7Ch
                mov     byte_1DC78, 1

locret_1C42C:                           ; CODE XREF: sub_1C412+5↑j
                retn
sub_1C412       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C42E       proc near               ; CODE XREF: sub_1CF2C:loc_1CF70↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C465
                mov     ax, 1
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 4
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1   ; CODE XREF: seg002:086D↑J

loc_1C465:                              ; CODE XREF: sub_1C42E+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C42E       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C46A       proc near               ; CODE XREF: sub_1CF2C:loc_1CF76↓p

var_4           = byte ptr -4
var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_1D2AE
                mov     [bp+var_4], al
                cmp     al, 1Bh
                jz      short loc_1C4B0
                mov     byte_1DC78, 1
                call    thk_2COMBAT_A7EA
                mov     [bp+var_2], al
                mov     al, byte_27815
                cmp     [bp+var_4], al
                jnb     short loc_1C492
                call    loc_1D170       ; CODE XREF: seg002:0B31↑J
                jmp     short loc_1C4B0
; ---------------------------------------------------------------------------

loc_1C492:                              ; CODE XREF: sub_1C46A+21↑j
                mov     byte_27812, 5
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_4]
                sub     ah, ah
                push    ax
                mov     al, [bp+var_2]
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C4B0:                              ; CODE XREF: sub_1C46A+E↑j
                                        ; sub_1C46A+26↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C46A       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C4B4       proc near               ; CODE XREF: sub_1CF2C:loc_1CF7C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C4EA
                mov     byte_1DC78, 1
                mov     ax, 6
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C4EA:                              ; CODE XREF: sub_1C4B4+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C4B4       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C4EE       proc near               ; CODE XREF: sub_1CF2C:loc_1CF82↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C520
                mov     byte_1DC78, 1
                mov     byte_27812, 6
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C520:                              ; CODE XREF: sub_1C4EE+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C4EE       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C524       proc near               ; CODE XREF: sub_1CF2C:loc_1CF88↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE       ; CODE XREF: seg002:0879↑J
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C568
                mov     byte_1DC78, 1
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1C546
                call    loc_1D170
                jmp     short loc_1C568
; ---------------------------------------------------------------------------

loc_1C546:                              ; CODE XREF: sub_1C524+1B↑j
                mov     ax, 1
                push    ax
                mov     ax, 5
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 6
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C568:                              ; CODE XREF: sub_1C524+E↑j
                                        ; sub_1C524+20↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C524       endp


; =============== S U B R O U T I N E =======================================


sub_1C56C       proc near               ; CODE XREF: sub_1CF2C:loc_1CF8E↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C586
                cmp     byte_1DC35, 0FFh
                jnb     short loc_1C57E
                inc     byte_1DC35

loc_1C57E:                              ; CODE XREF: sub_1C56C+C↑j
                call    near ptr byte_1D0C2+7Ch
                mov     byte_1DC78, 1

locret_1C586:                           ; CODE XREF: sub_1C56C+5↑j
                retn
sub_1C56C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C588       proc near               ; CODE XREF: sub_1CF2C:loc_1CF94↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C5A7
                mov     byte_1DC78, 1
                test    byte_231F0, 8
                jz      short loc_1C5A0
                call    loc_1D170
                jmp     short locret_1C5A7
; ---------------------------------------------------------------------------

loc_1C5A0:                              ; CODE XREF: sub_1C588+11↑j
                inc     byte_27818
                call    near ptr byte_1D0C2+7Ch

locret_1C5A7:                           ; CODE XREF: sub_1C588+5↑j
                                        ; sub_1C588+16↑j
                retn
sub_1C588       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5A8       proc near               ; CODE XREF: sub_1CF2C:loc_1CF9A↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2           ; CODE XREF: seg002:0885↑J
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C5E3
                mov     byte_1DC78, 1
                mov     al, byte_27815

loc_1C5C0:                              ; CODE XREF: seg002:029D↑J
                cmp     [bp+var_2], al
                jnb     short loc_1C5CA
                call    loc_1D170
                jmp     short loc_1C5E3
; ---------------------------------------------------------------------------

loc_1C5CA:                              ; CODE XREF: sub_1C5A8+1B↑j
                mov     word_27816, 64h ; 'd'
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah

loc_1C5D8:                              ; CODE XREF: seg002:01A1↑J
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1C5E3:                              ; CODE XREF: sub_1C5A8+E↑j
                                        ; sub_1C5A8+20↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C5A8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C5E8       proc near               ; CODE XREF: sub_1CF2C:loc_1CFA0↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C61B
                mov     byte_27812, 8
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C61B:                              ; CODE XREF: sub_1C5E8+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C5E8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C620       proc near               ; CODE XREF: sub_1CF2C:loc_1CFA6↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C656
                mov     ax, 1
                push    ax
                mov     ax, 7
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C656:                              ; CODE XREF: sub_1C620+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C620       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C65A       proc near               ; CODE XREF: sub_1CF2C:loc_1CFAC↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C68C
                mov     byte_27812, 9   ; CODE XREF: seg002:0891↑J
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C68C:                              ; CODE XREF: sub_1C65A+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C65A       endp


; =============== S U B R O U T I N E =======================================


sub_1C690       proc near               ; CODE XREF: sub_1CF2C:loc_1CFB2↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C6AF
                mov     byte_1DC78, 1
                test    byte_231F0, 1
                jz      short loc_1C6A8
                call    loc_1D170
                jmp     short locret_1C6AF
; ---------------------------------------------------------------------------

loc_1C6A8:                              ; CODE XREF: sub_1C690+11↑j
                inc     byte_27814
                call    near ptr byte_1D0C2+7Ch

locret_1C6AF:                           ; CODE XREF: sub_1C690+5↑j
                                        ; sub_1C690+16↑j
                retn
sub_1C690       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C6B0       proc near               ; CODE XREF: sub_1CF2C+8C↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C6F3
                mov     byte_1DC78, 1
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1C6D2
                call    loc_1D170
                jmp     short loc_1C6F3
; ---------------------------------------------------------------------------

loc_1C6D2:                              ; CODE XREF: sub_1C6B0+1B↑j
                mov     ax, 0Ah
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 3
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6           ; CODE XREF: seg002:0B19↑J

loc_1C6F3:                              ; CODE XREF: sub_1C6B0+E↑j
                                        ; sub_1C6B0+20↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C6B0       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C6F8       proc near               ; CODE XREF: sub_1CF2C+92↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C72E
                mov     ax, 14h
                push    ax
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C72E:                              ; CODE XREF: sub_1C6F8+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C6F8       endp


; =============== S U B R O U T I N E =======================================


sub_1C732       proc near               ; CODE XREF: sub_1CF2C+98↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C75A
                mov     ax, 1           ; CODE XREF: seg002:089D↑J
                push    ax
                mov     ax, 0Bh
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C75A:                           ; CODE XREF: sub_1C732+5↑j
                retn
sub_1C732       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C75C       proc near               ; CODE XREF: sub_1CF2C+9E↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D23A
                or      ax, ax
                jz      short loc_1C7AD
                mov     ax, 9
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     [bp+var_2], al
                cmp     al, 7
                jnz     short loc_1C78F
                mov     ax, 4
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     byte_27819, al

loc_1C78F:                              ; CODE XREF: sub_1C75C+20↑j
                mov     al, [bp+var_2]
                mov     byte_27812, al
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1C7AD:                              ; CODE XREF: sub_1C75C+B↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C75C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1C7B2       proc near               ; CODE XREF: sub_1CF2C+A4↓p

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1C7E9
                mov     ax, 13h
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 1
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6           ; CODE XREF: seg002:0849↑J
                mov     byte_1DC78, 1   ; CODE XREF: seg002:0B0D↑J

loc_1C7E9:                              ; CODE XREF: sub_1C7B2+E↑j
                mov     sp, bp
                pop     bp
                retn
sub_1C7B2       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C7EE       proc near               ; CODE XREF: sub_1CF2C+AA↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C819
                mov     ax, 7
                push    ax
                mov     ax, 9
                push    ax
                call    thk_2COMBAT_A82C
                add     sp, 4
                mov     ax, 2
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C819:                           ; CODE XREF: sub_1C7EE+5↑j
                retn
sub_1C7EE       endp


; =============== S U B R O U T I N E =======================================


sub_1C81A       proc near               ; CODE XREF: sub_1CF2C+B0↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C84E
                mov     ax, 15h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                add     ax, 18h
                mov     word_27816, ax
                inc     byte_2781A
                sub     ax, ax
                push    ax
                push    ax
                mov     al, byte_1DD58
                sub     ah, ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1C84E:                           ; CODE XREF: sub_1C81A+5↑j
                retn
sub_1C81A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================


sub_1C850       proc near               ; CODE XREF: sub_1CF2C+B6↓p
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1C86A
                cmp     byte_1DC36, 0FFh
                jnb     short loc_1C862
                inc     byte_1DC36

loc_1C862:                              ; CODE XREF: sub_1C850+C↑j
                call    near ptr byte_1D0C2+7Ch
                mov     byte_1DC78, 1

locret_1C86A:                           ; CODE XREF: sub_1C850+5↑j
                retn
sub_1C850       endp

; ---------------------------------------------------------------------------
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    2
                db 0E8h
                db  39h ; 9
                db  0Ah
                db  88h
                db  46h ; F
                db 0FEh
                db  3Ch ; <
                db  1Bh
                db  74h ; t
                db  1Eh
                db 0C7h
                db    6
                db 0C6h
                db  9Fh
                db 0E8h
                db    3
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db  8Ah
                db  46h ; F
                db 0FEh
                db  2Ah ; *
                db 0E4h
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db 0B8h
                db 0A8h
                db  83h
                db 0C4h
                db    6
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h
                db 0E8h
                db  99h
                db    9
                db  0Bh
                db 0C0h
                db  74h ; t
                db  24h ; $
                db 0B8h
                db    4
                db    0
                db  50h ; P
                db 0B8h
                db  10h
                db    0
                db  50h ; P
                db 0E8h
                db 0FAh
                db 0A8h
                db  83h
                db 0C4h
                db    4
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0B8h
                db  0Ah
                db    0
                db  50h ; P
                db 0E8h
                db  89h
                db 0A8h
                db  83h
                db 0C4h
                db    6
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db 0C3h
                db 0E8h
                db  6Dh ; m
                db    9
                db  0Bh
                db 0C0h
                db  74h ; t
                db  2Dh ; -
                db 0B8h
                db 0A1h
                db    0
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db  9Ah
                db 0A6h
                db  83h
                db 0C4h
                db    4
                db    5
                db  27h ; '
                db    0
                db 0A3h
                db 0C6h
                db  9Fh
                db 0FEh
                db    6
                db 0CAh
                db  9Fh
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db  50h ; P
                db 0A0h
                db    8
                db    5
                db  2Ah ; *
                db 0E4h
                db  50h ; P
                db 0E8h
                db  54h ; T
                db 0A8h
                db  83h
                db 0C4h
                db    6
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db 0C3h
                db  90h
                db 0E8h
                db  37h ; 7
                db    9
                db  0Bh
                db 0C0h
                db  74h ; t
                db  20h
                db 0C6h
                db    6
                db 0C2h
                db  9Fh
                db    3
                db 0C6h
                db    6
                db 0C3h
                db  9Fh
                db    1
                db 0B8h
                db    6
                db    0
                db  50h ; P
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db 0B8h
                db  0Ah
                db    0
                db  50h ; P
                db 0E8h
                db  2Bh ; +
                db 0A8h
                db  83h
                db 0C4h
                db    6
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db 0C3h
                db 0E8h
                db  0Fh
                db    9
                db  0Bh
                db 0C0h
                db  74h ; t
                db  13h
                db  80h
                db  3Eh ; >
                db 0E3h
                db    3
                db 0FFh
                db  73h ; s
                db    4
                db 0FEh
                db    6
                db 0E3h
                db    3
                db 0E8h
                db    1
                db    8
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db 0C3h
                db  90h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    2
                db  8Bh
                db  1Eh
                db 0D6h
                db  5Dh ; ]
                db  8Ah
                db  47h ; G
                db  71h ; q
                db  88h
                db  46h ; F
                db 0FEh
                db  3Ch ; <
                db  10h
                db  73h ; s
                db    5
                db 0B1h
                db    4
                db 0D2h
                db  66h ; f
                db 0FEh
                db  8Ah
                db  46h ; F
                db 0FEh
                db  2Ah ; *
                db 0E4h
                db  50h ; P
                db 0E8h
                db    4
                db    0
                db  8Bh
                db 0E5h
                db  5Dh ; ]
                db 0C3h
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db  0Ch
                db  56h ; V
                db 0C6h
                db  46h ; F
                db 0F8h
                db    0
                db 0C6h
                db  46h ; F
                db 0FEh
                db    0
                db 0C6h
                db  46h ; F
                db 0F6h
                db    0
                db 0E8h
                db 0BAh
                db    8
                db  0Bh
                db 0C0h
                db  75h ; u
                db    3
                db 0E9h
                db 0AFh
                db    0
                db  80h
                db  3Eh ; >
                db 0CBh
                db  9Fh
                db    0
                db  74h ; t
                db    3
                db 0E9h
                db 0A2h
                db    0
                db 0FEh
                db    6
                db 0CBh
                db  9Fh
                db 0A0h
                db    8
                db    5
                db  88h
                db  46h ; F
byte_1C99A      db 0FCh, 80h, 7Eh, 4, 0, 74h, 8, 3Ch, 0Ah, 76h, 4, 0C6h
                                        ; CODE XREF: seg002:07DD↑J
                db 46h, 0FCh, 0Ah, 8Ah, 46h, 4, 2Ah, 0E4h, 8Bh, 0F0h, 8Ah
                db 46h, 0F8h, 2Ah, 0E4h, 50h, 0E8h, 59h, 0A6h, 83h, 0C4h
                db 2, 80h, 3Eh, 33h, 9Eh, 0, 74h, 58h, 0C6h, 46h, 0FAh
                db 0FFh, 80h, 7Eh, 4, 0, 74h, 0Eh, 56h, 0B8h, 1, 0, 50h
                db 0E8h, 0A1h, 0A5h, 83h, 0C4h, 4, 88h, 46h, 0FAh, 8Ah
                db 5Eh, 0F8h, 2Ah, 0FFh, 8Ah, 46h, 0FAh, 38h, 87h, 80h
byte_1C9E6      db 96h, 77h, 32h, 0FEh, 46h, 0FEh, 2Bh, 0C0h, 50h, 0E8h
                                        ; CODE XREF: seg002:0B01↑J
                db 98h, 0A6h, 83h, 0C4h, 2, 0E8h, 32h, 0A6h, 0B8h, 0B9h
                db 31h, 50h, 0E8h, 53h, 0A5h, 83h, 0C4h, 2, 0C6h, 6, 0CCh
                db 9Fh, 1, 8Ah, 46h, 0F8h, 0A2h, 0CEh, 9Fh, 0E8h, 0D2h
                db 0A5h, 0C6h, 6, 0CCh, 9Fh, 0, 0FEh, 4Eh, 0F8h, 0E8h
                db 63h, 0A6h, 0FEh, 46h, 0F8h, 80h, 7Eh, 0F8h, 0Bh, 75h
                db 4, 0C6h, 46h, 0FCh, 1, 0FEh, 4Eh, 0FCh, 75h, 83h, 80h
                db 7Eh, 0FEh, 0, 75h, 3, 0E8h, 3Ah, 7, 0C6h, 6, 0CAh, 9Fh
                db 0, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 4, 0E8h, 5Dh, 7, 89h, 46h, 0FCh, 3Dh, 1Bh, 0, 74h, 18h
                db 0C6h
byte_1CA52      db 6, 28h, 4, 1, 50h, 0E8h, 8, 0A7h, 83h, 0C4h, 2, 89h
                                        ; CODE XREF: seg002:0A89↑J
                db 46h, 0FEh, 8Bh, 0D8h, 80h, 47h, 71h, 6, 0E8h, 0D5h
                db 6, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 2, 0E8h, 37h, 8, 88h, 46h, 0FEh, 3Ch, 1Bh, 74h, 2Ch
                db 0B8h, 0Ch, 0, 50h, 0B8h, 1, 0, 50h, 0E8h, 0EDh
byte_1CA88      db 0A4h, 83h, 0C4h, 4, 5, 3, 0, 0A3h, 0C6h, 9Fh, 2Bh, 0C0h
                                        ; CODE XREF: seg002:0801↑J
                db 50h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h
                db 0E8h, 0A8h, 0A6h, 83h, 0C4h, 6, 0C6h, 6, 28h, 4, 1
                db 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4
                db 0E8h, 0F7h, 7, 88h, 46h, 0FCh, 3Ch, 1Bh, 74h, 29h, 0E8h
                db 0DDh, 0A6h, 88h, 46h, 0FEh, 0C6h, 6, 0C2h, 9Fh, 1, 0C6h
                db 6, 0C3h, 9Fh, 1, 0B8h, 6, 0, 50h, 8Ah, 46h, 0FCh, 2Ah
                db 0E4h, 50h, 8Ah, 46h, 0FEh, 50h, 0E8h, 6Bh, 0A6h, 83h
                db 0C4h, 6, 0C6h, 6, 28h, 4, 1, 8Bh, 0E5h, 5Dh, 0C3h, 90h
                db 0E8h, 4Bh, 7, 0Bh, 0C0h, 74h, 1Dh, 0C6h, 6, 0C2h, 9Fh
                db 2, 0C6h, 6, 0C3h, 9Fh, 1, 2Bh, 0C0h, 2 dup(50h), 0B8h
                db 0Ah, 0, 50h, 0E8h, 42h, 0A6h, 83h, 0C4h, 6, 0C6h, 6
                db 28h, 4, 1, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 2
                db 0E8h, 93h, 7, 88h, 46h, 0FEh, 3Ch, 1Bh, 74h, 2Ch, 0C6h
                db 6, 28h, 4, 1, 0A0h, 0C5h, 9Fh, 38h, 46h, 0FEh, 73h
                db 5, 0E8h, 3Eh, 6, 0EBh, 1Ah, 0C7h, 6, 0C6h, 9Fh, 19h
                db 0, 0B8h, 3, 0, 50h, 8Ah, 46h
byte_1CB40      db 0FEh, 2Ah, 0E4h, 50h, 0B8h, 5, 0, 50h, 0E8h, 0FFh, 0A5h
                                        ; CODE XREF: seg002:0A65↑J
                db 83h, 0C4h, 6, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 2, 0E8h, 53h, 7, 88h, 46h, 0FEh, 3Ch, 1Bh
                db 74h, 23h, 0C6h, 6, 28h, 4, 1, 0C6h, 6, 0C2h, 9Fh, 5
                db 0C6h, 6, 0C3h, 9Fh, 1, 0B8h, 6, 0, 50h, 8Ah, 46h, 0FEh
                db 2Ah, 0E4h, 50h, 0B8h, 5, 0, 50h, 0E8h, 0C8h, 0A5h, 83h
                db 0C4h, 6, 8Bh, 0E5h, 5Dh, 0C3h, 90h

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB8A       proc near               ; CODE XREF: seg002:0AF5↑J

var_2           = byte ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp+var_2], al
                cmp     al, 1Bh
                jz      short loc_1CBD4
                mov     byte_1DC78, 1   ; CODE XREF: seg002:080D↑J
                mov     al, byte_27815
                cmp     [bp+var_2], al
                jnb     short loc_1CBAC
                call    loc_1D170
                jmp     short loc_1CBD4
; ---------------------------------------------------------------------------

loc_1CBAC:                              ; CODE XREF: sub_1CB8A+1B↑j
                mov     ax, 31h ; '1'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                add     ax, 0Bh
                mov     word_27816, ax
                mov     ax, 4
                push    ax
                mov     al, [bp+var_2]
                sub     ah, ah
                push    ax
                mov     ax, 3
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CBD4:                              ; CODE XREF: sub_1CB8A+E↑j
                                        ; sub_1CB8A+20↑j
                mov     sp, bp
                pop     bp
                retn
sub_1CB8A       endp

; ---------------------------------------------------------------------------
                call    sub_1D23A
                or      ax, ax          ; CODE XREF: seg002:01AD↑J
                jz      short locret_1CBF6
                mov     bx, word_23626
                mov     al, [bx+71h]
                sub     ah, ah
                shr     ax, 1
                add     byte_1DC37, al
                call    near ptr byte_1D0C2+7Ch
                mov     byte_1DC78, 1

locret_1CBF6:                           ; CODE XREF: ovl_2CAST2:CBDD↑j
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp-2], al
                cmp     al, 1Bh
                jz      short loc_1CC2F
                mov     byte_27819, 1
                mov     byte_27812, 7
                mov     byte_27813, 1
                sub     ax, ax
                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CC2F:                              ; CODE XREF: ovl_2CAST2:CC06↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1CC62
                mov     ax, 21h ; '!'
                push    ax
                mov     ax, 1
                push    ax
                call    thk_res_1C88
                add     sp, 4
                add     ax, 7
                mov     word_27816, ax
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1CC62:                           ; CODE XREF: ovl_2CAST2:CC39↑j
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                sub     sp, 4
                call    sub_1D1A6
                mov     [bp-4], ax
                cmp     ax, 1Bh
                jz      short loc_1CCD7
                mov     byte_1DC78, 1
                push    ax
                call    thk_res_37B6
                add     sp, 2
                mov     [bp-2], ax
                cmp     byte_27811, 0
                jz      short loc_1CC90

loc_1CC8B:                              ; CODE XREF: ovl_2CAST2:CC9B↓j
                                        ; ovl_2CAST2:CCA5↓j
                call    loc_1D170
                jmp     short loc_1CCD7
; ---------------------------------------------------------------------------

loc_1CC90:                              ; CODE XREF: ovl_2CAST2:CC89↑j
                inc     byte_27811
                mov     bx, [bp-2]
                cmp     byte ptr [bx+26h], 0
                jnz     short loc_1CC8B
                mov     byte ptr [bx+26h], 40h ; '@'
                cmp     byte ptr [bx+73h], 0
                jz      short loc_1CC8B
                dec     byte ptr [bx+73h]
                mov     word ptr [bx+5Eh], 0
                mov     al, [bx+4Ch]
                sub     ah, ah
                mov     cl, [bx+4Dh]
                sub     ch, ch
                add     ax, cx          ; CODE XREF: seg002:08FD↑J
                add     ax, 0Ah
                mov     word_27816, ax
                shl     word_27816, 1
                inc     byte_27810
                sub     ax, ax
                push    ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6

loc_1CCD7:                              ; CODE XREF: ovl_2CAST2:CC73↑j
                                        ; ovl_2CAST2:CC8E↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2
                call    sub_1D23A
                or      ax, ax
                jz      short locret_1CD03
                mov     byte_27812, 5
                mov     byte_27813, 1
                mov     ax, 6
                push    ax
                sub     ax, ax
                push    ax
                mov     ax, 0Ah
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

locret_1CD03:                           ; CODE XREF: ovl_2CAST2:CCE1↑j
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp-2], al
                cmp     al, 1Bh
                jz      short loc_1CD3C
                mov     byte_27819, 2
                mov     byte_27812, 7
                mov     byte_27813, 1
                mov     ax, 3
                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CD3C:                              ; CODE XREF: ovl_2CAST2:CD12↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp-2], al
                cmp     al, 1Bh
                jz      short loc_1CD78
                mov     byte_27819, 3
                mov     byte_27812, 7
                mov     byte_27813, 1
                mov     ax, 4
                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CD78:                              ; CODE XREF: ovl_2CAST2:CD4E↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                push    bp
                mov     bp, sp
                sub     sp, 2
                call    sub_1D2AE
                mov     [bp-2], al
                cmp     al, 1Bh
                jz      short loc_1CDB6
                mov     ax, 190h
                push    ax
                mov     ax, 0FFh
                push    ax
                call    thk_res_1C88
                add     sp, 4
                mov     word_27816, ax
                mov     ax, 1
                push    ax
                mov     al, [bp-2]
                sub     ah, ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_2COMBAT_8696
                add     sp, 6
                mov     byte_1DC78, 1

loc_1CDB6:                              ; CODE XREF: ovl_2CAST2:CD8A↑j
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db  55h ; U
                db  8Bh
                db 0ECh
                db  83h
                db 0ECh
                db    4
                db  57h ; W
                db  56h ; V
                db 0E8h
                db  75h ; u
                db    4
                db  0Bh
                db 0C0h
                db  74h ; t
                db  71h ; q
                db 0C6h
                db    6
                db  28h ; (
                db    4
                db    1
                db 0B8h
                db  5Bh ; [
                db    0
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db  9Dh
                db 0A1h
                db  83h
                db 0C4h
                db    4
                db    5
                db    9
                db    0
                db 0A3h
                db 0C6h
                db  9Fh
                db  2Bh ; +
                db 0C0h
                db  50h ; P
                db  50h ; P
                db 0B8h
                db  0Ah
                db    0
                db  50h ; P
                db 0E8h
                db  5Dh ; ]
                db 0A3h
                db  83h
                db 0C4h
                db    6
                db  2Bh ; +
                db 0FFh
                db  8Bh
                db  76h ; v
                db 0FEh
                db 0EBh
                db  37h ; 7
                db  90h
                db  57h ; W
                db 0E8h
                db  66h ; f
                db 0A3h
                db  83h
                db 0C4h
                db    2
                db  8Bh
                db 0F0h
                db 0B8h
                db  5Bh ; [
                db    0
                db  50h ; P
                db 0B8h
                db    1
                db    0
                db  50h ; P
                db 0E8h
                db  6Ah ; j
                db 0A1h
                db  83h
                db 0C4h
                db    4
                db    5
                db    9
                db    0
                db 0A3h
                db 0C6h
                db  9Fh
                db  80h
                db  7Ch ; |
                db  26h ; &
                db  80h
                db  73h ; s
                db  12h
                db  80h
                db  64h ; d
                db  26h ; &
                db  2Fh ; /
                db    1
                db  44h ; D
                db  5Eh ; ^
                db  8Bh
                db  44h ; D
                db  74h ; t
                db  39h ; 9
                db  44h ; D
                db  5Eh ; ^
                db  76h ; v
                db    3
                db  89h
                db  44h ; D
                db  5Eh ; ^
                db  47h ; G
                db  3Bh ; ;
                db  3Eh ; >
byte_1CE30      db 26h, 4, 7Ch, 0C4h, 89h, 7Eh, 0FCh, 89h, 76h, 0FEh, 5Eh
                                        ; CODE XREF: seg002:0831↑J
                db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 2, 0E8h, 65h, 4, 88h, 46h, 0FEh, 3Ch, 1Bh, 74h, 28h
                db 0C6h, 6, 28h, 4, 1, 0C6h, 6, 0C9h, 9Fh, 4, 0C6h, 6
                db 0C2h, 9Fh, 7, 0C6h, 6, 0C3h, 9Fh, 1, 0B8h, 1, 0, 50h
                db 8Ah, 46h, 0FEh, 2Ah, 0E4h, 50h, 0B8h, 1, 0, 50h, 0E8h
                db 0D5h, 0A2h, 83h, 0C4h, 6, 8Bh, 0E5h, 5Dh, 0C3h, 55h
                db 8Bh, 0ECh, 83h, 0ECh, 2, 56h, 0E8h, 28h, 4, 88h, 46h
                db 0FEh, 3Ch, 1Bh, 74h, 24h, 0C6h, 6, 28h, 4, 1, 2Ah, 0E4h
                db 8Bh, 0F0h, 8Bh, 0DEh, 0D1h, 0E3h, 8Bh, 87h, 0AAh, 9Fh
                db 0D1h, 0E8h, 0A3h, 0C6h, 9Fh, 2Bh, 0C0h, 50h, 56h, 0B8h
                db 2, 0, 50h, 0E8h, 9Ch, 0A2h, 83h, 0C4h, 6, 5Eh, 8Bh
                db 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4, 57h
                db 56h, 0E8h, 79h, 3, 0Bh, 0C0h, 74h, 51h, 0C6h, 6, 28h
byte_1CEC8      db 4, 1, 80h, 3Eh, 0CDh, 9Fh, 0, 74h, 5, 0E8h, 9Ch, 2
                                        ; CODE XREF: seg002:07AD↑J
                db 0EBh, 40h, 0FEh, 6, 0CDh, 9Fh, 0B8h, 5, 0, 50h, 0FFh
                db 36h, 0D6h, 5Dh, 0E8h, 0D5h, 0A7h, 83h, 0C4h, 4, 2Bh
                db 0FFh, 8Bh, 76h, 0FEh, 0EBh, 1Bh, 90h, 57h, 0E8h, 6Eh
                db 0A2h, 83h, 0C4h, 2, 8Bh, 0F0h, 80h, 7Ch, 26h, 0FFh
                db 74h, 4, 0C6h, 44h, 26h, 0, 8Bh, 44h, 74h, 89h, 44h
                db 5Eh, 47h, 3Bh, 3Eh, 26h, 4, 7Ch, 0E0h, 89h, 7Eh, 0FCh
                db 89h, 76h, 0FEh, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 0C6h
                db 6, 0CAh, 9Fh, 1, 2Bh, 0C0h, 50h, 0E8h, 43h, 0FAh, 83h
                db 0C4h, 2, 0C3h, 90h

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CF2C       proc near               ; CODE XREF: seg002:050D↑J

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                mov     ax, [bp+arg_0]
                sub     ax, 2           ; switch 31 cases
                cmp     ax, 5Bh
                jbe     short loc_1CF3D
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF3D:                              ; CODE XREF: sub_1CF2C+C↑j
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1CF40[bx] ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1CF46:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C130       ; jumptable 0001CF40 case 2
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF4C:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    near ptr byte_1C132+38h ; jumptable 0001CF40 case 3
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF52:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    near ptr byte_1C132+7Ah ; jumptable 0001CF40 case 6
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF58:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C1EA       ; jumptable 0001CF40 case 8
                jmp     near ptr byte_1D0C2+7Ah
                                        ; CODE XREF: seg002:0AE9↑J
; ---------------------------------------------------------------------------

loc_1CF5E:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C22C       ; jumptable 0001CF40 case 9
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF64:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    loc_1C3D6       ; jumptable 0001CF40 case 14
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF6A:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C412       ; jumptable 0001CF40 case 16
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF70:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C42E       ; jumptable 0001CF40 case 17
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF76:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C46A       ; jumptable 0001CF40 case 18
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF7C:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C4B4       ; jumptable 0001CF40 case 20
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF82:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; seg002:062D↑J
                                        ; DATA XREF: ...
                call    sub_1C4EE       ; jumptable 0001CF40 case 21
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF88:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C524       ; jumptable 0001CF40 case 22
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF8E:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C56C       ; jumptable 0001CF40 case 24
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF94:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C588       ; jumptable 0001CF40 case 25
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CF9A:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C5A8       ; jumptable 0001CF40 case 26
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CFA0:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C5E8       ; jumptable 0001CF40 case 27
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CFA6:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C620       ; jumptable 0001CF40 case 28
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CFAC:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C65A       ; jumptable 0001CF40 case 31
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------

loc_1CFB2:                              ; CODE XREF: sub_1CF2C+14↑j
                                        ; DATA XREF: sub_1CF2C:jpt_1CF40↓o
                call    sub_1C690       ; jumptable 0001CF40 case 32
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C6B0
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C6F8
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C732
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C75C
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C7B2
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C7EE
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C81A
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                call    sub_1C850
                jmp     near ptr byte_1D0C2+7Ah
; ---------------------------------------------------------------------------
                db 0E8h
                db  81h
                db 0F8h
                db 0E9h
                db  4Eh ; N
                db    1
                db 0E8h
                db 0ADh
                db 0F8h
                db 0E9h
                db  48h ; H
                db    1
                db 0E8h
                db 0D3h
                db 0F8h
                db 0E9h
                db  42h ; B
                db    1
                db 0E8h
                db    3
                db 0F9h
                db 0E9h
                db  3Ch ; <
                db    1
                db 0E8h
                db  25h ; %
                db 0F9h
                db 0E9h
                db  36h ; 6
                db    1
                db 0E8h
                db  3Bh ; ;
                db 0F9h
                db 0E9h
                db  30h ; 0
                db    1
                db 0E8h
                db  31h ; 1
                db 0FAh
                db 0E9h
                db  2Ah ; *
                db    1
                db 0E8h
                db  59h ; Y
                db 0FAh
                db 0E9h
                db  24h ; $
                db    1
                db 0E8h
                db  93h
                db 0FAh
                db 0E9h
                db  1Eh
                db    1
                db 0E8h
                db 0CBh
                db 0FAh
                db 0E9h
                db  18h
                db    1
                db 0E8h
                db 0EBh
                db 0FAh
                db 0E9h
                db  12h
                db    1
                db 0E8h
                db  25h ; %
                db 0FBh
                db 0E9h
                db  0Ch
                db    1
                db 0E8h
                db  57h ; W
                db 0FBh
                db 0E9h
                db    6
                db    1
                db 0E8h
                db  9Fh
                db 0FBh
                db 0E9h
                db    0
                db    1
                db 0E8h
                db 0B9h
                db 0FBh
                db 0E9h
                db 0FAh
                db    0
                db 0E8h
                db 0EFh
                db 0FBh
                db 0E9h
                db 0F4h
                db    0
                db 0E8h
                db  19h
                db 0FCh
                db 0E9h
                db 0EEh
                db    0
                db 0E8h
                db  8Bh
                db 0FCh
                db 0E9h
                db 0E8h
                db    0
                db 0E8h
                db 0ADh
                db 0FCh
                db 0E9h
                db 0E2h
                db    0
                db 0E8h
                db 0E3h
                db 0FCh
                db 0E9h
                db 0DCh
                db    0
                db 0E8h
                db  19h
                db 0FDh
                db 0E9h
                db 0D6h
                db    0
                db 0E8h
                db  51h ; Q
                db 0FDh
                db 0E9h
                db 0D0h
                db    0
                db 0E8h
                db 0D1h
                db 0FDh
                db 0E9h
                db 0CAh
                db    0
                db 0E8h
                db    7
                db 0FEh
                db 0E9h
                db 0C4h
                db    0
                db 0E8h
                db  3Bh ; ;
                db 0FEh
                db 0E9h
                db 0BEh
                db    0
                db 0E8h
                db  9Bh
                db 0FEh
                db 0E9h
                db 0B8h
                db    0
jpt_1CF40       dw offset loc_1CF46     ; DATA XREF: sub_1CF2C+14↑r
                dw offset loc_1CF4C     ; jump table for switch statement
                dw offset byte_1D0C2+7Ah
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF52
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF58
                dw offset loc_1CF5E
                dw offset byte_1D0C2+7Ah
                dw offset byte_1D0C2+7Ah
                dw offset byte_1D0C2+7Ah
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF64
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF6A
                dw offset loc_1CF70
                dw offset loc_1CF76
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF7C
                dw offset loc_1CF82
                dw offset loc_1CF88
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CF8E
                dw offset loc_1CF94
                dw offset loc_1CF9A
                dw offset loc_1CFA0
                dw offset loc_1CFA6
                dw offset byte_1D0C2+7Ah
                dw offset byte_1D0C2+7Ah
                dw offset loc_1CFAC
                dw offset loc_1CFB2
byte_1D0C2      db 0B8h, 0CFh, 3Ch, 0D1h, 0BEh, 0CFh, 0C4h, 0CFh, 3Ch
                                        ; CODE XREF: seg002:04DD↑J
                db 0D1h, 3Ch, 0D1h, 0CAh, 0CFh, 0D0h, 0CFh, 0D6h, 0CFh
                db 0DCh, 0CFh, 0E2h, 0CFh, 0E8h, 0CFh, 0EEh, 0CFh, 0F4h
                db 0CFh, 3Ch, 0D1h, 0FAh, 0CFh, 3Ch, 0D1h, 0, 0D0h, 3Ch
                db 0D1h, 3Ch, 0D1h, 3Ch, 0D1h, 6, 0D0h, 3Ch, 0D1h, 0Ch
                db 0D0h, 3Ch, 0D1h, 12h, 0D0h, 3Ch, 0D1h, 18h, 0D0h, 1Eh
                db 0D0h, 24h, 0D0h, 3Ch, 0D1h, 3Ch, 0D1h, 2Ah, 0D0h, 3Ch
                db 0D1h, 3Ch, 0D1h, 30h, 0D0h, 3Ch, 0D1h, 3Ch, 0D1h, 3Ch
                db 0D1h, 3Ch, 0D1h, 36h, 0D0h, 3Ch, 0D0h, 42h, 0D0h, 48h
                db 0D0h, 4Eh, 0D0h, 3Ch, 0D1h, 3Ch, 0D1h, 3Ch, 0D1h, 3Ch
                db 0D1h, 54h, 0D0h, 3Ch, 0D1h, 5Ah, 0D0h, 60h, 0D0h, 66h
                db 0D0h, 3Ch, 0D1h, 6Ch, 0D0h, 3Ch, 0D1h, 72h, 0D0h, 3Ch
                db 0D1h, 78h, 0D0h, 7Eh, 0D0h, 5Dh, 0C3h, 2Bh, 0C0h, 50h
                db 0E8h, 46h, 9Fh, 83h, 0C4h, 2, 0B8h, 10h, 0, 50h, 0B8h
                db 0Fh, 0, 50h, 0E8h, 0DCh, 9Dh, 83h, 0C4h, 4, 0B8h, 0CAh
                db 31h, 50h, 0E8h
; ---------------------------------------------------------------------------

loc_1D15A:                              ; CODE XREF: seg002:0825↑J
                neg     byte ptr [di-3B7Dh]
                add     bh, [bx+si+32h]
                push    ax
                call    thk_res_4EFE
                add     sp, 2
                mov     byte_1DC78, 1
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1D170:                              ; CODE XREF: sub_1C46A+23↑p
                                        ; sub_1C524+1D↑p ...
                sub     ax, ax
                push    ax
                call    thk_2COMBAT_8D7A
                add     sp, 2
                mov     ax, 10h
                push    ax
                mov     ax, 0Eh
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 31D3h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 9
                push    ax
                call    thk_res_57E0
                add     sp, 2
                mov     ax, 32h ; '2'
                push    ax
                call    thk_res_4EFE
                add     sp, 2
                retn
sub_1CF2C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D1A6       proc near               ; CODE XREF: ovl_2CAST2:CC6A↑p

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                mov     byte_1DC78, 0
                cmp     word_1DC76, 1
                jnz     short loc_1D1CE
                call    sub_1D23A
                or      ax, ax
                jz      short loc_1D1C6
                mov     [bp+var_2], 31h ; '1'
                jmp     short loc_1D20B
; ---------------------------------------------------------------------------

loc_1D1C6:                              ; CODE XREF: sub_1D1A6+17↑j
                mov     [bp+var_2], 1Bh
                jmp     short loc_1D20B
; ---------------------------------------------------------------------------
                align 2

loc_1D1CE:                              ; CODE XREF: sub_1D1A6+10↑j
                mov     bx, word ptr aL1ReturnToCast ; "L1'Return' to cast"
                mov     al, byte ptr word_1DC76
                add     al, 30h ; '0'
                mov     [bx+0Bh], al
                mov     ax, 0Fh
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
                mov     [bp+var_2], ax

loc_1D20B:                              ; CODE XREF: sub_1D1A6+1E↑j
                                        ; sub_1D1A6+25↑j
                call    thk_res_35A8
                cmp     [bp+var_2], 1Bh
                jz      short loc_1D21D
                sub     [bp+var_2], 31h ; '1'
                mov     byte_1DC78, 1

loc_1D21D:                              ; CODE XREF: sub_1D1A6+6C↑j
                cmp     byte_1DC78, 0
                jz      short loc_1D233
                test    byte_23218, 2
                jz      short loc_1D233
                mov     [bp+var_2], 1Bh
                call    loc_1D170

loc_1D233:                              ; CODE XREF: sub_1D1A6+7C↑j
                                        ; sub_1D1A6+83↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1D1A6       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D23A       proc near               ; CODE XREF: sub_1C412↑p
                                        ; sub_1C56C↑p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 2
                cmp     byte_2419E, 0
                jz      short loc_1D24E
                mov     [bp+var_2], 1
                jmp     short loc_1D28E
; ---------------------------------------------------------------------------

loc_1D24E:                              ; CODE XREF: sub_1D23A+B↑j
                mov     byte_1DC78, 0
                mov     ax, 0Fh
                push    ax
                mov     ax, 16h
                push    ax
                call    thk_res_1676
                add     sp, 4
                mov     ax, 31E4h
                push    ax
                call    thk_res_1726
                add     sp, 2
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_res_3268
                add     sp, 4
                sub     ah, ah
                mov     [bp+var_2], ax
                cmp     ax, 0Dh
                jnz     short loc_1D284
                mov     al, 1
                jmp     short loc_1D286
; ---------------------------------------------------------------------------

loc_1D284:                              ; CODE XREF: sub_1D23A+44↑j
                sub     al, al

loc_1D286:                              ; CODE XREF: sub_1D23A+48↑j
                mov     byte_1DC78, al
                sub     ah, ah
                mov     [bp+var_2], ax

loc_1D28E:                              ; CODE XREF: sub_1D23A+12↑j
                call    thk_res_35A8
                cmp     [bp+var_2], 0
                jz      short loc_1D2A6
                test    byte_23218, 2
                jz      short loc_1D2A6
                mov     [bp+var_2], 0
                call    loc_1D170

loc_1D2A6:                              ; CODE XREF: sub_1D23A+5B↑j
                                        ; sub_1D23A+62↑j
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1D23A       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D2AE       proc near               ; CODE XREF: sub_1C1EA+6↑p
                                        ; sub_1C22C+7↑p ...

var_6           = byte ptr -6
var_4           = byte ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    si
                mov     byte_1DC78, 0
                cmp     byte_1DD58, 1
                jnz     short loc_1D2D8
                call    sub_1D23A
                or      ax, ax
                jz      short loc_1D2D0
                mov     [bp+var_6], 41h ; 'A'
                jmp     short loc_1D34B
; ---------------------------------------------------------------------------
                align 4

loc_1D2D0:                              ; CODE XREF: sub_1D2AE+18↑j
                mov     [bp+var_6], 1Bh
                jmp     short loc_1D34B
; ---------------------------------------------------------------------------
                align 4

loc_1D2D8:                              ; CODE XREF: sub_1D2AE+11↑j
                mov     al, byte_1DD58
                mov     [bp+var_4], al
                cmp     al, 0Ah
                jbe     short loc_1D2E6
                mov     [bp+var_4], 0Ah

loc_1D2E6:                              ; CODE XREF: sub_1D2AE+32↑j
                add     [bp+var_4], 40h ; '@'
                mov     bx, word ptr unk_20A56
                mov     al, [bp+var_4]
                mov     [bx+0Ch], al
                mov     ax, 10h
                push    ax
                mov     ax, 17h
                push    ax
                call    thk_res_1676
                add     sp, 4
                push    word ptr unk_20A56
                call    thk_res_1726
                add     sp, 2

loc_1D30C:                              ; CODE XREF: sub_1D2AE+98↓j
                call    thk_res_56C6
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_6], al
                cmp     al, 1Bh
                jnz     short loc_1D322
                mov     al, 1
                jmp     short loc_1D324
; ---------------------------------------------------------------------------
                align 2

loc_1D322:                              ; CODE XREF: sub_1D2AE+6D↑j
                sub     al, al

loc_1D324:                              ; CODE XREF: sub_1D2AE+71↑j
                sub     ah, ah
                mov     si, ax
                or      si, si
                jnz     short loc_1D344
                cmp     [bp+var_6], 41h ; 'A'
                jb      short loc_1D33E
                mov     al, [bp+var_4]
                cmp     [bp+var_6], al
                ja      short loc_1D33E
                mov     al, 1
                jmp     short loc_1D340
; ---------------------------------------------------------------------------

loc_1D33E:                              ; CODE XREF: sub_1D2AE+82↑j
                                        ; sub_1D2AE+8A↑j
                sub     al, al

loc_1D340:                              ; CODE XREF: sub_1D2AE+8E↑j
                sub     ah, ah
                mov     si, ax

loc_1D344:                              ; CODE XREF: sub_1D2AE+7C↑j
                or      si, si
                jz      short loc_1D30C
                mov     [bp+var_2], si

loc_1D34B:                              ; CODE XREF: sub_1D2AE+1E↑j
                                        ; sub_1D2AE+26↑j
                call    thk_res_35A8
                cmp     [bp+var_6], 1Bh
                jz      short loc_1D35D
                sub     [bp+var_6], 41h ; 'A'
                mov     byte_1DC78, 1

loc_1D35D:                              ; CODE XREF: sub_1D2AE+A4↑j
                cmp     byte_1DC78, 0
                jz      short loc_1D372
                test    byte_23218, 2
                jz      short loc_1D372
                mov     [bp+var_6], 1Bh
                call    loc_1D170

loc_1D372:                              ; CODE XREF: sub_1D2AE+B4↑j
                                        ; sub_1D2AE+BB↑j
                mov     al, [bp+var_6]
                sub     ah, ah
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1D2AE       endp

; ---------------------------------------------------------------------------
                align 8
ovl_2CAST2      ends

