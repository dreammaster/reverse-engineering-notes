#pragma once
// The map-script interpreter (KEH.EXE doScript) with a host interface for everything UI related.
#include "fod/world.h"

namespace fod {

// Script event classes (record head low nibble / doScript arg_4).
enum EventClass { kEvEnter = 0, kEvUseItem = 3, kEvSkill = 4, kEvMonster = 6, kEvHire = 7 };

// Key codes returned by Host::readKey (KEH getKeypress): ASCII upper-case, F1-F5 = 0x3B..0x3F,
// cursor keys 0xFD up / 0xFC down / 0xFB left / 0xFA right, ESC 0xFF, Enter/Space 0xFE.
class Host {
public:
	virtual ~Host() = default;
	virtual void print(const std::string &text) = 0;          // append text to the dialog window ('\r' = newline)
	virtual void message(const std::string &text) = 0;        // show text and wait for space (KEH showMessage)
	virtual int readKey() = 0;
	virtual bool yesNo(const std::string &prompt) = 0;
	virtual std::string readLine() = 0;                       // typed word (op 15)
	virtual void sound(int /*n*/) {}
	virtual void redrawMap() {}
	virtual void showMenu(const std::string &text, bool npcMode) { print(text); (void)npcMode; }
	virtual void shop(int /*idx*/, int /*member*/) {}
	virtual void service(int /*idx*/, int /*member*/) {}
	virtual void characterInfo(int /*member*/) {}
	virtual void ending(int /*n*/) {}
	virtual void skillRaised(const std::string & /*member*/, int /*skill*/, int /*level*/) {}
	virtual void log(const std::string & /*msg*/) {}
	virtual void gameOver() {}                                  // the whole party is dead (KEH checkPartyWipeAndQuit)
	// monster conversations (KEH npcDialogMode)
	virtual void beginEncounter(int /*monsterIndex*/) {}
	virtual bool npcMode() const { return false; }
	virtual int pickFromList(const std::string & /*title*/, const std::vector<std::string> & /*items*/) { return -1; }
	virtual int chooseUseItem(int /*member*/, int & /*slot*/) { return -1; }   // returns the member index, or -1 to cancel
};

struct ScriptCall {
	int cellScript = -1;       // script index (map cell +6 / monster +0x66)
	int event = kEvEnter;
	int member = 0;            // current party slot
	int arg8 = 0;              // item slot / monster index / skill index depending on the event
	int argA = 0;
};

struct ScriptResult {
	bool ran = false;          // block existed and contained the event class
	bool changedMap = false;
	int request = -1;          // KEH _scriptRequest: 8 start combat, 6 run the monster's own script, 0 redraw, -1 none
	int monsterIndex = 0;      // monster concerned by request 6 / 8
	int chainScript = -1;      // script to run next for request 6
};

class ScriptVM {
public:
	ScriptVM(World &world, Host &host) : _w(world), _h(host) {}
	ScriptResult run(const ScriptCall &call);
	void damageParty(const std::string &message, int min, int max, bool ignoreArmor);   // KEH damageParty
	void applyRepeatingHazard();                                                        // KEH applyRepeatingHazard
private:
	struct Frame { size_t loopStart, loopEnd; int condDepth; };

	World &_w;
	Host &_h;
	int _lastKey = 0;
	std::string _typed;
};

} // namespace fod
