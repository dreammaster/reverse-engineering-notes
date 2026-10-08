#include "TGText.h"

#include <algorithm>
#include <string>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TCFont.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TManagedObject.h"
#include "datastruct/visionaire.h"
#include "vsplayer/control/gameControl.h"
#include "vscommon/scripting/argument.h"
#include "vscommon/scripting/lua.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/textGame.cpp";

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

std::vector<TGText *> TGText::s_runningTexts;
TTimer TGText::s_stopTime;
wxString TGText::HookFunctionText;
wxString TGText::HookFunctionRender;
wxString TGText::HookFunctionSetTextPosition;
wxString TGText::EventHandlerTextStopped;
wxString TGText::EventHandlerTextStarted;

// Confirmed (asm lines 212567-212607)
TGText::TGText(const TVisObjRef &active, const TVisObjRef &object)
	: TSText(active, object, g_pGameControl->GetSoundManager(), true) {
	_positionSet = false;
	_stopped = false;
	_placed = false;
	_speaker = nullptr;
	_owner = nullptr;
}

// Confirmed (asm lines 214043-214150): a new text is listed with the running ones.
TGText::TGText(const TVisObjRef &active, const TVisObjRef &object, TGCharacter *character, const TVisObjRef &text,
               TextAlignmentEnum alignment, const TVisObjRef &font, const wxPoint &pos, bool background,
               bool speech)
	: TSText(active, object, g_pGameControl->GetSoundManager(), speech) {
	_positionSet = false;
	_stopped = false;
	_placed = false;
	_speaker = nullptr;
	_owner = nullptr;

	// the record is named as the text object
	_active.SetName(_data.GetName());

	SetTextIntern(character, text, alignment, font, pos, background);

	s_runningTexts.push_back(this);
}

// Confirmed (asm lines 213765-213990)
TGText::~TGText() {
	std::vector<TGText *>::iterator running = std::find(s_runningTexts.begin(), s_runningTexts.end(), this);

	if (running != s_runningTexts.end())
		s_runningTexts.erase(running);

	NotifyOwnerTextFinished();

	for (GLCharBuffer *buffer : _buffers)
		delete buffer;

	if (_active.GetBool(kTextActive))
		TextStopped();

	TSText::ClearText();

	if (_speaker) {
		_speaker->StopTalkAnim();
		_speaker = nullptr;
		_active.ClearLink(kTextOwner, true);
	}

	NotifyOwnerTextFinished();

	// the record goes with the text
	_active.Remove();
}

// Confirmed (asm lines 212619-212635)
void TGText::NotifyOwnerTextFinished() {
	if (_owner) {
		_owner->TextFinished(this);
		_owner = nullptr;
	}
}

// Confirmed (asm lines 212646-212651)
void TGText::SetOwner(TManagedObject *owner) {
	_owner = owner;
}

// Confirmed (asm lines 212663-212679): only the owner that has the text can let go of it.
void TGText::DetachOwner(TManagedObject *owner) {
	if (_owner != owner) {
		x_assert(false, "false", kSourceFile, 0x76);
		return;
	}

	_owner = nullptr;
}

// Confirmed (asm lines 212691-212695)
TGCharacter *TGText::GetSpeaker() const {
	return _speaker;
}

// Confirmed (asm lines 212707-212790): the pan of the speaker's sound: -100 at the left edge of
// the screen to 100 at the right edge, scaled by the game's setting (kSpeakerSoundPanFactor,
// percent).
int TGText::CalculateSpeakerSoundBalance(TGCharacter *character) {
	if (!character)
		return 0;

	int factor = _active.GetVisionaire()->GetGame().GetInt(kSpeakerSoundPanFactor);
	TGScene *scene = gameControl()->GetScene();
	int scrollX = scene->GetScrollPos().x;
	const wxSize &visible = gameControl()->GetScene()->GetVisibleSize();
	float halfWidth = (float)((double)visible.width / 200.0);
	int x = character->GetPosition().x - scrollX;
	float onScreen = 0.0f;

	if (x >= 0)
		onScreen = (float)std::min(x, visible.width);

	int pan = (int)((double)(onScreen / halfWidth) - 100.0);

	return (int)((double)pan * ((double)factor / 100.0));
}

