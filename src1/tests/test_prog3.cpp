/* PROG3 (the book menu) against the original. Needs the yemu reference run $YENDOR1_REFP3_DIR (trace.txt, 05_PROG3_start.ydmp and
 * 05_PROG3_end.ydmp) of a game in party state 2 where the user:
 *   Enter (Open book: "no players assembled"), a key, Down + Enter (View: no players), a key, Down Down + Enter (Remove),
 *   a key, Down x3 + Enter (Assemble), a key, a click on "Open book", a click, Up (Close the book) + Enter (the confirmation),
 *   Esc, Up + Enter (Introduction: exit code 8).
 * See docs1/overview.md for how such a run is made. The port starts from the snapshot taken when the original started PROG3. */

#include "ScriptedInput.h"
#include "TestUtil.h"
#include "prog3/Prog3.h"
#include "vid/Fonts.h"

using namespace Yendor1;
using namespace ScriptedInput;

namespace {

class NoChildren : public Shared::Launcher {
public:
	int started;
	NoChildren() : started(-1) {}
	int run(int program, Shared::Params &, Shared::Runtime &) {
		started = program;
		return kNotImplemented;
	}
};

} // namespace

TEST_MAIN_BEGIN("prog3 against the original")
	const char *refEnv = getenv("YENDOR1_REFP3_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REFP3_DIR to a yemu --dump/--log62 output and have the game files)\n");
		return 0;
	}
	const std::string ref = refEnv;
	Tools::Snapshot start, end;
	if (!start.load((ref + "/05_PROG3_start.ydmp").c_str()) || !end.load((ref + "/05_PROG3_end.ydmp").c_str())) {
		printf("  skipped (no 05_PROG3_start/end.ydmp in %s)\n", refEnv);
		return 0;
	}
	const std::vector<std::string> want = withoutRefresh(TraceCompare::slice(ref + "/trace.txt", "PROG3.EXE", 1), true);
	CHECK(!want.empty());

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	Recorder rec;
	v.setTrace(&rec);

	std::vector<Step> steps;
	steps.push_back(key(0x0D));                       // Open book: "no players assembled"
	steps.push_back(key(0x0D));                       // closes the message
	steps.push_back(ext(0x50)); steps.push_back(key(0x0D)); steps.push_back(key(0x0D));                                       // View
	steps.push_back(ext(0x50)); steps.push_back(ext(0x50)); steps.push_back(key(0x0D)); steps.push_back(key(0x0D));          // Remove
	steps.push_back(ext(0x50)); steps.push_back(ext(0x50)); steps.push_back(ext(0x50)); steps.push_back(key(0x0D)); steps.push_back(key(0x0D));   // Assemble
	steps.push_back(moveTo(330, 188)); steps.push_back(click()); steps.push_back(click());     // a click on Open book, a click to close
	steps.push_back(ext(0x48)); steps.push_back(key(0x0D));                                    // Close the book: the confirmation
	steps.push_back(key(0x1B));                                                                // Esc: back to the menu
	steps.push_back(ext(0x48)); steps.push_back(key(0x0D));                                    // Introduction

	InputScript sys(v, steps);
	Shared::Runtime rt(v, sys);
	CHECK(rt.openGameFiles(TestUtil::gameDir()));
	NoChildren children;
	Prog3::Prog3 prog(rt, start.params, children);
	const int code = prog.run();
	v.setTrace(0);
	CHECK_EQ(code, 8);
	CHECK(sys.finished());

	const std::vector<std::string> got = withoutRefresh(rec.lines, true);
	const std::string diff = TraceCompare::compare(want, got);
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		showContext(diff, want, got);
	}
	CHECK(diff.empty());
	printf("  %zu calls in the original, %zu in the port\n", want.size(), got.size());
	CHECK_EQ(videoDifferences(v, end), 0);
TEST_MAIN_END()
