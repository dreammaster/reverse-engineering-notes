#include "Cpu86.h"

#include <stdio.h>
#include <string.h>

namespace Yemu {

Cpu::Cpu(uint8_t *mem1M, Bus *b)
	: ip(0), flags(0x0002), instructions(0), halted(false), irqInhibit(false), mem(mem1M), bus(b),
	  segOverride(-1), repPrefix(0), modrm(0), eaSeg(0), eaOff(0), eaIsReg(false), eaReg(0) {
	memset(r, 0, sizeof(r));
	memset(s, 0, sizeof(s));
}

// ---------------------------------------------------------------------------------------------
// memory
// ---------------------------------------------------------------------------------------------

uint8_t Cpu::rb(uint16_t seg, uint16_t off) {
	const uint32_t a = lin(seg, off);
	if ((a >> 16) == 0xA)
		return bus->vramRead(a & 0xFFFF);
	return mem[a];
}

uint16_t Cpu::rw(uint16_t seg, uint16_t off) {
	return (uint16_t)(rb(seg, off) | (rb(seg, (uint16_t)(off + 1)) << 8));
}

void Cpu::wb(uint16_t seg, uint16_t off, uint8_t v) {
	const uint32_t a = lin(seg, off);
	if ((a >> 16) == 0xA)
		bus->vramWrite(a & 0xFFFF, v);
	else
		mem[a] = v;
}

void Cpu::ww(uint16_t seg, uint16_t off, uint16_t v) {
	wb(seg, off, (uint8_t)v);
	wb(seg, (uint16_t)(off + 1), (uint8_t)(v >> 8));
}

void Cpu::push(uint16_t v) {
	r[SP] = (uint16_t)(r[SP] - 2);
	ww(s[SS], r[SP], v);
}

uint16_t Cpu::pop() {
	const uint16_t v = rw(s[SS], r[SP]);
	r[SP] = (uint16_t)(r[SP] + 2);
	return v;
}

void Cpu::interrupt(uint8_t n) {
	push(flags);
	flags &= (uint16_t)~(IF | TF);
	push(s[CS]);
	push(ip);
	ip = rw(0, (uint16_t)(n * 4));
	s[CS] = rw(0, (uint16_t)(n * 4 + 2));
}

uint8_t Cpu::fetch8() {
	return rb(s[CS], ip++);
}

uint16_t Cpu::fetch16() {
	const uint16_t lo = fetch8();
	return (uint16_t)(lo | (fetch8() << 8));
}

uint8_t Cpu::getReg8(int i) const {
	return i < 4 ? (uint8_t)r[i] : (uint8_t)(r[i - 4] >> 8);
}

void Cpu::setReg8(int i, uint8_t v) {
	if (i < 4)
		r[i] = (uint16_t)((r[i] & 0xFF00) | v);
	else
		r[i - 4] = (uint16_t)((r[i - 4] & 0x00FF) | (v << 8));
}

void Cpu::decodeModrm(bool) {
	modrm = fetch8();
	const int mod = modrm >> 6, rm = modrm & 7;
	if (mod == 3) {
		eaIsReg = true;
		eaReg = rm;
		return;
	}
	eaIsReg = false;
	int dseg = DS;
	uint16_t off = 0;
	switch (rm) {
	case 0: off = (uint16_t)(r[BX] + r[SI]); break;
	case 1: off = (uint16_t)(r[BX] + r[DI]); break;
	case 2: off = (uint16_t)(r[BP] + r[SI]); dseg = SS; break;
	case 3: off = (uint16_t)(r[BP] + r[DI]); dseg = SS; break;
	case 4: off = r[SI]; break;
	case 5: off = r[DI]; break;
	case 6:
		if (mod == 0) { off = fetch16(); }
		else { off = r[BP]; dseg = SS; }
		break;
	case 7: off = r[BX]; break;
	}
	if (mod == 1)
		off = (uint16_t)(off + (int16_t)(int8_t)fetch8());
	else if (mod == 2)
		off = (uint16_t)(off + fetch16());
	eaSeg = s[segFor(dseg)];
	eaOff = off;
}

uint8_t Cpu::getRm8() { return eaIsReg ? getReg8(eaReg) : rb(eaSeg, eaOff); }
uint16_t Cpu::getRm16() { return eaIsReg ? r[eaReg] : rw(eaSeg, eaOff); }
void Cpu::setRm8(uint8_t v) { if (eaIsReg) setReg8(eaReg, v); else wb(eaSeg, eaOff, v); }
void Cpu::setRm16(uint16_t v) { if (eaIsReg) r[eaReg] = v; else ww(eaSeg, eaOff, v); }

// ---------------------------------------------------------------------------------------------
// flags and ALU
// ---------------------------------------------------------------------------------------------

bool Cpu::parity(uint8_t v) const {
	v ^= v >> 4;
	v ^= v >> 2;
	v ^= v >> 1;
	return !(v & 1);
}

void Cpu::setSzp(uint16_t v, bool word) {
	const uint16_t mask = word ? 0xFFFF : 0xFF;
	const uint16_t sign = word ? 0x8000 : 0x80;
	v &= mask;
	flags = (uint16_t)((flags & ~(ZF | SF | PF)) | (v == 0 ? ZF : 0) | ((v & sign) ? SF : 0) | (parity((uint8_t)v) ? PF : 0));
}

uint16_t Cpu::alu(int op, uint16_t a, uint16_t b, bool word) {
	const uint32_t mask = word ? 0xFFFF : 0xFF;
	const uint32_t sign = word ? 0x8000 : 0x80;
	uint32_t res;
	const uint32_t c = (flags & CF) ? 1 : 0;
	switch (op) {
	case 0: case 2: {   // ADD, ADC
		const uint32_t cin = op == 2 ? c : 0;
		res = (uint32_t)a + b + cin;
		flags = (uint16_t)((flags & ~(CF | OF | AF)) | (res > mask ? CF : 0) | ((((a ^ res) & (b ^ res)) & sign) ? OF : 0) | (((a ^ b ^ res) & 0x10) ? AF : 0));
		break;
	}
	case 3: case 5: case 7: {   // SBB, SUB, CMP
		const uint32_t cin = op == 3 ? c : 0;
		res = (uint32_t)a - b - cin;
		flags = (uint16_t)((flags & ~(CF | OF | AF)) | ((uint32_t)a < (uint32_t)b + cin ? CF : 0) | ((((a ^ b) & (a ^ res)) & sign) ? OF : 0) | (((a ^ b ^ res) & 0x10) ? AF : 0));
		break;
	}
	case 1: res = a | b; flags &= (uint16_t)~(CF | OF | AF); break;
	case 4: res = a & b; flags &= (uint16_t)~(CF | OF | AF); break;
	default: res = a ^ b; flags &= (uint16_t)~(CF | OF | AF); break;   // 6 = XOR
	}
	setSzp((uint16_t)res, word);
	return (uint16_t)(res & mask);
}

uint16_t Cpu::shiftOp(int op, uint16_t v, int count, bool word) {
	const int bits = word ? 16 : 8;
	const uint32_t mask = word ? 0xFFFF : 0xFF;
	const uint32_t sign = word ? 0x8000 : 0x80;
	uint32_t x = v & mask;
	for (int i = 0; i < count; i++) {
		const uint32_t oldCf = (flags & CF) ? 1 : 0;
		uint32_t cf = 0;
		switch (op) {
		case 0: cf = (x & sign) ? 1 : 0; x = ((x << 1) | cf) & mask; break;                 // ROL
		case 1: cf = x & 1; x = (x >> 1) | (cf ? sign : 0); break;                          // ROR
		case 2: cf = (x & sign) ? 1 : 0; x = ((x << 1) | oldCf) & mask; break;              // RCL
		case 3: cf = x & 1; x = (x >> 1) | (oldCf ? sign : 0); break;                       // RCR
		case 4: case 6: cf = (x & sign) ? 1 : 0; x = (x << 1) & mask; break;                // SHL
		case 5: cf = x & 1; x >>= 1; break;                                                  // SHR
		default: cf = x & 1; x = (x >> 1) | (x & sign); break;                              // SAR
		}
		flags = (uint16_t)((flags & ~CF) | (cf ? CF : 0));
		if (i == count - 1) {
			// OF: defined for single-bit shifts; computed the same way for every count here
			uint32_t of;
			if (op == 0 || op == 2 || op == 4 || op == 6) of = ((x & sign) ? 1 : 0) ^ cf;
			else if (op == 1 || op == 3) of = ((x & sign) ? 1 : 0) ^ ((x & (sign >> 1)) ? 1 : 0);
			else if (op == 5) of = (v & sign) ? 1 : 0;
			else of = 0;
			flags = (uint16_t)((flags & ~OF) | (of ? OF : 0));
		}
	}
	if (count > 0 && op >= 4)
		setSzp((uint16_t)x, word);
	(void)bits;
	return (uint16_t)x;
}

bool Cpu::condition(int cc) const {
	const bool cf = flags & CF, zf = flags & ZF, sf = flags & SF, of = flags & OF, pf = flags & PF;
	switch (cc >> 1) {
	case 0: return cc & 1 ? !of : of;
	case 1: return cc & 1 ? !cf : cf;
	case 2: return cc & 1 ? !zf : zf;
	case 3: return cc & 1 ? !(cf || zf) : (cf || zf);
	case 4: return cc & 1 ? !sf : sf;
	case 5: return cc & 1 ? !pf : pf;
	case 6: return cc & 1 ? (sf == of) : (sf != of);
	default: return cc & 1 ? (!zf && sf == of) : (zf || sf != of);
	}
}

void Cpu::fail(const char *what, uint8_t op) {
	char buf[96];
	snprintf(buf, sizeof(buf), "%s opcode %02X at %04X:%04X", what, op, s[CS], (uint16_t)(ip - 1));
	fault = buf;
	halted = true;
}

// ---------------------------------------------------------------------------------------------
// string instructions
// ---------------------------------------------------------------------------------------------

void Cpu::stringOp(uint8_t op) {
	const bool word = op & 1;
	const int step = (flags & DF) ? -(word ? 2 : 1) : (word ? 2 : 1);
	const int srcSeg = segFor(DS);
	const bool isCmp = op == 0xA6 || op == 0xA7 || op == 0xAE || op == 0xAF;
	for (;;) {
		if (repPrefix && r[CX] == 0)
			return;
		switch (op & 0xFE) {
		case 0xA4:   // MOVS
			if (word) ww(s[ES], r[DI], rw(s[srcSeg], r[SI])); else wb(s[ES], r[DI], rb(s[srcSeg], r[SI]));
			r[SI] = (uint16_t)(r[SI] + step); r[DI] = (uint16_t)(r[DI] + step);
			break;
		case 0xA6: { // CMPS
			const uint16_t a = word ? rw(s[srcSeg], r[SI]) : rb(s[srcSeg], r[SI]);
			const uint16_t b = word ? rw(s[ES], r[DI]) : rb(s[ES], r[DI]);
			alu(7, a, b, word);
			r[SI] = (uint16_t)(r[SI] + step); r[DI] = (uint16_t)(r[DI] + step);
			break;
		}
		case 0xAA:   // STOS
			if (word) ww(s[ES], r[DI], r[AX]); else wb(s[ES], r[DI], (uint8_t)r[AX]);
			r[DI] = (uint16_t)(r[DI] + step);
			break;
		case 0xAC:   // LODS
			if (word) r[AX] = rw(s[srcSeg], r[SI]); else setReg8(0, rb(s[srcSeg], r[SI]));
			r[SI] = (uint16_t)(r[SI] + step);
			break;
		case 0xAE: { // SCAS
			const uint16_t b = word ? rw(s[ES], r[DI]) : rb(s[ES], r[DI]);
			alu(7, word ? r[AX] : (uint8_t)r[AX], b, word);
			r[DI] = (uint16_t)(r[DI] + step);
			break;
		}
		}
		instructions++;
		if (!repPrefix)
			return;
		r[CX]--;
		if (isCmp) {
			const bool zf = flags & ZF;
			if (repPrefix == 0xF3 && !zf) return;
			if (repPrefix == 0xF2 && zf) return;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// the instruction decoder
// ---------------------------------------------------------------------------------------------

bool Cpu::step() {
	if (halted)
		return false;
	irqInhibit = false;
	if (s[CS] == 0xF000) {
		bus->hleStub(*this, ip);
		return !halted;
	}
	segOverride = -1;
	repPrefix = 0;
	uint8_t op;
	for (;;) {
		op = fetch8();
		if (op == 0x26) segOverride = ES;
		else if (op == 0x2E) segOverride = CS;
		else if (op == 0x36) segOverride = SS;
		else if (op == 0x3E) segOverride = DS;
		else if (op == 0xF2 || op == 0xF3) repPrefix = op;
		else if (op == 0xF0) {}
		else break;
	}
	instructions++;

	// ALU block 00-3F
	if (op < 0x40 && (op & 7) < 6) {
		const int aop = op >> 3;
		const int form = op & 7;
		if (form < 4) {
			const bool word = form & 1;
			decodeModrm(word);
			const int reg = (modrm >> 3) & 7;
			if (form < 2) {   // rm <- rm op reg
				if (word) { const uint16_t res = alu(aop, getRm16(), r[reg], true); if (aop != 7) setRm16(res); }
				else { const uint16_t res = alu(aop, getRm8(), getReg8(reg), false); if (aop != 7) setRm8((uint8_t)res); }
			} else {          // reg <- reg op rm
				if (word) { const uint16_t res = alu(aop, r[reg], getRm16(), true); if (aop != 7) r[reg] = res; }
				else { const uint16_t res = alu(aop, getReg8(reg), getRm8(), false); if (aop != 7) setReg8(reg, (uint8_t)res); }
			}
		} else if (form == 4) {
			const uint16_t res = alu(aop, (uint8_t)r[AX], fetch8(), false);
			if (aop != 7) setReg8(0, (uint8_t)res);
		} else {
			const uint16_t res = alu(aop, r[AX], fetch16(), true);
			if (aop != 7) r[AX] = res;
		}
		return true;
	}

	switch (op) {
	case 0x06: push(s[ES]); return true;
	case 0x07: s[ES] = pop(); return true;
	case 0x0E: push(s[CS]); return true;
	case 0x16: push(s[SS]); return true;
	case 0x17: s[SS] = pop(); irqInhibit = true; return true;
	case 0x1E: push(s[DS]); return true;
	case 0x1F: s[DS] = pop(); return true;
	case 0x27: {   // DAA
		uint8_t al = (uint8_t)r[AX];
		const uint8_t old = al;
		const bool oldCf = flags & CF;
		flags &= (uint16_t)~CF;
		if ((al & 0x0F) > 9 || (flags & AF)) { al = (uint8_t)(al + 6); flags |= AF; if (oldCf || al < old) flags |= CF; }
		if (old > 0x99 || oldCf) { al = (uint8_t)(al + 0x60); flags |= CF; }
		setReg8(0, al);
		setSzp(al, false);
		return true;
	}
	case 0x2F: {   // DAS
		uint8_t al = (uint8_t)r[AX];
		const uint8_t old = al;
		const bool oldCf = flags & CF;
		flags &= (uint16_t)~CF;
		if ((al & 0x0F) > 9 || (flags & AF)) { al = (uint8_t)(al - 6); flags |= AF; if (oldCf || old < 6) flags |= CF; }
		if (old > 0x99 || oldCf) { al = (uint8_t)(al - 0x60); flags |= CF; }
		setReg8(0, al);
		setSzp(al, false);
		return true;
	}
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: {
		const uint16_t cf = flags & CF;
		r[op & 7] = alu(0, r[op & 7], 1, true);
		flags = (uint16_t)((flags & ~CF) | cf);
		return true;
	}
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: {
		const uint16_t cf = flags & CF;
		r[op & 7] = alu(5, r[op & 7], 1, true);
		flags = (uint16_t)((flags & ~CF) | cf);
		return true;
	}
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		if ((op & 7) == SP) { r[SP] = (uint16_t)(r[SP] - 2); ww(s[SS], r[SP], r[SP]); }   // 8086: pushes the decremented SP
		else push(r[op & 7]);
		return true;
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		r[op & 7] = pop();
		return true;
	case 0x60: { const uint16_t sp = r[SP]; push(r[AX]); push(r[CX]); push(r[DX]); push(r[BX]); push(sp); push(r[BP]); push(r[SI]); push(r[DI]); return true; }
	case 0x61: r[DI] = pop(); r[SI] = pop(); r[BP] = pop(); pop(); r[BX] = pop(); r[DX] = pop(); r[CX] = pop(); r[AX] = pop(); return true;
	case 0x68: push(fetch16()); return true;
	case 0x6A: push((uint16_t)(int16_t)(int8_t)fetch8()); return true;
	case 0x69: case 0x6B: {
		decodeModrm(true);
		const int reg = (modrm >> 3) & 7;
		const int32_t a = (int16_t)getRm16();
		const int32_t b = op == 0x69 ? (int16_t)fetch16() : (int16_t)(int8_t)fetch8();
		const int32_t p = a * b;
		r[reg] = (uint16_t)p;
		flags = (uint16_t)((flags & ~(CF | OF)) | (p != (int16_t)p ? (CF | OF) : 0));
		return true;
	}
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: {
		const int8_t d = (int8_t)fetch8();
		if (condition(op & 15))
			ip = (uint16_t)(ip + d);
		return true;
	}
	case 0x80: case 0x81: case 0x82: case 0x83: {
		const bool word = op & 1;
		decodeModrm(word);
		const int aop = (modrm >> 3) & 7;
		if (word) {
			const uint16_t imm = op == 0x81 ? fetch16() : (uint16_t)(int16_t)(int8_t)fetch8();
			const uint16_t res = alu(aop, getRm16(), imm, true);
			if (aop != 7) setRm16(res);
		} else {
			const uint8_t imm = fetch8();
			const uint16_t res = alu(aop, getRm8(), imm, false);
			if (aop != 7) setRm8((uint8_t)res);
		}
		return true;
	}
	case 0x84: case 0x85: {
		const bool word = op & 1;
		decodeModrm(word);
		const int reg = (modrm >> 3) & 7;
		if (word) alu(4, getRm16(), r[reg], true); else alu(4, getRm8(), getReg8(reg), false);
		return true;
	}
	case 0x86: case 0x87: {
		const bool word = op & 1;
		decodeModrm(word);
		const int reg = (modrm >> 3) & 7;
		if (word) { const uint16_t t = getRm16(); setRm16(r[reg]); r[reg] = t; }
		else { const uint8_t t = getRm8(); setRm8(getReg8(reg)); setReg8(reg, t); }
		return true;
	}
	case 0x88: decodeModrm(false); setRm8(getReg8((modrm >> 3) & 7)); return true;
	case 0x89: decodeModrm(true); setRm16(r[(modrm >> 3) & 7]); return true;
	case 0x8A: decodeModrm(false); setReg8((modrm >> 3) & 7, getRm8()); return true;
	case 0x8B: decodeModrm(true); r[(modrm >> 3) & 7] = getRm16(); return true;
	case 0x8C: decodeModrm(true); setRm16(s[(modrm >> 3) & 3]); return true;
	case 0x8D: decodeModrm(true); r[(modrm >> 3) & 7] = eaOff; return true;
	case 0x8E: decodeModrm(true); s[(modrm >> 3) & 3] = getRm16(); if (((modrm >> 3) & 3) == SS) irqInhibit = true; return true;
	case 0x8F: decodeModrm(true); { const uint16_t v = pop(); setRm16(v); } return true;
	case 0x90: return true;
	case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: {
		const uint16_t t = r[AX]; r[AX] = r[op & 7]; r[op & 7] = t; return true;
	}
	case 0x98: r[AX] = (uint16_t)(int16_t)(int8_t)r[AX]; return true;
	case 0x99: r[DX] = (r[AX] & 0x8000) ? 0xFFFF : 0; return true;
	case 0x9A: { const uint16_t off = fetch16(), seg = fetch16(); push(s[CS]); push(ip); s[CS] = seg; ip = off; return true; }
	case 0x9B: return true;
	case 0x9C: push(flags); return true;
	case 0x9D: flags = (uint16_t)(pop() | 0x0002); return true;
	case 0x9E: flags = (uint16_t)((flags & 0xFF00) | (r[AX] >> 8) | 0x0002); return true;
	case 0x9F: setReg8(4, (uint8_t)flags); return true;
	case 0xA0: { const uint16_t off = fetch16(); setReg8(0, rb(s[segFor(DS)], off)); return true; }
	case 0xA1: { const uint16_t off = fetch16(); r[AX] = rw(s[segFor(DS)], off); return true; }
	case 0xA2: { const uint16_t off = fetch16(); wb(s[segFor(DS)], off, (uint8_t)r[AX]); return true; }
	case 0xA3: { const uint16_t off = fetch16(); ww(s[segFor(DS)], off, r[AX]); return true; }
	case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
		stringOp(op);
		return true;
	case 0xA8: alu(4, (uint8_t)r[AX], fetch8(), false); return true;
	case 0xA9: alu(4, r[AX], fetch16(), true); return true;
	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
		setReg8(op & 7, fetch8());
		return true;
	case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		r[op & 7] = fetch16();
		return true;
	case 0xC0: case 0xC1: case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
		const bool word = op & 1;
		decodeModrm(word);
		const int sop = (modrm >> 3) & 7;
		int count = (op == 0xC0 || op == 0xC1) ? fetch8() : (op < 0xD2 ? 1 : (uint8_t)r[CX]);
		if (word) setRm16(shiftOp(sop, getRm16(), count, true));
		else setRm8((uint8_t)shiftOp(sop, getRm8(), count, false));
		return true;
	}
	case 0xC2: { const uint16_t n = fetch16(); ip = pop(); r[SP] = (uint16_t)(r[SP] + n); return true; }
	case 0xC3: ip = pop(); return true;
	case 0xC4: case 0xC5: {
		decodeModrm(true);
		const int reg = (modrm >> 3) & 7;
		r[reg] = rw(eaSeg, eaOff);
		const uint16_t seg = rw(eaSeg, (uint16_t)(eaOff + 2));
		if (op == 0xC4) s[ES] = seg; else s[DS] = seg;
		return true;
	}
	case 0xC6: decodeModrm(false); setRm8(fetch8()); return true;
	case 0xC7: decodeModrm(true); { const uint16_t v = fetch16(); setRm16(v); } return true;
	case 0xC8: { const uint16_t size = fetch16(); fetch8(); push(r[BP]); r[BP] = r[SP]; r[SP] = (uint16_t)(r[SP] - size); return true; }
	case 0xC9: r[SP] = r[BP]; r[BP] = pop(); return true;
	case 0xCA: { const uint16_t n = fetch16(); ip = pop(); s[CS] = pop(); r[SP] = (uint16_t)(r[SP] + n); return true; }
	case 0xCB: ip = pop(); s[CS] = pop(); return true;
	case 0xCC: if (!bus->softInt(*this, 3)) interrupt(3); return true;
	case 0xCD: { const uint8_t n = fetch8(); if (!bus->softInt(*this, n)) interrupt(n); return true; }
	case 0xCE: if (flags & OF) interrupt(4); return true;
	case 0xCF: ip = pop(); s[CS] = pop(); flags = (uint16_t)(pop() | 0x0002); return true;
	case 0xD4: { const uint8_t base = fetch8(); const uint8_t al = (uint8_t)r[AX]; setReg8(4, (uint8_t)(al / base)); setReg8(0, (uint8_t)(al % base)); setSzp((uint8_t)r[AX], false); return true; }
	case 0xD5: { const uint8_t base = fetch8(); const uint8_t v = (uint8_t)((uint8_t)r[AX] + (uint8_t)(r[AX] >> 8) * base); r[AX] = v; setSzp(v, false); return true; }
	case 0xD7: setReg8(0, rb(s[segFor(DS)], (uint16_t)(r[BX] + (uint8_t)r[AX]))); return true;
	case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
		decodeModrm(true);
		return true;
	case 0xE0: case 0xE1: case 0xE2: {
		const int8_t d = (int8_t)fetch8();
		r[CX]--;
		bool take = r[CX] != 0;
		if (op == 0xE0) take = take && !(flags & ZF);
		if (op == 0xE1) take = take && (flags & ZF);
		if (take) {
			ip = (uint16_t)(ip + d);
			// `loop $` busy wait: finish it at once (the instruction count approximates the time spent)
			if (op == 0xE2 && d == -2) {
				instructions += r[CX];
				r[CX] = 0;
				ip = (uint16_t)(ip + 2);
			}
		}
		return true;
	}
	case 0xE3: { const int8_t d = (int8_t)fetch8(); if (r[CX] == 0) ip = (uint16_t)(ip + d); return true; }
	case 0xE4: { const uint8_t p = fetch8(); setReg8(0, bus->in8(p)); return true; }
	case 0xE5: { const uint8_t p = fetch8(); r[AX] = (uint16_t)(bus->in8(p) | (bus->in8((uint16_t)(p + 1)) << 8)); return true; }
	case 0xE6: { const uint8_t p = fetch8(); bus->out8(p, (uint8_t)r[AX]); return true; }
	case 0xE7: { const uint8_t p = fetch8(); bus->out8(p, (uint8_t)r[AX]); bus->out8((uint16_t)(p + 1), (uint8_t)(r[AX] >> 8)); return true; }
	case 0xE8: { const uint16_t d = fetch16(); push(ip); ip = (uint16_t)(ip + d); return true; }
	case 0xE9: { const uint16_t d = fetch16(); ip = (uint16_t)(ip + d); return true; }
	case 0xEA: { const uint16_t off = fetch16(), seg = fetch16(); ip = off; s[CS] = seg; return true; }
	case 0xEB: { const int8_t d = (int8_t)fetch8(); ip = (uint16_t)(ip + d); return true; }
	case 0xEC: setReg8(0, bus->in8(r[DX])); return true;
	case 0xED: r[AX] = (uint16_t)(bus->in8(r[DX]) | (bus->in8((uint16_t)(r[DX] + 1)) << 8)); return true;
	case 0xEE: bus->out8(r[DX], (uint8_t)r[AX]); return true;
	case 0xEF: bus->out8(r[DX], (uint8_t)r[AX]); bus->out8((uint16_t)(r[DX] + 1), (uint8_t)(r[AX] >> 8)); return true;
	case 0xF4: halted = true; fault = "HLT"; return false;
	case 0xF5: flags ^= CF; return true;
	case 0xF6: case 0xF7: {
		const bool word = op & 1;
		decodeModrm(word);
		const int sub = (modrm >> 3) & 7;
		switch (sub) {
		case 0: case 1: { if (word) alu(4, getRm16(), fetch16(), true); else alu(4, getRm8(), fetch8(), false); return true; }
		case 2: if (word) setRm16((uint16_t)~getRm16()); else setRm8((uint8_t)~getRm8()); return true;
		case 3: {
			if (word) { const uint16_t v = getRm16(); const uint16_t res = alu(5, 0, v, true); setRm16(res); flags = (uint16_t)((flags & ~CF) | (v ? CF : 0)); }
			else { const uint8_t v = getRm8(); const uint16_t res = alu(5, 0, v, false); setRm8((uint8_t)res); flags = (uint16_t)((flags & ~CF) | (v ? CF : 0)); }
			return true;
		}
		case 4: case 5:
			if (word) {
				uint32_t p;
				if (sub == 4) p = (uint32_t)r[AX] * getRm16(); else p = (uint32_t)((int32_t)(int16_t)r[AX] * (int32_t)(int16_t)getRm16());
				r[AX] = (uint16_t)p; r[DX] = (uint16_t)(p >> 16);
				const bool big = sub == 4 ? (r[DX] != 0) : ((int32_t)p != (int16_t)p);
				flags = (uint16_t)((flags & ~(CF | OF)) | (big ? (CF | OF) : 0));
			} else {
				uint16_t p;
				if (sub == 4) p = (uint16_t)((uint8_t)r[AX] * getRm8()); else p = (uint16_t)((int16_t)(int8_t)r[AX] * (int16_t)(int8_t)getRm8());
				r[AX] = p;
				const bool big = sub == 4 ? ((p >> 8) != 0) : ((int16_t)p != (int8_t)p);
				flags = (uint16_t)((flags & ~(CF | OF)) | (big ? (CF | OF) : 0));
			}
			return true;
		default: {
			if (word) {
				const uint32_t num = ((uint32_t)r[DX] << 16) | r[AX];
				const uint16_t div = getRm16();
				if (sub == 6) {
					if (div == 0 || num / div > 0xFFFF) { interrupt(0); return true; }
					r[AX] = (uint16_t)(num / div); r[DX] = (uint16_t)(num % div);
				} else {
					const int32_t n = (int32_t)num; const int16_t d = (int16_t)div;
					if (d == 0 || n / d > 32767 || n / d < -32768) { interrupt(0); return true; }
					r[AX] = (uint16_t)(n / d); r[DX] = (uint16_t)(n % d);
				}
			} else {
				const uint16_t num = r[AX];
				const uint8_t div = getRm8();
				if (sub == 6) {
					if (div == 0 || num / div > 0xFF) { interrupt(0); return true; }
					setReg8(0, (uint8_t)(num / div)); setReg8(4, (uint8_t)(num % div));
				} else {
					const int16_t n = (int16_t)num; const int8_t d = (int8_t)div;
					if (d == 0 || n / d > 127 || n / d < -128) { interrupt(0); return true; }
					setReg8(0, (uint8_t)(n / d)); setReg8(4, (uint8_t)(n % d));
				}
			}
			return true;
		}
		}
	}
	case 0xF8: flags &= (uint16_t)~CF; return true;
	case 0xF9: flags |= CF; return true;
	case 0xFA: flags &= (uint16_t)~IF; return true;
	case 0xFB: flags |= IF; return true;
	case 0xFC: flags &= (uint16_t)~DF; return true;
	case 0xFD: flags |= DF; return true;
	case 0xFE: {
		decodeModrm(false);
		const uint16_t cf = flags & CF;
		const int sub = (modrm >> 3) & 7;
		if (sub > 1) { fail("bad FE group", op); return false; }
		setRm8((uint8_t)alu(sub == 0 ? 0 : 5, getRm8(), 1, false));
		flags = (uint16_t)((flags & ~CF) | cf);
		return true;
	}
	case 0xFF: {
		decodeModrm(true);
		const int sub = (modrm >> 3) & 7;
		switch (sub) {
		case 0: case 1: { const uint16_t cf = flags & CF; setRm16(alu(sub == 0 ? 0 : 5, getRm16(), 1, true)); flags = (uint16_t)((flags & ~CF) | cf); return true; }
		case 2: { const uint16_t t = getRm16(); push(ip); ip = t; return true; }
		case 3: { const uint16_t off = rw(eaSeg, eaOff), seg = rw(eaSeg, (uint16_t)(eaOff + 2)); push(s[CS]); push(ip); ip = off; s[CS] = seg; return true; }
		case 4: ip = getRm16(); return true;
		case 5: { const uint16_t off = rw(eaSeg, eaOff), seg = rw(eaSeg, (uint16_t)(eaOff + 2)); ip = off; s[CS] = seg; return true; }
		case 6: { const uint16_t t = getRm16(); push(t); return true; }
		default: fail("bad FF group", op); return false;
		}
	}
	default:
		fail("unsupported", op);
		return false;
	}
}

} // namespace Yemu
