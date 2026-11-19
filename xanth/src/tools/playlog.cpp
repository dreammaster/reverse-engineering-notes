// xanth_playlog <game folder> <script> <log> [gen N SEED]: plays commands ("verb dobj iobj" per line of the script) through Game::executeCommand on a new game with the headless
// Ui and writes, after each command, the same line as the oracle's `playlog` (engine/host/host_test): step and command, the room, a hash of the saved state, a hash of the event
// queue, the event count and the text printed. With "gen N SEED" the script is made up as the game goes (random verbs on what is in the room and in the inventory, walking,
// waiting) and written to <script> for the oracle to replay; generation stops at the first conversation or cutscene, which the oracle cannot play without a window.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "common/files.h"
#include "xanth/game.h"
#include "xanth/verbs.h"

static uint64_t fnv(uint64_t h, const uint8 *p, size_t n) {
	for (size_t i = 0; i < n; i++)
		h = (h ^ p[i]) * 1099511628211ull;
	return h;
}

static unsigned g_seed = 1;

static unsigned rnd(unsigned n) {
	g_seed = g_seed * 1103515245u + 12345u;
	return (g_seed >> 8) % n;
}

static FILE *g_detail = NULL;                 // PLAYLOG_DETAIL: a hash per object record and per variable region, like the oracle's (to see what differs)
struct RegionInfo {
	unsigned size;
	bool skip;
};
static std::vector<RegionInfo> g_regions;

static void loadRegions() {
	FILE *rf = fopen("reversing/analysis/xanth_save_regions.tsv", "rb");
	char line[256];
	if (rf && fgets(line, sizeof(line), rf))
		while (fgets(line, sizeof(line), rf)) {
			unsigned idx, addr, size;
			char name[96] = "";
			if (sscanf(line, "%u %x %u %95s", &idx, &addr, &size, name) >= 3) {
				RegionInfo r;
				r.size = size;
				r.skip = !strcmp(name, "show_text_with_voice") || !strcmp(name, "voice_on");      // (the player's voice options: not state)
				g_regions.push_back(r);
			}
		}
	if (rf)
		fclose(rf);
}

// bytes of object records that the program changes while it loads (pointers it fixes up: not game state): object, first byte, last byte
static const int kMask[][3] = {{49, 15, 17}, {103, 11, 13}};

/** The hash of the saved state like the oracle's: object records (with the loader's pointers masked), then the variable regions. */
static uint64_t stateHash(Xanth::Game &g, const Common::Bytes &saved) {
	static const unsigned recSize[] = {0, 18, 28, 32, 14, 32};
	uint64_t h = 1469598103934665603ull;
	size_t at = 0x20;
	for (int id = 1; id <= g.state.objects.count(); id++) {
		unsigned sz = recSize[g.state.objects.type(id)];
		std::vector<uint8> r(saved.data() + at, saved.data() + at + sz);
		for (size_t m = 0; m < sizeof(kMask) / sizeof(kMask[0]); m++)
			if (kMask[m][0] == id)
				for (int k = kMask[m][1]; k <= kMask[m][2] && k < (int)sz; k++)
					r[k] = 0;
		h = fnv(h, &r[0], sz);
		if (g_detail)
			fprintf(g_detail, "%08x ", (unsigned)fnv(1469598103934665603ull, &r[0], sz));
		at += sz;
	}
	for (size_t i = 0; i < g_regions.size(); i++) {
		if (g_regions[i].skip) {
			if (g_detail)
				fprintf(g_detail, "00000000 ");
		} else {
			h = fnv(h, saved.data() + at, g_regions[i].size);
			if (g_detail)
				fprintf(g_detail, "%08x ", (unsigned)fnv(1469598103934665603ull, saved.data() + at, g_regions[i].size));
		}
		at += g_regions[i].size;
	}
	if (g_detail)
		fputc(10, g_detail);
	return h;
}

