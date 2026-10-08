/* Hand-written hosts for the Borland C runtime's file routines (stdio, handles, findfirst).
 * Paths are resolved case-insensitively inside the game data directory; the original FILE* / handle
 * values are opaque tokens to the game code, so small table indices stand in for them. */
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "game.h"

#define MAXF 16
static FILE *files[MAXF];
static int fds[MAXF];
static const uint8_t *dsp(uint16_t off) { return DG + off; }

/* find an existing file case-insensitively; otherwise return the path as given (for creation) */
static void resolve(const char *name, char *out, size_t n) {
	const char *base = strrchr(name, '\\');
	base = base ? base + 1 : name;
	DIR *d = opendir(G.data_dir);
	snprintf(out, n, "%s/%s", G.data_dir, base);
	if (!d) return;
	struct dirent *e;
	while ((e = readdir(d)))
		if (!strcasecmp(e->d_name, base)) { snprintf(out, n, "%s/%s", G.data_dir, e->d_name); break; }
	closedir(d);
}

static FILE *fp_of(uint16_t t) { return t && t <= MAXF ? files[t - 1] : NULL; }

void host__fopen(Cpu *c) {
	char path[512], mode[8];
	snprintf(mode, sizeof mode, "%s", (const char *)dsp(host_arg(c, 1)));
	resolve((const char *)dsp(host_arg(c, 0)), path, sizeof path);
	FILE *f = fopen(path, mode);
	c->ax = 0;
	if (!f) return;
	for (int i = 0; i < MAXF; i++) if (!files[i]) { files[i] = f; c->ax = i + 1; return; }
	fclose(f);
}
void host__fclose(Cpu *c) {
	uint16_t t = host_arg(c, 0);
	FILE *f = fp_of(t);
	c->ax = f ? (fclose(f), files[t - 1] = NULL, 0) : 0xFFFF;
}
void host__fread(Cpu *c) {
	FILE *f = fp_of(host_arg(c, 3));
	c->ax = f ? (uint16_t)fread((void *)dsp(host_arg(c, 0)), host_arg(c, 1), host_arg(c, 2), f) : 0;
}
void host__fwrite(Cpu *c) {
	FILE *f = fp_of(host_arg(c, 3));
	c->ax = f ? (uint16_t)fwrite(dsp(host_arg(c, 0)), host_arg(c, 1), host_arg(c, 2), f) : 0;
}
void host__fseek(Cpu *c) {
	FILE *f = fp_of(host_arg(c, 0));
	long off = (int32_t)(host_arg(c, 1) | ((uint32_t)host_arg(c, 2) << 16));
	c->ax = f ? (uint16_t)fseek(f, off, host_arg(c, 3)) : 0xFFFF;
}
void host__ftell(Cpu *c) {
	FILE *f = fp_of(host_arg(c, 0));
	long p = f ? ftell(f) : -1;
	c->ax = (uint16_t)p; c->dx = (uint16_t)((uint32_t)p >> 16);
}
void host__rewind(Cpu *c) { FILE *f = fp_of(host_arg(c, 0)); if (f) rewind(f); }

/* low-level handles: access flags are Borland's (O_WRONLY 1, O_RDWR 2, O_CREAT 100h, O_TRUNC 200h, O_BINARY 8000h) */
void host__open(Cpu *c) {
	char path[512];
	unsigned fl = host_arg(c, 1);
	int mode = (fl & 3) == 1 ? O_WRONLY : (fl & 3) == 2 ? O_RDWR : O_RDONLY;
	if (fl & 0x100) mode |= O_CREAT;
	if (fl & 0x200) mode |= O_TRUNC;
	resolve((const char *)dsp(host_arg(c, 0)), path, sizeof path);
	int fd = open(path, mode, 0644);
	c->ax = 0xFFFF;
	if (fd < 0) return;
	for (int i = 0; i < MAXF; i++) if (!fds[i]) { fds[i] = fd; c->ax = i + 5; return; }
	close(fd);
}
void host__close(Cpu *c) {
	unsigned h = host_arg(c, 0);
	if (h >= 5 && h < 5 + MAXF && fds[h - 5]) { close(fds[h - 5]); fds[h - 5] = 0; c->ax = 0; } else c->ax = 0xFFFF;
}
void host_j____read(Cpu *c) {
	unsigned h = host_arg(c, 0);
	c->ax = (h >= 5 && h < 5 + MAXF && fds[h - 5]) ? (uint16_t)read(fds[h - 5], (void *)dsp(host_arg(c, 1)), host_arg(c, 2)) : 0xFFFF;
}

