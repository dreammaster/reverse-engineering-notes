#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TComposedFileManager.h"
#include "TMemoryBuffer.h"
#include "baselib/composedfile.h"
#include "baselib/file.h"
#include "baselib/memfile.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// A container is written with files of different kinds (plain, compressed, encrypted, both, in chunks) and read back
// through the manager (as a savegame container, the way a savegame is read): the bytes come back as they were put in.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// the contents: a short text, something that compresses well, something that does not
	std::vector<std::vector<unsigned char>> contents;

	contents.push_back(std::vector<unsigned char>{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'});

	std::vector<unsigned char> repetitive(200000);

	for (size_t i = 0; i < repetitive.size(); i++)
		repetitive[i] = (unsigned char)("abcdefgh"[i % 8]);
	contents.push_back(repetitive);

	std::vector<unsigned char> noisy(70000);
	unsigned int state = 12345;

	for (size_t i = 0; i < noisy.size(); i++) {
		state = state * 1103515245u + 12345u;
		noisy[i] = (unsigned char)(state >> 16);
	}
	contents.push_back(noisy);

	const int kinds[] = {0, kContentCompressed, kContentEncrypted, kContentCompressed | kContentEncrypted, kContentCompressedChunks};
	std::vector<std::vector<unsigned char>> expected;
	std::vector<wxFileName> references;

	{
		TComposedFile composed;

		composed.InitForWrite(-1, TContainerTypeEnum::kType4, true);

		std::vector<TMemoryBuffer *> buffers;

		for (size_t kind = 0; kind < sizeof(kinds) / sizeof(kinds[0]); kind++) {
			for (size_t c = 0; c < contents.size(); c++) {
				TMemoryBuffer *buffer = new TMemoryBuffer();

				buffer->AppendData(contents[c].data(), contents[c].size());
				buffers.push_back(buffer);

				wxFileName name(std::wstring(L"file") + std::to_wstring(kind) + L"_" + std::to_wstring(c) + L".dat");
				wxFileName reference;

				composed.AddData(*buffer, name, reference, kinds[kind]);
				references.push_back(reference);
				expected.push_back(contents[c]);
			}
		}

		CHECK(composed.GetNumberOfEntries() == (int)references.size());
		CHECK(composed.WriteToDisk(wxFileName(L"container.dat"), wxString(L"SECRET"), nullptr, nullptr));

		for (TMemoryBuffer *buffer : buffers)
			delete buffer;
	}

	printf("%d files written, first reference %ls\n", (int)references.size(), references[0].GetFullPath().wc_str());

	// the container is read as the container of a savegame
	CHECK(TComposedFileManager::SetSavegameFile(wxFileName(L"container.dat"), wxString(L"SECRET")));

	// (what comes back is what was stored: compressed and encrypted as the flags of the entry say; the readers of the
	// data - the pictures, the xml - take that off again. Here it is done by hand: decrypt, then uncompress.)
	for (size_t i = 0; i < references.size(); i++) {
		TMemoryFile file;
		int flags = kinds[i / contents.size()];

		if (!TComposedFileManager::GetMemoryFile(file, references[i])) {
			printf("FAIL: file %d (%ls) could not be read\n", (int)i, references[i].GetFullPath().wc_str());
			failures++;
			continue;
		}

		TMemoryBuffer data;

		data.AppendData(file.GetBuffer(), file.GetSize());

		if (flags & kContentEncrypted)
			CHECK(data.Decrypt(wxString(L"SECRET"), nullptr, 0));
		if (flags & kContentCompressed)
			CHECK(data.Uncompress((long)expected[i].size()));

		bool same = data.GetLen() == expected[i].size() && std::memcmp(data.GetData(), expected[i].data(), expected[i].size()) == 0;

		if (!same)
			printf("file %d (%ls, flags %d): size %lu, expected %lu\n", (int)i, references[i].GetFullPath().wc_str(), flags,
			       (unsigned long)data.GetLen(), (unsigned long)expected[i].size());
		CHECK(same);
	}

	// the same through a file object (the way the pictures and the data are opened): the plain ones are the bytes
	for (size_t i = 0; i < references.size(); i++) {
		if (kinds[i / contents.size()] != 0)
			continue;

		TFile file;

		if (!TComposedFileManager::Open(file, references[i])) {
			printf("FAIL: file %d could not be opened\n", (int)i);
			failures++;
			continue;
		}

		std::vector<unsigned char> bytes(expected[i].size());
		unsigned long got = file.ReadMem(bytes.data(), (unsigned long)bytes.size());

		CHECK(got == bytes.size());
		CHECK(bytes == expected[i]);
	}

	// a container with the wrong password gives nothing
	CHECK(!TComposedFileManager::SetSavegameFile(wxFileName(L"container.dat"), wxString(L"WRONG")));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
