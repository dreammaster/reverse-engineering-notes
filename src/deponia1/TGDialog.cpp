#include "TGDialog.h"

#include <list>
#include <string>

#include "AppGlobals.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/fontManager.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// The command of an action part that speaks a text, and the one that starts a dialog.
static const int kCommandSpeakText = 0x17;
static const int kCommandStartDialog = 0x0D;

// Confirmed (asm lines 220104-220404)
TGDialog::TGDialog() {
	_hovered = -1;
	_canScrollUp = false;
	_canScrollDown = false;
	_hoverUp = false;
	_hoverDown = false;
	_scroll = 0;
	_lineSpacing = 0;
}

TGDialog::~TGDialog() {
}

// Confirmed (asm lines 220406-220527): the dialog is forgotten, and so are its lines.
void TGDialog::Clear() {
	TTDialog::Clear();

	_hovered = -1;
	_scroll = 0;
	_lines.clear();
	_lineRects.clear();
	_lineParts.clear();
	_parts.clear();
}

// Confirmed (asm lines 220534-220640)
void TGDialog::SetScrollButtons() {
	_canScrollUp = (_scroll > 0);
	_canScrollDown = (!_lineRects.empty() && _lineRects.back().GetBottom() > _area.GetBottom());
}

// Confirmed (asm lines 220650-220800): the lines are of the part under the mouse when the mouse is
// on one of them (and in the area: a line that is scrolled out of it cannot be picked).
void TGDialog::HandleMouseMove(const wxPoint &pos) {
	_hovered = -1;

	_hoverUp = _canScrollUp ? _activeScrollUp.GetDestRect().Contains(pos) : false;
	_hoverDown = _canScrollDown ? _activeScrollDown.GetDestRect().Contains(pos) : false;

	if (!_area.Contains(pos))
		return;

	for (size_t i = 0; i < _lineRects.size(); i++) {
		const wxRect &rect = _lineRects[i];

		if (rect.Contains(pos) && rect.GetTop() >= _area.GetTop() && rect.GetBottom() <= _area.GetBottom()) {
			_hovered = _lineParts[i];
			return;
		}
	}
}

// Confirmed (asm lines 222080-222100)
TVisObjRef TGDialog::GetDialogCharacter() const {
	TVisObjRef ref(*this);

	while (!ref.IsEmpty() && ref.GetId()[3] != 0)
		ref = ref.GetParent();

	return ref;
}

bool TGDialog::HasAvailablePart(const TVisObjRef &dialog) {
	TVList parts;

	dialog.GetLinks(kDialogDialogParts, TypeOrder::kValue0, parts);

	for (TVisionaireObject *part : parts) {
		if (part->GetBool(kDialogPartAvailable))
			return true;
	}

	return false;
}

void TGDialog::AddActionPart(TVisObjRef &action, const TVisObjRef &link, int command, const TVisObjRef &speaker,
                             TMoveOrderEnum order) {
	TVisObjRef part = gameControl()->GetVisionaire()->CreateObject(8, action, kActionActionParts);

	part.SetLink(kActionPartLink, link, false);
	part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);

	if (!speaker.IsEmpty())
		part.SetLink(kActionPartAltLink, speaker, false);

	part.ChangeOrder(order);
}

