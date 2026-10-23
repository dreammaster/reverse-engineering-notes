/* PROG4 (the main game, translated by tools/asm2cpp.py) against the original. Needs $YENDOR1_REFP4_DIR with one sub-directory per
 * yemu reference run (trace.txt, NN_PROG4_start.ydmp and the party files before the run in in/CURRGAME). The run is cut off when
 * the port has made as many driver calls as the original had. Scenarios:
 *   idle     the tavern at the start of the game, no input: the first frames of the idle loop. */

#include "ScriptedInput.h"
#include "TestUtil.h"
#include "gen/Prog4Gen.h"
#include "shared/GeneratedProgram.h"
#include "vid/Fonts.h"

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

// stops the program when the port has recorded `limit` calls; input comes from the script
class Limited : public InputScript {
public:
	Limited(Vid::Video &v, const std::vector<Step> &s, const Recorder &r, size_t limit, unsigned clock, const std::vector<std::string> *want)
		: InputScript(v, s), _rec(r), _limit(limit), _clock(clock), _want(want), _checked(0), _matched(0), _failed(false) {}
	// stops when enough calls were made, or (with a reference) at the first call that differs from it
	bool pump() {
		while (_want && !_failed && _checked < _rec.lines.size()) {
			const std::string &l = _rec.lines[_checked++];
			if (TraceCompare::ahOf(l) == 0x2B)
				continue;
			if (_matched < _want->size() && TraceCompare::normalise(l) != (*_want)[_matched])
				_failed = true;
			else
				_matched++;
		}
		return !_failed && _rec.lines.size() < _limit && InputScript::pump();
	}
	bool failed() const { return _failed; }
	Shared::uint16 dosClock() { return (Shared::uint16)_clock; }

private:
	const Recorder &_rec;
	size_t _limit;
	unsigned _clock;
	const std::vector<std::string> *_want;
	size_t _checked, _matched;
	bool _failed;
};

struct Scenario {
	const char *name;
	const char *startFile;
	std::vector<Step> steps;
	unsigned clock;
};

bool run(const std::string &refRoot, const Scenario &sc, bool quiet = false) {
	const std::string ref = refRoot + "/" + sc.name;
	Tools::Snapshot start;
	if (!start.load((ref + "/" + sc.startFile).c_str())) {
		printf("  skipped %s (no %s in %s)\n", sc.name, sc.startFile, ref.c_str());
		return false;
	}
	const std::vector<std::string> want = withoutRefresh(TraceCompare::slice(ref + "/trace.txt", "PROG4.EXE", 1), true);
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
	Limited sys(v, sc.steps, rec, want.size(), sc.clock, quiet ? &want : 0);
	Shared::Runtime rt(v, sys);
	rt.openGameFiles(TestUtil::gameDir());
	rt.setWorkDir(work);
	class NoChildren : public Shared::Launcher {
	public:
		int started;
		NoChildren() : started(-1) {}
		int run(int program, Shared::Params &, Shared::Runtime &) { started = program; return kNotImplemented; }
	} children;
	Shared::runGenerated<Gen::Prog4Gen>(rt, &children, start.params, "PROG4.EXE");
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
	run(refEnv, idle);
TEST_MAIN_END()
