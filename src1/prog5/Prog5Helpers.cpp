/* PROG5: the helpers that the original takes from its shared code: party file access, money, number formatting and the rendering of a
 * character (body, armour and weapon layers composed off-screen, then stored as portrait tiles in BATPIC.DAT). */

#include "prog5/Prog5Layout.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog5 {

using namespace Layout;

// ---------------------------------------------------------------------------------------------
// party file
// ---------------------------------------------------------------------------------------------

static std::string partyPath(const std::string &dir) {
	std::string p = dir;
	if (!p.empty() && p[p.size() - 1] != '/' && p[p.size() - 1] != '\\')
		p += "/";
	return p + "CURRGAME/PARTY.DAT";
}

// fileReadRecordTo: a record that cannot be read leaves the descriptor's buffer as it was (it is copied anyway)
bool Prog5::readRecord(uint16 record, uint16 dest) {
	Shared::RecordFile f;
	_descRecord = record;
	_status = 1;
	if (f.open(partyPath(_rt.workDir()).c_str(), kRecordSize, false) && f.read(record, _descBuffer))
		_status = 0;
	memcpy(_ds.p(dest), _descBuffer, kRecordSize);
	return _status == 0;
}

// fileWriteRecordFrom: the descriptor's buffer is cleared, filled from the program's copy and written
void Prog5::writeRecord(uint16 record, uint16 src) {
	Shared::RecordFile f;
	_descRecord = record;
	memset(_descBuffer, 0, sizeof(_descBuffer));
	memcpy(_descBuffer, _ds.p(src), kRecordSize);
	_status = 1;
	if (f.open(partyPath(_rt.workDir()).c_str(), kRecordSize, true) && f.write(record, _descBuffer))
		_status = 0;
}

void Prog5::loadParty() {
	for (uint16 i = 0; i < 10; i++)
		readRecord(i, (uint16)(kParty + kRecordSize * i));
}

void Prog5::saveParty() {
	for (uint16 i = 0; i < 10; i++)
		writeRecord(i, (uint16)(kParty + kRecordSize * i));
}

// shared_c9c8cc: the first party member (of the six slots) who is up and a fighter-type class (1 or 5) is the one the others follow;
// reading the members leaves the last one read in the record buffer
void Prog5::pickLeaderRecord() {
	_ds.setW(kLeader, 0);
	for (int slot = 5; slot >= 0; slot--) {
		const uint16 id = slotWord(slot);
		if (id == 0)
			continue;
		readRecord(id, kRec);
		const byte status = _ds.b(kStatus);
		if (status == 6 || status == 9 || (int8_t)status >= 0x0B)
			continue;
		if (_ds.b(kClass) == 1 || _ds.b(kClass) == 5)
			_ds.setW(kLeader, id);
	}
	if (_ds.w(kLeader) == 0)
		_ds.setW(kLeader, slotWord(0));
}

// ---------------------------------------------------------------------------------------------
// numbers
// ---------------------------------------------------------------------------------------------

// shared_72afff / shared_04c688: the value as a right-aligned decimal with a comma after the thousands once a digit has appeared
uint16 Prog5::formatNumber(uint16 value, uint16 dest) {
	uint16 bx = dest;
	_status = 1;
	if (value == 0) {
		_status = 0;
		_ds.setW(bx, 0x2020);
		_ds.setW((uint16)(bx + 2), 0x2020);
		_ds.b((uint16)(bx + 4)) = '0';
		bx += 5;
		_ds.b(bx) = 0;
		return bx;
	}
	bool started = false;
	auto digit = [&](uint16 divisor) {
		const byte q = (byte)(value / divisor);
		value = (uint16)(value - q * divisor);
		_ds.b(bx) = (byte)(q + '0');
		bx++;
		if (!started && _ds.b((uint16)(bx - 1)) == '0')
			_ds.b((uint16)(bx - 1)) = ' ';
		return q != 0;
	};
	if (digit(10000)) started = true;
	if (digit(1000)) started = true;
	if (started) {
		_ds.b(bx) = ',';
		bx++;
	}
	if (digit(100)) started = true;
	if (digit(10)) started = true;
	digit(1);
	_ds.b(bx) = 0;
	return bx;
}

