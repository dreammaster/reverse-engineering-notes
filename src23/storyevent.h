#ifndef YENDOR23_STORYEVENT_H
#define YENDOR23_STORYEVENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * What the main loop does when the party uses a world object with flag 0x400 (worldobjects.h, WorldObjectFlagStoryEvent): `start` (yendor2.asm:748-870) tests the
 * flags in the order door 0x8000, destination 0x2000, talker 0x1000, then 0x400.
 *
 * Chapter 2 has exactly one such object (WORLD.DAT x = 340, y = 98, value 1): the Quartz Chamber's parchment. Using it plays the ending cinematic
 * (introstory.h, introEndingScript) if the three global flags 0x69, 0x6A and 0x6B are all set and otherwise shows the status message "YOU ARE NOT ..." (DS:0x86AD,
 * two lines); afterwards the main loop shows the intro picture and goes back to the title. storyEndingReady is that test.
 *
 * Chapter 3 has six (values 1-6; x, y = (40, 62) (69, 50) (352, 68) (487, 32) (603, 44) (635, 136)) and handles them in HandleScriptedStoryEventTrigger
 * (yendor3.asm:21124): each is a scripted teleport of the whole party -- to Thaine, into the artifact realm and back -- guarded by a quest flag and sometimes
 * converting or removing quest items. storyEvent describes one; session.c (sessionInteract) carries it out:
 *
 *   1  once (flag 5 = done): sound 0x21, travel mode 2, to (397, 94) facing west, day / night tracks cleared; the text "YOU HAVE ARRIVED IN THAINE."
 *   2  needs flag 0x1B (silently nothing without): clear the travel-flag bits 0xE003, sound 0x54, mode 4, to (540, 121) facing south, travel flags |= 0x2000
 *   3  needs flag 0x2E (without: the text "LOCKED"): mode 5, to (638, 36) facing west, travel flags |= 0x2000
 *   4  mode 5, to (619, 37) facing north, travel flags |= 0x2000; every item 0xC6 the party carries becomes 0xCD (the party's six global slots first, then
 *      each member's inventory, one item at a time until none is left); sound 0x2C; if any changed the text "THE RINGS HAVE BEEN DRAINED."
 *   5  needs flag 0x91 (without: "LOCKED"): mode 8, to (604, 123) facing east, travel flags |= 0x4000, sets flag 0x9D (the artifact realm), sound 0x2C
 *   6  mode 2, to (332, 70) facing south, travel flags &= ~0x4000, clears flag 0x9D; if the party holds item 0x183 sets flag 0x9E; every item 0x17F-0x181 (the
 *      SWORD / HAMMER / TRIDENT OF LIGHT) is removed (its stat bonuses taken off, one charge consumed) and counted; sound 0x2C; if any went the text
 *      "ALL OF YOUR NEW WEAPONS ARE GONE"
 *
 * The travel-flag word is the lighting module's travel flags (lighting.h: 0x8000 / 0x2000 pick the indoor gradients, 0x4000 the noon sky).
 */
enum {
    StoryEndingFlagA = 0x69,
    StoryEndingFlagB = 0x6A,
    StoryEndingFlagC = 0x6B,
    StoryEventCountYendor3 = 6
};

/* Chapter 2: whether the three quest flags are set (globalFlags: globalflags.h's buffer). */
bool storyEndingReady(const uint8_t *globalFlags, size_t flagsSize);

typedef struct {
    int requireFlag;           /* 0 = none; the event does nothing at all while it is clear, unless failTextOffset names a message */
    int doneFlag;              /* 0 = repeatable; else the event does nothing once it is set, and sets it */
    unsigned failTextOffset;   /* DS offset of the one line shown when requireFlag is clear (0 = silent) */
    unsigned sound;
    unsigned travelMode;       /* ds:0CF33 */
    int x, y;
    uint16_t facing;           /* SaveFacing */
    uint16_t travelFlagsAnd;   /* applied first (0xFFFF = unchanged) */
    uint16_t travelFlagsOr;
    int setFlag, clearFlag;    /* 0 = none */
    bool clearTracks;          /* the destination's day and night tracks are cleared (event 1) */
    uint16_t swapFrom, swapTo; /* every item swapFrom becomes swapTo (0 = none) */
    uint16_t removeLow, removeHigh; /* every item in this id range is removed (0 = none) */
    uint16_t checkItem;        /* before removing: if the party holds this item, set checkItemFlag */
    int checkItemFlag;
    unsigned doneTextOffset;   /* DS offset of the three lines shown when something was swapped / removed (or always, for event 1: doneTextAlways) */
    bool doneTextAlways;
} StoryEvent;

/* Chapter 3's event for an object value 1-6; NULL for anything else and for Chapter 2. */
const StoryEvent *storyEvent(GameKind game, unsigned value);

#endif
