// Confirmed (Deponia_Linux.asm lines 1385285-1388803, src/vscommon/objAccess.cpp, from the assert in
// GetTypeGroup()): how the game's data objects are named and found. An object is known by its id
// ("(table,id)") or by a path of the editor's syntax: a table's name and the name of the object
// in square brackets, then fields separated by dots, a link field going to the object it holds and a link
// list field to the one object of the list that the name (or number) in square brackets says:
//
//     Scenes[Hallway].Objects[Door].Name
//
// A field may be given as `VName` (with the `V` the scripts' constants have, see luaGlobals.cpp), or
// with the name of the table put before it (`SceneName` for the field `Name` of a scene).
#pragma once

#include <cstdint>
#include <string>

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "vscommon/scripting/id.h"

class TTypeGroup;
class TVisionaireGame;
class TVisionaireObject;

/** Sets the game that the paths are looked up in (InitCommands() does it). */
void InitObjectAccess(TVisionaireGame *game);

/** The type group (the list of the fields) of a table of the game's data (-1 is the game itself), or
 *  null for a number that is no table. */
TTypeGroup *GetTypeGroup(int table);

/** "(table,id)". */
wxString IdStr(const TId &id);
wxString IdStr(const TVisionaireObject &object);
wxString IdStr(const TVisObjRef &object);
std::string IdStrStd(const TId &id);
/** The same for the 4 raw bytes of an id (TVisionaireObject::GetId()). */
std::string IdStrStd(const std::uint8_t *id);

/** Follows the rest of a path (what comes after the object that `object` is now: dot, field,
 *  maybe a bracketed name, and so on) and leaves the object it ends at in `object`. A message tells
 *  what was wrong with the path when this returns false (and what was not found only when `warn`). */
bool FindObjectByNameRelative(const wxString &path, TVisObjRef &object, bool warn);
/** A whole path, starting with the name of a table. */
bool FindObjectByName(const wxString &path, TVisObjRef &object, bool warn);
/** Either "(table,id)" or a path; the reference is cleared when the object is not found. */
bool FindObjectByNameOrId(const wxString &path, TVisObjRef &object, bool warn);

/** The id of a field from its Lua name (`VName`: the name without the V). */
int GetFieldId(const wxString &name);
