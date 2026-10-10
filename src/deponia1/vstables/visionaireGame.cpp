#include "vstables/visionaireGame.h"
#include "vstables/visionaireGameUpgrade.h"

#include <EventHandler.h>

#include "LoadSaveProgressEvent.h"
#include "TXMLNames.h"
#include "datastruct/typegrp.h"
#include "TSText.h"
#include "TTAction.h"
#include "TTButton.h"
#include "TTScene.h"
#include "TTText.h"
#include "Diagnostics.h"
#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/xmlNamesData.h"

// Confirmed (asm lines 1499977-1504534): a TVisionaire of the game type group
// (file version 0xBA), the XML names, and the 39 tables of the game schema,
// registered with the same order, ids and names as the original. The field
// lookup table and the record types are built once per run
// (InitWithVersion), then the (empty) main object is created.
TVisionaireGame::TVisionaireGame() : TVisionaire(TTGame::GetTypeGroup(), 0xBA) {
	static bool initTypeGroups = false;

	InitXMLNames();
	CreateTables(0x27);

	AddTable(351, 21, L"eLoadings", L"Loading", L"Loadings", TTLoading::GetTypeGroup(), -1, true);
	AddTable(105, 0, L"eCharacters", L"Character", L"Characters", TTCharacter::GetTypeGroup(), -1, false);
	AddTable(102, 1, L"eInterfaces", L"Interface", L"Interfaces", TTInterface::GetTypeGroup(), -1, false);
	AddTable(106, 2, L"eButtons", L"Button", L"Buttons", TTButton::GetTypeGroup(), -1, false);
	AddTable(103, 3, L"eFonts", L"Font", L"Fonts", TTFont::GetTypeGroup(), -1, false);
	AddTable(101, 4, L"eScenes", L"Scene", L"Scenes", TTScene::GetTypeGroup(), -1, false);
	AddTable(104, 5, L"ePoints", L"Point", L"Points", TTPoint::GetTypeGroup(), -1, false);
	AddTable(107, 6, L"eObjects", L"Object", L"Objects", TTObject::GetTypeGroup(), -1, false);
	AddTable(108, 7, L"eActions", L"Action", L"Actions", TTAction::GetTypeGroup(), -1, false);
	AddTable(109, 8, L"eActionParts", L"ActionPart", L"ActionParts", TTActionPart::GetTypeGroup(), -1, false);
	AddTable(110, 9, L"eAnimations", L"Animation", L"Animations", TTAnimation::GetTypeGroup(), -1, false);
	AddTable(111, 10, L"eConditions", L"Condition", L"Conditions", TTCondition::GetTypeGroup(), -1, false);
	AddTable(112, 11, L"eDialogs", L"Dialog", L"Dialogs", TTDialog::GetTypeGroup(), -1, false);
	AddTable(113, 12, L"eDialogParts", L"DialogPart", L"DialogParts", TTDialogPart::GetTypeGroup(), -1, false);
	AddTable(114, 13, L"eSprites", L"Sprite", L"Sprites", TTSprite::GetTypeGroup(), -1, false);
	AddTable(115, 14, L"eTexts", L"Text", L"Texts", TTText::GetTypeGroup(), -1, false);
	AddTable(225, 15, L"eCursors", L"Cursor", L"Cursors", TTCursor::GetTypeGroup(), -1, false);
	AddTable(262, 16, L"eCommentSets", L"CommentSet", L"CommentSets", TTCommentSet::GetTypeGroup(), -1, false);
	AddTable(265, 17, L"eOutfits", L"Outfit", L"Outfits", TTOutfit::GetTypeGroup(), -1, false);
	AddTable(260, 18, L"eLanguages", L"Language", L"Languages", TTLanguage::GetTypeGroup(), -1, true);
	AddTable(254, 19, L"eTextLanguages", L"TextLanguage", L"TextLanguages", TTTextLanguage::GetTypeGroup(), -1, false);
	AddTable(326, 20, L"eValues", L"Value", L"Values", TTValue::GetTypeGroup(), -1, false);
	AddTable(390, 22, L"eParticles", L"Particle", L"Particles", TTParticles::GetTypeGroup(), -1, false);
	AddTable(449, 23, L"eParticleContainers", L"ParticleContainer", L"ParticleContainers", TTParticleContainer::GetTypeGroup(), -1, false);
	AddTable(532, 24, L"eActiveTexts", L"ActiveText", L"ActiveTexts", TSText::GetTypeGroup(), 624, false);
	AddTable(534, 25, L"eActiveActions", L"ActiveAction", L"ActiveActions", TSAction::GetTypeGroup(), 625, false);
	AddTable(536, 26, L"eActiveAnimations", L"ActiveAnimation", L"ActiveAnimations", TSAnimation::GetTypeGroup(), 626, false);
	AddTable(557, 27, L"eCommentSetEntries", L"CommentSetEntry", L"CommentSetEntries", TTCommentSetEntry::GetTypeGroup(), -1, false);
	AddTable(569, 28, L"eAnimationFrames", L"AnimationFrame", L"AnimationFrames", TTAnimationFrame::GetTypeGroup(), -1, false);
	AddTable(636, 29, L"eWaySystems", L"WaySystem", L"WaySystems", TTWaySystem::GetTypeGroup(), -1, false);
	AddTable(649, 30, L"eScripts", L"Script", L"Scripts", TTScript::GetTypeGroup(), -1, false);
	AddTable(657, 31, L"eInterfaceClasses", L"InterfaceClass", L"InterfaceClasses", TTInterfaceClass::GetTypeGroup(), -1, false);
	AddTable(672, 32, L"eActionAreas", L"ActionArea", L"ActionAreas", TTActionArea::GetTypeGroup(), -1, false);
	AddTable(676, 33, L"eAreaActions", L"AreaAction", L"AreaActions", TTAreaAction::GetTypeGroup(), -1, false);
	AddTable(692, 34, L"eScriptVariables", L"ScriptVariable", L"ScriptVariables", TTScriptVariable::GetTypeGroup(), -1, false);
	AddTable(704, 35, L"eModels", L"Model", L"Models", TTModel::GetTypeGroup(), -1, false);
	AddTable(816, 36, L"eShaders", L"Shader", L"Shaders", TTShader::GetTypeGroup(), -1, false);
	AddTable(828, 37, L"eAudioBusses", L"AudioBus", L"AudioBusses", TTAudioBus::GetTypeGroup(), -1, false);
	AddTable(833, 38, L"eEvents", L"Event", L"Events", TTEvent::GetTypeGroup(), -1, false);

	CompleteTables();

	if (!initTypeGroups) {
		InitWithVersion(0xBA);
		initTypeGroups = true;
	}

	NewGame();
}

