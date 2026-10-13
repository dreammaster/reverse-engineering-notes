#include "pcx.h"

bool pcxDecode(const uint8_t *data, size_t size, uint8_t *pixels, size_t capacity, unsigned *width, unsigned *height, uint8_t palette[PcxPaletteSize]) {
    if (size < PcxHeaderSize + PcxPaletteSize + 1 || data[0] != 0x0A || data[2] != 1 || data[3] != 8 || data[size - PcxPaletteSize - 1] != 0x0C) {
        return false;
    }
    unsigned bytesPerLine = (unsigned)(data[0x42] | (data[0x43] << 8));
    unsigned rows = (unsigned)(data[10] | (data[11] << 8)) + 1u - (unsigned)(data[6] | (data[7] << 8));
    size_t total = (size_t)bytesPerLine * rows;
    if (bytesPerLine == 0 || rows == 0 || total > capacity) {
        return false;
    }
    size_t at = PcxHeaderSize, end = size - PcxPaletteSize - 1, out = 0;
    while (at < end && out < total) {
        unsigned byte = data[at++];
        unsigned count = 1;
        if (byte >= 0xC0) {
            count = byte - 0xC0;
            if (at >= end) {
                break;
            }
            byte = data[at++];
        }
        for (; count > 0 && out < total; count--) {
            pixels[out++] = (uint8_t)byte;
        }
    }
    while (out < total) { /* a short file: the rest stays black */
        pixels[out++] = 0;
    }
    for (unsigned i = 0; i < PcxPaletteSize; i++) {
        palette[i] = (uint8_t)(data[size - PcxPaletteSize + i] >> 2);
    }
    *width = bytesPerLine;
    *height = rows;
    return true;
}

bool pcxLogoLocation(const ExeData *exe, GameKind game, size_t *offset, size_t *size) {
    if (game != GameYendor3) {
        return false;
    }
    size_t at = (size_t)exeDataU16(exe, 0xB20B) | ((size_t)exeDataU16(exe, 0xB20D) << 16);
    if (at == 0) {
        return false;
    }
    *offset = at;
    *size = PcxLogoSize;
    return true;
}
