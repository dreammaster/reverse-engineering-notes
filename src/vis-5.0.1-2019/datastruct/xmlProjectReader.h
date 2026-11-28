// Reconstructed from Deponia_Linux.asm, src/datastruct/xmlProjectReader.cpp (the path comes from the x_assert calls):
// TXMLProjectReader reads a project, a savegame or a clipboard that is XML (the editor's own format; the game's own data
// is binary, see binaryProjectReader.cpp) into a TVisionaire. The original parses with rapidxml; baselib/xmlDom.h stands
// in for it.
//
// parseNodes() is a walk over the elements with a small state machine (UserData::state):
//   0  expecting the root element (the project, the savegame)
//   1  the "Game" element (the main object): its attributes are the fields
//   2  between the elements of the root: the tables (Loadings, Characters ...)
//   3  inside a table: the objects (Loading, Character ...)
//   4  inside an object: the fields that are elements (lists, points, rects, sprites, links)
//   5  inside a sprite: its position
// An element is opened in the state it finds (and can change it), its children are read, then it is closed in the
// state that is then (which takes the object, the table or the field to its end).
#pragma once

#include <vector>

#include "TCharHolder.h"
#include "TSprite.h"
#include "TTextLanguage.h"
#include "WxStub.h"
#include "baselib/xmlDom.h"
#include "datastruct/link.h"
#include "datastruct/visenums.h"

class EventHandler;
class TSignalSlot;
class TTable;
class TTypeGroup;
class TVedFile;
class TVisionaire;
class TVisionaireObject;

/** What parseNodes() carries from element to element (the original's class of the same name). */
class UserData {
public:
	UserData(TVisionaire *visionaire, bool saveGame, int rootTag, TLoadingTypeEnum loadingType, int *outVersion,
	         TSignalSlot *slot, EventHandler *handler, int tableCount);

	int state = 0;                          // +0x00
	TVisionaire *visionaire;                // +0x08
	bool saveGame;                          // +0x10
	int rootTag;                            // +0x14, the name of the root element (eXMLNamesIntern)
	int loadingType;                        // +0x18
	bool loadingDone = false;               // +0x1C, set when the Loadings table has been read
	int *outVersion;                        // +0x20
	TVisionaireObject *object = nullptr;    // +0x28, the object being read
	TTable *table = nullptr;                // +0x30
	TTypeGroup *typeGroup = nullptr;        // +0x38
	bool skip = false;                      // +0x40, the elements are read over without being used
	int tableCount;                         // +0x44, the number of tables of the project
	int tablesRead = -1;                    // +0x48, the number of tables met so far (minus one)
	int tableTag = -1;                      // +0x4C
	int objectTag = -1;                     // +0x50
	int fieldTag = -1;                      // +0x54
	int fieldType = -1;                     // +0x58, the eTypeData of the field
	void *fieldData = nullptr;              // +0x60
	int itemTag = -1;                       // +0x68, the element of the sprite
	bool error = false;                     // +0x6C
	const char *text = nullptr;             // +0x70, the text that was parsed (for the progress)
	size_t textLength = 0;                  // +0x78
	// The lists of the field that is read: they keep their elements from field to field (index counts the ones in use).
	std::vector<TTextLanguage> texts;       // +0x88
	std::vector<int> ints;                  // +0xA0
	std::vector<float> floats;              // +0xB8
	std::vector<wxPoint> points;            // +0xD0
	std::vector<wxRect> rects;              // +0xE8
	std::vector<TCharHolder> strings;       // +0x100
	std::vector<TSprite> sprites;           // +0x118
	std::vector<TCharHolder> paths;         // +0x130
	std::vector<TLink> links;               // +0x148
	int index = 0;                          // +0x160
	TSprite *sprite = nullptr;              // +0x168, the sprite whose position is read
	TSignalSlot *slot;                      // +0x170
	EventHandler *handler;                  // +0x178
};

class TXMLProjectReader {
public:
	explicit TXMLProjectReader(TVisionaire *visionaire) : _visionaire(visionaire) {
	}
	~TXMLProjectReader() = default;

	void Clear() {
	}

	/** Reads the elements from `node` on (the next ones as well) into the data. An error is noted in
	 *  `ud->error`. */
	static void parseNodes(XmlNode *node, UserData *ud);

	/** Reads XML text (a clipboard) as game data in one go. False if something was wrong. */
	bool ReadXMLDoc(const wxString &text);
	/** Reads a project or savegame file (decompressed and decrypted as its flags say). The loading type 0 keeps the
	 *  document for the type 1 that follows; `tableCount` is the number of tables of the project. */
	bool ReadXMLDoc(TVedFile &file, const wxString &password, int rootTag, TLoadingTypeEnum type, int *outVersion,
	                TSignalSlot *slot, EventHandler *handler, int tableCount);

private:
	TVisionaire *_visionaire;
};
