#include "vscommon/scripting/command.h"

#include <unordered_map>

#include "Diagnostics.h"
#include "common/lua/lauxlib.h"
#include "vscommon/objAccess.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/scripting/command.cpp";

TVisionaireGame *Game = nullptr;
TCommandQueue *CommandQueue = nullptr;
void (*CommandCallback)() = nullptr;
TArgument TCommand::Result;

/** The syntaxes of the commands, by name. */
static std::unordered_map<std::string, TArgSyntax *> RegisteredCommands;

TCommand::~TCommand() {
}

TCommandQueue::~TCommandQueue() {
	Clear();
}

// Confirmed (asm lines 1441881-1441953)
void TCommandQueue::AddCommand(TCommand *command) {
	DeleteUndone();
	_commands.push_back(command);
	_lastDone++;
	x_assert(_lastDone == static_cast<int>(_commands.size()) - 1, "LastDone == Queue.size()-1", kSourceFile, 0x98);
}

// Confirmed (asm lines 1440145-1440228): the command is cleaned up and deleted.
bool TCommandQueue::DeleteCommand(size_t position) {
	if (position >= _commands.size())
		return false;

	TCommand *command = _commands[position];

	command->CleanUp();
	delete command;
	_commands.erase(_commands.begin() + position);

	if (static_cast<int>(position) <= _lastDone)
		_lastDone--;

	return true;
}

// Confirmed (asm lines 1440228-1440310): from the last to the first of those after the one done last.
void TCommandQueue::DeleteUndone() {
	while (_lastDone + 1 < static_cast<int>(_commands.size()))
		DeleteCommand(_commands.size() - 1);
}

// Confirmed (asm lines 1440310-1440377): going back from the last command, the first command that has a
// group marker and is not the end of one (null if the last is a plain command).
TCommand *TCommandQueue::GetFirstCmdOfGroup() const {
	for (int i = static_cast<int>(_commands.size()) - 1; i >= 0; i--) {
		TCommand *command = _commands[i];

		if (command->_groupMarker != TCommand::grpNone)
			return command->_groupMarker == TCommand::grpBegin ? command : nullptr;
	}

	return nullptr;
}

// Confirmed (asm lines 1440476-1440534)
void TCommandQueue::Clear() {
	for (size_t i = _commands.size(); i > 0; i--) {
		TCommand *command = _commands[i - 1];

		command->CleanUp();
		delete command;
	}

	_commands.clear();
	_lastDone = -1;
}

// Confirmed (asm lines 1440534-1440661)
bool TCommandQueue::Undo() {
	if (_lastDone < 0) {
		if (wxLog::loglevel > 1)
			wxLog::logexpanded(L"No command to undo in queue.");

		return false;
	}

	bool done;
	TCommand *command = _commands[_lastDone];

	if (command->_groupMarker == TCommand::grpEnd) {
		// a group is undone from its end to its beginning
		done = true;

		for (;;) {
			command = _commands[_lastDone];

			if (!command->Undo())
				done = false;

			_lastDone--;

			if (command->_groupMarker == TCommand::grpBegin)
				break;
		}
	} else {
		_lastDone--;
		done = command->Undo();
	}

	if (CommandCallback)
		CommandCallback();

	return done;
}

// Confirmed (asm lines 1440661-1440789)
bool TCommandQueue::Redo() {
	int next = _lastDone + 1;

	if (next >= static_cast<int>(_commands.size())) {
		if (wxLog::loglevel > 1)
			wxLog::logexpanded(L"No command to redo in queue.");

		return false;
	}

	bool done;
	TCommand *command = _commands[next];

	if (command->_groupMarker == TCommand::grpBegin) {
		// a group is redone from its beginning to its end
		done = true;

		for (;;) {
			command = _commands[_lastDone + 1];

			if (!command->Redo())
				done = false;

			_lastDone++;

			if (command->_groupMarker == TCommand::grpEnd)
				break;
		}
	} else {
		_lastDone = next;
		done = command->Redo();
	}

	if (CommandCallback)
		CommandCallback();

	return done;
}

// Confirmed (asm lines 1440789-1440889): what the command was for.
wxString TCommandQueue::GetAsString(size_t position) const {
	if (position >= _commands.size()) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Command-queue index out of bounds");

		return wxString(L"NA?????");
	}

	return _commands[position]->_commandText;
}

