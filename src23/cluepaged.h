#ifndef YENDOR23_CLUEPAGED_H
#define YENDOR23_CLUEPAGED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"
#include "viewrender.h"

/*
 * The clue book's COMPLETE WALK THROUGH (ShowPagedEntryScreen yendor2.asm:5368, UpdateScrollArrows :5877, HandlePagedEntryNavigation :5475;
 * Chapter 3 the same): a built-in hint book of 31 (Chapter 3: 33) pages. Each page is 25 lines of 51 bytes (50 characters + NUL) in WORLD.DAT's
 * second clue block (Chapter 2 at 1411160, Chapter 3 at 3969186; page n is at (n - 1) * 1275). The screen is the usual clue backdrop (category 0
 * picture 13 / Chapter 3 picture 6) with the title of the game ("DARK UNION" / "RESTORATION") at (6, 4) in 0xD, the heading COMPLETE WALK THROUGH
 * at the top right, the page's 25 lines from (10, 23) in 0xD, 6 pixels apart, the footer "a  MORE  b" at (129, 167) in 0xD -- its first character
 * (up arrow) is a space on the first page and its tenth (down arrow) on the last -- and the navigation bar. Navigation flags: 0x100 = a previous
 * page exists, 0x80 = a next one does.
 *
 * Keys I / Q (or the UiRegionsClueScroll arrows) turn the page. The shareware limit: unless the program is registered (flag bit 0 of
 * g_uiScratchFlags4) the page after page 5 is refused and the registration reminder is shown instead.
 */
enum { CluePagedLines = 25, CluePageLineSize = 51, CluePageSize = CluePagedLines * CluePageLineSize, CluePagedFreePages = 5 };

typedef struct {
    char title[24], heading[24], footer[16];
} CluePagedText;

bool cluePagedTextLoad(CluePagedText *text, const ExeData *exe, GameKind game);

unsigned cluePageCount(GameKind game);

/* Page `page` (1-based) of the hint book: CluePageSize bytes inside worldDat, or NULL. */
const uint8_t *cluePagedPage(GameKind game, const uint8_t *worldDat, size_t size, unsigned page);

uint16_t cluePagedNavFlags(GameKind game, unsigned page);

typedef enum { CluePagedNone, CluePagedPrevious, CluePagedNext, CluePagedNag } CluePagedResult;

/* `key` is 'I' or 'Q' (0 for a click, then `region` is 1 or 2); updates *page. */
CluePagedResult cluePagedNavigate(GameKind game, unsigned *page, uint8_t key, unsigned region, bool registered);

void cluePagedDraw(const ViewRenderer *r, const CluePagedText *text, const uint8_t *pageData, unsigned page, uint16_t navFlags);

/*
 * The help screen (Tab in the clue book; ShowClueBookHelpScreen yendor2.asm:7696, Chapter 3 sub_18691) and the shareware reminder
 * (ShowClueBookRegistrationNag :7595). Help: the usual backdrop with the book title ("DARK UNION : THE ON-LINE CLUE BOOK") at (6, 4), the heading
 * HELP SCREEN at the top right (Chapter 2 x = 249), "** PRESS TAB AT ANY TIME TO SEE THIS SCREEN **" at (21, 24) in 0x59, then 13 lines (the F1-F6
 * and key descriptions, some blank) from (16, 60), 6 pixels apart, in 0x08 (Chapter 3: 0x0A); the navigation bar is drawn with the hint bits
 * (0x60) cleared. The reminder plays sound 3 and writes "REGISTER YOUR COPY OF THE CLUE BOOK TODAY!" at (35, 16) in 0x59.
 */
enum { ClueHelpLines = 13 };

typedef struct {
    char title[40], heading[16], banner[48], lines[ClueHelpLines][48], nag[48];
} ClueHelpText;

bool clueHelpTextLoad(ClueHelpText *text, const ExeData *exe, GameKind game);
void clueHelpDraw(const ViewRenderer *r, const ClueHelpText *text, uint16_t navFlags);
void clueNagDraw(const ViewRenderer *r, const ClueHelpText *text);

#endif
