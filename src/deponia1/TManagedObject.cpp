#include "TManagedObject.h"

#include <algorithm>
#include <limits>

#include "AppGlobals.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGObjectManager.h"
#include "TGText.h"
#include "TTAction.h"
#include "datastruct/visionaire.h"
#include "datastruct/vlist.h"
#include "graphicslib/picture.h"
#include "vsplayer/control/gameControl.h"
#include "vstables/fieldIds.h"

int GetAngle(float /*dx*/, float /*dy*/) {
	return 0;
}

bool CreatePolygonsFromPointList(const std::vector<wxPoint> &points, TPolygonList &outPolygons) {
	outPolygons = points;
	return true;
}

wxRect GetBoundingBox(const TPolygonList &polygons) {
	if (polygons.empty())
		return wxRect();
	int minX = std::numeric_limits<int>::max();
	int minY = std::numeric_limits<int>::max();
	int maxX = std::numeric_limits<int>::min();
	int maxY = std::numeric_limits<int>::min();
	for (const wxPoint &pt : polygons) {
		minX = std::min(minX, pt.x);
		minY = std::min(minY, pt.y);
		maxX = std::max(maxX, pt.x);
		maxY = std::max(maxY, pt.y);
	}
	wxRect result;
	result.x = minX;
	result.y = minY;
	result.width = maxX - minX;
	result.height = maxY - minY;
	return result;
}

bool IsPointInsidePolygon(const wxPoint &pt, const TPolygonList &polygon) {
	if (polygon.size() < 3)
		return false;
	bool inside = false;
	for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
		const wxPoint &a = polygon[i];
		const wxPoint &b = polygon[j];
		if ((a.y > pt.y) != (b.y > pt.y)) {
			double intersectX = a.x + static_cast<double>(pt.y - a.y) / (b.y - a.y) * (b.x - a.x);
			if (pt.x < intersectX)
				inside = !inside;
		}
	}
	return inside;
}

void TManagedObject::ClickedWithoutReach(TGCharacter *character, TMouseEventEnum event) {
	_objRef.SetLink(kCharacterDestinationObject, character->GetRef(), true);
	static_cast<TGameControl *>(g_pGameControl)->GetObjectManager()->SaveEventInfo(event);
}

void TManagedObject::SetText(TGText *text) {
	_text = text;
	if (text)
		text->SetOwner(this);
}

void TManagedObject::RemoveSprites() {
	if (_picture)
		_picture->RemoveSprite();
	if (_currentAnimation)
		_currentAnimation->RemoveSprites();
}

void TManagedObject::Prepare() {
	if (_active) {
		if (_currentAnimation) {
			if (!_currentAnimation->IsBonesAnimation())
				_currentAnimation->Prepare();
		} else if (_picture) {
			_picture->RefreshSprite(false);
		}
	}
	for (TGAnimation *anim : _animations)
		if (anim->IsSpriteIndexValid())
			anim->Prepare();
}

void TManagedObject::Draw() {
	if (!_active)
		return;
	UpdateAlpha();
	if (_currentAnimation) {
		if (_currentAnimation->IsSpriteIndexValid()) {
			if (_currentAnimation->IsBonesAnimation())
				_currentAnimation->DrawMixed(_alpha, _color, _animations);
			else
				_currentAnimation->Draw(_alpha, _color, GetDirection());
		}
	} else if (_picture) {
		_picture->Draw(_alpha, _color);
	}
	if (_text) {
		_text->CalculateCurrentText();
		if (_text)
			_text->Draw(_alpha);
	}
	for (TGAnimation *anim : _animations)
		if (anim->IsSpriteIndexValid())
			anim->Draw(_alpha, _color, -1);
}

