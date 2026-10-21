#pragma once
// The character sheet screen (KEH characterInfo*): stats, inventory, active and passive skill lists.
#include "fod/screen.h"

namespace fod {

struct Win {                       // text window in character cells, right/bottom inclusive (KEH Window struct)
	int x0, y0, x1, y1;
	int width() const { return x1 - x0 + 1; }
};

void winWrite(Screen &s, const Assets &a, const Win &w, int x, int y, const std::string &text, bool inverse = false);
void winClear(Screen &s, const Win &w);

struct SheetState {
	int page = 0;                  // 0 inventory, 1 active skills, 2 passive skills
	int invOffset = 0, activeOffset = 0, passiveOffset = 0;
	bool showConditions = false;   // the (V)iew popup replaces the stats window
};

// Items shown for the active skill / passive skill lists (indices of learned skills).
std::vector<int> learnedSkills(const World &w, const Member &m, bool passive);

void drawCharacterSheet(Screen &s, const Assets &a, const World &w, int member, const SheetState &st);

} // namespace fod
