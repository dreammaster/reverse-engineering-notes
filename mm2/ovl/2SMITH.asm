; ===========================================================================

; Segment type: Pure code
ovl_2SMITH      segment byte public 'CODE' use16
                assume cs:ovl_2SMITH
                ;org 0C130h
                assume es:nothing, ss:nothing, ds:DGROUP, fs:nothing, gs:nothing

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

smith_common_helper proc near           ; CODE XREF: seg002:0615↑J
                                        ; seg002:0645↑J ...
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 8Bh, 1Eh, 3Eh, 58h, 8Bh, 46h, 4, 8Bh, 56h, 6
                                        ; DATA XREF: seg002:0038↑o
                db 1, 47h, 66h, 11h, 57h, 68h, 2Bh, 0C0h, 2 dup(50h), 0E8h
                db 0B2h, 5, 83h, 0C4h, 4, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh
                db 83h, 0ECh, 4, 57h, 56h, 0C7h, 46h, 0FEh, 2 dup(0), 0BEh
                db 2, 58h, 8Bh, 1Eh, 3Eh, 58h, 0B9h, 3, 0, 56h, 0BFh, 0BAh
                db 57h, 8Dh, 77h, 3Ah, 1Eh, 7, 0F2h, 0A5h, 5Eh, 0B9h, 3
                db 0, 56h, 0BFh, 40h, 58h, 8Dh, 77h, 40h, 0F2h, 0A5h, 5Eh
                db 0B9h, 3, 0, 56h, 0BFh, 0Eh, 58h, 8Dh, 77h, 46h, 0F2h
                db 0A5h, 5Eh, 8Bh, 0FBh, 8Bh, 4Eh, 0FEh, 8Bh, 0D9h, 0B0h
                db 14h, 0F6h, 61h, 3Ah, 5, 60h, 69h, 89h, 4, 83h, 0C6h
                db 2, 41h, 83h, 0F9h, 6, 7Ch, 0EBh, 89h, 4Eh, 0FEh, 5Eh
                db 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 0Ah, 57h, 56h, 0C7h, 46h, 0F8h, 2 dup(0), 8Bh, 1Eh
                db 3Eh, 58h, 80h, 7Fh, 26h, 0, 74h, 0Dh, 0B8h, 8, 0, 50h
                db 0E8h, 1Ah, 0Ah, 83h, 0C4h, 2, 0E9h, 0C8h, 3, 8Bh, 5Eh
                db 4, 80h, 0BFh, 0BAh
smith_common_helper endp ; sp-analysis failed

byte_1C1DA      db 57h, 0, 75h, 6, 0B8h, 6, 0, 0EBh, 0E7h, 90h, 0D1h, 0E3h
                                        ; CODE XREF: seg002:08CD↑J
                db 0D1h, 0E3h, 0FFh, 0B7h
byte_1C1EA      db 0C2h, 57h, 0FFh, 0B7h, 0C0h, 57h, 0E8h, 9, 5, 83h, 0C4h
                                        ; CODE XREF: seg002:07F5↑J
                db 4, 0Bh, 0C0h, 75h, 6, 0B8h, 4, 0, 0EBh, 0CBh, 90h, 0B8h
                db 2, 0, 50h, 0E8h, 0EFh, 0ABh, 83h, 0C4h, 2, 2Bh, 0F6h
                db 0BFh, 0C6h, 44h, 8Dh, 44h, 11h, 50h, 0B8h, 2, 0, 50h
                db 0E8h, 14h, 0ADh, 83h, 0C4h, 4, 0FFh, 35h, 0E8h, 30h
                db 0ADh, 83h, 0C4h, 2, 83h, 0C7h, 2, 46h, 83h, 0FEh, 5
                db 7Ch, 0E1h, 89h, 76h, 0FAh, 0B8h, 11h, 0, 50h, 0B8h
                db 0Dh, 0, 50h, 0E8h, 0F2h, 0ACh
byte_1C23C      db 83h, 0C4h, 4, 8Bh, 5Eh, 4 ; CODE XREF: seg002:08E5↑J
byte_1C242      db 0D1h, 0E3h, 0FFh, 0B7h, 2, 58h, 0E8h, 7, 0ADh, 83h
                                        ; CODE XREF: seg002:0639↑J
                db 0C4h, 2, 8Bh, 5Eh, 4, 8Ah, 87h, 0Eh, 58h, 24h, 3Fh
                db 88h, 46h, 0FEh, 0Ah, 0C0h, 74h, 1Eh, 0B8h, 2Bh, 0, 50h
                db 0E8h, 0D5h, 0ACh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h
                db 1, 0, 50h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 50h, 0E8h, 9
                db 0ADh, 83h, 0C4h, 6, 8Bh, 5Eh, 4, 8Ah, 87h, 0Eh, 58h
                db 2Ah, 0E4h, 0B1h, 6, 0D3h, 0E8h, 88h, 46h, 0FEh, 0Ah
                db 0C0h, 74h, 1Ch, 0B8h, 14h, 0, 2 dup(50h), 0E8h, 96h
                db 0ACh, 83h, 0C4h, 4, 8Ah, 5Eh, 0FEh, 2Ah, 0FFh, 0D1h
                db 0E3h, 0FFh, 0B7h, 0CEh, 44h, 0E8h, 0A9h, 0ACh, 83h
                db 0C4h, 2, 8Bh, 5Eh, 4, 8Ah, 87h, 38h, 58h, 88h, 46h
                db 0FEh, 0B8h, 12h, 0, 50h, 0B8h, 14h, 0, 50h, 0E8h, 6Dh
byte_1C2C0      db 0ACh, 83h, 0C4h, 4, 80h, 7Eh, 0FEh, 0, 75h, 0Ch, 0B8h
                                        ; CODE XREF: seg002:026D↑J
                db 12h, 45h, 50h, 0E8h, 81h, 0ACh, 83h, 0C4h, 2, 0EBh
                db 42h, 2Bh, 0F6h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 89h, 46h
                db 0F6h, 8Bh, 7Eh, 0F8h, 8Ah, 84h, 0BEh, 44h, 2Ah, 0E4h
                db 85h, 46h, 0F6h, 75h, 1Eh, 0Bh, 0FFh, 74h, 0Ah, 0B8h
                db 2Ch, 0, 50h, 0E8h, 41h
byte_1C2F8      db 0ACh, 83h, 0C4h, 2, 8Ah, 84h, 0D6h, 44h, 2Ah, 0E4h
                                        ; CODE XREF: seg002:0651↑J
                db 50h, 0E8h, 34h, 0ACh, 83h, 0C4h
byte_1C308      db 2, 0BFh, 1, 0, 46h, 83h, 0FEh, 8, 7Ch, 0D1h, 89h, 7Eh
                                        ; CODE XREF: seg002:08F1↑J
                db 0F8h, 89h, 76h, 0FAh, 8Bh, 5Eh, 4, 0D1h, 0E3h, 8Bh
                db 9Fh, 2, 58h, 8Ah, 47h, 0Eh, 88h, 46h, 0FEh, 0B8h, 13h
                db 0, 50h, 0B8h, 0Eh, 0, 50h, 0E8h, 0FCh, 0ABh, 83h, 0C4h
                db 4, 80h, 7Eh, 0FEh, 0F0h, 75h, 0Dh, 0FFh, 36h, 0DEh
                db 44h, 0E8h, 10h, 0ACh, 83h, 0C4h, 2, 0EBh, 69h, 90h
                db 0FFh, 36h, 0E0h, 44h, 0E8h, 3, 0ACh, 83h, 0C4h, 2, 8Bh
                db 5Eh, 4, 8Ah, 87h, 0Eh, 58h, 24h, 3Fh, 88h, 46h, 0FCh
                db 0F6h, 46h, 0FEh, 0Fh, 74h, 4Ch, 0B8h, 13h, 0, 50h, 0B8h
                db 14h, 0, 50h, 0E8h, 0BFh, 0ABh, 83h
byte_1C370      db 0C4h, 4, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 8Bh, 0F0h, 8Bh
                                        ; CODE XREF: seg002:0471↑J
                db 0DEh, 0B1h, 4, 0D3h, 0EBh, 0D1h, 0E3h, 0FFh, 0B7h, 0E2h
                db 44h, 0E8h, 0CAh, 0ABh, 83h, 0C4h, 2, 0B8h, 2Bh, 0, 50h
                db 0E8h, 0A8h, 0ABh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h
                db 1, 0, 50h, 8Ah, 46h, 0FCh, 2Ah, 0E4h, 8Bh, 0CEh, 83h
                db 0E1h, 0Fh, 3, 0C1h, 50h, 0E8h, 0D5h, 0ABh, 83h, 0C4h
                db 6, 8Bh, 5Eh, 4, 0D1h, 0E3h, 8Bh, 9Fh, 2, 58h, 8Ah, 47h
                db 0Fh, 88h, 46h, 0FEh, 0B8h, 14h, 0, 50h, 0B8h, 0Dh, 0
                db 50h, 0E8h, 64h, 0ABh, 83h, 0C4h, 4, 80h, 7Eh, 0FEh
                db 0, 75h, 0Dh, 0FFh, 36h, 0DEh, 44h, 0E8h, 78h, 0ABh
                db 83h, 0C4h, 2, 0E9h, 0F0h, 0, 0FFh, 36h, 0E0h, 44h, 0E8h
                db 6Bh, 0ABh, 83h, 0C4h, 2, 80h, 7Eh, 0FEh, 7Fh, 76h, 7Eh
                db 0B8h, 15h, 0, 50h, 0B8h, 14h
