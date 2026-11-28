#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "TSprite.h"
#include "baselib/xmlDom.h"
#include "datastruct/xmlProjectReader.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static void write(const char *name, const std::string &text) {
	std::ofstream out(name, std::ios::binary);

	out << text;
}

static std::string text(TVisionaireGame &game) {
	wxString xml = game.SaveDataGameToString();

	return std::string(xml.mb_str());
}

// The XML project: a project written as XML is read again into another project (the file by the two passes of a load,
// the text as a clipboard) and written again: the two texts have to be the same. And the parser of the document.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the parser
	{
		XmlDocument document;

		document.Parse("<?xml version=\"1.0\"?>\r\n<!-- c --><a x=\"1&amp;2\" y='&#65;&#x42;'>\r\n <b/><c k=\"v\"></c>\r\n</a>");
		XmlNode *a = document.GetFirstNode();

		CHECK(a && a->name == "a");
		CHECK(a->firstAttribute && a->firstAttribute->value == "1&2");
		CHECK(a->firstAttribute->next && a->firstAttribute->next->value == "AB");
		CHECK(a->firstChild && a->firstChild->name == "b");
		CHECK(a->firstChild->next && a->firstChild->next->name == "c");
		CHECK(a->firstChild->next->firstAttribute->name == "k");
		CHECK(!a->firstChild->next->next);

		bool failed = false;

		try {
			document.Parse("<a><b></a>");
		} catch (const XmlParseError &) {
			failed = true;
		}
		CHECK(failed);

		failed = false;
		try {
			document.Parse("<a x=1/>");
		} catch (const XmlParseError &) {
			failed = true;
		}
		CHECK(failed);
	}

	std::string original;

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();

		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test & <Company>"), TSendEventEnum::kNoEvent);

		TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
		scene.SetName(TCharHolder("Hall"));
		TVisObjRef other = game.CreateObject(4, gameRef, kGameSceneLinks);
		other.SetName(TCharHolder("Cellar"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef object = game.CreateObject(6, scene, kSceneObjects);
		object.SetName(TCharHolder("Door"));
		object.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> polygon = {{1, 2}, {3, 4}, {5, 6}};
		object.SetValue(kObjectPolygon, polygon, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, object, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);
		gameRef.SetLink(kGameFirstScene, other, true);

		TSprite sprite;

		sprite.SetPath(TCharHolder("pics/door.png"));
		sprite.SetPause(40);
		sprite.SetPosition(wxPoint{7, 8}, -1.0f);
		object.SetValue(kObjectSprite, sprite, TSendEventEnum::kNoEvent);
		std::vector<TSprite> sprites(3);

		for (int i = 0; i < 3; i++) {
			sprites[i].SetPath(TCharHolder("pics/frame.png"));
			sprites[i].SetPause(10 * (i + 1));
			sprites[i].SetPosition(wxPoint{i, -i}, -1.0f);
		}
		TVisObjRef noParent;
		TVisObjRef anim = game.CreateObject(9, noParent, -1);

		anim.SetName(TCharHolder("Walk"));
		anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);

		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);

		font.SetName(TCharHolder("Font"));
		std::vector<wxRect> rects;

		for (int k = 0; k < 4; k++) {
			wxRect r;

			r.x = k * 8;
			r.y = 1;
			r.width = 8;
			r.height = 12;
			rects.push_back(r);
		}
		font.SetValue(kFontLetters, rects, TSendEventEnum::kNoEvent);
		font.SetValue(kFontAlphabet, wxString(L"abcd"), TSendEventEnum::kNoEvent);

		TVisObjRef textRef = game.CreateObject(14, noParent, -1);
		std::vector<TTextLanguage> texts(2);

		texts[0].text = TCharHolder("hello <there>");
		texts[0].languageId = 5;
		texts[1].text = TCharHolder("hallo");
		texts[1].audioFile = TCharHolder("speech/hallo.ogg");
		texts[1].languageId = 6;
		textRef.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};

		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);

		original = text(game);
		write("proj.xml", original);
	}

	CHECK(original.size() > 1000);

	// the file, in the two passes
	{
		TVisionaireGame game;
		int version = 0;

		CHECK(game.Load(wxFileName(L"proj.xml"), wxString(), eSaveGame::kValue0, TLoadingTypeEnum::kValue2, &version, nullptr, nullptr));
		CHECK(version == 0xBA);

		std::string again = text(game);

		CHECK(again == original);
		if (again != original) {
			write("again.xml", again);
			printf("(the texts differ: %d and %d bytes; see again.xml)\n", (int)original.size(), (int)again.size());
		}

		// the made objects are there
		TVisObjRef gameRef = game.GetGame();

		CHECK(gameRef.GetStr(kGameGameName).ToStdWstring() == L"Test Game");
		CHECK(gameRef.GetStr(kGameCompanyName).ToStdWstring() == L"Test & <Company>");
		CHECK(gameRef.GetPoint(kGameWindowResolution)->x == 640);
	}

	// the two passes (type 0, then 1): the original reads every table in the first pass, as the second leaves out the Loadings
	// only, so the objects of the other tables are there twice (the game's data is binary: only that has a first pass of the
	// Loadings alone)
	{
		TVisionaireGame game;
		int version = 0;

		CHECK(game.Load(wxFileName(L"proj.xml"), wxString(), eSaveGame::kValue0, TLoadingTypeEnum::kValue0, &version, nullptr, nullptr));
		std::string first = text(game);

		CHECK(game.Load(wxFileName(L"proj.xml"), wxString(), eSaveGame::kValue0, TLoadingTypeEnum::kValue1, &version, nullptr, nullptr));
		CHECK(text(game).size() > first.size());
	}

	// the text, as a clipboard
	{
		TVisionaireGame game;
		TXMLProjectReader reader(&game);

		game.Clear();
		CHECK(reader.ReadXMLDoc(wxString(original.c_str())));
		game.SetupParents();
		game.SortLinks();
		std::string again = text(game);

		CHECK(again == original);
		if (again != original)
			write("again2.xml", again);

		// a wrong root
		TXMLProjectReader reader2(&game);

		CHECK(!reader2.ReadXMLDoc(wxString("<Other version=\"186\"/>")));
		CHECK(!reader2.ReadXMLDoc(wxString("<VisionaireAdventure/>")));
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
