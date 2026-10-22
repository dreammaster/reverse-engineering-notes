/* PROG1 against the original: needs reference files produced by yemu (docs1/overview.md):
 *   yemu <game> SW.EXE --fonts fonts --script <steps> --dump <ref> --log62 <ref>/trace.txt
 * Two runs are used:
 *   $YENDOR1_REF_DIR   a plain start (PROG1 mode 0, the full intro):      05_PROG1_start/end.ydmp
 *   $YENDOR1_REF2_DIR  the run that goes on to the book menu and its "About the Authors" entry (PROG1 mode 2, the second launch):
 *                      08_PROG1_start.ydmp
 * Each check is skipped when its files are absent. The C++ program runs from the snapshot taken when the original started, its driver
 * calls are compared with the original's trace (the pixel dissolve, whose order is random in both, as an unordered set) and, when an end
 * snapshot exists, the final video memory is compared too. */

#include "TestUtil.h"
#include "TraceCompare.h"
#include "prog1/Prog1.h"
#include "tools/Snapshot.h"
#include "vid/Fonts.h"
#include "vid/Trace.h"

#include <stdlib.h>
#include <string>
#include <vector>

using namespace Yendor1;

namespace {

class Recorder : public Vid::CallTrace {
public:
	std::vector<std::string> lines;
	void onCall(const Vid::Video &v, int ah, int al) { lines.push_back(Vid::formatCall(v, ah, al)); }
};

class ScriptedSystem : public Shared::System {
public:
	long polls;
	uint32_t now;
	ScriptedSystem() : polls(0), now(0) {}
	bool pump() { return true; }
	int pollKey() { return ++polls == 2000 ? 0x0D : -1; }   // Enter once the program waits
	void flushKeys() { polls = 0; }
	Shared::uint32 ticks() { return now++; }
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
};

// refVar: environment variable naming the yemu output directory; endFile may be 0; occurrence: which PROG1 launch in the trace
void checkProg1(const char *refVar, const char *startFile, const char *endFile, int occurrence, int expectMode) {
	const char *refEnv = getenv(refVar);
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped %s (set it to a yemu --dump/--log62 output and have the game files)\n", refVar);
		return;
	}
	const std::string ref = refEnv;
	Tools::Snapshot start, end;
	if (!start.load((ref + "/" + startFile).c_str()) || (endFile && !end.load((ref + "/" + endFile).c_str()))) {
		printf("  skipped %s (no %s in %s)\n", refVar, startFile, refEnv);
		return;
	}
	CHECK_EQ(start.params.parm(3), expectMode);
	std::vector<std::string> want = TraceCompare::slice(ref + "/trace.txt", "PROG1.EXE", occurrence);
	CHECK(!want.empty());
	for (size_t i = 0; i < want.size(); i++)
		want[i] = TraceCompare::normalise(want[i]);

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	Recorder rec;
	v.setTrace(&rec);
	ScriptedSystem sys;
	Shared::Runtime rt(v, sys);
	CHECK(rt.openGameFiles(TestUtil::gameDir()));

	Prog1::Prog1 prog(rt, start.params);
	const int code = prog.run();
	v.setTrace(0);
	CHECK_EQ(code, 0);

	for (size_t i = 0; i < rec.lines.size(); i++)
		rec.lines[i] = TraceCompare::normalise(rec.lines[i]);
	// a mouse movement while the program waits makes it call vidMouseRefresh (2Bh); that depends on the input, not the program
	while (!want.empty() && TraceCompare::ahOf(want.back()) == 0x2B)
		want.pop_back();
	while (!rec.lines.empty() && TraceCompare::ahOf(rec.lines.back()) == 0x2B)
		rec.lines.pop_back();
	const std::string diff = TraceCompare::compare(want, rec.lines);
	if (!diff.empty())
		printf("  trace mismatch: %s\n", diff.c_str());
	CHECK(diff.empty());
	printf("  mode %d: %zu calls in the original, %zu in the port\n", expectMode, want.size(), rec.lines.size());

	if (endFile) {
		int badBytes = 0, firstPlane = -1, firstOff = -1;
		for (int p = 0; p < 4; p++)
			for (int o = 0; o < 65536; o++)
				if (v.planeData(p)[o] != end.planes[p][o]) {
					if (!badBytes) { firstPlane = p; firstOff = o; }
					badBytes++;
				}
		if (badBytes)
			printf("  video memory differs in %d bytes (first: plane %d offset %04X)\n", badBytes, firstPlane, firstOff);
		CHECK_EQ(badBytes, 0);
	}
}

} // namespace

TEST_MAIN_BEGIN("prog1 against the original")
	checkProg1("YENDOR1_REF_DIR", "05_PROG1_start.ydmp", "05_PROG1_end.ydmp", 1, 0);
	checkProg1("YENDOR1_REF2_DIR", "08_PROG1_start.ydmp", 0, 2, 2);
	checkProg1("YENDOR1_REFS1_DIR", "05_PROG1_start.ydmp", "05_PROG1_end.ydmp", 1, 1);   // party state 1: the abortable intro
	checkProg1("YENDOR1_REFS2_DIR", "04_PROG1_start.ydmp", "04_PROG1_end.ydmp", 1, 3);   // party state 2: just the background
TEST_MAIN_END()
