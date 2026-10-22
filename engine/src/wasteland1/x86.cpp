#include "wasteland1/x86.h"
#include <cstdio>

namespace x86 {

void Cpu::wb(uint16_t seg, uint16_t off, uint8_t v) {
	const uint32_t a = lin(seg, off);
	if (a >= 0xA0000 && a < 0xB0000 && onWrite) { onWrite(a, v); return; }
	mem[a] = v;
}

uint16_t &Cpu::reg16(int i) {
	switch (i & 7) {
	case 0: return ax;
	case 1: return cx;
	case 2: return dx;
	case 3: return bx;
	case 4: return sp;
	case 5: return bp;
	case 6: return si;
	default: return di;
	}
}

uint8_t Cpu::get8(int i) {
	switch (i & 7) {
	case 0: return uint8_t(ax);
	case 1: return uint8_t(cx);
	case 2: return uint8_t(dx);
	case 3: return uint8_t(bx);
	case 4: return uint8_t(ax >> 8);
	case 5: return uint8_t(cx >> 8);
	case 6: return uint8_t(dx >> 8);
	default: return uint8_t(bx >> 8);
	}
}

void Cpu::set8(int i, uint8_t v) {
	switch (i & 7) {
	case 0: ax = uint16_t((ax & 0xFF00) | v); break;
	case 1: cx = uint16_t((cx & 0xFF00) | v); break;
	case 2: dx = uint16_t((dx & 0xFF00) | v); break;
	case 3: bx = uint16_t((bx & 0xFF00) | v); break;
	case 4: ax = uint16_t((ax & 0x00FF) | (v << 8)); break;
	case 5: cx = uint16_t((cx & 0x00FF) | (v << 8)); break;
	case 6: dx = uint16_t((dx & 0x00FF) | (v << 8)); break;
	default: bx = uint16_t((bx & 0x00FF) | (v << 8)); break;
	}
}

uint16_t &Cpu::sreg(int i) {
	switch (i & 3) {
	case 0: return es;
	case 1: return cs;
	case 2: return ss;
	default: return ds;
	}
}

uint16_t Cpu::segFor(uint16_t def) const {
	if (segOverride < 0) return def;
	switch (segOverride) {
	case 0: return es;
	case 1: return cs;
	case 2: return ss;
	default: return ds;
	}
}

Cpu::ModRM Cpu::decode(uint8_t b) {
	ModRM m{};
	m.mod = b >> 6; m.reg = (b >> 3) & 7; m.rm = b & 7;
	if (m.mod == 3) { m.isReg = true; return m; }
	uint16_t off = 0, def = ds;
	switch (m.rm) {
	case 0: off = uint16_t(bx + si); break;
	case 1: off = uint16_t(bx + di); break;
	case 2: off = uint16_t(bp + si); def = ss; break;
	case 3: off = uint16_t(bp + di); def = ss; break;
	case 4: off = si; break;
	case 5: off = di; break;
	case 6:
		if (m.mod == 0) off = fetch16();
		else { off = bp; def = ss; }
		break;
	default: off = bx; break;
	}
	if (m.mod == 1) off = uint16_t(off + int8_t(fetch8()));
	else if (m.mod == 2) off = uint16_t(off + fetch16());
	m.off = off;
	m.seg = segFor(def);
	return m;
}

uint16_t Cpu::readRm(const ModRM &m, bool wide) {
	if (m.isReg) return wide ? reg16(m.rm) : get8(m.rm);
	return wide ? rw(m.seg, m.off) : rb(m.seg, m.off);
}

void Cpu::writeRm(const ModRM &m, bool wide, uint16_t v) {
	if (m.isReg) { if (wide) reg16(m.rm) = v; else set8(m.rm, uint8_t(v)); return; }
	if (wide) ww(m.seg, m.off, v); else wb(m.seg, m.off, uint8_t(v));
}

void Cpu::szp(uint16_t r, bool wide) {
	const uint16_t mask = wide ? 0xFFFF : 0xFF;
	r &= mask;
	setFlag(ZF, r == 0);
	setFlag(SF, r & (wide ? 0x8000 : 0x80));
	uint8_t p = uint8_t(r);
	p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
	setFlag(PF, !(p & 1));
}

uint16_t Cpu::alu(int op, uint16_t a, uint16_t b, bool wide) {
	const uint32_t mask = wide ? 0xFFFF : 0xFF;
	const uint32_t sign = wide ? 0x8000 : 0x80;
	uint32_t r = 0;
	const uint32_t cf = flag(CF) ? 1 : 0;
	a &= uint16_t(mask); b &= uint16_t(mask);
	switch (op) {
	case 0: case 2: {   // add / adc
		const uint32_t c = op == 2 ? cf : 0;
		r = uint32_t(a) + b + c;
		setFlag(CF, r > mask);
		setFlag(AF, ((a ^ b ^ r) & 0x10) != 0);
		setFlag(OF, ((a ^ r) & (b ^ r) & sign) != 0);
		break;
	}
	case 3: case 5: case 7: {   // sbb / sub / cmp
		const uint32_t c = op == 3 ? cf : 0;
		r = uint32_t(a) - b - c;
		setFlag(CF, uint32_t(a) < uint32_t(b) + c);
		setFlag(AF, ((a ^ b ^ r) & 0x10) != 0);
		setFlag(OF, ((a ^ b) & (a ^ r) & sign) != 0);
		break;
	}
	case 1: r = a | b; setFlag(CF, false); setFlag(OF, false); setFlag(AF, false); break;
	case 4: r = a & b; setFlag(CF, false); setFlag(OF, false); setFlag(AF, false); break;
	default: r = a ^ b; setFlag(CF, false); setFlag(OF, false); setFlag(AF, false); break;
	}
	szp(uint16_t(r), wide);
	return op == 7 ? a : uint16_t(r & mask);
}

uint16_t Cpu::shiftRot(int op, uint16_t v, int count, bool wide) {
	const uint32_t mask = wide ? 0xFFFF : 0xFF;
	const uint32_t sign = wide ? 0x8000 : 0x80;
	uint32_t x = v & mask;
	for (int i = 0; i < count; ++i) {
		const uint32_t cf = flag(CF) ? 1 : 0;
		switch (op) {
		case 0: { const uint32_t o = (x & sign) ? 1 : 0; x = ((x << 1) | o) & mask; setFlag(CF, o); setFlag(OF, ((x & sign) != 0) != (o != 0)); break; }
		case 1: { const uint32_t o = x & 1; x = (x >> 1) | (o ? sign : 0); setFlag(CF, o); setFlag(OF, ((x ^ (x << 1)) & sign) != 0); break; }
		case 2: { const uint32_t o = (x & sign) ? 1 : 0; x = ((x << 1) | cf) & mask; setFlag(CF, o); setFlag(OF, ((x & sign) != 0) != (o != 0)); break; }
		case 3: { const uint32_t o = x & 1; const bool oldTop = (cf != 0) != ((x & sign) != 0); x = (x >> 1) | (cf ? sign : 0); setFlag(CF, o); setFlag(OF, ((x & sign) != 0) != ((x & (sign >> 1)) != 0)); (void)oldTop; break; }
		case 4: case 6: { const uint32_t o = (x & sign) ? 1 : 0; x = (x << 1) & mask; setFlag(CF, o); setFlag(OF, ((x & sign) != 0) != (o != 0)); szp(uint16_t(x), wide); break; }
		case 5: { const uint32_t o = x & 1; setFlag(OF, (x & sign) != 0); x >>= 1; setFlag(CF, o); szp(uint16_t(x), wide); break; }
		default: { const uint32_t o = x & 1; const uint32_t top = x & sign; x = (x >> 1) | top; setFlag(CF, o); setFlag(OF, false); szp(uint16_t(x), wide); break; }
		}
	}
	return uint16_t(x);
}

bool Cpu::cond(int cc) const {
	bool r = false;
	switch (cc >> 1) {
	case 0: r = flag(OF); break;
	case 1: r = flag(CF); break;
	case 2: r = flag(ZF); break;
	case 3: r = flag(CF) || flag(ZF); break;
	case 4: r = flag(SF); break;
	case 5: r = flag(PF); break;
	case 6: r = flag(SF) != flag(OF); break;
	default: r = flag(ZF) || (flag(SF) != flag(OF)); break;
	}
	return (cc & 1) ? !r : r;
}

void Cpu::stringOp(uint8_t op, bool wide, int rep) {
	const int delta = (wide ? 2 : 1) * (flag(DF) ? -1 : 1);
	const uint16_t srcSeg = segFor(ds);
	auto once = [&]() -> bool {   // returns whether the repeat-while-equal logic applies
		switch (op) {
		case 0xA4: if (wide) ww(es, di, rw(srcSeg, si)); else wb(es, di, rb(srcSeg, si)); si = uint16_t(si + delta); di = uint16_t(di + delta); return false;
		case 0xA6: { const uint16_t a = wide ? rw(srcSeg, si) : rb(srcSeg, si), b = wide ? rw(es, di) : rb(es, di); alu(7, a, b, wide); si = uint16_t(si + delta); di = uint16_t(di + delta); return true; }
		case 0xAA: if (wide) ww(es, di, ax); else wb(es, di, uint8_t(ax)); di = uint16_t(di + delta); return false;
		case 0xAC: if (wide) ax = rw(srcSeg, si); else set8(0, rb(srcSeg, si)); si = uint16_t(si + delta); return false;
		default: { const uint16_t b = wide ? rw(es, di) : rb(es, di); alu(7, wide ? ax : uint8_t(ax), b, wide); di = uint16_t(di + delta); return true; }
		}
	};
	if (rep == 0) { once(); return; }
	while (cx != 0) {
		const bool cmp = once();
		--cx;
		if (cmp) {
			if (rep == 1 && !flag(ZF)) break;   // repe
			if (rep == 2 && flag(ZF)) break;    // repne
		}
	}
}

void Cpu::interrupt(int n, bool) {
	push(flags);
	push(cs);
	push(ip);
	setFlag(IF, false);
	setFlag(TF, false);
	ip = rw(0, uint16_t(n * 4));
	cs = rw(0, uint16_t(n * 4 + 2));
}

bool Cpu::step() {
	if (halted || !fault.empty()) return false;
	if (onStep) onStep(*this);
	segOverride = -1;
	int rep = 0;
	const uint16_t startIp = ip, startCs = cs;
	uint8_t op = fetch8();
	for (;;) {   // prefixes
		if (op == 0x26) segOverride = 0;
		else if (op == 0x2E) segOverride = 1;
		else if (op == 0x36) segOverride = 2;
		else if (op == 0x3E) segOverride = 3;
		else if (op == 0xF3) rep = 1;
		else if (op == 0xF2) rep = 2;
		else if (op == 0xF0) {}
		else break;
		op = fetch8();
	}
	++steps;
	auto bad = [&]() {
		char buf[96];
		std::snprintf(buf, sizeof buf, "unsupported opcode %02X at %04X:%04X", op, startCs, startIp);
		fault = buf;
		return false;
	};

	if (op < 0x40 && (op & 7) < 6) {   // ALU
		const int aluOp = op >> 3;
		switch (op & 7) {
		case 0: { const ModRM m = decode(fetch8()); const uint16_t r = alu(aluOp, uint16_t(readRm(m, false)), get8(m.reg), false); if (aluOp != 7) writeRm(m, false, r); return true; }
		case 1: { const ModRM m = decode(fetch8()); const uint16_t r = alu(aluOp, readRm(m, true), reg16(m.reg), true); if (aluOp != 7) writeRm(m, true, r); return true; }
		case 2: { const ModRM m = decode(fetch8()); const uint16_t r = alu(aluOp, get8(m.reg), uint16_t(readRm(m, false)), false); if (aluOp != 7) set8(m.reg, uint8_t(r)); return true; }
		case 3: { const ModRM m = decode(fetch8()); const uint16_t r = alu(aluOp, reg16(m.reg), readRm(m, true), true); if (aluOp != 7) reg16(m.reg) = r; return true; }
		case 4: { const uint16_t r = alu(aluOp, uint8_t(ax), fetch8(), false); if (aluOp != 7) set8(0, uint8_t(r)); return true; }
		default: { const uint16_t r = alu(aluOp, ax, fetch16(), true); if (aluOp != 7) ax = r; return true; }
		}
	}
	switch (op) {
	case 0x06: push(es); return true;
	case 0x07: es = pop(); return true;
	case 0x0E: push(cs); return true;
	case 0x16: push(ss); return true;
	case 0x17: ss = pop(); return true;
	case 0x1E: push(ds); return true;
	case 0x1F: ds = pop(); return true;
	case 0x27: case 0x2F: case 0x37: case 0x3F: return bad();   // BCD adjust, not used
	default: break;
	}
	if (op >= 0x40 && op <= 0x47) {
		const bool c = flag(CF);
		uint16_t &r = reg16(op & 7);
		const uint16_t a = r;
		r = alu(0, a, 1, true);
		setFlag(CF, c);
		return true;
	}
	if (op >= 0x48 && op <= 0x4F) {
		const bool c = flag(CF);
		uint16_t &r = reg16(op & 7);
		const uint16_t a = r;
		r = alu(5, a, 1, true);
		setFlag(CF, c);
		return true;
	}
	if (op >= 0x50 && op <= 0x57) { const uint16_t v = (op & 7) == 4 ? uint16_t(sp - 2) : reg16(op & 7); push(v); return true; }
	if (op >= 0x58 && op <= 0x5F) { const uint16_t v = pop(); if ((op & 7) != 4) reg16(op & 7) = v; else sp = v; return true; }
	if (op >= 0x70 && op <= 0x7F) { const int8_t d = int8_t(fetch8()); if (cond(op & 15)) ip = uint16_t(ip + d); return true; }
	if (op >= 0x91 && op <= 0x97) { uint16_t &r = reg16(op & 7); std::swap(r, ax); return true; }
	if (op >= 0xB0 && op <= 0xB7) { set8(op & 7, fetch8()); return true; }
	if (op >= 0xB8 && op <= 0xBF) { reg16(op & 7) = fetch16(); return true; }

	switch (op) {
	case 0x80: case 0x81: case 0x82: case 0x83: {
		const bool wide = op & 1;
		const ModRM m = decode(fetch8());
		uint16_t imm = wide && op == 0x81 ? fetch16() : fetch8();
		if (op == 0x83) imm = uint16_t(int16_t(int8_t(imm)));
		const uint16_t r = alu(m.reg, readRm(m, wide), imm, wide);
		if (m.reg != 7) writeRm(m, wide, r);
		return true;
	}
	case 0x84: case 0x85: {
		const bool wide = op & 1;
		const ModRM m = decode(fetch8());
		alu(4, readRm(m, wide), wide ? reg16(m.reg) : get8(m.reg), wide);
		return true;
	}
	case 0x86: case 0x87: {
		const bool wide = op & 1;
		const ModRM m = decode(fetch8());
		const uint16_t a = readRm(m, wide), b = wide ? reg16(m.reg) : get8(m.reg);
		writeRm(m, wide, b);
		if (wide) reg16(m.reg) = a; else set8(m.reg, uint8_t(a));
		return true;
	}
	case 0x88: { const ModRM m = decode(fetch8()); writeRm(m, false, get8(m.reg)); return true; }
	case 0x89: { const ModRM m = decode(fetch8()); writeRm(m, true, reg16(m.reg)); return true; }
	case 0x8A: { const ModRM m = decode(fetch8()); set8(m.reg, uint8_t(readRm(m, false))); return true; }
	case 0x8B: { const ModRM m = decode(fetch8()); reg16(m.reg) = readRm(m, true); return true; }
	case 0x8C: { const ModRM m = decode(fetch8()); writeRm(m, true, sreg(m.reg)); return true; }
	case 0x8D: { const ModRM m = decode(fetch8()); reg16(m.reg) = m.off; return true; }
	case 0x8E: { const ModRM m = decode(fetch8()); sreg(m.reg) = readRm(m, true); return true; }
	case 0x8F: { const ModRM m = decode(fetch8()); const uint16_t v = pop(); writeRm(m, true, v); return true; }
	case 0x90: return true;
	case 0x98: ax = uint16_t(int16_t(int8_t(uint8_t(ax)))); return true;
	case 0x99: dx = (ax & 0x8000) ? 0xFFFF : 0; return true;
	case 0x9A: { const uint16_t o = fetch16(), s = fetch16(); push(cs); push(ip); cs = s; ip = o; return true; }
	case 0x9B: return true;
	case 0x9C: push(uint16_t(flags | 0xF002)); return true;
	case 0x9D: flags = uint16_t((pop() & 0x0FD5) | 0x0002); return true;
	case 0x9E: { const uint8_t ah = uint8_t(ax >> 8); flags = uint16_t((flags & 0xFF00) | (ah & 0xD5) | 2); return true; }
	case 0x9F: ax = uint16_t((ax & 0x00FF) | ((flags & 0xD7) | 2) << 8); return true;
	case 0xA0: { const uint16_t o = fetch16(); set8(0, rb(segFor(ds), o)); return true; }
	case 0xA1: { const uint16_t o = fetch16(); ax = rw(segFor(ds), o); return true; }
	case 0xA2: { const uint16_t o = fetch16(); wb(segFor(ds), o, uint8_t(ax)); return true; }
	case 0xA3: { const uint16_t o = fetch16(); ww(segFor(ds), o, ax); return true; }
	case 0xA4: case 0xA6: case 0xAA: case 0xAC: case 0xAE: stringOp(op, false, rep); return true;
	case 0xA5: case 0xA7: case 0xAB: case 0xAD: case 0xAF: stringOp(uint8_t(op - 1), true, rep); return true;
	case 0xA8: alu(4, uint8_t(ax), fetch8(), false); return true;
	case 0xA9: alu(4, ax, fetch16(), true); return true;
	case 0xC2: { const uint16_t n = fetch16(); ip = pop(); sp = uint16_t(sp + n); return true; }
	case 0xC3: ip = pop(); return true;
	case 0xC4: case 0xC5: {
		const ModRM m = decode(fetch8());
		reg16(m.reg) = rw(m.seg, m.off);
		const uint16_t s = rw(m.seg, uint16_t(m.off + 2));
		if (op == 0xC4) es = s; else ds = s;
		return true;
	}
	case 0xC6: { const ModRM m = decode(fetch8()); writeRm(m, false, fetch8()); return true; }
	case 0xC7: { const ModRM m = decode(fetch8()); writeRm(m, true, fetch16()); return true; }
	case 0xCA: { const uint16_t n = fetch16(); ip = pop(); cs = pop(); sp = uint16_t(sp + n); return true; }
	case 0xCB: ip = pop(); cs = pop(); return true;
	case 0xCC: if (onInt && onInt(*this, 3)) return true; interrupt(3); return true;
	case 0xCD: { const int n = fetch8(); if (onInt && onInt(*this, n)) return true; interrupt(n); return true; }
	case 0xCF: ip = pop(); cs = pop(); flags = uint16_t((pop() & 0x0FD5) | 2); return true;
	case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
		const bool wide = op & 1;
		const ModRM m = decode(fetch8());
		const int count = (op & 2) ? int(uint8_t(cx)) : 1;
		const uint16_t r = shiftRot(m.reg, readRm(m, wide), count, wide);
		writeRm(m, wide, r);
		return true;
	}
	case 0xD7: set8(0, rb(segFor(ds), uint16_t(bx + uint8_t(ax)))); return true;
	case 0xE0: case 0xE1: case 0xE2: case 0xE3: {
		const int8_t d = int8_t(fetch8());
		bool take;
		if (op == 0xE3) take = cx == 0;
		else {
			--cx;
			take = cx != 0;
			if (op == 0xE1) take = take && flag(ZF);
			if (op == 0xE0) take = take && !flag(ZF);
		}
		if (take) ip = uint16_t(ip + d);
		return true;
	}
	case 0xE4: { const uint16_t p = fetch8(); set8(0, uint8_t(onIn ? onIn(*this, p, false) : 0)); return true; }
	case 0xE5: { const uint16_t p = fetch8(); ax = onIn ? onIn(*this, p, true) : 0; return true; }
	case 0xE6: { const uint16_t p = fetch8(); if (onOut) onOut(*this, p, uint8_t(ax), false); return true; }
	case 0xE7: { const uint16_t p = fetch8(); if (onOut) onOut(*this, p, ax, true); return true; }
	case 0xE8: { const uint16_t d = fetch16(); push(ip); ip = uint16_t(ip + d); return true; }
	case 0xE9: { const uint16_t d = fetch16(); ip = uint16_t(ip + d); return true; }
	case 0xEA: { const uint16_t o = fetch16(), s = fetch16(); cs = s; ip = o; return true; }
	case 0xEB: { const int8_t d = int8_t(fetch8()); ip = uint16_t(ip + d); return true; }
	case 0xEC: set8(0, uint8_t(onIn ? onIn(*this, dx, false) : 0)); return true;
	case 0xED: ax = onIn ? onIn(*this, dx, true) : 0; return true;
	case 0xEE: if (onOut) onOut(*this, dx, uint8_t(ax), false); return true;
	case 0xEF: if (onOut) onOut(*this, dx, ax, true); return true;
	case 0xF4: halted = true; return false;
	case 0xF5: setFlag(CF, !flag(CF)); return true;
	case 0xF6: case 0xF7: {
		const bool wide = op & 1;
		const ModRM m = decode(fetch8());
		switch (m.reg) {
		case 0: case 1: alu(4, readRm(m, wide), wide ? fetch16() : fetch8(), wide); return true;
		case 2: writeRm(m, wide, uint16_t(~readRm(m, wide))); return true;
		case 3: { const uint16_t v = readRm(m, wide); writeRm(m, wide, alu(5, 0, v, wide)); setFlag(CF, v != 0); return true; }
		case 4: {
			const uint32_t r = wide ? uint32_t(ax) * readRm(m, true) : uint32_t(uint8_t(ax)) * readRm(m, false);
			if (wide) { ax = uint16_t(r); dx = uint16_t(r >> 16); setFlag(CF, dx != 0); } else { ax = uint16_t(r); setFlag(CF, (r >> 8) != 0); }
			setFlag(OF, flag(CF));
			return true;
		}
		case 5: {
			if (wide) { const int32_t r = int32_t(int16_t(ax)) * int16_t(readRm(m, true)); ax = uint16_t(r); dx = uint16_t(uint32_t(r) >> 16); setFlag(CF, r != int16_t(r)); }
			else { const int32_t r = int32_t(int8_t(uint8_t(ax))) * int8_t(readRm(m, false)); ax = uint16_t(r); setFlag(CF, r != int8_t(r)); }
			setFlag(OF, flag(CF));
			return true;
		}
		case 6: {
			if (wide) {
				const uint32_t d = readRm(m, true), n = (uint32_t(dx) << 16) | ax;
				if (d == 0 || n / d > 0xFFFF) { fault = "divide error"; return false; }
				ax = uint16_t(n / d); dx = uint16_t(n % d);
			} else {
				const uint32_t d = readRm(m, false), n = ax;
				if (d == 0 || n / d > 0xFF) { fault = "divide error"; return false; }
				ax = uint16_t(((n % d) << 8) | (n / d));
			}
			return true;
		}
		default: {
			if (wide) {
				const int32_t d = int16_t(readRm(m, true)), n = int32_t((uint32_t(dx) << 16) | ax);
				if (d == 0) { fault = "divide error"; return false; }
				ax = uint16_t(n / d); dx = uint16_t(n % d);
			} else {
				const int32_t d = int8_t(readRm(m, false)), n = int16_t(ax);
				if (d == 0) { fault = "divide error"; return false; }
				ax = uint16_t((uint8_t(n % d) << 8) | uint8_t(n / d));
			}
			return true;
		}
		}
	}
	case 0xF8: setFlag(CF, false); return true;
	case 0xF9: setFlag(CF, true); return true;
	case 0xFA: setFlag(IF, false); return true;
	case 0xFB: setFlag(IF, true); return true;
	case 0xFC: setFlag(DF, false); return true;
	case 0xFD: setFlag(DF, true); return true;
	case 0xFE: {
		const ModRM m = decode(fetch8());
		const bool c = flag(CF);
		const uint16_t v = readRm(m, false);
		writeRm(m, false, alu(m.reg == 0 ? 0 : 5, v, 1, false));
		setFlag(CF, c);
		return true;
	}
	case 0xFF: {
		const ModRM m = decode(fetch8());
		switch (m.reg) {
		case 0: case 1: { const bool c = flag(CF); const uint16_t v = readRm(m, true); writeRm(m, true, alu(m.reg == 0 ? 0 : 5, v, 1, true)); setFlag(CF, c); return true; }
		case 2: { const uint16_t t = readRm(m, true); push(ip); ip = t; return true; }
		case 3: { const uint16_t o = rw(m.seg, m.off), s = rw(m.seg, uint16_t(m.off + 2)); push(cs); push(ip); cs = s; ip = o; return true; }
		case 4: ip = readRm(m, true); return true;
		case 5: { const uint16_t o = rw(m.seg, m.off), s = rw(m.seg, uint16_t(m.off + 2)); cs = s; ip = o; return true; }
		case 6: { const uint16_t v = readRm(m, true); push(v); return true; }
		default: return bad();
		}
	}
	default: return bad();
	}
}

} // namespace x86