// ---------------------------------------------------------------------------------------------
// money: four BCD bytes at DS:09FCh (most significant first)
// ---------------------------------------------------------------------------------------------

namespace {

struct Flags {
	bool cf, af;
};

byte adc8(byte a, byte b, Flags &f) {
	const unsigned sum = (unsigned)a + b + (f.cf ? 1 : 0);
	f.af = ((a & 0x0F) + (b & 0x0F) + (f.cf ? 1 : 0)) > 0x0F;
	f.cf = sum > 0xFF;
	return (byte)sum;
}

byte daa(byte al, Flags &f) {
	const byte oldAl = al;
	const bool oldCf = f.cf;
	if ((al & 0x0F) > 9 || f.af) {
		const unsigned t = (unsigned)al + 6;
		f.cf = oldCf || t > 0xFF;
		al = (byte)t;
		f.af = true;
	} else {
		f.af = false;
	}
	if (oldAl > 0x99 || oldCf) {
		al = (byte)(al + 0x60);
		f.cf = true;
	} else {
		f.cf = false;
	}
	return al;
}

} // namespace

// shared_8f30c2 mode 1: add the four bytes at `bcd` to the money (as binary with carry, then a decimal correction pass)
void Prog5::money(int op, uint16 bcd) {
	if (op == 1) {
		Flags f = {false, false};
		uint16 di = (uint16)(bcd + 3), si = kGold + 3;
		for (int i = 0; i < 4; i++) {
			_ds.b(si) = adc8(_ds.b(si), _ds.b(di), f);
			si--;
			di--;
		}
		f = Flags{false, false};
		si = kGold + 3;
		byte al = _ds.b(si);
		for (int i = 0; i < 4; i++) {
			al = daa(al, f);
			_ds.b(si) = al;
			si--;
			al = adc8(_ds.b(si), 0, f);
		}
	}
}

// mode 2: is the money at least the four BCD bytes at `bcd`? (the original reports it in the carry flag: set when it is less)
bool Prog5::moneyAtLeast(uint16 bcd) {
	for (int i = 0; i < 4; i++) {
		const byte have = _ds.b((uint16)(kGold + i)), need = _ds.b((uint16)(bcd + i));
		if ((have & 0xF0) != (need & 0xF0))
			return (have & 0xF0) > (need & 0xF0);
		if ((have & 0x0F) != (need & 0x0F))
			return (have & 0x0F) > (need & 0x0F);
	}
	return true;
}

// mode 3: "12,345,678" (ten characters and a NUL) at `dest`
void Prog5::moneyAsText(uint16 dest) {
	_ds.fill(dest, ' ', 10);
	_ds.b((uint16)(dest + 10)) = 0;
	_ds.b((uint16)(dest + 2)) = ',';
	_ds.b((uint16)(dest + 6)) = ',';
	byte started = 0;
	uint16 di = dest;
	auto put = [&](byte digit) {            // shared_c444c7
		if (digit == 0) {
			if (started == 1) {
				if (_ds.b(di) == ',')
					di++;
				_ds.b(di) = '0';
				di++;
			} else {
				if (_ds.b(di) == ',') {
					_ds.b(di) = ' ';
					di++;
				}
				di++;
			}
		} else {
			if (started == 1) {
				if (_ds.b(di) == ',')
					di++;
			} else {
				if (_ds.b(di) == ',') {
					_ds.b(di) = ' ';
					di++;
				}
				started = 1;
			}
			_ds.b(di) = (byte)(digit + '0');
			di++;
		}
	};
	for (int i = 0; i < 4; i++) {
		const byte v = _ds.b((uint16)(kGold + i));
		put((byte)(v >> 4));
		put((byte)(v & 0x0F));
	}
}

