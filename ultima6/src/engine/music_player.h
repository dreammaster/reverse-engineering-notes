#ifndef ULTIMA6_ENGINE_MUSIC_PLAYER_H
#define ULTIMA6_ENGINE_MUSIC_PLAYER_H

#include "engine/opl2.h"
#include "files/data_file.h"

#include <cstdint>

namespace Ultima6 {

/**
 * The AdLib sound driver of the original (U6ADLIB.DRV, `snd_timer_isr` at 3A0), run by hand: the 60 Hz timer interrupt reads the events of a track (see music.h) and writes the
 * OPL2 registers exactly as the driver does, nine voices, one per event channel. `play` is snd_play, `stop` is snd_stop (every voice keyed off), `render` produces the audio
 * and runs a tick each time one falls due.
 */
class MusicPlayer {
public:
	enum { TICKS_PER_SECOND = 60 };

	MusicPlayer();

	/** snd_play: the decompressed track (MusicTrack::data) starts from its beginning; it is copied. */
	void play(const Bytes &track);
	/** snd_stop. */
	void stop();
	bool playing() const {
		return _playing;
	}
	/** Mono 16-bit samples at Opl2::SAMPLE_RATE. */
	void render(int16_t *out, int count);
	/** One run of the timer interrupt (for tests). */
	void tick();

	Opl2 &chip() {
		return _chip;
	}

private:
	void write(int reg, int value);
	void setPitch(int voice, int pitch, bool keyOn);
	void writePitch(int voice, int word);
	void writeLevel(int voice);
	void loadInstrument(int voice, size_t at);
	void silence();
	void effects();
	int pitchWord(int pitch) const;

	Opl2 _chip;
	Bytes _data;
	bool _playing = false;
	size_t _pos = 0, _loop = 0;
	int _delay = 0;
	struct Frame {
		int count;
		size_t returnTo, target;
	};
	Frame _stack[16];
	int _depth = 0;
	size_t _instrument[256];
	bool _haveInstrument[256];

	int _pitch[16];      // cs:0D6, the A0/B0 word of each voice (with the key bit 0x2000)
	int _vibCounter[16]; // cs:0E8
	int _level[16];      // cs:0F1, what the carrier's 0x40 register holds
	int _fadeReload[16]; // cs:0FA
	int _fadeCount[16];  // cs:103
	int _slide[16];      // cs:10C, signed
	int _vibDepth[16];   // cs:115
	int _vibRate[16];    // cs:11E
	int _vibUp[16];      // cs:127
	int _fade[16];       // cs:130, 0 none, 1 fade out (the level rises), -1 fade in
	int _sampleFraction = 0;
};

} // namespace Ultima6

#endif
