; ---------------------------------------------------------------------------

_stub_descr     struc ; (sizeof=0x20, mappedto_1)
                                        ; XREF: stub079:stru_3F520/r
                                        ; stub080:stru_3F550/r ...
int_code        db 2 dup(?)             ; Overlay manager interrupt
memswap         dw ?                    ; Runtime memory swap address
fileoff         dd ?                    ; XREF: seg061:032E/o ; Offset in the file to the code
codesize        dw ?                    ; Code size
relsize         dw ?                    ; Relocation area size
nentries        dw ?                    ; Number of overlay entries
prevstub        dw ?                    ; Previous stub
workarea        db 16 dup(?)            ; base 16
_stub_descr     ends

; ---------------------------------------------------------------------------

FIGHTING_ENTRY  struc ; (sizeof=0x3, mappedto_2) ; XREF: sub_1E03C+3A/r
                                        ; sub_2B00E+4F/r ...
data            db 3 dup(?)             ; XREF: sub_1B936+3D/r
                                        ; sub_1B936+51/r ...
FIGHTING_ENTRY  ends


word_2A399      dw 4, 6, 7, 8, 9, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh, 10h, 11h
                                        ; DATA XREF: sub_29CE2+90↑o
                dw 12h, 13h, 14h, 15h, 16h, 17h, 18h, 19h, 1Ah, 1Bh, 82h
                dw 83h, 84h, 85h, 8Bh, 8Ch, 8Dh, 8Eh, 8Fh, 90h, 94h, 95h
                dw 96h, 98h, 99h, 9Ah, 9Bh, 9Ch, 9Dh, 9Eh, 9Fh, 0A0h
                dw offset loc_29D88, offset loc_29D90, offset loc_29DA2
                dw offset loc_29D92, offset loc_29DA2, offset loc_29DAA
                dw offset loc_29DB2, offset loc_29EF8, offset loc_2A336
                dw offset loc_29D9A, offset loc_2A37E, offset loc_2A37E
                dw offset loc_2A33F, offset loc_2A324, offset loc_2A312
                dw offset loc_2A32D, offset loc_2A348, offset loc_2A31B
                dw offset loc_2A351, offset loc_2A35A, offset loc_2A363
                dw offset loc_2A36C, offset loc_29DA2, offset loc_29EC8
                dw offset loc_29EBB, offset loc_29EBB, offset loc_29EBB
                dw offset loc_2A37E, offset loc_2A37E, offset loc_2A37E
                dw offset loc_2A37E, offset loc_29EF8, offset loc_29F00
                dw offset loc_29F00, offset loc_29E63, offset loc_29E6C
                dw offset loc_29DBA, offset loc_29F0E, offset loc_29F7B
                dw offset loc_2A053, offset loc_2A0D6, offset loc_2A154
                dw offset loc_2A1DA, offset loc_29DA2, offset loc_29D92