// Confirmed (asm lines 220806-222075): an arrow scrolls by a line (the height of the line that
// goes by and the space between lines), a part that is picked builds the action the game runs
// (see the header) and ends the dialog.
void TGDialog::HandleMouseClick() {
	if (_hoverUp) {
		if (!_lineRects.empty() && _scroll > 0 && _scroll <= (int)_lineRects.size()) {
			int shift = _lineSpacing + _lineRects[_scroll - 1].GetHeight();

			for (wxRect &rect : _lineRects)
				rect.SetTop(rect.GetTop() + shift);

			_scroll--;
			SetScrollButtons();
		}

		return;
	}

	if (_hoverDown) {
		if (!_lineRects.empty() && _scroll < (int)_lineRects.size()) {
			int shift = _lineSpacing + _lineRects[_scroll].GetHeight();

			for (wxRect &rect : _lineRects)
				rect.SetTop(rect.GetTop() - shift);

			_scroll++;
			SetScrollButtons();
		}

		return;
	}

	if (_hovered == -1 || _hovered >= (int)_parts.size())
		return;

	TVisObjRef part(_parts.at(_hovered));
	TVisObjRef character = GetDialogCharacter();
	TVisionaire *visionaire = gameControl()->GetVisionaire();

	// a part that is used up can not be picked again
	if (part.GetBool(kDialogPartRemove))
		part.SetValue(kDialogPartAvailable, false, TSendEventEnum::kNoEvent);

	TVisObjRef partAction = part.GetLink(kDialogPartAction);
	TVisObjRef linkedAction = part.GetLink(kDialogPartLinkedAction);

	// the game's dialog action is made new from the actions of the part
	TVisObjRef game = visionaire->GetGame();
	TVisObjRef gameAction = game.GetLink(kGameDialogAction);

	if (gameAction.IsEmpty()) {
		gameAction = visionaire->CreateObject(7, game, kGameDialogAction);
	} else {
		TVList old;

		gameAction.GetLinks(kActionActionParts, TypeOrder::kValue1, old);

		for (TVisionaireObject *oldPart : old)
			gameAction.RemoveLink(kActionActionParts, oldPart->GetTId(), false);
	}

	TVisObjRef actions[2] = {partAction, linkedAction};

	for (TVisObjRef &action : actions) {
		if (action.IsEmpty())
			continue;

		TVList parts;

		action.GetLinks(kActionActionParts, TypeOrder::kValue1, parts);

		for (TVisionaireObject *actionPart : parts)
			visionaire->CopyObject(TVisObjRef(actionPart), gameAction, kActionActionParts);
	}

	// (the action that is already for this part - the player picked it again - is not changed)
	TVList present;
	bool same = false;

	gameAction.GetLinks(kActionActionParts, TypeOrder::kValue1, present);

	if (!present.empty()) {
		TVisObjRef first(present.front());

		same = (!first.IsEmpty() && first.GetLink(kActionPartLink) == part);
	}

	if (!same) {
		// the character says what the player picked, and the other one answers
		TVisObjRef answer = part.GetLink(kDialogPartAnswerText);

		if (!answer.IsEmpty())
			AddActionPart(gameAction, answer, kCommandSpeakText, character, TMoveOrderEnum::kFirst);

		TVisObjRef reply = part.GetBool(kDialogPartUseAltText) ? part.GetLink(kDialogPartAltText)
		                   : part.GetLink(kDialogPartText);

		if (!reply.IsEmpty())
			AddActionPart(gameAction, reply, kCommandSpeakText, _partner, TMoveOrderEnum::kFirst);

		// and what goes on: the next dialog, or by the part's "return" setting this one again or the one above
		TVisObjRef nextDialog = part.GetLink(kDialogPartNextDialog);
		bool done = false;

		if (!nextDialog.IsEmpty() && HasAvailablePart(nextDialog)) {
			AddActionPart(gameAction, nextDialog, kCommandStartDialog, TVisObjRef(), TMoveOrderEnum::kLast);
			done = true;
		}

		if (!done) {
			int returnTo = part.GetInt(kDialogPartReturn);

			if (returnTo == 0 && HasAvailablePart(*this)) {
				AddActionPart(gameAction, *this, kCommandStartDialog, TVisObjRef(), TMoveOrderEnum::kLast);
			} else if (returnTo == 0 || returnTo == 1) {
				// (a part that returns to the same dialog with nothing left to pick goes up like 1 does)
				TVisObjRef above = GetParent().GetParent();

				if (!above.IsEmpty() && above.GetId()[3] == 0x0B && HasAvailablePart(above))
					AddActionPart(gameAction, above, kCommandStartDialog, TVisObjRef(), TMoveOrderEnum::kLast);
			}
		}
	}

	TGAction::AddRunningAction(gameAction);
	gameControl()->EndDialog();
}

// Confirmed (asm lines 222102-222140 and the wheel handler): the wheel does what a click on the
// arrow does, and is only taken every 10 ms.
void TGDialog::HandleMouseWheel(TMouseMessageEnum msg) {
	if (_wheelTimer.GetTime() <= 9)
		return;

	if (msg == TMouseMessageEnum::kValue12) {
		if (_canScrollUp)
			_hoverUp = true;
	} else if (msg == TMouseMessageEnum::kValue13) {
		if (_canScrollDown)
			_hoverDown = true;
	}

	if (_hoverUp || _hoverDown)
		HandleMouseClick();

	_wheelTimer.SetTime();
}

