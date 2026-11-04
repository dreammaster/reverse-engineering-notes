#pragma once
// The character screen of the roster (WL.EXE 0x19130..0x19360): the summary, dividing and pooling money, using, equipping,
// trading and dropping items.
#include "wasteland1/machine.h"

namespace wl {

// 18325: divides the 24 bit value (high byte at ds:BA86, middle `mid`, low `low`) by `divisor`; quotient and remainder stay in
// ds:BA84..BA86 / BA80
void divideMoney(Machine &m, uint8_t &high, uint8_t &mid, uint8_t &low, uint8_t divisor);
// 19202: asks what to do with the item in inventory slot `slot` of party member ds:CCED
void useItem(Machine &m, uint8_t slot);
// 19343: lists the items of the member and runs useItem for the chosen one; returns the key code that ended the list
uint8_t useItemScreen(Machine &m);
// 19130: the character screen of party member `member`
void characterInfo(Machine &m, uint8_t member);

} // namespace wl
