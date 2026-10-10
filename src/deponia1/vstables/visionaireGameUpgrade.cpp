// Confirmed (Deponia_Linux.asm lines 1508677-1523438, TVisionaireGame::UpdateVersion): the fixes that bring the data of a
// project saved by an older editor up to date. Each function below is what the file does for a version up to the number
// in its name (the asm has them threaded into a chain, one entry for each version; a file of version V runs the fixes
// of every number >= V, oldest first, then the part of UpdateVersion() that every version gets). The names of the
// functions are the numbers of the versions.
//
// The numbers of the tables: 0 characters, 1 interfaces, 2 buttons, 3 fonts, 4 scenes, 6 objects, 8 action parts,
// 9 animations, 12 dialog parts, 15 cursors, 19 text languages. The 100 (kVisionaireAdventure) that is the value of many
// fields below is just 100 (percent).
#include "vstables/visionaireGameUpgrade.h"

#include "TTextLanguage.h"
#include "TTText.h"
#include "Diagnostics.h"
#include "datastruct/table.h"
#include "datastruct/visionaireobject.h"
#include "vstables/records.h"

#include "datastruct/visobjref.h"
#include "vstables/fieldIds.h"

namespace {

typedef TSendEventEnum Event;

const char *const kUpgradeSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vstables/visionaireGame.cpp";

/** Calls `fix` for every object of the table. */
template<typename F>
void forEach(TVisionaireGame &game, int table, F fix) {
	TVList list;

	game.GetList(table, list, false);

	for (TVisionaireObject *object : list) {
		TVisObjRef ref(object);

		fix(ref);
	}
}

/** All the places of `from` in `text` are `to` (the asm: replace(), asm 1504297). */
wxString replaceAll(const wxString &text, const wxString &from, const wxString &to) {
	const std::wstring source = text.ToStdWstring();
	const std::wstring search = from.ToStdWstring();
	std::wstring result;
	size_t position = 0;

	while (!search.empty()) {
		size_t found = source.find(search, position);

		if (found == std::wstring::npos)
			break;

		result += source.substr(position, found - position);
		result += to.ToStdWstring();
		position = found + search.size();
	}

	result += source.substr(position);
	return wxString(result);
}

/** The links of two fields change places. */
void swapLinks(TVisObjRef &ref, int first, int second) {
	TVisObjRef a = ref.GetLink(first);
	TVisObjRef b = ref.GetLink(second);

	ref.SetLink(first, b, false);
	ref.SetLink(second, a, false);
}

void fix63(TVisionaireGame &game) {
	forEach(game, 0, [](TVisObjRef &character) {
		character.SetValue(kCharacterVisibility, 100, Event::kNoEvent);
		character.SetValue(kCharacterDestVisibility, 100, Event::kNoEvent);
	});

	forEach(game, 6, [](TVisObjRef &object) {
		object.SetValue(kObjectVisibility, 100, Event::kNoEvent);
		object.SetValue(kObjectDestVisibility, 100, Event::kNoEvent);
	});

	forEach(game, 4, [](TVisObjRef &scene) {
		scene.SetValue(kSceneBrightness, 100, Event::kNoEvent);
	});

	// the button type moves to the command type, the action to the type
	forEach(game, 2, [](TVisObjRef &button) {
		button.SetValue(kButtonCommandType, button.GetInt(kButtonType), Event::kNoEvent);
		button.SetValue(kButtonType, button.GetInt(kButtonAction), Event::kNoEvent);
		button.SetValue(kButtonUseOnCurrentCharacter, button.GetBool(kButtonUseOnOwnCharacter), Event::kNoEvent);
	});

	forEach(game, 12, [](TVisObjRef &part) {
		part.SetValue(kDialogPartConditionNegate, !part.GetBool(kDialogPartConditionValue), Event::kNoEvent);
	});

	// the commands that were renumbered, and the commands whose links changed their order
	forEach(game, 8, [](TVisObjRef &part) {
		switch (part.GetInt(kActionPartCommand)) {
		case 0x0B:
			part.SetValue(kActionPartCommand, 0x3B, Event::kNoEvent);
			break;

		case 0x32:
		case 0x61:
		case 0x1F:
		case 0x77:
		case 0x51:
			swapLinks(part, kActionPartLink, kActionPartAltLink);
			break;

		case 0x50: {
			TVisObjRef alt = part.GetLink(kActionPartAltLink);

			part.SetLink(kActionPartLink, alt, false);
			part.ClearLink(kActionPartAltLink, false);
			break;
		}

		default:
			break;
		}
	});

	// the width of a space is that of the letter a
	forEach(game, 3, [](TVisObjRef &font) {
		wxString alphabet = font.GetStr(kFontAlphabet);
		size_t index = 0;

		while (index < alphabet.Length() && alphabet.GetChar(index) != L'a')
			index++;

		if (index == alphabet.Length())
			index = 0;

		std::vector<wxRect> letters;

		font.GetRects(kFontLetters, letters);
		font.SetValue(kFontSpaceWidth, (index < letters.size()) ? letters[index].GetWidth() : 10, Event::kNoEvent);
	});

	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameSceneComposedFiles, 0, Event::kNoEvent);
	gameRef.SetValue(kGameCharacterComposedFiles, 0, Event::kNoEvent);
	gameRef.SetValue(kGameMovieComposedFiles, 0, Event::kNoEvent);
	gameRef.SetValue(kGameKeepCharacterSpritesDuringMenus, true, Event::kNoEvent);

