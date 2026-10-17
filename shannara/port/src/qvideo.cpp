#include "qvideo.h"
#include <cstring>

namespace shannara {

#include "q_tables.inc"

static int64_t floordiv(int64_t a, int64_t b) {
	int64_t q = a / b;
	if ((a % b != 0) && ((a < 0) != (b < 0)))
		q--;
	return q;
}

QBlockDecoder::QBlockDecoder(int w, int h, const uint8_t *qmap, Bytes &video, const Bytes *composite, bool rawMode)
	: _W(w), _H(h), _qmap(qmap, qmap + 0x100), _v(video), _comp(composite), _raw(rawMode) {}

int64_t QBlockDecoder::baddr(int64_t idx) const {
	int64_t q = floordiv(idx, _W);
	return q * kRow + (idx - q * _W) * 4;
}

void QBlockDecoder::getBlock(const Bytes *buf, int64_t off, Block blk) const {
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++) {
			int64_t k = off + (int64_t)y * kPitch + x;
			blk[y][x] = (buf && k >= 0 && (size_t)k < buf->size()) ? (*buf)[k] : 0;
		}
}

void QBlockDecoder::putBlock(int64_t off, const Block blk) {
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++) {
			int64_t k = off + (int64_t)y * kPitch + x;
			if (k >= 0 && (size_t)k < _v.size())
				_v[k] = blk[y][x];
		}
}

void QBlockDecoder::maskRows(int hiByte, int loByte, int c0, int c1, Block out) const {
	// byte `hiByte` gives rows 0,1; byte `loByte` rows 2,3 (high nibble = upper row of each pair)
	const uint32_t m[4] = {kMaskHi[hiByte & 0xFF], kMaskLo[hiByte & 0xFF], kMaskHi[loByte & 0xFF], kMaskLo[loByte & 0xFF]};
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++)
			out[y][x] = ((m[y] >> (8 * x)) & 0xFF) ? c1 : c0;
}

size_t QBlockDecoder::decode(const uint8_t *stream, size_t size, int minCol, int minRow, int maxCol, int maxRow) {
	_s.assign(stream, stream + size);
	_s.resize(size + 64, 0); // the original pads with zeros the same way
	_p = 0;
	_idx = (int64_t)minRow * _W + minCol;
	_cols = maxCol;
	_rows = maxRow;
	_maxCol = maxCol;
	_dst = _rowstart = baddr(_idx);
	_rowI = 0;
	_state = T0;
	_c0 = _c1 = 0;
	_done = false;
	_lastAbs = 0;
	while (!_done) {
		if (_p < 0 || (size_t)_p >= _s.size())
			break; // malformed stream: never run past the padded buffer
		step();
	}
	return (size_t)_p;
}

void QBlockDecoder::nextBlock(int n) {
	_idx += n;
	_dst += 4 * n;
	_cols -= n;
	if (_cols == -1) {
		_cols = _maxCol;
		_rows--;
		_idx += _W - _maxCol - 1;
		if (_rows == 0) {
			_done = true;
			return;
		}
		_rowI++;
		_rowstart += kRow;
		_dst = _rowstart;
		_state = T0;
	}
}

void QBlockDecoder::finish(int n, State nxt) {
	int rowBefore = _rowI;
	nextBlock(n);
	if (!_done && _rowI == rowBefore)
		_state = nxt;
}

void QBlockDecoder::runDone(int n) {
	nextBlock(n);
	if (!_done)
		_state = T0;
}

void QBlockDecoder::step() {
	int op = b();
	if (op < 0xF8) {
		plain(op);
		return;
	}
	switch (op & 7) {
	case 0: esc0(); break;
	case 1: esc1(); break;
	case 2: esc2(); break;
	case 3: esc3(); break;
	case 4: esc4(); break;
	case 5: esc5(); break;
	case 6: esc6(); break;
	default: esc7(); break;
	}
}

