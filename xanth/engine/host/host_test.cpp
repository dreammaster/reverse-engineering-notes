// Runs the lifted Xanth game logic against the original data and checks what it does.
//
//   host_test <gamedir> <image.bin> <xanth_objects.tsv> objects      lifted object accessors vs the independently decoded object table
//   host_test <gamedir> <image.bin> <xanth_objects.tsv> scan         every handler x phase x verb: argument checks on everything the code calls
//   host_test <gamedir> <image.bin> <xanth_objects.tsv> look [room]  run_command(LOOK_AT) on the objects of a room, printing the texts
//   host_test <gamedir> <image.bin> <xanth_objects.tsv> rooms        enter every room (go_to_room) and look at everything in it; coverage of the message file
//
// "scan" and "rooms" print statistics and the problems found, but no game text.
#include "host.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

struct Obj {
	int id = 0, type = 0, loc = -1, handler = 0, prop_a = 0, verb = 0;
	std::string name;
	std::set<int> attrs;
};
static std::vector<Obj> objs;   // index = object id
static int g_count = 0;

static std::vector<std::string> split(const std::string &s, char d) {
	std::vector<std::string> out;
	std::string cur;
	for (char ch : s) {
		if (ch == d) { out.push_back(cur); cur.clear(); }
		else cur.push_back(ch);
	}
	out.push_back(cur);
	return out;
}

