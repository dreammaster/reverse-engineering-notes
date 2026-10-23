/* PROG5.EXE's data segment: the offsets the port uses, with the names of the original's fields where the code shows what they mean.
 * The character record (100 bytes) is read into DS:09DFh; the whole party file sits at DS:05F7h. */

#ifndef YENDOR1_PROG5_PROG5LAYOUT_H
#define YENDOR1_PROG5_PROG5LAYOUT_H

#include "prog5/Prog5.h"

namespace Yendor1 {
namespace Prog5 {
namespace Layout {

enum {
	kRecordSize = 100,

	// the party file in memory: record 0 is the header (first word = number of records, 10 + characters)
	kParty = 0x05F7,
	kCount = 0x05F7,
	kSlots = 0x061D,            // six words: the record numbers of the party members, 0 = empty
	kHeaderFlags = 0x0629,      // 10h: a special picture (the party strip shows a bare armour), 20h: ...
	kLeader = 0x06AB,           // word_13BFB: the record of the member the others follow
	kStock = 0x06BF,            // words: how many of each item the party has, indexed by item number - 1

	// the character record being worked on
	kRec = 0x09DF,
	kName = 0x09DF,             // 26 bytes, NUL-terminated
	kClass = 0x09F9,
	kStatus = 0x09FA,           // 0 well; 6 dead, 7..0Dh ailments (the picture changes), 9 ..., 0Eh and up: gone
	kLevel = 0x09FB,
	kGold = 0x09FC,             // four bytes of BCD, most significant first
	kSpriteFile = 0x0A00,       // the tile file of the character's pictures
	kSpriteBase = 0x0A01,       // first tile of the body set
	kHeadBase = 0x0A03,         // first tile of the head set
	kPortrait = 0x0A05,         // first BATPIC.DAT record of this character's 5x4 portrait
	kStat0 = 0x0A08,            // six attribute bytes, then six maximum values
	kStatMax0 = 0x0A0E,
	kFlags = 0x0A14,            // 01h in the party, 40h portrait needs re-rendering, 80h portrait rendered
	kPoints = 0x0A15,           // hit and magic points (words at A15, A17, A19, A1B)
	kArmour = 0x0A1D,           // the three equipped items (item numbers, 0 = none)
	kShield = 0x0A1F,
	kWeapon = 0x0A21,
	kDerived73 = 0x0A23, kDerived75 = 0x0A25, kBonus77 = 0x0A27, kBonus79 = 0x0A29, kDerived7B = 0x0A2B, kBonus7D = 0x0A2D,
	kBonus7F = 0x0A2F, kDerived81 = 0x0A31, kBonus83 = 0x0A33,

	// tables and scratch
	kBcdScratch = 0x4FDE,
	kWeaponTable = 0x4CB6,      // (item, pointer to its sprite tiles) pairs, 0-terminated
	kArmourTable = 0x4F28,
	kWeaponA = 0x4F52, kWeaponB = 0x4F54, kWeaponShift = 0x4F56,
	kPortraitTile = 0x4F50,
	kItems = 0x1B65,            // 35-byte item entries, the name at +0Eh
	kClassNames = 0x165F,       // 15-byte entries
	kStatusNames = 0x16D7,
	kImageBuffer = 0x0EBB       // the 512-byte image buffer the driver reads and writes
};

} // namespace Layout
} // namespace Prog5
} // namespace Yendor1

#endif
