#ifndef ULTIMA6_ULTIMA6_TRANSFER_H
#define ULTIMA6_ULTIMA6_TRANSFER_H

#include "files/data_file.h"
#include "ultima6/gypsy.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * Characters that can be brought over from Ultima V (SAVED.GAM) or Ultima IV (PARTY.SAV), as U.EXE's transfer_character_menu reads them (sub_1276D, sub_12630) and
 * turns them into the Avatar of a new game (sub_12904 for Ultima V, sub_12863 for Ultima IV).
 */
struct TransferCharacter {
	std::string name;
	bool female = false;
	int strength = 0, dexterity = 0, intelligence = 0;
	int experience = 0; // the points of the old game
	bool fromUltimaIV = false;

	/** Ultima V: sixteen records of 32 bytes from offset 2 (name 8, sex at +9: 0x0B male, 0x0C female, strength +0xC, dexterity, intelligence, experience at +0x14, all within the original's limits). */
	static void readUltimaV(const Bytes &savedGam, std::vector<TransferCharacter> &out);
	/** Ultima IV: eight records of 0x27 bytes from offset 8 (hit points +0, maximum +2, experience +4, strength +6, dexterity +8, intelligence +10, name +0x14, sex +0x24: 0x0B male). */
	static void readUltimaIV(const Bytes &partySav, std::vector<TransferCharacter> &out);

	/** The new Avatar: Ultima IV's statistics are scaled (sub_1282D), experience is a tenth of the old points, the level one more than the bits of points / 1000, hit points 30 a level. */
	NewCharacter toNewCharacter() const;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
