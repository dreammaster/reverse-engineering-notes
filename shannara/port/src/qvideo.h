// Legend "Q" block video (demo cutscenes). Port of reversing/scripts/q_decode_ref.py + q_play.py, which were verified
// against the real machine code (differential fuzzing under unicorn) and decode the real demo files completely.
// See analysis/q_codec_spec.md.
#pragma once
#include "common.h"

namespace shannara {

class QBlockDecoder {
public:
	static const int kPitch = 640;
	static const int kRow = kPitch * 4; // bytes per row of 4x4 blocks

	// `video` / `composite` are 640-pitch 8-bpp surfaces; composite may be null until a type-6 chunk snapshots one.
	QBlockDecoder(int blocksW, int blocksH, const uint8_t *qmap /*0x100 bytes*/, Bytes &video, const Bytes *composite, bool rawMode);
	// Decodes one block stream into `video`; returns the number of stream bytes consumed.
	// (minCol, minRow, maxCol, maxRow) follow the chunk trailer: maxCol+1 blocks per row, maxRow rows.
	size_t decode(const uint8_t *stream, size_t size, int minCol, int minRow, int maxCol, int maxRow);

private:
	enum State { T0, SKIP, BACK, BLK4, SOLID, FWD, MOTION, ABS, BLK3, COMP_COPY, COMP_OVL };
	typedef uint8_t Block[4][4];

	int64_t baddr(int64_t idx) const;
	void getBlock(const Bytes *buf, int64_t off, Block b) const;
	void putBlock(int64_t off, const Block b);
	void maskRows(int hiRowsByte, int loRowsByte, int c0, int c1, Block out) const;
	uint8_t at(int64_t k) const { return (k >= 0 && (size_t)k < _s.size()) ? _s[k] : 0; }
	uint8_t b(int k = 0) const { return at(_p + k); }
	void nextBlock(int n);
	void finish(int n, State nxt);
	void runDone(int n);
	void runEndRaw();
	void step();
	void plain(int c0);
	void esc0(), esc1(), esc2(), esc3(), esc4(), esc5(), esc6(), esc7();
	void rawRun(int n);
	void copyFrom(int64_t src, int64_t dst);

	int _W, _H;
	Bytes _qmap;
	Bytes &_v;
	const Bytes *_comp;
	bool _raw;
	Bytes _s;
	int64_t _p = 0, _idx = 0, _cols = 0, _rows = 0, _maxCol = 0, _dst = 0, _rowstart = 0, _lastAbs = 0;
	int _rowI = 0;
	State _state = T0;
	int _c0 = 0, _c1 = 0;
	bool _done = false;
};

struct QFrame {
	int index = 0;
	int type = 0;
	size_t streamBytes = 0, consumed = 0;
};

class QVideo {
public:
	explicit QVideo(Bytes data);
	int blocksWide() const { return _W; }
	int blocksHigh() const { return _H; }
	int width() const { return _W * 4; }
	int height() const { return _H * 4; }
	int fps() const { return _fps; }
	int headerFrameCount() const { return _nframes; }
	// Decodes the next video frame (first pass only; stops at the 0xFFFF end chunk). Returns false at the end.
	bool nextFrame(QFrame &info);
	const Bytes &video() const { return _video; }   // 640-pitch 8-bpp pixels
	const uint8_t *palette() const { return _pal; } // 768 bytes, 6-bit components
private:
	Bytes _d, _video, _comp, _qmap;
	bool _haveComp = false, _raw = false;
	int _W, _H, _nframes, _fps;
	uint32_t _rate, _audio;
	size_t _pos = 0x1A;
	int _f = 0;
	int _win[4];
	uint8_t _pal[768] = {};
};

} // namespace shannara
