#include "wasteland1/listmenu.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
// working variables of the list window (data segment offsets)
constexpr uint16_t listCallback = 0x4696, listPicked = 0x4698, listFirst = 0x4699, listLastShown = 0x469A, listCount = 0x469C,
                   listLimit = 0x469D, listRows = 0x469E, listShown = 0x469F, listPickedRow = 0x46A0, listTopLine = 0x4691,
                   listExtraKey = 0x470E, listCurrent = 0x4720, listLastIndex = 0x4721, listRowValues = 0xB1D0,
                   listBackup0 = 0xB1DA, listBackup1 = 0xB1DB, listMarked = 0xB1DC, listFirstSeen = 0xB1DD, listFirstFlag = 0xB1DE;

uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0, uint8_t dl = 0, uint8_t bl = 0) {
    X86Regs x;
    x.al = al; x.dl = dl; x.bl = bl;
    legacy::call(m, offset, x);
    return x;
}

// 16D30: the row callback; the two known ones are ported
bool callRow(Machine &m, uint8_t index) {
    const uint16_t callback = m.d16(listCallback);
    if (callback == 0x9582) return skillRow(m, index);
    if (callback == 0x95C6) return itemRow(m, index);
    return call(m, callback, index).cf;
}

// 16EBD: the line below the rows: "more" when items remain, otherwise a blank
void listFooter(Machine &m) {
    m.d8(0x4672) = m.d8(0x4674);
    m.d8(0x4673) = uint8_t(m.d8(listTopLine) + m.d8(listRows));
    if (uint8_t(m.d8(listRows) + 1) >= m.d8(listLimit)) {
        writeCharacter2(m, 3);
        return;
    }
    m.setD16(g::textString, 0xCDBF);
    const X86Regs at = call(m, 0xA26C);   // sub_1A26C: dl, bl = cursor
    call(m, 0x7868, 0, at.dl, at.bl);   // writeTextStringSupportsLowercase
}
}

void listWindowSetup(Machine &m) {
    m.d8(listTopLine) = 3;
    m.d8(listRows) = 9;
    m.d8(0x4672) = m.d8(0x4674);
    m.d8(g::inverseText) = 0;
}

void pagedListDraw(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    listWindowSetup(m);
    D(listMarked) = 0;
    D(listShown) = 0;
    uint8_t index = D(listFirst);
    if (index == 1) D(listFirstFlag) = 1;
    for (;;) {   // 16DCD
        D(listCurrent) = index;
        if (!callRow(m, index)) {   // the row exists (CF clear in the original)
            if (D(listFirstFlag)) {
                D(listFirstSeen) = D(listFirst);
                D(listFirstFlag) = 0;
            }
            D(uint16_t(listRowValues + D(listShown))) = D(listCurrent);
            writeCharacter2(m, 3);
            ++D(listShown);
            if (D(listShown) == D(listRows)) {   // page full
                D(listLastShown) = D(listCurrent);
                listFooter(m);
                return;
            }
        }
        index = uint8_t(D(listCurrent) + 1);   // 16E05
        if (index == D(listFirst) || index == D(listCount)) break;
    }
    if (D(listShown) == 0) {   // 16E16: nothing on this page: back up
        pagedListBack(m);
        return;
    }
    for (uint8_t row = D(listShown); row != D(listRows); ++row) {   // pad the page with empty lines
        D(listPicked) = row;
        scrollLine(m);
        writeCharacter2(m, 3);
    }
    D(listLastShown) = D(listCurrent);
    listFooter(m);
}

void pagedListBack(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    D(listBackup0) = D(listShown);
    D(listBackup1) = D(listMarked);
    listWindowSetup(m);
    D(listMarked) = 8;
    D(listShown) = 8;
    uint8_t index = D(listFirst);
    for (;;) {   // 16E90
        index = uint8_t(index - 1);
        if (index == 0) {   // reached the start of the list without filling a page
            D(listShown) = D(listBackup0);
            D(listMarked) = D(listBackup1);
            return;
        }
        D(listCurrent) = index;   // 16E62
        if (callRow(m, index)) continue;   // empty slot
        D(uint16_t(listRowValues + D(listShown))) = D(listCurrent);
        if (D(listShown) == 8) D(listLastShown) = D(listCurrent);
        writeCharacter2(m, 3);
        D(listShown) = uint8_t(D(listShown) - 1);
        if (D(listShown) & 0x80) {   // the page is full
            D(listFirst) = D(listCurrent);
            listFooter(m);
            return;
        }
    }
}

