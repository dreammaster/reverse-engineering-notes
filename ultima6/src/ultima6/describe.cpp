#include "ultima6/describe.h"

namespace Ultima6 {
namespace Ultima6 {
namespace Describe {

// ds:0765 of GAME.EXE
static const uint16_t STACKABLE[21] = {0x5A, 0x3F, 0x4D, 0x37, 0x38, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
									   0x47, 0x48, 0x151, 0x80, 0x81, 0x53, 0x14F, 0x59, 0x5B, 0x5C};

// Martian Dreams (ds:0DD6 of its GAME.EXE, 21 types) and Savage Empire (ds:0806, 48 types) have lists of their own; neither has a thing with a word quantity (Ultima VI's gold)
static const uint16_t MD_STACKABLE[21] = {0x39, 0x3A, 0x3B, 0x3C, 0x3F, 0x40, 0x7C, 0x6B, 0x6D, 0x83, 0x49, 0x4A, 0x4B, 0x4D, 0x1C1, 0x18, 0x84, 0x9E, 0x7A, 0x4C, 0x144};
static const uint16_t SE_STACKABLE[48] = {0x24, 0x2D, 0x29, 0x36, 0x37, 0x38, 0x48, 0x46, 0x49, 0x4A, 0x66, 0x67, 0x68, 0x69, 0x6C, 0x6D, 0x6E, 0xD2, 0x84, 0x2C, 0x85, 0x73, 0x78, 0x7B,
										  0x81, 0x89, 0xB4, 0x82, 0xC0, 0x86, 0x8B, 0x8C, 0xBF, 0xCC, 0xCE, 0xD6, 0x5D, 0x30, 0x0A, 0x2F, 0x1A, 0x89, 0xF0, 0xD3, 0xFC, 0x1B, 0xD0, 0x45};

int quantityKind(const World &world, int type) {
	if (world.game == GameId::MartianDreams) {
		for (uint16_t t : MD_STACKABLE)
			if (t == type)
				return 2;
		return 0;
	}
	if (world.game == GameId::SavageEmpire) {
		for (uint16_t t : SE_STACKABLE)
			if (t == type)
				return 2;
		return 0;
	}
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
	int kind = quantityKind(world, o.type);
	if (kind) {
		int qty = kind == 4 ? o.quantity | (o.quality << 8) : o.quantity;
		return std::to_string(qty) + " " + tileName(world, tile, qty != 1);
	}
	return article(world, tile) + tileName(world, tile, false);
}

} // namespace Describe
} // namespace Ultima6
} // namespace Ultima6
