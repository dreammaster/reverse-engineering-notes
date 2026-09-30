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
                push    bp
; ---------------------------------------------------------------------------
                db  8Bh
byte_1C132      db 0ECh, 83h, 0ECh, 1Ah, 57h, 56h, 2Bh, 0F6h, 80h, 0Eh
                                        ; DATA XREF: seg002:0038↑o
                db 30h, 4, 2, 89h, 76h, 0F6h, 0EBh, 0Bh, 41h, 83h, 0F9h
                db 6, 7Dh, 36h, 0EBh, 20h, 0FFh, 46h, 0F6h, 0A1h, 26h
                db 4, 39h, 46h, 0F6h, 7Dh, 33h, 0FFh, 76h, 0F6h, 0E8h
                db 5, 0B0h, 83h, 0C4h, 2, 89h, 46h, 0FCh, 0C7h, 46h, 0F0h
                db 2 dup(0), 8Bh, 0F8h, 2Bh, 0C9h, 8Bh, 0D9h, 8Ah, 51h
                db 3Ah, 80h, 0FAh, 0D0h, 72h, 6, 80h, 0FAh, 0D3h, 77h
                db 1, 46h, 0Bh, 0F6h, 74h, 0C4h, 88h, 56h, 0FEh, 89h, 4Eh
                db 0F0h, 0Bh, 0F6h, 74h, 0C2h, 89h, 76h, 0EEh, 0Bh, 0F6h
                db 75h, 35h, 2Bh, 0C0h, 50h, 0E8h, 5Fh, 0ACh, 83h, 0C4h
                db 2, 0B8h, 14h, 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 89h, 0ADh
                db 83h, 0C4h, 4, 0B8h, 60h, 40h, 50h, 0E8h, 0A3h, 0ADh
                db 83h, 0C4h, 2, 0B8h, 15h, 0, 50h, 0B8h, 9, 0, 50h, 0E8h
                db 71h, 0ADh, 83h, 0C4h, 4, 0B8h, 85h, 40h, 0E9h, 0DBh
                db 1, 8Ah, 46h, 0FEh, 2Ch, 0D0h, 0A2h, 2Fh, 4, 0FFh, 76h
                db 0F0h, 0FFh, 76h, 0FCh, 0E8h, 0DFh, 0AFh, 83h, 0C4h
                db 4
byte_1C1DA      db 2Bh, 0C0h, 50h, 0E8h, 16h, 0ACh, 83h, 0C4h, 2, 0B8h
                                        ; CODE XREF: seg002:08CD↑J
                db 14h, 0, 50h, 0B8h, 1, 0
byte_1C1EA      db 50h, 0E8h, 40h, 0ADh, 83h, 0C4h, 4, 0B8h, 9Dh, 40h
                                        ; CODE XREF: seg002:07F5↑J
                db 50h, 0E8h, 5Ah, 0ADh, 83h, 0C4h, 2, 0B8h, 15h, 0, 50h
                db 0B8h, 9, 0, 50h, 0E8h, 28h, 0ADh, 83h, 0C4h, 4, 0B8h
                db 0C3h, 40h, 50h, 0E8h, 42h, 0ADh, 83h, 0C4h, 2, 0A0h
                db 2Fh, 4, 8Bh, 0C8h, 0D0h, 0E0h, 2, 0C1h, 88h, 46h, 0FAh
                db 8Ah, 1Eh, 92h, 3, 2Ah, 0FFh, 8Ah, 87h, 2, 41h, 0, 46h
                db 0FAh, 0B1h, 4, 0D2h, 66h, 0FAh, 0B8h, 10h, 0, 50h, 0B8h
                db 1, 0, 50h, 0E8h, 3Ah, 0ADh
byte_1C23C      db 83h, 0C4h, 4, 0, 46h, 0FAh ; CODE XREF: seg002:08E5↑J
byte_1C242      db 0C7h, 46h, 0F6h, 2 dup(0), 2Bh, 0C0h, 0B9h, 5, 0, 0BFh
                                        ; CODE XREF: seg002:0639↑J
                db 80h, 96h, 1Eh, 7, 0F2h, 0ABh, 0AAh, 83h, 46h, 0F6h
                db 0Bh, 2Bh, 0F6h, 8Bh, 0Eh, 26h, 4, 0EBh, 8, 8Ah, 46h
                db 0FAh, 88h, 84h, 80h, 96h, 46h, 3Bh, 0F1h, 7Ch, 0F4h
                db 89h, 76h, 0F6h, 0C6h, 6, 15h, 4, 80h, 0E8h, 93h, 0AFh
                db 80h, 3Eh, 9, 5, 0, 75h, 8, 0C6h, 6, 95h, 3, 1, 0E9h
                db 22h, 1, 0C6h, 6, 9, 5, 0, 0E8h, 44h, 0AEh, 8Ah, 46h
                db 0FEh, 2Ch, 0D0h, 88h, 46h, 0FAh, 0A0h, 92h, 3, 88h
                db 46h, 0F8h, 3Ch, 2, 76h, 4, 0C6h, 46h, 0F8h, 2, 8Ah
                db 46h, 0FAh, 2Ah, 0E4h, 8Bh, 0D8h, 0D1h, 0E3h, 3, 0D8h
                db 0D1h, 0E3h, 0D1h, 0E3h, 8Bh, 76h, 0F8h, 81h, 0E6h, 0FFh
                db 0, 0D1h, 0E6h, 0D1h, 0E6h, 8Bh, 80h