	forEach(game, 1, [](TVisObjRef &interface) {
		interface.SetValue(kInterfaceVisible, true, Event::kNoEvent);
	});

	gameRef.SetValue(kGameAutoHideInterfacesInMenu, true, Event::kNoEvent);
	gameRef.SetValue(kGameDisableInteractionDuringAnim, 1, Event::kSendEvent);

	// the line breaks of the about text and of the texts are written <br/>
	const wxString oldBreak(L"<br>");
	const wxString newBreak(L"<br/>");

	game.GetGame().SetValue(kGameAbout, replaceAll(game.GetGame().GetStr(kGameAbout), oldBreak, newBreak), Event::kSendEvent);

	forEach(game, 19, [&](TVisObjRef &textLanguage) {
		wxString text = textLanguage.GetStr(kTextLanguageText);

		if (text.Length() != 0) {
			std::wstring changed = replaceAll(text, oldBreak, newBreak).ToStdWstring();

			// no spaces and line ends at the end of the text (not the first character)
			size_t last = changed.size() - 1;

			if (last != 0) {
				while (last > 0 && (changed[last] == L' ' || changed[last] == L'\n'))
					last--;

				changed = changed.substr(0, last + 1);
			}

			text = wxString(changed);
		}

		textLanguage.SetValue(kTextLanguageText, text, Event::kSendEvent);
	});

	// the animations of the cursors turn about the hotspot, once
	forEach(game, 15, [](TVisObjRef &cursor) {
		wxPoint hotspot = *cursor.GetPoint(kCursorHotspot);

		for (int field : {kCursorActiveAnimation, kCursorInactiveAnimation}) {
			TVisObjRef animation = cursor.GetLink(field);

			if (!animation.IsEmpty()) {
				animation.SetValue(kAnimationCenter, hotspot, Event::kSendEvent);
				animation.SetValue(kAnimationNumberOfLoops, 0, Event::kSendEvent);
			}
		}
	});
}

void fix64(TVisionaireGame &game) {
	// the "set interface part" commands got their own numbers (0x8B: the background, 0x8C: the buttons of an interface)
	forEach(game, 8, [](TVisObjRef &part) {
		int command = part.GetInt(kActionPartCommand);

		if (command == 1 || command == 2) {
			part.SetValue(kActionPartCommand, 0x8B, Event::kNoEvent);
			part.SetValue(kActionPartInt, command == 1, Event::kNoEvent);
		} else if (command == 3 || command == 4) {
			part.SetValue(kActionPartCommand, 0x8C, Event::kNoEvent);
			part.SetValue(kActionPartInt, command == 3, Event::kNoEvent);
		}
	});

	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameDisableInteractionDuringAnim, 0, Event::kNoEvent);

	if (gameRef.GetInt(kGameLeftMouseHolding) == 3)
		gameRef.SetValue(kGameLeftMouseHolding, 4, Event::kNoEvent);
}

void fix65(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		int command = part.GetInt(kActionPartCommand);

		if (command == 0x61) {
			part.SetValue(kActionPartCommand, 0x1F, Event::kNoEvent);
			command = 0x1F;
		}

		if (command == 0x1F) {
			part.SetValue(kActionPartInt, 0, Event::kNoEvent);
		} else {
			if (command == 0x60) {
				part.SetValue(kActionPartCommand, 0x2F, Event::kNoEvent);
				command = 0x2F;
			}

			if (command == 0x2F)
				part.SetValue(kActionPartAltInt2, 0, Event::kNoEvent);
		}
	});
}

void fix66(TVisionaireGame &game) {
	// the actions of the items: the execution types 6 and 3 are 0x11 and 0x14
	forEach(game, 6, [](TVisObjRef &object) {
		if (!object.GetBool(kObjectIsItem))
			return;

		TVList actions;

		object.GetLinks(kObjectActions, TypeOrder::kValue0, actions);

		for (TVisionaireObject *action : actions) {
			TVisObjRef ref(action);

			switch (ref.GetInt(kActionExecutionType)) {
			case 6:
				ref.SetValue(kActionExecutionType, 0x11, Event::kNoEvent);
				break;

			case 3:
				ref.SetValue(kActionExecutionType, 0x14, Event::kNoEvent);
				break;

			default:
				break;
			}
		}
	});
}

void fix67(TVisionaireGame &game) {
	// the action of a dialog part is now a linked action of its own, named like the part
	forEach(game, 12, [](TVisObjRef &part) {
		TVisObjRef action = part.GetLink(kDialogPartAction);

		if (action.IsEmpty())
			return;

		action.SetName(part.GetName());
		action.SetValue(kActionExecutionType, 5, Event::kNoEvent);
		part.SetLink(kDialogPartLinkedAction, action, false);
		part.ClearLink(kDialogPartAction, false);
	});
}

void fix68(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		int command = part.GetInt(kActionPartCommand);

		switch (command) {
		case 0x77:
			part.SetValue(kActionPartCommand, 0x1F, Event::kNoEvent);
			part.SetValue(kActionPartInt, 1, Event::kNoEvent);
			break;

		case 0x78:
			part.SetValue(kActionPartCommand, 0x2F, Event::kNoEvent);
			part.SetValue(kActionPartAltInt2, 1, Event::kNoEvent);
			break;

		case 0x76:
			part.SetValue(kActionPartCommand, 0x13, Event::kNoEvent);
			part.SetValue(kActionPartAltInt, 1, Event::kNoEvent);
			break;

		case 0x68:
			part.SetValue(kActionPartCommand, 0x1E, Event::kNoEvent);
			break;

		case 0x29:
		case 0x2A:
			part.SetValue(kActionPartCommand, 0x8D, Event::kNoEvent);
			part.SetValue(kActionPartInt, command == 0x29, Event::kNoEvent);
			break;

		case 0x2B:
		case 0x2C:
			part.SetValue(kActionPartCommand, 0x8E, Event::kNoEvent);
			part.SetValue(kActionPartInt, command == 0x2B, Event::kNoEvent);
			break;

		default:
			break;
		}
	});
}

