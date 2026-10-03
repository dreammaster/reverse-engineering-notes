#ifndef YENDOR23_DIALOG_H
#define YENDOR23_DIALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The NPC conversation / service catalog -- the data behind `UseItem`
 * (yendor2.asm:13169; the same structure in Chapter 3), which `start` calls
 * for a world object (worldobjects.h) with the matching flag, passing the
 * object's own `value` field (its +4 word) as the NPC id. The name "UseItem"
 * and the older "item data catalog" name for this data were early guesses:
 * what it actually holds is every talking character in the game -- their
 * dialogue text and the services they offer (shops, healers, trainers, key
 * and tome givers, the enhance/repair/sell screens, the riddle/password
 * checks). Found 2026-10-03 while decoding LoadItemData
 * (yendor2.asm:22216), which reads three WORLD.DAT blocks through the
 * PrepareItemDataBlockRead28/3A/22 stubs; confirmed by reading the text
 * itself ("AS YOU APPROACH, YOU CAN SEE THAT THE GOVERNOR IS VERY CONCERNED
 * ABOUT SOMETHING.", "WELCOME TO OUR ESTABLISHMENT..."). (RunConversation,
 * by contrast, shows found documents -- document.h.)
 *
 * Three tables, each indexed from 0 with entry 0 a dummy/blank:
 *
 *   NPC headers, 40 bytes. NPC id N is entry N. See DialogNpcField.
 *   Topics, 58 bytes in Chapter 2 and 60 in Chapter 3 (one extra word at
 *     +0x10, before the argument; every field from there on sits 2 bytes
 *     later -- dialogTopicU16 hides this): one per conversation keyword ("HELLO", "BLACKWING",
 *     "NUORE", "DONE", "BYE", "PURCHASE FOOD", ...). An NPC owns the
 *     contiguous run [DialogNpcFirstTopic, + DialogNpcTopicCount). Chapter 2
 *     has 929 entries, Chapter 3 1073.
 *   Text lines, 34 bytes: 33 characters (space padded; '%' marks a line
 *     break in the displayed text, '~' is the font's apostrophe) then a NUL.
 *     An NPC's lines are the run [DialogNpcFirstLine, + DialogNpcLineCount);
 *     Chapter 2 has 3218 lines, Chapter 3 4090. Each topic says which of its
 *     NPC's lines it shows: DialogTopicTextOffset is a *byte* offset (a
 *     multiple of 34) from the NPC's first line and DialogTopicTextLines a
 *     line count.
 *
 * Within each game the NPCs tile all three tables exactly: every header's
 * first topic/line equals the previous header's first + count, which is how
 * the table sizes above were determined (and are checked by the tests).
 *
 * The file offsets come from the stubs' DS words (0xCE43/0xCE47/0xCE4F in
 * Chapter 2, 0xB1DB/0xB1DF/0xB1E7 in Chapter 3; yendor{2,3}/ida_scripts/
 * dump_itemdata_offsets.py). Chapter 2's header block is followed by 1760
 * bytes of slack (including a leftover build path), Chapter 3's isn't.
 */

enum {
    DialogNpcRecordSize = 40,
    DialogTopicRecordSizeYendor2 = 58,
    DialogTopicRecordSizeYendor3 = 60,
    DialogTopicRecordSizeMax = 60,
    DialogLineSize = 34,
    DialogLineChars = 33,
    DialogTopicNameSize = 13, /* characters, space padded; a NUL follows */

    DialogNpcCountYendor2 = 106, /* slots, including the blank slot 0 */
    DialogNpcCountYendor3 = 141,
    DialogTopicCountYendor2 = 929,
    DialogTopicCountYendor3 = 1073,
    DialogLineCountYendor2 = 3218,
    DialogLineCountYendor3 = 4090,

    DialogNpcCountMax = 141,
    DialogTopicCountMax = 1073,
    DialogLineCountMax = 4090
};