void TManagedObject::HandlePostExecution(TGEventInfo &info, const TGActionInfo &actionInfo) {
	if (!actionInfo.flag4) {
		if (actionInfo.flag7) {
			int field264 = info.command.GetInt(kButtonCommandType);
			if ((field264 == 2 || actionInfo.flag8) && !info.action.IsEmpty() && ReceiveItem(info))
				return;
			if (info.character)
				info.character->ShowComment(info.command);
			return;
		}
		if (actionInfo.flagB && !actionInfo.flag6 && info.character)
			ClickedWithoutReach(info.character, static_cast<TMouseEventEnum>(info.mouseEvent));
	}

	TVisObjRef game = _objRef.GetVisionaire()->GetGame();
	int field0xF4 = game.GetInt(kGameCommandBehaviour);
	if (actionInfo.flag9)
		return;

	if (actionInfo.flag4) {
		if (!actionInfo.flagB)
			return;
	} else {
		if (!actionInfo.flag7)
			return;
	}

	if (!info.flag8 && !info.action.IsEmpty())
		game.ClearLink(kGameUsedItem, true);

	if (actionInfo.flag4) {
		if ((field0xF4 & ~2) != 0)
			return;
	} else {
		if (actionInfo.flagA)
			return;
		if (field0xF4 != 0)
			return;
	}

	TVisObjRef parent = info.command.GetParent();
	parent.SetLink(kInterfaceActiveCommand, parent.GetLink(kInterfaceStandardCommand), true);
	if (info.flag8)
		static_cast<TGameControl *>(g_pGameControl)->GetObjectManager()->RemoveItem(true);
}

void TManagedObject::ExecuteMatchingAction(TVList &candidates, std::vector<TypeActionExecution> &types,
                                            const TGEventInfo &info, TGActionInfo &outAction) {
	outAction.flag4 = false;
	outAction.flag5 = false;

	bool matched = false;
	for (TVisionaireObject *candidateObj : candidates.items) {
		TVisObjRef actionRef(candidateObj);
		TVisObjRef commandLink = actionRef.GetLink(kActionFixture);
		int actionType = actionRef.GetInt(kActionExecutionType);
		bool isAnyObjectCmd = commandLink.IsAnyObject();
		bool cmdEmpty = commandLink.IsEmpty();
		bool notAnyObjectIfEmpty = cmdEmpty ? !isAnyObjectCmd : false;
		TTButton button(commandLink.GetLink(kActionCommand));

		bool linkedMatch;
		if (button.IsCommand() && (button == info.command)) {
			linkedMatch = true;
		} else {
			TVList linkedItems;
			commandLink.GetList(kButtonGroup, linkedItems);
			linkedMatch = false;
			for (TVisionaireObject *item : linkedItems.items) {
				if (info.command == *item) {
					linkedMatch = true;
					break;
				}
			}
		}

		matched = false;
		if (!types.empty()) {
			bool shortcut10 = (actionType == 6) && linkedMatch;
			bool shortcut18 = (actionType == 0x11) && linkedMatch;

			for (TypeActionExecution t : types) {
				int tv = static_cast<int>(t);
				bool caseMatched = false;

				switch (tv) {
				case 3:
				case 20:
				case 25:
				case 26:
				case 27:
				case 28:
					if (actionType == tv && (commandLink == info.action || isAnyObjectCmd)) {
						if ((tv == 25 || tv == 26 || tv == 27 || tv == 28) && isAnyObjectCmd)
							outAction.flagA = true;
						caseMatched = true;
					}
					break;
				case 10:
					if (shortcut10 && notAnyObjectIfEmpty)
						caseMatched = true;
					break;
				case 11:
					if (shortcut10 && (commandLink == info.action || isAnyObjectCmd))
						caseMatched = true;
					break;
				case 18:
					if (shortcut18 && notAnyObjectIfEmpty)
						caseMatched = true;
					break;
				case 19:
					if (shortcut18 && (commandLink == info.action || isAnyObjectCmd))
						caseMatched = true;
					break;
				case 21:
				case 22:
				case 23:
				case 24:
					if (actionType == tv && linkedMatch && (commandLink == info.action || isAnyObjectCmd)) {
						if (isAnyObjectCmd)
							outAction.flagA = true;
						caseMatched = true;
					}
					break;
				default:
					if (actionType == tv)
						caseMatched = true;
					break;
				}

				if (!caseMatched)
					continue;

				outAction.matchedType = actionType;
				if (outAction.flag6 || TTAction::IsImmediateExecutionType(t)) {
					outAction.flag4 = true;
					matched = true;
				} else if (isAnyObjectCmd) {
					outAction.flag5 = true;
					matched = true;
				} else {
					matched = false;
				}

				if (info.character && !_bypassReachCheck) {
					TVisObjRef game = _objRef.GetVisionaire()->GetGame();
					bool shouldSetAngle = game.GetBool(kGameAlignCharacterOnImExecution) &&
					                      (tv == 0xD || tv == 0xF || tv == 0x10 || tv == 0x12 ||
					                       tv == 0x13 || tv == 0x14 || tv == 0x22);
					if (shouldSetAngle) {
						info.character->StopWalking(true);
						wxPoint objPos = GetPosition();
						wxPoint charPos = info.character->GetPosition();
						int angle = GetAngle(static_cast<float>(objPos.x - charPos.x),
						                     static_cast<float>(objPos.y - charPos.y));
						info.character->GetRef().SetValue(kCharacterDirection, angle, TSendEventEnum::kSendEvent);
					}
				}

				if (outAction.flag4)
					TGAction::AddRunningAction(actionRef);

				break;
			}
		}

		if (matched)
			break;
	}
}

