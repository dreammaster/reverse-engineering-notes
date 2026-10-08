// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vscommon/scripting/argument.cpp - see manifest/source_layout.tsv.
//
// TArgument is a tagged-union style value passed to/from Lua script calls
// (see TMasterControl::ProcessMessage, which builds a std::vector<TArgument*>
// of these to call a registered handler). Confirmed in full (Deponia_Linux.
// asm lines 1435493-1439510, all 57 manifest-listed methods): a type tag
// plus a heap-allocated payload whose shape depends on the tag - modeled
// here with std::variant instead of the original's manual per-type new/
// delete/switch dance, since nothing depends on the exact memory layout
// (same "behavioral over binary fidelity" reasoning already used for
// _charactersByHash/TVList elsewhere in this project). kString/kPath share
// one wxString variant alternative (they differ only in the "vispath:"-
// prefix handling SetPath()/AddPath()/GetPath() do, not in storage), and
// kStringList/kPathList likewise share one vector<TCharHolder> alternative.
#pragma once

#include <variant>
#include <vector>

#include "TCharHolder.h"
#include "TSprite.h"
#include "TTextLanguage.h"
#include "WxStub.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"

class TVisionaireObject;

// Confirmed 20 distinct values (0-19), read directly off the raw switch/cmp
// chains in Clear()/SetType()/ToLua()/CopyTo() - a classic tagged-union
// "value passed to/from Lua" scheme. Real enumerator names aren't
// recoverable (just raw case values); named here for the concrete C++ type
// each one carries.
enum class TArgType {
	kNone = 0,
	kBool = 1,
	kInt = 2,
	kFloat = 3,
	kPoint = 4,
	kRect = 5,
	kString = 6,
	kPath = 7,
	kSprite = 8,
	kObject = 9,      // TVisObjRef
	kText = 10,       // TTextLanguage
	kIntList = 11,
	kFloatList = 12,
	kPointList = 13,
	kRectList = 14,
	kStringList = 15,
	kPathList = 16,
	kSpriteList = 17,
	kObjectList = 18, // TVList
	kTextList = 19,
	// Confirmed (ConvertArgumentFromLua(), asm lines 1433460-1434417): the type that a command asks for when
	// it takes any value (the type is then taken from the Lua value), and the `flags` table of a call
	// (which is not converted).
	kAny = 20,
	kFlags = 21,
};


class TArgument {
public:
	TArgument() = default;

	void Clear();
	TArgType GetType() const {
		return _type;
	}
	void SetType(TArgType type);

	void Set(bool value);
	void Set(int value);
	void Set(double value);
	void Set(const wxPoint &value);
	void Set(const wxRect &value);
	void Set(const wxString &value);
	// Confirmed (asm lines 1436054-1436163): strips a leading "vispath:"
	// prefix (recovered byte-for-byte from the binary) if present, keeping
	// the rest of the path unchanged otherwise.
	void SetPath(const wxString &value);
	void Set(const TSprite &value);
	void Set(const TTextLanguage &value);
	void Set(const TVisObjRef &value);
	void Set(TVisionaireObject *value);
	void Set(const TVList &value);
	void Set(const std::vector<int> &value);
	void Set(const std::vector<float> &value);
	void Set(const std::vector<wxPoint> &value);
	void Set(const std::vector<wxRect> &value);
	void Set(const std::vector<TSprite> &value);
	void Set(const std::vector<TCharHolder> &value);
	// Confirmed a distinct overload from Set(vector<TCharHolder>&) above,
	// setting kPathList instead of kStringList - otherwise identical (no
	// per-entry "vispath:" stripping the way SetPath()/AddPath() do).
	void SetPaths(const std::vector<TCharHolder> &value);
	void Set(const std::vector<TTextLanguage> &value);

	void Add(const TVisObjRef &value);
	void Add(int value);
	void Add(float value);
	void Add(const wxPoint &value);
	void Add(const wxRect &value);
	void Add(const TSprite &value);
	void Add(const wxString &value);
	void AddPath(const wxString &value);

	bool GetBool() const;
	int GetInt() const;
	float GetFloat() const;
	const wxPoint &GetPoint() const;
	const wxRect &GetRect() const;
	const wxString &GetString() const;
	const wxString &GetPath() const;
	const TSprite &GetSprite() const;
	const TTextLanguage &GetText() const;
	TVisObjRef GetObject() const;
	const std::vector<int> &GetIntList() const;
	const std::vector<float> &GetFloatList() const;
	const std::vector<wxPoint> &GetPointList() const;
	const std::vector<wxRect> &GetRectList() const;
	const std::vector<TCharHolder> &GetStringList() const;
	const std::vector<TCharHolder> &GetPathList() const;
	const std::vector<TSprite> &GetSpriteList() const;
	const TVList &GetObjectList() const;
	const std::vector<TTextLanguage> &GetTextList() const;

	// Confirmed (asm lines 1437413-1437432): only succeeds (returns true)
	// when currently kFloat, converting in place to kInt; otherwise leaves
	// this unchanged and returns false.
	bool ConvertToInt();
	// Confirmed (asm lines 1438702-1438887): only succeeds when currently
	// kFloatList (converting each element to int) or kNone (becoming an
	// empty kIntList); otherwise leaves this unchanged and returns false.
	bool ConvertToIntList();
	// Confirmed (asm lines 1437440-1437665): resolves the current value to
	// a game-data object reference, converting in place to kObject. A
	// kString value is parsed as "id1,id2" (TId::TId(int,int)) - the AnyId
	// sentinel resolves via TVisionaire::GetAnyObject(), anything else via
	// FindObjectByNameOrId() (both new, call-shape stubs - the Lua-side
	// object-lookup contract behind them isn't reversed); a kNone value
	// resolves to an empty TVisObjRef; any other type fails (returns false,
	// unchanged).
	bool ConvertToObject();
	// Confirmed (asm lines 1437671-1437912): a kStringList resolves each
	// entry via FindObjectByNameOrId() into a new kObjectList, failing
	// (unchanged) if any entry doesn't resolve; kNone or an empty
	// kObjectList becomes an empty kObjectList; any other type fails.
	bool ConvertToObjectList();

	// Confirmed call shape only (Deponia_Linux.asm lines 1437920-1438155) -
	// pushes the current value onto the Lua stack (raw lua_push*() calls, or
	// a separate ConvertToLua() free function per composite type) - not
	// reversed beyond that (same standing "Lua bridge contract" gap as
	// LuaExecuteFunction/LuaDoString elsewhere in this project).
	void ToLua() const;
	// Confirmed (asm lines 1439341-1439510+): copies this value into
	// another TArgument - a plain copy assignment with this project's
	// variant-based modeling, since nothing depends on the original's own
	// per-type reuse-existing-allocation optimizations.
	void CopyTo(TArgument &dest) const {
		dest._type = _type;
		dest._value = _value;
	}

private:
	using Payload = std::variant<std::monostate, bool, int, double, wxPoint, wxRect, wxString, TSprite, TVisObjRef,
	      TTextLanguage, std::vector<int>, std::vector<float>, std::vector<wxPoint>,
	      std::vector<wxRect>, std::vector<TCharHolder>, std::vector<TSprite>, TVList,
	      std::vector<TTextLanguage>>;

	template<typename T>
	const T &GetOr(TArgType type, const T &defaultValue) const {
		return _type == type ? std::get<T>(_value) : defaultValue;
	}

	TArgType _type = TArgType::kNone;
	Payload _value;
};
