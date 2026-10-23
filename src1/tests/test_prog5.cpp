/* PROG5 (the character screens) against the original. Needs $YENDOR1_REFP5_DIR with one sub-directory per yemu reference run (made
 * with the scripts in tests/refs/prog5_*.txt on a game in party state 2), each holding trace.txt, the NN_PROG5_start/end.ydmp
 * snapshots, and the party files before (`in/CURRGAME`) and after (`out/CURRGAME`) the run:
 *   create    book menu -> Create: sex, class, next picture, the name TESTER, one re-roll of the attributes, accept.
 *   assemble  book menu -> Assemble party members on a game with three characters (tests/refs/prog5_three_characters.txt made them):
 *             all three join (Space, Down, Space, Down, Space), Enter.
 *   view      book menu -> View on the assembled party: change the first character's equipment and keep it, change the second's
 *             and leave without keeping, leave the list (tests/refs/prog5_view.txt).
 * The random generator is given the state that reproduces the attribute rolls of the original run (found by searching all
 * 16-bit states for the two rolls shown on the screens). */

#include "ScriptedInput.h"
#include "TestUtil.h"
#include "prog5/Prog5.h"
#include "shared/Blob.h"
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

bool sameFile(const std::string &a, const std::string &b) {
	Shared::Blob x, y;
	if (!x.load(a.c_str()) || !y.load(b.c_str()) || x.size() != y.size())
		return false;
	return memcmp(x.data(), y.data(), x.size()) == 0;
}

struct Scenario {
	const char *name;
	const char *startFile, *endFile;
	unsigned seed;
	std::vector<Step> steps;
};

void run(const std::string &refRoot, const Scenario &sc) {
	const std::string ref = refRoot + "/" + sc.name;
	Tools::Snapshot start, end;
	if (!start.load((ref + "/" + sc.startFile).c_str()) || !end.load((ref + "/" + sc.endFile).c_str())) {
		printf("  skipped %s (no %s / %s in %s)\n", sc.name, sc.startFile, sc.endFile, ref.c_str());
		return;
	}
	const std::vector<std::string> want = withoutRefresh(TraceCompare::slice(ref + "/trace.txt", "PROG5.EXE", 1), true);
	CHECK(!want.empty());

	const std::string work = std::string("test_prog5_work_") + sc.name;
	MKDIR(work.c_str());
	MKDIR((work + "/CURRGAME").c_str());
	static const char *files[2] = {"PARTY.DAT", "BATPIC.DAT"};
	for (int i = 0; i < 2; i++)
		CHECK(copyFile(ref + "/in/CURRGAME/" + files[i], work + "/CURRGAME/" + files[i]));

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	start.apply(v);
	Recorder rec;
	v.setTrace(&rec);
	InputScript sys(v, sc.steps);
	Shared::Runtime rt(v, sys);
	CHECK(rt.openGameFiles(TestUtil::gameDir()));
	rt.setWorkDir(work);
	Prog5::Prog5 prog(rt, start.params);
	prog.rng().setSeed((uint16_t)sc.seed);
	const int code = prog.run();
	v.setTrace(0);
	CHECK_EQ(code, 0);
	CHECK(sys.finished());

	const std::vector<std::string> got = withoutRefresh(rec.lines, true);
	const std::string diff = TraceCompare::compare(want, got);
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		showContext(diff, want, got);
	}
	CHECK(diff.empty());
	printf("  %s: %zu calls in the original, %zu in the port\n", sc.name, want.size(), got.size());
	CHECK_EQ(videoDifferences(v, end), 0);
	for (int i = 0; i < 2; i++)
		if (!sameFile(work + "/CURRGAME/" + files[i], ref + "/out/CURRGAME/" + files[i])) {
			printf("  %s: CURRGAME/%s differs from the original's\n", sc.name, files[i]);
			CHECK(false);
		}
}

} // namespace

TEST_MAIN_BEGIN("prog5 against the original")
	const char *refEnv = getenv("YENDOR1_REFP5_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REFP5_DIR, see the comment in test_prog5.cpp)\n");
		return 0;
	}
	const std::string ref = refEnv;

	Scenario create;
	create.name = "create";
	create.startFile = "06_PROG5_start.ydmp";
	create.endFile = "06_PROG5_end.ydmp";
	create.seed = 6157;
	create.steps.push_back(key(0x0D));                        // Male
	create.steps.push_back(key(0x0D));                        // Miner
	create.steps.push_back(ext(0x4D));                        // the next picture
	create.steps.push_back(key(0x0D));
	for (const char *p = "Tester"; *p; p++)
		create.steps.push_back(key(*p));
	create.steps.push_back(key(0x0D));                        // the name is done
	create.steps.push_back(key(0x20));                        // roll again
	create.steps.push_back(key(0x0D));                        // keep
	run(ref, create);

	Scenario assemble;
	assemble.name = "assemble";
	assemble.startFile = "06_PROG5_start.ydmp";
	assemble.endFile = "06_PROG5_end.ydmp";
	assemble.seed = 1;
	assemble.steps.push_back(key(0x20));                      // the first character joins
	assemble.steps.push_back(ext(0x50));
	assemble.steps.push_back(key(0x20));
	assemble.steps.push_back(ext(0x50));
	assemble.steps.push_back(key(0x20));
	assemble.steps.push_back(key(0x0D));                      // assemble
	run(ref, assemble);

	Scenario view;
	view.name = "view";
	view.startFile = "06_PROG5_start.ydmp";
	view.endFile = "06_PROG5_end.ydmp";
	view.seed = 1;
	view.steps.push_back(key(0x0D));                          // the first character (in the party): the equipment can change
	view.steps.push_back(ext(0x4D)); view.steps.push_back(ext(0x4D));                       // two weapons on
	view.steps.push_back(ext(0x50)); view.steps.push_back(ext(0x4D));                       // armour row, next armour
	view.steps.push_back(ext(0x50)); view.steps.push_back(ext(0x4D));                       // shield row, next shield
	view.steps.push_back(key(0x0D));                          // keep
	view.steps.push_back(ext(0x50)); view.steps.push_back(key(0x0D));                       // the second character
	view.steps.push_back(ext(0x4D));
	view.steps.push_back(key(0x1B));                          // leave without keeping
	view.steps.push_back(key(0x1B));                          // leave the list
	run(ref, view);

	Scenario remove;
	remove.name = "remove";
	remove.startFile = "06_PROG5_start.ydmp";
	remove.endFile = "06_PROG5_end.ydmp";
	remove.seed = 1;
	remove.steps.push_back(ext(0x50)); remove.steps.push_back(key(0x0D));                   // the second character: the sheet
	remove.steps.push_back(key(0x1B));                        // not this time
	remove.steps.push_back(key(0x0D)); remove.steps.push_back(key(0x0D));                   // again, and confirm
	remove.steps.push_back(key(0x1B));                        // leave the list
	run(ref, remove);

	Scenario assigned;
	assigned.name = "remove_assigned";
	assigned.startFile = "06_PROG5_start.ydmp";
	assigned.endFile = "06_PROG5_end.ydmp";
	assigned.seed = 1;
	assigned.steps.push_back(key(0x0D));                      // a member of the party cannot be removed: the notice
	assigned.steps.push_back(key(0x0D));
	assigned.steps.push_back(key(0x1B));
	run(ref, assigned);
TEST_MAIN_END()