void fix69(TVisionaireGame &game) {
	// the commands that were pairs ("on" and "off") become one command with a flag; (int, altInt, altInt2) are
	// those of the part before
	forEach(game, 8, [](TVisObjRef &part) {
		const int command = part.GetInt(kActionPartCommand);
		const int a = part.GetInt(kActionPartInt);
		const int b = part.GetInt(kActionPartAltInt);
		auto set = [&](int field, int value) {
			part.SetValue(field, value, Event::kNoEvent);
		};

		if (command == 0x6C || command == 0x6D) {
			set(kActionPartCommand, 0x8F);
			set(kActionPartInt, command != 0x6C);
			set(kActionPartAltInt, a);
			set(kActionPartAltInt2, b);
		} else if (command == 0x43 || command == 0x44) {
			set(kActionPartCommand, 0x90);
			set(kActionPartInt, command != 0x43);
		} else if (command == 0x40 || command == 0x41) {
			set(kActionPartCommand, 0x91);
			set(kActionPartInt, command != 0x41);
			set(kActionPartAltInt, a);
			set(kActionPartAltInt2, b);
		} else if (command == 0x23 || command == 0x24) {
			set(kActionPartCommand, 0x92);
			set(kActionPartInt, command != 0x24);
			set(kActionPartAltInt, a);
		} else if (command >= 0x4A && command <= 0x4C) {
			set(kActionPartCommand, 0x93);
			set(kActionPartInt, (command == 0x4A) ? 0 : (command == 0x4B) ? 1 : 2);
			set(kActionPartAltInt, a);
			set(kActionPartAltInt2, b);
		} else if (command == 0x5D || command == 0x0F) {
			set(kActionPartCommand, 0x94);
			set(kActionPartInt, command != 0x0F);
		} else if (command == 0x80 || command == 0x81) {
			set(kActionPartCommand, 0x95);
			set(kActionPartInt, command != 0x80);
		} else if (command == 0x11 || command == 0x12) {
			set(kActionPartCommand, 0x96);
			set(kActionPartInt, command != 0x11);
			set(kActionPartAltInt, a);
		} else if (command == 0x17) {
			set(kActionPartInt, 0);
		} else if (command == 0x65) {
			set(kActionPartCommand, 0x17);
			set(kActionPartInt, 1);
		} else if (command == 0x47) {
			set(kActionPartAltInt2, 0);
		} else if (command == 0x66) {
			set(kActionPartCommand, 0x47);
			set(kActionPartAltInt2, 1);
		} else if (command == 0x13) {
			set(kActionPartInt, 0);
			set(kActionPartAltInt, a);
			set(kActionPartAltInt2, b);
		} else if (command == 0x14 || command == 0x15) {
			set(kActionPartCommand, 0x13);
			set(kActionPartInt, 1);
			set(kActionPartAltInt, 0);
			set(kActionPartAltInt2, 0);
		} else if (command == 0x3E || command == 0x3F) {
			set(kActionPartCommand, 0x97);
			set(kActionPartInt, command != 0x3E);
		} else if (command == 0x8D || command == 0x8E) {
			set(kActionPartInt, a == 0);
		}
	});
}

void fix6a(TVisionaireGame &game) {
	// the font of the texts of actions is that of the first interface of the first character
	TVisObjRef gameRef = game.GetGame();
	TVisObjRef first = gameRef.GetLink(kGameFirstCharacter);

	if (!first.IsEmpty()) {
		TVList interfaces;

		first.GetLinks(kCharacterInterfaces, TypeOrder::kValue1, interfaces);

		if (!interfaces.empty()) {
			TVisObjRef font(interfaces.front()->GetLink(kInterfaceActionTextFont));

			gameRef.SetLink(kGameActionTextFont, font, false);
		}
	}

	// the integers of two commands change places
	forEach(game, 8, [](TVisObjRef &part) {
		const int command = part.GetInt(kActionPartCommand);
		const int a = part.GetInt(kActionPartInt);
		const int b = part.GetInt(kActionPartAltInt);
		const int c = part.GetInt(kActionPartAltInt2);

		if (command == 0x38) {
			part.SetValue(kActionPartInt, b, Event::kNoEvent);
			part.SetValue(kActionPartAltInt, (a == 0) ? 1 : (a == 1) ? 0 : a, Event::kNoEvent);
		} else if (command == 0x70) {
			part.SetValue(kActionPartInt, c, Event::kNoEvent);
			part.SetValue(kActionPartAltInt, a, Event::kNoEvent);
			part.SetValue(kActionPartAltInt2, b, Event::kNoEvent);
		}
	});

	forEach(game, 15, [](TVisObjRef &cursor) {
		TVisObjRef active = cursor.GetLink(kCursorActiveAnimation);
		TVisObjRef inactive = cursor.GetLink(kCursorInactiveAnimation);

		active.SetValue(kAnimationUseIndividualPause, true, Event::kNoEvent);
		inactive.SetValue(kAnimationUseIndividualPause, true, Event::kNoEvent);
	});
}

