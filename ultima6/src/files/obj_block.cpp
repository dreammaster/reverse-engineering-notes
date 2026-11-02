#include "files/obj_block.h"

namespace Ultima6 {

bool ObjBlock::load(const Bytes &data) {
	objects.clear();
	if (data.size() < 2)
		return false;
	size_t count = get16(&data[0]);
	if (count > MAX_OBJECTS || data.size() != 2 + count * RECORD_SIZE)
		return false;

	objects.resize(count);
	for (size_t i = 0; i < count; ++i) {
		const uint8_t *r = &data[2 + i * RECORD_SIZE];
		ObjRecord &o = objects[i];
		o.status = r[0];
		o.x = r[1] | ((r[2] & 0x03) << 8);
		o.y = ((r[2] & 0xFC) >> 2) | ((r[3] & 0x0F) << 6);
		o.z = r[3] >> 4;
		o.typeFrame = get16(r + 4);
		o.quantity = r[6];
		o.quality = r[7];
	}
	return true;
}

std::string ObjBlock::surfaceFileName(int col, int row) {
	std::string name = "OBJBLKAA";
	name[6] = (char)('A' + col);
	name[7] = (char)('A' + row);
	return name;
}

std::string ObjBlock::dungeonFileName(int level) {
	std::string name = "OBJBLKAI";
	name[6] = (char)('A' + level - 1);
	return name;
}

} // namespace Ultima6
