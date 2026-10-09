#include "TSoundFFMPEG.h"

#include <algorithm>

#include "Diagnostics.h"

static int s_uniqueId = 0;

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

// Confirmed (asm lines 1060041-1060070): the volume of the sound as a part of 1.
void TFFMPEGSoundItem::UpdateVolume() {
	if (_stream)
		_stream->SetGain(_volume / 100.0f);
}

// Confirmed (asm lines 1059977-1060000)
void TFFMPEGSoundItem::ResetVolume() {
	if (_stream)
		_stream->SetGain(0.0f);
}

// Confirmed (asm lines 1060008-1060035)
void TFFMPEGSoundItem::ScaleVolume(float factor) {
	if (_stream)
		_stream->SetGain(factor * _volume / 100.0f);
}

// Confirmed (asm lines 1059727-1059742)
int TFFMPEGSoundItem::GetVolume() const {
	return static_cast<int>(_volume);
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
	_adjustedMovieVolume = static_cast<float>(AdjustToGeneralVolume(_movieVolume, TSoundTypeEnum::kMovie));
	_adjustedGlobalVolume = static_cast<float>(AdjustToGeneralVolume(_globalVolume, TSoundTypeEnum::kGlobal));
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
	return false;
}

TSoundStream *TSoundFFMPEG::CreateStream(const wxFileName &/*file*/, bool /*loop*/) {
	// TODO: the OpenAL/FFmpeg streams (soundengine::Stream, asm 1050000-1060000) are not reconstructed.
	return nullptr;
}

void TSoundFFMPEG::ApplyListenerVolume(float /*volume*/) {
}

// Confirmed (asm lines 1055974-1056720). The file is a sound of the engine under its name (without what follows a '#'); a
// `streamed` sound that was stopped for the same file is used again. The volume is the one the sound gets with the general
// ones (SetStats), `fadeIn` starts it silent (to be faded in) and else it plays at its volume (a new sound begins by
// itself, one that is used again is told to play). The answer is the id of the sound, -1 when the file cannot be played.
int TSoundFFMPEG::PlaySound(const wxFileName &file, int volume, int balance, bool loop, bool streamed, bool fadeIn,
                            TSoundTypeEnum type, int offset) {
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
	item->_streamed = streamed;
	item->_name = name;
	item->_baseVolume = volume;
	item->_volume = static_cast<float>(AdjustToGeneralVolume(volume, type));
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

	SetStats(item, item->_baseVolume, balance, type, loop, offset);

	if (fadeIn) {
		item->_fade = TFadeEnum::kIn;
		item->_stream->SetGain(0.0f);
		item->_stream->Play();
	} else {
		item->_fade = TFadeEnum::kNone;
		item->_stream->SetGain(_muted ? 0.0f : item->_volume / 100.0f);

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

// Confirmed (asm lines 1055098-1055220): the volume of the sound is the one given (0 to 100) with the general ones, its
// balance -100 to 100; the loop flag of the stream is switched on when it differs (it is never switched off); a
// `offset` that is not 0 puts the sound at the place that Play() was given (not at `offset`).
bool TSoundFFMPEG::SetStats(TSoundItem *base, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) {
	TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(base);

	if (!item)
		return false;

	item->_baseVolume = std::max(0, std::min(volume, 100));
	item->_volume = static_cast<float>(AdjustToGeneralVolume(item->_baseVolume, type));
	item->_pan = static_cast<float>(std::max(-100, std::min(balance, 100))) / 100.0f;

	if (TSoundStream *stream = item->_stream) {
		if (!_muted) {
			stream->SetGain(item->_volume / 100.0f);
			stream->SetPan(item->_pan);
		}

		if (loop != stream->IsLoop())
			stream->SetLoop(true);

		if (offset != 0)
			stream->SetOffset(item->_offset);
	}

	return true;
}

// Confirmed (asm lines 1053026-1053500): the volumes of the sounds of a kind are the volume given (-1: not changed) and
// the volume each one has; the movies and the global volume are kept for the engine, which sets its own volume.
void TSoundFFMPEG::AdjustVolume(int music, int sound, int speech, int movie, int global) {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *base : _items) {
		TFFMPEGSoundItem *item = static_cast<TFFMPEGSoundItem *>(base);
		int percent;

		switch (item->_type) {
		case TSoundTypeEnum::kMusic:
			percent = music;
			break;
		case TSoundTypeEnum::kSound:
		case TSoundTypeEnum::kSound2:
			percent = sound;
			break;
		case TSoundTypeEnum::kSpeech:
			percent = speech;
			break;
		default:
			continue;
		}

		if (percent == -1)
			continue;

		item->_volume = static_cast<float>(percent * item->_baseVolume / 100);

		if (!_muted)
			item->UpdateVolume();
	}

	if (movie != -1)
		_adjustedMovieVolume = static_cast<float>(movie * _movieBase / 100);

	if (global != -1)
		_adjustedGlobalVolume = static_cast<float>(global * _globalBase / 100);

	if (!_muted) {
		_listenerVolume = _adjustedGlobalVolume;
		ApplyListenerVolume(_adjustedGlobalVolume * 0.01f);
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

// Confirmed (asm lines 1058521-1058640)
void TSoundFFMPEG::Update() {
	wxCriticalSectionLocker locker(_lock);

	updateFinishedStreams();
	updateStreamEnded();
}
