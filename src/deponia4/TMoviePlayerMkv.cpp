// The player of the movies of the game (TMoviePlayer): Matroska files with a VP8 video (libvpx, third_party/libvpx), a Vorbis track for
// each language and subtitles as ASS text for each language. It does what the ffplay of the original does with the part of it that
// is for these files: the clock, the decoding of the pictures that are due, the picture drawn in the middle of the window with
// black round it, the subtitles of the language the game asks for in the font of the video subtitles (the position the game sets),
// a click or a key that ends a movie that can be skipped, the pause, the seek, the loop, and a picture that a script draws itself.
// What the events of the mouse, touch and controller are for is the game's (the event function).
// The sound is the Vorbis track of the language the game asks for (libvorbis, third_party/vorbis): its packets are decoded a little
// ahead of the clock and mixed (resampled to 48000 Hz) by soundengine::AudioMixer with the volume of the movies; the clock is the
// one of the system, the mixer goes by it too. The subtitles are drawn as plain text (the styles and the positions of the ASS lines
// are not used).
#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <vector>

#include "SdlStub.h"
#include "TMovie.h"
#include "TSpriteHandle.h"
#include "baselib/matroska.h"
#include "datastruct/visionaireobject.h"
#include "graphicslib/graphics.h"
#include "vscommon/cfont.h"
#include "vscommon/fontManager.h"

#include "soundengine/audioMixer.h"

#include "vpx/vp8dx.h"
#include "vpx/vpx_decoder.h"

#include <vorbis/codec.h>

namespace {

/** The English names that ISO 639-2 gives the languages the game has (the game names the language of the subtitles and the audio
 *  of a movie by the name, "German", "Greek, Modern (1453-)", "Mongo" ...; the tracks of the file have the three letter code). The
 *  ones after "Esperanto" are those of the language of the Droggelbecher joke (the fictional languages, taken from the real ones). */
const struct {
	const char *code;
	const char *name;
} kLanguageNames[] = {
	{"ara", "Arabic"}, {"bos", "Bosnian"}, {"bul", "Bulgarian"}, {"chi", "Chinese"}, {"ady", "Adyghe; Adygei"}, {"dan", "Danish"},
	{"ger", "German"}, {"eng", "English"}, {"est", "Estonian"}, {"fin", "Finnish"}, {"fre", "French"}, {"geo", "Georgian"},
	{"gre", "Greek, Modern (1453-)"}, {"heb", "Hebrew"}, {"ind", "Indonesian"}, {"ice", "Icelandic"}, {"ita", "Italian"},
	{"gle", "Irish"}, {"jpn", "Japanese"}, {"hrv", "Croatian"}, {"kor", "Korean"}, {"lav", "Latvian"}, {"lit", "Lithuanian"},
	{"mlt", "Maltese"}, {"may", "Malay"}, {"dut", "Dutch"}, {"nor", "Norwegian"}, {"pol", "Polish"}, {"por", "Portuguese"},
	{"rum", "Romanian"}, {"rus", "Russian"}, {"gla", "Gaelic"}, {"swe", "Swedish"}, {"srp", "Serbian"}, {"slo", "Slovak"},
	{"slv", "Slovenian"}, {"spa", "Spanish"}, {"tha", "Thai"}, {"ukr", "Ukrainian"}, {"cze", "Czech"}, {"tur", "Turkish"},
	{"hun", "Hungarian"}, {"epo", "Esperanto"}, {"art", "Artificial languages"}, {"lol", "Mongo"}, {"gon", "Gondi"},
	{"afh", "Afrihili"}, {"ach", "Acoli"}, {"mus", "Creek"}, {"ain", "Ainu"}, {"bre", "Breton"}, {"bik", "Bikol"},
	{"ale", "Aleut"}, {"arw", "Arawak"}, {"asm", "Assamese"}, {"ave", "Avestan"}, {"bam", "Bambara"}, {"bas", "Basa"},
	{"cha", "Chamorro"}, {"cad", "Caddo"}, {"che", "Chechen"}
};

std::string lowerCase(std::string text) {
	for (char &c : text)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

	return text;
}

/** Whether the language of a track (its code) is the one the game asks for (the name of the language, or the code). */
bool isLanguage(const std::string &code, const std::string &wanted) {
	std::string want = lowerCase(wanted);

	if (code == want)
		return true;

	for (const auto &entry : kLanguageNames) {
		if (code == entry.code && lowerCase(entry.name) == want)
			return true;
	}

	return false;
}

/** A line of subtitles and the time it is shown. */
struct SubtitleCue {
	long long _start;
	long long _end;
	std::string _text;
};

/** The audio track of a movie: the packets of Vorbis are decoded and their samples kept until the mixer takes them (resampled, by
 *  linear interpolation, to the rate of the mixer). */
class TMovieAudio : public soundengine::AudioSource {
public:
	~TMovieAudio() override {
		Close();
	}