byte_1C3F6      db 0, 50h, 0E8h, 33h, 0ABh, 83h, 0C4h, 4, 0B8h, 20h, 45h
                                        ; CODE XREF: seg002:047D↑J
                db 50h, 0E8h, 4Dh, 0ABh, 83h, 0C4h, 2, 80h, 66h, 0FEh
                db 7Fh, 0C6h, 46h, 0FCh, 53h, 80h, 7Eh, 0FEh, 31h, 72h
                db 8, 0C6h, 46h, 0FCh, 43h, 80h, 6Eh, 0FEh, 30h, 8Ah, 46h
                db 0FCh, 2Ah, 0E4h, 50h, 0E8h, 13h, 0ABh, 83h, 0C4h, 2
                db 0B8h, 20h, 0, 50h, 0E8h, 9, 0ABh, 83h, 0C4h, 2, 0FEh
                db 4Eh, 0FEh, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 8Bh, 0F0h, 56h
                db 0E8h, 14h, 0B3h, 83h, 0C4h, 2, 2Ah, 0E4h, 5, 30h, 0
                db 50h, 0E8h, 0ECh, 0AAh, 83h, 0C4h, 2, 0B8h, 2Dh, 0, 50h
                db 0E8h, 0E2h, 0AAh, 83h, 0C4h, 2, 56h, 0E8h, 3, 0B3h
                db 83h, 0C4h, 2
byte_1C462      db 2Ah, 0E4h, 5, 30h, 0, 50h, 0E8h, 0CFh, 0AAh, 0E9h, 6Ch
                                        ; CODE XREF: seg002:086D↑J
                db 0FFh, 8Bh, 5Eh, 4, 8Ah, 87h, 0Eh, 58h, 24h, 3Fh, 88h
                db 46h, 0FCh, 0Ah, 0C0h, 75h, 6, 0F6h, 46h, 0FEh, 0Fh
                db 74h, 4Ch, 0B8h, 15h, 0, 50h, 0B8h, 14h, 0, 50h, 0E8h
                db 9Fh
byte_1C48E      db 0AAh, 83h, 0C4h, 4, 8Ah, 5Eh, 0FEh, 83h, 0E3h, 70h
                                        ; CODE XREF: seg002:0B31↑J
                db 0B1h, 4, 0D3h, 0EBh, 0D1h, 0E3h, 0FFh, 0B7h, 2, 45h
                db 0E8h, 0ADh, 0AAh, 83h, 0C4h, 2, 0B8h, 2Bh, 0, 50h, 0E8h
                db 8Bh, 0AAh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1
                db 0, 50h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 25h, 0Fh, 0, 8Ah
                db 4Eh, 0FCh, 2Ah, 0EDh, 3, 0C1h, 50h, 0E8h, 0B5h, 0AAh
                db 83h, 0C4h, 6, 0B8h, 15h, 0, 50h, 0B8h, 0Ch, 0, 50h
                db 0E8h, 53h, 0AAh, 83h, 0C4h, 4, 0B8h, 20h, 0, 50h, 0B8h
                db 1, 0, 50h, 8Bh, 5Eh, 4, 8Ah, 87h, 40h, 58h, 2Ah, 0E4h
                db 50h, 0E8h, 8Fh, 0AAh, 83h, 0C4h, 6, 0B8h, 16h, 0, 50h
                db 0B8h, 2, 0, 50h, 0E8h, 2Dh, 0AAh, 83h, 0C4h, 4, 8Bh
                db 5Eh, 4, 8Ah, 87h, 0Eh, 58h, 24h, 3Fh, 88h, 46h, 0FEh
                db 80h, 0BFh, 0BAh, 57h, 6Fh, 73h, 39h, 0B8h, 2Eh, 45h
                db 50h, 0E8h, 34h, 0AAh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
                db 0B8h, 1, 0, 50h, 8Bh, 5Eh, 4
byte_1C52C      db 0D1h, 0E3h, 8Bh, 9Fh, 2, 58h, 8Ah, 47h, 10h, 2Ah, 0E4h
                                        ; CODE XREF: seg002:0879↑J
                db 50h, 0E8h, 47h, 0AAh, 83h, 0C4h, 6, 80h, 7Eh, 0FEh
                db 0, 74h, 4Ah, 0B8h, 2Bh, 0, 50h, 0E8h, 0EFh, 0A9h, 83h
                db 0C4h, 2, 0EBh, 2Ah, 8Bh, 5Eh, 4, 80h, 0BFh, 0BAh, 57h
                db 0A0h, 73h, 34h, 80h, 0BFh, 0BAh, 57h, 72h, 76h, 2Dh
                db 0B8h, 3Ah, 45h, 50h, 0E8h, 0EAh, 0A9h, 83h, 0C4h, 2
                db 8Bh, 5Eh, 4, 0D1h, 0E3h, 8Bh, 9Fh, 2, 58h, 8Ah, 47h
                db 10h, 0, 46h, 0FEh, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h
                db 8Ah, 46h, 0FEh, 2Ah, 0E4h, 50h, 0E8h, 0F7h, 0A9h, 83h
                db 0C4h, 6, 0E8h, 0B1h, 0AAh, 3Dh, 1Bh, 0, 75h, 0F8h, 0E8h
                db 9Fh, 6, 0E8h, 0FEh, 5, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h
                db 55h, 8Bh, 0ECh, 8Bh, 1Eh, 3Eh, 58h, 80h, 7Fh, 26h
byte_1C5AC      db 0, 74h, 0Dh, 0B8h, 8, 0, 50h, 0E8h, 32h, 6, 83h, 0C4h
                                        ; CODE XREF: seg002:0885↑J
                db 2, 0EBh, 39h, 90h, 8Bh, 5Eh, 4, 80h
byte_1C5C0      db 0BFh, 0BAh, 57h, 0, 75h, 6, 0B8h, 6, 0, 0EBh, 0E7h
                                        ; CODE XREF: seg002:029D↑J
                db 90h, 0D1h, 0E3h, 0D1h, 0E3h, 0FFh, 0B7h, 0C2h, 57h
                db 0FFh, 0B7h, 0C0h, 57h
byte_1C5D8      db 0E8h, 55h, 0FBh, 83h, 0C4h, 4, 2Bh, 0C0h, 50h, 0E8h
                                        ; CODE XREF: seg002:01A1↑J
                db 4, 6, 83h, 0C4h, 2, 0FFh, 76h, 4, 0FFh, 36h, 3Eh, 58h
                db 0E8h, 0C5h, 0ABh, 83h, 0C4h, 4, 5Dh, 0C3h, 55h, 8Bh
                db 0ECh, 83h, 0ECh, 8, 57h, 56h, 0B8h, 7, 0, 50h, 0E8h
                db 0F1h, 0A7h, 83h, 0C4h, 2, 2Bh, 0F6h, 0BFh, 2, 58h, 0C7h
                db 46h, 0F8h, 0C0h, 57h, 0C6h, 46h, 0FEh, 20h, 8Dh, 44h
                db 11h, 50h, 0B8h, 0Fh, 0, 50h, 0E8h, 0Dh, 0A9h, 83h, 0C4h
                db 4, 80h, 0BCh, 38h, 58h, 0, 74h, 1Dh, 8Bh, 1Eh, 3Eh
                db 58h, 8Ah, 5Fh, 0Fh, 2Ah, 0FFh, 8Ah, 87h, 0BEh, 44h
                db 2Ah, 0E4h, 8Ah, 8Ch, 38h, 58h, 2Ah, 0EDh, 85h, 0C1h
                db 74h, 4, 0C6h, 46h, 0FEh, 2Dh, 8Ah, 46h, 0FEh, 2Ah, 0E4h
                db 50h, 0E8h, 0E9h, 0A8h, 83h, 0C4h, 2, 8Bh, 0C6h, 5, 41h
                db 0, 50h, 0E8h, 0DDh, 0A8h, 83h, 0C4h, 2, 0B8h, 29h, 0
                db 50h, 0E8h, 0D3h, 0A8h, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
