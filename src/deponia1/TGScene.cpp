#include "TGScene.h"

#include <algorithm>

#include "AppGlobals.h"
#include "TGAction.h"
#include "THObject.h"
#include "TGCharacter.h"
#include "TMSavegame.h"
#include "TMSavegameArea.h"
#include "TSceneActionArea.h"
#include "graphicslib/graphics.h"
#include "vsplayer/control/gameControl.h"

// TGameControl implements every accessor used below, but g_pGameControl is
// only declared as TMasterControl* (AppGlobals.h) - same cast already
// established at TManagedObject::ClickedWithoutReach's own call site.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

std::unordered_map<int, std::list<TSceneActionArea *> *> TGScene::s_sceneActionAreas;

TGScene::TGScene() {
	// The real constructor sets up a 128-bucket hash map for the draw-order
	// index (std::unordered_map manages its own buckets), leaves the
	// selected-slot/first-visible-slot ints uninitialized and clears every
	// other member as below.
	_selectedSavegame = -1;
}

TGScene::~TGScene() {
	delete _particleContainer;
	_particleContainer = nullptr;
	Clear();
}

// One past the last savegame slot that's both shown and has a click area to
// show it in (the loop bound Prepare()/Draw() use identically).
int TGScene::visibleSavegameEnd() const {
	return std::min(static_cast<int>(_savegames.size()),
	                static_cast<int>(_savegameAreas.size()) + _firstVisibleSavegame);
}

// Confirmed (asm lines 166435-166568): one 'P' tag set around the whole
// method (g_loadingState), the background picture refreshed, every
// currently visible savegame slot's screenshot rect set and its own Prepare()
// run, then every scene object's Prepare().
void TGScene::Prepare() {
	g_loadingState = "P";
	if (IsActive()) {
		_background.RefreshSprite(false);

		int visibleEnd = visibleSavegameEnd();
		for (int i = _firstVisibleSavegame; i < visibleEnd; i++) {
			TMSavegame *savegame = _savegames.at(i);
			savegame->SetScreenshotRect(_savegameAreas[i - _firstVisibleSavegame]->GetBoundingRect());
			savegame->Prepare();
		}

		for (TManagedObject *object : _drawList)
			object->Prepare();
	}
	g_loadingState = "L";
}

// Confirmed (asm lines 166568-166997) except the particle-drawing branch's
// own calls into the unmodeled `graphics` global (vtable slots 0x58 -
// translate by the scene's negated float scroll position, before and after
// - and 0x88 - draw a TParticleSystem - plus an optional TMatrix4 multiply
// against the global matrix1 when matricesActive is set), which are skipped,
// same as this project's other not-yet-integrated external dependencies.
// ParticleContainer's own Update() and the particle system's fixed-step
// IncTime() catch-up loop are kept.
void TGScene::Draw() {
	if (!IsActive())
		return;

	SetCurrent();

	unsigned int color = 0xFFFFFFFF;
	if (_brightness != 1.0f) {
		unsigned int level = static_cast<unsigned char>(static_cast<int>(_brightness * 255.0f));
		color = 0xFF000000 | level | (level << 8) | (level << 16);
	}

	// The original's `defaultShader` substitution only takes effect when
	// the global is nonzero (a `test eax, eax / cmovnz` after the -1 check).
	int shader = _ref.GetInt(0x31A);
	if (shader == -1 && defaultShader != 0)
		shader = defaultShader;
	ShaderCallback(shader, &_ref);
	_background.SetShader(shader);
	_background.Draw(1.0f, color);

	int visibleEnd = visibleSavegameEnd();
	for (int i = _firstVisibleSavegame; i < visibleEnd; i++) {
		TMSavegame *savegame = _savegames.at(i);
		savegame->SetScreenshotRect(_savegameAreas[i - _firstVisibleSavegame]->GetBoundingRect());
		savegame->Draw();
	}

	for (TManagedObject *object : _drawList)
		object->Draw();

	for (int i = _firstVisibleSavegame; i < visibleEnd; i++) {
		TMSavegame *savegame = _savegames.at(i);
		savegame->SetScreenshotRect(_savegameAreas[i - _firstVisibleSavegame]->GetBoundingRect());
		savegame->DrawText();
	}

	if (_hasParticles) {
		if (_particlesFromLua && _particleContainer) {
			_particleContainer->Update(false, vec2(), 0.016666668f, true);
		} else {
			long elapsed = _particleTimer.GetTime();
			if (elapsed > 24) {
				do {
					elapsed -= 25;
					_particleSystem.IncTime();
				} while (elapsed > 25);
				_particleTimer.SetTime();
				_particleTimer.AdjustTimer(-elapsed);
			}
		}
	}

	for (TManagedObject *object : _drawList)
		object->DrawSnoopAnimation();
}