void QBlockDecoder::plain(int c0) {
	int c1 = b(1);
	Block blk;
	State nxt;
	if (c1 == c0) {
		memset(blk, c0, sizeof blk);
		_p += 2;
		nxt = SOLID;
	} else if (b(2) & 0x80) {
		int b2 = b(2), b3 = b(3);
		_c0 = c0;
		_c1 = c1;
		maskRows(b3, b2, c0, c1, blk);
		_p += 4;
		nxt = BLK4;
	} else {
		int q = b(2);
		_c0 = c0;
		_c1 = c1;
		maskRows(_qmap[q * 2 + 1], _qmap[q * 2], c0, c1, blk);
		_p += 3;
		nxt = BLK3;
	}
	putBlock(_dst, blk);
	finish(1, nxt);
}

void QBlockDecoder::copyFrom(int64_t src, int64_t dst) {
	Block blk;
	getBlock(&_v, src, blk);
	putBlock(dst, blk);
}

void QBlockDecoder::esc7() {
	_p += 1;
	finish(1, SKIP);
}

void QBlockDecoder::esc6() {
	int bb = b(1);
	copyFrom(baddr((_idx - bb - 1) & 0xFFFF), _dst);
	_p += 2;
	finish(1, BACK);
}

void QBlockDecoder::esc5() {
	int bb = b(1);
	copyFrom(baddr(_idx + bb + 1), _dst);
	_p += 2;
	finish(1, FWD);
}

void QBlockDecoder::esc4() {
	int bb = b(1);
	copyFrom(_dst + kMotion[bb], _dst);
	_p += 2;
	finish(1, MOTION);
}

void QBlockDecoder::esc2() {
	int i = b(1) | (b(2) << 8);
	copyFrom(baddr(i), _dst);
	_lastAbs = baddr(i);
	_p += 3;
	finish(1, ABS);
}

void QBlockDecoder::esc0() {
	int sub = b(1);
	int64_t compOff = baddr(_idx);
	Block cb, blk;
	getBlock(_comp, compOff, cb);
	if (sub == 0xF8) {
		putBlock(_dst, cb);
		_p += 2;
		finish(1, COMP_COPY);
	} else if (sub == 0xFB) {
		for (int y = 0; y < 4; y++)
			for (int x = 0; x < 4; x++) {
				int v = b(2 + y * 4 + x);
				blk[y][x] = (v == 0xF8) ? cb[y][x] : v;
			}
		putBlock(_dst, blk);
		_p += 18;
		finish(1, COMP_OVL);
	} else {
		int c = sub, b2 = b(2), b3 = b(3);
		const uint32_t m[4] = {kMaskHi[b3], kMaskLo[b3], kMaskHi[b2], kMaskLo[b2]};
		for (int y = 0; y < 4; y++)
			for (int x = 0; x < 4; x++)
				blk[y][x] = ((m[y] >> (8 * x)) & 0xFF) ? cb[y][x] : c;
		putBlock(_dst, blk);
		_p += 4;
		finish(1, COMP_OVL);
	}
}

void QBlockDecoder::esc1() {
	int n = b(1);
	int64_t d = _dst;
	Block blk;
	switch (_state) {
	case T0: case BLK4: case SOLID: case BLK3:
		getBlock(&_v, baddr(_idx - 1), blk);
		for (int k = 0; k < n; k++)
			putBlock(d + 4 * k, blk);
		break;
	case SKIP:
		break;
	case BACK: {
		int64_t src = baddr((_idx - at(_p - 1) - 1) & 0xFFFF);
		for (int k = 0; k < n; k++)
			copyFrom(src + 4 * k, d + 4 * k);
		break;
	}
	case FWD: {
		int64_t src = baddr(_idx + at(_p - 1) + 1);
		for (int k = 0; k < n; k++)
			copyFrom(src + 4 * k, d + 4 * k);
		break;
	}
	case MOTION: {
		int64_t src = d + kMotion[at(_p - 1)];
		for (int k = 0; k < n; k++)
			copyFrom(src + 4 * k, d + 4 * k);
		break;
	}
	case ABS:
		getBlock(&_v, _lastAbs, blk);
		for (int k = 0; k < n; k++)
			putBlock(d + 4 * k, blk);
		break;
	case COMP_COPY: {
		int64_t src = baddr(_idx);
		for (int k = 0; k < n; k++) {
			getBlock(_comp, src + 4 * k, blk);
			putBlock(d + 4 * k, blk);
		}
		break;
	}
	case COMP_OVL: // quirk: consumes 2 bytes, skips ONE block
		_p += 2;
		finish(1, T0);
		_state = T0;
		return;
	}
	_p += 2;
	runDone(n);
}

