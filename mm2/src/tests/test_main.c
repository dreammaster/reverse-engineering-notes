/* Tests against the installed game data (set MM2_DIR or use the default GOG path).
 * Expected values come from the Python reference tools (the tools/ scripts) that were verified while reverse
 * engineering; see docs/. */
#include "../mm2_files.h"
#include "../mm2_gfx.h"
#include "../mm2_map.h"
#include "../mm2_monpic.h"
#include "../mm2_party.h"
#include "../mm2_reward.h"
#include "../mm2_smith.h"
#include "../mm2_spells.h"
#include "../mm2_town.h"
#include "../mm2_text.h"
#include "../mm2_battle.h"
#include "../mm2_combat.h"
#include "../mm2_data.h"
#include "../mm2_events.h"
#include "../mm2_game.h"
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
		{0, 8, 8, 'N', "TOWN", 0xc9106f11u},   {0, 13, 8, 'N', "TOWN", 0xe9566955u},
		{33, 8, 8, 'E', "CASTLE", 0x5c7aa316u}, {17, 8, 8, 'N', "CAVE", 0x46784c1du},
		{0, 11, 2, 'N', "TOWN", 0x0fe3dc77u},
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
		{8, 8, 'N', 0x64594d6fu}, {4, 10, 'E', 0x2e516abfu}, {12, 5, 'S', 0xa9625e22u}};
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

static void test_monster_pictures(const Mm2Game *g) {
	static const struct { int id, cga, frames; uint32_t hash; } cases[] = {
		{1, 0, 12, 0xd3e78801u}, {5, 0, 7, 0x87985a6cu}, {44, 0, 8, 0x3afbb894u}, {1, 1, 12, 0x93dce801u}};
	size_t i;
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		Mm2MonPic p;
		uint32_t h = 2166136261u;
		int k;
		CHECK(mm2_monpic_load(g, cases[i].id, cases[i].cga, &p));
		CHECK(p.frames == cases[i].frames);
		for (k = 0; k < p.frames; k++) {
			uint8_t f[MM2_MONPIC_W * MM2_MONPIC_H];
			size_t j;
			mm2_monpic_frame(&p, k, 0, f);
			for (j = 0; j < sizeof(f); j++)
				h = (h ^ f[j]) * 16777619u;
		}
		if (h != cases[i].hash) printf("  monpic %d: got %08x\n", (int)i, h);
		CHECK(h == cases[i].hash);
		mm2_monpic_free(&p);
	}
}

static void test_session(const Mm2Game *g) {
	Mm2GameSession s;
	/* walking onto the inn door of Middlegate runs script 2 ("Middlegate Inn") */
	CHECK(mm2_session_start(&s, g, 0, 6, 4, 'W'));
	CHECK(mm2_session_step(&s, 0));
	CHECK(s.x == 5 && s.y == 4);
	CHECK(s.nMessages == 1 && strcmp(s.messages[0].text, "Middlegate Inn") == 0 && s.messages[0].opcode == EV_TITLE);
	CHECK(strcmp(mm2_location_name(1), "Inn") == 0 && strcmp(mm2_location_name(6), "Blacksmith") == 0 && strcmp(mm2_location_name(0x11), "Entrance") == 0);
	/* turning and the wall rule */
	mm2_session_turn(&s, 1);
	CHECK(s.facing == 'N');
	mm2_session_turn(&s, -1);
	mm2_session_turn(&s, -1);
	CHECK(s.facing == 'S');
	mm2_session_end(&s);
	/* a yes/no script that teleports out of town (script 20 at 5,15 facing N) */
	CHECK(mm2_session_start(&s, g, 0, 5, 15, 'N'));
	s.yesNo = 1;
	CHECK(mm2_session_run_trigger(&s));
	CHECK(s.map == 11 && s.x == 7 && s.y == 3);
	mm2_session_end(&s);
	CHECK(mm2_session_start(&s, g, 0, 5, 15, 'N'));
	s.yesNo = 0;
	CHECK(mm2_session_run_trigger(&s));
	CHECK(s.map == 0 && s.x == 5 && s.y == 15);
	mm2_session_end(&s);
}

