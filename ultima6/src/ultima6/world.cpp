#include "ultima6/world.h"

#include "files/lib_n.h"
#include "files/shape.h"
#include "files/lzw.h"

#include "files/portrait.h"
#include "ultima6/describe.h"
#include "ultima6/stats_view.h"
#include "ultima6/gypsy.h"

#include <algorithm>
#include <cstring>
#include <functional>

namespace Ultima6 {
namespace Ultima6 {

static uint32_t positionKey(int x, int y, int z) {
	return ((uint32_t)z << 20) | ((uint32_t)y << 10) | (uint32_t)x;
}

bool World::loadBlock(const Bytes &data, int level) {
	ObjBlock block;
	if (!block.load(data))
		return false;
	size_t base = objects.size();
	for (size_t i = 0; i < block.objects.size(); ++i) {
		const ObjRecord &r = block.objects[i];
		Obj o;
		o.status = r.status;
		o.x = r.x;
		o.y = r.y;
		o.z = r.z;
		o.type = r.type();
		o.frame = r.frame();
		o.quantity = r.quantity;
		o.quality = r.quality;
		o.actor = -1;
		o.parent = -1;
		if (r.location() == ObjRecord::IN_CONTAINER)
			o.parent = (int)(base + r.containerIndex());
		else if (r.location() != ObjRecord::ON_MAP)
			o.parent = r.x; // owning actor
		objects.push_back(o);
	}
	(void)level;
	return true;
}

bool World::load(const std::string &dir, const NewCharacter *newGame, const std::string &saveDir) {
	Bytes d, d2, d3, d4;
	if (!readFile(joinPath(dir, "TILEFLAG"), d) || !flags.load(d))
		return fail("TILEFLAG");
	if (!readFile(joinPath(dir, "BASETILE"), d) || !baseTile.load(d))
		return fail("BASETILE");
	const bool spinoff = game != GameId::Ultima6;
	Bytes mapTiles, maskType, objTiles, index;
	Bytes raw;
	if (!spinoff) {
		if (!readFile(joinPath(dir, "U6PAL"), d) || !palette.load(d))
			return fail("U6PAL");
		if (!readFile(joinPath(dir, "U6.CH"), d) || !font.load(d))
			return fail("U6.CH");
		if (!paper.load(joinPath(dir, "PAPER.BMP")))
			return fail("PAPER.BMP");
		if (!readFile(joinPath(dir, "MAPTILES.VGA"), raw) || !lzwDecompress(raw, 0, mapTiles))
			return fail("MAPTILES.VGA");
		if (!readFile(joinPath(dir, "MASKTYPE.VGA"), raw) || !lzwDecompress(raw, 0, maskType))
			return fail("MASKTYPE.VGA");
		if (!readFile(joinPath(dir, "OBJTILES.VGA"), objTiles))
			return fail("OBJTILES.VGA");
	} else {
		// Martian Dreams and Savage Empire: their palette files hold two palettes of 768 bytes, the pixel data is in one-item archives (docs/spinoffs.md), the font is
		// not read yet (the glyphs stay blank) and there is no PAPER.BMP: the play screen has no frame yet
		if (!readFile(joinPath(dir, game == GameId::MartianDreams ? "MDPAL" : "SEPAL"), d) || !palette.load(d))
			return fail("palette");
		std::memset(font.glyph, 0, sizeof font.glyph);
		{
			// FONTS.LZC (MD) / SAVAGE.FNT (SE): one item, a WOU font (see Font::loadWou)
			LibN fontLib;
			Bytes fontData;
			if (readFile(joinPath(dir, game == GameId::MartianDreams ? "FONTS.LZC" : "SAVAGE.FNT"), raw) && fontLib.load(raw, true) && fontLib.get(0, fontData))
				font.loadWou(fontData);
		}
		paper.width = 320;
		paper.height = 200;
		paper.pixels.assign(320 * 200, 0);
		LibN lib;
		{
			// the frame of the play screen: item 0 of MDSCREEN.LZC / SCREEN.LZC (an archive whose own header is 4 / 8 bytes, then a 320 x 200 shape)
			Bytes item;
			Shape frame;
			if (readFile(joinPath(dir, game == GameId::MartianDreams ? "MDSCREEN.LZC" : "SCREEN.LZC"), raw) && lib.load(raw, true) && lib.get(0, item)) {
				size_t skip = game == GameId::MartianDreams ? 4 : 8;
				if (item.size() > skip && frame.load(&item[skip], item.size() - skip) && frame.width == 320 && frame.height == 200) {
					std::fill(paper.pixels.begin(), paper.pixels.end(), 0);
					for (int i = 0; i < 320 * 200; ++i)
						if (frame.pixels[i] != Shape::TRANSPARENT)
							paper.pixels[i] = frame.pixels[i];
				}
			}
		}
		if (!readFile(joinPath(dir, "MAPTILES.VGA"), raw) || !lib.load(raw, true) || !lib.get(0, mapTiles))
			return fail("MAPTILES.VGA");
		if (!readFile(joinPath(dir, "MASKTYPE.VGA"), raw) || !lib.load(raw, true) || !lib.get(0, maskType))
			return fail("MASKTYPE.VGA");
		if (!readFile(joinPath(dir, "OBJTILES.VGA"), objTiles))
			return fail("OBJTILES.VGA");
	}
	if (!readFile(joinPath(dir, "TILEINDX.VGA"), index) || !tiles.load(mapTiles, maskType, objTiles, index))
		return fail("tile set");
	if (!readFile(joinPath(dir, "MAP"), d) || !readFile(joinPath(dir, "CHUNKS"), d2) || !map.load(d, d2))
		return fail("MAP/CHUNKS");

	if (!spinoff) {
		static const char *const PORTRAIT_FILES[3] = {"PORTRAIT.A", "PORTRAIT.B", "PORTRAIT.Z"};
		for (int i = 0; i < 3; ++i)
			if (!readFile(joinPath(dir, PORTRAIT_FILES[i]), portraits[i]))
				return fail(PORTRAIT_FILES[i]);
		Bytes lz;
		if (!readFile(joinPath(dir, "LOOK.LZD"), lz) || !lzwDecompress(lz, 0, d) || !look.load(d))
			return fail("LOOK.LZD");
	} else {
		// LOOK.LZC: one item, stored plain; the descriptions are in the format of Ultima VI's LOOK.LZD
		LibN lib;
		if (!readFile(joinPath(dir, "LOOK.LZC"), raw) || !lib.load(raw, true) || !lib.get(0, d) || !look.load(d))
			return fail("LOOK.LZC");
	}

	readFile(joinPath(dir, "BOOK.DAT"), book); // optional: the text of books, scrolls and signs
	if (!readFile(joinPath(dir, "SCHEDULE"), d) || !schedule.load(d))
		return fail("SCHEDULE");
	if (!spinoff && (!readFile(joinPath(dir, "CONVERSE.A"), converse[0]) || !readFile(joinPath(dir, "CONVERSE.B"), converse[1])))
		return fail("CONVERSE");
	if (spinoff) {
		readFile(joinPath(dir, "TALK.LZC"), talk);
		readFile(joinPath(dir, game == GameId::MartianDreams ? "MDFACES.LZC" : "FACES.LZC"), faces);
	}

	objects.clear();
	if (newGame) {
		// A new game is built from LZOBJBLK (the 64 surface blocks, one after another) and LZDNGBLK
		// (the 5 dungeon blocks and then the objlist), as U.EXE's installer does.
		Bytes lz, plain;
		if (!readFile(joinPath(dir, "LZOBJBLK"), lz) || !lzwDecompress(lz, 0, plain))
			return fail("LZOBJBLK");
		size_t pos = 0;
		for (int i = 0; i < 64; ++i) {
			if (pos + 2 > plain.size())
				return fail("LZOBJBLK");
			size_t len = 2 + (size_t)get16(&plain[pos]) * 8;
			if (pos + len > plain.size() || !loadBlock(Bytes(plain.begin() + pos, plain.begin() + pos + len), 0))
				return fail("LZOBJBLK");
			pos += len;
		}
		if (!readFile(joinPath(dir, "LZDNGBLK"), lz) || !lzwDecompress(lz, 0, plain))
			return fail("LZDNGBLK");
		pos = 0;
		for (int level = 1; level <= MapData::DUNGEON_LEVELS; ++level) {
			size_t len = 2 + (size_t)get16(&plain[pos]) * 8;
			if (pos + len > plain.size() || !loadBlock(Bytes(plain.begin() + pos, plain.begin() + pos + len), level))
				return fail("LZDNGBLK");
			pos += len;
		}
		Bytes objlist(plain.begin() + pos, plain.end());
		if (!newGame->applyTo(objlist) || !actors.load(objlist))
			return fail("new game objlist");
	} else {
		std::string save = saveDir.empty() ? joinPath(dir, "SAVEGAME") : saveDir;
		if (!readFile(joinPath(save, "OBJLIST"), d))
			return fail("SAVEGAME/OBJLIST");
		if (spinoff && d.size() >= 0x15F1) {
			// Martian Dreams and Savage Empire have one more 256-byte array in front of the movement points (0x14F1): everything after it is 0x100 further on than in Ultima VI
			_spinoffExtra.assign(d.begin() + 0x14F1, d.begin() + 0x15F1);
			d.erase(d.begin() + 0x14F1, d.begin() + 0x15F1);
		}
		if (!actors.load(d))
			return fail("SAVEGAME/OBJLIST");
		for (int row = 0; row < 8; ++row) {
			for (int col = 0; col < 8; ++col) {
				if (!readFile(joinPath(save, ObjBlock::surfaceFileName(col, row)), d) || !loadBlock(d, 0))
					return fail(ObjBlock::surfaceFileName(col, row));
			}
		}
		for (int level = 1; level <= MapData::DUNGEON_LEVELS; ++level) {
			if (!readFile(joinPath(save, ObjBlock::dungeonFileName(level)), d) || !loadBlock(d, level))
				return fail(ObjBlock::dungeonFileName(level));
		}
	}

	_actorObject.assign(ObjList::ACTORS, -1);
	// The actors' bodies: objects 0-255 of the original. An actor without a type is not in the world.
	for (int a = 0; a < ObjList::ACTORS; ++a) {
		if (actors.objType(a) == 0)
			continue;
		Obj o;
		int x, y, z;
		actors.position(a, x, y, z);
		o.status = actors.objStatus[a];
		o.x = x;
		o.y = y;
		o.z = z;
		o.type = actors.objType(a);
		o.frame = actors.frame(a);
		o.quantity = actors.objQuantity[a] & 0xFF;
		o.quality = actors.objQuantity[a] >> 8;
		o.actor = a;
		o.parent = -1;
		_actorObject[a] = (int)objects.size();
		objects.push_back(o);
	}

	_byPosition.clear();
	for (size_t i = 0; i < objects.size(); ++i) {
		const Obj &o = objects[i];
		if (o.onMap())
			_byPosition.push_back(std::make_pair(positionKey(o.x, o.y, o.z), (int)i));
	}
	std::stable_sort(_byPosition.begin(), _byPosition.end(),
					 [](const std::pair<uint32_t, int> &a, const std::pair<uint32_t, int> &b) { return a.first < b.first; });
	return true;
}

void World::objectsAt(int x, int y, int z, std::vector<int> &out) const {
	out.clear();
	uint32_t key = positionKey(x & 0x3FF, y & 0x3FF, z);
	auto it = std::lower_bound(_byPosition.begin(), _byPosition.end(), std::make_pair(key, -1));
	for (; it != _byPosition.end() && it->first == key; ++it)
		out.push_back(it->second);
}

bool World::portrait(int actor, Bytes &pixels) const {
	if (game != GameId::Ultima6) {
		// MDFACES.LZC: item = actor number (the Avatar's is 0 for a woman and 1 for a man); FACES.LZC: item = actor number - 1; the picture is a shape 4 / 8 bytes into the item
		LibN lib;
		Bytes item;
		int n = game == GameId::MartianDreams ? (actor == 1 ? (actors.globals.size() > 0x80 && actors.globals[0x80] ? 0 : 1) : actor) : actor - 1;
		if (n < 0 || !lib.load(faces, true) || !lib.present(n) || !lib.get(n, item))
			return false;
		size_t skip = game == GameId::MartianDreams ? 4 : 8;
		Shape s;
		if (item.size() <= skip || !s.load(&item[skip], item.size() - skip))
			return false;
		pixels = s.pixels;
		for (uint8_t &p : pixels)
			if (p == Shape::TRANSPARENT)
				p = 0;
		portraitW = s.width;
		portraitH = s.height;
		return true;
	}
	portraitW = 56;
	portraitH = 64;
	int k = actorObject(actor);
	int type = k >= 0 ? objects[k].type : 0;
	int avatar = actors.globals.size() > 0x81 ? actors.globals[0x81] : 0; // objlist +0x1C72
	PortraitRef ref;
	if (!Portrait::locate(actor, type, avatar, ref))
		return false;
	return Portrait::decode(portraits[ref.file == 'A' ? 0 : ref.file == 'B' ? 1 : 2], ref.item, pixels);
}

void World::setQuantity(Obj &o, int q) const {
	o.quantity = q & 0xFF;
	if (o.type == 0x58)
		o.quality = (q >> 8) & 0xFF; // gold keeps a word
}

bool World::isStackable(int type) const {
	return Describe::quantityKind(type) != 0;
}

void World::inventory(int actor, std::vector<int> &out, bool nested) const {
	out.clear();
	std::vector<int> owners;
	for (size_t i = 0; i < objects.size(); ++i) {
		const Obj &o = objects[i];
		if (o.deleted || o.location() == ObjRecord::ON_MAP || o.location() == ObjRecord::IN_CONTAINER)
			continue;
		if (o.parent == actor) {
			out.push_back((int)i);
			owners.push_back((int)i);
		}
	}
	if (!nested)
		return;
	// containers inside: objects whose location is "container" and whose parent is an object already collected
	bool grew = true;
	std::vector<bool> taken(objects.size(), false);
	for (int i : out)
		taken[i] = true;
	while (grew) {
		grew = false;
		for (size_t i = 0; i < objects.size(); ++i) {
			const Obj &o = objects[i];
			if (taken[i] || o.deleted || o.location() != ObjRecord::IN_CONTAINER || o.parent < 0)
				continue;
			if (o.parent < (int)taken.size() && taken[o.parent]) {
				out.push_back((int)i);
				taken[i] = true;
				grew = true;
			}
		}
	}
}

// sub_1D8B1: block-3 weight times the quantity (gold and the reagents, the nine types of ds:081F, divided by ten); 255 means "cannot be moved" (10000)
bool World::script(int actor, ConverseScript &out) const {
	if (game == GameId::Ultima6) {
		int k = actorObject(actor);
		ConverseRef ref;
		return ConverseScript::locate(actor, k >= 0 ? objects[k].type : 0, ref) && out.load(converse[ref.second ? 1 : 0], ref.item);
	}
	// the scripts of TALK.LZC carry their actor number in their second byte (the items are not numbered by actor in Savage Empire); the first item of Martian Dreams is a batch file
	LibN lib;
	Bytes item;
	if (actor <= 0 || actor >= ObjList::ACTORS || !lib.load(talk, true))
		return false;
	if (_talkItem.empty()) {
		_talkItem.assign(ObjList::ACTORS, -1);
		for (size_t i = 0; i < lib.count(); ++i)
			if (lib.present(i) && lib.get(i, item) && item.size() > 2 && item[0] == ConverseScript::SIDENT && _talkItem[item[1]] < 0)
				_talkItem[item[1]] = (int)i;
	}
	return _talkItem[actor] >= 0 && lib.get(_talkItem[actor], item) && out.fromBytes(item);
}

bool World::resurrect(int dead) {
	if (dead <= 0 || dead >= ObjList::ACTORS || !(actors.status[dead] & 0x10))
		return false;
	actors.status[dead] &= ~0x10;
	actors.hp[dead] = (uint8_t)ActorStats::maxHealth(*this, dead);
	if (!(actors.status[dead] & 0x80))
		actors.worktype[dead] = 0x81;
	for (size_t b = 0; b < objects.size(); ++b) {
		if (objects[b].deleted || objects[b].type != 339 || objects[b].quality != dead || !objects[b].onMap())
			continue;
		for (size_t i = 0; i < objects.size(); ++i)
			if (!objects[i].deleted && objects[i].location() == ObjRecord::IN_CONTAINER && objects[i].parent == (int)b) {
				objects[i].status = (uint8_t)((objects[i].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
				objects[i].parent = dead;
			}
		deleteObject((int)b);
		break;
	}
	return true;
}

std::string World::bookText(int n) const {
	if (n < 1 || n > 128 || book.size() < 256)
		return "";
	size_t off = book[(n - 1) * 2] | (book[(n - 1) * 2 + 1] << 8);
	std::string out;
	for (size_t i = off; i < book.size() && book[i]; ++i)
		out += (char)book[i];
	return out;
}

int World::weightOfType(int type, int quantity) const {
	int w = flags.block3[type & (TileFlags::BLOCK3 - 1)];
	if (isStackable(type) && quantity)
		w *= quantity;
	else if (isStackable(type))
		w *= 1;
	if (type == 0x58 || (type >= 0x41 && type <= 0x48))
		w /= 10;
	if (w == 0xFF)
		w = 10000;
	return w;
}

int World::weightOf(int index) const {
	return weightOfType(objects[index].type, quantityOf(objects[index]));
}

// sub_1D957: what the things inside an object weigh
int World::moveCost(int x, int y, int z) const {
	int mask = z == 0 ? 0x3FF : 0xFF;
	int cost = 5;
	int f = flags.flags1[map.tile(z, x & mask, y & mask) & (TileFlags::TILES - 1)];
	if (!(f & 2))
		cost += f >> 4;
	std::vector<int> here;
	objectsAt(x & mask, y & mask, z, here);
	for (int i : here) {
		const Obj &o = objects[i];
		int g = flags.flags1[(baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)];
		if (!(g & 2))
			cost += g >> 4;
	}
	return cost;
}

void World::spendMoves(int actor, int cost) {
	int k = actorObject(actor);
	if (k >= 0 && objects[k].type == 0x1AF)
		cost >>= 1; // a horse
	if (cost < 1)
		cost = 1;
	actors.moves[actor] = (uint8_t)(actors.moves[actor] - cost);
}

int World::contentsWeight(int index) const {
	int total = 0;
	for (size_t i = 0; i < objects.size(); ++i) {
		const Obj &o = objects[i];
		if (!o.deleted && o.location() == ObjRecord::IN_CONTAINER && o.parent == index)
			total += weightOf((int)i) + contentsWeight((int)i);
	}
	return total;
}

// sub_1D998: worn things count their bare type weight, everything else carried its weight with its contents
int World::carriedWeight(int actor) const {
	std::vector<int> inv;
	inventory(actor, inv, false);
	int total = 0;
	for (int i : inv) {
		if (objects[i].location() == ObjRecord::READIED)
			total += flags.block3[objects[i].type & (TileFlags::BLOCK3 - 1)];
		else
			total += weightOf(i) + contentsWeight(i);
	}
	return total;
}

int World::addToInventory(int actor, int type, int quality, int quantity) {
	bool stack = isStackable(type);
	if (stack) {
		std::vector<int> inv;
		inventory(actor, inv, false);
		for (int i : inv) {
			Obj &o = objects[i];
			if (o.type == type && o.frame == 0 && o.location() == ObjRecord::IN_INVENTORY) {
				int q = quantityOf(o) + quantity;
				setQuantity(o, q);
				return i;
			}
		}
	}
	Obj o;
	o.status = ObjRecord::IN_INVENTORY | 0x01; // carried, may be taken
	o.x = o.y = 0;
	o.z = 0;
	o.type = type;
	o.frame = 0;
	o.actor = -1;
	o.parent = actor;
	if (stack) {
		setQuantity(o, quantity);
	} else {
		o.quantity = 1;
		o.quality = quality;
	}
	objects.push_back(o);
	return (int)objects.size() - 1;
}

int World::removeFromInventory(int actor, int type, int quality, int quantity) {
	std::vector<int> inv;
	inventory(actor, inv);
	int taken = 0;
	for (int i : inv) {
		Obj &o = objects[i];
		if (o.type != type || (quality && o.quality != quality && !isStackable(type)))
			continue;
		int have = quantityOf(o);
		int take = quantity <= 0 ? have : std::min(have, quantity - taken);
		if (take <= 0)
			break;
		if (take >= have)
			deleteObject(i);
		else {
			setQuantity(o, have - take);
		}
		taken += take;
		if (quantity > 0 && taken >= quantity)
			break;
	}
	return taken;
}

void World::deleteObject(int index) {
	Obj &o = objects[index];
	if (o.onMap()) {
		auto it = std::find(_byPosition.begin(), _byPosition.end(), std::make_pair(positionKey(o.x, o.y, o.z), index));
		if (it != _byPosition.end())
			_byPosition.erase(it);
	}
	o.deleted = true;
}

// APPROX(41): not the original's exact behaviour, see docs/approximations.md
bool World::save(const std::string &saveDir) {
	// bring the actors' arrays up to date with their bodies
	for (int a = 0; a < ObjList::ACTORS; ++a) {
		int k = _actorObject.empty() ? -1 : _actorObject[a];
		if (k < 0)
			continue;
		const Obj &o = objects[k];
		actors.objStatus[a] = o.status;
		actors.setPosition(a, o.x, o.y, o.z);
		actors.objFrame[a] = (uint16_t)(o.type | (o.frame << 10));
		actors.objQuantity[a] = (uint16_t)(o.quantity | (o.quality << 8));
	}
	Bytes list;
	actors.save(list);
	if (game != GameId::Ultima6 && _spinoffExtra.size() == 0x100 && list.size() >= 0x14F1)
		list.insert(list.begin() + 0x14F1, _spinoffExtra.begin(), _spinoffExtra.end());
	if (!writeFile(joinPath(saveDir, "OBJLIST"), list))
		return false;

	// which block each object goes to: 0-63 surface (row * 8 + col), 64-68 dungeon levels
	std::vector<int> block(objects.size(), -1);
	auto blockOfPosition = [](int x, int y, int z) { return z == 0 ? (y >> 7) * 8 + (x >> 7) : 63 + z; };
	std::function<int(size_t)> resolve = [&](size_t i) -> int {
		if (block[i] >= 0)
			return block[i];
		const Obj &o = objects[i];
		int b = 0;
		if (o.location() == ObjRecord::ON_MAP) {
			b = blockOfPosition(o.x, o.y, o.z);
		} else if (o.location() == ObjRecord::IN_CONTAINER) {
			b = o.parent >= 0 ? resolve((size_t)o.parent) : 0;
		} else {
			int k = o.parent >= 0 && o.parent < ObjList::ACTORS ? _actorObject[o.parent] : -1;
			b = k >= 0 ? blockOfPosition(objects[k].x, objects[k].y, objects[k].z) : 0;
		}
		return block[i] = b;
	};
	std::vector<std::vector<size_t>> members(64 + MapData::DUNGEON_LEVELS);
	for (size_t i = 0; i < objects.size(); ++i) {
		const Obj &o = objects[i];
		if (o.deleted || o.actor >= 0)
			continue;
		int b = resolve(i);
		if (b >= 0 && b < (int)members.size())
			members[b].push_back(i);
	}
	for (size_t b = 0; b < members.size(); ++b) {
		std::vector<size_t> &m = members[b];
		std::vector<int> indexInBlock(objects.size(), -1);
		for (size_t n = 0; n < m.size(); ++n)
			indexInBlock[m[n]] = (int)n;
		Bytes out(2 + m.size() * ObjBlock::RECORD_SIZE, 0);
		out[0] = m.size() & 0xFF;
		out[1] = (m.size() >> 8) & 0xFF;
		for (size_t n = 0; n < m.size(); ++n) {
			const Obj &o = objects[m[n]];
			uint8_t *r = &out[2 + n * ObjBlock::RECORD_SIZE];
			int x = o.x, y = o.y, z = o.z;
			if (o.location() == ObjRecord::IN_CONTAINER) {
				x = o.parent >= 0 ? indexInBlock[o.parent] : 0;
				y = z = 0;
			} else if (o.location() != ObjRecord::ON_MAP) {
				x = o.parent;
				y = z = 0;
			}
			r[0] = o.status;
			r[1] = x & 0xFF;
			r[2] = (uint8_t)(((x >> 8) & 0x03) | ((y & 0x3F) << 2));
			r[3] = (uint8_t)(((y >> 6) & 0x0F) | ((z & 0x0F) << 4));
			r[4] = (o.type | (o.frame << 10)) & 0xFF;
			r[5] = (o.type | (o.frame << 10)) >> 8;
			r[6] = o.quantity;
			r[7] = o.quality;
		}
		std::string name = b < 64 ? ObjBlock::surfaceFileName((int)(b % 8), (int)(b / 8)) : ObjBlock::dungeonFileName((int)(b - 63));
		if (!writeFile(joinPath(saveDir, name), out))
			return false;
	}
	return true;
}

void World::addToMap(int index) {
	const Obj &o = objects[index];
	std::pair<uint32_t, int> e(positionKey(o.x, o.y, o.z), index);
	auto at = std::upper_bound(_byPosition.begin(), _byPosition.end(), e,
							   [](const std::pair<uint32_t, int> &a, const std::pair<uint32_t, int> &b) { return a.first < b.first; });
	_byPosition.insert(at, e);
}

int World::addActor(int actor, int type, int x, int y, int z) {
	Obj o;
	o.status = 0;
	o.x = (uint16_t)x;
	o.y = (uint16_t)y;
	o.z = (uint8_t)z;
	o.type = (uint16_t)type;
	o.frame = 0;
	o.quantity = 1;
	o.quality = 0;
	o.actor = actor;
	o.parent = -1;
	objects.push_back(o);
	int k = (int)objects.size() - 1;
	_actorObject[actor] = k;
	addToMap(k);
	return k;
}

void World::moveObject(int index, int x, int y, int z) {
	Obj &o = objects[index];
	if (o.onMap()) {
		auto it = std::find(_byPosition.begin(), _byPosition.end(), std::make_pair(positionKey(o.x, o.y, o.z), index));
		if (it != _byPosition.end())
			_byPosition.erase(it);
	}
	o.x = x;
	o.y = y;
	o.z = z;
	if (o.onMap()) {
		std::pair<uint32_t, int> e(positionKey(o.x, o.y, o.z), index);
		auto at = std::upper_bound(_byPosition.begin(), _byPosition.end(), e,
								   [](const std::pair<uint32_t, int> &a, const std::pair<uint32_t, int> &b) { return a.first < b.first; });
		_byPosition.insert(at, e);
	}
}

// The path of one bolt of sub_37CBD: a straight line of pixels (16 to a square) from the middle of the start square to the middle of the goal square. Squares are
// collected in order until one lies beyond the reach (the distance table at ds:196C, at most `reach`) or blocks the bolt (flags2 bit 0x20 on the terrain or on any
// object there, unless it is next to the start and has bit 8). Returns whether the goal square was reached.
// APPROX(70): the step sizes and error terms of the original's Bresenham are not copied, and sub_18C09 gives only the top tile there.
bool World::boltPath(int z, int cx, int cy, int gx, int gy, int reach, std::vector<std::pair<int, int>> &squares) const {
	static const uint8_t REACH[8][8] = {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 1, 2, 3, 4, 5, 6, 7}, {2, 2, 2, 3, 4, 5, 6, 7}, {3, 3, 3, 4, 5, 6, 7, 7},
										{4, 4, 4, 5, 6, 7, 7, 8}, {5, 5, 5, 6, 7, 7, 8, 8}, {6, 6, 6, 7, 7, 8, 8, 8}, {7, 7, 7, 7, 8, 8, 8, 8}};
	int dxp = (gx - cx) * 16, dyp = (gy - cy) * 16;
	int major = std::max(std::abs(dxp), std::abs(dyp));
	int lastX = cx, lastY = cy;
	for (int step = 1; step <= major; ++step) {
		int sx = cx * 16 + 8 + (2 * dxp * step + (dxp < 0 ? -major : major)) / (2 * major);
		int sy = cy * 16 + 8 + (2 * dyp * step + (dyp < 0 ? -major : major)) / (2 * major);
		int qx = sx >> 4, qy = sy >> 4;
		if (qx == lastX && qy == lastY)
			continue;
		lastX = qx;
		lastY = qy;
		int adx = std::abs(qx - cx), ady = std::abs(qy - cy);
		if (adx > 7 || ady > 7 || REACH[adx][ady] > reach)
			return false;
		bool next = adx <= 1 && ady <= 1;
		auto stops = [&](int tile) {
			uint8_t f = flags.flags2[tile & (TileFlags::TILES - 1)];
			return (f & 0x20) && !(next && (f & 8));
		};
		std::vector<int> there;
		objectsAt(qx, qy, z, there);
		bool blocked = stops(map.tile(z, qx, qy));
		for (int i : there)
			blocked |= stops(baseTile.firstTile[objects[i].type] + objects[i].frame);
		if (blocked)
			return false;
		squares.push_back(std::make_pair(qx, qy));
	}
	return true;
}

} // namespace Ultima6
} // namespace Ultima6
