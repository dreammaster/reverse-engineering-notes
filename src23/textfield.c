#include "textfield.h"

void textFieldInit(TextField *field, unsigned max) {
    field->text[0] = 0;
    field->length = 0;
    field->max = max < TextFieldMaxSize ? max : TextFieldMaxSize - 1;
}

TextFieldResult textFieldKey(TextField *field, uint8_t key) {
    if (key == 0x0D) {
        return TextFieldConfirmed;
    }
    if (key == 0x1B) {
        return TextFieldCancelled;
    }
    if (key == 0x08) {
        if (field->length == 0) {
            return TextFieldBeep;
        }
        field->text[--field->length] = 0;
        return TextFieldEditing;
    }
    if (key >= 0x20 && key <= 0x7F) {
        if (field->length + 1 >= field->max) {
            return TextFieldBeep;
        }
        field->text[field->length++] = (char)key;
        field->text[field->length] = 0;
    }
    return TextFieldEditing;
}
