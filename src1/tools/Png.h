/* Minimal PNG writer (stored deflate blocks, no zlib dependency) for the
 * command line tools. Not part of the engine. */

#ifndef YENDOR1_TOOLS_PNG_H
#define YENDOR1_TOOLS_PNG_H

#include <stdint.h>
#include <stdio.h>

namespace Yendor1 {
namespace Tools {

/** Write an 8-bit RGB image. Returns false on I/O failure. */
bool writePng(const char *path, int width, int height, const uint8_t *rgb);

} // namespace Tools
} // namespace Yendor1

#endif
