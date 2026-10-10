#include <cstdio>

#include "baselib/xmlWriter.h"
#include "datastruct/binaryProjectReader.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/visionaireobject.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	game.NewGame();
	TVisObjRef gameRef = game.GetGame();
	TVisObjRef font = game.CreateObject(3, gameRef, 0x75);
	font.SetName(TCharHolder("RoundTrip"));
	std::vector<wxRect> rects;
	for (int i = 0; i < 3; i++) {
		wxRect r;
		r.x = i * 10; r.y = 1; r.width = 5 + i; r.height = 12;
		rects.push_back(r);
	}
	font.SetValue(kFontLetters, rects, TSendEventEnum::kSendEvent);
	font.SetValue(kFontAlphabet, wxString(L"abc"), TSendEventEnum::kSendEvent);
	font.SetValue(kFontSpaceWidth, 7, TSendEventEnum::kSendEvent);
	font.SetValue(kFontKerning, wxString(L"ab=3"), TSendEventEnum::kSendEvent);

	wxFileName file(L"roundtrip.ved");
	CHECK(game.BinarySave(file, nullptr, false, nullptr));
	printf("written, %ld bytes\n", (long)wxFile(file.GetFullPath()).Length());

	TVisionaireGame loaded;
	CHECK(loaded.LoadDataGame(file, wxString(L""), TLoadingTypeEnum::kValue2, false, nullptr, nullptr));

	TVisObjRef loadedGame = loaded.GetGame();
	TVisObjRef again;
	bool found = loaded.GetObjectByName(wxString(L"RoundTrip"), 3, again);
	CHECK(found);
	if (found)
		printf("loaded: space %d alphabet [%ls] id %d\n", again.GetInt(kFontSpaceWidth), again.GetStr(kFontAlphabet).wc_str(), PackVisId(again.GetId()));
	printf("orig: space %d alphabet [%ls] id %d\n", font.GetInt(kFontSpaceWidth), font.GetStr(kFontAlphabet).wc_str(), PackVisId(font.GetId()));
	if (found) {
		CHECK(again.GetInt(kFontSpaceWidth) == 7);
		CHECK(again.GetStr(kFontAlphabet).ToStdWstring() == L"abc");
		CHECK(again.GetStr(kFontKerning).ToStdWstring() == L"ab=3");
		std::vector<wxRect> back;
		again.GetRects(kFontLetters, back);
		CHECK(back.size() == 3 && back[2].width == 7 && back[1].x == 10);
	}

	// a savegame into the buffer of the string writer
	TXMLStringWriter writer;
	CHECK(game.SaveSaveGame(writer));
	const unsigned char *data = writer.GetBuffer().GetData();
	printf("savegame buffer: %lu bytes, starts %c%c%c%c\n", writer.GetBuffer().GetLen(), data[0], data[1], data[2], data[3]);
	CHECK(data[0] == 'V' && data[1] == 'B' && data[2] == 'I' && data[3] == 'N');

	// a savegame of a character, loaded back over a game that has the character
	TVisObjRef character = game.CreateObject(0, gameRef, kGameCharacterLinks);
	character.SetName(TCharHolder("Hero"));
	character.SetValue(kCharacterDirection, 3, TSendEventEnum::kNoEvent);
	character.SetValue(kCharacterPosition, wxPoint{123, 45}, TSendEventEnum::kNoEvent);
	TXMLStringWriter saveWriter;
	CHECK(game.SaveSaveGame(saveWriter));
	{
		wxFile out;
		out.Open(wxString(L"hero.sav"), wxString(L"wb"));
		out.Write(saveWriter.GetBuffer().GetData(), saveWriter.GetBuffer().GetLen());
		out.Close();
	}
	character.SetValue(kCharacterDirection, 5, TSendEventEnum::kNoEvent);
	character.SetValue(kCharacterPosition, wxPoint{1, 2}, TSendEventEnum::kNoEvent);
	CHECK(game.LoadSaveGame(wxFileName(L"hero.sav"), wxString(L"")));
	printf("hero direction %d position %d,%d", character.GetInt(kCharacterDirection), character.GetPoint(kCharacterPosition)->x, character.GetPoint(kCharacterPosition)->y);
	CHECK(character.GetInt(kCharacterDirection) == 3);
	CHECK(character.GetPoint(kCharacterPosition)->x == 123 && character.GetPoint(kCharacterPosition)->y == 45);

	// the XML of the game
	wxString xml = game.SaveDataGameToString();
	printf("xml: %zu characters\n", xml.ToStdWstring().size());
	CHECK(xml.ToStdWstring().size() > 100);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
