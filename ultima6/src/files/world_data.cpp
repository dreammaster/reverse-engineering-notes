#include "files/world_data.h"

#include <cstring>

namespace Ultima6 {

bool Schedule::load(const Bytes &data) {
	if (data.size() < HEADER_SIZE)
		return false;
	for (int i = 0; i <= ACTORS; ++i)
		index[i] = get16(&data[i * 2]);
	size_t bytes = (size_t)total() * ENTRY_SIZE;
	if (data.size() < HEADER_SIZE + bytes)
		return false;
	entries.assign(data.begin() + HEADER_SIZE, data.begin() + HEADER_SIZE + bytes);
	return true;
}

int Schedule::entryCount(int actor) const {
	int t = total();
	if (index[actor] > t - 1)
		return 0;
	if (actor == ACTORS - 1 || index[actor + 1] > t - 1)
		return t - index[actor];
	return index[actor + 1] - index[actor];
}

bool ObjList::load(const Bytes &data) {
	if (data.size() < MIN_SIZE)
		return false;
	const uint8_t *d = &data[0];

	std::memcpy(objStatus, d + OFF_OBJ_STATUS, ACTORS);
	std::memcpy(pos, d + OFF_POS, ACTORS * 3);
	for (int i = 0; i < ACTORS; ++i) {
		objFrame[i] = get16(d + OFF_OBJ_FRAME + i * 2);
		exp[i] = get16(d + OFF_EXP + i * 2);
		baseObjFrame[i] = get16(d + OFF_BASE_OBJ_FRAME + i * 2);
		objQuantity[i] = get16(d + OFF_OBJ_QUANTITY + i * 2);
	}
	std::memcpy(status, d + OFF_STATUS, ACTORS);
	std::memcpy(strength, d + OFF_STRENGTH, ACTORS);
	std::memcpy(dexterity, d + OFF_DEXTERITY, ACTORS);
	std::memcpy(intelligence, d + OFF_INTELLIGENCE, ACTORS);
	std::memcpy(hp, d + OFF_HP, ACTORS);
	std::memcpy(partyNames, d + OFF_PARTY_NAMES, sizeof(partyNames));
	std::memcpy(partyRoster, d + OFF_PARTY_ROSTER, PARTY_MAX);
	partySize = d[OFF_PARTY_SIZE];
	std::memcpy(level, d + OFF_LEVEL, ACTORS);
	std::memcpy(unknown10f1, d + OFF_UNKNOWN_10F1, ACTORS);
	std::memcpy(worktype, d + OFF_WORKTYPE, ACTORS);
	std::memcpy(unknown12f1, d + OFF_UNKNOWN_12F1, ACTORS);
	std::memcpy(unknown13f1, d + OFF_UNKNOWN_13F1, ACTORS);
	std::memcpy(moves, d + OFF_MOVES, ACTORS);
	std::memcpy(talkFlags, d + OFF_TALK_FLAGS, ACTORS);
	std::memcpy(unknown18f1, d + OFF_UNKNOWN_18F1, ACTORS);
	std::memcpy(movementFlags, d + OFF_MOVEMENT_FLAGS, ACTORS);
	std::memcpy(unknown1af1, d + OFF_UNKNOWN_1AF1, ACTORS);
	globals.assign(data.begin() + OFF_GLOBALS, data.end());
	return true;
}

void ObjList::position(int actor, int &x, int &y, int &z) const {
	const uint8_t *p = pos[actor];
	x = p[0] | ((p[1] & 0x03) << 8);
	y = ((p[1] & 0xFC) >> 2) | ((p[2] & 0x0F) << 6);
	z = (p[2] & 0xF0) >> 4;
}

} // namespace Ultima6
