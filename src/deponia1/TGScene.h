// Not yet assert-confirmed to a specific file; stays at the top level.
//
// The concrete "scene" drawable: TSceneControl::GetScene() actually returns
// a TGScene* (confirmed - callers immediately use it as TGScene::IsMenu(),
// TGScene::GetObject(), etc., not just TPaintControl's interface), so it
// derives from TPaintControl the same way TCursorControl/TLoadingControl do.
//
// Implemented in full (Deponia_Linux.asm lines 166435-173218, all 39
// manifest-listed methods) except for the three places that depend on the
// Lua bridge or the unmodeled `graphics` backend - see TGScene.cpp's own
// comments at GetTint()/BeginScene()/Draw()/InitialiseBackground(). Real
// layout (offsets from the object base, where TPaintControl occupies
// +0x00-0x47): a TTScene (a TVisObjRef-derived data-record handle, +0x48)
// whose first field after the handle is the brightness float (+0x50), the
// background picture (+0x58), its shader id (+0x128 - actually a field of
// the background TPictureIO, see TPictureIO::SetShader()), the lightmap
// picture (+0x140) and its scale factors (+0x228/0x22C), two particle-
// system flag bytes (+0x230/0x231), the particle system (+0x238), a heap
// particle container (+0x290) and its frame timer (+0x298), then the four
// scene-object collections - the scene's own objects (+0x2A8), its
// characters (+0x2C0), the depth-sorted draw order built from both
// (+0x2D8) and a hash map from object id to draw-order index (+0x2F0-
// 0x308) - followed by the savegame-picker state (savegames +0x310, their
// click areas +0x328, slot size +0x340/0x344, selected slot +0x348, first
// visible slot +0x34C) and the snoop-animation fade state (+0x350-0x370).
#pragma once

#include <cstdint>
#include <list>
#include <unordered_map>
#include <vector>

#include "vsplayer/particlesGame.h"
#include "graphicslib/particleHaduken.h"
#include "TPaintControl.h"
#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "graphicslib/picture.h"

class TGCharacter;
class TManagedObject;
class TMSavegame;
class TMSavegameArea;
class TSceneActionArea;

class TGScene : public TPaintControl {
public:
	TGScene();
	~TGScene() override;

	void Prepare() override;
	void Draw() override;

	// Confirmed present at a fixed offset (TGameControl::CenterScene
	// compares the current character's scene-link against this, asm lines
	// 460533-460715) - same "TVisObjRef at a known offset" pattern as
	// TGCharacter/TGDialog/TSText/TGText.
	const TVisObjRef &GetRef() const {
		return _ref;
	}
	// Confirmed accessed directly as a private field from TSceneControl::Set
	// (Deponia_Linux.asm line 60F910) - modeled as a public accessor
	// instead of a cross-class friendship, matching GetRef() above.
	void SetRef(const TVisObjRef &ref) {
		_ref = ref;
	}

	void Clear();
	void EndScene();
	void BeginScene();
	void BeforeFade();
	bool IsMenu() const;

	// Builds the scene's own objects and savegame-slot click areas (see
	// SetScene()'s own comment in the .cpp), then characters and draw order.
	void SetScene();
	void InitialiseBackground();
	void SetCurrentLightmap();
	const TPictureIO &GetLightMap() const {
		return _lightmap;
	}
	float GetBrightness() const {
		return _brightness;
	}
	// Per-pixel lightmap colour at a screen position, tinted for `object`; with a Lua lightmap callback (set by
	// graphics.lightmapCallback) the function may replace the colour.
	unsigned int GetTint(const TVisObjRef &object, const wxPoint &pos) const;

	/** The name of the Lua function that may change the colour of a lightmap pixel (empty: none). */
	static std::string LuaLightmapCallback;

	// Confirmed TManagedObject* (TGameControl::ReattachSceneObjectTexts
	// calls TManagedObject::SetText() directly on the result, asm lines
	// 461599-461666) - the manifest's void* was a placeholder guess. Looks
	// the object up by id in the draw-order index.
	TManagedObject *GetObject(const TVisObjRef &object) const;
	// The topmost object under a screen position (a spatial lookup walking
	// the draw order back to front; confirmed against TGameControl::
	// HandleMouseMove, Deponia_Linux.asm line 472295).
	TManagedObject *GetObject(const wxPoint &pos) const;

