// Reconstructed from Deponia_Linux.asm (soundengine::AudioDataSource and AudioDataSourceOggOpus, asm 1051132-1052279,
// 1053880-1053920 and 1053920-1054049): what feeds a stream of the sound engine with PCM. The source is made from the bytes of
// a sound file by AudioDataSource::TestOpen(), which looks at the first bytes: "OggS" with an Opus header is an Ogg Opus,
// other "OggS" an Ogg Vorbis, "RIFF" a wave, anything else FFmpeg's. Only the Ogg Opus source is here (all the sounds of
// Deponia 1 are Opus: THIRD_PARTY.md); libopusfile (third_party/opusfile) decodes.
#pragma once

#include <cstddef>

struct OggOpusFile;

namespace soundengine {

class AudioDataSource {
public:
	virtual ~AudioDataSource() {
	}

	/** Opens the file in memory (`data` stays valid as long as the source lives). */
	virtual bool Open(char *data, size_t size) = 0;
	/** Decodes up to `size` bytes of 16 bit samples into `buffer`: the bytes made, or a negative error of the decoder. */
	virtual int FillBuffer(char *buffer, size_t size) = 0;
	/** The length and the position, in milliseconds. */
	virtual int GetDuration() = 0;
	virtual int GetOffset() = 0;
	/** Goes to a position in milliseconds. */
	virtual int Seek(int milliseconds) = 0;

	int GetChannels() const {
		return _channels;
	}
	int GetSampleRate() const {
		return _sampleRate;
	}
	/** The OpenAL format of the samples (AL_FORMAT_MONO16 0x1101, AL_FORMAT_STEREO16 0x1103, the multichannel ones of the
	 *  extension), -1 for a number of channels that is not supported. */
	int GetFormat() const {
		return _format;
	}
	int GetBytesPerSample() const {
		return _bytesPerSample;
	}

	/** The source for the sound file in `data` (an Ogg Opus, or null if the kind is not one that is reconstructed or the file
	 *  can not be opened). The source is the caller's to delete. */
	static AudioDataSource *TestOpen(char *data, size_t size);

protected:
	int _channels = 0;       // +0x14
	int _sampleRate = 0;     // +0x18
	int _format = 0;         // +0x1C
	int _bytesPerSample = 0; // +0x28
};

/** An Ogg Opus file (always 48000 Hz, 16 bit). */
class AudioDataSourceOggOpus : public AudioDataSource {
public:
	AudioDataSourceOggOpus() = default;
	~AudioDataSourceOggOpus() override;

	bool Open(char *data, size_t size) override;
	int FillBuffer(char *buffer, size_t size) override;
	int GetDuration() override;
	int GetOffset() override;
	int Seek(int milliseconds) override;

private:
	OggOpusFile *_file = nullptr;  // +0x30
};

} // End of namespace soundengine