byte_1C66E      db 0E8h, 0C9h, 0A8h, 83h, 0C4h, 2, 80h, 0BCh, 0BAh, 57h
                                        ; CODE XREF: seg002:0891↑J
                db 0, 74h, 67h, 0FFh, 35h, 0E8h, 0D2h, 0A8h, 83h, 0C4h
                db 2, 8Ah, 84h, 0Eh, 58h, 24h, 3Fh, 88h, 46h, 0FEh, 0Ah
                db 0C0h, 74h, 2Ch, 8Dh, 44h, 11h, 50h, 0B8h, 1Fh, 0, 50h
                db 0E8h, 93h, 0A8h, 83h, 0C4h, 4, 0B8h, 2Bh, 0, 50h, 0E8h
                db 95h, 0A8h, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1
                db 0, 50h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 50h, 0E8h, 0C9h
                db 0A8h, 83h, 0C4h, 6, 8Dh, 44h, 11h, 50h, 0B8h, 22h, 0
                db 50h, 0E8h, 67h, 0A8h, 83h, 0C4h, 4, 0B8h, 2Dh, 0, 50h
                db 0E8h, 69h, 0A8h, 83h, 0C4h, 2, 8Bh, 5Eh, 0F8h, 0FFh
                db 77h, 2, 0FFh, 37h, 0E8h, 8Fh, 0B0h, 83h, 0C4h, 4, 83h
                db 0C7h, 2, 83h, 46h, 0F8h, 4, 46h, 83h, 0FEh, 6, 7Dh
                db 3, 0E9h, 20h, 0FFh
byte_1C6F2      db 89h, 76h, 0FCh, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h, 90h
                                        ; CODE XREF: seg002:0B19↑J
                                        ; smith_action_prompt+A9↓p ...
                db 55h, 8Bh, 0ECh, 83h, 0ECh, 2, 8Bh, 1Eh, 3Eh, 58h, 8Bh
                db 46h, 4, 8Bh, 56h, 6, 39h, 57h, 68h, 72h, 0Dh, 77h, 5
                db 39h, 47h, 66h, 72h, 6, 0B8h, 1, 0, 0EBh, 3, 90h, 2Bh
                db 0C0h, 89h, 46h, 0FEh, 0Bh, 0C0h, 74h, 48h, 0B8h, 13h
                db 0, 50h, 0B8h, 0Eh, 0, 50h, 0B8h, 13h, 0, 50h, 0B8h
                db 7, 0, 50h, 0E8h, 0C4h, 0A7h
byte_1C73A      db 83h, 0C4h, 8, 0B8h, 13h, 0, 50h, 0B8h, 7, 0, 50h, 0E8h
                                        ; CODE XREF: seg002:089D↑J
                db 0E6h, 0A7h, 83h, 0C4h, 4, 8Bh, 1Eh, 3Eh, 58h, 8Bh, 46h
                db 4, 8Bh, 56h, 6, 29h, 47h, 66h, 19h, 57h, 68h, 0B8h
                db 20h, 0, 50h, 0B8h, 1, 0, 50h, 0FFh, 77h, 68h, 0FFh
                db 77h, 66h, 0E8h, 0EAh, 0A9h, 83h, 0C4h, 8, 8Bh, 46h
                db 0FEh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 2, 56h, 8Bh, 1Eh, 3Eh, 58h, 80h, 7Fh, 26h, 0, 74h, 5
                db 0B8h, 8, 0, 0EBh, 63h, 2Bh, 0C9h, 8Bh, 0D3h, 8Bh, 0F1h
                db 8Bh, 0DAh, 80h, 78h, 3Ah, 0, 75h, 0Eh, 89h, 4Eh, 0FEh
                db 83h, 0F9h, 6, 75h, 0Eh, 0B8h, 2, 0, 0EBh, 48h, 90h
                db 41h, 83h, 0F9h, 6, 7Dh, 0ECh, 0EBh, 0E0h, 8Bh, 5Eh
                db 4, 0D1h, 0E3h, 0D1h, 0E3h, 0FFh, 0B7h, 0C2h, 57h, 0FFh
                db 0B7h, 0C0h, 57h, 0E8h, 3Ah, 0FFh, 83h, 0C4h, 4, 0Bh
                db 0C0h, 75h, 5, 0B8h, 4, 0, 0EBh, 21h, 8Bh, 76h, 0FEh
                db 3, 36h, 3Eh, 58h, 8Bh, 5Eh, 4, 8Ah, 87h, 0BAh, 57h
                db 88h, 44h, 3Ah, 8Ah, 87h, 40h
byte_1C7E2      db 58h, 88h, 44h, 40h, 8Ah, 87h
                                        ; CODE XREF: seg002:0849↑J
byte_1C7E8      db 0Eh, 58h, 88h, 44h, 46h, 2Bh, 0C0h, 50h, 0E8h, 0F5h
                                        ; CODE XREF: seg002:0B0D↑J
                db 3, 83h, 0C4h, 2, 5Eh, 8Bh, 0E5h, 5Dh, 0C3h, 90h, 55h
                db 8Bh, 0ECh, 83h, 0ECh, 0Ah, 2Bh, 0C0h, 89h, 46h, 0FAh
                db 89h, 46h, 0F8h, 8Bh, 5Eh, 4, 80h, 0BFh, 0BAh, 57h, 0
                db 75h, 3, 0E9h, 0BFh, 0, 8Ah, 87h, 0Eh, 58h, 24h, 3Fh
                db 88h, 46h, 0FEh, 83h, 3Eh, 2Ah, 58h, 6, 75h, 2Dh, 0Ah
                db 0C0h, 75h, 0Dh, 0C7h, 46h, 0F8h, 0Ah, 0, 0C7h, 46h
                db 0FAh, 2 dup(0), 0E9h, 9Eh, 0, 0B8h, 64h, 0, 99h, 52h
                db 50h, 8Ah, 46h, 0FEh, 2Ah, 0E4h, 2Bh, 0C9h, 51h, 50h
                db 0E8h, 64h, 0AEh, 89h, 46h, 0F8h, 89h, 56h, 0FAh, 0E9h
                db 83h, 0, 90h, 8Bh, 5Eh, 4, 0D1h, 0E3h, 8Bh, 9Fh, 2, 58h
                db 8Bh, 47h, 12h, 89h, 46h, 0F8h, 0C7h, 46h, 0FAh, 2 dup(0)
                db 80h, 7Eh, 0FEh, 0, 74h, 9, 0D1h, 66h, 0F8h, 0D1h, 56h
                db 0FAh, 0FEh, 4Eh, 0FEh, 80h, 7Eh, 0FEh, 0, 74h, 28h
                db 8Ah, 46h, 0FEh, 0F6h, 0D8h, 98h, 0F7h, 0D8h, 89h, 46h
                db 0F6h, 0B8h, 0E8h, 3, 99h, 52h, 50h, 8Bh, 46h, 0F6h
                db 99h, 52h, 50h, 0E8h, 17h, 0AEh, 1, 46h, 0F8h, 11h, 56h
                db 0FAh, 8Bh, 46h, 0F6h, 0F7h, 0D8h, 0, 46h, 0FEh, 0B8h
                db 0Ah, 0, 50h, 0FFh, 36h, 3Eh, 58h, 0E8h, 9Ah, 0AEh, 83h
                db 0C4h, 4, 89h, 46h, 0FCh, 83h, 3Eh, 2Ah, 58h, 5, 75h
                db 0Dh, 0D1h, 6Eh, 0FAh, 0D1h, 5Eh, 0F8h, 0Bh, 0C0h, 75h
                db 0Fh, 0EBh, 7, 90h, 83h, 7Eh, 0FCh, 0, 74h, 6, 0D1h
                db 6Eh, 0FAh, 0D1h, 5Eh, 0F8h, 8Bh, 46h, 0F8h, 8Bh, 56h
                db 0FAh, 8Bh, 0E5h, 5Dh, 0C3h, 55h, 8Bh, 0ECh, 83h, 0ECh
                db 16h, 57h, 56h, 2Bh, 0F6h, 2Ah, 0C0h, 88h, 84h, 0Eh
                db 58h, 88h, 84h, 40h, 58h, 46h, 83h, 0FEh, 6, 7Ch, 0F0h
                db 89h, 76h, 0F6h, 83h, 3Eh, 2Ah, 58h, 5, 74h, 7, 83h
                db 3Eh, 2Ah, 58h, 6, 75h, 7, 0E8h, 42h, 0F8h, 0E9h, 3Fh
                db 1, 90h, 0A1h, 2Ah, 58h, 3Dh, 1, 0, 74h, 1Ah, 3Dh, 2
                db 0, 75h, 3, 0E9h, 80h, 0, 3Dh, 3, 0, 75h, 3, 0E9h, 84h
                db 0, 3Dh, 4, 0, 75h, 3, 0E9h, 88h, 0, 0EBh, 0Ah, 0C7h
                db 46h, 0FCh, 0C8h, 43h, 0C7h, 46h, 0F8h, 0E6h, 43h, 0A0h
                db 92h, 3, 2Ah, 0E4h, 8Bh, 0C8h, 0D1h, 0E0h, 3, 0C1h, 0D1h
                db 0E0h, 89h, 46h, 0EEh, 1, 46h, 0FCh, 1, 46h, 0F8h, 0C7h
                db 46h, 0F6h, 2 dup(0), 0BEh, 2, 58h, 8Bh, 46h, 0FCh, 0B9h
                db 3, 0, 56h, 0BFh, 0BAh, 57h, 8Bh, 0F0h, 1Eh, 7, 0F2h
                db 0A5h, 5Eh, 8Bh, 46h, 0F8h, 0B9h, 3, 0, 56h, 0BFh, 0Eh
                db 58h, 8Bh, 0F0h, 0F2h, 0A5h, 5Eh, 0C7h, 46h, 0F0h, 6
                db 0, 83h, 46h, 0F6h, 6, 83h, 46h, 0F8h, 6, 8Bh, 7Eh, 0FCh
                db 8Bh, 4Eh, 0F0h, 0B0h, 14h, 0F6h, 25h, 5, 60h, 69h, 89h
                db 4, 47h, 83h
