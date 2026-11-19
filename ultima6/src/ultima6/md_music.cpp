#include "ultima6/md_music.h"

#include "files/lib_n.h"

#include <algorithm>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// The channels' tables of MARTIAN.EXE's data segment at the start (ds:1B26, 1B31, 1B3C, 1B47, 1B5F, 1B6C), index 0-16 (the MIDI channel + 1)
const uint8_t SENT_INIT[17] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 127, 127, 127, 127, 127, 127};
const uint8_t VOLUME_INIT[17] = {127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 64, 64, 64, 64, 64, 64};
const uint8_t PAN_INIT[17] = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 0, 0, 0, 0, 0, 0};
const uint8_t PROGRAM_INIT[17] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 4, 5, 6};
const uint8_t ACTIVE_INIT[17] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 39, 0, 0}; // (what the data holds: 13 and 14 are not 0 or 1)

// The drums (ds:1B81 and 1BCE, indexed by the note of channel 10): the voice ("channel" 10-26) and the note that play it
const uint8_t DRUM_CHANNEL[77] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 18, 11, 0, 12, 13, 17, 13, 16, 13, 14, 13, 13, 15, 13, 19,
								  0, 0, 0, 0, 21, 0, 0, 0, 26, 26, 25, 20, 20, 0, 0, 21, 21, 22, 23, 0, 0, 24, 0, 20, 0};
const uint8_t DRUM_NOTE[77] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 48, 48, 48, 0, 48, 42, 71, 42, 71, 47, 71, 47, 52, 79, 52, 77,
							   0, 0, 0, 0, 71, 0, 0, 0, 72, 79, 79, 64, 58, 0, 0, 89, 84, 48, 72, 0, 0, 36, 0, 96, 0};
// the programs the drum voices get when the card is set up (sub_21ABC for the AdLib: voice, program)
const uint8_t DRUM_PROGRAM[17][2] = {{0x0A, 0x80}, {0x0B, 0x72}, {0x0C, 0x83}, {0x0D, 0x71}, {0x0E, 0x86}, {0x0F, 0x87}, {0x10, 0x85}, {0x11, 0x84}, {0x12, 0x81},
									 {0x13, 0x88}, {0x14, 0x8D}, {0x15, 0x8F}, {0x16, 0x90}, {0x17, 0x91}, {0x18, 0x93}, {0x19, 0x8C}, {0x1A, 0x8B}};

const double TICKS_PER_SECOND = 60.0;

} // namespace

MdMusic::MdMusic() : _requestPlay(-1), _requestStop(-1) {
	std::memcpy(_sent, SENT_INIT, sizeof _sent);
	std::memcpy(_volume, VOLUME_INIT, sizeof _volume);
	std::memcpy(_pan, PAN_INIT, sizeof _pan);
	std::memcpy(_program, PROGRAM_INIT, sizeof _program);
	std::memcpy(_active, ACTIVE_INIT, sizeof _active);
	std::memset(_held, 0, sizeof _held);
}

bool MdMusic::load(const std::string &dir) {
	_ok = false;
	if (!_adlib.load(dir))
		return false;
	Bytes raw;
	LibN lib;
	if (!readFile(joinPath(dir, "MDD_MUS.LZC"), raw) || !lib.load(raw, false))
		return false;
	_songs.clear();
	for (size_t i = 1; i < lib.count(); ++i) {
		Bytes song;
		if (!lib.present(i) || !lib.get(i, song))
			break;
		_songs.push_back(song);
	}
	// sub_21ABC for the AdLib: the drum voices' programs
	for (const uint8_t *d : DRUM_PROGRAM)
		_adlib.program(d[0], d[1]);
	allNotesOff();
	_ok = !_songs.empty();
	return _ok;
}

void MdMusic::play(int song) {
	_requestPlay = song;
}

void MdMusic::stop(int fadeTicks) {
	_requestStop = fadeTicks;
}

// sub_216C3: the song is over, every voice is silenced
void MdMusic::allNotesOff() {
	_ended = 1;
	_flags |= 1;
	for (int c = 1; c <= 16; ++c)
		if (_active[c] == 1)
			_adlib.controller(c, 0x7B, 0);
	_playing = false;
}

// sub_21700: the master volume (0x100: the songs as they are) reaches the voices
void MdMusic::setVolume(int volume) {
	_master = volume;
	for (int c = 2; c <= 10; ++c)
		if (_active[c] == 1)
			_adlib.controller(c, 7, (int)(int16_t)(_volume[c] * _master) >> 8);
}

// sub_218FC (and sub_21EB8's first half)
void MdMusic::endSong() {
	_flags &= ~2;
	if (_fade == 0)
		_level = 0x7FFF;
	_queued = -1;
	_fade = 0;
	allNotesOff();
	setVolume(_base);
}

