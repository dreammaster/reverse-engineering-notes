#include "TSoundBase.h"

#include <algorithm>

#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/mmedialib/soundBase.cpp";

/** The name of a file as the sounds know it: what is before a '#'. */
static wxString soundName(const wxFileName &file) {
	wxString name = file.GetFullPath();
	int mark = name.Find(wxString(L"#"));

	if (mark != -1)
		name = name.Mid(0, mark);

	return name;
}

// Confirmed (asm lines 1105856-1105885)
TSoundBase::TSoundBase() {
}

// Confirmed (asm lines 1060317-1060440)
TSoundBase::~TSoundBase() {
}

// Confirmed (asm lines 1101561-1101625): the sounds of a muted engine play with the volume 0.
void TSoundBase::Mute(bool mute) {
	wxCriticalSectionLocker locker(_lock);

	_muted = mute;

	for (TSoundItem *item : _items) {
		if (mute)
			item->ResetVolume();
		else
			item->UpdateVolume();
	}
}

// Confirmed (asm lines 1101347-1101420): the paused sounds of the kind go on (a sound that is stopped does not).
void TSoundBase::Continue(TSoundTypeEnum type) {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *item : _items) {
		if (!item->_paused || item->_type != type)
			continue;

		item->_paused = false;

		if (item->_stopped)
			continue;

		if (_muted)
			item->ResetVolume();

		item->Play();
	}
}

// Confirmed (asm lines 1101276-1101340)
void TSoundBase::Pause(TSoundTypeEnum type) {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *item : _items) {
		if (item->_paused || item->_type != type)
			continue;

		item->_paused = true;
		item->Pause();
	}
}

// Confirmed (asm lines 1101428-1101490)
void TSoundBase::ContinueAll() {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *item : _items) {
		if (!item->_paused)
			continue;

		item->_paused = false;

		if (item->_stopped)
			continue;

		if (_muted)
			item->ResetVolume();

		item->Play();
	}
}

// Confirmed (asm lines 1101498-1101555)
void TSoundBase::PauseAll() {
	wxCriticalSectionLocker locker(_lock);

	for (TSoundItem *item : _items) {
		if (item->_paused)
			continue;

		item->_paused = true;
		item->Pause();
	}
}

// Confirmed (asm lines 1102440-1102520): all the sounds are stopped and let go.
void TSoundBase::CleanUp() {
	wxCriticalSectionLocker locker(_lock);

	for (auto it = _items.begin(); it != _items.end();) {
		TSoundItem *item = *it;

		item->Stop();
		it = _items.erase(it);
		delete item;
	}
}

// Confirmed (asm lines 1104364-1104455): a sound that is kept (streamed) is only stopped and marked as such, another
// is let go.
void TSoundBase::Stop(const wxFileName &file) {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(file.GetFullPath());

	if (!item)
		return;

	if (item->_streamed) {
		item->Stop();
		item->_stopped = true;
		return;
	}

	for (auto it = _items.begin(); it != _items.end(); ++it) {
		if (*it == item) {
			item->Stop();
			_items.erase(it);
			delete item;
			return;
		}
	}
}

// Confirmed (asm lines 1102524-1102640)
bool TSoundBase::Stop(int id) {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	if (!item)
		return false;

	if (item->_streamed) {
		item->Stop();
		item->_stopped = true;
		return true;
	}

	for (auto it = _items.begin(); it != _items.end(); ++it) {
		if (*it == item) {
			item->Stop();
			_items.erase(it);
			delete item;
			break;
		}
	}

	return true;
}

// Confirmed (asm lines 1102335-1102430): a sound that plays is paused, a paused one goes on (with the volume 0 when
// the sounds are muted).
bool TSoundBase::TogglePause(int id) {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	if (!item)
		return false;

	if (item->_paused) {
		item->_paused = false;

		if (_muted)
			item->ResetVolume();

		item->Play();
	} else {
		item->_paused = true;
		item->Pause();
	}

	return true;
}