// Confirmed (asm lines 166997-167139): frees every scene object that isn't
// being kept alive by a lifetime counter (characters, which are not
// owned by the scene, are left alone), then forgets the objects, draw order
// and index, and frees every savegame slot.
void TGScene::Clear() {
	for (TManagedObject *object : _objects) {
		if (object->GetLifetime() > 0)
			continue;
		delete object;
	}
	_objects.clear();
	_drawList.clear();
	_drawIndex.clear();

	for (TMSavegameArea *area : _savegameAreas)
		delete area;
	_savegameAreas.clear();

	for (TMSavegame *savegame : _savegames)
		delete savegame;
	_savegames.clear();
}

// Confirmed (asm lines 167139-167268): the scene's own "scene finished"
// actions (every action link of field 0x14F whose type field 0x9F is 32)
// run, then both pictures and the particle state are released.
void TGScene::EndScene() {
	_background.RemoveSprite();
	_lightmap.Clear();

	TVList links;
	if (!_ref.IsEmpty())
		_ref.GetLinks(0x14F, TypeOrder::kValue1, links);

	for (TVisionaireObject *link : links.items) {
		if (link->GetInt(0x9F) != 0x20)
			continue;
		TGAction *action = TGAction::AddRunningAction(TVisObjRef(*link));
		if (action)
			action->Execute(false, nullptr);
	}

	_particleSystem.Clear();
	delete _particleContainer;
	_particleContainer = nullptr;
}

// Confirmed (asm lines 167268-167333).
void TGScene::ClearSavegames() {
	for (TMSavegameArea *area : _savegameAreas)
		delete area;
	_savegameAreas.clear();

	for (TMSavegame *savegame : _savegames)
		delete savegame;
	_savegames.clear();
}

// Confirmed (asm lines 167333-167446): the topmost object under a position -
// the draw order walked back to front, each object asked whether it's hit
// (with the object manager's current detection settings), in this scene's
// own coordinates. An inactive scene has no objects.
TManagedObject *TGScene::GetObject(const wxPoint &pos) const {
	if (!IsActive())
		return nullptr;

	TGDetectInfo info;
	gameControl()->GetObjectManager()->GetDetectInfo(info);
	wxPoint relative = GetRelativePoint(pos);

	for (auto it = _drawList.rbegin(); it != _drawList.rend(); ++it) {
		if ((*it)->IsInside(relative, info))
			return *it;
	}
	return nullptr;
}

std::vector<TGCharacter *> &TGScene::GetCharacters() {
	return _characters;
}

// Confirmed (asm lines 167462-167607): puts a character into a scene. A
// "no position" input ((-1, -1) or (0, 0)) is replaced by the scene's own
// first start position (field 0x153 of the first of its field-0x88 links)
// when that one is a real point (both coordinates positive); otherwise the
// position is used as given.
void TGScene::InitialiseCharacter(const TVisObjRef &character, const wxPoint &pos, int direction,
                                  const TVisObjRef &scene) {
	TGCharacter *gameCharacter = gameControl()->GetCharacter(character);

	if (pos == wxPoint{-1, -1} || pos == wxPoint{0, 0}) {
		TVList links;
		_ref.GetList(0x88, links);
		if (links.size() != 0 && !links.front()->IsEmpty()) {
			wxPoint start = *links.front()->GetPoint(0x153);
			if (start.x > 0 && start.y > 0) {
				gameCharacter->AssignToScene(scene, start, direction);
				return;
			}
		}
	}

	gameCharacter->AssignToScene(scene, pos, direction);
}

// Confirmed (asm lines 167607-167649): the real call is the vtable's own
// slot 0 (Prepare()); the GetInt(0x223) read of the game data's own fade
// field just before it has its result discarded.
void TGScene::BeforeFade() {
	Prepare();
}

// Confirmed (asm lines 167649-167666).
bool TGScene::IsMenu() const {
	return _ref.GetBool(0x124);
}

