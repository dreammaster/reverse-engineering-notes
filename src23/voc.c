#include "voc.h"

#include <string.h>

bool vocOpen(VocSound *sound, const uint8_t *data, size_t size) {
    static const char kSignature[] = "Creative Voice File\x1A";
    if (size < 26 || memcmp(data, kSignature, 20) != 0) {
        return false;
    }
    size_t position = (size_t)data[20] | ((size_t)data[21] << 8);
    if (position + 4 > size || data[position] != 1) {
        return false;
    }
    size_t length = (size_t)data[position + 1] | ((size_t)data[position + 2] << 8) | ((size_t)data[position + 3] << 16);
    if (length < 2 || position + 4 + length > size || data[position + 5] != 0 || data[position + 4] == 255) {
        return false;
    }
    sound->sampleRate = 1000000u / (256u - data[position + 4]);
    sound->samples = data + position + 6;
    sound->length = length - 2;
    size_t after = position + 4 + length;
    return after >= size || data[after] == 0;
}