// Confirmed (asm lines 1480082-1480237): clears (allocating on first use) the
// 0x348-entry field lookup table, then builds every record type for the file
// version range [versionLow, the object's version].
void TVisionaireGame::InitWithVersion(int versionLow) {
	if (!g_lookupTable.pEntries) {
		g_lookupTable.pEntries = new TLookupEntry[0x348];
		g_lookupTable.numEntries = 0x348;
	}
	for (int i = 0; i < 0x348; i++)
		g_lookupTable.pEntries[i].valid = false;

	TTGame::InitType(versionLow, _version);
	TTLoading::InitType(versionLow, _version);
	TTCharacter::InitType(versionLow, _version);
	TTInterface::InitType(versionLow, _version);
	TTButton::InitType(versionLow, _version);
	TTFont::InitType(versionLow, _version);
	TTScene::InitType(versionLow, _version);
	TTPoint::InitType(versionLow, _version);
	TTObject::InitType(versionLow, _version);
	TTAction::InitType(versionLow, _version);
	TTActionPart::InitType(versionLow, _version);
	TTAnimation::InitType(versionLow, _version);
	TTCondition::InitType(versionLow, _version);
	TTDialog::InitType(versionLow, _version);
	TTDialogPart::InitType(versionLow, _version);
	TTSprite::InitType(versionLow, _version);
	TTText::InitType(versionLow, _version);
	TTCursor::InitType(versionLow, _version);
	TTCommentSet::InitType(versionLow, _version);
	TTOutfit::InitType(versionLow, _version);
	TTLanguage::InitType(versionLow, _version);
	TTTextLanguage::InitType(versionLow, _version);
	TTValue::InitType(versionLow, _version);
	TTParticles::InitType(versionLow, _version);
	TTParticleContainer::InitType(versionLow, _version);
	TSText::InitType(versionLow, _version);
	TSAction::InitType(versionLow, _version);
	TSAnimation::InitType(versionLow, _version);
	TTCommentSetEntry::InitType(versionLow, _version);
	TTAnimationFrame::InitType(versionLow, _version);
	TTWaySystem::InitType(versionLow, _version);
	TTScript::InitType(versionLow, _version);
	TTInterfaceClass::InitType(versionLow, _version);
	TTActionArea::InitType(versionLow, _version);
	TTAreaAction::InitType(versionLow, _version);
	TTScriptVariable::InitType(versionLow, _version);
	TTModel::InitType(versionLow, _version);
	TTShader::InitType(versionLow, _version);
	TTAudioBus::InitType(versionLow, _version);
	TTEvent::InitType(versionLow, _version);
}

// Confirmed (asm lines 1499936-1499976): TVisionaire::NewGame(), then the new
// game object gets its defaults from TTGame::OnCreate().
bool TVisionaireGame::NewGame() {
	TVisionaire::NewGame();
	TVisObjRef game = GetGame();
	TTGame::OnCreate(game.GetObjectPointer());
	return true;
}

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vstables/visionaireGame.cpp";