	// Confirmed (TGameControl::UpdateRandomTimers copies the result and
	// iterates the copy, asm lines 463363-463466) - the real accessor
	// returns the address of the member vector itself.
	std::vector<TGCharacter *> &GetCharacters();
	void SetCharacters();
	void SetCharacter(const TVisObjRef &character, const TVisObjRef &scene, const wxPoint &pos, int direction);
	void SetCharacter(const TVisObjRef &character, const TVisObjRef &scene, int direction);
	void InitialiseCharacter(const TVisObjRef &character, const wxPoint &pos, int direction,
	                         const TVisObjRef &scene);

	// Called once per frame by TGameControl::Update() (Deponia_Linux.asm
	// lines 469738, 470595; the second gated by EngineUpdatePaused).
	void SortAllObjects();

	// The savegame-slot-picker scene (a "load game" menu) tracks which slot
	// is currently selected/hovered (confirmed from TGameControl::
	// SavegameExists/DeleteSavegame/HandleMouseUp, asm lines 462562-473091).
	TMSavegame *GetSelectedSavegame(bool create);
	TMSavegame *GetSavegameAt(const wxPoint &pos) const;
	void DeleteSelectedSavegame();
	void SelectSavegame(const wxPoint &pos);
	void SetSelectedSavegame(int index);
	/** The savegame-picker state that the scripts read (`system.savegamesCount`, `selectedSavegame`,
	 *  `savegamesScrollPos`). */
	int GetSavegameCount() const {
		return static_cast<int>(_savegames.size());
	}
	int GetSelectedSavegameIndex() const {
		return _selectedSavegame;
	}
	int GetFirstVisibleSavegame() const {
		return _firstVisibleSavegame;
	}
	void ScrollSavegames(bool backwards);
	void SetActiveSavegames();
	void SetSavegames();
	void ClearSavegames();

	// Snoop animations ("peek at what's hidden" overlays) fade in and out
	// over a duration, driven once per frame by UpdateSnoopAnimAlpha().
	void ShowSnoopAnimations(bool show, int durationMs);
	void UpdateSnoopAnimAlpha();
	float GetSnoopAnimAlpha() const {
		return _snoopAlpha;
	}

	// Confirmed to touch no object of this type at any call site (TGameControl::
	// Init, asm lines 467226-467624; TGCharacter::AssignToScene) - process-
	// wide registries of every scene's "action area" objects, keyed by the
	// scene's packed id. Modeled as static, same pattern as TGAction/
	// TGAnimation's entry points.
	static void InitActionAreas();
	static void ClearActionAreas();
	static std::list<TSceneActionArea *> *GetActionAreas(const TVisObjRef &scene);

protected:
	// Confirmed protected-by-need (THScene reads the data object at +0x48 and sets the
	// brightness, asm lines 219228-219640): moved up from private.
	TVisObjRef _ref;
	float _brightness = 1.0f;

private:
	static std::uint32_t objectKey(const TVisObjRef &ref);
	int visibleSavegameEnd() const;
	void appendToDrawList(TManagedObject *object);
	void setSavegamesActive();

	static std::unordered_map<int, std::list<TSceneActionArea *> *> s_sceneActionAreas;

	TPictureIO _background;
	TPictureIO _lightmap;
	float _lightmapScaleX = 0.0f;
	float _lightmapScaleY = 0.0f;
	bool _hasParticles = false;
	bool _particlesFromLua = false;
	TGParticleSystem _particleSystem;
	ParticleContainer *_particleContainer = nullptr;
	TTimer _particleTimer;

	std::vector<TManagedObject *> _objects;
	std::vector<TGCharacter *> _characters;
	std::vector<TManagedObject *> _drawList;
	std::unordered_map<std::uint32_t, int> _drawIndex;

	std::vector<TMSavegame *> _savegames;
	std::vector<TMSavegameArea *> _savegameAreas;
	int _slotWidth = 0;
	int _slotHeight = 0;
	int _selectedSavegame = 0;
	int _firstVisibleSavegame = 0;

	// 0 = hidden, 1 = fading in, 2 = shown, 3 = fading out.
	int _snoopState = 0;
	int _snoopDurationMs = 0;
	TTimer _snoopTimer;
	float _snoopAlpha = 0.0f;
	float _snoopFromAlpha = 0.0f;
	float _snoopToAlpha = 0.0f;
};
