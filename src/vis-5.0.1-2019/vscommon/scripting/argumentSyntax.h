// Confirmed (Deponia_Linux.asm lines 1410539-1415400, src/vscommon/scripting/argumentSyntax.cpp by the
// asserts): what a command of the scripts takes: the types of its arguments (positional, the last ones may
// be optional) and its flags (named arguments, given in a table with the key `flags`:
// `getTime({flags = 1, reset = true})` ... a flag has a short name of up to 3 letters and a long name of 4 or more,
// either can be the key in the table, a type, and may be mandatory).
//
// The original also keeps the texts that the script editor shows for each command (a description of the command, of each
// argument and result, examples) and dumps them as Lua syntax or wiki pages (AddDoc, AddArgDoc, AddFlagDoc,
// AddReturnDoc, AddBoolRetDoc, AddExampleDoc, Dump, DumpLuaSyntax, DumpWiki). They are help for the editor and
// not reconstructed; what a command does is in the comments of the code of the command.
#pragma once

#include <unordered_map>
#include <vector>

#include "WxStub.h"
#include "vscommon/scripting/argument.h"

/** A flag of a command. */
struct TArgFlagSyntax {
	/** The number of the flag in the order they were added. */
	int index = 0;
	wxString shortName;
	wxString longName;
	bool mandatory = false;
	TArgType type = TArgType::kNone;
};

class TArgSyntax {
public:
	/** Adds an argument. A mandatory argument can only come before the optional ones (else it is added as an
	 *  optional one, and logged). */
	bool AddArg(TArgType type, bool mandatory);
	/** Adds a flag: the names must be of 1 to 3 and of 4 or more letters, the short name must be new. */
	bool AddFlag(const wxString &shortName, const wxString &longName, TArgType type, bool mandatory);

	/** Gives the long name of the flag whose short name is `name` (and puts it in `name`), or false. */
	bool GetLongFlagName(wxString &name) const;
	/** Gives the short name of the flag whose long name is `name`. */
	bool GetShortFlagName(wxString &name) const;

	/** The number of the mandatory arguments (the first ones). */
	int GetMinArgs() const {
		return _minArgs;
	}
	const std::vector<TArgType> &GetArgTypes() const {
		return _args;
	}
	const std::vector<TArgFlagSyntax> &GetFlags() const {
		return _flags;
	}
	/** The oldest version of the editor that has the command (kept as it is given; nothing uses it). */
	void SetMinVersion(const wxString &version) {
		_minVersion = version;
	}

private:
	std::vector<TArgType> _args;
	int _minArgs = 0;
	std::vector<TArgFlagSyntax> _flags;
	std::unordered_map<std::wstring, size_t> _byShortName;
	std::unordered_map<std::wstring, size_t> _byLongName;
	wxString _minVersion;
};
