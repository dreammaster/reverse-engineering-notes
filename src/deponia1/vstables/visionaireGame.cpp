#include "vstables/visionaireGame.h"

#include "TXMLNames.h"
#include "datastruct/typegrp.h"
#include "TSText.h"
#include "TTAction.h"
#include "TTButton.h"
#include "TTScene.h"
#include "TTText.h"
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

void SaveGlobalScriptVariables(TVisionaireGame &/*game*/) {
}

void LoadGlobalScriptVariables(TVisionaireGame &/*game*/) {
}
