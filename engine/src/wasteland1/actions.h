#pragma once
// Handlers of the map action interpreter of WL.EXE (0x1A470..0x1AD14). The dispatch table sits in the data segment (ds:A4CC,
// one near pointer per action type). A handler works on the action record at locationDetailsPtr: bytes 1 and 2 name the next
// action (table, index) and the handler may replace them with one of the alternatives stored in the record's parameters (a
// "branch" copies the pair at offset 3 or 5 over bytes 1 and 2). The original returns the carry flag; it is set when the
// action sequence must stop.
#include "wasteland1/machine.h"

namespace wl {
namespace actions {

// 1A470: another party group stands at one of the (x, y) pairs listed from offset 7 -> branch 3, else branch 5
void branchIfOtherPartyAt(Machine &m);
// 1A4F4: first member unconscious -> branch 5, else branch 3
void branchIfFirstUnconscious(Machine &m);
// 1A526: saved2 byte 0Ch in 6..17 -> branch 3, else 5
void branchByDayTime(Machine &m);
// 1A54B: stores a copy of the first member (inventory, weapon, armor and money cleared) in stash slot rec[3]
void stashFirstMember(Machine &m);
// 1A611: copies the action chain of table 5 index rec[4] over the one of index rec[3] up to its end marker
void copyActionChain(Machine &m);
// 1A699: members without item 2Fh drop to -5 hit points; branch 3 when somebody had the item, else 5
void branchIfItemOrDrop(Machine &m);
// 1A6F5 / 1AABF: jump action 5E,{rec[3],rec[4]} written into index 9 of table 5; 1A738: index 19h with rec[3] only; 1AB7B: index 0Fh
void patchJumpAction9(Machine &m);
void patchJumpActionShort(Machine &m);
void patchJumpActionF(Machine &m);
// 1A773 / 1A795 / 1A7B6 / 1A8D2: afflictions of the table at ds:CF4E[rec[3]] are added to the member(s)
void afflictSelected(Machine &m);
void afflictFirst(Machine &m);
void afflictAll(Machine &m);
void afflictRoster(Machine &m);
// 1A879: party stands at (rec[7], rec[8]) -> branch 3 else 5
void branchIfPartyAt(Machine &m);
// 1A8AF: toggles the sex of every roster character
void toggleSex(Machine &m);
// 1A903..1A9C7: bytes 2Fh, 31h, 32h of the map block (light / time counters) stepped up or down
void decrement31(Machine &m);
void increment2F_2(Machine &m);
void decrement32(Machine &m);
void decrement2F_2(Machine &m);
void increment32(Machine &m);
void increment31Max0B(Machine &m);
void increment31Max0C(Machine &m);
void increment31Max11(Machine &m);
void set2F_14(Machine &m);
void increment2F_10(Machine &m);
void set2F_0(Machine &m);
void set2F_1E(Machine &m);
void set2F_64(Machine &m);
// 1A9DD / 1AA12 / 1AA3F / 1AA8E / 1ABCA: bytes of the records of table 3 set to a parameter
void setTable3Byte9Range10_21(Machine &m);
void setTable3Byte9At22(Machine &m);
void setTable3Params0_1D(Machine &m);
void setTable3Byte9To1Range21_28(Machine &m);
void setTable3Byte9Range0_9(Machine &m);
// 1AB35: cells (13h,9..11) all of class 4 action 2 -> next action 0C,3
void checkGateCells(Machine &m);
// 1AD14: some member carries an item listed from offset 7 -> branch 3, else branch 5
void branchIfItemListed(Machine &m);

} // namespace actions
} // namespace wl