void fix6b(TVisionaireGame &game) {
	// the dialog parts get names that say where they are
	forEach(game, 12, [](TVisObjRef &part) {
		part.SetName(TCharHolder(TTDialogPart(part).GenerateName()));
	});

	// a dialog under a dialog part is named like the part
	forEach(game, 11, [](TVisObjRef &dialog) {
		TVisObjRef parent = dialog.GetParent();

		if (parent.GetId()[3] == 12)
			dialog.SetName(parent.GetName());
	});
}

void fix6c(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		const int command = part.GetInt(kActionPartCommand);

		if (command == 0x3B || command == 0x18 || command == 0x4D || command == 0x48) {
			const int a = part.GetInt(kActionPartInt);
			const int b = part.GetInt(kActionPartAltInt);

			part.SetValue(kActionPartInt, b, Event::kNoEvent);
			part.SetValue(kActionPartAltInt, a, Event::kNoEvent);
		} else if (command == 0x8B || command == 0x8C) {
			part.SetValue(kActionPartInt, part.GetInt(kActionPartInt) == 0, Event::kNoEvent);
		}
	});
}

void fix6d(TVisionaireGame &game) {
	// the time of a wait (a number of milliseconds) is given in minutes (AltInt 2), seconds (1) or milliseconds (0)
	forEach(game, 8, [](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) != 0x1E)
			return;

		const int value = part.GetInt(kActionPartInt);

		if (part.GetLink(kActionPartLink).IsEmpty()) {
			if (value % 60000 == 0) {
				part.SetValue(kActionPartInt, value / 60000, Event::kNoEvent);
				part.SetValue(kActionPartAltInt, 2, Event::kNoEvent);
				return;
			}

			if (value % 1000 == 0) {
				part.SetValue(kActionPartInt, value / 1000, Event::kNoEvent);
				part.SetValue(kActionPartAltInt, 1, Event::kNoEvent);
				return;
			}
		}

		part.SetValue(kActionPartAltInt, 0, Event::kNoEvent);
	});
}

void fix6e(TVisionaireGame &game) {
	forEach(game, 17, [&](TVisObjRef &outfit) {
		TVList walkAnimations;

		outfit.GetLinks(kOutfitWalkAnimations, TypeOrder::kValue0, walkAnimations);

		// a walk animation with a standing sprite and no mirror gets a standing animation of its direction
		for (TVisionaireObject *item : walkAnimations) {
			TVisObjRef walk(item);

			if (walk.GetSprite(kAnimationStandingSprite).IsEmpty() || !walk.GetLink(kAnimationMirror).IsEmpty())
				continue;

			int direction = walk.GetInt(kAnimationDirection);
			TVList standing;
			bool exists = false;

			outfit.GetLinks(kOutfitStandingAnimations, TypeOrder::kValue0, standing);

			for (TVisionaireObject *candidate : standing) {
				if (TVisObjRef(candidate).GetInt(kAnimationDirection) == direction) {
					exists = true;
					break;
				}
			}

			if (exists)
				continue;

			TVisObjRef created = game.CreateObject(9, outfit, kOutfitStandingAnimations);
			std::vector<TSprite> sprites;

			created.SetName(walk.GetName());
			sprites.push_back(walk.GetSprite(kAnimationStandingSprite));
			created.SetValue(kAnimationSprites, sprites, Event::kNoEvent);
			created.SetValue(kAnimationCenter, *walk.GetPoint(kAnimationCenter), Event::kNoEvent);
			created.SetValue(kAnimationDirection, direction, Event::kNoEvent);
			created.SetValue(kAnimationNumberOfLoops, 0, Event::kNoEvent);
			created.GetObjectPointer()->SetOrder(direction);
		}

		// a walk animation that mirrors another one with a standing sprite gets a standing animation that mirrors the
		// standing animation of the direction of the other one
		for (TVisionaireObject *item : walkAnimations) {
			TVisObjRef walk(item);
			TVisObjRef mirror = walk.GetLink(kAnimationMirror);

			if (mirror.IsEmpty() || mirror.GetSprite(kAnimationStandingSprite).IsEmpty())
				continue;

			int direction = walk.GetInt(kAnimationDirection);
			int mirrorDirection = mirror.GetInt(kAnimationDirection);
			TVList standing;
			TVisObjRef mirrored;
			bool exists = false;

			outfit.GetLinks(kOutfitStandingAnimations, TypeOrder::kValue0, standing);

			for (TVisionaireObject *candidate : standing) {
				TVisObjRef other(candidate);

				if (other.GetInt(kAnimationDirection) == direction)
					exists = true;
				else if (mirrorDirection == other.GetInt(kAnimationDirection))
					mirrored = other;
			}

			x_assert(!mirrored.IsEmpty(), "!mirroredStandingAnim.IsEmpty()", kUpgradeSourceFile, 0x818);

			if (!exists && !mirrored.IsEmpty()) {
				TVisObjRef created = game.CreateObject(9, outfit, kOutfitStandingAnimations);

				created.SetName(walk.GetName());
				created.SetValue(kAnimationDirection, direction, Event::kNoEvent);
				created.SetLink(kAnimationMirror, mirrored, false);
				created.GetObjectPointer()->SetOrder(direction);
			}
		}

		// a walk animation with nothing in it and no mirror goes
		for (TVisionaireObject *item : walkAnimations) {
			TVisObjRef walk(item);
			std::vector<TSprite> sprites;

			walk.GetSprites(kAnimationSprites, sprites);

			if (sprites.empty() && walk.GetLink(kAnimationMirror).IsEmpty())
				walk.Remove();
		}
	});

	// an animation of an object in a scene, or of a button, that is not reset to its position and has moved its last
	// sprite, moves there with an action of the last frame
	forEach(game, 9, [&](TVisObjRef &animation) {
		TVisObjRef parent = animation.GetParent();
		bool applies;

		if (parent.GetId()[3] == 6)
			applies = (parent.GetParent().GetId()[3] == 4);
		else
			applies = (parent.GetId()[3] == 2);

		if (!applies || animation.GetBool(kAnimationResetPosition) || animation.GetInt(kAnimationNumberOfLoops) == 1)
			return;

		std::vector<TSprite> sprites;

		animation.GetSprites(kAnimationSprites, sprites);

		if (sprites.empty())
			return;

		wxPoint position = sprites.back().GetPosition();

		if (position.x == 0 && position.y == 0)
			return;

		int last = static_cast<int>(sprites.size()) - 1;
		TVList frames;
		TVisObjRef frame;

		animation.GetLinks(kAnimationPropertyFrames, TypeOrder::kValue0, frames);

		for (TVisionaireObject *item : frames) {
			if (item->GetInt(kAnimationFrameIndex) == last) {
				frame = TVisObjRef(item);
				break;
			}
		}

		if (frame.IsEmpty()) {
			frame = game.CreateObject(28, animation, kAnimationPropertyFrames);
			frame.SetValue(kAnimationFrameIndex, last, Event::kNoEvent);
		}

		TVisObjRef action = frame.GetLink(kAnimationFrameAction);

		if (action.IsEmpty())
			action = game.CreateObject(7, frame, kAnimationFrameAction);

		TVisObjRef part = game.CreateObject(8, action, kActionActionParts);

		part.SetValue(kActionPartCommand, 0x98, Event::kNoEvent);
		part.SetValue(kActionPartInt, position.x, Event::kNoEvent);
		part.SetValue(kActionPartAltInt, position.y, Event::kNoEvent);
		part.SetValue(kActionPartAltInt2, 1, Event::kNoEvent);

		while (part.ChangeOrder(TMoveOrderEnum::kUp)) {
		}
	});

	// the way the text of actions is drawn has 3 values
	TVisObjRef gameRef = game.GetGame();

	if (gameRef.GetInt(kGameDrawActionText) > 2)
		gameRef.SetValue(kGameDrawActionText, 0, Event::kNoEvent);
}

