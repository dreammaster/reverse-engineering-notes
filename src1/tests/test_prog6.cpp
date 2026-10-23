/* PROG6 (the Bookmarker) against the original. Needs the yemu reference run $YENDOR1_REFP6_DIR made with tests/refs/prog6_bookmarker.txt
 * (trace.txt, 06_PROG6_start/end.ydmp, 07_PROG6_start.ydmp, 08_PROG0_start.ydmp): from the book menu of a game in party state 2 the user
 * opens the Bookmarker twice. First time: Down Down, Enter ("RESTORE?"), Esc, Up, a click on "Save and return to DOS" ("SAVE?"), a click on
 * the Esc icon, Esc (exit code 0). Second time: Enter, Enter (confirm) -- PROG0 is started to save, and the port stops there. */

#include "ScriptedInput.h"
#include "TestUtil.h"
#include "prog6/Prog6.h"
#include "vid/Fonts.h"

using namespace Yendor1;
using namespace ScriptedInput;

namespace {

class OnlyProg0 : public Shared::Launcher {
public:
	int program, mode;
	OnlyProg0() : program(-1), mode(-1) {}
	int run(int p, Shared::Params &params, Shared::Runtime &) {
		program = p;
		mode = params.parm(0x0E);
		return kNotImplemented;
	}
};

// one run of PROG6 from a start snapshot: returns the exit code; `endSnap` is compared with the video memory afterwards
void checkRun(const std::string &ref, const char *startFile, const char *endFile, int occurrence, const std::vector<Step> &steps, int wantCode,
              OnlyProg0 &launcher) {
	Tools::Snapshot start, end;
	if (!start.load((ref + "/" + startFile).c_str()) || !end.load((ref + "/" + endFile).c_str())) {
		printf("  skipped (no %s / %s in %s)\n", startFile, endFile, ref.c_str());
		return;
	}
	const std::vector<std::string> want = withoutRefresh(TraceCompare::slice(ref + "/trace.txt", "PROG6.EXE", occurrence), true);
	CHECK(!want.empty());

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	Recorder rec;
	v.setTrace(&rec);
	InputScript sys(v, steps);
	Shared::Runtime rt(v, sys);
	CHECK(rt.openGameFiles(TestUtil::gameDir()));
	Prog6::Prog6 prog(rt, start.params, launcher);
	const int code = prog.run();
	v.setTrace(0);
	CHECK_EQ(code, wantCode);
	CHECK(sys.finished());

	const std::vector<std::string> got = withoutRefresh(rec.lines, true);
	const std::string diff = TraceCompare::compare(want, got);
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		showContext(diff, want, got);
	}
	CHECK(diff.empty());
	printf("  run %d: %zu calls in the original, %zu in the port\n", occurrence, want.size(), got.size());
	CHECK_EQ(videoDifferences(v, end), 0);
}

} // namespace

TEST_MAIN_BEGIN("prog6 against the original")
	const char *refEnv = getenv("YENDOR1_REFP6_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REFP6_DIR to a yemu --dump/--log62 output and have the game files)\n");
		return 0;
	}
	const std::string ref = refEnv;

	std::vector<Step> first;
	first.push_back(ext(0x50)); first.push_back(ext(0x50));
	first.push_back(key(0x0D));                            // Restore: "RESTORE?"
	first.push_back(key(0x1B));                            // not confirmed
	first.push_back(ext(0x48));
	first.push_back(moveTo(300, 146)); first.push_back(click());     // Save and return to DOS: "SAVE?"
	first.push_back(moveTo(392, 326)); first.push_back(click());     // the Esc icon
	first.push_back(key(0x1B));                            // leave the Bookmarker
	OnlyProg0 none;
	checkRun(ref, "06_PROG6_start.ydmp", "06_PROG6_end.ydmp", 1, first, 0, none);

	std::vector<Step> second;
	second.push_back(key(0x0D));                           // Save and continue to play
	second.push_back(key(0x0D));                           // confirmed: PROG0 starts
	OnlyProg0 saver;
	checkRun(ref, "07_PROG6_start.ydmp", "08_PROG0_start.ydmp", 2, second, Shared::Launcher::kNotImplemented, saver);
	CHECK_EQ(saver.program, 0);
	CHECK_EQ(saver.mode, 2);
TEST_MAIN_END()
