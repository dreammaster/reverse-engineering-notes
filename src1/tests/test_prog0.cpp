/* PROG0 (the saved-game copier) against the original. PROG0 draws nothing with the video driver, so its result is the files. Needs
 * $YENDOR1_REFP0_DIR with the CURRGAME\ and SAVEGAME.A\ files (PARTY, NPC, AREA) before and after yemu runs of the original (scripts in
 * tests/refs/prog0_*.txt, run on a game directory whose current and saved game differ):
 *   in_diff  -> out_save     PROG6 "Save and continue" (PROG0 mode 2: CURRGAME -> SAVEGAME.A)
 *   in_diff  -> out_restore  PROG6 "Restore from last save" (mode 3: SAVEGAME.A -> CURRGAME)
 *   in_diff1 -> out_new      SW.EXE starting a game that has not begun (mode 1: the only saved game is copied to CURRGAME) */

#include "ScriptedInput.h"
#include "TestUtil.h"
#include "prog0/Prog0.h"
#include "shared/Blob.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

using namespace Yendor1;

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

const char *kDirs[2] = {"CURRGAME", "SAVEGAME.A"};
const char *kFiles[3] = {"PARTY.DAT", "NPC.DAT", "AREA.DAT"};

void checkCase(const std::string &ref, const char *in, const char *out, int mode, int wantCode) {
	const std::string work = std::string("test_prog0_work_") + out;
	MKDIR(work.c_str());
	for (int d = 0; d < 2; d++) {
		MKDIR((work + "/" + kDirs[d]).c_str());
		for (int f = 0; f < 3; f++)
			CHECK(copyFile(ref + "/" + in + "/" + kDirs[d] + "/" + kFiles[f], work + "/" + kDirs[d] + "/" + kFiles[f]));
	}
	Vid::Video v;
	ScriptedInput::InputScript sys(v, std::vector<ScriptedInput::Step>());
	Shared::Runtime rt(v, sys);
	rt.setWorkDir(work);
	Shared::Params params;
	params.setParm(0x0E, (Shared::byte)mode);
	params.setParm(0x10, 'A');
	Prog0::Prog0 prog(rt, params);
	CHECK_EQ(prog.run(), wantCode);
	int bad = 0;
	for (int d = 0; d < 2; d++)
		for (int f = 0; f < 3; f++)
			if (!sameFile(work + "/" + kDirs[d] + "/" + kFiles[f], ref + "/" + out + "/" + kDirs[d] + "/" + kFiles[f])) {
				printf("  %s: %s\\%s differs from the original's\n", out, kDirs[d], kFiles[f]);
				bad++;
			}
	CHECK_EQ(bad, 0);
	printf("  mode %d: files match the original's\n", mode);
}

} // namespace

TEST_MAIN_BEGIN("prog0 against the original")
	const char *refEnv = getenv("YENDOR1_REFP0_DIR");
	if (!refEnv) {
		printf("  skipped (set YENDOR1_REFP0_DIR, see the comment in test_prog0.cpp)\n");
		return 0;
	}
	const std::string ref = refEnv;
	checkCase(ref, "in_diff", "out_save", 2, 0);
	checkCase(ref, "in_diff", "out_restore", 3, 0);
	checkCase(ref, "in_diff1", "out_new", 1, 0);
	// error codes: nothing saved in slot B; an unknown mode
	{
		Vid::Video v;
		ScriptedInput::InputScript sys(v, std::vector<ScriptedInput::Step>());
		Shared::Runtime rt(v, sys);
		rt.setWorkDir("test_prog0_work_out_save");
		Shared::Params params;
		params.setParm(0x0E, 2);
		params.setParm(0x10, 'B');
		Prog0::Prog0 missing(rt, params);
		CHECK_EQ(missing.run(), 9);
		params.setParm(0x0E, 7);
		Prog0::Prog0 bad(rt, params);
		CHECK_EQ(bad.run(), 0xFE);
	}
TEST_MAIN_END()
