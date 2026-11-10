// The record callbacks tools/gen_callbacks.py could not translate mechanically
// (they create and link sub-objects, or compute something), written by hand
// from the same sources (Deponia_Linux.asm lines noted per function).
//
// OnCreate() runs when an object is *created* (TTable::CreateObject), which
// only the editor and the savegame code do; the player loads its objects from
// the data files. The savegame classes (TS*) are generated; the editor-side
// ones that are not reconstructed yet are empty and say so.

#include "vstables/records.h"

#include <cstdlib>
#include <ctime>

#include "TTAction.h"
#include "TTButton.h"
#include "TTScene.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// The names a new action gets by where it hangs (asm lines 1478536-1478560:
// all three are initialized from the literal "Action").
wxString TTAction::strAction(L"Action");
wxString TTAction::strRightClickAction(L"Action");
wxString TTAction::strStartAction(L"Action");

// Confirmed (asm lines 1477463-1477598)
void TTAction::OnCreate(TVisionaireObject *object) {
	TVisionaireObject *parent = object->GetParent();
	if (!parent)
		return;

	if (parent->GetTId().getTable() == 0)
		object->SetName(TCharHolder(strAction));
	if (object->GetParentField() == kGameRightClickAction)
		object->SetName(TCharHolder(strRightClickAction));
	if (object->GetParentField() == kGameStartAction)
		object->SetName(TCharHolder(strStartAction));
	if (parent->GetTId().getTable() == 6)
		object->SetValue(kActionExecutionType, 6, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1478... TTAreaAction::OnCreate): an area action starts
// with a new action (of execution type 0x1D) and an "any" character.
void TTAreaAction::OnCreate(TVisionaireObject *object) {
	TVisObjRef parent(object);
	TVisObjRef action = object->GetVisionaire()->CreateObject(7, parent, kAreaActionAction);

	action.SetValue(kActionExecutionType, 0x1D, TSendEventEnum::kNoEvent);
	object->SetLinkAnyObject(kAreaActionCharacter, false);
}

// Confirmed: a new character gets a name text, a first outfit (its current
// one) and default scale/dialog values.
void TTCharacter::OnCreate(TVisionaireObject *object) {
	TVisObjRef self(object);

	object->GetVisionaire()->CreateObject(14, self, kCharacterName);
	TVisObjRef outfit = object->GetVisionaire()->CreateObject(17, self, kCharacterOutfits);
	self.SetLink(kCharacterCurrentOutfit, outfit, true);

	object->SetValue(kCharacterScale, true, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterScaleFactor, 100, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterDialogVerticalSpace, 10, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterActionDestPosition, wxPoint{-1, -1}, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1461061-1461702; editor only, but the test games are made with it): the defaults of a new game
// object - the loading record, the language, the first interface classes, the sizes of the picture buffers and a random
// game id.
void TTGame::OnCreate(TVisionaireObject *object) {
	TVisObjRef game(object);
	TVisionaire *visionaire = object->GetVisionaire();

	visionaire->CreateObject(21, game, kGameLoading);
	object->SetValue(kGameFadeDelay, 600, TSendEventEnum::kNoEvent);
	object->SetValue(kGameFadeEffect, 3, TSendEventEnum::kNoEvent);
	object->SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
	object->SetValue(kGameScrollSpeed, 300, TSendEventEnum::kNoEvent);
	object->SetValue(kGameHoldTime, 600, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMinificationFilter, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMagnificationFilter, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameLeftHoldBehaviour, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMiddleClickBehaviour, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameRightClickBehaviour, 0, TSendEventEnum::kNoEvent);

	TVisObjRef language = visionaire->CreateObject(18, game, kGameLanguages);

	object->SetLink(kGameStandardLanguage, language.GetObjectPointer()->GetTId(), false);
	object->SetValue(kGameCommandBehaviour, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameAlwaysAllowSkipText, false, TSendEventEnum::kNoEvent);
	object->SetValue(kGameDrawActionText, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kGameSceneComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCharacterComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameInterfaceComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameGameComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMovieComposedFiles, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameSpeakerTextAlignment, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameTextAlignment, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameKeepCharacterSpritesDuringMenus, true, TSendEventEnum::kNoEvent);
	object->SetValue(kGameDisableInteractionDuringAnim, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameAutoHideInterfacesInMenu, true, TSendEventEnum::kNoEvent);

	static const wchar_t *const kInterfaceClassNames[] = {
		L"MainInterface", L"Inventory", L"SecondaryInterface", L"OptionsInterface", L"Miscellaneous"
	};

	for (const wchar_t *name : kInterfaceClassNames) {
		TVisObjRef interfaceClass = visionaire->CreateObject(31, game, kGameInterfaceClasses);

		interfaceClass.SetValue(kInterfaceClassName, wxString(name), TSendEventEnum::kSendEvent);
	}

	object->SetValue(kGamePictureCacheSize, 40, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCursorHorizontalScrollDistance, 5, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCursorVerticalScrollDistance, 5, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePreloadPicThreads, 3, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePreloadedPicBufferSize, 0x4000, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePicBufferSize, 0x1D4C0, TSendEventEnum::kNoEvent);
	object->SetValue(kSpeakerSoundPanFactor, 8, TSendEventEnum::kNoEvent);
	object->SetValue(kGameExecuteCalledActionImmediately, true, TSendEventEnum::kNoEvent);

	// the id of the game is a random number
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	int identifier = std::rand();

	object->SetValue(kGameId, CONVTOSTR(identifier), TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1466337-1466431; editor only): a new interface moves by one, scrolls its items by one and is
// visible, and takes the first interface class of the game.
void TTInterface::OnCreate(TVisionaireObject *object) {
	object->SetValue(kInterfaceDisplacement, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceScrollStepSize, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceDisplacement, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceVisible, true, TSendEventEnum::kNoEvent);

	TVList classes;

	object->GetVisionaire()->GetGame().GetLinks(kGameInterfaceClasses, TypeOrder::kValue1, classes);
	if (!classes.empty())
		object->SetLink(kInterfaceInterfaceClass, classes.front()->GetTId(), false);
}

// Confirmed (asm lines 1478613-1479177; editor only): a new outfit walks and stands in the four directions (front, back,
// left, right), and has the defaults of the 3D model shown with it and of the random animations.
void TTOutfit::OnCreate(TVisionaireObject *object) {
	TVisObjRef outfit(object);
	TVisionaire *visionaire = object->GetVisionaire();

	object->SetValue(kOutfitCharacterSpeed, 350, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitSlideWalkAnimation, true, TSendEventEnum::kSendEvent);

	static const struct {
		int direction;
		const wchar_t *name;
	} kDirections[] = {
		{270, L"front"}, {90, L"back"}, {180, L"left"}, {0, L"right"}
	};
	static const int kLists[] = {kOutfitWalkAnimations, kOutfitStandingAnimations};

	for (int list : kLists) {
		for (const auto &direction : kDirections) {
			TVisObjRef animation = visionaire->CreateObject(9, outfit, list);

			animation.SetValue(kAnimationDirection, direction.direction, TSendEventEnum::kSendEvent);
			animation.SetName(TCharHolder(direction.name));
		}
	}

	object->SetValue(kOutfitCameraHeight, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitCameraAngle, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitLightPosX, 1.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitLightPosY, 1.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitLightPosZ, 1.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitLightColor, 0x8E9999, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelScaleFactor, 100, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelShadowVisibility, 50, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelShadow, 2, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelUseToonShading, false, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelToonNuances, 3, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelShowOutline, false, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelOutlineColor, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelOutlineWidth, 5, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitAmbientLightColor, 0x667F7F, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitModelAmbientOcclusion, 50, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitRandomMinTime, 10000, TSendEventEnum::kSendEvent);
	object->SetValue(kOutfitRandomMaxTime, 30000, TSendEventEnum::kSendEvent);
	// Confirmed (Night of the Rabbit, TTOutfit::OnCreate): the turn speed of a model character (the upgrade of old data gives 100 too)
	object->SetValue(kOutfitModelTurnSpeed, 100, TSendEventEnum::kSendEvent);
}

// Confirmed (Night of the Rabbit, TTParticles::OnCreate, asm lines 1611495-1611913; Deponia 1's sets only six of these): the defaults of a new
// particle system of the editor: soft additive material, one tile, the type 1 (a tile by chance) at (200, 200) with a point emitter, 250
// particles of 0.03 to 0.3, 60 steps of life, the curves 100 numbers long (the colour curves and the tile curve 0, the size curve 1, the
// control points of the editor 1 at both ends).
void TTParticles::OnCreate(TVisionaireObject *object) {
	const std::vector<float> zeros(100, 0.0f);
	const std::vector<float> ones(100, 1.0f);
	std::vector<int> controls(100, 0);

	controls[0] = 1;
	controls[99] = 1;

	object->SetValue(kParticleTexture, TCharHolder(), TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaterialMode, 2, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleTilesX, 1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleTilesY, 1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleType, 1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticlePosX, 200, TSendEventEnum::kSendEvent);
	object->SetValue(kParticlePosY, 200, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleEmitterType, 1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleEmitterSizeX, 200, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleEmitterSizeY, 200, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleEmitterSizeZ, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaxParticles, 250, TSendEventEnum::kSendEvent);
	object->SetValue(kParticlePerTime, 1.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticlePhiX, 22, TSendEventEnum::kSendEvent);
	object->SetValue(kParticlePhiY, 22, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMinSize, 0.0299999993f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaxSize, 0.300000012f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMinVelocity, 0.00999999978f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaxVelocity, 0.0199999996f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMinLife, 60, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaxLife, 60, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMinColor, 8355711, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleMaxColor, 16777215, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleRotationX, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleRotationZ, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleActiveStages, 1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColor0, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColor1, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColor2, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColor3, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColorCurve0, zeros, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColorCurve1, zeros, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColorCurve2, zeros, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendColorCurve3, zeros, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleColorControlPoints0, controls, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleColorControlPoints1, controls, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleColorControlPoints2, controls, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleColorControlPoints3, controls, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceType, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceStrength, 0.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceRotationX, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceRotationZ, 0, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceXPos, 0.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceYPos, 0.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceZPos, 0.0f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleForceBlend, 0.999000013f, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendSizeCurve, ones, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleSizeControlPoints, controls, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendMaxTile, -1, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendTilesCurve, zeros, TSendEventEnum::kSendEvent);
	object->SetValue(kParticleBlendTilesControlPoints, controls, TSendEventEnum::kSendEvent);
}

// The editor-list names of actions parts and texts (a long switch over the
// command / the text language) are not reconstructed; the plain name stands in.
wxString TTActionPart::GetNameInList(const TVisionaireObject *object) {
	return wxString(object->GetName());
}

wxString TTText::GetNameInList(const TVisionaireObject *object) {
	return wxString(object->GetName());
}
