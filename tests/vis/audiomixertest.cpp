#include <cstdio>
#include <fstream>
#include <vector>

#include "AppGlobals.h"
#include "soundengine/audioMixer.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

using soundengine::AudioMixer;

/** A source with a constant level on each side. */
class Constant : public soundengine::AudioSource {
public:
	Constant(float left, float right) : _left(left), _right(right) {
	}

	void Mix(float *out, int frames) override {
		for (int i = 0; i < frames; i++) {
			out[i * 2] += _left;
			out[i * 2 + 1] += _right;
		}
	}

	float _left;
	float _right;
};

int main() {
	AudioMixer &mixer = AudioMixer::Get();
	std::vector<int16_t> out(8);

	// nothing: silence
	mixer.Render(out.data(), 4);
	CHECK(out[0] == 0 && out[7] == 0);

	// the sources add up, each side for itself
	Constant first(0.25f, -0.25f);
	Constant second(0.25f, 0.0f);

	mixer.AddSource(&first);
	mixer.AddSource(&second);
	mixer.AddSource(&second);   // (a source is in the mixer once)
	mixer.Render(out.data(), 4);
	CHECK(out[0] == static_cast<int16_t>(0.5f * 32767.0f));
	CHECK(out[1] == static_cast<int16_t>(-0.25f * 32767.0f));

	// the sum is clipped, not wrapped
	Constant loud(0.9f, -0.9f);

	mixer.AddSource(&loud);
	mixer.Render(out.data(), 4);
	CHECK(out[0] == 32767 && out[1] == static_cast<int16_t>(-1.15f < -1.0f ? -32767 : 0));

	// a source that is taken out is not heard
	mixer.RemoveSource(&loud);
	mixer.RemoveSource(&second);
	mixer.Render(out.data(), 4);
	CHECK(out[0] == static_cast<int16_t>(0.25f * 32767.0f));
	mixer.RemoveSource(&first);

	long long rendered = mixer.GetRenderedFrames();

	CHECK(rendered == 16);

	// the clock: the first Pump starts it, the next ones render the time that has gone by to the sink
	long long heard = 0;

	mixer.SetSink([&heard](const int16_t *, int frames) {
		heard += frames;
	});
	mixer.Pump();
	wxMilliSleep(200);
	mixer.Pump();
	printf("200 ms: %lld frames\n", heard);
	CHECK(heard >= 48000 * 150 / 1000 && heard <= 48000 * 400 / 1000);
	mixer.SetSink(nullptr);

	// the WAV file: the header and the samples
	{
		soundengine::WavWriter wav;

		CHECK(wav.Open("audiomixer_test.wav", 48000, 2));

		int16_t samples[6] = {1, -2, 300, -400, 32767, -32768};

		wav.Write(samples, 3);
		wav.Close();
	}
	{
		std::ifstream in("audiomixer_test.wav", std::ios::binary);
		std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		CHECK(bytes.size() == 44 + 12);
		CHECK(bytes.size() == 56 && std::string(bytes.begin(), bytes.begin() + 4) == "RIFF");
		CHECK(std::string(bytes.begin() + 8, bytes.begin() + 16) == "WAVEfmt ");
		CHECK(bytes[22] == 2 && bytes[24] == 0x80 && bytes[25] == 0xBB && bytes[34] == 16);
		CHECK(std::string(bytes.begin() + 36, bytes.begin() + 40) == "data");
		CHECK(bytes[40] == 12 && bytes[4] == 36 + 12);
		CHECK(bytes[44] == 1 && bytes[45] == 0 && bytes[46] == 0xFE && bytes[47] == 0xFF);
		CHECK(bytes[54] == 0x00 && bytes[55] == 0x80);
	}
	std::remove("audiomixer_test.wav");

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
