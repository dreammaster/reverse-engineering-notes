#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "AppGlobals.h"
#include "TComposedFileManager.h"
#include "TMemoryBuffer.h"
#include "baselib/composedfile.h"
#include "baselib/file.h"
#include "baselib/memfile.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The containers of a game: the main one, the ones of the game, the scenes (two volumes), the characters (one), the interfaces
// (two) and the movies; the manager opens a file by the reference in its name, and says no for a volume that is not there.
static std::string textOf(const std::string &word, int index) {
	return word + " number " + std::to_string(index);
}

int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	const wxString password = L"PASSWORD";

	// (the files that are taken out of a container are put in the directory for the temporary files)
	::mkdir("tmp");

	// a container is written with a few entries: their references are returned
	auto write = [&](TContainerTypeEnum type, long volume, const wxString &path, const std::string &word,
	                 std::vector<wxFileName> &references) {
		TComposedFile composed;
		// (the container keeps the buffers it is given until it is written)
		std::deque<TMemoryBuffer> buffers;

		composed.InitForWrite(volume, type, true);

		for (int i = 0; i < 3; i++) {
			buffers.emplace_back();

			TMemoryBuffer &buffer = buffers.back();
			std::string text = textOf(word, i);

			buffer.AppendData(text.data(), text.size());

			wxFileName reference;

			composed.AddData(buffer, wxFileName(std::wstring(word.begin(), word.end()) + std::to_wstring(i) + L".txt"), reference, 0);
			references.push_back(reference);
		}
		return composed.WriteToDisk(wxFileName(path.ToStdWstring()), password, nullptr, nullptr);
	};

	std::vector<wxFileName> mainRefs, gameRefs, sceneRefs[2], characterRefs, interfaceRefs[2], movieRefs;

	CHECK(write(TContainerTypeEnum::kType0, -1, L"main", "main", mainRefs));
	CHECK(write(TContainerTypeEnum::kType6, -1, L"game", "game", gameRefs));
	CHECK(write(TContainerTypeEnum::kType1, 0, L"scenes", "scene0", sceneRefs[0]));
	CHECK(write(TContainerTypeEnum::kType1, 1, L"scenes", "scene1", sceneRefs[1]));
	CHECK(write(TContainerTypeEnum::kType2, -1, L"characters", "character", characterRefs));
	CHECK(write(TContainerTypeEnum::kType5, 0, L"interfaces", "interface0", interfaceRefs[0]));
	CHECK(write(TContainerTypeEnum::kType5, 1, L"interfaces", "interface1", interfaceRefs[1]));
	CHECK(write(TContainerTypeEnum::kType3, 0, L"movies", "movie", movieRefs));

	printf("references: %ls %ls %ls %ls\n", mainRefs[0].GetFullPath().wc_str(), sceneRefs[1][2].GetFullPath().wc_str(),
	       characterRefs[1].GetFullPath().wc_str(), movieRefs[0].GetFullPath().wc_str());

	auto content = [&](const wxFileName &reference) {
		TMemoryFile file;

		if (!TComposedFileManager::GetMemoryFile(file, reference))
			return std::string("<none>");
		return std::string(reinterpret_cast<const char *>(file.GetBuffer()), file.GetSize());
	};

	// not a game that is compiled yet: the manager reads plain files
	CHECK(!TComposedFileManager::IsGameCompiled());
	CHECK(TComposedFileManager::GetMainContainer() != nullptr);

	// the main container opens the game
	CHECK(TComposedFileManager::InitMainContainer(wxFileName(L"main.vis"), password));
	CHECK(TComposedFileManager::IsGameCompiled());
	CHECK(content(mainRefs[0]) == textOf("main", 0));
	CHECK(content(mainRefs[1]) == textOf("main", 1));

	// (a main container that is not one: the game is not compiled)
	CHECK(!TComposedFileManager::InitMainContainer(wxFileName(L"nothing.vis"), password));
	CHECK(!TComposedFileManager::IsGameCompiled());
	CHECK(TComposedFileManager::InitMainContainer(wxFileName(L"main.vis"), password));

	// the other containers: the game's, two of scenes, one of characters, two of interfaces, the movies
	TComposedFileManager::Init(wxFileName(L"main.vis"), password, 1, wxFileName(L"game.vo"), 2, wxFileName(L"scenes.vs"), 1,
	                           wxFileName(L"characters.vc"), 2, wxFileName(L"interfaces.vi"), 1, wxFileName(L"movies.vv"));

	for (int i = 0; i < 3; i++) {
		CHECK(content(gameRefs[i]) == textOf("game", i));
		CHECK(content(sceneRefs[0][i]) == textOf("scene0", i));
		CHECK(content(sceneRefs[1][i]) == textOf("scene1", i));
		CHECK(content(characterRefs[i]) == textOf("character", i));
		CHECK(content(interfaceRefs[0][i]) == textOf("interface0", i));
		CHECK(content(interfaceRefs[1][i]) == textOf("interface1", i));
	}

	// opened as a file, and exported to a file of its own
	{
		TFile file;

		CHECK(TComposedFileManager::Open(file, sceneRefs[1][2]));
	}
	{
		wxFileName exported;

		CHECK(TComposedFileManager::Export(sceneRefs[0][1], wxFileName(L"."), exported));
		printf("exported to %ls\n", exported.GetFullPath().wc_str());
		CHECK(wxFile::Exists(exported.GetFullPath()));

		wxFile taken;
		char text[64] = {0};

		CHECK(taken.Open(exported.GetFullPath(), 1));
		taken.Read(text, sizeof(text) - 1);
		CHECK(std::string(text) == textOf("scene0", 1));
		taken.Close();
		wxRemoveFile(exported.GetFullPath());
	}

	// a volume that is not there
	{
		wxFileName missing(sceneRefs[1][0]);
		TMemoryFile file;

		wxString name = missing.GetFullPath();
		TFile opened;

		name.Replace(L"#001#", L"#007#");
		CHECK(!TComposedFileManager::GetMemoryFile(file, wxFileName(name.ToStdWstring())));
		CHECK(!TComposedFileManager::Open(opened, wxFileName(name.ToStdWstring())));
	}

	// the movies: the container is written, taken apart and put together again (the header is changed back and forth)
	{
		TComposedFileInfo info;

		CHECK(TComposedFileManager::GetComposedFileInfo(movieRefs[0], info));
		CHECK(info.containerType == TContainerTypeEnum::kType3);

		wxFileName decrypted;

		bool done = TComposedFileManager::DecryptComposedFile(movieRefs[0], decrypted, password);

		printf("decrypted: %d (%ls)\n", (int)done, decrypted.GetFullPath().wc_str());
		CHECK(done);
		CHECK(TComposedFileManager::EncryptComposedFile(movieRefs[0], password));
		CHECK(!TComposedFileManager::DecryptComposedFile(sceneRefs[0][0], decrypted, password));
	}

	// the references that are no references
	{
		TComposedFileInfo info;

		CHECK(!TComposedFileManager::GetComposedFileInfo(wxFileName(L"plain.png"), info));
		CHECK(!TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#q#-01#00001#"), info));
		CHECK(!TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#s#-01#0000a#"), info));
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#s#-01#00001#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType1 && info.volume == -1 && info.entryIndex == 1);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#v#-01#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType0);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#o#-01#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType6);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#i#002#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType5 && info.volume == 2);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#c#-01#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType2);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#m#001#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType3);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#g#-01#00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kType4);

		// in the executable, and the manuals
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#001#000004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kEmbeddedInExecutable && info.volume == 1 && info.entryIndex == 4);
		CHECK(TComposedFileManager::GetComposedFileInfo(wxFileName(L"x.png#001#m00004#"), info));
		CHECK(info.containerType == TContainerTypeEnum::kEmbeddedInExecutableAlt);
	}

	// cleaned up: the scenes, the characters and the interfaces are gone
	TComposedFileManager::CleanUp();
	CHECK(content(sceneRefs[0][0]) == "<none>");
	CHECK(content(characterRefs[0]) == "<none>");
	CHECK(content(interfaceRefs[1][0]) == "<none>");
	CHECK(content(gameRefs[0]) == textOf("game", 0));

	// the manuals: containers of their own, each its own volume
	{
		std::vector<wxFileName> manualRefs;

		CHECK(write(TContainerTypeEnum::kType1, -2, L"manual", "manual", manualRefs));
		TComposedFileManager::Init(wxFileName(L"main.vis"), password, std::vector<TCharHolder>{TCharHolder("manual")});
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
