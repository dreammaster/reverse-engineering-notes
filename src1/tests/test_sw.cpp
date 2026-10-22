/* Sw (the shell) + Prog1 against the original, from a fresh machine: needs the yemu reference files (see test_prog1.cpp):
 *   $YENDOR1_REF_DIR/trace.txt  and  06_PROG2_start.ydmp (the machine state when the original started PROG2).
 * The C++ shell runs on a scratch copy of the CURRGAME files; when it asks for a program that is not implemented (PROG2) it stops.
 * Every INT 62h call from the first one up to that point is compared with the original's trace, and so is the video memory. */

#include "TestUtil.h"
#include "TraceCompare.h"
#include "sw/Shell.h"
#include "tools/Snapshot.h"
#include "vid/Fonts.h"
#include "vid/Trace.h"

#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

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
	int pollKey() { return ++polls % 2000 == 0 ? 0x0D : -1; }
	void flushKeys() { polls = 0; }
	Shared::uint32 ticks() { return now++; }
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
};

// DefaultLauncher for PROG0/PROG1; the first request for anything else is recorded (the shell then ends the run)
class StopAtUnimplemented : public Sw::Launcher {
public:
	Sw::DefaultLauncher inner;
	int stoppedAt;
	StopAtUnimplemented() : stoppedAt(-1) {}
	int run(int program, Shared::Params &params, Shared::Runtime &rt) {
		const int r = inner.run(program, params, rt);
		if (r == kNotImplemented && stoppedAt < 0)
			stoppedAt = program;
		return r;
	}
};

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





} // namespace

// refVar: yemu output directory; stopSnap: snapshot taken when the original started the first program the port lacks (named stopProgram,
// number stopNumber); state: the party header's game state byte to run with (the reference run used a PARTY.DAT patched the same way)
static void checkShell(const char *refVar, const char *stopSnap, const char *stopProgram, int stopNumber, int state) {
	const char *refEnv = getenv(refVar);
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped %s (set it to a yemu --dump/--log62 output and have the game files)\n", refVar);
		return;
	}
	const std::string ref = refEnv;
	Tools::Snapshot atStop;
	if (!atStop.load((ref + "/" + stopSnap).c_str())) {
		printf("  skipped %s (no %s in %s)\n", refVar, stopSnap, refEnv);
		return;
	}
	std::vector<std::string> want = TraceCompare::until(ref + "/trace.txt", stopProgram);
	CHECK(!want.empty());
	for (size_t i = 0; i < want.size(); i++)
		want[i] = TraceCompare::normalise(want[i]);

	// scratch work directory with the CURRGAME files (the shell empties BATPIC.DAT and may rewrite PARTY.DAT)
	const std::string work = "test_sw_work";
	MKDIR(work.c_str());
	MKDIR((work + "/CURRGAME").c_str());
	static const char *files[] = {"PARTY.DAT", "NPC.DAT", "AREA.DAT", "BATPIC.DAT"};
	for (size_t i = 0; i < 4; i++)
		CHECK(copyFile(std::string(TestUtil::gameDir()) + "/CURRGAME/" + files[i], work + "/CURRGAME/" + files[i]));
	{
		Shared::Blob party;
		CHECK(party.load((work + "/CURRGAME/PARTY.DAT").c_str()));
		if (!party.empty()) {
			party.data()[0x25] = (Shared::byte)state;
			FILE *f = fopen((work + "/CURRGAME/PARTY.DAT").c_str(), "wb");
			if (f) { fwrite(party.data(), 1, party.size(), f); fclose(f); }
		}
	}

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	Recorder rec;
	v.setTrace(&rec);
	ScriptedSystem sys;
	Shared::Runtime rt(v, sys);
	StopAtUnimplemented launcher;
	Sw::Config cfg;
	cfg.dataDir = TestUtil::gameDir();
	cfg.workDir = work;
	Sw::Shell shell(rt, launcher, cfg);
	shell.run();
	v.setTrace(0);
	CHECK_EQ(launcher.stoppedAt, stopNumber);

	for (size_t i = 0; i < rec.lines.size(); i++)
		rec.lines[i] = TraceCompare::normalise(rec.lines[i]);
	const std::string diff = TraceCompare::compare(want, rec.lines);
	if (!diff.empty())
		printf("  trace mismatch: %s\n", diff.c_str());
	CHECK(diff.empty());
	printf("  state %d, up to %s: %zu calls in the original, %zu in the port\n", state, stopProgram, want.size(), rec.lines.size());

	int bad = 0, plane = -1, off = -1;
	for (int p = 0; p < 4; p++)
		for (int o = 0; o < 65536; o++)
			if (v.planeData(p)[o] != atStop.planes[p][o]) {
				if (!bad) { plane = p; off = o; }
				bad++;
			}
	if (bad)
		printf("  video memory differs in %d bytes (first: plane %d offset %04X)\n", bad, plane, off);
	CHECK_EQ(bad, 0);
}

TEST_MAIN_BEGIN("sw shell + prog1 from a fresh machine")
	checkShell("YENDOR1_REF_DIR", "06_PROG2_start.ydmp", "PROG2.EXE", 2, 0);       // new game: ... PROG1, then the prologue
	checkShell("YENDOR1_REFS1_DIR", "06_PROG3_start.ydmp", "PROG3.EXE", 3, 1);      // state 1: PROG1 (mode 1), then the book menu
	checkShell("YENDOR1_REFS2_DIR", "05_PROG3_start.ydmp", "PROG3.EXE", 3, 2);      // state 2: PROG1 (mode 3), then the book menu
TEST_MAIN_END()