void fix6f(TVisionaireGame &game) {
	// random order and opposite direction are one setting: how the animation plays again
	forEach(game, 9, [](TVisObjRef &animation) {
		int replay = animation.GetBool(kAnimationRandomOrder) ? 2 : (animation.GetBool(kAnimationOppositeDirection) ? 1 : 0);

		animation.SetValue(kAnimationReplay, replay, Event::kNoEvent);
	});
}

void fix70(TVisionaireGame &game) {
	// only the animations of outfits and cursors keep their centre
	forEach(game, 9, [](TVisObjRef &animation) {
		int table = animation.GetParent().GetId()[3];

		if (table != 17 && table != 15)
			animation.SetValue(kAnimationCenter, wxPoint(), Event::kNoEvent);
	});
}

void fix72(TVisionaireGame &game) {
	forEach(game, 0, [](TVisObjRef &character) {
		character.SetValue(kCharacterTint, -1, Event::kNoEvent);
	});
}


void fix75(TVisionaireGame &game) {
	game.GetGame().SetValue(kGamePictureCacheSize, 40, Event::kNoEvent);
}

void fix76(TVisionaireGame &game) {
	game.GetGame().SetValue(kGameInterfaceComposedFiles, 0, Event::kNoEvent);
}

void fix78(TVisionaireGame &game) {
	game.GetGame().SetValue(kGameGameComposedFiles, 0, Event::kNoEvent);
}

void fix79(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) == 0x7B && part.GetInt(kActionPartInt) != 0)
			part.SetValue(kActionPartInt, 2, Event::kNoEvent);
	});
}

void fix7a(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) != 0x7B)
			return;

		const int value = part.GetInt(kActionPartInt);

		if (value > 0)
			part.SetValue(kActionPartInt, value + 1, Event::kNoEvent);
	});
}

/** The log of an IF_CURRENT_OBJECT action part that has settings the conversion cannot map. */
void logInvalidCurrentObject(TVisionaireGame &game, const TVisObjRef &part) {
	if (wxLog::loglevel <= 0)
		return;

	TVisObjRef action = part.GetParent();
	TVisObjRef stored = action.GetParent();

	wxLog::logexpanded(L"IF_CURRENT_OBJECT action part (id: %d) of action '%s' (id: %d) contains invalid settings. Action is stored in '%s' (id %d, table: %s). Please correct this settings.",
	                   PackVisId(part.GetId()), action.GetName().c_str().wc_str(), PackVisId(action.GetId()),
	                   stored.GetName().c_str().wc_str(), PackVisId(stored.GetId()),
	                   game.GetTableNameSingular((std::int8_t)stored.GetId()[3], true).wc_str());
}