// Confirmed (asm lines 212814-212880)
void TGText::RestartTalkAnimations(const TVisObjRef &scene) {
	for (TGText *text : s_runningTexts) {
		TGCharacter *speaker = text->_speaker;

		if (speaker && !speaker->IsWalking() && speaker->GetRef().GetLink(kCharacterScene) == scene)
			speaker->StartTalkAnim();
	}
}

// Confirmed (asm lines 212896-212915)
void TGText::StopRunningTexts() {
	for (TGText *text : s_runningTexts)
		text->_stopped = true;

	s_stopTime.SetTime();
}

// Confirmed (asm lines 212927-212976)
void TGText::ContinueStoppedTexts() {
	for (TGText *text : s_runningTexts) {
		if (!text->_stopped)
			continue;

		if (text->_speaker && !text->_speaker->IsWalking())
			text->_speaker->StartTalkAnim();

		text->_stopped = false;
		text->_timer.AdjustTimer(s_stopTime.GetTime());
	}
}

// Confirmed (asm lines 212991-213014)
void TGText::ContinueStoppedText() {
	if (_speaker && !_speaker->IsWalking())
		_speaker->StartTalkAnim();

	_stopped = false;
	_timer.AdjustTimer(s_stopTime.GetTime());
}

// Confirmed (asm lines 213025-213065)
void TGText::Save() {
	TTimer elapsed = _timer;

	if (_stopped)
		elapsed.AdjustTimer(s_stopTime.GetTime());

	_active.SetValue(kTextTimeElapsed, (int)elapsed.GetTime(), TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 214429-214500): the text goes on from the time it had been shown.
void TGText::Load() {
	_placed = false;
	_stopped = false;
	_timer.SetTime();
	_timer.AdjustTimer(-_active.GetInt(kTextTimeElapsed));

	TVisObjRef speaker = _active.GetLink(kTextOwner);

	if (speaker.IsEmpty() || speaker.GetId()[3] != 0)
		_speaker = nullptr;
	else
		_speaker = gameControl()->GetCharacterPointer(speaker);

	SplitText();
}

// Confirmed (asm lines 213082-213135): the scripts are told that the text starts.
void TGText::TextStarted() {
	if (EventHandlerTextStarted.IsEmpty())
		return;

	LuaDebugName("TextStartedHook");
	LuaExecuteEventHandler(std::string((const char *)EventHandlerTextStarted.mb_str()), _active);
}

// Confirmed (asm lines 213465-213520)
void TGText::TextStopped() {
	if (EventHandlerTextStopped.IsEmpty())
		return;

	LuaDebugName("TextStoppedHook");
	LuaExecuteEventHandler(std::string((const char *)EventHandlerTextStopped.mb_str()), _active);
}

// Confirmed (asm lines 213150-213400): the speaker's font (or the given one), where it is, the
// pan of its speech, and the speaker starts talking (unless it is walking and the text is a
// background one).
void TGText::SetTextIntern(TGCharacter *character, const TVisObjRef &text, TextAlignmentEnum alignment,
                           const TVisObjRef &font, const wxPoint &pos, bool background) {
	int balance = 0;

	if (character) {
		_active.SetLink(kTextFont, character->GetRef().GetLink(kCharacterFont), true);
		balance = CalculateSpeakerSoundBalance(character);
	} else {
		_active.SetLink(kTextFont, font, true);
		_active.SetValue(kTextPosition, pos, TSendEventEnum::kNoEvent);
	}

	// the output setting of the game; the texts of the game's other tables have their own
	TVisObjRef game = _active.GetVisionaire()->GetGame();
	int mode = game.GetInt(kGameTextOutput);

	if (!text.IsEmpty() && text.GetId()[3] != 0)
		mode = game.GetInt(kGameObjectTextOutput);

	TSText::SetTextIntern((TextOutputEnum)mode, balance);

	_stopped = false;
	_active.SetValue(kTextBackground, background, TSendEventEnum::kSendEvent);
	_active.SetValue(kTextAlignment, (int)alignment, TSendEventEnum::kSendEvent);
	_speaker = character;

	if (character)
		_active.SetLink(kTextOwner, character->GetRef(), true);
	else if (!text.IsEmpty())
		_active.SetLink(kTextOwner, text, true);
	else
		_active.ClearLink(kTextOwner, true);

	if (!_active.GetBool(kTextActive))
		return;

	TextStarted();

	if (!_speaker)
		return;

	bool walking = _speaker->IsWalking();
	bool skeleton = _speaker->IsSpineAnimation();

	// a speaker that is walking goes on walking when the text is a background one
	if (walking && !skeleton && background)
		return;

	if (!skeleton)
		_speaker->StopWalking(true);

	_speaker->StartTalkAnim();
}

// Confirmed (asm lines 213408-213453)
void TGText::SetText(const TVisObjRef &active, TGCharacter *character, const TVisObjRef &text,
                     TextAlignmentEnum alignment, const TVisObjRef &font, const wxPoint &pos, bool background) {
	_active = active;
	SetTextIntern(character, text, alignment, font, pos, background);
}

// Confirmed (asm lines 213534-213575)
void TGText::ClearText() {
	if (_active.GetBool(kTextActive))
		TextStopped();

	TSText::ClearText();

	if (_speaker) {
		_speaker->StopTalkAnim();
		_speaker = nullptr;
		_active.ClearLink(kTextOwner, true);
	}

	NotifyOwnerTextFinished();
}

// Confirmed (asm lines 214513-214798). When the scripts registered a function for the hook "text"
// (RegisterHookFunctionText), it is called with the text's record and the string it returns is the
// part of the text to show.
void TGText::CalculateRestText() {
	TSText::CalculateRestText();
	_placed = false;

	if (!HookFunctionText.IsEmpty()) {
		TArgument record;
		TArgument result;

		record.Set(_active);
		result.SetType(TArgType::kString);

		std::vector<TArgument *> arguments = {&record};
		std::vector<TArgument *> results = {&result};

		LuaDebugName("TextTextHook");

		if (LuaExecuteFunction(std::string(HookFunctionText.mb_str()), arguments, results))
			_active.SetValue(kTextCurrentText, result.GetString(), TSendEventEnum::kSendEvent);
	}

	if (!_active.GetStr(kTextCurrentText).IsEmpty())
		SplitText();
}

// Confirmed (asm lines 214174-214420): the part of the text that is shown is cut in lines that
// fit the screen ('<' is a line break in a text), and the lines are measured.
void TGText::SplitText() {
	_lines.clear();
	_lineWidths.clear();

	g_pGameControl->GetFontManager()->SetCurrentFont(_active.GetLink(kTextFont));

	wxString text = _active.GetStr(kTextCurrentText);

	text.Replace(L"<", L"\n", true);
	g_pGameControl->GetFontManager()->PerformAutoLineBreak(text, _lines);

	for (const wxString &line : _lines) {
		wxPoint size;

		g_pGameControl->GetFontManager()->GetTextDimension(line, size);
		_lineWidths.push_back(size.x);
	}
}

// Confirmed (asm lines 214808-215345). The script's position hook ("TextPositionHook", with the text's record:
// a true answer means the script has set the position and nothing more is done). The lines go
// above the speaker, centred on its sprite; when they would not fit above it, beside it (on the
// side with more room); and always inside the part of the scene that is shown (or the whole scene
// for a background text). The position is kept as the centre of the lines.
void TGText::CalculateTextPos() {
	if (!HookFunctionSetTextPosition.IsEmpty()) {
		TArgument record;
		TArgument result;

		record.Set(_active);
		result.SetType(TArgType::kBool);

		std::vector<TArgument *> arguments = {&record};
		std::vector<TArgument *> results = {&result};

		LuaDebugName("TextPositionHook");

		if (LuaExecuteFunction(std::string(HookFunctionSetTextPosition.mb_str()), arguments, results) &&
		        result.GetBool())
			return;
	}

	if (!_speaker || !g_pGameControl)
		return;

	_placed = true;

	wxRect sprite = _speaker->GetCurrentSpriteRect();

	g_pGameControl->GetFontManager()->SetCurrentFont(_active.GetLink(kTextFont));

	wxPoint size;

	g_pGameControl->GetFontManager()->GetTextDimension(_lines, size);

	wxRect area;
	TGScene *scene = gameControl()->GetScene();

	if (_active.GetBool(kTextBackground)) {
		area = scene->GetWorktopArea();
	} else {
		const wxPoint &scroll = scene->GetScrollPos();
		const wxSize &visible = scene->GetVisibleSize();

		area.x = scroll.x;
		area.y = scroll.y;
		area.width = visible.width;
		area.height = visible.height;
	}

	wxRect lines;

	if (sprite.GetTop() - size.y < 0) {
		// no room above the speaker
		int toLeft = sprite.GetLeft() - area.GetLeft();
		int toRight = area.GetRight() - sprite.GetRight();

		lines.x = (toLeft > toRight) ? sprite.GetLeft() - size.x : sprite.GetRight();
		lines.width = size.x;
		lines.y = sprite.GetTop();
	} else {
		lines.x = sprite.GetLeft() + sprite.GetWidth() / 2 - size.x / 2;
		lines.width = size.x;
		lines.y = sprite.GetTop() - size.y;
	}

	lines.height = size.y;

	// inside the area
	wxPoint position = *_active.GetPoint(kTextPosition);

	if (lines.GetLeft() < area.GetLeft())
		position.x = area.GetLeft();
	else if (lines.GetRight() > area.GetRight())
		position.x = area.GetRight() - size.x;
	else
		position.x = lines.GetLeft();

	if (lines.GetTop() < area.GetTop())
		position.y = area.GetTop();
	else if (lines.GetBottom() > area.GetBottom())
		position.y = area.GetBottom() - size.y;
	else
		position.y = lines.GetTop();

	position.x += size.x / 2;
	_active.SetValue(kTextPosition, position, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 215356-216885). The scripts' drawing hook ("TextRenderHook", with the text's record, the
// lines, their widths, the position, the alignment and the alpha; a true answer means the script has drawn the
// text). Not drawn while the text is stopped, empty, or its speaker is not in the scene that is shown.
void TGText::Draw(float scale) {
	if (_stopped)
		return;

	g_pGameControl->GetFontManager()->SetCurrentFont(_active.GetLink(kTextFont));

	if (_active.GetStr(kTextCurrentText).IsEmpty())
		return;

	float alpha = scale;
	bool zoom = false;

	if (_speaker) {
		// (only in the scene that is shown)
		if (!(_speaker->GetRef().GetLink(kCharacterScene) == gameControl()->GetScene()->GetRef()))
			return;

		if (_speaker->IsWalking() || !_placed)
			CalculateTextPos();

		alpha = _speaker->GetAlpha();
	} else {
		CalculateTextPos();
		zoom = true;
	}

	bool drawnByScript = false;

	if (!HookFunctionRender.IsEmpty()) {
		TArgument record;
		TArgument textLines;
		TArgument widths;
		TArgument position;
		TArgument alignment;
		TArgument alphaArgument;
		TArgument result;
		std::vector<TCharHolder> lines;

		for (const wxString &line : _lines)
			lines.push_back(TCharHolder(line.wc_str()));

		record.Set(_active);
		textLines.Set(lines);
		widths.Set(_lineWidths);
		position.Set(*_active.GetPoint(kTextPosition));
		alignment.Set(_active.GetInt(kTextAlignment));
		alphaArgument.Set(static_cast<double>(alpha));
		result.SetType(TArgType::kBool);

		std::vector<TArgument *> arguments = {&record, &textLines, &widths, &position, &alignment, &alphaArgument};
		std::vector<TArgument *> results = {&result};

		LuaDebugName("TextRenderHook");
		drawnByScript = LuaExecuteFunction(std::string(HookFunctionRender.mb_str()), arguments, results) &&
		                result.GetBool();
	}

	if (zoom)
		TCFont::ZoomText = true;

	// the lines are printed with automatic breaks unless the position comes from the data
	if (!drawnByScript) {
		g_pGameControl->GetFontManager()->PrintTextLines(_lines, _lineWidths, (TextAlignmentEnum)_active.GetInt(kTextAlignment),
		        *_active.GetPoint(kTextPosition), alpha, !_positionSet, &_buffers);
	}

	if (zoom)
		TCFont::ZoomText = false;
}

// Confirmed (asm lines 213593-213760)
void TGText::RegisterEventHandlerTextStarted(const wxString &name) {
	EventHandlerTextStarted = name;
}

void TGText::RegisterEventHandlerTextStopped(const wxString &name) {
	EventHandlerTextStopped = name;
}

wxString TGText::GetEventHandlerTextStarted() {
	return EventHandlerTextStarted;
}

wxString TGText::GetEventHandlerTextStopped() {
	return EventHandlerTextStopped;
}

void TGText::RegisterHookFunctionSetTextPosition(const wxString &name) {
	HookFunctionSetTextPosition = name;
}

void TGText::RegisterHookFunctionRender(const wxString &name) {
	HookFunctionRender = name;
}

void TGText::RegisterHookFunctionText(const wxString &name) {
	HookFunctionText = name;
}
