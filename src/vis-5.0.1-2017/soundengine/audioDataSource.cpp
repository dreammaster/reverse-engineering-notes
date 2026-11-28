#include "soundengine/audioDataSource.h"

#include <opusfile.h>

#include "WxStub.h"

namespace soundengine {

// The OpenAL formats of the multichannel extension (alGetEnumValue("AL_FORMAT_QUAD16") ... in the original).
static const int kFormatMono16 = 0x1101;
static const int kFormatStereo16 = 0x1103;
static const int kFormatQuad16 = 0x1205;
static const int kFormat51Chn16 = 0x120B;
static const int kFormat61Chn16 = 0x120E;
static const int kFormat71Chn16 = 0x1211;

// Confirmed (asm lines 1053920-1054049): the kind of sound file is told from its first bytes: "OggS" (whose first packet starts
// with 'O' at 0x1C: "OpusHead") is an Opus, other "OggS" a Vorbis, "RIFF" a wave, the rest is left to FFmpeg. Only the Opus is
// reconstructed.
AudioDataSource *AudioDataSource::TestOpen(char *data, size_t size) {
	if (size > 0x1C && data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S' && data[0x1C] == 'O') {
		AudioDataSourceOggOpus *source = new AudioDataSourceOggOpus();

		if (!source->Open(data, size)) {
			delete source;
			return nullptr;
		}
		return source;
	}
	return nullptr;
}

// Confirmed (asm lines 1053880-1053888 and 1051234-1051259)
AudioDataSourceOggOpus::~AudioDataSourceOggOpus() {
	if (_file)
		op_free(_file);
	_file = nullptr;
}

// Confirmed (asm lines 1052089-1052279). The file is read from memory; the channels decide the format (1, 2, 4, 6, 7 and 8 are
// supported); the rate is 48000 whatever the file says (libopusfile decodes to it).
bool AudioDataSourceOggOpus::Open(char *data, size_t size) {
	int error = 0;

	_file = op_open_memory(reinterpret_cast<const unsigned char *>(data), size, &error);

	if (error != 0) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Ogg/Opus failed");
		return false;
	}

	_channels = op_channel_count(_file, -1);

	switch (_channels) {
	case 1:
		_format = kFormatMono16;
		break;
	case 2:
		_format = kFormatStereo16;
		break;
	case 4:
		_format = kFormatQuad16;
		break;
	case 6:
		_format = kFormat51Chn16;
		break;
	case 7:
		_format = kFormat61Chn16;
		break;
	case 8:
		_format = kFormat71Chn16;
		break;
	default:
		_format = -1;
		break;
	}

	if (_format == -1) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"File contained more channels than we support (%d)", _channels);
		return false;
	}

	_sampleRate = 48000;
	_bytesPerSample = 2;
	return true;
}

// Confirmed (asm lines 1051958-1052079): reads until the buffer is full or the file is over; the bytes made, or the (negative)
// error of the decoder after it has been logged.
int AudioDataSourceOggOpus::FillBuffer(char *buffer, size_t size) {
	if (!_file || size == 0)
		return 0;

	int filled = 0;

	while (static_cast<size_t>(filled) < size) {
		int samples = op_read(_file, reinterpret_cast<opus_int16 *>(buffer + filled), static_cast<int>((size - filled) / 2), nullptr);

		if (samples < 0) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Ogg/Opus decode error: Error %d ()", samples);
			return samples;
		}
		if (samples == 0)
			break;

		filled += samples * _channels * 2;
	}
	return filled;
}

// Confirmed (asm lines 1051132-1051158): the samples (at 48000 a second) as milliseconds; 1 without a file.
int AudioDataSourceOggOpus::GetDuration() {
	if (!_file)
		return 1;
	return static_cast<int>(op_pcm_total(_file, -1) / 48);
}

// Confirmed (asm lines 1051168-1051193)
int AudioDataSourceOggOpus::GetOffset() {
	if (!_file)
		return 0;
	return static_cast<int>(op_pcm_tell(_file) / 48);
}

// Confirmed (asm lines 1051203-1051224)
int AudioDataSourceOggOpus::Seek(int milliseconds) {
	if (_file)
		op_pcm_seek(_file, static_cast<ogg_int64_t>(milliseconds) * 48);
	return 0;
}

} // End of namespace soundengine