static void test_battle(const Mm2Game *g) {
	static Mm2Monster table[MM2_MONSTERS];
	Mm2Char a, b;
	Mm2Char *party[2] = {&a, &b};
	Mm2Rng lo = {rng_lo, 0};
	static const uint8_t ids[3] = {0, 1, 2};   /* speeds 20, 15, 12 */
	Mm2Battle bt;
	Mm2ActorKind k;
	int idx, seq = 0;
	static const int expect[5][2] = {{MM2_ACTOR_MONSTER, 0}, {MM2_ACTOR_PARTY, 1}, {MM2_ACTOR_MONSTER, 1}, {MM2_ACTOR_PARTY, 0}, {MM2_ACTOR_MONSTER, 2}};
	CHECK(mm2_load_monsters(g, table));
	memset(&a, 0, sizeof(a));
	memset(&b, 0, sizeof(b));
	a.raw[0x6E] = 14;
	b.raw[0x6E] = 18;
	mm2_battle_init(&bt, table, ids, 3, party, 2, 0, MM2_SURPRISE_NONE, &lo);
	CHECK(bt.frontMonsters == 3 && bt.frontParty == 2 && bt.count == 3);
	CHECK(bt.hp[0] == 5 && bt.speed[0] == 20 && bt.speed[2] == 12 && bt.usesLeft[0] >= 1);
	mm2_battle_start_round(&bt);
	while ((k = mm2_battle_next_actor(&bt, &idx)) != MM2_ACTOR_NONE) {
		CHECK(seq < 5 && (int)k == expect[seq][0] && idx == expect[seq][1]);
		mm2_battle_mark_acted(&bt, k, idx);
		seq++;
	}
	CHECK(seq == 5);
	mm2_battle_remove_monster(&bt, 0);
	CHECK(bt.count == 2 && bt.id[0] == 1 && bt.speed[0] == 15 && mm2_battle_visible(&bt) == 2);
	/* monster decisions */
	bt.frontMonsters = 1;
	mm2_battle_init(&bt, table, ids, 3, party, 2, 0, MM2_SURPRISE_NONE, &lo);
	bt.frontMonsters = 1;
	table[1].castChancePct = 0;
	table[2].castChancePct = 0;
	CHECK(mm2_monster_decide(&bt, 0, 0, 0, 0) == MM2_MON_MELEE);
	CHECK(mm2_monster_decide(&bt, 1, 0, 0, 0) == MM2_MON_ADVANCE);   /* back rank, not ranged */
	table[1].ranged = 1;
	CHECK(mm2_monster_decide(&bt, 1, 0, 0, 0) == MM2_MON_RANGED);   /* rng_lo: roll 1 <= 80 */
	bt.status[0] = MS_ASLEEP;
	CHECK(mm2_monster_decide(&bt, 0, 0, 0, 0) == MM2_MON_IDLE);
	bt.status[0] = 0;
	table[0].verb = 0;
	CHECK(mm2_monster_decide(&bt, 0, 9, 0, 0) == MM2_MON_FLEE);     /* strength 9 > tier table[verb] and roll 1 <= 50 */
	CHECK(mm2_monster_decide(&bt, 0, 9, 0, 1) == MM2_MON_MELEE);    /* summoned monsters never flee */
	table[0].castChancePct = 50;
	table[0].spell = 0x10;
	CHECK(mm2_monster_decide(&bt, 0, 0, 0, 0) == MM2_MON_CAST && bt.usesLeft[0] == table[0].specialUses - 1);
	bt.usesLeft[0] = 5;
	CHECK(mm2_monster_decide(&bt, 0, 0, 1, 0) == MM2_MON_CAST_FAILED);
	/* surprise: party surprised -> monsters double their front rank */
	mm2_battle_init(&bt, table, ids, 3, party, 2, 0, MM2_SURPRISE_PARTY, &lo);
	CHECK(bt.frontParty == 2 && bt.frontMonsters == 3);
	mm2_battle_init(&bt, table, ids, 3, party, 2, 0, MM2_SURPRISE_MONSTERS, &lo);
	CHECK(bt.frontParty == 1 && bt.frontMonsters == 2);
}

