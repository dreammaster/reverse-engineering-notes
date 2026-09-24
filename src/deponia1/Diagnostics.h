#pragma once

// x_assert(condition, expressionText, fileText, line) - used throughout the
// engine as its assert macro; call sites embed __FILE__/__LINE__/the
// stringified expression (which is how we recovered several original source
// paths, e.g. "vsplayer/control/gameController.cpp" - see NOTES.md).
void x_assert(bool condition, const char* expression, const char* file, int line);
