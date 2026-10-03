#include "lighting.h"

typedef struct {
    uint16_t start, end;
    int16_t gradient[LightingGradientSize];
} DayEntry;

static const DayEntry g_dayYendor2[] = {
    {0, 381, {-10, -9, -8, -7, -6, -5, -4}},
    {382, 385, {-10, -9, -8, -7, -6, -5, -4}},
    {386, 391, {-10, -9, -8, -7, -6, -5, -4}},
    {392, 397, {-9, -8, -7, -6, -5, -4, -3}},
    {398, 403, {-9, -8, -7, -6, -5, -4, -3}},
    {404, 409, {-8, -7, -6, -5, -4, -3, -2}},
    {410, 415, {-7, -6, -5, -4, -3, -2, -1}},
    {416, 421, {-7, -6, -5, -4, -3, -2, -1}},
    {422, 427, {-6, -5, -4, -3, -2, -1, 0}},
    {428, 433, {-6, -5, -4, -3, -2, -1, 0}},
    {434, 439, {-5, -4, -3, -2, -1, 0, 0}},
    {440, 445, {-4, -3, -2, -1, 0, 0, 0}},
    {446, 451, {-3, -2, -1, 0, 0, 0, 0}},
    {452, 457, {-2, -1, 0, 0, 0, 0, 0}},
    {458, 463, {-1, 0, 0, 0, 0, 0, 0}},
    {464, 469, {0, 0, 0, 0, 0, 0, 0}},
    {470, 475, {0, 0, 0, 0, 0, 0, 0}},
    {476, 481, {1, 0, 0, 0, 0, 0, 0}},
    {482, 1101, {1, 0, 0, 0, 0, 0, 0}},
    {1102, 1105, {1, 0, 0, 0, 0, 0, 0}},
    {1106, 1111, {0, 0, 0, 0, 0, 0, 0}},
    {1112, 1117, {0, 0, 0, 0, 0, 0, 0}},
    {1118, 1123, {-1, 0, 0, 0, 0, 0, 0}},
    {1124, 1129, {-2, -1, 0, 0, 0, 0, 0}},
    {1129, 1135, {-3, -2, -1, 0, 0, 0, 0}},
    {1136, 1141, {-4, -3, -2, -1, 0, 0, 0}},
    {1142, 1147, {-5, -4, -3, -2, -1, 0, 0}},
    {1148, 1153, {-6, -5, -4, -3, -2, -1, 0}},
    {1154, 1159, {-6, -5, -4, -3, -2, -1, 0}},
    {1159, 1165, {-7, -6, -5, -4, -3, -2, -1}},
    {1166, 1171, {-7, -6, -5, -4, -3, -2, -1}},
    {1172, 1177, {-8, -7, -6, -5, -4, -3, -2}},
    {1178, 1183, {-9, -8, -7, -6, -5, -4, -3}},
    {1184, 1189, {-9, -8, -7, -6, -5, -4, -3}},
    {1189, 1195, {-10, -9, -8, -7, -6, -5, -4}},
    {1196, 1201, {-10, -9, -8, -7, -6, -5, -4}},
    {1202, 1443, {-10, -9, -8, -7, -6, -5, -4}},
};

static const DayEntry g_dayYendor3[] = {
    {0, 381, {-10, -9, -8, -7, -6, -5, -4}},
    {382, 385, {-10, -9, -8, -7, -6, -5, -4}},
    {386, 391, {-10, -9, -8, -7, -6, -5, -4}},
    {392, 397, {-9, -8, -7, -6, -5, -4, -3}},
    {398, 403, {-9, -8, -7, -6, -5, -4, -3}},
    {404, 409, {-8, -7, -6, -5, -4, -3, -2}},
    {410, 415, {-7, -6, -5, -4, -3, -2, -1}},
    {416, 421, {-7, -6, -5, -4, -3, -2, -1}},
    {422, 427, {-6, -5, -4, -3, -2, -1, 0}},
    {428, 433, {-6, -5, -4, -3, -2, -1, 0}},
    {434, 439, {-5, -4, -3, -2, -1, 0, 0}},
    {440, 445, {-4, -3, -2, -1, 0, 0, 0}},
    {446, 451, {-3, -2, -1, 0, 0, 0, 0}},
    {452, 457, {-2, -1, 0, 0, 0, 0, 0}},
    {458, 463, {-1, 0, 0, 0, 0, 0, 0}},
    {464, 469, {0, 0, 0, 0, 0, 0, 0}},
    {470, 475, {0, 0, 0, 0, 0, 0, 0}},
    {476, 481, {0, 0, 0, 0, 0, 0, 0}},
    {482, 1101, {0, 0, 0, 0, 0, 0, 0}},
    {1102, 1105, {0, 0, 0, 0, 0, 0, 0}},
    {1106, 1111, {0, 0, 0, 0, 0, 0, 0}},
    {1112, 1117, {0, 0, 0, 0, 0, 0, 0}},
    {1118, 1123, {-1, 0, 0, 0, 0, 0, 0}},
    {1124, 1129, {-2, -1, 0, 0, 0, 0, 0}},
    {1129, 1135, {-3, -2, -1, 0, 0, 0, 0}},
    {1136, 1141, {-4, -3, -2, -1, 0, 0, 0}},
    {1142, 1147, {-5, -4, -3, -2, -1, 0, 0}},
    {1148, 1153, {-6, -5, -4, -3, -2, -1, 0}},
    {1154, 1159, {-6, -5, -4, -3, -2, -1, 0}},
    {1159, 1165, {-7, -6, -5, -4, -3, -2, -1}},
    {1166, 1171, {-7, -6, -5, -4, -3, -2, -1}},
    {1172, 1177, {-8, -7, -6, -5, -4, -3, -2}},
    {1178, 1183, {-9, -8, -7, -6, -5, -4, -3}},
    {1184, 1189, {-9, -8, -7, -6, -5, -4, -3}},
    {1189, 1195, {-10, -9, -8, -7, -6, -5, -4}},
    {1196, 1201, {-10, -9, -8, -7, -6, -5, -4}},
    {1202, 1443, {-10, -9, -8, -7, -6, -5, -4}},
};