void fix7b(TVisionaireGame &game) {
	// IF_CURRENT_OBJECT: the second integer (1-3) is folded into the first (2-4)
	forEach(game, 8, [&game](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) != 0x67)
			return;

		const int value = part.GetInt(kActionPartInt);
		const int alt = part.GetInt(kActionPartAltInt);

		if (value == 0 || value == 1) {
			if (alt >= 1 && alt <= 3) {
				part.SetValue(kActionPartInt, alt + 1, Event::kNoEvent);
			} else if (value == 0) {
				logInvalidCurrentObject(game, part);
				part.SetValue(kActionPartInt, -1, Event::kNoEvent);
			} else {
				part.SetValue(kActionPartInt, 0, Event::kNoEvent);
			}
		} else if (value == 2) {
			if (alt >= 1 && alt <= 3) {
				logInvalidCurrentObject(game, part);
				part.SetValue(kActionPartInt, -1, Event::kNoEvent);
			} else {
				part.SetValue(kActionPartInt, 1, Event::kNoEvent);
			}
		}

		part.SetValue(kActionPartAltInt, 0, Event::kNoEvent);
	});
}

void fix7e(TVisionaireGame &game) {
	// the relations of a point: only points, each once
	forEach(game, 5, [](TVisObjRef &point) {
		TVList relations;
		TVList unique;

		point.GetLinks(kPointRelations, TypeOrder::kValue0, relations);

		for (TVisionaireObject *object : relations) {
			TVisObjRef relation(object);

			if (relation.GetId()[3] != 5)
				continue;

			bool found = false;

			for (TVisionaireObject *other : unique) {
				if (relation == TVisObjRef(other)) {
					found = true;
					break;
				}
			}

			if (!found)
				unique.push_back(relation);
		}

		if (unique.size() != relations.size())
			point.SetValue(kPointRelations, unique, false);
	});
}

void fix80(TVisionaireGame &game) {
	game.BeforeSave();
}

void fix82(TVisionaireGame &game) {
	// the animations that were only made to mirror others
	const wxString temporary(L"<TempMirroredAnimation>");

	forEach(game, 9, [&temporary](TVisObjRef &animation) {
		if (animation.GetName() == temporary)
			animation.Remove();
	});
}

void fix85(TVisionaireGame &game) {
	game.GetGame().SetValue(kGameVideosEncrypted, true, Event::kNoEvent);
}

void fix86(TVisionaireGame &game) {
	// frames that point to a sprite that is not there
	forEach(game, 9, [](TVisObjRef &animation) {
		TVList frames;

		animation.GetLinks(kAnimationPropertyFrames, TypeOrder::kValue0, frames);

		if (frames.empty())
			return;

		std::vector<TSprite> sprites;

		animation.GetSprites(kAnimationSprites, sprites);

		for (TVisionaireObject *object : frames) {
			TVisObjRef frame(object);
			const int index = frame.GetInt(kAnimationFrameIndex);

			if (index < 0 || index >= (int)sprites.size())
				frame.Remove();
		}
	});
}

void fix88(TVisionaireGame &game) {
	game.GetGame().SetValue(kGameObjectTextOutput, 0, Event::kNoEvent);
}

void fix8b(TVisionaireGame &game) {
	// the 3D settings of an outfit (camera and light)
	forEach(game, 17, [](TVisObjRef &outfit) {
		outfit.SetValue(kOutfitCameraHeight, 0, Event::kSendEvent);
		outfit.SetValue(kOutfitCameraAngle, 0, Event::kSendEvent);
		outfit.SetValue(kOutfitLightPosX, 1.0f, Event::kSendEvent);
		outfit.SetValue(kOutfitLightPosY, 1.0f, Event::kSendEvent);
		outfit.SetValue(kOutfitLightPosZ, 1.0f, Event::kSendEvent);
		outfit.SetValue(kOutfitLightColor, 0x99998E, Event::kSendEvent);
	});
}

void fix8d(TVisionaireGame &game) {
	// ... and those of its model
	forEach(game, 17, [](TVisObjRef &outfit) {
		outfit.SetValue(kOutfitModelScaleFactor, 100, Event::kSendEvent);
		outfit.SetValue(kOutfitModelUseToonShading, false, Event::kSendEvent);
		outfit.SetValue(kOutfitModelToonNuances, 3, Event::kSendEvent);
		outfit.SetValue(kOutfitModelShowOutline, false, Event::kSendEvent);
		outfit.SetValue(kOutfitModelOutlineColor, 0, Event::kSendEvent);
		outfit.SetValue(kOutfitModelOutlineWidth, 20, Event::kSendEvent);
	});
}

void fix8f(TVisionaireGame &game) {
	forEach(game, 17, [](TVisObjRef &outfit) {
		outfit.SetValue(kOutfitAmbientLightColor, 0x7F7F66, Event::kNoEvent);
	});
}

void fix92(TVisionaireGame &game) {
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGamePreloadPicThreads, 3, Event::kNoEvent);
	gameRef.SetValue(kGamePreloadedPicBufferSize, 0x4000, Event::kNoEvent);
	gameRef.SetValue(kGamePicBufferSize, 0x1D4C0, Event::kNoEvent);
}

void fix94(TVisionaireGame &game) {
	forEach(game, 0, [](TVisObjRef &character) {
		character.SetValue(kCharacterActionDestPosition, wxPoint{-1, -1}, Event::kNoEvent);
	});
}

void fix97(TVisionaireGame &game) {
	// the position of a particle is whole pixels now
	forEach(game, 22, [](TVisObjRef &particle) {
		particle.SetValue(kParticlePosX, (int)particle.GetFloat(kParticleXPos), Event::kNoEvent);
		particle.SetValue(kParticlePosY, (int)particle.GetFloat(kParticleYPos), Event::kNoEvent);
	});
}

void fix99(TVisionaireGame &game) {
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGamePreallocatedTextures, gameRef.GetInt(kGamePreallocatedTextures3Bpt) + gameRef.GetInt(kGamePreallocatedTextures4Bpt),
	                 Event::kNoEvent);
}

