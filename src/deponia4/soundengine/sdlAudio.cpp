#ifdef USE_REAL_SDL

#include "soundengine/sdlAudio.h"

#include "SdlStub.h"
#include "WxStub.h"
#include "soundengine/audioMixer.h"

namespace soundengine {

static SDL_AudioDeviceID s_device = 0;

// (called by the thread of the device: the mixer is locked inside Render())
static void audioCallback(void * /*userData*/, Uint8 *stream, int length) {
	AudioMixer::Get().Render(reinterpret_cast<int16_t *>(stream), length / 4);
}

bool OpenSdlAudio() {
	if (s_device)
		return true;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Unable to start the audio of SDL: %s", wxString(SDL_GetError()).wc_str());

		return false;
	}

	SDL_AudioSpec wanted;
	SDL_AudioSpec obtained;

	SDL_zero(wanted);
	wanted.freq = AudioMixer::kSampleRate;
	wanted.format = AUDIO_S16SYS;
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = audioCallback;

	s_device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);

	if (!s_device) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"No audio device: %s", wxString(SDL_GetError()).wc_str());

		return false;
	}

	AudioMixer::Get().SetDeviceDriven(true);
	SDL_PauseAudioDevice(s_device, 0);
	return true;
}

void CloseSdlAudio() {
	if (!s_device)
		return;

	SDL_CloseAudioDevice(s_device);
	s_device = 0;
	AudioMixer::Get().SetDeviceDriven(false);
}

bool IsSdlAudioOpen() {
	return s_device != 0;
}

} // End of namespace soundengine

#endif // USE_REAL_SDL
