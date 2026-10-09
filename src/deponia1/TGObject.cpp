#include "TGObject.h"

#include "AppGlobals.h"
#include "TGDetectInfo.h"
#include "TGScene.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/particles.h"
#include "vstables/records.h"
#include "graphicslib/graphics.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// Confirmed (asm lines 259457-259625). The object starts inactive (TMObject), and is set
// active when the data says it is shown at the start (kObjectConditionNegate) and its
// condition is not met, or the other way round.
TGObject::TGObject(const TVisObjRef &ref) : TMObject(ref, true) {
	std::vector<wxPoint> polygon;

	_objRef.GetPoints(kObjectPolygon, polygon);
	SetPolygon(polygon);

	_bypassReachCheck = false;
	_skipFinalPostExecution = false;

	_position = *_objRef.GetPoint(kObjectPosition);
	_center = _objRef.GetInt(kObjectCenter);

	// the scroll factors are in percent; the object only scrolls on its own when one is not 100
	_scrollX = _objRef.GetInt(kObjectScrollFactorX);
	_scrollY = _objRef.GetInt(kObjectScrollFactorY);
	_scrolls = (_scrollX != 100 || _scrollY != 100);
	_scrollX -= 100;
	_scrollY -= 100;
	_sprite.SetParallax(_scrollX, _scrollY);

	_sprite.SetRotation(_objRef.GetFloat(kObjectRotation));
	_sprite.SetRotationCenter(*_objRef.GetPoint(kObjectRotationCenter));
	_sprite.SetScale(_objRef.GetFloat(kObjectScaleX), _objRef.GetFloat(kObjectScaleY));
	_sprite.SetMatrixId(_objRef.GetInt(kObjectMatrixId));
	_sprite.SetShader(_objRef.GetInt(kObjectShaderSet));

	_particleContainer = nullptr;
	_particlesFromScript = false;
	_hasParticles = !_objRef.GetLink(kObjectParticleSystem).IsEmpty();
	_snoopAnimation = nullptr;

	TTCondition condition(_objRef.GetLink(kObjectCondition));

	SetActive(_objRef.GetBool(kObjectConditionNegate) != condition.IsTrue());
}

// Confirmed (asm lines 258656-258702)
TGObject::~TGObject() {
	if (_snoopAnimation) {
		TGAnimation::HideAnimation(_snoopAnimation, this);
		_snoopAnimation = nullptr;
	}

	delete _particleContainer;
}

// Confirmed (asm lines 258158-258216)
void TGObject::ShowSnoopAnimation(bool show) {
	TVisObjRef animation = _objRef.GetLink(kObjectSnoopAnimation);

	if (!show) {
		if (_snoopAnimation) {
			TGAnimation::HideAnimation(_snoopAnimation, this);
			_snoopAnimation = nullptr;
		}
		return;
	}

	if (!animation.IsEmpty() && !_snoopAnimation)
		_snoopAnimation = TGAnimation::StartAnimation(animation, this, false, 100.0f, -1);
}

// Confirmed (asm lines 258229-258241)
int TGObject::GetCenter() const {
	return _center + _objRef.GetPoint(kObjectOffset)->y;
}

