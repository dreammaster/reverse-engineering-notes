#pragma once
// Combat and encounter data helpers of WL.EXE (0x12A40..0x13A56 and 0x15xxx).
//
// The monsters of an encounter live in a table at ds:6B31: 4 groups x 4 slots x 94 bytes. A slot has a 4 byte header
// (x, y, ...) followed by 3 "members" of 30 bytes; a member holds the hit points (words) of up to 10 monsters followed
// by 10 status bytes. Combatants are numbered 0..47 as (group * 4 + slot) * 3 + member.
#include "wasteland1/machine.h"

namespace wl {

constexpr uint16_t kCombatTable = 0x6B31;
constexpr uint16_t kGroupBytes = 0x178, kSlotBytes = 0x5E, kMemberBytes = 0x1E, kSlotHeaderBytes = 4;

struct CombatantId { uint8_t group, slot, member; };

// 13A56 / 13A37: combatant number <-> (group, slot, member)
CombatantId splitCombatantIndex(uint8_t index);
uint8_t combatantIndex(uint8_t group, uint8_t slot, uint8_t member);

// 137F4: monsterPtr = the slot record; 137CE: monsterPtr = the member record inside it
void selectSlot(Machine &m, uint8_t group, uint8_t slot);
void selectMember(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 13787: (x, y) stored in the header of the selected slot
struct Position { uint8_t x, y; };
Position slotPosition(Machine &m);
// 138B0: zeroes the 94 bytes of the selected slot
void clearSlot(Machine &m);
// 13924: partyStatusPtr / partyOrdersPtr = the two 8 byte arrays of party group `group` inside the savegame (offsets 0x38..)
void selectPartyBlock(Machine &m, uint8_t group);
// 13449 / 1353C: low six bits / top two bits of the order byte of party member `index`
uint8_t orderLow(Machine &m, uint8_t index);
uint8_t orderHigh(Machine &m, uint8_t index);

// ---- encounter (location record) and item record accessors --------------------------------------------------------------
// 12A40 (continuing into 12A4C): the monster type id of member `index` (0..2) of the saved location record, then
// getMonsterDetails of it
uint8_t encounterMemberId(Machine &m, uint8_t index);
// 12A4C: itemDetailsPtr = record `id` of the map's monster table (8 byte records; the pointer variable is shared with the
// item table); returns the sex flag table entry stored in itemSexFlag
uint8_t getMonsterDetails(Machine &m, uint8_t id);
void selectMonsterDetails(Machine &m, uint8_t member);       // 12A40
// 1379E / 13797 / 13787: savedLocationDetailsPtr = the location record named by (class, action) / by the cell (x, y) /
// by the position of the selected slot
void saveLocationRecord(Machine &m, uint8_t table, uint8_t index);
void selectCellRecord(Machine &m, uint8_t x, uint8_t y);
void selectSlotCellRecord(Machine &m);
// 15CC0: selectSlot, selectSlotCellRecord and selectMonsterDetails(member)
void selectMonsterType(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 12A8D / 12A92 / 12A9D / 12AA2 / 12AAB / 12ABA: fields of the item record found by getItemDetails
uint8_t itemDamageDice(Machine &m);           // 12A8D: low nibble of byte 5 (dice count)
uint8_t itemClipSizeNibble(Machine &m);       // 12A92: low nibble of byte 4
uint8_t itemAmmunitionType(Machine &m);       // 12A9D: byte 7
uint8_t itemArmorClassByte(Machine &m);       // 12AA2: byte 6
// 12AC5 / 15C08 / 15C27: flags of the saved location record
uint8_t locationRecordBits2(Machine &m);      // byte 9 >> 2
uint8_t locationRecordBits4(Machine &m);      // byte 9 >> 4
void markLocationVisited(Machine &m);         // byte 9 = (byte 9 | 1) & 0FDh

// 1391E / 13878 / 138C1: readiness of the combatant (state byte at 46CC) and ammunition accounting
void markReady(Machine &m);
uint8_t combatReadiness(Machine &m, uint8_t partyNum);
uint8_t useAmmunition(Machine &m, uint8_t rounds, bool *zeroFlag = nullptr);   // zeroFlag: the zero flag the original returns
// 15705..15714: damage bonus of the current character's luck / dexterity / agility / strength (16 bit, may be negative)
uint16_t attributeBonus(Machine &m, uint8_t attributeOffset);
// weapon class predicates (item types): true when the type belongs to the class
bool isTypeGroupA(uint8_t type);      // 15C6E: 5, 6, 7
bool isTypeGroupB(uint8_t type);       // 15C7E: 0Ah, 0Bh, 0Ch
bool isTypeGroupC(uint8_t type);       // 15C8C: 8, 9
// 15CB2: n * 10 (8 bit)
uint8_t timesTen(uint8_t n);
// 157AD: number of monsters of the member (group, slot, member) that are still alive (hit points != 0)
uint8_t countAliveMonsters(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 15C96: quantity and jam byte of the equipped weapon
uint8_t weaponQuantityByte(Machine &m);

// ---- combat messages ----------------------------------------------------------------------------------------------------
// 1393E: prints string `bl` of the string index at the start of the map block's location data
void printLocationMessage(Machine &m, uint8_t index);
// 13939 / 1394D: location message `base` + random(2)
void printLocationMessageVariant(Machine &m, uint8_t base);
// 15A84: number in dl:al, then message 59h ("... N damage"-style); itemQuantity is 1 for exactly one point
void printDamageNumber(Machine &m, uint16_t value);
// 15C36: itemQuantity = n; message 62h or a variant of 59h depending on the hit flag
void printHitMessage(Machine &m, uint8_t n);
// 15BBC / 15BCB / 15BEF: attack announcement chosen from the weapon type
void printAttackMessageA(Machine &m);
void printAttackMessageB(Machine &m);
void printAttackMessageC(Machine &m);
// 1592C: both bytes of the current character's hit points = value
void setHitPointBytes(Machine &m, uint8_t value);
// 156DE: 0, 1 or 2 depending on the weapon type (damage class)
uint8_t weaponDamageClass(uint8_t type);
// 15672: damage dice of a monster member: sumOfD6(0, dice count of the member's record)
uint16_t rollMonsterDamage(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 15A18: numberValue = the first two bytes of the item/monster record (0 as third byte)
void recordWordToNumber(Machine &m);
// 1597E: reduces `damage` by the armor roll of the monster combatant `index` (the roll happens even when armor is ignored,
// byte hitIgnoresArmor); the result never goes below 0
uint16_t reduceByMonsterArmor(Machine &m, uint16_t damage, uint8_t index);
// 159C7: numberValue = experience for killing one monster of combatant `index`: record word * (dice nibble + 1)
void monsterExperience(Machine &m, uint8_t index);
// 15755: number of dice rolled for the defense of party member `partyNum` against a monster of toughness `threshold`
uint16_t defenseRoll(Machine &m, uint8_t partyNum, uint8_t threshold);
// 15738: defenseRoll plus the luck bonus of the current character
uint16_t attackStrength(Machine &m, uint8_t partyNum, uint8_t threshold);
// 15500: party member `partyNum` hits a random monster of combatant `index`. Result: alreadyDead when the chosen monster
// had no hit points (try again); otherwise `killed` is true when the hit killed it
struct HitResult { bool alreadyDead, killed; };
HitResult hitMonster(Machine &m, uint8_t partyNum, uint8_t index);
// 15A30: a combatant was fought with the equipped weapon: maybe improve the weapon skill (needs ammunition left)
void weaponSkillRoll(Machine &m, uint8_t index);
// 157D6: damage (16 bit) to party member `member`; true when the member was already dead (nothing happens)
bool damagePartyMember(Machine &m, uint16_t damage, uint8_t member);
// 15A9D: combatant `index` (group, slot, member) takes a hit on monster number `monster` (0..9) of the member. Damage is the
// current seed1 value reduced by the monster's armor. Returns false when that monster was already dead (CF set, nothing
// happens); `killed` is set when the hit killed it.
bool attackMonster(Machine &m, uint8_t index, uint8_t monster, bool &killed);
// 129E9: writes the name of the monster group `index` (0..2) of the selected location record: a table string when bit 0 of
// the record's byte 9 is set, else the standard name 52h + armor class byte
void writeMonsterName(Machine &m, uint8_t index);
// 129D5 / 129DA: message 43h / 1Ah, then the monster name `index` (the routines fall into 129E9)
void printMonsterMessage(Machine &m, uint8_t id, uint8_t index);
// 13697: writes "<count> <name>" of the monster group `index` of the location record; false (CF) when the group is empty
bool writeEncounterGroup(Machine &m, uint8_t index);
// 13651: writes the whole encounter description (all groups) followed by the distance `distance`
void describeEncounter(Machine &m, uint8_t distance);
// 13726 / 1372B: sets bytes 14h..1Dh (the status bytes) of the three members of slot `slot` of the active party group to value
void setSlotStatus(Machine &m, uint8_t slot, uint8_t value);
// 12A76: damage roll of the monster group `group` of the location record: byte 5 high nibble + (byte 3) d6
uint16_t monsterDamage(Machine &m, uint8_t group);
// 1295D: a monster of `group` attacks a random living member of the party, unless the member is protected (status 5 and
// flag `b` clear and level `a` < 6); the member is retried while already dead
void monsterHitsParty(Machine &m, uint8_t a, uint8_t b, uint8_t group);
// 129B6: true when some member of the active group still has hit points (CF clear in the original)
bool anyMemberAlive(Machine &m);
// 141FA: applies seed2 to field `offset` of party member `member`: subtracts (flags bit 7 set) or adds. Hit points go through
// damagePartyMember without armor messages; money and experience through the number value
void adjustMemberField(Machine &m, uint8_t member, uint8_t offset, uint8_t flags);
// 142D7 / 142E2: bytes 8 (masked with 7Fh) and 0 of the action data being executed
uint8_t actionByte8(Machine &m);
uint8_t actionByte0(Machine &m);
// 156CB: range modifier of the monster group `group` at `distance` (selects the group's monster record)
uint8_t monsterRangeModifier(Machine &m, uint8_t distance, uint8_t group);
// 12783: the monsters of the location around the active party get their free attacks (ambush); `partyGroup` selects the
// party block. Stops early when the whole party is down
void ambush(Machine &m, uint8_t partyGroup);
// 12D3F: every able party member of group `partyGroup` whose order is 2 and who can act attacks
void partyAttackRound(Machine &m, uint8_t partyGroup);
// 1820C: skill/attribute check of the current character: field `offset` against difficulty `count`. Offset 18h (nationality)
// must equal count, 24h (level) must be at least count; any other field rolls dice against 15 + 5 * count. True on success
bool fieldCheck(Machine &m, uint8_t count, uint8_t offset);
// ---- the table of monster groups that follow the party (16 entries of {x, y, party id, distance} at ds:A96F) --------------
struct EntrySlot { uint8_t group, slot; };
// 14B9C: entry offset (4 * n) -> party group n / 4 and slot n % 4
EntrySlot entrySlot(uint8_t entryOffset);
// 14B36: the slot whose record starts with the entry's {x, y, party id}; false when none does
bool findEntrySlot(Machine &m, uint8_t entryOffset, EntrySlot &found);
// 14AEA: entry (offset / 4) holding {x, y, party id}; false when there is none
bool findEntry(Machine &m, uint8_t partyId, uint8_t x, uint8_t y, uint8_t &entry);
// 14BAD: swaps the 5Eh byte records of slot (group, slot) and of the slot of the entry
void swapEntrySlot(Machine &m, uint8_t group, uint8_t slot, uint8_t entryOffset);
// 14480: places the monsters of every table entry into its slot and generates the encounter (random counts and types,
// hit points) of the entries that are new
void placeTrackedMonsters(Machine &m);
// ---- the orders given to the party outside combat (status byte of each member: 1 talk, 3 equip, 5/6 reload / message, 7 ...) --
// 19ACD: tries to clear the jam of the weapon of party member `member` (weapon item at inventory offset `weaponOffset`)
void unjam(Machine &m, uint8_t member, uint8_t weaponOffset);
// 130B1: order "equip" of party member `member`
void equipOrder(Machine &m, uint8_t member);
// 13228 / 1322B: order "reload" of party member `member`
void reloadOrder(Machine &m, uint8_t member);
// 12CC3: carries out the order of every member of the party group `partyGroup` that is able to act
void executeOrders(Machine &m, uint8_t partyGroup);
// 14AAA: forgets the tracked group standing on the pending cell (A60D, A60C) of the active party
void forgetTrackedGroup(Machine &m);
// 14A88: records the pending cell, the map id and ds:A60B as entry `entryOffset`
void recordTrackedGroup(Machine &m, uint8_t entryOffset);
// 14AC7: false (CF) when the pending move belongs to another party group; otherwise runs the move (sub_18024)
bool continuePendingMove(Machine &m);
// 136C7: stores the distance to the party into byte 3 of every monster slot of the groups on the current map
void updateSlotDistances(Machine &m);
// 169CF: true (CF) when cell (x, y) is outside the visible window
bool outsideView(Machine &m, uint8_t x, uint8_t y);
// 149F7: for every party group (3..0) that can reach the pending cell, the distance table at ds:7111 gets the distance, FFh otherwise
void updateGroupDistances(Machine &m);
// 14664: looks at every cell of the view window for monster markers (cell classes 3 and 0Fh), draws them and keeps the table of
// tracked monster groups; true (CF) when nothing new was found
bool scanForMonsters(Machine &m, bool draw = true);   // 1465F: without drawing
// 1AD73: one combat round of the party group `partyGroup`: initiative of monsters and party members, then everybody acts
// in turn (party members shoot, monsters attack) until the table is used up
void combatInitiative(Machine &m, uint8_t partyGroup);
void evasionChance(Machine &m, uint8_t base);
void combatRound(Machine &m, uint8_t partyGroup);
// ---- monsters walking towards (or away from) the party
// 16840: true (CF) when a party group stands on the cell
bool partyAtCell(Machine &m, uint8_t x, uint8_t y);
// 14F26: true (CF) when a monster cannot enter the cell (edge, party, outside the map or not open ground)
bool monsterCellBlocked(Machine &m, uint8_t x, uint8_t y);
// 14ECB: tries the step (ds:A641, A642) from the monster position (ds:A63F/A640), else one axis only; gives the move direction
bool monsterStepDirection(Machine &m, uint8_t &direction);
// 14E4A / 14E45: step direction towards (or away from) the party, trying single axis steps when the diagonal is blocked
bool monsterApproach(Machine &m, bool away, uint8_t &direction);
// 14FDE / 14F9D: average hit points of the monster group (to seed2) / of the party members (to seed1)
void averageMonsterHitPoints(Machine &m);
void averagePartyHitPoints(Machine &m);
// 14F7A: true (CF) when the monsters are too weak against the party (party average >> shift beats monster average)
bool monsterHoldsBack(Machine &m, uint8_t shift);
// 15036: carries out the monster moves planned in the table at ds:711D (direction in the low six bits, message type in the
// top two): the monster slot moves one cell and the move is announced
void moveMonsters(Machine &m);
// 14BF0: plans the move of every monster slot on the map (table at ds:711D, see moveMonsters): towards the party, away from it
// when the monsters are weak, or none
void planMonsterMoves(Machine &m);
// 13762: the cell change of the saved location record is applied at the position of the selected slot (al = record offset)
bool restoreLocationAtSlot(Machine &m, uint8_t offset);
// 135C6: after a fight the survivors of slot (group, slot) are written back into the location record: counts per monster group,
// types cleared for groups that are gone; a slot without survivors is cleared (and its cell restored). True (CF) when cleared
bool updateSlotSurvivors(Machine &m, uint8_t group, uint8_t slot);
// 13580: updateSlotSurvivors for every slot on the current map
void updateAllSlotSurvivors(Machine &m);
// 122E9: can the selected monster slot be targeted: it is on this map, within the weapon range (ds:46CC), inside the view,
// within the distance the location record allows and in line of sight. `distance` receives the distance; true when targetable
bool slotTargetable(Machine &m, uint8_t &distance);
// 13828: number of monster groups party member `member` can target with the equipped weapon
uint8_t countTargetableGroups(Machine &m, uint8_t member);
// 17AB7: itemQuantity = 1
void setItemQuantityOne(Machine &m);
// ---- the round of one party member -----------------------------------------------------------------------------------------
// 13440: alive monster count of the target group of party member `member` (0: CF set in the original)
uint8_t memberTargetsAlive(Machine &m, uint8_t member);
// 13079: alive monsters in the member `ds:A559` of the combatant `ds:A55B`
uint8_t targetMembersAlive(Machine &m);
// 1306C: prints message 3Eh once the special weapon note has been shown
void printWeaponNote(Machine &m);
// 13086: number of monsters of this round's target entry `ds:A55A` that were hit; also stored as the item quantity
uint8_t countHitMonsters(Machine &m);
// 19D4D: distance table entry between cell (x, y) and the party position
uint8_t cellDistance(Machine &m, uint8_t x, uint8_t y);
// 1595C: sum of max(1, n / 4) rolls of 1d4 (rounds of a burst)
uint8_t burstHits(Machine &m, uint8_t n);
// 15956: burst size of party member `member`: burstHits of the ammunition accounting result
uint8_t burstSize(Machine &m, uint8_t member);
// 134B2: the party member's roll to fire: false (CF clear) when the attack goes ahead with the roll accumulated in seed2;
// true when the weapon jammed or the attack is aborted
bool rollToFire(Machine &m, uint8_t member);
// 139CE: weapon range modifier. Builds a table of (threshold, base, base + 7) triples at ds:7931 with thresholds 0, 5, ..,
// 100 (the bases accumulate the per-class increment from ds:A5B8+cls / A5BB+cls) and returns the entry (1 + ammoFlag) bytes
// behind the first triple whose threshold is >= distance
uint8_t rangeModifier(Machine &m, uint8_t distance, uint8_t ammoFlag, uint8_t weaponClass);
// 1567F: the difficulty the distance and the weapon give the attack of party member `member` (added to seed1)
uint8_t attackDifficulty(Machine &m, uint8_t member);
// 13452: the party member's turn ends: status and orders cleared; a thrown weapon is used up and re-equipped from the
// stack of the same item
void finishTurn(Machine &m, uint8_t member);
// 12D84: the attack of party member `member` against the monsters of its target group
void partyMemberAttack(Machine &m, uint8_t member);
// 15CD1: random 0..9
uint8_t randomDigit(Machine &m);
// 15C4A: remembers whether a damage value (dl:al) is below the "overkill" limit in the hit flag
uint8_t setHitFlag(Machine &m, uint16_t damage);

} // namespace wl
