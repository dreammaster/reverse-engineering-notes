/* PROG4 (the main game, translated by tools/asm2cpp.py) against the original. Needs $YENDOR1_REFP4_DIR with one sub-directory per
 * yemu reference run (trace.txt, NN_PROG4_start.ydmp and the party files before the run in in/CURRGAME). The run is cut off when
 * the port has made as many driver calls as the original had. Scenarios:
 *   idle     the tavern at the start of the game, no input: the first frames of the idle loop. */

#include "ReplaySystem.h"
#include "ScriptedInput.h"
#include "TestUtil.h"
#include "gen/Prog4Gen.h"
#include "shared/GeneratedProgram.h"
#include "sw/Shell.h"
#include "vid/Fonts.h"

#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

using namespace Yendor1;
using namespace ScriptedInput;

namespace {

bool copyFile(const std::string &from, const std::string &to) {
	FILE *a = fopen(from.c_str(), "rb");
	if (!a)
		return false;
	FILE *b = fopen(to.c_str(), "wb");
	if (!b) { fclose(a); return false; }
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), a)) > 0)
		fwrite(buf, 1, n, b);
	fclose(a);
	fclose(b);
	return true;
}

struct Scenario {
	const char *name;
	const char *startFile;
	std::vector<Step> steps;
	unsigned clock;
	bool events;       // keys come from trace.txt.events
};

bool run(const std::string &refRoot, const Scenario &sc, bool quiet = false) {
	const std::string ref = refRoot + "/" + sc.name;
	Tools::Snapshot start;
	if (!start.load((ref + "/" + sc.startFile).c_str())) {
		printf("  skipped %s (no %s in %s)\n", sc.name, sc.startFile, ref.c_str());
		return false;
	}
	const std::vector<std::string> want = withoutRefresh(TraceCompare::sliceThrough(ref + "/trace.txt", "PROG4.EXE", 1), true);
	if (want.empty())
		return false;

	const std::string work = std::string("test_prog4_work_") + sc.name;
	MKDIR(work.c_str());
	MKDIR((work + "/CURRGAME").c_str());
	static const char *files[] = {"PARTY.DAT", "NPC.DAT", "AREA.DAT", "BATPIC.DAT"};
	for (int i = 0; i < 4; i++)
		copyFile(ref + "/in/CURRGAME/" + files[i], work + "/CURRGAME/" + files[i]);

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	Recorder rec;
	v.setTrace(&rec);
	KeyReads keys;
	if (sc.events)
		keys.load(ref + "/trace.txt.events", callsBefore(ref + "/trace.txt", "PROG4.EXE"));
	Replay sys(v, sc.steps, rec, want.size() * 3, sc.clock, &want, keys);
	Shared::Runtime rt(v, sys);
	rt.openGameFiles(TestUtil::gameDir());
	rt.setWorkDir(work);
	Sw::DefaultLauncher children;
	sys.programStarted(4, true);
	Shared::runGenerated<Gen::Prog4Gen>(rt, &children, start.params, "PROG4.EXE");
	sys.programEnded();
	v.setTrace(0);

	std::vector<std::string> got = withoutRefresh(rec.lines, true);
	std::vector<std::string> expect = want;
	const size_t n = got.size() < expect.size() ? got.size() : expect.size();
	got.resize(n);
	expect.resize(n);
	const std::string diff = TraceCompare::compare(expect, got);
	if (const char *dump = getenv("YENDOR1_DUMP")) {
		FILE *f = fopen(dump, "w");
		for (size_t i = 0; f && i < n; i++)
			fprintf(f, "%zu W %s\n%zu G %s\n", i, expect[i].c_str(), i, got[i].c_str());
		if (f)
			fclose(f);
	}
	if (quiet)
		return diff.empty() && n + 2 >= want.size();
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		showContext(diff, expect, got);
	}
	CHECK(diff.empty());
	printf("  %s: %zu of %zu calls compared\n", sc.name, n, want.size());
	return diff.empty();
}

} // namespace

TEST_MAIN_BEGIN("prog4 (generated) against the original")
	const char *refEnv = getenv("YENDOR1_REFP4_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REFP4_DIR, see the comment in test_prog4.cpp)\n");
		return 0;
	}
	Scenario idle;
	idle.events = false;
	idle.name = "idle";
	idle.startFile = "06_PROG4_start.ydmp";
	const char *clockEnv = getenv("YENDOR1_CLOCK");
	idle.clock = clockEnv ? (unsigned)atoi(clockEnv) : 0x1234;
	if (getenv("YENDOR1_SCAN")) {
		for (unsigned c = getenv("YENDOR1_FROM") ? (unsigned)atoi(getenv("YENDOR1_FROM")) : 1; c < 65536; c++) {
			idle.clock = c;
			if (getenv("YENDOR1_VERBOSE")) { fprintf(stderr, "%u ", c); fflush(stderr); }
			if (run(refEnv, idle, true)) {
				printf("  clock %u (0x%04X) reproduces the run\n", c, c);
				break;
			}
		}
		return 0;
	}
	const char *only = getenv("YENDOR1_ONLY");
	if (!only || !strcmp(only, "idle"))
		run(refEnv, idle);
	Scenario walk;
	walk.name = "walk";
	walk.startFile = "06_PROG4_start.ydmp";
	walk.clock = 0x1234;
	walk.events = true;
	if (!only || !strcmp(only, "walk"))
		run(refEnv, walk);
	Scenario keys = walk;
	keys.name = "keys";
	if (!only || !strcmp(only, "keys"))
		run(refEnv, keys);
TEST_MAIN_END()
