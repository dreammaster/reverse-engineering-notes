#include "wiz/monster_art.h"
#include "wiz/surface.h"

namespace wiz {

void blitPortrait(Surface &dst, Bytes rec, int dx, int dy, u8 fg, int bg) {
    if (rec.n < 512) return;
    for (int trow = 0; trow < 5; ++trow)
        for (int tcol = 0; tcol < 6; ++tcol) {
            const u8 *tile = rec.p + (trow * 6 + tcol) * 16;   // 8 rows x 2 B
            for (int y = 0; y < 8; ++y) {
                unsigned bits = (unsigned(tile[y * 2]) << 8) | tile[y * 2 + 1];
                for (int x = 0; x < 16; ++x) {
                    int px = dx + tcol * 16 + x, py = dy + trow * 8 + y;
                    if ((bits >> (15 - x)) & 1)   dst.set(px, py, fg);
                    else if (bg >= 0)             dst.set(px, py, u8(bg));
                }
            }
        }
}

} // namespace wiz
