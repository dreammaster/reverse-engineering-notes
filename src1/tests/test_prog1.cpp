/* PROG1 against the original: needs the reference files produced by yemu (docs1/overview.md):
 *   <ref>/05_PROG1_start.ydmp  <ref>/05_PROG1_end.ydmp  <ref>/trace.txt   (yemu ... --dump <ref> --log62 <ref>/trace.txt)
 * with <ref> = $YENDOR1_REF_DIR. Skipped when they are absent. The C++ program runs from the snapshot taken when the original
 * started, its driver calls are compared with the original's trace (the pixel dissolve, whose order is random in both, is compared
 * as an unordered set) and the final video memory is compared with the original's. */

#include "TestUtil.h"
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
};

std::vector<std::string> readSlice(const std::string &file, const std::string &program) {
	std::ifstream f(file.c_str());
	std::vector<std::string> out;
	std::string line;
	bool on = false;
	while (std::getline(f, line)) {
		if (!line.empty() && line[0] == '#') {
			if (on)
				break;
			on = line == "# start " + program;
			continue;
		}
		if (on)
			out.push_back(line);
	}
	return out;
}

// AL is only an argument of AH=0Dh, 12h and 2Dh; for the other functions it is whatever the caller's register held
std::string normalise(const std::string &l) {
	const int ah = (int)strtol(l.substr(0, 2).c_str(), 0, 16);
	std::string r = (ah == 0x0D || ah == 0x12 || ah == 0x2D) ? l : l.substr(0, 3) + "--" + l.substr(5);
	if (ah == 0x04) {
		// getPixel's colour field is its result: before the call it holds whatever the previous call left
		const size_t c = r.find(" col=");
		const size_t e = r.find(' ', c + 1);
		if (c != std::string::npos && e != std::string::npos)
			r = r.substr(0, c) + " col=-" + r.substr(e);
	}
	return r;
}

bool isPixelOp(const std::string &l) {
	return l.compare(0, 2, "03") == 0 || l.compare(0, 2, "04") == 0;
}

// compare two call sequences: exact, except that runs of pixel ops (03/04) only have to hold the same multiset
std::string compareTraces(const std::vector<std::string> &want, const std::vector<std::string> &got) {
	size_t i = 0, j = 0;
	while (i < want.size() && j < got.size()) {
		if (isPixelOp(want[i]) || isPixelOp(got[j])) {
			size_t ie = i, je = j;
			while (ie < want.size() && isPixelOp(want[ie])) ie++;
			while (je < got.size() && isPixelOp(got[je])) je++;
			std::vector<std::string> a(want.begin() + i, want.begin() + ie), b(got.begin() + j, got.begin() + je);
			std::sort(a.begin(), a.end());
			std::sort(b.begin(), b.end());
			if (a.size() != b.size())
				return "pixel run at call " + std::to_string(i) + ": want " + std::to_string(a.size()) + " ops, got " + std::to_string(b.size());
			for (size_t k = 0; k < a.size(); k++)
				if (a[k] != b[k])
					return "pixel run at call " + std::to_string(i) + " differs: want [" + a[k] + "] got [" + b[k] + "]";
			i = ie;
			j = je;
			continue;
		}
		if (want[i] != got[j])
			return "call " + std::to_string(i) + " (got index " + std::to_string(j) + "):\n   want " + want[i] + "\n   got  " + got[j];
		i++;
		j++;
	}
	if (i != want.size() || j != got.size())
		return "length differs: want " + std::to_string(want.size()) + " calls, got " + std::to_string(got.size());
	return "";
}

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
	std::vector<std::string> want = readSlice(ref + "/trace.txt", "PROG1.EXE");
	for (size_t i = 0; i < want.size(); i++)
		want[i] = normalise(want[i]);
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
		rec.lines[i] = normalise(rec.lines[i]);
	const std::string diff = compareTraces(want, rec.lines);
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
