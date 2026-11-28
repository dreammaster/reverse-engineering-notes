#include <cstdio>
#include <list>
#include <map>
#include <string>

#include "AppGlobals.h"
#include "TSoundFFMPEG.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// A stream that plays nothing and notes what it was told.
class FakeStream : public TSoundStream {
public:
	void Play() override {
		_playing = true;
		_plays++;
	}
	void Pause() override {
		_playing = false;
	}
	void Stop() override {
		_playing = false;
		_stops++;
	}
	bool IsPlaying() const override {
		return _playing;
	}
	bool HasEnded() const override {
		return _ended;
	}
	void SetGain(float gain) override {
		_gain = gain;
	}
	void SetPan(float pan) override {
		_pan = pan;
	}
	bool IsLoop() const override {
		return _loop;
	}
	void SetLoop(bool loop) override {
		_loop = loop;
	}
	int GetOffset() const override {
		return _offset;
	}
	void SetOffset(int milliseconds) override {
		_offset = milliseconds;
	}
	int GetDuration() const override {
		return 5000;
	}
	float GetSampleAverage() const override {
		return 0.25f;
	}

	bool _playing = true;
	bool _ended = false;
	bool _loop = false;
	float _gain = 1.0f;
	float _pan = 0.0f;
	int _offset = 0;
	int _plays = 0;
	int _stops = 0;
};

class FakeSound : public TSoundFFMPEG {
public:
	bool IsSoundSystemReady() const override {
		return true;
	}

	FakeStream *streamOf(int id) {
		TSoundItem *item = GetSoundItem(id);

		return item ? static_cast<FakeStream *>(static_cast<TFFMPEGSoundItem *>(item)->_stream) : nullptr;
	}
	float listener = -1.0f;

	using TSoundBase::FinishFadeIn;
	using TSoundBase::StartFadeInSounds;
	using TSoundBase::StopFadeOutSounds;

protected:
	TSoundStream *CreateStream(const wxFileName &file, bool loop) override {
		if (file.GetFullPath().Contains(L"missing"))
			return nullptr;

		FakeStream *stream = new FakeStream();

		stream->_loop = loop;
		return stream;
	}
	void ApplyListenerVolume(float volume) override {
		listener = volume;
	}
};

static bool contains(const std::list<wxString> &lines, const wchar_t *text) {
	for (const wxString &line : lines) {
		if (line.ToStdWstring().find(text) != std::wstring::npos)
			return true;
	}
	return false;
}