std::uint32_t TGScene::objectKey(const TVisObjRef &ref) {
	const std::uint8_t *id = ref.GetId();
	return id[0] | (id[1] << 8) | (id[2] << 16) | (static_cast<std::uint32_t>(id[3]) << 24);
}

// Confirmed (asm lines 167666-167742): looks an object up by its data
// record's id in the draw-order index; null when absent.
TManagedObject *TGScene::GetObject(const TVisObjRef &object) const {
	auto it = _drawIndex.find(objectKey(object));
	if (it == _drawIndex.end())
		return nullptr;
	return _drawList[it->second];
}

// Confirmed (asm lines 167742-167896): fades the "snoop" overlay in (show)
// or out (!show) over durationMs, restarting from wherever an unfinished
// fade in the opposite direction had got to - the fraction of the old
// duration already elapsed becomes the new fade's starting point and
// shortens its duration. State: 0 hidden, 1 fading in, 2 shown, 3 fading
// out; a show request while already fading in/shown (or a hide request while
// hidden/fading out) does nothing.
void TGScene::ShowSnoopAnimations(bool show, int durationMs) {
	if (show) {
		if (_snoopState != 0 && _snoopState != 3)
			return;
		bool wasFadingOut = _snoopState == 3;

		_snoopFromAlpha = 0.0f;
		_snoopToAlpha = 1.0f;
		if (wasFadingOut) {
			long elapsed = _snoopTimer.GetTime();
			if (elapsed < _snoopDurationMs) {
				double ratio = static_cast<double>(elapsed) / static_cast<double>(_snoopDurationMs);
				_snoopFromAlpha = 1.0f - static_cast<float>(ratio);
				_snoopDurationMs = static_cast<int>(durationMs * ratio);
			} else {
				_snoopDurationMs = durationMs;
			}
		} else {
			_snoopDurationMs = durationMs;
		}

		_snoopState = 1;
		_snoopTimer.SetTime();
		for (TManagedObject *object : _drawList)
			object->ShowSnoopAnimation(true);
	} else {
		if (_snoopState != 1 && _snoopState != 2)
			return;
		bool wasFadingIn = _snoopState == 1;

		_snoopFromAlpha = 1.0f;
		_snoopToAlpha = 0.0f;
		if (wasFadingIn) {
			long elapsed = _snoopTimer.GetTime();
			if (elapsed < _snoopDurationMs) {
				double ratio = static_cast<double>(elapsed) / static_cast<double>(_snoopDurationMs);
				_snoopFromAlpha = static_cast<float>(ratio);
				_snoopDurationMs = static_cast<int>(durationMs * ratio);
			} else {
				_snoopDurationMs = durationMs;
			}
		} else {
			_snoopDurationMs = durationMs;
		}

		_snoopState = 3;
		_snoopTimer.SetTime();
	}
}

// Confirmed (asm lines 167896-167998): per-frame step of the fade above.
// Once the alpha has reached its target, a finished fade-in becomes "shown"
// and a finished fade-out hides every object's snoop animation again.
void TGScene::UpdateSnoopAnimAlpha() {
	if (_snoopAlpha == _snoopToAlpha) {
		if (_snoopState == 3) {
			for (TManagedObject *object : _drawList)
				object->ShowSnoopAnimation(false);
			_snoopState = 0;
		} else if (_snoopState == 1) {
			_snoopState = 2;
		}
		return;
	}

	long elapsed = _snoopTimer.GetTime();
	if (_snoopDurationMs != 0 && elapsed < _snoopDurationMs) {
		float fraction = static_cast<float>(elapsed) / static_cast<float>(_snoopDurationMs);
		_snoopAlpha = (_snoopToAlpha - _snoopFromAlpha) * fraction + _snoopFromAlpha;
	} else {
		_snoopAlpha = _snoopToAlpha;
	}
}

// Confirmed (asm lines 168015-168068): selects the slot whose click area
// contains `pos`, counting from the first visible slot.
void TGScene::SelectSavegame(const wxPoint &pos) {
	int index = _firstVisibleSavegame;
	for (TMSavegameArea *area : _savegameAreas) {
		if (area->IsInside(pos)) {
			_selectedSavegame = index;
			return;
		}
		index++;
	}
}

