#include "TSoundInterface.h"

#include "AppGlobals.h"
#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/mmedialib/sound.cpp";

// The name of a background music is what is before a '#' in its file name (the rest tells which
// version of it is meant).
static wxString musicName(const wxFileName &file) {
	wxString name = file.GetFullPath();
	int mark = name.Find(wxString(L"#"));

	if (mark != -1)
		name = name.Mid(0, mark);

	return name;
}

// Confirmed (asm lines 1106061-1106085)
TSoundInterface::TSoundInterface()
	: _disabled(false), _flag(false), _field0C(0), _soundVolume(100), _musicVolume(100), _speechVolume(100),
	  _movieVolume(100), _globalVolume(100) {
}

TSoundInterface::~TSoundInterface() {
}

// Confirmed (asm lines 1106089-1106097)
void TSoundInterface::DisableSounds(bool disable) {
	_disabled = disable;
}

// Confirmed (asm lines 1106519-1106693): the message types 0x1000..0x1012 (the others are
// refused). The answer has the type of the question (0x1002 answers with 0), and the result in
// `value`.
void TSoundInterface::Signal(const TSignalData &signal, TSignalData &result) {
	result.type = 0;

	switch (signal.type) {
	case kSignalSoundIsPlaying:
		result.type = kSignalSoundIsPlaying;
		result.value = IsPlaying(wxFileName(signal.speechFile.ToStdWstring()));
		break;
	case kSignalSoundFlag:
		result.type = kSignalSoundFlag;
		result.value = _flag;
		break;
	case kSignalSoundMute:
		Mute(signal.value != 0);
		break;
	case kSignalSoundsDisabled:
		result.type = kSignalSoundsDisabled;
		result.value = _disabled;
		break;
	case kSignalMusicVolume:
		result.type = kSignalMusicVolume;
		result.value = _musicVolume;
		break;
	case kSignalSpeechVolume:
		result.type = kSignalSpeechVolume;
		result.value = _speechVolume;
		break;
	case kSignalMovieVolume:
		result.type = kSignalMovieVolume;
		result.value = _movieVolume;
		break;
	case kSignalGlobalVolume:
		result.type = kSignalGlobalVolume;
		result.value = _globalVolume;
		break;
	case kSignalSoundPlay:
		result.value = Play(wxFileName(signal.speechFile.ToStdWstring()), signal.value, signal.value2, signal.value3 != 0,
		                    static_cast<TSoundTypeEnum>(signal.value4), true, 0);
		break;
	case kSignalSoundStop:
		Stop(wxFileName(signal.speechFile.ToStdWstring()));
		break;
	case kSignalSoundFade:
		// (the fade is updated, and the answer is the one of the next message)
		UpdateSoundFade(signal.value != 0);
	// fall through
	case kSignalSoundSystemReady:
		result.type = kSignalSoundSystemReady;
		result.value = IsSoundSystemReady();
		break;
	default:
		x_assert(false, "false", kSourceFile, 0x77);
		break;
	}
}

// Confirmed (asm lines 1106106-1106510)
int TSoundInterface::Play(const wxFileName &file, int volume, int balance, bool loop, TSoundTypeEnum type, bool flag,
                          int value) {
	if (_disabled)
		return -1;

	if (type != TSoundTypeEnum::kMusic) {
		// a sound: the speech (2) and the walking sound (3) are told to the engine as such
		return PlaySound(file, volume, balance, loop, type == TSoundTypeEnum::kSound2, (int)type, false, value);
	}

	if (musicName(file).Cmp(musicName(_backgroundMusic)) == 0) {
		// the music that plays already goes on
		if (flag)
			Keep(_backgroundMusic);

		return -1;
	}

	// another music: the old one fades out (when the flag says so)
	if (flag)
		FadeOut(_backgroundMusic);

	_backgroundMusic = file;
	return PlaySound(_backgroundMusic, volume, balance, true, false, 0, flag, value);
}

// Confirmed (asm lines 1106703-1106745)
void TSoundInterface::StopBackgroundMusic(bool fade) {
	if (!_backgroundMusic.IsOk())
		return;

	if (fade)
		FadeOut(_backgroundMusic);
	else
		Stop(_backgroundMusic);

	_backgroundMusic.Clear();
}

