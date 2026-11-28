#include <cstdio>
#include <list>
#include <vector>

#include "AppGlobals.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/cfont.h"
#include "vscommon/fontManager.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The fonts made of a picture of letters: the width of a letter, the space between two (the letter spacing and the kerning of
// pairs and of single letters), the size of a text of several lines, the line height of a font that stands for another, and
// the kerning that is not valid.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();

	auto makeFont = [&](const char *name, const char *kerning, int vertical) {
		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);

		font.SetName(TCharHolder(name));

		std::vector<wxRect> letters;

		for (int i = 0; i < 5; i++)
			letters.push_back(wxRect{i * 10, 0, 5 + i, 12});

		font.SetValue(kFontLetters, letters, TSendEventEnum::kNoEvent);
		font.SetValue(kFontAlphabet, wxString(L"abcde"), TSendEventEnum::kNoEvent);
		font.SetValue(kFontLetterSpacing, 1, TSendEventEnum::kNoEvent);
		font.SetValue(kFontSpaceWidth, 4, TSendEventEnum::kNoEvent);
		font.SetValue(kVerticalLetterSpacing, vertical, TSendEventEnum::kNoEvent);
		if (kerning)
			font.SetValue(kFontKerning, wxString(kerning), TSendEventEnum::kNoEvent);
		return font;
	};

	TVisObjRef plain = makeFont("Plain", nullptr, 3);
	TVisObjRef kerned = makeFont("Kerned", "ab=2|c=3", 0);
	TVisObjRef both = makeFont("Both", "ab=2|a=7", 0);
	TVisObjRef broken = makeFont("Broken", "ab=2|a=1=2", 0);
	TVisObjRef partly = makeFont("Partly", "ab=x|c=3|abc=4", 0);
	TVisObjRef standIn = makeFont("StandIn", nullptr, 5);

	standIn.SetLink(kFontFont, plain, true);

	TVList fonts;

	game.GetList(3, fonts, false);

	TFontManager manager;

	manager.Initialize(fonts);
	CHECK(manager.GetFontCount() == 6);

	TCFont *plainFont = manager.GetFont(plain);
	TCFont *kernedFont = manager.GetFont(kerned);
	TCFont *bothFont = manager.GetFont(both);
	TCFont *brokenFont = manager.GetFont(broken);
	TCFont *partlyFont = manager.GetFont(partly);
	TCFont *standInFont = manager.GetFont(standIn);

	CHECK(plainFont && kernedFont && bothFont && brokenFont && partlyFont && standInFont);

	if (!plainFont || !kernedFont || !bothFont || !brokenFont || !partlyFont || !standInFont)
		return 1;

	// the width of a letter: that of its rectangle; the space has its own; a letter that is not there is the first (the space)
	CHECK(plainFont->GetCharWidth(L'a') == 5);
	CHECK(plainFont->GetCharWidth(L'e') == 9);
	CHECK(plainFont->GetCharWidth(L' ') == 4);
	CHECK(plainFont->GetCharWidth(L'z') == 4);
	CHECK(plainFont->GetLetter(L'c')._advance == 7);
	CHECK(plainFont->GetLetter(L'z')._advance == 4);

	// the spacing: the font's, or the kerning of the pair, or of the letter before
	CHECK(plainFont->GetCharSpacing(L'b', L'a') == 1);
	CHECK(kernedFont->GetCharSpacing(L'b', L'a') == -2);
	CHECK(kernedFont->GetCharSpacing(L'a', L'b') == 1);
	CHECK(kernedFont->GetCharSpacing(L'a', L'c') == -3);
	CHECK(kernedFont->GetCharSpacing(L'e', L'c') == -3);
	// (both: the pair is looked up and then the letter alone, which wins here and not when the text is measured)
	CHECK(bothFont->GetCharSpacing(L'b', L'a') == -7);

	// the size of a text
	wxPoint size;

	plainFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 5 + 1 + 6 && size.y == 12);
	plainFont->GetTextDimension(wxString(L"a b"), size);
	CHECK(size.x == 5 + 1 + 4 + 1 + 6);
	plainFont->GetTextDimension(wxString(), size);
	CHECK(size.x == 0 && size.y == 12);
	kernedFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 5 - 2 + 6);
	kernedFont->GetTextDimension(wxString(L"cd"), size);
	CHECK(size.x == 7 - 3 + 8);
	kernedFont->GetTextDimension(wxString(L"ca"), size);
	CHECK(size.x == 7 - 3 + 5);
	bothFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 5 - 2 + 6);

	// lines: the widest, and the height of all of them with the space between (the two letters that end a line, in any order)
	plainFont->GetTextDimension(wxString(L"ab\ncde"), size);
	CHECK(size.x == 7 + 1 + 8 + 1 + 9);
	CHECK(size.y == 12 + 12 + 3);
	plainFont->GetTextDimension(wxString(L"a\r\nb\n\rc\rd"), size);
	CHECK(size.y == 12 * 4 + 3 * 3);
	CHECK(size.x == 8);

	std::list<wxString> lines = {wxString(L"ab"), wxString(L"cde"), wxString(L"a")};

	plainFont->GetTextDimension(lines, size);
	CHECK(size.x == 7 + 1 + 8 + 1 + 9);
	CHECK(size.y == 12 * 3 + 3 * 2);

	// the kerning that is not valid: all of it goes with one pair that is wrong; a value that is not a number only leaves its pair
	brokenFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 5 + 1 + 6);
	CHECK(brokenFont->GetCharSpacing(L'b', L'a') == 1);
	partlyFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 5 + 1 + 6);
	partlyFont->GetTextDimension(wxString(L"cd"), size);
	CHECK(size.x == 7 - 3 + 8);

	// a font that stands for another: the other measures; the line height is the one of the end of the chain, with the
	// vertical spacing of the font that is asked
	standInFont->GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 12);
	CHECK(plainFont->GetLineHeight() == 12 + 3);
	CHECK(standInFont->GetLineHeight() == 12 + 3);

	// by the manager: nothing is selected, then a font
	CHECK(manager.GetCurrentFont() == nullptr);
	CHECK(manager.GetLineHeight() == 0);
	manager.GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 0 && size.y == 0);
	manager.SetCurrentFont(plain);
	CHECK(manager.GetCurrentFont() == plainFont);
	CHECK(manager.GetLineHeight() == 15);
	manager.GetTextDimension(wxString(L"ab"), size);
	CHECK(size.x == 12);
	manager.GetTextDimension(lines, size);
	CHECK(size.y == 12 * 3 + 3 * 2);

	int index = 2;

	manager.SetCurrentFont(index);
	CHECK(manager.GetCurrentFont() == manager.GetFonts()[2]);
	index = -1;
	manager.SetCurrentFont(index);
	CHECK(index == 5);
	index = 8;
	manager.SetCurrentFont(index);
	CHECK(index == 2);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