void TVisionaireGame::InitXMLNames() {
	static bool init = false;

	if (init)
		return;
	init = true;

	TXMLNames::InitXMLNamesIntern();

	int id = 100;
	for (const char *name : kGameXMLNames)
		TXMLNames::AddXMLName(wxString(name), id++, true);
}

// Confirmed (asm lines 1523439-1523825): the file is looked up by its name
// (relative to the current directory; with `relativeToFile` that directory is
// first set to the one the file is in), loaded as game data, brought up to date
// and the result marked unmodified.
bool TVisionaireGame::LoadDataGame(const wxFileName &file, const wxString &extra, TLoadingTypeEnum type,
                                   bool relativeToFile, TSignalSlot *slot, EventHandler *handler) {
	wxFileName path;

	if (relativeToFile) {
		path = file;
		if (!path.FileExists())
			return false;

		path.SetCwd();
		path.Clear();
	}
	path.SetFullName(file.GetFullName());

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(wxString(L"ProgressLoad")));

	int version = 0;
	if (!Load(path, extra, eSaveGame::kValue0, type, &version, slot, handler))
		return false;

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(wxString(L"ProgressUpdateVersion")));

	if (!UpdateVersion(version, type == TLoadingTypeEnum::kValue2)) {
		x_assert(false, "false", kSourceFile, 0x4B7);
		return false;
	}

	if (handler)
		handler->AddPendingEvent(new LoadSaveProgressEvent(wxString(L"ProgressLoadFinalizing")));

	if (version != 0xBA) {
		InitWithVersion(0xBA);
		RemoveTempData();
	}

	SetModified(false);
	return true;
}

// Confirmed (asm lines 1504935-1504976)
bool TVisionaireGame::LoadSaveGame(const wxFileName &file, const wxString &extra) {
	int version = 0;

	if (!Load(file, extra, eSaveGame::kValue1, TLoadingTypeEnum::kValue2, &version, nullptr, nullptr))
		return false;
	return UpdateVersionGame(version);
}

// Confirmed (asm lines 1504534-1504934): the fixes savegames of older versions
// need, each applying to every version up to its own.
bool TVisionaireGame::UpdateVersionGame(int version) {
	TVList list;

	if (version <= 0x29) {
		GetList(9, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef animation(object);

			animation.SetValue(kPlayOppositeDirection, animation.GetBool(kAnimationOppositeDirection),
			                   TSendEventEnum::kNoEvent);
			animation.SetValue(kFrameCount, 0, TSendEventEnum::kNoEvent);
		}
	}

	if (version <= 0x2E) {
		GetList(0, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef character(object);

			character.SetValue(kCharacterInterfaceAlpha, 100, TSendEventEnum::kNoEvent);
			character.SetValue(kCharacterInventoryAlpha, 100, TSendEventEnum::kNoEvent);
			character.SetValue(kCharacterAlpha, 100, TSendEventEnum::kNoEvent);
			character.SetValue(kCharacterDestAlpha, 100, TSendEventEnum::kNoEvent);
			character.SetValue(kCharacterTimeToDestAlpha, 0, TSendEventEnum::kNoEvent);
		}

		TVisObjRef game = GetGame();
		game.SetValue(kGameSceneBrightness, 100, TSendEventEnum::kSendEvent);
	}

	if (version <= 0x31) {
		TVisObjRef game = GetGame();
		game.SetValue(kGameScrollCenterCharacter, true, TSendEventEnum::kNoEvent);
	}

	if (version <= 0x3A) {
		TVisObjRef game = GetGame();
		game.SetValue(kGameTextOutput, 0, TSendEventEnum::kNoEvent);
	}

	if (version <= 0x7C) {
		GetList(0x18, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef text(object);
			TVisObjRef speaker = text.GetLink(kTextSpeaker);

			text.SetLink(kTextOwner, speaker, false);
		}
	}

	if (version <= 0x84) {
		GetList(0x1A, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef animation(object);

			animation.SetValue(kAnimationStartedByUser, true, TSendEventEnum::kNoEvent);
		}
	}

	if (version <= 0x9A) {
		GetList(0x18, list, false);
		for (TVisionaireObject *object : list) {
			TVisObjRef text(object);

			text.SetValue(kTextAlignment, 0, TSendEventEnum::kNoEvent);
		}
	}

	return true;
}