// sub_21EB8 / sub_21CB2 / sub_21858: a song is read and its tracks started
bool MdMusic::start(int song) {
	if (song < 0 || song >= (int)_songs.size())
		return false;
	endSong();
	_fade = 0;
	_file = _songs[(size_t)song];
	_current = song;
	_ended = 1;
	_tracks.clear();
	// the header: "MThd", its length (6), the format (0-2), the tracks (up to 32), the division
	const Bytes &f = _file;
	if (f.size() < 14 || std::memcmp(&f[0], "MThd", 4) != 0)
		return false;
	_format = (f[8] << 8) | f[9];
	int n = (f[10] << 8) | f[11];
	_division = (f[12] << 8) | f[13];
	if (_format > 2 || n > 0x20 || _division == 0)
		return false;
	size_t at = 8 + (((size_t)f[4] << 24) | (f[5] << 16) | (f[6] << 8) | f[7]);
	while ((int)_tracks.size() < n && at + 8 <= f.size()) {
		size_t len = ((size_t)f[at + 4] << 24) | (f[at + 5] << 16) | (f[at + 6] << 8) | f[at + 7];
		if (std::memcmp(&f[at], "MTrk", 4) == 0) {
			Track t;
			t.pos = at;
			_tracks.push_back(t);
		}
		at += 8 + len;
	}
	if (_tracks.empty())
		return false;
	// sub_21858: the controllers of the voices are reset, each track skips its header and reads its first delta
	for (int c = 2; c <= 10; ++c)
		_adlib.controller(c, 0x79, 0);
	for (size_t t = 0; t < _tracks.size(); ++t) {
		_tracks[t].wait = 0;
		_tracks[t].done = false;
		_tracks[t].pos += 8;
		_tracks[t].wait += (int32_t)varLength((int)t);
	}
	_marker = 0;
	_loop = 0;
	_ended = 0;
	_fade = 0;
	_queued = -1;
	_flags = (_flags & ~1) | 2;
	_playing = true;
	return true;
}

uint8_t MdMusic::next(int t) {
	Track &tr = _tracks[(size_t)t];
	if (tr.pos >= _file.size())
		return 0;
	return _file[tr.pos++];
}

// sub_20DC8: a number in the 7-bit pieces of a MIDI file
uint32_t MdMusic::varLength(int t) {
	uint32_t v = 0;
	for (int i = 0; i < 5; ++i) {
		uint8_t b = next(t);
		v = (v << 7) + (b & 0x7F);
		if (!(b & 0x80))
			break;
	}
	return v;
}

// sub_20F94: the track whose next event comes first; the time that passes is added to what the timer owes and taken from every track
int MdMusic::pickTrack() {
	int best = -1;
	int32_t bestTime = 0x7FFFFFFF;
	for (size_t t = 0; t < _tracks.size(); ++t)
		if (!_tracks[t].done && _tracks[t].wait < bestTime) {
			best = (int)t;
			bestTime = _tracks[t].wait;
		}
	if (bestTime != 0)
		_elapsed = (int32_t)((uint32_t)_elapsed + (uint32_t)bestTime * _factor);
	for (Track &tr : _tracks)
		tr.wait -= bestTime;
	return best;
}

// sub_20ECF: a meta event's body (a tempo changes the time a MIDI tick takes: microseconds a quarter note / division, scaled to the 60 ticks a second)
void MdMusic::tempo(int t, int type, int length) {
	if (type == 0x51) {
		uint32_t us = 0;
		size_t at = _tracks[(size_t)t].pos;
		for (int i = 0; i < length; ++i)
			us = (us << 8) + (at + i < _file.size() ? _file[at + i] : 0);
		uint32_t perTick = us / (uint32_t)_division;
		_factor = (perTick * 0x1317u) / 1243u;
	}
	_tracks[(size_t)t].pos += (size_t)length;
}

void MdMusic::initChannel(int di) {
	_adlib.controller(di, 0x7B, 0);
	if (_sent[di] != _program[di]) {
		if (di != 10)
			_adlib.program(di, _program[di]);
		_sent[di] = _program[di];
	}
	_adlib.controller(di, 7, (int)(int16_t)(_volume[di] * _master) >> 8);
	_adlib.controller(di, 0x0A, _pan[di]);
	_active[di] = 1;
}

// the notes of channel 10 are sent to the drum voices
void MdMusic::percussionOrNote(int di, int note, int velocity) {
	if (di == 10) {
		int n = note < 77 ? note : 0;
		_adlib.note(DRUM_CHANNEL[n], DRUM_NOTE[n], velocity);
	} else {
		_adlib.note(di, note, velocity);
	}
}

