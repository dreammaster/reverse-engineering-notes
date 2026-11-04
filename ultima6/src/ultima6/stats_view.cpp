#include "ultima6/stats_view.h"
#include "ultima6/describe.h"
#include "ultima6/equipment.h"
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

// `member` is a place in the party's roster, or 100 + an actor's number to look at somebody else's things (the Pickpocket spell)
static int actorOfMember(const World &world, int member) {
	return member >= 100 ? member - 100 : world.actors.partyRoster[member];
}

void inventoryItems(const World &world, int member, std::vector<int> &items) {
	int actor = actorOfMember(world, member);
	Equipment::Slots slots = Equipment::of(world, actor);
	std::vector<int> inv;
	world.inventory(actor, inv, false);
	items.clear();
	for (int k = 0; k < Equipment::SLOTS; ++k)
		if (slots.object[k] >= 0)
			items.push_back(slots.object[k]);
	size_t equipped = items.size();
	for (int i : inv)
		if (world.objects[i].location() != ObjRecord::READIED)
			items.push_back(i);
	for (int i : inv)
		if (world.objects[i].location() == ObjRecord::READIED && std::find(items.begin(), items.begin() + equipped, i) == items.begin() + equipped)
			items.push_back(i);
}

// The tile of a person's picture in the corner of the view: sub_19533 gives the frame of the direction 4 (south), for the people two frames a direction
static int dollTile(const World &world, int actor) {
	int type = world.actors.baseObjFrame[actor] & 0x3FF;
	if (!type) {
		int k = world.actorObject(actor);
		type = k >= 0 ? world.objects[k].type : 0;
	}
	int frame = (type >= 0x178 && type <= 0x183) || type == 0x199 || type == 0x19A ? 4 * 2 + 1 : 0;
	return world.baseTile.firstTile[type] + frame;
}

// sub_1DB44: the eight places of the doll, offset by (dx, dy)
static void drawDoll(Framebuffer &screen, const World &world, const Equipment::Slots &slots, int dx, int dy) {
	for (int k = 0; k < Equipment::SLOTS; ++k) {
		int x = Equipment::SLOT_X[k] + dx, y = Equipment::SLOT_Y[k] + dy;
		if (slots.object[k] >= 0) {
			const World::Obj &o = world.objects[slots.object[k]];
			Ui::drawTile(screen, world, 0x19B, x, y);
			Ui::drawTile(screen, world, world.baseTile.firstTile[o.type] + o.frame, x, y);
		} else {
			Ui::drawTile(screen, world, k == 5 && slots.twoHanded ? 0x185 : 0x19A, x, y);
		}
	}
}

// sub_2F969: what looking at a person shows: the portrait, and what the person has readied around it, then the name in the middle of row 10. With a portrait and equipment the portrait stands
// at (248, 24) and the doll is lowered by 8; with equipment alone the doll is lowered by 8 and moved 32 to the right; with a portrait alone it stands at (216, 24).
void drawActorView(Framebuffer &screen, const World &world, int actor, const std::string &name) {
	Ui::restorePaper(screen, world.paper, 176, 8, 136, 96);
	Equipment::Slots slots = Equipment::of(world, actor);
	bool equipped = false;
	for (int o : slots.object)
		equipped |= o >= 0;
	Bytes portrait;
	bool shown = world.portrait(actor, portrait);
	if (equipped && shown) {
		screen.blit(248, 24, &portrait[0], world.portraitW, world.portraitH, world.portraitW);
		drawDoll(screen, world, slots, 0, 8);
	} else if (equipped) {
		drawDoll(screen, world, slots, 32, 8);
	} else if (shown) {
		screen.blit(216, 24, &portrait[0], world.portraitW, world.portraitH, world.portraitW);
	}
	Ui::drawText(screen, world.font, 176 + (17 - (int)name.size()) / 2 * 8, 8 + 10 * 8, name, 0x48);
}

