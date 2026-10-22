// Register adapters for the low-level routines that already exist as clean ports (text, rules, party records).
#include "wasteland1/ports/port.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"
#include "wasteland1/ui.h"

namespace wl {
namespace rc {

// ---- WLA.BIN glyph writers ------------------------------------------------------------------------------------------
PORT(writeCharacterAt) { r.es = 0xA000; writeCharacterAt(m, r.a.b.l); }
PORT(writeColorCharacter) { r.es = 0xA000; writeColorCharacter(m, r.a.b.l); }
PORT(clearRect) { r.es = 0xA000; clearRect(m); }

// ---- random numbers and dice ------------------------------------------------------------------------------------------
PORT(rngByte) { r.a.b.l = Rules(m).rngByte(r.a.b.l); }
PORT(random) { r.a.b.l = Rules(m).random(r.a.b.l); }
PORT(d6) { r.a.b.l = Rules(m).d6(r.a.b.l); }
PORT(rollDice) { r.a.b.l = Rules(m).rollDice(); }

// ---- party records --------------------------------------------------------------------------------------------------
PORT(setPartyIndexesPtr) { setPartyIndexesPtr(m, r.a.b.l); }
PORT(setCurrentCharacter) { setCurrentCharacter(m, r.a.b.l); }
PORT(isMemberUnconscious) { r.cf = isMemberUnconscious(m, r.a.b.l); }
PORT(conBytesOr) { r.a.b.l = conBytesOr(m); r.zf = r.a.b.l == 0; }
PORT(getMemberConditionIndex) { r.d.b.l = getMemberConditionIndex(m); }
PORT(getMemberSkillLevel) { r.a.b.l = getMemberSkillLevel(m, r.a.b.l); r.zf = r.a.b.l == 0; }
PORT(findItemById) {
	const int off = findItemById(m, r.a.b.l);
	r.b.b.l = off < 0 ? 0xFF : uint8_t(off);
	r.cf = off < 0;
}

// ---- text engine -------------------------------------------------------------------------------------------------------
PORT(convertNumberToBcd) { convertNumberToBcd(m); }
PORT(writeNumber) { writeNumber(m); }
PORT(writeCharacter) { writeCharacter(m, r.a.b.l); }
PORT(writeCharacter2) { r.es = 0xA000; writeCharacter2(m, r.a.b.l); }
PORT(printMessage) { r.es = 0; printMessage(m, r.a.b.l); }
PORT(hintOverlay) { hintOverlay(m); }
PORT(flushLine) { flushLine(m); }
PORT(newline) { r.es = 0; newline(m); }

} // namespace rc
} // namespace wl
