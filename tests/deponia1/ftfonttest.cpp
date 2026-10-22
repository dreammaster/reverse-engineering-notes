#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "THGameControl.h"
#include "TSpriteHandle.h"
#include "datastruct/visionaireobject.h"
#include "graphicslib/freetypeFont.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/command.h"
#include "vstables/fieldIds.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s" "\n", __LINE__, #c); failures++; } } while (0)

// A backend that keeps what is drawn: the sprite of the glyph and where it goes.
class RecordingGraphics : public TGraphicsInterface {
public:
	struct Drawn {
		TSpriteHandle *sprite;
		float x;
		float y;
		float width;
		float height;
	};

	std::vector<Drawn> drawn;

	void Draw(TSpriteHandle *sprite, const wxRect &, const FloatRect &dest, float, bool, const unsigned int &, int, float,
	          const wxPoint &, float, float, int) override {
		drawn.push_back({sprite, dest.x, dest.y, dest.width, dest.height});
	}
};

static RecordingGraphics *recorder = nullptr;

/** What a string draws: one entry per glyph that has a bitmap (a space has none). */
static std::vector<RecordingGraphics::Drawn> render(TFreetypeFont &font, float x, float y, const char *utf8, float scale) {
	recorder->drawn.clear();
	font.RenderString(x, y, utf8, scale, -1, -1, nullptr, false);
	return recorder->drawn;
}

