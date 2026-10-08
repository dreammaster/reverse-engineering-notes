// Confirmed (Deponia_Linux.asm lines 1439934-1442300, src/vscommon/scripting/command.cpp by the asserts, and the
// Cmd* classes in the player): the functions that the scripts call (getObject, startAction, startSound ...)
// are commands. A command is an object of a class derived from TCommand that has its own Lua function:
//
//   - `Register_CmdXxx()` registers the Lua function (a global of that name) and the syntax of the command
//     (TArgSyntax, filled by `CmdXxx_GetSyntax()`) under its name;
//   - when the script calls it, the Lua function (`lua_CmdXxx`, see RunCommand()) makes the object, has its
//     TArgParser read the arguments from the Lua stack as the syntax says, and calls Do() (which takes
//     the arguments from the parser and calls Redo(), which does the work and puts the result into
//     TCommand::Result); the result goes back to the script (ToLua() pushes it) if it is not empty;
//   - a command that can be undone (CanUndo(); the commands of the editor) is kept in the TCommandQueue.
//     None of the commands of the player can be.
#pragma once

#include <vector>

#include "WxStub.h"
#include "common/lua/lauxlib.h"
#include "vscommon/scripting/argument.h"
#include "vscommon/scripting/argumentParser.h"
#include "vscommon/scripting/argumentSyntax.h"
#include "vscommon/scripting/visLua.h"

class TVisionaireGame;

class TCommand {
public:
	/** Commands can be grouped to be undone together: the first of a group is marked grpBegin and the last
	 *  grpEnd. */
	enum GroupMarkerType { grpNone = 0, grpBegin = 1, grpEnd = 2 };

	virtual ~TCommand();

	virtual bool CanUndo() const {
		return false;
	}
	/** Reads the arguments from the parser and does the command. */
	virtual bool Do() = 0;
	/** Does the command (again). */
	virtual bool Redo() = 0;
	virtual bool Undo() {
		return false;
	}
	virtual void CleanUp() {
	}

	/** The arguments of the call. */
	TArgParser _parser;
	/** The text the command was given in (by ExecuteCommand()) and what it was for. */
	wxString _commandText;
	wxString _description;
	GroupMarkerType _groupMarker = grpNone;

	/** The result of the last command that was called. */
	static TArgument Result;
};

/** The commands that can be undone and redone (for the editor). */
class TCommandQueue {
public:
	TCommandQueue() = default;
	~TCommandQueue();

	/** Puts the command at the end, after the one that was undone last (those are dropped). */
	void AddCommand(TCommand *command);
	bool DeleteCommand(size_t position);
	/** Drops the commands that were undone. */
	void DeleteUndone();
	/** The first command of the group that the last one is in. */
	TCommand *GetFirstCmdOfGroup() const;
	void Clear();
	/** Undoes the last command done (the whole group when it ends one). */
	bool Undo();
	bool Redo();
	wxString GetAsString(size_t position) const;

	/** The number of the last command done (-1 if none). */
	int _lastDone = -1;
	std::vector<TCommand *> _commands;
};

/** The game whose commands run (InitCommands()) - a global of the original. */
extern TVisionaireGame *Game;
/** The commands done that can be undone. */
extern TCommandQueue *CommandQueue;
/** Called by the queue after an undo or redo. */
extern void (*CommandCallback)();

/** Makes the Lua state and the object access for `game` (once), and the command queue. */
void InitCommands(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir, const wxString &unused);
/** InitCommands() and then all the commands of the player (command list in playerCommands.cpp). */
void InitPlayerCommands(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir, const wxString &unused);
/** Registers the commands both the editor and the player have (getObject and getTime). */
void InitCommonCommands();
void ClosePlayerCommands();
void CloseCommands();

/** Registers the syntax of a command (the command is then known by that name); false if there is one. The
 *  syntax is the registry's from then on. */
bool RegisterCommand(const wxString &name, TArgSyntax *syntax);
/** The syntax of the command of that name, or null. */
const TArgSyntax *FindCommandSyntax(const char *name);
void UnregisterCommands();

/** Registers a command: its function in Lua and its syntax, which `getSyntax` fills in. */
void RegisterCommandFunction(const char *name, lua_CFunction function, void (*getSyntax)(TArgSyntax &));

/** The Lua function of a command. */
template<class Cmd>
int RunCommand(lua_State *state, const char *name) {
	Cmd *command = new Cmd();
	const TArgSyntax *syntax = FindCommandSyntax(name);

	if (!syntax) {
		delete command;

		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"No syntax defined for command %s", wxString(name).wc_str());

		return 0;
	}

	TCommand::Result.Clear();

	if (!command->_parser.ParseArguments(*syntax)) {
		delete command;
		return luaL_error(state, "Invalid syntax for command %s", name);
	}

	if (!command->Do()) {
		delete command;
		return luaL_error(state, "Failed to execute command %s", name);
	}

	int results = TCommand::Result.GetType() != TArgType::kNone ? 1 : 0;

	if (CommandQueue && command->CanUndo())
		CommandQueue->AddCommand(command);
	else
		delete command;

	return results;
}

/** Runs a piece of Lua that calls a command (the editor's console); its result is put into `result`, and
 *  the command it made (if it can be undone) is marked with the group marker and the description. */
bool ExecuteCommand(const wxString &commandText, const TArgument **result, TCommand::GroupMarkerType marker,
                    const wxString &description);
/** Ends the group of commands that was begun. */
void CloseOpenedCmdGroup();