byte_1C99A      db 0C6h, 2, 49h, 74h, 29h, 0EBh, 0EEh, 90h, 0B8h, 7Ch
                                        ; CODE XREF: seg002:07DD↑J
                db 44h, 89h, 46h, 0F8h, 89h, 46h, 0FCh, 0EBh, 91h, 90h
                db 0C7h, 46h, 0FCh, 4, 44h, 0C7h, 46h, 0F8h, 22h, 44h
                db 0EBh, 84h, 0C7h, 46h, 0FCh, 40h, 44h, 0C7h, 46h, 0F8h
                db 5Eh, 44h, 0E9h, 77h, 0FFh, 90h, 89h, 7Eh, 0FCh, 83h
                db 3Eh, 2Ah, 58h, 2, 75h, 4Eh, 8Bh, 1Eh, 0CAh, 3, 0D1h
                db 0E3h, 8Bh, 87h, 0A2h, 3, 99h, 89h, 46h, 0EAh, 89h, 56h
                db 0ECh, 0B9h, 1Eh, 0
byte_1C9E6      db 0F7h, 0F9h, 89h, 56h, 0FEh, 8Bh, 46h, 0EAh, 8Bh, 56h
                                        ; CODE XREF: seg002:0B01↑J
                db 0ECh, 0F7h, 0F9h, 89h, 46h, 0F4h, 83h, 7Eh, 0FEh, 1Dh
                db 75h, 8, 8Bh, 0D8h, 8Ah, 87h, 9Ah, 44h, 0EBh, 7, 8Bh
                db 5Eh, 0FEh, 8Ah, 87h, 0A0h, 44h, 88h, 46h, 0FAh, 2Bh
                db 0F6h, 8Ah, 46h, 0FAh, 88h, 84h, 0Eh, 58h, 46h, 83h
                db 0FEh, 6, 7Ch, 0F3h, 89h, 76h, 0F6h, 83h, 3Eh, 2Ah, 58h
                db 4, 75h, 29h, 2Bh, 0F6h, 8Ah, 84h, 0Eh, 58h, 88h, 84h
                db 40h, 58h, 0C6h, 84h, 0Eh, 58h, 0, 46h, 83h, 0FEh, 6
                db 7Ch, 0EDh, 89h, 76h, 0F6h, 80h, 3Eh, 92h, 3, 1, 75h
                db 0Ah, 0C6h, 6, 10h, 58h, 5, 0C6h, 6, 12h, 58h, 2, 2Bh
                db 0F6h
byte_1CA52      db 0BFh, 0C0h, 57h, 0C7h, 46h, 0EEh, 2, 58h, 56h, 0E8h
                                        ; CODE XREF: seg002:0A89↑J
                db 9Eh, 0FDh, 83h, 0C4h, 2, 89h, 5, 89h, 55h, 2, 8Bh, 5Eh
                db 0EEh, 8Bh, 1Fh, 8Ah, 47h, 0Dh, 88h, 84h, 38h, 58h, 83h
                db 0C7h, 4, 83h, 46h, 0EEh, 2, 46h, 83h, 0FEh, 6, 7Ch
                db 0DBh, 89h, 76h, 0F6h, 5Eh, 5Fh, 8Bh, 0E5h, 5Dh, 0C3h

; =============== S U B R O U T I N E =======================================

; Identify / Sell / Buy (A-F)
; Attributes: bp-based frame

smith_action_prompt proc near           ; CODE XREF: seg002:0801↑J
                                        ; blacksmith_menu+18C↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     di, 1
                mov     ax, [bp+arg_0]
                mov     word_2307A, ax

loc_1CA99:                              ; CODE XREF: smith_action_prompt+D1↓j
                or      di, di
                jz      short loc_1CADC
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                cmp     [bp+arg_0], 6
                jnz     short loc_1CAB6
                mov     ax, offset aIdentifyAF ; "Identify(A-F)"
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------

loc_1CAB6:                              ; CODE XREF: smith_action_prompt+27↑j
                cmp     [bp+arg_0], 5
                jnz     short loc_1CAC2
                mov     ax, offset aSellAF ; "Sell (A-F)   "
                jmp     short loc_1CAC5
; ---------------------------------------------------------------------------
                align 2

loc_1CAC2:                              ; CODE XREF: smith_action_prompt+32↑j
                mov     ax, offset aBuyAF ; "Buy (A-F)    "

loc_1CAC5:                              ; CODE XREF: smith_action_prompt+2C↑j
                                        ; smith_action_prompt+37↑j
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                call    near ptr byte_1C7E8+0F8h
                call    near ptr byte_1C5D8+1Eh

loc_1CADC:                              ; CODE XREF: smith_action_prompt+13↑j
                mov     di, 1
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     si, ax
                cmp     ax, 41h ; 'A'
                jb      short loc_1CB1C
                cmp     ax, 46h ; 'F'
                ja      short loc_1CB1C
                sub     si, 41h ; 'A'
                cmp     word_2307A, 5
                jnz     short loc_1CB08
                push    si
                call    near ptr byte_1C52C+76h

loc_1CB03:                              ; CODE XREF: smith_action_prompt+8B↓j
                                        ; smith_action_prompt+92↓j
                add     sp, 2
                jmp     short loc_1CB54
; ---------------------------------------------------------------------------

loc_1CB08:                              ; CODE XREF: smith_action_prompt+75↑j
                cmp     word_2307A, 6
                jnz     short loc_1CB16
                push    si
                call    near ptr byte_1C132+7Eh
                jmp     short loc_1CB03
; ---------------------------------------------------------------------------
                align 2

loc_1CB16:                              ; CODE XREF: smith_action_prompt+85↑j
                push    si
                call    near ptr byte_1C73A+3Ch
                jmp     short loc_1CB03
; ---------------------------------------------------------------------------

loc_1CB1C:                              ; CODE XREF: smith_action_prompt+66↑j
                                        ; smith_action_prompt+6B↑j
                mov     ax, si
                cmp     ax, 47h ; 'G'
                jnz     short loc_1CB3A
                push    word_23028
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    near ptr byte_1C6F2+0Ah
                add     sp, 4
                jmp     short loc_1CB52
; ---------------------------------------------------------------------------
                align 2

loc_1CB3A:                              ; CODE XREF: smith_action_prompt+99↑j
                push    si
                call    smith_draw
                add     sp, 2           ; CODE XREF: seg002:0A65↑J
                or      ax, ax
                jz      short loc_1CB52
                mov     ax, si
                sub     ax, 31h ; '1'
                mov     word_23028, ax
                call    loc_1CB9A
                jmp     short loc_1CB54
; ---------------------------------------------------------------------------

loc_1CB52:                              ; CODE XREF: smith_action_prompt+AF↑j
                                        ; smith_action_prompt+BB↑j
                sub     di, di

loc_1CB54:                              ; CODE XREF: smith_action_prompt+7E↑j
                                        ; smith_action_prompt+C8↑j
                cmp     si, 1Bh
                jz      short loc_1CB5C
                jmp     loc_1CA99
; ---------------------------------------------------------------------------

loc_1CB5C:                              ; CODE XREF: smith_action_prompt+CF↑j
                mov     [bp+var_2], di
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
smith_action_prompt endp


; =============== S U B R O U T I N E =======================================

; " Blacksmith ", "Gold=", G-Gather Gold, #-Other Char
; Attributes: bp-based frame

