#include "vscommon/fontManager.h"

#include "AppGlobals.h"
#include "vscommon/cfont.h"
#include "TSignalData.h"
#include "datastruct/visionaireobject.h"

TFontManager::~TFontManager() {
	for (TCFont *font : _fonts)
		delete font;
}

void TFontManager::Initialize(TVList &fonts) {
	// Confirmed (Deponia_Linux.asm lines 1390109-1390289).
	for (TVisionaireObject *obj : fonts) {
		TVisObjRef ref(obj);
		TCFont *font = new TCFont(ref, this);
		_fonts.push_back(font);
		_fontsById[PackVisId(obj->GetId())] = font;
	}
}

TCFont *TFontManager::GetFont(const TVisObjRef &font) const {
	auto it = _fontsById.find(PackVisId(font.GetId()));
	return it != _fontsById.end() ? it->second : nullptr;
}

void TFontManager::SetCurrentFont(const TVisObjRef &font) {
	auto it = _fontsById.find(PackVisId(font.GetId()));
	_currentFont = it != _fontsById.end() ? it->second : nullptr;
}

void TFontManager::SetCurrentFont(int &index) {
	// Confirmed (asm lines 1389221-1389266).
	if (_fonts.empty()) {
		_currentFont = nullptr;
		return;
	}
	int count = static_cast<int>(_fonts.size());
	if (index < 0)
		index = count - 1;
	else if (index >= count)
		index %= count;
	_currentFont = _fonts[index];
}

void TFontManager::PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float scale,
                             GLCharBuffer *buffer) {
	if (_currentFont != nullptr && !text.IsEmpty())
		_currentFont->PrintText(text, alignment, pos, scale, -1, buffer);
}

void TFontManager::PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
                                  TextAlignmentEnum alignment, const wxPoint &pos, float scale, bool wrap,
                                  std::vector<GLCharBuffer *> *buffers) {
	if (_currentFont != nullptr)
		_currentFont->PrintTextLines(lines, lineWidths, alignment, pos, scale, -1, wrap, buffers);
}

void TFontManager::GetTextDimension(const wxString &text, wxPoint &outSize) const {
	if (_currentFont != nullptr)
		_currentFont->GetTextDimension(text, outSize);
	else
		outSize = wxPoint();
}

void TFontManager::GetTextDimension(const std::list<wxString> &lines, wxPoint &outSize) const {
	if (_currentFont != nullptr)
		_currentFont->GetTextDimension(lines, outSize);
	else
		outSize = wxPoint();
}

int TFontManager::GetLineHeight() const {
	return _currentFont != nullptr ? _currentFont->GetLineHeight() : 0;
}

void TFontManager::PerformAutoLineBreak(const wxString &text, std::list<wxString> &outLines) {
	if (_currentFont != nullptr)
		_currentFont->PerformAutoLineBreak(text, outLines);
}

void TFontManager::SplitTexts(std::list<wxString> &texts, int maxWidth) {
	// Confirmed (Deponia_Linux.asm lines 1389467-1389654) - the manual list-
	// node splicing in the disassembly is just inlined std::list splice/
	// erase codegen, reproduced here with the equivalent standard calls.
	if (_currentFont == nullptr)
		return;
	for (auto it = texts.begin(); it != texts.end();) {
		std::list<wxString> lines;
		_currentFont->SplitIntoLines(*it, lines, maxWidth);
		if (lines.size() <= 1) {
			++it;
			continue;
		}
		it = texts.erase(it);
		texts.insert(it, lines.begin(), lines.end());
	}
}

void TFontManager::Signal(const TSignalData &signal, TSignalData &result) {
	// Confirmed (Deponia_Linux.asm lines 1389662-1390023). Any other signal
	// type hits an x_assert(false) in the original - skipped rather than
	// reproduced, matching this project's usual treatment of "impossible per
	// the caller" assertions.
	result = TSignalData{};
	if (signal.type == kSignalPrintText) {
		result.type = kSignalPrintText;
		if (_currentFont != nullptr && !signal.text.IsEmpty())
			_currentFont->PrintText(signal.text, TextAlignmentEnum::kCenter, signal.point, 1.0f, -1, nullptr);
	} else if (signal.type == kSignalPrintTextLines) {
		result.type = kSignalPrintTextLines;
		// Confirmed (asm lines 1389740-1389773): looks the font up by an
		// already-packed id, not via a TVisObjRef's own GetId() - distinct
		// from SetCurrentFont(const TVisObjRef&) above.
		auto it = _fontsById.find(signal.fontId);
		_currentFont = it != _fontsById.end() ? it->second : nullptr;

		std::list<wxString> lines;
		std::vector<int> lineWidths;
		wxStringTokenizer tok(signal.text, L'\n');
		while (tok.HasMoreTokens()) {
			wxString line = tok.GetNextToken();
			lines.push_back(line);
			wxPoint dim;
			if (_currentFont != nullptr)
				_currentFont->GetTextDimension(line, dim);
			else
				dim = wxPoint();
			lineWidths.push_back(dim.x);
		}

		if (_currentFont != nullptr) {
			bool savedMatrices = matricesActive;
			matricesActive = false;
			_currentFont->PrintTextLines(lines, lineWidths, TextAlignmentEnum::kCenter, signal.point, 1.0f, -1, true,
			                             nullptr);
			matricesActive = savedMatrices;
		}
	}
}