// mode 4: a binary number (up to 99,999) becomes four BCD bytes and is added to the money
void Prog5::addMoneyBinary(uint16 value) {
	uint16 v = value;
	const byte tenThousands = (byte)(v / 10000);
	v = (uint16)(v - tenThousands * 10000);
	const byte thousands = (byte)(v / 1000);
	v = (uint16)(v - thousands * 1000);
	const byte hundreds = (byte)(v / 100);
	v = (uint16)(v - hundreds * 100);
	const byte tens = (byte)(v / 10);
	const byte ones = (byte)(v - tens * 10);
	_ds.b(kBcdScratch) = 0;
	_ds.b((uint16)(kBcdScratch + 1)) = tenThousands;
	_ds.b((uint16)(kBcdScratch + 2)) = (byte)((thousands << 4) | hundreds);
	_ds.b((uint16)(kBcdScratch + 3)) = (byte)((tens << 4) | ones);
	money(1, kBcdScratch);
}

// ---------------------------------------------------------------------------------------------
// derived values (shared_d809a1): from the first and fourth attribute and the equipment bonuses
// ---------------------------------------------------------------------------------------------

void Prog5::deriveStats() {
	const byte a = _ds.b(kStat0), d = _ds.b((uint16)(kStat0 + 3));
	uint16 ax = (uint16)((byte)(a + d) * 0x4B + 0x32) / 100;
	_ds.setW(kDerived75, ax);
	int al = (int)d - 0x3C;
	if (al < 0)
		al = 0;
	_ds.addW(kDerived75, (uint16)(al / 4));
	ax = (uint16)(_ds.w(kDerived75) + _ds.w(kBonus77));
	if (_ds.b(kStatus) == 9)
		ax >>= 1;
	_ds.setW(kDerived73, ax);

	_ds.setW(kDerived81, _ds.w(kBonus7F));
	al = (int)a - 0x3C;
	if (al < 0)
		al = 0;
	_ds.addW(kDerived81, (uint16)(al / 5));
	_ds.setW(kDerived81, (uint16)(_ds.w(kDerived81) + _ds.w(kBonus83)));

	_ds.setW(kDerived7B, _ds.w(kBonus79));
	al = (int)d - 0x3C;
	if (al < 0)
		al = 0;
	_ds.addW(kDerived7B, (uint16)(al / 3));
	_ds.addW(kDerived7B, _ds.w(kBonus7D));
}

// ---------------------------------------------------------------------------------------------
// drawing a character
// ---------------------------------------------------------------------------------------------

// drawTileKeyed: copy the 32x32 area at (_scratchX,_scratchY) to (_destX,_destY), colour 0 transparent
void Prog5::drawKeyed() {
	const uint16 x1 = _c.x1, x2 = _c.x2, y1 = _c.y1, y2 = _c.y2, x3 = _c.x3, y3 = _c.y3;
	_c.x1 = _scratchX;
	_c.x2 = (uint16)(_scratchX + 0x20);
	_c.y1 = _scratchY;
	_c.y2 = (uint16)(_scratchY + 0x20);
	_c.x3 = _destX;
	_c.y3 = _destY;
	_c.keyColour = _c.colourMap[0];
	_rt.vid(0x13);
	_c.y2 = y2; _c.y1 = y1; _c.x2 = x2; _c.x1 = x1; _c.y3 = y3; _c.x3 = x3;
}

void Prog5::drawTileScratch() {            // shared_9790fc
	_c.x1 = 0;
	_c.y1 = 0;
	_rt.drawTile(_file, _tile);
	drawKeyed();
}

void Prog5::drawBackdrop() {               // drawBackdropTile
	_c.x1 = 0;
	_c.y1 = 0;
	_rt.drawTile(0, 0xD5);
}