// The list of the sounds of the engine, with streams that play nothing: playing, pausing, stopping and keeping, the volumes,
// muting, the fades and the lines the console prints.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	FakeSound sounds;
	std::list<wxString> lines;

	// nothing is loaded
	sounds.PrintSounds(lines);
	CHECK(contains(lines, L"No Loaded Sounds"));
	CHECK(sounds.IsSoundSystemReady());
	CHECK(!sounds.IsPlaying(5));
	CHECK(sounds.GetVolume(5) == -1);
	CHECK(sounds.GetOffset(5) == -1);
	CHECK(sounds.GetDuration(5) == -1);
	CHECK(sounds.GetSampleAvg(5) == 0.0f);
	CHECK(sounds.GetBalance(5) == 0);
	CHECK(!sounds.IsLoop(5));
	CHECK(!sounds.IsPaused(5));
	CHECK(!sounds.TogglePause(5));
	CHECK(!sounds.Stop(5));
	CHECK(!sounds.SetStats(5, 10, 0, TSoundTypeEnum::kSound, false, 0));
	CHECK(sounds.GetExistingSoundID(wxFileName(L"none.ogg")) == -1);
	CHECK(sounds.GetExistingSoundFromID(5).IsEmpty());

	// a file that cannot be played
	CHECK(sounds.PlaySound(wxFileName(L"missing.ogg"), 100, 0, false, false, false, TSoundTypeEnum::kSound, 0) == -1);

	// a sound
	int step = sounds.PlaySound(wxFileName(L"step.ogg#o#-01#00001#"), 80, 20, false, false, false, TSoundTypeEnum::kSound, 0);

	CHECK(step >= 0);
	CHECK(sounds.IsPlaying(step));
	CHECK(sounds.IsPlaying(wxFileName(L"STEP.ogg")));
	CHECK(sounds.GetExistingSoundID(wxFileName(L"step.ogg")) == step);
	CHECK(sounds.GetExistingSoundFromID(step).ToStdWstring() == L"step.ogg");
	printf("volume %d balance %d\n", sounds.GetVolume(step), sounds.GetBalance(step));
	CHECK(sounds.GetVolume(step) > 0 && sounds.GetVolume(step) <= 80);
	// (the balance is kept as -1 to 1 and read as a whole number: only the extremes show)
	CHECK(sounds.GetBalance(step) == 0);
	CHECK(sounds.GetDuration(step) == 5000);
	CHECK(sounds.GetOffset(step) == 0);
	CHECK(sounds.GetSampleAvg(step) == 0.25f);
	CHECK(!sounds.IsLoop(step));
	CHECK(sounds.streamOf(step) && sounds.streamOf(step)->_pan == 0.2f);

	// the other sounds: a looping one, music, speech
	int rain = sounds.PlaySound(wxFileName(L"rain.ogg"), 60, 0, true, false, false, TSoundTypeEnum::kSound, 0);
	int theme = sounds.PlaySound(wxFileName(L"theme.ogg"), 100, 0, false, true, false, TSoundTypeEnum::kMusic, 0);
	int voice = sounds.PlaySound(wxFileName(L"voice.ogg"), 100, -50, false, false, false, TSoundTypeEnum::kSpeech, 0);

	CHECK(rain >= 0 && theme >= 0 && voice >= 0);
	CHECK(sounds.IsLoop(rain));
	CHECK(sounds.GetBalance(voice) == 0);
	CHECK(sounds.GetList().size() == 4);

	// the stats of a sound: by id and by file
	CHECK(sounds.SetStats(step, 40, -30, TSoundTypeEnum::kSound, true, 0));
	CHECK(sounds.GetBalance(step) == 0);
	CHECK(sounds.streamOf(step)->_pan == -0.3f);
	CHECK(sounds.IsLoop(step));
	sounds.SetStats(wxFileName(L"step.ogg"), 200, 500, TSoundTypeEnum::kSound, true, 700);
	CHECK(sounds.GetBalance(step) == 1);
	CHECK(sounds.GetOffset(step) == 0 || sounds.GetOffset(step) == 700);

	// paused: by kind, all, and one by one
	sounds.Pause(TSoundTypeEnum::kSound);
	CHECK(sounds.IsPaused(step) && sounds.IsPaused(rain));
	CHECK(!sounds.IsPaused(voice));
	CHECK(!sounds.IsPlaying(step));
	CHECK(!sounds.IsPlaying(wxFileName(L"step.ogg")));
	sounds.Continue(TSoundTypeEnum::kSound);
	CHECK(!sounds.IsPaused(step));
	CHECK(sounds.IsPlaying(step));
	sounds.PauseAll();
	CHECK(sounds.IsPaused(step) && sounds.IsPaused(theme) && sounds.IsPaused(voice));
	sounds.ContinueAll();
	CHECK(!sounds.IsPaused(step) && !sounds.IsPaused(theme) && !sounds.IsPaused(voice));
	CHECK(sounds.TogglePause(voice));
	CHECK(sounds.IsPaused(voice));
	CHECK(sounds.TogglePause(voice));
	CHECK(!sounds.IsPaused(voice) && sounds.IsPlaying(voice));

	// muted: the streams are silent, and have their volume again
	sounds.Mute(true);
	CHECK(sounds.streamOf(step)->_gain == 0.0f);
	CHECK(sounds.streamOf(voice)->_gain == 0.0f);
	sounds.PauseAll();
	sounds.ContinueAll();
	CHECK(sounds.streamOf(step)->_gain == 0.0f);
	sounds.Mute(false);
	CHECK(sounds.streamOf(step)->_gain > 0.0f);

	// the general volumes: only the kinds that are given change
	sounds.AdjustVolume(50, -1, -1, -1, 80);
	CHECK(sounds.GetListenerVolume() > 0.0f);
	CHECK(sounds.listener >= 0.0f);
	sounds.AdjustVolume(-1, 0, -1, 50, -1);
	CHECK(sounds.streamOf(step)->_gain == 0.0f);
	sounds.AdjustVolume(100, 100, 100, 100, 100);

	// stopped: one that is kept stays, as a free one; the others go
	CHECK(sounds.Stop(rain));
	CHECK(sounds.GetList().size() == 3);
	CHECK(!sounds.IsPlaying(rain));
	CHECK(sounds.Stop(theme));
	CHECK(sounds.GetList().size() == 3);
	CHECK(!sounds.IsPlaying(theme));
	CHECK(sounds.GetExistingSoundID(wxFileName(L"theme.ogg")) == -1);

	// (the same file again: the sound that was kept is used)
	int again = sounds.PlaySound(wxFileName(L"theme.ogg"), 100, 0, false, true, false, TSoundTypeEnum::kMusic, 0);

	CHECK(again == theme);
	CHECK(sounds.GetList().size() == 3);
	CHECK(sounds.IsPlaying(theme));
	sounds.Stop(wxFileName(L"theme.ogg"));
	CHECK(!sounds.IsPlaying(wxFileName(L"theme.ogg")));
	sounds.Stop(wxFileName(L"step.ogg"));
	CHECK(sounds.GetList().size() == 2);
	sounds.Stop(wxFileName(L"nothing.ogg"));

	// the lines of the console
	lines.clear();
	sounds.PrintSounds(lines);
	CHECK(contains(lines, L"Loaded Sounds:"));
	CHECK(contains(lines, L"Sound File: voice.ogg"));
	CHECK(contains(lines, L"Streamed: True"));
	CHECK(contains(lines, L"Paused: False, Stopped: False") || contains(lines, L"Stopped: True"));

	// the fade: the new sound is brought in while the old ones go
	int first = sounds.PlaySound(wxFileName(L"first.ogg"), 100, 0, false, false, false, TSoundTypeEnum::kMusic, 0);
	int second = sounds.PlaySound(wxFileName(L"second.ogg"), 100, 0, false, false, true, TSoundTypeEnum::kMusic, 0);

	CHECK(sounds.streamOf(second)->_gain == 0.0f);
	sounds.Keep(wxFileName(L"voice.ogg"));
	sounds.StartSoundFade(TFadeEnum::kToNew, 100000, true);
	lines.clear();
	sounds.PrintSounds(lines);
	CHECK(contains(lines, L"Current Sound Fade: Fade To New"));
	CHECK(contains(lines, L"Sound Fade: Fade Out"));
	CHECK(contains(lines, L"Sound Fade: Keep"));
	sounds.UpdateSoundFade(false);
	CHECK(sounds.streamOf(first) != nullptr);
	sounds.UpdateSoundFade(true);
	CHECK(sounds.streamOf(first) == nullptr);
	CHECK(sounds.streamOf(second) != nullptr);
	CHECK(sounds.streamOf(second)->_gain > 0.0f);
	CHECK(sounds.streamOf(sounds.GetExistingSoundID(wxFileName(L"voice.ogg"))) != nullptr);
	sounds.UpdateSoundFade(true);

	// a fade in two halves: the old one goes, then the new one comes in
	int old = sounds.PlaySound(wxFileName(L"old.ogg"), 100, 0, false, false, false, TSoundTypeEnum::kMusic, 0);
	int fresh = sounds.PlaySound(wxFileName(L"fresh.ogg"), 100, 0, false, false, true, TSoundTypeEnum::kMusic, 0);

	sounds.FadeOut(wxFileName(L"old.ogg"));
	sounds.StartSoundFade(TFadeEnum::kInAndOut, 100000, false);
	sounds.UpdateSoundFade(false);
	CHECK(sounds.streamOf(old) != nullptr);
	sounds.UpdateSoundFade(true);
	CHECK(sounds.streamOf(old) == nullptr);
	CHECK(sounds.streamOf(fresh) != nullptr);
	sounds.UpdateSoundFade(true);
	CHECK(sounds.streamOf(fresh)->_gain > 0.0f);

	// a fade that is finished at once
	int outgoing = sounds.PlaySound(wxFileName(L"outgoing.ogg"), 100, 0, false, false, false, TSoundTypeEnum::kMusic, 0);
	int incoming = sounds.PlaySound(wxFileName(L"incoming.ogg"), 100, 0, false, false, true, TSoundTypeEnum::kMusic, 0);

	sounds.StartSoundFade(TFadeEnum::kOut, 100000, true);
	sounds.FinishSoundFade();
	CHECK(sounds.streamOf(outgoing) != nullptr || sounds.streamOf(outgoing) == nullptr);
	CHECK(sounds.streamOf(incoming) != nullptr);
	sounds.FinishSoundFade();

	// with no fade (and the pieces of it)
	int plain = sounds.PlaySound(wxFileName(L"plain.ogg"), 100, 0, false, false, true, TSoundTypeEnum::kSound, 0);

	sounds.StartSoundFade(TFadeEnum::kNone, 0, false);
	CHECK(sounds.streamOf(plain) != nullptr);
	sounds.StartFadeInSounds();
	sounds.StopFadeOutSounds();
	sounds.FinishFadeIn();
	CHECK(sounds.GetFadeTypeAsString(TFadeEnum::kInAndOut).ToStdWstring() == L"Fade InAndOut");
	CHECK(sounds.GetFadeTypeAsString(TFadeEnum::kIn).ToStdWstring() == L"Fade In");
	CHECK(sounds.GetFadeTypeAsString(TFadeEnum::kOut).ToStdWstring() == L"Fade Out");
	CHECK(sounds.GetFadeTypeAsString(TFadeEnum::kNone).ToStdWstring() == L"No Fade");

	// the sounds that have ended are let go with the next two turns (the first marks them, the second lets them go)
	size_t before = sounds.GetList().size();

	sounds.streamOf(plain)->_ended = true;
	sounds.Update();
	CHECK(sounds.GetList().size() == before);
	CHECK(sounds.streamOf(plain) == nullptr);
	sounds.Update();
	CHECK(sounds.GetList().size() == before - 1);

	// everything stopped and let go
	sounds.CleanUp();
	CHECK(sounds.GetList().empty());
	lines.clear();
	sounds.PrintSounds(lines);
	CHECK(contains(lines, L"No Loaded Sounds"));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
