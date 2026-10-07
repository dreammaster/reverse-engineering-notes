// Confirmed (Deponia_Linux.asm lines 556367-558635, all 22 functions): how the values of the game's
// data go to Lua and come back. A point is a table {x, y}, a rect {x, y, width, height}, a sprite
// {path, position = {x, y}, transparency, transpcolor, pause}, a text {text, sound, language}; a list is
// a table with the numbers 1, 2 ... as keys. All of them work on the Lua state of the game (`L`,
// visLua.h).
//
// ConvertToLua() pushes the value. ConvertFromLua() reads the value from the table at the stack
// position `index`, and gives whether it is complete (and so was read); it leaves the stack as it
// was. Note that the reading pushes the key it looks up before it asks the table at `index`: a
// relative index (-1 for the top) must be given as -2, the one that the table has then
// (the original is that way, and the callers know).
#pragma once

#include <vector>

#include "TCharHolder.h"
#include "TSprite.h"
#include "TTextLanguage.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

/** Pushes a data object (a userdata of the scripts; see visLuaObjects.h); an empty one is nil. */
void ConvertToLua(const TVisObjRef &object);
void ConvertToLua(const wxPoint &point);
void ConvertToLua(const wxRect &rect);
void ConvertToLua(const TSprite &sprite);
void ConvertToLua(const TTextLanguage &text);
void ConvertToLua(const std::vector<int> &values);
void ConvertToLua(const std::vector<float> &values);
void ConvertToLua(const std::vector<wxPoint> &values);
void ConvertToLua(const std::vector<TTextLanguage> &values);
void ConvertToLua(const std::vector<wxRect> &values);
void ConvertToLua(const std::vector<TCharHolder> &values);
void ConvertToLua(const std::vector<TSprite> &values);

bool ConvertFromLua(wxPoint &point, int index);
bool ConvertFromLua(wxRect &rect, int index);
bool ConvertFromLua(TSprite &sprite, int index);
bool ConvertFromLua(TTextLanguage &text, int index);
bool ConvertFromLua(std::vector<wxPoint> &values, int index);
bool ConvertFromLua(std::vector<wxRect> &values, int index);
bool ConvertFromLua(std::vector<TSprite> &values, int index);
bool ConvertFromLua(std::vector<int> &values, int index);
bool ConvertFromLua(std::vector<float> &values, int index);
bool ConvertFromLua(std::vector<TCharHolder> &values, int index);
bool ConvertFromLua(std::vector<TTextLanguage> &values, int index);
