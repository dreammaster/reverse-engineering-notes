// Original path confirmed via x_assert() calls: src/datastruct/data.cpp - see
// manifest/source_layout.tsv.
//
// TData is the typed-value storage layer under every game-data record
// (TDataGroup, TVisionaireObject). A record's fields live side by side in one
// block of raw memory; each field has an eTypeData (datastruct/type.h) and a
// byte offset (TTypeData::GetOffset()), and TData's static methods operate on
// "the value of type T at this address" with a switch over the 19 kinds:
// construct, destroy, reset, assign, compare, print and write it out. The
// GetRef...() free functions at the top are the typed views of such an
// address.
//
// Confirmed (Deponia_Linux.asm lines 623914-625860 for the 19 GetRef...()
// helpers and 625861-635588 for the 13 TData methods).
//
// Deviations: in the original each kind has a fixed byte size
// (CSWTCH_408: 1, 4, 16, 16, 4, 0, 24 x 7, 8, 16, 80, 8, 24, 24) matching its
// 64-bit libstdc++/engine layout. Here GetDataSize() reports the size of the
// reconstructed C++ type (rounded up to 8 so consecutive fields stay
// aligned), and the vector kinds are real std::vectors constructed in place
// (what the original's zeroed three-pointer block amounts to).
//
// Also: TData::Compare()/CompareDataWithValue() return 0 for "equal" and -1
// otherwise, which is what their callers test. The disassembly agrees for
// bool/int/float/rect/point/sprite/link values, but on several paths (string
// and string-list kinds in Compare(), and the "equal" exits of most loops) it
// leaves a stale register value in the result, so a literal reading would
// flag equal strings as different; the contract is implemented instead.
#pragma once

#include <vector>

#include "TCharHolder.h"
#include "TSprite.h"
#include "TTextLanguage.h"
#include "WxStub.h"
#include "datastruct/link.h"
#include "datastruct/type.h"

class TDataGroup;
class TProjectFileWriter;
class TVisionaire;

// The typed views of a value's address. A null address is reported (logged and
// asserted) and answered with a shared, freshly emptied dummy value instead.
int &GetRefInt(const void *value);
float &GetRefFloat(const void *value);
bool &GetRefBool(const void *value);
TCharHolder &GetRefString(const void *value);
TCharHolder &GetRefPath(const void *value);
wxRect &GetRefRect(const void *value);
wxPoint &GetRefPoint(const void *value);
TSprite &GetRefSprite(const void *value);
TLink &GetRefLink(const void *value);
std::vector<TLink> &GetRefLinks(const void *value);
std::vector<wxRect> &GetRefVRect(const void *value);
std::vector<wxPoint> &GetRefVPoint(const void *value);
std::vector<TCharHolder> &GetRefVPath(const void *value);
std::vector<int> &GetRefVInt(const void *value);
std::vector<float> &GetRefVFloat(const void *value);
std::vector<bool> &GetRefVBool(const void *value);
std::vector<TCharHolder> &GetRefVString(const void *value);
std::vector<TSprite> &GetRefVSprite(const void *value);
std::vector<TTextLanguage> &GetRefVText(const void *value);

class TData {
public:
	/** The number of bytes a field of the given kind occupies in a record's
	 *  storage (0 for an unknown kind). */
	static int GetDataSize(eTypeData type);

	/** Constructs a default value of the kind at `value` (raw memory of at
	 *  least GetDataSize() bytes). */
	static void CreateDataInstance(eTypeData type, void *value);
	/** Destroys the value at `value`. For link kinds, the links are first
	 *  unregistered from the record's visionaire (unless it's being torn
	 *  down), except parent links. */
	static void DeleteDataInstance(eTypeData type, void *value, TDataGroup *group);
	/** Resets the value to its empty state (-1 for ints, floats, points and
	 *  rects, "" for strings, false, no list elements...). */
	static void ClearDataInstance(eTypeData type, void *value);

	/** Reads an attribute's text, a [begin, end) character range, into the
	 *  value (bool/int/string/path/float kinds only). */
	static void SerializeParam(const char *begin, const char *end, eTypeData type, void *value);
	/** Writes the value as the attribute `name` (the same five kinds). */
	static void SerializeParam(TProjectFileWriter &writer, eTypeData type, int name, void *value);
	/** Writes the value as the element `name` (list, point, rect, sprite and
	 *  link kinds); always true. */
	static bool SerializeContent(TProjectFileWriter &writer, eTypeData type, int name, void *value);

	/** 0 if the two values are equal, -1 if not (see the file comment). Link
	 *  lists are compared without regard to order. */
	static int Compare(eTypeData type, const void *a, const void *b);
	/** The same, for a stored value against a plain value of the same kind. */
	static int CompareDataWithValue(eTypeData type, const void *data, const void *value);

	/** A human-readable form: lists are joined with ", " (links with a new
	 *  line); a link is shown as the name of the object it points to, found in
	 *  `visionaire`, or "[any]"/"[empty]". */
	static wxString ToString(eTypeData type, void *value, TVisionaire *visionaire);
	/** A memory-use estimate for the value: the sizes of its strings plus, for
	 *  lists, the bytes of their elements (in the original's layout). */
	static unsigned long GetSizeMemory(eTypeData type, void *value, bool deep, TVisionaire *visionaire);
	static unsigned long SizePath(const TCharHolder &path);

	/** Copies `source` into the value at `dest`. Link kinds are not copied. */
	static void Assign(eTypeData type, void *dest, const void *source);
};
