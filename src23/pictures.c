#include "pictures.h"

/* From ida_scripts/dump_picture_dir.py; counts are (next base - base) / (width * height), the last up to the file's end
 * (12,550,618 bytes in Chapter 2, 17,221,076 in Chapter 3). */
static const PictureCategory kYendor2[PictureCategoryCount] = {
    {318, 198, 0, 15},        {210, 105, 944460, 101},   {140, 155, 3171510, 215},  {190, 110, 7837010, 162}, {224, 74, 11222810, 18},
    {224, 62, 11521178, 12},  {56, 136, 11687834, 55},   {32, 32, 12106714, 270},   {16, 16, 12383194, 510},  {8, 8, 12513754, 576},
};

static const PictureCategory kYendor3[PictureCategoryCount] = {
    {318, 198, 0, 23},        {210, 105, 1448172, 156},  {140, 155, 4887972, 270},  {190, 110, 10746972, 238}, {224, 74, 15721172, 28},
    {224, 62, 16185300, 14},  {56, 136, 16379732, 70},   {32, 32, 16912852, 180},   {16, 16, 17097172, 340},   {8, 8, 17184212, 576},
};

uint32_t pictureMasterPaletteOffset(GameKind game) {
    return game == GameYendor3 ? 0x95BDA : 0x8270A;
}

const PictureCategory *pictureCategory(GameKind game, unsigned category) {
    if (category >= PictureCategoryCount) {
        return NULL;
    }
    return (game == GameYendor3 ? kYendor3 : kYendor2) + category;
}

bool pictureLocate(GameKind game, unsigned category, unsigned id, uint32_t *offset, uint32_t *size) {
    const PictureCategory *c = pictureCategory(game, category);
    if (!c || id >= c->count) {
        return false;
    }
    *size = (uint32_t)c->width * c->height;
    *offset = c->base + id * *size;
    return true;
}
