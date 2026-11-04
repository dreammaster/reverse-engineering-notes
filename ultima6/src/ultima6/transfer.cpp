#include "ultima6/transfer.h"

namespace Ultima6 {
namespace Ultima6 {

static std::string nameAt(const Bytes &d, size_t at) {
	std::string s;
	for (size_t i = 0; i < 8 && at + i < d.size() && d[at + i]; ++i)
		s += (char)d[at + i];
	return s;
}

static int word(const Bytes &d, size_t at) {
	return d[at] | (d[at + 1] << 8);
}

void TransferCharacter::readUltimaV(const Bytes &d, std::vector<TransferCharacter> &out) {
	for (int i = 0; i < 16; ++i) {
		size_t r = 2 + 0x20 * (size_t)i;
		if (r + 0x20 > d.size())
			break;
		TransferCharacter c;
		c.name = nameAt(d, r);
		if (c.name.empty())
			continue;
		c.female = d[r + 9] != 0x0B;
		c.strength = d[r + 0xC];
		c.dexterity = d[r + 0xD];
		c.intelligence = d[r + 0xE];
		c.experience = word(d, r + 0x14);
		int hp = word(d, r + 0x10), maxHp = word(d, r + 0x12);
		// sub_1276D refuses a record with a statistic over 30 or points over 9999
		if (c.strength > 30 || c.dexterity > 30 || c.intelligence > 30 || hp > 9999 || maxHp > 9999 || c.experience > 9999)
			continue;
		out.push_back(c);
	}
}

void TransferCharacter::readUltimaIV(const Bytes &d, std::vector<TransferCharacter> &out) {
	for (int i = 0; i < 8; ++i) {
		size_t r = 8 + 0x27 * (size_t)i;
		if (r + 0x27 > d.size())
			break;
		TransferCharacter c;
		c.fromUltimaIV = true;
		c.name = nameAt(d, r + 0x14);
		if (c.name.empty())
			continue;
		c.female = d[r + 0x24] != 0x0B;
		c.strength = word(d, r + 6);
		c.dexterity = word(d, r + 8);
		c.intelligence = word(d, r + 10);
		c.experience = word(d, r + 4);
		int hp = word(d, r), maxHp = word(d, r + 2);
		// sub_12630: statistics up to 99, points up to 9999, a class under 8
		if (c.strength > 99 || c.dexterity > 99 || c.intelligence > 99 || hp > 9999 || maxHp > 9999 || c.experience > 9999 || d[r + 0x25] > 7)
			continue;
		out.push_back(c);
	}
}

// sub_1282D
static int scaled(int v) {
	if (v < 10)
		return v;
	if (v < 30)
		return (v - 9) / 2 + 10;
	return (v - 30) / 4 + 20;
}

NewCharacter TransferCharacter::toNewCharacter() const {
	NewCharacter n;
	n.name = name;
	n.female = female;
	n.portrait = 0;
	n.stats.strength = fromUltimaIV ? scaled(strength) : strength;
	n.stats.dexterity = fromUltimaIV ? scaled(dexterity) : dexterity;
	n.stats.intelligence = fromUltimaIV ? scaled(intelligence) : intelligence;
	n.experience = experience / 10;
	int level = 1;
	for (int bits = experience / 1000; bits > 0; bits >>= 1)
		++level;
	n.level = level;
	n.hitPoints = 30 * level;
	n.stats.karma = 0x4B; // new_char_karma
	return n;
}

} // namespace Ultima6
} // namespace Ultima6