// Confirmed (asm lines 168068-168129).
void TGScene::DeleteSelectedSavegame() {
	if (_selectedSavegame < 0 || _selectedSavegame >= static_cast<int>(_savegames.size()))
		return;

	delete _savegames[_selectedSavegame];
	_savegames.erase(_savegames.begin() + _selectedSavegame);
}

// Confirmed (asm lines 168129-168202): the savegame behind whichever click
// area contains `pos`; null if none does or that slot has no savegame.
TMSavegame *TGScene::GetSavegameAt(const wxPoint &pos) const {
	int index = _firstVisibleSavegame;
	for (TMSavegameArea *area : _savegameAreas) {
		if (area->IsInside(pos)) {
			if (index < 0 || index >= static_cast<int>(_savegames.size()))
				return nullptr;
			return _savegames[index];
		}
		index++;
	}
	return nullptr;
}

// Confirmed (asm lines 168202-168304): moves the first visible slot by the
// scene's own page step (field 0x143), clamped at the start and - if that
// would run past the end - left where it was, then re-flags every
// savegame's own active state accordingly.
void TGScene::ScrollSavegames(bool backwards) {
	int oldFirst = _firstVisibleSavegame;
	int step = _ref.GetInt(0x143);

	if (backwards)
		_firstVisibleSavegame -= step;
	else
		_firstVisibleSavegame += step;
	if (_firstVisibleSavegame < 0)
		_firstVisibleSavegame = 0;
	else if (static_cast<int>(_savegames.size()) < _firstVisibleSavegame)
		_firstVisibleSavegame = oldFirst;

	setSavegamesActive();
}

// A savegame counts as active when its index is at or past the first
// visible slot and below (savegame count + first visible slot) - the
// original's exact bound, which (not being clamped to the click areas'
// own count) is wider than what's actually on screen; confirmed identical
// at all three call sites (ScrollSavegames/SetActiveSavegames/SetSavegames).
void TGScene::setSavegamesActive() {
	int limit = static_cast<int>(_savegames.size()) + _firstVisibleSavegame;
	int index = 0;
	for (TMSavegame *savegame : _savegames) {
		savegame->SetActive(index >= _firstVisibleSavegame && index < limit);
		index++;
	}
}

// Confirmed (asm lines 168304-168366).
void TGScene::SetActiveSavegames() {
	setSavegamesActive();
}

// Confirmed (asm lines 168366-168383).
void TGScene::SetSelectedSavegame(int index) {
	_selectedSavegame = index;
}

// Confirmed (asm lines 168383-168670): loads the scene's lightmap (the
// per-pixel tint GetTint() reads) from the file named by field 0x226 into
// the shared lightmap memory block, remembering how much bigger/smaller it
// is than the background so a screen position can be mapped onto it; no
// file, or a load failure, leaves it cleared. (The load setting is the
// binary's own value 2, which TPictureIO::eLoadSetting hasn't named yet.)
void TGScene::SetCurrentLightmap() {
	_lightmap.Clear();

	wxFileName path(_ref.GetPath(0x226));
	if (!path.IsOk()) {
		_lightmap.Clear();
		return;
	}

	_lightmap.SetPath(TCharHolder(path.GetFullPath().mb_str()));
	_lightmap.SetMemoryBlock(graphics->GetLightMapMemBlock());

	wxFileName normalized = path;
	normalized.NormalizePath();
	if (_lightmap.LoadPicture(normalized, static_cast<TPictureIO::eLoadSetting>(2))) {
		_lightmapScaleX = static_cast<float>(_lightmap.GetWidth()) / static_cast<float>(_background.GetWidth());
		_lightmapScaleY = static_cast<float>(_lightmap.GetHeight()) / static_cast<float>(_background.GetHeight());
	} else {
		_lightmap.Clear();
	}
}

