#include "ultima6/view.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// ds:05EA, 128 bytes: the distance table (8 x 8, rows of the difference in x) and, because the original indexes it past its end, the bytes that follow it
const int8_t DISTANCE[128] = {0, 1, 2, 3, 4, 5, 6, 7, 1, 1, 2, 3, 4, 5, 6, 7, 2, 2, 3, 4, 5, 6, 6, 7, 3, 3, 4, 4, 5, 6, 7, 8, 4, 4, 5, 5, 6, 7, 7, 8, 5, 5, 6, 6, 7, 7, 8, 9,
							  6, 6, 6, 7, 7, 8, 8, 9, 7, 7, 7, 8, 8, 9, 9, 10, 0, 0, 0, 1, 0, 0, -1, -1, 0, 0, -1, 0, 1, 1, 0, 0, -1, -1, -40, 1, 40, 40, -1, -1, -40, -40, 0, 0,
							  1, 10, 0, 0, 2, 2, 1, 5, 1, 1, 11, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// ds:062C, 0634, 063C: the eight steps round a square, each from the one before it (north, north-east, east, south-east, south, south-west, west, north-west) as changes in x, y and
// the index into the grids
const int8_t STEP_X[8] = {0, 1, 0, 0, -1, -1, 0, 0};
const int8_t STEP_Y[8] = {-1, 0, 1, 1, 0, 0, -1, -1};
const int STEP_CELL[8] = {-40, 1, 40, 40, -1, -1, -40, -40};
// ds:0644: which wall tile goes with which of the four neighbours are seen
const uint8_t WALL_TILE[16] = {0, 0, 1, 10, 0, 0, 2, 2, 1, 5, 1, 1, 11, 0, 2, 2};

} // namespace

int ViewBuilder::distance(int dx, int dy) const {
	int i = std::abs(dx) * 8 + std::abs(dy);
	return i < 128 ? DISTANCE[i] : 0;
}

// sub_18C8B: whether light and sight pass the square at the current index (word_4961C). Nothing does that has a thing on it that blocks (0x20); a square that is opaque (terrain
// flags2 0x08, or a thing with 0x10) lets it pass only on the squares next to the party in line with it; else a thing that is see-through (0x04) lets it pass, and terrain with
// flags2 0x04 stops it
bool ViewBuilder::passes(int x, int y) const {
	int f = _flags[_cell], t = _terrain[_cell];
	if (f & 0x20)
		return false;
	if ((_world.flags.flags2[t & (TileFlags::TILES - 1)] & 8) || (f & 0x10)) {
		int px = _ox + 5, py = _oy + 5;
		return (px == x && std::abs(py - y) < 2) || (py == y && std::abs(px - x) < 2);
	}
	if (f & 4)
		return true;
	return !(_world.flags.flags2[t & (TileFlags::TILES - 1)] & 4);
}

// sub_18D4F: a square joins the queue (and is marked as queued); a full queue drops it
void ViewBuilder::enqueue(int x, int y) {
	if (x < 0 || x >= 40 || y < 0 || y >= 40)
		return;
	_queueX[_tail] = (uint8_t)x;
	_queueY[_tail] = (uint8_t)y;
	++_tail;
	if (_tail == _head) {
		--_tail;
		return;
	}
	_flags[y * WIDTH + x] |= 0x40;
}

