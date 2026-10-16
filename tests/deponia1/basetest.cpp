#include <cstdio>
#include <cstring>
#include <string>

#include "AppGlobals.h"
#include "TCharHolder.h"
#include "TMemoryBuffer.h"
#include "TSprite.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/visionaireGame.h"
#include "zipdata.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The small classes everything is made of: the string handle, the growing byte buffer (and its compression and cipher), and
// the sprite description with its Lua form.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// --- TCharHolder
	{
		TCharHolder none;
		TCharHolder hello("Hello");
		TCharHolder shout("HELLO");
		TCharHolder wide(L"Hello");

		CHECK(!none.IsOk());
		CHECK(none.size() == 0);
		CHECK(hello.IsOk());
		CHECK(hello.size() == 6);
		CHECK(hello == wide);
		CHECK(hello == "Hello");
		CHECK(!(hello == "hello"));
		CHECK(hello == L"Hello");
		CHECK(!(hello == L"Help"));
		CHECK(!(none == L"Hello"));
		CHECK(hello == wxString(L"Hello"));
		CHECK(hello != shout);

		// comparisons: a handle with nothing is less than any other
		CHECK(none.Cmp(none) == 0);
		CHECK(none.Cmp(hello) == -1);
		CHECK(hello.Cmp(none) == 1);
		CHECK(hello.Cmp(shout) == 1);
		CHECK(shout.Cmp(hello) == -1);
		CHECK(hello.CmpNoCase(shout) == 0);
		CHECK(none.CmpNoCase(none) == 0);
		CHECK(none.CmpNoCase(hello) == -1);
		CHECK(hello.CmpNoCase(none) == 1);
		CHECK(TCharHolder("abc").CmpNoCase(TCharHolder("ABD")) == -1);
		CHECK(TCharHolder("ABD").CmpNoCase(TCharHolder("abc")) == 1);
		CHECK(TCharHolder("abc").CmpNoCase(TCharHolder("ABCD")) == -1);
		CHECK(TCharHolder("ABCD").CmpNoCase(TCharHolder("abc")) == 1);

		// the text of it
		CHECK(hello.Lower().ToStdWstring() == L"hello");
		CHECK(none.Lower().IsEmpty());
		CHECK(hello.GetFullPath(1).ToStdWstring() == L"Hello");
		CHECK(std::strcmp(hello.mb_str(), "Hello") == 0);
		CHECK(std::strcmp(none.mb_str(), "") == 0);

		double number = 0;

		CHECK(TCharHolder("12.5").ToDouble(&number) && number == 12.5);
		CHECK(!none.ToDouble(&number));
		CHECK(!hello.ToDouble(&number));

		// a range: a terminator is added when the range does not end with one
		const char text[] = "range";
		TCharHolder withoutEnd(text, text + 5);
		TCharHolder withEnd(text, text + 6);

		CHECK(withoutEnd == "range");
		CHECK(withoutEnd.size() == 6);
		CHECK(withEnd == "range");
		CHECK(withEnd.size() == 6);

		TCharHolder assigned;

		assigned.Assign(text, text + 3);
		CHECK(assigned == "ran");
		assigned.Assign(text, text);
		CHECK(!assigned.IsOk());
		assigned.Assign(nullptr, nullptr);
		CHECK(!assigned.IsOk());
		assigned.Assign(wxString(L"from a string"));
		CHECK(assigned == "from a string");
		assigned = "plain";
		CHECK(assigned == "plain");
		assigned = wxFileName(L"dir/file.png");
		CHECK(assigned.GetFullPath().Contains(L"file.png"));
		assigned = hello;
		CHECK(assigned == hello);
		assigned.copy("copied");
		CHECK(assigned == "copied");
		assigned.copy("abcdef", 3);
		CHECK(assigned.size() == 3);
		assigned.copy("", 3);
		CHECK(!assigned.IsOk());
		assigned.resize(10);
		CHECK(assigned.size() == 10);
		assigned.resize(0);
		CHECK(!assigned.IsOk());

		TCharHolder first("first");
		TCharHolder second("second");

		first.exchange(second);
		CHECK(first == "second");
		CHECK(!second.IsOk());
		CHECK(TCharHolder("a").SameAs(TCharHolder("a")));
		CHECK(TCharHolder(wxFileName(L"a.png")).IsOk());
	}

	// --- TMemoryBuffer
	{
		TMemoryBuffer buffer;

		CHECK(buffer.GetLen() == 0);
		CHECK(buffer.GetData() == nullptr);

		// reserved: the buffer is there and empty; a smaller size keeps it, a larger one makes a new one
		buffer.Reserve(16);
		CHECK(buffer.GetData() != nullptr);
		CHECK(buffer.GetLen() == 0);
		buffer << 'a' << 'b';
		CHECK(buffer.GetLen() == 2);
		buffer.Reserve(8);
		CHECK(buffer.GetLen() == 2);
		buffer.Reserve(1 << 23);
		CHECK(buffer.GetLen() == 0);

		// grown with the content kept
		buffer.ClearMemory();
		buffer << "abcd";
		buffer.EnsureBufferSize(1 << 24);
		CHECK(buffer.GetLen() == 4);
		CHECK(std::memcmp(buffer.GetData(), "abcd", 4) == 0);

		TMemoryBuffer fresh;

		fresh.EnsureBufferSize(32);
		CHECK(fresh.GetData() != nullptr);
		CHECK(fresh.GetLen() == 0);

		// the numbers are written the big end first
		TMemoryBuffer numbers;

		numbers << static_cast<unsigned char>(0x7F) << static_cast<short>(-2) << static_cast<unsigned short>(0x1234)
		        << static_cast<long>(0x01020304) << static_cast<unsigned long>(0xA0B0C0D0ul);
		CHECK(numbers.GetLen() == 1 + 2 + 2 + 4 + 4);

		const unsigned char expected[] = {0x7F, 0xFF, 0xFE, 0x12, 0x34, 1, 2, 3, 4, 0xA0, 0xB0, 0xC0, 0xD0};

		CHECK(numbers.GetLen() == sizeof(expected) && std::memcmp(numbers.GetData(), expected, sizeof(expected)) == 0);

		// the texts: the bytes of it, or with its length before
		TMemoryBuffer texts;

		texts << wxString(L"abc") << wxFileName(L"x.png");
		CHECK(texts.GetLen() == 3 + 5);
		texts.ClearMemory();
		texts.AppendStringWithLen(wxString(L"hello"));
		CHECK(texts.GetLen() == 2 + 5);
		CHECK(texts.GetData()[0] == 0 && texts.GetData()[1] == 5 && texts.GetData()[2] == 'h');

		// the contents released
		texts.ReleaseMemory();
		CHECK(texts.GetData() == nullptr && texts.GetLen() == 0);

		// compressed and uncompressed again (into another buffer, whose length is not set, and in itself)
		TMemoryBuffer packed;

		packed.AppendData(kPacked_fixed, sizeof(kPacked_fixed));

		TMemoryBuffer plain;

		CHECK(packed.Uncompress(plain, (long)sizeof(kPlain_fixed)));
		CHECK(std::memcmp(plain.GetData(), kPlain_fixed, sizeof(kPlain_fixed)) == 0);
		CHECK(plain.GetLen() == 0);
		CHECK(packed.Uncompress((long)sizeof(kPlain_fixed)));
		CHECK(packed.GetLen() == sizeof(kPlain_fixed));
		CHECK(std::memcmp(packed.GetData(), kPlain_fixed, sizeof(kPlain_fixed)) == 0);

		// (not a zlib stream: the first fails and says so, the second keeps what it has)
		TMemoryBuffer garbage;

		garbage << "this is not packed";

		TMemoryBuffer target;

		CHECK(!garbage.Uncompress(target, 100));
		CHECK(!garbage.Uncompress(100));
		CHECK(garbage.GetLen() == 18);

		TMemoryBuffer toPack;

		toPack.AppendData(kPlain_dynamic, sizeof(kPlain_dynamic));
		CHECK(toPack.Compress());
		CHECK(toPack.GetLen() > 0);
		CHECK(toPack.Uncompress((long)sizeof(kPlain_dynamic)));
		CHECK(toPack.GetLen() == sizeof(kPlain_dynamic));
		CHECK(std::memcmp(toPack.GetData(), kPlain_dynamic, sizeof(kPlain_dynamic)) == 0);

		// the cipher: twice is the text again; a part of it from an offset leaves the start alone
		TMemoryBuffer secret;

		secret << "the quick brown fox jumps over the lazy dog";

		unsigned long length = secret.GetLen();

		CHECK(secret.Decrypt(wxString(L"key"), nullptr, 0));
		CHECK(std::memcmp(secret.GetData(), "the quick", 9) != 0);
		CHECK(secret.Encrypt(wxString(L"key"), nullptr, 0));
		CHECK(secret.GetLen() == length);
		CHECK(std::memcmp(secret.GetData(), "the quick brown fox jumps over the lazy dog", length) == 0);
		CHECK(secret.Decrypt(wxString(L"key"), nullptr, 10));
		CHECK(std::memcmp(secret.GetData(), "the quick ", 10) == 0);
		CHECK(std::memcmp(secret.GetData() + 10, "brown fox", 9) != 0);
	}

	// --- TSprite
	{
		TSprite sprite;

		sprite.SetPath(TCharHolder("pics/a.png"));
		sprite.SetPause(40);
		sprite.SetTransparency(eTransparencyMode::kColorKey, 0x00FF00);
		sprite.SetPosition(wxPoint{3, 4}, 100.0f);

		TSprite same(sprite);

		CHECK(same == sprite);
		CHECK(!same.CmpReloadNeeded(sprite));
		CHECK(same.IsTransparencyEqual(sprite));

		// what needs the picture to be loaded again: the file, the way it is transparent, the colour of that
		TSprite other(sprite);

		other.SetPath(TCharHolder("pics/b.png"));
		CHECK(other.CmpReloadNeeded(sprite));
		CHECK(!(other == sprite));
		other = sprite;
		other.SetTransparency(eTransparencyMode::kAlpha, 0);
		CHECK(other.CmpReloadNeeded(sprite));
		// (only the colours are compared when the other one is a colour key, whatever the mode of this one)
		CHECK(other.IsTransparencyEqual(sprite));
		other.SetTransparency(eTransparencyMode::kColorKey, 0x0000FF);
		CHECK(other.CmpReloadNeeded(sprite));
		CHECK(!other.IsTransparencyEqual(sprite));
		other = sprite;
		other.SetPause(80);
		CHECK(!other.CmpReloadNeeded(sprite));
		CHECK(!(other == sprite));
		CHECK(TSprite(wxFileName(L"pics/c.png")).GetPath().GetFullName().ToStdWstring() == L"c.png");

		// "any" transparency is equal to every one
		TSprite any;

		any.SetTransparency(eTransparencyMode::kAny, 0);
		CHECK(sprite.IsTransparencyEqual(any));

		// mirrored
		CHECK(!sprite.IsMirrored());
		sprite.SetMirrored(true);
		CHECK(sprite.IsMirrored());
		CHECK(!(sprite == same));
		sprite.SetMirrored(false);
		CHECK(sprite == same);
		CHECK(!sprite.IsLoadedImage());
		sprite.SetLoadedImage();
		CHECK(sprite.IsLoadedImage());

		// the Lua text
		wxString lua = sprite.ToLuaString();

		printf("%ls\n", lua.ToStdWstring().c_str());
		CHECK(lua.Contains(L"path='pics/a.png'"));
		CHECK(lua.Contains(L"position={x=3,y=4}"));
		CHECK(lua.Contains(L"eTransparencyColorKey"));
		CHECK(lua.Contains(L"pause=40"));

		TSprite alpha;

		alpha.SetTransparency(eTransparencyMode::kAlpha, 0);
		CHECK(alpha.ToLuaString().Contains(L"eTransparencyAlpha"));
		alpha.SetTransparency(eTransparencyMode::kNone, 0);
		CHECK(alpha.ToLuaString().Contains(L"eTransparencyNone"));
		alpha.SetTransparency(eTransparencyMode::kAny, 0);
		CHECK(alpha.ToLuaString().Contains(L"eTransparencyUndefined"));

		// (reading it back always says yes and leaves the sprite as it was: see the comment of the function)
		TVisionaireGame game;

		InitLua(&game, wxString(L"."), wxString(L"."), wxString());

		TSprite back;

		CHECK(back.SetFromLuaString(wxString(L"return " + lua.ToStdWstring())));

		// cleared
		sprite.Clear();
		CHECK(sprite.IsEmpty());
		CHECK(sprite.GetPause() == -1);
		CHECK(sprite.GetTransparency() == eTransparencyMode::kAlpha);
		CHECK(sprite.GetSize() == 100.0f);
		CHECK(sprite.GetPosition() == wxPoint{});
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
