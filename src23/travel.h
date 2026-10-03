#ifndef YENDOR23_TRAVEL_H
#define YENDOR23_TRAVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * Party teleport/fast-travel destinations: TravelToDestination
 * (yendor2.asm:18170, yendor3.asm:10081) and its unlock gate
 * IsDestinationUnlocked (yendor2.asm:18260, yendor3.asm:10142).
 * Reached from a WorldObjectFlagUnknown2000 world-object record's own
 * `value` field (worldobjects.h) -- a 1-based index into the table
 * below. This is the mechanism behind this project's long-open
 * "region/town password" question: most locked destinations require
 * literally typing a password (a real English word, embedded in the
 * data) at a prompt -- see file-formats.md's "Party teleport/
 * fast-travel destinations" section for the full writeup, including
 * the extracted real password words for both games.
 *
 * Both tables' entry counts (187 Chapter 2, 139 Chapter 3) are proven
 * exact by address arithmetic in the extraction scripts
 * (ida_scripts/dump_travel_destination_table.py, one per game): each
 * destination table's own end lands exactly on its IsDestinationUnlocked
 * gate table's start, with zero gap.
 *
 * Deliberately NOT covered here: the music-track/travel-mode side
 * effects TravelToDestination itself applies on arrival
 * (word_36CB1/word_36CB3/word_36C79/word_36CBF in Chapter 2,
 * ds:0xCF2F/0xCF31/0xCF33/0xCF3F/0xCEF9 in Chapter 3) -- none of these
 * have been traced to a confirmed consumer, so composing them here
 * would mean building against unconfirmed inputs. This module only
 * covers the position/facing lookup and the unlock decision, both
 * fully understood.
 */

enum {
    TravelDestinationCountYendor2 = 187,
    TravelDestinationCountYendor3 = 139
};

unsigned travelDestinationCount(GameKind game);

typedef struct {
    int worldX, worldY;
    uint16_t facing;  /* one of the 4 SaveFacing bits */
    uint16_t sound;   /* arrival sound id, 0 = silent */
    uint16_t flags;   /* +0xE, raw -- per-game bit meaning differs, see file-formats.md */
    uint16_t rawA;    /* Chapter 2 +0xA: day-track music id (word_36CB1). Chapter 3 +8: ds:0xCF2F, undecoded. */
    uint16_t rawB;    /* Chapter 2 +0xC: night-track music id (word_36CB3). Chapter 3 +0xA: ds:0xCF31, undecoded. */
    uint16_t rawC;    /* Chapter 3 only, +0xC: a small "travel mode" value (1-7 observed), ds:0xCF33, undecoded. 0 for Chapter 2 (no such field). */
    uint16_t rawD;    /* Chapter 3 only, +0x10: ds:0xCEF9, undecoded. 0 for Chapter 2 (no such field). */
} TravelDestination;

/* 1-based id, matching the value a WorldObjectFlagUnknown2000 record's own `value` field supplies. */
bool travelDestinationLookup(GameKind game, unsigned id, TravelDestination *out);

typedef enum {
    TravelUnlockAlreadyUnlocked,    /* proceed: no gate-table entry at all, or the global flag is already set */
    TravelUnlockDeniedWithMessage,  /* refused; messageId selects a canned rejection string (the gate row's own hasMsg) */
    TravelUnlockDeniedFixedMessage, /* Chapter 3 only: refused with a different, fixed 3-line message, not table-selected */
    TravelUnlockDeniedSilent,       /* refused, no message at all -- Chapter 3's "neither flag bit set" case, presumed unreachable in real data */
    TravelUnlockNeedsPassword       /* caller should prompt (promptId), then call travelResolvePassword with what the player typed */
} TravelUnlockOutcome;

typedef struct {
    TravelUnlockOutcome outcome;
    uint16_t messageId; /* valid when outcome == TravelUnlockDeniedWithMessage */
    uint16_t promptId;  /* valid when outcome == TravelUnlockNeedsPassword */
} TravelUnlockCheck;

/*
 * IsDestinationUnlocked's own decision logic, with no UI performed here.
 * destinationFlags is the looked-up TravelDestination's own `flags`
 * field (+0xE): Chapter 2 ignores it entirely (its gate table's hasMsg
 * alone decides message-vs-password); Chapter 3 re-tests it a second
 * time, once the gate row's own hasMsg is 0, to choose between the
 * fixed-message and password sub-cases -- a real per-game difference,
 * not an oversight. globalFlags is the caller's g_globalFlags-equivalent
 * buffer (see globalflags.h), read only, never written here.
 */
TravelUnlockCheck travelCheckUnlock(GameKind game, unsigned destinationId, uint16_t destinationFlags,
                                     const uint8_t *globalFlags, size_t globalFlagsSize);

/*
 * Called after the player types text at a TravelUnlockNeedsPassword
 * prompt. Compares up to 12 characters against the stored password; a
 * stored space ends the comparison early and still counts as a match
 * (so typed text longer than the stored word is accepted, matching the
 * original's own byte-loop exactly -- not a general "prefix ok"
 * design choice, just what the disassembly does). Case-sensitive, like
 * the original; uppercasing typed input is the caller's job. On a
 * match, sets the same global flag travelCheckUnlock would then read as
 * AlreadyUnlocked and returns true. Returns false with no effect if
 * destinationId has no gate-table entry, or its stored password can
 * never match (the one Chapter 2 destination whose stored text is all
 * null bytes).
 */
bool travelResolvePassword(GameKind game, unsigned destinationId, const char *typedText,
                            uint8_t *globalFlags, size_t globalFlagsSize);

/*
 * ExamineTarget (yendor2.asm:53404 / yendor3.asm:54337) -- the handler for an
 * item whose consumable-target entry has flag 0x4 and no container/conversation
 * bits (word 1 & 0x7E00 == 0): the KEY OF PORT HOPE / PARIAH / NUMAGIK / STONY
 * PEAK / TRACKING of Chapter 2 (target word 0 = 0x000C) and ANKH OF PORTALS /
 * ATHANEUM KEY of Chapter 3 -- the only seven such items. The entry's word 2
 * (+4) is a travel destination id. HandleGameCommand first refuses the item
 * (a warning flash) while UI scratch flag 0x1000 is set.
 *
 * Decision: if the destination has a gate-table row (the same table
 * travelCheckUnlock reads) whose global flag is NOT set, the item only shows
 * its description ("ShowAbilityDescriptionColumn", cx=1) -- it never prompts for
 * the password, unlike a travel object; otherwise it calls TravelToDestination.
 *
 * Chapter 3 prepends a story gate: while global flag 0x9D is set (the "spirit
 * realm" HandleScriptedStoryEventTrigger enters, set on event 5, cleared on
 * event 6) and the party holds any item 0x17F-0x181 (SWORD / HAMMER / TRIDENT
 * OF LIGHT -- storyItemHeld, from itemRangeAvailable), the item shows a 3-line
 * refusal (and sets UI flag 0x100 in word 0x536A) instead; if none is held the
 * flag 0x9D is cleared on the spot and the ordinary path runs. Chapter 2 has no
 * such gate.
 */
typedef enum {
    TravelExamineTravels,
    TravelExamineGateClosed,
    TravelExamineStoryBlocked
} TravelExamineOutcome;

enum { TravelStoryRealmFlag = 0x9D, TravelStoryItemLow = 0x17F, TravelStoryItemHigh = 0x181 };

TravelExamineOutcome travelExamineKey(GameKind game, unsigned destinationId, uint8_t *globalFlags, size_t globalFlagsSize,
                                      bool storyItemHeld);

#endif