smith_draw      proc near               ; CODE XREF: smith_action_prompt+B3↑p
                                        ; blacksmith_menu+1D1↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     [bp+arg_0], 31h ; '1'
                mov     ax, word_23028
                cmp     [bp+arg_0], ax
                jz      short loc_1CB96
                cmp     [bp+arg_0], 0
                jl      short loc_1CB96
                mov     ax, g_party_size
                cmp     [bp+arg_0], ax
                jge     short loc_1CB96
                mov     bx, [bp+arg_0]
                shl     bx, 1

loc_1CB8A:                              ; CODE XREF: seg002:0AF5↑J
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CB96
                mov     ax, 1
                jmp     short loc_1CB98
; ---------------------------------------------------------------------------

loc_1CB96:                              ; CODE XREF: smith_draw+D↑j
                                        ; smith_draw+13↑j ...
                sub     ax, ax

loc_1CB98:                              ; CODE XREF: smith_draw+2C↑j
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1CB9A:                              ; CODE XREF: smith_action_prompt+C5↑p
                                        ; blacksmith_menu+105↓p ...
                push    word_23028
                call    thk_char_ptr
                add     sp, 2
                mov     word_2308E, ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, word_23028
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    word_2308E
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    near ptr byte_1C6F2+0Ah
                                        ; CODE XREF: seg002:01AD↑J
                add     sp, 4
                mov     word_23078, 0
                retn
; ---------------------------------------------------------------------------
                align 2
                push    bp
                mov     bp, sp
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                inc     [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+5814h]
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 12h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     bx, [bp+arg_0]
                shl     bx, 1
                push    word ptr [bx+5814h]
                call    thk_text_puts
                add     sp, 2
                call    thk_monster_anim_step
                pop     bp
                retn
; ---------------------------------------------------------------------------
                align 2

loc_1CC38:                              ; CODE XREF: blacksmith_menu+EF↓p
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                call    thk_res_5440
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
                mov     ax, offset aBlacksmith ; " Blacksmith "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_set_flag_8
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGold_3 ; "Gold="
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aGGatherGold_1 ; "G-Gather Gold"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aOtherChar_1 ; "#-Other Char"
                push    ax
                call    thk_text_puts
                add     sp, 2
                retn
; ---------------------------------------------------------------------------
                align 2
smith_draw      endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

blacksmith_menu proc near               ; CODE XREF: seg002:08FD↑J

var_C           = word ptr -0Ch
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 0Eh
                push    di
                push    si
                mov     [bp+var_2], 1
                mov     ax, 2
                push    ax
                call    thk_res_670A
                add     sp, 2
                mov     [bp+var_6], 0Ah
                mov     si, 5814h
                mov     di, 0Ah

loc_1CCDC:                              ; CODE XREF: blacksmith_menu+2B↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CCDC
                mov     [bp+var_6], 5
                mov     [bp+var_8], 4
                mov     [bp+var_C], 0

loc_1CCF6:                              ; CODE XREF: blacksmith_menu+59↓j
                mov     si, [bp+var_C]
                add     si, 57DAh
                mov     di, 4

loc_1CD00:                              ; CODE XREF: blacksmith_menu+4F↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CD00
                add     [bp+var_C], 8
                cmp     [bp+var_C], 28h ; '('
                jl      short loc_1CCF6
                mov     [bp+var_6], 6
                mov     si, 582Ch
                mov     di, 6

loc_1CD20:                              ; CODE XREF: blacksmith_menu+6F↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1CD20
                mov     byte_2294F, 0FDh
                or      byte_1DC80, 6
                mov     word_23028, 0
                cmp     g_party_size, 0
                jle     short loc_1CD59
                mov     si, 416h
                mov     dx, g_party_size
                mov     cx, word_23028

loc_1CD4D:                              ; CODE XREF: blacksmith_menu:loc_1CE3F↓j
                cmp     word ptr [si], 18h
                jl      short loc_1CD55
                jmp     loc_1CE34
; ---------------------------------------------------------------------------

loc_1CD55:                              ; CODE XREF: blacksmith_menu+96↑j
                                        ; blacksmith_menu+182↓j
                mov     word_23028, cx

loc_1CD59:                              ; CODE XREF: blacksmith_menu+86↑j
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CD66:                              ; CODE XREF: blacksmith_menu+D5↓j
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
                push    word ptr [bx+di+57DAh]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1CD66
                mov     [bp+var_6], si
                call    thk_2PLAY_946E
                cmp     byte_1DC7F, 0
                jnz     short loc_1CDA1
                jmp     loc_1CEBE
; ---------------------------------------------------------------------------

loc_1CDA1:                              ; CODE XREF: blacksmith_menu+E2↑j
                or      byte_1DC80, 1
                call    thk_res_34BA
                call    loc_1CC38
                mov     word_23078, 1

loc_1CDB2:                              ; CODE XREF: blacksmith_menu+1FE↓j
                mov     word_2307A, 0
                cmp     word_23078, 0
                jz      short loc_1CDC2
                call    loc_1CB9A

loc_1CDC2:                              ; CODE XREF: blacksmith_menu+103↑j
                cmp     [bp+var_2], 0
                jz      short loc_1CE11
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aSelectAF ; "Select (A-F) "
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     si, si
                mov     di, 582Ch

loc_1CDEF:                              ; CODE XREF: blacksmith_menu+152↓j
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
                jl      short loc_1CDEF
                mov     [bp+var_6], si

loc_1CE11:                              ; CODE XREF: blacksmith_menu+10C↑j
                mov     [bp+var_2], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_4], ax
                sub     ax, 41h ; 'A'   ; switch 7 cases
                cmp     ax, 6
                ja      short def_1CE2E ; jumptable 0001CE2E default case
                add     ax, ax
                xchg    ax, bx
                jmp     cs:jpt_1CE2E[bx] ; CODE XREF: seg002:0831↑J
                                        ; switch jump
; ---------------------------------------------------------------------------
                align 2

loc_1CE34:                              ; CODE XREF: blacksmith_menu+98↑j
                add     si, 2
                inc     cx
                cmp     cx, dx
                jl      short loc_1CE3F
                jmp     loc_1CD55
; ---------------------------------------------------------------------------

loc_1CE3F:                              ; CODE XREF: blacksmith_menu+180↑j
                jmp     loc_1CD4D
; ---------------------------------------------------------------------------

loc_1CE42:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu:jpt_1CE2E↓o
                mov     ax, 1           ; jumptable 0001CE2E case 65

loc_1CE45:                              ; CODE XREF: blacksmith_menu+197↓j
                                        ; blacksmith_menu+19D↓j ...
                push    ax
                call    smith_action_prompt
                add     sp, 2
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------

loc_1CE4E:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1EC↓o
                mov     ax, 2           ; jumptable 0001CE2E case 66
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE54:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1EE↓o
                mov     ax, 3           ; jumptable 0001CE2E case 67
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE5A:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F0↓o
                mov     ax, 4           ; jumptable 0001CE2E case 68
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE60:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F2↓o
                mov     ax, 5           ; jumptable 0001CE2E case 69
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE66:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F4↓o
                mov     ax, 6           ; jumptable 0001CE2E case 70
                jmp     short loc_1CE45
; ---------------------------------------------------------------------------
                align 2

loc_1CE6C:                              ; CODE XREF: blacksmith_menu+174↑j
                                        ; DATA XREF: blacksmith_menu+1F6↓o
                push    word_23028      ; jumptable 0001CE2E case 71
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                call    near ptr byte_1C6F2+0Ah
                add     sp, 4

loc_1CE80:                              ; CODE XREF: blacksmith_menu+1DC↓j
                mov     [bp+var_2], 0
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------
                align 2

def_1CE2E:                              ; CODE XREF: blacksmith_menu+16F↑j
                push    [bp+var_4]      ; jumptable 0001CE2E default case
                call    smith_draw
                add     sp, 2
                mov     word_23078, ax
                or      ax, ax
                jz      short loc_1CE80
                mov     ax, [bp+var_4]
                sub     ax, 31h ; '1'
                mov     word_23028, ax
                jmp     short loc_1CEB2
; ---------------------------------------------------------------------------
                align 2
jpt_1CE2E       dw offset loc_1CE42     ; DATA XREF: blacksmith_menu+174↑r
                                        ; jump table for switch statement
                dw offset loc_1CE4E     ; jumptable 0001CE2E case 66
                dw offset loc_1CE54     ; jumptable 0001CE2E case 67
                dw offset loc_1CE5A     ; jumptable 0001CE2E case 68
                dw offset loc_1CE60     ; jumptable 0001CE2E case 69
                dw offset loc_1CE66     ; jumptable 0001CE2E case 70
                dw offset loc_1CE6C     ; jumptable 0001CE2E case 71
; ---------------------------------------------------------------------------

