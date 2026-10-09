// Confirmed (Deponia_Linux.asm lines 500424-503036 `LoadConfigFile` and 540151 `ConvertStringToSize`, called by Init() in
// mainSDL.cpp): the settings file "config.ini" of the player. Each line is `Name = value` (names in any case, the value is
// the first word after the `=`); lines that start with ' or # are comments. Every setting is optional: the caller gives
// the places for those it wants (a null place is a setting that is skipped).
#pragma once

#include "WxStub.h"

/** What the resolution of the window is (Resolution = Game | Auto | Desktop | <width>x<height>). */
enum class eResolution {
	kGame = 0,
	kAuto = 1,
	kDesktop = 2,
	kSize = 3
};

/** The video interface (Device = OGL | DX9 | DX11). */
enum class eGraphicsInterface {
	kOpenGL = 1,
	kDirectX9 = 2,
	kDirectX11 = 3
};

/** How much is said in the log (LogLevel = Error | Warning | Info | Max). */
enum class eLogLevel {
	kError = 0,
	kWarning = 1,
	kInfo = 2,
	kMax = 3
};

/** The places for the settings of config.ini. */
struct TConfigTargets {
	wxString *language = nullptr;       ///< Language
	wxString *file = nullptr;           ///< File: the game file (a path, with or without "")
	bool *fullscreen = nullptr;         ///< Fullscreen = Yes | No
	bool *resizeable = nullptr;         ///< Resizeable = Yes | No
	bool *intro = nullptr;              ///< Intro = Yes | No
	eResolution *resolution = nullptr;  ///< Resolution
	wxSize *resolutionSize = nullptr;   ///< the size of Resolution = <width>x<height>
	eLogLevel *logLevel = nullptr;      ///< LogLevel
	long *musicVolume = nullptr;        ///< MusicVolume, 0 to 100
	long *soundVolume = nullptr;        ///< SoundVolume
	long *speechVolume = nullptr;       ///< SpeechVolume
	long *movieVolume = nullptr;        ///< MovieVolume
	long *globalVolume = nullptr;       ///< GlobalVolume
	long *brightness = nullptr;         ///< Brightness, 0 to 100
	bool *textureCompression = nullptr; ///< UseTextureCompression = Enabled | Disabled
	bool *textureForWidescreen = nullptr; ///< UseTextureForWidescreen = Yes | No
	eGraphicsInterface *device = nullptr; ///< Device
	bool *lockCursor = nullptr;         ///< LockCursor = Enabled | Disabled
	wxString *password = nullptr;       ///< Password
};

/** "<digits>x<digits>" as a size; false for anything else. */
bool ConvertStringToSize(const wxString &text, wxSize &size);

/** Reads the settings file at `path` into the places; false when there is no such file or a line has no `=`. */
bool LoadConfigFile(const wxString &path, const TConfigTargets &targets);

/** Confirmed (asm 503046-504200): writes the five volumes (0 to 100) into config.ini in the config directory
 *  (MusicVolume, SoundVolume, SpeechVolume, MovieVolume, GlobalVolume, the order of the arguments): the lines of the old file that
 *  begin with one of the five names (in any case) are left out and the five new lines are put at the end, through a temporary
 *  file config.tmp (the old file is moved to config.tmp2 while the new one takes its place, then that is deleted). The file and
 *  the directory are made if they do not exist. */
void WriteVolume(int music, int sound, int speech, int movie, int global);
