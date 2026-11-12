// Which Ultima VI file formats the two spin-offs share (skipped when their data is absent).

#include "files/lib_n.h"
#include "ultima6/combat.h"
#include "ultima6/conversation.h"
#include "ultima6/world.h"
#include "files/map_data.h"
#include "files/obj_block.h"
#include "files/tile_data.h"
#include "files/tile_set.h"
#include "files/world_data.h"

#include <cstdio>
#include <map>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

using ::Ultima6::Ultima6::Conversation;
using ::Ultima6::Ultima6::ConversationHost;

class SpinStubHost : public ConversationHost {
public:
	std::string out;
	std::map<int, int> flags;
	int karma = 0;

	void print(const std::string &t) override {
		for (char c : t) {
			if (c == '\b' && !out.empty())
				out.pop_back();
			else
				out += c;
		}
	}
	std::string playerName() override { return "Avatar"; }
	std::string genderTitle() override { return "milord"; }
	std::string timeOfDay() override { return "morning"; }
	void showPortrait(int) override {}
	int roll(int lo, int) override { return lo; }
	std::string scriptName(int npc) override { return "Npc" + std::to_string(npc); }
	std::string partyName(int, bool &in) override { in = false; return ""; }
	int talkFlags(int n) override { return flags[n]; }
	void setTalkFlags(int n, int f) override { flags[n] = f; }
	bool inParty(int) override { return false; }
	int partyMember(int) override { return 0; }
	int joinParty(int) override { return 0; }
	int leaveParty(int) override { return 0; }
	bool npcNearby(int) override { return true; }
	bool wounded(int) override { return false; }
	bool poisoned(int) override { return false; }
	void heal(int) override {}
	void cure(int) override {}
	void resurrect(int) override {}
	void setWorktype(int, int) override {}
	int adjustStat(int, int, int d) override { return d; }
	bool hasObject(int, int, int, bool) override { return false; }
	int countObject(int, int) override { return 0; }
	void newObject(int, int, int, int) override {}
	void deleteObject(int, int, int, int) override {}
	void giveObject(int, int, int, int) override {}
	int whoHasObject(int, int) override { return -1; }
	int maxCarry(int) override { return 100; }
	int objectWeight(int, int q) override { return q; }
	bool horsed(int) override { return false; }
	void mount(int) override {}
	void addKarma(int n) override { karma += n; }
	void subtractKarma(int n) override { karma -= n; }
	void sleep() override {}
};

