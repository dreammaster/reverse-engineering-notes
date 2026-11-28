// The stream of a sound (TSoundFFMPEG::CreateStream, what a platform backend can replace): the file is decoded as it is played
// (soundengine::AudioDataSource, the Ogg Opus of the data) and mixed into soundengine::AudioMixer with the volume and the
// balance of the sound, so that the sound has its real offset, length and end whether or not there is a device behind the mixer.
#include <algorithm>
#include <cmath>
#include <vector>

#include "TComposedFileManager.h"
#include "TMemoryBuffer.h"
#include "TSoundFFMPEG.h"
#include "baselib/file.h"
#include "soundengine/audioDataSource.h"
#include "soundengine/audioMixer.h"

namespace {

using soundengine::AudioMixer;

class TMixedSoundStream : public TSoundStream, public soundengine::AudioSource {
public:
	TMixedSoundStream(std::vector<char> &&data, bool loop) : _data(std::move(data)), _loop(loop) {
		_source = soundengine::AudioDataSource::TestOpen(_data.data(), _data.size());

		if (!_source)
			return;

		_duration = _source->GetDuration();
		_channels = _source->GetChannels();
		_buffer.resize(4096 * static_cast<size_t>(_channels));
		AudioMixer::Get().AddSource(this);

		// (a new sound begins by itself)
		_playing = true;
	}

	~TMixedSoundStream() override {
		AudioMixer::Get().RemoveSource(this);
		delete _source;
	}

	bool IsOpen() const {
		return _source != nullptr;
	}

	void Mix(float *out, int frames) override {
		if (!_playing || _ended)
			return;

		float left = (_pan > 0.0f) ? 1.0f - _pan : 1.0f;
		float right = (_pan < 0.0f) ? 1.0f + _pan : 1.0f;
		float sum = 0.0f;
		int mixed = 0;

		while (mixed < frames) {
			if (_bufferPos >= _bufferFrames && !refill())
				break;

			int available = std::min(frames - mixed, _bufferFrames - _bufferPos);

			for (int i = 0; i < available; i++) {
				const int16_t *in = &_buffer[static_cast<size_t>(_bufferPos + i) * _channels];
				float a = static_cast<float>(in[0]) / 32768.0f;
				float b = (_channels > 1) ? static_cast<float>(in[1]) / 32768.0f : a;

				out[(mixed + i) * 2] += a * _gain * left;
				out[(mixed + i) * 2 + 1] += b * _gain * right;
				sum += std::fabs(a) + std::fabs(b);
			}

			_bufferPos += available;
			_positionFrames += available;
			mixed += available;
		}

		if (mixed > 0)
			_sampleAverage = sum / static_cast<float>(mixed * 2);
	}

	void Play() override {
		auto lock = AudioMixer::Get().Lock();

		_playing = true;
	}

	void Pause() override {
		auto lock = AudioMixer::Get().Lock();

		_playing = false;
	}

	void Stop() override {
		auto lock = AudioMixer::Get().Lock();

		_playing = false;
		_ended = false;
		seekTo(0);
	}

	bool IsPlaying() const override {
		return _playing && !_ended;
	}

	bool HasEnded() const override {
		return _ended;
	}

	void SetGain(float gain) override {
		auto lock = AudioMixer::Get().Lock();

		_gain = gain;
	}

	void SetPan(float pan) override {
		auto lock = AudioMixer::Get().Lock();

		_pan = std::max(-1.0f, std::min(1.0f, pan));
	}

	bool IsLoop() const override {
		return _loop;
	}

	void SetLoop(bool loop) override {
		auto lock = AudioMixer::Get().Lock();

		_loop = loop;
	}

	int GetOffset() const override {
		return static_cast<int>(_positionFrames * 1000 / AudioMixer::kSampleRate);
	}

	void SetOffset(int milliseconds) override {
		auto lock = AudioMixer::Get().Lock();

		_ended = false;
		seekTo(milliseconds);
	}

	int GetDuration() const override {
		return _duration;
	}

	float GetSampleAverage() const override {
		return _sampleAverage;
	}

private:
	/** The next samples of the file; false at its end (a sound that loops starts again). */
	bool refill() {
		for (int attempt = 0; attempt < 2; attempt++) {
			int bytes = _source->FillBuffer(reinterpret_cast<char *>(_buffer.data()), _buffer.size() * sizeof(int16_t));

			if (bytes > 0) {
				_bufferFrames = bytes / static_cast<int>(sizeof(int16_t) * _channels);
				_bufferPos = 0;
				return true;
			}

			if (!_loop) {
				_ended = true;
				return false;
			}

			_source->Seek(0);
			_positionFrames = 0;
		}

		// (a file with nothing in it that loops)
		_ended = true;
		return false;
	}

	void seekTo(int milliseconds) {
		_source->Seek(milliseconds);
		_positionFrames = static_cast<long long>(milliseconds) * AudioMixer::kSampleRate / 1000;
		_bufferFrames = 0;
		_bufferPos = 0;
	}

	std::vector<char> _data;
	soundengine::AudioDataSource *_source = nullptr;
	std::vector<int16_t> _buffer;
	int _bufferFrames = 0;
	int _bufferPos = 0;
	int _channels = 2;
	int _duration = 0;
	long long _positionFrames = 0;
	bool _loop;
	bool _playing = false;
	bool _ended = false;
	float _gain = 1.0f;
	float _pan = 0.0f;
	float _sampleAverage = 0.0f;
};

} // End of anonymous namespace

TSoundStream *TSoundFFMPEG::CreateStream(const wxFileName &file, bool loop) {
	TFile input;
	TMemoryBuffer buffer;

	if (!TComposedFileManager::Open(input, file))
		return nullptr;

	input.ReadToBuf(buffer, 0);

	const char *bytes = reinterpret_cast<const char *>(buffer.GetData());
	TMixedSoundStream *stream = new TMixedSoundStream(std::vector<char>(bytes, bytes + buffer.GetLen()), loop);

	if (!stream->IsOpen()) {
		delete stream;
		return nullptr;
	}

	return stream;
}