// Confirmed (asm lines 1508677-1523438, a 14000-line function): the project upgrade. The code is
// laid out as a chain of "if (version <= N) fix" for N from 0x63 up (the compiler threaded them into
// one jump table of entries); the fixes of the file versions up to 0xAC are in visionaireGameUpgrade.cpp
// and run first, oldest first. Then comes the part every version gets (read off the code path for
// the current version, 0xBA, which no per-version fix applies to):
//
//  - a TTObject at the "unset" position (-1, -1) that isn't an item is put at
//    the first point of its scene's current way system;
//  - random TTValues draw their value;
//  - the game's scroll distances come from its window resolution;
//  - (unless `allAtOnce`) every animation with a mirror link takes over that
//    animation's content, marked mirrored, and gets a minimum pause; the
//    "set an item" action parts get a minimum int; and a game that has scenes
//    must have a first character;
//
// and last the fixes for the file versions 0xB3-0xB8 (the smooth scrolling and the steps of the walk).
//
// NOT reconstructed: the upgrade step of version 0x71 (about 1300 lines of the asm), and the step of
// version 0xB3 (it makes the lists of the files an editor packs into its containers, the
// kGameContainers/kGameBuildRules settings that only the editor's build reads). A file that old is
// accepted without them, with a warning in the log; and games older than version 99 are rejected as in
// the original.
bool TVisionaireGame::UpdateVersion(int version, bool allAtOnce) {
	TVList list;
	TVisObjRef game = GetGame();

	if (version <= 0x62) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"only games in Version >= 99 are supported... (please convert your game from 2.x to 3.0 with the converter tool)");
		return false;
	}

	if (version <= 0xAC)
		applyVersionFixes(*this, version);

	// objects
	GetList(6, list, false);
	for (TVisionaireObject *object : list) {
		TTObject item((TVisObjRef(object)));

		if (item.GetBool(kObjectIsItem) || !(*item.GetPoint(kObjectPosition) == wxPoint{-1, -1}))
			continue;

		TVisObjRef scene = item.GetParent();
		TVisObjRef waySystem = scene.GetLink(kSceneCurrentWaySystem);
		TVList points;

		waySystem.GetLinks(kWaySystemPoints, static_cast<TypeOrder>(3), points);
		if (points.size() != 0)
			item.SetValue(kObjectPosition, *(*points.begin())->GetPoint(kPointPosition), TSendEventEnum::kSendEvent);
	}

	// values
	GetList(0x14, list, false);
	for (TVisionaireObject *object : list) {
		TTValue value((TVisObjRef(object)));

		if (value.GetBool(kValueRandom))
			value.SetRandomValue(value.GetInt(kValueRandomMin), value.GetInt(kValueRandomMax));
	}

	// the scroll distances
	const wxPoint *resolution = game.GetPoint(kGameWindowResolution);
	int horizontal = 100;
	int vertical = 30;
	if (resolution->x > 0 && resolution->y > 0) {
		horizontal = resolution->x / 6;
		vertical = resolution->y / 16;
	}
	game.SetValue(kGameHorizontalScrollDistance, horizontal, TSendEventEnum::kNoEvent);
	game.SetValue(kGameVerticalScrollDistance, vertical, TSendEventEnum::kNoEvent);

	if (allAtOnce) {
		applyLateVersionFixes(*this, version);
		return true;
	}

	// animations that mirror another one
	GetList(9, list, false);
	for (TVisionaireObject *object : list) {
		TTAnimation animation((TVisObjRef(object)));
		TVisObjRef mirror = animation.GetLink(kAnimationMirror);

		if (!mirror.IsEmpty()) {
			if (animation == mirror && wxLog::loglevel > 0)
				wxLog::logexpanded(L"The animation '%ls' (id: %d) mirrors itself.", animation.GetNameWithParents(3).c_str(),
				                   animation.GetObjectPointer()->GetId24());

			int direction = animation.GetInt(kAnimationDirection);
			animation.CopyContent(mirror, false, nullptr);
			animation.SetValue(kAnimationDirection, direction, TSendEventEnum::kSendEvent);
			animation.SetMirrored(true);

			TVList frames;
			mirror.GetLinks(kAnimationPropertyFrames, static_cast<TypeOrder>(1), frames);
			if (animation.GetLinksListSize(kAnimationPropertyFrames) == 0)
				animation.SetValue(kAnimationPropertyFrames, frames, true);
		}

		if (animation.GetInt(kAnimationPause) <= 0xF)
			animation.SetValue(kAnimationPause, 0xF, TSendEventEnum::kSendEvent);
	}

	// action parts
	GetList(8, list, false);
	for (TVisionaireObject *object : list) {
		if (object->GetInt(kActionPartCommand) == 0x1E && object->GetInt(kActionPartAltInt) == 0 &&
		    object->GetInt(kActionPartInt) <= 0xF)
			object->SetValue(kActionPartInt, 0xF, TSendEventEnum::kSendEvent);
	}

	// a game with scenes needs its first character
	if (GetListSize(4) != 0 && game.GetLink(kGameFirstCharacter).IsEmpty()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"The game must have one character activated as first character in the game settings");
		return false;
	}

	applyLateVersionFixes(*this, version);
	return true;
}