/* Offsets within an NPC header. */
typedef enum {
    DialogNpcPictureId = 0x00, /* picture id (category 0x70) drawn in the dialog frame: the portrait */
    /*
     * word_3197C. Nonzero means LoadItemData first asks which party member is
     * speaking (ShowConfirmPrompt 0x0B, rejecting incapacitated ones); values
     * seen: 0, 2, 4. ComputeCostMessageIndentMode also keys its text-wrapping
     * thresholds off it.
     */
    DialogNpcSpeakerMode = 0x02,
    DialogNpcTopicCount = 0x04,
    DialogNpcLineCount = 0x06,
    DialogNpcFirstTopic = 0x08,
    DialogNpcFirstLine = 0x0A,
    /*
     * Three global-flag ids (0 = unused). LoadItemData picks the topic the
     * conversation opens on (word_2E550): the NPC's 2nd topic if the 0x0C
     * flag is set, else 3rd if 0x0E, else 4th if 0x10, else the 1st -- the
     * NPC's first greeting ("HELLO"), second ("HELLO-2") and so on.
     */
    DialogNpcGreetingFlagA = 0x0C,
    DialogNpcGreetingFlagB = 0x0E,
    DialogNpcGreetingFlagC = 0x10,
    /*
     * 0x12 and up: service parameters whose meaning depends on the topic
     * handler that reads them (all through DS:0xBCE, the header buffer):
     * 0x12 the one-time global flag a tome-giver sets
     * (UseAttributeBoostItem/UseExperienceBoostItem), 0x14/0x16 the stat's
     * party-record offset and the amount (attribute tomes; also a min/max
     * pair for IsItemEligibleForEnhance and the cap UseTrainingItem
     * checks), 0x18 a price multiplier (ShowHealingCostPrompt), 0x1A the
     * per-character flag-bank index used with TestRecordFlag_10C /
     * SetRecordFlag_10C ("this character already used this service"),
     * 0x1C/0x1E cost-message parameters, 0x20 an item range the party must
     * hold (IsItemRangeAvailable, feeding word_2E40E bit 1).
     */
    DialogNpcOneTimeFlag = 0x12,
    DialogNpcParamA = 0x14,
    DialogNpcParamB = 0x16,
    DialogNpcPriceMultiplier = 0x18,
    DialogNpcCharacterFlagIndex = 0x1A,
    DialogNpcParamC = 0x1C,
    DialogNpcParamD = 0x1E,
    DialogNpcRequiredItemRange = 0x20
} DialogNpcField;

/* Offsets within a topic record. */
typedef enum {
    DialogTopicName = 0x00,       /* 13 characters + NUL */
    DialogTopicFlags = 0x0E,      /* es:[si+0Eh] in UseItem: see DialogTopicFlag */
    /* The offsets below are Chapter 2's; Chapter 3's are 2 higher (use dialogTopicU16). */
    DialogTopicArg = 0x10,        /* es:[si+10h] (Chapter 3: +12h): handler argument (an exit code for bit 0x1, a type word, a key id, ...) */
    DialogTopicTextOffset = 0x12, /* byte offset of the topic's first line from the NPC's first line (multiple of 34) */
    DialogTopicTextLines = 0x14,  /* number of lines shown */
    DialogTopicBit = 0x16,        /* this topic's bit in the NPC's topic mask (0x8000, 0x4000, ... down to 1) */
    DialogTopicUnlocks = 0x1A,    /* bits of the same mask this topic makes available */
    DialogTopicBitCopy = 0x1E,    /* equal to DialogTopicBit in most records; -1 in the rest */
    DialogTopicFlagFirst = 0x22   /* the first of six signed global-flag ids ApplyItemEffectFlags walks: set if > 0, cleared (negated) if < 0 */
} DialogTopicField;

/*
 * DialogTopicFlags bits as UseItem tests them (es:[si+0Eh]). Partial: the
 * handlers behind most are UI-heavy and not yet reimplemented.
 */
