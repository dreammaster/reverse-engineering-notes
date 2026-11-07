# The sound engine of Night of the Rabbit (read in `rabbit.asm`, 9 Nov 2026)

Working notes for the audio rework: what this build's sound code does that Deponia 1's does not, and what of it the reconstruction has. The listing is
`build/nrida/rabbit.asm`; addresses are the symbols' names. "D1" is Deponia 1, whose sound code `src/deponia1` (and this tree, seeded from it) has.

## Sizes (instructions, from `build/d4work/sizes.py`)

* `soundengine::` (the backend, OpenAL): D1 150 functions / 11,292 instructions, this build 171 / 22,069 (inlined `rapidjson` and `std::` templates are a
  good part of both). The D1 `soundengine::Stream` (SDL) is gone; the backend is **OpenAL with EFX effects**: `priv::alCheckError`, `priv::AudioDevice`,
  `AudioBufferFactory` (`LoadBuffer(std::string const &)`, `Update`), `AudioEffectFactory` (`SlotForEffect`, `FreeSlot`, `EffectFromObject`, `Update`),
  `AudioEffect` (`Load`, `Update`, `Serialize`), `AudioMixer` (`MixerFromJSON`, `MixerFromObject(TVisObjRef)`, `Load`, `Update`, `Serialize`),
  `AudioBusBridge` (`Play Stop Pause Seek FadeIn(int,int) FadeOut(int,int) SetVolume SetPosition SetLoop IsPlaying IsLooping GetOffset GetDuration
  HasFinished ReProcess`), `AudioSourceBus` (`Process`, `ProcessInternalNoConvert`, `Seek`), `SyncBus`, `SwitchBus` (new: `Load Seek Clone Update EmitBus
  Process Serialize`), `AudioSend`.
* The tables `AudioBusInit()` fills (415 instructions) are maps from one enum to another (node types of the bus graph), not volumes.
* **The data of all five games has no audio bus record** (`PLAYER.md`); the three default busses `eMusicVolume`, `eSoundVolume`, `eSpeechVolume` are made at
  the end of `TVisionaireGame::UpdateVersion` (not done yet), so the graph (switch / sync / random buses, effects) is not used by these games' data; the
  engine of this tree plays through SDL and has no graph.

## The volume model of the sound items (TSoundFFMPEG, TFFMPEGSoundItem; read in Play, SetStats, Update, AdjustVolume, FadeSound, FadeAll)

* **Every volume that the engine is given is in decibels**: `Play(file, bus, volume, balance, loop, ...)`, `SetStats(item, volume, balance, ...)`. `SetStats` clamps
  the volume to -100..0 dB and the balance to -100..100. The data's `ActionPartInt` of the commands 24, 59, 72, 77, 166, the scene music volume and the
  animation frame sound volume are in dB since the version 190 step, and they are passed on as they are.
* The item (`TFFMPEGSoundItem`, 0x68 bytes) has, in the base `TSoundItem`: `+0x08` name, `+0x10` id, `+0x14` the fade's elapsed time (ms, int), `+0x18` its
  duration (ms), `+0x1C` the factor it starts at, `+0x20` the factor it ends at (floats), `+0x24` paused, `+0x25` stopped, `+0x26` streamed, `+0x27` dead,
  `+0x28` fading, `+0x29` keep (the end factor becomes the volume); in the sound item: `+0x30` the backend source (`AudioBusBridge`), `+0x38` muted,
  `+0x3C` the volume in dB (float), `+0x40` the volume in percent = `100 * 10^(dB / 20)`, `+0x44` the balance / 100, `+0x48` the fade's present factor
  (1.0), `+0x4C` the factor of its bus / kind (from its mixer), `+0x50` the mixer, `+0x58` the offset.
