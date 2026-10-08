// The constants the scripts get as globals: the id of every field (SetField), the number of every
// table (SetTables), and the enums (SetEnums). They are in the Lua state when the scripts run, so a
// script can say `GetInt(obj, VName)` or `eFadeIn` and not the numbers.
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"

#include "TXMLNames.h"
#include "WxStub.h"
#include "datastruct/visionaire.h"
#include "vstables/visionaireGame.h"

// Confirmed (asm lines 1401184-1401400): the global is named "V" and then the XML name of the field
// (the prefix is the wide string dword_E05920), and holds the id of the field.
void SetField(int field) {
	lua_pushnumber(L, field);

	wxString name = wxString(L"V") + TXMLNames::GetString(field);

	lua_setfield(L, LUA_GLOBALSINDEX, name.mb_str());
}

// Confirmed (asm lines 1401400-1401427): the ids of the fields; the last is 0x347.
void SetFields() {
	for (int field = 0x65; field != 0x348; field++)
		SetField(field);
}

// Confirmed (asm lines 1401427-1401525): every table of the game's data (-1 to 0x26) is a global with
// the name of the table, holding its number.
void SetTables() {
	for (int table = -1; table != 0x27; table++) {
		lua_pushnumber(L, table);

		lua_setfield(L, LUA_GLOBALSINDEX, luaGame->GetVisTableName(table, true).mb_str());
	}
}

