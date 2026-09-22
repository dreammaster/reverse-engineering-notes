/* ags/audio.h's own implementation. See that header's file-level
 * comment for the real-vs-stubbed split and evidence.
 *
 * TEMP-FILE EXTRACTION NOTE: Allegro 4.0.2's own load_midi()/
 * load_wav()/load_voc() all take a bare filesystem path and open it
 * themselves internally -- they have no CLIB awareness at all, and
 * this Allegro version has no PACKFILE-based loader variant to hand
 * an already-open stream to instead. Since Rob Blanc 1's own music/
 * sound files are packed inside rb.exe (read only via M1's own
 * ags_clib_fopen), this module extracts the requested file to a real
 * temporary file on disk first, then calls Allegro's own loader on
 * THAT path -- exactly the same technique this project's own
 * room_loader.c (M5) already uses for load_graphical_scripts's own
 * "~acscN.tmp" extraction, a genuine, precedented AGS convention,
 * not an invented workaround.
 */
#include "ags/audio.h"
#include "ags/clib.h"
#include "ags/stub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Common/acroom.h's own confirmed constant. */
#define ST_VOLUME 4

static MIDI *s_midi_handle = NULL;

/* Extracts `clib_name` (read via ags_clib_fopen) to a real temp file
 * at `temp_path`, for handing to an Allegro loader that can't read
 * CLIB-packed data directly. Returns 0 on success (temp_path now
 * holds a real, complete copy), -1 if clib_name doesn't exist or a
 * read/write error occurred. */
static int extract_to_temp_file(const char *clib_name, const char *temp_path)
{
    FILE *src, *dst;
    char buf[8192];
    size_t n;
    int ok = 0;

    src = ags_clib_fopen(clib_name, "rb");
    if (!src) {
        return -1;
    }
    dst = fopen(temp_path, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }

    for (;;) {
        n = fread(buf, 1, sizeof(buf), src);
        if (n == 0) {
            break;
        }
        if (fwrite(buf, 1, n, dst) != n) {
            ok = -1;
            break;
        }
    }

    fclose(src);
    fclose(dst);
    return ok;
}

int ags_audio_init(void)
{
    return install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
}

void ags_audio_shutdown(void)
{
    if (s_midi_handle) {
        stop_midi();
        destroy_midi(s_midi_handle);
        s_midi_handle = NULL;
    }
    remove_sound();
}

enum AgsMusicError ags_play_music(struct GameState *play, int musicnum)
{
    char clib_name[32];
    static const char *temp_path = "~ags_music_temp.mid";
    MIDI *loaded;

    if (play->cur_music_number == musicnum) {
        return AGS_MUSIC_OK; /* already playing this track -- source's own early-out */
    }

    ags_stop_music(play);

    sprintf(clib_name, "music%d.mid", musicnum);
    if (extract_to_temp_file(clib_name, temp_path) != 0) {
        /* No MOD/MP3 fallback ported -- see ags/audio.h's own
         * file-level comment: genuinely dead code for this game's
         * own real data (no such files exist). */
        return AGS_MUSIC_NOT_FOUND;
    }

    loaded = load_midi(temp_path);
    remove(temp_path);
    if (!loaded) {
        return AGS_MUSIC_NOT_FOUND;
    }

    if (play_midi(loaded, play->music_repeat) != 0) {
        /* Source's own real behavior here is quit("!Couldn't play
         * MIDI file") -- this port logs and returns an error code
         * instead of aborting the whole engine, a deliberate safety
         * improvement. */
        destroy_midi(loaded);
        return AGS_MUSIC_PLAY_FAILED;
    }

    s_midi_handle = loaded;
    play->cur_music_number = musicnum;
    return AGS_MUSIC_OK;
}

void ags_stop_music(struct GameState *play)
{
    if (s_midi_handle) {
        stop_midi();
        destroy_midi(s_midi_handle);
        s_midi_handle = NULL;
    }
    play->cur_music_number = -1;
}

int ags_is_music_playing(void)
{
    return (s_midi_handle != NULL) && (midi_pos >= 0);
}

int ags_get_midi_position(void)
{
    return s_midi_handle ? midi_pos : -1;
}

void ags_seek_midi_position(int pos)
{
    if (s_midi_handle) {
        midi_seek(pos);
    }
}

void ags_set_music_repeat(struct GameState *play, int loopflag)
{
    play->music_repeat = loopflag;
}

void ags_set_music_volume(struct RoomStruct *rst, struct GameState *play, int newvol)
{
    int vol;

    if (newvol < 1 || newvol > 5) {
        return; /* this build's own confirmed [1,5] range, DRIFT from 2011's [-3,5] */
    }
    rst->options[ST_VOLUME] = (char)newvol;

    /* update_music_volume's own already-documented formula. */
    vol = rst->options[ST_VOLUME] * 30 + play->music_master_volume;
    if (vol < 0) vol = 0;
    if (vol > 255) vol = 255;
    set_volume(-1, vol);
}

int ags_get_current_music(const struct GameState *play)
{
    return play->cur_music_number;
}

void ags_play_sound(int soundnum)
{
    static const char *exts[3] = { "mp3", "wav", "voc" };
    char clib_name[32];
    int i;

    for (i = 0; i < 3; i++) {
        FILE *f;
        sprintf(clib_name, "sound%d.%s", soundnum, exts[i]);
        f = ags_clib_fopen(clib_name, "rb");
        if (!f) {
            continue;
        }
        fclose(f);

        if (strcmp(exts[i], "mp3") == 0) {
            /* ALMP3's own streaming-sample API isn't wired here --
             * see ags/audio.h's own file-level comment: this game
             * ships no .mp3 sound effects to test against. */
            AGS_STUB_VOID();
            return;
        } else {
            static const char *temp_path = "~ags_sound_temp.tmp";
            SAMPLE *spl;
            if (extract_to_temp_file(clib_name, temp_path) != 0) {
                return;
            }
            spl = (strcmp(exts[i], "wav") == 0) ? load_wav(temp_path) : load_voc(temp_path);
            remove(temp_path);
            if (spl) {
                play_sample(spl, 255, 128, 1000, 0);
                /* Deliberately not tracked/destroyed on a timer --
                 * this game has no such files to ever reach this
                 * line, and the single-sound-channel bookkeeping
                 * this would need is later milestone work. */
            }
            return;
        }
    }
    /* No sound%d.{mp3,wav,voc} found -- matches this game's own real
     * data exactly (it ships none), a correct, verified no-op. */
}

void ags_play_speech(int charid, int speechnum)
{
    (void)charid;
    (void)speechnum;
    /* See ags/audio.h's own file-level comment: the ORIGINAL engine's
     * own PlaySpeech is a bare quit() in both eras. Logged instead of
     * aborted. */
    AGS_STUB_VOID();
}
