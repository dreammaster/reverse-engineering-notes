/* PROG1 against the original: needs the reference files produced by yemu (docs1/overview.md):
 *   <ref>/05_PROG1_start.ydmp  <ref>/05_PROG1_end.ydmp  <ref>/trace.txt   (yemu ... --dump <ref> --log62 <ref>/trace.txt)
 * with <ref> = $YENDOR1_REF_DIR. Skipped when they are absent. The C++ program runs from the snapshot taken when the original
 * started, its driver calls are compared with the original's trace (the pixel dissolve, whose order is random in both, is compared
 * as an unordered set) and the final video memory is compared with the original's. */

#include "TestUtil.h"
#include "TraceCompare.h"
#include "prog1/Prog1.h"
#include "tools/Snapshot.h"
#include "vid/Fonts.h"
#include "vid/Trace.h"

#include <algorithm>
#include <fstream>
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
	long keyAfter;
	uint32_t now;
	ScriptedSystem() : polls(0), keyAfter(2000), now(0) {}
	bool pump() { return true; }
	int pollKey() { return ++polls == keyAfter ? 0x0D : -1; }   // Enter once the program waits
	void flushKeys() { polls = 0; }
	Shared::uint32 ticks() { return now++; }
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
};





} // namespace

static void testProg1Intro() {
	const char *refEnv = getenv("YENDOR1_REF_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REF_DIR to a yemu --dump/--log62 output and have the game files)\n");
		return;
	}
	const std::string ref = refEnv;
	Tools::Snapshot start, end;
	if (!start.load((ref + "/05_PROG1_start.ydmp").c_str()) || !end.load((ref + "/05_PROG1_end.ydmp").c_str())) {
		printf("  skipped (no 05_PROG1 snapshots in %s)\n", refEnv);
		return;
	}
	std::vector<std::string> want = TraceCompare::slice(ref + "/trace.txt", "PROG1.EXE");
	for (size_t i = 0; i < want.size(); i++)
		want[i] = TraceCompare::normalise(want[i]);
	CHECK(!want.empty());

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
	const std::string diff = TraceCompare::compare(want, rec.lines);
	if (!diff.empty())
		printf("  trace mismatch: %s\n", diff.c_str());
	CHECK(diff.empty());
	printf("  %zu calls in the original, %zu in the port\n", want.size(), rec.lines.size());

	// final video memory
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

TEST_MAIN_BEGIN("prog1 against the original")
	testProg1Intro();
TEST_MAIN_END()