byte_1C2C0      db 8, 41h, 8Bh, 90h, 0Ah, 41h, 89h, 46h, 0F2h, 89h, 56h
                                        ; CODE XREF: seg002:026D↑J
                db 0F4h, 0C7h, 46h, 0F6h, 2 dup(0), 83h, 3Eh, 26h, 4, 0
                db 7Eh, 75h, 8Ah, 46h, 0FAh, 2Ah, 0E4h, 8Bh, 0C8h, 0D1h
                db 0E0h, 3, 0C1h, 8Ah, 4Eh, 0F8h, 2Ah, 0EDh, 3, 0C1h, 89h
                db 46h, 0E6h, 5, 44h, 41h, 89h, 46h, 0ECh, 8Bh, 46h, 0E6h
                db 5, 38h
byte_1C2F8      db 41h, 89h, 46h, 0EAh, 0C7h, 46h, 0E8h, 16h, 4, 8Bh, 76h
                                        ; CODE XREF: seg002:0651↑J
                db 0F6h, 56h, 0E8h, 5Ah, 0AEh
byte_1C308      db 83h, 0C4h, 2, 8Bh, 0F8h, 8Bh, 5Eh, 0E8h, 83h, 3Fh, 18h
                                        ; CODE XREF: seg002:08F1↑J
                db 7Dh, 14h, 8Bh, 46h, 0F2h, 8Bh, 56h, 0F4h, 1, 45h, 66h
                db 11h, 55h, 68h, 2Bh, 0C0h, 89h, 46h, 0F4h, 89h, 46h
                db 0F2h, 8Bh, 5Eh, 0EAh, 8Ah, 1Fh, 2Ah, 0FFh, 8Dh, 41h
                db 79h, 8Bh, 5Eh, 0ECh, 8Ah, 0Fh, 8Bh, 0D8h, 8, 0Fh, 83h
                db 46h, 0E8h, 2, 46h, 3Bh, 36h, 26h, 4, 7Ch, 0BDh, 89h
                db 7Eh, 0FCh, 89h, 76h, 0F6h, 2Bh, 0C0h, 50h, 0E8h, 0A3h
                db 0AAh, 83h, 0C4h, 2, 0B8h, 14h, 0, 50h, 0B8h, 5, 0, 50h
                db 0E8h, 0CDh, 0ABh, 83h, 0C4h, 4, 0B8h, 0DAh, 40h, 50h
                db 0E8h, 0E7h, 0ABh, 83h, 0C4h, 2, 0B8h, 20h
byte_1C370      db 0, 50h, 0B8h, 1, 0, 50h, 8Ah, 46h, 0FAh, 2Ah, 0E4h
                                        ; CODE XREF: seg002:0471↑J
                db 8Bh, 0D8h, 0D1h, 0E3h, 3, 0D8h, 0D1h, 0E3h, 0D1h, 0E3h
                db 8Bh, 76h, 0F8h, 81h, 0E6h, 0FFh, 0, 0D1h, 0E6h, 0D1h
                db 0E6h, 0FFh, 0B0h, 0Ah, 41h, 0FFh, 0B0h, 8, 41h, 0E8h
                db 0BBh, 0ADh, 83h, 0C4h, 8, 0B8h, 0EFh, 40h, 50h, 0E8h
                db 0ADh, 0ABh, 83h, 0C4h, 2, 80h, 3Eh, 95h, 3, 0, 75h
                db 0Ah, 0E8h, 8, 0AAh, 0Bh, 0C0h, 74h, 0F9h, 0E8h, 1Dh
                db 0AFh, 80h, 0Eh, 0C8h, 59h, 80h, 8Bh, 36h, 94h, 3, 81h
                db 0E6h, 0FFh, 0, 0B1h, 4, 0D3h, 0E6h, 8Ah, 1Eh, 93h, 3
                db 2Ah, 0FFh, 80h, 88h, 0D6h, 5Ah, 80h, 5Eh, 5Fh, 8Bh
                db 0E5h, 5Dh, 0C3h, 90h, 55h, 8Bh, 0ECh, 83h, 0ECh, 4
                db 80h, 0Eh, 30h, 4, 1, 8Bh, 1Eh, 0, 41h, 0A0h, 26h, 2 dup(4)
                db 30h, 88h, 47h, 1Eh, 0E8h, 50h, 0ABh
tavern_common_helper endp ; sp-analysis failed

byte_1C3F6      db 0FFh, 36h, 0, 41h, 0E8h, 7Dh, 0AAh, 83h, 0C4h, 2, 8Bh
                                        ; CODE XREF: seg002:047D↑J
                db 1Eh, 0, 41h, 8Ah, 47h, 1Eh, 2Ah, 0E4h, 50h, 0B8h, 31h
                db 0, 50h, 0E8h, 89h, 0AEh, 83h, 0C4h, 4, 2Ah, 0E4h, 89h
                db 46h, 0FCh, 3Dh, 1Bh, 0, 75h, 3, 0E9h, 0A1h, 1, 50h
                db 0E8h, 15h, 0ABh, 83h, 0C4h, 2, 83h, 6Eh, 0FCh, 31h
                db 0FFh, 76h, 0FCh, 0E8h, 30h, 0ADh, 83h, 0C4h, 2, 89h
                db 46h, 0FEh, 80h, 0Eh, 30h, 4, 2, 2Bh, 0C0h, 50h, 0E8h
                db 0B3h, 0A9h, 83h, 0C4h, 2, 0B8h, 13h, 0, 50h, 0B8h, 2
                db 0, 50h, 0E8h, 0DDh, 0AAh, 83h, 0C4h, 4, 0B8h, 50h, 41h
                db 50h, 0E8h, 0F7h, 0AAh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
