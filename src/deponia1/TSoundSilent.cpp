// The stream of a sound for a sound engine without a sound device (TSoundFFMPEG::CreateStream, what a backend replaces): the
// file is opened and its length read as the engine does, but nothing is played; the stream goes by the clock instead, so
// that the sounds have an offset, an end and a duration. That is what the actions that wait for a sound or a speech, and the
// texts that stay as long as the voice, need.
#include <algorithm>
#include <vector>

#include "TComposedFileManager.h"
#include "TMemoryBuffer.h"
#include "TSoundFFMPEG.h"
#include "baselib/file.h"
#include "soundengine/audioDataSource.h"

namespace {

class TSilentSoundStream : public TSoundStream {
public:
	TSilentSoundStream(std::vector<char> &&data, bool loop) : _data(std::move(data)), _loop(loop) {
		_source = soundengine::AudioDataSource::TestOpen(_data.data(), _data.size());

		if (_source)
			_duration = _source->GetDuration();

		// (a new sound begins by itself)
		Play();
	}

	~TSilentSoundStream() override {
		delete _source;
	}

	bool IsOpen() const {
		return _source != nullptr;
	}

	void Play() override {
		if (_playing)
			return;

		_started = wxGetLocalTimeMillis();
		_playing = true;
	}

	void Pause() override {
		_offset = GetOffset();
		_playing = false;
	}

	void Stop() override {
		_offset = 0;
		_playing = false;
	}

	bool IsPlaying() const override {
		return _playing && !HasEnded();
	}

	bool HasEnded() const override {
		return !_loop && _playing && _duration > 0 && position() >= _duration;
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
		int at = position();

		if (_duration > 0)
			at = _loop ? at % _duration : std::min(at, _duration);

		return at;
	}

	void SetOffset(int milliseconds) override {
		_offset = milliseconds;
		_started = wxGetLocalTimeMillis();
	}

	int GetDuration() const override {
		return _duration;
	}

	float GetSampleAverage() const override {
		return 0.0f;
	}

private:
	/** Where the sound is, before it is kept to its length. */
	int position() const {
		return _playing ? _offset + static_cast<int>(wxGetLocalTimeMillis() - _started) : _offset;
	}

	std::vector<char> _data;
	soundengine::AudioDataSource *_source = nullptr;
	bool _loop;
	bool _playing = false;
	int _duration = 0;
	int _offset = 0;
	long long _started = 0;
	float _gain = 1.0f;
	float _pan = 0.0f;
};

} // End of anonymous namespace

TSoundStream *TSoundFFMPEG::CreateStream(const wxFileName &file, bool loop) {
	TFile input;
	TMemoryBuffer buffer;

	if (!TComposedFileManager::Open(input, file))
		return nullptr;

	input.ReadToBuf(buffer, 0);

	const char *bytes = reinterpret_cast<const char *>(buffer.GetData());
	TSilentSoundStream *stream = new TSilentSoundStream(std::vector<char>(bytes, bytes + buffer.GetLen()), loop);

	if (!stream->IsOpen()) {
		delete stream;
		return nullptr;
	}

	return stream;
}
