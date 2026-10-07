#include "cmfplayer.h"

#include <math.h>
#include <string.h>

static void setFrequency(CmfPlayer *p, unsigned voice, bool keyOn) {
    unsigned channel = p->voice[voice].channel;
    double bend = (double)p->bend[channel] / 8192.0 * 2.0 + (double)p->fine[channel] / 128.0;
    double freq = 440.0 * pow(2.0, ((double)p->voice[voice].note - 69.0 + bend) / 12.0);
    unsigned block = 0;
    double fnum = freq * (double)(1u << (20 - block)) / OplChipRate;
    while (fnum >= 1024.0 && block < 7) {
        block++;
        fnum = freq * (double)(1u << (20 - block)) / OplChipRate;
    }
    unsigned f = fnum > 1023.0 ? 1023u : (unsigned)(fnum + 0.5);
    oplWrite(&p->opl, (uint8_t)(0xA0 + voice), (uint8_t)(f & 0xFF));
    oplWrite(&p->opl, (uint8_t)(0xB0 + voice), (uint8_t)((keyOn ? 0x20 : 0) | (block << 2) | (f >> 8)));
}

static unsigned carrierLevel(unsigned patchLevel, unsigned velocity, unsigned volume) {
    /* attenuation grows as velocity and volume fall: total level in 0.75 dB steps */
    unsigned loud = velocity * volume / 127;
    unsigned level = 63 - ((63 - patchLevel) * loud) / 127;
    return level > 63 ? 63 : level;
}

static void loadPatch(CmfPlayer *p, unsigned voice, unsigned channel, unsigned velocity) {
    static const uint8_t kMod[] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
    unsigned m = kMod[voice], c = m + 3;
    unsigned program = p->program[channel];
    const uint8_t *ins = program < p->track.instrumentCount ? p->track.instruments + program * CmfInstrumentSize : NULL;
    static const uint8_t kSilent[16] = {0};
    if (!ins) {
        ins = kSilent;
    }
    oplWrite(&p->opl, (uint8_t)(0x20 + m), ins[0]);
    oplWrite(&p->opl, (uint8_t)(0x20 + c), ins[1]);
    oplWrite(&p->opl, (uint8_t)(0x40 + m), ins[2]);
    oplWrite(&p->opl, (uint8_t)(0x40 + c), (uint8_t)((ins[3] & 0xC0) | carrierLevel(ins[3] & 63, velocity, p->volume[channel])));
    oplWrite(&p->opl, (uint8_t)(0x60 + m), ins[4]);
    oplWrite(&p->opl, (uint8_t)(0x60 + c), ins[5]);
    oplWrite(&p->opl, (uint8_t)(0x80 + m), ins[6]);
    oplWrite(&p->opl, (uint8_t)(0x80 + c), ins[7]);
    oplWrite(&p->opl, (uint8_t)(0xE0 + m), ins[8]);
    oplWrite(&p->opl, (uint8_t)(0xE0 + c), ins[9]);
    oplWrite(&p->opl, (uint8_t)(0xC0 + voice), ins[10]);
}

static void noteOff(CmfPlayer *p, unsigned channel, unsigned note) {
    for (unsigned v = 0; v < CmfVoiceCount; v++) {
        if (p->voice[v].active && p->voice[v].channel == channel && p->voice[v].note == note) {
            setFrequency(p, v, false);
            p->voice[v].active = false;
        }
    }
}

static void noteOn(CmfPlayer *p, unsigned channel, unsigned note, unsigned velocity) {
    unsigned best = 0;
    bool found = false;
    for (unsigned v = 0; v < CmfVoiceCount; v++) {
        if (!p->voice[v].active) {
            best = v;
            found = true;
            break;
        }
    }
    if (!found) {
        for (unsigned v = 1; v < CmfVoiceCount; v++) {
            if (p->voice[v].age < p->voice[best].age) {
                best = v;
            }
        }
        oplWrite(&p->opl, (uint8_t)(0xB0 + best), 0);
    }
    p->voice[best].active = true;
    p->voice[best].channel = (uint8_t)channel;
    p->voice[best].note = (uint8_t)note;
    p->voice[best].age = ++p->clock;
    oplWrite(&p->opl, (uint8_t)(0xB0 + best), 0); /* a fresh key on */
    loadPatch(p, best, channel, velocity);
    setFrequency(p, best, true);
}

