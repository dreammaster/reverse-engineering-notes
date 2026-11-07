#include "TSoundFFMPEG.h"

#include <algorithm>
#include <cmath>

#include "Diagnostics.h"
#include "soundengine/audioMixer.h"
#include "soundengine/sdlAudio.h"

static int s_uniqueId = 0;

// The factor of the whole engine (the global volume / 100): the listener's gain of the original.
static float s_listenerGain = 1.0f;

// The gain a sound has: its volume as a percent / 100, the factor of its bus and the factor of its fade now, and the listener's.
static float gainOf(const TFFMPEGSoundItem &item) {
	return item._percent * 0.01f * item._busFactor * item._scale * s_listenerGain;
}

// 100 * 10^(dB / 20): the volume in decibels as a percent.
static float percentOf(float decibels) {
	return static_cast<float>(std::pow(10.0, decibels / 20.0) * 100.0);
}

// Confirmed (asm lines 1054181-1054280)
TFFMPEGSoundItem::~TFFMPEGSoundItem() {
	if (_stream) {
		_stream->Stop();
		delete _stream;
		_stream = nullptr;
	}
}

// Confirmed (asm lines 1060188-1060280): the sound plays, from where it is.
void TFFMPEGSoundItem::Play() {
	if (_stream)
		_stream->Play();
}

// Confirmed (asm lines 1059944-1059970)
void TFFMPEGSoundItem::Stop() {
	if (_stream)
		_stream->Stop();
}

// Confirmed (Night of the Rabbit): the gain is the volume, the factor of the bus and the factor of the fade.
void TFFMPEGSoundItem::UpdateVolume() {
	if (_stream)
		_stream->SetGain(gainOf(*this));
}

// Confirmed (asm lines 1059977-1060000)
void TFFMPEGSoundItem::ResetVolume() {
	if (_stream)
		_stream->SetGain(0.0f);
}

// Confirmed (Night of the Rabbit): the factor of the fade is kept.
void TFFMPEGSoundItem::ScaleVolume(float factor) {
	_scale = factor;
	UpdateVolume();
}

// Confirmed (Night of the Rabbit): the volume as a percent.
int TFFMPEGSoundItem::GetVolume() const {
	return static_cast<int>(_percent);
}

// Confirmed (asm lines 1059745-1059760)
int TFFMPEGSoundItem::GetPan() const {
	return static_cast<int>(_pan);
}

// Confirmed (asm lines 1059763-1059780)
bool TFFMPEGSoundItem::IsPlaying() const {
	return _stream && _stream->IsPlaying();
}

// Confirmed (asm lines 1060163-1060180)
int TFFMPEGSoundItem::GetOffset() const {
	return _stream ? _stream->GetOffset() : -1;
}

// Confirmed (asm lines 1059790-1059820)
int TFFMPEGSoundItem::GetDuration() const {
	return _stream ? _stream->GetDuration() : -1;
}

// Confirmed (asm lines 1060292-1060312)
float TFFMPEGSoundItem::GetSampleAvg() const {
	return _stream ? _stream->GetSampleAverage() : 0.0f;
}

// Confirmed (asm lines 1059826-1059845)
bool TFFMPEGSoundItem::IsLoop() const {
	return _stream && _stream->IsLoop();
}

// Confirmed (asm lines 1059911-1059935)
void TFFMPEGSoundItem::Pause() {
	if (_stream)
		_stream->Pause();
}

// Confirmed (asm lines 1057901-1057985): the volumes of the movies and of all are taken from the engine; the thread of
// the engine that calls Update() is the business of the program.
TSoundFFMPEG::TSoundFFMPEG() {
	_movieBase = _movieVolume;
	_globalBase = _globalVolume;
	_adjustedMovieVolume = static_cast<float>(_movieVolume * _movieBase / 100);
	_adjustedGlobalVolume = static_cast<float>(_globalVolume * _globalBase / 100);

#ifdef USE_REAL_SDL
	// (the standalone player has a sound device: the mixer is played by it)
	soundengine::OpenSdlAudio();
#endif
}

// Confirmed (asm lines 1057808-1057870)
TSoundFFMPEG::~TSoundFFMPEG() {
	CleanUp();
}

// Confirmed (asm lines 1056901-1056910)
int TSoundFFMPEG::GetNextUniqueId() {
	return s_uniqueId++;
}

// Confirmed (asm lines 1057016-1057040)
float TSoundFFMPEG::ConvertVolume(int volume) const {
	return static_cast<float>(volume);
}

float TSoundFFMPEG::ConvertBalance(int balance) const {
	return static_cast<float>(balance) / 100.0f;
}

// Confirmed (asm lines 1059852-1059860): there is a sound device when the backend says so.
bool TSoundFFMPEG::IsSoundSystemReady() const {
#ifdef USE_REAL_SDL
	return soundengine::IsSdlAudioOpen();
#else
	return false;
#endif
}

// (CreateStream() is in TSoundStreams.cpp.)

void TSoundFFMPEG::ApplyListenerVolume(float /*volume*/) {
}

