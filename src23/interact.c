#include "interact.h"

bool interactBitmapTest(SaveGame *save, unsigned bitIndex) {
    uint8_t *byte = saveGameRecord(save, SaveSectionEventState, bitIndex / 8);
    if (!byte) {
        return false;
    }
    return (*byte >> (7 - (bitIndex % 8))) & 1;
}

void interactBitmapSet(SaveGame *save, unsigned bitIndex) {
    uint8_t *byte = saveGameRecord(save, SaveSectionEventState, bitIndex / 8);
    if (byte) {
        *byte |= (uint8_t)(0x80 >> (bitIndex % 8));
    }
}

unsigned interactLockBitIndex(unsigned lockId) {
    return lockId - 1;
}

unsigned interactCurgameIdOffset(GameKind game) {
    return game == GameYendor2 ? LockRecordCountYendor2 : 0;
}

unsigned interactCurgameBitIndex(GameKind game, unsigned curgameId) {
    return curgameId - 1 + interactCurgameIdOffset(game);
}

InteractBranch interactSelectBranch(const WorldObjectRecord *object) {
    if (!object) {
        return InteractBranchNone;
    }
    if (object->flags & WorldObjectFlagDoor) {
        return InteractBranchLock;
    }
    if (object->flags & WorldObjectFlagCurgameRecord) {
        return InteractBranchCurgame;
    }
    if (object->flags & WorldObjectFlagFixedResponse) {
        return InteractBranchFixedResponse;
    }
    if (object->flags & WorldObjectFlagMonsterSpawn) {
        return InteractBranchMonsterSpawn;
    }
    return InteractBranchNone;
}

InteractOutcome interactClassifyLock(const LockRecord *lock, bool alreadyUnlocked) {
    if (alreadyUnlocked) {
        return InteractOutcomeNone;
    }
    if (lock->flags & LockFlagMagical) {
        return InteractOutcomeLockMagical;
    }
    if (lock->flags & LockFlagUnknown40) {
        return InteractOutcomeLockFlag40;
    }
    if (lock->price != 0) {
        return InteractOutcomeLockPriced;
    }
    return InteractOutcomeNone;
}

InteractOutcome interactClassifyCurgame(uint16_t curgameFlags, bool alreadyTriggered) {
    if (alreadyTriggered) {
        return InteractOutcomeNone;
    }
    if (curgameFlags & 0x10) {
        return InteractOutcomeCurgameFlag10;
    }
    if (curgameFlags & 0x8) {
        return InteractOutcomeCurgameFlag8;
    }
    if (curgameFlags & 0x40) {
        return InteractOutcomeCurgameFlag40;
    }
    if (curgameFlags & 0x20) {
        return InteractOutcomeCurgameFallbackB;
    }
    return InteractOutcomeCurgameFallbackA;
}

InteractOutcome interactClassifyMonsterSpawn(bool alreadySpawned) {
    return alreadySpawned ? InteractOutcomeNone : InteractOutcomeUnspawnedMonster;
}

InteractOutcome interactClassify(const WorldObjectRecord *object, const LockRecord *lock, bool lockAlreadyUnlocked,
                                  uint16_t curgameFlags, bool curgameAlreadyTriggered, bool monsterAlreadySpawned) {
    switch (interactSelectBranch(object)) {
    case InteractBranchLock:
        return interactClassifyLock(lock, lockAlreadyUnlocked);
    case InteractBranchCurgame:
        return interactClassifyCurgame(curgameFlags, curgameAlreadyTriggered);
    case InteractBranchFixedResponse:
        return InteractOutcomeFixedResponse;
    case InteractBranchMonsterSpawn:
        return interactClassifyMonsterSpawn(monsterAlreadySpawned);
    case InteractBranchNone:
    default:
        return InteractOutcomeNone;
    }
}

unsigned interactWorldObjectBitIndex(GameKind game, const WorldObjectRecord *object) {
    if (object->flags & WorldObjectFlagDoor) {
        return interactLockBitIndex(object->value);
    }
    return interactCurgameBitIndex(game, object->value);
}

bool interactResolveIfOutcome(SaveGame *save, GameKind game, const WorldObjectRecord *object, const LockRecord *lock,
                               bool lockAlreadyUnlocked, uint16_t curgameFlags, bool curgameAlreadyTriggered,
                               bool monsterAlreadySpawned, const InteractOutcome *qualifying,
                               unsigned qualifyingCount) {
    InteractOutcome outcome = interactClassify(object, lock, lockAlreadyUnlocked, curgameFlags,
                                                curgameAlreadyTriggered, monsterAlreadySpawned);
    if (outcome == InteractOutcomeNone) {
        return false;
    }
    for (unsigned i = 0; i < qualifyingCount; i++) {
        if (outcome == qualifying[i]) {
            interactBitmapSet(save, interactWorldObjectBitIndex(game, object));
            return true;
        }
    }
    return false;
}

bool interactKnock(SaveGame *save, GameKind game, const WorldObjectRecord *object, const LockRecord *lock,
                    bool lockAlreadyUnlocked, uint16_t curgameFlags, bool curgameAlreadyTriggered,
                    bool monsterAlreadySpawned) {
    static const InteractOutcome qualifying[] = {InteractOutcomeLockMagical, InteractOutcomeCurgameFallbackB};
    return interactResolveIfOutcome(save, game, object, lock, lockAlreadyUnlocked, curgameFlags,
                                     curgameAlreadyTriggered, monsterAlreadySpawned, qualifying, 2);
}

bool interactTriggerFacingCurgameEvent(SaveGame *save, GameKind game, const WorldObjectRecord *object,
                                        const LockRecord *lock, bool lockAlreadyUnlocked, uint16_t curgameFlags,
                                        bool curgameAlreadyTriggered, bool monsterAlreadySpawned) {
    static const InteractOutcome qualifying[] = {InteractOutcomeCurgameFlag10, InteractOutcomeCurgameFlag8};
    return interactResolveIfOutcome(save, game, object, lock, lockAlreadyUnlocked, curgameFlags,
                                     curgameAlreadyTriggered, monsterAlreadySpawned, qualifying, 2);
}