namespace {

struct EnumConstant {
	const char *name;
	int value;
};

// Confirmed (asm lines 1401525-1405988): in the order the original sets them. A few names occur twice
// (eButtonNormal, eButtonCombined, eButtonGive: the same values, for the two kinds of buttons), and a
// few are shared by enums that count from 0 again (eKeySpace ... ). The names of the easings are
// "ease" + the kind + In, Out or InOut, which the original builds as strings.
const EnumConstant kEnumConstants[] = {
	{"eTransparencyUndefined", -1},
	{"eTransparencyAlpha", 2},
	{"eTransparencyNone", 0},
	{"eTransparencyColorKey", 1},
	{"eExecutionTypeRightMouseClick", 0},
	{"eExecutionTypeMouseEntersArea", 1},
	{"eExecutionTypeMouseLeavesArea", 2},
	{"eExecutionTypeFixtureDropped", 3},
	{"eExecutionTypeCalledByOtherAction", 5},
	{"eExecutionTypeActionCommand", 6},
	{"eExecutionTypeLeftMouseClickIm", 7},
	{"eExecutionTypeLeftMouseDblClick", 12},
	{"eExecutionTypeLeftMouseDblClickIm", 13},
	{"eExecutionTypeLeftMouseHold", 14},
	{"eExecutionTypeLeftMouseHoldIm", 15},
	{"eExecutionTypeRightMouseClickIm", 16},
	{"eExecutionTypeActionCommandIm", 17},
	{"eExecutionTypeCommandMouseClickIm", 18},
	{"eExecutionTypeCombinedCommandMouseClickIm", 19},
	{"eExecutionTypeFixtureDroppedIm", 20},
	{"eExecutionTypeActionCommandBoth", 21},
	{"eExecutionTypeActionCommandImBoth", 22},
	{"eExecutionTypeActionCommandOther", 23},
	{"eExecutionTypeActionCommandImOther", 24},
	{"eExecutionTypeFixtureDroppedBoth", 25},
	{"eExecutionTypeFixtureDroppedImBoth", 26},
	{"eExecutionTypeFixtureDroppedOther", 27},
	{"eExecutionTypeFixtureDroppedImOther", 28},
	{"eAtBeginningOfScene", 31},
	{"eAtEndOfScene", 32},
	{"eReplayNormal", 0},
	{"eReplayReverse", 1},
	{"eReplayRandom", 2},
	{"eButtonNormal", 0},
	{"eButtonCombined", 1},
	{"eButtonGive", 2},
	{"eButtonPlaceholder", 0},
	{"eButtonScrollUp", 1},
	{"eButtonScrollDown", 2},
	{"eButtonCommand", 3},
	{"eButtonActionArea", 4},
	{"eButtonCommandGroup", 5},
	{"eButtonCommandInGroup", 6},
	{"eButtonNormal", 0},
	{"eButtonCombined", 1},
	{"eButtonGive", 2},
	{"eUseOnAll", 0},
	{"eUseOnCharacters", 1},
	{"eUseOnObjects", 2},
	{"eNoAnim", 0},
	{"eWalkAnim", 1},
	{"eTalkAnim", 2},
	{"eStandingAnim", 3},
	{"eCharacterAnim", 4},
	{"eRandomAnim", 5},
	{"eDialogReturnToSame", 0},
	{"eDialogReturnToUpper", 1},
	{"eDialogReturnToEnd", 2},
	{"eLinearInterpolation", 0},
	{"eNearestNeighborInterpolation", 1},
	{"eTriLinearInterpolation", 2},
	{"eMainContainer", 0},
	{"eSingleContainer", 1},
	{"eMultipleContainers", 2},
	{"eSetStdCommandAlways", 0},
	{"eSetStdCommandNever", 1},
	{"eSetStdCommandOnSuccess", 2},
	{"eMouseActionBehaviourDoNotSendCharacter", 0},
	{"eMouseActionBehaviourSendCharacterToCursor", 1},
	{"eMouseActionBehaviourSendCharacterToObjects", 2},
	{"eTextAndSpeechOutput", 0},
	{"eOnlySpeechOutput", 1},
	{"eOnlyTextOutput", 2},
	{"eDrawNoActionText", 0},
	{"eDrawActionTextAtCurrentPos", 1},
	{"eDrawActionTextAtRect", 2},
	{"eDisableInteractionNever", 0},
	{"eDisableInteractionCharacterAnim", 1},
	{"eDisableInteractionAlways", 2},
	{"eShaderExcludeNothing", 0},
	{"eShaderExcludeInterfaces", 1},
	{"eShaderExcludeTextsAndCursor", 2},
	{"eShaderExcludeCursor", 3},
	{"eFadeNo", 0},
	{"eFadeIn", 1},
	{"eFadeOut", 2},
	{"eFadeInAndOut", 3},
	{"eFadeToNew", 4},
	{"eShiftLeft", 5},
	{"eShiftRight", 6},
	{"eTunnelEffect", 7},
	{"eTunnelEffectFadeOut", 8},
	{"eTunnelEffectFadeIn", 9},
	{"eFadeKeep", 10},
	{"eFadeShader", 11},
	{"eInterfaceDisplacementTop", 0},
	{"eInterfaceDisplacementBottom", 1},
	{"eInterfaceDisplacementLeft", 2},
	{"eInterfaceDisplacementRight", 3},
	{"eInterfaceDisplacementAbsolute", 4},
	{"eInterfaceDisplacementRelative", 5},
	{"eScriptTypeExecution", 0},
	{"eScriptTypeDefinition", 1},
	{"eAlignLeft", 0},
	{"eAlignRight", 1},
	{"eAlignCentered", 2},
	{"eAlignLeftWithCenterPos", 3},
	{"eAlignRightWithCenterPos", 4},
	{"eAlignCenteredWithLeftPos", 5},
	{"eMusicVolume", 0},
	{"eSoundVolume", 1},
	{"eSpeechVolume", 2},
	{"eMovieVolume", 3},
	{"eGlobalVolume", 4},
	{"eEvtMouseMove", 1},
	{"eEvtMouseLeftButtonDoubleClick", 2},
	{"eEvtMouseLeftButtonDown", 3},
	{"eEvtMouseLeftButtonUp", 4},
	{"eEvtMouseLeftButtonHold", 5},
	{"eEvtMouseLeftButtonHolding", 6},
	{"eEvtMouseRightButtonDoubleClick", 7},
	{"eEvtMouseRightButtonDown", 8},
	{"eEvtMouseRightButtonUp", 9},
	{"eEvtMouseMiddleButtonDown", 10},
	{"eEvtMouseMiddleButtonUp", 11},
	{"eEvtMouseWheelUp", 12},
	{"eEvtMouseWheelDown", 13},
	{"eEvtMultiGesture", 14},
	{"eEvtDollarGesture", 15},
	{"eEvtTouchDown", 16},
	{"eEvtTouchUp", 17},
	{"eEvtTouchMove", 18},
	{"eEvtKeyUp", 2},
	{"eEvtKeyDown", 1},
	{"eEvtKeyTextInput", 3},
	{"eEvtControllerKeyUp", 5},
	{"eEvtControllerKeyDown", 4},
	{"eEvtControllerAxis", 6},
	{"eMsgControllerConnected", 7},
	{"eMsgControllerDisconnected", 8},
	{"eMsgControllerRemapped", 9},
	{"eKeyModLShift", 1},
	{"eKeyModRShift", 2},
	{"eKeyModLCtrl", 64},
	{"eKeyModRCtrl", 128},
	{"eKeyModLAlt", 256},
	{"eKeyModRAlt", 512},
	{"eKeyModLGui", 1024},
	{"eKeyModRGui", 2048},
	{"eKeyModNum", 4096},
	{"eKeyModCaps", 8192},
	{"eKeyModMode", 16384},
	{"eKeyModCtrl", 192},
	{"eKeyModShift", 3},
	{"eKeyModAlt", 768},
	{"eKeyModGui", 3072},
	{"eKeyReturn", 13},
	{"eKeyEscape", 27},
	{"eKeyBackspace", 8},
	{"eKeyTab", 9},
	{"eKeySpace", 32},
	{"easeBackIn", 0},
	{"easeBackOut", 1},
	{"easeBackInOut", 2},
	{"easeBounceIn", 3},
	{"easeBounceOut", 4},
	{"easeBounceInOut", 5},
	{"easeCircIn", 6},
	{"easeCircOut", 7},
	{"easeCircInOut", 8},
	{"easeCubicIn", 9},
	{"easeCubicOut", 10},
	{"easeCubicInOut", 11},
	{"easeElasticIn", 12},
	{"easeElasticOut", 13},
	{"easeElasticInOut", 14},
	{"easeLinearIn", 15},
	{"easeLinearOut", 16},
	{"easeLinearInOut", 17},
	{"easeNoneIn", 18},
	{"easeNoneOut", 19},
	{"easeNoneInOut", 20},
	{"easeQuadIn", 21},
	{"easeQuadOut", 22},
	{"easeQuadInOut", 23},
	{"easeQuartIn", 24},
	{"easeQuartOut", 25},
	{"easeQuartInOut", 26},
	{"easeQuintIn", 27},
	{"easeQuintOut", 28},
	{"easeQuintInOut", 29},
	{"easeSineIn", 30},
	{"easeSineOut", 31},
	{"easeSineInOut", 32},
};

} // End of anonymous namespace

void SetEnums() {
	for (const EnumConstant &constant : kEnumConstants) {
		lua_pushnumber(L, constant.value);
		lua_setfield(L, LUA_GLOBALSINDEX, constant.name);
	}
}
