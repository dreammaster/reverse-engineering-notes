#pragma once

#include <vector>

#include "TCharHolder.h"
#include "WxStub.h"

class TComposedFile;

// Not yet reverse-engineered; only the methods TStandardPaths/TGameControl
// call are stubbed here (see manifest/proprietary_classes.tsv for its full
// method list when this class gets its own pass).
class TComposedFileManager {
public:
	static bool IsGameCompiled();

	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// lines 464020-464030) - not reversed beyond that.
	static bool InitMainContainer(const wxFileName &file, const wxString &password);
	static TComposedFile *GetMainContainer();

	// Confirmed call shape only (asm lines 464281-464283) - initializes the
	// main container from a fixed file plus a list of extra strings (e.g.
	// per-volume names); not reversed beyond that.
	static void Init(const wxFileName &file, const wxString &password, const std::vector<TCharHolder> &strings);
	// A second overload for up to 6 extra (path, size-or-ordinal) pairs
	// instead of the string list above - confirmed call shape only (asm
	// lines 464411-464478), field ids for each pair (0x26D/0x26E,
	// 0x29E/0x29F, 0x26B/0x26C, 0x269/0x26A, 0x2AA/0x2AB) not resolved
	// beyond their raw values.
	static void Init(const wxFileName &file, const wxString &password, int size1, const wxFileName &path1, int size2,
	                 const wxFileName &path2, int size3, const wxFileName &path3, int size4, const wxFileName &path4,
	                 int size5, const wxFileName &path5);
	// Confirmed static call shape only (TGameControl::LoadGame(TMSavegame*),
	// Deponia_Linux.asm lines 477585, 477591) - registers the container file
	// a savegame's own data will be read from/written to; not reversed
	// beyond that call shape.
	static bool SetSavegameFile(const wxFileName &file, const wxString &name);
};