void fix9e(TVisionaireGame &game) {
	forEach(game, 8, [](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) == 0x42 && part.GetInt(kActionPartAltInt) == 2)
			part.SetValue(kActionPartAltInt, 3, Event::kNoEvent);
	});
}

/** The shadow of the model of an outfit; the files up to 0x8E had none, later ones had a translucency. */
void fix9f(TVisionaireGame &game, int version) {
	forEach(game, 17, [version](TVisObjRef &outfit) {
		int visibility = 100;

		if (version > 0x8E) {
			const unsigned translucency = (unsigned)outfit.GetInt(kOutfitModelShadowTranslucency);

			visibility = (translucency > 100) ? -1 : 100 - (int)translucency;
		}

		outfit.SetValue(kOutfitModelShadowVisibility, visibility, Event::kNoEvent);
		outfit.SetValue(kOutfitModelShadow, 2, Event::kNoEvent);
		outfit.SetValue(kOutfitModelAmbientOcclusion, 50, Event::kNoEvent);
	});
}

void fixa2(TVisionaireGame &game) {
	forEach(game, 17, [](TVisObjRef &outfit) {
		outfit.SetValue(kOutfitRandomMinTime, 10000, Event::kSendEvent);
		outfit.SetValue(kOutfitRandomMaxTime, 30000, Event::kSendEvent);
	});
}

void fixa3(TVisionaireGame &game) {
	game.GetGame().SetValue(kSpeakerSoundPanFactor, 8, Event::kNoEvent);
}

void fixa4(TVisionaireGame &game) {
	// the execution types of the actions that started on a mouse event or a key move to a range of their own
	TVList actions;

	game.GetGame().GetList(kGameActions, actions);

	for (TVisionaireObject *object : actions) {
		TVisObjRef action(object);
		const int type = action.GetInt(kActionExecutionType);
		int result = type;

		if ((unsigned)(type - 0x11A) <= 0xB)
			result = type + 0x3FFFFF20;
		else if (type == 0x111)
			result = 0x40000052;
		else if (type == 0x112)
			result = 0x40000051;
		else if (type == 0x113)
			result = 0x4000004F;
		else if (type == 0x114)
			result = 0x40000050;
		else
			continue;

		action.SetValue(kActionExecutionType, result, Event::kSendEvent);
	}
}

void fixa6(TVisionaireGame &game) {
	// the texts keep their languages in themselves; the objects of the language table are no more
	forEach(game, 14, [](TVisObjRef &text) {
		TVList languages;
		std::vector<TTextLanguage> *entries = nullptr;

		text.GetLinks(kTextAll, TypeOrder::kValue0, languages);
		text.GetTexts(kTextTextLanguages, &entries);

		// (the original writes entry n of the vector of a text that has none yet: the same as adding)
		for (TVisionaireObject *object : languages) {
			TVisObjRef language(object);

			if (language.GetStrHolder(kTextLanguageText).size() == 0 && language.GetStrHolder(kTextLanguageSound).size() == 0)
				continue;

			TTextLanguage entry;

			entry.languageId = PackVisId(language.GetLink(kTextLanguageLanguage).GetId());
			entry.text = language.GetStrHolder(kTextLanguageText);
			entry.audioFile = language.GetStrHolder(kTextLanguageSound);
			entries->push_back(entry);
		}
	});

	TTable *table = nullptr;

	game.SetLinkRemovalSuppressed(true);
	game.GetTable(kTableId, &table);
	table->Clear();
	game.SetLinkRemovalSuppressed(false);
}

void fixa7(TVisionaireGame &game) {
	forEach(game, 17, [](TVisObjRef &outfit) {
		outfit.SetValue(kOutfitSlideWalkAnimation, true, Event::kSendEvent);
	});
}

void fixaa(TVisionaireGame &game) {
	forEach(game, 3, [](TVisObjRef &font) {
		font.SetValue(kFontTrueTypeFont, false, Event::kNoEvent);
		font.SetValue(kFontSize, 12.0f, Event::kNoEvent);
		font.SetValue(kFontBorderSize, 2.0f, Event::kNoEvent);
	});

	forEach(game, 8, [](TVisObjRef &part) {
		if (part.GetInt(kActionPartCommand) == 0x2D)
			part.SetValue(kActionPartInt, 0, Event::kNoEvent);
	});

	// of the mouse events of the game only those that run an action stay; a right click that
	// was "1" or "2" runs an action that does what the setting did
	TVisObjRef gameRef = game.GetGame();
	const unsigned leftClick = (unsigned)gameRef.GetInt(kGameLeftMouseClick) - 3;

	if (leftClick > 1)
		gameRef.ClearLink(kGameLeftClickAction, false);

	const int rightClick = gameRef.GetInt(kGameRightMouseClick);

	if ((unsigned)(rightClick - 3) > 1) {
		gameRef.ClearLink(kGameRightClickAction, false);

		if ((unsigned)(rightClick - 1) <= 1) {
			TVisObjRef action = game.CreateObject(7, gameRef, kGameRightClickAction);
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);

			part.SetValue(kActionPartCommand, 0x2D, Event::kNoEvent);
			part.SetValue(kActionPartInt, rightClick, Event::kNoEvent);
		}
	}

	if ((unsigned)gameRef.GetInt(kGameLeftMouseDblClick) - 3 > 1)
		gameRef.ClearLink(kGameLeftDblClickAction, false);

	if ((unsigned)gameRef.GetInt(kGameLeftMouseHold) - 3 > 1)
		gameRef.ClearLink(kGameLeftHoldAction, false);

	if ((unsigned)gameRef.GetInt(kGameLeftMouseHolding) - 3 > 1)
		gameRef.ClearLink(kGameLeftHoldingAction, false);

	// the objects without a picture
	forEach(game, 6, [](TVisObjRef &object) {
		TVisObjRef sprite = object.GetLink(kObjectSprite);

		if (!sprite.IsEmpty() && sprite.GetSprite(kSpriteSprite).IsEmpty())
			sprite.Remove();
	});
}

