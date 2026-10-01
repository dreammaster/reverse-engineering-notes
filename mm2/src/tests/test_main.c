/* Tests against the installed game data (set MM2_DIR or use the default GOG path).
 * Expected values come from the Python reference tools (the tools/ scripts) that were verified while reverse
 * engineering; see docs/. */
#include "../mm2_files.h"
#include "../mm2_gfx.h"
#include "../mm2_map.h"
#include "../mm2_data.h"
#include "../mm2_tables.h"
#include "../mm2_view.h"

#include <stdio.h>
#include <string.h>

static int failures = 0, checks = 0;

#define CHECK(cond)                                                          \
	do {                                                                     \
		checks++;                                                            \
		if (!(cond)) {                                                       \
			failures++;                                                      \
			printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
		}                                                                    \
	} while (0)

static uint32_t fnv(const uint8_t *p, size_t n) {
	uint32_t h = 2166136261u;
	size_t i;
	for (i = 0; i < n; i++)
		h = (h ^ p[i]) * 16777619u;
	return h;
}

static void test_lzw_files(const Mm2Game *g) {
	Mm2Blob b = mm2_load_lzw_file(g, "MONSTERS.DAT");
	CHECK(b.data && b.size == 6656);
	mm2_blob_free(&b);
	b = mm2_load_lzw_file(g, "ATTRIB.DAT");
	CHECK(b.data && b.size == 3840);
	mm2_blob_free(&b);
	b = mm2_load_lzw_file(g, "STR.DAT");
	CHECK(b.data && b.size > 0);
	mm2_blob_free(&b);
}

static void test_maps_and_events(const Mm2Game *g) {
	int m, withEvents = 0;
	for (m = 0; m < MM2_MAPS; m++) {
		uint8_t map[512];
		Mm2Blob ev;
		CHECK(mm2_load_map(g, m, map));
		ev = mm2_load_events(g, m);
		if (ev.data) {
			Mm2EventChunk c;
			CHECK(mm2_parse_events(&ev, &c));
			CHECK(c.scriptsLen > 0);
			withEvents++;
		}
		mm2_blob_free(&ev);
	}
	CHECK(withEvents > 40);
}

static void test_banks(const Mm2Game *g) {
	static const char *names[] = {"TOWN",     "TOWNF",   "TOWNT",   "TOWNB", "SKY",      "CAVE",    "CASTLE",
								  "OUTDOOR1", "OUTDOOR2", "OUTDOOR3", "OUTF",  "DESERT",   "OCEAN",   "SWAMP",
								  "TUNDRA",   "BOOK",    "GLOBE",   "DISK",  "MASTER",   "NWCP"};
	size_t i;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		char file[64];
		Mm2Bank b;
		int k, okAll = 1;
		snprintf(file, sizeof(file), "%s.16", names[i]);
		if (!mm2_bank_load(g, file, 4, &b)) {
			printf("cannot load %s\n", file);
			CHECK(0);
			continue;
		}
		for (k = 0; k < b.count; k++) {
			const Mm2Image *im = mm2_bank_image(&b, k);
			if (!im || im->w <= 0 || im->h <= 0) okAll = 0;
		}
		CHECK(okAll);
		mm2_bank_free(&b);
	}
}

typedef struct {
	int map, x, y;
	char facing;
	const char *style;
	uint32_t hash;
} IndoorCase;

static void test_indoor_render(const Mm2Game *g) {
	static const IndoorCase cases[] = {
		{0, 8, 8, 'N', "TOWN", 0x8148cf60u},   {0, 13, 8, 'N', "TOWN", 0xe4e91d3du},
		{33, 8, 8, 'E', "CASTLE", 0x878f51fcu}, {17, 8, 8, 'N', "CAVE", 0x46784c1du},
		{0, 11, 2, 'N', "TOWN", 0x59814abeu},
	};
	static uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	size_t i;
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		uint8_t map[512];
		Mm2View v;
		CHECK(mm2_load_map(g, cases[i].map, map));
		CHECK(mm2_view_load_indoor(&v, g, cases[i].style));
		mm2_view_render_indoor(&v, canvas, map, cases[i].x, cases[i].y, cases[i].facing);
		if (fnv(canvas, sizeof(canvas)) != cases[i].hash)
			printf("  indoor case %d: got %08x\n", (int)i, fnv(canvas, sizeof(canvas)));
		CHECK(fnv(canvas, sizeof(canvas)) == cases[i].hash);
		mm2_view_free(&v);
	}
}

static void test_outdoor_render(const Mm2Game *g) {
	static const struct { int x, y; char f; uint32_t hash; } cases[] = {
		{8, 8, 'N', 0x9964da3cu}, {4, 10, 'E', 0xca8dace9u}, {12, 5, 'S', 0x8345ed1du}};
	static uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	uint8_t map[512];
	Mm2View v;
	size_t i;
	CHECK(mm2_load_map(g, 5, map));
	CHECK(mm2_view_load_outdoor(&v, g, "OCEAN"));
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		mm2_view_render_outdoor(&v, canvas, map, cases[i].x, cases[i].y, cases[i].f);
		if (fnv(canvas, sizeof(canvas)) != cases[i].hash)
			printf("  outdoor case %d: got %08x\n", (int)i, fnv(canvas, sizeof(canvas)));
		CHECK(fnv(canvas, sizeof(canvas)) == cases[i].hash);
	}
	mm2_view_free(&v);
}

