#include "audio.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace shannara {

Audio::Audio() : _seqs(8) {}

void Audio::setMidiSink(std::function<void(uint32_t)> s) {
	std::lock_guard<std::mutex> lk(_mu);
	_sink = std::move(s);
}

void Audio::send(uint32_t msg) {
	midiMessages++;
	if (_sink)
		_sink(msg);
}

Audio::Seq *Audio::get(uint32_t handle) {
	if (handle < kHandleBase || handle >= kHandleBase + 64 * _seqs.size() || (handle - kHandleBase) % 64)
		return nullptr;
	Seq &s = _seqs[(handle - kHandleBase) / 64];
	return s.used ? &s : nullptr;
}

static uint32_t be32(const std::vector<uint8_t> &d, size_t o) { return ((uint32_t)d[o] << 24) | (d[o + 1] << 16) | (d[o + 2] << 8) | d[o + 3]; }

// `addr` points at an XMIDI file (FORM XDIR ... CAT XMID FORM XMID ... EVNT): take the EVNT chunk of the number-th FORM XMID
bool Audio::parse(Seq &s, const Machine &m, uint32_t addr, int number) {
	std::vector<uint8_t> d(0x20000);
	size_t n = 0;
	for (; n < d.size() && addr + n < m.mem.size(); n++)
		d[n] = m.r8(addr + (uint32_t)n);
	d.resize(n);
	if (d.size() < 32 || memcmp(d.data(), "FORM", 4))
		return false;
	// find the CAT chunk holding the sequences
	size_t p = 12;
	while (p + 8 <= d.size() && memcmp(&d[p], "CAT ", 4)) {
		uint32_t len = be32(d, p + 4);
		p += 8 + len + (len & 1);
	}
	if (p + 12 > d.size())
		return false;
	size_t catEnd = std::min(d.size(), p + 8 + be32(d, p + 4));
	p += 12; // skip "CAT " len "XMID"
	int k = 0;
	while (p + 12 <= catEnd) {
		uint32_t len = be32(d, p + 4);
		if (!memcmp(&d[p], "FORM", 4) && !memcmp(&d[p + 8], "XMID", 4)) {
			if (k++ == number) {
				size_t q = p + 12, end = std::min(catEnd, p + 8 + len);
				while (q + 8 <= end) {
					uint32_t cl = be32(d, q + 4);
					if (!memcmp(&d[q], "EVNT", 4)) {
						s.evnt.assign(d.begin() + q + 8, d.begin() + std::min(end, q + 8 + cl));
						return true;
					}
					q += 8 + cl + (cl & 1);
				}
				return false;
			}
		}
		p += 8 + len + (len & 1);
	}
	return false;
}

void Audio::silence(Seq &s) {
	for (auto &o : s.offs)
		send(0x80 | o.ch | (o.note << 8));
	s.offs.clear();
	for (int c = 0; c < 16; c++)
		if (s.chanUsed[c]) {
			send(0xB0 | c | (123 << 8)); // all notes off
			send(0xB0 | c | (121 << 8)); // reset controllers
		}
}

