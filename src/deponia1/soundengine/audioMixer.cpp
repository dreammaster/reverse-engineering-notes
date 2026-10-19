#include "soundengine/audioMixer.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "WxStub.h"

namespace soundengine {

AudioMixer &AudioMixer::Get() {
	static AudioMixer mixer;

	return mixer;
}

void AudioMixer::AddSource(AudioSource *source) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	if (std::find(_sources.begin(), _sources.end(), source) == _sources.end())
		_sources.push_back(source);
}

void AudioMixer::RemoveSource(AudioSource *source) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_sources.erase(std::remove(_sources.begin(), _sources.end(), source), _sources.end());
}

void AudioMixer::Render(int16_t *out, int frames) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_mix.assign(static_cast<size_t>(frames) * 2, 0.0f);

	// (a copy of the list: a source can end and be taken out while it is mixed)
	std::vector<AudioSource *> sources = _sources;

	for (AudioSource *source : sources)
		source->Mix(_mix.data(), frames);

	for (size_t i = 0; i < _mix.size(); i++) {
		float value = std::max(-1.0f, std::min(1.0f, _mix[i]));

		out[i] = static_cast<int16_t>(value * 32767.0f);
	}

	_rendered += frames;
}

void AudioMixer::Pump() {
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	long long now = wxGetLocalTimeMillis();

	if (_clockStart < 0) {
		_clockStart = now;
		return;
	}

	long long due = (now - _clockStart) * kSampleRate / 1000 - _rendered;

	if (due <= 0)
		return;

	// (a long pause - a loading screen, a breakpoint - is not made up for)
	if (due > kSampleRate / 4) {
		_clockStart += (due - kSampleRate / 4) * 1000 / kSampleRate;
		due = kSampleRate / 4;
	}

	_out.resize(static_cast<size_t>(due) * 2);
	Render(_out.data(), static_cast<int>(due));

	if (_sink)
		_sink(_out.data(), static_cast<int>(due));
}

void AudioMixer::SetSink(const std::function<void(const int16_t *, int)> &sink) {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	_sink = sink;
}

long long AudioMixer::GetRenderedFrames() const {
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	return _rendered;
}

WavWriter::~WavWriter() {
	Close();
}

static void put32(unsigned char *at, uint32_t value) {
	for (int i = 0; i < 4; i++)
		at[i] = static_cast<unsigned char>(value >> (8 * i));
}

static void put16(unsigned char *at, uint16_t value) {
	at[0] = static_cast<unsigned char>(value);
	at[1] = static_cast<unsigned char>(value >> 8);
}

bool WavWriter::Open(const char *path, int sampleRate, int channels) {
	Close();

	FILE *file = std::fopen(path, "wb");

	if (!file)
		return false;

	_file = file;
	_channels = channels;
	_sampleRate = sampleRate;
	_frames = 0;

	unsigned char header[44] = {};

	std::memcpy(header, "RIFF", 4);
	std::memcpy(header + 8, "WAVEfmt ", 8);
	put32(header + 16, 16);
	put16(header + 20, 1);
	put16(header + 22, static_cast<uint16_t>(channels));
	put32(header + 24, static_cast<uint32_t>(sampleRate));
	put32(header + 28, static_cast<uint32_t>(sampleRate * channels * 2));
	put16(header + 32, static_cast<uint16_t>(channels * 2));
	put16(header + 34, 16);
	std::memcpy(header + 36, "data", 4);
	std::fwrite(header, 1, sizeof(header), file);
	return true;
}

void WavWriter::Write(const int16_t *samples, int frames) {
	if (!_file)
		return;

	// (the samples of a WAV file are little endian)
	std::vector<unsigned char> bytes(static_cast<size_t>(frames) * _channels * 2);

	for (size_t i = 0; i < bytes.size() / 2; i++)
		put16(&bytes[i * 2], static_cast<uint16_t>(samples[i]));

	std::fwrite(bytes.data(), 1, bytes.size(), static_cast<FILE *>(_file));
	_frames += frames;
}

void WavWriter::Close() {
	if (!_file)
		return;

	FILE *file = static_cast<FILE *>(_file);
	unsigned char size[4];

	put32(size, static_cast<uint32_t>(_frames * _channels * 2));
	std::fseek(file, 40, SEEK_SET);
	std::fwrite(size, 1, 4, file);
	put32(size, static_cast<uint32_t>(36 + _frames * _channels * 2));
	std::fseek(file, 4, SEEK_SET);
	std::fwrite(size, 1, 4, file);
	std::fclose(file);
	_file = nullptr;
}

} // End of namespace soundengine
