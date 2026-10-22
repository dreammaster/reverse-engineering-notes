#ifndef YENDOR23_PARTYINFO_H
#define YENDOR23_PARTYINFO_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * What a click on a member's icons in his or her panel at the bottom of the main screen writes into the text area (HandlePartyStatusPanelInput, yendor2.asm:38611; Chapter 3 the same code):
 * the panel region picks the list --
 *   2-4 (the three affliction icons)   AFFLICTIONS (DrawAfflictionsList, :38887)
 *   5 (the training mark)              TRAINING: CURRENTLY LEVEL n, and READY FOR LEVEL m when a level is pending (ShowLevelUpMessage, :38772)
 *   6 (the protection icon)            PROTECTIONS: the nine resistances (DrawCharacterProtectionsList, :39103)
 *   7 (the abilities icon)             TRANSPORTS: the mounts the member has learned, bright when ready (DrawAbilityReadinessList, :38820)
 *   8 (the name bar) and the key 1-4   HEALTH / MAGIC / WEIGHT: the three bars' numbers (DrawThreeStatBars, :39037)
 * Every list starts with DrawCharacterNameHeader: the member's name over a blank 12 characters at (241, 87), and writes its rows from (241, 96) in the colours below. The texts are the
 * same in both chapters.
 *
 * Each function fills `out` with up to `max` runs (position, colour, opaque or transparent over the panel, text) and returns how many.
 */
typedef struct {
    int x, y;
    uint8_t colour;
    bool opaque;
    char text[16];
} PanelRun;

enum { PartyInfoMaxRuns = 32 };

/* The list a panel region (2-8) shows: 0 afflictions, 1 training, 2 protections, 3 transports, 4 the stat bars; -1 for any other region. */
int partyInfoKindForRegion(unsigned part);

unsigned partyInfoAfflictions(const uint8_t *record, PanelRun *out, unsigned max);
unsigned partyInfoTraining(const uint8_t *record, PanelRun *out, unsigned max);
unsigned partyInfoProtections(const uint8_t *record, PanelRun *out, unsigned max);
unsigned partyInfoTransports(const uint8_t *record, unsigned clockMinutes, PanelRun *out, unsigned max);
unsigned partyInfoStatBars(const uint8_t *record, PanelRun *out, unsigned max);

/* FormatNumber's text: up to five digits without leading zeros, a comma after the thousands. */
void partyInfoFormatNumber(unsigned value, char out[8]);

#endif
