#ifndef YENDOR23_TITLEMENU_H
#define YENDOR23_TITLEMENU_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The title menu (RunTitleScreen, yendor2.asm:23524; Chapter 3 the same) and the boot title sequence (PlayTitleScreenSequence, :31040).
 *
 * The menu is PICTURES category 0 picture 2 drawn at (1, 1) with the five commands baked into it; the click regions (UiRegionsTitleMenu)
 * and keys give:
 *   1  C  the party roster / character management (ShowPartyMembers)
 *   2  A  the world map (ShowWorldMap)
 *   3  E  enter the game (the menu returns); refused with a beep (sound event 3) while no hero exists in the party slots
 *   4  R  the intro picture (ShowIntroPicture, picture 5 with a palette fade)
 *   5  I  character creation (RunCharacterCreation; the forced music track is 0 while it runs, 1 on the menu)
 * Return toggles the music and Ctrl+S the sound effects (the same toggles as the pause dialog, gamedialog.h); Ctrl+Q ends the game.
 * The menu temporarily sets the animation speed to 3 and plays forced music track 1.
 *
 * Chapter 3 draws the same menu (picture 2, same keys and regions) but also counts idle time: after 75 periodic ticks without input the menu
 * fades out, stops the music and plays the intro sequence (sub_20DC4, yendor3.asm:30192 -- Chapter 3's counterpart of the boot sequence below,
 * with music track 8) before showing character creation; Escape inside it returns to the menu.
 *
 * The Chapter 2 boot sequence shows PICTURES category 0 picture 0 at (1, 1) with a palette fade, plays music track 3 and waits 216 timer ticks
 * (about 12 seconds at 18.2 Hz) or until Escape; any key before the picture appears skips to the next stage, Escape leaves at once.
 */
typedef enum {
    TitleNone,
    TitleManageParty,
    TitleWorldMap,
    TitleEnterGame,
    TitleIntro,
    TitleCreateCharacter,
    TitleToggleMusic,
    TitleToggleSoundFx,
    TitleQuit
} TitleAction;

enum {
    TitleMenuPicture = 2,
    TitleBootPicture = 0,
    TitleBootMusicTrack = 3,
    TitleBootWaitTicks = 216,
    TitleMenuAnimationSpeed = 3,
    TitleMenuMusicTrack = 1
};

TitleAction titleKeyAction(uint8_t key);

/* The action for a UiRegionsTitleMenu result (1-5). */
TitleAction titleRegionAction(unsigned region);

/* Entering the game needs a hero: the sum of the four party slot assignments (active and three reserve slots) must not be zero. */
bool titleCanEnter(unsigned slotAssignmentSum);

#endif