void Audio::step(Seq &s, uint32_t handle) {
	auto readDelay = [&]() {
		while (s.pos < s.evnt.size() && s.evnt[s.pos] < 0x80)
			s.nextTime += s.evnt[s.pos++];
	};
	// pending note-offs
	for (size_t i = 0; i < s.offs.size();) {
		if (s.offs[i].at <= s.now) {
			send(0x80 | s.offs[i].ch | (s.offs[i].note << 8));
			s.offs.erase(s.offs.begin() + i);
		} else {
			i++;
		}
	}
	while (s.pos < s.evnt.size() && s.nextTime <= s.now) {
		uint8_t st = s.evnt[s.pos++];
		uint8_t ch = st & 15;
		auto rd = [&]() -> uint8_t { return s.pos < s.evnt.size() ? s.evnt[s.pos++] : 0; };
		if (st >= 0x80 && st < 0xF0) {
			s.chanUsed[ch] = true;
			switch (st & 0xF0) {
			case 0x90: {
				uint8_t note = rd(), vel = rd();
				uint32_t dur = 0;
				for (int k = 0; k < 4; k++) { // variable-length duration in intervals
					uint8_t b = rd();
					dur = (dur << 7) | (b & 0x7F);
					if (!(b & 0x80))
						break;
				}
				send(0x90 | ch | (note << 8) | ((uint32_t)vel << 16));
				s.offs.push_back({s.now + dur, ch, note});
				break;
			}
			case 0xB0: {
				uint8_t cc = rd(), v = rd();
				if (cc == 7) { // channel volume follows the sequence volume
					s.chanVol[ch] = v;
					send(0xB0 | ch | (7 << 8) | ((uint32_t)scaled(s, v) << 16));
				} else if (cc == 116) { // XMIDI FOR loop (count 0 = forever)
					s.loops.push_back({s.pos, v});
				} else if (cc == 117) { // NEXT / break
					if (!s.loops.empty()) {
						Seq::Loop &lp = s.loops.back();
						if (v >= 64) {
							s.loops.pop_back();
						} else if (lp.count == 0 || --lp.count > 0) {
							s.pos = lp.pos;
						} else {
							s.loops.pop_back();
						}
					}
				} else if (cc >= 110 && cc <= 120) {
					// other XMIDI controllers (channel/voice locks, protection, patch bank select, trigger): not sent to a GM synthesizer
				} else {
					send(0xB0 | ch | (cc << 8) | ((uint32_t)v << 16));
				}
				break;
			}
			case 0xC0: case 0xD0: {
				uint8_t a = rd();
				send(st | (a << 8));
				break;
			}
			case 0xA0: case 0xE0: {
				uint8_t a = rd(), b = rd();
				send(st | (a << 8) | ((uint32_t)b << 16));
				break;
			}
			default:
				break;
			}
		} else if (st == 0xFF) {
			uint8_t type = rd();
			uint32_t len = 0;
			for (int k = 0; k < 4; k++) {
				uint8_t b = rd();
				len = (len << 7) | (b & 0x7F);
				if (!(b & 0x80))
					break;
			}
			if (type == 0x51 && len == 3 && s.pos + 3 <= s.evnt.size())
				s.tempoUs = (s.evnt[s.pos] << 16) | (s.evnt[s.pos + 1] << 8) | s.evnt[s.pos + 2];
			if (type == 0x2F) { // end of track
				s.pos = s.evnt.size();
				break;
			}
			s.pos += len;
		} else if (st == 0xF0 || st == 0xF7) {
			uint32_t len = 0;
			for (int k = 0; k < 4; k++) {
				uint8_t b = rd();
				len = (len << 7) | (b & 0x7F);
				if (!(b & 0x80))
					break;
			}
			s.pos += len;
		}
		readDelay();
	}
	if (s.pos >= s.evnt.size() && s.offs.empty() && s.playing) {
		s.playing = false;
		s.finished = true;
		silence(s);
		if (s.callback)
			_done.push_back({s.callback, handle});
	}
}

void Audio::advance(double seconds) {
	std::lock_guard<std::mutex> lk(_mu);
	for (size_t i = 0; i < _seqs.size(); i++) {
		Seq &s = _seqs[i];
		if (!s.used || !s.playing)
			continue;
		if (s.fadeMs > 0) {
			s.fadeElapsed += seconds * 1000;
			double f = std::min(1.0, s.fadeElapsed / s.fadeMs);
			int nv = (int)(s.fadeFrom + (s.fadeTo - s.fadeFrom) * f);
			if (nv != s.volume) {
				s.volume = nv;
				for (int c = 0; c < 16; c++)
					if (s.chanUsed[c])
						send(0xB0 | c | (7 << 8) | ((uint32_t)scaled(s, s.chanVol[c]) << 16));
			}
			if (f >= 1.0)
				s.fadeMs = 0;
		}
		double intervalUs = s.tempoUs / 60.0;
		s.now += seconds * 1e6 / intervalUs;
		step(s, kHandleBase + (uint32_t)i * 64);
	}
}

void Audio::takeCallbacks(std::vector<std::pair<uint32_t, uint32_t>> &out) {
	std::lock_guard<std::mutex> lk(_mu);
	out.insert(out.end(), _done.begin(), _done.end());
	_done.clear();
}

