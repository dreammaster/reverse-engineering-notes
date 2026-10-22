/* Yendorian Tales Book I reimplementation -- basic types.
 *
 * Fixed-width types only; game code never relies on sizeof(int) or struct
 * layout matching a file (fields are always read through Shared::Bytes). */

#ifndef YENDOR1_SHARED_TYPES_H
#define YENDOR1_SHARED_TYPES_H

#include <stddef.h>
#include <stdint.h>

namespace Yendor1 {
namespace Shared {

typedef uint8_t byte;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;

} // namespace Shared
} // namespace Yendor1

#endif