// sub_21038: the events that are due; true when the song is over
int MdMusic::sequence() {
	int ended = 0;
	while (!ended) {
		int t = pickTrack();
		if (t < 0 || (int16_t)(_elapsed >> 16) > 0)
			return 0;
		Track &tr = _tracks[(size_t)t];
		int b = next(t);
		if (b >= 0x80 && b < 0xF0) {
			tr.status = (uint8_t)b;
			b = next(t);
		}
		if (b >= 0xF0) { // system exclusive, the Origin events (0xFE) and the meta events
			if (b == 0xF0 || b == 0xF7) {
				uint32_t len = varLength(t);
				tr.pos += len;
			} else if (b == 0xFE) {
				int di = (tr.status & 0x0F) + 1;
				int type = next(t), len = next(t);
				auto offset = [&]() {
					int v = (tr.pos < _file.size() ? _file[tr.pos] : 0) | ((tr.pos + 1 < _file.size() ? _file[tr.pos + 1] : 0) << 8);
					return v;
				};
				auto jumpBack = [&]() {
					_flags |= 1;
					_adlib.controller(di, 0x7B, 0);
					tr.pos -= (size_t)offset();
				};
				if (type == 0) {
					if (_loop == 0)
						tr.pos += (size_t)len;
					else
						jumpBack();
				} else if (type == 1) {
					if (_loop != 0) {
						_flags |= 1;
						for (int c = 2; c <= 10; ++c)
							if (_enabled && _active[c] == 1)
								_adlib.controller(c, 0x7B, 0);
						return 1;
					}
				} else if (type == 2) {
					if (_loop != 0)
						jumpBack();
					else
						tr.pos += (size_t)len;
				} else if (type == 3) {
					_marker = tr.pos < _file.size() ? _file[tr.pos] : 0;
					tr.pos += (size_t)len;
				} else if (type == 4) {
					int v = tr.pos + 2 < _file.size() ? _file[tr.pos + 2] : 0;
					if (v == _expect)
						tr.pos += (size_t)len;
					else
						jumpBack();
				} else if (type == 5) {
					int v = tr.pos + 2 < _file.size() ? _file[tr.pos + 2] : 0;
					if (v != _expect)
						tr.pos += (size_t)len;
					else
						jumpBack();
				} else {
					tr.pos += (size_t)len;
				}
			} else if (b == 0xFF) {
				int type = next(t);
				int len = (int)varLength(t);
				tempo(t, type, len);
				if (type == 0x2F) {
					tr.done = true;
					bool all = true;
					for (const Track &x : _tracks)
						if (!x.done)
							all = false;
					if (all)
						ended = 1;
				}
			}
		} else {
			int di = (tr.status & 0x0F) + 1;
			if (_active[di] == 0 || _held[di] < 0)
				initChannel(di);
			int kind = tr.status & 0xF0;
			if (kind == 0x80 || kind == 0x90) {
				int vel = next(t);
				if (_enabled && _active[di] == 1)
					percussionOrNote(di, b, kind == 0x80 ? 0 : vel);
			} else if (kind == 0xA0) {
				next(t);
			} else if (kind == 0xB0) {
				int val = next(t);
				if (_enabled && _active[di] == 1) {
					if (b == 7)
						_adlib.controller(di, 7, (int)(int16_t)(val * _master) >> 8);
					else
						_adlib.controller(di, b, val);
				}
				if (b == 7)
					_volume[di] = (uint8_t)val;
				if (b == 0x0A)
					_pan[di] = (uint8_t)val;
			} else if (kind == 0xC0) {
				if (_enabled && _active[di] == 1) {
					_adlib.program(di, b);
					_sent[di] = (uint8_t)b;
				}
				_program[di] = (uint8_t)b;
			} else if (kind == 0xE0) {
				int second = next(t);
				if (_enabled && _active[di] == 1)
					_adlib.pitchBend(di, b, second);
			}
		}
		if (!ended)
			tr.wait += (int32_t)varLength(t);
	}
	return ended;
}

// the interrupt of seg048:12D7 at 60 a second, then the driver's own
void MdMusic::tick() {
	if (_fade != 0 && _ended == 0) {
		_level += _fade;
		if (_level >= 0) {
			setVolume((int)(int16_t)(((_level >> 8) * _base) & 0xFFFF) >> 7);
		} else {
			_fade = 0;
		}
		if (_fade == 0) {
			if (_keep == 0)
				allNotesOff();
			setVolume(_base);
		}
	}
	if (_ended == 0) {
		_ended = sequence();
		if (_ended) {
			_flags |= 1;
			_playing = false;
		}
		_elapsed -= 0x10000;
	} else if (_queued >= 0) {
		int q = _queued;
		_queued = -1;
		start(q);
	}
	_adlib.tick();
}

void MdMusic::render(int16_t *out, int count) {
	int p = _requestPlay.exchange(-1);
	int s = _requestStop.exchange(-1);
	if (s >= 0) {
		if (s == 0) {
			endSong();
		} else if (_playing) { // sub_218FC with a number: a fade of that many ticks
			if (_fade == 0)
				_level = 0x7FFF;
			_fade = -(0x8000 / s);
			_keep = 0;
		}
	}
	if (p >= 0)
		start(p);
	int done = 0;
	const double perTick = Opl2::SAMPLE_RATE / TICKS_PER_SECOND;
	while (done < count) {
		if (_sampleDebt <= 0) {
			tick();
			_sampleDebt += perTick;
		}
		int n = std::min(count - done, (int)_sampleDebt + 1);
		_adlib.chip().generate(out + done, n);
		done += n;
		_sampleDebt -= n;
	}
}

} // namespace Ultima6
} // namespace Ultima6