	/** The three headers are in the codec private data of the track, with the lacing of Xiph: the number of packets less one, the
	 *  sizes of all but the last, then the packets. */
	bool Open(const TMatroskaFile::Track &track) {
		Close();

		const std::vector<uint8_t> &data = track._codecPrivate;

		if (data.empty() || data[0] != 2)
			return false;

		size_t at = 1;
		size_t sizes[3] = {0, 0, 0};

		for (int i = 0; i < 2; i++) {
			uint8_t part;

			do {
				if (at >= data.size())
					return false;

				part = data[at++];
				sizes[i] += part;
			} while (part == 255);
		}

		if (at + sizes[0] + sizes[1] > data.size())
			return false;

		sizes[2] = data.size() - at - sizes[0] - sizes[1];
		vorbis_info_init(&_info);
		vorbis_comment_init(&_comment);
		_started = true;

		for (int i = 0; i < 3; i++) {
			ogg_packet packet;

			std::memset(&packet, 0, sizeof(packet));
			packet.packet = const_cast<unsigned char *>(&data[at]);
			packet.bytes = static_cast<long>(sizes[i]);
			packet.b_o_s = (i == 0);
			packet.packetno = i;

			if (vorbis_synthesis_headerin(&_info, &_comment, &packet) != 0) {
				Close();
				return false;
			}

			at += sizes[i];
		}

		if (vorbis_synthesis_init(&_dsp, &_info) != 0) {
			Close();
			return false;
		}

		vorbis_block_init(&_dsp, &_block);
		_ready = true;
		_step = static_cast<double>(_info.rate) / soundengine::AudioMixer::kSampleRate;
		soundengine::AudioMixer::Get().AddSource(this);
		return true;
	}

	void Close() {
		if (_ready)
			soundengine::AudioMixer::Get().RemoveSource(this);

		if (_ready) {
			vorbis_block_clear(&_block);
			vorbis_dsp_clear(&_dsp);
		}

		if (_started) {
			vorbis_comment_clear(&_comment);
			vorbis_info_clear(&_info);
		}

		_ready = false;
		_started = false;
		_queue.clear();
		_read = 0;
		_phase = 0.0;
	}

	/** Decodes one packet of the track and keeps the samples. */
	void Decode(const std::vector<uint8_t> &data) {
		if (!_ready)
			return;

		auto lock = soundengine::AudioMixer::Get().Lock();
		ogg_packet packet;

		std::memset(&packet, 0, sizeof(packet));
		packet.packet = const_cast<unsigned char *>(data.data());
		packet.bytes = static_cast<long>(data.size());
		packet.granulepos = -1;
		packet.packetno = _packet++;

		if (vorbis_synthesis(&_block, &packet) != 0 || vorbis_synthesis_blockin(&_dsp, &_block) != 0)
			return;

		float **pcm;
		int samples;

		while ((samples = vorbis_synthesis_pcmout(&_dsp, &pcm)) > 0) {
			for (int i = 0; i < samples; i++) {
				_queue.push_back(pcm[0][i]);
				_queue.push_back(_info.channels > 1 ? pcm[1][i] : pcm[0][i]);
			}

			vorbis_synthesis_read(&_dsp, samples);
		}
	}

