/* Machine setup, the DOS-style memory allocator, the resource layer and the C runtime of the recompiled game. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "game.h"

Game G;

void recomp_unimplemented(const char *name, Cpu *c) {
	fprintf(stderr, "\nunimplemented host routine: %s  (ax=%04X bx=%04X cx=%04X dx=%04X sp=%04X; args on stack: %04X %04X %04X %04X)\n", name,
		c->ax, c->bx, c->cx, c->dx, c->sp, host_arg(c, 0), host_arg(c, 1), host_arg(c, 2), host_arg(c, 3));
	exit(3);
}

/* ---- allocator: free areas of the address space, in paragraphs */
typedef struct { uint16_t seg, paras; int used; } Block;
static Block blocks[4096];
static int nblocks;

static void add_free_area(uint16_t seg, uint16_t paras) { blocks[nblocks++] = (Block){ seg, paras, 0 }; }

/* first fit; the blocks are kept sorted by address, a larger free block is split */
uint16_t dos_alloc(uint32_t bytes) {
	uint32_t paras = (bytes + 15) >> 4;
	if (!paras) paras = 1;
	for (int i = 0; i < nblocks; i++) {
		if (blocks[i].used || blocks[i].paras < paras) continue;
		if (blocks[i].paras > paras && nblocks < 4095) {
			memmove(&blocks[i + 2], &blocks[i + 1], (nblocks - i - 1) * sizeof(Block));
			blocks[i + 1] = (Block){ (uint16_t)(blocks[i].seg + paras), (uint16_t)(blocks[i].paras - paras), 0 };
			blocks[i].paras = (uint16_t)paras;
			nblocks++;
		}
		blocks[i].used = 1;
		return blocks[i].seg;
	}
	return 0;
}

void dos_free(uint16_t seg) {
	for (int i = 0; i < nblocks; i++)
		if (blocks[i].seg == seg && blocks[i].used) {
			blocks[i].used = 0;
			/* merge with free neighbours */
			for (int k = 0; k + 1 < nblocks;) {
				if (!blocks[k].used && !blocks[k + 1].used && blocks[k].seg + blocks[k].paras == blocks[k + 1].seg) {
					blocks[k].paras = (uint16_t)(blocks[k].paras + blocks[k + 1].paras);
					memmove(&blocks[k + 1], &blocks[k + 2], (nblocks - k - 2) * sizeof(Block));
					nblocks--;
				} else k++;
			}
			return;
		}
}

uint32_t dos_block_size(uint16_t seg) {
	for (int i = 0; i < nblocks; i++)
		if (blocks[i].seg == seg && blocks[i].used) return (uint32_t)blocks[i].paras << 4;
	return 0;
}

/* ---- startup */
int game_init(const char *dir) {
	char path[600];
	FILE *f;
	size_t n;
	snprintf(G.data_dir, sizeof G.data_dir, "%s", dir);
	snprintf(path, sizeof path, "%s/IMAGE.BIN", dir);
	if (!(f = fopen(path, "rb"))) { fprintf(stderr, "cannot open %s (make it with tools/mm3_image.py)\n", path); return -1; }
	n = fread(MEM + 0x10000, 1, 0x42165, f);
	fclose(f);
	if (n != 0x42165) { fprintf(stderr, "%s: wrong size\n", path); return -1; }
	snprintf(path, sizeof path, "%s/MM3.CC", dir);
	if (mm3_cc_open(&G.cc, path)) { fprintf(stderr, "cannot open %s\n", path); return -1; }
	snprintf(path, sizeof path, "%s/MM3.CUR", dir);
	if (mm3_cc_open(&G.cur, path)) { fprintf(stderr, "cannot open %s\n", path); return -1; }
	if (mm3_palette_load(&G.palette, &G.cc) || mm3_scale_patterns_load(G.scale_patterns, &G.cc)) return -1;
	/* free memory: above the image up to the stack segment, and above the video memory */
	nblocks = 0;
	add_free_area(0x5400, 0x9F00 - 0x5400);
	add_free_area(0xB000, 0x10F00 - 0xB000 > 0xFFFF ? 0xFFFF : 0x10F00 - 0xB000);
	return 0;
}

