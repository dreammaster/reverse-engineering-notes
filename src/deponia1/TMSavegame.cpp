#include "TMSavegame.h"

#include <algorithm>
#include <ctime>
#include <string>

#include "AppGlobals.h"
#include "vscommon/fontManager.h"
#include "TComposedFileManager.h"
#include "TGameClientSDK.h"
#include "TGScene.h"
#include "TStandardPaths.h"
#include "TTScene.h"
#include "TTempFile.h"
#include "baselib/composedfile.h"
#include "datastruct/visionaire.h"
#include "datastruct/vlist.h"
#include "graphicslib/graphics.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/visionaireGame.h"

// The real code builds each slot-number part with wxString::Format(L"%d", n).
static wxString slotText(int number) {
	return wxString(std::to_wstring(number));
}

// TGameControl implements every accessor used below, but g_pGameControl is
// only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

wxString TMSavegame::s_strSaveGamePath;

// Confirmed (asm lines 159541-159885): the screenshot is addressed by a
// sprite path that names a file inside the slot's composed savegame file
// ("vtp_savepic<slot>.webp"; the "#g#-01#00001#" suffix is the sprite
// path's own embedded settings, parsed by TSprite); the slot starts out
// inactive.
TMSavegame::TMSavegame(bool isBookmark, int slot, int width, int height, TVisionaireGame *game)
	: _slot(slot), _width(width), _height(height), _game(game), _isBookmark(isBookmark) {
	_active = false;

	wxString name = wxString(L"vtp_savepic") + slotText(_slot) + wxString(L".webp#g#-01#00001#");
	wxFileName path;
	path.SetFullName(name);
	_picture.SetPath(TCharHolder(path.GetFullPath()));
	_picture.SetTransparency(static_cast<eTransparencyMode>(0), 0);
}

// Confirmed (asm lines 159388-159521).
TMSavegame::~TMSavegame() {
	graphics->RemoveFromCache(_picture.GetSpriteName());
}

// Confirmed (asm lines 159885-159959).
void TMSavegame::InitSaveGamePath() {
	TStandardPaths paths;
	s_strSaveGamePath = wxString(paths.GetSavegamePath());
}

// Confirmed (asm lines 159959-159999).
wxString TMSavegame::GetSavegameComposedFile() const {
	return _composedFile;
}

// Confirmed (asm lines 160014-160032).
int TMSavegame::GetSavegameNr() const {
	return _slot;
}

// Confirmed (asm lines 160032-160199).
wxString TMSavegame::GetFileName() const {
	wxString name = s_strSaveGamePath;
	name += _isBookmark ? wxString(L"/bookmark") : wxString(L"/savegame");
	if (_slot <= 9)
		name += wxString(L"0");
	name += slotText(_slot);
	name += wxString(L".dat");
	return name;
}

// Confirmed (asm lines 160199-160840): draws the slot's screenshot. While
// the picture isn't loaded yet (and hasn't already failed to), points it at
// the screenshot inside the savegame's composed file - first with the game's
// own password ("SAVEGAMEPWD30"), then, if that file won't open, with none -
// and loads it; a savegame that opens neither way is marked so Draw() stops
// retrying every frame.
void TMSavegame::Draw() {
	if (!_picture.RefreshSprite(false) && !_screenshotUnavailable) {
		wxString pictureName = wxString(L"vtp_savepic") + slotText(_slot) + wxString(L".webp#g#-01#00001#");
		wxFileName pictureFile(pictureName.ToStdWstring());
		pictureFile.NormalizePath();
		_picture.SetPath(TCharHolder(pictureFile.GetFullPath()));

		wxFileName saveFile(GetFileName().ToStdWstring());
		saveFile.NormalizePath();
		if (TComposedFileManager::SetSavegameFile(saveFile, wxString(L"SAVEGAMEPWD30"))) {
			_picture.LoadPicture(_picture.GetPath(), TPictureIO::eLoadSetting::Normal);
		} else {
			wxFileName saveFileNoPassword(GetFileName().ToStdWstring());
			saveFileNoPassword.NormalizePath();
			if (TComposedFileManager::SetSavegameFile(saveFileNoPassword, wxString()))
				_picture.LoadPicture(_picture.GetPath(), TPictureIO::eLoadSetting::Normal);
			else
				_screenshotUnavailable = true;
		}
	}
	_picture.Draw(1.0f, 0xFFFFFFFF);
}

