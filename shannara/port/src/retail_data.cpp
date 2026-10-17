#include "retail_data.h"

namespace shannara {

// linear addresses in SHANNARA.EXE (retail build 1.0.0 11-13-1995)
static const uint32_t kObjectTable = 0x20918;  // 744 x {u8 type; u32 recordPtr}
static const uint32_t kObjectCount = 0x217A0;  // u16
static const uint32_t kObjectRecords = 0xB0958; // records, packed, sizes by type
static const uint32_t kRoomPicTable = 0x1CFE4; // 68 x {u16 room; u32 picId}
static const uint32_t kRoomMusicTable = 0xAE9FC; // 68 x {u16 room; u32 track}
static const int kRoomCount = 68;

RetailTables RetailTables::load(const LeExe &exe) {
	RetailTables t;
	int n = exe.u16(kObjectCount);
	uint32_t addr = kObjectRecords;
	for (int i = 0; i < n; i++) {
		ObjectRecord r;
		r.id = i + 1;
		r.type = exe.u8(kObjectTable + 5 * i);
		if (r.type < 1 || r.type > 5)
			throw FormatError("bad object type in the object table");
		size_t sz = ObjectRecord::recordSize(r.type);
		r.raw.resize(sz);
		exe.read(addr, r.raw.data(), sz);
		addr += (uint32_t)sz;
		t.objects.push_back(r);
	}
	for (int i = 0; i < kRoomCount; i++) {
		RoomInfo ri;
		ri.room = exe.u16(kRoomPicTable + 6 * i);
		ri.picId = exe.u32(kRoomPicTable + 6 * i + 2);
		// the music table lists the same rooms in the same order
		if (exe.u16(kRoomMusicTable + 6 * i) == ri.room)
			ri.music = (int32_t)exe.u32(kRoomMusicTable + 6 * i + 2);
		t.rooms.push_back(ri);
	}
	return t;
}

} // namespace shannara
