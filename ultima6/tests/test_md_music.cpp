// Martian Dreams' music: the sequencer of MARTIAN.EXE and the AdLib driver of STRAX.DRV (md_music.h, md_adlib.h). Skipped without the game data.

#include "ultima6/md_music.h"

#include <cstdio>
#include <cstdlib>

using namespace Ultima6;
using ::Ultima6::Ultima6::MdAdlib;
using ::Ultima6::Ultima6::MdMusic;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

// the ticks (60 a second) a song takes to end
static int length(MdMusic &m, int song, int limit) {
	m.play(song);
	int16_t one;
	m.render(&one, 1); // the request is carried out by render()
	int ticks = 0;
	while (m.playing() && ticks < limit) {
		m.tick();
		++ticks;
	}
	return ticks;
}

int main() {
	const char *dir = "md/game/MARTIAN";
	Bytes probe;
	if (!readFile(joinPath(dir, "MDD_MUS.LZC"), probe)) {
		std::printf("skipped (no Martian Dreams data)\n");
		return 0;
	}
	MdMusic m;
	CHECK(m.load(dir) && m.songs() == 12);
	// song 4 is 5.33 s long by its tempo (the checksum of the tempo events and deltas, worked out apart); song 3 10.67 s
	int n4 = length(m, 4, 1000);
	CHECK(!m.playing() && n4 >= 316 && n4 <= 324);
	int n3 = length(m, 3, 2000);
	CHECK(!m.playing() && n3 >= 636 && n3 <= 644);
	// the driver gets notes: the chip's registers A0-B8 are written, keys go on
	m.driver().clearLog();
	m.play(4);
	int16_t buf[4971];
	int keyOn = 0, peak = 0;
	for (int i = 0; i < 4; ++i) {
		m.render(buf, 4971);
		for (int k = 0; k < 4971; ++k)
			peak = std::max(peak, std::abs((int)buf[k]));
	}
	for (const auto &w : m.driver().log())
		if (w.first >= 0xB0 && w.first <= 0xB8 && (w.second & 0x20))
			++keyOn;
	CHECK(keyOn > 5 && peak > 500);
	// a fade over 60 ticks ends the song within a second and a bit
	m.play(2);
	m.render(buf, 100);
	for (int i = 0; i < 100; ++i)
		m.tick();
	m.stop(60);
	m.render(buf, 1);
	int t = 0;
	while (m.playing() && t < 200) {
		m.tick();
		++t;
	}
	CHECK(!m.playing() && t >= 55 && t <= 70);
	// stop at once
	m.play(2);
	m.render(buf, 10);
	m.stop(0);
	m.render(buf, 10);
	CHECK(!m.playing());
	// the jingle of a fight (sub_2FF7D): the game's own songs (MUSIC.LZC, 16): song 5 plays and the song of the place is queued
	{
		MdMusic g;
		if (g.load(dir, "MUSIC.LZC") && g.songs() >= 16) {
			g.sting(5, 0);
			int16_t out[512];
			g.render(out, 10);
			CHECK(g.playing() && g.current() == 5);
		}
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