loc_1CEB2:                              ; CODE XREF: blacksmith_menu+192↑j
                                        ; blacksmith_menu+1CB↑j ...
                cmp     [bp+var_4], 1Bh
                jz      short loc_1CEBB
                jmp     loc_1CDB2
; ---------------------------------------------------------------------------

loc_1CEBB:                              ; CODE XREF: blacksmith_menu+1FC↑j
                call    thk_res_35A8

loc_1CEBE:                              ; CODE XREF: blacksmith_menu+E4↑j
                call    thk_2PLAY_A580
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
blacksmith_menu endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CEC8       proc near               ; CODE XREF: seg002:07AD↑J

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                mov     byte_2294F, 0FDh
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     [bp+var_2], 0
                sub     ax, ax
                mov     cx, 5
                mov     di, 9680h
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                stosb
                add     [bp+var_2], 0Bh
                sub     si, si
                mov     di, 58B8h

loc_1CEF9:                              ; CODE XREF: sub_1CEC8+4E↓j
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
                jl      short loc_1CEF9
                mov     [bp+var_2], si
                call    thk_monster_anim_step
                mov     byte_26ED0, 0FFh
                mov     byte_26ED1, 0E1h
                mov     byte_26ED2, 0C2h ; CODE XREF: seg002:050D↑J
                mov     byte_26ED3, 0C1h
                mov     byte_26ED4, 0E0h
                mov     byte_1DC65, 83h
                mov     [bp+var_2], 5
                sub     ax, ax
                mov     cx, 3
                mov     di, 9685h
                push    ds
                pop     es
                repne stosw
                add     [bp+var_2], 6
                call    thk_start_combat
                cmp     byte_1DD59, 0
                jz      short loc_1CF6C
                mov     ax, 0FFFFh      ; CODE XREF: seg002:0AE9↑J
                push    ax
                push    ax
                push    ax
                call    thk_monster_gfx_draw
                add     sp, 6
                call    loc_1D2A4
                jmp     short loc_1CF71
; ---------------------------------------------------------------------------

loc_1CF6C:                              ; CODE XREF: sub_1CEC8+91↑j
                mov     byte_1DBE5, 1

loc_1CF71:                              ; CODE XREF: sub_1CEC8+A2↑j
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CEC8       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CF78       proc near               ; CODE XREF: sub_1D098+18↓p

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
                sub     sp, 10h
                mov     [bp+var_10], 30h ; '0'
                mov     ax, word_1DC86  ; CODE XREF: seg002:062D↑J
                mov     dx, word_1DC88
                mov     [bp+var_A], ax
                mov     [bp+var_8], dx
                call    thk_text_get_x
                mov     [bp+var_C], ax
                call    thk_text_get_y
                mov     [bp+var_E], ax
                mov     word_221B4, 1
                sub     ax, ax
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 0EA60h
                sub     dx, dx
                push    dx
                push    ax
                push    [bp+var_8]
                push    [bp+var_A]
                call    thk__aFuldiv
                mov     [bp+var_2], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_2]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0EA60h
                sub     dx, dx
                push    dx
                push    ax
                sub     ax, ax
                push    ax
                push    [bp+var_2]
                call    thk__aFulmul
                sub     [bp+var_A], ax
                sbb     [bp+var_8], dx
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 3E8h
                cwd
                push    dx
                push    ax
                push    [bp+var_8]
                push    [bp+var_A]
                call    thk__aFuldiv
                mov     [bp+var_4], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_4]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 3E8h
                mul     [bp+var_4]
                sub     dx, dx
                sub     [bp+var_A], ax
                sbb     [bp+var_8], dx
                mov     ax, 3Ah ; ':'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_A]
                sub     dx, dx
                mov     cx, 0Ah
                div     cx
                mov     [bp+var_6], ax
                mov     ax, 30h ; '0'
                push    ax
                mov     ax, 2
                push    ax
                push    [bp+var_6]
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_E]
                push    [bp+var_C]
                call    thk_text_goto_xy
                add     sp, 4
                mov     word_221B4, 0
                cmp     [bp+var_2], 0
                jnz     short loc_1D094
                cmp     [bp+var_4], 0
                jnz     short loc_1D094
                cmp     [bp+var_6], 0
                jnz     short loc_1D094
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D092
                mov     ax, 1
                jmp     short loc_1D094
; ---------------------------------------------------------------------------

loc_1D092:                              ; CODE XREF: sub_1CF78+113↑j
                sub     ax, ax

loc_1D094:                              ; CODE XREF: sub_1CF78+FE↑j
                                        ; sub_1CF78+104↑j ...
                mov     sp, bp
                pop     bp
                retn
sub_1CF78       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D098       proc near               ; CODE XREF: sub_1D098:loc_1D127↓p

var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

; FUNCTION CHUNK AT D16A SIZE 00000032 BYTES

                push    bp
                mov     bp, sp
                sub     sp, 4
                push    si
                mov     ax, word_221B4
                mov     [bp+var_2], ax
                mov     word_221B4, 0

loc_1D0AB:                              ; CODE XREF: sub_1D098+66↓j
                call    thk_kbd_poll
                mov     si, ax
                call    sub_1CF78
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D0C2
                mov     si, 0Dh
                jmp     short loc_1D0CC
; ---------------------------------------------------------------------------
                align 2

loc_1D0C2:                              ; CODE XREF: seg002:04DD↑J
                                        ; sub_1D098+22↑j
                sub     word_1DC86, 4Bh ; 'K'
                sbb     word_1DC88, 0

loc_1D0CC:                              ; CODE XREF: sub_1D098+27↑j
                or      si, si
                jnz     short loc_1D0FC
                mov     bx, word_1DD62
                inc     word_1DD62
                mov     al, [bx+50Ah]
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
                cmp     word_1DD62, 8
                jnz     short loc_1D0F2
                mov     word_1DD62, 0

loc_1D0F2:                              ; CODE XREF: sub_1D098+52↑j
                mov     ax, 32h ; '2'
                push    ax
                call    thk_delay_ticks
                add     sp, 2

loc_1D0FC:                              ; CODE XREF: sub_1D098+36↑j
                or      si, si
                jz      short loc_1D0AB
                mov     [bp+var_4], si
                mov     ax, 20h ; ' '
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, [bp+var_2]
                mov     word_221B4, ax
                mov     ax, si
                pop     si
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------

loc_1D11A:                              ; CODE XREF: ovl_2SMITH:D5F1↓p
                push    bp
                mov     bp, sp
                sub     sp, 4
                push    di
                push    si
                sub     si, si
                mov     di, [bp+arg_0]

loc_1D127:                              ; CODE XREF: sub_1D098+DC↓j
                call    sub_1D098
                mov     byte ptr [bp+var_4], al
                cmp     al, 8
                jz      short loc_1D135
                cmp     al, 0F0h
                jnz     short loc_1D146

loc_1D135:                              ; CODE XREF: sub_1D098+97↑j
                or      si, si
                jz      short loc_1D146
                mov     ax, 1
                push    ax
                call    thk_text_erase_chars
                add     sp, 2
                dec     si
                jmp     short loc_1D16A
; ---------------------------------------------------------------------------

loc_1D146:                              ; CODE XREF: sub_1D098+9B↑j
                                        ; sub_1D098+9F↑j
                cmp     byte ptr [bp+var_4], 20h ; ' '
                jb      short loc_1D16A
                cmp     byte ptr [bp+var_4], 7Fh
                ja      short loc_1D16A
                cmp     si, 8
                jge     short loc_1D16A
                mov     bx, si
                add     bx, di          ; CODE XREF: seg002:0825↑J
sub_1D098       endp

                mov     al, [bp-4]
                mov     [bx], al
                inc     si
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2
; START OF FUNCTION CHUNK FOR sub_1D098

loc_1D16A:                              ; CODE XREF: sub_1D098+AC↑j
                                        ; sub_1D098+B2↑j ...
                cmp     byte ptr [bp+var_4], 1Bh
                jz      short loc_1D176
                cmp     byte ptr [bp+var_4], 0Dh
                jnz     short loc_1D127

loc_1D176:                              ; CODE XREF: sub_1D098+D6↑j
                mov     [bp+var_2], si
                cmp     byte ptr [bp+var_4], 1Bh
                jnz     short loc_1D18B
                push    si
                call    thk_text_erase_chars
                add     sp, 2
                mov     [bp+var_2], 0

loc_1D18B:                              ; CODE XREF: sub_1D098+E5↑j
                mov     bx, [bp+var_2]
                mov     si, [bp+arg_0]
                mov     byte ptr [bx+si], 0
                mov     ax, bx
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; END OF FUNCTION CHUNK FOR sub_1D098

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D19C       proc near               ; CODE XREF: ovl_2SMITH:D558↓p

