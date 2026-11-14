/* The player's side of the game: mouse clicks turn into commands (the sentence builder of the original).
 *
 * A click on a verb of the list selects it (it turns yellow); a click on an object of the room picture then completes the sentence for a verb that takes one
 * object, or gives the first object of one that takes two ("Attach <thing> to ..."), and the next object click completes that. A completed sentence is one turn:
 * Game::executeCommand, after which the screen shows the answer.
 */
#ifndef XANTH_INTERFACE_H
#define XANTH_INTERFACE_H

#include "common/files.h"
#include "xanth/display.h"
#include "xanth/game.h"

namespace Xanth {

class Interface {
public:
	Interface(Game &game, Display &display);

	/** Starts the game on the screen: the interface, the first room. */
	void start();
	/** The pointer moved: the name of the object under it is shown. */
	void move(int x, int y);
	/** A click (left button) at a point of the 320x200 screen. */
	void click(int x, int y);

	/** Where saved games are written and read (the game folder by default); without it Save and Restore do nothing. */
	void setSaveFiles(const Common::GameFiles *files) { _files = files; }
	/** True once the player chose Quit. */
	bool quitRequested() const { return _quit; }

	int verb() const { return _verb; }
	int firstObject() const { return _dobj; }

private:
	void execute(int dobj, int iobj);
	std::string shortName(int obj);
	void diskMenu();
	void saveGame();
	void restoreGame();
	std::string slotName(int slot) const;
	const Common::GameFiles *_files;
	bool _quit;

	Game &_game;
	Display &_display;
	std::vector<int> _verbIds;
	int _selected;               // index into _verbIds, -1 = none
	int _verb, _dobj;
};

} // namespace Xanth

#endif
