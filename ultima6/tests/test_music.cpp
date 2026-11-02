// The twelve Ultima VI music tracks (skipped when the game data is absent).

#include "files/music.h"

#include <cstdio>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	const char *names[] = {"ULTIMA", "BOOTUP", "INTRO", "CREATE", "FOREST", "HORNPIPE", "ENGAGE", "STONES", "DUNGEON", "BRIT", "GARGOYLE", "END"};
	Bytes raw;
	if (!readFile("u6/game/ULTIMA.M", raw)) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	for (const char *name : names) {
		MusicTrack track;
		CHECK(readFile(joinPath("u6/game", std::string(name) + ".M"), raw) && track.load(raw));
		int instruments = 0, calls = 0, returns = 0;
		for (const MusicTrack::Event &e : track.events) {
			instruments += e.command == MusicTrack::SUBCOMMAND && e.channel == MusicTrack::LOAD_INSTRUMENT;
			calls += e.command == MusicTrack::SUBCOMMAND && e.channel == MusicTrack::CALL;
			returns += e.command == MusicTrack::RETURN;
		}
		CHECK(instruments > 0 && returns > 0);
		std::printf("%-9s %5zu bytes, %4zu events, %2d instruments, %2d calls, %2d returns\n", name, track.data.size(), track.events.size(),
					instruments, calls, returns);
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