var_1E          = byte ptr -1Eh
var_4           = word ptr -4
var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 20h
                push    di
                push    si
                mov     [bp+var_4], 0
                sub     ax, ax
                mov     cx, 0Dh
                lea     di, [bp+var_1E]
                push    ss
                pop     es
                assume es:nothing
                repne stosw
                add     [bp+var_4], 1Ah
                sub     si, si

loc_1D1BB:                              ; CODE XREF: sub_1D19C+3D↓j
                                        ; sub_1D19C+55↓j
                mov     ax, 1Ah
                push    ax
                mov     ax, 1
                push    ax
                call    thk_rand_range
                add     sp, 4
                dec     al
                mov     byte ptr [bp+var_2], al
                mov     di, [bp+var_2]
                and     di, 0FFh
                cmp     [bp+di+var_1E], 0
                jnz     short loc_1D1BB
                mov     di, [bp+var_2]
                and     di, 0FFh
                mov     [bp+di+var_1E], 1
                mov     al, byte ptr [bp+var_2]
                mov     [si+584Eh], al
                inc     si
                cmp     si, 1Ah
                jl      short loc_1D1BB
                mov     [bp+var_4], si
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D19C       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D1FC       proc near               ; CODE XREF: sub_1D236+11↓p
                                        ; ovl_2SMITH:D5A5↓p

arg_0           = byte ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                test    byte ptr [si+482Bh], 1
                jz      short loc_1D216
                mov     al, [si+580Dh]
                add     al, 41h ; 'A'
                jmp     short loc_1D22A
; ---------------------------------------------------------------------------

loc_1D216:                              ; CODE XREF: sub_1D1FC+10↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                mov     si, ax
                test    byte ptr [si+482Bh], 2
                jz      short loc_1D22D
                mov     al, [si+57EDh]
                add     al, 61h ; 'a'

loc_1D22A:                              ; CODE XREF: sub_1D1FC+18↑j
                mov     [bp+arg_0], al

loc_1D22D:                              ; CODE XREF: sub_1D1FC+26↑j
                mov     al, [bp+arg_0]
                sub     ah, ah
                pop     si
                pop     bp
                retn
sub_1D1FC       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D236       proc near               ; CODE XREF: ovl_2SMITH:D570↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
                mov     si, [bp+arg_0]
                jmp     short loc_1D256
; ---------------------------------------------------------------------------
                align 2

loc_1D240:                              ; CODE XREF: sub_1D236+23↓j
                inc     si
                mov     al, [si-1]
                sub     ah, ah
                push    ax
                call    sub_1D1FC
                add     sp, 2
                sub     ah, ah
                push    ax
                call    thk_text_putc
                add     sp, 2

loc_1D256:                              ; CODE XREF: sub_1D236+7↑j
                cmp     byte ptr [si], 0
                jnz     short loc_1D240
                mov     [bp+arg_0], si
                pop     si
                pop     bp
                retn
sub_1D236       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D262       proc near               ; CODE XREF: ovl_2SMITH:D60F↓p

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    di
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D274
                mov     ax, 2
                jmp     short loc_1D2A0
; ---------------------------------------------------------------------------

loc_1D274:                              ; CODE XREF: sub_1D262+B↑j
                push    ds
                pop     es
                assume es:DGROUP
                mov     di, [bp+arg_0]
                mov     cx, 0FFFFh
                xor     ax, ax
                repne scasb
                not     cx
                dec     cx
                cmp     cx, 8
                jz      short loc_1D28C

loc_1D288:                              ; CODE XREF: sub_1D262+39↓j
                sub     ax, ax
                jmp     short loc_1D2A0
; ---------------------------------------------------------------------------

loc_1D28C:                              ; CODE XREF: sub_1D262+24↑j
                mov     ax, 58AEh
                push    ax
                push    [bp+arg_0]
                call    thk__stricmp
                add     sp, 4
                or      ax, ax
                jnz     short loc_1D288
                mov     ax, 1

loc_1D2A0:                              ; CODE XREF: sub_1D262+10↑j
                                        ; sub_1D262+28↑j
                pop     di
                pop     bp
                retn
sub_1D262       endp

; ---------------------------------------------------------------------------
                align 2

loc_1D2A4:                              ; CODE XREF: sub_1CEC8+9F↑p
                push    bp
                mov     bp, sp
                sub     sp, 24h
                push    di
                push    si
                mov     word ptr [bp-1Ah], 0
                sub     ax, ax
                mov     [bp-1Ch], ax
                mov     [bp-1Eh], ax
                mov     [bp-0Eh], ax
                mov     [bp-10h], ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                mov     ax, 3
                push    ax
                call    thk_res_670A
                add     sp, 2
                mov     word ptr [bp-18h], 4
                mov     si, 58B8h
                mov     di, 4

loc_1D2DB:                              ; CODE XREF: ovl_2SMITH:D2E4↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D2DB
                mov     word ptr [bp-18h], 4
                mov     si, 58C0h
                mov     di, 4

loc_1D2F1:                              ; CODE XREF: ovl_2SMITH:D2FA↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D2F1
                mov     word ptr [bp-18h], 0Eh
                mov     si, 5892h
                mov     di, 0Eh

loc_1D307:                              ; CODE XREF: ovl_2SMITH:D310↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D307
                mov     word ptr [bp-18h], 4
                mov     si, 5846h
                mov     di, 4

loc_1D31D:                              ; CODE XREF: ovl_2SMITH:D326↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D31D
                mov     word ptr [bp-18h], 0Bh
                mov     si, 5868h
                mov     di, 0Bh

loc_1D333:                              ; CODE XREF: ovl_2SMITH:D33C↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D333
                mov     word ptr [bp-18h], 0Ah
                mov     si, 587Eh
                mov     di, 0Ah

loc_1D349:                              ; CODE XREF: ovl_2SMITH:D352↓j
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D349
                mov     g_disk_needed, 1

loc_1D35A:                              ; CODE XREF: ovl_2SMITH:D36C↓j
                push    word_1DD50
                call    thk_gfx_load_image
                add     sp, 2
                mov     [bp-10h], ax
                mov     [bp-0Eh], dx
                or      ax, dx
                jz      short loc_1D35A
                mov     g_disk_needed, 2
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 40h ; '@'
                push    ax
                sub     ax, ax
                push    ax
                push    dx
                push    word ptr [bp-10h]
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 14h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 45BCh
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 3
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aTheControlRoom ; "        the control room..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                call    thk_res_34BA
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 58C0h

loc_1D3DE:                              ; CODE XREF: ovl_2SMITH:D3FB↓j
                lea     ax, [si+11h]
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
                jl      short loc_1D3DE
                mov     [bp-18h], si
                mov     ax, 15h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4

loc_1D40E:                              ; CODE XREF: ovl_2SMITH:D420↓j
                mov     ax, 0Ah
                push    ax
                lea     ax, [bp-0Ch]
                push    ax
                call    thk_read_string
                add     sp, 4
                mov     si, ax
                or      si, si
                jz      short loc_1D40E
                mov     [bp-18h], si
                cmp     si, 0Ah
                jz      short loc_1D444
                mov     ax, 0Ah
                sub     ax, si
                mov     [bp-24h], ax
                mov     al, 20h ; ' '
                mov     cx, [bp-24h]
                lea     di, [bp+si-0Ch]
                push    ss
                pop     es
                assume es:nothing
                repne stosb
                mov     ax, [bp-24h]
                add     [bp-18h], ax

loc_1D444:                              ; CODE XREF: ovl_2SMITH:D428↑j
                mov     byte ptr [bp-2], 0
                sub     si, si
                mov     di, [bp-1Ah]
                jmp     short loc_1D451
; ---------------------------------------------------------------------------
                align 2

loc_1D450:                              ; CODE XREF: ovl_2SMITH:D46A↓j
                inc     si

loc_1D451:                              ; CODE XREF: ovl_2SMITH:D44D↑j
                cmp     si, g_party_size
                jge     short loc_1D46C
                push    si
                call    thk_char_ptr
                add     sp, 2
                mov     bx, ax
                test    byte ptr [bx+81h], 20h
                jz      short loc_1D468
                inc     di

loc_1D468:                              ; CODE XREF: ovl_2SMITH:D465↑j
                or      di, di
                jz      short loc_1D450

loc_1D46C:                              ; CODE XREF: ovl_2SMITH:D455↑j
                mov     [bp-1Ah], di
                mov     [bp-18h], si
                mov     ax, offset aWafe ; "WAFE      "
                push    ax
                lea     ax, [bp-0Ch]
                push    ax
                call    thk__stricmp
                add     sp, 4
                or      ax, ax
                jz      short loc_1D487
                jmp     loc_1D7E8
; ---------------------------------------------------------------------------

loc_1D487:                              ; CODE XREF: ovl_2SMITH:D482↑j
                or      di, di
                jnz     short loc_1D48E
                jmp     loc_1D7E8