static void check(const char *dir, int schedules) {
	Bytes d;
	if (!readFile(joinPath(dir, "ANIMDATA"), d)) {
		std::printf("skip %s (no game data)\n", dir);
		return;
	}
	AnimData anim;
	CHECK(anim.load(d));
	TileFlags flags;
	CHECK(readFile(joinPath(dir, "TILEFLAG"), d) && flags.load(d));
	BaseTile base;
	CHECK(readFile(joinPath(dir, "BASETILE"), d) && base.load(d));
	Schedule sched;
	CHECK(readFile(joinPath(dir, "SCHEDULE"), d) && sched.load(d));
	CHECK(sched.total() == schedules);

	Bytes map, chunks;
	MapData md;
	CHECK(readFile(joinPath(dir, "MAP"), map) && readFile(joinPath(dir, "CHUNKS"), chunks) && md.load(map, chunks));
	for (int z = 0; z <= MapData::DUNGEON_LEVELS; ++z) {
		int size = z == 0 ? MapData::SURFACE_SIZE : MapData::DUNGEON_SIZE;
		for (int y = 0; y < size; y += 8)
			for (int x = 0; x < size; x += 8)
				CHECK(md.chunkIndex(z, x, y) < MapData::CHUNK_COUNT);
	}

	// the tile pixel data: MAPTILES and MASKTYPE are one-item archives (a size header, then the table; flag 0xE0 or 2 means stored plain), OBJTILES and TILEINDX are plain
	{
		Bytes raw, mapTiles, maskType, objTiles, index;
		LibN lib;
		CHECK(readFile(joinPath(dir, "MAPTILES.VGA"), raw) && lib.load(raw, true) && lib.get(0, mapTiles));
		CHECK(readFile(joinPath(dir, "MASKTYPE.VGA"), raw) && lib.load(raw, true) && lib.get(0, maskType));
		CHECK(readFile(joinPath(dir, "OBJTILES.VGA"), objTiles) && readFile(joinPath(dir, "TILEINDX.VGA"), index));
		TileSet tiles;
		CHECK(tiles.load(mapTiles, maskType, objTiles, index));
	}

	// conversation scripts: TALK.LZC items are numbered by actor
	{
		::Ultima6::Ultima6::World w;
		w.game = std::string(dir).find("MARTIAN") != std::string::npos ? GameId::MartianDreams : GameId::SavageEmpire;
		CHECK(w.load(dir));
		int found = 0;
		for (int actor = 1; actor < 224; ++actor) {
			ConverseScript cs;
			if (w.script(actor, cs)) {
				++found;
				if (cs.actor() != actor) std::printf("item %d holds actor %d (%s)\n", actor, cs.actor(), cs.name().c_str());
			}
		}
		CHECK(found > 40);
		// every script runs to its first question and then ends on "bye"
		int asked = 0, unknown = 0;
		for (int actor = 1; actor < 224; ++actor) {
			ConverseScript cs;
			if (!w.script(actor, cs))
				continue;
			SpinStubHost h;
			Conversation c(cs.bytes(), actor, h);
			Conversation::State st = c.run();
			while (st == Conversation::NEED_KEY) {
				c.giveKey();
				st = c.run();
			}
			asked += st == Conversation::NEED_LINE;
			if (h.out.find("[Unknown command]") != std::string::npos) {
				std::printf("%s: the script of actor %d has a command the interpreter does not know\n", dir, actor);
				++unknown;
			}
		}
		std::printf("%s: %d scripts reach a question\n", dir, asked);
		CHECK(asked > found / 2);
		CHECK(unknown == 0);
		std::printf("%s: %d conversation scripts\n", dir, found);
	}

	int objects = 0, outside = 0;
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			ObjBlock blk;
			CHECK(readFile(joinPath(joinPath(dir, "SAVEGAME"), ObjBlock::surfaceFileName(col, row)), d) && blk.load(d));
			for (const ObjRecord &o : blk.objects) {
				++objects;
				outside += o.location() == ObjRecord::ON_MAP && (o.x >> 7 != col || o.y >> 7 != row);
			}
		}
	}
	CHECK(outside == 0);
	std::printf("%s: %d schedules, %d surface objects, all in their superchunks\n", dir, sched.total(), objects);
}

int main() {
	// the per-game tables are the games' own (tools/extract_creatures.py)
	{
		using ::Ultima6::Ultima6::Combat;
		using ::Ultima6::Ultima6::World;
		static World md;
		md.game = GameId::MartianDreams;
		if (md.load("md/game/MARTIAN")) {
			CHECK(Combat::stats(0x171) && Combat::stats(0x171)->hp == 45); // a Martian creature
			CHECK(!Combat::stats(0x16C) || Combat::stats(0x16C)->hp != 10); // not Ultima VI's acid slug
			CHECK(Combat::weaponDamageOf(0x2C) == 30 && Combat::rangeOf(0x2C) == 9 && Combat::armourOf(0x17) == 5);
		}
		static World se;
		se.game = GameId::SavageEmpire;
		if (se.load("se/game/SAVAGE"))
			{
				CHECK(Combat::stats(317) && Combat::stats(317)->hp == 255);
				CHECK(Combat::weaponDamageOf(0x28) == 35 && Combat::armourOf(0x43) == 7);
			}
		static World u6;
		if (u6.load("u6/game"))
			{
				CHECK(Combat::stats(364) && Combat::stats(364)->hp == 10); // the acid slug again
				CHECK(Combat::weaponDamageOf(43) == 15 && Combat::rangeOf(42) == 7);
			}
	}
	check("md/game/MARTIAN", 192);
	check("se/game/SAVAGE", 354);
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
