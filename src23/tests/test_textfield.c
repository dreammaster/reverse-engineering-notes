/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_textfield test_textfield.c ../textfield.c && ./test_textfield
 */
#include <stdio.h>
#include <string.h>

#include "textfield.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

int main(void) {
    TextField f;
    textFieldInit(&f, 5);
    check("typing appends", textFieldKey(&f, 'A') == TextFieldEditing && textFieldKey(&f, 'b') == TextFieldEditing && strcmp(f.text, "Ab") == 0);
    check("backspace deletes", textFieldKey(&f, 8) == TextFieldEditing && strcmp(f.text, "A") == 0);
    check("backspace on nothing beeps", textFieldKey(&f, 8) == TextFieldEditing && textFieldKey(&f, 8) == TextFieldBeep && f.length == 0);
    textFieldKey(&f, '1');
    textFieldKey(&f, '2');
    textFieldKey(&f, '3');
    textFieldKey(&f, '4');
    check("a field of size 5 holds four characters", f.length == 4 && strcmp(f.text, "1234") == 0);
    check("the fifth beeps and is lost", textFieldKey(&f, '5') == TextFieldBeep && strcmp(f.text, "1234") == 0);
    check("control and high keys are ignored", textFieldKey(&f, 0x01) == TextFieldEditing && textFieldKey(&f, 0x90) == TextFieldEditing && f.length == 4);
    check("DEL (0x7F) is text like any other printable key", textFieldKey(&f, 8) == TextFieldEditing && textFieldKey(&f, 0x7F) == TextFieldEditing && f.text[3] == 0x7F);
    check("enter confirms, escape cancels", textFieldKey(&f, 0x0D) == TextFieldConfirmed && textFieldKey(&f, 0x1B) == TextFieldCancelled);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
