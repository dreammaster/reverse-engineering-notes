// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 212566-216200, all 31 manifest-listed methods): the text
// the game shows on the screen. It is a TSText (see there for the parts, pause tags and the
// speech) that is drawn: it knows who speaks it (a TGCharacter, whose talking animation plays
// while the text is shown and who is turned to the speech's pan), where it is put (next to the
// speaker, or at the position it was given) and what to tell the object it belongs to (a
// TManagedObject, for texts of objects) when it is over.
//
// All texts that are showing are in a list (s_vRunningTexts); when the game is stopped (a menu)
// they are stopped with it (StopRunningTexts()) and go on afterwards (ContinueStoppedTexts()).
// The scripts can hook into a text (the registered names of Lua functions: TextStarted,
// TextStopped, the text itself, its position and its drawing); the hooks are called through the Lua
// bridge (LuaExecuteFunction()).
//
// Original layout: TSText ends at +0x40; +0x40 whether the position was set by the data (THText
// sets it), +0x48 the lines of the text (std::list<wxString>), +0x58 their widths, +0x70 the glyph
// buffers of the printed text, +0x88 whether the text is stopped, +0x89 whether the text has been
// put in its place, +0x90 the speaker, +0x98 the owner.
#pragma once

#include <list>
#include <vector>

#include "TSText.h"
#include "TTimer.h"
#include "WxStub.h"
#include "vscommon/fontManager.h"

class GLCharBuffer;
class TGCharacter;
class TManagedObject;

class TGText : public TSText {
public:
	/** A text that is made from the record `active` and the text object `object` (the other
	 *  constructor's own parts are set by THText). */
	TGText(const TVisObjRef &active, const TVisObjRef &object);
	/** A text spoken by `character` (null: a text at `pos`) from the text object `text`;
	 *  `font` is the font of a text without a speaker, `background`: shown in the whole scene
	 *  (not just the part that is in view), `speech`: whether it can have speech. */
	TGText(const TVisObjRef &active, const TVisObjRef &object, TGCharacter *character, const TVisObjRef &text,
	       TextAlignmentEnum alignment, const TVisObjRef &font, const wxPoint &pos, bool background, bool speech);
	~TGText() override;

	using TSText::SetText;
	using TSText::SetTextIntern;
	/** Makes the text again from another record. */
	void SetText(const TVisObjRef &active, TGCharacter *character, const TVisObjRef &text, TextAlignmentEnum alignment,
	             const TVisObjRef &font, const wxPoint &pos, bool background);
	/** Makes the text for the speaker (or the position): the speaker's font and the pan of
	 *  its speech, the speaker's talking animation. */
	void SetTextIntern(TGCharacter *character, const TVisObjRef &text, TextAlignmentEnum alignment,
	                   const TVisObjRef &font, const wxPoint &pos, bool background);
	/** Ends the text: the speaker stops talking and the owner is told. */
	void ClearText() override;
	/** The next part of the text (the script's text hook can change it); it is cut in lines. */
	void CalculateRestText() override;

	/** Draws the lines of the shown part (`scale` is for a text without a speaker). */
	void Draw(float scale);
	/** Saves how long the text has been shown. */
	void Save();
	/** Restores the text from a loaded game: the time it has been shown, its speaker. */
	void Load();

	TGCharacter *GetSpeaker() const;
	/** The text tells `owner` when it is over (instead of the owner it has). */
	void SetOwner(TManagedObject *owner);
	void DetachOwner(TManagedObject *owner);

	/** The pan (-100...100, scaled by the game's setting) of the sound of a speaker, from where
	 *  it is on the screen. */
	int CalculateSpeakerSoundBalance(TGCharacter *character);
	/** Stops this text with the game that was stopped. */
	void ContinueStoppedText();

	/** The texts of this scene's characters talk again (when the characters are not walking). */
	static void RestartTalkAnimations(const TVisObjRef &scene);
	/** The game stops (a menu): all showing texts are stopped. */
	static void StopRunningTexts();
	/** The game goes on: all texts that were stopped go on. */
	static void ContinueStoppedTexts();

	// The registered names of the Lua functions the scripts hook into the text with.
	static void RegisterEventHandlerTextStarted(const wxString &name);
	static void RegisterEventHandlerTextStopped(const wxString &name);
	static wxString GetEventHandlerTextStarted();
	static wxString GetEventHandlerTextStopped();
	static void RegisterHookFunctionSetTextPosition(const wxString &name);
	static void RegisterHookFunctionRender(const wxString &name);
	static void RegisterHookFunctionText(const wxString &name);

protected:
	/** Tells the owner that the text is over (once). */
	void NotifyOwnerTextFinished();

	bool _positionSet;                      // +0x40, the position comes from the data
	std::list<wxString> _lines;             // +0x48
	std::vector<int> _lineWidths;           // +0x58
	std::vector<GLCharBuffer *> _buffers;   // +0x70
	bool _stopped;                          // +0x88
	bool _placed;                           // +0x89
	TGCharacter *_speaker;                  // +0x90
	TManagedObject *_owner;                 // +0x98

private:
	/** The text starts: the scripts are told. */
	void TextStarted();
	/** The text is over: the scripts are told. */
	void TextStopped();
	/** Cuts the shown part into lines that fit the screen and measures them. */
	void SplitText();
	/** Puts the lines next to the speaker (above it, or beside it when there is not room above),
	 *  inside the part of the scene that is shown. */
	void CalculateTextPos();

	static std::vector<TGText *> s_runningTexts;
	static TTimer s_stopTime;
	static wxString HookFunctionText;
	static wxString HookFunctionRender;
	static wxString HookFunctionSetTextPosition;
	static wxString EventHandlerTextStopped;
	static wxString EventHandlerTextStarted;
};
