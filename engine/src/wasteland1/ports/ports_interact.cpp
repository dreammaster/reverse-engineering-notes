// Register adapters: matching action conditions (see interact.h).
#include "wasteland1/ports/port.h"
#include "wasteland1/interact.h"

namespace wl {
namespace rc {

PORT(conditionType) { r.a.b.l = conditionType(m, r.b.b.l); }                            // 1417F: bl = record offset
PORT(conditionValue) { r.a.b.l = conditionValue(m, r.b.b.l); }                          // 1418A
PORT(skillRoll) {                                                                       // 18146: al = skill id
	const int roll = skillRoll(m, r.a.b.l);
	r.cf = roll < 0;
	if (roll >= 0) r.a.b.l = uint8_t(roll);
}
PORT(skillCheck) { r.cf = !skillCheck(m, r.a.b.l, r.d.b.l); }                           // 180F0: al = skill id, dl = difficulty
PORT(useUpItem) { useUpItem(m, r.b.b.l); }                                              // 19A58: bl = inventory offset
PORT(matchItemCondition) {                                                              // 14090: bl = inventory offset
	uint8_t at = 0;
	r.cf = !matchItemCondition(m, r.b.b.l, at);
	if (!r.cf) { r.b.b.l = at; r.a.b.l = 0; }
}
PORT(matchSkillCondition) {                                                             // 140DD: al = skill id
	uint8_t at = 0;
	r.cf = matchSkillCondition(m, r.a.b.l, at);
	r.b.b.l = at;
	r.a.b.l = 0;
}
PORT(matchAttributeCondition) {                                                         // 14126: al = attribute id
	uint8_t at = 0;
	r.cf = matchAttributeCondition(m, r.a.b.l, at);
	r.b.b.l = at;
	r.a.b.l = 0;
}

} // namespace rc
} // namespace wl