// sub_1DF45 / sub_1DB44: the name at the top; the doll's eight places for what is readied (the frame tile 0x19B behind the thing, 0x19A for a free place, 0x185 for the hand a two-handed
// weapon holds), the person in the corner, a pack of twelve places (four across, three down) that scrolls by rows of four with arrows when there is more, and the weights.
// APPROX(28): the weights lines and the cursor are not the original's
void drawInventoryView(Framebuffer &screen, const World &world, int member, int selected, std::vector<int> &items, int &scroll) {
	Ui::restorePaper(screen, world.paper, 176, 8, 136, 96);
	const int FG = 0x48;
	int actor = actorOfMember(world, member);
	std::string name;
	if (member >= 100) {
		ConverseScript script;
		if (world.script(actor, script))
			name = script.name();
		std::replace(name.begin(), name.end(), '_', '.');
	} else {
		name = (const char *)world.actors.partyNames[member];
	}
	Ui::drawText(screen, world.font, 176 + (17 - (int)name.size()) / 2 * 8, 8, name, FG);

	inventoryItems(world, member, items);
	Equipment::Slots slots = Equipment::of(world, actor);
	int equipped = 0;
	for (int o : slots.object)
		equipped += o >= 0;
	auto tileOf = [&](int idx) { return world.baseTile.firstTile[world.objects[idx].type] + world.objects[idx].frame; };
	auto cell = [&](int idx, int x, int y) {
		Ui::drawTile(screen, world, 0x19B, x, y);
		Ui::drawTile(screen, world, tileOf(idx), x, y);
		const World::Obj &o = world.objects[idx];
		if (Describe::quantityKind(o.type) && world.quantityOf(o) > 1)
			Ui::drawText(screen, world.font, x, y + 8, std::to_string(world.quantityOf(o) % 100), 0xFF);
	};
	drawDoll(screen, world, slots, 0, 0);
	Ui::drawTile(screen, world, 0x19B, 272, 16);
	Ui::drawTile(screen, world, dollTile(world, actor), 272, 16);

	int packCount = (int)items.size() - equipped;
	int row = selected >= equipped ? (selected - equipped) / 4 : -1;
	if (row >= 0) {
		if (row * 4 < scroll)
			scroll = row * 4;
		else if (row * 4 >= scroll + 12)
			scroll = (row - 2) * 4;
	}
	if (scroll < 0 || scroll >= std::max(packCount, 1))
		scroll = 0;
	for (int n = 0; n < 12; ++n) {
		int x = 248 + (n & 3) * 16, y = 32 + (n >> 2) * 16;
		if (scroll + n < packCount)
			cell(items[equipped + scroll + n], x, y);
		else
			Ui::drawTile(screen, world, 0x19A, x, y);
	}
	if (scroll != 0)
		Ui::drawText(screen, world.font, 240, 32, std::string(1, (char)0x18), FG);
	if (packCount > scroll + 12)
		Ui::drawText(screen, world.font, 240, 72, std::string(1, (char)0x19), FG);

	if (!items.empty() && selected >= 0 && selected < (int)items.size()) {
		int x, y;
		if (selected < equipped) {
			int k = 0;
			for (; k < Equipment::SLOTS; ++k)
				if (slots.object[k] == items[selected])
					break;
			x = Equipment::SLOT_X[k < Equipment::SLOTS ? k : 0];
			y = Equipment::SLOT_Y[k < Equipment::SLOTS ? k : 0];
		} else {
			int n = selected - equipped - scroll;
			x = 248 + (n & 3) * 16;
			y = 32 + (n >> 2) * 16;
		}
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
		screen.blit(176 + (136 - world.portraitW) / 2, 16, &portrait[0], world.portraitW, world.portraitH, world.portraitW);
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
		screen.blit(176, 16, &portrait[0], world.portraitW, world.portraitH, world.portraitW);

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
