#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

#include "AppGlobals.h"
#include "TSoundFFMPEG.h"
#include "datastruct/visobjref.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)
#define NEAR(a, b) do { if (std::fabs((a) - (b)) > 0.02f) { printf("FAIL line %d: %s = %f, not %f\n", __LINE__, #a, (double)(a), (double)(b)); failures++; } } while (0)

// A stream that only says what it was told (the engine's backend, here a fake): the volume it was given and whether it plays.
static std::map<int, bool> g_alive;

class FakeStream : public TSoundStream {
public:
	FakeStream(int serial, bool loop) : _serial(serial), _loop(loop) {
		g_alive[serial] = true;
	}
	~FakeStream() override {
		g_alive[_serial] = false;
	}
	void Play() override {
		_playing = true;
	}
	void Pause() override {
		_playing = false;
	}
	void Stop() override {
		_playing = false;
	}
	bool IsPlaying() const override {
		return _playing;
	}
	bool HasEnded() const override {
		return false;
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
		return 0;
	}
	void SetOffset(int) override {
	}
	int GetDuration() const override {
		return 10000;
	}
	float GetSampleAverage() const override {
		return 0.0f;
	}

	int _serial;
	bool _playing = true;
	bool _loop;
	float _gain = -1.0f;
	float _pan = 0.0f;
};

class FakeSound : public TSoundFFMPEG {
public:
	std::vector<FakeStream *> made;

protected:
	TSoundStream *CreateStream(const wxFileName &, bool loop) override {
		FakeStream *stream = new FakeStream(static_cast<int>(made.size()), loop);

		made.push_back(stream);
		return stream;
	}
};

static void pass(FakeSound &sound, int milliseconds) {
	// (the time of the engine is the real one: a few updates over the time)
	for (int i = 0; i < milliseconds / 10; i++) {
		wxMilliSleep(10);
		sound.Update();
	}
}

// What Night of the Rabbit's sound engine does with volumes (decibels, the general volumes as the factor of the bus) and fades (a fade
// of each sound: Play with a fade in, FadeSound, FadeAll, and the background music's), checked with a stream that only records.
// Build with `tools/tbuild.py soundfadetest.cpp --game nightoftherabbit`.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	FakeSound sound;
	TVisObjRef none;

	// the volume is in decibels: -6 dB is half (0.501), above 0 is full, below -100 is -100
	int a = sound.Play(wxFileName(L"a.opus"), none, -6, 0, false, TSoundTypeEnum::kSound, 0, 0);

	CHECK(a != -1 && sound.made.size() == 1);
	NEAR(sound.made[0]->_gain, 0.501f);

	int b = sound.Play(wxFileName(L"b.opus"), none, 10, 50, false, TSoundTypeEnum::kSound, 0, 0);

	NEAR(sound.made[1]->_gain, 1.0f);
	NEAR(sound.made[1]->_pan, 0.5f);

	int c = sound.Play(wxFileName(L"c.opus"), none, -200, 0, false, TSoundTypeEnum::kSound, 0, 0);

	CHECK(c != -1);
	NEAR(sound.made[2]->_gain, 0.0f);

	// the general volumes are the factor of the bus of the kind: the sounds at half
	sound.SetVolume(100, 50, 100, 100, 100);
	sound.Update();
	NEAR(sound.made[0]->_gain, 0.501f * 0.5f);
	NEAR(sound.made[1]->_gain, 0.5f);
	sound.SetVolume(100, 100, 100, 100, 100);
	sound.Update();
	NEAR(sound.made[1]->_gain, 1.0f);

	// a fade in: silent at the start, at its volume at the end
	// (TSoundInterface::Play gives a sound that is not music no fade in: the engine's PlaySound is asked)
	int d = sound.PlaySound(wxFileName(L"d.opus"), none, 0, 0, false, false, 300, TSoundTypeEnum::kSound, 0);

	CHECK(d != -1);
	NEAR(sound.made[3]->_gain, 0.0f);
	pass(sound, 150);
	// (the sleeps of the machine are longer than asked: only that it is on the way is checked)
	CHECK(sound.made[3]->_gain > 0.05f && sound.made[3]->_gain < 0.99f);
	pass(sound, 250);
	NEAR(sound.made[3]->_gain, 1.0f);

	// FadeSound with keep: the sound (at 0 dB) fades to -20 dB (0.1) and keeps that volume
	sound.FadeSound(b, 200, -20, true);
	pass(sound, 100);
	CHECK(sound.made[1]->_gain < 0.9f && sound.made[1]->_gain > 0.1f);
	pass(sound, 200);
	NEAR(sound.made[1]->_gain, 0.1f);
	pass(sound, 50);
	NEAR(sound.made[1]->_gain, 0.1f);
	CHECK(sound.GetVolume(b) == 10);

	// FadeAll: the sounds (not the music) fade to 0 and are let go; the music goes on
	int m = sound.Play(wxFileName(L"m1.opus"), none, 0, 0, false, TSoundTypeEnum::kMusic, 0, 0);

	CHECK(m != -1);
	sound.FadeAll(200, 0);
	pass(sound, 100);
	CHECK(sound.made[3]->_gain < 0.9f);
	pass(sound, 250);
	CHECK(!g_alive[0] && !g_alive[1] && !g_alive[2] && !g_alive[3]);
	CHECK(g_alive[4] && sound.made[4]->_playing);
	NEAR(sound.made[4]->_gain, 1.0f);

	// the background music: the same file goes on (-1), another fades the old one out and plays, looped, with a fade in
	CHECK(sound.Play(wxFileName(L"m1.opus#s#001#00002#"), none, 0, 0, false, TSoundTypeEnum::kMusic, 100, 0) == -1);
	CHECK(sound.made.size() == 5);

	int first = sound.Play(wxFileName(L"x1.opus"), none, 0, 0, false, TSoundTypeEnum::kMusic, 100, 0);
	int second = sound.Play(wxFileName(L"x2.opus"), none, 0, 0, false, TSoundTypeEnum::kMusic, 200, 0);

	CHECK(first != -1 && second != -1 && second != first);
	CHECK(sound.made[6]->_loop);
	NEAR(sound.made[6]->_gain, 0.0f);
	pass(sound, 350);
	CHECK(!g_alive[5] && g_alive[6]);
	NEAR(sound.made[6]->_gain, 1.0f);

	// StopBackgroundMusic with a fade: the sound fades to nothing in a second and is let go; the music is forgotten
	sound.StopBackgroundMusic(true);
	pass(sound, 500);
	CHECK(g_alive[6] && sound.made[6]->_gain < 0.9f);
	pass(sound, 650);
	CHECK(!g_alive[6]);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
