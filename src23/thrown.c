#include "thrown.h"

#include "monster.h"

ThrownKind thrownKindForItem(GameKind game, unsigned itemId) {
    if (game == GameYendor3) {
        switch (itemId) {
        case 0x3D:
            return ThrownGoldPotion;
        case 0x3E:
            return ThrownSilverPotion;
        case 0x3F:
            return ThrownBluePotion;
        case 0x3C:
            return ThrownFlamingOil;
        default:
            return ThrownNone;
        }
    }
    switch (itemId) {
    case 0x19:
        return ThrownGoldPotion;
    case 0x1A:
        return ThrownSilverPotion;
    case 0x1B:
        return ThrownBluePotion;
    case 0x240:
        return ThrownFlamingOil;
    default:
        return ThrownNone;
    }
}

ThrownEffect thrownResolveAbilityEffect(GameKind game, unsigned itemId, uint8_t *monsterRecord, bool inFormalCombat, RandomState *rng) {
    ThrownEffect effect = {0, 0, 0, false};
    monsterSetU16(monsterRecord, MonsterFieldTickAmount, 0);
    monsterSetU16(monsterRecord, MonsterFieldTickCountdown, 0);
    if (randomInRange(rng, 100) > ThrownFailAbove) {
        return effect;
    }
    ThrownKind kind = thrownKindForItem(game, itemId);
    switch (kind) {
    case ThrownGoldPotion:
        effect.damage = 60;
        break;
    case ThrownSilverPotion:
        if (!(monsterGetU16(monsterRecord, MonsterFieldImmunities) & MonsterImmunePoison)) {
            effect.statusFlags = 0x8000;
            monsterSetU16(monsterRecord, MonsterFieldTickAmount, 6);
            monsterSetU16(monsterRecord, MonsterFieldTickCountdown, 5);
        }
        effect.damage = 35;
        break;
    case ThrownBluePotion:
        if (monsterGetU16(monsterRecord, MonsterFieldUnknown4E) == ThrownKindUndead &&
            !(monsterGetU16(monsterRecord, MonsterFieldResistances) & ThrownResistHolyBlock)) {
            effect.damage = game == GameYendor3 ? 85 : 50;
        }
        break;
    case ThrownFlamingOil:
        if (inFormalCombat) {
            return effect; /* returns before even the type flag is set */
        }
        effect.damage = 40;
        effect.corridor = true;
        break;
    default:
        break;
    }
    effect.typeFlags = ThrownTypePhysical;
    if (effect.statusFlags != 0 && randomInRange(rng, 100) > ThrownStatusKeepAtMost) {
        effect.statusFlags = 0;
    }
    return effect;
}

bool thrownApplyResolvedDamage(uint8_t *monsterRecord, const ThrownEffect *effect) {
    if (effect->damage == 0) {
        return false;
    }
    uint16_t state = monsterGetU16(monsterRecord, MonsterFieldState);
    state = (uint16_t)(state | ((uint16_t)~monsterGetU16(monsterRecord, MonsterFieldImmunities) & effect->statusFlags));
    uint16_t overlap = (uint16_t)(monsterGetU16(monsterRecord, MonsterFieldResistances) & effect->typeFlags);
    uint16_t damage = effect->damage;
    for (unsigned bit = 0; bit < 16; bit++) {
        if (overlap & (1u << bit)) {
            damage >>= 1;
        }
    }
    int32_t health = (int32_t)monsterGetU16(monsterRecord, MonsterFieldHealth) - damage;
    monsterSetU16(monsterRecord, MonsterFieldHealth, health > 0 ? (uint16_t)health : 0);
    monsterSetU16(monsterRecord, MonsterFieldState, (uint16_t)(state | 3));
    return true;
}

uint16_t thrownWeaponTypeFlags(uint16_t entryWord1, uint16_t entryWord4) {
    uint16_t flags = ThrownTypePhysical;
    if (entryWord1 & 0x0400) {
        flags |= ThrownTypeWeaponMagic;
    }
    if (entryWord4 != 0) {
        flags |= ThrownTypeWeaponSecondary;
    }
    return flags;
}

void thrownCorridorRowStarts(unsigned depthRow, unsigned starts[3]) {
    static const unsigned kTable[4][3] = {{0x18, 0x23, 0x27}, {0x23, 0x27, 0x2A}, {0x27, 0x2A, 0x2D}, {0x2A, 0x2D, 0x30}};
    unsigned row = depthRow == 0x24 ? 0 : depthRow == 0x28 ? 1 : depthRow == 0x2B ? 2 : 3;
    for (unsigned i = 0; i < 3; i++) {
        starts[i] = kTable[row][i];
    }
}
