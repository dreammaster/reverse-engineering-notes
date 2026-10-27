// Music for the full-program build: the Miles AIL high-level sequence API implemented natively, with an XMIDI sequencer that emits short MIDI messages to a sink
// (the Windows front end sends them to the system synthesizer). The original talks to real DOS sound drivers (16-bit real-mode code that cannot run here), so the
// `_AIL_*` entry points the game calls are replaced by shims (see Audio::install). The digital sound/voice path reports "no driver": this release keeps its voices as MP3
// files and the original voice archive is absent.
#pragma once
#include "machine.h"

#include <mutex>
#include <vector>

namespace shannara {

class Audio {
public:
	Audio();
	void install(Machine &m);                            // registers the native AIL shims
	void setMidiSink(std::function<void(uint32_t)> s);   // short MIDI message: status | data1 << 8 | data2 << 16
	void advance(double seconds);                        // runs the sequencers for that much time (called from the audio thread)
	// sequences that ended since the last call: (callback function address, handle) - to be run on the game thread
	void takeCallbacks(std::vector<std::pair<uint32_t, uint32_t>> &out);
	bool verbose = false;
	int midiMessages = 0;     // statistics
	int sequencesStarted = 0;

private:
	struct Seq {
		bool used = false, playing = false, finished = false;
		std::vector<uint8_t> evnt;
		size_t pos = 0;
		double now = 0, nextTime = 0, tempoUs = 500000; // time in XMIDI intervals (120 per second at the default tempo)
		int volume = 127, fadeFrom = 127, fadeTo = 127;
		double fadeMs = 0, fadeElapsed = 0;
		uint32_t callback = 0;
		struct Loop { size_t pos; int count; };
		std::vector<Loop> loops;
		struct Off { double at; uint8_t ch, note; };
		std::vector<Off> offs;
		uint8_t chanVol[16];
		bool chanUsed[16];
	};
	bool parse(Seq &s, const Machine &m, uint32_t addr, int number);
	void step(Seq &s, uint32_t handle);
	void send(uint32_t msg);
	void silence(Seq &s);
	Seq *get(uint32_t handle);
	int scaled(const Seq &s, int v) const { return v * s.volume / 127; }

	std::mutex _mu;
	std::vector<Seq> _seqs;
	std::function<void(uint32_t)> _sink;
	std::vector<std::pair<uint32_t, uint32_t>> _done;
	static const uint32_t kHandleBase = 0x5E0000;
};

} // namespace shannara