* The gain given to the backend is always `percent * 0.01 * busFactor(+0x4C) * fadeFactor(+0x48)`. The general volumes of the kinds (music, sound, speech) are no
  longer folded into the item (D1's `AdjustToGeneralVolume`): `TSoundFFMPEG::Update` gives each of the default mixers `logf(volume / 100) * 10`
  (`dword_E1A554` is 10.0, not 8.686: natural log times 10; `+0x34` music, `+0x30` sound, `+0x38` speech of the engine), and the items read their mixer's
  factor (`+0x50` of the mixer) each frame. The mixers' own code (`AudioMixer::Update`, 194 instructions) was not read; the factor that results is taken here as
  `volume / 100`. `AdjustVolume(music, sound, speech, movie,
  global)` keeps only the movie and global ones (`movie * base / 100`, the listener's gain `alListenerf(AL_GAIN, global * 0.01)`).
* **The fade of an item** (`TSoundFFMPEG::Update`, per frame with the time since the last one): `elapsed += dt`; before the end the factor is
  `from + (to - from) * elapsed / duration`; at the end it is `to`, and with `keep` the item's percent is multiplied by `to` and the factor goes back to 1.0;
  `fading` is cleared and an item whose end factor is at most a small number (`dword_EC0750` = 0.0001) is stopped and let go.
  `TSoundFFMPEG::FadeSound(id, ms, dB, keep)`: for the items with that id: fading, elapsed 0, duration `ms`, `keep` flag; the target in percent is
  `100 * 10^(dB / 20)`, the present volume is `+0x40` (the integer part); with `keep`: if the target is above the present volume the item's percent becomes
  the target and the factor goes from present/target to 1.0, else the percent stays and the factor goes from 1.0 to target/present; without `keep` the factor
  goes from present/100 to target/100.
  `TSoundBase::FadeAll(ms, percent)`: the same for every item that is not fading and has a volume: from its present volume / 100 to `percent / 100`.
* In D1 the fades were a global state of `TSoundBase` (`StartSoundFade`, `UpdateSoundFade`, `FinishSoundFade`, `FinishFadeIn`, `StartFadeInSounds`,
  `StopFadeOutSounds`, `FadeOut`, `Keep`, `GetFadeTypeAsString`, `TFadeEnum` on the items); this build has none of them.
* `TSoundInterface::Play(file, bus, volume, balance, loop, type, fadeMs, offset)` for the music (type 0): when the file is not the music that is playing, the
  music that is (id `+0x20`) is faded out with `FadeSound(id, fadeMs, -100, false)` and the new one is played; D1's `KeepCurrentBackgroundMusic*` are gone.

## What this tree has of it (9 Nov 2026)

Done (checked with `tests/deponia1/soundfadetest.cpp`, a stream that only records what it is told, and `rabbitcmdtest.cpp`):

* the volume model: decibels at every call (`Play`, `SetStats`, `FadeSound`), the percent, the factor of the bus (the general volume of the sound's kind / 100), the
  factor of the fade and the global volume as the gain of a sound; `TSoundFFMPEG::PlaySound`, `SetStats`, `AdjustVolume`, `Update`, `FadeSound`, `TSoundBase::FadeAll`;
* `TSoundInterface::Play` with the audio bus (not used: the bus of a sound is the one of its kind) and the fade in, the music rules (same file: -1; another: the old fades out
  by `FadeSound(id, ms, -100, false)`, the new plays looped), `StopBackgroundMusic` (a second of fade), `SetVolume` as it was, `Signal` (the speech message plays without a
  bus, the fade message runs `Update()`);
* the callers: the sound commands (24, 72: the part's link is the audio bus; 24 with the third integer 1 waits for the sound to end, its id is kept in `ActionSavedValue`),
  the scene music command (59: sets the music with no event and plays it at once, with the third integer as the fade, when the scene is the one that is shown), 77 (decibels as
  they are), **166** (`FadeSound(id, ms, dB, true)`, or `SetStats` at once when skipping), the scene change (`FadeAll(time, 0)` instead of `StartSoundFade`; the music's fade
  is the `Play`'s: half of the time of the effect for 3 and 7), `THScene` (3 seconds), the frame sounds, the walking sound, the Lua `startSound` (`20 * log10(percent / 100)`);
* the three default mixes `eMusicVolume`, `eSoundVolume`, `eSpeechVolume` made after the upgrade steps (`createDefaultMixes`).
* removed, because this build has none of it: `StartSoundFade`, `UpdateSoundFade`, `FinishSoundFade`, `FinishFadeIn`, `StartFadeInSounds`, `StopFadeOutSounds`, `FadeOut`, `Keep`,
  `KeepCurrentBackgroundMusic*`, `AdjustToGeneralVolume`, `GetFadeTypeAsString`, the `TFadeEnum` of the items, and the call of `UpdateSoundFade(false)` in `TGameControl::Update`.

Not done:

* the mixes themselves: `AudioMixer` (`MixerFromObject`, `MixerFromJSON`, `Load`, `Update`, `Serialize`), `TSoundFFMPEG::LoadMixers` / `FindMixer` / `FindRMixer` (a mix of the game's
  `GameAudioMixLinks` that a bus names gives its factor), `BusActivate` / `BusValuesUpdate` and the events and values of a bus (`AudioBusValues`, `AudioBusEvents`); none of the games' data
  has a bus record, so the general volumes are all that sets a bus;
* the effects (`AudioEffect`, `AudioEffectFactory`: reverb and the like on OpenAL EFX), `SwitchBus`, `SyncBus` and the buses of the graph, the OpenAL backend itself
  (`AudioBufferFactory`, `AudioBusBridge`); this tree plays through SDL;
* `TSoundBase::PrintSounds` (446 instructions here, 610 in Deponia 1: the lines were not read; the fade state is printed as `Fading`);
* `TLoadingControl::Init` plays the loading sound with `Play(path, no bus, 100, 0, loop, music, 0, 0)` (a stub in this tree, as it was).
