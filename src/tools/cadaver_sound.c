/* cadaver_sound GAMEDIR beeper|adlib ID|music OUT.wav [seconds]  - renders a sound or the intro tune of a sound driver to a WAV file */
#include "beeper.h"
#include "gamefiles.h"
#include "mjm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
	if (argc < 5) { fprintf(stderr, "usage: %s gamedir beeper|adlib id|music out.wav [seconds]\n", argv[0]); return 2; }
	GameFiles g;
	if (gamefiles_open(&g, argv[1]) != 0) { fprintf(stderr, "no game in %s\n", argv[1]); return 1; }
	int sb = !strcmp(argv[2], "sblaster"), adlib = !strcmp(argv[2], "adlib") || sb;
	uint8_t *bin; size_t bl;
	if (gamefiles_read(&g, sb ? "sblaster.bin" : adlib ? "adlib.bin" : "beeper.bin", -1, &bin, &bl) != 0) { fprintf(stderr, "driver not found\n"); return 1; }
	uint8_t *un = NULL; size_t ul = 0;
	if (bl > 4 && !memcmp(bin, "RNC\1", 4) && rnc1_unpack(bin, bl, &un, &ul) == 0) { free(bin); bin = un; bl = ul; }
	static Beeper b;
	if (beeper_init_kind(&b, bin, bl, 22050, adlib) != 0) { fprintf(stderr, "driver init failed\n"); return 1; }
	int music = !strcmp(argv[3], "music");
	double secs = argc > 5 ? atof(argv[5]) : 10;
	if (music) beeper_music(&b, 1); else beeper_play(&b, atoi(argv[3]));
	int n = (int)(secs * 22050);
	int16_t *pcm = malloc((size_t)n * 2);
	for (int i = 0; i < n; i += 441) {
		if (music) beeper_music(&b, 1);
		beeper_render(&b, pcm + i, i + 441 <= n ? 441 : n - i);
	}
	FILE *f = fopen(argv[4], "wb");
	uint32_t dl = (uint32_t)n * 2, rate = 22050, br = rate * 2, riff = 36 + dl;
	fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
	uint32_t fl = 16; uint16_t pf = 1, ch = 1, ba = 2, bits = 16;
	fwrite(&fl, 4, 1, f); fwrite(&pf, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f);
	fwrite("data", 1, 4, f); fwrite(&dl, 4, 1, f); fwrite(pcm, 2, (size_t)n, f);
	fclose(f);
	long nz = 0, mx = 0;
	for (int i = 0; i < n; i++) { if (pcm[i]) nz++; if (abs(pcm[i]) > mx) mx = abs(pcm[i]); }
	printf("%d samples, %ld non-zero, peak %ld\n", n, nz, mx);
	return 0;
}