	/** What is decoded and not yet played is dropped (a seek). */
	void Flush() {
		if (!_ready)
			return;

		auto lock = soundengine::AudioMixer::Get().Lock();

		vorbis_synthesis_restart(&_dsp);
		_queue.clear();
		_read = 0;
		_phase = 0.0;
	}

	void SetPaused(bool paused) {
		_paused = paused;
	}

	void SetGain(float gain) {
		_gain = gain;
	}

	void Mix(float *out, int frames) override {
		if (_paused || !_ready)
			return;

		for (int i = 0; i < frames; i++) {
			size_t index = _read + static_cast<size_t>(_phase);

			// (the sample after this one is needed to go between the two)
			if ((index + 1) * 2 + 1 >= _queue.size())
				break;

			float fraction = static_cast<float>(_phase - static_cast<double>(static_cast<size_t>(_phase)));
			const float *a = &_queue[index * 2];
			const float *b = a + 2;

			out[i * 2] += (a[0] + (b[0] - a[0]) * fraction) * _gain;
			out[i * 2 + 1] += (a[1] + (b[1] - a[1]) * fraction) * _gain;
			_phase += _step;
		}

		size_t whole = static_cast<size_t>(_phase);

		_read += whole;
		_phase -= static_cast<double>(whole);

		// (what was played is let go now and then)
		if (_read > 48000) {
			_queue.erase(_queue.begin(), _queue.begin() + static_cast<ptrdiff_t>(_read * 2));
			_read = 0;
		}
	}

private:
	vorbis_info _info;
	vorbis_comment _comment;
	vorbis_dsp_state _dsp;
	vorbis_block _block;
	bool _started = false;
	bool _ready = false;
	bool _paused = false;
	float _gain = 1.0f;
	double _step = 1.0;
	double _phase = 0.0;
	long long _packet = 3;
	std::vector<float> _queue;   // frames of two floats at the rate of the track
	size_t _read = 0;            // the frame of the queue that is played now
};

class TMatroskaMoviePlayer : public TMoviePlayer {
public:
	~TMatroskaMoviePlayer() override {
		Close();
	}

	bool Open(const wxString &path, const TMovieSettings &settings) override;
	bool Frame(const std::function<void(void *)> &eventFunction) override;
	void Close() override;

	void Pause() override {
		if (_paused)
			return;

		_position = currentTime();
		_paused = true;
		_audio.SetPaused(true);
	}

	void Resume() override {
		if (!_paused)
			return;

		_started = wxGetLocalTimeMillis();
		_paused = false;
		_audio.SetPaused(false);
	}

	void Seek(float seconds) override;

	float GetTime() override {
		return static_cast<float>(currentTime()) / 1000.0f;
	}

	float GetDuration() override {
		return static_cast<float>(_file.GetDurationMs()) / 1000.0f;
	}

	float GetCompletition() override {
		long long length = _file.GetDurationMs();

		return length > 0 ? static_cast<float>(currentTime()) * 1000.0f / static_cast<float>(length) : 0.0f;
	}

	int GetWidth() override {
		return _width;
	}

	int GetHeight() override {
		return _height;
	}

	void SetBlend(int blend) override {
		_blend = blend;
	}

	void SetColour(const wxColour &colour) override {
		_colour = colour;
	}

	void SetLoop(bool loop) override {
		_loop = loop;
	}

	bool Draw(float x, float y, float width, float height) override;

private:
	long long currentTime() const {
		return _paused ? _position : _position + (wxGetLocalTimeMillis() - _started);
	}