// Confirmed (asm lines 160840-161687). A slot only changes state when the
// request differs from its current one, and a bookmark never does.
// Deactivating releases the screenshot (sprite, cache entry and path).
// Activating - if the savegame's composed file opens - loads the slot's
// title from the savegame's own data (field 0x219 of its game record), its
// composed file name (field 0x268's file name) and the font id its text uses
// (id of link 0x1D5), then, if the menu scene names a font (link 0x15C),
// measures the title in that font to centre it across the slot's width.
// The slot becomes active either way.
void TMSavegame::SetActive(bool active) {
	if (_active == active)
		return;
	if (_isBookmark)
		return;

	if (!active) {
		_picture.RemoveSprite();
		graphics->RemoveFromCache(_picture.GetSpriteName());
		_picture.SetPath(TCharHolder(""));
		_active = active;
		return;
	}

	wxFileName saveFile(GetFileName().ToStdWstring());
	saveFile.NormalizePath();
	if (TComposedFileManager::SetSavegameFile(saveFile, passwd)) {
		TVisObjRef font = gameControl()->GetScene()->GetRef().GetLink(0x15C);
		if (!font.IsEmpty()) {
			// The real constructor builds a stand-alone TVisionaireGame here
			// (the same standing "same object as TVisionaire" gap as
			// everywhere else this pair of classes shows up).
			TVisionaireGame game;
			wxString dataName = wxString(L"vtp_savedata") + slotText(_slot) + wxString(L".xml#g#-01#00000#");
			wxFileName dataPath;
			dataPath.SetFullName(dataName);
			wxFileName dataFile(dataPath.GetFullPath().ToStdWstring());
			dataFile.NormalizePath();
			game.LoadSaveGame(dataFile, passwd);

			_displayName = game.GetGame().GetStr(0x219);
			_composedFile = wxFileName(game.GetGame().GetPath(0x268)).GetFullName();
			TVisObjRef fontLink = game.GetGame().GetLink(0x1D5);
			_fontId = *reinterpret_cast<const int *>(fontLink.GetId());

			TFontManager *fonts = gameControl()->GetFontManager();
			fonts->SetCurrentFont(font);
			wxPoint textSize;
			fonts->GetTextDimension(_displayName, textSize);
			_textOffsetX = (_width >> 1) - (textSize.x >> 1);
		}
		_picture.RefreshSprite(false);
	}
	_active = active;
}

// Confirmed (asm lines 161687-162236) - see the header comment. The real
// code formats each number with wxString::Format(L"%d", n) and, when
// there's no current time to read, returns just the scene name.
wxString TMSavegame::MakeSaveGameName(const TVisObjRef &scene) {
	wxString name = TTScene(scene).GetNameLanguage();

	struct tm *now = wxDateTime::GetTmNow();
	if (!now)
		return name;

	name += wxString(L" ");
	name += slotText(now->tm_mday);
	name += wxString(L".");
	name += slotText(now->tm_mon + 1);
	name += wxString(L".");
	name += slotText(now->tm_year + 1900);
	name += wxString(L", ");
	name += slotText(now->tm_hour);
	name += (now->tm_min > 9) ? wxString(L":") : wxString(L":0");
	name += slotText(now->tm_min);
	name += wxString(L"h");
	return name;
}

// Confirmed (asm lines 162236-162327): the captured frame (if there is one
// with a real size) is rescaled to this slot's size and saved to `file`.
bool TMSavegame::SaveSnapShot(const wxFileName &file) {
	TPictureIO snapshot;
	TPictureMEM *frame = graphics->GetCapturedFrame();
	if (!frame || frame->GetWidth() <= 0 || frame->GetHeight() <= 0)
		return false;

	snapshot.SetMemoryBlock(graphics->GetMainMemBlock());
	snapshot.ResizeImage(*frame, wxPoint{_width, _height}, false, true);
	return snapshot.SavePicture(file, true);
}

// Confirmed (asm lines 162327-162986): the same data load as SetActive()'s,
// without the font handling, run when a savegame's file paths need
// refreshing (TGameControl::LoadGame()).
void TMSavegame::CheckVisPaths() {
	wxFileName saveFile(GetFileName().ToStdWstring());
	saveFile.NormalizePath();
	if (!TComposedFileManager::SetSavegameFile(saveFile, passwd))
		return;

	wxString dataName = wxString(L"vtp_savedata") + slotText(_slot) + wxString(L".xml#g#-01#00000#");
	wxFileName dataPath;
	dataPath.SetFullName(dataName);
	TVisionaireGame game;
	wxFileName dataFile(dataPath.GetFullPath().ToStdWstring());
	dataFile.NormalizePath();
	game.LoadSaveGame(dataFile, passwd);

	_displayName = game.GetGame().GetStr(0x219);
	_composedFile = wxFileName(game.GetGame().GetPath(0x268)).GetFullName();
}

// Confirmed (asm lines 162986-163037): the screenshot goes at the rect's
// top-left; the title's anchor is the rect's left edge (plus the centring
// offset SetActive() worked out) and its bottom. (The 100.0 scale constant
// passed to TSprite::SetPosition() is the sprite's own percentage.)
void TMSavegame::SetScreenshotRect(const wxRect &rect) {
	_picture.SetPosition(wxPoint{rect.GetLeft(), rect.GetTop()}, 100.0f);
	_textPos.x = rect.GetLeft() + _textOffsetX;
	_textPos.y = rect.GetBottom();
}

