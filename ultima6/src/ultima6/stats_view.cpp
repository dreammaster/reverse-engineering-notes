#include "ultima6/stats_view.h"
#include "ultima6/describe.h"
#include "ultima6/ui.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

int ActorStats::maxHealth(const World &w, int actor) {
	int v = w.actors.level[actor] * 30;
	if (v == 0)
		v = 1;
	return v > 255 ? 255 : v;
}

int ActorStats::maxMagic(const World &w, int actor) {
	int intel = w.actors.intelligence[actor];
	switch (w.actors.objType(actor)) {
	case 0x19A:
		return intel * 2;
	case 0x17A:
		return intel;
	case 0x179:
	case 0x182:
		return intel / 2;
	default:
		return 0;
	}
}

void inventoryItems(const World &world, int member, std::vector<int> &items) {
	world.inventory(world.actors.partyRoster[member], items, false);
	std::stable_sort(items.begin(), items.end(), [&](int a, int b) {
		return (world.objects[a].location() == ObjRecord::READIED) > (world.objects[b].location() == ObjRecord::READIED);
	});
}

// APPROX(28): not the original's exact behaviour, see docs/approximations.md
void drawInventoryView(Framebuffer &screen, const World &world, int member, int selected, std::vector<int> &items) {
	Ui::restorePaper(screen, world.paper, 176, 8, 136, 96);
	const int FG = 0x48;
	int actor = world.actors.partyRoster[member];
	std::string name = (const char *)world.actors.partyNames[member];
	Ui::drawText(screen, world.font, 176 + (17 - (int)name.size()) / 2 * 8, 8, name, FG);

	inventoryItems(world, member, items);
	const int COLS = 8, X0 = 176 + 4, Y0 = 20;
	for (size_t n = 0; n < items.size() && n < (size_t)COLS * 4; ++n) {
		const World::Obj &o = world.objects[items[n]];
		int x = X0 + ((int)n % COLS) * 16, y = Y0 + ((int)n / COLS) * 16;
		Ui::drawTile(screen, world, world.baseTile.firstTile[o.type] + o.frame, x, y);
		int kind = Describe::quantityKind(o.type);
		if (kind && world.quantityOf(o) > 1)
			Ui::drawText(screen, world.font, x, y + 8, std::to_string(world.quantityOf(o) % 100), 0xFF);
		if (o.location() == ObjRecord::READIED)
			Ui::drawText(screen, world.font, x + 8, y, "*", 10);
	}
	if (!items.empty() && selected >= 0 && selected < (int)items.size()) {
		int x = X0 + (selected % COLS) * 16, y = Y0 + (selected / COLS) * 16;
		Ui::drawTile(screen, world, 0x16C, x, y);
		const World::Obj &o = world.objects[items[selected]];
		std::string d = Describe::object(world, o);
		Ui::drawText(screen, world.font, 176, 88, d.substr(0, 17), FG);
	}
	char buf[40];
	int carried = world.carriedWeight(actor), most = world.actors.strength[actor] * 2 * 10;
	std::snprintf(buf, sizeof buf, "%d.%d/%d.%d", carried / 10, carried % 10, most / 10, most % 10);
	Ui::drawText(screen, world.font, 176, 96, buf, FG);
}

void drawTalkView(Framebuffer &screen, const World &world, int actor) {
	Ui::restorePaper(screen, world.paper, 176, 8, 136, 96);
	Bytes portrait;
	if (world.portrait(actor, portrait))
		screen.blit(176 + (136 - 56) / 2, 16, &portrait[0], 56, 64, 56);
}

void drawStatsView(Framebuffer &screen, const World &world, int member) {
	const ObjList &a = world.actors;
	Ui::restorePaper(screen, world.paper, 176, 8, 136, 96);
	int actor = a.partyRoster[member];
	const int FG = 0x48;
	char buf[40];

	std::string name = (const char *)a.partyNames[member];
	Ui::drawText(screen, world.font, 176 + (17 - (int)name.size()) / 2 * 8, 8, name, FG);

	Bytes portrait;
	if (world.portrait(actor, portrait))
		screen.blit(176, 16, &portrait[0], 56, 64, 56);

	const int X = 176 + 10 * 8;
	std::snprintf(buf, sizeof buf, "STR:%d", a.strength[actor]);
	Ui::drawText(screen, world.font, X, 8 + 2 * 8, buf, FG);
	std::snprintf(buf, sizeof buf, "DEX:%d", a.dexterity[actor]);
	Ui::drawText(screen, world.font, X, 8 + 3 * 8, buf, FG);
	std::snprintf(buf, sizeof buf, "INT:%d", a.intelligence[actor]);
	Ui::drawText(screen, world.font, X, 8 + 4 * 8, buf, FG);

	Ui::drawText(screen, world.font, X, 8 + 6 * 8, "Magic", FG);
	std::snprintf(buf, sizeof buf, "%2d/%d", a.unknown13f1[actor], ActorStats::maxMagic(world, actor));
	Ui::drawText(screen, world.font, X, 8 + 7 * 8, buf, FG);

	Ui::drawText(screen, world.font, X, 8 + 8 * 8, "Health", FG);
	int hpColour = (a.status[actor] & 0x08) ? 10 : a.hp[actor] < 10 ? 12 : FG; // poisoned: green, nearly dead: red
	std::snprintf(buf, sizeof buf, "%3d", a.hp[actor]);
	Ui::drawText(screen, world.font, X, 8 + 9 * 8, buf, hpColour);
	std::snprintf(buf, sizeof buf, "/%d", ActorStats::maxHealth(world, actor));
	Ui::drawText(screen, world.font, X + 3 * 8, 8 + 9 * 8, buf, FG);

	Ui::drawText(screen, world.font, X, 8 + 10 * 8, "Lev/Exp", FG);
	std::snprintf(buf, sizeof buf, "%d/%d", ActorStats::level(world, actor), a.exp[actor]);
	Ui::drawText(screen, world.font, X, 8 + 11 * 8, buf, FG);

	// the buttons: previous member, (two icons), next member
	if (member > 0)
		Ui::drawTile(screen, world, 0x183, 0xB0, 0x58);
	Ui::drawTile(screen, world, 0x180, 0xC0, 0x58);
	Ui::drawTile(screen, world, 0x182, 0xD0, 0x58);
	if (member < a.partySize - 1)
		Ui::drawTile(screen, world, 0x184, 0xE0, 0x58);
}

} // namespace Ultima6
} // namespace Ultima6
