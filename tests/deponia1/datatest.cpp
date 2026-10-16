#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "datastruct/data.h"
#include "datastruct/link.h"
#include "TSprite.h"
#include "TTextLanguage.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// A value of a kind, made with the engine's own functions.
struct Value {
	explicit Value(eTypeData kind) : type(kind), memory(new char[TData::GetDataSize(kind) + 16]()) {
		TData::CreateDataInstance(kind, memory);
	}
	~Value() {
		TData::DeleteDataInstance(type, memory, nullptr);
		delete[] memory;
	}
	eTypeData type;
	char *memory;
};

static std::wstring text(eTypeData type, void *value) {
	return TData::ToString(type, value, nullptr).ToStdWstring();
}

static unsigned long sizeOf(eTypeData type, void *value) {
	return TData::GetSizeMemory(type, value, false, nullptr);
}

// The values of the fields: the text that is shown for each kind, the sizes in memory, the comparison, the copy and the clear,
// and the reading of a value from the characters of a file.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the simple kinds
	{
		Value flag(eTypeData::kBool);
		Value number(eTypeData::kInt);
		Value name(eTypeData::kString);
		Value path(eTypeData::kPath);
		Value real(eTypeData::kFloat);

		GetRefBool(flag.memory) = true;
		GetRefInt(number.memory) = -42;
		GetRefString(name.memory) = TCharHolder("Hero");
		GetRefPath(path.memory) = TCharHolder("dir/file.png");
		GetRefFloat(real.memory) = 1.5f;

		CHECK(text(eTypeData::kBool, flag.memory) == L"true");
		GetRefBool(flag.memory) = false;
		CHECK(text(eTypeData::kBool, flag.memory) == L"false");
		CHECK(text(eTypeData::kInt, number.memory) == L"-42");
		CHECK(text(eTypeData::kString, name.memory) == L"Hero");
		CHECK(text(eTypeData::kPath, path.memory) == L"dir/file.png");
		CHECK(text(eTypeData::kFloat, real.memory) == L"1.500000");

		CHECK(sizeOf(eTypeData::kString, name.memory) == 5);
		CHECK(sizeOf(eTypeData::kPath, path.memory) == 13);
		CHECK(sizeOf(eTypeData::kInt, number.memory) == 0);
		CHECK(TData::SizePath(TCharHolder("abc")) == 4);

		// compared: equal is 0, anything else -1
		Value other(eTypeData::kInt);

		GetRefInt(other.memory) = -42;
		CHECK(TData::Compare(eTypeData::kInt, number.memory, other.memory) == 0);
		GetRefInt(other.memory) = 7;
		CHECK(TData::Compare(eTypeData::kInt, number.memory, other.memory) == -1);
		CHECK(TData::CompareDataWithValue(eTypeData::kInt, number.memory, other.memory) == -1);

		// copied, and cleared
		TData::Assign(eTypeData::kInt, other.memory, number.memory);
		CHECK(GetRefInt(other.memory) == -42);
		TData::ClearDataInstance(eTypeData::kInt, other.memory);
		// (an int that is cleared is -1, as a float)
		CHECK(GetRefInt(other.memory) == -1);
		TData::ClearDataInstance(eTypeData::kFloat, real.memory);
		CHECK(GetRefFloat(real.memory) == -1.0f);
		TData::ClearDataInstance(eTypeData::kBool, flag.memory);
		CHECK(!GetRefBool(flag.memory));
		TData::ClearDataInstance(eTypeData::kString, name.memory);
		CHECK(GetRefString(name.memory).size() == 0);
	}

	// the lists and the other values
	{
		Value rects(eTypeData::kRectList);
		Value points(eTypeData::kPointList);
		Value strings(eTypeData::kStringList);
		Value ints(eTypeData::kIntList);
		Value paths(eTypeData::kPathList);
		Value floats(eTypeData::kFloatList);
		Value point(eTypeData::kPoint);
		Value rect(eTypeData::kRect);

		GetRefVRect(rects.memory) = {wxRect{1, 2, 10, 20}, wxRect{0, 0, 1, 1}};
		GetRefVPoint(points.memory) = {wxPoint{1, 2}, wxPoint{3, 4}};
		GetRefVString(strings.memory) = {TCharHolder("a"), TCharHolder("bc")};
		GetRefVInt(ints.memory) = {1, 2, 3};
		GetRefVPath(paths.memory) = {TCharHolder("p/a")};
		GetRefVFloat(floats.memory) = {1.0f, 2.5f};
		GetRefPoint(point.memory) = wxPoint{3, 4};
		GetRefRect(rect.memory) = wxRect{1, 2, 10, 20};

		printf("rects: %ls | rect: %ls\n", text(eTypeData::kRectList, rects.memory).c_str(), text(eTypeData::kRect, rect.memory).c_str());
		CHECK(text(eTypeData::kRectList, rects.memory) == L"(1,2)-(10,21), (0,0)-(0,0)");
		CHECK(text(eTypeData::kPointList, points.memory) == L"(1,2), (3,4)");
		CHECK(text(eTypeData::kStringList, strings.memory) == L"a, bc");
		CHECK(text(eTypeData::kIntList, ints.memory) == L"1, 2, 3");
		CHECK(text(eTypeData::kPathList, paths.memory) == L"p/a");
		CHECK(text(eTypeData::kFloatList, floats.memory) == L"1.00, 2.50");
		CHECK(text(eTypeData::kPoint, point.memory) == L"3,4");
		CHECK(text(eTypeData::kRect, rect.memory) == L"1,2-10,21");

		// (the sizes are those of the original's layout)
		CHECK(sizeOf(eTypeData::kRectList, rects.memory) == 32);
		CHECK(sizeOf(eTypeData::kPointList, points.memory) == 16);
		CHECK(sizeOf(eTypeData::kStringList, strings.memory) == (0x10 + 2) + (0x10 + 3));
		CHECK(sizeOf(eTypeData::kIntList, ints.memory) == 12);
		CHECK(sizeOf(eTypeData::kPathList, paths.memory) == 0x10 + 4);
		CHECK(sizeOf(eTypeData::kFloatList, floats.memory) == 8);
		CHECK(sizeOf(eTypeData::kPoint, point.memory) == 0);

		// compared with copies, and with a change
		const eTypeData kinds[] = {eTypeData::kRectList, eTypeData::kPointList, eTypeData::kStringList, eTypeData::kIntList,
		                           eTypeData::kPathList, eTypeData::kFloatList, eTypeData::kPoint, eTypeData::kRect};
		void *memories[] = {rects.memory, points.memory, strings.memory, ints.memory, paths.memory, floats.memory, point.memory,
		                    rect.memory};

		for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
			Value copy(kinds[i]);

			TData::Assign(kinds[i], copy.memory, memories[i]);
			CHECK(TData::Compare(kinds[i], copy.memory, memories[i]) == 0);
			CHECK(text(kinds[i], copy.memory) == text(kinds[i], memories[i]));

			TData::ClearDataInstance(kinds[i], copy.memory);
			CHECK(TData::Compare(kinds[i], copy.memory, memories[i]) == -1);
		}

		GetRefVInt(ints.memory).push_back(4);
		Value differs(eTypeData::kIntList);

		GetRefVInt(differs.memory) = {1, 2, 3};
		CHECK(TData::Compare(eTypeData::kIntList, ints.memory, differs.memory) == -1);
	}

	// sprites
	{
		Value sprite(eTypeData::kSprite);
		Value sprites(eTypeData::kSpriteList);

		GetRefSprite(sprite.memory).SetPath(TCharHolder("a.png"));
		GetRefSprite(sprite.memory).SetPause(30);
		GetRefVSprite(sprites.memory) = {GetRefSprite(sprite.memory), GetRefSprite(sprite.memory)};

		CHECK(text(eTypeData::kSprite, sprite.memory) == GetRefSprite(sprite.memory).ToLuaString().ToStdWstring());
		CHECK(text(eTypeData::kSpriteList, sprites.memory).find(L"path='a.png'") != std::wstring::npos);
		CHECK(text(eTypeData::kSpriteList, sprites.memory).find(L", {path=") != std::wstring::npos);
		CHECK(sizeOf(eTypeData::kSprite, sprite.memory) == 6);
		CHECK(sizeOf(eTypeData::kSpriteList, sprites.memory) == 2 * (0x50 + 6));

		Value copy(eTypeData::kSprite);

		TData::Assign(eTypeData::kSprite, copy.memory, sprite.memory);
		CHECK(TData::Compare(eTypeData::kSprite, copy.memory, sprite.memory) == 0);
		GetRefSprite(copy.memory).SetPause(31);
		CHECK(TData::Compare(eTypeData::kSprite, copy.memory, sprite.memory) == -1);
	}

	// links (an object cannot be looked up here: only the ones that name none)
	{
		Value link(eTypeData::kLink);
		Value anyLink(eTypeData::kLink);
		Value links(eTypeData::kLinkList);

		CHECK(text(eTypeData::kLink, link.memory) == L"[empty]");
		GetRefLink(anyLink.memory) = TLink(true);
		CHECK(text(eTypeData::kLink, anyLink.memory) == L"[any]");
		CHECK(TData::Compare(eTypeData::kLink, link.memory, anyLink.memory) == -1);
		CHECK(TData::Compare(eTypeData::kLink, link.memory, link.memory) == 0);
		CHECK(text(eTypeData::kLinkList, links.memory).empty());

		GetRefLinks(links.memory) = {TLink(true), TLink(false)};
		CHECK(sizeOf(eTypeData::kLinkList, links.memory) == 16);

		Value same(eTypeData::kLinkList);

		GetRefLinks(same.memory) = {TLink(false), TLink(true)};
		// (the links are compared as a set: the same ones in another order are the same)
		CHECK(TData::Compare(eTypeData::kLinkList, links.memory, same.memory) == 0);
		GetRefLinks(same.memory).pop_back();
		CHECK(TData::Compare(eTypeData::kLinkList, links.memory, same.memory) == -1);
	}

	// texts: sized, but never equal
	{
		Value texts(eTypeData::kTextList);
		TTextLanguage one;

		one.text = TCharHolder("hello");
		one.audioFile = TCharHolder("hello.ogg");
		GetRefVText(texts.memory) = {one};

		CHECK(sizeOf(eTypeData::kTextList, texts.memory) == 6 + 10 + 0x28);
		CHECK(text(eTypeData::kTextList, texts.memory).empty());
		CHECK(TData::Compare(eTypeData::kTextList, texts.memory, texts.memory) == -1);
		CHECK(TData::Compare(eTypeData::kUnused5, texts.memory, texts.memory) == -1);
	}

	// the values from the characters of a file
	{
		Value flag(eTypeData::kBool);
		Value number(eTypeData::kInt);
		Value name(eTypeData::kString);
		Value path(eTypeData::kPath);
		Value real(eTypeData::kFloat);
		Value other(eTypeData::kRect);

		// (a flag is the letter T or F)
		const char *yes = "t";
		const char *no = "F";
		const char *digits = "12345";
		const char *words = "Hero";
		const char *file = "dir\\sub\\file.png";
		const char *decimal = "2.5";

		TData::SerializeParam(yes, yes + 1, eTypeData::kBool, flag.memory);
		CHECK(GetRefBool(flag.memory));
		TData::SerializeParam(no, no + 1, eTypeData::kBool, flag.memory);
		CHECK(!GetRefBool(flag.memory));
		TData::SerializeParam(digits, digits + 5, eTypeData::kInt, number.memory);
		CHECK(GetRefInt(number.memory) == 12345);
		TData::SerializeParam(digits, digits + 3, eTypeData::kInt, number.memory);
		CHECK(GetRefInt(number.memory) == 123);
		TData::SerializeParam(words, words + 4, eTypeData::kString, name.memory);
		CHECK(GetRefString(name.memory) == "Hero");
		TData::SerializeParam(file, file + std::strlen(file), eTypeData::kPath, path.memory);
		printf("path: %s\n", GetRefPath(path.memory).mb_str());
		CHECK(std::strchr(GetRefPath(path.memory).mb_str(), '\\') == nullptr);
		TData::SerializeParam(file, file, eTypeData::kPath, path.memory);
		CHECK(GetRefPath(path.memory).size() == 0);
		TData::SerializeParam(decimal, decimal + 3, eTypeData::kFloat, real.memory);
		CHECK(GetRefFloat(real.memory) == 2.5f);
		// (a kind that is not read from the characters is left as it is)
		TData::SerializeParam(decimal, decimal + 3, eTypeData::kRect, other.memory);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