// Confirmed (asm lines 1103873-1103960): the id of the sound of the file when it plays, else -1.
int TSoundBase::GetExistingSoundID(const wxFileName &file) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(file.GetFullPath());

	if (item && item->IsPlaying())
		return item->_id;

	return -1;
}

// Confirmed (asm lines 1103331-1103470): the file of the sound with the id when it plays or is paused.
wxString TSoundBase::GetExistingSoundFromID(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	if (item && (item->IsPlaying() || item->_paused))
		return item->_name;

	return wxString();
}

// Confirmed (asm lines 1104148-1104250)
void TSoundBase::SetStats(const wxFileName &file, int volume, int balance, TSoundTypeEnum type, bool loop,
                          int offset) {
	wxCriticalSectionLocker locker(_lock);

	SetStats(GetSoundItem(file.GetFullPath()), volume, balance, type, loop, offset);
}

// Confirmed (asm lines 1101635-1101725): (the engine is asked also when there is no such sound)
bool TSoundBase::SetStats(int id, int volume, int balance, TSoundTypeEnum type, bool loop, int offset) {
	wxCriticalSectionLocker locker(_lock);

	return SetStats(GetSoundItem(id), volume, balance, type, loop, offset);
}

// Confirmed (asm lines 1104261-1104355): a sound that is neither paused nor stopped.
bool TSoundBase::IsPlaying(const wxFileName &file) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(file.GetFullPath());

	return item && !item->_paused && !item->_stopped && item->IsPlaying();
}

// Confirmed (asm lines 1102261-1102330)
bool TSoundBase::IsPlaying(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item && !item->_paused && item->IsPlaying();
}

// Confirmed (asm lines 1102200-1102255)
bool TSoundBase::IsPaused(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item && item->_paused;
}

// Confirmed (asm lines 1102123-1102195): the volume of the sound; -1 for none.
int TSoundBase::GetVolume(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item ? item->GetVolume() : -1;
}

// Confirmed (asm lines 1102045-1102118)
int TSoundBase::GetBalance(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item ? item->GetPan() : 0;
}

// Confirmed (asm lines 1101968-1102040)
int TSoundBase::GetOffset(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item ? item->GetOffset() : -1;
}

// Confirmed (asm lines 1101890-1101962)
int TSoundBase::GetDuration(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item ? item->GetDuration() : -1;
}

// Confirmed (asm lines 1101812-1101884)
float TSoundBase::GetSampleAvg(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item ? item->GetSampleAvg() : 0.0f;
}

// Confirmed (asm lines 1101735-1101806)
bool TSoundBase::IsLoop(int id) const {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(id);

	return item && item->IsLoop();
}

void TSoundBase::BusActivate(TVList */*busses*/) {
	// TODO (asm lines 1059271-1059700): the audio busses of the engine (soundengine::AudioBus, AudioBusBridge) are
	// not reconstructed.
}

void TSoundBase::BusValuesUpdate() {
	// TODO (asm lines 1050713-1050790): see BusActivate().
}

// The sounds that go with a fade (kOut) are stopped and let go.
void TSoundBase::removeFadeOutSounds() {
	for (auto it = _items.begin(); it != _items.end();) {
		TSoundItem *item = *it;

		if (item->_fade == TFadeEnum::kOut) {
			it = _items.erase(it);
			item->Stop();
			delete item;
		} else {
			++it;
		}
	}
}

// The new sounds of a fade (kIn) begin to play.
void TSoundBase::startNewSounds() {
	for (TSoundItem *item : _items) {
		if (item->_fade != TFadeEnum::kIn)
			continue;

		if (_muted)
			item->ResetVolume();

		item->Play();
	}
}

// The new sounds get their own volume, and the kept ones are like the others again.
void TSoundBase::clearFadeStates() {
	for (TSoundItem *item : _items) {
		if (item->_fade == TFadeEnum::kIn) {
			if (!_muted)
				item->UpdateVolume();

			item->_fade = TFadeEnum::kNone;
		} else if (item->_fade == TFadeEnum::kKeep) {
			item->_fade = TFadeEnum::kNone;
		}
	}
}

