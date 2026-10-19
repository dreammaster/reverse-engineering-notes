// The sound device of the standalone player (USE_REAL_SDL): SDL's audio callback takes the frames from soundengine::AudioMixer. In the
// original the sources and buffers of OpenAL play the sounds (soundengine::Stream); the mixing is the mixer's and this is what plays it.
#pragma once

#ifdef USE_REAL_SDL

namespace soundengine {

/** Opens the audio device (48000 Hz, 16 bit, two channels) and starts it; the mixer goes by the device from then on. False without a device. */
bool OpenSdlAudio();
void CloseSdlAudio();
bool IsSdlAudioOpen();

} // End of namespace soundengine

#endif // USE_REAL_SDL