byte_1C462      db 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh, 8Ah, 47h, 16h, 2Ah
                                        ; CODE XREF: seg002:086D↑J
                db 0E4h, 50h, 0E8h, 10h, 0ABh, 83h, 0C4h, 6, 0B8h, 13h
                db 0, 50h, 0B8h, 16h, 0, 50h, 0E8h, 0AEh, 0AAh, 83h, 0C4h
                db 4, 0B8h, 5Fh, 41h, 50h, 0E8h, 0C8h, 0AAh, 83h, 0C4h
                db 2, 0B8h
byte_1C48E      db 20h, 0, 50h, 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh, 8Ah, 47h
                                        ; CODE XREF: seg002:0B31↑J
                db 1Ah, 2Ah, 0E4h, 50h, 0E8h, 0E1h, 0AAh, 83h, 0C4h, 6
                db 0B8h, 14h, 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 7Fh, 0AAh
                db 83h, 0C4h, 4, 0B8h, 6Eh, 41h, 50h, 0E8h, 99h, 0AAh
                db 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h, 8Bh
                db 5Eh, 0FEh, 8Ah, 47h, 17h, 2Ah, 0E4h, 50h, 0E8h, 0B2h
                db 0AAh, 83h, 0C4h, 6, 0B8h, 14h, 0, 50h, 0B8h, 16h, 0
                db 50h, 0E8h, 50h, 0AAh, 83h, 0C4h, 4, 0B8h, 7Dh, 41h
                db 50h, 0E8h, 6Ah, 0AAh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
                db 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh, 8Ah, 47h, 1Bh, 2Ah
                db 0E4h, 50h, 0E8h, 83h, 0AAh, 83h, 0C4h, 6, 0B8h, 15h
                db 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 21h, 0AAh, 83h, 0C4h
                db 4, 0B8h, 8Ch, 41h, 50h, 0E8h, 3Bh, 0AAh, 83h, 0C4h
                db 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh
                db 8Ah, 47h, 18h, 2Ah, 0E4h, 50h, 0E8h
byte_1C52C      db 54h, 0AAh, 83h, 0C4h, 6, 0B8h, 15h, 0, 50h, 0B8h, 16h
                                        ; CODE XREF: seg002:0879↑J
                db 0, 50h, 0E8h, 0F2h, 0A9h, 83h, 0C4h, 4, 0B8h, 9Bh, 41h
                db 50h, 0E8h, 0Ch, 0AAh, 83h, 0C4h, 2, 0B8h, 20h, 0, 50h
                db 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh, 8Ah, 47h, 1Ch, 2Ah
                db 0E4h, 50h, 0E8h, 25h, 0AAh, 83h, 0C4h, 6, 0B8h, 16h
                db 0, 50h, 0B8h, 2, 0, 50h, 0E8h, 0C3h, 0A9h, 83h, 0C4h
                db 4, 0B8h, 0AAh, 41h, 50h, 0E8h, 0DDh, 0A9h, 83h, 0C4h
                db 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h, 8Bh, 5Eh, 0FEh
                db 8Ah, 47h, 19h, 2Ah, 0E4h, 50h, 0E8h, 0F6h, 0A9h, 83h
                db 0C4h, 6, 0B8h, 16h, 0, 2 dup(50h), 0E8h, 97h, 0A9h
                db 83h, 0C4h, 4, 0B8h, 0B9h, 41h, 50h, 0E8h, 0B1h, 0A9h
                db 83h, 0C4h, 2, 0B8h, 20h, 0, 50h, 0B8h, 1, 0, 50h
byte_1C5AC      db 8Bh, 5Eh, 0FEh, 8Ah, 47h, 1Dh, 2Ah, 0E4h, 50h, 0E8h
                                        ; CODE XREF: seg002:0885↑J
                db 0CAh, 0A9h, 83h, 0C4h, 6, 0E8h, 34h, 0ACh, 0Bh, 0C0h
byte_1C5C0      db 74h, 0F9h, 0E8h, 99h, 0A9h, 8Bh, 0E5h, 5Dh, 0C3h, 90h
                                        ; CODE XREF: seg002:029D↑J
                                        ; tavern_retrain_skills+131↓p ...
                db 55h, 8Bh, 0ECh, 8Ah, 46h, 6, 2Ah, 0E4h, 2Dh, 1, 0, 3Dh
                db 0Eh, 0