	bool startDecoder();
	void stopDecoder();
	/** Decodes the audio that is due in the next half second. */
	void decodeAudio();
	/** Decodes the pictures that are due; false when the movie is over. */
	bool advance();
	void convertPicture(const vpx_image_t *image);
	void releasePicture();
	void drawPicture(float x, float y, float width, float height);
	void drawSubtitles(long long time);
	void parseSubtitles(const TMatroskaFile::Track &track);
	unsigned int colourValue() const {
		return (static_cast<unsigned int>(_colour.alpha) << 24) | (static_cast<unsigned int>(_colour.blue) << 16) |
		       (static_cast<unsigned int>(_colour.green) << 8) | static_cast<unsigned int>(_colour.red);
	}

	TMatroskaFile _file;
	TMovieSettings _settings;
	bool _open = false;
	bool _decoderStarted = false;
	vpx_codec_ctx_t _codec;
	std::vector<size_t> _videoBlocks;      // the indexes (of the blocks of the file) of the pictures
	size_t _nextVideo = 0;
	std::vector<SubtitleCue> _cues;
	int _width = 0;
	int _height = 0;
	long long _frameMs = 40;               // the time one picture is shown, for the last one
	TSpriteHandle *_picture = nullptr;     // the picture that is shown now
	bool _hasPicture = false;
	bool _paused = false;
	bool _loop = false;
	bool _over = false;
	int _blend = 0;
	wxColour _colour = wxColour(255, 255, 255, 255);
	long long _position = 0;               // the time of the movie when the clock was set (milliseconds)
	long long _started = 0;                // the time of the system then
	std::vector<uint8_t> _data;
	TMovieAudio _audio;
	bool _hasAudio = false;
	std::vector<size_t> _audioBlocks;      // the indexes of the packets of the audio track
	size_t _nextAudio = 0;
};

bool TMatroskaMoviePlayer::Open(const wxString &path, const TMovieSettings &settings) {
	Close();
	_settings = settings;

	if (!_file.Open(std::string(static_cast<const char *>(path.mb_str()))))
		return false;

	const TMatroskaFile::Track *video = _file.FindTrack(TMatroskaFile::kVideo);

	if (!video || video->_codecId != "V_VP8")
		return false;

	_width = video->_width;
	_height = video->_height;

	if (video->_defaultDurationNs > 0)
		_frameMs = std::max<long long>(1, video->_defaultDurationNs / 1000000);

	_videoBlocks.clear();

	const std::vector<TMatroskaFile::Block> &blocks = _file.GetBlocks();

	for (size_t i = 0; i < blocks.size(); i++) {
		if (blocks[i]._track == video->_number)
			_videoBlocks.push_back(i);
	}

	if (_videoBlocks.empty())
		return false;

	// the subtitles of the language of the game (none: no subtitles)
	_cues.clear();

	if (!settings.subtitleLanguage.IsEmpty()) {
		std::string language(static_cast<const char *>(settings.subtitleLanguage.mb_str()));

		for (const TMatroskaFile::Track &track : _file.GetTracks()) {
			if (track._type == TMatroskaFile::kSubtitle && isLanguage(track._language, language)) {
				parseSubtitles(track);
				break;
			}
		}
	}

	// the audio: the track of the language of the game (the first one when there is none like it)
	_hasAudio = false;
	_audioBlocks.clear();

	const TMatroskaFile::Track *audio = nullptr;
	std::string audioLanguage(static_cast<const char *>(settings.audioLanguage.mb_str()));

	for (const TMatroskaFile::Track &track : _file.GetTracks()) {
		if (track._type != TMatroskaFile::kAudio || track._codecId != "A_VORBIS")
			continue;

		if (!audio)
			audio = &track;

		if (!audioLanguage.empty() && isLanguage(track._language, audioLanguage)) {
			audio = &track;
			break;
		}
	}

	if (audio && _audio.Open(*audio)) {
		_hasAudio = true;
		_audio.SetGain(static_cast<float>(settings.volume) / 100.0f);

		for (size_t i = 0; i < blocks.size(); i++) {
			if (blocks[i]._track == audio->_number)
				_audioBlocks.push_back(i);
		}
	}

	if (!startDecoder())
		return false;

	_nextAudio = 0;
	_position = 0;
	_started = wxGetLocalTimeMillis();
	_paused = false;
	_over = false;
	_hasPicture = false;
	_nextVideo = 0;
	_open = true;
	return true;
}

void TMatroskaMoviePlayer::Close() {
	_audio.Close();
	_hasAudio = false;
	_audioBlocks.clear();
	stopDecoder();
	releasePicture();
	_file.Close();
	_videoBlocks.clear();
	_cues.clear();
	_open = false;
}

bool TMatroskaMoviePlayer::startDecoder() {
	stopDecoder();
	std::memset(&_codec, 0, sizeof(_codec));

	if (vpx_codec_dec_init(&_codec, vpx_codec_vp8_dx(), nullptr, 0) != VPX_CODEC_OK)
		return false;

	_decoderStarted = true;
	return true;
}

void TMatroskaMoviePlayer::stopDecoder() {
	if (_decoderStarted) {
		vpx_codec_destroy(&_codec);
		_decoderStarted = false;
	}
}

void TMatroskaMoviePlayer::releasePicture() {
	if (_picture) {
		_picture->Release();

		if (_picture->GetRefCount() == 0 && graphics)
			graphics->OnSpriteHandleReleased(_picture);

		_picture = nullptr;
	}
}

/** The text of the lines of an ASS track: the ninth field of the block ("ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV,
 *  Effect, Text"), without the override codes in braces; "\N" is a new line and "\h" a space. */
void TMatroskaMoviePlayer::parseSubtitles(const TMatroskaFile::Track &track) {
	std::vector<uint8_t> data;

	for (const TMatroskaFile::Block &block : _file.GetBlocks()) {
		if (block._track != track._number || !_file.ReadBlock(block, data))
			continue;

		std::string line(data.begin(), data.end());
		size_t at = 0;

		for (int field = 0; field < 8 && at != std::string::npos; field++) {
			at = line.find(',', at);

			if (at != std::string::npos)
				at++;
		}

		if (at == std::string::npos)
			continue;

		std::string raw = line.substr(at);
		std::string clean;

		for (size_t i = 0; i < raw.size(); i++) {
			if (raw[i] == '{') {
				size_t close = raw.find('}', i);

				if (close != std::string::npos) {
					i = close;
					continue;
				}
			}

			if (raw[i] == '\\' && i + 1 < raw.size()) {
				char code = raw[i + 1];

				if (code == 'N' || code == 'n') {
					clean += '\n';
					i++;
					continue;
				}

				if (code == 'h') {
					clean += ' ';
					i++;
					continue;
				}
			}

			clean += raw[i];
		}

		if (clean.empty())
			continue;

		SubtitleCue cue;

		cue._start = block._timeMs;
		cue._end = block._timeMs + block._durationMs;
		cue._text = clean;
		_cues.push_back(cue);
	}
}

/** The colours of a picture of planes (I420, the limited range of BT.601) as RGBA pixels, in a sprite of the backend. */
void TMatroskaMoviePlayer::convertPicture(const vpx_image_t *image) {
	if (!graphics || !image || image->fmt != VPX_IMG_FMT_I420)
		return;

	const int width = static_cast<int>(image->d_w);
	const int height = static_cast<int>(image->d_h);
	std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);