// sub_18DB1: light (or, in mode 1, the party's sight) spreads out from (x, y). The first squares are the one it starts on or, if that blocks, the ones east and south of it; each square
// taken from the queue gets 4 less its distance plus the strength of the light (at least 0 and at most 4) added to its light, is marked as seen if it is a view (mode 1 and 2), and if
// light passes it gives its eight neighbours that are not queued yet to the queue: in a view (1) those within one square of the window, in mode 2 all, for a light (0) those no farther
// than the strength plus 3 from where it started
void ViewBuilder::spread(int mode, int level, int y, int x) {
	_cell = y * WIDTH + x;
	if (passes(x, y)) {
		enqueue(x, y);
	} else {
		++_cell;
		if (passes(x + 1, y))
			enqueue(x + 1, y);
		_cell += 39;
		if (!passes(x, y + 1))
			return;
		enqueue(x, y + 1);
	}
	while (_tail != _head) {
		int px = _queueX[_head], py = _queueY[_head];
		++_head;
		_cell = py * WIDTH + px;
		int lv = 4 - distance(px - x, py - y) + level;
		lv = std::max(0, std::min(4, lv));
		_light[_cell] = (uint8_t)(_light[_cell] + lv);
		if (mode)
			_flags[_cell] |= 0x80;
		if (!passes(px, py))
			continue;
		int nx = px, ny = py;
		for (int k = 0; k < 8; ++k) {
			nx += STEP_X[k];
			ny += STEP_Y[k];
			_cell += STEP_CELL[k];
			if (_flags[_cell] & 0x40)
				continue;
			bool take;
			if (mode == 1)
				take = !(_ox - 1 > nx || _ox + 11 < nx || _oy - 1 > ny || _oy + 11 < ny);
			else if (mode == 2)
				take = true;
			else
				take = !(distance(nx - x, ny - y) > level + 3);
			if (take)
				enqueue(nx, ny);
		}
	}
}

