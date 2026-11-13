#include "engine/cpu8086.h"

namespace Ultima6 {

Cpu8086::Cpu8086() : _mem(1 << 20, 0) {
}

uint16_t &Cpu8086::reg16(int i) {
	switch (i & 7) {
	case 0:
		return ax;
	case 1:
		return cx;
	case 2:
		return dx;
	case 3:
		return bx;
	case 4:
		return sp;
	case 5:
		return bp;
	case 6:
		return si;
	default:
		return di;
	}
}

// the 8-bit registers: al cl dl bl ah ch dh bh (the byte of the 16-bit register in the host's order, little endian)
uint8_t &Cpu8086::reg8(int i) {
	uint16_t &r = reg16(i & 3);
	uint8_t *p = reinterpret_cast<uint8_t *>(&r);
	return (i & 4) ? p[1] : p[0];
}

uint16_t &Cpu8086::seg(int i) {
	switch (i & 3) {
	case 0:
		return es;
	case 1:
		return cs;
	case 2:
		return ss;
	default:
		return ds;
	}
}

void Cpu8086::push16(uint16_t v) {
	sp = (uint16_t)(sp - 2);
	write16(ss, sp, v);
}

uint16_t Cpu8086::pop16() {
	uint16_t v = read16(ss, sp);
	sp = (uint16_t)(sp + 2);
	return v;
}

uint8_t Cpu8086::fetch8() {
	uint8_t v = read8(cs, ip);
	ip = (uint16_t)(ip + 1);
	return v;
}

uint16_t Cpu8086::fetch16() {
	uint16_t v = fetch8();
	v = (uint16_t)(v | (fetch8() << 8));
	return v;
}

void Cpu8086::modrm(int &mod, int &reg, int &rm) {
	uint8_t b = fetch8();
	mod = b >> 6;
	reg = (b >> 3) & 7;
	rm = b & 7;
}

Cpu8086::Operand Cpu8086::decodeRM(int mod, int rm) {
	Operand o;
	o.isReg = mod == 3;
	o.reg = rm;
	o.segment = 0;
	o.offset = 0;
	if (o.isReg)
		return o;
	int base = 3; // ds
	uint16_t off = 0;
	switch (rm) {
	case 0:
		off = (uint16_t)(bx + si);
		break;
	case 1:
		off = (uint16_t)(bx + di);
		break;
	case 2:
		off = (uint16_t)(bp + si);
		base = 2;
		break;
	case 3:
		off = (uint16_t)(bp + di);
		base = 2;
		break;
	case 4:
		off = si;
		break;
	case 5:
		off = di;
		break;
	case 6:
		if (mod == 0) {
			off = fetch16();
		} else {
			off = bp;
			base = 2;
		}
		break;
	default:
		off = bx;
		break;
	}
	if (mod == 1)
		off = (uint16_t)(off + (int16_t)(int8_t)fetch8());
	else if (mod == 2)
		off = (uint16_t)(off + fetch16());
	o.segment = seg(_segOverride >= 0 ? _segOverride : base);
	o.offset = off;
	return o;
}

uint16_t Cpu8086::load(const Operand &o, bool wide) {
	if (o.isReg)
		return wide ? reg16(o.reg) : reg8(o.reg);
	return wide ? read16(o.segment, o.offset) : read8(o.segment, o.offset);
}

void Cpu8086::store(const Operand &o, bool wide, uint16_t v) {
	if (o.isReg) {
		if (wide)
			reg16(o.reg) = v;
		else
			reg8(o.reg) = (uint8_t)v;
	} else if (wide) {
		write16(o.segment, o.offset, v);
	} else {
		write8(o.segment, o.offset, (uint8_t)v);
	}
}

void Cpu8086::setSzp(uint16_t r, bool wide) {
	const uint16_t msb = wide ? 0x8000 : 0x80;
	zf = (wide ? r : (r & 0xFF)) == 0;
	sf = (r & msb) != 0;
	uint8_t b = (uint8_t)r;
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	pf = !(b & 1);
}

uint16_t Cpu8086::alu(int op, uint16_t a, uint16_t b, bool wide) {
	const uint32_t mask = wide ? 0xFFFF : 0xFF, msb = wide ? 0x8000 : 0x80;
	uint32_t r = 0;
	switch (op) {
	case 0: // add
	case 2: { // adc
		uint32_t c = (op == 2 && cf) ? 1 : 0;
		r = (uint32_t)a + b + c;
		cf = r > mask;
		of = ((~(a ^ b) & (a ^ r)) & msb) != 0;
		af = ((a ^ b ^ r) & 0x10) != 0;
		break;
	}
	case 3: // sbb
	case 5: // sub
	case 7: { // cmp
		uint32_t c = (op == 3 && cf) ? 1 : 0;
		r = (uint32_t)a - b - c;
		cf = (uint32_t)a < (uint32_t)b + c;
		of = (((a ^ b) & (a ^ r)) & msb) != 0;
		af = ((a ^ b ^ r) & 0x10) != 0;
		break;
	}
	case 1:
		r = a | b;
		cf = of = af = false;
		break;
	case 4:
		r = a & b;
		cf = of = af = false;
		break;
	default: // 6 xor
		r = a ^ b;
		cf = of = af = false;
		break;
	}
	setSzp((uint16_t)r, wide);
	return (uint16_t)(r & mask);
}

uint16_t Cpu8086::shift(int op, uint16_t v, int count, bool wide) {
	const uint32_t mask = wide ? 0xFFFF : 0xFF, msb = wide ? 0x8000 : 0x80;
	uint32_t x = v & mask;
	for (int i = 0; i < count; ++i) {
		switch (op) {
		case 0: // rol
			cf = (x & msb) != 0;
			x = ((x << 1) | (cf ? 1 : 0)) & mask;
			of = ((x & msb) != 0) != cf;
			break;
		case 1: // ror
			cf = (x & 1) != 0;
			x = (x >> 1) | (cf ? msb : 0);
			of = ((x & msb) != 0) != ((x & (msb >> 1)) != 0);
			break;
		case 2: { // rcl
			bool nc = (x & msb) != 0;
			x = ((x << 1) | (cf ? 1 : 0)) & mask;
			cf = nc;
			of = ((x & msb) != 0) != cf;
			break;
		}
		case 3: { // rcr
			of = ((x & msb) != 0) != cf;
			bool nc = (x & 1) != 0;
			x = (x >> 1) | (cf ? msb : 0);
			cf = nc;
			break;
		}
		case 4:
		case 6: // shl
			cf = (x & msb) != 0;
			x = (x << 1) & mask;
			of = ((x & msb) != 0) != cf;
			setSzp((uint16_t)x, wide);
			break;
		case 5: // shr
			cf = (x & 1) != 0;
			of = (x & msb) != 0;
			x >>= 1;
			setSzp((uint16_t)x, wide);
			break;
		default: // 7 sar
			cf = (x & 1) != 0;
			x = (x >> 1) | (x & msb);
			of = false;
			setSzp((uint16_t)x, wide);
			break;
		}
	}
	return (uint16_t)x;
}

bool Cpu8086::condition(int cc) const {
	bool r = false;
	switch (cc >> 1) {
	case 0:
		r = of;
		break;
	case 1:
		r = cf;
		break;
	case 2:
		r = zf;
		break;
	case 3:
		r = cf || zf;
		break;
	case 4:
		r = sf;
		break;
	case 5:
		r = pf;
		break;
	case 6:
		r = sf != of;
		break;
	default:
		r = zf || sf != of;
		break;
	}
	return (cc & 1) ? !r : r;
}

bool Cpu8086::fail(const char *msg) {
	_error = msg;
	return false;
}

bool Cpu8086::step() {
	_segOverride = -1;
	int rep = 0; // 0 none, 1 repe/rep, 2 repne
	uint8_t op;
	for (;;) {
		op = fetch8();
		if (op == 0x26)
			_segOverride = 0;
		else if (op == 0x2E)
			_segOverride = 1;
		else if (op == 0x36)
			_segOverride = 2;
		else if (op == 0x3E)
			_segOverride = 3;
		else if (op == 0xF3)
			rep = 1;
		else if (op == 0xF2)
			rep = 2;
		else if (op != 0xF0)
			break;
	}
	int mod, reg, rm;

	if (op < 0x40 && (op & 7) < 6) { // the arithmetic and logic forms
		int aop = op >> 3;
		bool wide = op & 1;
		switch (op & 7) {
		case 0:
		case 1: {
			modrm(mod, reg, rm);
			Operand o = decodeRM(mod, rm);
			uint16_t a = load(o, wide), b = wide ? reg16(reg) : reg8(reg);
			uint16_t r = alu(aop, a, b, wide);
			if (aop != 7)
				store(o, wide, r);
			return true;
		}
		case 2:
		case 3: {
			modrm(mod, reg, rm);
			Operand o = decodeRM(mod, rm);
			uint16_t a = wide ? reg16(reg) : reg8(reg), b = load(o, wide);
			uint16_t r = alu(aop, a, b, wide);
			if (aop != 7) {
				if (wide)
					reg16(reg) = r;
				else
					reg8(reg) = (uint8_t)r;
			}
			return true;
		}
		case 4: {
			uint16_t r = alu(aop, (uint8_t)ax, fetch8(), false);
			if (aop != 7)
				reg8(0) = (uint8_t)r;
			return true;
		}
		default: {
			uint16_t r = alu(aop, ax, fetch16(), true);
			if (aop != 7)
				ax = r;
			return true;
		}
		}
	}
	if (op < 0x40) {
		switch (op) {
		case 0x06:
		case 0x0E:
		case 0x16:
		case 0x1E:
			push16(seg(op >> 3));
			return true;
		case 0x07:
		case 0x17:
		case 0x1F:
			seg(op >> 3) = pop16();
			return true;
		default:
			return fail("opcode 0x00-0x3F");
		}
	}
	if (op < 0x50) { // inc, dec
		bool saved = cf;
		uint16_t &r = reg16(op);
		r = alu(op < 0x48 ? 0 : 5, r, 1, true);
		cf = saved;
		return true;
	}
	if (op < 0x58) {
		push16(reg16(op));
		return true;
	}
	if (op < 0x60) {
		reg16(op) = pop16();
		return true;
	}
	if (op >= 0x70 && op < 0x80) {
		int8_t d = (int8_t)fetch8();
		if (condition(op & 15))
			ip = (uint16_t)(ip + d);
		return true;
	}
	switch (op) {
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		uint16_t imm = wide && op == 0x81 ? fetch16() : (op == 0x83 ? (uint16_t)(int16_t)(int8_t)fetch8() : fetch8());
		if (!wide)
			imm &= 0xFF;
		uint16_t r = alu(reg, load(o, wide), imm, wide);
		if (reg != 7)
			store(o, wide, r);
		return true;
	}
	case 0x84:
	case 0x85: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		alu(4, load(o, wide), wide ? reg16(reg) : reg8(reg), wide);
		return true;
	}
	case 0x86:
	case 0x87: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		uint16_t a = load(o, wide), b = wide ? reg16(reg) : reg8(reg);
		store(o, wide, b);
		if (wide)
			reg16(reg) = a;
		else
			reg8(reg) = (uint8_t)a;
		return true;
	}
	case 0x88:
	case 0x89: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		store(o, wide, wide ? reg16(reg) : reg8(reg));
		return true;
	}
	case 0x8A:
	case 0x8B: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		uint16_t v = load(o, wide);
		if (wide)
			reg16(reg) = v;
		else
			reg8(reg) = (uint8_t)v;
		return true;
	}
	case 0x8C: {
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		store(o, true, seg(reg));
		return true;
	}
	case 0x8D: {
		modrm(mod, reg, rm);
		_segOverride = -1;
		Operand o = decodeRM(mod, rm);
		reg16(reg) = o.offset;
		return true;
	}
	case 0x8E: {
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		seg(reg) = load(o, true);
		return true;
	}
	case 0x8F: {
		modrm(mod, reg, rm);
		uint16_t v = pop16();
		Operand o = decodeRM(mod, rm);
		store(o, true, v);
		return true;
	}
	case 0x90:
		return true;
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97: {
		uint16_t t = ax;
		ax = reg16(op);
		reg16(op) = t;
		return true;
	}
	case 0x98:
		ax = (uint16_t)(int16_t)(int8_t)(uint8_t)ax;
		return true;
	case 0x99:
		dx = (ax & 0x8000) ? 0xFFFF : 0;
		return true;
	case 0x9A: {
		uint16_t o = fetch16(), s = fetch16();
		push16(cs);
		push16(ip);
		cs = s;
		ip = o;
		return true;
	}
	case 0x9C: {
		uint16_t f = 0x0002;
		f |= cf ? 0x0001 : 0;
		f |= pf ? 0x0004 : 0;
		f |= af ? 0x0010 : 0;
		f |= zf ? 0x0040 : 0;
		f |= sf ? 0x0080 : 0;
		f |= 0x0200;
		f |= df ? 0x0400 : 0;
		f |= of ? 0x0800 : 0;
		push16(f);
		return true;
	}
	case 0x9D: {
		uint16_t f = pop16();
		cf = f & 0x0001;
		pf = f & 0x0004;
		af = f & 0x0010;
		zf = f & 0x0040;
		sf = f & 0x0080;
		df = f & 0x0400;
		of = f & 0x0800;
		return true;
	}
	case 0x9E: {
		uint8_t f = (uint8_t)(ax >> 8);
		cf = f & 1;
		pf = f & 4;
		af = f & 0x10;
		zf = f & 0x40;
		sf = f & 0x80;
		return true;
	}
	case 0x9F: {
		uint8_t f = 2;
		f |= cf ? 1 : 0;
		f |= pf ? 4 : 0;
		f |= af ? 0x10 : 0;
		f |= zf ? 0x40 : 0;
		f |= sf ? 0x80 : 0;
		ax = (uint16_t)((ax & 0xFF) | (f << 8));
		return true;
	}
	case 0xA0:
	case 0xA1: {
		uint16_t off = fetch16();
		uint16_t s = seg(_segOverride >= 0 ? _segOverride : 3);
		if (op == 0xA0)
			reg8(0) = read8(s, off);
		else
			ax = read16(s, off);
		return true;
	}
	case 0xA2:
	case 0xA3: {
		uint16_t off = fetch16();
		uint16_t s = seg(_segOverride >= 0 ? _segOverride : 3);
		if (op == 0xA2)
			write8(s, off, (uint8_t)ax);
		else
			write16(s, off, ax);
		return true;
	}
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF: {
		bool wide = op & 1;
		int delta = (wide ? 2 : 1) * (df ? -1 : 1);
		uint16_t srcSeg = seg(_segOverride >= 0 ? _segOverride : 3);
		bool conditional = op == 0xA6 || op == 0xA7 || op == 0xAE || op == 0xAF;
		for (;;) {
			if (rep && cx == 0)
				break;
			switch (op & 0xFE) {
			case 0xA4:
				if (wide)
					write16(es, di, read16(srcSeg, si));
				else
					write8(es, di, read8(srcSeg, si));
				si = (uint16_t)(si + delta);
				di = (uint16_t)(di + delta);
				break;
			case 0xA6: {
				uint16_t a = wide ? read16(srcSeg, si) : read8(srcSeg, si), b = wide ? read16(es, di) : read8(es, di);
				alu(7, a, b, wide);
				si = (uint16_t)(si + delta);
				di = (uint16_t)(di + delta);
				break;
			}
			case 0xAA:
				if (wide)
					write16(es, di, ax);
				else
					write8(es, di, (uint8_t)ax);
				di = (uint16_t)(di + delta);
				break;
			case 0xAC:
				if (wide)
					ax = read16(srcSeg, si);
				else
					reg8(0) = read8(srcSeg, si);
				si = (uint16_t)(si + delta);
				break;
			default: { // 0xAE scas
				uint16_t b = wide ? read16(es, di) : read8(es, di);
				alu(7, wide ? ax : (uint8_t)ax, b, wide);
				di = (uint16_t)(di + delta);
				break;
			}
			}
			if (!rep)
				break;
			cx = (uint16_t)(cx - 1);
			if (conditional && ((rep == 1 && !zf) || (rep == 2 && zf)))
				break;
		}
		return true;
	}
	case 0xA8:
		alu(4, (uint8_t)ax, fetch8(), false);
		return true;
	case 0xA9:
		alu(4, ax, fetch16(), true);
		return true;
	case 0xC0:
	case 0xC1:
	case 0xD0:
	case 0xD1:
	case 0xD2:
	case 0xD3: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		int count = op >= 0xD2 ? (uint8_t)cx : op >= 0xD0 ? 1 : fetch8();
		store(o, wide, shift(reg, load(o, wide), count, wide));
		return true;
	}
	case 0xC2: {
		uint16_t n = fetch16();
		ip = pop16();
		sp = (uint16_t)(sp + n);
		return true;
	}
	case 0xC3:
		ip = pop16();
		return true;
	case 0xC4:
	case 0xC5: {
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		reg16(reg) = read16(o.segment, o.offset);
		uint16_t s = read16(o.segment, (uint16_t)(o.offset + 2));
		if (op == 0xC4)
			es = s;
		else
			ds = s;
		return true;
	}
	case 0xC6:
	case 0xC7: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		store(o, wide, wide ? fetch16() : fetch8());
		return true;
	}
	case 0xCA: {
		uint16_t n = fetch16();
		ip = pop16();
		cs = pop16();
		sp = (uint16_t)(sp + n);
		return true;
	}
	case 0xCB:
		ip = pop16();
		cs = pop16();
		return true;
	case 0xD7:
		reg8(0) = read8(seg(_segOverride >= 0 ? _segOverride : 3), (uint16_t)(bx + (uint8_t)ax));
		return true;
	case 0xE0:
	case 0xE1:
	case 0xE2:
	case 0xE3: {
		int8_t d = (int8_t)fetch8();
		bool take;
		if (op == 0xE3) {
			take = cx == 0;
		} else {
			cx = (uint16_t)(cx - 1);
			take = cx != 0 && (op == 0xE2 || (op == 0xE1 ? zf : !zf));
		}
		if (take)
			ip = (uint16_t)(ip + d);
		return true;
	}
	case 0xE8: {
		int16_t d = (int16_t)fetch16();
		push16(ip);
		ip = (uint16_t)(ip + d);
		return true;
	}
	case 0xE9: {
		int16_t d = (int16_t)fetch16();
		ip = (uint16_t)(ip + d);
		return true;
	}
	case 0xEA: {
		uint16_t o = fetch16(), s = fetch16();
		cs = s;
		ip = o;
		return true;
	}
	case 0xEB: {
		int8_t d = (int8_t)fetch8();
		ip = (uint16_t)(ip + d);
		return true;
	}
	case 0xF5:
		cf = !cf;
		return true;
	case 0xF8:
		cf = false;
		return true;
	case 0xF9:
		cf = true;
		return true;
	case 0xFA:
	case 0xFB:
		return true;
	case 0xFC:
		df = false;
		return true;
	case 0xFD:
		df = true;
		return true;
	case 0xF6:
	case 0xF7: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		switch (reg) {
		case 0:
		case 1:
			alu(4, load(o, wide), wide ? fetch16() : fetch8(), wide);
			return true;
		case 2:
			store(o, wide, (uint16_t)~load(o, wide));
			return true;
		case 3: {
			uint16_t v = load(o, wide);
			uint16_t r = alu(5, 0, v, wide);
			cf = v != 0;
			store(o, wide, r);
			return true;
		}
		case 4:
		case 5: {
			uint16_t v = load(o, wide);
			if (wide) {
				if (reg == 4) {
					uint32_t r = (uint32_t)ax * v;
					ax = (uint16_t)r;
					dx = (uint16_t)(r >> 16);
					cf = of = dx != 0;
				} else {
					int32_t r = (int32_t)(int16_t)ax * (int16_t)v;
					ax = (uint16_t)r;
					dx = (uint16_t)((uint32_t)r >> 16);
					cf = of = r != (int16_t)r;
				}
			} else if (reg == 4) {
				uint16_t r = (uint16_t)((uint8_t)ax * (uint8_t)v);
				ax = r;
				cf = of = (r >> 8) != 0;
			} else {
				int16_t r = (int16_t)((int8_t)ax * (int8_t)v);
				ax = (uint16_t)r;
				cf = of = r != (int8_t)r;
			}
			return true;
		}
		default: {
			uint16_t v = load(o, wide);
			if (v == 0)
				return fail("division by zero");
			if (wide) {
				if (reg == 6) {
					uint32_t n = ((uint32_t)dx << 16) | ax;
					uint32_t q = n / v;
					if (q > 0xFFFF)
						return fail("division overflow");
					ax = (uint16_t)q;
					dx = (uint16_t)(n % v);
				} else {
					int32_t n = (int32_t)(((uint32_t)dx << 16) | ax);
					int32_t d = (int16_t)v;
					int32_t q = n / d;
					if (q > 32767 || q < -32768)
						return fail("division overflow");
					ax = (uint16_t)q;
					dx = (uint16_t)(n % d);
				}
			} else if (reg == 6) {
				uint16_t q = (uint16_t)(ax / (uint8_t)v);
				if (q > 0xFF)
					return fail("division overflow");
				ax = (uint16_t)((q & 0xFF) | ((ax % (uint8_t)v) << 8));
			} else {
				int16_t n = (int16_t)ax;
				int16_t d = (int8_t)v;
				int16_t q = n / d;
				if (q > 127 || q < -128)
					return fail("division overflow");
				ax = (uint16_t)((q & 0xFF) | (((n % d) & 0xFF) << 8));
			}
			return true;
		}
		}
	}
	case 0xFE:
	case 0xFF: {
		bool wide = op & 1;
		modrm(mod, reg, rm);
		Operand o = decodeRM(mod, rm);
		switch (reg) {
		case 0:
		case 1: {
			bool saved = cf;
			uint16_t r = alu(reg == 0 ? 0 : 5, load(o, wide), 1, wide);
			cf = saved;
			store(o, wide, r);
			return true;
		}
		case 2:
			if (!wide)
				return fail("opcode 0xFE");
			{
				uint16_t target = load(o, true);
				push16(ip);
				ip = target;
			}
			return true;
		case 3:
			if (o.isReg)
				return fail("call far register");
			{
				uint16_t t = read16(o.segment, o.offset), s = read16(o.segment, (uint16_t)(o.offset + 2));
				push16(cs);
				push16(ip);
				cs = s;
				ip = t;
			}
			return true;
		case 4:
			ip = load(o, true);
			return true;
		case 5:
			if (o.isReg)
				return fail("jmp far register");
			{
				uint16_t t = read16(o.segment, o.offset), s = read16(o.segment, (uint16_t)(o.offset + 2));
				cs = s;
				ip = t;
			}
			return true;
		case 6:
			push16(load(o, true));
			return true;
		default:
			return fail("opcode 0xFF /7");
		}
	}
	default:
		break;
	}
	if (op >= 0xB0 && op < 0xB8) {
		reg8(op) = fetch8();
		return true;
	}
	if (op >= 0xB8 && op < 0xC0) {
		reg16(op) = fetch16();
		return true;
	}
	if (op == 0x68) {
		push16(fetch16());
		return true;
	}
	if (op == 0x6A) {
		push16((uint16_t)(int16_t)(int8_t)fetch8());
		return true;
	}
	return fail("unknown opcode");
}

bool Cpu8086::run(uint32_t stopAt, uint64_t maxSteps) {
	_error = nullptr;
	_steps = 0;
	while (linear(cs, ip) != stopAt) {
		if (_steps++ >= maxSteps) {
			_error = "step limit";
			return false;
		}
		if (!step())
			return false;
	}
	return true;
}

} // namespace Ultima6