byte_1C5D8      db 76h, 3, 0E9h, 3, 2, 3, 0C0h, 93h, 2Eh, 0FFh, 0A7h, 0C2h
                                        ; CODE XREF: seg002:01A1↑J
                db 0C7h, 90h, 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5, 14h, 0
                db 50h, 0E8h, 32h, 0B1h, 83h, 0C4h, 4, 0B8h, 5, 0, 50h
                db 8Bh, 46h, 4, 5, 6Fh, 0, 50h, 0E8h, 21h, 0B1h, 83h, 0C4h
                db 4, 0E9h, 0D5h, 1, 90h, 0B8h, 5, 0, 50h, 8Bh, 46h, 4
                db 5, 13h, 0, 50h, 0E8h, 0Ch, 0B1h, 83h, 0C4h, 4, 0B8h
                db 5, 0, 50h, 8Bh, 46h, 4, 5, 6Eh, 0, 0EBh, 0D8h, 90h
                db 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5, 12h, 0, 50h, 0E8h
                db 0EEh, 0B0h, 83h, 0C4h, 4, 0B8h, 5, 0, 50h, 8Bh, 46h
                db 4, 5, 6Dh, 0, 0EBh, 0BAh, 90h, 0B8h, 5, 0, 50h, 8Bh
                db 46h, 4, 5, 15h, 0, 50h, 0E8h, 0D0h, 0B0h, 83h, 0C4h
                db 4, 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5, 70h, 0, 0EBh, 9Ch
                db 90h, 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5
byte_1C66E      db 10h, 0, 50h, 0E8h, 0B2h, 0B0h, 83h, 0C4h, 4, 0B8h, 5
                                        ; CODE XREF: seg002:0891↑J
                db 0, 50h, 8Bh, 46h, 4, 5, 6Bh, 0, 0E9h, 7Dh, 0FFh, 0B8h
                db 1, 0, 50h, 8Bh, 46h, 4, 5, 10h, 0, 50h, 0E8h, 94h, 0B0h
                db 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5, 11h
                db 0, 50h, 0E8h, 83h, 0B0h, 83h, 0C4h, 4, 0B8h, 1, 0, 50h
                db 8Bh, 46h, 4, 5, 12h, 0, 50h, 0E8h, 72h, 0B0h, 83h, 0C4h
                db 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5, 13h, 0, 50h, 0E8h
                db 61h, 0B0h, 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h
                db 4, 5, 14h, 0, 50h, 0E8h, 50h, 0B0h, 83h, 0C4h, 4, 0B8h
                db 1, 0, 50h, 8Bh, 46h, 4, 5, 15h, 0, 50h, 0E8h, 3Fh, 0B0h
                db 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5
byte_1C6F2      db 1Eh, 0, 50h, 0E8h, 2Eh, 0B0h, 83h, 0C4h, 4, 0B8h, 1
                                        ; CODE XREF: seg002:0B19↑J
                db 0, 50h, 8Bh, 46h, 4, 5, 27h, 0, 50h, 0E8h, 1Dh, 0B0h
                db 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5, 6Bh
                db 0, 50h, 0E8h, 0Ch, 0B0h, 83h, 0C4h, 4, 0B8h, 1, 0, 50h
                db 8Bh, 46h, 4, 5, 6Eh, 0, 50h, 0E8h, 0FBh, 0AFh, 83h
                db 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5, 6Fh, 0, 50h
                db 0E8h
byte_1C73A      db 0EAh, 0AFh, 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h
                                        ; CODE XREF: seg002:089D↑J
                db 4, 5, 73h, 0, 50h, 0E8h, 0D9h, 0AFh, 83h, 0C4h, 4, 0B8h
                db 1, 0, 50h, 8Bh, 46h, 4, 5, 6Ch, 0, 50h, 0E8h, 0C8h
                db 0AFh, 83h, 0C4h, 4, 0B8h, 1, 0, 50h, 8Bh, 46h, 4, 5
                db 6Dh, 0, 50h, 0E8h, 0B7h, 0AFh, 83h, 0C4h, 4, 0B8h, 1
                db 0, 0E9h, 0E4h, 0FEh, 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5
                db 11h, 0, 50h, 0E8h, 0A0h, 0AFh, 83h, 0C4h, 4, 0B8h, 5
                db 0, 50h, 8Bh, 46h, 4, 5, 6Ch, 0, 0E9h, 6Bh, 0FEh, 0B8h
                db 0Fh, 0, 50h, 8Bh, 46h, 4, 5, 1Eh, 0, 0E9h, 5Eh, 0FEh
                db 90h, 0B8h, 5, 0, 50h, 8Bh, 46h, 4, 5, 27h, 0, 50h, 0E8h
                db 74h, 0AFh, 83h, 0C4h, 4, 0B8h, 5, 0, 50h, 8Bh, 46h
                db 4, 5, 73h, 0, 0E9h, 3Fh, 0FEh, 0E6h, 0C5h, 0Ch, 0C6h
                db 0E0h, 0C7h, 0E0h, 0C7h, 2Ah, 0C6h, 48h, 0C6h, 66h, 0C6h
                db 84h, 0C6h, 78h, 0C7h, 0E0h, 0C7h, 0E0h, 0C7h, 0E0h
                db 0C7h, 0E0h, 0C7h, 96h, 0C7h, 0A4h, 0C7h, 5Dh, 0C3h

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
                call    near ptr byte_1C370+6Ch
                jmp     loc_1C945
; ---------------------------------------------------------------------------