/* findfirst / findnext: Borland ffblk has the name at +1Eh, size at +1Ah; matching supports "*" and "?" */
static char **found; static int nfound, nextfound;
static int wild(const char *p, const char *s) {
	for (; *p; p++, s++) {
		if (*p == '*') { while (p[1] == '*') p++; for (;; s++) { if (wild(p + 1, s)) return 1; if (!*s) return 0; } }
		if (!*s || (*p != '?' && toupper((unsigned char)*p) != toupper((unsigned char)*s))) return 0;
	}
	return !*s;
}
static int cmpstr(const void *a, const void *b) { return strcasecmp(*(char *const *)a, *(char *const *)b); }
static void fill_ffblk(uint16_t off) {
	uint8_t *b = DG + off;
	memset(b, 0, 0x2B);
	snprintf((char *)b + 0x1E, 13, "%s", found[nextfound++]);
}
void host__findfirst(Cpu *c) {
	const char *pat = (const char *)dsp(host_arg(c, 0));
	const char *base = strrchr(pat, '\\'); base = base ? base + 1 : pat;
	for (int i = 0; i < nfound; i++) free(found[i]);
	free(found); found = NULL; nfound = nextfound = 0;
	DIR *d = opendir(G.data_dir);
	struct dirent *e;
	while (d && (e = readdir(d)))
		if (e->d_name[0] != '.' && strlen(e->d_name) <= 12 && wild(base, e->d_name)) {
			found = realloc(found, (nfound + 1) * sizeof *found);
			found[nfound++] = strdup(e->d_name);
		}
	if (d) closedir(d);
	qsort(found, nfound, sizeof *found, cmpstr);
	if (!nfound) { c->ax = 0xFFFF; return; }
	fill_ffblk(host_arg(c, 1));
	c->ax = 0;
}
void host__findnext(Cpu *c) {
	if (nextfound >= nfound) { c->ax = 0xFFFF; return; }
	fill_ffblk(host_arg(c, 0));
	c->ax = 0;
}

/* misc runtime */
void host__atol(Cpu *c) {
	long v = atol((const char *)dsp(host_arg(c, 0)));
	c->ax = (uint16_t)v; c->dx = (uint16_t)((uint32_t)v >> 16);
}
void host__ctime(Cpu *c) { DG[host_arg(c, 0)] = 0; c->ax = host_arg(c, 0); }
void host__memcpy(Cpu *c) { memmove(DG + host_arg(c, 0), DG + host_arg(c, 1), host_arg(c, 2)); c->ax = host_arg(c, 0); }
/* _memset_0: far destination (offset, segment), value, count */
void host__memset_0(Cpu *c) {
	memset(SEGP(host_arg(c, 1)) + host_arg(c, 0), (int)host_arg(c, 2), host_arg(c, 3));
	c->ax = host_arg(c, 0); c->dx = host_arg(c, 1);
}
/* _movedata(srcseg, srcoff, dstseg, dstoff, n) */
void host__movedata(Cpu *c) {
	memmove(SEGP(host_arg(c, 2)) + host_arg(c, 3), SEGP(host_arg(c, 0)) + host_arg(c, 1), host_arg(c, 4));
}