void QBlockDecoder::rawRun(int n) {
	int64_t pos = _p + 2;
	int64_t d = _dst;
	_cols -= n;
	_idx += n;
	int k = 0;
	while (k < n) {
		int bb = at(pos);
		Block blk;
		uint8_t px[16];
		if (bb < 0xF8) {
			for (int y = 0; y < 4; y++)
				for (int x = 0; x < 4; x++)
					blk[y][x] = at(pos + y * 4 + x);
			pos += 16;
		} else if (bb == 0xF9 || bb == 0xFA) {
			uint32_t bits = at(pos + 1) | (at(pos + 2) << 8) | (at(pos + 3) << 16) | ((uint32_t)at(pos + 4) << 24);
			int ncol = (bb & 7) + 2;
			uint8_t cols[16];
			for (int i = 0; i < ncol; i++)
				cols[i] = at(pos + 5 + i);
			for (int i = 0; i < 16; i++) {
				int ci = (bits >> (2 * i)) & 3;
				px[i] = ci < ncol ? cols[ci] : 0;
			}
			for (int y = 0; y < 4; y++)
				for (int x = 0; x < 4; x++)
					blk[y][x] = px[y * 4 + x];
			pos += 5 + ncol;
		} else if (bb >= 0xFB && bb <= 0xFE) {
			uint32_t lo = at(pos + 1) | (at(pos + 2) << 8) | (at(pos + 3) << 16) | ((uint32_t)at(pos + 4) << 24);
			uint32_t hi = at(pos + 4) | (at(pos + 5) << 8) | (at(pos + 6) << 16) | ((uint32_t)at(pos + 7) << 24);
			int ncol = (bb & 7) + 2;
			uint8_t cols[16] = {};
			for (int i = 0; i < ncol; i++)
				cols[i] = at(pos + 7 + i);
			for (int i = 0; i < 8; i++)
				px[i] = cols[(lo >> (3 * i)) & 7];
			for (int i = 0; i < 8; i++)
				px[8 + i] = cols[(hi >> (3 * i)) & 7];
			for (int y = 0; y < 4; y++)
				for (int x = 0; x < 4; x++)
					blk[y][x] = px[y * 4 + x];
			pos += 7 + ncol;
		} else { // F8 / FF: skip one block and leave the run
			pos += 2;
			d += 4;
			_cols -= 1;
			_idx += 1;
			break;
		}
		putBlock(d, blk);
		d += 4;
		k++;
	}
	_p = pos;
	_dst = d;
	runEndRaw();
}

void QBlockDecoder::runEndRaw() {
	if (_cols == -1) {
		_cols = _maxCol;
		_rows--;
		_idx += _W - _maxCol - 1;
		if (_rows == 0) {
			_done = true;
			return;
		}
		_rowI++;
		_rowstart += kRow;
		_dst = _rowstart;
	}
	_state = T0;
}

