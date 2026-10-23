/* The whole game, SW.EXE's shell and every program it runs, against a yemu reference run made from the original SW.EXE ($YENDOR1_REFG_DIR:
 * trace.txt, trace.txt.events and in/ with the CURRGAME and SAVEGAME.A files the run started with; see tests/refs/README.md). The keys of the
 * original run are delivered when the port has made as many driver calls; the run stops at the first call that differs. */

#include "ReplaySystem.h"
#include "ScriptedInput.h"
#include "TestUtil.h"
#include "sw/Shell.h"
#include "vid/Fonts.h"

#include <string.h>

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

} // namespace

TEST_MAIN_BEGIN("the whole game against the original")
	const char *refEnv = getenv("YENDOR1_REFG_DIR");
	if (!refEnv || !TestUtil::haveGameFiles()) {
		printf("  skipped (set YENDOR1_REFG_DIR, see the comment in test_game.cpp)\n");
		return 0;
	}
	const std::string ref = refEnv;
	std::vector<std::string> want;
	{
		const std::vector<std::string> raw = TraceCompare::until(ref + "/trace.txt", "\x01never");
		want = withoutRefresh(raw, true);
	}
	CHECK(!want.empty());

	const std::string work = "test_game_work";
	MKDIR(work.c_str());
	MKDIR((work + "/CURRGAME").c_str());
	MKDIR((work + "/SAVEGAME.A").c_str());
	static const char *files[] = {"PARTY.DAT", "NPC.DAT", "AREA.DAT", "BATPIC.DAT"};
	for (size_t i = 0; i < 4; i++)
		copyFile(ref + "/in/CURRGAME/" + files[i], work + "/CURRGAME/" + files[i]);
	for (size_t i = 0; i < 3; i++)
		copyFile(ref + "/in/SAVEGAME.A/" + files[i], work + "/SAVEGAME.A/" + files[i]);

	Vid::Video v;
	Vid::Fonts fonts;
	fonts.load("../fonts");
	fonts.attach(v);
	Recorder rec;
	v.setTrace(&rec);
	std::vector<Step> none;
	KeyReads keys;
	keys.load(ref + "/trace.txt.events", 0);
	Replay sys(v, none, rec, want.size() * 3, 0x1234, &want, keys);
	Shared::Runtime rt(v, sys);
	// YENDOR1_SWAP=n: the reference run was made with PROG4.EXE replaced by PROGn.EXE (a way to reach a program without playing to it)
	class SwapLauncher : public Sw::DefaultLauncher {
	public:
		int target;
		SwapLauncher() : target(0) {}
		int run(int program, Shared::Params &params, Shared::Runtime &rt) {
			const int code = Sw::DefaultLauncher::run(target && program == 4 ? target : program, params, rt);
			if (getenv("YENDOR1_EXITS"))
				fprintf(stderr, "program %d exited with %d\n", program, code);
			return code;
		}
	} launcher;
	if (const char *swap = getenv("YENDOR1_SWAP")) {
		launcher.target = atoi(swap);
		sys.alias(launcher.target, "PROG4.EXE");
	}
	Sw::Config cfg;
	cfg.dataDir = TestUtil::gameDir();
	cfg.workDir = work;
	Sw::Shell shell(rt, launcher, cfg);
	shell.run();
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
	if (!diff.empty()) {
		printf("  trace mismatch: %s\n", diff.c_str());
		showContext(diff, expect, got);
	}
	CHECK(diff.empty());
	printf("  %zu of %zu calls compared\n", n, want.size());
TEST_MAIN_END()