// Confirmed (asm lines 220406-220800 region, Draw): the background, the lines in the area (the
// part under the mouse in the active font, unless the mouse is on an arrow) and the arrows.
void TGDialog::Draw() {
	_background.Draw(1.0f, 0xFFFFFFFF);

	TFontManager *fonts = g_pGameControl->GetFontManager();

	for (size_t i = 0; i < _lines.size(); i++) {
		const wxRect &rect = _lineRects[i];

		if (rect.GetTop() < _area.GetTop() || rect.GetBottom() > _area.GetBottom())
			continue;

		bool hovered = (_lineParts[i] == _hovered && !_hoverUp && !_hoverDown);

		fonts->SetCurrentFont(hovered ? _activeFont : _normalFont);
		fonts->PrintText(_lines[i], TextAlignmentEnum::kLeft, wxPoint{rect.GetLeft(), rect.GetTop()}, 1.0f, nullptr);
	}

	if (_canScrollUp)
		(_hoverUp ? _activeScrollUp : _inactiveScrollUp).Draw(1.0f, 0xFFFFFFFF);

	if (_canScrollDown)
		(_hoverDown ? _activeScrollDown : _inactiveScrollDown).Draw(1.0f, 0xFFFFFFFF);
}

// Confirmed (asm lines 222140-223152)
void TGDialog::SetDialog(const TVisObjRef &dialog) {
	Clear();

	if (dialog.IsEmpty())
		return;

	TVisObjRef::operator=(dialog);

	_partner = gameControl()->GetCurrentCharacter()->GetRef();
	_activeFont = _partner.GetLink(kCharacterActiveDialogFont);
	_normalFont = _partner.GetLink(kCharacterInactiveDialogFont);
	_area = *_partner.GetRect(kCharacterDialogArea);

	_activeScrollUp.Set(_partner.GetSprite(kCharacterDialogActiveScrollUp));
	_inactiveScrollUp.Set(_partner.GetSprite(kCharacterDialogInactiveScrollUp));
	_activeScrollDown.Set(_partner.GetSprite(kCharacterDialogActiveScrollDown));
	_inactiveScrollDown.Set(_partner.GetSprite(kCharacterDialogInactiveScrollDown));
	_background.Set(_partner.GetSprite(kCharacterDialogSprite));
	_lineSpacing = _partner.GetInt(kCharacterDialogVerticalSpace);

	_activeScrollUp.RefreshSprite(false);
	_inactiveScrollUp.RefreshSprite(false);
	_activeScrollDown.RefreshSprite(false);
	_inactiveScrollDown.RefreshSprite(false);

	// a character without a (valid) area has the whole window
	if (_area.GetTop() == -1 || _area.GetBottom() <= _area.GetTop()) {
		int width = 0, height = 0;

		g_pGameControl->GetWindowSize(&width, &height);
		_area = wxRect{0, 0, width, height};
	}

	TFontManager *fonts = g_pGameControl->GetFontManager();

	fonts->SetCurrentFont(_normalFont);

	// the parts the player can pick now, cut in the lines that fit the area
	TVList parts;
	int index = 0;

	GetList(kDialogDialogParts, parts);

	for (TVisionaireObject *object : parts) {
		TVisObjRef part(object);

		if (!part.GetBool(kDialogPartAvailable))
			continue;

		TVisObjRef condition = part.GetLink(kDialogPartCondition);

		if (!condition.IsEmpty() && TTCondition(condition).IsTrue() == part.GetBool(kDialogPartConditionNegate))
			continue;

		// (the Replace comes before the pause tags are looked for - and takes the '<' of them -, as in the original)
		wxString text = TTText(part.GetLink(kDialogPartText)).GetTextString();

		text.Replace(wxString(L"<"), wxString(L"\n"), true);
		TTText::ReplaceValues(text, GetVisionaire());

		std::wstring plain = text.ToStdWstring();
		std::size_t tag = plain.find(L"<p");

		if (tag != std::wstring::npos)
			text = wxString(plain.substr(0, tag));

		std::list<wxString> texts;

		texts.push_back(text);
		fonts->SplitTexts(texts, _area.GetWidth());

		for (const wxString &line : texts) {
			_lines.push_back(line);
			_lineRects.push_back(wxRect());
			_lineParts.push_back(index);
		}

		_parts.push_back(part);
		index++;
	}

	// the lines go one under the other
	wxPoint pos{_area.GetLeft(), _area.GetTop()};

	for (size_t i = 0; i < _lines.size(); i++) {
		wxPoint size;

		fonts->GetTextDimension(_lines[i], size);

		_lineRects[i].SetLeft(pos.x);
		_lineRects[i].SetTop(pos.y);
		_lineRects[i].SetWidth(size.x);
		_lineRects[i].SetHeight(size.y);

		pos.y += size.y + _lineSpacing;
	}

	wxRect destRect;
	FloatRect srcRect;

	_activeScrollUp.PreparePaint(destRect, srcRect);
	_inactiveScrollUp.PreparePaint(destRect, srcRect);
	_activeScrollDown.PreparePaint(destRect, srcRect);
	_inactiveScrollDown.PreparePaint(destRect, srcRect);

	SetScrollButtons();
}
