#include "fod/charsheet.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <functional>

namespace fod {

namespace {
const Win kStats{1, 1, 23, 13}, kInventory{1, 14, 23, 23}, kActive{25, 1, 38, 11}, kPassive{25, 13, 38, 23}, kConditions{1, 2, 23, 13};

std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char *f, ...) {
	char buf[128];
	va_list ap;
	va_start(ap, f);
	std::vsnprintf(buf, sizeof buf, f, ap);
	va_end(ap);
	return buf;
}
}

void winClear(Screen &s, const Win &w) {
	for (int y = w.y0 * 8; y < (w.y1 + 1) * 8; ++y)
		for (int x = w.x0 * 8; x < (w.x1 + 1) * 8; ++x) s.put(x, y, 0);
}

void winWrite(Screen &s, const Assets &a, const Win &w, int x, int y, const std::string &text, bool inverse) {
	for (size_t i = 0; i < text.size(); ++i) {
		const int cx = x + int(i);
		if (cx >= w.width()) break;
		drawGlyph(s, a, (w.x0 + cx) * 8, (w.y0 + y) * 8, uint8_t(text[i]), inverse);
	}
}

std::vector<int> learnedSkills(const World &w, const Member &m, bool passive) {
	(void)w;
	std::vector<int> out;
	for (int i = 0; i < 16; ++i)
		if (m.b[(passive ? 0x34 : 0x24) + size_t(i)] != 0) out.push_back(i);
	return out;
}

static void drawListPage(Screen &s, const Assets &a, const Win &win, const std::string &title, bool highlightTitle, int total, int offset,
                         const std::function<std::string(int, int)> &row, int &shown) {
	winClear(s, win);
	shown = std::min(8, total - offset);
	if (total > 8) {
		const int marker = offset == 0 ? 0x19 : (offset + shown < total ? 0x1A : 0x18);
		drawGlyph(s, a, win.x0 * 8, win.y0 * 8, marker);
	}
	winWrite(s, a, win, 2, 0, title, highlightTitle);
	for (int i = 0; i < shown; ++i) winWrite(s, a, win, 0, i + 1, row(i + 1, offset + i));
}

void drawCharacterSheet(Screen &s, const Assets &a, const World &w, int member, const SheetState &st) {
	const Member &m = w.party(member);
	s.clear();
	drawBorders(s, a, 1);

	// ---- stats ----
	if (st.showConditions) {
		winClear(s, kConditions);
		winWrite(s, a, kConditions, 0, 0, "Conditions:");
		int y = 2;
		for (int i = 0; i < 7; ++i)
			if (m.b[0x58] & (1u << i)) {
				const char *n = reinterpret_cast<const char *>(w.data.hdspct.data() + 0x3EE + 0x16 * size_t(i));
				winWrite(s, a, kConditions, 3, y++, std::string(n, strnlen(n, 20)));
			}
		if (m.b[0x54]) {
			const char *n = reinterpret_cast<const char *>(w.data.hdspct.data() + 0x488);
			winWrite(s, a, kConditions, 3, y++, std::string(n, strnlen(n, 20)));
			if (m.b[0x21]) winWrite(s, a, kConditions, 0, y++, fmt("(remission exp:%d hrs.)", m.b[0x21]));
		}
	} else {
		winClear(s, kStats);
		std::string name = m.name();
		winWrite(s, a, kStats, std::max(0, (25 - int(name.size())) / 2), 0, name, st.page == 0);
		winWrite(s, a, kStats, 0, 1, fmt("Rank: %u", m.b[0x4E]));
		winWrite(s, a, kStats, 0, 2, std::string("Cond: ") + (m.b[0x58] ? "Afflicted" : "Unafflicted"));
		if (m.b[0x58]) winWrite(s, a, kStats, 16, 2, "(V)iew");
		static const char *kState[6] = {"OK ", "UNC", "SER", "CRT", "COM", "DED"};
		const int cond = World::conState(m);
		if (cond == 0) winWrite(s, a, kStats, 0, 3, fmt(" Con: %3u  MaxCon:  %3u", uint16_t(m.hp()), uint16_t(m.hpMax())));
		else winWrite(s, a, kStats, 0, 3, fmt(" Con: %3s  MaxCon:  %3u", kState[cond], uint16_t(m.hpMax())));
		winWrite(s, a, kStats, 0, 4, fmt("  AC: %3u", w.armorClass(m)));
		const std::string weap = m.b[0x4A] == 0xFF ? "Hands" : w.item(m.b[0x62 + 6 * m.b[0x4A]]).name;
		winWrite(s, a, kStats, 0, 5, "Weap: " + weap.substr(0, 17));
		winWrite(s, a, kStats, 6, 7, "Attributes");
		static const struct { int x, y; const char *label; } kAttr[7] = {{1, 9, "ST:"}, {17, 9, "IQ:"}, {1, 10, "DX:"}, {9, 10, "WP:"},
		                                                              {17, 10, "AP:"}, {1, 11, "CH:"}, {17, 11, "LK:"}};
		for (int i = 0; i < 7; ++i) winWrite(s, a, kStats, kAttr[i].x, kAttr[i].y, fmt("%3s%2u", kAttr[i].label, m.attr(i)));
	}

	// ---- inventory ----
	int shown = 0;
	drawListPage(s, a, kInventory, "Inventory Items", st.page == 0, m.b[0x52], st.invOffset,
	             [&](int n, int idx) {
		             bool equipped = m.b[0x4A] == idx;
		             for (int k = 0; k < 3; ++k) equipped |= m.b[0x4B + k] == idx;
		             return fmt("%1d>%c", n, equipped ? '*' : ' ') + w.item(m.b[0x62 + 6 * size_t(idx)]).name;
	             }, shown);
	// jammed weapons are drawn inverse
	for (int i = 0; i < shown; ++i) {
		const int idx = st.invOffset + i;
		if (m.b[0x65 + 6 * size_t(idx)] & 1)
			winWrite(s, a, kInventory, 3, i + 1, w.item(m.b[0x62 + 6 * size_t(idx)]).name, true);
	}

	// ---- skills ----
	const auto active = learnedSkills(w, m, false), passive = learnedSkills(w, m, true);
	drawListPage(s, a, kActive, "Active", st.page == 1, int(active.size()), st.activeOffset,
	             [&](int n, int idx) { const int k = active[size_t(idx)]; return fmt("%1d> %3d ", n, m.b[0x24 + size_t(k)]) + w.skillName(k); }, shown);
	drawListPage(s, a, kPassive, "Passive", st.page == 2, int(passive.size()), st.passiveOffset,
	             [&](int n, int idx) { const int k = passive[size_t(idx)]; return fmt("%1d> %3d ", n, m.b[0x34 + size_t(k)]) + w.skillName(16 + k); }, shown);
}

} // namespace fod
