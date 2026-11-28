// Confirmed (Deponia_Linux.asm lines 1432257-1435370, src/vscommon/scripting/argumentParser.cpp by the
// asserts): reads the arguments of a call of a command from the Lua stack, as its syntax (TArgSyntax) says.
// The positional arguments are on the stack from 1 up; a table with the key `flags` that comes after the
// mandatory ones (and is the last) holds the flags by their long or short names. Each is converted by
// ConvertArgumentFromLua() to the type the syntax says (kAny takes the type of the Lua value).
#pragma once

#include <unordered_map>
#include <vector>

#include "vscommon/scripting/argument.h"
#include "vscommon/scripting/argumentSyntax.h"

// The type of the Lua value at `index` (the stack of the game's Lua state) for ConvertArgumentFromLua() with kAny:
// a table with `path` is a sprite, with x, y, width and height a rect, with x and y a point, with `text` a
// text; a table whose first element is a number is a list of floats, a string a list of strings, a table a list of
// sprites, rects, points or texts (by that element), a userdata a list of objects; kAny when it cannot be told.
bool IsPointArgument(int index);
bool IsRectArgument(int index);
bool IsSpriteArgument(int index);
bool IsTextArgument(int index);
TArgType GetArgumentType(int index);
/** Whether the table at `index` has the key `flags`. */
bool IsArgumentFlags(int index);
/** Reads the Lua value at `index` as `type`; false when it is not one (or cannot be converted). */
bool ConvertArgumentFromLua(TArgument &argument, TArgType type, int index);

/** The names of the types for the messages of the parser. (The original has none for text and the list of texts, so
 *  the names after `rect` are of the type before: kept.) */
const wchar_t *ArgTypeName(TArgType type);

class TArgParser {
public:
	~TArgParser();

	/** Drops the arguments. */
	void Clear();
	/** Reads the call on the Lua stack; false (with a message) when it does not fit the syntax. */
	bool ParseArguments(const TArgSyntax &syntax);

	/** The argument of a flag, from its short or long name (a name of up to 3 letters is a short one); null if not
	 *  given. */
	const TArgument *FindFlagArgument(wxString name) const;
	/** Whether the flag `e` (edit) is given and true. */
	bool IsEdit() const;
	/** The long name of the flag at `position` (in the order they were given). */
	bool GetFlagName(size_t position, wxString &name) const;
	bool GetFlagArgument(const wxString &name, const TArgument **argument) const;
	bool GetArgument(size_t position, const TArgument **argument) const;
	/** The number of positional arguments given. */
	size_t GetArgumentCount() const {
		return _arguments.size();
	}

private:
	const TArgSyntax *_syntax = nullptr;
	std::unordered_map<std::wstring, TArgument *> _flags;
	std::vector<std::wstring> _flagOrder;
	std::vector<TArgument *> _arguments;
};
