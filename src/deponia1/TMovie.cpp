#include "TMovie.h"

#include <algorithm>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TComposedFileManager.h"
#include "TSoundInterface.h"
#include "graphicslib/graphics.h"

// The key of the scrambled headers of the movies (asm: unk_DCADC8).
static const wchar_t *const kMovieKey = L"VIS4MOVPWS";

std::vector<TMovie *> TMovie::s_openMovies;

TMoviePlayer *(*g_createMoviePlayer)() = nullptr;

// Without a player the movies cannot be shown; the movie ends at once.
TMoviePlayer *CreateMoviePlayer() {
	return g_createMoviePlayer ? g_createMoviePlayer() : nullptr;
}

// Confirmed (asm lines 1071898-1071950)
TMovie::TMovie() {
	s_openMovies.push_back(this);
}

// Confirmed (asm lines 491536-491650): the strings and the functions go; the player is not looked at (Finish() has let it go).
TMovie::~TMovie() {
	delete _player;
}

// Confirmed (asm lines 1069490-1069540): the original makes the VideoState (and answers whether that worked).
bool TMovie::Initialize(bool preview) {
	delete _player;
	_player = CreateMoviePlayer();

	if (!_player)
		return false;

	if (preview)
		_preview = true;

	return true;
}

// Confirmed (asm lines 1069552-1070393)
bool TMovie::PlayCutScene(const wxFileName &file, const TMovieSettings &settings, bool encrypted, bool showBlackScreenAfter) {
	if (!_player)
		return false;

	_deleteFile = false;
	_scrambleAgain = false;
	_blackScreenAfter = showBlackScreenAfter;

	TMovieSettings movieSettings = settings;

	if (settings.soundManager)
		movieSettings.volume = std::min(settings.soundManager->GetMovieVolume(), settings.soundManager->GetGlobalVolume());

	wxFileName normalized = file;

	normalized.NormalizePath();

	bool decrypted = false;

	if (encrypted) {
		// the header of a movie in a container is scrambled: unscramble it for the time of the movie
		wxFileName unscrambled;

		if (TComposedFileManager::DecryptComposedFile(normalized, unscrambled, wxString(kMovieKey))) {
			_path = unscrambled.GetFullPath();
			_scrambleAgain = true;
			decrypted = true;
		}
	}

	if (!decrypted) {
		wxFileName container;

		if (TComposedFileManager::GetComposedMovieFileName(normalized, container))
			_path = container.GetFullPath();
		else
			_path = normalized.GetFullPath();
	}

	bool opened = _player->Open(_path, movieSettings);

	if (!opened) {
		// maybe the header of the movie is scrambled after all: unscrambled (for good) and tried again
		wxFileName unscrambled;

		if (TComposedFileManager::DecryptComposedFile(file, unscrambled, wxString(kMovieKey))) {
			_path = unscrambled.GetFullPath();
			opened = _player->Open(_path, movieSettings);

			if (!opened) {
				// it was not that: scrambled again
				TComposedFileManager::DecryptComposedFile(file, unscrambled, wxString(kMovieKey));
			}
		}
	}

	_file = file;
	return opened;
}

// Confirmed (asm lines 1071209-1071863; the player does what the ffplay of the original does)
bool TMovie::OneFrame() {
	if (!_player)
		return false;

	if (_player->Frame(_eventFunction))
		return true;

	Finish();
	return false;
}

// Confirmed (asm lines 1070622-1070825)
void TMovie::Finish() {
	auto it = std::find(s_openMovies.begin(), s_openMovies.end(), this);

	if (it != s_openMovies.end())
		s_openMovies.erase(it);

	if (_deleteFile)
		wxRemoveFile(_path);

	if (_scrambleAgain)
		TComposedFileManager::EncryptComposedFile(_file, wxString(kMovieKey));

	if (_player) {
		_player->Close();
		delete _player;
		_player = nullptr;
	}

	if (_blackScreenAfter) {
		wxCriticalSectionLocker lock(g_loadingScreenLock);

		graphics->SetMatrixMode(true, false);
		graphics->ResetMatrix(true, true);
	}
}

// Confirmed (asm lines 1069054-1069120): both toggle the pause of the stream (the original asks whether it is paused first)
void TMovie::Pause() {
	if (_player)
		_player->Pause();
}

void TMovie::Resume() {
	if (_player)
		_player->Resume();
}

// Confirmed (asm lines 1071863-1071898)
void TMovie::Seek(float seconds) {
	if (_player)
		_player->Seek(seconds);
}

// Confirmed (asm lines 1070442-1070502): not a number when there is no movie
float TMovie::GetTime() {
	return _player ? _player->GetTime() : 0.0f;
}

float TMovie::GetDuration() {
	return _player ? _player->GetDuration() : 0.0f;
}

float TMovie::GetCompletition() {
	return _player ? _player->GetCompletition() : 0.0f;
}

// Confirmed (asm lines 1068991-1069039): 0 without a movie
int TMovie::GetWidth() {
	return _player ? _player->GetWidth() : 0;
}

int TMovie::GetHeight() {
	return _player ? _player->GetHeight() : 0;
}

void TMovie::SetBlend(int blend) {
	if (_player)
		_player->SetBlend(blend);
}

void TMovie::SetColour(const wxColour &colour) {
	if (_player)
		_player->SetColour(colour);
}

void TMovie::SetLoop(bool loop) {
	if (_player)
		_player->SetLoop(loop);
}

// Confirmed (asm lines 1070825-1071209)
bool TMovie::NowaitDraw(float x, float y, float width, float height) {
	return _player ? _player->Draw(x, y, width, height) : false;
}