static void test_map_rules(const Mm2Game *g) {
	uint8_t map[512];
	int blocked = 0, x, y;
	CHECK(mm2_map_style(0) == 0 && mm2_map_style(5) == 3 && mm2_map_style(17) == 1 && mm2_map_style(33) == 6);
	CHECK(mm2_map_style(41) == 4 && mm2_map_style(45) == 5 && mm2_map_style(59) == 2);
	CHECK(mm2_style_is_outdoor(3) && mm2_style_is_outdoor(6) && !mm2_style_is_outdoor(0));
	/* Town map 0 (docs/file-formats.md): of the 273 drawn walls 260 are blocked. */
	CHECK(mm2_load_map(g, 0, map));
	for (y = 0; y < 16; y++)
		for (x = 0; x < 16; x++) {
			const char sides[] = "NESW";
			int k;
			for (k = 0; k < 4; k++)
				if (mm2_wall_side(map[y * 16 + x], sides[k]) == 1 && !mm2_can_step(map, x, y, sides[k]))
					blocked++;
		}
	CHECK(blocked == 260);
}

static void test_tables_and_rules(const Mm2Game *g) {
	static Mm2Item items[MM2_ITEMS];
	static Mm2Monster mons[MM2_MONSTERS];
	static Mm2Spell spells[MM2_SPELLS];
	static Mm2Roster roster;
	int i;
	CHECK(mm2_load_items(g, items));
	CHECK(strcmp(items[4].name, "Dagger") == 0 && items[4].price == 8 && items[4].value == 4);
	CHECK(strcmp(items[99].name, "Cinder Pipe") == 0 && items[99].useEffect == 151 && items[99].price == 2500);
	CHECK(mm2_item_kind(1) == MM2_ITEM_ONEHAND && mm2_item_kind(66) == MM2_ITEM_TWOHAND && mm2_item_kind(120) == MM2_ITEM_SHIELD &&
		  mm2_item_kind(160) == MM2_ITEM_MISC);
	CHECK(mm2_load_monsters(g, mons));
	CHECK(strcmp(mons[0].name, "Creepy Crawler") == 0 && mons[0].hp == 5 && mons[0].exp == 150 && mons[0].ac == 4);
	CHECK(mons[0].speed == 20 && mons[0].blows == 2 && mons[0].damageDie == 6 && mons[0].groupSize == 6 && mons[0].picture == 1);
	CHECK(mons[0].touch == 3 /* poison */ && mons[7].magicResistPct == 10 && mons[7].picture == 27);
	CHECK(mm2_load_spells(g, spells));
	CHECK(spells[1].usage == 2 && spells[2].usage == 1 && spells[2].gems == 1);   /* Detect Magic non-combat, Energy Blast combat */
	CHECK(mm2_load_roster(g, &roster));
	for (i = 0; i < MM2_ROSTER_CHARS; i++)
		if (roster.chars[i].raw[MC_NAME]) {
			CHECK(mm2_c8(&roster.chars[i], MC_CLASS) < 8 && mm2_c8(&roster.chars[i], MC_RACE) < 5);
			CHECK(mm2_c16(&roster.chars[i], MC_HP_MAX) >= mm2_c16(&roster.chars[i], MC_HP) || mm2_c8(&roster.chars[i], MC_CONDITION) != 0);
		}
	/* rules: values from tools/mm2_rules.py */
	CHECK(mm2_exp_for_level(0, 2) == 1500 && mm2_exp_for_level(0, 5) == 12000 && mm2_exp_for_level(0, 10) == 384000);
	CHECK(mm2_exp_for_level(0, 11) == 576000 && mm2_exp_for_level(0, 25) == 13248000 && mm2_exp_for_level(0, 80) == 154048000u);
	CHECK(mm2_exp_for_level(1, 2) == 2000 && mm2_exp_for_level(1, 60) == 98880000u && mm2_exp_for_level(3, 16) == 2496000);
	CHECK(mm2_training_cost(0, 5) == 250 && mm2_training_cost(1, 5) == 1250 && mm2_training_cost(3, 5) == 750);
	CHECK(mm2_bracket(1) == -3 && mm2_bracket(15) == 1 && mm2_bracket(255) >= 18);
}

int main(void) {
	Mm2Game g;
	mm2_game_init(&g, NULL);
	test_lzw_files(&g);
	test_maps_and_events(&g);
	test_map_rules(&g);
	test_tables_and_rules(&g);
	test_banks(&g);
	test_indoor_render(&g);
	test_outdoor_render(&g);
	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
