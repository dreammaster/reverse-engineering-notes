// Register adapters: key driven menus (see menu.h).
#include "wasteland1/ports/port.h"
#include "wasteland1/globals.h"
#include "wasteland1/buttons.h"
#include "wasteland1/combat.h"
#include "wasteland1/combatmenu.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"
#include "wasteland1/roster.h"
#include "wasteland1/shop.h"

namespace wl {
namespace rc {

PORT(chooseByKey) {                                                                     // 173B0
	const uint8_t at = chooseByKey(m);
	r.a.b.l = at; r.b.b.l = at;
	r.sf = at & 0x80; r.zf = at == 0;
}
PORT(pickGroupMember) { r.a.b.l = pickGroupMember(m, r.a.b.l != 0); r.cf = r.a.b.l == 0; }   // 1721B
PORT(askDirection) {                                                                    // 125BD
	const int d = askDirection(m);
	r.cf = d < 0;
	r.a.b.l = r.b.b.l = uint8_t(d);
}

PORT(chooseUseAction) {                                                                 // 13AE4: al = member
	uint8_t mode = 0, target = 0, what = 0;
	r.cf = !chooseUseAction(m, r.a.b.l, mode, target, what);
	r.a.b.l = mode; r.d.b.l = target; r.b.b.l = what;
}

PORT(listWindowSetup) { listWindowSetup(m); r.a.b.l = 0; }                              // 16EAA
PORT(pagedListDraw) { pagedListDraw(m); }                                               // 16DB4
PORT(pagedListBack) { pagedListBack(m); }                                               // 16E43
PORT(pagedListKeys) { const ListResult l = pagedListKeys(m); r.cf = l.cf; r.a.b.l = l.al; }   // 16D34
PORT(markPickedRow) { markPickedRow(m); }                                               // 16F20
PORT(skillList) {                                                                       // 1997B: al = title
	const ListResult l = skillList(m, r.a.b.l);
	r.cf = l.cf; r.a.b.l = l.al; if (l.empty) r.d.b.l = 1;
}
PORT(skillScreen) { const ListResult l = skillScreen(m); r.cf = l.cf; r.a.b.l = l.al; }   // 198F0
PORT(itemList) { const ListResult l = itemList(m, r.a.b.l); r.cf = l.cf; r.a.b.l = l.al; }   // 19454
PORT(itemScreen) { const ListResult l = itemScreen(m, r.a.b.l); r.cf = l.cf; r.a.b.l = l.al; }   // 19394

PORT(rearrangeParty) { rearrangeParty(m); }                                             // 12AE0

PORT(drawRosterRow) { drawRosterRow(m); }                                               // 1708B
PORT(redrawRosterRowHere) { redrawRosterRow(m, r.a.b.l, 0xFF); }                          // 17029: al = position, inverse text
PORT(redrawRosterRowPlain) { redrawRosterRow(m, r.a.b.l, 0); }                            // 1702E
PORT(redrawRosterRowKeep) { redrawRosterRow(m, r.a.b.l, m.d8(0x471F)); }                  // 17033
PORT(drawRosterFrame) { drawRosterFrame(m); }                                           // 16F70

PORT(drawRosterHeadersAndMembers) { drawRosterHeadersAndMembers(m); }                  // 1728C (al is preserved)
PORT(showTitleText) { showTitleText(m, r.a.b.l); }                                      // 17923

PORT(countGroupsInRange) { r.a.b.l = countGroupsInRange(m); }                              // 1382B
PORT(showTargetList) { showTargetList(m, r.a.b.l); }                                    // 12262
PORT(chooseTargetGroup) {                                                               // 1236F
	uint8_t group = 0;
	r.cf = !chooseTargetGroup(m, group);
	r.a.b.l = group;
}

namespace {
void setResult(Regs &r, const CommandResult &c) { r.cf = c.cf; r.a.b.l = c.al; }
}
PORT(collectOrders) { r.cf = collectOrders(m); }                                         // 11F76
PORT(commandPause) { setResult(r, commandPause(m, r.a.b.l)); }                          // 1208D
PORT(commandUseItem) { setResult(r, commandUseItem(m, r.a.b.l)); }                      // 1234E
PORT(commandMove) { setResult(r, commandMove(m, r.a.b.l)); }                            // 123A6
PORT(commandReload) { setResult(r, commandReload(m, r.a.b.l)); }                        // 123E2
PORT(commandUseSkill) { setResult(r, commandUseSkill(m, r.a.b.l)); }                    // 1240B
PORT(commandAttack) { setResult(r, commandAttack(m, r.a.b.l)); }                        // 120C1
PORT(pickFirstTarget) { setResult(r, pickFirstTarget(m)); }                             // 121A8

PORT(runCombat) { runCombat(m); }                                                       // 11D39

PORT(regroupParty) { regroupParty(m); r.cf = true; }                                                 // 130C8

PORT(recruitMonster) { r.cf = recruitMonster(m, r.a.b.l); }                              // 132AC

PORT(getButtonsText) { getButtonsText(m); r.a.x = m.d16(g::textString); }                // 17564
PORT(findButton) { r.cf = !findButton(m); }                                              // 17533
PORT(drawButtonPlain) { drawButton(m, false); }                                         // 17451
PORT(drawButtonHighlighted) { drawButton(m, true); }                                    // 1744C
PORT(drawButtonsRow) { drawButtonsRow(m); }                                             // 173D2
PORT(buttonKey) { buttonKey(m); }                                                       // 17574

PORT(textInput) { r.a.b.l = textInput(m, false); }                                          // 17748
PORT(textInputAl) { r.a.b.l = textInput(m, r.a.b.l != 0); }                              // 17745

PORT(askKeyword) { r.cf = askKeyword(m); }                                                // 15160

PORT(splitParty) { splitParty(m); }                                                     // 15E82

PORT(askYesLoose) { r.cf = askYesLoose(m); r.a.b.l = m.d8(g::lastKey); }                  // 19B4F
PORT(switchToGroup) { r.cf = switchToGroup(m, r.a.b.l); }                                // 160E1

PORT(payNumberValue) { r.cf = payNumberValue(m); }                                        // 17B15
PORT(poolMoney) { poolMoney(m, r.a.b.l); r.cf = true; }                                               // 19B81
PORT(doctorShop) { r.cf = doctorShop(m); }                                               // 1C260

PORT(raiseMaxCon) { r.sf = raiseMaxCon(m, r.a.b.l); r.b.b.l = 0x1C; }          // 1BB32
PORT(trainingMessage) { trainingMessage(m, r.a.b.l); }                                   // 1BB5D
PORT(setRankTitle) { setRankTitle(m, r.a.b.l); }                                          // 1BB6C
PORT(distributePoints) { distributePoints(m, r.a.b.l); }                                  // 1BA72

PORT(skillCost) { r.a.b.l = skillCost(r.a.b.l, r.d.b.l); }                                // 1C68E (al = base, dl = level)
PORT(buildTrainableList) { buildTrainableList(m); }                                       // 1BD37
PORT(skillSchool) { r.cf = skillSchool(m); }                                              // 1BBA0

PORT(trainingRowOfSkill) { r.cf = trainingRowOfSkill(m, r.a.b.l); }                                     // 1BD94

PORT(itemTypeAccepted) { r.cf = !itemTypeAccepted(m); }                                   // 17AF5
PORT(sellPrice) { sellPrice(m); }                                                         // 1C1C2
PORT(buyPrice) { buyPrice(m); }                                                           // 1C1CC
PORT(countSellable) { r.d.b.l = countSellable(m); r.a.b.l = r.d.b.l; }                    // 1963F
PORT(sellScreen) { sellScreen(m); }                                                       // 1BF5F
PORT(countStock) { r.a.b.l = countStock(m); }                                             // 1C140
PORT(buyScreen) { buyScreen(m); }                                                         // 1C073

PORT(textInputCaseSensitive) { r.a.b.l = textInput(m, false, true); }                      // 17750
PORT(creationRowPort) { r.cf = creationRow(m, r.a.b.l); }                                 // 1C622 (al = skill)
PORT(createCharacter) { createCharacter(m); }                                             // 1C6C9

PORT(rollAttribute) { r.a.b.l = rollAttribute(m, r.a.b.l); }                                // 1CAD1 (bx is kept)
PORT(startingItems) { startingItems(m); }                                                  // 1C9DE

PORT(writeMemberAttribute) { writeMemberAttribute(m, r.b.b.l); }                          // 19579
PORT(writeMemberStats) { writeMemberStats(m); r.a.b.l = 0; r.d.x = 0; r.b.x = 0; }                      // 19506

PORT(writeSummary) { writeSummary(m); }                                                  // 19362

} // namespace rc
} // namespace wl
