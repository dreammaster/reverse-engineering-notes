// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 558635-558660, 1523826-1523860,
// 1524382-1524430: destructor, default and copy constructors): a 0x28-byte
// record of two TCharHolders (+0 and +0x10) and an int (+0x20, -1 by default).
// The XML writer writes them as the "Text" attribute, the "path" attribute and
// the "int" attribute respectively (TXMLWriter::Serialize for a vector of
// them); the member names are guesses from that and from the class name, not
// recovered.
#pragma once

#include "TCharHolder.h"

struct TTextLanguage {
	TCharHolder text;
	TCharHolder audioFile;
	int languageId = -1;
};
