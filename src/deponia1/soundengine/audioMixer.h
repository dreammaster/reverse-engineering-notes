// The mixer of the sound engine: the sounds (TSoundStreams.cpp) and the audio of the movies (TMoviePlayerMkv.cpp) are sources that add
// their samples to the output, 48000 Hz, two channels. In the original this is OpenAL (the sources and buffers of
// soundengine::Stream, asm 1050000-1060000); this mixer is the part of it that is the same for every platform, and what plays the
// result is a sink: the engine on a platform with a sound device gives its device the frames of Render(), the tests and
// the headless run write them to a WAV file or drop them. The mixer goes by the clock of the system when nothing pulls from it
// (Pump(): it renders the frames that are due since the last call), so a sound has an offset and an end without a device.
#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace soundengine {

class AudioSource {
public:
	virtual ~AudioSource() {
	}

	/** Adds `frames` frames (two floats each, left then right, 1.0 is full scale) to `out`. Called with the mixer locked. */
	virtual void Mix(float *out, int frames) = 0;
};

class AudioMixer {
public:
	static const int kSampleRate = 48000;

	/** The mixer of the engine. */
	static AudioMixer &Get();

	void AddSource(AudioSource *source);
	void RemoveSource(AudioSource *source);

	/** Mixes `frames` frames of 16 bit stereo into `out` (the sum of the sources, clipped). */
	void Render(int16_t *out, int frames);

	/** Renders the frames that are due by the clock of the system since the last call (at most a quarter of a second at once) and
	 *  gives them to the sink; the first call starts the clock. */
	void Pump();

	/** Where the rendered frames go (null: nowhere). The sink is called with the mixer locked. */
	void SetSink(const std::function<void(const int16_t *, int)> &sink);

	/** How many frames have been rendered. */
	long long GetRenderedFrames() const;

	/** Takes the lock of the mixer: what a source's controls (volume, position ...) hold while they change the source. */
	std::unique_lock<std::recursive_mutex> Lock() {
		return std::unique_lock<std::recursive_mutex>(_mutex);
	}

private:
	mutable std::recursive_mutex _mutex;
	std::vector<AudioSource *> _sources;
	std::function<void(const int16_t *, int)> _sink;
	long long _rendered = 0;
	long long _clockStart = -1;
	std::vector<float> _mix;
	std::vector<int16_t> _out;
};

/** A WAV file of 16 bit stereo (what the headless run writes the sound to). */
class WavWriter {
public:
	WavWriter() = default;
	~WavWriter();

	WavWriter(const WavWriter &) = delete;
	WavWriter &operator=(const WavWriter &) = delete;

	bool Open(const char *path, int sampleRate, int channels);
	void Write(const int16_t *samples, int frames);
	/** Writes the sizes into the header and closes the file. */
	void Close();

private:
	void *_file = nullptr;
	int _channels = 2;
	int _sampleRate = 48000;
	long long _frames = 0;
};

} // End of namespace soundengine