static void loadObjects(const std::string &tsv) {
	std::ifstream f(tsv);
	std::string line;
	std::getline(f, line);
	objs.assign(1, Obj());
	while (std::getline(f, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();
		auto c = split(line, '\t');
		if (c.size() < 12) continue;
		Obj o;
		o.id = atoi(c[0].c_str());
		o.type = atoi(c[1].c_str());
		o.name = c[2];
		o.loc = c[4].empty() ? -1 : atoi(c[4].c_str());
		o.handler = c[7].empty() ? 0 : atoi(c[7].c_str());
		o.verb = c[8].empty() ? 0 : atoi(c[8].c_str());
		o.prop_a = c[10].empty() ? 0 : atoi(c[10].c_str());
		for (auto &a : split(c[11], ','))
			if (!a.empty()) o.attrs.insert(atoi(a.c_str()));
		if ((size_t)o.id >= objs.size()) objs.resize(o.id + 1);
		objs[o.id] = o;
	}
}

// the state execute_command sets up before run_command
static void setupCommand(Host &h, int verb, int dobj, int iobj) {
	h.setw(D_actor_obj, h.w(D_player_obj));
	h.setw(D_selectedCommand, (uint16_t)verb);
	h.setw(D_dobj, (uint16_t)dobj);
	h.setw(D_iobj, (uint16_t)iobj);
	h.setw(D_dobj_type, dobj > 0 && dobj < (int)objs.size() ? objs[dobj].type : 0);
	h.setw(D_iobj_type, iobj > 0 && iobj < (int)objs.size() ? objs[iobj].type : 0);
}

// --- argument checks of the calls recorded while a scenario ran --------------------------------------------------------------------------------
static bool validObj(int v, bool allowZero = false) { return (allowZero && v == 0) || (v >= 1 && v <= g_count); }
static bool validConv(int v) { return v < 0x40C || ((v & 0x8000) && (v & 0x7FFF) < 0x190); }

static std::map<std::string, long> g_checked;   // call name -> number of calls whose arguments were checked
static std::set<uint32_t> g_messagesSeen;

// print_obj / the list accessors are guarded against object 0 in the original (they return/print nothing), so a 0 is not an error for them
static void checkCalls(Host &h, const std::string &ctx) {
	for (const Event &e : h.calls) {
		const std::string &n = e.name;
		auto bad = [&](const char *what) {
			char b[240];
			snprintf(b, sizeof(b), "%s: %s(%X, %X, %X): %s", ctx.c_str(), n.c_str(), e.w[0], e.w[1], e.w[2], what);
			h.problem(b);
		};
		bool known = true;
		if (n == "object_move?") {
			if (!validObj(e.w[0]) || !validObj(e.w[1], true)) bad("object id out of range");
		} else if (n == "object_has_attr?" || n == "object_set_attr?" || n == "object_clear_attr?") {
			if (!validObj(e.w[0])) bad("object id out of range");
			if (e.w[1] < 1 || e.w[1] > 23) bad("attribute id out of range");
		} else if (n == "object_parent_is?" || n == "object_is_within?") {
			if (!validObj(e.w[0], true) || !validObj(e.w[1], true)) bad("object id out of range");
		} else if (n == "object_get_prop_a?" || n == "object_get_location?" || n == "object_get_next_sibling?" || n == "object_get_list_head?" || n == "object_set_prop_a?") {
			if (!validObj(e.w[0], true)) bad("object id out of range");
		} else if (n == "object_set_default_verb?") {
			if (!validObj(e.w[0]) || e.w[1] > 92) bad("object or verb out of range");
		} else if (n == "go_to_room?") {
			if (!validObj(e.w[0]) || objs[e.w[0]].type != 1) bad("not a room");
		} else if (n == "schedule_event?" || n == "cancel_event" || n == "event_pending") {
			if (e.w[0] < 1 || e.w[0] > 45) bad("event id out of range");
		} else if (n == "call_object_handler") {
			if (!validObj(e.w[0], true)) bad("object id out of range");
		} else if (n == "print_obj") {
			if (!validObj(e.w[1], true)) bad("object id out of range");
		} else if (n == "conversation_set_enabled" || n == "Conversation_Show") {
			if (!validConv(e.w[0])) bad("conversation id out of range");
		} else {
			known = false;
		}
		if (known) g_checked[n]++;
	}
	// text output: every message id must exist, every %s argument must point at a NUL-terminated string (Font_WriteMessage reports these through h.problem)
	for (const Event &e : h.events)
		if ((e.name == "Font_WriteMessage" || e.name == "ResourceText_GetStr" || e.name == "show_text_box") && (e.w[1] >> 8) == 0xF0)
			g_messagesSeen.insert(((uint32_t)e.w[1] << 16) | e.w[0]);
}

// --- tests -----------------------------------------------------------------------------------------------------------------------------------
static int testObjects(Host &h) {
	h.reset();
	g_count = h.w(D_object_count);
	printf("object_count in image: %d, in object table: %zu\n", g_count, objs.size() - 1);
	int bad = 0, checks = 0;
	for (int o = 1; o <= g_count && o < (int)objs.size(); o++)
		for (int a = 1; a <= 23; a++) {
			int got = h.call("object_has_attr_q", {(uint16_t)o, (uint16_t)a}) & 0xFFFF;
			checks++;
			if (((got != 0) != (objs[o].attrs.count(a) > 0)) && bad++ < 10)
				printf("  attr mismatch: object %d (%s) attr %d: lifted %d\n", o, objs[o].name.c_str(), a, got);
		}
	printf("object_has_attr?: %d checks, %d mismatches\n", checks, bad);
	int locBad = 0, locChecks = 0, propBad = 0, propChecks = 0;
	for (int o = 1; o <= g_count && o < (int)objs.size(); o++) {
		if (objs[o].loc >= 0 && (objs[o].type == 2 || objs[o].type == 3 || objs[o].type == 5)) {
			int got = h.call("object_get_location_q", {(uint16_t)o}) & 0xFFFF;
			locChecks++;
			if (got != objs[o].loc && locBad++ < 8) printf("  location mismatch: object %d (%s): lifted %d, table %d\n", o, objs[o].name.c_str(), got, objs[o].loc);
		}
		if (objs[o].type != 4) {
			int got = h.call("object_get_prop_a_q", {(uint16_t)o}) & 0xFFFF;
			propChecks++;
			if (got != objs[o].prop_a && propBad++ < 8) printf("  prop_a mismatch: object %d (%s): lifted %d, table %d\n", o, objs[o].name.c_str(), got, objs[o].prop_a);
		}
	}
	printf("object_get_location?: %d checks, %d mismatches;  object_get_prop_a?: %d checks, %d mismatches\n", locChecks, locBad, propChecks, propBad);
	int fwd = 0, rev = 0, n = 0;
	for (int o = 1; o <= g_count && o < (int)objs.size(); o++) {
		if (objs[o].loc <= 0 || objs[o].type == 1) continue;
		n++;
		fwd += (h.call("object_parent_is_q", {(uint16_t)o, (uint16_t)objs[o].loc}) & 0xFFFF) != 0;
		rev += (h.call("object_parent_is_q", {(uint16_t)objs[o].loc, (uint16_t)o}) & 0xFFFF) != 0;
	}
	printf("object_parent_is?(x, location(x)) true for %d of %d objects; reversed order true for %d\n", fwd, n, rev);
	return bad + locBad + propBad + (fwd != n);
}

static void reportProblems(std::vector<std::string> &all, size_t show) {
	std::map<std::string, int> kinds;
	std::map<std::string, std::string> example;
	for (auto &p : all) {
		std::string key = p.substr(p.find(": ") == std::string::npos ? 0 : p.find(": ") + 2);
		size_t paren = key.find('(');
		std::string fn = paren == std::string::npos ? key : key.substr(0, paren);
		std::string what = key.substr(key.rfind(": ") == std::string::npos ? 0 : key.rfind(": ") + 2);
		std::string k = fn + ": " + what;
		if (!kinds[k]++) example[k] = p;
	}
	printf("%zu problem(s) in %zu kind(s)\n", all.size(), kinds.size());
	size_t shown = 0;
	for (auto &kv : kinds)
		if (shown++ < show) printf("  %5d x  %s\n", kv.second, example[kv.first].c_str());
}

static void printChecked() {
	long tot = 0;
	for (auto &kv : g_checked) tot += kv.second;
	printf("argument checks: %ld calls (", tot);
	for (auto &kv : g_checked) printf("%s %ld, ", kv.first.c_str(), kv.second);
	printf(")\ndistinct message ids shown: %zu\n", g_messagesSeen.size());
}

static int testScan(Host &h) {
	h.reset();
	g_count = h.w(D_object_count);
	static const int verbs[] = {0, 19, 25, 68, 69, 70, 71, 5, 33, 40, 55, 7, 12, 22, 47, 60};
	long scenarios = 0, withText = 0, runaway = 0;
	std::vector<std::string> all;
	int handlers = 0;
	for (int o = 1; o <= g_count && o < (int)objs.size(); o++) {
		if (!objs[o].handler) continue;
		handlers++;
		for (int phase = 1; phase <= 6; phase++)
			for (int v : verbs) {
				h.reset();
				h.tracing = true;
				bool asIobj = phase == 3 || phase == 4;
				setupCommand(h, v, asIobj ? 7 : o, asIobj ? o : 8);
				h.problems.clear();
				h.call("call_object_handler", {(uint16_t)o, (uint16_t)phase});
				scenarios++;
				bool text = false;
				for (auto &e : h.events)
					if (!e.text.empty()) text = true;
				withText += text;
				if (h.cpu.steps > 3000000) runaway++;
				char ctx[96];
				snprintf(ctx, sizeof(ctx), "obj %d (%s) phase %d verb %d", o, objs[o].name.c_str(), phase, v);
				checkCalls(h, ctx);
				for (auto &p : h.problems) all.push_back(p);
			}
	}
	printf("scenarios %ld (%d objects with handlers x 6 phases x %zu verbs), %ld produced text, %ld hit the runaway guard\n", scenarios, handlers, sizeof(verbs) / sizeof(verbs[0]), withText, runaway);
	printChecked();
	reportProblems(all, 20);
	return 0;
}

static int testLook(Host &h, int room) {
	h.reset();
	g_count = h.w(D_object_count);
	if (!room) room = h.w(D_current_room);
	printf("room %d (%s)\n", room, room < (int)objs.size() ? objs[room].name.c_str() : "?");
	for (int o = 1; o <= g_count && o < (int)objs.size(); o++) {
		if (objs[o].loc != room || objs[o].type == 1) continue;
		h.reset();
		h.tracing = true;
		setupCommand(h, 19, o, 0);
		h.call("run_command_q", {19, (uint16_t)o, 0});
		printf("LOOK AT %d %s:\n", o, objs[o].name.c_str());
		for (auto &e : h.events)
			if (!e.text.empty()) printf("    [%s] %s\n", e.name.c_str(), e.text.c_str());
		if (h.events.empty()) printf("    (no output)\n");
		char ctx[64];
		snprintf(ctx, sizeof(ctx), "look at %d", o);
		checkCalls(h, ctx);
	}
	for (auto &p : h.problems) printf("PROBLEM %s\n", p.c_str());
	return 0;
}

// enter every room through go_to_room (phases 0xF, 0xD, 0xE of the room handler and its contents), then LOOK_AT every object in it
static int testRooms(Host &h) {
	h.reset();
	g_count = h.w(D_object_count);
	std::vector<std::string> all;
	long rooms = 0, entered = 0, looks = 0, lookText = 0, enterText = 0;
	for (int r = 1; r <= g_count && r < (int)objs.size(); r++) {
		if (objs[r].type != 1) continue;
		rooms++;
		h.reset();
		h.tracing = true;
		h.problems.clear();
		h.call("go_to_room_q", {(uint16_t)r});
		if (h.w(D_current_room) == r) entered++;
		else printf("  room %d (%s): go_to_room left current_room = %d\n", r, objs[r].name.c_str(), h.w(D_current_room));
		{
			long cs = 0;
			for (auto &c : h.calls) cs += c.name == "Conversation_Show";
			if (cs > 100) printf("  room %d (%s): %ld Conversation_Show calls (steps %ld)\n", r, objs[r].name.c_str(), cs, h.cpu.steps);
		}
		bool text = false;
		for (auto &e : h.events)
			if (!e.text.empty()) text = true;
		enterText += text;
		char ctx[96];
		snprintf(ctx, sizeof(ctx), "enter room %d (%s)", r, objs[r].name.c_str());
		checkCalls(h, ctx);
		for (auto &p : h.problems) all.push_back(p);
		for (int o = 1; o <= g_count && o < (int)objs.size(); o++) {
			if (objs[o].loc != r || objs[o].type == 1) continue;
			h.reset();
			h.tracing = true;
			h.problems.clear();
			h.setw(D_current_room, (uint16_t)r);
			setupCommand(h, 19, o, 0);
			h.call("run_command_q", {19, (uint16_t)o, 0});
			looks++;
			bool t = false;
			for (auto &e : h.events)
				if (!e.text.empty()) t = true;
			lookText += t;
			snprintf(ctx, sizeof(ctx), "look at %d (%s) in room %d", o, objs[o].name.c_str(), r);
			checkCalls(h, ctx);
			for (auto &p : h.problems) all.push_back(p);
		}
	}
	printf("%ld rooms: go_to_room reached %ld of them (%ld printed text); %ld LOOK_AT commands, %ld printed text\n", rooms, entered, enterText, looks, lookText);
	printChecked();
	reportProblems(all, 25);
	return 0;
}

// writes the emulated VGA screen (320x200, A000:0000) with the DAC palette as a PPM
static void dumpScreen(Host &h, const char *path) {
	FILE *f = fopen(path, "wb");
	if (!f) return;
	{ int nz = 0, pnz = 0; for (int i = 0; i < 320 * 200; i++) nz += h.mem[0xA0000 + i] != 0; for (int i = 0; i < 768; i++) pnz += h.machine.palette[i] != 0; fprintf(stderr, "screen dump %s: %d non-zero indices, %d non-zero palette components\n", path, nz, pnz); }
	fprintf(f, "P6\n320 200\n255\n");
	for (int i = 0; i < 320 * 200; i++) {
		uint8_t px = h.mem[0xA0000 + i];
		uint8_t rgb[3] = {Legend::palette8(h.machine.palette[px * 3]), Legend::palette8(h.machine.palette[px * 3 + 1]), Legend::palette8(h.machine.palette[px * 3 + 2])};
		fwrite(rgb, 1, 3, f);
	}
	fclose(f);
}

// shared by the tests below: run the original startup on a fresh machine
static bool startup(Host &h) {
	h.reset();
	Cpu &c = h.cpu;
	uint32_t prog = h.putString("xanth");
	c.wr16(0xD800, 0x7000, (uint16_t)prog);
	c.wr16(0xD800, 0x7002, (uint16_t)(prog >> 16));
	c.wr16(0xD800, 0x7004, 0);
	c.wr16(0xD800, 0x7006, 0);
	c.wr16(0xD800, 0x7100, 0);
	c.wr16(0xD800, 0x7102, 0);
	try {
		h.call("init_game", {1, 0x7000, 0xD800, 0x7100, 0xD800}, 2);
	} catch (const GameExit &e) {
		printf("init_game exited with %d: %s\n", e.code, h.machine.stdoutText.c_str());
		return false;
	}
	return true;
}

// the original string decoder (Resources_LoadText / ResourceText_GetStr, lifted) against the C++ port of the message file reader, for every string
static int testText(Host &h) {
	if (!startup(h)) return 1;
	long checked = 0, bad = 0;
	for (size_t b = 0; b < h.msgs->blockCount(); b++)
		for (size_t i = 0; i < h.msgs->stringCount(b); i++) {
			uint32_t id = 0xF0000000u | ((uint32_t)b << 16) | (uint32_t)i;
			uint32_t r = h.call("ResourceText_GetStr", {(uint16_t)(id & 0xFFFF), (uint16_t)(id >> 16)}, 2);
			std::string got = h.str((uint16_t)(r >> 16), (uint16_t)r, 2000);
			std::string want = h.msgs->string(b, i);
			checked++;
			if (got != want && bad++ < 5) printf("  mismatch %zu:%zu: original '%.60s' vs port '%.60s'\n", b, i, got.c_str(), want.c_str());
		}
	printf("ResourceText_GetStr (original code) vs MessageFile (C++ port): %ld strings, %ld mismatches\n", checked, bad);
	return bad != 0;
}

// the original save/load code on the emulated file system. The IDB names are swapped: load_game?(1) WRITES restart.dat (mode 1), save_game?(1) READS it back.
static int testSave(Host &h) {
	if (!startup(h)) return 1;
	g_count = h.w(D_object_count);
	std::vector<uint8_t> first = h.machine.vfs["restart.dat"];
	{ FILE *f = fopen("../../game_out/restart.dat", "wb"); if (f) { fwrite(first.data(), 1, first.size(), f); fclose(f); } }
	printf("restart.dat written by init_game: %zu bytes\n", first.size());
	uint16_t room0 = h.w(D_current_room), desk0 = h.call("object_get_location_q", {35}) & 0xFFFF;
	h.call("object_move_q", {35, 90, 0});                         // change some state
	h.setw(D_current_room, 99);
	h.cpu.steps = 0;
	uint32_t w = h.call("load_game_q", {1}, 2);                   // write
	std::vector<uint8_t> second = h.machine.vfs["restart.dat"];
	size_t diff = 0;
	for (size_t i = 0; i < second.size() && i < first.size(); i++) diff += second[i] != first[i];
	printf("write (load_game?(1)) returned %u: restart.dat %zu bytes, %zu bytes differ from the initial state\n", w & 0xFFFF, second.size(), diff);
	h.setw(D_current_room, room0);
	h.call("object_move_q", {35, desk0, 0});
	printf("after reset: room %u (was %u), desk location %u\n", h.w(D_current_room), room0, h.call("object_get_location_q", {35}) & 0xFFFF);
	h.cpu.steps = 0;
	size_t strays = h.stray.size();
	uint32_t r = h.call("save_game_q", {1}, 2);                   // read
	printf("read (save_game?(1)) returned %u; room %u (saved 99), desk location %u (saved 90); %zu longjmp(s) to the main loop\n", r & 0xFFFF, h.w(D_current_room), h.call("object_get_location_q", {35}) & 0xFFFF, h.stray.size() - strays);
	return 0;
}

// original go_to_room: the room picture is drawn by the lifted decoder and blitters into the emulated VGA memory
static int testRoom(Host &h, int room) {
	if (!startup(h)) return 1;
	g_count = h.w(D_object_count);
	h.events.clear();
	if (getenv("HOST_SHOT_TEXT")) h.onText = [](Host &hh) { static bool done = false; if (!done) { done = true; dumpScreen(hh, getenv("HOST_SHOT_TEXT")); } };
	h.tracing = getenv("HOST_TRACE") != nullptr;
	if (getenv("HOST_LABELPFX")) h.cpu.onLabel = [](Cpu &c, const char *w) { static int cnt = 0; if (!strncmp(w, getenv("HOST_LABELPFX"), strlen(getenv("HOST_LABELPFX"))) && cnt++ < 100000) fprintf(stderr, "L %s ax=%04X bx=%04X cx=%04X di=%04X si=%04X es=%04X ds=%04X\n", w, c.ax.x, c.bx.x, c.cx.x, c.di, c.si, c.es, c.ds); };
	if (getenv("HOST_NZ")) h.cpu.onLabel = [](Cpu &c, const char *w) { if (!strcmp(w, getenv("HOST_NZ"))) { int nz = 0; for (int i = 0; i < 64000; i++) nz += host().mem[0xA0000 + i] != 0; fprintf(stderr, "nz %s: %d\n", w, nz); } };
	if (getenv("HOST_SRC")) h.cpu.onLabel = [](Cpu &c, const char *w) { static int k = 0; if (!strcmp(w, getenv("HOST_SRC")) && k++ < 6) { fprintf(stderr, "src %s ds:si=%04X:%04X cx=%04X:", w, c.ds, c.si, c.cx.x); for (int i = 0; i < 12; i++) fprintf(stderr, " %02X", c.rd8(c.ds, (uint16_t)(c.si + i))); fprintf(stderr, " | es:di"); for (int i = 0; i < 6; i++) fprintf(stderr, " %02X", c.rd8(c.es, (uint16_t)(c.di + i))); fprintf(stderr, "\n"); } };
	if (getenv("HOST_CMPIMG")) h.cpu.onLabel = [](Cpu &c, const char *w) {
		static int hits = 0;
		if (strcmp(w, getenv("HOST_CMPIMG")) || hits++ > 6) return;
		uint16_t io = c.arg(0), is = c.arg(1);
		uint16_t wd = c.rd16(is, (uint16_t)(io + 6)), ht = c.rd16(is, (uint16_t)(io + 8)), po = c.rd16(is, (uint16_t)(io + 12)), ps = c.rd16(is, (uint16_t)(io + 14));
		int nz = 0; for (unsigned i = 0; i < (unsigned)wd * ht; i++) nz += c.rd8(ps, (uint16_t)(po + i)) != 0;
		fprintf(stderr, "image %04X:%04X %ux%u pixels %04X:%04X, %d non-zero\n", is, io, wd, ht, ps, po, nz);
		for (int f = 1; f <= 30; f++) {
			char fn[32]; snprintf(fn, sizeof(fn), "XANTH_%02d.PIC", f);
			std::vector<uint8_t> data;
			if (!host().machine.load(fn, data)) continue;
			Legend::PicFile pf(data);
			for (size_t i = 0; i < pf.size(); i++) {
				Legend::PicFile::Image im;
				try { im = pf.decode(i); } catch (...) { continue; }
				if (im.width != wd || im.height != ht) continue;
				int same = 0, inz = 0; for (unsigned k = 0; k < (unsigned)wd * ht; k++) { same += c.rd8(ps, (uint16_t)(po + k)) == im.pixels[k]; inz += im.pixels[k] != 0; }
				if (same * 100 / (wd * ht) > 90 && inz > 100) fprintf(stderr, "  matches %s image %zu: %d of %d pixels (%d non-zero)\n", fn, i, same, wd * ht, inz);
			}
		}
	};
	if (getenv("HOST_WATCH")) h.cpu.onLabel = [](Cpu &c, const char *w) { if (!strcmp(w, getenv("HOST_WATCH"))) fprintf(stderr, "watch %s bp=%04X sp=%04X es=%04X args %04X %04X %04X locals C=%04X:%04X 2=%04X\n", w, c.bp, c.sp, c.es, c.rd16(c.ss, (uint16_t)(c.bp + 6)), c.rd16(c.ss, (uint16_t)(c.bp + 8)), c.rd16(c.ss, (uint16_t)(c.bp + 10)), c.rd16(c.ss, (uint16_t)(c.bp - 10)), c.rd16(c.ss, (uint16_t)(c.bp - 12)), c.rd16(c.ss, (uint16_t)(c.bp - 2))); };
	if (getenv("HOST_MEMWATCH")) h.cpu.onLabel = [](Cpu &c, const char *w) { static uint16_t last = 0; static bool init = false; unsigned a = (unsigned)strtoul(getenv("HOST_MEMWATCH"), nullptr, 16); uint16_t sg = getenv("HOST_MEMWATCH_SEG") ? (uint16_t)strtoul(getenv("HOST_MEMWATCH_SEG"), nullptr, 16) : c.ss; uint16_t v = c.rd16(sg, (uint16_t)a); if (!init) { if (getenv("HOST_MEMWATCH_SEG") || !strcmp(w, "setup_pic_regions_q:loc_1CB07")) { init = true; last = v; } } else if (v != last) { fprintf(stderr, "mem %04X: %04X -> %04X before label %s (bp=%04X sp=%04X) prev %s %s %s\n", a, last, v, w, c.bp, c.sp, c.hist[(c.hpos - 2) & 63], c.hist[(c.hpos - 3) & 63], c.hist[(c.hpos - 4) & 63]); last = v; } };
	if (getenv("HOST_DSWATCH")) h.cpu.onLabel = [](Cpu &c, const char *w) { static uint16_t last = 0x84FC; unsigned want = (unsigned)strtoul(getenv("HOST_DSWATCH"), nullptr, 16); if (c.ds != last) { if (c.ds == want || last == want) fprintf(stderr, "ds %04X -> %04X at %s prev %s %s %s\n", last, c.ds, w, c.hist[(c.hpos - 2) & 63], c.hist[(c.hpos - 3) & 63], c.hist[(c.hpos - 4) & 63]); last = c.ds; } };
	try {
		h.call("go_to_room_q", {(uint16_t)room});
	} catch (const GameExit &e) {
		printf("exit %d\n", e.code);
	}
	if (h.tracing) for (size_t i = 0; i < h.calls.size(); i++) printf("  call %s(%X,%X,%X)\n", h.calls[i].name.c_str(), h.calls[i].w[0], h.calls[i].w[1], h.calls[i].w[2]);
	const char *shot = getenv("HOST_SHOT");
	if (shot) dumpScreen(h, shot);
	int nonzero = 0;
	for (int i = 0; i < 320 * 200; i++) nonzero += h.mem[0xA0000 + i] != 0;
	{ uint16_t po = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 12)), ps = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 14)), w = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 6)), hh = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 8)); int nz = 0; for (unsigned i = 0; i < (unsigned)w * hh; i++) nz += h.cpu.rd8(ps, (uint16_t)(po + i)) != 0 && ((unsigned)ps * 16 + po + i) < 0x1FFFFF; printf("_background pixels %04X:%04X %ux%u: %d nonzero of %u\n", ps, po, w, hh, nz, w * hh); }
	{ uint16_t pio = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 16)), pis = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 18));
	  uint16_t ppo = h.cpu.rd16(pis, (uint16_t)(pio + 24)), pps = h.cpu.rd16(pis, (uint16_t)(pio + 26)), pw = h.cpu.rd16(pis, (uint16_t)(pio + 12)), ph = h.cpu.rd16(pis, (uint16_t)(pio + 10)), pp = h.cpu.rd16(pis, (uint16_t)(pio + 20));
	  int nz = 0; for (unsigned i = 0; i < (unsigned)pp * ph && ((unsigned)pps * 16 + ppo + i) < 0x1FFFFF; i++) nz += h.cpu.rd8(pps, (uint16_t)(ppo + i)) != 0;
	  { printf("rows:"); for (unsigned y = 0; y < 122; y += 6) { int c2 = 0; for (unsigned x = 0; x < pp; x++) c2 += h.cpu.rd8(pps, (uint16_t)(ppo + y * pp + x)) != 0; printf(" %u:%d", y, c2); } printf("\n"); }
	  { unsigned first = 0; for (unsigned i = 0; i < (unsigned)pp * 121; i++) if (h.cpu.rd8(pps, (uint16_t)(ppo + i))) { first = i; break; } printf("first nonzero pixel offset %04X (row %u col %u)\n", first, first / pp, first % pp); }
	  printf("Picture %04X:%04X %ux%u pitch %u pixels %04X:%04X: %d nonzero\n", pis, pio, pw, ph, pp, pps, ppo, nz); }
	{ uint16_t pio = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 16)), pis = h.cpu.rd16(SEG_seg037, (uint16_t)(D__background + 18));
	  uint16_t ppo = h.cpu.rd16(pis, (uint16_t)(pio + 24)), pps = h.cpu.rd16(pis, (uint16_t)(pio + 26)), pp = h.cpu.rd16(pis, (uint16_t)(pio + 20));
	  for (int f = 1; f <= 30; f++) {
		char fn[32]; snprintf(fn, sizeof(fn), "XANTH_%02d.PIC", f);
		std::vector<uint8_t> data;
		if (!h.machine.load(fn, data)) continue;
		Legend::PicFile pf(data);
		for (size_t i = 0; i < pf.size(); i++) {
			Legend::PicFile::Image im;
			try { im = pf.decode(i); } catch (...) { continue; }
			if (im.width != 264 || im.height < 100) continue;
			int same = 0, total = 0;
			for (unsigned y = 0; y < im.height && y < 121; y++) for (unsigned x = 0; x < im.width; x++) { total++; same += h.cpu.rd8(pps, (uint16_t)(ppo + y * pp + x)) == im.pixels[y * im.width + x]; }
			if (total && same * 100 / total > 50) printf("  background matches %s image %zu (%ux%u): %d of %d pixels in the first rows\n", fn, i, im.width, im.height, same, total);
		}
	  }
	}
	{ printf("_background:"); for (int i = 0; i < 32; i++) printf(" %02X", h.cpu.rd8(SEG_seg037, (uint16_t)(D__background + i))); printf("\n"); }
	printf("screen_activePageSeg %04X\n", h.w(D_screen_activePageSeg));
	{ int blocks = 0; for (unsigned a = 0x10000; a < 0xF0000; a += 0x1000) { int nz = 0; for (unsigned k = 0; k < 0x1000; k++) nz += h.mem[a + k] != 0; if (a >= 0xA0000 || nz > 0x800) { if (a >= 0xA0000) printf("  %05X: %d nonzero\n", a, nz); } blocks++; } }
	printf("go_to_room(%d): current_room %u, %d non-black pixels, %ld palette writes, video mode %d\n", room, h.w(D_current_room), nonzero, h.machine.paletteWrites, h.machine.videoMode);
	for (auto &e : h.events) if (!e.text.empty()) printf("  [%s] %s\n", e.name.c_str(), e.text.c_str());
	for (auto &p : h.problems) printf("PROBLEM %s\n", p.c_str());
	return 0;
}

