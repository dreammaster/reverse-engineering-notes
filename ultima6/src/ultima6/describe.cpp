#include "ultima6/describe.h"

namespace Ultima6 {
namespace Ultima6 {
namespace Describe {

// ds:0765 of GAME.EXE
static const uint16_t STACKABLE[21] = {0x5A, 0x3F, 0x4D, 0x37, 0x38, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
									   0x47, 0x48, 0x151, 0x80, 0x81, 0x53, 0x14F, 0x59, 0x5B, 0x5C};

int quantityKind(int type) {
	if (type == 0x58)
		return 4;
	for (uint16_t t : STACKABLE)
		if (t == type)
			return 2;
	return 0;
}

std::string tileName(const World &world, int tile, bool plural) {
	const std::string &d = world.look.description(tile & (TileSet::COUNT - 1));
	std::string out;
	for (size_t i = 0; i < d.size();) {
		char c = d[i];
		if (c == '\\' || c == '/') {
			bool show = (c == '\\') == plural;
			++i;
			while (i < d.size() && ((d[i] >= 'a' && d[i] <= 'z') || (d[i] >= 'A' && d[i] <= 'Z'))) {
				if (show)
					out += d[i];
				++i;
			}
		} else {
			out += c;
			++i;
		}
	}
	return out;
}

std::string article(const World &world, int tile) {
	switch (world.flags.flags3[tile & (TileFlags::TILES - 1)] & 0xC0) {
	case 0x40:
		return "a ";
	case 0x80:
		return "an ";
	case 0xC0:
		return "the ";
	default:
		return "";
	}
}

std::string object(const World &world, const World::Obj &o) {
	int tile = world.baseTile.firstTile[o.type] + o.frame;
	int kind = quantityKind(o.type);
	if (kind) {
		int qty = kind == 4 ? o.quantity | (o.quality << 8) : o.quantity;
		return std::to_string(qty) + " " + tileName(world, tile, qty != 1);
	}
	return article(world, tile) + tileName(world, tile, false);
}

} // namespace Describe
} // namespace Ultima6
} // namespace Ultima6
