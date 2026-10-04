#include "roster.h"

#include "font.h"
#include "party.h"
#include "uiregions.h"

static unsigned field(const uint8_t *record, unsigned offset) {
    return (unsigned)record[offset] | ((unsigned)record[offset + 1] << 8);
}

void rosterDraw(const ViewRenderer *r, const uint8_t *const records[RosterSlots]) {
    unsigned count;
    const uint16_t(*grid)[5] = uiRegionEntries(r->game, UiRegionsPartyRoster, &count);
    viewDrawPicture(r, 0, 4, 1, 1, false, 0);
    for (unsigned slot = 0; slot < RosterSlots && count >= (slot + 1) * 4; slot++) {
        const uint8_t *record = records[slot];
        if (!record || field(record, PartyFieldLevel) == 0) {
            continue;
        }
        const uint16_t(*e)[5] = grid + slot * 4;
        viewDrawPicture(r, 7, field(record, PartyFieldPanelFace), e[0][0], e[0][2], false, 0);
        char name[PartyNameBufferSize];
        for (unsigned i = 0; i < PartyNameMaxLength; i++) {
            name[i] = (char)record[i];
        }
        name[PartyNameMaxLength] = 0;
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, e[1][0], e[1][2], name, 0xF, 0, FontTransparent);
        const char *className = partyClassName(field(record, PartyFieldClass));
        if (className) {
            fontDrawString(r->game, 0, r->screen, ViewScreenWidth, e[2][0], e[2][2], className, 0xF, 0, FontTransparent);
        }
        viewDrawPicture(r, 9, (field(record, 0x15C) & 0x800) ? 0x12 : 0x11, e[3][0], e[3][2], true, 0);
    }
}
