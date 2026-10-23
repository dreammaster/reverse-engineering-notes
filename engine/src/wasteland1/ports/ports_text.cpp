// Register adapters: the text engine and keyboard layer (text.cpp).
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/text.h"

namespace wl {
namespace rc {

PORT(waitForKeypress) { r.cf = waitForKeypress(m); r.a.b.l = m.d8(g::lastKey); }   // 18E90
PORT(waitSpaceOrEnter) { r.cf = waitSpaceOrEnter(m); }                           // 18EAD
PORT(checkKeypressCaseSensitive) { r.a.b.l = checkKeypress(m, 0); }              // 18EF9
PORT(checkKeypress) { r.a.b.l = checkKeypress(m, 0x20); }                        // 18EFE
PORT(pollSpeedKeys) { pollSpeedKeys(m); }                                        // 19004

PORT(writeNumberShort) { writeNumberShort(m, r.a.b.l); }                         // 1769C
PORT(textDecodeOffset) { r.a.b.l = textDecodeOffset(m); r.zf = r.a.b.l == 0; }           // 17BC7
PORT(textDecodeChar) { r.a.b.l = textDecodeChar(m); }                                   // 17B8F
PORT(textDecodeInit) { textDecodeInit(m, r.a.b.l); }                                    // 178B9: al = index
PORT(textFilterQuantity) { r.a.b.l = textFilterQuantity(m, r.a.b.l); }                  // 1A00A
PORT(textFilterSex) { r.a.b.l = textFilterSex(m, r.a.b.l); }                            // 1A045
PORT(textFilterE) { r.a.b.l = textFilterE(m, r.a.b.l); }                                // 1A07E
PORT(textClearLineBuffer) { textClearLineBuffer(m); r.c.x = 0; r.si = 0x8DDC + 25; }    // 19EBE
PORT(writeNumberLeft) { writeNumberLeft(m, r.a.b.l); }                           // 176A2
PORT(writeSpaceNumber) { writeSpaceNumber(m, uint16_t(r.a.b.l | (r.d.b.l << 8))); }   // 176C5
PORT(writeNumber16) { writeNumber16(m, uint16_t(r.a.b.l | (r.d.b.l << 8))); }    // 176D0
PORT(setNumberValueShort) { setNumberValue(m, r.a.b.l); convertNumberToBcd(m); }   // 176A8
PORT(setNumberValueWord) { setNumberValue(m, uint16_t(r.a.b.l | (r.d.b.l << 8))); convertNumberToBcd(m); }   // 176B6

PORT(setWriteCharacterFn2) { setWriteCharacterFn2(m); }                          // 17857
PORT(setWriteCharacterFn) { setWriteCharacterFn(m); }                            // 1785E
PORT(writeString) { writeString(m); }                                            // 1786E
PORT(writeString2) { writeString2(m); }                                          // 17877
PORT(printMapMessage) { printMapMessage(m, r.a.b.l); }                           // 178A0
PORT(writeEncodedText) { writeEncodedText(m, r.a.b.l); }                         // 178A3
PORT(writeMemberName) { writeMemberName(m); }                                    // 171B9

PORT(saveRectBounds) { saveRectBounds(m); }                                      // 19083
PORT(restoreRectBounds) { restoreRectBounds(m); }                                // 19060
PORT(drawInfoWindow) { r.es = 0xA000; drawInfoWindow(m); }                                      // 19727
PORT(drawRosterWindow) { r.es = 0xA000; drawRosterWindow(m); }                                  // 19770
PORT(clearTextArea) { r.es = 0xA000; clearTextArea(m); }                                        // 197AE
PORT(drawBox) { r.es = 0xA000; drawBox(m, r.d.b.l, r.b.b.l); }                                  // 19812
PORT(setInverseText) { r.a.b.l = 0xFF; m.d8(g::inverseText) = 0xFF; }            // 19E2A
PORT(clearInverseText) { r.a.b.l = 0; m.d8(g::inverseText) = 0; }                // 19E30
PORT(scrollLine) { r.es = 0; scrollLine(m); }                                              // 1A0C5
PORT(scrollWindowUp) { r.es = 0xA000; scrollWindowUp(m); }                                      // 107D8
PORT(eraseToRight) { r.es = 0xA000; eraseToRight(m); }                                          // 106C9

} // namespace rc
} // namespace wl
