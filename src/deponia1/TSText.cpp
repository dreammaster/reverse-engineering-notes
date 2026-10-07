#include "TSText.h"

#include <cstdio>
#include <cwchar>
#include <string>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TComposedFileManager.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vstables/text.cpp";

// The time a character of a text is shown for when it has no speech (ms).
static const float kMillisecondsPerCharacter = 130.0f;

// Asks the sound manager `type` about the speech file. Returns whether its answer is of the
// type asked (the answer's `value` is the result).
static bool ask(TSignalSlot *slot, int type, const wxFileName &file, TSignalData &answer) {
	TSignalData question;

	question.type = type;
	question.speechFile = file.GetFullPath();
	slot->Signal(question, answer);
	return answer.type == type;
}

// Confirmed (asm lines 1458005-1458054)
TSText::TSText(const TVisObjRef &active, const TVisObjRef &data, TSignalSlot *slot, bool speech)
	: _active(active), _data(data), _slot(slot) {
	_speech = speech;
	_noSpeech = false;
}

// Confirmed (asm lines 216919-216953)
TSText::~TSText() {
}

// Confirmed (asm lines 1454188-1454230)
void TSText::SetText(const TVisObjRef &data) {
	_data = data;

	int mode = _active.GetVisionaire()->GetGame().GetInt(kGameTextOutput);

	SetTextIntern((TextOutputEnum)mode, 0);
}

// Confirmed (asm lines 1458434-1458443)
TVisObjRef TSText::GetDataObject() const {
	return _data;
}