static void test_rewards(const Mm2Game *g) {
	static Mm2Item items[MM2_ITEMS];
	static Mm2Monster mons[MM2_MONSTERS];
	Mm2Rng lo = {rng_lo, 0}, hi = {rng_hi, 0};
	Mm2Loot loot = {0, 0, 0, 0, 0};
	Mm2TreasureItem t[3];
	Mm2Monster m;
	CHECK(mm2_load_items(g, items) && mm2_load_monsters(g, mons));
	memset(&m, 0, sizeof(m));
	m.goldClass = 1; m.dropsGems = 1; m.itemClass = 2; m.exp = 150;
	mm2_monster_reward(&loot, &m, 0x35, &lo);
	CHECK(loot.gold == 7 && loot.gems == 1 && loot.exp == 150 && loot.itemClass == 2 && loot.itemTier == 3);
	m.goldClass = 3;                                  /* id>>1 = 26, + rand(1,26) = 27 -> 27*256 + 7 */
	mm2_monster_reward(&loot, &m, 0x35, &lo);
	CHECK(loot.gold == 7 + 7 + 27 * 256 && loot.exp == 300);
	m.itemClass = 1;                                  /* a lower class never replaces the best one */
	mm2_monster_reward(&loot, &m, 0x71, &lo);
	CHECK(loot.itemClass == 2 && loot.itemTier == 3);
	loot.itemClass = 2;
	loot.itemTier = 0;
	CHECK(mm2_treasure_roll(&loot, items, t, &lo) == 3);
	CHECK(t[0].item == 1 && t[1].item == 1 && t[2].item == 1 && t[0].flags == 0);
	CHECK(mm2_treasure_roll(&loot, items, t, &hi) == 0);
	loot.itemTier = 5;                                /* tier >= 2 adds a magical bonus: rand(1,5) */
	CHECK(mm2_treasure_roll(&loot, items, t, &lo) == 3 && t[0].flags == 1);
	CHECK(mons[0].exp == 150);
}

static void test_smith(const Mm2Game *g) {
	static Mm2Item items[MM2_ITEMS];
	Mm2SmithSlot st[6];
	CHECK(mm2_load_items(g, items));
	/* stock tables checked against the dump in docs/shops.md */
	mm2_smith_stock(0, 1, 1, st);
	CHECK(st[0].item == 4 && st[1].item == 6 && st[5].item == 13 && st[0].bonus == 0);
	mm2_smith_stock(1, 1, 1, st);
	CHECK(st[0].item == 15 && st[0].bonus == 3 && st[5].bonus == 5);
	mm2_smith_stock(1, 3, 1, st);
	CHECK(st[0].item == 155 && st[0].bonus == 4 && st[1].item == 117);
	mm2_smith_stock(0, 4, 1, st);
	CHECK(st[0].item == 161 && st[0].charges == 1 && st[1].charges == 20 && st[0].bonus == 0);
	mm2_smith_stock(2, 2, 1, st);     /* day-dependent bonus: day 1 -> DAY_BONUS[1] */
	CHECK(st[0].item == 96 && st[0].bonus == 1);
	mm2_smith_stock(2, 2, 29, st);    /* day 29 mod 30 = 29 -> special table[0] */
	CHECK(st[0].bonus == 5);
	/* prices (docs/shops.md): Dagger costs 8, a +1 item costs 2P, a +3 item 2P + 2000 */
	CHECK(mm2_smith_price(&items[4], 0, MM2_SMITH_BUY_A, 0) == 8);
	CHECK(mm2_smith_price(&items[4], 1, MM2_SMITH_BUY_A, 0) == 16);
	CHECK(mm2_smith_price(&items[4], 3, MM2_SMITH_BUY_A, 0) == 16 + 2000);
	CHECK(mm2_smith_price(&items[4], 3, MM2_SMITH_BUY_A, 1) == (16 + 2000) / 2);
	CHECK(mm2_smith_price(&items[4], 0, MM2_SMITH_SELL, 0) == 2 && mm2_smith_price(&items[4], 0, MM2_SMITH_SELL, 1) == 4);
	CHECK(mm2_smith_price(&items[4], 0, MM2_SMITH_IDENTIFY, 0) == 10 && mm2_smith_price(&items[4], 4, MM2_SMITH_IDENTIFY, 0) == 400);
}

