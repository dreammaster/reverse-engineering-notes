// Conversations: the compiled-in graph of the original (1,036 lines, 400 menus) walks with the headless Ui: the opening phone call runs to its end, a menu option's
// callback can enable further options and the enabled flags are the original's.
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "xanth/game.h"
#include "xanth/logic/logic.h"

static int g_failures = 0, g_checks = 0;

#define CHECK(cond, ...)                                                      \
	do {                                                                      \
		g_checks++;                                                           \
		if (!(cond)) {                                                        \
			g_failures++;                                                     \
			printf("FAIL %s:%d: ", __FILE__, __LINE__);                       \
			printf(__VA_ARGS__);                                              \
			printf("\n");                                                     \
		}                                                                     \
	} while (0)

int main() {
	const char *dir = getenv("XANTH_DATA");
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!dir || !gf.setFolder(dir) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("test_conversation: XANTH_DATA not set, skipped\n");
		return 0;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str))
		return 1;
	CHECK(Xanth::kConvLineCount == 1036 && Xanth::kConvOptionCount == 1300 && Xanth::kConvMenuCount == 400, "the tables have the original's sizes");

	Xanth::RecordingUi ui;
	Xanth::Game game(ui, messages, names);
	game.newGame();
	// the entries: line 0 and its menu 0 (option 0 enabled, option 1 disabled)
	CHECK(game.Conversation_GetEntry(0, 0) && (*game.Conversation_GetEntry(0, 0) & 1), "line 0 is enabled");
	CHECK(game.Conversation_GetEntry(0x8000, 0) && (*game.Conversation_GetEntry(0x8000, 0) & 1), "menu 0, option 0 is enabled");
	CHECK(game.Conversation_GetEntry(0x8000, 1) && !(*game.Conversation_GetEntry(0x8000, 1) & 1), "menu 0, option 1 is disabled");
	CHECK(game.Conversation_GetEntry(0x8000 + 400, 0) == nullptr, "no menu 400");
	game.conversation_set_enabled(0x8000, 1, 1);
	CHECK(*game.Conversation_GetEntry(0x8000, 1) & 1, "an option can be enabled");

	game.newGame();
	game.show_compendium(0, 0x5A04);               // the phone call: a line, a menu (the headless Ui takes the first entry), the answers
	int lines = 0, options = 0, begins = 0;
	for (size_t i = 0; i < ui.log.size(); i++) {
		lines += ui.log[i].compare(0, 18, "conversation_line|") == 0;
		options += ui.log[i].compare(0, 20, "conversation_option|") == 0;
		begins += ui.log[i].compare(0, 13, "conversation|") == 0;
	}
	CHECK(begins == 1 && lines >= 2 && options >= 1, "the phone call shows lines and a menu (%d lines, %d options)", lines, options);
	printf("test_conversation: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}
