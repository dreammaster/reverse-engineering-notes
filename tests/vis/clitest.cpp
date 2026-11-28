#include <cstdio>
#include <cstring>

#include "AppFunctions.h"
#include "AppGlobals.h"
#include "Diagnostics.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static bool parse(wxCmdLineParser &p, std::vector<const char *> args) {
	std::vector<char *> argv;

	argv.push_back(const_cast<char *>("deponia"));
	for (const char *a : args)
		argv.push_back(const_cast<char *>(a));

	return ParseCommandLine(static_cast<int>(argv.size()), argv.data(), p);
}

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);

	{
		wxCmdLineParser p;
		CHECK(parse(p, {}));   // nothing to parse
	}
	{
		wxCmdLineParser p;
		CHECK(parse(p, {"-w", "-re", "-r", "800x600", "-ll=info", "--scene", "Hello", "game.vis"}));
		CHECK(p.Found(wxString(L"w")));
		CHECK(p.Found(wxString(L"window")));
		CHECK(p.Found(wxString(L"re")));
		CHECK(!p.Found(wxString(L"tc")));
		wxString v;
		CHECK(p.Found(wxString(L"r"), &v) && v.ToStdWstring() == L"800x600");
		CHECK(p.Found(wxString(L"ll"), &v) && v.ToStdWstring() == L"info");
		CHECK(p.Found(wxString(L"sc"), &v) && v.ToStdWstring() == L"Hello");
		CHECK(!p.Found(wxString(L"lf"), &v));
		CHECK(p.GetParamCount() == 1 && p.GetParam(0).ToStdWstring() == L"game.vis");
	}
	{
		wxCmdLineParser p;
		CHECK(parse(p, {"--savegame=3", "--language", "German", "x.vis"}));
		long n = 0;
		wxString v;
		CHECK(p.Found(wxString(L"savegame"), &n) && n == 3);
		CHECK(p.Found(wxString(L"l"), &v) && v.ToStdWstring() == L"German");
	}
	{
		wxCmdLineParser p;
		CHECK(!parse(p, {"-w"}));            // the input file is mandatory
	}
	{
		wxCmdLineParser p;
		CHECK(!parse(p, {"--nonsense", "x.vis"}));
	}
	{
		wxCmdLineParser p;
		CHECK(!parse(p, {"-s", "abc", "x.vis"}));   // not a number
	}
	{
		wxCmdLineParser p;
		CHECK(!parse(p, {"a.vis", "b.vis"}));       // one parameter only
	}

	// Init without a game: fails and says so
	{
		wxCmdLineParser p;
		wxSize surface = {1280, 720}, render = {1280, 720};
		char name[] = "deponia";
		char file[] = "does_not_exist.vis";
		char *argv[] = {name, file};
		wxLog::loglevel = 2;
		CHECK(ParseCommandLine(2, argv, p));
		CHECK(!Init(wxString(L"deponia"), surface, render, 2, argv, p));
		CleanUp(false);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
