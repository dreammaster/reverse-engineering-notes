#include "xanth/gamestate.h"

#include "common/stream.h"

namespace Xanth {

static const size_t kHeaderSize = 0x20;
static const size_t kMaxEvents = 16;

GameState::GameState() {
	reset();
}

void GameState::reset() {
	objects.reset();
	memset(&v, 0, sizeof(v));
	unsigned pos = 0;
	for (int i = 0; i < ARRAYSIZE(kVarMembers); i++) {
		memcpy(reinterpret_cast<byte *>(&v) + kVarMembers[i].offset, kVarsInitial + pos, kVarMembers[i].size);
		pos += kVarMembers[i].size;
	}
	music[0] = music[1] = music[3] = 0;
	music[2] = 0x7F;
	events.clear();
}

size_t GameState::saveSize(size_t eventCount) {
	size_t objectBytes = 0;
	ObjectModel m;
	for (int id = 1; id <= m.count(); id++)
		objectBytes += m.recordSize(id);
	size_t regionBytes = 0;
	for (int i = 0; i < ARRAYSIZE(kVarRegions); i++)
		regionBytes += kVarRegions[i].size;
	return kHeaderSize + objectBytes + regionBytes + 8 + 2 + 4 * eventCount;
}

static void put16(Common::Bytes &out, uint16 v) {
	out.push_back((byte)v);
	out.push_back((byte)(v >> 8));
}

void GameState::save(Common::Bytes &out) const {
	out.assign(kHeaderSize, 0xFF);
	for (int id = 1; id <= objects.count(); id++)
		out.insert(out.end(), objects.record(id), objects.record(id) + objects.recordSize(id));
	for (int i = 0; i < ARRAYSIZE(kVarRegions); i++) {
		const byte *p = reinterpret_cast<const byte *>(&v) + kVarRegions[i].offset;
		out.insert(out.end(), p, p + kVarRegions[i].size);
	}
	for (int i = 0; i < 4; i++)
		put16(out, music[i]);
	put16(out, (uint16)events.size());
	for (size_t i = 0; i < events.size(); i++) {
		put16(out, events[i].a);
		put16(out, events[i].b);
	}
}

bool GameState::load(const Common::Bytes &data) {
	if (data.size() < saveSize(0))
		return false;
	Common::ReadStream s(data);
	for (size_t i = 0; i < kHeaderSize; i++)
		if (s.readByte() != 0xFF)
			return false;
	// parse into a copy first so that a bad file leaves the state unchanged
	GameState n;
	for (int id = 1; id <= n.objects.count(); id++) {
		size_t size = n.objects.recordSize(id);
		if (s.remaining() < size)
			return false;
		n.objects.setRecord(id, s.data() + s.pos(), size);
		s.skip(size);
	}
	for (int i = 0; i < ARRAYSIZE(kVarRegions); i++)
		if (!s.read(reinterpret_cast<byte *>(&n.v) + kVarRegions[i].offset, kVarRegions[i].size))
			return false;
	for (int i = 0; i < 4; i++)
		n.music[i] = s.readUint16LE();
	size_t count = s.readUint16LE();
	if (s.err() || count > kMaxEvents || s.remaining() != 4 * count)
		return false;
	n.events.clear();
	for (size_t i = 0; i < count; i++) {
		ScheduledEvent e;
		e.a = s.readUint16LE();
		e.b = s.readUint16LE();
		n.events.push_back(e);
	}
	*this = n;
	return true;
}

} // namespace Xanth
