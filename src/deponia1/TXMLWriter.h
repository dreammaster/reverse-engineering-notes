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

#include "TMemoryBuffer.h"
#include "WxStub.h"

class TProjectFileWriter {
};

// The recovered vtable has three pure-virtual slots (+0x00/+0x08/+0x10);
// TMSavegame::SaveGame() (Deponia_Linux.asm lines 163411-164367) calls
// slot +0x00 to get the writer's accumulated data (passed straight to
// TComposedFile::AddData() as a TMemoryBuffer const&) and slot +0x10 to get
// the name that data is stored under. Their real names/semantics (and the
// +0x08 slot) are not reversed; modeled here as two virtuals with inert
// defaults, named for their observed role.
class TBufferedProjectFileWriter : public TProjectFileWriter {
public:
	virtual ~TBufferedProjectFileWriter() = default;

	virtual const TMemoryBuffer &GetBuffer() const {
		static const TMemoryBuffer empty;
		return empty;
	}
	virtual const wxString &GetBufferName() const {
		static const wxString empty;
		return empty;
	}
};

class TXMLWriter : public TBufferedProjectFileWriter {
};

class TXMLStringWriter : public TXMLWriter {
public:
	TXMLStringWriter() = default;
};
