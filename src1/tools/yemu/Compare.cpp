#include "Compare.h"

#include <stdio.h>
#include <string.h>

namespace Yemu {

using namespace Yendor1;

VidCompare::VidCompare() : _video(new Vid::Video()), _mismatchTotal(0), _ctxSeg(0) {
}

void VidCompare::attach(Machine &m, const std::string &fontsDir) {
	if (!fontsDir.empty()) {
		_fonts.load(fontsDir.c_str());
		_fonts.attach(*_video);
	}
	m.int62Hook = [this](Machine &mm) { return onCall(mm); };
}

std::string VidCompare::diffPlanes(Machine &m, const Vid::Video &v) {
	for (int p = 0; p < 4; p++) {
		const uint8_t *a = m.ega.plane[p];
		const uint8_t *b = v.planeData(p);
		for (int i = 0; i < 65536; i++) {
			if (a[i] != b[i]) {
				const int page = i >= 0x8000 ? 1 : 0;
				const int off = i & 0x7FFF;
				char buf[160];
				snprintf(buf, sizeof(buf), "plane %d offset %04X (page %d row %d byte %d): real %02X mine %02X", p, i, page, off / 80, off % 80, a[i], b[i]);
				return buf;
			}
		}
	}
	return "";
}

bool VidCompare::onCall(Machine &m) {
	const int ah = m.cpu.r[AX] >> 8, al = m.cpu.r[AX] & 0xFF;
	_calls[ah]++;
	if (ah == 0 || ah == 1 || _ctxSeg == 0) {
		// init / shutdown: just run the real driver and learn the context segment
		m.syncInterrupt(0x62);
		m.cpu.r[AX] = m.syncResultAx;
		if (ah == 0 && m.syncResultAx != 0xFFFF)
			_ctxSeg = m.syncResultAx;
		return true;
	}

	// snapshot the machine, run my driver on the copy
	_shadow.assign(m.mem, m.mem + (1 << 20));
	Vid::Video &v = *_video;
	uint8_t *ctxMem = &_shadow[Cpu::lin(_ctxSeg, 0)];
	v.loadContext(ctxMem);
	for (int p = 0; p < 4; p++)
		v.loadPlane(p, m.ega.plane[p]);
	v.setDisplayedPage(m.ega.displayedPage);
	const uint16_t imgSeg = (uint16_t)(ctxMem[0x16] | (ctxMem[0x17] << 8)) ;
	const uint16_t imgOff = (uint16_t)(ctxMem[0x18] | (ctxMem[0x19] << 8));
	// the far pointer: segment at 16h, offset at 18h
	const uint32_t imgBase = Cpu::lin(imgSeg, 0);
	const uint32_t room = imgBase < (1u << 20) ? (1u << 20) - imgBase : 0;
	v.ctx.image.set(&_shadow[imgBase], room > 0x10000 ? 0x10000 : room, imgOff);
	// cursor store: ctx+96Eh holds its segment
	const uint16_t curSeg = (uint16_t)(ctxMem[0x96E] | (ctxMem[0x96F] << 8));
	if (curSeg)
		memcpy(v.cursorStore(), &_shadow[Cpu::lin(curSeg, 0)], Vid::kCursorSlots * Vid::kCursorSlotBytes);
	v.call((uint8_t)ah, (uint8_t)al);
	// my driver's results
	std::vector<uint8_t> mine(0x980, 0);
	v.storeContext(&mine[0]);

	// run the real one
	m.syncInterrupt(0x62);
	m.cpu.r[AX] = m.syncResultAx;

	// compare
	const bool cursorOp = ah >= 0x28 || ah == 0x12 || ah == 0x14 || ah == 0x19 || ah == 0x1E || ah == 0x23;
	std::string why = diffPlanes(m, v);
	if (why.empty() && v.displayedPage() != m.ega.displayedPage) {
		char b[80];
		snprintf(b, sizeof(b), "displayed page: real %d mine %d", m.ega.displayedPage, v.displayedPage());
		why = b;
	}
	if (why.empty()) {
		const uint8_t *real = &m.mem[Cpu::lin(_ctxSeg, 0)];
		static const struct { int off, len; const char *name; } fields[] = {
			{0x10, 2, "imgRows"}, {0x12, 2, "imgBytes"}, {0x14, 1, "imgMask"}, {0x18, 2, "imgOff"}, {0x1A, 2, "x1"}, {0x1C, 2, "y1"},
			{0x1E, 2, "x2"}, {0x20, 2, "y2"}, {0x22, 2, "x3"}, {0x24, 2, "y3"}, {0x30, 1, "colour"}, {0x31, 1, "drawPage"},
			{0x953, 1, "mouseFlags"}, {0x955, 1, "cursorHide"}, {0x956, 1, "cursorShape"}, {0x957, 1, "cursorPage"}, {0x958, 1, "cursorMode"},
			{0x962, 2, "curX"}, {0x964, 2, "curY"}, {0x966, 2, "mouseX"}, {0x968, 2, "mouseY"}, {0x95A, 2, "clipX1"}, {0x95C, 2, "clipX2"},
			{0x95E, 2, "clipY1"}, {0x960, 2, "clipY2"},
		};
		for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]) && why.empty(); i++) {
			const int o = fields[i].off;
			if (o == 0x18) {
				if (cursorOp)
					continue;   // the real driver leaves 16h:18h pointing into its cursor store
				// image offset: my pointer advances inside ctx.image
				if ((uint16_t)(real[o] | (real[o + 1] << 8)) != (uint16_t)v.ctx.image.offset) {
					char b[96];
					snprintf(b, sizeof(b), "field imgOff: real %04X mine %04X", real[o] | (real[o + 1] << 8), (unsigned)v.ctx.image.offset);
					why = b;
				}
				continue;
			}
			if (memcmp(real + o, &mine[o], fields[i].len) != 0) {
				char b[128];
				unsigned rv = real[o] | (fields[i].len > 1 ? real[o + 1] << 8 : 0), mv = mine[o] | (fields[i].len > 1 ? mine[o + 1] << 8 : 0);
				snprintf(b, sizeof(b), "field %s: real %X mine %X", fields[i].name, rv, mv);
				why = b;
			}
		}
	}
	if (why.empty() && (ah == 0x09 || ah == 0x0B) && imgBase < (1u << 20)) {
		// data written through the image pointer (fn09/fn0B): compare the 64 KB the pointer addressed
		const uint32_t n = room > 0x10000 ? 0x10000 : room;
		for (uint32_t i = 0; i < n; i++)
			if (m.mem[imgBase + i] != _shadow[imgBase + i]) {
				char b[128];
				snprintf(b, sizeof(b), "image buffer %04X:%04X: real %02X mine %02X", imgSeg, (unsigned)i, m.mem[imgBase + i], _shadow[imgBase + i]);
				why = b;
				break;
			}
	}
	if (why.empty() && curSeg && cursorOp) {
		// the cursor store (slot 0 receives the saved background)
		const uint8_t *real = &m.mem[Cpu::lin(curSeg, 0)];
		const int n = Vid::kCursorSlots * Vid::kCursorSlotBytes;
		for (int i = 0; i < n; i++)
			if (real[i] != v.cursorStore()[i]) {
				char b[128];
				snprintf(b, sizeof(b), "cursor store slot %d byte %d: real %02X mine %02X", i / Vid::kCursorSlotBytes, i % Vid::kCursorSlotBytes, real[i], v.cursorStore()[i]);
				why = b;
				break;
			}
	}
	if (!why.empty()) {
		_bad[ah]++;
		_mismatchTotal++;
		if (_details.size() < 30) {
			char b[400];
			snprintf(b, sizeof(b), "AH=%02X AL=%02X x1=%u y1=%u x2=%u y2=%u x3=%u y3=%u col=%u page=%u: %s", ah, al,
				ctxMem[0x1A] | (ctxMem[0x1B] << 8), ctxMem[0x1C] | (ctxMem[0x1D] << 8), ctxMem[0x1E] | (ctxMem[0x1F] << 8),
				ctxMem[0x20] | (ctxMem[0x21] << 8), ctxMem[0x22] | (ctxMem[0x23] << 8), ctxMem[0x24] | (ctxMem[0x25] << 8),
				ctxMem[0x30], ctxMem[0x31], why.c_str());
			_details.push_back(b);
		}
		// resynchronise: the next call starts from the real machine state anyway
	}
	return true;
}

void VidCompare::report(FILE *out) const {
	fprintf(out, "Vid comparison: %d mismatching calls\n", _mismatchTotal);
	for (std::map<int, int>::const_iterator it = _calls.begin(); it != _calls.end(); ++it) {
		std::map<int, int>::const_iterator b = _bad.find(it->first);
		fprintf(out, "  AH=%02X calls %6d  mismatches %d\n", it->first, it->second, b == _bad.end() ? 0 : b->second);
	}
	for (size_t i = 0; i < _details.size(); i++)
		fprintf(out, "  %s\n", _details[i].c_str());
}

} // namespace Yemu