void TManagedObject::GetActionsToTest(TGEventInfo &info, std::vector<TypeActionExecution> &outTypes,
                                      TGActionInfo &outAction) {
	bool sceneFlag124 = static_cast<TGameControl *>(g_pGameControl)->GetScene()->GetRef().GetBool(kSceneIsMenu);
	outAction.flag7 = false;
	outAction.flag8 = false;
	outAction.flagB = false;

	if (_bypassReachCheck || (info.character && IsReached(info.character->GetRef())))
		outAction.flag6 = true;

	if (info.mouseEvent == 3) {
		if (sceneFlag124) {
			outTypes.push_back(TypeActionExecution::kValue13);
		} else {
			outTypes.push_back(TypeActionExecution::kValue12);
			outAction.flagB = true;
			outTypes.push_back(TypeActionExecution::kValue13);
		}
	} else {
		switch (info.mouseEvent) {
		case 4:
			if (sceneFlag124) {
				outTypes.push_back(TypeActionExecution::kValue15);
			} else {
				outTypes.push_back(TypeActionExecution::kValue0);
				TVisObjRef game = _objRef.GetVisionaire()->GetGame();
				if (game.GetInt(kGameRightClickBehaviour))
					outAction.flagB = true;
				outTypes.push_back(TypeActionExecution::kValue16);
			}
			break;
		case 1:
			outTypes.push_back(TypeActionExecution::kValue7);
			if (!sceneFlag124) {
				outAction.flagB = true;
				if (!info.action.IsEmpty()) {
					if (info.flag8) {
						if (outAction.flag6)
							outAction.flag7 = true;
						outTypes.push_back(TypeActionExecution::kValue3);
						outAction.flag8 = true;
						outTypes.push_back(TypeActionExecution::kValue25);
						outTypes.push_back(TypeActionExecution::kValue20);
						outTypes.push_back(TypeActionExecution::kValue26);
					} else {
						if (outAction.flag6)
							outAction.flag7 = true;
						outTypes.push_back(TypeActionExecution::kValue11);
						outTypes.push_back(TypeActionExecution::kValue21);
						outTypes.push_back(TypeActionExecution::kValue19);
						outTypes.push_back(TypeActionExecution::kValue22);
					}
				} else {
					if (outAction.flag6)
						outAction.flag7 = true;
					outTypes.push_back(TypeActionExecution::kValue10);
					outTypes.push_back(TypeActionExecution::kValue18);
				}
			}
			break;
		case 2:
			if (sceneFlag124) {
				outTypes.push_back(TypeActionExecution::kValue16);
			} else {
				outTypes.push_back(TypeActionExecution::kValue0);
				TVisObjRef game = _objRef.GetVisionaire()->GetGame();
				if (game.GetInt(kGameRightClickBehaviour))
					outAction.flagB = true;
				outTypes.push_back(TypeActionExecution::kValue16);
			}
			break;
		case 5:
			outTypes.push_back(TypeActionExecution::kValue1);
			break;
		case 6:
			outTypes.push_back(TypeActionExecution::kValue2);
			break;
		case 7:
			if (sceneFlag124) {
				outTypes.push_back(TypeActionExecution::kValue34);
			} else {
				outTypes.push_back(TypeActionExecution::kValue33);
				TVisObjRef game = _objRef.GetVisionaire()->GetGame();
				if (game.GetInt(kGameMiddleClickBehaviour))
					outAction.flagB = true;
				outTypes.push_back(TypeActionExecution::kValue34);
			}
			break;
		case 8:
			outTypes.push_back(TypeActionExecution::kValue35);
			break;
		case 9:
			outTypes.push_back(TypeActionExecution::kValue36);
			break;
		default:
			break;
		}
	}

	if ((info.command.IsStandardCommand() && info.action.IsEmpty()) || sceneFlag124)
		outAction.flag7 = false;
}

