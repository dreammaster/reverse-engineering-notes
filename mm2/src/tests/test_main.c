/* Tests against the installed game data (set MM2_DIR or use the default GOG path).
 * Expected values come from the Python reference tools (the tools/ scripts) that were verified while reverse
 * engineering; see docs/. */
#include "../mm2_files.h"
#include "../mm2_gfx.h"
#include "../mm2_map.h"
#include "../mm2_party.h"
#include "../mm2_text.h"
#include "../mm2_combat.h"
#include "../mm2_data.h"
#include "../mm2_events.h"
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
	{
		/* save/load round trip: the serialised roster equals the shipped file */
		uint8_t out[0x1860 + 2052];
		Mm2Blob orig = mm2_read_file(g, "ROSTER.DAT");
		CHECK(orig.data && orig.size >= sizeof(out));
		CHECK(mm2_roster_to_bytes(&roster, out, sizeof(out)) == sizeof(out));
		CHECK(memcmp(out, orig.data, sizeof(out)) == 0);
		mm2_blob_free(&orig);
	}
	/* character creation reproduces the shipped level-1 characters (Gene Eric exactly; the others differ only in
	 * equipment-dependent fields: AC, food, backpack, gold, location) */
	for (i = 0; i < MM2_ROSTER_CHARS; i++) {
		const Mm2Char *c = &roster.chars[i];
		if (c->raw[MC_NAME] && c->raw[MC_LEVEL] == 1 && c->raw[MC_AGE] == 18 && i < 6) {
			Mm2NewChar n;
			Mm2Char m;
			int k, diffs = 0;
			memset(&n, 0, sizeof(n));
			n.cls = c->raw[MC_CLASS]; n.race = c->raw[MC_RACE]; n.alignment = c->raw[MC_ALIGN]; n.sex = c->raw[MC_SEX];
			memcpy(n.name, c->raw, 11);
			n.stats[0] = c->raw[0x10]; n.stats[1] = c->raw[0x11]; n.stats[2] = c->raw[0x12]; n.stats[3] = c->raw[0x27];
			n.stats[4] = c->raw[0x13]; n.stats[5] = c->raw[0x14]; n.stats[6] = c->raw[0x15];
			mm2_create_character(&m, &n);
			for (k = 0; k < MM2_CHAR_SIZE; k++)
				if (m.raw[k] != c->raw[k] && k != MC_AC && k != MC_FOOD && k != MC_PACK_ID && k != MC_GOLD && k != MC_TOWN) diffs++;
			CHECK(diffs == 0);
			if (i == 3) CHECK(memcmp(m.raw, c->raw, MM2_CHAR_SIZE) == 0);
		}
	}
	CHECK(mm2_roster_find_free(&roster) == -1 || roster.chars[mm2_roster_find_free(&roster)].raw[MC_NAME] == 0);
	CHECK(mm2_race_stat_adjust(1, 0) == -1 && mm2_race_stat_adjust(3, 6) == 2 && mm2_race_stat_adjust(0, 3) == 0);
	/* rules: values from tools/mm2_rules.py */
	CHECK(mm2_exp_for_level(0, 2) == 1500 && mm2_exp_for_level(0, 5) == 12000 && mm2_exp_for_level(0, 10) == 384000);
	CHECK(mm2_exp_for_level(0, 11) == 576000 && mm2_exp_for_level(0, 25) == 13248000 && mm2_exp_for_level(0, 80) == 154048000u);
	CHECK(mm2_exp_for_level(1, 2) == 2000 && mm2_exp_for_level(1, 60) == 98880000u && mm2_exp_for_level(3, 16) == 2496000);
	CHECK(mm2_training_cost(0, 5) == 250 && mm2_training_cost(1, 5) == 1250 && mm2_training_cost(3, 5) == 750);
	CHECK(mm2_bracket(1) == -3 && mm2_bracket(15) == 1 && mm2_bracket(255) >= 18);
}

static int g_msgs, g_ops;
static int count_exec(void *ud, Mm2Vm *vm, int op, const uint8_t *args) {
	(void)ud; (void)vm; (void)args;
	g_ops++;
	if (op == EV_MSG) g_msgs++;
	return 1;
}
static int fixed_rand(void *ud, int lo, int hi) {
	(void)ud; (void)hi;
	return lo;
}

static void test_event_vm(const Mm2Game *g) {
	static const uint8_t script[] = {
		EV_SET_VAR, 3, 5, EV_VAR_COND, 3, 0, EV_SKIP_IF_NOT, 1, EV_MSG, 1, EV_END, 0xFF,   /* script 0 */
		EV_VAR_COND, 4, 0, EV_SKIP_IF_NOT, 1, EV_MSG, 2, EV_COND_RAND, 9, EV_MSG, 3, 0xFF, /* script 1 */
	};
	static Mm2State st;
	Mm2EventHost host = {count_exec, fixed_rand, 0};
	Mm2Vm vm = {0};
	Mm2EventChunk c;
	Mm2Blob ev;
	int m;
	vm.state = &st;
	vm.host = &host;
	g_msgs = g_ops = 0;
	CHECK(mm2_vm_run(&vm, script, sizeof(script), 0) == 5);
	CHECK(*mm2_state_ptr(&st, mm2_event_var_dgroup(3)) == 5 && vm.cond == 5 && g_msgs == 1);
	g_msgs = 0;
	CHECK(mm2_vm_run(&vm, script, sizeof(script), 1) == 4);   /* var 4 = 0 -> skip the first message, then random + message */
	CHECK(vm.cond == 1 && g_msgs == 1);
	CHECK(mm2_vm_run(&vm, script, sizeof(script), 2) == -1);
	CHECK(mm2_event_op_len(EV_FIGHT) == 13 && mm2_event_op_len(EV_PLACE_TREASURE) == 15 && mm2_event_op_len(0) == 0);
	CHECK(mm2_event_var_dgroup(0x84) == 0x3CA && mm2_event_var_dgroup(0x23) == 0x3D8 && mm2_event_var_dgroup(0x50) == 0);
	/* every shipped script runs to its end with a do-nothing host */
	for (m = 0; m < MM2_MAPS; m++) {
		ev = mm2_load_events(g, m);
		if (ev.data && mm2_parse_events(&ev, &c)) {
			int s, n = 0;
			size_t p = 0;
			while (p < c.scriptsLen) {
				if (c.scripts[p] == 0xFF) { n++; p++; }
				else p += (size_t)mm2_event_op_len(c.scripts[p]);
			}
			for (s = 0; s < n; s++)
				CHECK(mm2_vm_run(&vm, c.scripts, c.scriptsLen, s) >= 0);
			if (m == 0) {
				CHECK(mm2_event_find_trigger(&c, 8, 0, 'W') == 29 && mm2_event_find_trigger(&c, 8, 0, 'N') == -1);
				CHECK(mm2_event_find_trigger(&c, 8, 1, 'S') == 41);
			}
		}
		mm2_blob_free(&ev);
	}
}

