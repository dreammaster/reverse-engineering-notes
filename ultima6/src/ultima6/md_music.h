#ifndef ULTIMA6_ULTIMA6_MD_MUSIC_H
#define ULTIMA6_ULTIMA6_MD_MUSIC_H

#include "engine/backend.h"
#include "ultima6/md_adlib.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The music of Martian Dreams for the AdLib card: the twelve MIDI files of MDD_MUS.LZC played by the sequencer MARTIAN.EXE keeps in its segment seg048 (sub_21CB2 starts a file, the timer interrupt at
 * seg048:12D7 and sub_21038 read its tracks, sub_218FC stops or fades it out), which hands the events to the sound driver (MdAdlib). The timer runs 60 times a second; the chip's samples are made
 * between the ticks, so the object is a SampleSource for the audio thread. play() and stop() may be called from another thread: they leave a request that the next render() carries out.
 *
 * The sequencer's own variables are the original's (word_28BF0 ...): the file's tracks with their times and running status, the tempo (the number of timer ticks a MIDI tick takes, in 16.16),
 * the volume and the fade, the channels' programs, volumes and pans (the tables at ds:1B26-1B6C), the percussion tables (ds:1B81, 1BCE, which send each drum note of channel 10 to a voice of its own
 * with a note of its own) and the Origin loop events (0xFE: the song jumps back, to the loop points the composer marked, until the game says it has had enough).
 *
 * APPROX(94): the sound effects' use of the same voices (sub_20496, sub_207F4 and the tables at ds:1967) is not done, so the music always has every voice; the AdLib driver is run, not ported (md_adlib.h);
 * the chip is the floating-point model of engine/opl2; the loops of the Origin events play for ever (the original's word_28BFC is set by the callers that want a song to end, which are not ported).
 */
class MdMusic : public SampleSource {
public:
	MdMusic();

	/** Reads the driver, the timbres and the songs of the game's directory: the front end's songs (MDD_MUS.LZC, the default) or the game's own (MUSIC.LZC). */
	bool load(const std::string &dir, const char *songFile = "MDD_MUS.LZC");
	bool ok() const {
		return _ok;
	}
	int songs() const {
		return (int)_songs.size();
	}

	/** Starts song n (0-11: the files of MDD_MUS.LZC in order), as sub_21EB8 does: the one playing is stopped first. */
	void play(int song);
	/** sub_38C3D: the volume the songs play at (0x100: as the file says); the game gives each song its own. */
	void setBaseVolume(int volume);
	/** sub_38B8E: the song playing fades out over `ticks` timer ticks and `song` starts in its place. */
	void crossfade(int song, int ticks);
	/**
	 * What the game does every turn (sub_2FC44, sub_2FD45): `song` is the one that goes with the place and time (-1: none, a fade out over 60 ticks). A new song is faded in over the old one, the
	 * same song starts again when it has ended, each song has its own volume (ds:189E of GAME.EXE).
	 */
	void follow(int song);
	/**
	 * sub_2FD5E(1): the morning song after a dream or a faint: song 4 (a short one) at its volume, and the day's song 0 is queued to start when it ends (sub_38BFA); the game's current song
	 * is then 0. The dream scenes that call it are not ported.
	 */
	void morning();
	/** sub_218FC: stops the song, over `fadeTicks` timer ticks if that is not 0. */
	void stop(int fadeTicks = 0);
	/** True from play() until the song has ended or been stopped. */
	bool playing() const {
		return _playing;
	}

	/**
	 * The sound effects (sub_20951, sub_20D32, sub_207F4): a step (8 bytes: flags, timbre + 1, note, velocity, duration in ticks, second note) plays a note of a timbre of its own on a voice that the
	 * effect takes from the music (the voice 2); the flags say to go on to the next step (1), slide the pitch to the second note (2), hold (4) or play the note again each tick (8). The
	 * requests may come from another thread.
	 */
	enum EffectTable { FRONT_END, GAME };
	void useEffects(EffectTable table);
	/** sub_20951: effect `index` of the table with the entry's flags (1: stereo), volume, pan, the number that names it (the same number replaces the effect) and priority. */
	void effect(int index, int flags, int volume, int pan, int id, int priority);
	/** sub_303A9: effect n of GAME.EXE as the game plays it. */
	void soundEffect(int n) {
		effect(n, 0, 0xFF, 0x3F, 1, 0xC);
	}
	/** sub_20D32 with no step: the effects named `id` (0: all) stop. */
	void stopEffects(int id = 0);

	void render(int16_t *out, int count) override;

	// for tests: the timer interrupt by hand, and the driver's writes to the chip
	void tick();
	MdAdlib &driver() {
		return _adlib;
	}
	/** The file being played, or -1. */
	int current() const {
		return _current;
	}

private:
	struct Track {
		size_t pos = 0;
		int32_t wait = 0; // ticks to the next event
		uint8_t status = 0;
		bool done = true;
	};

	struct Slot { // ds:1966 + 14 n of the original
		int flags = 0, channel = 0, channel2 = 0, note = 0, volume = 0, pan = 0, id = 0, timer = 0;
		const uint8_t *step = nullptr;
	};
	struct Request {
		int kind, a, b, c, d, e, f; // 0 effect (index, flags, volume, pan, id, priority), 1 stop (id)
	};
	void doEffect(int index, int flags, int volume, int pan, int id, int priority);
	void doStop(int id);
	void startStep(int s);
	void releaseChannel(int channel);
	void restoreChannel(int channel);
	int allocateChannel(int priority);
	void claim(int channel);
	void effectsTick();
	void lockRequests();
	void unlockRequests();

	bool start(int song);
	void parse();
	int sequence();
	int pickTrack();
	uint32_t varLength(int t);
	uint8_t next(int t);
	void notesOff(int channel);
	void allNotesOff();
	void setVolume(int volume);
	void endSong();
	void tempo(int track, int type, int length);
	void initChannel(int di);
	void percussionOrNote(int di, int note, int velocity);

	MdAdlib _adlib;
	bool _ok = false, _playing = false;
	std::vector<Bytes> _songs;
	Bytes _file;
	int _current = -1;
	std::vector<Track> _tracks;
	int _format = 0, _division = 96;
	uint32_t _factor = 10000;     // word_28E19: timer ticks (16.16) a MIDI tick takes
	int32_t _elapsed = 0;         // word_28E1D: the time that is owed to the timer (16.16)
	int _enabled = 1;             // word_28BF0
	int _master = 0x100, _base = 0x100; // word_28BF2, word_28BF4
	int _fade = 0, _level = 0, _keep = 0; // word_28BF6, 28BF8, 28BFA
	int _loop = 0;                // word_28BFC
	int _queued = -1;             // word_28BFE: a song to start when this one ends
	int _marker = 0, _expect = 0; // word_28BEA, word_28BEC
	std::atomic<int> _ended;      // word_28EBB
	int _flags = 0;               // word_28EBD
	uint8_t _program[17], _sent[17], _volume[17], _pan[17], _active[17];
	int8_t _held[17];

	Slot _slot[32];
	const uint8_t (*_steps)[8] = nullptr;
	int _stepCount = 0;
	std::vector<Request> _requests;
	std::atomic_flag _requestLock = ATOMIC_FLAG_INIT;
	double _sampleDebt = 0;
	std::atomic<int> _requestPlay, _requestStop, _requestBase, _requestCross, _requestQueue;
	int _gameSong = -1;           // word_4118A of GAME.EXE: the song the game last asked for
};

} // namespace Ultima6
} // namespace Ultima6

#endif