static const int16_t kGradientA[LightingGradientSize] = {-12, -11, -10, -9, -8, -7, -6};
static const int16_t kGradientB[LightingGradientSize] = {-8, -7, -6, -5, -4, -3, -2};
static const int16_t kGradientC[LightingGradientSize] = {-7, -6, -5, -4, -3, -2, -1};

static const int16_t kAdjustment[LightingGradientSize][6] = {
    {10, 8, 5, 4, 3, 2}, {10, 8, 5, 4, 3, 2}, {9, 8, 5, 4, 3, 2}, {9, 7, 5, 4, 3, 2}, {8, 7, 5, 4, 3, 2}, {7, 6, 5, 4, 3, 2}, {6, 6, 5, 4, 3, 2},
};

unsigned lightingWallTorchTier(const uint16_t nearOverlayTypes[LightingNearCells], uint16_t facing) {
    for (unsigned i = 0; i < LightingNearCells; i++) {
        uint16_t type = nearOverlayTypes[i];
        bool match = (type == LightingWallTorchBase && (facing & 0x8000)) || (type == LightingWallTorchBase + 1 && (facing & 0x4000)) ||
                     (type == LightingWallTorchBase + 2 && (facing & 0x1000)) || (type == LightingWallTorchBase + 3 && (facing & 0x2000));
        if (match) {
            return i / 3 + 1;
        }
    }
    return 0;
}

void lightingComputeGradient(GameKind game, const LightingInput *in, unsigned wallTorchTier, int16_t out[LightingGradientSize],
                             bool *clockReset) {
    const int16_t *base = NULL;
    bool reset = false;
    if (game == GameYendor3) {
        if (in->flagsA & 0x8000) {
            base = kGradientA;
        } else if (in->flagsA & 0x2000) {
            base = kGradientC;
        }
    } else if (in->flagsA & 4) {
        base = kGradientA;
    } else if (in->flagsA & 2) {
        base = kGradientB;
    } else if (in->flagsA & 1) {
        base = kGradientC;
    }
    if (!base) {
        const DayEntry *table = game == GameYendor3 ? g_dayYendor3 : g_dayYendor2;
        unsigned count = game == GameYendor3 ? sizeof(g_dayYendor3) / sizeof(g_dayYendor3[0]) : sizeof(g_dayYendor2) / sizeof(g_dayYendor2[0]);
        uint16_t clock = in->clockMinutes;
        if (game == GameYendor3 && (in->flagsA & 0x4000)) {
            clock = 720;
        }
        const DayEntry *entry = NULL;
        for (unsigned i = 0; i < count; i++) {
            if ((int16_t)clock <= (int16_t)table[i].end) {
                entry = &table[i];
                break;
            }
        }
        if (!entry) {
            reset = true;
            entry = &table[0];
        }
        base = entry->gradient;
    }
    for (unsigned i = 0; i < LightingGradientSize; i++) {
        out[i] = base[i];
    }
    if (clockReset) {
        *clockReset = reset;
    }

    uint16_t f = in->flagsB;
    int tier = -1;
    if (f & (0x200 | 0x8)) {
        tier = 0;
    } else if (f & (0x400 | 0x10)) {
        tier = 1;
    } else if (f & (0x800 | 0x20)) {
        tier = 2;
    } else if ((f & (0x1000 | 0x40)) || wallTorchTier == 3) {
        tier = 3;
    } else if ((f & (0x2000 | 0x80)) || wallTorchTier == 2) {
        tier = 4;
    } else if ((f & (0x4000 | 0x100)) || wallTorchTier == 1) {
        tier = 5;
    }
    if (tier >= 0) {
        for (unsigned i = 0; i < LightingGradientSize; i++) {
            if (out[i] < 0) {
                int sum = out[i] + kAdjustment[i][tier];
                out[i] = (int16_t)(sum > 0 ? 0 : sum);
            }
        }
    }
}

void lightingViewportTable(const int16_t gradient[LightingGradientSize], int16_t out[LightingViewportCells]) {
    static const uint8_t kBand3[] = {4, 12, 14, 20, 24, 28, 34, 38, 42, 48, 50, 58};
    static const uint8_t kBand4[] = {13, 21, 23, 29, 33, 39, 41, 49};
    static const uint8_t kBand5[] = {22, 30, 32, 40};
    for (unsigned i = 0; i < LightingViewportCells; i++) {
        out[i] = gradient[2];
    }
    for (unsigned i = 0; i < sizeof(kBand3); i++) {
        out[kBand3[i]] = gradient[3];
    }
    for (unsigned i = 0; i < sizeof(kBand4); i++) {
        out[kBand4[i]] = gradient[4];
    }
    for (unsigned i = 0; i < sizeof(kBand5); i++) {
        out[kBand5[i]] = gradient[5];
    }
    out[31] = gradient[6];
}