loc_1C7FC:                              ; CODE XREF: tavern_retrain_skills+12↑j
                or      byte_1DC80, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
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
                call    thk_res_00E8
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
                call    thk_res_3FA0
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
                call    near ptr byte_1C5C0+0Ah
                add     sp, 4
                mov     bx, [bp+var_2]
                mov     al, [bx+50h]
                sub     ah, ah
                mov     cl, 4
                shr     ax, cl
                push    ax
                push    bx
                call    near ptr byte_1C5C0+0Ah
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

loc_1C94E:                              ; CODE XREF: sub_1CB7C+FE↓p
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

tavern_helper_a proc near               ; CODE XREF: sub_1CD60+186↓p

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

tavern_helper_b proc near               ; CODE XREF: sub_1CB08+68↓p
                                        ; sub_1CB7C+D9↓p ...

arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                push    si
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

sub_1CA46       proc near               ; CODE XREF: sub_1CF74:loc_1CFD2↓p
                                        ; sub_1D038:loc_1D054↓p

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

tavern_helper_c proc near               ; CODE XREF: sub_1CB08+1F↓p
                                        ; sub_1CB7C+CB↓p ...

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


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB08       proc near               ; CODE XREF: tavern_menu+357↓p

var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 0Ah
                push    si
                mov     bl, g_map_id
                sub     bh, bh
                shl     bx, 1
                mov     ax, [bx+4226h]
                cwd
                mov     [bp+var_4], ax
                mov     [bp+var_2], dx
                push    dx
                push    ax
                push    [bp+arg_0]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CB36
                mov     ax, 2
                jmp     short loc_1CB6F
; ---------------------------------------------------------------------------

loc_1CB36:                              ; CODE XREF: sub_1CB08+27↑j
                mov     [bp+var_6], 0
                cmp     g_party_size, 0

loc_1CB40:                              ; CODE XREF: seg002:0A65↑J
                jle     short loc_1CB6D
                mov     si, 416h
                mov     cx, g_party_size
                mov     ax, cx
                add     [bp+var_6], ax

loc_1CB4E:                              ; CODE XREF: sub_1CB08+63↓j
                mov     ax, 82h
                imul    word ptr [si]
                mov     bx, ax
                cmp     byte ptr [bx+7E45h], 28h ; '('
                jnb     short loc_1CB68
                mov     ax, 82h
                imul    word ptr [si]
                mov     bx, ax
                mov     byte ptr [bx+7E45h], 28h ; '('

loc_1CB68:                              ; CODE XREF: sub_1CB08+52↑j
                add     si, 2
                loop    loc_1CB4E

loc_1CB6D:                              ; CODE XREF: sub_1CB08:loc_1CB40↑j
                sub     ax, ax

loc_1CB6F:                              ; CODE XREF: sub_1CB08+2C↑j
                push    ax
                call    tavern_helper_b
                add     sp, 2
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CB08       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CB7C       proc near               ; CODE XREF: tavern_menu+36E↓p

var_12          = word ptr -12h
var_10          = word ptr -10h
var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     [bp+var_4], 1
                mov     bx, [bp+arg_0]  ; CODE XREF: seg002:0AF5↑J
                mov     ax, [bx]
                mov     [bp+var_2], ax
                mov     bx, [bp+arg_2]
                mov     ax, [bx]
                mov     [bp+var_6], ax
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

loc_1CBB1:                              ; CODE XREF: sub_1CB7C+1CA↓j
                cmp     [bp+var_4], 0
                jz      short loc_1CC04
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                mov     di, 56C6h
                mov     [bp+var_10], 4230h

loc_1CBCB:                              ; CODE XREF: sub_1CB7C+83↓j
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
                mov     bx, [bp+var_10]
                push    word ptr [bx]
                call    thk_text_put_number_pad
                add     sp, 6
                add     di, 2
                add     [bp+var_10], 2
                inc     si
                cmp     si, 6
                jl      short loc_1CBCB
                mov     [bp+var_C], si

loc_1CC04:                              ; CODE XREF: sub_1CB7C+39↑j
                mov     [bp+var_4], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_A], ax
                cmp     ax, 41h ; 'A'
                jnb     short loc_1CC1E
                jmp     loc_1CCD8
; ---------------------------------------------------------------------------

loc_1CC1E:                              ; CODE XREF: sub_1CB7C+9D↑j
                cmp     ax, 46h ; 'F'
                jbe     short loc_1CC26
                jmp     loc_1CCD8
; ---------------------------------------------------------------------------

loc_1CC26:                              ; CODE XREF: sub_1CB7C+A5↑j
                mov     bx, [bp+var_2]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CC34
                mov     ax, 4
                jmp     short loc_1CC54
; ---------------------------------------------------------------------------

loc_1CC34:                              ; CODE XREF: sub_1CB7C+B1↑j
                sub     [bp+var_A], 41h ; 'A'
                mov     bx, [bp+var_A]
                shl     bx, 1
                mov     ax, [bx+4230h]
                cwd
                push    dx
                push    ax
                push    [bp+var_2]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CC5E
                mov     ax, 2

loc_1CC54:                              ; CODE XREF: sub_1CB7C+B6↑j
                                        ; sub_1CB7C+154↓j ...
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1CD40
; ---------------------------------------------------------------------------