// Confirmed (asm lines 168670-169184) except the `graphics` backend call
// (vtable slot 0x130, one bool = false) made while returning from a menu
// scene to a game scene, which is skipped like the rest of that unmodeled
// global. Sets up this scene as the current one: its background and
// lightmap, brightness, visible/worktop sizes and scrollability, then -
// differently for a menu scene and a game scene, and depending on whether
// the scene it replaces was a menu - pauses/resumes the game's running
// actions/animations/texts (and the dialog and the per-character timers),
// restores or recentres the scroll position, and picks the cursor.
void TGScene::InitialiseBackground() {
	TSceneControl *sceneControl = gameControl()->GetSceneControl();
	if (!sceneControl->OldSceneIsMenu())
		sceneControl->SetLastPlayableScrollPos(sceneControl->GetOldScene()->GetScrollPos());

	_background.Set(_ref.GetSprite(0xEC));
	_background.RefreshSprite(false);
	SetCurrentLightmap();
	_brightness = static_cast<float>(_ref.GetInt(0x252)) / 100.0f;

	int windowWidth, windowHeight;
	gameControl()->GetWindowSize(&windowWidth, &windowHeight);
	SetVisibleSize(windowWidth, windowHeight);

	wxRect worktop = *_ref.GetRect(0x238);
	SetWorktopArea(worktop, _background.GetWidth(), _background.GetHeight());
	SetIsScrollable(_ref.GetBool(0xE8));

	bool isMenu = _ref.GetBool(0x124);
	if (isMenu) {
		if (!sceneControl->OldSceneIsMenu()) {
			// Entering a menu from the game: freeze the game, remember (and
			// close) any open dialog, mark characters that aren't already
			// being kept alive, and hand the menu the scroll position the
			// game was at.
			TGameControl::GetStopTime().SetTime();
			TGAction::StopRunningActions();
			TGAnimation::StopRunningAnimations();
			TGText::StopRunningTexts();

			TVisObjRef dialog = gameControl()->GetDialog()->GetTarget();
			TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
			game.SetLink(0x243, dialog, true);
			if (!dialog.IsEmpty())
				gameControl()->EndDialog();

			TVisObjRef game2 = gameControl()->GetGameSystem()->GetGame();
			if (game2.GetBool(0x282)) {
				for (TGCharacter *character : _characters) {
					if (character->GetLifetime() <= 0)
						character->SetLifetime(3);
				}
			}

			sceneControl->SetNextStartScrollPosFromLastPlayable();
		}
	} else {
		if (sceneControl->OldSceneIsMenu()) {
			// Leaving a menu back into the game: resume it, and reopen the
			// dialog the menu was entered from, if any.
			TGAction::ContinueStoppedActions();
			TGAnimation::ContinueStoppedAnimations();
			TGText::ContinueStoppedTexts();
			for (TGCharacter *character : gameControl()->GetAllCharacters())
				character->AdjustTimers();

			TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
			TVisObjRef dialog = game.GetLink(0x243);
			if (!dialog.IsEmpty()) {
				gameControl()->StartDialog(dialog);
				TVisObjRef game2 = gameControl()->GetGameSystem()->GetGame();
				game2.ClearLink(0x243, true);
			}
		}
		sceneControl->SetCurrentSceneRef(_ref);
	}

	if (sceneControl->GetNextStartScrollPos() == wxPoint{-1, -1}) {
		gameControl()->CenterScene();
	} else {
		AdjustWindowHorizontal(static_cast<float>(sceneControl->GetNextStartScrollPos().x));
		AdjustWindowVertical(static_cast<float>(sceneControl->GetNextStartScrollPos().y));
		sceneControl->SetNextStartScrollPos(wxPoint{-1, -1});
	}

	TGText::RestartTalkAnimations(_ref);
	gameControl()->AdjustInterfacesOnScreen(false, nullptr);

	if (isMenu) {
		TVisObjRef cursor = _ref.GetLink(0x125);
		gameControl()->GetCursorControl()->SetCursor(false, PackVisId(cursor.GetId()), true);
	} else {
		TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
		TVisObjRef cursor = game.GetLink(0x262);
		if (!cursor.IsEmpty())
			gameControl()->GetCursorControl()->SetCursor(false, PackVisId(cursor.GetId()), false);
	}
}

// Confirmed (asm lines 169215-169506): for every scene in the game data
// (list 4), registers a TSceneActionArea per entry of the scene's own
// action-area links (field 0x2A9), appended in order under the scene's
// packed id.
void TGScene::InitActionAreas() {
	TVList scenes;
	gameControl()->GetGameSystem()->GetList(4, scenes, false);

	for (TVisionaireObject *sceneObject : scenes.items) {
		TVisObjRef scene(*sceneObject);

		TVList links;
		scene.GetLinks(0x2A9, TypeOrder::kValue0, links);

		int key = PackVisId(scene.GetId());
		for (TVisionaireObject *link : links.items) {
			std::list<TSceneActionArea *> *&areas = s_sceneActionAreas[key];
			if (!areas)
				areas = new std::list<TSceneActionArea *>();
			areas->push_back(new TSceneActionArea(TVisObjRef(*link)));
		}
	}
}

