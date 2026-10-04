#include "cmf.h"

#include <string.h>

static unsigned u16(const uint8_t *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

bool cmfOpen(CmfTrack *track, const uint8_t *data, size_t size) {
    if (size < 40 || memcmp(data, "CTMF", 4) != 0) {
        return false;
    }
    unsigned instrumentOffset = u16(data + 6), musicOffset = u16(data + 8);
    memset(track, 0, sizeof(*track));
    track->data = data;
    track->size = size;
    track->version = u16(data + 4);
    track->ticksPerQuarter = u16(data + 10);
    track->ticksPerSecond = u16(data + 12);
    memcpy(track->channelsInUse, data + 20, 16);
    track->instrumentCount = u16(data + 36);
    track->tempo = u16(data + 38);
    if (instrumentOffset + (size_t)track->instrumentCount * CmfInstrumentSize > size || musicOffset > size || musicOffset < instrumentOffset) {
        return false;
    }
    track->instruments = data + instrumentOffset;
    track->music = data + musicOffset;
    track->musicSize = size - musicOffset;
    return true;
}

void cmfReaderStart(CmfReader *reader, const CmfTrack *track) {
    reader->position = track->music;
    reader->end = track->music + track->musicSize;
    reader->runningStatus = 0;
}

static bool readVlq(CmfReader *r, uint32_t *value) {
    *value = 0;
    for (unsigned i = 0; i < 4; i++) {
        if (r->position >= r->end) {
            return false;
        }
        uint8_t b = *r->position++;
        *value = (*value << 7) | (b & 0x7F);
        if (!(b & 0x80)) {
            return true;
        }
    }
    return false;
}

bool cmfReadEvent(CmfReader *r, CmfEvent *e) {
    if (r->position >= r->end) {
        return false;
    }
    memset(e, 0, sizeof(*e));
    if (!readVlq(r, &e->delta) || r->position >= r->end) {
        return false;
    }
    uint8_t status = *r->position;
    if (status & 0x80) {
        r->position++;
        if (status < 0xF0) {
            r->runningStatus = status;
        }
    } else {
        status = r->runningStatus;
        if (!(status & 0x80)) {
            return false;
        }
    }
    e->status = status;
    if (status == 0xFF) {
        if (r->position >= r->end) {
            return false;
        }
        e->metaType = *r->position++;
        uint32_t length;
        if (!readVlq(r, &length) || length > (uint32_t)(r->end - r->position)) {
            return false;
        }
        e->payload = r->position;
        e->payloadLength = length;
        r->position += length;
        if (e->metaType == 0x2F) {
            r->end = r->position; /* nothing follows the end of track */
        }
        return true;
    }
    if (status == 0xF0 || status == 0xF7) {
        uint32_t length;
        if (!readVlq(r, &length) || length > (uint32_t)(r->end - r->position)) {
            return false;
        }
        e->payload = r->position;
        e->payloadLength = length;
        r->position += length;
        return true;
    }
    unsigned type = status & 0xF0, dataBytes = (type == 0xC0 || type == 0xD0) ? 1 : 2;
    if ((size_t)(r->end - r->position) < dataBytes) {
        return false;
    }
    e->data1 = *r->position++;
    if (dataBytes == 2) {
        e->data2 = *r->position++;
    }
    return true;
}
