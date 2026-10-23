// Register adapters: map action handlers (see actions.h).
#include "wasteland1/ports/port.h"
#include "wasteland1/actions.h"

namespace wl {
namespace rc {

PORT(act_a470) { actions::branchIfOtherPartyAt(m); r.cf = false; }   // 1A470
PORT(act_a4f4) { actions::branchIfFirstUnconscious(m); r.cf = false; }   // 1A4F4
PORT(act_a526) { actions::branchByDayTime(m); r.cf = false; }   // 1A526
PORT(act_a54b) { actions::stashFirstMember(m); r.cf = false; }   // 1A54B
PORT(act_a611) { actions::copyActionChain(m); r.cf = false; }   // 1A611
PORT(act_a699) { actions::branchIfItemOrDrop(m); r.cf = false; }   // 1A699
PORT(act_a6f5) { actions::patchJumpAction9(m); r.cf = false; }   // 1A6F5
PORT(act_a738) { actions::patchJumpActionShort(m); r.cf = false; }   // 1A738
PORT(act_a773) { actions::afflictSelected(m); r.cf = false; }   // 1A773
PORT(act_a795) { actions::afflictFirst(m); r.cf = false; }   // 1A795
PORT(act_a7b6) { actions::afflictAll(m); r.cf = false; }   // 1A7B6
PORT(act_a879) { actions::branchIfPartyAt(m); r.cf = false; }   // 1A879
PORT(act_a8af) { actions::toggleSex(m); r.cf = false; }   // 1A8AF
PORT(act_a8d2) { actions::afflictRoster(m); r.cf = false; }   // 1A8D2
PORT(act_a903) { actions::decrement31(m); r.cf = false; }   // 1A903
PORT(act_a919) { actions::increment2F_2(m); r.cf = false; }   // 1A919
PORT(act_a931) { actions::decrement32(m); r.cf = false; }   // 1A931
PORT(act_a947) { actions::decrement2F_2(m); r.cf = false; }   // 1A947
PORT(act_a961) { actions::increment32(m); r.cf = false; }   // 1A961
PORT(act_a977) { actions::increment31Max0B(m); r.cf = false; }   // 1A977
PORT(act_a98d) { actions::increment31Max0C(m); r.cf = false; }   // 1A98D
PORT(act_a9a3) { actions::increment31Max11(m); r.cf = false; }   // 1A9A3
PORT(act_a9bc) { actions::set2F_14(m); }   // 1A9BC
PORT(act_a9c7) { actions::increment2F_10(m); r.cf = false; }   // 1A9C7
PORT(act_a9dd) { actions::setTable3Byte9Range10_21(m); r.cf = false; }   // 1A9DD
PORT(act_aa12) { actions::setTable3Byte9At22(m); r.cf = false; }   // 1AA12
PORT(act_aa3f) { actions::setTable3Params0_1D(m); r.cf = false; }   // 1AA3F
PORT(act_aa8e) { actions::setTable3Byte9To1Range21_28(m); r.cf = false; }   // 1AA8E
PORT(act_aabf) { actions::patchJumpAction9(m); r.cf = false; }   // 1AABF
PORT(act_ab35) { actions::checkGateCells(m); r.cf = false; }   // 1AB35
PORT(act_ab7b) { actions::patchJumpActionF(m); r.cf = false; }   // 1AB7B
PORT(act_abca) { actions::setTable3Byte9Range0_9(m); r.cf = false; }   // 1ABCA
PORT(act_acf0) { actions::set2F_0(m); r.cf = false; }   // 1ACF0
PORT(act_acfc) { actions::set2F_1E(m); r.cf = false; }   // 1ACFC
PORT(act_ad08) { actions::set2F_64(m); r.cf = false; }   // 1AD08
PORT(act_ad14) { actions::branchIfItemListed(m); r.cf = false; }   // 1AD14

} // namespace rc
} // namespace wl