	for (int y = 0; y < height; y++) {
		const uint8_t *lumaRow = image->planes[VPX_PLANE_Y] + static_cast<ptrdiff_t>(y) * image->stride[VPX_PLANE_Y];
		const uint8_t *uRow = image->planes[VPX_PLANE_U] + static_cast<ptrdiff_t>(y / 2) * image->stride[VPX_PLANE_U];
		const uint8_t *vRow = image->planes[VPX_PLANE_V] + static_cast<ptrdiff_t>(y / 2) * image->stride[VPX_PLANE_V];
		uint8_t *out = &rgba[static_cast<size_t>(y) * width * 4];

		for (int x = 0; x < width; x++) {
			int luma = (lumaRow[x] - 16) * 298;
			int u = uRow[x / 2] - 128;
			int v = vRow[x / 2] - 128;
			int red = (luma + 409 * v + 128) >> 8;
			int green = (luma - 100 * u - 208 * v + 128) >> 8;
			int blue = (luma + 516 * u + 128) >> 8;

			out[0] = static_cast<uint8_t>(std::min(255, std::max(0, red)));
			out[1] = static_cast<uint8_t>(std::min(255, std::max(0, green)));
			out[2] = static_cast<uint8_t>(std::min(255, std::max(0, blue)));
			out[3] = 255;
			out += 4;
		}
	}