ListResult pagedListKeys(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    for (;;) {
        waitForKeypress(m);
        const uint8_t key = D(g::lastKey);
        if (key == 0xCB || key == 0xC8 || key == 'I') {   // page up
            if (D(listFirst) > D(listFirstSeen)) pagedListBack(m);
            continue;
        }
        if (key == 0xCD || key == 0xD0 || key == 'K') {   // page down
            const uint8_t next = uint8_t(D(listLastShown) + 1);
            if (next != D(listCount)) {
                D(listFirst) = next;
                pagedListDraw(m);
            }
            continue;
        }
        if (key == 27) return {true, 0xFF};
        if (key == D(listExtraKey)) return {true, 0xFE};
        if (key == 13 || key == 32) return {true, 0xFD};
        if (key < '1' || key > '9') continue;
        const uint8_t row = uint8_t((key - 1) & 0x0F);
        if (row >= D(listShown)) continue;
        D(listPickedRow) = row;
        D(listPicked) = D(uint16_t(listRowValues + row));
        return {false, D(listPicked)};
    }
}

void markPickedRow(Machine &m) {
    m.d8(g::inverseText) = 0xFF;
    for (int i = 0; i < 3; ++i) {   // 16F35: three inverse blanks in the picked row's marker column
        m.d8(0x4673) = uint8_t(m.d8(listPickedRow) + m.d8(listTopLine));
        m.d8(0x4672) = 15;
        call(m, 0x0015);
        call(m, 0x8DBE, 0xA0);
    }
    m.d8(g::inverseText) = 0;
}

bool skillRow(Machine &m, uint8_t index) {
    const uint8_t slot = uint8_t(index * 2 + 0x7E);
    m.d8(0xCCE6) = slot;
    const uint8_t id = m.d8(uint16_t(cur(m) + slot));
    if (id == 0 || id >= 0x24) return true;   // CF set: nothing to show
    call(m, 0x6EE8);   // sub_16EE8: row marker
    m.d8(0x467F) = 3;
    writeNumberShort(m, m.d8(uint16_t(cur(m) + uint8_t(m.d8(0xCCE6) + 1))));
    ++m.d8(0x4672);
    writeItemText(m, m.d8(uint16_t(cur(m) + m.d8(0xCCE6))));   // 17AC8: skill name
    return false;
}

bool itemRow(Machine &m, uint8_t index) {
    X86Regs z;
    legacy::call(m, 0x8D27, z);   // nullBxDx
    m.d8(0xCCE5) = index;
    const uint8_t slotOffset = getInventoryItemOffset(index);
    const uint8_t id = m.d8(uint16_t(cur(m) + slotOffset));
    if (id == 0) {
        legacy::call(m, 0x8D2C, z);
        return true;
    }
    call(m, 0x6EE8);
    const uint8_t wanted = m.d8(0xCCE5);
    if (wanted == m.d8(uint16_t(cur(m) + 0x25)) || wanted == m.d8(uint16_t(cur(m) + 0x1F))) m.d8(g::inverseText) = 0xFF;
    setWriteCharacterFn2(m);
    m.d8(0x4687) = 1;
    writeItemName(m, id);
    m.d8(g::inverseText) = 0;
    legacy::call(m, 0x8D2C, z);
    return false;
}


namespace {
// 199DC (twice in 199D9): moves one byte of the current character to the scratch list at ds:7931+dl and clears it
void moveByte(Machine &m, uint8_t &bl, uint8_t &dl) {
    const uint16_t at = uint16_t(cur(m) + bl);
    m.d8(uint16_t(0x7931 + dl)) = m.d8(at);
    m.d8(at) = 0;
    ++dl;
    ++bl;
}
void moveEntry(Machine &m, uint8_t &bl, uint8_t &dl) {   // 199D9
    moveByte(m, bl, dl);
    moveByte(m, bl, dl);
}
}

ListResult skillList(Machine &m, uint8_t title) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    D(0xCCD9) = title;
    D(listCount) = 0;
    D(listFirst) = 1;
    uint8_t count = 1;
    for (uint8_t bl = 0x80; bl != 0xBE; bl = uint8_t(bl + 2)) {
        ++D(listCount);
        const uint8_t id = D(uint16_t(cur(m) + bl));
        if (id != 0 && id < 0x24) {
            D(listLastIndex) = D(listCount);
            ++count;
        }
    }
    if (count == 1) return {true, 0xFF, true};
    D(listLimit) = count;
    writeItemTextNormal(m, 0x99);
    setWriteCharacterFn2(m);
    writeItemText(m, D(0xCCD9));
    m.setD16(listCallback, 0x9582);
    pagedListDraw(m);
    D(listExtraKey) = 18;
    return pagedListKeys(m);
}

