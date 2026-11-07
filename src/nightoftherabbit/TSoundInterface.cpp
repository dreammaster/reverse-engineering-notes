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
	: _disabled(false), _muted(false), _musicId(-1), _soundVolume(100), _musicVolume(100), _speechVolume(100), _movieVolume(100),
	  _globalVolume(100) {
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
		result.value = _muted;
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
		// (Night of the Rabbit: the sound is played on the audio bus of the message, none here; no fade, no offset)
		result.value = Play(wxFileName(signal.speechFile.ToStdWstring()), TVisObjRef(), signal.value, signal.value2, signal.value3 != 0,
		                    static_cast<TSoundTypeEnum>(signal.value4), 0, 0);
		break;
	case kSignalSoundStop:
		Stop(wxFileName(signal.speechFile.ToStdWstring()));
		break;
	case kSignalSoundFade:
		// (Night of the Rabbit: the engine goes on, and the answer is the one of the next message)
		Update();
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

// Confirmed (Night of the Rabbit, TSoundInterface::Play(wxFileName const &, TVisObjRef const &, int, int, bool, TSoundTypeEnum, int, int),
// asm lines 1215018-1215657): with the sounds turned off nothing plays. A sound of the kind 3 (the walking sound) is kept when it is
// over and used again for the same file (streamed), the others are not. The music: the one of the same name goes on (-1); another
// fades the old one out (to -100 dB in `fadeMs`) and is played looped, whatever `loop` is, with `fadeMs` to fade in.
int TSoundInterface::Play(const wxFileName &file, const TVisObjRef &bus, int volume, int balance, bool loop, TSoundTypeEnum type,
                          int fadeMs, int offset) {
	if (_disabled)
		return -1;

	if (type != TSoundTypeEnum::kMusic)
		return PlaySound(file, bus, volume, balance, loop, type == TSoundTypeEnum::kSound2, 0, type, offset);

	if (musicName(file).Cmp(musicName(_backgroundMusic)) == 0)
		return -1;

	if (_musicId != -1)
		FadeSound(_musicId, fadeMs, -100, false);

	_backgroundMusic = file;
	_musicId = PlaySound(file, bus, volume, balance, true, false, fadeMs, TSoundTypeEnum::kMusic, offset);
	return _musicId;
}

// Confirmed (Night of the Rabbit, TSoundInterface::StopBackgroundMusic): without `fade` the sound of the file stops, with it the sound
// fades to -100 dB in a second (FadeSound); the music is forgotten either way. (The id of the music is kept, as in the original.)
void TSoundInterface::StopBackgroundMusic(bool fade) {
	if (!_backgroundMusic.IsOk())
		return;

	if (fade)
		FadeSound(GetExistingSoundID(_backgroundMusic), 1000, -100, false);
	else
		Stop(_backgroundMusic);

	_backgroundMusic.Clear();
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

	AdjustVolume(music, sound, speech, movie, global);
}

// The sound engine (not reconstructed): nothing plays.
void TSoundInterface::Mute(bool /*mute*/) {
}

void TSoundInterface::Continue(TSoundTypeEnum /*type*/) {
}

void TSoundInterface::Pause(TSoundTypeEnum /*type*/) {
}

void TSoundInterface::PauseAll() {
}

void TSoundInterface::BusValuesUpdate() {
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

void TSoundInterface::FadeSound(int /*id*/, int /*milliseconds*/, int /*decibels*/, bool /*keep*/) {
}

void TSoundInterface::FadeAll(int /*milliseconds*/, int /*percent*/) {
}

bool TSoundInterface::IsSoundSystemReady() const {
	return false;
}

int TSoundInterface::PlaySound(const wxFileName &/*file*/, const TVisObjRef &/*bus*/, int /*volume*/, int /*balance*/, bool /*loop*/,
                               bool /*streamed*/, int /*fadeInMs*/, TSoundTypeEnum /*type*/, int /*offset*/) {
	return -1;
}

void TSoundInterface::AdjustVolume(int /*music*/, int /*sound*/, int /*speech*/, int /*movie*/, int /*global*/) {
}

void TSoundInterface::Update() {
}

void TSoundInterface::BusActivate(TVList */*busses*/) {
}

wxString TSoundInterface::GetExistingSoundFromID(int /*id*/) const {
	return wxString();
}

float TSoundInterface::GetSampleAvg(int /*id*/) const {
	return 0.0f;
}

void TSoundInterface::PrintSounds(std::list<wxString> &/*lines*/) const {
}

void TSoundInterface::Play(const wxFileName &/*file*/) {
}
