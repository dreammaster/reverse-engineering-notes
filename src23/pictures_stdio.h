#ifndef YENDOR23_PICTURES_STDIO_H
#define YENDOR23_PICTURES_STDIO_H

#include <stdint.h>

#include "pictures.h"

/*
 * PICTURES.VGA read through stdio with a small cache of recently used pictures. pictureFileGet has the signature of
 * viewrender.h's ViewPictureFn (the pointer stays valid until PictureFileCacheSlots further pictures are requested).
 */
enum { PictureFileCacheSlots = 24 };

typedef struct PictureFile PictureFile;

PictureFile *pictureFileOpen(const char *path, GameKind game);
void pictureFileClose(PictureFile *file);

/* Pixels of picture `id` of `category` (width * height bytes), or NULL if it does not exist or cannot be read. */
const uint8_t *pictureFileGet(void *file, unsigned category, unsigned id);

#endif
