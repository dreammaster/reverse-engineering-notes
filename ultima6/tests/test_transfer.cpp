// Characters brought over from Ultima V and Ultima IV (U.EXE sub_1276D, sub_12630, sub_12904, sub_12863).

#include "ultima6/transfer.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::NewCharacter;
using ::Ultima6::Ultima6::TransferCharacter;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	// Ultima V: Paula the Jester's record from the audit workspace's saved game (skipped when it is absent)
	Bytes v;
	if (readFile("../ultima5/tests/fixtures/paula/SAVED.GAM", v)) {
		std::vector<TransferCharacter> found;
		TransferCharacter::readUltimaV(v, found);
		CHECK(found.size() >= 3);
		if (found.size() >= 3) {
			CHECK(found[0].name == "Paula" && found[0].female && found[0].strength == 20 && found[0].intelligence == 21 && found[0].experience == 150);
			CHECK(found[1].name == "Shamino" && !found[1].female);
			NewCharacter n = found[0].toNewCharacter();
			CHECK(n.stats.strength == 20 && n.experience == 15 && n.level == 1 && n.hitPoints == 30);
		}
	}
	// Ultima IV: one hand-made record (hp 150, max 200, experience 2500, 40 / 12 / 99, name Lance, male)
	Bytes iv(8 + 0x27 * 8, 0);
	size_t r = 8;
	iv[r] = 150;
	iv[r + 2] = 200;
	iv[r + 4] = 2500 & 0xFF;
	iv[r + 5] = 2500 >> 8;
	iv[r + 6] = 40;
	iv[r + 8] = 12;
	iv[r + 10] = 99;
	const char *name = "Lance";
	for (int i = 0; name[i]; ++i)
		iv[r + 0x14 + i] = (uint8_t)name[i];
	iv[r + 0x24] = 0x0B;
	std::vector<TransferCharacter> found;
	TransferCharacter::readUltimaIV(iv, found);
	CHECK(found.size() == 1 && found[0].name == "Lance" && !found[0].female);
	if (found.size() == 1) {
		NewCharacter n = found[0].toNewCharacter();
		CHECK(n.stats.strength == 22 && n.stats.dexterity == 11 && n.stats.intelligence == 37); // sub_1282D: (40-30)/4+20, (12-9)/2+10, (99-30)/4+20
		CHECK(n.experience == 250 && n.level == 3 && n.hitPoints == 90); // 2500 / 1000 = 2 has two bits: level 3
		CHECK(n.stats.karma == 0x4B);
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
