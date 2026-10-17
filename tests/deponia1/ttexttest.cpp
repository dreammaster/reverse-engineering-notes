#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "TTText.h"
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

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
