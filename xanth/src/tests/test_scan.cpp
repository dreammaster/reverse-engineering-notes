// Handler scenarios: every object handler x phases 1-6 x 16 verbs, run on a new game with the headless Ui, compared with the oracle's `scanlog` output
// (XANTH_SCANLOG=<file written by `host_test ... scanlog <file>`; it holds game text and stays local). Each line gives the handler's return value, a hash of the saved
// state (object records and the 205 variable regions), a hash of the event queue and the text the scenario printed.
// Without the file only a smoke run is done.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>

#include "common/files.h"
#include "xanth/game.h"

static uint64_t fnv(uint64_t h, const uint8 *p, size_t n) {
	for (size_t i = 0; i < n; i++)
		h = (h ^ p[i]) * 1099511628211ull;
	return h;
}

static const int kVerbs[] = {0, 19, 25, 68, 69, 70, 71, 5, 33, 40, 55, 7, 12, 22, 47, 60};

struct Scenario {
	int obj, phase, verb;
	unsigned ret;
	uint64_t state, events;
	unsigned eventCount;
	std::string text;
};

static std::string line(const Scenario &s) {
	char b[160];
	snprintf(b, sizeof(b), "%d %d %d\t%u\t%016" PRIx64 "\t%016" PRIx64 "\t%u\t", s.obj, s.phase, s.verb, s.ret & 0xFFFF, s.state, s.events, s.eventCount);
	return std::string(b) + s.text;
}

int main() {
	const char *dir = getenv("XANTH_DATA");
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!dir || !gf.setFolder(dir) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("test_scan: XANTH_DATA not set, skipped\n");
		return 0;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str)) {
		printf("test_scan: cannot load OBJECT.DAT / XANTHSTR.DAT\n");
		return 1;
	}
	std::map<std::string, std::string> golden;
	const char *logPath = getenv("XANTH_SCANLOG");
	if (logPath) {
		std::ifstream in(logPath, std::ios::binary);
		std::string l, cur;
		// a scenario's text can contain line breaks: lines that do not start with "obj phase verb<TAB>" continue the previous one
		auto flush = [&]() {
			size_t tab = cur.find('\t');
			if (tab != std::string::npos)
				golden[cur.substr(0, tab)] = cur;
		};
		while (std::getline(in, l)) {
			int a, b, c, n = 0;
			if (sscanf(l.c_str(), "%d %d %d%n", &a, &b, &c, &n) == 3 && n > 0 && l.size() > (size_t)n && l[n] == '\t') {
				flush();
				cur = l;
			} else {
				cur += "\n" + l;
			}
		}
		flush();
	}

	FILE *dump = getenv("XANTH_SCANOUT") ? fopen(getenv("XANTH_SCANOUT"), "wb") : nullptr;
	long scenarios = 0, skipped = 0, same = 0, retDiff = 0, stateDiff = 0, eventDiff = 0, textDiff = 0;
	int shown = 0;
	Xanth::RecordingUi ui;
	Xanth::Game g(ui, messages, names);
	for (int o = 1; o <= g.state.objects.count(); o++) {
		if (!g.state.objects.handlerIndex(o))
			continue;
		for (int phase = 1; phase <= 6; phase++)
			for (size_t vi = 0; vi < ARRAYSIZE(kVerbs); vi++) {
				int verb = kVerbs[vi];
				g.newGame();
				ui.log.clear();
				ui.others.clear();
				bool asIobj = phase == 3 || phase == 4;
				int dobj = asIobj ? 7 : o, iobj = asIobj ? o : 8;
				g.rt.actor_obj = g.rt.player_obj;
				g.rt.selectedCommand = (uint16)verb;
				g.rt.dobj = (uint16)dobj;
				g.rt.iobj = (uint16)iobj;
				g.rt.dobj_type = (uint16)g.state.objects.type(dobj);
				g.rt.iobj_type = (uint16)g.state.objects.type(iobj);
				Scenario s;
				s.obj = o;
				s.phase = phase;
				s.verb = verb;
				if (getenv("XANTH_SCANTRACE")) {
					fprintf(stderr, "%d %d %d\n", o, phase, verb);
					fflush(stderr);
				}
				s.ret = (unsigned)g.call_object_handler(o, phase);
				Common::Bytes saved;
				g.state.save(saved);
				size_t eventsAt = saved.size() - 4 * g.state.events.size();
				s.state = fnv(1469598103934665603ull, saved.data() + 0x20, 13400 + 4522);
				s.events = fnv(1469598103934665603ull, saved.data() + eventsAt, 4 * g.state.events.size());
				s.eventCount = (unsigned)g.state.events.size();
				for (size_t k = 0; k < ui.log.size(); k++) {
					const std::string &e = ui.log[k];
					if (e.compare(0, 14, "realsound_play") == 0)     // the oracle runs the original's sound routine and does not log it
						continue;
					s.text += e + "\x1f";
				}
				scenarios++;
				bool conversation = false;
				for (size_t k = 0; k < ui.others.size(); k++)
					conversation |= ui.others[k] == "showCutscene";
				for (size_t k = 0; k < ui.log.size(); k++)
					conversation |= ui.log[k].compare(0, 13, "conversation|") == 0;
				if (conversation) {
					// the oracle cannot run a conversation or cutscene without a window and a player: it stops somewhere inside, so these scenarios are not compared
					skipped++;
					continue;
				}
				if (dump)
					fprintf(dump, "%s\n", line(s).c_str());
				if (golden.empty())
					continue;
				char key[48];
				snprintf(key, sizeof(key), "%d %d %d", o, phase, verb);
				auto it = golden.find(key);
				if (it == golden.end()) {
					continue;
				}
				std::string mine = line(s);
				if (mine == it->second) {
					same++;
					continue;
				}
				// which part differs
				Scenario t = s;
				std::string a = mine, b = it->second;
				std::vector<std::string> fa, fb;
				for (int pass = 0; pass < 2; pass++) {
					std::string &src = pass ? b : a;
					std::vector<std::string> &dst = pass ? fb : fa;
					size_t pos = 0;
					for (int f = 0; f < 6; f++) {
						size_t tab = src.find('\t', pos);
						dst.push_back(src.substr(pos, tab == std::string::npos ? std::string::npos : tab - pos));
						pos = tab == std::string::npos ? src.size() : tab + 1;
					}
					dst.push_back(src.substr(pos));
				}
				bool r = fa[1] != fb[1], st = fa[2] != fb[2], ev = fa[3] != fb[3], tx = fa.size() > 6 && fb.size() > 6 && fa[6] != fb[6];
				retDiff += r;
				stateDiff += st;
				eventDiff += ev;
				textDiff += tx;
				if (shown++ < 15)
					printf("  scenario %s: ret %s/%s%s%s%s\n", key, fa[1].c_str(), fb[1].c_str(), st ? " state" : "", ev ? " events" : "", tx ? " text" : "");
			}
	}
	if (dump)
		fclose(dump);
	printf("test_scan: %ld scenarios run (%ld start a conversation or cutscene and are not compared)", scenarios, skipped);
	if (!golden.empty())
		printf(", %ld identical to the oracle; differences: %ld return value, %ld state, %ld event queue, %ld text", same, retDiff, stateDiff, eventDiff, textDiff);
	printf("\n");
	return 0;
}