	TSpriteHandle *sprite = nullptr;

	if (graphics->CreateSprite(&sprite, reinterpret_cast<const char *>(rgba.data()), width, height, 4, width * 4, false)) {
		releasePicture();
		_picture = sprite;
		_hasPicture = true;
	}
}

void TMatroskaMoviePlayer::decodeAudio() {
	if (!_hasAudio)
		return;

	const std::vector<TMatroskaFile::Block> &blocks = _file.GetBlocks();
	long long until = currentTime() + 500;

	while (_nextAudio < _audioBlocks.size() && blocks[_audioBlocks[_nextAudio]]._timeMs <= until) {
		if (_file.ReadBlock(blocks[_audioBlocks[_nextAudio]], _data))
			_audio.Decode(_data);

		_nextAudio++;
	}
}

bool TMatroskaMoviePlayer::advance() {
	const std::vector<TMatroskaFile::Block> &blocks = _file.GetBlocks();
	long long time = currentTime();

	decodeAudio();

	while (_nextVideo < _videoBlocks.size()) {
		const TMatroskaFile::Block &block = blocks[_videoBlocks[_nextVideo]];

		if (block._timeMs > time)
			break;

		_nextVideo++;

		if (!_file.ReadBlock(block, _data))
			continue;

		if (vpx_codec_decode(&_codec, _data.data(), static_cast<unsigned int>(_data.size()), nullptr, 0) != VPX_CODEC_OK)
			continue;

		// (the picture is only made for the last one that is due: the others are only decoded)
		bool last = _nextVideo >= _videoBlocks.size() || blocks[_videoBlocks[_nextVideo]]._timeMs > time;

		if (!last)
			continue;

		vpx_codec_iter_t iterator = nullptr;
		const vpx_image_t *image = vpx_codec_get_frame(&_codec, &iterator);

		if (image)
			convertPicture(image);
	}

	if (_nextVideo >= _videoBlocks.size()) {
		long long end = blocks[_videoBlocks.back()]._timeMs + _frameMs;

		if (time >= end) {
			if (!_loop)
				return false;

			Seek(0.0f);
		}
	}

	return true;
}

void TMatroskaMoviePlayer::Seek(float seconds) {
	if (!_open)
		return;

	const std::vector<TMatroskaFile::Block> &blocks = _file.GetBlocks();
	long long target = std::max<long long>(0, static_cast<long long>(seconds * 1000.0f));

	// the last picture at the time, and the key frame before it: the decoding goes on from there
	size_t index = 0;

	while (index + 1 < _videoBlocks.size() && blocks[_videoBlocks[index + 1]]._timeMs <= target)
		index++;

	while (index > 0 && !blocks[_videoBlocks[index]]._keyFrame)
		index--;

	startDecoder();

	if (_hasAudio) {
		// (the packet before the time too: the windows of the blocks overlap)
		_audio.Flush();
		_nextAudio = 0;

		while (_nextAudio + 1 < _audioBlocks.size() && blocks[_audioBlocks[_nextAudio + 1]]._timeMs < target)
			_nextAudio++;
	}

	_nextVideo = index;
	_position = target;
	_started = wxGetLocalTimeMillis();
	_over = false;
}

