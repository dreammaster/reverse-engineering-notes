// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 516915-519803, all 17
// manifest-listed methods reached, see each method's own comment below for
// its exact asm range). A pure static manager coordinating several global
// TComposedFile instances: one "main" container, one "savegame" container,
// one "game" container, three resizable arrays (scene/character/interface
// containers, each either a single shared instance or one-per-numbered-
// volume), a set of "manual" containers built straight from a caller-
// supplied path list, and a plain movie-container path string (not a
// TComposedFile at all).
#pragma once

#include <vector>

#include "TCharHolder.h"
#include "WxStub.h"
#include "baselib/composedfile.h"

class TFile;
class TMemoryFile;

// Confirmed fields (TComposedFileManager::GetComposedFile/GetComposedFileInfo/GetMemoryFile, Deponia_Linux.asm lines
// 517790-518700): `entryIndex` (long, +0x00) is the number of the file in its container, `volume` (long, +0x08) which
// container of its kind (-1: the only one; the scene, character and interface containers are numbered when there are
// several, a manual or a movie by its number), `containerType` (TContainerTypeEnum, +0x10) the kind.
struct TComposedFileInfo {
	long entryIndex = 0;
	long volume = 0;
	TContainerTypeEnum containerType = TContainerTypeEnum::kType0;
};

class TComposedFileManager {
public:
	static bool IsGameCompiled();

	// Confirmed (Deponia_Linux.asm lines 516915-517064): rebuilds
	// s_manualContainers from `strings`, each initialized with offset -2 -
	// `file` itself is read but never used (a dead parameter in the
	// original).
	static void Init(const wxFileName &file, const wxString &password, const std::vector<TCharHolder> &strings);
	// Confirmed (asm lines 517072-517446): initializes, in order, the game
	// container (size1/path1, offset -1), the scene containers (size2/
	// path2 - offset -1 if there's exactly one, else one Init() per index),
	// the character containers (size3/path3, same shape), the interface
	// containers (size4/path4, same shape), and finally - NOT a
	// TComposedFile at all - just assigns s_movieComposedFile = path5 when
	// size5 > 0.
	static void Init(const wxFileName &file, const wxString &password, int size1, const wxFileName &path1, int size2,
	                 const wxFileName &path2, int size3, const wxFileName &path3, int size4, const wxFileName &path4,
	                 int size5, const wxFileName &path5);
	// Confirmed (asm lines 517454-517551): frees the scene/character/
	// interface container arrays only - NOT s_manualContainers,
	// s_gameContainer, s_savegameContainer, or s_movieComposedFile.
	static void CleanUp();

	static TComposedFile *GetMainContainer();
	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// lines 464020-464030) - not reversed beyond that.
	static bool InitMainContainer(const wxFileName &file, const wxString &password);
	// Confirmed (asm lines 517577-517589): a tail call to
	// TComposedFile::Init(&s_savegameContainer, file, name, -1).
	static bool SetSavegameFile(const wxFileName &file, const wxString &name);

	// Confirmed in full (asm lines 517597-517710): opens `file` directly
	// (simplified from the real TFile::OpenReadFromComposedFile(), which
	// this call site always invokes with offset 0 - equivalent to a plain
	// open here) and checks the first 4 bytes for the "VIS3" magic.
	static bool IsComposedFile(const wxFileName &file);

	// Confirmed in full (asm lines 517944-518473): the reference to a container file in the extension of `file`, see the .cpp.
	static bool GetComposedFileInfo(const wxFileName &file, TComposedFileInfo &outInfo);
	// Confirmed in full (asm lines 517790-517936): dispatches on
	// info.containerType to the matching global container(s). For the
	// array-backed types (kType1=scene, kType2=character, kType5=
	// interface), info.index selects within the array - or, if index == -1,
	// returns the array's own base address when there's exactly one
	// container in it (matching how a single, non-multi-volume container
	// doesn't need a numbered sub-index). kType0/kType4/kEmbeddedInExecutable
	// map to the main/savegame/game containers directly (info.index
	// ignored). kEmbeddedInExecutable/kEmbeddedInExecutableAlt (manual
	// containers) are 1-indexed - info.index==1 is the first one - unlike
	// every other case. Any other containerType (including kType3, the
	// movie type - handled separately by DecryptComposedFile()/
	// GetComposedMovieFileName() instead, never through this method)
	// returns null.
	static TComposedFile *GetComposedFile(const TComposedFileInfo &info);

	// Confirmed in full (asm lines 518474-518972): resolves `file` through
	// GetComposedFileInfo()/GetComposedFile() first, falling back to a
	// plain wxFile read (open, get length, TMemoryFile::Reserve(), read)
	// when GetComposedFileInfo() reports `file` isn't a composed-file
	// reference.
	static bool GetMemoryFile(TMemoryFile &outFile, const wxFileName &file);
	// Confirmed in full (asm lines 518972-519254): same resolve-then-
	// fall-back shape as GetMemoryFile() above, but opening via
	// TFile::OpenRead() in the fallback case.
	static bool Open(TFile &outFile, const wxFileName &file);
	// Confirmed call shape only (asm lines 519262-519378): resolves `file`
	// the same way as GetMemoryFile()/Open() above, then delegates to
	// TComposedFile::Export() - Visionaire Studio's own authoring-tool
	// export pipeline (TComposedFile::Export() is itself a signature-only
	// stub, see composedfile.h) - not needed to play a shipped game.
	static bool Export(const wxFileName &file, const wxFileName &outPath, wxFileName &outName);

	// Confirmed in full (asm lines 519379-519499): only acts on a kType3
	// ("movie") reference with a non-negative index - builds the
	// "<moviePath>.v<index>" container file name and decrypts its header
	// via the real TFile::DecryptHeader() cipher.
	static bool DecryptComposedFile(const wxFileName &file, wxFileName &outFile, const wxString &password);
	// Confirmed (asm lines 519507-519559): a thin wrapper around
	// DecryptComposedFile() - the cipher is its own inverse.
	static bool EncryptComposedFile(const wxFileName &file, const wxString &password);

	// Confirmed in full (asm lines 519571-519714): for a kEmbeddedInExecutableAlt
	// ("manual") reference, returns the matching manual container's own
	// composed-file path; for kType3 ("movie"), the same
	// "<moviePath>.v<index>" construction DecryptComposedFile() uses; false
	// for anything else (including when GetComposedFileInfo() fails).
	static bool GetComposedMovieFileName(const wxFileName &file, wxFileName &outName);

	// Confirmed in full (asm lines 519722-519803): when not running as a
	// compiled/packaged game, a plain wxFileName::Exists(); when compiled,
	// a heuristic that just checks for a literal '#' in the path (the
	// marker a composed-file-reference suffix always contains) rather than
	// actually verifying - reproduced as observed, not "fixed".
	static bool FileExists(const wxFileName &file);

private:
	static bool s_bCompiledGame;
	static std::vector<TComposedFile> s_manualContainers;
	static TComposedFile s_mainContainer;
	static TComposedFile s_savegameContainer;
	static TComposedFile s_gameContainer;
	static std::vector<TComposedFile> s_sceneContainers;
	static std::vector<TComposedFile> s_characterContainers;
	static std::vector<TComposedFile> s_interfaceContainers;
	static std::wstring s_movieComposedFile;
};
