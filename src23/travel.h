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

/*
 * The arrival half of TravelToDestination (yendor2.asm:18170-18240, yendor3.asm:10081-10130), once the gate has been passed (2026-10-08):
 *
 *   Chapter 2: the arrival sound (if any); the low three bits of the place flags (word_36C79, save header +0x1A) are replaced: flags 0x8000 -> 4, else 0x4000 -> 2 (and
 *     the light sources are ticked, TickTravelResourceAilments), else 0x1000 -> 1, else 0 (those bits pick the indoor lighting gradients A / B / C, lighting.h; the rest of
 *     the word is the lit light sources and stays); the party's position and facing from the record -- except that a destination with flags 0x800 whose position is (98, 83)
 *     sends the party to (378, 227) when the clock is at or before 07:00 or at or after 19:00 (the clock at 420 / 1140 minutes); the day and night tracks from rawA / rawB.
 *   Chapter 3: the travel mode (ds:0CF33 = rawC), the arrival sound, the whole travel-flag word (ds:0CEF9, save header +0x1C) = rawD, position and facing, the day and night
 *     tracks (ds:0CF2F / 0CF31 = rawA / rawB, save header +0x52 / +0x54).
 * The window is then rebuilt and the cells around the party revealed.
 */
typedef struct {
    int x, y;
    uint16_t facing;
    uint16_t sound;       /* 0 = none */
    uint16_t placeFlags;  /* the new place-flag word (Chapter 3: the travel flags) */
    uint16_t dayTrack, nightTrack;
    uint16_t travelMode;  /* Chapter 3; 0 in Chapter 2 */
    bool tickLights;      /* Chapter 2: the light sources burn one tick (flags 0x4000 destinations) */
} TravelArrival;

TravelArrival travelArrival(GameKind game, const TravelDestination *destination, uint16_t placeFlags, uint16_t clockMinutes);

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

/*
 * UseAbilityOnTarget (yendor2.asm:45586, yendor3.asm:46029): a "discovery key" used on a destination object (flag 0x2000) whose gate-table row names it -- the row's hasMsg word doubles as the key's
 * item id (Chapter 2: 0x242-0x245 for destinations 170-176; Chapter 3: 0x176 for destination 58). No row: nothing happens. The row's flag already set: UNLOCKED. The right key: the flag is set
 * and UNLOCKED. Any other item: LOCKED.
 */
typedef enum { TravelKeyNoRow, TravelKeyAlready, TravelKeyUnlocked, TravelKeyWrong } TravelKeyOutcome;
TravelKeyOutcome travelUseKeyOnDestination(GameKind game, unsigned destinationId, unsigned itemId, uint8_t *globalFlags, size_t globalFlagsSize);

TravelExamineOutcome travelExamineKey(GameKind game, unsigned destinationId, uint8_t *globalFlags, size_t globalFlagsSize,
                                      bool storyItemHeld);

#endif
