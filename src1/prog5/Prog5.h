/* Yendor1::Prog5 -- PROG5.EXE, the character screens opened from the book menu: Create a character (mode 1), View (2), Remove (3)
 * and Assemble the party (4). parm_0E is the mode, parm_02 the caller's variant (0 = from the book menu, 1/2 = from the game).
 * The program keeps its state in tables and records addressed by offset, so it works on a Shared::DataSegment loaded from PROG5.EXE;
 * the offsets are the original's data-segment offsets, named where their meaning is known. Function names map to the disassembly
 * (prog5.idb). The whole PARTY.DAT (10 records of 100 bytes; characters follow) is read into DS:05F7h by `loadParty` and written back
 * by `saveParty`. */

#ifndef YENDOR1_PROG5_PROG5_H
#define YENDOR1_PROG5_PROG5_H

#include "shared/DataSegment.h"
#include "shared/Launcher.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Ui.h"

#include <vector>

namespace Yendor1 {
namespace Prog5 {

using Shared::byte;
using Shared::uint16;

class Prog5 {
public:
	Prog5(Shared::Runtime &rt, const Shared::Params &params);

	/** Run the program for the mode in parm_0E; the DOS exit code is returned (0 normal, FDh bad mode, FEh out of memory). */
	int run();

	/** The generator the attribute rolls use (tests give it the original run's seed). */
	Shared::Rng &rng() { return _ui.rng; }

private:
	// ---- the three level of helpers the shared code of the original provides ----
	bool readRecord(uint16 record, uint16 dest);               // fileReadRecordTo(PARTY.DAT)
	void writeRecord(uint16 record, uint16 src);               // fileWriteRecordFrom(PARTY.DAT)
	void loadParty();                                          // shared_d2992b: records 0..9 to DS:05F7
	void saveParty();                                          // shared_4a4289
	void pickLeaderRecord();                                   // shared_c9c8cc
	uint16 formatNumber(uint16 value, uint16 dest);            // shared_72afff: "12,345" into DS at dest, returns the end
	void money(int op, uint16 bcd);                            // shared_8f30c2
	bool moneyAtLeast(uint16 bcd);
	void moneyAsText(uint16 dest);
	void addMoneyBinary(uint16 value);
	void deriveStats();                                        // shared_d809a1
	void drawKeyed();                                          // drawTileKeyed
	void drawTileScratch();                                    // shared_9790fc
	void drawBackdrop();                                       // drawBackdropTile
	void drawTileAt(uint16 x, uint16 y, uint16 tileFile, uint16 tile);   // shared_063c13 + keyed copy
	void drawBodyTile(uint16 tile);                            // shared_89c35e
	void shieldTile(int row);                                  // shared_6228a1
	void weaponTables();                                       // shared_9cebbd
	void composeCharacter();                                   // shared_63ca2b
	void copyLayers();                                         // shared_ab7386
	void composeSimple();                                      // shared_cf62f3
	void drawPartyStrip();                                     // drawPartyStrip: render the character and store the portrait tiles
	uint16 slotWord(int slot) const;                           // the character record in party slot 0..5 (0 = empty)
	void stepStat(uint16 offset);

	// ---- the screens ----
	bool buildList();                                          // sub_10AEF
	void listEntry(uint16 entry);                              // sub_10D1C
	void drawCreateFrame();                                    // sub_10F44
	void drawCreateLabels();                                   // sub_10F4D
	uint16 itemNameOffset(uint16 item) const;                  // sub_10F7D
	bool noPurchaseRoom();                                     // sub_10F8D
	uint16 itemLacks(uint16 item);                             // sub_11008
	void clearRecord();                                        // sub_110A9
	void drawItemC();                                          // sub_113CB
	void drawItemA();                                          // sub_110C3
	void drawItemB();                                          // sub_113AB
	void drawAttributes();                                     // sub_110E3
	void drawPoints();                                         // sub_111C7
	void drawCombat();                                         // sub_1129A
	void drawGoldText();                                       // sub_112E1
	void drawStatus();                                         // sub_11305
	void drawLevel();                                          // sub_11325
	void drawSheet();                                          // sub_1134F
	void changeItem(uint16 item);                              // sub_113EB
	void rollStats();                                          // sub_11498
	void chooseSex();                                          // sub_11740 (menu at DS:163Bh)
	void chooseClass();                                        // sub_11569 (menu at DS:1643h)
	void chooseName();                                         // sub_11582
	void choosePortrait();                                     // sub_11651
	void drawPortraitChoice();                                 // sub_11759
	void putStrip(uint16 buffer, byte row);                    // sub_117CD / sub_1183E (put / get)
	void getStrip(uint16 buffer, byte row);
	void selectList();                                         // sub_1188A
	void accept();                                             // sub_11D79: the stat roll
	void menuChoice(uint16 table);                             // sub_12DFC
	void readText(uint16 dest, int length);                    // shared_ede455
	void restoreBackground();                                  // shared_b65991
	void saveBackground();                                     // shared_b65991_11819
	void putFrame(uint16 entry, byte colour);                  // shared_0bb58f for the list entries
	byte waitFor(byte keys, byte icons, byte flags);           // shared_21818f
	void showMessageBox(uint16 boxOffset, uint16 textOffset);

	void modeCreate();                                         // loc_101E4
	void modeView();                                           // loc_1032D
	void modeRemove();                                         // loc_1062B
	void modeAssemble();                                       // loc_107C9
	void assembleParty();                                      // loc_10925
	void editEquipment();                                      // loc_10407
	void finish();                                             // loc_10AB7

	Shared::Runtime &_rt;
	Vid::Context &_c;
	Shared::BookUi _ui;
	Shared::Params _params;
	Shared::DataSegment _ds;
	bool _haveExe;
	byte _descBuffer[100];       // the party file descriptor's record buffer
	uint16 _descRecord;          // [DS:052Fh + 8]: the record number the party file descriptor points at
	byte _status;                // byte_1481F: the input result, file error flag and the screens' "cancelled" flag in one
	byte _cur;                   // byte_1481B: the selected list entry (1-based)
	byte _savedCur;              // byte_14814
	byte _iconMask;              // byte_14813
	uint16 _record;              // word_1481C: the record number of the selected character
	uint16 _listCount;           // word_13FED: number of entries in the list
	byte _firstRow;              // byte_14829
	uint16 _scratchX, _scratchY; // word_1480B / word_1480D: where the portrait is composed
	uint16 _destX, _destY;       // word_1480F / word_14811
	byte _file;                  // byte_12C8: tile file for the next tile
	uint16 _tile;                // word_12C9: tile index for the next tile
	std::vector<byte> _bufferA, _bufferB;   // the two saved screen areas (word_14820, word_14822)
	bool _quit;
};

} // namespace Prog5
} // namespace Yendor1

#endif