// Confirmed (asm lines 1458455-1458685): the speech stops and the record is reset.
void TSText::ClearText() {
	if (TComposedFileManager::FileExists(_speechFile)) {
		if (_slot) {
			TSignalData stop;
			TSignalData ignored;

			stop.type = kSignalSpeechStop;
			stop.speechFile = _speechFile.GetFullPath();
			_slot->Signal(stop, ignored);
		}

		_speechFile.Clear();
	}

	_noSpeech = false;

	_active.SetValue(kTextRemainingText, wxString(), TSendEventEnum::kSendEvent);
	_active.SetValue(kTextCurrentText, wxString(), TSendEventEnum::kSendEvent);
	_active.SetValue(kTextTimeToWait, 0, TSendEventEnum::kSendEvent);
	_active.SetValue(kTextWaitForAudio, false, TSendEventEnum::kSendEvent);
	_active.SetValue(kTextActive, false, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 1458699-1458705): the virtual CalculateRestText().
void TSText::SkipCurrentText() {
	CalculateRestText();
}

// Confirmed (asm lines 1458716-1458900)
void TSText::CalculateCurrentText() {
	int duration = _active.GetInt(kTextTimeToWait);

	if (_timer.GetTime() >= duration && duration >= 0) {
		CalculateRestText();
		return;
	}

	// a part that waits for its speech goes on when the speech has ended
	if (!_active.GetBool(kTextWaitForAudio) || !_slot)
		return;

	TSignalData answer;

	if (!ask(_slot, kSignalSpeechIsPlaying, _speechFile, answer)) {
		x_assert(false, "false", kSourceFile, 0x243);
		return;
	}

	if (answer.value == 0)
		CalculateRestText();
}

// Confirmed (asm lines 1458913-1459000)
void TSText::InvalidTag() {
	wxString text = _active.GetStr(kTextRemainingText);

	if (wxLog::loglevel > 0)
		wxLog::logexpanded(L"Invalid tag in text: '%s'", text.wc_str());
}

// Confirmed (asm lines 1460684-1460704)
int TSText::GetPauseLength(unsigned long characters) {
	return (int)((float)characters * kMillisecondsPerCharacter);
}

// Confirmed (asm lines 1459008-1460140): the text that is left is cut at its first pause tag:
// what is before the tag is the part shown now, what is after it stays. A text is made of parts
// and pause tags in between:
//   <p>        the part stays until it is skipped
//   <pt>       the part stays as long as a text of its length without speech would
//   <pa>       the part stays until its speech has ended (without speech: as <pt>)
//   <p5> <p5s> <p500ms>   the part stays 5 seconds, or 500 milliseconds (a number after <pa> is
//              the time when there is no speech)
// The time of the tags that go by a number or the length (not <p> and <pa>) is scaled by the
// player's text speed (kGameTextSpeed, percent).
void TSText::CalculateRestText() {
	std::wstring rest = _active.GetStr(kTextRemainingText).ToStdWstring();
	std::wstring current = _active.GetStr(kTextCurrentText).ToStdWstring();
	int duration = _active.GetInt(kTextTimeToWait);
	std::size_t tag = rest.find(L"<p");
	bool scaled = false;

	_active.SetValue(kTextWaitForAudio, false, TSendEventEnum::kSendEvent);

	if (tag == std::wstring::npos) {
		// no more tags: everything that is left is the part shown now
		current = rest;
		rest.clear();
		duration = -1;

		if (_speech) {
			TSignalData answer;
			bool speechEnabled = (_slot && ask(_slot, kSignalSpeechEnabled, _speechFile, answer) && answer.value != 0);

			if (TComposedFileManager::FileExists(_speechFile) && speechEnabled) {
				// it stays until the speech has ended
				if (!current.empty())
					_active.SetValue(kTextWaitForAudio, true, TSendEventEnum::kSendEvent);
			} else {
				duration = GetPauseLength(current.size());
				scaled = true;
			}
		}
	} else {
		current = rest.substr(0, tag);

		std::size_t start = tag + 2;
		std::size_t close = start;

		while (close < rest.size() && rest[close] != L'>')
			close++;

		if (start >= rest.size() || close >= rest.size()) {
			// (the tag does not end: the text is left as it is)
			InvalidTag();
		} else {
			std::size_t next = close + 1;
			bool numeric = false;

			if (rest.compare(start, 2, L"t>") == 0) {
				// <pt>
				duration = GetPauseLength(tag);
				scaled = true;
			} else if (start == close) {
				// <p>
				duration = -1;
			} else if (rest[start] == L'a') {
				// <pa>: the speech decides
				duration = -1;

				if (TComposedFileManager::FileExists(_speechFile)) {
					_active.SetValue(kTextWaitForAudio, true, TSendEventEnum::kSendEvent);
				} else {
					if (!_noSpeech && wxLog::loglevel > 0) {
						TVisObjRef parent = _data.GetParent();
						wxString name = parent.GetNameWithParents(3);
						int id = PackVisId(_data.GetId());

						if (_speechFile.GetName().IsEmpty()) {
							wxLog::logexpanded(L"text of '%s' (id: %d) has a <pa> pause tag but no speech file.",
							                   name.wc_str(), id);
						} else {
							wxLog::logexpanded(L"Speech file '%s' for text of '%s' (id: %d) does not exist.",
							                   _speechFile.GetFullPath().wc_str(), name.wc_str(), id);
						}
					}

					if (close - 1 == start) {
						// without a number: as long as the text so far takes
						duration = GetPauseLength(tag);
					} else {
						start++;
						numeric = true;
					}
				}
			} else {
				numeric = true;
			}

			if (numeric) {
				// <p5>, <p5s>, <p500ms>
				std::size_t end = close;
				bool milliseconds = false;

				if (rest[close - 1] == L's') {
					end = close - 1;

					if (end > start && rest[close - 2] == L'm') {
						end = close - 2;
						milliseconds = true;
					}
				}

				std::wstring digits = rest.substr(start, end - start);

				if (digits.find_first_not_of(L"0123456789") != std::wstring::npos) {
					InvalidTag();
				} else {
					int value = duration;

					std::swscanf(digits.c_str(), L"%d", &value);
					duration = milliseconds ? value : value * 1000;
					scaled = true;
				}
			}

			// the tag and what is before it are done with
			rest = rest.substr(next);
		}
	}

	_timer.SetTime();

	if (scaled) {
		int speed = _active.GetVisionaire()->GetGame().GetInt(kGameTextSpeed);

		if (speed > 0 && speed != 100)
			duration = (int)((double)duration * (100.0 / (double)speed));
	}

	_active.SetValue(kTextRemainingText, wxString(rest), TSendEventEnum::kSendEvent);
	_active.SetValue(kTextCurrentText, wxString(current), TSendEventEnum::kSendEvent);
	_active.SetValue(kTextTimeToWait, duration, TSendEventEnum::kSendEvent);

	// nothing left to show, and nothing to wait for: the text is over
	if (current.empty() && rest.empty() && !_active.GetBool(kTextWaitForAudio))
		ClearText();
}

// Confirmed (asm lines 1460145-1460680)
void TSText::SetTextIntern(TextOutputEnum mode, int balance) {
	ClearText();

	if (_active.IsEmpty())
		return;

	_active.SetValue(kTextActive, true, TSendEventEnum::kSendEvent);

	wxString text;

	TTText(_data).GetTextProperties(text, _speechFile);

	// has the player turned speech off? (without a sound manager he has not)
	bool speechDisabled = false;

	if (_slot) {
		TSignalData answer;

		if (ask(_slot, kSignalSpeechDisabled, _speechFile, answer))
			speechDisabled = (answer.value != 0);
	}

	if (!speechDisabled && mode == TextOutputEnum::kSpeechOnly) {
		// only the speech is output: there is no text to show, the part waits for the speech
		_active.SetValue(kTextRemainingText, wxString(), TSendEventEnum::kSendEvent);
		_active.SetValue(kTextCurrentText, wxString(), TSendEventEnum::kSendEvent);
		_active.SetValue(kTextWaitForAudio, true, TSendEventEnum::kSendEvent);
		_active.SetValue(kTextTimeToWait, -1, TSendEventEnum::kSendEvent);
	} else {
		if (speechDisabled || mode == TextOutputEnum::kTextOnly) {
			_speechFile.Clear();
			_noSpeech = true;
		}

		if (!TTText::ReplaceValues(text, _active.GetVisionaire()))
			InvalidTag();

		_active.SetValue(kTextRemainingText, text, TSendEventEnum::kSendEvent);
		CalculateRestText();
	}

	// the speech starts
	if (TComposedFileManager::FileExists(_speechFile) && _slot) {
		TSignalData answer;

		if (!ask(_slot, kSignalSpeechLoad, _speechFile, answer)) {
			x_assert(false, "false", kSourceFile, 0x208);
		} else {
			TSignalData play;
			TSignalData ignored;

			play.type = kSignalSpeechPlay;
			play.speechFile = _speechFile.GetFullPath();
			play.value = answer.value;
			play.value2 = balance;
			play.value3 = 0;
			play.value4 = 2;
			_slot->Signal(play, ignored);
		}
	}
}