void QBlockDecoder::esc3() {
	int n = b(1);
	State st = _state;
	int64_t d = _dst;
	int64_t q = _p + 2;
	if (_raw || st == T0 || st == SKIP || st == COMP_COPY || st == COMP_OVL) {
		rawRun(n);
		return;
	}
	Block blk;
	switch (st) {
	case SOLID:
		for (int k = 0; k < n; k++) {
			memset(blk, at(q + k), sizeof blk);
			putBlock(d + 4 * k, blk);
		}
		_p = q + n;
		break;
	case BACK:
		for (int k = 0; k < n; k++)
			copyFrom(baddr((_idx + k - at(q + k) - 1) & 0xFFFF), d + 4 * k);
		_p = q + n;
		break;
	case FWD:
		for (int k = 0; k < n; k++)
			copyFrom(baddr(_idx + k + at(q + k) + 1), d + 4 * k);
		_p = q + n;
		break;
	case MOTION:
		for (int k = 0; k < n; k++)
			copyFrom(d + 4 * k + kMotion[at(q + k)], d + 4 * k);
		_p = q + n;
		break;
	case ABS:
		for (int k = 0; k < n; k++) {
			int i = at(q + 2 * k) | (at(q + 2 * k + 1) << 8);
			copyFrom(baddr(i), d + 4 * k);
		}
		_p = q + 2 * n;
		break;
	case BLK4:
		for (int k = 0; k < n; k++) {
			maskRows(at(q + 2 * k + 1), at(q + 2 * k), _c0, _c1, blk);
			putBlock(d + 4 * k, blk);
		}
		_p = q + 2 * n;
		break;
	case BLK3: {
		int64_t pos = q;
		for (int k = 0; k < n; k++) {
			if (at(pos) & 0x80) {
				maskRows(at(pos + 1), at(pos), _c0, _c1, blk);
				pos += 2;
			} else {
				int qi = at(pos);
				pos += 1;
				maskRows(_qmap[qi * 2 + 1], _qmap[qi * 2], _c0, _c1, blk);
			}
			putBlock(d + 4 * k, blk);
		}
		_p = pos;
		break;
	}
	default:
		break;
	}
	runDone(n);
}

// ---- container -----------------------------------------------------------------------------------------------------

QVideo::QVideo(Bytes data) : _d(std::move(data)) {
	Cursor c(_d);
	if (c.u16() != 0x6839)
		throw FormatError("not a Q file");
	c.seek(4);
	_W = c.u16();
	_H = c.u16();
	c.seek(0xA);
	_nframes = c.u16();
	c.seek(0x10);
	_fps = c.u8();
	if (!_fps)
		_fps = 5;
	c.seek(0x16);
	_rate = c.u32();
	_audio = _rate / _fps;
	_video.assign((size_t)QBlockDecoder::kRow * _H + 0x10000, 0);
	_qmap.assign(0x100, 0);
	_win[0] = 0;
	_win[1] = 0;
	_win[2] = _W - 1;
	_win[3] = _H;
}

bool QVideo::nextFrame(QFrame &info) {
	while (_pos + 6 <= _d.size()) {
		Cursor c(_d, _pos);
		int type = c.u16();
		uint32_t size = c.u32();
		size_t body = _pos + 6;
		if (type == 0xFFFF)
			return false;
		if (type == 6) {
			_comp.assign(_video.begin(), _video.begin() + (size_t)_W * 4 * _H * 4);
			_comp.resize(_comp.size() + 0x10000, 0);
			_haveComp = true;
			_pos = body;
			continue;
		}
		if (type == 7) {
			_raw = true;
			_pos = body;
			continue;
		}
		if (type == 2 || type == 3 || type == 4)
			size += _audio;
		if (body + size > _d.size())
			throw FormatError("Q chunk past end of file");
		const uint8_t *payload = &_d[body];
		_pos = body + size;
		if (type == 1) {
			memset(_pal, 0, 768);
			memcpy(_pal, payload, size < 768 ? size : 768);
		} else if (type == 5) {
			_qmap.assign(0x100, 0);
			memcpy(_qmap.data(), payload, size < 0x100 ? size : 0x100);
		} else if (type == 2 || type == 3 || type == 4) {
			if (type == 3 || type == 4) {
				Cursor t(_d, _pos);
				for (int i = 0; i < 4; i++)
					_win[i] = t.u16();
				_pos += 8;
			}
			QBlockDecoder dec(_W, _H, _qmap.data(), _video, _haveComp ? &_comp : nullptr, _raw);
			size_t streamBytes = size - _audio;
			info.consumed = dec.decode(payload + _audio, streamBytes, _win[0], _win[1], _win[2], _win[3]);
			info.index = _f++;
			info.type = type;
			info.streamBytes = streamBytes;
			return true;
		}
	}
	return false;
}

} // namespace shannara
