// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 159356-164367, all 22 manifest-
// listed methods) - one savegame slot (or "bookmark" slot): a TManagedObject
// subclass (recovered RTTI; 0x1E8 bytes in all, per the `operator new` size
// at TGScene::GetSelectedSavegame()'s call site) that shows its slot's
// screenshot and title in the "load game" menu scene and can write itself to
// disk. TGScene (TGScene.h) owns these, one per slot found on disk, and
// activates the ones currently scrolled into view.
//
// Real layout (offsets from the object base; TManagedObject occupies
// +0x00-0xB7): the slot's screenshot as an embedded TPictureIO (+0xB8), a
// "screenshot file couldn't be opened, stop retrying" flag (+0x1A0), the
// slot's display title (+0x1A8) and the name of its composed file
// (+0x1B0), the text's screen position (+0x1B8/0x1BC), a font id (+0x1C0) and
// the x offset that centres the title in the slot (+0x1C4) - the pair
// constructed together via TId(-1, -1), then written separately - the slot
// number (+0x1C8), the slot rectangle's size (+0x1CC/0x1D0), the game data it
// reads titles from (+0x1D8) and the bookmark flag (+0x1E0).
//
// Bookmarks (isBookmark) share the file naming scheme but save no screenshot
// and, once constructed, never become active.
//
// SaveGame() writes through TComposedFile::WriteToDisk() (a real, tested
// implementation); the screenshot it saves comes from the unmodeled GL
// backend's captured frame, so with the current stub backend a real
// savegame's snapshot step fails (bookmarks, which save no screenshot, work).
#pragma once

#include <vector>

#include "TManagedObject.h"
#include "baselib/xmlWriter.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/picture.h"

class TVisionaireGame;

class TMSavegame : public TManagedObject {
public:
	TMSavegame(bool isBookmark, int slot, int width, int height, TVisionaireGame *game);
	~TMSavegame() override;

	// Confirmed no-ops (asm lines 159356-159388): a savegame slot has no
	// scripted actions or events.
	void GetActionList(TVList &/*outActions*/) const override {
	}
	void ExecuteEvent(TGEventInfo &/*info*/) override {
	}

	// Confirmed overrides of the base's virtual SetActive()/Draw(): activating
	// a slot loads its title (and checks its screenshot is reachable);
	// deactivating releases the screenshot.
	void SetActive(bool active) override;
	void Draw() override;

	// Places this slot's screenshot and title: the screenshot at the rect's
	// top-left, the title at the rect's left edge (plus the centring offset)
	// and bottom.
	void SetScreenshotRect(const wxRect &rect);
	void DrawText();

	// Confirmed static (asm lines 159885-159959; the manifest listed it as a
	// member, but the body never touches `this`) - looks up the platform's
	// savegame directory once, into s_strSaveGamePath.
	static void InitSaveGamePath();

	wxString GetSavegameComposedFile() const;
	// Confirmed no-op (asm lines 159999-160014).
	void SetSaveDataOnline(bool /*online*/) {
	}
	int GetSavegameNr() const;
	// "<savegame dir>/savegame" or "/bookmark", then the slot number (zero
	// padded to two digits) and ".dat".
	wxString GetFileName() const;

	// Confirmed static, taking the scene to be named (the manifest's
	// "this"-based signature hid that, asm lines 161687-162236): the scene's
	// language-specific name followed by the current local date and time,
	// "name 3.10.2026, 14:05h".
	static wxString MakeSaveGameName(const TVisObjRef &scene);
	// Takes a screenshot of the current frame, scaled to this slot's size,
	// and saves it to `file`.
	bool SaveSnapShot(const wxFileName &file);
	// Re-reads this slot's title and composed file name from its savegame.
	void CheckVisPaths();

	bool Delete();
	bool Exists() const;
	// True if the savegame directory holds at least one "savegame*.*" file.
	static bool SavegameExists();
	// Writes this slot out: the writer's data (plus, for a real savegame, its
	// screenshot) into a composed file under the savegame directory.
	bool SaveGame(const TBufferedProjectFileWriter &writer);
	// The slot numbers of every savegame file on disk, ascending; false if
	// there are none.
	static bool GetExistingSaveGames(std::vector<int> &outSlots);

private:
	static wxString s_strSaveGamePath;

	TPictureIO _picture;
	bool _screenshotUnavailable = false;
	wxString _displayName;
	wxString _composedFile;
	wxPoint _textPos;
	int _fontId = -1;
	int _textOffsetX = -1;
	int _slot;
	int _width;
	int _height;
	TVisionaireGame *_game;
	bool _isBookmark;
};