static int rng_lo(void *ud, int lo, int hi) { (void)ud; (void)hi; return lo; }
static int rng_hi(void *ud, int lo, int hi) { (void)ud; (void)lo; return hi; }

static void test_combat(void) {
	Mm2Rng lo = {rng_lo, 0}, hi = {rng_hi, 0};
	Mm2AttackMods mods = {0, 0, 0};
	Mm2Char c;
	Mm2AttackResult r;
	Mm2Monster mon;
	Mm2MonsterAttackResult mr;
	memset(&c, 0, sizeof(c));
	memset(&mon, 0, sizeof(mon));
	c.raw[MC_CLASS] = MM2_KNIGHT;
	c.raw[MC_LEVEL] = 10;
	c.raw[MC_CUR_STATS] = 15;        /* Might: bracket 1 */
	c.raw[MC_CUR_STATS + 4] = 15;    /* Accuracy: bracket 1 */
	c.raw[0x4C] = 8;                 /* weapon dice */
	c.raw[0x4D] = 2;                 /* weapon bonus */
	r = mm2_party_attack(&c, 30, 0, &mods, &lo);          /* d100 = 1 always hits */
	CHECK(r.swings == 3 && r.hits == 3 && r.damage == 12 && r.kind == MM2_HIT_NORMAL);
	r = mm2_party_attack(&c, 30, 0, &mods, &hi);          /* d100 = 100, to-hit 35 + 3 >= AC 30 */
	CHECK(r.hits == 3 && r.damage == 33);
	r = mm2_party_attack(&c, 40, 0, &mods, &hi);          /* AC 40 > 38: all miss */
	CHECK(r.hits == 0 && r.damage == 0);
	mods.damageBonus = 5;
	r = mm2_party_attack(&c, 30, 0, &mods, &hi);
	CHECK(r.damage == 38);
	mods.hitFloor = 60;                                    /* roll 38 is below the floor */
	r = mm2_party_attack(&c, 30, 0, &mods, &hi);
	CHECK(r.hits == 0);
	mods.hitFloor = 0;
	c.raw[MC_CLASS] = MM2_ROBBER;                          /* swings = 10/5+1 = 2, back stab on roll 100 */
	r = mm2_party_attack(&c, 30, 0, &mods, &hi);
	CHECK(r.swings == 3);
	CHECK(r.kind == MM2_HIT_BACKSTAB);
	/* monsters */
	mon.blows = 2;
	mon.damageDie = 6;
	CHECK(mm2_monster_hit_chance(0, 10) == 30 && mm2_monster_hit_chance(0, 99) == 5 && mm2_monster_hit_chance(13, 0) == 250);
	mr = mm2_monster_melee(&mon, 0, 10, 0, 0, &lo);
	CHECK(mr.blows == 2 && mr.hits == 2 && mr.damage == 2);
	mr = mm2_monster_melee(&mon, 0, 10, 0, 0, &hi);        /* roll 100 > 30: miss */
	CHECK(mr.hits == 0);
	mr = mm2_monster_melee(&mon, 0, 10, 0, 1, &lo);
	CHECK(mr.damage == 1);
	CHECK(mm2_spell_damage_roll(5, 5, 1, &lo) == 10 && mm2_spell_damage_roll(5, 0, 6, &lo) == 30 && mm2_spell_damage_roll(5, 5, 1, &hi) == 30);
}

static void test_text(const Mm2Game *g) {
	static Mm2Font font;
	static uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	int i, on = 0;
	CHECK(mm2_font_load(g, &font));
	mm2_draw_text(canvas, &font, 2, 3, "A", 15, -1);
	for (i = 0; i < MM2_SCREEN_W * MM2_SCREEN_H; i++)
		on += canvas[i] == 15;
	CHECK(on == 31);                                        /* the 'A' glyph has 31 pixels */
	CHECK(canvas[(3 * 8) * MM2_SCREEN_W + 2 * 8 + 2] == 15); /* row 0 of 'A' is ..###... */
	CHECK(canvas[(3 * 8) * MM2_SCREEN_W + 2 * 8 + 0] == 0);
}

int main(void) {
	Mm2Game g;
	mm2_game_init(&g, NULL);
	test_lzw_files(&g);
	test_maps_and_events(&g);
	test_map_rules(&g);
	test_tables_and_rules(&g);
	test_event_vm(&g);
	test_combat();
	test_text(&g);
	test_banks(&g);
	test_indoor_render(&g);
	test_outdoor_render(&g);
	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
