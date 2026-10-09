// Not yet assert-confirmed to a specific file; stays at the top level.
//
// The movies (Deponia_Linux.asm lines 1068991-1072000, TMovie; the glue of TMasterControl::PlayAVI, VideoFrame and
// MovieEvent is in masterControl.cpp). The original plays them with its own port of ffplay: a `VideoState` of 0x102550
// bytes (+0x60 of a TMovie) with the demuxer, decoder and audio threads, the subtitles, the drawing of the picture and the
// reading of the keys; those are FFmpeg's code and not reconstructed. The ScummVM engine plays movies with its own
// video decoders, so everything the engine does around that is here, and the player itself is behind TMoviePlayer, which
// CreateMoviePlayer() makes (here: nothing, and a movie that cannot be played ends at once).
//
// What a TMovie does (all confirmed):
//  - The constructor puts the movie in `openMovies`, the list of all that are alive (graphics.openedVideos() lists them);
//    Finish() takes it out. The movie of the master control is one of them.
//  - PlayCutScene() works out the file to play: with `encrypted` the header of a movie in a composed file (a ".v<n>" file,
//    TComposedFileManager) is unscrambled for the time of the movie (and scrambled again by Finish()); a movie in a
//    composed file is played from its container; anything else from the file itself. If the player cannot open the file
//    the other way of reading it is tried.
//  - OneFrame() is called by the main loop for each frame of a movie (TMasterControl::VideoFrame()): true while it goes
//    on. The mouse, touch and controller events the player sees are handed to the `event function` (the master control's
//    MovieEvent(), which gives them to the script function movieEvent()). When the movie is over (or was skipped) Finish()
//    is called and OneFrame() says false.
//  - Finish() ends the movie: out of `openMovies`, the temporary file of a decrypted movie is removed, the header scrambled
//    again, the player closed, and (with kGameShowBlackScreenAfterVideo) the screen is cleared.
#pragma once

#include <functional>
#include <vector>

#include "WxStub.h"

class TPictureIO;
class TFontManager;
class TSoundInterface;

/** What a player is told about the movie (the original keeps this in globals of its ffplay: g_subtitleLanguage ...). */
struct TMovieSettings {
	wxPoint subtitlePosition;        ///< kGameVideoSubtitlePosition: where the subtitles are
	wxString subtitleLanguage;       ///< kGameVideoSubtitleLanguage
	wxString audioLanguage;          ///< kGameVideoAudioLanguage
	wxPoint videoResolution;         ///< the size the picture is shown in (kGameWindowResolution)
	bool skippable = false;          ///< a key or a click ends the movie
	TPictureIO *pauseScreen = nullptr;   ///< kGameVideoPauseScreen: the picture shown while the movie is paused (owned by the caller)
	TFontManager *fontManager = nullptr; ///< the fonts of the subtitles
	int fontId = 0;                  ///< the id of the font object, packed (see PackVisId)
	TSoundInterface *soundManager = nullptr;  ///< null for a movie a script opened
	int volume = 100;                ///< the lower of the volume of the movies and the global volume (0 to 100)
};

/** The player of one movie: the part of the original that is its ffplay. */
class TMoviePlayer {
public:
	virtual ~TMoviePlayer() {}

	/** Starts the movie in `path` (a file the player can read). False when it cannot. */
	virtual bool Open(const wxString &path, const TMovieSettings &settings) = 0;
	/** One turn of the loop of the movie: reads the input, decodes and shows what is due. The mouse, touch and controller
	 *  events are given to `eventFunction` (null: nobody wants them). False when the movie is over. */
	virtual bool Frame(const std::function<void(void *)> &eventFunction) = 0;
	/** Closes the movie (the player can be thrown away then). */
	virtual void Close() = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
	/** Goes to the time `seconds`. */
	virtual void Seek(float seconds) = 0;
	virtual float GetTime() = 0;
	virtual float GetDuration() = 0;
	/** How much of the movie was played, in thousandths. */
	virtual float GetCompletition() = 0;
	virtual int GetWidth() = 0;
	virtual int GetHeight() = 0;
	/** How the picture is mixed with the screen (the number the script gave as `blend`). */
	virtual void SetBlend(int blend) = 0;
	/** The colour (red, green, blue, alpha) the picture is multiplied by. */
	virtual void SetColour(const wxColour &colour) = 0;
	virtual void SetLoop(bool loop) = 0;
	/** Draws the current picture at once in the rectangle (a script draws the movie itself: the player does not look at
	 *  the input and the clock). The sizes are -1 for the size of the picture, a negative number of the game for the
	 *  window. False when there is nothing to draw (the movie is over). */
	virtual bool Draw(float x, float y, float width, float height) = 0;
};

/** Makes the player of the movies of the platform; null when there is none (the ScummVM engine provides it, here it is
 *  whatever `g_createMoviePlayer` makes, which is nothing to begin with). */
TMoviePlayer *CreateMoviePlayer();
extern TMoviePlayer *(*g_createMoviePlayer)();

class TMovie {
public:
	TMovie();
	~TMovie();

	TMovie(const TMovie &) = delete;
	TMovie &operator=(const TMovie &) = delete;

	/** Makes the player (`preview`: the editor's simple preview, which draws by itself). False when there is none. */
	bool Initialize(bool preview);
	/** Starts the movie `file`. The parameters are those of TMovie::PlayCutScene() of the original; `showBlackScreenAfter`
	 *  is kGameShowBlackScreenAfterVideo. False when it cannot be played. */
	bool PlayCutScene(const wxFileName &file, const TMovieSettings &settings, bool encrypted, bool showBlackScreenAfter);
	/** One frame of the movie: true while it goes on (false: it is over, and Finish() has been called). */
	bool OneFrame();
	void Finish();
	void Pause();
	void Resume();
	void Seek(float seconds);
	float GetTime();
	float GetDuration();
	float GetCompletition();
	int GetWidth();
	int GetHeight();
	void SetBlend(int blend);
	void SetColour(const wxColour &colour);
	void SetLoop(bool loop);
	bool NowaitDraw(float x, float y, float width, float height);
	/** A movie a script opened (nothing: the original does nothing here). */
	void SetInScene() {}

	/** Called for each frame to draw what is around the picture (+0x00 of the original: the master control's DrawInterfaces). */
	std::function<void()> _drawFunction;
	/** Called with the SDL event for a mouse, touch or controller event (+0x20: the master control's MovieEvent). */
	std::function<void(void *)> _eventFunction;
	/** The Lua stack at the time a script opened the movie (+0x40), for graphics.openedVideos(). */
	wxString _callstack;

	/** The movie that is played from the file `_path` (+0x50: the file the player reads), whose name in the game is
	 *  `_file` (+0x58). */
	wxString _path;
	wxFileName _file;

	/** All the movies that exist (`openMovies`). */
	static std::vector<TMovie *> s_openMovies;

private:
	bool _preview = false;          // +0x48
	bool _blackScreenAfter = false; // +0x49
	bool _deleteFile = false;       // +0x4A: _path is a temporary file that Finish() removes
	bool _scrambleAgain = false;    // +0x4B: the header of the movie in its container was unscrambled
	TMoviePlayer *_player = nullptr; // +0x60 (the VideoState in the original)
};