void game_call(void (*fn)(Cpu *c), Cpu *c, const uint16_t *args, int nargs) {
	for (int i = nargs - 1; i >= 0; i--) PUSH(c, args[i]);
	PUSH(c, 0); PUSH(c, 0); /* far return address */
	fn(c);
	c->sp = (uint16_t)(c->sp + 2 * nargs);
}

/* ---- resources: maze files (names starting with MAZE) are stored members of MM3.CUR, everything else LZHUF members of MM3.CC */
/* writeResource stores changed members (party, roster, maze state) in memory; the data files are never modified */
#define MAXOV 64
static struct { char name[16]; uint8_t *data; size_t len; } overrides[MAXOV];
static int novr;

void host_writeResource(Cpu *c) {
	const char *name = (const char *)(SEGP(host_arg(c, 1)) + host_arg(c, 0));
	const uint8_t *src = SEGP(host_arg(c, 3)) + host_arg(c, 2);
	size_t len = 0;
	uint8_t *orig = mm3_cc_read(&G.cur, name, &len);
	if (!orig) orig = mm3_cc_read(&G.cc, name, &len);
	free(orig);
	if (!len) return;
	int i;
	for (i = 0; i < novr; i++) if (!strcasecmp(overrides[i].name, name)) break;
	if (i == novr) {
		if (novr == MAXOV) return;
		snprintf(overrides[novr].name, sizeof overrides[0].name, "%s", name);
		overrides[novr].data = malloc(len);
		overrides[novr++].len = len;
	}
	memcpy(overrides[i].data, src, len < overrides[i].len ? len : overrides[i].len);
}

uint16_t game_load_resource(const char *name, uint32_t *size) {
	if (getenv("MM3_DUMPLIST")) fprintf(stderr, "load %s\n", name);
	for (int i = 0; i < novr; i++)
		if (!strcasecmp(overrides[i].name, name)) {
			uint16_t seg = dos_alloc(overrides[i].len + 16);
			if (!seg) return 0;
			memcpy(SEGP(seg), overrides[i].data, overrides[i].len);
			if (size) *size = (uint32_t)overrides[i].len;
			return seg;
		}
	Mm3Cc *cc = (strlen(name) >= 4 && !strncasecmp(name, "maze", 4)) ? &G.cur : &G.cc;
	size_t len;
	uint8_t *data = mm3_cc_read(cc, name, &len);
	uint16_t seg;
	if (!data && cc == &G.cur) { cc = &G.cc; data = mm3_cc_read(cc, name, &len); } /* MAZE72.DAT etc. also exist in MM3.CC */
	if (!data) { fprintf(stderr, "resource not found: %s\n", name); return 0; }
	seg = dos_alloc(len + 16);
	if (!seg) { fprintf(stderr, "out of memory loading %s\n", name); free(data); return 0; }
	memcpy(SEGP(seg), data, len);
	free(data);
	if (size) *size = (uint32_t)len;
	return seg;
}

static const char *cstr_at(uint16_t seg, uint16_t off) { return (const char *)(SEGP(seg) + off); }

/* sub_22740 / loadResourceByName(name far): the segment of the loaded member in bx (dx for the wrapper), size in DGROUP `n` */
void host_loadResourceByName(Cpu *c) {
	uint32_t size;
	uint16_t seg = game_load_resource(cstr_at(host_arg(c, 1), host_arg(c, 0)), &size);
	if (!seg) exit(4);
	wr16(DG, 0xF086, size); /* `n` */
	c->dx = seg;
	c->ax = 0;
}

/* sub_22534: bx = bytes -> bx = segment (the original ends the program when DOS has no memory) */
void host_sub_22534(Cpu *c) {
	uint16_t seg = dos_alloc(c->bx);
	if (!seg) { fprintf(stderr, "Might and Magic ]I[ ran out of memory!\n"); exit(5); }
	c->bx = seg;
	c->ax = seg;
}
void host_allocFar(Cpu *c) { uint16_t seg = dos_alloc(host_arg(c, 0)); c->dx = seg; c->ax = 0; }
void host_MemFree(Cpu *c) { (void)c; wr16(DG, 0x1B7, 0xFFF); /* word_28873: free paragraphs / 64: plenty */ }

