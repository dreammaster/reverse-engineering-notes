/* ui_run DATADIR SCRIPT INITIAL.scr OUT.bin
 * Runs a script of text-engine commands on the C port and writes the screen (64000 bytes) after every command.
 * Script lines (text is hex): "P <hex>" print, "O x y w h colour <hex|->" open window, "C n" close windows.
 * Used by tools/mm3_ui_fuzz.py to compare against the original module. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_text.h"

static char *unhex(const char *h) {
	size_t n = strlen(h) / 2;
	char *s = malloc(n + 1);
	for (size_t i = 0; i < n; i++) { unsigned v; sscanf(h + 2 * i, "%2x", &v); s[i] = (char)v; }
	s[n] = 0;
	return s;
}

int main(int argc, char **argv) {
	char path[512], line[8192];
	Mm3Cc cc;
	Mm3Ui *ui;
	FILE *f, *out;
	if (argc < 5) return 2;
	snprintf(path, sizeof path, "%s/MM3.CC", argv[1]);
	if (mm3_cc_open(&cc, path)) return 2;
	ui = mm3_ui_create(&cc);
	if (!ui) return 2;
	f = fopen(argv[3], "rb");
	if (f) { if (fread(ui->screen, 1, 64000, f) != 64000) return 2; fclose(f); }
	out = fopen(argv[4], "wb");
	f = fopen(argv[2], "r");
	while (f && fgets(line, sizeof line, f)) {
		char cmd = line[0];
		if (cmd == 'P') {
			char *t = unhex(line + 2);
			mm3_ui_print(ui, t);
			free(t);
		} else if (cmd == 'O') {
			int x, y, w, h, col;
			char hex[8000];
			char *t = NULL;
			if (sscanf(line + 2, "%d %d %d %d %d %7999s", &x, &y, &w, &h, &col, hex) == 6) {
				if (strcmp(hex, "-")) t = unhex(hex);
				mm3_ui_open_window(ui, x, y, w, h, col, t);
				free(t);
			}
		} else if (cmd == 'C') {
			mm3_ui_close_windows(ui, atoi(line + 2));
		}
		fwrite(ui->screen, 1, 64000, out);
	}
	fclose(out);
	return 0;
}
