// Initial game data that lives in SHANNARA.EXE's static data (read through LeExe): the 744 object records and the room
// picture/music tables. Layouts from analysis/retail_game_layer.md sections 4, 5, 11 and 29; the object records were
// compared byte for byte with the IDA dump (retail_objects.tsv).
#pragma once
#include "le.h"

namespace shannara {

enum ObjectType { kRoom = 1, kItem = 2, kContainer = 3, kProp = 4, kCharacter = 5 }; // 1 and 5 are the best-supported meanings

struct ObjectRecord {
	int id = 0;   // 1-based
	int type = 0; // 1..5
	Bytes raw;    // record bytes (36/44/48/24/52 for types 1-5)

	uint32_t nameOffset() const { return u32(0); } // 1-based offset into the OBJECT.DAT pool
	uint32_t descOffset() const { return u32(4); } // 0 = none
	// types 2, 3, 5 only (rooms and props have no location)
	bool hasLocation() const { return type == 2 || type == 3 || type == 5; }
	int location() const { return !hasLocation() ? 0 : type == 3 ? u16(0xB) : u16(0xA); }
	int slot() const { return !hasLocation() ? 0 : type == 3 ? raw[0xD] : raw[0xC]; }
	int nextSibling() const { return !hasLocation() ? 0 : type == 3 ? u16(0xE) : u16(0xD); }
	// 1-based index into the 640 compiled handlers (0 = none)
	uint32_t handlerIndex() const {
		static const int off[6] = {0, 0x10, 0x25, 0x2C, 0x14, 0x2D};
		return u32(off[type]);
	}
	static size_t recordSize(int type) {
		static const size_t sz[6] = {0, 36, 44, 48, 24, 52};
		return sz[type];
	}
	uint32_t u32(size_t o) const { return raw[o] | (raw[o + 1] << 8) | (raw[o + 2] << 16) | ((uint32_t)raw[o + 3] << 24); }
	uint16_t u16(size_t o) const { return raw[o] | (raw[o + 1] << 8); }
};

struct RoomInfo {
	int room = 0;        // object id of the room
	uint32_t picId = 0;  // (pic file << 8) | entry; 0x32 = blank
	int32_t music = 0;   // track reference (negative = none)
};

struct RetailTables {
	std::vector<ObjectRecord> objects;
	std::vector<RoomInfo> rooms; // 68 entries; picId here is the *base* picture, get_room_pic_id applies flag-dependent overrides

	static RetailTables load(const LeExe &exe);
};

} // namespace shannara
