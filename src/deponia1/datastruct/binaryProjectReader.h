// Original path confirmed via x_assert() calls: src/datastruct/binaryProjectReader.cpp -
// see manifest/source_layout.tsv.
//
// The binary ("VBIN") project / savegame format. A file is
//
//     "VBIN", int unknown, int uncompressedSize, int storedSize, stored bytes
//
// where the stored bytes are the (zlib-compressed, if the two sizes differ)
// payload. The payload is read through a global TMemoryBuffer (`membuf`) and a
// global cursor (`offset`) - every load() below reads from there - and is:
//
//     the schema: a tree of bin_structure (u16 id <= 0x347, u16 child count <=
//     999, then the children). The root (id 0x64) has the children 0x35, 0x36 and
//     0x11 (project type, revision, version: no children), the "Game" object
//     (0x74) whose children are the ids of its fields, and one child per table
//     (its description) that has one child, the node of the type (the description
//     of the type group), whose children are 0x0C, 0x0D, 0x0E, 0x37 (the name, id,
//     order and last-modified stamp: each record has them at its head) and the ids
//     of the fields the file stores for that table's records (so a file written by
//     an older or newer version of the schema can still be read);
//     int projectType, int revision, int version;
//     then per table (in the schema's order): an int (skipped), the record
//     count, and per record its name (int length + bytes), id, order,
//     lastModified and then each field of the schema as loadObject() reads it.
//
// Confirmed (Deponia_Linux.asm lines 647056-657820; the save functions are
// writeStructure(), save(), saveObject() and TVisionaire::BinarySave(), which write
// into the same buffer and cursor). The original passes an unused `_IO_FILE *` to every
// function; it is kept in the signatures (always null) for fidelity.
#pragma once

#include <cstdio>
#include <vector>

#include "TMemoryBuffer.h"
#include "WxStub.h"

class TCharHolder;
class TLink;
class TSprite;
struct TTextLanguage;

/** The global read buffer and cursor of the binary reader (recovered names). */
extern TMemoryBuffer membuf;
extern unsigned long offset;
/** Set when a read ran past the end of the buffer (the original has no check
 *  and would read garbage); the loaders report failure if it is set. */
extern bool g_binaryReadOverrun;
/** offset / length of the buffer, updated while loading (recovered name). */
extern float X_LoadProgress;

/** One node of the schema tree at the head of a file. */
struct bin_structure {
	unsigned short id = 0;
	std::vector<bin_structure> children;
};

/** Reads one node (and its children) at the cursor; false if the data is
 *  invalid (id > 0x347 or more than 999 children). */
bool make_structure(bin_structure &out);

void load(int &value, std::FILE *file);
void load(long &value, std::FILE *file);
void load(bool &value, std::FILE *file);
void load(char &value, std::FILE *file);
void load(float &value, std::FILE *file);
void load(TCharHolder &value, std::FILE *file);
void load(wxPoint &value, std::FILE *file);
void load(wxRect &value, std::FILE *file);
void load(TSprite &value, std::FILE *file);
void load(TLink &value, int field, std::FILE *file);
void load(std::vector<TLink> &values, int field, std::FILE *file);
void load(std::vector<wxRect> &values, std::FILE *file);
void load(std::vector<wxPoint> &values, std::FILE *file);
void load(std::vector<TSprite> &values, std::FILE *file);
void load(std::vector<TCharHolder> &values, std::FILE *file);
void load(std::vector<float> &values, std::FILE *file);
void load(std::vector<int> &values, std::FILE *file);
void load(std::vector<TTextLanguage> &values, std::FILE *file);

/** Reads a value of the given eTypeData kind into `data` (the field's
 *  storage); `field` is the field's description id (links need it). */
void loadObject(std::FILE *file, void *data, int type, int field);

/** The writing side: everything is appended to `membuf`. */
void writeStructure(bin_structure &structure, std::FILE *file);
void savec(const wxString &value, std::FILE *file);
void save(int value, std::FILE *file);
void save(long value, std::FILE *file);
void save(char value, std::FILE *file);
void save(bool value, std::FILE *file);
void save(float value, std::FILE *file);
void save(const wxString &value, std::FILE *file);
void save(TCharHolder &value, std::FILE *file);
void save(wxFileName &value, std::FILE *file);
void save(const wxPoint &value, std::FILE *file);
void save(wxRect &value, std::FILE *file);
void save(TSprite &value, std::FILE *file);
void save(TLink &value, std::FILE *file);
void save(std::vector<TLink> &values, std::FILE *file);
void save(std::vector<wxRect> &values, std::FILE *file);
void save(std::vector<wxPoint> &values, std::FILE *file);
void save(std::vector<TSprite> &values, std::FILE *file);
void save(std::vector<TCharHolder> &values, std::FILE *file);
void save(std::vector<float> &values, std::FILE *file);
void save(std::vector<int> &values, std::FILE *file);
void save(std::vector<wxString> &values, std::FILE *file);
void save(std::vector<TTextLanguage> &values, std::FILE *file);
/** Writes a value of the given eTypeData kind from `data` (the field's storage). */
void saveObject(std::FILE *file, void *data, int type);
