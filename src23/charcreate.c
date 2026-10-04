#include "charcreate.h"

unsigned charCreateBodyPicture(unsigned gender, unsigned portrait) {
    return 2 * (portrait - 1) + (gender == 1 ? 0 : 1);
}

unsigned charCreateFacePicture(unsigned gender, unsigned portrait) {
    return charCreateBodyPicture(gender, portrait) + CharCreateFaceBase;
}

static void put16(uint8_t *record, unsigned offset, unsigned value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

void charCreateChoosePortrait(uint8_t *record, unsigned gender, unsigned portrait) {
    put16(record, 0x14, charCreateBodyPicture(gender, portrait));
    put16(record, 0x12, charCreateFacePicture(gender, portrait));
}

void charCreatePortraitGridDraw(const ViewRenderer *r, unsigned gender) {
    for (unsigned row = 0; row < 3; row++) {
        for (unsigned column = 0; column < 3; column++) {
            viewDrawPicture(r, 7, charCreateFacePicture(gender, row * 3 + column + 1), 8 + 33 * (int)column, 42 + 33 * (int)row, false, 0);
        }
    }
}