// Confirmed (asm lines 1441506-1441751)
bool ExecuteCommand(const wxString &commandText, const TArgument **result, TCommand::GroupMarkerType marker,
                    const wxString &description) {
	int before = CommandQueue->_lastDone;
	bool done = false;

	if (luaL_loadstring(L, commandText.mb_str()) == 0 && lua_pcall(L, 0, LUA_MULTRET, 0) == 0) {
		if (result)
			*result = &TCommand::Result;

		// a command that went into the queue gets the text and the group marker
		if (CommandQueue->_lastDone + 1 > before + 1) {
			TCommand *command = CommandQueue->_commands.at(CommandQueue->_commands.size() - 1);

			command->_groupMarker = marker;
			command->_commandText = commandText;
			command->_description = description;
		}

		done = true;
	} else {
		const char *message = lua_tolstring(L, -1, nullptr);
		wxString text;

		toUTF(&text, message ? message : "");

		if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"%s", text.wc_str());
			wxLog::logexpanded(L"Command string: %s", commandText.wc_str());
		}

		lua_settop(L, -2);
	}

	if (CommandCallback)
		CommandCallback();

	return done;
}

// Confirmed (asm lines 1441751-1441881): the last command of the queue ends the group that began at the
// nearest command before it that is marked as a beginning (a begin alone is no group).
void CloseOpenedCmdGroup() {
	int last = CommandQueue->_lastDone;

	if (last < 0) {
		x_assert(false, "false", kSourceFile, 0x170);
		return;
	}

	TCommand *end = CommandQueue->_commands.at(last);

	if (end->_groupMarker == TCommand::grpBegin) {
		end->_groupMarker = TCommand::grpNone;
		return;
	}

	int i = last - 1;

	for (; i >= 0; i--) {
		TCommand *command = CommandQueue->_commands.at(i);

		if (command->_groupMarker == TCommand::grpBegin) {
			end->_groupMarker = TCommand::grpEnd;
			break;
		}

		x_assert(command->_groupMarker != TCommand::grpEnd, "cmd->GroupMarker != TCommand::grpEnd", kSourceFile, 0x16A);
	}

	x_assert(i >= 0, "i >= 0", kSourceFile, 0x16C);
}

// Confirmed (asm lines 1440889-1441247)
bool RegisterCommand(const wxString &name, TArgSyntax *syntax) {
	if (!syntax)
		return false;

	std::string key(name.mb_str());

	if (RegisteredCommands.count(key) != 0) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Command \"%s\" already defined.", name.wc_str());

		return false;
	}

	RegisteredCommands[key] = syntax;
	return true;
}

const TArgSyntax *FindCommandSyntax(const char *name) {
	auto found = RegisteredCommands.find(name);

	return found == RegisteredCommands.end() ? nullptr : found->second;
}

// Confirmed (asm lines 1441247-1441423)
void UnregisterCommands() {
	for (auto &command : RegisteredCommands)
		delete command.second;

	RegisteredCommands.clear();
}

void RegisterCommandFunction(const char *name, lua_CFunction function, void (*getSyntax)(TArgSyntax &)) {
	lua_pushcclosure(L, function, 0);
	lua_setfield(L, LUA_GLOBALSINDEX, name);

	TArgSyntax *syntax = new TArgSyntax();

	getSyntax(*syntax);

	if (!RegisterCommand(wxString(name), syntax))
		delete syntax;
}

// Confirmed (asm lines 1441423-1441506)
void CloseCommands() {
	TCommand::Result.Clear();
	delete CommandQueue;
	CommandQueue = nullptr;
	UnregisterCommands();
	CloseLua();
}

// Confirmed (asm lines 407231-407235)
void ClosePlayerCommands() {
	CloseCommands();
}

// Confirmed (asm lines 1440087-1440145)
void InitCommands(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir, const wxString &unused) {
	x_assert(Game == nullptr, "Game == NULL", kSourceFile, 0x2F);
	Game = game;
	InitLua(game, appDir, resourcesDir, unused);
	InitObjectAccess(game);
	CommandQueue = new TCommandQueue();
}
