// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 1454187-1460704, all 15 manifest-listed methods): the
// text that is shown while a character speaks (or a text appears on the screen). It has two
// halves: the record it keeps its state in (the "ActiveText" table, a TVisionaireObject made
// by TGameControl::StartText(); its schema - GetTypeGroup()/InitType()/OnCreate()/OnInit() -
// is the static part below), and the object that works on it.
//
// The record holds the text that is left to show (kTextRemainingText), the part that is
// shown now (kTextCurrentText), how long the shown part stays (kTextTimeToWait, -1: until
// something else ends it), whether it waits for the speech to end (kTextWaitForAudio) and
// whether it is active at all (kTextActive). The text of a speaker is cut in parts at the
// pause tags (see CalculateRestText()); each part is shown for as long as the tag says, or
// for as long as the speech file plays, or - for a text without speech - for 130 ms for
// each character. The player can skip a part (SkipCurrentText()).
//
// The speech is played by the sound manager (a TSignalSlot): the text only asks it, by
// signals (see TSignalData.h), whether the player has speech on, and to load, play and stop
// the file, and whether it is still playing. The sound manager itself is not reconstructed;
// without one every question is answered with "no" and a text goes by its length.
//
// Original layout: +0x08 the record, +0x10 the speech file (a wxFileName), +0x18 whether the
// text goes without speech, +0x19 whether it has speech at all (the constructor's last
// argument), +0x20 the timer of the shown part, +0x30 the text object (TTText), +0x38 the
// sound manager.
//
// Virtuals (vtable slots 0, 8, 0x10, 0x18): SetText(), SetTextIntern(), ClearText() and
// CalculateRestText(); TGText overrides the last two. The names of the first two virtuals
// that the code of TGameControl used earlier ("Discard()", "OnCleared()") were the deleting
// destructor and ClearText().
#pragma once

#include "TSignalSlot.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/typegrp.h"
#include "datastruct/visobjref.h"

/** How a text is output: with its speech (when there is one), or only the speech (a text for
 *  the player who has it on), or only the text. The value comes from the game's setting
 *  (kGameTextOutput; for the texts of objects kGameObjectTextOutput). Names are invented. */
enum class TextOutputEnum {
	kTextAndSpeech = 0,
	kSpeechOnly = 1,
	kTextOnly = 2
};

class TSText {
public:
	// Recovered from the binary's schema (vstables/records.cpp).
	static TTypeGroup &GetTypeGroup();
	static void InitType(int versionLow, int versionHigh);
	static void OnCreate(TVisionaireObject *object);
	static void OnInit(TVisionaireObject *object);

	/** `active` is the record of the text, `data` the text object (TTText) it shows, `slot`
	 *  the sound manager (null: no speech), `speech`: whether the text can have speech. */
	TSText(const TVisObjRef &active, const TVisObjRef &data, TSignalSlot *slot, bool speech);
	virtual ~TSText();

	/** Takes the text object to show and starts showing it (as the game's text output setting
	 *  says). */
	virtual void SetText(const TVisObjRef &data);
	/** Makes the text from the text object. `balance` is the pan of the speech (-100...100). */
	virtual void SetTextIntern(TextOutputEnum mode, int balance);
	/** Ends the text: the speech stops and the record is reset. */
	virtual void ClearText();
	/** Goes on with the text that is left: shows the next part up to the next pause tag. */
	virtual void CalculateRestText();

	/** The text object. */
	TVisObjRef GetDataObject() const;
	/** The record of the text. */
	const TVisObjRef &GetTarget() const {
		return _active;
	}

	/** Goes on to the next part when the shown one has been shown long enough (or its speech has
	 *  ended). */
	void CalculateCurrentText();
	/** The player skips the part that is shown. */
	void SkipCurrentText();

	/** How long (ms) a text of this many characters is shown without speech. */
	static int GetPauseLength(unsigned long characters);

protected:
	/** A pause tag in the text is not valid: it is logged. */
	void InvalidTag();

	TVisObjRef _active;     // +0x08
	wxFileName _speechFile;  // +0x10
	bool _noSpeech;         // +0x18
	bool _speech;           // +0x19
	TTimer _timer;          // +0x20
	TVisObjRef _data;       // +0x30
	TSignalSlot *_slot;     // +0x38
};