// Confirmed (asm lines 169506-169631) except that the original leaves the
// emptied registry's own hash nodes (and their now-dangling list pointers)
// in place; the registry is cleared here instead, since it would otherwise
// hold dangling pointers a later GetActionAreas() could return.
void TGScene::ClearActionAreas() {
	for (auto &entry : s_sceneActionAreas) {
		for (TSceneActionArea *area : *entry.second)
			delete area;
		delete entry.second;
	}
	s_sceneActionAreas.clear();
}

// Confirmed (asm lines 169631-169691). Static in the original too - the
// scene's own id is the argument, not `this` (the manifest's
// "GetActionAreas(TVisObjRef const&)" listing hid that).
std::list<TSceneActionArea *> *TGScene::GetActionAreas(const TVisObjRef &scene) {
	auto it = s_sceneActionAreas.find(PackVisId(scene.GetId()));
	if (it == s_sceneActionAreas.end())
		return nullptr;
	return it->second;
}

// Confirmed (asm lines 170139-170482): reconciles the scene's own character
// list with every character in the game. A character whose scene link
// (field 0x1F7) is this scene joins it (and is set up: walking sound,
// random timer, walk-way system sized to the background's height); every
// other character is removed from the scene - its walking sound is
// stopped, and once its lifetime counter has run out it's unloaded (or,
// with the game's "keep characters loaded" flag (0x282) clear on a non-menu
// scene, kept loaded and given a new lifetime). Finally every character
// that's in the scene starts its standing animation.
void TGScene::SetCharacters() {
	std::vector<TGCharacter *> oldCharacters = _characters;
	bool isMenu = _ref.GetBool(0x124);
	_characters.clear();

	for (TGCharacter *character : gameControl()->GetAllCharacters()) {
		if (character->GetRef().GetLink(0x1F7) == _ref) {
			character->CheckWalkingSound();
			character->SetRandomTime();
			character->InitWaySystem(_background.GetHeight());

			bool wasInScene = std::find(oldCharacters.begin(), oldCharacters.end(), character) !=
			                  oldCharacters.end();
			if (!wasInScene && character->GetLifetime() <= 0) {
				character->PreloadAnimations();
				character->StartFittingAnimation();
			}
			character->SetLifetime(0);
			_characters.push_back(character);
		} else {
			if (character->IsWalkingSoundPlaying())
				character->StopWalkingSound();

			bool wasInScene = std::find(oldCharacters.begin(), oldCharacters.end(), character) !=
			                  oldCharacters.end();

			bool unload = wasInScene;
			if (character->GetLifetime() > 0) {
				character->DecreaseLifetime();
				if (character->GetLifetime() <= 0) {
					unload = true;
				} else if (!isMenu) {
					TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
					unload = game.GetBool(0x282);
				} else {
					continue;
				}
			}
			if (unload) {
				character->RemoveSprites();
				character->UnloadAnimations();
			}
		}
	}

	for (TGCharacter *character : _characters)
		character->StartStandingAnim();
}

// Confirmed (asm lines 170482-170599): moves an already-known character to a
// scene position. When the scene is empty or it's the one being moved to,
// the character is simply repositioned (field 0x200); otherwise it's put
// into the scene via InitialiseCharacter() (the direction argument is
// ignored - always -1 there), re-reading the scene's
// character list if that scene is this one, and the object manager's event
// state is reset.
void TGScene::SetCharacter(const TVisObjRef &character, const TVisObjRef &scene, const wxPoint &pos,
                           int /*direction*/) {
	TVisObjRef characterScene = character.GetLink(0x1F7);

	if (scene.IsEmpty() || scene == characterScene) {
		gameControl()->GetCharacter(character)->GetRef().SetValue(0x200, pos, TSendEventEnum::kSendEvent);
		return;
	}

	bool isThisScene = scene == _ref || characterScene == _ref;
	InitialiseCharacter(character, pos, -1, scene);
	if (isThisScene)
		SetCharacters();
	gameControl()->GetObjectManager()->ResetEventInfo();
}