typedef enum {
    DialogTopicEndsConversation = 0x0001, /* after showing the text, wait for a key and leave; DialogTopicArg <= 2 becomes the exit code (word_2E52A) */
    DialogTopicRepair = 0x0040,           /* RunRepairItemScreen */
    DialogTopicBuy = 0x0080,              /* "BUY ..." topics: PromptBuyOreQuantity; other 0x80 topics are riddles (UseRiddleAnswerItem) */
    DialogTopicEnhance = 0x0100,          /* RunEnhanceItemScreen */
    DialogTopicAttributeBoost = 0x0200,   /* UseAttributeBoostItem */
    DialogTopicExperienceBoost = 0x0400,  /* UseExperienceBoostItem */
    DialogTopicPreview = 0x0800,          /* ShowItemUsagePreview */
    DialogTopicCheckKey = 0x1000,         /* CheckKeyItem (the arg is a lock id) */
    DialogTopicSell = 0x4000,             /* RunSellItemScreen after ConfirmAndValidatePartyTarget */
    DialogTopicUseKey = 0x8000            /* UseKeyItem */
} DialogTopicFlag;

typedef struct {
    GameKind game;
    uint16_t npcCount;
    uint16_t topicCount;
    uint16_t lineCount;
    uint8_t npcs[DialogNpcCountMax * DialogNpcRecordSize];
    uint16_t topicRecordSize;
    uint8_t topics[DialogTopicCountMax * DialogTopicRecordSizeMax];
    uint8_t lines[DialogLineCountMax * DialogLineSize];
} DialogCatalog;

typedef struct {
    uint32_t npcOffset, topicOffset, lineOffset; /* WORLD.DAT file offsets */
    uint16_t npcCount, topicCount, lineCount;
    uint16_t topicRecordSize;
} DialogLayout;

const DialogLayout *dialogLayout(GameKind game);

/* Parses from a whole WORLD.DAT image already in memory. False if it's too short. */
bool dialogCatalogParseWorldDat(DialogCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

/* NPC header for id 1..npcCount-1 (0 and out of range are NULL), a topic by absolute index, a line by absolute index. */
const uint8_t *dialogNpc(const DialogCatalog *catalog, unsigned id);
const uint8_t *dialogTopic(const DialogCatalog *catalog, unsigned index);
const uint8_t *dialogLine(const DialogCatalog *catalog, unsigned index);

uint16_t dialogGetU16(const uint8_t *record, unsigned offset);

/* A topic field by its (Chapter 2) DialogTopicField offset, compensating for Chapter 3's shifted layout. */
uint16_t dialogTopicU16(const DialogCatalog *catalog, const uint8_t *topic, DialogTopicField field);

/* The topic's keyword, trailing spaces trimmed. out must hold DialogTopicNameSize + 1 bytes. */
void dialogTopicName(const uint8_t *topic, char *out);

/*
 * The text a topic shows: its lines, concatenated without separators (each
 * line is already space padded to 33 characters; '%' line breaks left in
 * place), trailing spaces trimmed. Returns the full length needed (excluding
 * the NUL); writes at most capacity - 1 characters and a NUL. 0 lines gives
 * "". npc is the header the topic belongs to.
 */
size_t dialogTopicText(const DialogCatalog *catalog, const uint8_t *npc, const uint8_t *topic, char *out, size_t capacity);

/*
 * The topic the conversation opens on, as an absolute index (LoadItemData's
 * word_2E550 choice): the NPC's 2nd topic if flagA, else 3rd if flagB, else
 * 4th if flagC, else the 1st. The flags are the caller's evaluation of the
 * three DialogNpcGreetingFlag* global flags (an id of 0 counts as unset).
 */
unsigned dialogOpeningTopic(const uint8_t *npc, bool flagA, bool flagB, bool flagC);

#endif