void ViewBuilder::build(int px, int py, int pz, int ambient, ViewGrid &out) {
	const TileFlags &fl = _world.flags;
	const int mapMask = pz == 0 ? 0x3FF : 0xFF;
	std::memset(_flags, 0, sizeof _flags);
	std::memset(_light, 0, sizeof _light);
	std::memset(_terrain, 0, sizeof _terrain);
	_tail = _head = 0;

	// build_terrain_cache: the window of the map, 24 x 24 squares in the grids' columns and rows 8 to 31
	int cx = (px - 16) & 0x3F8, cy = (py - 16) & 0x3F8;
	_ox = (px - cx - 5) & 0x1F;
	_oy = (py - cy - 5) & 0x1F;
	for (int gy = 8; gy < 32; ++gy)
		for (int gx = 8; gx < 32; ++gx)
			_terrain[gy * WIDTH + gx] = _world.map.tile(pz, (cx + gx) & mapMask, (cy + gy) & mapMask);

	// sub_18F86 19011-191DE: what stands on the squares (the things of the level that lie in the window, by their own coordinates: across the edge of the map there are none)
	int ox0 = (px - 16) & ~7, oy0 = (py - 16) & ~7;
	for (const World::Obj &o : _world.objects) {
		if (o.deleted || (o.status & 0x18) || o.z != pz)
			continue;
		int x = o.x & mapMask & 0x3FF, y = o.y & mapMask & 0x3FF;
		if (x < ox0 + 8 || x > ox0 + 0x1F || y < oy0 + 8 || y > oy0 + 0x1F)
			continue;
		int rx = x - ox0, ry = y - oy0;
		int si = ry * WIDTH + rx;
		int tile = (_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1);
		uint8_t f1 = fl.flags1[tile], f2 = fl.flags2[tile], f3 = fl.flags3[tile];
		if (f2 & 8)
			_flags[si] |= 0x10;
		else if (f2 & 4)
			_flags[si] |= 0x20;
		else if (f3 & 0x20)
			_flags[si] |= 4;
		if (fl.flags2[(tile - 1) & (TileFlags::TILES - 1)] & 4) {
			if ((f2 & 0x40) && ry != 0)
				_flags[si - 40] |= 0x20;
			if ((f2 & 0x80) && rx != 0)
				_flags[si - 1] |= 0x20;
		}
		if (f1 & 4) {
			_flags[si] |= 8;
			if ((f2 & 0x40) && ry != 0)
				_flags[si - 40] |= 8;
			if ((f2 & 0x80) && rx != 0)
				_flags[si - 1] |= 8;
		}
		if (f2 & 3) {
			int v = f2 & 3;
			if ((_flags[si] & 3) < v)
				_flags[si] = (uint8_t)((_flags[si] & 0xFC) | v);
		}
	}

	// the party's own sight and light, then the lights of the things in sight when it is not bright day
	spread(1, ambient, _oy + 5, _ox + 5);
	if (ambient < 7) {
		int si = 8 * WIDTH + 8, t = 8 * WIDTH + 8;
		for (int row = 8; row < 32; ++row) {
			int di = 8;
			while (di < 32) {
				bool skip = false;
				if ((_flags[si] & 3) && (_flags[si] & 0x80)) {
					int tile = _terrain[t];
					if ((fl.flags1[tile & (TileFlags::TILES - 1)] & 4) && !(_flags[si + 40] & 0x80)) {
						skip = true; // the original goes on to the next column without moving its two pointers on
					} else {
						for (int xx = di - 3; xx <= di + 3; ++xx)
							for (int yy = row - 3; yy <= row + 3; ++yy)
								_flags[yy * WIDTH + xx] &= 0xBF;
						spread(0, (_flags[si] & 3) - 4, row, di);
					}
				}
				if (!skip) {
					++si;
					++t;
				}
				++di;
			}
			si += 16;
			t += 16;
		}
	}

	// sub_18F86 192C0-19490: the terrain of the 11 x 11 squares
	for (int r = 0; r < ViewGrid::CELLS; ++r) {
		for (int c = 0; c < ViewGrid::CELLS; ++c) {
			int idx = (_oy + r) * WIDTH + _ox + c;
			int tile;
			if (!(_flags[idx] & 0x80)) {
				tile = ViewGrid::UNSEEN;
			} else if (_light[idx] == 0) {
				tile = ViewGrid::DARK;
			} else {
				tile = _terrain[idx];
				int tf = fl.flags1[tile & (TileFlags::TILES - 1)];
				if ((tf & 0xF) == 6) {
					int family = tile & 0xF0;
					if (family < 0x90)
						family = 0x90;
					int seen = 0;
					if (_flags[idx - 40] & 0x80)
						seen |= 8;
					if (_flags[idx + 1] & 0x80)
						seen |= 4;
					if (_flags[idx + 40] & 0x80)
						seen |= 2;
					if (_flags[idx - 1] & 0x80)
						seen |= 1;
					if (seen != 0xF) {
						int sides = tf >> 4;
						if (sides & 4) {
							int n = _terrain[idx - 40];
							int nf = fl.flags1[n & (TileFlags::TILES - 1)];
							if (!(_flags[idx - 40] & 8) && (!(nf & 4) || !(nf & 0x20)))
								sides &= ~8;
						}
						if (sides & 2) {
							int w = _terrain[idx - 1];
							int wf = fl.flags1[w & (TileFlags::TILES - 1)];
							if (!(_flags[idx - 1] & 8) && (!(wf & 4) || !(wf & 0x40)))
								sides &= ~1;
						}
						if (sides == 7 || sides == 0xB || sides > 0xC || sides == seen) {
							sides &= seen;
							if (sides == 3 || sides == 0xC)
								tile = (family >> 3) + WALL_TILE[sides] + 0xEE;
							else
								tile = WALL_TILE[sides] + family;
						}
					}
				}
			}
			out.tile[r][c] = (uint16_t)tile;
			out.light[r][c] = _light[idx];
		}
	}
	// sub_1CB82 1CE97-1CF8E: a wall that has an unseen square to its east or south does not show what is on it
	for (int r = 0; r < ViewGrid::CELLS; ++r)
		for (int c = 0; c < ViewGrid::CELLS; ++c) {
			int idx = (_oy + r) * WIDTH + _ox + c;
			bool wall = fl.flags1[out.tile[r][c] & (TileFlags::TILES - 1)] & 4;
			out.hideThings[r][c] = wall && (!(_flags[idx + 1] & 0x80) || !(_flags[idx + 40] & 0x80));
		}
}

} // namespace Ultima6
} // namespace Ultima6