void TManagedObject::ExecuteEvent(TGEventInfo &info) {
	HandlePreExecution(info);

	TGActionInfo outAction;
	std::vector<TypeActionExecution> types;
	GetActionsToTest(info, types, outAction);

	if (info.mouseEvent != 5 && info.mouseEvent != 6 && !_bypassReachCheck && info.character) {
		if (IsReached(info.character->GetRef()))
			AlignCharacter(info.character->GetRef());
	}

	TVList actionList;
	GetActionList(actionList);
	ExecuteMatchingAction(actionList, types, info, outAction);

	bool handled = outAction.flag4 || outAction.flag5;

	if (!handled && (info.mouseEvent == 3 || info.mouseEvent == 4)) {
		TGEventInfo retryInfo = info;
		retryInfo.flag8 = true;
		GetActionsToTest(retryInfo, types, outAction);
		ExecuteMatchingAction(actionList, types, retryInfo, outAction);
		handled = outAction.flag4 || outAction.flag5;
	}

	if (!handled && !info.action.IsEmpty() && _hasActionTypeFallback) {
		std::vector<TypeActionExecution> newTypes;
		for (TypeActionExecution t : types) {
			switch (static_cast<int>(t)) {
			case 3:
				newTypes.push_back(TypeActionExecution::kValue27);
				newTypes.push_back(TypeActionExecution::kValue25);
				break;
			case 20:
				newTypes.push_back(TypeActionExecution::kValue28);
				newTypes.push_back(TypeActionExecution::kValue26);
				break;
			case 11:
				newTypes.push_back(TypeActionExecution::kValue23);
				newTypes.push_back(TypeActionExecution::kValue21);
				break;
			case 19:
				newTypes.push_back(TypeActionExecution::kValue24);
				newTypes.push_back(TypeActionExecution::kValue22);
				break;
			default:
				break;
			}
		}
		if (!newTypes.empty()) {
			TVList altActionList;
			info.action.GetList(kObjectActions, altActionList);
			ExecuteMatchingAction(altActionList, newTypes, info, outAction);
		}
	}

	if (!_skipFinalPostExecution) {
		bool sceneFlag124 = static_cast<TGameControl *>(g_pGameControl)->GetScene()->GetRef().GetBool(kSceneIsMenu);
		if (!sceneFlag124)
			HandlePostExecution(info, outAction);
	}
}