/* ---- C runtime (Borland helper routines: register or stack conventions as noted) */
void host_LXMUL_AT(Cpu *c) { uint32_t r = (((uint32_t)c->dx << 16) | c->ax) * (((uint32_t)c->cx << 16) | c->bx); c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16); }
void host_LXLSH_AT(Cpu *c) { uint32_t v = ((uint32_t)c->dx << 16) | c->ax; unsigned n = c->cx & 0xFF; v = n >= 32 ? 0 : v << n; c->ax = (uint16_t)v; c->dx = (uint16_t)(v >> 16); }
void host_LXURSH_AT(Cpu *c) { uint32_t v = ((uint32_t)c->dx << 16) | c->ax; unsigned n = c->cx & 0xFF; v = n >= 32 ? 0 : v >> n; c->ax = (uint16_t)v; c->dx = (uint16_t)(v >> 16); }
/* unsigned 32-bit divide / modulo: dividend and divisor on the stack (low word first), the routine removes them (retf 8) */
void host_F_LUDIV_AT(Cpu *c) {
	uint32_t a = host_arg(c, 0) | ((uint32_t)host_arg(c, 1) << 16), b = host_arg(c, 2) | ((uint32_t)host_arg(c, 3) << 16);
	uint32_t r = b ? a / b : 0xFFFFFFFFu;
	c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16); c->sp += 8;
}
void host_LUMOD_AT(Cpu *c) {
	uint32_t a = host_arg(c, 0) | ((uint32_t)host_arg(c, 1) << 16), b = host_arg(c, 2) | ((uint32_t)host_arg(c, 3) << 16);
	uint32_t r = b ? a % b : a;
	c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16); c->sp += 8;
}
/* SCOPY@: copy cx bytes from the far pointer (arg0, arg1) to the far pointer (arg2, arg3); removes the 8 argument bytes */
void host_SCOPY_AT(Cpu *c) {
	memmove(SEGP(host_arg(c, 3)) + host_arg(c, 2), SEGP(host_arg(c, 1)) + host_arg(c, 0), c->cx);
	c->sp += 8;
}

static uint8_t *dsp(uint16_t off) { return DG + off; }

