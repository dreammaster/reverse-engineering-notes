// Engine functions of Companions of Xanth that are not game logic, implemented for the host: text output is decoded and recorded, drawing/sound/input
// functions are recorded with their arguments (the generated host_defaults.cpp provides the logging fallback for everything not defined here).
#include "host.h"

#include <cstdio>

// the first two words of a text argument: either an 0xF0BBIIII message id or a far pointer to a format string
static std::string textArg(Cpu &c, int &n, bool &valid) {
	Host &h = host();
	uint32_t d = (uint32_t)c.arg(0) | ((uint32_t)c.arg(1) << 16);
	n = 2;
	valid = true;
	if ((d >> 24) == 0xF0) {
		bool ok;
		std::string s = h.message(d, &ok);
		if (!ok) {
			char b[64];
			snprintf(b, sizeof(b), "bad message id %08X", d);
			h.problem(b);
			valid = false;
		}
		return s;
	}
	bool ok;
	std::string s = h.str(c.arg(1), c.arg(0), 400, &ok);
	if (!ok) {
		char b[80];
		snprintf(b, sizeof(b), "format string pointer %04X:%04X is not a NUL-terminated string", c.arg(1), c.arg(0));
		h.problem(b);
		valid = false;
	}
	return s;
}

// printf-like expansion of %s (far pointer: 2 words), %d / %c / %u (1 word); arguments follow the text argument on the stack
static std::string expand(Cpu &c, const std::string &fmt, int n) {
	Host &h = host();
	std::string out;
	for (size_t i = 0; i < fmt.size(); i++) {
		if (fmt[i] != '%' || i + 1 >= fmt.size()) {
			out.push_back(fmt[i]);
			continue;
		}
		char k = fmt[++i];
		if (k == 's') {
			uint16_t off = c.arg(n), seg = c.arg(n + 1);
			n += 2;
			bool ok;
			std::string s = h.str(seg, off, 400, &ok);
			if (!ok) {
				char b[96];
				snprintf(b, sizeof(b), "%%s argument %04X:%04X is not a NUL-terminated string", seg, off);
				h.problem(b);
			}
			out += s;
		} else if (k == 'd' || k == 'u') {
			out += std::to_string(k == 'd' ? (int)(int16_t)c.arg(n) : (int)c.arg(n));
			n++;
		} else if (k == 'c') {
			out.push_back((char)c.arg(n++));
		} else {
			out.push_back('%');
			out.push_back(k);
		}
	}
	return out;
}

void H_Font_WriteMessage(Cpu &c) {
	Host &h = host();
	int n;
	bool valid;
	std::string fmt = textArg(c, n, valid);
	Event e;
	e.name = "Font_WriteMessage";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	e.text = expand(c, fmt, n);
	h.events.push_back(e);
	if (h.onText) h.onText(h);
	c.ax.x = c.dx.x = 0;
}

void H_ResourceText_GetStr(Cpu &c) {
	Host &h = host();
	int n;
	bool valid;
	std::string fmt = textArg(c, n, valid);
	uint32_t p = h.putString(fmt);
	Event e;
	e.name = "ResourceText_GetStr";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	e.text = fmt;
	h.events.push_back(e);
	c.ax.x = (uint16_t)p;
	c.dx.x = (uint16_t)(p >> 16);
}

void H_show_text_box(Cpu &c) {
	Host &h = host();
	int n;
	bool valid;
	std::string fmt = textArg(c, n, valid);
	Event e;
	e.name = "show_text_box";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	e.text = expand(c, fmt, n);
	h.events.push_back(e);
	c.ax.x = c.dx.x = 0;
}

void H_wputchar_q(Cpu &c) {
	Event e;
	e.name = "wputchar?";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	if (e.w[0] >= 0x20 && e.w[0] < 0x7F)
		e.text = std::string(1, (char)e.w[0]);
	host().events.push_back(e);
	c.ax.x = c.dx.x = 0;
}

void H_realsound_play(Cpu &c) {
	Host &h = host();
	Event e;
	e.name = "realsound_play";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	bool ok;
	e.text = h.str(c.arg(1), c.arg(0), 64, &ok);
	if (!ok)
		h.problem("realsound_play: file name pointer is not a string");
	h.events.push_back(e);
	c.ax.x = c.dx.x = 0;
}