// The factor of the bus of a kind of sound: the general volume of the kind / 100. (The original gives each default mixer
// `logf(volume / 100) * 10` and the sounds read the factor of their mixer; the mixers are not reconstructed, and the data of the
// games has no audio bus of its own, so the bus of a sound is the one of its kind.)
float TSoundFFMPEG::busFactorOf(TSoundTypeEnum type) const {
	switch (type) {
	case TSoundTypeEnum::kMusic:
		return static_cast<float>(_musicVolume) / 100.0f;
	case TSoundTypeEnum::kSound:
	case TSoundTypeEnum::kSound2:
		return static_cast<float>(_soundVolume) / 100.0f;
	case TSoundTypeEnum::kSpeech:
		return static_cast<float>(_speechVolume) / 100.0f;
	default:
		return 1.0f;
	}
}

// Confirmed (Night of the Rabbit, TSoundFFMPEG::Play(wxFileName const &, TVisObjRef const &, int, int, bool, bool, int, TSoundTypeEnum, int),
// asm lines 1227547-1228796). The file is a sound of the engine under its name (without what follows a '#'); a `streamed` sound
// that was stopped for the same file is used again. The volume is in decibels (-100 to 0): the sound's percent is 100 * 10^(dB / 20),
// its gain that with the factor of its bus. `fadeInMs` above 0 starts it silent and fades it in over that time; else it plays at its
// volume (a new sound begins by itself, one that is used again is told to play). The answer is the id of the sound, -1 when the
// file cannot be played.
int TSoundFFMPEG::PlaySound(const wxFileName &file, const TVisObjRef &/*bus*/, int volume, int balance, bool loop, bool streamed,
                            int fadeInMs, TSoundTypeEnum type, int offset) {
	wxString name = file.GetFullPath();
	int mark = name.Find(wxString(L"#"));

	if (mark != -1)
		name = name.Mid(0, mark);

	TFFMPEGSoundItem *item = nullptr;

	{
		wxCriticalSectionLocker locker(_lock);

		if (streamed)
			item = static_cast<TFFMPEGSoundItem *>(GetFreeSoundItem(name));
	}

	bool isNew = (item == nullptr);

	if (isNew)
		item = new TFFMPEGSoundItem();

	item->_paused = false;
	item->_stopped = false;
	item->_dead = false;
	item->_fading = false;
	item->_streamed = streamed;
	item->_name = name;
	item->_decibels = static_cast<float>(volume);
	item->_percent = percentOf(static_cast<float>(volume));
	item->_busFactor = busFactorOf(type);
	item->_offset = offset;
	item->_type = type;
	item->_pan = static_cast<float>(balance) / 100.0f;

	if (isNew) {
		item->_id = GetNextUniqueId();
		item->_stream = CreateStream(file, loop);
	}

	if (!item->_stream) {
		if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"Failed to open soundfile '%s'. Audio format is probably not supported or file is corrupt.",
			                   item->_name.wc_str());
		}

		if (isNew)
			delete item;

		return -1;
	}

	SetStats(item, static_cast<int>(item->_decibels), balance, type, loop, offset);

	if (fadeInMs > 0) {
		item->_fadeKeep = false;
		item->_fadeElapsed = 0;
		item->_fading = true;
		item->_fadeDuration = fadeInMs;
		item->_fadeFrom = 0.0f;
		item->_fadeTo = 1.0f;
		item->_scale = 0.0f;
		item->_stream->SetGain(0.0f);
		item->_stream->Play();
	} else {
		item->_stream->SetGain(_muted ? 0.0f : gainOf(*item));

		if (!isNew)
			item->Play();
	}

	int id = item->_id;

	if (isNew) {
		wxCriticalSectionLocker locker(_lock);

		_items.push_back(item);
	}

	return id;
}

// Confirmed (Night of the Rabbit, TSoundFFMPEG::SetStats(TSoundItem *, int, int, TSoundTypeEnum, bool, int), asm lines 1221698-1221816): the
// volume is clamped to -100..0 dB, the balance to -100..100; the loop flag of the stream is switched on when it differs (it is never
// switched off); an `offset` that is not 0 puts the sound at the place that Play() was given (not at `offset`).
bool TSoundFFMPEG::SetStats(TSoundItem *base, int volume, int balance, TSoundTypeEnum /*type*/, bool loop, int offset) {
	TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(base);

	if (!item)
		return false;

	item->_decibels = static_cast<float>(std::max(-100, std::min(volume, 0)));
	item->_percent = percentOf(item->_decibels);
	item->_pan = static_cast<float>(std::max(-100, std::min(balance, 100))) / 100.0f;

	if (TSoundStream *stream = item->_stream) {
		if (!_muted) {
			stream->SetGain(gainOf(*item));
			stream->SetPan(item->_pan);
		}

		if (loop != stream->IsLoop())
			stream->SetLoop(true);

		if (offset != 0)
			stream->SetOffset(item->_offset);
	}

	return true;
}