void host__memset(Cpu *c) { memset(dsp(host_arg(c, 0)), (int)host_arg(c, 1), host_arg(c, 2)); c->ax = host_arg(c, 0); }
/* _memcpy_0 / _strncpy_0: destination and source are far pointers (offset, segment) */
void host__memcpy_0(Cpu *c) {
	memmove(SEGP(host_arg(c, 1)) + host_arg(c, 0), SEGP(host_arg(c, 3)) + host_arg(c, 2), host_arg(c, 4));
	c->ax = host_arg(c, 0); c->dx = host_arg(c, 1);
}
void host__strncpy_0(Cpu *c) {
	char *d = (char *)(SEGP(host_arg(c, 1)) + host_arg(c, 0));
	const char *s = (const char *)(SEGP(host_arg(c, 3)) + host_arg(c, 2));
	unsigned n = host_arg(c, 4), i = 0;
	for (; i < n && s[i]; i++) d[i] = s[i];
	for (; i < n; i++) d[i] = 0;
	c->ax = host_arg(c, 0); c->dx = host_arg(c, 1);
}
void host__strcpy(Cpu *c) { strcpy((char *)dsp(host_arg(c, 0)), (const char *)dsp(host_arg(c, 1))); c->ax = host_arg(c, 0); }
void host__stricmp(Cpu *c) { c->ax = (uint16_t)(int16_t)strcasecmp((const char *)dsp(host_arg(c, 0)), (const char *)dsp(host_arg(c, 1))); }
void host__ultoa(Cpu *c) {
	uint32_t v = host_arg(c, 0) | ((uint32_t)host_arg(c, 1) << 16);
	unsigned radix = host_arg(c, 3);
	char tmp[40];
	int n = 0;
	char *out = (char *)dsp(host_arg(c, 2));
	if (!v) tmp[n++] = '0';
	while (v) { unsigned d = v % radix; tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= radix; }
	for (int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
	out[n] = 0;
	c->ax = host_arg(c, 2);
}
void host__time(Cpu *c) { uint32_t t = (uint32_t)time(NULL); c->ax = (uint16_t)t; c->dx = (uint16_t)(t >> 16); }

/* sprintf with the DOS C library's formats: %d %u %ld %lu %x %c %s %p and widths / zero padding; arguments are words on the stack */
void host__sprintf(Cpu *c) {
	char *out = (char *)dsp(host_arg(c, 0));
	const char *f = (const char *)dsp(host_arg(c, 1));
	unsigned ai = 2;
	char *o = out, num[40];
	for (; *f; f++) {
		int zero = 0, left = 0, width = 0, lng = 0, nlen;
		char spec;
		if (*f != '%') { *o++ = *f; continue; }
		f++;
		if (*f == '%') { *o++ = '%'; continue; }
		for (;; f++) { if (*f == '-') left = 1; else if (*f == '0') zero = 1; else if (*f == '+' || *f == ' ' || *f == '#') {} else break; }
		if (*f == '*') { width = (int16_t)host_arg(c, (int)ai++); f++; } else while (isdigit((unsigned char)*f)) width = width * 10 + (*f++ - '0');
		if (*f == '.') { f++; while (isdigit((unsigned char)*f)) f++; }
		if (*f == 'l') { lng = 1; f++; }
		spec = *f;
		switch (spec) {
		case 'd': case 'i': {
			int32_t v = lng ? (int32_t)(host_arg(c, (int)ai) | ((uint32_t)host_arg(c, (int)ai + 1) << 16)) : (int16_t)host_arg(c, (int)ai);
			ai += lng ? 2 : 1;
			nlen = snprintf(num, sizeof num, "%d", v);
			break;
		}
		case 'u': case 'x': case 'X': {
			uint32_t v = lng ? (host_arg(c, (int)ai) | ((uint32_t)host_arg(c, (int)ai + 1) << 16)) : host_arg(c, (int)ai);
			ai += lng ? 2 : 1;
			nlen = snprintf(num, sizeof num, spec == 'u' ? "%u" : spec == 'x' ? "%x" : "%X", v);
			break;
		}
		case 'p': nlen = snprintf(num, sizeof num, "%04X", host_arg(c, (int)ai++)); break;
		case 'c': num[0] = (char)host_arg(c, (int)ai++); num[1] = 0; nlen = 1; break;
		case 's': {
			const char *s = (const char *)dsp(host_arg(c, (int)ai++));
			nlen = (int)strlen(s);
			if (!left) for (int k = nlen; k < width; k++) *o++ = ' ';
			memcpy(o, s, nlen); o += nlen;
			if (left) for (int k = nlen; k < width; k++) *o++ = ' ';
			continue;
		}
		default: num[0] = '?'; num[1] = 0; nlen = 1; break;
		}
		if (!left) for (int k = nlen; k < width; k++) *o++ = zero ? '0' : ' ';
		memcpy(o, num, nlen); o += nlen;
		if (left) for (int k = nlen; k < width; k++) *o++ = ' ';
	}
	*o = 0;
	c->ax = (uint16_t)(o - out);
}

void host__sscanf(Cpu *c) { /* only "%d" style scanning of a number is used (a typed amount) */
	const char *s = (const char *)dsp(host_arg(c, 0));
	int v = atoi(s);
	if (host_arg(c, 2)) wr16(DG, host_arg(c, 2), (uint16_t)v);
	c->ax = isdigit((unsigned char)s[0]) ? 1 : 0;
}

void host__cputs(Cpu *c) { fputs((const char *)dsp(host_arg(c, 0)), stderr); }
void host__exit(Cpu *c) { exit((int)host_arg(c, 0)); }
void host__textmode(Cpu *c) { (void)c; }
void host_sub_27F86(Cpu *c) { (void)c; fprintf(stderr, "game exit\n"); exit(0); }
void host_sub_250F8(Cpu *c) { (void)c; }
void host_sub_378C0(Cpu *c) { (void)c; fprintf(stderr, "sub_378C0 (fatal error handler)\n"); exit(6); }

/* rnd(lo, hi): uniform in [lo, hi] */
static uint32_t rng_state = 0x1234567u;
static unsigned rng_next(void) { rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5; return rng_state; }
void host_rnd(Cpu *c) {
	unsigned lo = host_arg(c, 0), hi = host_arg(c, 1);
	c->ax = (uint16_t)(hi >= lo ? lo + rng_next() % (hi - lo + 1) : lo);
}
