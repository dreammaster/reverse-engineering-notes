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
		fprintf(stderr, "unknown command\n");
		return 2;
	} catch (const std::exception &e) {
		fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
}