loc_1CC5E:                              ; CODE XREF: sub_1CB7C+D3↑j
                mov     ax, [bp+var_A]
                shl     ax, 1
                mov     [bp+var_12], ax
                mov     bx, ax
                mov     si, ax
                mov     ax, [si+423Ch]
                cmp     [bx+5772h], ax
                jl      short loc_1CC90
                push    [bp+var_2]
                push    [bp+var_A]
                call    loc_1C94E
                add     sp, 4
                mov     bx, [bp+var_2]
                cmp     byte ptr [bx+6Eh], 2
                jb      short loc_1CC99
                sub     byte ptr [bx+6Eh], 2
                jmp     short loc_1CC99
; ---------------------------------------------------------------------------
                align 2

loc_1CC90:                              ; CODE XREF: sub_1CB7C+F6↑j
                mov     bx, [bp+var_A]
                shl     bx, 1
                inc     word ptr [bx+5772h]

loc_1CC99:                              ; CODE XREF: sub_1CB7C+10B↑j
                                        ; sub_1CB7C+111↑j
                mov     bx, [bp+var_2]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_res_354A
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
                push    [bp+var_2]
                call    thk_char_reset_current_stats
                add     sp, 2
                mov     bx, [bp+var_2]
                or      byte ptr [bx+26h], 8
                mov     ax, 8
                jmp     short loc_1CC54
; ---------------------------------------------------------------------------

loc_1CCD2:                              ; CODE XREF: sub_1CB7C+13F↑j
                mov     ax, 6
                jmp     loc_1CC54
; ---------------------------------------------------------------------------

loc_1CCD8:                              ; CODE XREF: sub_1CB7C+9F↑j
                                        ; sub_1CB7C+A7↑j
                mov     [bp+var_4], 0
                cmp     [bp+var_A], 47h ; 'G'
                jnz     short loc_1CCFC
                push    [bp+var_6]
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_2]
                call    tavern_helper_c
                add     sp, 6
                jmp     short loc_1CD40
; ---------------------------------------------------------------------------
                align 2