// Confirmed (asm lines 258252-258535), except the drawing of the particle effect through
// the graphics backend (translating by the scroll position, the matrix multiplication
// and the draw calls), which is skipped like in TGScene::Draw(); the container's own
// update and the particle system's fixed-step catch-up are kept. TODO (low priority, see
// /TODO.md): the shader, the matrix id and the particle drawing.
void TGObject::Draw() {
	// the colour is the scene's brightness
	float brightness = gameControl()->GetScene()->GetBrightness();

	_color = 0xFFFFFFFF;

	if (brightness != 1.0f) {
		unsigned int level = (unsigned char)(int)(brightness * 255.0f);

		_color = 0xFF000000 | level | (level << 8) | (level << 16);
	}

	// an object that is not shown through a matrix is drawn without them
	bool savedMatrices = false;

	if (_sprite.GetMatrixId() == 0) {
		savedMatrices = matricesActive;
		matricesActive = false;
	}

	// the shader of the object, or the default one when it has none (-1)
	int shader = _objRef.GetInt(kObjectShaderSet);

	if (defaultShader != 0 && shader == -1)
		shader = defaultShader;

	_sprite.SetShader(shader);
	ShaderCallback(shader, &_objRef);

	TManagedObject::Draw();

	if (_active && _hasParticles) {
		if (_particlesFromScript && _particleContainer) {
			_particleContainer->Update(false, vec2(), 0.016666668f, true);
			_particleContainer->Draw();
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

	if (_sprite.GetMatrixId() == 0)
		matricesActive = savedMatrices;
}

// Confirmed (asm lines 258546-258552)
bool TGObject::IsWalkable() const {
	return _objRef.GetBool(kObjectIsWalkable);
}

// Confirmed (asm lines 258563-258587)
wxPoint TGObject::GetPosition() const {
	return *_objRef.GetPoint(kObjectOffset) + _position;
}

// Confirmed (asm lines 258598-258621)
void TGObject::AlignCharacter(TVisObjRef &character) const {
	int direction = _objRef.GetInt(kObjectDirection);

	if (direction != -1)
		character.SetValue(kCharacterDirection, direction, TSendEventEnum::kSendEvent);
}

// Confirmed (asm lines 258632-258645)
void TGObject::AnimationStopped(TGAnimation *animation) {
	if (_snoopAnimation == animation)
		_snoopAnimation = nullptr;
	else
		TManagedObject::AnimationStopped(animation);
}

// Confirmed (asm lines 258763-258921). TODO (low priority, see /TODO.md): while the object
// is drawn through a matrix (kObjectMatrixId, with an inverse matrix set) the original
// first moves the point back through that matrix.
bool TGObject::IsInside(const wxPoint &position, const TGDetectInfo &info) const {
	if (!info.flagA)
		return false;

	wxPoint local = position - *_objRef.GetPoint(kObjectOffset);

	// an object that scrolls on its own is further along by that part of the scroll position
	if (_scrolls) {
		const wxPoint &scroll = gameControl()->GetScene()->GetScrollPos();

		local.x += scroll.x * _scrollX / 100;
		local.y += scroll.y * _scrollY / 100;
	}

	return TManagedObject::IsInside(local);
}

// Confirmed (asm lines 258931-259137). TODO (low priority, see /TODO.md): the matrix
// transform of the position.
void TGObject::DrawSnoopAnimation() {
	if (!_active || !_snoopAnimation || !_snoopAnimation->IsSpriteIndexValid())
		return;

	TGScene *scene = gameControl()->GetScene();
	wxPoint position = *_objRef.GetPoint(kObjectSnoopAnimationPos);

	if (_scrolls) {
		const wxPoint &scroll = scene->GetScrollPos();

		position.x -= scroll.x * _scrollX / 100;
		position.y -= scroll.y * _scrollY / 100;
	}

	// (drawn without the matrices)
	bool savedMatrices = matricesActive;

	matricesActive = false;
	_snoopAnimation->SetPosition(position, -1.0f);
	_snoopAnimation->Draw(scene->GetSnoopAnimAlpha(), 0xFFFFFFFF, -1);
	matricesActive = savedMatrices;
}

// Confirmed (asm lines 259145-259300). Switching on an object with particles: a script (field
// 0x326) builds the container ("return particleSystem:new(<script>)" through LuaDoString(),
// the container of the userdata it returns taken over), else a plain TGParticleSystem is
// initialised from the particle system the object links to.
void TGObject::SetActive(bool active) {
	if (_active == active)
		return;

	if (_hasParticles && active) {
		TVisObjRef particleSystem = _objRef.GetLink(kObjectParticleSystem);

		if (particleSystem.GetStrHolder(kParticleContainerSettings).size() != 0) {
			// the container from the script
			delete _particleContainer;
			_particleContainer = nullptr;

			std::string code = "return particleSystem:new(";

			code += particleSystem.GetStrHolder(kParticleContainerSettings).mb_str();
			code += ")";
			LuaDoString(code);
			_particleContainer = takeParticleContainer();
			_particlesFromScript = true;
		} else {
			_particleSystem.Init(particleSystem, wxString());

			int width = 0, height = 0;

			gameControl()->GetWindowSize(&width, &height);
			_particleSystem.SetWindowSize(width, height);
		}
	}

	TMObject::SetActive(active);

	if (_currentAnimation && _scrolls)
		_currentAnimation->SetParallax(_scrollX, _scrollY);
}
