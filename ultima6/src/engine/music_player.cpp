#include "engine/music_player.h"

#include <cstring>

namespace Ultima6 {

namespace {

// The note table of the driver (cs:0176): per octave group of eight a rest and seven notes; the three groups are three tunings a little apart.
const int NOTES[32] = {0, 0x158, 0x182, 0x1B0, 0x1CC, 0x203, 0x241, 0x286, 0, 0x16A, 0x196, 0x1C7, 0x1E4, 0x21E, 0x25F, 0x2A8,
					   0, 0x147, 0x16E, 0x19A, 0x1B5, 0x1E9, 0x224, 0x266, 0, 0, 0, 0, 0, 0, 0, 0};
// The operator register offsets of the nine voices (cs:0164 modulators, cs:016D carriers)
const int MODULATOR[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
const int CARRIER[9] = {3, 4, 5, 11, 12, 13, 19, 20, 21};

} // namespace

MusicPlayer::MusicPlayer() {
	std::memset(_pitch, 0, sizeof(_pitch));
	std::memset(_vibCounter, 0, sizeof(_vibCounter));
	std::memset(_level, 0, sizeof(_level));
	std::memset(_fadeReload, 0, sizeof(_fadeReload));
	std::memset(_fadeCount, 0, sizeof(_fadeCount));
	std::memset(_slide, 0, sizeof(_slide));
	std::memset(_vibDepth, 0, sizeof(_vibDepth));
	std::memset(_vibRate, 0, sizeof(_vibRate));
	std::memset(_vibUp, 0, sizeof(_vibUp));
	std::memset(_fade, 0, sizeof(_fade));
	std::memset(_haveInstrument, 0, sizeof(_haveInstrument));
	// snd_init
	_chip.write(0x01, 0x20);
	_chip.write(0xBD, 0);
	_chip.write(0x08, 0);
}

void MusicPlayer::write(int reg, int value) {
	_chip.write(reg, value);
}

// sub_348: a pitch byte (block in the top three bits, tuning group and note below) as the A0 / B0 pair
int MusicPlayer::pitchWord(int pitch) const {
	return NOTES[pitch & 0x1F] + ((pitch & 0xE0) << 5);
}

// sub_2B1: the pitch word of a voice to registers A0 + voice (low byte) and B0 + voice (high byte)
void MusicPlayer::writePitch(int voice, int word) {
	if (voice > 8)
		return;
	write(0xA0 + voice, word & 0xFF);
	write(0xB0 + voice, (word >> 8) & 0xFF);
}

// sub_2A9: the level of a voice to the total level register of its carrier
void MusicPlayer::writeLevel(int voice) {
	if (voice > 8)
		return;
	write(0x40 + CARRIER[voice], _level[voice] & 0xFF);
}

// sub_2C4: eleven bytes of instrument to the operators of a voice
void MusicPlayer::loadInstrument(int voice, size_t at) {
	if (voice > 8 || at + 11 > _data.size())
		return;
	static const int REGS[5] = {0x20, 0x40, 0x60, 0x80, 0xE0};
	for (int i = 0; i < 5; ++i)
		write(REGS[i] + MODULATOR[voice], _data[at + i]);
	for (int i = 0; i < 5; ++i)
		write(REGS[i] + CARRIER[voice], _data[at + 5 + i]);
	write(0xC0 + voice, _data[at + 10]);
}

void MusicPlayer::play(const Bytes &track) {
	_data = track;
	std::memset(_pitch, 0, sizeof(_pitch));
	std::memset(_vibCounter, 0, sizeof(_vibCounter));
	std::memset(_level, 0, sizeof(_level));
	std::memset(_fadeReload, 0, sizeof(_fadeReload));
	std::memset(_fadeCount, 0, sizeof(_fadeCount));
	std::memset(_slide, 0, sizeof(_slide));
	std::memset(_vibDepth, 0, sizeof(_vibDepth));
	std::memset(_vibRate, 0, sizeof(_vibRate));
	std::memset(_vibUp, 0, sizeof(_vibUp));
	std::memset(_fade, 0, sizeof(_fade));
	std::memset(_haveInstrument, 0, sizeof(_haveInstrument));
	_depth = 0;
	_pos = _loop = 0;
	_delay = 0;
	_playing = !_data.empty();
}

// sub_383: every voice keyed off with a pitch of 0
void MusicPlayer::silence() {
	for (int v = 8; v >= 0; --v) {
		_pitch[v] = 0;
		writePitch(v, 0);
	}
}

void MusicPlayer::stop() {
	_playing = false;
	silence();
}

// The tick's last part (snd_timer_isr from 5A1): slides, vibrato and fades of every voice, the last voice first
void MusicPlayer::effects() {
	for (int v = 8; v >= 0; --v) {
		if (_slide[v] != 0) {
			_pitch[v] = (_pitch[v] + _slide[v]) & 0xFFFF;
			writePitch(v, _pitch[v]);
		} else if (_vibRate[v] != 0 && (_pitch[v] & 0x2000)) {
			int c = (int8_t)_vibCounter[v];
			if (c >= _vibDepth[v])
				_vibUp[v] = 1;
			else if (c <= 0)
				_vibUp[v] = 0;
			if (_vibUp[v])
				c -= 2;
			c += 1;
			_vibCounter[v] = (int8_t)c;
			int centred = (int8_t)c - (_vibDepth[v] >> 1);
			int shift = (int8_t)(centred & 0xFF) * (int8_t)_vibRate[v];
			writePitch(v, (_pitch[v] + shift) & 0xFFFF);
		}
		if (_fade[v] != 0 && --_fadeCount[v] == 0) {
			_fadeCount[v] = _fadeReload[v];
			int level = (int8_t)(_fade[v] + (_level[v] & 0xFF));
			if (level > 0x3F) {
				level = 0x3F;
				_fade[v] = 0;
			} else if (level < 0) {
				level = 0;
				_fade[v] = 0;
			}
			_level[v] = level;
			writeLevel(v);
		}
	}
}

// snd_timer_isr: after the delay of the last event has run out, events are read until the next delay
void MusicPlayer::tick() {
	if (!_playing)
		return;
	if (_delay != 0) {
		if (--_delay != 0) {
			effects();
			return;
		}
	}
	for (int guard = 0; guard < 100000; ++guard) {
		if (_pos >= _data.size()) {
			_playing = false;
			return;
		}
		int b = _data[_pos++];
		int cmd = b >> 4, voice = b & 15;
		auto next = [&]() { return _pos < _data.size() ? (int)_data[_pos++] : 0; };
		switch (cmd) {
		case 0: { // note off: the pitch without the key bit
			int word = pitchWord(next());
			_pitch[voice] = word;
			writePitch(voice, word);
			break;
		}
		case 1: { // note on: key off at the new pitch, then on
			_vibUp[voice] = 0;
			_vibCounter[voice] = 0;
			int word = pitchWord(next());
			writePitch(voice, word);
			word |= 0x2000;
			_pitch[voice] = word;
			writePitch(voice, word);
			break;
		}
		case 2: { // pitch while sounding
			int word = pitchWord(next()) | 0x2000;
			_pitch[voice] = word;
			writePitch(voice, word);
			break;
		}
		case 3: // level
			_fade[voice] = 0;
			_level[voice] = next();
			writeLevel(voice);
			break;
		case 4: // the modulator's level
			if (voice <= 8)
				write(0x40 + MODULATOR[voice], next());
			else
				next();
			break;
		case 5: // slide
			_slide[voice] = (int8_t)next();
			break;
		case 6: { // vibrato: depth in the high nibble, rate in the low one
			int v = next();
			_vibDepth[voice] = v >> 4;
			_vibRate[voice] = v & 15;
			break;
		}
		case 7: { // program change
			int slot = next();
			if (_haveInstrument[slot])
				loadInstrument(voice, _instrument[slot]);
			break;
		}
		case 8:
			switch (voice) {
			case 1: { // call: a repeat count and the offset of the subroutine
				int count = next();
				int lo = next(), hi = next();
				if (_depth < 16) {
					_stack[_depth].count = count;
					_stack[_depth].returnTo = _pos;
					_stack[_depth].target = (size_t)(lo | (hi << 8));
					_pos = _stack[_depth].target;
					++_depth;
				}
				break;
			}
			case 2: // delay: the tick ends here
				_delay = next();
				effects();
				return;
			case 3: { // load an instrument into a slot (it stays in the track)
				int slot = next();
				_instrument[slot] = _pos;
				_haveInstrument[slot] = true;
				_pos += 11;
				break;
			}
			case 5:
			case 6: { // fade out (the level rises) or in: the voice in the high nibble, the number of ticks per step less one in the low
				int v = next();
				int vc = v >> 4;
				_fade[vc] = voice == 5 ? 1 : -1;
				_fadeReload[vc] = _fadeCount[vc] = (v & 15) + 1;
				break;
			}
			default:
				break;
			}
			break;
		case 0xE: // loop point
			_loop = _pos;
			break;
		case 0xF: // return, or back to the loop point at the top
			if (_depth == 0) {
				_pos = _loop;
			} else if (--_stack[_depth - 1].count != 0) {
				_pos = _stack[_depth - 1].target;
			} else {
				_pos = _stack[_depth - 1].returnTo;
				--_depth;
			}
			break;
		default:
			break;
		}
	}
}

void MusicPlayer::render(int16_t *out, int count) {
	while (count > 0) {
		// samples until the next tick: the timer runs at 60 Hz
		int toTick = (Opl2::SAMPLE_RATE - _sampleFraction + TICKS_PER_SECOND - 1) / TICKS_PER_SECOND;
		if (toTick < 1)
			toTick = 1;
		int n = toTick < count ? toTick : count;
		_chip.generate(out, n);
		out += n;
		count -= n;
		_sampleFraction += n * TICKS_PER_SECOND;
		if (_sampleFraction >= Opl2::SAMPLE_RATE) {
			_sampleFraction -= Opl2::SAMPLE_RATE;
			tick();
		}
	}
}

} // namespace Ultima6