loc_1CCFC:                              ; CODE XREF: sub_1CB7C+165↑j
                cmp     [bp+var_A], 1Bh
                jz      short loc_1CD40
                mov     ax, [bp+var_A]
                sub     ax, 31h ; '1'
                mov     [bp+var_8], ax
                or      ax, ax
                jl      short loc_1CD40
                mov     ax, g_party_size
                cmp     [bp+var_8], ax
                jge     short loc_1CD40
                mov     ax, [bp+var_6]
                cmp     [bp+var_8], ax
                jz      short loc_1CD40
                mov     bx, [bp+var_8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CD40
                mov     ax, [bp+var_8]
                mov     [bp+var_6], ax
                call    sub_1D13C
                push    [bp+var_8]
                call    sub_1D0BA
                add     sp, 2
                mov     [bp+var_2], ax

loc_1CD40:                              ; CODE XREF: sub_1CB7C+DF↑j
                                        ; sub_1CB7C+17D↑j ...
                cmp     [bp+var_A], 1Bh
                jz      short loc_1CD49
                jmp     loc_1CBB1
; ---------------------------------------------------------------------------

loc_1CD49:                              ; CODE XREF: sub_1CB7C+1C8↑j
                mov     bx, [bp+arg_2]
                mov     ax, [bp+var_6]
                mov     [bx], ax
                mov     bx, [bp+arg_0]
                mov     ax, [bp+var_2]
                mov     [bx], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CB7C       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CD60       proc near               ; CODE XREF: tavern_menu+37E↓p

var_C           = word ptr -0Ch
var_A           = word ptr -0Ah
var_8           = word ptr -8
var_6           = word ptr -6
var_4           = word ptr -4
var_2           = word ptr -2
arg_0           = word ptr  4
arg_2           = word ptr  6

                push    bp
                mov     bp, sp
                sub     sp, 12h
                push    di
                push    si
                mov     [bp+var_2], 1
                mov     bx, [bp+arg_0]
                mov     ax, [bx]
                mov     [bp+var_4], ax
                mov     bx, [bp+arg_2]
                mov     ax, [bx]
                mov     [bp+var_6], ax
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

loc_1CD95:                              ; CODE XREF: sub_1CD60+1FB↓j
                cmp     [bp+var_2], 0
                jnz     short loc_1CD9E
                jmp     loc_1CE47
; ---------------------------------------------------------------------------

loc_1CD9E:                              ; CODE XREF: sub_1CD60+39↑j
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                sub     si, si
                sub     di, di

loc_1CDAC:                              ; CODE XREF: sub_1CD60+A3↓j
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
                mov     [bp+var_C], si
                mov     [bp+var_C], 3
                mov     di, 12h
                sub     si, si

loc_1CE12:                              ; CODE XREF: sub_1CD60+E5↓j
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

loc_1CE47:                              ; CODE XREF: sub_1CD60+3B↑j
                mov     [bp+var_2], 1
                call    thk_monster_anim_step
                push    ax
                call    thk_res_00E8
                add     sp, 2
                mov     [bp+var_A], ax
                cmp     ax, 41h ; 'A'
                jnb     short loc_1CE61
                jmp     loc_1CEF2
; ---------------------------------------------------------------------------

loc_1CE61:                              ; CODE XREF: sub_1CD60+FC↑j
                cmp     ax, 43h ; 'C'
                jbe     short loc_1CE69
                jmp     loc_1CEF2
; ---------------------------------------------------------------------------

loc_1CE69:                              ; CODE XREF: sub_1CD60+104↑j
                mov     bx, [bp+var_4]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CE78
                mov     ax, 4
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------
                align 2

loc_1CE78:                              ; CODE XREF: sub_1CD60+110↑j
                sub     [bp+var_A], 41h ; 'A'
                mov     al, g_map_id
                sub     ah, ah
                mov     bx, ax
                shl     bx, 1
                add     bx, ax
                shl     bx, 1
                mov     si, [bp+var_A]
                shl     si, 1
                mov     ax, [bx+si+4208h]
                cwd
                push    dx
                push    ax
                push    [bp+var_4]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CEB0
                mov     ax, 2

loc_1CEA5:                              ; CODE XREF: sub_1CD60+115↑j
                                        ; sub_1CD60+17E↓j ...
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1CF55
; ---------------------------------------------------------------------------
                align 2

loc_1CEB0:                              ; CODE XREF: sub_1CD60+140↑j
                mov     bx, [bp+var_4]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_res_354A
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
                mov     bx, [bp+var_4]
                or      byte ptr [bx+26h], 4
                mov     ax, 0Ah
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------

loc_1CEE0:                              ; CODE XREF: sub_1CD60+172↑j
                push    [bp+var_A]
                push    [bp+var_4]
                call    tavern_helper_a
                add     sp, 4
                mov     ax, 0Ch
                jmp     short loc_1CEA5
; ---------------------------------------------------------------------------
                align 2

loc_1CEF2:                              ; CODE XREF: sub_1CD60+FE↑j
                                        ; sub_1CD60+106↑j
                mov     [bp+var_2], 0
                cmp     [bp+var_A], 47h ; 'G'
                jnz     short loc_1CF16
                push    [bp+var_6]
                call    thk_res_6532
                add     sp, 2
                sub     ax, ax
                push    ax
                push    ax
                push    [bp+var_4]
                call    tavern_helper_c
                add     sp, 6
                jmp     short loc_1CF55
; ---------------------------------------------------------------------------
                align 2

loc_1CF16:                              ; CODE XREF: sub_1CD60+19B↑j
                cmp     [bp+var_A], 1Bh
                jz      short loc_1CF55
                mov     ax, [bp+var_A]
                sub     ax, 31h ; '1'
                mov     [bp+var_8], ax
                or      ax, ax
                jl      short loc_1CF55
                mov     ax, g_party_size

loc_1CF2C:                              ; CODE XREF: seg002:050D↑J
                cmp     [bp+var_8], ax
                jge     short loc_1CF55
                mov     ax, [bp+var_6]
                cmp     [bp+var_8], ax
                jz      short loc_1CF55
                mov     bx, [bp+var_8]
                shl     bx, 1
                cmp     word ptr [bx+416h], 18h
                jge     short loc_1CF55
                mov     ax, [bp+var_8]
                mov     [bp+var_6], ax
                push    ax
                call    sub_1D0BA
                add     sp, 2
                mov     [bp+var_4], ax

loc_1CF55:                              ; CODE XREF: sub_1CD60+14C↑j
                                        ; sub_1CD60+1B3↑j ...
                cmp     [bp+var_A], 1Bh
                jz      short loc_1CF5E
                jmp     loc_1CD95       ; CODE XREF: seg002:0AE9↑J
; ---------------------------------------------------------------------------

loc_1CF5E:                              ; CODE XREF: sub_1CD60+1F9↑j
                mov     bx, [bp+arg_2]
                mov     ax, [bp+var_6]
                mov     [bx], ax
                mov     bx, [bp+arg_0]
                mov     ax, [bp+var_4]
                mov     [bx], ax
                pop     si
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1CD60       endp


; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1CF74       proc near               ; CODE XREF: tavern_menu+387↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1CF92

loc_1CF84:                              ; CODE XREF: seg002:062D↑J
                mov     ax, 4

loc_1CF87:                              ; CODE XREF: sub_1CF74+34↓j
                                        ; sub_1CF74+5C↓j
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     loc_1D032
; ---------------------------------------------------------------------------
                align 2

loc_1CF92:                              ; CODE XREF: sub_1CF74+E↑j
                mov     ax, 1
                cwd
                push    dx
                push    ax
                push    [bp+arg_0]
                call    tavern_helper_c
                add     sp, 6
                or      ax, ax
                jnz     short loc_1CFAA
                mov     ax, 2
                jmp     short loc_1CF87
; ---------------------------------------------------------------------------

loc_1CFAA:                              ; CODE XREF: sub_1CF74+2F↑j
                mov     bx, [bp+arg_0]
                mov     al, [bx+73h]
                sub     ah, ah
                push    ax
                call    thk_res_354A
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

loc_1CFD2:                              ; CODE XREF: sub_1CF74+58↑j
                call    sub_1CA46
                shl     ax, 1
                mov     [bp+var_2], ax
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
                mov     ax, [bp+var_2]
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

loc_1D032:                              ; CODE XREF: sub_1CF74+1A↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1CF74       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D038       proc near               ; CODE XREF: tavern_menu+38F↓p

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    si
                mov     bx, [bp+arg_0]
                cmp     byte ptr [bx+26h], 0
                jz      short loc_1D054
                mov     ax, 4
                push    ax
                call    tavern_helper_b
                add     sp, 2
                jmp     short loc_1D0B4
; ---------------------------------------------------------------------------

loc_1D054:                              ; CODE XREF: sub_1D038+E↑j
                call    sub_1CA46
                shl     ax, 1
                mov     [bp+var_2], ax
                mov     ax, 7
                push    ax
                call    thk_res_3FA0
                add     sp, 2
                mov     ax, 13h
                push    ax
                mov     ax, 11h
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+var_2]
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

