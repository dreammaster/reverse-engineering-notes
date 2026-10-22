/* PROG2 (the prologue) against the original: needs the yemu reference files of the run that goes on to the book menu
 * ($YENDOR1_REF2_DIR: trace.txt, 06_PROG2_start.ydmp and 06_PROG2_end.ydmp; see test_prog1.cpp and docs1/overview.md).
 * The port starts from the snapshot taken when the original started PROG2 and its driver calls are compared with the original's trace.
 * Three things in that trace come from the outside world and are reproduced here:
 *   - the random generator's seed (the original seeds it from the DOS clock when the dissolve starts; 7943 is the value that reproduces
 *     the dissolve of that run, found by searching the 16-bit seeds). The dissolve gets trapped in the generator's short cycles and is
 *     reseeded from the clock every 602 failed draws; only the first of these clock readings was recovered, so the order of the pixels
 *     can differ from the original's late in the dissolve. The set of pixels copied is the same, and is what is compared; the one
 *     visible trace of the order, the coordinates left in the context by the last pixel, is not compared,
 *   - the generator's state when page 3 begins: the dissolve before it leaves the generator in a state that depends on the clock
 *     readings that could not all be recovered, so the test sets the state that reproduces page 3's animation timing (8817, found by
 *     searching all states against the order of the animation events in the original's trace),
 *   - when the page waits end: pages 1 and 2 time out (the test clock runs fast), the others end with a click on the "More" icon, which
 *     the test delivers once the port has made as many driver calls as the original had made when its click came,
 *   - the cursor position (the click lands on the icon at 512,326) and the pointer-refresh calls (2Bh), which depend on mouse movement
 *     and are left out on both sides. */

#include "TestUtil.h"
#include "TraceCompare.h"
#include "prog2/Prog2.h"
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

// A clock that runs one tick per query and a mouse that clicks the "More" icon when the port has reached the next boundary
class ScriptedSystem : public Shared::System {
public:
	Vid::Video &video;
	const Recorder &rec;
	std::vector<size_t> clickAt;     // number of recorded calls at which each click is delivered
	size_t next;
	uint32_t now;
	unsigned clocks;
	Shared::Rng *rng;                // given a known state when page 3 begins (see the file comment)
	size_t seedAt;
	ScriptedSystem(Vid::Video &v, const Recorder &r) : video(v), rec(r), next(0), now(0), clocks(0), rng(0), seedAt(0) {}
	bool pump() { return true; }
	int pollKey() {
		if (rng && rec.lines.size() >= seedAt) {
			rng->setSeed(8817);
			rng = 0;
		}
		if (next < clickAt.size() && rec.lines.size() >= clickAt[next]) {
			next++;
			video.ctx.mouseFlags |= 0x20;
		}
		return -1;
	}
	void flushKeys() {}
	Shared::uint32 ticks() { return now++; }
	// The original's clock readings only matter to the order of the dissolve (see the file comment): the first reseed of the original's
	// run read 7962 (1Fh seconds, 1Ah hundredths); later ones are not known and any changing value will do.
	Shared::uint16 dosClock() {
		static const Shared::uint16 known[] = {7962};
		const Shared::uint16 v = clocks < 1 ? known[clocks] : (Shared::uint16)(0x2058 + 0x61 * clocks);
		clocks++;
		return v;
	}
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
};

} // namespace

