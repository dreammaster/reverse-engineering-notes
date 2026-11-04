#include "gamefiles.h"
#include "mjm.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

/* find `name` inside `dir` ignoring case; writes the real entry name */
static int find_nocase(const char *dir, const char *name, char *real, size_t realsz) {
	DIR *d = opendir(dir);
	if (!d)
		return -1;
	struct dirent *e;
	int rc = -1;
	while ((e = readdir(d)) != NULL) {
		if (strcasecmp(e->d_name, name) == 0) {
			snprintf(real, realsz, "%s", e->d_name);
			rc = 0;
			break;
		}
	}
	closedir(d);
	return rc;
}

int gamefiles_resolve(const GameFiles *g, const char *dospath, int disk, char *out, size_t outsz) {
	char cur[1024];
	snprintf(cur, sizeof(cur), "%s", g->root);
	const char *p = dospath;
	if (p[0] && p[1] == ':')
		p += 2;
	while (*p) {
		char part[256];
		size_t n = 0;
		while (*p == '/' || *p == '\\')
			p++;
		while (*p && *p != '/' && *p != '\\' && n < sizeof(part) - 1) {
			char c = *p++;
			part[n++] = (c == '?' && disk >= 0) ? (char)('0' + disk) : c;
		}
		part[n] = 0;
		if (!n)
			continue;
		char real[256];
		if (find_nocase(cur, part, real, sizeof(real)) != 0)
			return -1;
		size_t l = strlen(cur);
		snprintf(cur + l, sizeof(cur) - l, "/%s", real);
	}
	struct stat st;
	if (stat(cur, &st) != 0)
		return -1;
	snprintf(out, outsz, "%s", cur);
	return 0;
}

int gamefiles_open(GameFiles *g, const char *dir) {
	memset(g, 0, sizeof(*g));
	snprintf(g->root, sizeof(g->root), "%s", dir);
	char tmp[1024];
	if (gamefiles_resolve(g, "game.exe", -1, tmp, sizeof(tmp)) == 0 ||
	    gamefiles_resolve(g, "tata", -1, tmp, sizeof(tmp)) == 0)
		g->id = GAME_PAYOFF;
	else if (gamefiles_resolve(g, "cad.exe", -1, tmp, sizeof(tmp)) == 0 ||
	         gamefiles_resolve(g, "cadaver.com", -1, tmp, sizeof(tmp)) == 0)
		g->id = GAME_CADAVER;
	else
		return -1;
	return 0;
}

const char *gamefiles_name(GameId id) {
	switch (id) {
	case GAME_CADAVER: return "Cadaver";
	case GAME_PAYOFF: return "Cadaver: The Payoff";
	default: return "unknown";
	}
}

int gamefiles_read(const GameFiles *g, const char *dospath, int disk, uint8_t **data, size_t *len) {
	char path[1024];
	if (gamefiles_resolve(g, dospath, disk, path, sizeof(path)) != 0)
		return -1;
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *d = (uint8_t *)malloc(n > 0 ? (size_t)n : 1);
	if (!d || fread(d, 1, (size_t)n, f) != (size_t)n) {
		free(d);
		fclose(f);
		return -1;
	}
	fclose(f);
	*data = d;
	*len = (size_t)n;
	return 0;
}

int gamefiles_read_mjm(const GameFiles *g, const char *dospath, int disk, uint8_t **data, size_t *len) {
	uint8_t *raw;
	size_t rlen;
	if (gamefiles_read(g, dospath, disk, &raw, &rlen) != 0)
		return -1;
	int rc = mjm_unpack(raw, rlen, data, len);
	free(raw);
	return rc;
}