loc_1D0B4:                              ; CODE XREF: sub_1D038+1A↑j
                pop     si
                mov     sp, bp
                pop     bp
                retn
sub_1D038       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D0BA       proc near               ; CODE XREF: sub_1CB7C+1BB↑p
                                        ; sub_1CD60+1EC↑p ...

var_2           = word ptr -2
arg_0           = word ptr  4

                push    bp
                mov     bp, sp
                sub     sp, 2
                push    [bp+arg_0]      ; CODE XREF: seg002:04DD↑J
                call    thk_char_ptr
                add     sp, 2
                mov     [bp+var_2], ax
                mov     ax, 12h
                push    ax
                mov     ax, 2
                push    ax
                call    thk_text_goto_xy
                add     sp, 4
                mov     ax, [bp+arg_0]
                add     ax, 31h ; '1'
                push    ax
                call    thk_text_putc
                add     sp, 2
                mov     ax, 29h ; ')'
                push    ax
                call    thk_text_putc
                add     sp, 2
                push    [bp+var_2]
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
                mov     bx, [bp+var_2]
                push    word ptr [bx+68h]
                push    word ptr [bx+66h]
                call    thk_text_put_number
                mov     ax, [bp+var_2]
                mov     sp, bp
                pop     bp
                retn
sub_1D0BA       endp

; ---------------------------------------------------------------------------
                align 2

; =============== S U B R O U T I N E =======================================

; Attributes: bp-based frame

sub_1D13C       proc near               ; CODE XREF: sub_1CB7C+1B5↑p
                                        ; tavern_menu+116↓p ...

var_2           = word ptr -2

                push    bp
                mov     bp, sp
                sub     sp, 6
                push    di
                mov     [bp+var_2], 6
                sub     ax, ax
                mov     cx, 6
                mov     di, 5772h
                push    ds
                pop     es
                assume es:DGROUP
                repne stosw
                pop     di
                mov     sp, bp
                pop     bp
                retn
sub_1D13C       endp

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
                call    thk_res_670A
                add     sp, 2
                mov     [bp+var_C], 5
                mov     [bp+var_E], 4
                mov     [bp+var_12], 0

loc_1D185:                              ; CODE XREF: tavern_menu+48↓j
                mov     si, [bp+var_12]
                add     si, 573Eh
                mov     di, 4

loc_1D18F:                              ; CODE XREF: tavern_menu+3E↓j
                call    thk_res_67BC
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
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D1AF
                mov     [bp+var_C], 0Eh
                mov     si, 5722h
                mov     di, 0Eh

loc_1D1C5:                              ; CODE XREF: tavern_menu+74↓j
                call    thk_res_67BC
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
                call    thk_res_67BC
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
                call    thk_res_67BC
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
                call    thk_res_67BC
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
                call    thk_res_67BC
                mov     [si], ax
                add     si, 2
                dec     di
                jnz     short loc_1D25B
                add     [bp+var_16], 0Ch
                cmp     [bp+var_16], 3Ch ; '<'
                jl      short loc_1D251
                call    sub_1D13C
                mov     byte_2294F, 0FDh
                sub     ax, ax
                push    ax
                call    thk_gfx_select_page
                add     sp, 2
                sub     ax, ax
                push    ax
                call    thk_res_3FA0
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
                call    thk_res_34BA
                call    thk_res_5440
                mov     ax, 2
                push    ax
                call    thk_res_3FA0
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
                call    sub_1D0BA
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
                call    thk_res_3FA0
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
                call    thk_res_00E8
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
                call    sub_1D13C
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
                call    sub_1CB08

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
                call    sub_1CB7C

loc_1D4CB:                              ; CODE XREF: tavern_menu+381↓j
                add     sp, 4
                jmp     short loc_1D4B7
; ---------------------------------------------------------------------------

loc_1D4D0:                              ; CODE XREF: tavern_menu+2EA↑j
                lea     ax, [bp+var_6]
                push    ax
                lea     ax, [bp+var_4]
                push    ax
                call    sub_1CD60
                jmp     short loc_1D4CB
; ---------------------------------------------------------------------------
                align 2

loc_1D4DE:                              ; CODE XREF: tavern_menu+2F2↑j
                push    [bp+var_4]
                call    sub_1CF74
                jmp     short loc_1D4B4
; ---------------------------------------------------------------------------

loc_1D4E6:                              ; CODE XREF: tavern_menu+2FA↑j
                push    [bp+var_4]
                call    sub_1D038
                jmp     short loc_1D4B4
; ---------------------------------------------------------------------------

loc_1D4EE:                              ; CODE XREF: tavern_menu+302↑j
                push    [bp+var_6]
                call    thk_res_6532
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
                call    thk_res_35A8

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