// Confirmed (asm lines 170599-170770): the same, taking the scene's own
// starting position (field 0x153 plus the scene-origin offset 0x30B). When
// the scene already is the character's own, the position is applied
// directly (and, unless the direction is -1, so is the facing, field
// 0x257).
void TGScene::SetCharacter(const TVisObjRef &character, const TVisObjRef &scene, int direction) {
	if (scene.IsEmpty())
		return;

	TVisObjRef sceneParent = scene.GetParent();
	TVisObjRef characterScene = character.GetLink(0x1F7);

	if (sceneParent == characterScene) {
		wxPoint pos = *scene.GetPoint(0x153);
		TVisObjRef &characterRef = gameControl()->GetCharacter(character)->GetRef();
		characterRef.SetValue(0x200, pos, TSendEventEnum::kSendEvent);
		if (direction != -1)
			characterRef.SetValue(0x257, direction, TSendEventEnum::kSendEvent);
		return;
	}

	bool isThisScene = sceneParent == _ref || characterScene == _ref;
	wxPoint pos = *scene.GetPoint(0x153) + *scene.GetPoint(0x30B);
	InitialiseCharacter(character, pos, direction, sceneParent);
	if (isThisScene)
		SetCharacters();
	gameControl()->GetObjectManager()->ResetEventInfo();
}

// Confirmed (asm lines 170770-170896): the selected slot's savegame - for a
// selected slot past the end of the savegames, either null, or (when
// `create` is set) a brand-new savegame numbered one past the last one,
// added to the scene.
TMSavegame *TGScene::GetSelectedSavegame(bool create) {
	if (_selectedSavegame < 0)
		return nullptr;
	if (_selectedSavegame < static_cast<int>(_savegames.size()))
		return _savegames[_selectedSavegame];
	if (!create)
		return nullptr;

	int slot = _savegames.empty() ? 0 : _savegames.back()->GetSavegameNr() + 1;
	TMSavegame *savegame = new TMSavegame(false, slot, _slotWidth, _slotHeight, gameControl()->GetVisionaire());
	_savegames.push_back(savegame);
	return savegame;
}

// Confirmed (asm lines 170896-171104): rebuilds the savegame list from the
// slots that exist on disk, scrolled to the last full page when there are
// more of them than click areas.
void TGScene::SetSavegames() {
	std::vector<int> slots;
	if (!TMSavegame::GetExistingSaveGames(slots))
		return;

	_firstVisibleSavegame = 0;
	int step = std::abs(_ref.GetInt(0x143));
	int slotCount = static_cast<int>(slots.size());
	int areaCount = static_cast<int>(_savegameAreas.size());
	if (step != 0 && slotCount > areaCount) {
		int next = _firstVisibleSavegame + step;
		int first;
		do {
			slotCount -= step;
			first = next;
			next += step;
		} while (slotCount > areaCount);
		_firstVisibleSavegame = first;
	}

	for (int slot : slots) {
		_savegames.push_back(
		    new TMSavegame(false, slot, _slotWidth, _slotHeight, gameControl()->GetVisionaire()));
	}

	setSavegamesActive();
}

// Confirmed (asm lines 171104-171805) except the optional Lua "lightmap
// callback" override at its end (skipped - the same standing Lua-bridge
// gap used throughout this project; it would only ever trigger for a game
// script that registers one, passing the pixel's colour channels and the
// object to a Lua function that may replace the result). Colours a position
// by the scene's lightmap: with no lightmap, the plain brightness as a grey;
// otherwise the lightmap's pixel (clamped inside it) at the position scaled
// to the lightmap's size.
unsigned int TGScene::GetTint(const TVisObjRef &/*object*/, const wxPoint &pos) const {
	if (_lightmap.IsEmpty()) {
		unsigned int level = static_cast<unsigned char>(static_cast<int>(255.0f * _brightness));
		return 0xFF000000 | level | (level << 8) | (level << 16);
	}

	int x = static_cast<int>(static_cast<float>(pos.x) * _lightmapScaleX);
	int y = static_cast<int>(static_cast<float>(pos.y) * _lightmapScaleY);
	if (x < 0)
		x = 0;
	else if (x >= _lightmap.GetWidth())
		x = _lightmap.GetWidth() - 1;
	if (y < 0)
		y = 0;
	else if (y >= _lightmap.GetHeight())
		y = _lightmap.GetHeight() - 1;

	return _lightmap.GetPixel(wxPoint{x, y}, _brightness);
}

