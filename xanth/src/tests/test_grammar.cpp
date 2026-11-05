// print_obj grammar: compares formatObject with the original's print_obj for every object x style x verb. XANTH_DATA is the game folder (OBJECT.DAT);
// XANTH_PRINTOBJ is the tab separated file written by `host_test ... printobj <file>` (local only: it holds game text). Without the file only
// a few built-in properties are checked.
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "common/files.h"
#include "xanth/grammar.h"

static int g_failures = 0, g_checks = 0;

#define CHECK(cond, ...)                                                      \
	do {                                                                      \
		g_checks++;                                                           \
		if (!(cond)) {                                                        \
			g_failures++;                                                     \
			if (g_failures <= 40) {                                           \
				printf("FAIL %s:%d: ", __FILE__, __LINE__);                   \
				printf(__VA_ARGS__);                                          \
				printf("\n");                                                 \
			}                                                                 \
		}                                                                     \
	} while (0)

// The objects with a name callback in the original (their names depend on the game state; see NameProvider): compared once the callbacks exist.
static const int kNameCallbackObjects[] = {0x2, 0x15, 0x17, 0x31, 0x4A, 0x56, 0x6C, 0x6D, 0x73, 0x7D, 0x7E, 0x88, 0x89, 0x9B, 0xB5, 0xBD, 0xC7, 0xC8, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3,
                                           0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEE, 0xFC, 0x102, 0x103, 0x149, 0x14A, 0x14B, 0x14C, 0x14D, 0x158, 0x182, 0x183, 0x194, 0x195, 0x198};

static bool hasNameCallback(int obj) {
	for (int i = 0; i < ARRAYSIZE(kNameCallbackObjects); i++)
		if (kNameCallbackObjects[i] == obj)
			return true;
	return false;
}

int main() {
	const char *dir = getenv("XANTH_DATA");
	Common::GameFiles gf;
	Common::Bytes dat;
	if (!dir || !gf.setFolder(dir) || !gf.load("OBJECT.DAT", dat)) {
		printf("test_grammar: XANTH_DATA not set or OBJECT.DAT missing, skipped\n");
		return 0;
	}
	Xanth::ObjectNames names;
	CHECK(names.load(dat), "OBJECT.DAT loads");
	Xanth::ObjectModel om;
	Xanth::RecordNames recordNames(om, names);
	Xanth::GrammarContext ctx(om, recordNames, 1, 1);

	CHECK(names.name(om.nameOffset(1)) == "You" && names.name(om.altNameOffset(1)) == "me", "the player's names");
	CHECK(Xanth::formatObject(ctx, Xanth::PO_SUBJECT | Xanth::PO_CAPITAL, 1, "") == "You", "capitalised subject pronoun of the player");

	const char *path = getenv("XANTH_PRINTOBJ");
	if (path) {
		std::ifstream in(path, std::ios::binary);
		std::string line;
		long lines = 0, bad = 0, emptyWant = 0;
		while (std::getline(in, line)) {
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			size_t t1 = line.find('\t'), t2 = line.find('\t', t1 + 1), t3 = line.find('\t', t2 + 1);
			if (t1 == std::string::npos || t2 == std::string::npos || t3 == std::string::npos)
				continue;
			int obj = atoi(line.substr(0, t1).c_str());
			unsigned style = (unsigned)atoi(line.substr(t1 + 1, t2 - t1 - 1).c_str());
			std::string verb = line.substr(t2 + 1, t3 - t2 - 1);
			std::string want = line.substr(t3 + 1);
			std::string got = Xanth::formatObject(ctx, style, obj, verb);
			if (!om.valid(obj))
				got = "<null>";
			// objects without a name (offset 0) read whatever is at address 0 in the original
			if (hasNameCallback(obj) || (om.valid(obj) && om.nameOffset(obj) == 0))
				continue;
			if (want.empty() && !got.empty()) {
				emptyWant++;
				if (emptyWant <= 3)
					printf("  original gave nothing: obj %d style %02X verb '%s' (port '%s')\n", obj, style, verb.c_str(), got.c_str());
				continue;
			}
			lines++;
			if (got != want) {
				if (bad++ < 12)
					printf("  mismatch obj %d style %02X verb '%s': original '%s' vs port '%s'\n", obj, style, verb.c_str(), want.c_str(), got.c_str());
			}
		}
		CHECK(lines > 0, "the oracle file has results");
		CHECK(bad == 0, "%ld of %ld results differ from the original's print_obj", bad, lines);
		printf("print_obj comparison: %ld results, %ld mismatches, %ld where the original gave an empty string\n", lines, bad, emptyWant);
	} else {
		printf("test_grammar: XANTH_PRINTOBJ not set, comparison with the original skipped\n");
	}
	printf("test_grammar: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}
