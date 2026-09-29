// Not yet assert-confirmed to a specific file.
//
// A serialization sink for savegame data: TVisionaire::SaveSaveGame takes a
// TProjectFileWriter&, TMSavegame::SaveGame takes a
// TBufferedProjectFileWriter const&, and TGameControl::SaveGame constructs
// one concrete TXMLStringWriter and passes it to both (Deponia_Linux.asm
// lines 463082-463130), so TXMLStringWriter must derive from both (directly
// or transitively) - modeled here as a single inheritance chain, the
// simplest hierarchy that satisfies both call shapes. Confirmed
// construction/destruction call shape only: TXMLStringWriter is default-
// constructible and destroyed via a direct (non-virtual)
// TXMLWriter::~TXMLWriter() call. Nothing about the real API (how writing
// actually happens, what "buffered" or "XML" mean here beyond the name) has
// been reversed.
#pragma once

class TProjectFileWriter {
};

class TBufferedProjectFileWriter : public TProjectFileWriter {
};

class TXMLWriter : public TBufferedProjectFileWriter {
};

class TXMLStringWriter : public TXMLWriter {
public:
	TXMLStringWriter() = default;
};
