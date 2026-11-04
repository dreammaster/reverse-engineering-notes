// The AdLib driver run by hand (music_player.*) and the OPL2 model (opl2.*): every shipped track plays for a while and makes sound (skipped when the game data is absent).

#include "engine/music_player.h"
#include "files/music.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond, what)                                                        \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, what);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	static const char *const NAMES[12] = {"ULTIMA", "BOOTUP", "INTRO", "CREATE", "FOREST", "HORNPIPE", "ENGAGE", "STONES", "DUNGEON", "BRIT", "GARGOYLE", "END"};
	// a note by hand: a plain tone is heard at the pitch written, and silence after the key is off
	{
		Opl2 chip;
		chip.write(0x01, 0x20);
		chip.write(0x20, 0x21);
		chip.write(0x23, 0x21);
		chip.write(0x40, 0x3F); // modulator silent
		chip.write(0x43, 0x00);
		chip.write(0x60, 0xF0);
		chip.write(0x63, 0xF0);
		chip.write(0x80, 0x0F);
		chip.write(0x83, 0x0F);
		chip.write(0xC0, 0x01); // additive
		chip.write(0xA0, 0x41);
		chip.write(0xB0, 0x20 | (4 << 2) | 0x02); // fnum 0x241, block 4: A4
		std::vector<int16_t> buf(Opl2::SAMPLE_RATE / 2);
		chip.generate(buf.data(), (int)buf.size());
		int crossings = 0, peak = 0;
		for (size_t i = 1; i < buf.size(); ++i) {
			crossings += (buf[i - 1] < 0) != (buf[i] < 0);
			peak = std::max(peak, (int)std::abs(buf[i]));
		}
		double hz = crossings / 2.0 / 0.5;
		std::printf("A4 measured %.1f Hz, peak %d\n", hz, peak);
		CHECK(hz > 425 && hz < 455, "A4 sounds near 440 Hz");
		CHECK(peak > 1000, "audible");
		chip.write(0xB0, (4 << 2) | 0x02); // key off
		chip.generate(buf.data(), (int)buf.size());
		chip.generate(buf.data(), (int)buf.size());
		int tail = 0;
		for (size_t i = buf.size() - 1000; i < buf.size(); ++i)
			tail = std::max(tail, (int)std::abs(buf[i]));
		CHECK(tail < 100, "the note dies away");
	}
	bool any = false;
	for (const char *name : NAMES) {
		std::vector<unsigned char> raw;
		std::string path = std::string("u6/game/") + name + ".M";
		FILE *f = std::fopen(path.c_str(), "rb");
		if (!f)
			continue;
		any = true;
		int c;
		while ((c = std::fgetc(f)) != EOF)
			raw.push_back((unsigned char)c);
		std::fclose(f);
		MusicTrack t;
		CHECK(t.load(Bytes(raw.begin(), raw.end())), "track loads");
		int bad = 0;
		for (const MusicTrack::Event &e : t.events)
			if (e.command <= 2 && (t.data[e.dataOffset] & 0x1F) >= 24)
				++bad;
		CHECK(bad == 0, "every pitch is in the driver's note table");
		MusicPlayer player;
		player.play(t.data);
		std::vector<int16_t> buf(Opl2::SAMPLE_RATE);
		long long energy = 0;
		int peak = 0;
		for (int sec = 0; sec < 12; ++sec) {
			player.render(buf.data(), (int)buf.size());
			for (int16_t s : buf) {
				energy += (long long)s * s;
				peak = std::max(peak, (int)std::abs(s));
			}
		}
		std::printf("%s: peak %d, rms %.0f, still playing %d\n", name, peak, std::sqrt((double)energy / (12.0 * Opl2::SAMPLE_RATE)), (int)player.playing());
		CHECK(peak > 500, "the track makes sound");
		CHECK(player.playing(), "and loops");
		player.stop();
		player.render(buf.data(), (int)buf.size());
		player.render(buf.data(), (int)buf.size());
		player.render(buf.data(), (int)buf.size());
		int tail = 0;
		for (size_t i = buf.size() - 500; i < buf.size(); ++i)
			tail = std::max(tail, (int)std::abs(buf[i]));
		CHECK(tail < 300, "stopped");
	}
	if (!any) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