// Confirmed (asm lines 163037-163096): the title, in the menu scene's font,
// at the position SetScreenshotRect() worked out - nothing if the scene
// names no font.
void TMSavegame::DrawText() {
	TVisObjRef font = gameControl()->GetScene()->GetRef().GetLink(0x15C);
	if (font.IsEmpty())
		return;

	TFontManager *fonts = gameControl()->GetFontManager();
	fonts->SetCurrentFont(font);
	gameControl()->GetFontManager()->PrintText(_displayName, TextAlignmentEnum::kLeft, _textPos, 1.0f, nullptr);
}

// Confirmed (asm lines 163096-163219): removes the savegame file, telling
// Steam's cloud to drop it first when Steam is active.
bool TMSavegame::Delete() {
	wxString fileName = GetFileName();
	TSteamSDK *steam = gameControl()->GetGameClientSDK()->GetSteam();
	if (steam->GetStatus())
		steam->DeleteCloudSavegame(fileName);
	return wxRemoveFile(fileName);
}

// Confirmed (asm lines 163219-163282).
bool TMSavegame::Exists() const {
	return wxFile::Exists(GetFileName());
}

// Confirmed (asm lines 163282-163411).
bool TMSavegame::SavegameExists() {
	wxDir dir;
	if (!dir.Open(s_strSaveGamePath))
		return false;

	wxString name;
	return dir.GetFirst(&name, wxString(L"savegame*.*"), 0);
}

// Confirmed (asm lines 163411-164367) - see the header comment for why this
// currently writes nothing. A real savegame first saves its screenshot to a
// temp file (failing the whole save if that fails); either kind then packs
// the writer's data - and, for a real savegame, the screenshot - into one
// composed file (container type 4, password "SAVEGAMEPWD30"), creating the
// savegame directory if need be, and writes it to this slot's file. On
// success the screenshot's cached sprite is evicted and the slot reloaded
// (deactivated then reactivated). The original also passes an empty
// StringHashMap<wxString,wxString,...> to AddFile(); that overload isn't
// modeled, so the 3-argument one stands in.
bool TMSavegame::SaveGame(const TBufferedProjectFileWriter &writer) {
	wxFileName snapshotFile;
	if (!_isBookmark) {
		wxString name = wxString(L"vtp_savepic") + slotText(_slot);
		snapshotFile = wxFileName(TTempFile::AddTempFile(name, wxString(L"webp")).ToStdWstring());
		if (!SaveSnapShot(snapshotFile)) {
			if (wxLog::loglevel > 0)
				wxLog::logexpanded(L"SaveGame: Saving snapshot failed");
			return false;
		}
	}

	TComposedFile composed;
	composed.InitForWrite(-1, static_cast<TContainerTypeEnum>(4), true);

	wxFileName emptyName;
	wxFileName dataName(writer.GetBufferName().ToStdWstring());
	dataName.NormalizePath();
	composed.AddData(writer.GetBuffer(), dataName, emptyName, 0);

	if (!_isBookmark) {
		wxFileName snapshot = snapshotFile;
		snapshot.NormalizePath();
		composed.AddFile(snapshot, emptyName, 0);
	}

	wxString password = L"SAVEGAMEPWD30";
	if (!wxDir::Exists(s_strSaveGamePath))
		wxFileName::Mkdir(s_strSaveGamePath, 0x1FF, 0);

	wxFileName outFile(GetFileName().ToStdWstring());
	outFile.NormalizePath();
	bool written = composed.WriteToDisk(outFile, password, nullptr, nullptr);
	if (!written && wxLog::loglevel > 0)
		wxLog::logexpanded(L"SaveGame: Write to disk failed");

	TTempFile::DeleteTempFiles();

	if (written) {
		graphics->RemoveFromCache(_picture.GetSpriteName());
		SetActive(false);
		SetActive(true);
	}
	return written;
}

// Confirmed (asm lines 164367-164898): every "savegame*.*" file in the
// savegame directory contributes its slot number (the digits after the
// 8-character "savegame" prefix, up to the first "."); names whose number
// doesn't parse are skipped. Sorted ascending; false if none were found.
bool TMSavegame::GetExistingSaveGames(std::vector<int> &outSlots) {
	outSlots.clear();

	wxDir dir;
	if (wxDir::Exists(s_strSaveGamePath) && dir.Open(s_strSaveGamePath)) {
		wxString name;
		bool more = dir.GetFirst(&name, wxString(L"savegame*.*"), 0);
		while (more) {
			std::wstring full = name.ToStdWstring();
			std::size_t dot = full.find(L'.');
			// The original's std::wstring::substr() throws out_of_range for a
			// name shorter than "savegame"; the directory filter above makes
			// that unreachable.
			wxString number(full.substr(8, dot == std::wstring::npos ? std::wstring::npos : dot - 8));

			long value;
			if (!number.IsEmpty() && number.ToLong(&value, 10))
				outSlots.push_back(static_cast<int>(value));
			more = dir.GetNext(&name);
		}
	}

	std::sort(outSlots.begin(), outSlots.end());
	return !outSlots.empty();
}
