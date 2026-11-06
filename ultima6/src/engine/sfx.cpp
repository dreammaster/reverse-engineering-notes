#include "engine/sfx.h"

#include <cmath>

namespace Ultima6 {

namespace {

const int AMPLITUDE = 3000;
const double UNIT_MS = 1.0;                                // sub_3B8AA's unit (APPROX 73)
const int SHIFT[12] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3}; // ds:3066

// The speaker as the original drives it: a tone from the timer (sub_3BA08, a square wave of the frequency), silence (sub_3BA00), or the speaker bit itself (sub_3B956).
class Speaker {
public:
	explicit Speaker(BorlandRandom &rng) : _rng(rng) {
	}

	void tone(int freq) {
		_freq = freq > 0 ? freq : 0;
	}
	void off() {
		_freq = 0;
	}
	// sub_3B8AA: `count` units of the delay of scale `scale`
	void delay(int count, int scale) {
		advance(count * UNIT_MS / (1 << SHIFT[scale]));
	}
	// sub_3BAB8: a tone for `count` units
	void toneFor(int count, int freq) {
		tone(freq);
		delay(count, 1);
		off();
	}
	// sub_36E28: a sweep from `lo` to `hi` over `count` of the delay units
	void sweep(int count, int unit, int hi, int lo) {
		if (count <= 0)
			return;
		int step = (int16_t)(((hi - lo) * unit) & 0xFFFF) / count;
		int f = lo;
		for (int done = 0; done < count; done += unit) {
			tone(f);
			delay(unit, 1);
			f += step;
		}
		off();
	}
	// sub_3BA34: random tones between 100 Hz and `top`, each for `each` sixteenths of a unit, until `total` of them have gone
	void noise(int top, int total, int each) {
		int sum = 0;
		do {
			_noise = (uint16_t)(_noise + 0x9248);
			_noise = (uint16_t)((_noise >> 3) | (_noise << 13));
			_noise = (uint16_t)((_noise ^ 0x9248) + 0x11);
			int span = top - 100 + 1;
			tone(span > 0 ? _noise % span + 100 : 100);
			advance(each * UNIT_MS / 16.0);
			sum += each;
		} while (sum < total);
		off();
	}
	// sub_3B956: the speaker bit is set while a phase running up by `rate` a step exceeds a threshold that moves by `slope`, for `steps` steps of `width` 24ths of a unit
	void pulse(int slope, int start, int steps, int width, int rate) {
		uint16_t dx = 0, bx = (uint16_t)start;
		for (int i = 0; i < steps; ++i) {
			dx = (uint16_t)(dx + rate);
			bool on = dx > bx;
			bx = (uint16_t)(bx + slope);
			advance(width * UNIT_MS / 24.0, on ? 1 : -1);
		}
		off();
	}
	int roll(int a, int b) {
		if (a > b)
			std::swap(a, b);
		return a + _rng.next() % (b - a + 1);
	}

	std::vector<int16_t> out;

private:
	// `level` 0 follows the tone, +1 / -1 hold the speaker up or down
	void advance(double ms, int level = 0) {
		_carry += ms * Sfx::SAMPLE_RATE / 1000.0;
		int n = (int)_carry;
		_carry -= n;
		for (int i = 0; i < n; ++i) {
			int s = 0;
			if (level)
				s = level * AMPLITUDE / 2;
			else if (_freq > 0) {
				s = _phase < 0.5 ? AMPLITUDE / 2 : -AMPLITUDE / 2;
				_phase += (double)_freq / Sfx::SAMPLE_RATE;
				if (_phase >= 1)
					_phase -= 1;
			}
			out.push_back((int16_t)s);
		}
	}

	BorlandRandom &_rng;
	int _freq = 0;
	double _phase = 0, _carry = 0;
	uint16_t _noise = 0;
};

} // namespace

std::vector<int16_t> Sfx::render(int effect, int param, BorlandRandom &rng) {
	Speaker s(rng);
	int di = param;
	switch (effect) {
	case 0:
		break;
	case 1:
		s.toneFor(10, di);
		break;
	case 2:
		s.pulse(-5, 30000, 6000, 1, di >> 1);
		break;
	case 3:
		s.pulse(-3, 15000, 4000, 1, di);
		break;
	case 4:
		s.noise(10000, 800, 1);
		break;
	case 5:
		di = 2000 - di * 16;
		for (int i = 0; i < 40; ++i) {
			if (s.roll(0, 3) == 0) {
				int f = s.roll(0, 200) + di - 100;
				if (f > 19)
					s.tone(f);
			} else {
				s.off();
			}
			s.delay(1, 8);
		}
		s.off();
		break;
	case 6:
		di = 1000 - di * 8;
		for (int i = 0; i < 40; ++i) {
			if (s.roll(0, 7) == 0) {
				int f = s.roll(0, di * 2) + di;
				if (f > 19)
					s.tone(f);
			} else {
				s.off();
			}
			s.delay(1, 8);
		}
		s.off();
		break;
	case 7:
		if (di < 0) {
			s.noise(3000, 800, 1);
			break;
		}
		for (int i = 0; i < 40; ++i) {
			if (di > 19) {
				if (s.roll(0, 7) == 0)
					s.tone(s.roll(0, 40) + di);
				else
					s.off();
			}
			s.delay(1, 8);
		}
		s.off();
		break;
	case 8:
		for (int i = 0; i < 40; ++i) {
			s.tone(s.roll(200, 19));
			s.delay(1, 8);
		}
		s.off();
		break;
	case 9:
		s.sweep(40, 1, 2000, 1200);
		break;
	case 10:
		s.noise(600, 7000, 1);
		break;
	case 11:
		for (int i = 0; i < 20; ++i)
			s.noise(s.roll(4800, 600), 350, 1);
		break;
	case 12:
	case 13:
		s.sweep(50, 1, 2000, 800);
		break;
	case 14:
		s.toneFor(10, 2000);
		break;
	case 15:
		for (int f = 2000; f < 20000; f += 1000)
			s.noise(f, 120, 40);
		break;
	case 16:
		di = 1000 - di * 8;
		for (int i = 0; i < 40; ++i) {
			if (s.roll(0, 1) != 0 && di > 20)
				s.tone(s.roll(s.roll(di, 20), 19));
			else
				s.off();
			s.delay(1, 8);
		}
		s.off();
		break;
	case 17:
		s.sweep(150, 5, 750, 400);
		break;
	case 18:
		s.pulse(-20, 20000, 2000, 1, di >> 1);
		break;
	case 19:
		s.toneFor(200, di >> 2);
		break;
	case 20:
		s.toneFor(6, di);
		s.pulse(-25, 30000, 700, 1, di);
		break;
	case 21:
		for (int i = 0; i < 50; ++i)
			s.pulse((50 - i) * 4, 10000 - i * 64, 1000, 1, i * 16 + 800);
		s.pulse(8, 0, 8000, 1, 0x640);
		break;
	case 22:
		s.sweep(100, 1, 8000, 5000);
		break;
	default:
		s.toneFor(100, di);
		break;
	}
	return s.out;
}

} // namespace Ultima6