ListResult skillScreen(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    for (;;) {
        D(0xCCDA) = 0;
        ListResult r = skillList(m, 0);
        if (r.al != 0xFE) return {true, r.al};
        if (D(listLimit) == 1) {
            D(listExtraKey) = 18;
            return pagedListKeys(m);
        }
        for (int i = 59; i >= 0; --i) D(uint16_t(0x7931 + i)) = D(uint16_t(cur(m) + 0xBB - (59 - i)));
        const uint16_t keep1 = m.d16(g::buttonMask1), keep2 = m.d16(g::buttonMask2);
        m.setD16(g::buttonMask1, 0x34);
        m.setD16(g::buttonMask2, 0);
        for (;;) {   // 19933
            r = skillList(m, 0xA8);
            if (r.cf) {
                if (r.al != 0xFF || !r.empty) continue;
                m.setD16(g::buttonMask2, keep2);
                m.setD16(g::buttonMask1, keep1);
                for (int i = 59; i >= 0; --i) D(uint16_t(cur(m) + 0xBB - (59 - i))) = D(uint16_t(0x7931 + i));
                break;   // back to 198F0
            }
            markPickedRow(m);
            uint8_t bl = uint8_t(D(listPicked) * 2 + 0x7E), dl = D(0xCCDA);
            moveEntry(m, bl, dl);
            D(0xCCDA) = dl;
        }
    }
}

ListResult itemList(Machine &m, uint8_t title) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    D(0xCCE8) = title;
    // 1963A: count the items of the current character
    D(0xCCE4) = 0;
    uint8_t found = 0;
    for (uint8_t bl = 0xBD; bl != 0xF9; bl = uint8_t(bl + 2)) {
        if (D(uint16_t(cur(m) + bl)) == 0) continue;
        D(listLastIndex) = uint8_t((bl - 0xBB) >> 1);
        ++found;
    }
    const uint8_t count = uint8_t(found + 1);
    if (count == 1) return {true, 0, true};
    D(listLimit) = count;
    writeItemTextNormal(m, 0x97);
    setWriteCharacterFn2(m);
    writeItemText(m, D(0xCCE8));
    D(listCount) = 0x1F;
    D(listFirst) = 1;
    D(0x4687) = 1;
    m.setD16(listCallback, 0x95C6);
    pagedListDraw(m);
    D(listExtraKey) = 18;
    ListResult r = pagedListKeys(m);
    if (r.al >= 0xFD) { r.cf = true; return r; }
    markPickedRow(m);
    return {false, D(listPicked)};
}

ListResult itemScreen(Machine &m, uint8_t member) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    setCurrentCharacter(m, member);
    for (;;) {
        D(0xCCEC) = 0;
        D(0xCCEB) = 0;
        ListResult r = itemList(m, 0);
        if (!r.cf) return r;
        if (r.al != 0xFE) return {true, r.al};
        D(0xCCE9) = 0;
        D(0xCCEA) = 0;
        for (int i = 59; i >= 0; --i) D(uint16_t(0x7931 + i)) = 0;
        if (D(listLimit) == 1) {   // 1948A
            D(listExtraKey) = 18;
            r = pagedListKeys(m);
            if (r.al >= 0xFD) return {true, r.al};
            markPickedRow(m);
            return {false, D(listPicked)};
        }
        const uint16_t keep1 = m.d16(g::buttonMask1), keep2 = m.d16(g::buttonMask2);
        m.setD16(g::buttonMask1, 0x34);
        m.setD16(g::buttonMask2, 0);
        for (;;) {   // 193E0
            ++D(0xCCEB);
            r = itemList(m, 0xA8);
            if (r.cf) {
                if (r.al != 0) continue;
                m.setD16(g::buttonMask2, keep2);
                m.setD16(g::buttonMask1, keep1);
                D(uint16_t(cur(m) + 0x1F)) = D(0xCCEA);
                D(uint16_t(cur(m) + 0x25)) = D(0xCCE9);
                for (int i = 59; i >= 0; --i) D(uint16_t(cur(m) + 0xF8 - (59 - i))) = D(uint16_t(0x7931 + i));
                break;
            }
            const uint8_t picked = r.al;
            if (picked == D(uint16_t(cur(m) + 0x1F))) D(0xCCEA) = D(0xCCEB);
            if (picked == D(uint16_t(cur(m) + 0x25))) D(0xCCE9) = D(0xCCEB);
            uint8_t bl = getInventoryItemOffset(picked), dl = D(0xCCEC);
            moveEntry(m, bl, dl);
            D(0xCCEC) = dl;
        }
    }
}

} // namespace wl