TEST_MAIN_BEGIN("prog2 against the original")
	const char *refEnv = getenv("YENDOR1_REF2_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REF2_DIR to a yemu --dump/--log62 output and have the game files)\n");
		return 0;
	}
	const std::string ref = refEnv;
	Tools::Snapshot start, end;
	if (!start.load((ref + "/06_PROG2_start.ydmp").c_str()) || !end.load((ref + "/06_PROG2_end.ydmp").c_str())) {
		printf("  skipped (no 06_PROG2_start/end.ydmp in %s)\n", refEnv);
		return 0;
	}
	std::vector<std::string> all = TraceCompare::slice(ref + "/trace.txt", "PROG2.EXE", 1);
	CHECK(!all.empty());
	std::vector<std::string> want;
	for (size_t i = 0; i < all.size(); i++)
		if (TraceCompare::ahOf(all[i]) != 0x2B)
			want.push_back(TraceCompare::normalise(all[i]));

	// the driver calls that start a page (vidCopyScreen0To1AndShow, 14h) tell where each page's wait ended; the click on the icon is
	// six calls (hide, tile, show, hide, tile, show) before it
	std::vector<size_t> pageStarts;
	for (size_t i = 0; i < want.size(); i++)
		if (TraceCompare::ahOf(want[i]) == 0x14)
			pageStarts.push_back(i);
	CHECK_EQ(pageStarts.size(), (size_t)5);

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	// the pointer sits on the "More" icon (the original's run moved it there during the first page)
	{
		const uint16_t x1 = v.ctx.x1, y1 = v.ctx.y1;
		v.ctx.x1 = 512;
		v.ctx.y1 = 326;
		v.call(0x2C);
		v.ctx.x1 = x1;
		v.ctx.y1 = y1;
	}
	Recorder rec;
	v.setTrace(&rec);
	ScriptedSystem sys(v, rec);
	for (size_t k = 1; k < pageStarts.size(); k++)
		sys.clickAt.push_back(pageStarts[k] - 6);
	sys.clickAt.push_back(want.size() - 1 - 6);
	sys.seedAt = pageStarts[2] + 1;
	Shared::Runtime rt(v, sys);
	CHECK(rt.openGameFiles(TestUtil::gameDir()));
	printf("  parm_03 = %d\n", start.params.parm(3));

	Prog2::Prog2 prog(rt, start.params);
	prog.rng().setSeed(7943);
	sys.rng = &prog.rng();
	const int code = prog.run();
	v.setTrace(0);
	CHECK_EQ(code, 0);

	for (size_t i = 0; i < rec.lines.size(); i++)
		rec.lines[i] = TraceCompare::normalise(rec.lines[i]);
	// the coordinates the last pixel of the dissolve leaves in the context depend on the order (see above); they stay there until
	// the next tile is drawn
	struct Mask {
		static void afterPixels(std::vector<std::string> &lines) {
			for (size_t i = 1; i < lines.size(); i++) {
				if (!(TraceCompare::isPixelOp(lines[i - 1]) && !TraceCompare::isPixelOp(lines[i])))
					continue;
				for (size_t j = i; j < lines.size() && TraceCompare::ahOf(lines[j]) != 0x0A; j++)
					lines[j] = TraceCompare::maskField(TraceCompare::maskField(lines[j], " x1="), " y1=");
			}
		}
	};
	Mask::afterPixels(want);
	Mask::afterPixels(rec.lines);
	const std::string diff = TraceCompare::compare(want, rec.lines);
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		unsigned long atv = 0;
		const size_t at = sscanf(diff.c_str(), "call %lu", &atv) == 1 ? (size_t)atv : 0;
		for (size_t k = at > 8 ? at - 8 : 0; at && k < at + 3 && k < want.size() && k < rec.lines.size(); k++)
			printf("   [%lu] want %.70s\n        got  %.70s\n", (unsigned long)k, want[k].c_str(), rec.lines[k].c_str());
	}
	CHECK(diff.empty());
	printf("  %zu calls in the original, %zu in the port\n", want.size(), rec.lines.size());

	int badBytes = 0, firstPlane = -1, firstOff = -1;
	for (int p = 0; p < 4; p++)
		for (int o = 0; o < 65536; o++)
			if (v.planeData(p)[o] != end.planes[p][o]) {
				if (!badBytes) { firstPlane = p; firstOff = o; }
				badBytes++;
			}
	if (badBytes) {
		printf("  video memory differs in %d bytes (first: plane %d offset %04X)\n", badBytes, firstPlane, firstOff);
		int shown = 0;
		for (int p = 0; p < 4 && shown < 24; p++)
			for (int o = 0; o < 65536 && shown < 24; o++)
				if (v.planeData(p)[o] != end.planes[p][o]) {
					printf("    plane %d page %d row %d byte %d (x=%d): port %02X original %02X\n", p, o >> 15, (o & 0x7FFF) / 80,
					       (o & 0x7FFF) % 80, ((o & 0x7FFF) % 80) * 8, v.planeData(p)[o], end.planes[p][o]);
					shown++;
				}
	}
	CHECK_EQ(badBytes, 0);
TEST_MAIN_END()