// Microsoft C large-model sprintf(char far *dest, const char far *fmt, ...): %s takes a far pointer (2 words), %d/%u/%x/%c one word, %ld/%lu two words.
void H__sprintf(Cpu &c) {
	Host &h = host();
	uint16_t doff = c.arg(0), dseg = c.arg(1);
	bool ok;
	std::string fmt = h.str(c.arg(3), c.arg(2), 400, &ok);
	int n = 4;
	std::string out;
	for (size_t i = 0; i < fmt.size(); i++) {
		if (fmt[i] != '%') { out.push_back(fmt[i]); continue; }
		std::string spec = "%";
		i++;
		bool lng = false;
		while (i < fmt.size() && (isdigit((unsigned char)fmt[i]) || fmt[i] == '-' || fmt[i] == '+' || fmt[i] == ' ' || fmt[i] == '#' || fmt[i] == '.' || fmt[i] == 'l' || fmt[i] == 'F' || fmt[i] == 'N')) {
			if (fmt[i] == 'l') lng = true;
			else if (fmt[i] != 'F' && fmt[i] != 'N') spec.push_back(fmt[i]);
			i++;
		}
		if (i >= fmt.size()) break;
		char k = fmt[i], buf[256];
		if (k == 's') {
			uint16_t off = c.arg(n), seg = c.arg(n + 1);
			n += 2;
			snprintf(buf, sizeof(buf), (spec + "s").c_str(), h.str(seg, off, 200).c_str());
		} else if (k == 'd' || k == 'i') {
			if (lng) { snprintf(buf, sizeof(buf), (spec + "d").c_str(), (int)((uint32_t)c.arg(n) | ((uint32_t)c.arg(n + 1) << 16))); n += 2; }
			else snprintf(buf, sizeof(buf), (spec + "d").c_str(), (int)(int16_t)c.arg(n++));
		} else if (k == 'u' || k == 'x' || k == 'X') {
			if (lng) { snprintf(buf, sizeof(buf), (spec + std::string(1, k)).c_str(), (unsigned)((uint32_t)c.arg(n) | ((uint32_t)c.arg(n + 1) << 16))); n += 2; }
			else snprintf(buf, sizeof(buf), (spec + std::string(1, k)).c_str(), (unsigned)c.arg(n++));
		} else if (k == 'c') {
			snprintf(buf, sizeof(buf), "%c", (char)c.arg(n++));
		} else if (k == '%') {
			snprintf(buf, sizeof(buf), "%%");
		} else {
			snprintf(buf, sizeof(buf), "?");
		}
		out += buf;
	}
	if (out.size() > 120) { char pb[200]; snprintf(pb, sizeof(pb), "sprintf produced %zu bytes into %04X:%04X, format %04X:%04X '%.40s'", out.size(), dseg, doff, c.arg(3), c.arg(2), fmt.c_str()); h.problem(pb); out.resize(120); }
	for (size_t i = 0; i <= out.size(); i++)
		c.wr8(dseg, (uint16_t)(doff + i), i < out.size() ? (uint8_t)out[i] : 0);
	c.ax.x = (uint16_t)out.size();
	c.dx.x = 0;
}

// Font_WriteString(const char far *text): the text the game draws on screen (the host records it instead of rendering glyphs)
void H_Font_WriteString(Cpu &c) {
	Host &h = host();
	bool ok;
	Event e;
	e.name = "Font_WriteString";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	e.text = h.str(c.arg(1), c.arg(0), 400, &ok);
	h.events.push_back(e);
	c.ax.x = c.dx.x = 0;
}

// sub_676B6: helper of the three big cutscene routines (sub_67E2A / sub_69FF7 / sub_6C42D); it builds on code that jumps into the middle of an instruction
// (`call loc_4226A+1`), which the lifter cannot follow, so the host records the call and reports success
void H_sub_676B6(Cpu &c) {
	Event e;
	e.name = "sub_676B6";
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	host().events.push_back(e);
	c.ax.x = c.dx.x = 0;
}

// RTLink overlay manager: the lifted program is one image, so "loading" an overlay segment always succeeds
void H_rtlink_countFlags(Cpu &c) { c.ax.x = 0; c.dx.x = 0; }
void H_rtlink_loadSegment(Cpu &c) { c.ax.x = 1; c.dx.x = 0; }

// int _int86(int intno, union REGS far *in, union REGS far *out): the C library builds `int N` on the stack and runs it; the host executes the interrupt directly.
// WORDREGS = { ax, bx, cx, dx, si, di, cflag }
void H___int86(Cpu &c) {
	int intno = c.arg(0) & 0xFF;
	uint16_t inOff = c.arg(1), inSeg = c.arg(2), outOff = c.arg(3), outSeg = c.arg(4);
	uint16_t si = c.si, di = c.di, es = c.es, ds = c.ds;
	c.ax.x = c.rd16(inSeg, inOff);
	c.bx.x = c.rd16(inSeg, (uint16_t)(inOff + 2));
	c.cx.x = c.rd16(inSeg, (uint16_t)(inOff + 4));
	c.dx.x = c.rd16(inSeg, (uint16_t)(inOff + 6));
	c.si = c.rd16(inSeg, (uint16_t)(inOff + 8));
	c.di = c.rd16(inSeg, (uint16_t)(inOff + 10));
	c.CF = false;
	host().machine.intr(c, intno);
	c.wr16(outSeg, outOff, c.ax.x);
	c.wr16(outSeg, (uint16_t)(outOff + 2), c.bx.x);
	c.wr16(outSeg, (uint16_t)(outOff + 4), c.cx.x);
	c.wr16(outSeg, (uint16_t)(outOff + 6), c.dx.x);
	c.wr16(outSeg, (uint16_t)(outOff + 8), c.si);
	c.wr16(outSeg, (uint16_t)(outOff + 10), c.di);
	c.wr16(outSeg, (uint16_t)(outOff + 12), c.CF ? 1 : 0);
	c.si = si; c.di = di; c.es = es; c.ds = ds;
}

// void longjmp(jmp_buf env, int val): unwinds the lifted C++ frames up to the function that called _setjmp for this buffer
void H__longjmp(Cpu &c) {
	throw LongJmp{c.arg(1), c.arg(0), c.arg(2)};
}