// Confirmed (Night of the Rabbit, TSoundFFMPEG::FadeSound(int, int, int, bool), asm lines 1221403-1221529): the sounds with that id start a
// fade of `milliseconds` to `decibels` (as a percent: 100 * 10^(dB / 20)); the volume they have now is the percent of the sound. With
// `keep`: a target above the volume becomes the volume of the sound and the factor goes from volume / target up to 1, a lower target
// leaves the volume and the factor goes from 1 down to target / volume; without `keep` the factor goes from volume / 100 to
// target / 100 (the volume is not changed).
void TSoundFFMPEG::FadeSound(int id, int milliseconds, int decibels, bool keep) {
	for (TSoundItem *base : _items) {
		TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(base);

		if (item->_id != id)
			continue;

		item->_fading = true;
		item->_fadeElapsed = 0;
		item->_fadeDuration = milliseconds;
		item->_fadeKeep = false;

		const float target = percentOf(static_cast<float>(decibels));
		const float now = static_cast<float>(item->GetVolume());

		if (keep) {
			item->_fadeKeep = true;

			if (target > now) {
				item->_percent = target;
				item->_fadeTo = 1.0f;
				item->_fadeFrom = now / target;
			} else {
				item->_fadeFrom = 1.0f;
				item->_fadeTo = target / now;
			}
		} else {
			item->_fadeTo = target / 100.0f;
			item->_fadeFrom = now / 100.0f;
		}
	}
}

// Confirmed (Night of the Rabbit, TSoundFFMPEG::AdjustVolume, asm lines 1225874-1225987): only the movies and the global volume are
// taken here (volume * base / 100); the global one is the gain of the whole engine. The volumes of the kinds of sound are the
// general volumes that Update() reads.
void TSoundFFMPEG::AdjustVolume(int /*music*/, int /*sound*/, int /*speech*/, int movie, int global) {
	wxCriticalSectionLocker locker(_lock);

	if (movie != -1)
		_adjustedMovieVolume = static_cast<float>(movie * _movieBase / 100);

	if (global != -1)
		_adjustedGlobalVolume = static_cast<float>(global * _globalBase / 100);

	if (!_muted) {
		_listenerVolume = _adjustedGlobalVolume;
		s_listenerGain = _adjustedGlobalVolume * 0.01f;
		ApplyListenerVolume(s_listenerGain);
	}
}

// Confirmed (asm lines 1056917-1057010): the streams that have ended are let go with their sounds.
void TSoundFFMPEG::updateFinishedStreams() {
	for (TSoundStream *stream : _finishedStreams) {
		for (auto it = _items.begin(); it != _items.end(); ++it) {
			TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(*it);

			if (item->_stream == stream) {
				_items.erase(it);
				delete item;
				break;
			}
		}
	}

	_finishedStreams.clear();
}

// Confirmed in the structure (asm lines 1058394-1058520); what makes a stream "ended" is the backend's (HasEnded()): a
// sound that is not streamed is marked dead and its stream is to be let go by the next turn.
void TSoundFFMPEG::updateStreamEnded() {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *base : _items) {
		TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(base);

		if (!item->_stream || !item->_stream->HasEnded() || item->_streamed)
			continue;

		item->_dead = true;
		_finishedStreams.push_back(item->_stream);
	}
}

// Confirmed (Night of the Rabbit, TSoundFFMPEG::Update, asm lines 1229114-1229541): the streams that have ended are let go; then, with
// the time since the last call, every sound goes through its fade (the factor is `from + (to - from) * elapsed / duration`; at the
// end it is `to`, and with `keep` the percent of the sound becomes percent * to and the factor 1; a sound whose end factor is
// 0.0001 or less is stopped and let go) and gets its gain from the volume, the factor of its bus and the factor of its fade.
void TSoundFFMPEG::Update() {
	// (the mixer renders the time that has gone by: the sounds play on, and their ends are known below)
	soundengine::AudioMixer::Get().Pump();

	wxCriticalSectionLocker locker(_lock);

	updateFinishedStreams();
	updateStreamEnded();

	const int elapsed = static_cast<int>(_timer.GetTime());

	_timer.SetTime();

	for (auto it = _items.begin(); it != _items.end();) {
		TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(*it);

		item->_busFactor = busFactorOf(item->_type);

		if (!item->_fading) {
			if (!_muted)
				item->UpdateVolume();

			++it;
			continue;
		}

		item->_fadeElapsed += elapsed;

		if (item->_fadeElapsed < item->_fadeDuration) {
			item->_scale = item->_fadeFrom + (item->_fadeTo - item->_fadeFrom) * static_cast<float>(item->_fadeElapsed) /
			                                     static_cast<float>(item->_fadeDuration);

			if (!_muted)
				item->UpdateVolume();

			++it;
			continue;
		}

		// the fade is over
		item->_scale = item->_fadeTo;

		if (!_muted)
			item->UpdateVolume();

		if (item->_fadeKeep) {
			item->_percent *= item->_fadeTo;
			item->_scale = 1.0f;
		}

		item->_fading = false;

		if (item->_fadeTo <= 0.0001f) {
			it = _items.erase(it);
			item->Stop();
			delete item;
		} else {
			++it;
		}
	}
}