// shared_063c13: a tile (file in `_file`, index `tile`) is drawn at the scratch area and copied keyed to (x,y)
void Prog5::drawTileAt(uint16 x, uint16 y, uint16 tileFile, uint16 tile) {
	_destX = x;
	_destY = y;
	_c.x1 = _scratchX;
	_c.y1 = _scratchY;
	_file = (byte)tileFile;
	_tile = tile;
	_rt.drawTile(_file, _tile);
	drawKeyed();
}

void Prog5::drawBodyTile(uint16 tile) {    // shared_89c35e
	if (tile != 0x30)
		tile = (uint16)(tile + _ds.w(kWeaponShift));
	_tile = tile;
	_rt.drawTile(_file, _tile);
	drawKeyed();
}

// shared_6228a1: the armour layer `row`: the tile for the armour in the third equipment slot, from the table at DS:4F28h
void Prog5::shieldTile(int row) {
	_tile = 0x30;
	_file = 3;
	const uint16 armour = _ds.w(kShield);
	int bx;
	static const int kBase[6] = {0x15, 0x3E, 0x67, 0x90, 0xB9, 0xE2};
	int k = -1;
	for (int i = 0; i < 6; i++) {
		bx = (int16_t)armour - kBase[i];
		if (bx < 0)
			return;
		if (bx <= 4) {
			k = i;
			break;
		}
	}
	if (k < 0)
		return;
	bx = (int16_t)armour - kBase[k];
	const uint16 entry = (uint16)(kArmourTable + 2 * bx + (row - 3));
	uint16 t = _ds.w(entry);
	if (armour >= 0x3E)
		t = (uint16)(t + 0xA0);
	_tile = t;
}

// shared_9cebbd: the weapon sprites for the weapon in the first equipment slot (pairs in the table at DS:4CB6h)
void Prog5::weaponTables() {
	_ds.setW(kWeaponShift, 0);
	_ds.setW(kWeaponA, 0);
	_file = 3;
	uint16 bx = kWeaponTable;
	const uint16 weapon = _ds.w(kWeapon);
	for (;;) {
		if (_ds.w(bx) == weapon)
			break;
		if (_ds.w(bx) == 0)
			return;
		bx += 4;
	}
	bx = _ds.w((uint16)(bx + 2));
	_ds.setW(kWeaponA, bx);
	_ds.setW(kWeaponB, (uint16)(bx + 8));
	if (weapon >= 0x3E)
		_ds.setW(kWeaponShift, 0xA0);
}

