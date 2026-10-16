#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static int counter = 0;
static int cleaned = 0;
static int callbacks = 0;

// A command that can be undone: it adds its number to a counter.
class CmdCounter : public TCommand {
public:
	bool CanUndo() const override {
		return true;
	}
	bool Do() override {
		const TArgument *argument = nullptr;

		if (_parser.GetArgument(0, &argument))
			_value = argument->GetInt();

		return Redo();
	}
	bool Redo() override {
		counter += _value;
		Result.Set(counter);
		Result.ToLua();
		return true;
	}
	bool Undo() override {
		counter -= _value;
		return true;
	}
	void CleanUp() override {
		cleaned++;
	}

	int _value = 0;
};

static int lua_counter(lua_State *state) {
	return RunCommand<CmdCounter>(state, "counter");
}

static void counterSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kInt, true);
}

static void onCommand() {
	callbacks++;
}

// The commands that can be undone (the editor's): they are kept in the queue, undone and redone one by one or as a group,
// dropped when something new is done, and told by the console text they were made from.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;

	InitCommands(&game, wxString(L"."), wxString(L"."), wxString());
	RegisterCommandFunction("counter", lua_counter, counterSyntax);
	CommandCallback = onCommand;

	CHECK(CommandQueue != nullptr);
	CHECK(CommandQueue->_lastDone == -1);

	const TArgument *result = nullptr;

	// nothing to undo or redo
	CHECK(!CommandQueue->Undo());
	CHECK(!CommandQueue->Redo());
	CHECK(CommandQueue->GetFirstCmdOfGroup() == nullptr);
	CHECK(CommandQueue->GetAsString(3).ToStdWstring() == L"NA?????");

	// a command from the console
	CHECK(ExecuteCommand(wxString(L"counter(5)"), &result, TCommand::grpNone, wxString(L"add five")));
	CHECK(counter == 5);
	CHECK(result != nullptr && result->GetInt() == 5);
	CHECK(CommandQueue->_commands.size() == 1 && CommandQueue->_lastDone == 0);
	CHECK(CommandQueue->GetAsString(0).ToStdWstring() == L"counter(5)");
	CHECK(CommandQueue->_commands[0]->_description.ToStdWstring() == L"add five");
	CHECK(callbacks == 1);

	// undone and redone
	CHECK(CommandQueue->Undo());
	CHECK(counter == 0 && CommandQueue->_lastDone == -1);
	CHECK(CommandQueue->Redo());
	CHECK(counter == 5 && CommandQueue->_lastDone == 0);

	// a new command drops what was undone
	CHECK(CommandQueue->Undo());
	CHECK(ExecuteCommand(wxString(L"counter(7)"), nullptr, TCommand::grpNone, wxString()));
	CHECK(counter == 7);
	CHECK(CommandQueue->_commands.size() == 1);
	CHECK(cleaned == 1);
	CHECK(!CommandQueue->Redo());

	// a group is undone and redone as one
	counter = 0;
	CommandQueue->Clear();
	CHECK(cleaned == 2);
	CHECK(ExecuteCommand(wxString(L"counter(1)"), nullptr, TCommand::grpBegin, wxString(L"first")));
	CHECK(ExecuteCommand(wxString(L"counter(2)"), nullptr, TCommand::grpNone, wxString(L"second")));
	CHECK(ExecuteCommand(wxString(L"counter(3)"), nullptr, TCommand::grpNone, wxString(L"third")));
	CHECK(counter == 6);
	CHECK(CommandQueue->GetFirstCmdOfGroup() == CommandQueue->_commands[0]);
	CloseOpenedCmdGroup();
	CHECK(CommandQueue->_commands[2]->_groupMarker == TCommand::grpEnd);
	CHECK(CommandQueue->GetFirstCmdOfGroup() == nullptr);
	CHECK(ExecuteCommand(wxString(L"counter(10)"), nullptr, TCommand::grpNone, wxString()));
	CHECK(counter == 16);

	CHECK(CommandQueue->Undo());
	CHECK(counter == 6);
	CHECK(CommandQueue->Undo());
	CHECK(counter == 0 && CommandQueue->_lastDone == -1);
	CHECK(CommandQueue->Redo());
	CHECK(counter == 6 && CommandQueue->_lastDone == 2);
	CHECK(CommandQueue->Redo());
	CHECK(counter == 16);
	CHECK(!CommandQueue->Redo());

	// a group of one command is no group
	CHECK(ExecuteCommand(wxString(L"counter(100)"), nullptr, TCommand::grpBegin, wxString()));
	CloseOpenedCmdGroup();
	CHECK(CommandQueue->_commands.back()->_groupMarker == TCommand::grpNone);

	// deleted: the one before the last done moves the number back
	size_t size = CommandQueue->_commands.size();
	int done = CommandQueue->_lastDone;

	CHECK(!CommandQueue->DeleteCommand(size + 5));
	CHECK(CommandQueue->DeleteCommand(0));
	CHECK(CommandQueue->_commands.size() == size - 1);
	CHECK(CommandQueue->_lastDone == done - 1);
	CHECK(CommandQueue->Undo());
	CHECK(CommandQueue->Undo());
	CHECK(CommandQueue->_lastDone < done - 1);
	CommandQueue->DeleteUndone();
	CHECK(CommandQueue->_commands.size() == (size_t)(CommandQueue->_lastDone + 1));

	// a command that is no command, one with the wrong arguments
	CHECK(!ExecuteCommand(wxString(L"counter('text')"), nullptr, TCommand::grpNone, wxString()));
	CHECK(!ExecuteCommand(wxString(L"counter()"), nullptr, TCommand::grpNone, wxString()));
	CHECK(!ExecuteCommand(wxString(L"nothing()"), nullptr, TCommand::grpNone, wxString()));
	CHECK(!ExecuteCommand(wxString(L"this is not Lua"), nullptr, TCommand::grpNone, wxString()));

	CommandQueue->Clear();
	CHECK(CommandQueue->_commands.empty() && CommandQueue->_lastDone == -1);
	CommandCallback = nullptr;

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
