/*
 * Renders a music track of a game's WORLD.DAT to a 16-bit mono WAV file through the OPL2 synthesizer (opl.h, cmfplayer.h): a way to hear the parsed CMF music.
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o cmf_wav cmf_wav.c ../cmfplayer.c ../opl.c ../cmf.c ../audio.c -lm
 *   ./cmf_wav <2|3> <game dir> <track id> <out.wav> [max seconds, default 60]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "cmfplayer.h"

static void put32(FILE *f, unsigned v) {
    fputc((int)(v & 255), f);
    fputc((int)((v >> 8) & 255), f);
    fputc((int)((v >> 16) & 255), f);
    fputc((int)((v >> 24) & 255), f);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s <2|3> <game dir> <track id> <out.wav> [max seconds]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    unsigned id = (unsigned)atoi(argv[3]);
    unsigned maxSeconds = argc > 5 ? (unsigned)atoi(argv[5]) : 60;
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", argv[2]);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *world = malloc(size);
    if (!world || fread(world, 1, size, f) != size) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    fclose(f);
    uint32_t offset, length;
    static CmfPlayer player;
    if (!audioMusicTrack(game, id, &offset, &length) || offset + length > size || !cmfPlayerStart(&player, world + offset, length, 44100)) {
        fprintf(stderr, "no such track\n");
        return 1;
    }
    unsigned total = 44100 * maxSeconds;
    int16_t *pcm = malloc((size_t)total * 2);
    unsigned done = 0;
    while (done < total) {
        unsigned got = cmfPlayerRender(&player, pcm + done, total - done < 4096 ? total - done : 4096);
        if (got == 0) {
            break;
        }
        done += got;
    }
    /* normalise: the tracks use low MIDI velocities, so scale the peak to 70% of full scale */
    int peak = 1;
    for (unsigned i = 0; i < done; i++) {
        int a = pcm[i] < 0 ? -pcm[i] : pcm[i];
        peak = a > peak ? a : peak;
    }
    double gain = 23000.0 / peak;
    for (unsigned i = 0; i < done; i++) {
        pcm[i] = (int16_t)(pcm[i] * gain);
    }
    FILE *out = fopen(argv[4], "wb");
    if (!out) {
        return 1;
    }
    fwrite("RIFF", 1, 4, out);
    put32(out, 36 + done * 2);
    fwrite("WAVEfmt ", 1, 8, out);
    put32(out, 16);
    fputc(1, out); fputc(0, out); fputc(1, out); fputc(0, out);
    put32(out, 44100);
    put32(out, 44100 * 2);
    fputc(2, out); fputc(0, out); fputc(16, out); fputc(0, out);
    fwrite("data", 1, 4, out);
    put32(out, done * 2);
    fwrite(pcm, 2, done, out);
    fclose(out);
    printf("track %u: %u samples (%.1f s)%s\n", id, done, done / 44100.0, done == total ? " (cut at the limit)" : "");
    return 0;
}