// run the original startup (game_init) on the emulated PC
static int testClock(Host &h) {
	h.reset();
	for (int i = 0; i < 6; i++) {
		uint32_t r = h.call("_clock", {}, 2);
		printf("_clock() = %u (0x%08X)  cs=%u\n", r, r, h.machine.clockCs);
	}
	return 0;
}

static int testInit(Host &h) {
	h.reset();
	Cpu &c = h.cpu;
	// argv = { "xanth", NULL } and an empty environment in scratch memory
	uint32_t prog = h.putString("xanth");
	uint16_t argvOff = 0x7000;
	c.wr16(0xD800, argvOff, (uint16_t)prog);
	c.wr16(0xD800, argvOff + 2, (uint16_t)(prog >> 16));
	c.wr16(0xD800, argvOff + 4, 0);
	c.wr16(0xD800, argvOff + 6, 0);
	c.wr16(0xD800, 0x7100, 0);
	c.wr16(0xD800, 0x7102, 0);
	h.tracing = getenv("HOST_TRACE") != nullptr;
	if (getenv("HOST_KEYS"))
		for (int i = 0; i < atoi(getenv("HOST_KEYS")); i++) h.machine.keys.push_back(0x1C0D);       // Enter
	static int probes = 0;
	if (getenv("HOST_PROBE")) h.cpu.onLabel = [](Cpu &c, const char *w) {
		if (probes < 12 && !strcmp(w, getenv("HOST_PROBE"))) {
			probes++;
			printf("  probe %s ds=%04X es=%04X ss=%04X bp=%04X ax=%04X bx=%04X [bp-18E]=%04X [bp-18C]=%04X'\n", w, c.ds, c.es, c.ss, c.bp, c.ax.x, c.bx.x, c.rd16(c.ss, (uint16_t)(c.bp - 0x18E)), c.rd16(c.ss, (uint16_t)(c.bp - 0x18C)));
		}
	};
	int code = -1;
	try {
		if (getenv("HOST_MAIN")) h.call("main", {1, argvOff, 0xD800, 0x7100, 0xD800}, 1); else h.call("init_game", {1, argvOff, 0xD800, 0x7100, 0xD800}, 2);
		code = 0;
	} catch (const GameExit &e) {
		code = 1000 + e.code;
	}
	if (getenv("HOST_SHOT")) dumpScreen(h, getenv("HOST_SHOT"));
	printf("init_game %s (code %d); steps %ld\n", code == 0 ? "returned" : "exited", code, h.cpu.steps);
	if (h.tracing && getenv("HOST_HIST")) {
		std::map<std::string, long> cnt;
		for (auto &cl : h.calls) cnt[cl.name]++;
		std::vector<std::pair<long, std::string>> v;
		for (auto &kv : cnt) v.push_back({kv.second, kv.first});
		std::sort(v.rbegin(), v.rend());
		for (size_t i = 0; i < v.size() && i < 25; i++) printf("  count %ld %s\n", v[i].first, v[i].second.c_str());
	}
	if (h.tracing) {
		size_t at = h.calls.size();
		for (size_t i = 0; i < h.calls.size(); i++)
			if (h.calls[i].name == "_printf") { at = i; break; }
		for (size_t i = at > 150 ? at - 150 : 0; i < at + 3 && i < h.calls.size(); i++)
			printf("  call %s(%X,%X,%X)\n", h.calls[i].name.c_str(), h.calls[i].w[0], h.calls[i].w[1], h.calls[i].w[2]);
	}
	for (unsigned i = 0; i < 24; i++) printf("  hist %s\n", h.cpu.hist[(h.cpu.hpos - 24 + i) & 63] ? h.cpu.hist[(h.cpu.hpos - 24 + i) & 63] : "-");
	printf("  regs ax=%04X bx=%04X cx=%04X dx=%04X si=%04X di=%04X bp=%04X sp=%04X es=%04X ds=%04X ss=%04X\n", h.cpu.ax.x, h.cpu.bx.x, h.cpu.cx.x, h.cpu.dx.x, h.cpu.si, h.cpu.di, h.cpu.bp, h.cpu.sp, h.cpu.es, h.cpu.ds, h.cpu.ss);
	printf("  [bp-18E]=%04X [bp-18C]=%04X [bp-19E]=%02X [bp-1A8]=%02X\n", h.cpu.rd16(h.cpu.ss, (uint16_t)(h.cpu.bp - 0x18E)), h.cpu.rd16(h.cpu.ss, (uint16_t)(h.cpu.bp - 0x18C)), h.cpu.rd8(h.cpu.ss, (uint16_t)(h.cpu.bp - 0x19E)), h.cpu.rd8(h.cpu.ss, (uint16_t)(h.cpu.bp - 0x1A8)));
	printf("  vectors[8]=%08X timer ticks delivered %ld\n", (unsigned)h.cpu.rd16(0, 0x20) | ((unsigned)h.cpu.rd16(0, 0x22) << 16), h.machine.irqCount);
	printf("last label: %s\n", h.cpu.where);
	printf("files opened: ");
	for (auto &f : h.machine.openLog) printf("%s ", f.c_str());
	printf("\nvideo mode %d, palette writes %ld, EMS pages used %d, DOS blocks:", h.machine.videoMode, h.machine.paletteWrites, h.machine.emsUsed);
	int used = 0;
	for (auto &b : h.machine.blocks) used += b.used;
	printf(" %d allocated\n", used);
	for (auto &e : h.events) printf("  event %s %s\n", e.name.c_str(), e.text.c_str());
	for (auto &p : h.problems) printf("PROBLEM %s\n", p.c_str());
	if (!h.machine.stdoutText.empty()) printf("stdout: %s\n", h.machine.stdoutText.c_str());
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 5) {
		fprintf(stderr, "usage: host_test <gamedir> <image.bin> <xanth_objects.tsv> objects|scan|look [room]|rooms\n");
		return 2;
	}
	try {
		Host &h = host();
		h.init(argv[2], std::string(argv[1]) + "/XANTHSTR.DAT");
		loadObjects(argv[3]);
		std::string cmd = argv[4];
		if (cmd == "objects") return testObjects(h) ? 1 : 0;
		if (cmd == "scan") return testScan(h);
		if (cmd == "look") return testLook(h, argc > 5 ? atoi(argv[5]) : 0);
		if (cmd == "rooms") return testRooms(h);
		if (cmd == "init") return testInit(h);
		if (cmd == "clock") return testClock(h);
		if (cmd == "text") return testText(h);
		if (cmd == "save") return testSave(h);
		if (cmd == "room") return testRoom(h, argc > 5 ? atoi(argv[5]) : 34);
		fprintf(stderr, "unknown command\n");
		return 2;
	} catch (const std::exception &e) {
		fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
}
