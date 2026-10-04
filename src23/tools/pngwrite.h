#ifndef YENDOR23_TOOLS_PNGWRITE_H
#define YENDOR23_TOOLS_PNGWRITE_H

/*
 * A minimal PNG writer for the tools: an 8-bit indexed picture with a 6-bit VGA DAC palette (the game's, scaled to 8 bits), optionally
 * enlarged by an integer `scale`, written with stored (uncompressed) deflate blocks so no zlib is needed.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crcTable[256];

static void crcInit(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        }
        crcTable[n] = c;
    }
}

static uint32_t crc(uint32_t c, const uint8_t *p, size_t n) {
    c = ~c;
    while (n--) {
        c = crcTable[(c ^ *p++) & 0xFF] ^ (c >> 8);
    }
    return ~c;
}

static void be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void chunk(FILE *f, const char *tag, const uint8_t *data, size_t n) {
    uint8_t head[8];
    be32(head, (uint32_t)n);
    memcpy(head + 4, tag, 4);
    fwrite(head, 1, 8, f);
    fwrite(data, 1, n, f);
    uint32_t c = crc(0, (const uint8_t *)tag, 4);
    c = crc(c, data, n);
    uint8_t tail[4];
    be32(tail, c);
    fwrite(tail, 1, 4, f);
}

static bool writePng(const char *path, const uint8_t *indexed, const uint8_t *palette, unsigned w, unsigned h, unsigned scale) {
    crcInit();
    unsigned ow = w * scale, oh = h * scale;
    size_t rowBytes = 1 + (size_t)ow * 3, rawSize = rowBytes * oh;
    uint8_t *raw = malloc(rawSize);
    for (unsigned y = 0; y < oh; y++) {
        uint8_t *row = raw + y * rowBytes;
        row[0] = 0;
        for (unsigned x = 0; x < ow; x++) {
            const uint8_t *c = &palette[indexed[(y / scale) * w + x / scale] * 3];
            for (int k = 0; k < 3; k++) {
                row[1 + x * 3 + k] = (uint8_t)(c[k] * 4 + c[k] / 16);
            }
        }
    }
    size_t blocks = (rawSize + 65534) / 65535;
    uint8_t *z = malloc(rawSize + blocks * 5 + 6);
    size_t zn = 0;
    z[zn++] = 0x78;
    z[zn++] = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < rawSize; i++) {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }
    for (size_t off = 0; off < rawSize; off += 65535) {
        size_t n = rawSize - off < 65535 ? rawSize - off : 65535;
        z[zn++] = off + n >= rawSize;
        z[zn++] = (uint8_t)n;
        z[zn++] = (uint8_t)(n >> 8);
        z[zn++] = (uint8_t)~n;
        z[zn++] = (uint8_t)(~n >> 8);
        memcpy(z + zn, raw + off, n);
        zn += n;
    }
    be32(z + zn, (b << 16) | a);
    zn += 4;
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    be32(ihdr, ow);
    be32(ihdr + 4, oh);
    ihdr[8] = 8;
    ihdr[9] = 2;
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, zn);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);
    return true;
}


#endif