void Audio::install(Machine &m) {
	// every function below is cdecl: arguments at stackArg(i), result in eax, the caller pops
	auto shim = [&](const char *name, std::function<uint32_t(Machine &)> fn) {
		m.setShim(name, [this, name, fn](Machine &mm) {
			uint32_t r = fn(mm);
			mm.eax = r;
			mm.finishShim(name);
		});
	};
	shim("_AIL_startup", [](Machine &) { return 0u; });
	shim("_AIL_shutdown", [this](Machine &) {
		std::lock_guard<std::mutex> lk(_mu);
		for (auto &s : _seqs)
			if (s.used && s.playing)
				silence(s);
		return 0u;
	});
	shim("_AIL_delay", [](Machine &) { return 0u; });
	shim("_AIL_set_preference", [](Machine &) { return 0u; });
	shim("_AIL_set_GTL_filename_prefix", [](Machine &) { return 0u; });
	// drivers: a General MIDI synthesizer for music; no digital driver (see the header comment)
	shim("_AIL_install_MDI_INI", [](Machine &mm) {
		uint32_t out = mm.stackArg(0);
		mm.w32(out, 0x5DF000); // an opaque non-zero driver handle
		return 0u;             // 0 = installed
	});
	shim("_AIL_install_MDI_driver_file", [](Machine &) { return 0x5DF000u; });
	shim("_AIL_MDI_driver_type", [](Machine &) { return 1u; });
	shim("_AIL_install_DIG_INI", [](Machine &) { return 1u; }); // 1 = no digital driver configured
	shim("_AIL_allocate_sample_handle", [](Machine &) { return 0u; }); // no sample voices
	// sequences
	shim("_AIL_allocate_sequence_handle", [this](Machine &) {
		std::lock_guard<std::mutex> lk(_mu);
		for (size_t i = 0; i < _seqs.size(); i++)
			if (!_seqs[i].used) {
				Seq &s = _seqs[i];
				s = Seq();
				memset(s.chanVol, 127, sizeof s.chanVol);
				memset(s.chanUsed, 0, sizeof s.chanUsed);
				s.used = true;
				return kHandleBase + (uint32_t)i * 64;
			}
		return 0u;
	});
	shim("_AIL_init_sequence", [this](Machine &mm) {
		uint32_t h = mm.stackArg(0), addr = mm.stackArg(1);
		int number = (int)mm.stackArg(2);
		std::lock_guard<std::mutex> lk(_mu);
		Seq *s = get(h);
		if (!s)
			return 0u;
		bool ok = parse(*s, mm, addr, number);
		if (!ok && addr + 4 < mm.mem.size()) // the cache entry may hold a pointer to the pointer
			ok = parse(*s, mm, mm.r32(addr), number);
		s->pos = 0;
		s->now = s->nextTime = 0;
		while (s->pos < s->evnt.size() && s->evnt[s->pos] < 0x80)
			s->nextTime += s->evnt[s->pos++];
		s->playing = false;
		if (verbose)
			fprintf(stderr, "AIL init_sequence %X addr=%X #%d -> %s (%zu bytes of events)\n", h, addr, number, ok ? "ok" : "unparsable", s->evnt.size());
		return ok ? 1u : 0u;
	});
	shim("_AIL_start_sequence", [this](Machine &mm) {
		std::lock_guard<std::mutex> lk(_mu);
		if (Seq *s = get(mm.stackArg(0))) {
			s->pos = 0;
			s->now = s->nextTime = 0;
			while (s->pos < s->evnt.size() && s->evnt[s->pos] < 0x80)
				s->nextTime += s->evnt[s->pos++];
			s->loops.clear();
			s->offs.clear();
			s->playing = !s->evnt.empty();
			s->finished = false;
			sequencesStarted++;
			if (verbose)
				fprintf(stderr, "AIL start_sequence %X\n", mm.stackArg(0));
		}
		return 0u;
	});
	auto stopper = [this](Machine &mm) {
		std::lock_guard<std::mutex> lk(_mu);
		if (Seq *s = get(mm.stackArg(0))) {
			if (s->playing)
				silence(*s);
			s->playing = false;
		}
		return 0u;
	};
	shim("_AIL_stop_sequence", stopper);
	shim("_AIL_end_sequence", stopper);
	shim("_AIL_release_sequence_handle", [this](Machine &mm) {
		std::lock_guard<std::mutex> lk(_mu);
		if (Seq *s = get(mm.stackArg(0))) {
			if (s->playing)
				silence(*s);
			s->used = false;
			s->playing = false;
		}
		return 0u;
	});
	shim("_AIL_set_sequence_volume", [this](Machine &mm) {
		uint32_t h = mm.stackArg(0);
		int vol = std::max(0, std::min(127, (int)mm.stackArg(1)));
		int ms = (int)mm.stackArg(2);
		std::lock_guard<std::mutex> lk(_mu);
		if (Seq *s = get(h)) {
			if (ms > 0) {
				s->fadeFrom = s->volume;
				s->fadeTo = vol;
				s->fadeMs = ms;
				s->fadeElapsed = 0;
			} else {
				s->volume = vol;
				s->fadeMs = 0;
				for (int c = 0; c < 16; c++)
					if (s->chanUsed[c])
						send(0xB0 | c | (7 << 8) | ((uint32_t)scaled(*s, s->chanVol[c]) << 16));
			}
		}
		return 0u;
	});
	shim("_AIL_register_sequence_callback", [this](Machine &mm) {
		std::lock_guard<std::mutex> lk(_mu);
		uint32_t prev = 0;
		if (Seq *s = get(mm.stackArg(0))) {
			prev = s->callback;
			s->callback = mm.stackArg(1);
		}
		return prev;
	});
	shim("_AIL_sequence_position", [this](Machine &mm) {
		uint32_t pb = mm.stackArg(1), pm = mm.stackArg(2);
		if (pb) mm.w32(pb, 0);
		if (pm) mm.w32(pm, 0);
		return 0u;
	});
}

} // namespace shannara