// Confirmed (asm lines 1102644-1102800): the fade is over at once.
void TSoundBase::FinishSoundFade() {
	wxCriticalSectionLocker locker(_lock);

	if (_soundFade == 0)
		return;

	removeFadeOutSounds();

	if (_soundFade >= static_cast<int>(TFadeEnum::kOut) && _soundFade <= static_cast<int>(TFadeEnum::kToNew))
		startNewSounds();

	clearFadeStates();
	_soundFade = 0;
}

// Confirmed (asm lines 1102808-1103095): the volumes of the sounds are scaled by the time that is gone; when the time
// is over (or `finish`) the fade ends - the new sounds begin and the old ones go, in a kInAndOut fade the old ones go
// first and the fade of the new ones begins then.
void TSoundBase::UpdateSoundFade(bool finish) {
	wxCriticalSectionLocker locker(_lock);

	if (_soundFade == 0)
		return;

	float elapsed = static_cast<float>(_timer.GetTime());

	if (_fadeDuration > elapsed && !finish) {
		if (_muted)
			return;

		for (TSoundItem *item : _items) {
			if (item->_fade == TFadeEnum::kOut) {
				if (_soundFade >= static_cast<int>(TFadeEnum::kOut) && _soundFade <= static_cast<int>(TFadeEnum::kToNew))
					item->ScaleVolume(1.0f - elapsed / _fadeDuration);
			} else if (item->_fade == TFadeEnum::kIn) {
				if (_soundFade == static_cast<int>(TFadeEnum::kToNew) || _soundFade == static_cast<int>(TFadeEnum::kIn))
					item->ScaleVolume(elapsed / _fadeDuration);
			}
		}

		return;
	}

	removeFadeOutSounds();

	if (_soundFade == static_cast<int>(TFadeEnum::kInAndOut)) {
		startNewSounds();
		_soundFade = static_cast<int>(TFadeEnum::kIn);
		_timer.SetTime();
		return;
	}

	if (_soundFade == static_cast<int>(TFadeEnum::kOut))
		startNewSounds();

	clearFadeStates();
	_soundFade = 0;
}

// Confirmed (asm lines 1103103-1103320): the sounds that are there are marked to go when `markExisting`; the new ones
// play at once in a fade that takes the old ones out and brings them in at the same time (kToNew), or in kIn, or when
// there is no fade.
void TSoundBase::StartSoundFade(TFadeEnum fade, int milliseconds, bool markExisting) {
	wxCriticalSectionLocker locker(_lock);

	x_assert(_soundFade == 0, "m_eSoundFade == eFadeNo", kSourceFile, 0x1DB);
	_soundFade = static_cast<int>(fade);
	_fadeDuration = (fade == TFadeEnum::kInAndOut) ? static_cast<float>(milliseconds >> 1) : static_cast<float>(milliseconds);

	if (markExisting) {
		for (TSoundItem *item : _items) {
			if (item->_fade == TFadeEnum::kNone)
				item->_fade = TFadeEnum::kOut;
		}
	}

	if (fade == TFadeEnum::kNone || fade == TFadeEnum::kIn)
		removeFadeOutSounds();

	if (fade == TFadeEnum::kToNew || fade == TFadeEnum::kIn || fade == TFadeEnum::kNone)
		startNewSounds();

	if (fade != TFadeEnum::kNone) {
		_timer.SetTime();
		return;
	}

	clearFadeStates();
	_soundFade = 0;
	_timer.SetTime();
}

// Confirmed (asm lines 1103494-1103550)
void TSoundBase::StartFadeInSounds() {
	startNewSounds();
}

// Confirmed (asm lines 1103554-1103610)
void TSoundBase::StopFadeOutSounds() {
	removeFadeOutSounds();
}

// Confirmed (asm lines 1103612-1103670)
void TSoundBase::FinishFadeIn() {
	clearFadeStates();
	_soundFade = 0;
}

