#pragma once
// Top-level game controller: exploration loop, dialog text, script hosting.
#include "fod/screen.h"
#include "fod/vm.h"
#include "fod/charsheet.h"
#include "fod/combat.h"
#include <functional>

namespace fod {

// Platform services (window, keyboard). Implemented by the front-ends in engine/platform.
class Platform {
public:
	virtual ~Platform() = default;
	virtual int waitKey() = 0;                      // KEH key codes (see vm.h); returns -1 when the window closes
	virtual void present(const Screen &s) = 0;
};

class Game : public Host {
public:
	Game(const GameData &data, const Assets &assets, Platform &platform, Disk1 disk);
	static Disk1 defaultParty(const GameData &data);    // 4 starter characters (stand-in for FOD.EXE)
	void run();

	World &world() { return _w; }

	// Host interface
	void print(const std::string &text) override;
	void message(const std::string &text) override;
	int readKey() override;
	bool yesNo(const std::string &prompt) override;
	std::string readLine() override;
	void redrawMap() override { redraw(); }
	void showMenu(const std::string &text, bool npcMode) override;
	void log(const std::string &msg) override { _log.push_back(msg); }
	void beginEncounter(int monsterIndex) override;
	bool npcMode() const override { return _npcMode; }
	int pickFromList(const std::string &title, const std::vector<std::string> &items) override;
	int chooseUseItem(int member, int &slot) override;

	bool movePartyStep(int dir, bool forced = false);                    // KEH movePartyStep; false if the game should end
	void runScriptChain(int script, int event, int arg8 = 0);

private:
	void redraw();
	void passTime();
	void startPendingFight(int monsterIndex);   // KEH startPendingFight: combat against one scripted monster
	int characterInfo(int partyNum, bool inCombat);   // KEH characterInfo: returns the member with a queued command, or -1
	void afterRosterScreen(int result);
	int rosterScreen();                               // KEH rosterScreen: returns the member with a queued command or -1
	void reorderRoster();
	void banishMember();
	void endOfTurn();
	std::function<Combat::GroupAction(Combat &, int, Combatant &)> defaultCombatCommands();
	void useUpAmmoFor(int slot, int itemSlot);
	void lootCell(int x, int y);                // KEH lootCell: pick up the items lying on a cell
	bool chooseCommandTarget(int slot);         // KEH chooseCommandTargetCell
	void useCommandOnCell(int slot, bool inCombat);
	void useWeaponOnCell(int slot, bool inCombat);
	void useItemScriptOnCell(int slot, int event, bool inCombat);
	void handleScriptResult(const ScriptResult &r);
	bool askDirection(int &dx, int &dy);        // KEH askDirectionGetCell / getDirectionKey
	bool pickMapCell(int &x, int &y);           // KEH pickMapCell              // KEH moveMonstersAndCheckEncounter: random encounters and combat
	void openDialog();
	void flushDialogPage();

	const GameData &_data;
	const Assets &_assets;
	Platform &_platform;
	World _w;
	ScriptVM _vm;
	Screen _screen;
	std::vector<std::string> _log;
	std::string _dialog;           // pending dialog text ('\r' = newline)
	bool _dialogOpen = false;
	bool _quit = false;
	int _targetX[5] = {}, _targetY[5] = {};   // per-member command target cell
	bool _rosterActive = false;
	bool _sheetActive = false;
	SheetState _sheet;
	int _sheetMember = 0;
	bool _npcMode = false;         // a monster conversation is open (KEH npcDialogMode)
};

} // namespace fod