void TGScene::appendToDrawList(TManagedObject *object) {
	_drawList.push_back(object);
	_drawIndex[objectKey(object->GetRef())] = static_cast<int>(_drawList.size()) - 1;
}

// Confirmed (asm lines 171805-172600): rebuilds the draw order. Every
// character, plus every scene object that's currently moving, is sorted by
// depth (GetCenter(), ascending); then the stationary scene objects (in
// their own, already-ordered sequence) are merged with that sorted list so
// the result is back-to-front overall, ties going to the moving/character
// side. Each entry's draw-order index is recorded under its id.
void TGScene::SortAllObjects() {
	std::vector<TManagedObject *> moving(_characters.begin(), _characters.end());
	for (TManagedObject *object : _objects) {
		if (object->IsMovingObject())
			moving.push_back(object);
	}
	std::sort(moving.begin(), moving.end(), TManagedObject::CompObjectCenter);

	_drawList.clear();

	size_t next = 0;
	for (TManagedObject *object : _objects) {
		if (object->IsMovingObject())
			continue;

		while (next < moving.size() && moving[next]->GetCenter() <= object->GetCenter())
			appendToDrawList(moving[next++]);
		appendToDrawList(object);
	}
	while (next < moving.size())
		appendToDrawList(moving[next++]);
}

// Confirmed (asm lines 172600-172880): rebuilds the scene's contents from
// its game data. Its objects (one THObject per field-0x88 link, built in
// reverse so the data's own order ends up back-to-front) get their
// animations reattached; for a menu scene, one click area per rectangle of
// field 0x142 becomes a savegame slot (the first one's size is recorded
// for new savegames) and the savegame list is built; finally the scene's
// characters and draw order are set up and the game's own state reset.
void TGScene::SetScene() {
	TVList links;
	_ref.GetLinks(0x88, TypeOrder::kValue1, links);

	Clear();

	for (auto it = links.items.rbegin(); it != links.items.rend(); ++it)
		_objects.push_back(new THObject(TVisObjRef(*it)));

	for (TManagedObject *object : _objects)
		TGAnimation::ReattachAnimations(*object);

	_selectedSavegame = -1;
	_firstVisibleSavegame = 0;

	if (_ref.GetBool(0x124)) {
		std::vector<wxRect> rects;
		_ref.GetRects(0x142, rects);

		if (!rects.empty()) {
			for (const wxRect &rect : rects)
				_savegameAreas.push_back(new TMSavegameArea(rect));

			_slotWidth = rects.front().GetWidth();
			_slotHeight = rects.front().GetHeight();
			SetSavegames();
		}
	}

	SetCharacters();
	SortAllObjects();
	gameControl()->ResetState();
}

// Confirmed (asm lines 172880-173218) except the Lua branch that builds a
// particle container from a script expression ("return particleSystem:new(
// <field 0x326>)" through LuaDoString(), the userdata it returns adopted as
// _particleContainer) - skipped, the same standing Lua-bridge gap used
// throughout this project; a scene with a non-empty field 0x326 therefore
// gets no particles here. Starts the scene: background, objects and texts
// set up, its particle effect (field 0x1B9, the container flag aside - an
// empty script name falls through to a plain TGParticleSystem initialised
// from the scene) started, running animations continued, and the scene's
// own "scene started" actions (every action link of field 0x14F whose type
// field 0x9F is 31) executed.
void TGScene::BeginScene() {
	InitialiseBackground();
	SetScene();
	gameControl()->ReattachSceneObjectTexts();

	TVisObjRef particleLink = _ref.GetLink(0x1B9);
	_hasParticles = !particleLink.IsEmpty();
	if (_hasParticles && _ref.GetStrHolder(0x326).size() == 0) {
		_particleSystem.Init(particleLink, wxString());
		int windowWidth, windowHeight;
		gameControl()->GetWindowSize(&windowWidth, &windowHeight);
		_particleSystem.SetWindowSize(windowWidth, windowHeight);
		_particleTimer.SetTime();
	}

	_snoopState = 0;
	TGAnimation::ContinueAnimations();

	TVList links;
	_ref.GetLinks(0x14F, TypeOrder::kValue1, links);
	for (TVisionaireObject *link : links.items) {
		if (link->GetInt(0x9F) != 0x1F)
			continue;
		TGAction *action = TGAction::AddRunningAction(TVisObjRef(*link));
		if (action)
			action->Execute(false, nullptr);
	}
}