void fixac(TVisionaireGame &game) {
	TVisObjRef gameRef = game.GetGame();

	gameRef.SetValue(kGameLeftHoldBehaviour, 0, Event::kNoEvent);
	gameRef.SetValue(kGameMiddleClickBehaviour, 0, Event::kNoEvent);
	gameRef.SetValue(kGameRightClickBehaviour, 0, Event::kNoEvent);
}


void fixb6(TVisionaireGame &game) {
	game.GetGame().SetValue(kGameSmoothScrolling, true, Event::kSendEvent);
}

void fixb8(TVisionaireGame &game) {
	// the steps of a walk animation were a speed (in 1/1000 of a pixel per ms); now they are the length
	// of the step of each frame - unless the outfit slides, then nobody looks at them
	forEach(game, 17, [](TVisObjRef &outfit) {
		const bool slides = outfit.GetBool(kOutfitSlideWalkAnimation);
		TVList animations;

		outfit.GetLinks(kOutfitWalkAnimations, TypeOrder::kValue0, animations);

		for (TVisionaireObject *object : animations) {
			TVisObjRef animation(object);

			if (slides || !animation.GetLink(kAnimationMirror).IsEmpty())
				continue;

			std::vector<float> steps;
			std::vector<float> result;
			std::vector<TSprite> sprites;

			animation.GetFloats(kAnimationWalkSteps, steps);
			animation.GetSprites(kAnimationSprites, sprites);

			int pause = animation.GetInt(kAnimationPause);
			const bool individualPause = animation.GetBool(kAnimationUseIndividualPause);

			if (individualPause && !sprites.empty())
				pause = sprites[0].GetPause();

			for (size_t i = 0; i < sprites.size(); i++) {
				const float step = (i < steps.size()) ? steps[i] : 0.0f;

				result.push_back((step != 0.0f) ? step / 1000.0f * (float)pause : step);

				// (the pause of a frame is that of the frame before it - as in the original)
				if (individualPause)
					pause = sprites[i].GetPause();
			}

			animation.SetValue(kAnimationWalkSteps, result, Event::kSendEvent);
		}
	});
}

}

void applyVersionFixes(TVisionaireGame &game, int version) {
	if (version <= 0x63)
		fix63(game);
	if (version <= 0x64)
		fix64(game);
	if (version <= 0x65)
		fix65(game);
	if (version <= 0x66)
		fix66(game);
	if (version <= 0x67)
		fix67(game);
	if (version <= 0x68)
		fix68(game);
	if (version <= 0x69)
		fix69(game);
	if (version <= 0x6A)
		fix6a(game);
	if (version <= 0x6B)
		fix6b(game);
	if (version <= 0x6C)
		fix6c(game);
	if (version <= 0x6D)
		fix6d(game);
	if (version <= 0x6E)
		fix6e(game);
	if (version <= 0x6F)
		fix6f(game);
	if (version <= 0x70)
		fix70(game);
	if (version <= 0x71 && wxLog::loglevel >= 0)
		wxLog::logexpanded(L"TVisionaireGame::UpdateVersion: the upgrade step for version 113 is not reconstructed");
	if (version <= 0x72)
		fix72(game);
	if (version <= 0x75)
		fix75(game);
	if (version <= 0x76)
		fix76(game);
	if (version <= 0x78)
		fix78(game);
	if (version <= 0x79)
		fix79(game);
	if (version <= 0x7A)
		fix7a(game);
	if (version <= 0x7B)
		fix7b(game);
	if (version <= 0x7E)
		fix7e(game);
	if (version <= 0x80)
		fix80(game);
	if (version <= 0x82)
		fix82(game);
	if (version <= 0x85)
		fix85(game);
	if (version <= 0x86)
		fix86(game);
	if (version <= 0x88)
		fix88(game);
	if (version <= 0x8B)
		fix8b(game);
	if (version <= 0x8D)
		fix8d(game);
	if (version <= 0x8F)
		fix8f(game);
	if (version <= 0x92)
		fix92(game);
	if (version <= 0x94)
		fix94(game);
	if (version <= 0x97)
		fix97(game);
	if (version <= 0x99)
		fix99(game);
	if (version <= 0x9E)
		fix9e(game);
	if (version <= 0x9F)
		fix9f(game, version);
	if (version <= 0xA2)
		fixa2(game);
	if (version <= 0xA3)
		fixa3(game);
	if (version <= 0xA4)
		fixa4(game);
	if (version <= 0xA6)
		fixa6(game);
	if (version <= 0xA7)
		fixa7(game);
	if (version <= 0xAA)
		fixaa(game);
	if (version <= 0xAC)
		fixac(game);
}

void applyLateVersionFixes(TVisionaireGame &game, int version) {
	if (version <= 0xB3 && wxLog::loglevel >= 0)
		wxLog::logexpanded(L"TVisionaireGame::UpdateVersion: the container build rules of version 179 are not reconstructed");
	if (version <= 0xB6)
		fixb6(game);
	if (version <= 0xB8)
		fixb8(game);
}
