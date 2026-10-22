#include "wasteland1/text.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/rules.h"
#include "wasteland1/ui.h"
#include <algorithm>

namespace wl {

void scrollLine(Machine &m);
void eraseToRight(Machine &m);
void writeMemberName(Machine &m);
void writeNumberLeft(Machine &m, uint8_t value);
void drawInfoWindow(Machine &m);
void morePrompt(Machine &m);
void drawSpeedSlider(Machine &m);

namespace {

// BIOS timer tick counter (0040:006C), read by the scrolling code
uint8_t biosTick(const Machine &m) { return m.mem[0x46C]; }

uint8_t lineAt(const Machine &m, int x) { return m.d8(uint16_t(g::lineBuffer + x)); }
void setLine(Machine &m, int x, uint8_t v) { m.d8(uint16_t(g::lineBuffer + x)) = v; }

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// keyboard

bool biosKeyPending(Machine &m, uint16_t &key) {
	if (m.keys.empty()) {
		if (m.onIdle) m.onIdle();   // like int 16h AH=1 in the recompiled runtime: an empty poll lets the host run
		return false;
	}
	key = m.keys.front();
	return true;
}

uint16_t biosKeyRead(Machine &m) {
	while (m.keys.empty()) {
		if (!m.onIdle) return 0;
		m.onIdle();
	}
	const uint16_t k = m.keys.front();
	m.keys.pop_front();
	return k;
}

void pollSpeedKeys(Machine &m) {   // 19004
	if (m.d8(g::textSpeedFlag) == 0) return;
	m.setD16(g::buttonMask1, 0x300);
	m.setD16(g::buttonMask2, 0);
	hintOverlay(m);
	uint16_t key;
	if (!biosKeyPending(m, key)) return;
	const uint8_t scan = uint8_t(key >> 8);
	uint8_t &speed = m.d8(uint16_t(g::saved2 + 0x0D));
	if (scan == 0x50) { if (speed != 0) { --speed; drawSpeedSlider(m); } }         // down arrow: slower (19038)
	else if (scan == 0x48) { if (speed != 8) { ++speed; drawSpeedSlider(m); } }    // up arrow: faster (1904A)
	biosKeyRead(m);
}

uint8_t checkKeypress(Machine &m, uint8_t caseDelta) {   // 18EFE
	m.d8(g::keyCaseDelta) = caseDelta;
	const uint8_t tick = m.onPortraitTick ? m.onPortraitTick() : 0;   // 190F8
	hintOverlay(m);
	Rules(m).rngByte(tick);                              // the generator is stirred on every poll with whatever 190F8 leaves in al
	uint8_t al = 0;
	bool haveKey = false;
	if (m.d8(g::macroPlaySlot) != 0) {   // playing back a recorded macro
		pollSpeedKeys(m);
		const uint16_t base = uint16_t(0xC062 + (uint16_t(m.d8(g::macroPlaySlot) - 1) << 8));
		uint8_t pos = m.d8(g::macroPos);
		al = m.d8(uint16_t(base + pos));
		m.d8(g::macroPos) = uint8_t(pos + 1);
		if (al != 0) haveKey = true;
		else m.d8(g::macroPlaySlot) = 0;
	}
	if (!haveKey) {
		pollSpeedKeys(m);   // initMouse? starts with sub_19004
		uint16_t key;
		if (!biosKeyPending(m, key)) al = 0;
		else {
			biosKeyRead(m);
			al = uint8_t(key);
			if (al == 0) al = uint8_t((key >> 8) | 0x80);
		}
	}
	// F1..F10 start a macro, Ctrl+F1..F10 record one
	if (al >= 0xBB && al <= 0xC4) {
		if (m.d8(g::macroSlot) != 0) al = 0;
		else { m.d8(g::macroPlaySlot) = uint8_t(al - 0xBA); m.d8(g::macroPos) = 0; }
	}
	if (al >= 0xDE && al <= 0xE7) {
		const uint8_t slot = uint8_t(al - 0xDD);
		if (m.d8(g::macroSlot) == 0) { m.d8(g::macroSlot) = slot; al = 0; m.d8(g::macroPos) = 0; }
		else if (slot == m.d8(g::macroSlot)) {
			const uint16_t base = uint16_t(0xC062 + (uint16_t(slot - 1) << 8));
			const uint8_t pos = m.d8(g::macroPos);
			m.d8(g::macroSlot) = 0;
			m.d8(uint16_t(base + pos)) = 0;
			al = 0;
		} else al = 0;
	}
	if (al != 0 && m.d8(g::macroSlot) != 0) {   // record the key
		const uint16_t base = uint16_t(0xC062 + (uint16_t(m.d8(g::macroSlot) - 1) << 8));
		const uint8_t pos = m.d8(g::macroPos);
		if (pos != 0xFF) { m.d8(uint16_t(base + pos)) = al; m.d8(g::macroPos) = uint8_t(pos + 1); }
	}
	m.d8(g::moreCounter) = 0;
	m.d8(uint16_t(0x4704 + 1)) = 0;   // high byte of word_21524
	if (al >= 'a' && al < '{') al = uint8_t(al - m.d8(g::keyCaseDelta));
	m.d8(g::lastKey) = al;
	return al;
}

bool waitForKeypress(Machine &m) {   // 18E90
	const uint8_t saved = m.d8(g::textSpeedFlag);
	m.d8(g::textSpeedFlag) = 0;
	uint8_t al;
	do {
		al = checkKeypress(m);
		if (al == 0 && m.onIdle) m.onIdle();
	} while (al == 0);
	m.d8(g::textSpeedFlag) = saved;
	return al == 27;
}

bool waitSpaceOrEnter(Machine &m) {   // 18EAD
	const uint16_t s1 = m.d16(g::buttonMask1), s2 = m.d16(g::buttonMask2);
	m.setD16(g::buttonMask1, 0);
	m.setD16(g::buttonMask2, 1);
	const uint8_t saved = m.d8(g::textSpeedFlag);
	m.d8(g::textSpeedFlag) = 0;
	bool carry;
	for (;;) {
		const bool esc = waitForKeypress(m);
		const uint8_t k = m.d8(g::lastKey);
		if (esc || k == 0x20) { carry = true; break; }
		if (k == 0x0D) { carry = false; break; }
	}
	m.d8(g::textSpeedFlag) = saved;
	m.setD16(g::buttonMask2, s2);
	m.setD16(g::buttonMask1, s1);
	hintOverlay(m);
	return carry;
}

// ---------------------------------------------------------------------------------------------------------------------
// hot key hints (the key labels drawn around the window frame)

namespace {

void eraseSpeedSlider(Machine &m) {   // 18B1B
	m.setD16(g::mouseCursorOn, 0);
	m.d8(g::stringXPos) = 0x17;
	m.d8(g::stringYPos) = 0x0D;
	for (int i = 0; i < 8; ++i) writeColorCharacter(m, 0x12);
}

void drawHotspot(Machine &m, uint8_t index, bool erase) {   // 18B78
	const uint16_t entry = uint16_t(g::hotspotLabels + (index - 4) * 6);
	m.d8(g::stringXPos) = m.d8(entry);
	m.d8(g::stringYPos) = m.d8(uint16_t(entry + 1));
	uint8_t len = m.d8(uint16_t(entry + 2));
	uint16_t si = m.d16(uint16_t(entry + 4));
	if (!erase) {
		if (index >= 8) {
			for (uint8_t c; (c = m.d8(si)) != 0; ++si) writeColorCharacter(m, c);
		} else {
			for (uint8_t c; (c = m.d8(si)) != 0; ++si) {
				writeColorCharacter(m, c);
				++m.d8(g::stringYPos);
				--m.d8(g::stringXPos);
			}
		}
	} else if (index >= 8) {
		do { writeColorCharacter(m, 0x12); } while (--len != 0);
	} else {
		do {
			writeColorCharacter(m, 0x13);
			++m.d8(g::stringYPos);
			--m.d8(g::stringXPos);
		} while (--len != 0);
	}
}

void hintPass(Machine &m) {   // 18A4F
	uint16_t cur = m.d16(g::buttonMask1);
	uint16_t changes = uint16_t(cur ^ m.d16(g::prevMask1));
	uint16_t bit = 0x10;
	int index = 4;
	m.setD16(g::hintMask, cur);
	for (;;) {
		if (changes & bit) {
			const bool nowSet = (m.d16(g::hintMask) & bit) != 0;
			if (nowSet) {
				if (m.d8(g::hintPass) != 0) {   // second pass draws the labels that appeared
					if (m.d8(g::hotLabelCurrent) == index) m.d8(g::hotLabelCurrent) = 0;
					drawHotspot(m, uint8_t(index), false);
					if (bit == 0x100) drawSpeedSlider(m);
				}
			} else if (m.d8(g::hintPass) == 0) {   // first pass erases the labels that vanished
				if (m.d8(g::hotLabelCurrent) == index) m.d8(g::hotLabelCurrent) = 0;
				drawHotspot(m, uint8_t(index), true);
				if (bit == 0x100) eraseSpeedSlider(m);
			}
		}
		bit = uint16_t(bit << 1);
		++index;
		if (index == 0x10) {
			bit = 1;
			m.setD16(g::hintMask, m.d16(g::buttonMask2));
			changes = uint16_t(m.d16(g::buttonMask2) ^ m.d16(g::prevMask2));
		} else if (index < 0x10) {
			changes = uint16_t(m.d16(g::buttonMask1) ^ m.d16(g::prevMask1));
		} else {
			changes = uint16_t(m.d16(g::buttonMask2) ^ m.d16(g::prevMask2));
		}
		if (index == 0x15) break;
	}
}

} // namespace

void drawSpeedSlider(Machine &m) {   // 18AC4: the text speed bar between the SLOW and FAST labels
	m.setD16(g::mouseCursorOn, 0);
	m.d8(g::stringXPos) = 0x17;
	m.d8(g::stringYPos) = 0x0D;
	const uint8_t start = m.d8(g::stringXPos);
	const uint8_t end = uint8_t(start + 8);
	m.d8(0x8DD8) = end;                                                 // IDB byte_25BF8
	const uint8_t filledTo = uint8_t(start + m.d8(uint16_t(g::saved2 + 0x0D)));
	m.d8(0x8DD7) = filledTo;                                            // IDB _buttonsXStart
	if (start != filledTo) {
		do { writeColorCharacter(m, 0x14); } while (m.d8(g::stringXPos) != filledTo);
		if (m.d8(g::stringXPos) == end) return;
	}
	do { writeColorCharacter(m, 0x15); } while (m.d8(g::stringXPos) != end);
}

void hintOverlay(Machine &m) {   // 189B1
	if (m.d8(g::macroSlot) == 0) {
		if (m.d8(g::hintsDrawn) == 0) {   // clear the recording indicator
			m.d8(g::hintsDrawn) = 1;
			m.d8(g::stringXPos) = 2;
			m.d8(g::stringYPos) = 0;
			do { writeColorCharacter(m, 0x12); } while (m.d8(g::stringXPos) != 0x0C);
		}
	} else if (m.d8(g::hintsDrawn) != 0) {   // show "(F<n>)" while recording
		m.d8(g::hintsDrawn) = 0;
		m.d8(g::stringXPos) = 2;
		m.d8(g::stringYPos) = 0;
		writeColorCharacter(m, 0x0A);
		uint8_t glyph = 0xA6;
		do { writeColorCharacter(m, glyph++); } while (m.d8(g::stringXPos) != 9);
		if (m.d8(g::macroSlot) == 0x0A) { writeColorCharacter(m, 1); writeColorCharacter(m, 0); }
		else { writeColorCharacter(m, 0); writeColorCharacter(m, m.d8(g::macroSlot)); }
		writeColorCharacter(m, 0x0B);
	}
	m.d8(g::hintPass) = 0;
	hintPass(m);
	m.d8(g::hintPass) = 1;
	hintPass(m);
	m.setD16(g::prevMask1, m.d16(g::buttonMask1));
	m.setD16(g::prevMask2, m.d16(g::buttonMask2));
}

} // namespace wl

// ---------------------------------------------------------------------------------------------------------------------
// numbers

namespace wl {

void setNumberValue(Machine &m, uint32_t v) {
	m.d8(g::numberValue) = uint8_t(v);
	m.d8(uint16_t(g::numberValue + 1)) = uint8_t(v >> 8);
	m.d8(uint16_t(g::numberValue + 2)) = uint8_t(v >> 16);
}

void convertNumberToBcd(Machine &m) {   // 18DCE
	for (uint16_t i = 0; i < 4; ++i) m.d8(uint16_t(g::numberBcd + i)) = 0;
	for (int step = 7; step >= 0; --step) {
		for (;;) {
			const uint32_t value = m.d8(g::numberValue) | (m.d8(uint16_t(g::numberValue + 1)) << 8) | (m.d8(uint16_t(g::numberValue + 2)) << 16);
			const uint32_t pw = m.d8(uint16_t(0xC020 + step)) | (m.d8(uint16_t(0xC028 + step)) << 8) | (m.d8(uint16_t(0xC030 + step)) << 16);
			if (value < pw) break;
			setNumberValue(m, value - pw);
			unsigned carry = 0;
			for (uint16_t k = 0; k < 4; ++k) {
				const unsigned sum = m.d8(uint16_t(g::numberBcd + k)) + m.d8(uint16_t(0xC038 + 8 * k + step)) + carry;
				m.d8(uint16_t(g::numberBcd + k)) = uint8_t(sum);
				carry = sum > 0xFF;
			}
		}
	}
}

} // namespace wl


// ---------------------------------------------------------------------------------------------------------------------
// window frames

namespace wl {

namespace {

struct RectState { uint8_t textX, textY, left, right, top, bottom; };
std::vector<RectState> &rectStack() { static std::vector<RectState> s; return s; }

void boxBottomRun(Machine &m) {   // 19886: horizontal bar until stringXPos == byte_29AFD
	do { writeColorCharacter(m, 0x12); } while (m.d8(g::stringXPos) != m.d8(0xCCDD));
}

} // namespace

void saveRectBounds(Machine &m) {   // 19083
	rectStack().push_back({m.d8(g::rectTextX), m.d8(g::rectTextY), m.d8(g::rectLeftCol), m.d8(g::rectRightCol), m.d8(g::rectTopLine), m.d8(g::rectBottomLine)});
}

void restoreRectBounds(Machine &m) {   // 19060
	if (rectStack().empty()) return;
	const RectState r = rectStack().back();
	rectStack().pop_back();
	m.d8(g::rectBottomLine) = r.bottom;
	m.d8(g::rectTopLine) = r.top;
	m.d8(g::rectRightCol) = r.right;
	m.d8(g::rectLeftCol) = r.left;
	m.d8(g::rectTextY) = r.textY;
	m.d8(g::rectTextX) = r.textX;
}

// 19812: single line frame from (stringXPos, stringYPos) to column `right` / row `bottom`
void drawBox(Machine &m, uint8_t right, uint8_t bottom) {
	m.d8(0xCCDB) = 0;              // IDB byte_29AFB
	m.d8(g::rectRightCol) = 0x27;
	m.d8(g::rectBottomLine) = 0x70;
	m.d8(0xCCDD) = right;
	m.d8(0xCCDE) = bottom;
	m.d8(0xCCDC) = m.d8(g::stringXPos);
	if (m.d8(g::graphicsBusy) != 0) m.d8(g::stringYPos) = 0x0E;
	else {
		writeColorCharacter(m, 0x0E);
		boxBottomRun(m);
		writeColorCharacter(m, 0x0F);
		++m.d8(g::stringYPos);
	}
	do {
		m.d8(g::stringXPos) = m.d8(0xCCDC);
		writeColorCharacter(m, 0x0D);
		m.d8(g::stringXPos) = m.d8(0xCCDD);
		writeColorCharacter(m, 0x13);
		++m.d8(g::stringYPos);
	} while (m.d8(g::stringYPos) != m.d8(0xCCDE));
	m.d8(g::stringXPos) = m.d8(0xCCDC);
	writeColorCharacter(m, 0x10);
	boxBottomRun(m);
	writeColorCharacter(m, 0x11);
}

void clearTextArea(Machine &m) {   // 197AE
	m.d8(g::rectTopLine) = 8;
	m.d8(g::rectBottomLine) = 0x68;
	clearRect(m);
}

void drawInfoWindow(Machine &m) {   // 19727: the frame around the top right information window
	m.d8(g::rectRightCol) = 39;
	m.d8(g::rectBottomLine) = 112;
	m.d8(g::rectLeftCol) = 14;
	if (m.d8(g::rosterShown) == 0) {
		m.setD16(g::prevMask1, uint16_t(m.d16(g::prevMask1) & 0x70CF));
		m.setD16(g::prevMask2, uint16_t(m.d16(g::prevMask2) & 0xFFE0));
		m.d8(g::rosterShown) = 1;
		m.d8(g::stringXPos) = 14;
		m.d8(g::stringYPos) = 0;
		drawBox(m, 39, 13);
	}
	++m.d8(g::rectLeftCol);
	--m.d8(g::rectRightCol);
	clearTextArea(m);
}

void drawRosterWindow(Machine &m) {   // 19770: frame of the left (portrait) window
	saveRectBounds(m);
	m.d8(g::hintsDrawn) = 1;
	m.d8(g::rectLeftCol) = 0;
	m.d8(g::stringXPos) = 0;
	m.d8(g::stringYPos) = 0;
	drawBox(m, 0x0D, 0x0D);
	++m.d8(g::rectLeftCol);
	m.d8(g::rectRightCol) = 0x0C;
	m.d8(g::rosterAnimOn) = 0;
	m.d8(g::rosterAnim) = 1;
	clearTextArea(m);
	restoreRectBounds(m);
}

} // namespace wl

// ---------------------------------------------------------------------------------------------------------------------
// character writers

namespace wl {

void setWriteCharacterFn(Machine &m) { m.setD16(g::writeCharacterFn, 0x9E53); }    // writeCharacter
void setWriteCharacterFn2(Machine &m) { m.setD16(g::writeCharacterFn, 0x9DC3); }   // writeCharacter2

namespace {

// j_writeCharacter: the original jumps through a near pointer; only the two text writers exist outside of printing
void jWriteCharacter(Machine &m, uint8_t ch) {
	if (m.d16(g::writeCharacterFn) == 0x9DC3) writeCharacter2(m, ch);
	else writeCharacter(m, ch);
}

void writeDigit(Machine &m, uint8_t digit) {   // 17709
	uint8_t al = digit;
	if (m.d8(g::numberDigitsLeft) != 1 && m.d8(g::numberLeading) != 0 && al == 0) al = m.d8(g::writeNumberPrefix);
	else {
		m.d8(g::numberLeading) = 0;
		al = uint8_t(al | 0x30);
	}
	if (m.d8(g::tempTextX) >= m.d8(g::numberDigitsLeft) && al != 0) jWriteCharacter(m, al);
	--m.d8(g::numberDigitsLeft);
}

void writeNumberDigits(Machine &m) {   // 176DD
	m.d8(g::numberLeading) = 0xFF;
	m.d8(g::numberDigitsLeft) = 8;
	for (int i = 3; i >= 0; --i) {
		const uint8_t b = m.d8(uint16_t(g::numberBcd + i));
		writeDigit(m, uint8_t(b >> 4));
		writeDigit(m, uint8_t(b & 0x0F));
	}
}

} // namespace

void writeNumber(Machine &m) {   // 17696 / 176D3: numberValue already set; no padding, up to 8 digits
	convertNumberToBcd(m);
	m.d8(g::tempTextX) = 8;
	m.d8(g::writeNumberPrefix) = 0;
	writeNumberDigits(m);
}

void writeNumberShort(Machine &m, uint8_t value) {   // 1769C: space padded to tempTextX columns
	setNumberValue(m, value);
	convertNumberToBcd(m);
	m.d8(g::writeNumberPrefix) = 0x20;
	writeNumberDigits(m);
}

void writeNumberLeft(Machine &m, uint8_t value) {   // 176A2: up to 3 digits, no padding
	setNumberValue(m, value);
	writeNumber(m);
}

void writeNumber16(Machine &m, uint16_t value) {   // 176D0
	setNumberValue(m, value);
	writeNumber(m);
}

void writeSpaceNumber(Machine &m, uint16_t value) {   // 176C5
	setNumberValue(m, value);
	convertNumberToBcd(m);
	m.d8(g::writeNumberPrefix) = 0x20;
	writeNumberDigits(m);
}

namespace {

uint8_t filterQuantity(Machine &m, uint8_t al) {   // 1A00A: text between two 0Ah markers is plural-only
	uint8_t &state = m.d8(g::filterQtyState);
	if (state == 0) {
		if (al == 0x0A) { ++state; return 0; }
		return m.d8(g::itemQuantity) == 1 ? al : 0;
	}
	if (al == 0x0A) { m.d8(g::filterQtyOn) = 0; state = 0; return 0; }
	return m.d8(g::itemQuantity) == 1 ? 0 : al;
}

uint8_t filterSex(Machine &m, uint8_t al) {   // 1A045: text between two 0Ch markers is female-only
	uint8_t &state = m.d8(g::filterSexState);
	const uint8_t sex = m.d8(g::currentCharacterSex);
	if (state == 0) {
		if (al == 0x0C) { ++state; return 0; }
		return sex == 0 ? al : 0;
	}
	if (al == 0x0C) { m.d8(g::filterSexOn) = 0; state = 0; return 0; }
	return sex == 0 ? 0 : al;
}

uint8_t filterE(Machine &m, uint8_t al) {   // 1A07E: three alternatives separated by 0Eh markers, chosen by byte_2153A
	uint8_t &state = m.d8(g::filterEState);
	const uint8_t sel = m.d8(g::itemSexFlag);
	if (state == 0) {
		if (al == 0x0E) { ++state; return 0; }
		return sel == 0 ? al : 0;
	}
	if (state == 1) {
		if (al == 0x0E) { ++state; return 0; }
		return sel == 1 ? al : 0;
	}
	if (al == 0x0E) { m.d8(g::filterEOn) = 0; state = 0; return 0; }
	return sel == 2 ? al : 0;
}

void controlCode(Machine &m, bool buffered, uint8_t code);

} // namespace

void writeCharacter2(Machine &m, uint8_t al) {   // 19DC3
	if (m.d8(g::textSuppressed) == 0) {
		if (m.d8(g::captureNextChar) != 0) { m.d8(uint16_t(g::charCapture + m.d8(g::rectTextY) - 1)) = al; m.d8(g::captureNextChar) = 0; }
		if (m.d8(g::filterSexOn) != 0) al = filterSex(m, al);
		if (m.d8(g::filterEOn) != 0) al = filterE(m, al);
		if (m.d8(g::filterQtyOn) != 0) al = filterQuantity(m, al);
		if (al < 18) controlCode(m, false, al);
		else {
			writeCharacterAt(m, uint8_t(al - 0x20));
			++m.d8(g::rectTextX);
		}
	}
	pollSpeedKeys(m);
}

void writeCharacter(Machine &m, uint8_t al) {   // 19E53
	if (m.d8(g::textSuppressed) == 0) {
		if (m.d8(g::captureNextChar) != 0) { m.d8(uint16_t(g::charCapture + m.d8(g::rectTextY) - 1)) = al; m.d8(g::captureNextChar) = 0; }
		if (m.d8(g::filterSexOn) != 0) al = filterSex(m, al);
		if (m.d8(g::filterEOn) != 0) al = filterE(m, al);
		if (m.d8(g::filterQtyOn) != 0) al = filterQuantity(m, al);
		if (al < 18) controlCode(m, true, al);
		else {
			al = uint8_t(al - 0x20);
			const uint8_t x = m.d8(g::rectTextX);
			setLine(m, x, al);   // 19EE5: the character goes into the line buffer ...
			if (int8_t(x) > int8_t(m.d8(g::rectRightCol))) newline(m);   // ... and the line wraps past the right edge
			else ++m.d8(g::rectTextX);
		}
	}
	pollSpeedKeys(m);
}

void newline(Machine &m) {   // 19EFC
	if (m.d8(g::textSuppressed) != 0) return;
	flushLine(m);
	const uint8_t x = m.d8(g::rectTextX);
	scrollLine(m);
	m.d8(g::rectTextX) = x;
}

void flushLine(Machine &m) {   // 19F12
	int bl = m.d8(g::rectTextX);
	for (;;) {
		if (lineAt(m, bl) == 0 || bl == m.d8(g::rectLeftCol)) break;
		--bl;
		if (bl < 0) break;
	}
	const uint8_t wrap = uint8_t(bl);
	m.d8(g::wrapColumn) = wrap;
	if (m.d8(g::historyRecording) != 0) {   // keep the line for the history (message log) redraw
		uint8_t row = uint8_t(m.d8(g::historyLine) + 1);
		m.d8(g::historyLine) = row;
		--row;
		uint16_t off = uint16_t(0x8CE0 + uint16_t(row) * 40);
		for (uint8_t x = m.d8(g::rectLeftCol); x != wrap; ++x) m.mem[kGfxLinear + uint32_t(off) + x] = lineAt(m, x);
		off = uint16_t(off + 0x28);
		if (off >= 0xB4DF) off = uint16_t(off - 0x2800);
		for (int si = 1; si != 0x27; ++si) m.mem[kGfxLinear + uint32_t(off) + uint32_t(si)] = 0;
	}
	for (uint8_t x = m.d8(g::rectLeftCol); x != wrap; ++x) {
		m.d8(g::rectTextX) = x;
		writeCharacterAt(m, lineAt(m, x));
	}
	uint8_t dl = m.d8(g::rectLeftCol);
	uint8_t bl2 = wrap;
	const uint8_t right = m.d8(g::rectRightCol);
	if (bl2 <= right) {   // move the word that did not fit to the start of the buffer
		++bl2;
		for (;;) {
			const uint8_t al = lineAt(m, bl2);
			if (al == 0) break;
			setLine(m, dl, al);
			if (bl2 > right) { ++dl; break; }
			++dl; ++bl2;
			if (bl2 == 0) { ++dl; break; }
		}
	}
	m.d8(g::rectTextX) = dl;
	for (; dl != 0x33; ++dl) setLine(m, dl, 0);
}

namespace {

void clearLineBufferColumn(Machine &m) {   // 19EBE
	for (int i = 0; i < 25; ++i) m.d8(uint16_t(g::charCapture + i)) = 0;
}

void controlCode(Machine &m, bool buffered, uint8_t code) {
	// the original calls through _charSpecialFn (ds:4661), which keeps the handler address of the last control code
	m.setD16(0x4661, m.d16(uint16_t((buffered ? g::textSpecialFn2 : g::textSpecialFn1) + 2 * code)));
	switch (code) {
	case 0: pollSpeedKeys(m); break;
	case 1: m.d8(g::inverseText) = 0xFF; break;
	case 2: m.d8(g::inverseText) = 0; break;
	case 3: m.d8(g::inverseText) = 0; eraseToRight(m); break;
	case 4: clearRect(m); break;
	case 5: case 6: waitSpaceOrEnter(m); break;
	case 7: drawInfoWindow(m); break;
	case 8: if (buffered) flushLine(m); else pollSpeedKeys(m); break;
	case 9: {   // skip the number of columns given by the next string byte
		const uint16_t p = m.d16(g::textString);
		m.d8(g::rectTextX) = uint8_t(m.d8(g::rectTextX) + m.d8(uint16_t(p + 1)));
		m.setD16(g::textString, uint16_t(p + 1));
		break;
	}
	case 10: m.d8(g::filterQtyOn) = 1; break;
	case 11: {   // member name
		const uint16_t keep = m.d16(g::textString);
		writeMemberName(m);
		m.setD16(g::textString, keep);
		break;
	}
	case 12: m.d8(g::filterSexOn) = 1; break;
	case 13: if (buffered) newline(m); else scrollLine(m); break;
	case 14: m.d8(g::filterEOn) = 1; break;
	case 15: writeNumberLeft(m, m.d8(g::itemQuantity) == 0 ? 1 : m.d8(g::itemQuantity)); break;
	case 16: m.d8(g::captureNextChar) = 1; break;
	case 17: clearLineBufferColumn(m); break;
	default: break;
	}
}

} // namespace

} // namespace wl

// ---------------------------------------------------------------------------------------------------------------------
// scrolling

namespace wl {

namespace {

void selectPlaneForRead(Machine &m) {   // out 3C4h,2 / 3CEh,4 / map mask F / read plane 0
	m.ega.seqIdx = 2; m.ega.mapMask = 0x0F;
	m.ega.gcIdx = 4; m.ega.gc[4] = 0;
}

uint16_t lineOffsetOf(const Machine &m, int line) { return m.d16(uint16_t(g::gfxLineOffsets + 2 * line)); }

void clearScrollRow(Machine &m, uint8_t line) {   // 10879
	const uint16_t di = lineOffsetOf(m, line);
	for (int bl = m.d8(g::rectRightCol);;) {
		m.ega.write(0xA0000u + uint16_t(di + bl), 0);
		if (--bl < 0 || bl < m.d8(g::rectLeftCol)) break;
	}
}

} // namespace

void eraseToRight(Machine &m) {   // 106C9 with the AND opcode: monochrome copy of plane 0 ANDed with inverseText
	selectPlaneForRead(m);
	const int x0 = m.d8(g::rectTextX), right = m.d8(g::rectRightCol);
	if (x0 >= 0x28 || x0 > right) return;
	uint16_t row = lineOffsetOf(m, m.d8(g::rectTextY) * 8);
	for (int line = 0; line < 8; ++line) {
		for (int x = x0; x <= right; ++x) {
			const uint32_t a = 0xA0000u + uint16_t(row + x);
			const uint8_t v = uint8_t(m.ega.read(a) & m.d8(g::inverseText));
			m.ega.write(a, v);
		}
		row = uint16_t(row + 0x28);
	}
}

// 107D8: scrolls the rectangle up by `step` pixel lines and blanks the bottom lines
void scrollWindowUp(Machine &m) {
	uint8_t dst = m.d8(g::rectTopLine);
	uint8_t step = m.d8(uint16_t(0xCD9B + m.d8(uint16_t(g::saved2 + 0x0D))));
	uint8_t src = uint8_t(dst + step);
	m.d8(0x8FAF) = dst;            // IDB byte_25DCF
	m.d8(0x8FAE) = src;            // IDB byte_25DCE
	m.d8(0x8FB0) = step;           // IDB byte_25DD0
	selectPlaneForRead(m);
	do {
		const uint16_t di = lineOffsetOf(m, m.d8(0x8FAF)), si = lineOffsetOf(m, m.d8(0x8FAE));
		for (int bl = m.d8(g::rectRightCol);;) {
			m.ega.write(0xA0000u + uint16_t(di + bl), m.ega.read(0xA0000u + uint16_t(si + bl)));
			if (--bl < 0 || bl < m.d8(g::rectLeftCol)) break;
		}
		++m.d8(0x8FAE);
		++m.d8(0x8FAF);
	} while (m.d8(0x8FAE) != m.d8(g::rectBottomLine));
	do {
		clearScrollRow(m, m.d8(0x8FAF));
		++m.d8(0x8FAF);
	} while (--m.d8(0x8FB0) != 0);
}

void scrollLine(Machine &m) {   // 1A0C5: moves to the next text row, scrolling the window when the last row is reached
	uint8_t &speed = m.d8(uint16_t(g::saved2 + 0x0D));
	const uint8_t saved = speed;
	if (m.d8(g::textSpeedFlag) == 0) speed = 8;
	if (m.d8(g::pagingEnabled) != 0) {
		++m.d8(g::moreCounter);
		if (m.d8(g::moreCounter) == 7) {
			morePrompt(m);
			++m.d8(g::moreCounter);
			setWriteCharacterFn(m);
		}
	}
	m.d8(g::scrollSaveTick) = biosTick(m);
	m.d8(g::rectTextX) = m.d8(g::rectLeftCol);
	const uint8_t lastRow = uint8_t((m.d8(g::rectBottomLine) >> 3) - 1);
	auto waitTicks = [&]() {
		for (;;) {
			if (m.onPortraitTick) m.onPortraitTick();
			pollSpeedKeys(m);
			if (uint8_t(biosTick(m) - m.d8(g::scrollSaveTick)) >= m.d8(g::scrollDelay)) break;
			if (m.onIdle) m.onIdle(); else break;
		}
	};
	if (lastRow == m.d8(g::rectTextY)) {
		// shift the per-row capture table up by one
		const uint8_t n = uint8_t(m.d8(g::rectTextY) - 1);
		for (int i = 0; i <= n; ++i) {
			m.d8(uint16_t(g::charCapture + i)) = m.d8(uint16_t(g::charCapture + i + 1));
			m.d8(uint16_t(g::charCapture + i + 1)) = 0;
		}
		m.d8(g::scrollSavedSpeed) = speed;
		m.d8(g::scrollDelay) = m.d8(uint16_t(g::scrollSpeedTables + 9 + speed));
		m.d8(g::scrollSteps) = m.d8(uint16_t(g::scrollSpeedTables + speed));
		do {
			const uint8_t cur = speed;
			speed = m.d8(g::scrollSavedSpeed);
			m.d8(g::scrollSavedSpeed) = cur;
			scrollWindowUp(m);
			const uint8_t cur2 = speed;
			speed = m.d8(g::scrollSavedSpeed);
			m.d8(g::scrollSavedSpeed) = cur2;
			waitTicks();
			m.d8(g::scrollSaveTick) = uint8_t(m.d8(g::scrollSaveTick) + m.d8(g::scrollDelay));
		} while (--m.d8(g::scrollSteps) != 0);
	} else {
		m.d8(g::scrollDelay) = m.d8(uint16_t(0xCDB6 + speed));
		waitTicks();
		++m.d8(g::rectTextY);
	}
	if (m.d8(g::textSpeedFlag) == 0) speed = saved;
}

} // namespace wl

// ---------------------------------------------------------------------------------------------------------------------
// strings

namespace wl {

void morePrompt(Machine &m) {   // 1A1EF: the "MORE" button of the paged text (not ported yet)
	X86Regs x;
	legacy::call(m, 0xA1EF, x);
}

namespace {

void jWriteChar(Machine &m, uint8_t ch) {
	if (m.d16(g::writeCharacterFn) == 0x9DC3) writeCharacter2(m, ch);
	else writeCharacter(m, ch);
}

uint8_t decodeTextGetOffset(Machine &m) {   // 17BC7: next 5 bit code (least significant bit first)
	uint8_t al = 0;
	int bl = 5;
	do {
		if (int8_t(m.d8(g::decodeTextCtr)) < 0) {
			m.d8(g::decodeTextCtr) = 7;
			const uint16_t p = m.d16(g::textString);
			m.d8(g::decodeTextMask) = m.d8(p);
			m.setD16(g::textString, uint16_t(p + 1));
		}
		const uint8_t bit = m.d8(g::decodeTextMask) & 1;
		m.d8(g::decodeTextMask) = uint8_t(m.d8(g::decodeTextMask) >> 1);
		al = uint8_t((al >> 1) | (bit << 7));
		--m.d8(g::decodeTextCtr);
		--bl;
	} while (bl != 0);
	return uint8_t(al >> 3);
}

uint8_t decodeTextGetChar(Machine &m) {   // 17B8F
	uint8_t &caps = m.d8(g::decodeTextCapitals);
	caps = uint8_t(caps >> 1);   // clc; rcr: bit 7 cleared
	bool carry = false;
	uint8_t al;
	for (;;) {
		al = decodeTextGetOffset(m);
		if (al != 30) break;
		carry = true;   // shift-lock marker: the next letter is capitalised
		caps = uint8_t((caps >> 1) | 0x80);
	}
	(void)carry;
	if (al == 31) al = uint8_t(decodeTextGetOffset(m) + 30);
	al = m.d8(uint16_t(m.d16(g::textPtrTemp1) + al));
	caps = uint8_t(caps >> 1);
	if ((caps & 0x40) != 0 && al >= 0x61 && al < 0x7B) al = uint8_t(al - 0x20);
	return al;
}

void decodeTextInit(Machine &m, uint8_t index) {   // 178B9
	m.setD16(g::textPtrTemp1, m.d16(g::textPointer1));
	m.setD16(g::textString, uint16_t(m.d16(g::textPtrTemp1) + 60));   // decodeTextSetSource
	m.d8(g::decodeTextCtr) = 0xFF;
	const uint8_t hi = uint8_t(m.d16(g::textString) >> 8);
	m.d8(0xB263) = hi;                                                 // IDB byte_28083
	const uint8_t bl = uint8_t((index >> 1) & 0xFE);
	const uint16_t base = m.d16(g::textString);
	const unsigned lo = unsigned(m.d8(uint16_t(base + bl))) + (base & 0xFF);
	const uint8_t newHi = uint8_t(m.d8(uint16_t(base + bl + 1)) + hi + (lo > 0xFF ? 1 : 0));
	m.setD16(g::textString, uint16_t((newHi << 8) | (lo & 0xFF)));
	const uint8_t skip = index & 3;
	if (skip != 0) {
		m.d8(g::encodedTextNum) = skip;
		do {
			while (decodeTextGetChar(m) != 0) {}
		} while (--m.d8(g::encodedTextNum) != 0);
	}
}

} // namespace

void writeEncodedText(Machine &m, uint8_t index) {   // 178A3
	decodeTextInit(m, index);
	for (;;) {
		const uint8_t al = decodeTextGetChar(m);
		if (al == 0) break;
		jWriteChar(m, al);
	}
}

void writeCharacterVia(Machine &m, uint8_t ch) { jWriteChar(m, ch); }

void writeItemText(Machine &m, uint8_t index) {   // 17AD3
	m.setD16(g::textPointer1, 0xB270);   // ITEMS_TEXT
	writeEncodedText(m, index);
}

void writeItemTextNormal(Machine &m, uint8_t index) {   // 17ACE
	setWriteCharacterFn(m);
	writeItemText(m, index);
}

void writeItemName(Machine &m, uint8_t itemId) { writeItemText(m, uint8_t(itemId + 36)); }   // 17AF0

void printMessage(Machine &m, uint8_t id) {   // 16CB2
	setWriteCharacterFn(m);
	m.setD16(g::textPointer1, 0xAB3E);
	writeEncodedText(m, id);
}

void printMapMessage(Machine &m, uint8_t id) {   // 178A0: string `id` of the current map
	m.setD16(g::textPointer1, m.d16(m.d16(g::locationsPtr)));
	writeEncodedText(m, id);
}

void writeString2(Machine &m) {   // 17877
	for (;;) {
		const uint8_t al = m.d8(m.d16(g::textString));
		if (al == 0) break;
		jWriteChar(m, al);
		m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
	}
	m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
}

void writeString(Machine &m) {   // 1786E
	setWriteCharacterFn(m);
	writeString2(m);
}

void writeMemberName(Machine &m) {   // 171B9
	m.setD16(g::textString, m.d16(g::currentCharacter));
	writeString2(m);
}

} // namespace wl
