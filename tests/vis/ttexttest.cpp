#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "TTScene.h"
#include "TTText.h"
#include "vstables/eCommand.h"
#include "datastruct/visionaireobject.h"
#include "TSprite.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The records of texts, animations and sprites: the text of a language (found, made when missing), the languages of the
// text and of the speech, the mirrored flag of all the sprites of an animation, the position of a sprite.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;
	TVisObjRef gameRef = game.GetGame();
	TVisObjRef english = game.CreateObject(18, gameRef, kGameLanguages);
	english.SetName(TCharHolder("English"));
	TVisObjRef german = game.CreateObject(18, gameRef, kGameLanguages);
	german.SetName(TCharHolder("German"));

	TVisObjRef noParent;
	TTText text(game.CreateObject(14, noParent, -1));

	{
		std::vector<TTextLanguage> texts(1);

		texts[0].text = TCharHolder("hello");
		texts[0].languageId = PackVisId(english.GetId());
		text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);
	}

	int englishId = PackVisId(english.GetId());
	int germanId = PackVisId(german.GetId());

	// the language of the text
	TTText::SetLanguage(english);
	CHECK(text.GetTextString().ToStdWstring() == L"hello");
	CHECK(text.GetTextLanguage(englishId).text == TCharHolder("hello"));
	CHECK(text.GetTextLanguage(germanId).languageId == -1);

	// a language that has no text yet is made, and then found
	TTextLanguage &made = text.GetTextLanguageOrCreate(germanId);

	CHECK(made.languageId == germanId);
	made.text = TCharHolder("hallo");
	CHECK(text.GetTextLanguage(germanId).text == TCharHolder("hallo"));
	CHECK(&text.GetTextLanguageOrCreate(germanId) == &text.GetTextLanguage(germanId));
	{
		std::vector<TTextLanguage> *texts = nullptr;

		text.GetTexts(kTextTextLanguages, &texts);
		CHECK(texts->size() == 2);
		text.GetTextLanguageOrCreate(englishId);
		CHECK(texts->size() == 2);
	}
	TTText::SetLanguage(german);
	CHECK(text.GetTextString().ToStdWstring() == L"hallo");

	// the language of the speech is another one than the language of the text
	TTText::SetSpeechLanguage(english);
	CHECK(text.GetTextString().ToStdWstring() == L"hallo");

	// the animation: the mirrored flag of every sprite
	TVisObjRef animRef = game.CreateObject(9, noParent, -1);
	TTAnimation anim(animRef);
	std::vector<TSprite> sprites(3);

	sprites[1].SetMirrored(true);
	anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
	anim.SetMirrored(true);
	{
		std::vector<TSprite> got;

		anim.GetSprites(kAnimationSprites, got);
		CHECK(got.size() == 3);
		for (const TSprite &sprite : got)
			CHECK(sprite.IsMirrored());
	}
	anim.SetMirrored(false);
	{
		std::vector<TSprite> got;

		anim.GetSprites(kAnimationSprites, got);
		CHECK(got.size() == 3);
		for (const TSprite &sprite : got)
			CHECK(!sprite.IsMirrored());
	}

	// the sprite record
	TVisObjRef spriteRef = game.CreateObject(13, noParent, -1);
	TTSprite sprite(spriteRef);

	sprite.SetPosition(wxPoint{12, 34});
	CHECK((sprite.GetSprite(kSpriteSprite).GetPosition() == wxPoint{12, 34}));

	// the scene: the loading points, the name, the sounds of its actions
	{
		TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
		TTScene tscene(scene);
		std::vector<TCharHolder> sounds;
		auto isRect = [](const wxRect &r, int x, int y, int w, int h) {
			return r.x == x && r.y == y && r.width == w && r.height == h;
		};

		CHECK(tscene.GetNameLanguage().IsEmpty());

		// (the field of the points (0x87) is not in the scene of this version of the data: always no points)
		CHECK(isRect(tscene.GetLoadingRect(), -1, -1, -1, -1));

		// the name of the scene is a text
		TVisObjRef nameText = game.CreateObject(14, noParent, -1);
		std::vector<TTextLanguage> names(1);

		names[0].text = TCharHolder("The Hall");
		names[0].languageId = englishId;
		nameText.SetValue(kTextTextLanguages, names, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneName, nameText, true);
		TTText::SetLanguage(english);
		CHECK(tscene.GetNameLanguage().ToStdWstring() == L"The Hall");

		// the sounds
		TVisObjRef action = game.CreateObject(7, scene, kSceneActions);
		auto part = [&](int command, const wchar_t *file) {
			TVisObjRef p = game.CreateObject(8, action, kActionActionParts);

			p.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			p.SetValue(kActionPartPath, wxFileName(file), TSendEventEnum::kNoEvent);
		};

		part(kCommandPlaySound, L"sounds/a.ogg");
		part(kCommandWait, L"sounds/not.ogg");
		part(kCommandSceneMusic, L"music/m.ogg");
		part(kCommandLoopSound, L"sounds/loop.ogg");
		part(kCommandSetWalkingSound, L"sounds/step.ogg");
		part(kCommandPlaySound, L"");
		sounds.push_back(TCharHolder("stale"));
		tscene.GetSoundsOnScene(sounds);
		CHECK(sounds.size() == 4);
		if (sounds.size() == 4) {
			CHECK(sounds[0] == TCharHolder("sounds/a.ogg"));
			CHECK(sounds[1] == TCharHolder("music/m.ogg"));
			CHECK(sounds[3] == TCharHolder("sounds/step.ogg"));
		}
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
