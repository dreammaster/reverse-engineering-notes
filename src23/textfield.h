#ifndef YENDOR23_TEXTFIELD_H
#define YENDOR23_TEXTFIELD_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The game's single-line text editor (EditTextField, yendor2.asm:23401; identical in Chapter 3), used for the hero's name and the numeric
 * prompts. `max` is the buffer size including the terminating NUL (the name prompt passes 13), so at most max - 1 characters are kept.
 *
 * Keys (the original's key codes): 0x0D confirms, 0x1B cancels, 0x08 deletes the last character (a beep with nothing to delete), 0x20-0x7F
 * is appended; typing when only the terminator's slot is left beeps and the character is lost (the original stores it in the last slot,
 * which the terminator then overwrites); everything else is ignored. A '-' cursor is drawn after the text and is overwritten by a space on
 * exit; the pen starts where the text begins and advances 6 per character.
 */
enum { TextFieldMaxSize = 32 };

typedef struct {
    char text[TextFieldMaxSize];
    unsigned length;
    unsigned max;
} TextField;

typedef enum { TextFieldEditing, TextFieldBeep, TextFieldConfirmed, TextFieldCancelled } TextFieldResult;

void textFieldInit(TextField *field, unsigned max);
TextFieldResult textFieldKey(TextField *field, uint8_t key);

#endif