static void handle(CmfPlayer *p, const CmfEvent *e) {
    unsigned channel = e->status & 15;
    switch (e->status & 0xF0) {
    case 0x90:
        if (e->data2) {
            noteOn(p, channel, e->data1, e->data2);
        } else {
            noteOff(p, channel, e->data1);
        }
        break;
    case 0x80:
        noteOff(p, channel, e->data1);
        break;
    case 0xB0:
        if (e->data1 == 7) {
            p->volume[channel] = e->data2;
        } else if (e->data1 == 0x68) {
            p->fine[channel] = (int8_t)e->data2;
        } else if (e->data1 == 0x69) {
            p->fine[channel] = (int8_t)-(int)e->data2;
        }
        break;
    case 0xC0:
        p->program[channel] = e->data1;
        break;
    case 0xE0:
        p->bend[channel] = (int16_t)(((e->data2 << 7) | e->data1) - 8192);
        for (unsigned v = 0; v < CmfVoiceCount; v++) {
            if (p->voice[v].active && p->voice[v].channel == channel) {
                setFrequency(p, v, true);
            }
        }
        break;
    case 0xF0:
        if (e->status == 0xFF && e->metaType == 0x51 && e->payloadLength == 3) {
            unsigned micros = ((unsigned)e->payload[0] << 16) | ((unsigned)e->payload[1] << 8) | e->payload[2];
            p->samplesPerTick = (double)micros * p->sampleRate / 1e6 / p->track.ticksPerQuarter;
        }
        break;
    default:
        break;
    }
}

bool cmfPlayerStart(CmfPlayer *p, const uint8_t *data, size_t size, unsigned sampleRate) {
    memset(p, 0, sizeof(*p));
    if (!cmfOpen(&p->track, data, size) || p->track.ticksPerQuarter == 0) {
        return false;
    }
    cmfReaderStart(&p->reader, &p->track);
    oplReset(&p->opl, sampleRate);
    p->sampleRate = sampleRate;
    unsigned tempo = p->track.tempo ? p->track.tempo : 120;
    p->samplesPerTick = 60.0 * sampleRate / ((double)tempo * p->track.ticksPerQuarter);
    for (unsigned c = 0; c < 16; c++) {
        p->volume[c] = 127;
    }
    oplWrite(&p->opl, 0x01, 0x20);
    p->haveEvent = cmfReadEvent(&p->reader, &p->pending);
    p->finished = false;
    p->samplesUntilEvent = p->haveEvent ? p->pending.delta * p->samplesPerTick : 0;
    return true;
}

unsigned cmfPlayerRender(CmfPlayer *p, int16_t *out, unsigned count) {
    unsigned produced = 0;
    memset(out, 0, (size_t)count * sizeof(int16_t));
    /* the chip runs at its own rate: this renders at the player's sample rate directly (the synth uses it for all its times) */
    while (produced < count) {
        while (p->haveEvent && p->samplesUntilEvent <= 0) {
            handle(p, &p->pending);
            p->haveEvent = cmfReadEvent(&p->reader, &p->pending);
            if (p->haveEvent) {
                p->samplesUntilEvent += p->pending.delta * p->samplesPerTick;
            }
        }
        if (!p->haveEvent) {
            if (!p->finished) {
                p->finished = true;
                for (unsigned v = 0; v < CmfVoiceCount; v++) {
                    if (p->voice[v].active) {
                        setFrequency(p, v, false);
                        p->voice[v].active = false;
                    }
                }
                p->tail = p->sampleRate * 2;
            }
            if (p->tail == 0) {
                break;
            }
            unsigned run = count - produced < p->tail ? count - produced : p->tail;
            oplGenerate(&p->opl, out + produced, run);
            produced += run;
            p->tail -= run;
            continue;
        }
        unsigned chunk = count - produced;
        if (p->haveEvent && p->samplesUntilEvent < chunk) {
            chunk = (unsigned)ceil(p->samplesUntilEvent);
            if (chunk == 0) {
                chunk = 1;
            }
        }
        oplGenerate(&p->opl, out + produced, chunk);
        produced += chunk;
        p->samplesUntilEvent -= chunk;
    }
    return produced;
}