static std::string runCommand(Xanth::Game &g, Xanth::RecordingUi &ui, int step, int verb, int dobj, int iobj, bool *conversation) {
	ui.log.clear();
	ui.others.clear();
	bool pre = verb < 0;
	if (pre && getenv("PLAYLOG_RECS")) {          // (see the oracle's playlog)
		static const unsigned recSize[] = {0, 18, 28, 32, 14, 32};
		Common::Bytes saved;
		g.state.save(saved);
		size_t at = 0x20;
		std::vector<size_t> offs(1, 0);
		for (int id = 1; id <= g.state.objects.count(); id++) {
			offs.push_back(at);
			at += recSize[g.state.objects.type(id)];
		}
		for (const char *q = getenv("PLAYLOG_RECS"); *q;) {
			int id = atoi(q);
			fprintf(stderr, "obj %d:", id);
			for (unsigned k = 0; k < recSize[g.state.objects.type(id)]; k++)
				fprintf(stderr, " %02x", saved[offs[id] + k]);
			fputc(10, stderr);
			while (*q && *q != ',')
				q++;
			if (*q)
				q++;
		}
	}                         // the state before the first command
	if (pre)
		verb = dobj = iobj = 0;
	else {
		g.setSeed(12345u + (unsigned)step * 7919u);      // (the oracle's playlog does the same before every command)
		g.executeCommand(verb, dobj, iobj);
	}
	Common::Bytes saved;
	g.state.save(saved);
	size_t eventsAt = saved.size() - 4 * g.state.events.size();
	uint64_t state = stateHash(g, saved);
	uint64_t events = fnv(1469598103934665603ull, saved.data() + eventsAt, 4 * g.state.events.size());
	std::string text;
	*conversation = false;
	for (size_t k = 0; k < ui.log.size() && !pre; k++) {
		const std::string &e = ui.log[k];
		if (e.compare(0, 14, "realsound_play") == 0)
			continue;
		if (e.compare(0, 13, "conversation|") == 0)
			*conversation = true;
		text += e + "\x1f";
	}
	for (size_t k = 0; k < ui.others.size(); k++)
		if (ui.others[k] == "showCutscene")
			*conversation = true;
	if (getenv("PLAYLOG_VARS"))
		fprintf(stderr, "step %d: light_room=%04x room=%04x\n", step, (unsigned)g.state.v.light_room, (unsigned)g.state.v.current_room);
	if (getenv("PLAYLOG_VARS")) {
		fprintf(stderr, "  seed=%u\n", g.seed());
		fprintf(stderr, "  events:");
		for (size_t k = 0; k < g.state.events.size(); k++)
			fprintf(stderr, " [%u in %d]", g.state.events[k].a & 0xFF, (int)(int16)g.state.events[k].b);
		fputc(10, stderr);
	}
	char b[200];
	snprintf(b, sizeof(b), "%d %d %d %d\t%u\t%016" PRIx64 "\t%016" PRIx64 "\t%u\t", step, verb, dobj, iobj, (unsigned)g.state.v.current_room, state, events, (unsigned)g.state.events.size());
	return std::string(b) + text;
}

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage: xanth_playlog <game folder> <script> <log> [gen N SEED]\n");
		return 2;
	}
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!gf.setFolder(argv[1]) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("cannot read the game files in %s\n", argv[1]);
		return 1;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str)) {
		printf("cannot load OBJECT.DAT / XANTHSTR.DAT\n");
		return 1;
	}
	Xanth::RecordingUi ui;
	Xanth::Game g(ui, messages, names);
	g.newGame();
	g.beginGame();
	FILE *log = fopen(argv[3], "wb");
	if (!log)
		return 1;
	loadRegions();
	if (getenv("PLAYLOG_DETAIL"))
		g_detail = fopen((std::string(argv[3]) + ".detail").c_str(), "wb");
	int count = 0;
	bool generate = argc >= 7 && !strcmp(argv[4], "gen");
	std::vector<int> verbs;
	for (int id = 1; id < 100; id++)
		if (Xanth::findVerb(id) && id != 0x51 && id != 80)
			verbs.push_back(id);
	std::vector<int> usual = Xanth::initialVerbs();
	{
		bool c0 = false;
		std::string first = runCommand(g, ui, 0, -1, 0, 0, &c0);
		fprintf(log, "%s\n", first.c_str());
	}
	if (generate) {
		count = atoi(argv[5]);
		g_seed = (unsigned)atoi(argv[6]) * 2654435761u + 1;
		FILE *script = fopen(argv[2], "wb");
		if (!script)
			return 1;
		for (int step = 1; step <= count; step++) {
			std::vector<int> things;
			for (int root = 0; root < 2; root++)
				g.walkContents(root ? 1 : g.state.v.current_room, true, [&](int o) {
					if (o != 1 && o != g.state.v.current_room && g.state.objects.type(o) != 1)
						things.push_back(o);
					return false;
				});
			int verb, dobj = 0, iobj = 0;
			unsigned r = rnd(100);
			if (r < 25 || things.empty()) {
				verb = 0x51;                               // walk: the direction code is the arrow number + 1
				dobj = 1 + (int)rnd(8);
			} else if (r < 32) {
				verb = 80;                                 // wait
			} else {
				verb = r < 80 ? usual[rnd((unsigned)usual.size())] : verbs[rnd((unsigned)verbs.size())];
				const Xanth::VerbInfo *vi = Xanth::findVerb(verb);
				dobj = things[rnd((unsigned)things.size())];
				if (vi && vi->objects == 2)
					iobj = things[rnd((unsigned)things.size())];
				if (!vi || vi->objects == 0)
					dobj = 0;
			}
			fprintf(script, "%d %d %d\n", verb, dobj, iobj);
			bool conversation = false;
			std::string l = runCommand(g, ui, step, verb, dobj, iobj, &conversation);
			fprintf(log, "%s\n", l.c_str());
			if (conversation)
				break;
			// a text of three lines or more grows the text window, and the oracle (which runs the turns without the player) then hangs in its next word wrap: stop after it
			size_t textAt = 0;
			for (int t = 0; t < 5 && textAt != std::string::npos; t++)
				textAt = l.find(9, textAt == 0 && t == 0 ? 0 : textAt + 1);
			if (textAt != std::string::npos && l.size() - textAt > 140)
				break;
		}
		fclose(script);
	} else {
		FILE *script = fopen(argv[2], "rb");
		if (!script)
			return 1;
		char buf[256];
		int step = 1;
		while (fgets(buf, sizeof(buf), script)) {
			int verb, dobj, iobj;
			if (sscanf(buf, "%d %d %d", &verb, &dobj, &iobj) != 3)
				continue;
			bool conversation = false;
			fprintf(log, "%s\n", runCommand(g, ui, step++, verb, dobj, iobj, &conversation).c_str());
		}
		fclose(script);
	}
	fclose(log);
	return 0;
}
