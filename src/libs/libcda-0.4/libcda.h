/* libcda 0.4 -- public API header.
 *
 * The original libcda-0.4 archive used by this project only preserved
 * libcdaWin.C; this header reconstructs its public interface directly
 * from that implementation file so the ags/src build can link against it.
 *
 * Peter Wang <tjaden@psynet.net>
 */

#ifndef LIBCDA_H
#define LIBCDA_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char *cd_error;

int  cd_init(void);
void cd_exit(void);

int  cd_play(int track);
int  cd_play_range(int start, int end);
int  cd_play_from(int track);
int  cd_current_track(void);

void cd_pause(void);
void cd_resume(void);
int  cd_is_paused(void);
void cd_stop(void);

int  cd_get_tracks(int *first, int *last);
int  cd_is_audio(int track);

void cd_get_volume(int *c0, int *c1);
void cd_set_volume(int c0, int c1);

void cd_eject(void);
void cd_close(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBCDA_H */