// Confirmed (asm lines 1103974-1104055): the sound of the file fades out in the next fade.
void TSoundBase::FadeOut(const wxFileName &file) {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(file.GetFullPath());

	if (item && item->_fade == TFadeEnum::kNone)
		item->_fade = TFadeEnum::kOut;
}

// Confirmed (asm lines 1104063-1104140): the sound of the file is not touched by the fade.
void TSoundBase::Keep(const wxFileName &file) {
	wxCriticalSectionLocker locker(_lock);
	TSoundItem *item = GetSoundItem(file.GetFullPath());

	if (item)
		item->_fade = TFadeEnum::kKeep;
}

// Confirmed (asm lines 1103676-1103865)
TSoundItem *TSoundBase::GetSoundItem(const wxString &file) const {
	wxString name = file;
	int mark = name.Find(wxString(L"#"));

	if (mark != -1)
		name = name.Mid(0, mark);

	for (TSoundItem *item : _items) {
		if (!item->_dead && !item->_stopped && name.CmpNoCase(item->_name) == 0)
			return item;
	}

	return nullptr;
}

// Confirmed (asm lines 1104494-1104530)
TSoundItem *TSoundBase::GetSoundItem(int id) const {
	for (TSoundItem *item : _items) {
		if (!item->_dead && !item->_stopped && item->_id == id)
			return item;
	}

	return nullptr;
}

// Confirmed (asm lines 1104539-1104590): a sound that is stopped (and kept) for the same file.
TSoundItem *TSoundBase::GetFreeSoundItem(const wxString &file) const {
	for (TSoundItem *item : _items) {
		if (!item->_dead && item->_stopped && file.Cmp(item->_name) == 0)
			return item;
	}

	return nullptr;
}

// Confirmed (asm lines 1104598-1104650)
void TSoundBase::DeleteSoundItem(TSoundItem *item) {
	for (auto it = _items.begin(); it != _items.end(); ++it) {
		if (*it == item) {
			item->Stop();
			_items.erase(it);
			delete item;
			return;
		}
	}
}

// Confirmed (asm lines 1104658-1104910)
wxString TSoundBase::GetFadeTypeAsString(TFadeEnum fade) const {
	switch (fade) {
	case TFadeEnum::kNone:
		return wxString(L"No Fade");
	case TFadeEnum::kIn:
		return wxString(L"Fade In");
	case TFadeEnum::kOut:
		return wxString(L"Fade Out");
	case TFadeEnum::kInAndOut:
		return wxString(L"Fade InAndOut");
	case TFadeEnum::kToNew:
		return wxString(L"Fade To New");
	case TFadeEnum::kKeep:
		return wxString(L"Keep");
	}

	return wxString();
}

// Confirmed (asm lines 1104918-1105850): the lines for the console.
void TSoundBase::PrintSounds(std::list<wxString> &lines) const {
	wxCriticalSectionLocker locker(_lock);

	if (_items.empty()) {
		lines.push_back(wxString(L"No Loaded Sounds"));
		return;
	}

	if (_soundFade != 0) {
		lines.push_back(wxString(L"Current Sound Fade: ") + GetFadeTypeAsString(static_cast<TFadeEnum>(_soundFade)));
		lines.push_back(wxString());
	}

	lines.push_back(wxString(L"Loaded Sounds:"));

	for (TSoundItem *item : _items) {
		if (item->_dead)
			continue;

		wchar_t text[512];

		lines.push_back(wxString(L"Sound File: ") + item->_name);
		std::swprintf(text, sizeof(text) / sizeof(text[0]), L"Volume: %ld, Balance: %ld, Paused: %ls, Stopped: %ls",
		              static_cast<long>(item->GetVolume()), static_cast<long>(item->GetPan()),
		              item->_paused ? L"True" : L"False", item->_stopped ? L"True" : L"False");
		lines.push_back(wxString(text));
		std::swprintf(text, sizeof(text) / sizeof(text[0]), L"Streamed: %ls, Sound Fade: %ls",
		              item->_streamed ? L"True" : L"False", GetFadeTypeAsString(item->_fade).wc_str());
		lines.push_back(wxString(text));
		lines.push_back(wxString());
	}
}