static void test_town(void) {
	int sp[4], n;
	uint32_t pr[4];
	Mm2Char c;
	CHECK(mm2_price_decode(10) == 10 && mm2_price_decode(129) == 1000 && mm2_price_decode(37) == 50 && mm2_price_decode(65) == 100);
	CHECK(mm2_price_decode(148) == 20000 && mm2_price_decode(165) == 50000 && mm2_price_decode(170) == 100000);
	n = mm2_temple_stock(0, sp, pr);                    /* Apparition 10, Awaken 10, Power Cure 1000 */
	CHECK(n == 3 && sp[0] == 48 && sp[2] == 53 && pr[0] == 10 && pr[2] == 1000);
	n = mm2_temple_stock(1, sp, pr);                    /* Mass Distortion 20000, Resurrection 50000, Uncurse Item 100000 */
	CHECK(n == 3 && sp[0] == 90 && pr[0] == 20000 && pr[1] == 50000 && pr[2] == 100000);
	n = mm2_guild_stock(0, sp, pr);                     /* Awaken 10, Energy Blast 1000, Sleep 50, Identify Monster 100 */
	CHECK(n == 4 && sp[1] == 2 && pr[1] == 1000 && pr[2] == 50 && pr[3] == 100);
	memset(&c, 0, sizeof(c));
	c.raw[MC_LEVEL] = 4;
	CHECK(mm2_temple_restore_cost(&c, 0) == 0);
	c.raw[MC_HP_MAX] = 20; c.raw[MC_HP] = 5;
	CHECK(mm2_temple_restore_cost(&c, 1) == 10u * 4 * 5);
	c.raw[MC_CONDITION] = 0x81;
	CHECK(mm2_temple_restore_cost(&c, 2) == 100u * 4 * 2);
	c.raw[MC_CONDITION] = 0xFF;
	CHECK(mm2_temple_restore_cost(&c, 0) == 4000u);
	c.raw[MC_ALIGN] = 2; c.raw[MC_ORIG_ALIGN] = 0;
	CHECK(mm2_temple_alignment_cost(&c, 3) == 100u * 4 * 3 && mm2_temple_donation_cost(1) == 500);
}

static void test_spells(const Mm2Game *g) {
	static Mm2Spell spells[MM2_SPELLS];
	Mm2Rng lo = {rng_lo, 0};
	Mm2CombatSpell cs;
	Mm2Char c;
	CHECK(mm2_load_spells(g, spells));
	CHECK(strcmp(MM2_SPELL_NAMES[2], "Energy Blast") == 0 && strcmp(MM2_SPELL_NAMES[95], "Uncurse Item") == 0 && MM2_SPELL_LEVEL[95] == 9);
	CHECK(mm2_spell_sp_cost(spells, 0, 5, 3) == 1);                 /* Awaken: 1 SP */
	CHECK(mm2_spell_sp_cost(spells, 2, 7, 3) == 7);                 /* Energy Blast: 1 x caster level */
	CHECK(mm2_spell_sp_cost(spells, 42, 12, 15) == 8 + 15);         /* Meteor Shower: 8 + one per monster */
	CHECK(mm2_spell_sp_cost(spells, 46, 12, 4) == 10 + 4);          /* Star Burst: 10 + one per monster */
	CHECK(mm2_combat_spell(17, &cs) && cs.targets == 4 && cs.element == 2);
	CHECK(mm2_combat_spell_damage(&cs, 6, &lo) == 6 * 2);           /* rand low: 1 + 1 per level */
	CHECK(mm2_combat_spell(44, &cs) && mm2_combat_spell_damage(&cs, 9, &lo) == 1000);
	CHECK(!mm2_combat_spell(0, &cs));
	memset(&c, 0, sizeof(c));
	c.raw[MC_HP] = 3; c.raw[MC_HP_MAX] = 10; c.raw[MC_CONDITION] = 0x50;   /* unconscious + asleep */
	CHECK(mm2_heal_character(&c, mm2_heal_amount(51, 5, &lo)));            /* First Aid: 8 */
	CHECK(mm2_c16(&c, MC_HP) == 10 && c.raw[MC_CONDITION] == 0);
	c.raw[MC_CONDITION] = 0x81;
	CHECK(!mm2_heal_character(&c, 5));
	CHECK(mm2_heal_amount(53, 4, &lo) == 4 && mm2_heal_amount(55, 1, &lo) == 15);
}

int main(void) {
	Mm2Game g;
	mm2_game_init(&g, NULL);
	test_lzw_files(&g);
	test_maps_and_events(&g);
	test_map_rules(&g);
	test_tables_and_rules(&g);
	test_event_vm(&g);
	test_session(&g);
	test_monster_pictures(&g);
	test_combat();
	test_battle(&g);
	test_rewards(&g);
	test_smith(&g);
	test_town();
	test_spells(&g);
	test_text(&g);
	test_banks(&g);
	test_indoor_render(&g);
	test_outdoor_render(&g);
	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