// shared_cf62f3: a defeated or absent character: the plain 4x4 picture
void Prog5::composeSimple() {
	_c.y1 = 0x0E;
	_file = _ds.b(kSpriteFile);
	_tile = (uint16)(_ds.w(kSpriteBase) + 7);
	for (int row = 0; row < 4; row++) {
		_c.x1 = 0x30;
		for (int col = 0; col < 4; col++) {
			_rt.drawTile(_file, _tile);
			_c.x1 = (uint16)(_c.x1 + 0x20);
		}
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	_c.x1 = 0xB0;
	_c.y1 = 0x0E;
	_file = 3;
	_tile = 0x30;
	for (int i = 0; i < 4; i++) {
		_rt.drawTile(_file, _tile);
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
}

// shared_63ca2b: the body in four rows of tiles from the character's picture set
void Prog5::composeCharacter() {
	if (_ds.b(kHeaderFlags) & 0x10) {
		_c.x1 = 0x30; _c.y1 = 0x0E; _c.x2 = 0xD0; _c.y2 = 0x8E;
		_c.colour = _c.colourMap[0];
		_rt.vid(0x07);
		_c.x1 = 0x90; _c.y1 = 0x6E;
		shieldTile(0x21);
		_rt.drawTile(_file, _tile);
		return;
	}
	const uint16 base = _ds.w(kSpriteBase), head = _ds.w(kHeadBase);
	_file = _ds.b(kSpriteFile);
	auto draw = [&](uint16 x, uint16 y, uint16 tile) {
		_c.x1 = x;
		_c.y1 = y;
		_tile = tile;
		_rt.drawTile(_file, _tile);
	};
	_c.x1 = 0x30; _c.y1 = 0x0E;
	_tile = (uint16)(base + 4); _rt.drawTile(_file, _tile);
	_c.x1 = 0x50; _tile = (uint16)(head + 8); _rt.drawTile(_file, _tile);
	_c.x1 = 0x70; _tile++; _rt.drawTile(_file, _tile);
	_c.x1 = 0x90; _tile = (uint16)(base + 5); _rt.drawTile(_file, _tile);
	draw(0x30, 0x2E, (uint16)(base + 0x0E));
	_c.x1 = 0x50; _tile = (uint16)(head + 0x0A); _rt.drawTile(_file, _tile);
	_c.x1 = 0x70; _tile++; _rt.drawTile(_file, _tile);
	_c.x1 = 0x90; _tile = (uint16)(base + 0x0F); _rt.drawTile(_file, _tile);
	draw(0x30, 0x4E, (uint16)(base + 0x18));
	_c.x1 = 0x50; _tile = (uint16)(head + 0x0C); _rt.drawTile(_file, _tile);
	_c.x1 = 0x70; _tile++; _rt.drawTile(_file, _tile);
	_c.x1 = 0x90; _tile = (uint16)(base + 0x19); _rt.drawTile(_file, _tile);
	draw(0x30, 0x6E, (uint16)(base + 0x22));
	_c.x1 = 0x50; _tile = (uint16)(head + 0x0E); _rt.drawTile(_file, _tile);
	_c.x1 = 0x70; _tile++; _rt.drawTile(_file, _tile);
	_c.x1 = 0x90;
	shieldTile(0x21);
	_rt.drawTile(_file, _tile);
	_c.x1 = 0xB0; _c.y1 = 0x0E;
	_file = 3;
	_tile = 0x30;
	for (int i = 0; i < 4; i++) {
		_rt.drawTile(_file, _tile);
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
}

// shared_ab7386: lay the equipment and body layers over the picture (keyed copies from tiles in file 3 to the 4x4 area at the left)
void Prog5::copyLayers() {
	_scratchX = 0;
	_scratchY = 0;
	_destY = 0xFFEE;
	_file = _ds.b(kSpriteFile);
	uint16 tile;
	const byte flags = _ds.b(kHeaderFlags);
	if (flags & 0x10) {
		tile = (flags & 0x20) ? 0x57F : 0x56F;
	} else if (flags & 0x20) {
		tile = 0x57F;
	} else {
		_ds.setW(kPortraitTile, 0);
		const byte status = _ds.b(kStatus);
		if ((int8_t)status < 7 || (int8_t)status >= 0x0E)
			return;
		tile = (uint16)(((status - 7) << 4) + 0x4FF);
	}
	_tile = tile;
	for (int row = 0; row < 3; row++) {
		_destY = (uint16)(_destY + 0x20);
		_destX = 0x10;
		for (int col = 0; col < 4; col++) {
			_destX = (uint16)(_destX + 0x20);
			_c.x3 = _destX;
			_c.y3 = _destY;
			_c.x1 = _scratchX;
			_c.x2 = (uint16)(_scratchX + 0x20);
			_c.y1 = _scratchY;
			_c.y2 = (uint16)(_scratchY + 0x20);
			_tile++;
			_rt.drawTile(_file, _tile);
			_rt.vid(0x13);
		}
	}
	_destY = (uint16)(_destY + 0x20);
	_destX = 0x10;
	for (int col = 0; col < 3; col++) {
		_destX = (uint16)(_destX + 0x20);
		_c.x3 = _destX;
		_c.y3 = _destY;
		_c.x1 = _scratchX;
		_c.x2 = (uint16)(_scratchX + 0x20);
		_c.y1 = _scratchY;
		_c.y2 = (uint16)(_scratchY + 0x20);
		_tile++;
		_rt.drawTile(_file, _tile);
		_rt.vid(0x13);
	}
	_tile++;
	_ds.setW(kPortraitTile, _tile);
}

// drawPartyStrip: render the character (body, armour and weapon layers) and keep its 5x4 tiles in BATPIC.DAT for the party strip
void Prog5::drawPartyStrip() {
	const byte status = _ds.b(kStatus);
	if (status >= 0x0E || status == 6) {
		composeSimple();
	} else {
		composeCharacter();
		copyLayers();
		_c.x1 = 0;
		_c.y1 = 0;
		_destX = 0x10;
		_destY = 0x0E;
		shieldTile(3);
		for (int i = 0; i < 4; i++) { _destX = (uint16)(_destX + 0x20); drawTileScratch(); }
		_destX = 0x10;
		_destY = 0x2E;
		shieldTile(0x0D);
		for (int i = 0; i < 4; i++) { _destX = (uint16)(_destX + 0x20); drawTileScratch(); }
		_destX = 0x10;
		_destY = 0x4E;
		shieldTile(0x17);
		for (int i = 0; i < 4; i++) { _destX = (uint16)(_destX + 0x20); drawTileScratch(); }
		_destX = 0x10;
		_destY = 0x6E;
		shieldTile(0x21);
		for (int i = 0; i < 3; i++) { _destX = (uint16)(_destX + 0x20); drawTileScratch(); }
		_destX = 0x90;
		_destY = 0x6E;
		if (!(_ds.b(kHeaderFlags) & 0x10)) {
			_tile = (uint16)(_ds.w(kSpriteBase) + 0x23);
			_file = _ds.b(kSpriteFile);
			drawTileScratch();
		}
		if (_ds.w(kPortraitTile) != 0) {
			_tile = _ds.w(kPortraitTile);
			_file = _ds.b(kSpriteFile);
			drawTileScratch();
		}
		if (_ds.w(kWeapon) != 0) {
			weaponTables();
			if (_ds.w(kWeaponB) != 0) {                              // (the original tests the second pointer)
				_destY = 0x0E;
				for (int row = 0; row < 4; row++) {
					_destX = 0x30;
					drawBodyTile(_ds.w(_ds.w(kWeaponA)));
					for (int i = 0; i < 2; i++) { _destX = (uint16)(_destX + 0x20); drawKeyed(); }
					_destX = (uint16)(_destX + 0x20);
					drawBodyTile(_ds.w(_ds.w(kWeaponB)));
					_destX = (uint16)(_destX + 0x20);
					drawBodyTile(_ds.w((uint16)(_ds.w(kWeaponB) + 2)));
					_destY = (uint16)(_destY + 0x20);
					_ds.addW(kWeaponA, 2);
					_ds.addW(kWeaponB, 4);
				}
			}
		}
	}
	// store the picture: 4 rows of 5 tiles from the screen into BATPIC.DAT
	const Vid::ImagePtr savedImage = _c.image;
	_c.image.set(_ds.p(kImageBuffer), Data::kTileBytes, 0);
	uint16 record = _ds.w(kPortrait);
	_file = Shared::Runtime::kBatpicFile;
	_tile = record;
	_c.y1 = 0x0E;
	for (int row = 0; row < 4; row++) {
		_c.x1 = 0x30;
		for (int col = 0; col < 5; col++) {
			_rt.vid(0x09);
			_rt.writeBatpic(record, _ds.p(kImageBuffer));
			record++;
			_c.x1 = (uint16)(_c.x1 + 0x20);
			_tile++;
		}
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	_c.image = savedImage;
	drawBackdrop();
	_ds.b(kFlags) &= 0xBF;
}

} // namespace Prog5
} // namespace Yendor1
