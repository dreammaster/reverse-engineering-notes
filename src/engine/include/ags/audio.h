/* ags/audio.h -- M10 ("It has a voice", see src/PLAN.md): real
 * PlayMusic/PlaySound/PlaySpeech wiring, built by reading matches.json's
 * own already-extensive documentation for this subsystem (PlayMusic/
 * scr_StopMusic/IsMusicPlaying/GetMIDIPosition/SeekMIDIPosition/
 * SetMusicVolume/SetMusicRepeat/GetCurrentMusic/PlaySound/PlaySpeech
 * were all already matched with real field evidence before this
 * milestone -- this module just gives their already-documented real
 * behavior an actual C implementation).
 *
 * SCOPE, narrowed by this specific game's own real data: Rob Blanc
 * 1's CLIB manifest (M1) contains exactly two music files (Music1.MID,
 * music2.mid) and NO sound-effect or speech files of any kind (no
 * .wav/.voc/.mp3/.mod anywhere). This directly shapes what's worth
 * building real vs. stubbed for THIS milestone:
 *   - PlayMusic: MIDI only. This build's own real PlayMusic
 *     (matches.json's own entry, retroactive documentation) tries
 *     MIDI first via bare Allegro load_midi/play_midi calls on plain
 *     globals (CONFIRMED no MYMIDI wrapper object exists in this
 *     build at all -- see AmbientSound's own earlier-established
 *     "bare globals, no wrapper" pattern, same here). The MOD/MP3
 *     fallback cascade PlayMusic falls through to on a MIDI miss is
 *     NOT ported -- genuinely dead code for this specific game (no
 *     such files exist to ever reach it), the same "confirmed dead
 *     for this game" standard M5/M8 already applied elsewhere.
 *   - PlaySound: the real "sound%d.mp3/.wav/.voc" cascade IS ported
 *     (cheap, and testable: this game has none of the three, so the
 *     real, correct behavior to verify is "gracefully finds nothing
 *     and does nothing" -- which is exactly what happens against real
 *     data). Only the .wav/.voc legs actually load+play (Allegro's
 *     own load_wav/load_voc/play_sample, already linked); an .mp3
 *     hit falls through to AGS_STUB_VOID() rather than wiring ALMP3's
 *     own streaming-sample API for a code path this game's own data
 *     can never exercise.
 *   - PlaySpeech: the ORIGINAL engine's own body, in BOTH the 2011
 *     reference and this exact build, is a bare
 *     `quit("PlaySpeech not yet implemented")` -- matches.json's own
 *     entry calls this "a complete, exact, zero-drift match... a
 *     stub in BOTH builds, nine years apart." This port keeps that
 *     honestly: AGS_STUB_VOID() instead of aborting the whole
 *     process, a deliberate safety improvement over the original's
 *     own quit()-on-call behavior, not a faithfulness gap -- there is
 *     no "real" PlaySpeech to port, the original game never had one
 *     either.
 */
#ifndef AGS_AUDIO_H
#define AGS_AUDIO_H

#include "ags/gamestate.h"
#include "ags/room.h"

enum AgsMusicError {
    AGS_MUSIC_OK = 0,
    AGS_MUSIC_NOT_FOUND = -1,  /* no music<N>.mid exists for this number */
    AGS_MUSIC_PLAY_FAILED = -2 /* load_midi succeeded but play_midi failed -- source's own quit() case, not aborted here */
};

/* install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) -- call once
 * after allegro_init(). Returns 0 on success, matching install_sound's
 * own convention (see allegro_error on failure). */
int ags_audio_init(void);

void ags_audio_shutdown(void);

/* PlayMusic(musicnum) (Engine/AC.CPP, retroactively documented in
 * matches.json -- MIDI-only real path, see the file-level comment
 * above). Early-outs if this track is already playing
 * (play->cur_music_number==musicnum), matching source exactly. */
enum AgsMusicError ags_play_music(struct GameState *play, int musicnum);

/* scr_StopMusic() -- stop_midi()+destroy_midi(), reset
 * play->cur_music_number to -1. */
void ags_stop_music(struct GameState *play);

/* IsMusicPlaying() -- our MIDI handle is set AND Allegro's own
 * midi_pos is >=0. */
int ags_is_music_playing(void);

/* GetMIDIPosition()/SeekMIDIPosition(pos) -- direct Allegro midi_pos/
 * midi_seek() passthroughs, gated on our MIDI handle being set. */
int ags_get_midi_position(void);
void ags_seek_midi_position(int pos);

/* SetMusicRepeat(loopflag) -- play->music_repeat=loopflag; (source's
 * entire one-line body, matched verbatim). */
void ags_set_music_repeat(struct GameState *play, int loopflag);

/* SetMusicVolume(newvol), validated to this build's own confirmed
 * [1,5] range (DRIFT from 2011's [-3,5] -- the negative "quieter than
 * room default" values are confirmed absent here). Writes
 * rst->options[ST_VOLUME] then recomputes the real Allegro MIDI
 * volume via update_music_volume's own already-documented formula
 * (rst->options[ST_VOLUME]*30 + play->music_master_volume, clamped
 * [0,255]) and applies it with set_volume(-1, vol). */
void ags_set_music_volume(struct RoomStruct *rst, struct GameState *play, int newvol);

/* GetCurrentMusic() -- play->cur_music_number passthrough. */
int ags_get_current_music(const struct GameState *play);

/* PlaySound(soundnum) -- see the file-level comment above for the
 * real (if, for this game, always sound-file-less) cascade. */
void ags_play_sound(int soundnum);

/* PlaySpeech -- see the file-level comment above: not a real
 * function even in the original. */
void ags_play_speech(int charid, int speechnum);

#endif /* AGS_AUDIO_H */