// Confirmed (asm lines 1106762-1106768)
void TSoundInterface::KeepCurrentBackgroundMusic() {
	Keep(_backgroundMusic);
}

// Confirmed (asm lines 1106778-1107064)
void TSoundInterface::KeepCurrentBackgroundMusicIfSameAs(const wxFileName &file) {
	if (musicName(file).Cmp(musicName(_backgroundMusic)) == 0)
		Keep(_backgroundMusic);
}

// Confirmed (asm lines 1107076-1107107)
void TSoundInterface::SetVolume(int music, int sound, int speech, int movie, int global) {
	if ((unsigned int)music <= 100)
		_musicVolume = music;

	if ((unsigned int)sound <= 100)
		_soundVolume = sound;

	if ((unsigned int)speech <= 100)
		_speechVolume = speech;

	if ((unsigned int)movie <= 100)
		_movieVolume = movie;

	if ((unsigned int)global <= 100)
		_globalVolume = global;

	VolumesChanged();
}

// Confirmed (asm lines 1107204-1107268)
int TSoundInterface::AdjustToGeneralVolume(int volume, TSoundTypeEnum type) const {
	switch (type) {
	case TSoundTypeEnum::kMusic:
		return volume * _musicVolume / 100;
	case TSoundTypeEnum::kSound:
	case TSoundTypeEnum::kSound2:
		return volume * _soundVolume / 100;
	case TSoundTypeEnum::kSpeech:
		return volume * _speechVolume / 100;
	case TSoundTypeEnum::kMovie:
		return volume * _movieVolume / 100;
	case TSoundTypeEnum::kGlobal:
		return volume * _globalVolume / 100;
	}

	x_assert(false, "false", kSourceFile, 0x16D);
	return 0;
}

// The sound engine (not reconstructed): nothing plays.
void TSoundInterface::Mute(bool /*mute*/) {
}

void TSoundInterface::ContinueAll() {
}

void TSoundInterface::CleanUp() {
}

void TSoundInterface::Stop(const wxFileName &/*file*/) {
}

void TSoundInterface::SetStats(const wxFileName &/*file*/, int /*volume*/, int /*balance*/, TSoundTypeEnum /*type*/,
                               bool /*flag*/, int /*value*/) {
}

bool TSoundInterface::Stop(int /*id*/) {
	return false;
}

bool TSoundInterface::TogglePause(int /*id*/) {
	return false;
}

int TSoundInterface::GetExistingSoundID(const wxFileName &/*file*/) const {
	return -1;
}

bool TSoundInterface::SetStats(int /*id*/, int /*volume*/, int /*balance*/, TSoundTypeEnum /*type*/, bool /*loop*/,
                               int /*offset*/) {
	return false;
}

bool TSoundInterface::IsPlaying(int /*id*/) const {
	return false;
}

bool TSoundInterface::IsPaused(int /*id*/) const {
	return false;
}

int TSoundInterface::GetVolume(int /*id*/) const {
	return -1;
}

int TSoundInterface::GetBalance(int /*id*/) const {
	return 0;
}

int TSoundInterface::GetOffset(int /*id*/) const {
	return -1;
}

int TSoundInterface::GetDuration(int /*id*/) const {
	return -1;
}

bool TSoundInterface::IsLoop(int /*id*/) const {
	return false;
}

bool TSoundInterface::IsPlaying(const wxFileName &/*file*/) const {
	return false;
}

void TSoundInterface::FinishSoundFade() {
}

void TSoundInterface::StartSoundFade(TFadeEnum /*fade*/, int /*milliseconds*/, bool /*flag*/) {
}

void TSoundInterface::UpdateSoundFade(bool /*update*/) {
}

void TSoundInterface::Keep(const wxFileName &/*file*/) {
}

bool TSoundInterface::IsSoundSystemReady() const {
	return false;
}

int TSoundInterface::PlaySound(const wxFileName &/*file*/, int /*volume*/, int /*balance*/, bool /*loop*/,
                               bool /*walking*/, int /*type*/, bool /*flag*/, int /*value*/) {
	return -1;
}

void TSoundInterface::FadeOut(const wxFileName &/*file*/) {
}

void TSoundInterface::VolumesChanged() {
}

void TSoundInterface::Play(const wxFileName &/*file*/) {
}