; ---------------------------------------------------------------------------

loc_1D48E:                              ; CODE XREF: ovl_2SMITH:D489↑j
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 12h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aThankYou ; "         Thank you."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 5
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aRecalculatingT ; "Recalculating trajectory now..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     ax, 15h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aErrorErrorComp ; "Error, Error!  Computer malfunction."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 16h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aInternalProgra ; "    Internal program override..."
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 12Ch
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                mov     ax, 4
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_main_frame
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                sub     si, si
                mov     di, 5892h

loc_1D536:                              ; CODE XREF: ovl_2SMITH:D553↓j
                lea     ax, [si+2]
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
                cmp     si, 0Eh
                jl      short loc_1D536
                mov     [bp-18h], si
                call    sub_1D19C
                sub     si, si
                mov     di, 5846h

loc_1D560:                              ; CODE XREF: ovl_2SMITH:D57D↓j
                lea     ax, [si+11h]
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    sub_1D236
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 4
                jl      short loc_1D560
                mov     [bp-18h], si
                mov     ax, 16h
                push    ax
                mov     ax, 1
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aAnswerPreamble ; "Answer= Preamble     Code="
                push    ax
                call    thk_text_puts
                add     sp, 2
                sub     si, si

loc_1D59C:                              ; CODE XREF: ovl_2SMITH:D5B3↓j
                mov     bx, word ptr unk_21E0A
                mov     al, [bx+si]
                sub     ah, ah
                push    ax
                call    sub_1D1FC
                add     sp, 2
                mov     [si+58AEh], al
                inc     si
                cmp     si, 8
                jl      short loc_1D59C
                mov     [bp-18h], si
                mov     byte_23106, 0
                mov     word_1DC86, 0BBA0h
                mov     word_1DC88, 0Dh

loc_1D5C9:                              ; CODE XREF: ovl_2SMITH:D609↓j
                                        ; ovl_2SMITH:D619↓j
                mov     ax, 16h
                push    ax
                mov     ax, 26h ; '&'
                push    ax
                mov     ax, 16h
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_clear_text_rect
                add     sp, 8           ; CODE XREF: seg002:0795↑J
                mov     ax, 16h
                push    ax
                mov     ax, 1Ch
                push    ax
                call    thk_text_goto_xy ; CODE XREF: seg002:07A1↑J
                add     sp, 4
                lea     ax, [bp-0Ch]
                push    ax
                call    loc_1D11A

loc_1D5F4:                              ; CODE XREF: seg002:0819↑J
                add     sp, 2
                mov     si, ax
                or      si, si
                jnz     short loc_1D607
                mov     ax, word_1DC86
                or      ax, word_1DC88
                jnz     short loc_1D607
                inc     si

loc_1D607:                              ; CODE XREF: ovl_2SMITH:D5FB↑j
                                        ; ovl_2SMITH:D604↑j
                or      si, si
                jz      short loc_1D5C9
                lea     ax, [bp-0Ch]
                push    ax
                call    sub_1D262
                add     sp, 2
                mov     si, ax
                or      si, si
                jz      short loc_1D5C9
                mov     [bp-18h], si
                cmp     si, 2
                jnz     short loc_1D62C
                mov     byte_1DBE5, 3
                jmp     loc_1D830
; ---------------------------------------------------------------------------
                align 2

loc_1D62C:                              ; CODE XREF: ovl_2SMITH:D621↑j
                mov     ax, 1
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     di, di
                mov     si, [bp-12h]
                jmp     short loc_1D66A
; ---------------------------------------------------------------------------
                align 2

loc_1D63E:                              ; CODE XREF: ovl_2SMITH:D66E↓j
                push    di
                call    thk_char_ptr
                add     sp, 2
                mov     si, ax
                test    byte ptr [si+81h], 8
                jnz     short loc_1D65D
                or      byte ptr [si+81h], 8
                add     word ptr [si+62h], 0F080h
                adc     word ptr [si+64h], 2FAh

loc_1D65D:                              ; CODE XREF: ovl_2SMITH:D64C↑j
                mov     ax, [si+62h]
                mov     dx, [si+64h]
                add     [bp-1Eh], ax
                adc     [bp-1Ch], dx
                inc     di

loc_1D66A:                              ; CODE XREF: ovl_2SMITH:D63B↑j
                cmp     di, g_party_size
                jl      short loc_1D63E
                mov     [bp-18h], di
                mov     [bp-12h], si
                mov     al, byte_1DB95
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_main_frame
                mov     al, byte_1DB96
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                mov     ax, 3
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 5868h

loc_1D6A0:                              ; CODE XREF: ovl_2SMITH:D6BD↓j
                lea     ax, [si+1]
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                push    word ptr [di]
                call    thk_text_puts
                add     sp, 2
                add     di, 2
                inc     si
                cmp     si, 0Bh
                jl      short loc_1D6A0
                mov     [bp-18h], si
                sub     si, si
                mov     di, 587Eh

loc_1D6C7:                              ; CODE XREF: ovl_2SMITH:D6E4↓j
                lea     ax, [si+0Dh]
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
                cmp     si, 0Ah
                jl      short loc_1D6C7
                mov     [bp-18h], si
                mov     ax, 0Dh
                push    ax
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_1DC60
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0Dh
                push    ax
                mov     ax, 19h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word_1DC62
                call    thk_text_put_number_pad
                add     sp, 6
                mov     ax, 0Eh
                push    ax
                mov     ax, 15h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, 20h ; ' '
                push    ax
                mov     ax, 1
                push    ax
                push    word ptr [bp-1Ch]
                push    word ptr [bp-1Eh]
                call    thk_text_put_number
                add     sp, 8
                mov     ax, 0Ch
                push    ax
                mov     ax, 0Eh
                push    ax
                mov     ax, 1
                push    ax
                push    ax
                call    thk_text_window_create
                add     sp, 8
                mov     [bp-20h], ax
                push    ax
                call    thk_text_window_open
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_text_window_set_font
                add     sp, 2
                mov     al, byte_1DB8E
                sub     ah, ah
                push    ax
                call    thk_text_set_fg
                add     sp, 2
                call    thk_draw_frame_alt
                push    word ptr [bp-20h]
                call    thk_text_window_close
                add     sp, 2
                sub     ax, ax
                push    ax
                mov     ax, 1
                push    ax
                call    thk_gfx_copy_page
                add     sp, 4
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     si, si

loc_1D79E:                              ; CODE XREF: ovl_2SMITH:D7D5↓j
                mov     word ptr [bp-14h], 10h
                mov     di, 0Dh
                or      si, si
                jz      short loc_1D7AD
                add     di, 16h

loc_1D7AD:                              ; CODE XREF: ovl_2SMITH:D7A8↑j
                push    di
                push    word ptr [bp-14h]
                push    si
                inc     si
                push    word ptr [bp-0Eh]
                push    word ptr [bp-10h]
                call    thk_gfx_draw_op13
                add     sp, 0Ah
                mov     ax, 4Bh ; 'K'
                push    ax
                call    thk_delay_ticks
                add     sp, 2
                cmp     si, 0Ch
                jnz     short loc_1D7D0
                sub     si, si

loc_1D7D0:                              ; CODE XREF: ovl_2SMITH:D7CC↑j
                call    thk_kbd_poll
                or      ax, ax
                jz      short loc_1D79E
                mov     [bp-16h], di
                mov     [bp-18h], si
                call    thk_save_roster
                mov     byte_1DBE5, 2
                jmp     short loc_1D830
; ---------------------------------------------------------------------------
                align 2

loc_1D7E8:                              ; CODE XREF: ovl_2SMITH:D484↑j
                                        ; ovl_2SMITH:D48B↑j
                mov     ax, 16h
                push    ax
                mov     ax, 0Fh
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, offset aIncorrect ; "Incorrect!"
                push    ax
                call    thk_text_puts
                add     sp, 2
                mov     ax, 0C8h
                push    ax
                call    thk_wait_key_timeout
                add     sp, 2
                sub     al, al
                mov     g_party_y, al
                mov     g_party_x, al
                call    thk_2PLAY_B75E
                mov     byte_1DC80, 7
                call    thk_2PLAY_A580
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                cmp     g_view_mode, 1
                jnz     short loc_1D830
                call    thk_2PLAY_B862

loc_1D830:                              ; CODE XREF: ovl_2SMITH:D628↑j
                                        ; ovl_2SMITH:D7E5↑j ...
                push    word ptr [bp-0Eh]
                push    word ptr [bp-10h]
                call    thk_free_far_block
                add     sp, 4
                sub     ax, ax
                mov     word_1DC88, ax
                mov     word_1DC86, ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
; ---------------------------------------------------------------------------
                db    0
                db    0
                db    0
                db    0
                db    0
                db    0
ovl_2SMITH      ends