// The TrueType font of the game data (the one that has a font file first), the letter spacing and the heights of the glyphs, as
// Deponia 4 does them (src/deponia4): the advance of a space has the letter spacing in it (the first time it is asked for, after the
// spacing was set), the pen moves by the advance and the spacing again, and the top of a line is the highest glyph made so far
// until the height is locked. Run from the game directory with GAME_PASSW set to its password (tests/deponia1/README.md).
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	recorder = new RecordingGraphics();
	graphics = recorder;

	if (getenv("GAME_PASSW"))
		passw = wxString(getenv("GAME_PASSW"));

	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;
	bool pre = control->PreLoad(file, warning, true);

	CHECK(pre);
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));

	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);

	CHECK(loaded);
	if (!loaded) {
		printf("%d failures\n", failures);
		return 1;
	}

	wxString path;
	float size = 0.0f;
	TVList fonts;

	control->GetVisionaire()->GetList(3, fonts, false);
	for (TVisionaireObject *record : fonts) {
		wxFileName candidate = record->GetPath(kFontTrueTypeFontPath);

		if (!candidate.GetFullPath().IsEmpty()) {
			path = candidate.GetFullPath();
			size = record->GetFloat(kFontSize);
			break;
		}
	}

	CHECK(!path.IsEmpty() && size > 8.0f);
	if (path.IsEmpty()) {
		printf("%d failures\n", failures);
		return 1;
	}

	printf("font file '%ls', size %g\n", path.c_str(), size);

	const int spacing = 3;
	const int spaceBase = static_cast<int>(static_cast<double>(static_cast<int>(size)) / 4.2);

	// 1. A space: without letter spacing it is the size divided by 4.2; with the spacing set before it is first asked for, the
	// spacing is in the advance (Deponia 1 measures "abc" first and sets the spacing after it, which leaves the space without).
	{
		TFreetypeFont plain;

		CHECK(plain.CreateFont(path, size, 0xFFFFFFFF, false, 0.0f, 0xFF000000, false, wxPoint()));
		CHECK(plain.GetChar(0x20)->_advance == spaceBase);
		CHECK(plain.GetChar(0x09)->_advance == 2 * spaceBase);
		CHECK(plain.GetChar(0x0A)->_advance == 0 && plain.GetChar(0x0D)->_advance == 0);

		TFreetypeFont spaced;

		CHECK(spaced.CreateFont(path, size, 0xFFFFFFFF, false, 0.0f, 0xFF000000, false, wxPoint()));
		spaced.SetSpacing(spacing);

		int space = spaced.GetChar(0x20)->_advance;
		int a = spaced.GetChar('a')->_advance;

		CHECK(space == static_cast<int>(static_cast<double>(static_cast<int>(size)) / 4.2 + spacing));
		CHECK(space == spaceBase + spacing);
		CHECK(spaced.GetChar(0x09)->_advance == 2 * space + spacing);

		// the width of a text counts the spacing after every character, a space included (its advance and the spacing again)
		CHECK(spaced.StringLength("a") == a + spacing);
		CHECK(spaced.StringLength("a a") == 2 * (a + spacing) + space + spacing);
		CHECK(spaced.StringLength("aa\na") == 2 * (a + spacing));

		// 2. The pen when the string is drawn: from the first a to the second one is a, the spacing, the space, the spacing
		for (float scale : {1.0f, 2.0f}) {
			std::vector<RecordingGraphics::Drawn> glyphs = render(spaced, 100.0f, 50.0f, "a a", scale);

			CHECK(glyphs.size() == 2);
			if (glyphs.size() == 2) {
				CHECK(glyphs[0].sprite == glyphs[1].sprite && glyphs[0].sprite == spaced.GetChar('a')->_sprite);
				CHECK(std::fabs((glyphs[1].x - glyphs[0].x) - (static_cast<float>(a + spacing + space + spacing) * scale)) < 0.01f);
				CHECK(glyphs[0].y == glyphs[1].y);
				CHECK(glyphs[0].width == static_cast<float>(spaced.GetChar('a')->_width) * scale);
			}
		}

		// a new line goes down by the line height, back to the left
		std::vector<RecordingGraphics::Drawn> lines = render(spaced, 100.0f, 50.0f, "a\na", 1.0f);

		CHECK(lines.size() == 2);
		if (lines.size() == 2) {
			CHECK(lines[0].x == lines[1].x);
			CHECK(std::fabs((lines[1].y - lines[0].y) - static_cast<float>(spaced._lineHeight)) < 0.01f);
		}
	}

	// 3. The top of a line follows the highest glyph made so far (the game's fonts are not locked in Deponia 4).
	{
		TFreetypeFont open;

		CHECK(open.CreateFont(path, size, 0xFFFFFFFF, false, 0.0f, 0xFF000000, false, wxPoint()));

		std::vector<RecordingGraphics::Drawn> before = render(open, 10.0f, 20.0f, "x", 1.0f);
		int lowTop = open.GetChar('x')->_bearingY;

		CHECK(before.size() == 1);

		int highTop = open.GetChar('H')->_bearingY;

		CHECK(highTop > lowTop);

		std::vector<RecordingGraphics::Drawn> after = render(open, 10.0f, 20.0f, "x", 1.0f);

		CHECK(after.size() == 1);
		if (before.size() == 1 && after.size() == 1) {
			// the x was drawn at the top of the line (20) with the baseline lowTop below it; now the baseline is highTop below it
			CHECK(std::fabs(before[0].y - 20.0f) < 0.01f);
			CHECK(std::fabs((after[0].y - before[0].y) - static_cast<float>(highTop - lowTop)) < 0.01f);
		}

		// mixed heights in one string: both glyphs stand on the same baseline
		std::vector<RecordingGraphics::Drawn> mixed = render(open, 10.0f, 20.0f, "xH", 1.0f);

		CHECK(mixed.size() == 2);
		if (mixed.size() == 2) {
			float xBase = mixed[0].y + static_cast<float>(open.GetChar('x')->_bearingY);
			float hBase = mixed[1].y + static_cast<float>(open.GetChar('H')->_bearingY);

			CHECK(std::fabs(xBase - hBase) < 0.01f);
			CHECK(std::fabs(hBase - (20.0f + static_cast<float>(highTop))) < 0.01f);
		}
	}

	// 4. With the height locked (the editor's fonts, Deponia 1's game fonts after "abc") a taller glyph does not move the line.
	{
		TFreetypeFont locked;

		CHECK(locked.CreateFont(path, size, 0xFFFFFFFF, false, 0.0f, 0xFF000000, false, wxPoint()));

		std::vector<RecordingGraphics::Drawn> before = render(locked, 10.0f, 20.0f, "x", 1.0f);

		locked.LockHeight();
		CHECK(locked.GetChar('H')->_bearingY > locked.GetChar('x')->_bearingY);

		std::vector<RecordingGraphics::Drawn> after = render(locked, 10.0f, 20.0f, "x", 1.0f);

		CHECK(before.size() == 1 && after.size() == 1);
		if (before.size() == 1 && after.size() == 1)
			CHECK(before[0].y == after[0].y);

		// (a new CreateFont lets go of the lock)
		CHECK(locked.CreateFont(path, size, 0xFFFFFFFF, false, 0.0f, 0xFF000000, false, wxPoint()));
		render(locked, 10.0f, 20.0f, "x", 1.0f);
		locked.GetChar('H');

		std::vector<RecordingGraphics::Drawn> again = render(locked, 10.0f, 20.0f, "x", 1.0f);

		CHECK(again.size() == 1);
		if (again.size() == 1 && !before.empty())
			CHECK(again[0].y > before[0].y);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
