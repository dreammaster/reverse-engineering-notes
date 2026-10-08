#include "vscommon/scripting/argumentSyntax.h"

#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/scripting/argumentSyntax.cpp";

// Confirmed (asm lines 1413439-1413550)
bool TArgSyntax::AddArg(TArgType type, bool mandatory) {
	x_assert(type != TArgType::kNone, "arg != A_NONE", kSourceFile, 0x83);

	_args.push_back(type);

	if (mandatory) {
		if (_minArgs + 1 >= static_cast<int>(_args.size())) {
			_minArgs++;
		} else if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"Can't add mandatory argument after optional argument (argument added as optional).");
		}
	}

	return true;
}

// Confirmed (asm lines 1413597-1414148)
bool TArgSyntax::AddFlag(const wxString &shortName, const wxString &longName, TArgType type, bool mandatory) {
	x_assert(type != TArgType::kNone, "argType != A_NONE", kSourceFile, 0x60);

	if (shortName.IsEmpty() || longName.IsEmpty()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"You must specify both short and long name for the flag.");
		return false;
	}

	// (the message of the original is in German: the short name has to be at most 3 letters, the long name at least 4)
	if (shortName.Length() > 3 || longName.Length() <= 3) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Der Kurzname darf max. 3 Zeichen, der Langname muss mind. 4 Zeichen lang sein.");
		return false;
	}

	if (_byShortName.count(shortName.ToStdWstring()) != 0) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"The flag \"%s\" already exists.", shortName.wc_str());
		return false;
	}

	TArgFlagSyntax flag;

	flag.index = static_cast<int>(_flags.size());
	flag.shortName = shortName;
	flag.longName = longName;
	flag.mandatory = mandatory;
	flag.type = type;
	_flags.push_back(flag);
	_byShortName[shortName.ToStdWstring()] = _flags.size() - 1;
	_byLongName[longName.ToStdWstring()] = _flags.size() - 1;
	return true;
}

// Confirmed (asm lines 1411674-1411781)
bool TArgSyntax::GetLongFlagName(wxString &name) const {
	auto found = _byShortName.find(name.ToStdWstring());

	if (found == _byShortName.end())
		return false;

	name = _flags[found->second].longName;
	return true;
}

// Confirmed (asm lines 1411781-1411887)
bool TArgSyntax::GetShortFlagName(wxString &name) const {
	auto found = _byLongName.find(name.ToStdWstring());

	if (found == _byLongName.end())
		return false;

	name = _flags[found->second].shortName;
	return true;
}