bool TMatroskaMoviePlayer::Frame(const std::function<void(void *)> &eventFunction) {
	if (!_open || _over)
		return false;

	SDL_PumpEvents();

	SDL_Event event;

	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, 0, 0xFFFF) > 0) {
		if (event.type == SDL_QUIT || (_settings.skippable && (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONUP))) {
			_over = true;
			return false;
		}

		if (eventFunction)
			eventFunction(&event);
	}

	soundengine::AudioMixer::Get().Pump();

	if (!advance()) {
		_over = true;
		return false;
	}

	// the picture in the middle of the window, as big as it can be with its proportions
	float windowWidth = static_cast<float>(_settings.videoResolution.x > 0 ? _settings.videoResolution.x : graphics->GetWidth());
	float windowHeight = static_cast<float>(_settings.videoResolution.y > 0 ? _settings.videoResolution.y : graphics->GetHeight());

	graphics->BeforeDrawScene(true, 0);

	wxRect screen;

	screen.x = 0;
	screen.y = 0;
	screen.width = static_cast<int>(windowWidth);
	screen.height = static_cast<int>(windowHeight);
	graphics->DrawBox(screen, 0xFF000000u, 1.0f);

	if (_width > 0 && _height > 0) {
		float scale = std::min(windowWidth / static_cast<float>(_width), windowHeight / static_cast<float>(_height));
		float width = static_cast<float>(_width) * scale;
		float height = static_cast<float>(_height) * scale;

		drawPicture((windowWidth - width) / 2.0f, (windowHeight - height) / 2.0f, width, height);
	}

	drawSubtitles(currentTime());
	graphics->AfterDrawScene(true, true);
	graphics->Swap();
	return true;
}

void TMatroskaMoviePlayer::drawPicture(float x, float y, float width, float height) {
	if (!_picture || !_hasPicture)
		return;

	wxRect source;
	FloatRect destination;
	wxPoint center;

	source.x = 0;
	source.y = 0;
	source.width = _width;
	source.height = _height;
	destination.x = x;
	destination.y = y;
	destination.width = width;
	destination.height = height;
	center.x = -1;
	center.y = -1;
	graphics->Draw(_picture, source, destination, 1.0f, false, colourValue(), -1, 0.0f, center, 1.0f, 1.0f, 0);
}

/** The lines that are due, in the font of the subtitles at the position the game gives (the middle of the text, its first line). */
void TMatroskaMoviePlayer::drawSubtitles(long long time) {
	if (_cues.empty() || !_settings.fontManager)
		return;

	TCFont *font = nullptr;

	for (TCFont *candidate : _settings.fontManager->GetFonts()) {
		if (PackVisId(candidate->GetFontObject().GetId()) == _settings.fontId) {
			font = candidate;
			break;
		}
	}

	if (!font)
		return;

	for (const SubtitleCue &cue : _cues) {
		if (time < cue._start || time >= cue._end)
			continue;

		wxString text;

		toUTF(&text, cue._text.c_str());
		wxPoint size;

		font->GetTextDimension(text, size);

		int lines = 1 + static_cast<int>(std::count(cue._text.begin(), cue._text.end(), '\n'));
		wxPoint where = _settings.subtitlePosition;

		// (a text of several lines ends at the position, so that it does not leave the window)
		where.y -= size.y * (lines - 1) / lines;
		font->PrintText(text, TextAlignmentEnum::kCenter, where, 1.0f, -1, nullptr);
	}
}

bool TMatroskaMoviePlayer::Draw(float x, float y, float width, float height) {
	if (!_open || _over)
		return false;

	if (!advance()) {
		_over = true;
		return false;
	}

	if (width <= 0.0f || height <= 0.0f) {
		width = static_cast<float>(_width);
		height = static_cast<float>(_height);
	}

	drawPicture(x, y, width, height);
	return true;
}

} // End of anonymous namespace

/** The player of the movies of the game (what TMovie makes unless the platform has its own). */
TMoviePlayer *CreateMatroskaMoviePlayer() {
	return new TMatroskaMoviePlayer();
}
