#ifndef ULTIMA6_ENGINE_OPL2_H
#define ULTIMA6_ENGINE_OPL2_H

#include <cstdint>

namespace Ultima6 {

/**
 * The Yamaha YM3812 (OPL2) of an AdLib card: nine two-operator voices, written through the same register numbers the sound driver
 * U6ADLIB.DRV uses. This is a floating-point model of the chip, not a cycle-exact one: sine, half sine, absolute sine and quarter
 * waves, the envelope with the datasheet's attack and decay times (rate, key scale rate, sustain level, release), key scale level,
 * total level, frequency multipliers, feedback, both connection types, tremolo and vibrato. The rhythm section is not modelled
 * (the driver never switches it on).
 */
class Opl2 {
public:
	enum { SAMPLE_RATE = 49716 };

	Opl2();
	void reset();
	/** A write to register `reg` (0 to 0xF5). */
	void write(int reg, int value);
	/** Mono 16-bit samples at SAMPLE_RATE. */
	void generate(int16_t *out, int count);

	/** The last value written to a register (for tests). */
	int read(int reg) const {
		return _regs[reg & 0xFF];
	}

private:
	enum State { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };

	struct Op {
		bool am = false, vib = false, sustained = false, ksr = false;
		int mult = 0, ksl = 0, tl = 0, ar = 0, dr = 0, sl = 0, rr = 0, wave = 0;
		State state = OFF;
		double env = 511; // attenuation in steps of 0.1875 dB
		uint32_t phase = 0;
		double prev1 = 0, prev2 = 0;
	};
	struct Voice {
		int fnum = 0, block = 0;
		bool keyOn = false, additive = false;
		int feedback = 0;
		Op op[2]; // modulator, carrier
	};

	void keyChange(Voice &v, bool on);
	double rateEffective(const Voice &v, const Op &o, int rate) const;
	double step(const Voice &v, Op &o, double modulation, double tremolo, double vibrato, bool feedback, int fb);
	void updateEnvelope(const Voice &v, Op &o);

	uint8_t _regs[256];
	Voice _voice[9];
	bool _waveSelect = false, _noteSelect = false, _deepTremolo = false, _deepVibrato = false;
	double _tremoloPhase = 0, _vibratoPhase = 0;
};

} // namespace Ultima6

#endif
