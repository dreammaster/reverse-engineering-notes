// Original path confirmed via x_assert() calls: src/baselib/xmlWriter.cpp -
// see manifest/source_layout.tsv.
//
// The serialization sinks that write the game-data/savegame XML. Confirmed
// from the vtables (Deponia_Linux.asm lines 3163787-3163860, 3168828+):
//
//  - TProjectFileWriter : TVedFile is the abstract interface - 22 pure
//    virtuals after the destructors (FinishWrite() and then the tag/attribute/
//    Serialize() family below, in vtable order). Its own methods are only the
//    destructors (asm lines 522192-522228).
//  - TXMLWriter : TProjectFileWriter writes it as indented XML text into a
//    TMemoryBuffer (at +0x80). The indentation prefixes are ten strings of
//    0-9 tabs; every opened tag that has children bumps the indent, and the
//    closing tag drops it again.
//  - TXMLStringWriter : TXMLWriter, TBufferedProjectFileWriter keeps the text
//    in memory (FinishWrite() compresses/encrypts it per the TFile content
//    flags). The second base is the pure interface TMSavegame::SaveGame()
//    takes (GetBuffer() at slot +0, GetFilePath() at slot +0x10); its real
//    name isn't recovered - TBufferedProjectFileWriter is invented.
//
// All attribute/tag names are TXMLNames ids (eFieldId in vstables/
// fieldIds.h); the "...S" variants take the name as a plain string instead.
#pragma once

#include <string>
#include <vector>

#include "TCharHolder.h"
#include "TFieldValue.h"
#include "TMemoryBuffer.h"
#include "TSprite.h"
#include "WxStub.h"
#include "datastruct/vedfile.h"
#include "TTextLanguage.h"

class TProjectFileWriter : public TVedFile {
public:
	~TProjectFileWriter() override = default;

	virtual bool FinishWrite() = 0;
	virtual void StartTag(int name) = 0;
	virtual void StartTagWithoutAttributes(int name) = 0;
	virtual void AddAttribute(int name, int value) = 0;
	virtual void AddAttribute(int name, float value) = 0;
	virtual void AddAttribute(int name, bool value) = 0;
	virtual void AddAttribute(int name, const TFieldValue &value) = 0;
	virtual void AddAttributeS(int name, const TCharHolder &value) = 0;
	virtual void FinishAttributes(bool hasChildren) = 0;
	virtual void FinishTag(int name) = 0;
	virtual void Serialize(int name, const wxFileName &value) = 0;
	virtual void Serialize(int name, const wxPoint &value) = 0;
	virtual void Serialize(int name, const wxRect &value) = 0;
	virtual void Serialize(int name, const TSprite &value) = 0;
	virtual void Serialize(int name, const std::vector<wxRect> &values) = 0;
	virtual void Serialize(int name, const std::vector<TSprite> &values) = 0;
	virtual void Serialize(int name, const std::vector<TCharHolder> &values) = 0;
	virtual void Serialize(int name, const std::vector<wxPoint> &values) = 0;
	virtual void Serialize(int name, const std::vector<int> &values) = 0;
	virtual void Serialize(int name, const std::vector<float> &values) = 0;
	virtual void Serialize(int name, const std::vector<TTextLanguage> &values) = 0;
	virtual void StartTagS(const char *name) = 0;
};

/** The second base of TXMLStringWriter - see the file comment. */
class TBufferedProjectFileWriter {
public:
	virtual const TMemoryBuffer &GetBuffer() const = 0;
	virtual TMemoryBuffer &GetBufferNonConst() = 0;
	virtual const wxFileName &GetFilePath() const = 0;

protected:
	~TBufferedProjectFileWriter() = default;
};

class TXMLWriter : public TProjectFileWriter {
public:
	TXMLWriter();
	~TXMLWriter() override;

	void StartTag(int name) override;
	void StartTagWithoutAttributes(int name) override;
	void AddAttribute(int name, int value) override;
	void AddAttribute(int name, float value) override;
	void AddAttribute(int name, bool value) override;
	void AddAttribute(int name, const TFieldValue &value) override;
	void AddAttributeS(int name, const TCharHolder &value) override;
	void FinishAttributes(bool hasChildren) override;
	void FinishTag(int name) override;
	void Serialize(int name, const wxFileName &value) override;
	void Serialize(int name, const wxPoint &value) override;
	void Serialize(int name, const wxRect &value) override;
	void Serialize(int name, const TSprite &value) override;
	void Serialize(int name, const std::vector<wxRect> &values) override;
	void Serialize(int name, const std::vector<TSprite> &values) override;
	void Serialize(int name, const std::vector<TCharHolder> &values) override;
	void Serialize(int name, const std::vector<wxPoint> &values) override;
	void Serialize(int name, const std::vector<int> &values) override;
	void Serialize(int name, const std::vector<float> &values) override;
	void Serialize(int name, const std::vector<TTextLanguage> &values) override;
	void StartTagS(const char *name) override;

	void AddAttribute(int name, unsigned int value);
	void WriteXMLHeader();
	void StartTagWithoutAttributesS(const char *name);
	void AddAttributeS(const char *name, const wxString &value);
	void AddAttributeS(const char *name, const std::string &value);
	void FinishWithContent(int name, const wxString &content);
	void FinishWithContent(const wxString &name, const wxString &content);
	void AddContent(const wxString &content);
	void FinishTagS(const char *name);

protected:
	static const int kMaxIndentationLevels = 10;

	TMemoryBuffer _buffer;
	char *_indents[kMaxIndentationLevels];
	int _indent;
};

class TXMLStringWriter : public TXMLWriter, public TBufferedProjectFileWriter {
public:
	TXMLStringWriter() = default;

	bool FinishWrite() override;
	const TMemoryBuffer &GetBuffer() const override {
		return _buffer;
	}
	TMemoryBuffer &GetBufferNonConst() override {
		return _buffer;
	}
	const wxFileName &GetFilePath() const override {
		return GetPath();
	}
};
