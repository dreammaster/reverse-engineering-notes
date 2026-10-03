#include "baselib/xmlWriter.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "Diagnostics.h"
#include "TXMLNames.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/baselib/xmlWriter.cpp";
static const char *const kIndentAssert = "Indent >= 0 && Indent < MAX_INDENTATION_LEVELS";

// Confirmed (asm lines 521440-521525)
TXMLWriter::TXMLWriter() : _indent(0) {
	for (int i = 0; i < kMaxIndentationLevels; i++) {
		_indents[i] = new char[i + 1];
		for (int j = 0; j < i; j++)
			_indents[i][j] = '\t';
		_indents[i][i] = '\0';
	}

	_buffer.Init(0x400000);
	_buffer << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n";
}

// Confirmed (asm lines 520859-520954)
TXMLWriter::~TXMLWriter() {
	for (int i = 0; i < kMaxIndentationLevels; i++)
		delete[] _indents[i];
}

// Confirmed (asm lines 520552-520584)
void TXMLWriter::StartTag(int name) {
	StartTagS(TXMLNames::GetStringUtf8(name));
}

// Confirmed (asm lines 519891-519915)
void TXMLWriter::StartTagWithoutAttributes(int name) {
	StartTag(name);
	FinishAttributes(true);
}

// Confirmed (asm lines 521543-521566)
void TXMLWriter::StartTagWithoutAttributesS(const char *name) {
	StartTagS(name);
	FinishAttributes(true);
}

// Confirmed (asm lines 520336-520374)
void TXMLWriter::StartTagS(const char *name) {
	x_assert((unsigned)_indent < kMaxIndentationLevels, kIndentAssert, kSourceFile, 0x43);
	_buffer << _indents[_indent] << '<' << name;
}

// Confirmed (asm lines 520585-520720): ` name="value"`
void TXMLWriter::AddAttribute(int name, int value) {
	_buffer << ' ' << TXMLNames::GetStringUtf8(name) << '=' << '"' << std::to_string(value).c_str() << '"';
}

// Confirmed (asm lines 521567-521683): the unsigned twin of the above.
void TXMLWriter::AddAttribute(int name, unsigned int value) {
	_buffer << ' ' << TXMLNames::GetStringUtf8(name) << '=' << '"' << std::to_string(value).c_str() << '"';
}

// Confirmed (asm lines 520955-521044): printed with "%f" in whatever the C
// locale currently is.
void TXMLWriter::AddAttribute(int name, float value) {
	char text[64];
	snprintf(text, sizeof(text), "%f", (double)value);
	_buffer << " " << TXMLNames::GetStringUtf8(name) << "=" << '"' << text << '"';
}

// Confirmed (asm lines 520499-520551): "T" or "F".
void TXMLWriter::AddAttribute(int name, bool value) {
	_buffer << " " << TXMLNames::GetStringUtf8(name) << "=\"" << (value ? 'T' : 'F') << '"';
}

// Confirmed (asm lines 521249-521334)
void TXMLWriter::AddAttribute(int name, const TFieldValue &value) {
	_buffer << ' ' << TXMLNames::GetStringUtf8(name) << '=' << '"';
	_buffer << value.ToString();
	_buffer << '"';
}

// Confirmed (asm lines 520375-520498): the value's ", &, ', < and > are
// replaced by their entities.
void TXMLWriter::AddAttributeS(int name, const TCharHolder &value) {
	_buffer << " " << TXMLNames::GetStringUtf8(name) << "=\"";

	const char *text = value.mb_str();
	if (text) {
		for (; *text; text++) {
			switch (*text) {
			case '"':
				_buffer << "&quot;";
				break;
			case '&':
				_buffer << "&amp;";
				break;
			case '\'':
				_buffer << "&apos;";
				break;
			case '<':
				_buffer << "&lt;";
				break;
			case '>':
				_buffer << "&gt;";
				break;
			default:
				_buffer << *text;
				break;
			}
		}
	}
	_buffer << '"';
}

// Confirmed (asm lines 521813-521860): no escaping.
void TXMLWriter::AddAttributeS(const char *name, const wxString &value) {
	_buffer << " " << name << "=" << '"' << value << '"';
}

// Confirmed (asm lines 521861-521908): no escaping.
void TXMLWriter::AddAttributeS(const char *name, const std::string &value) {
	_buffer << " " << name << "=" << '"' << value.c_str() << '"';
}

// Confirmed (asm lines 520721-520752)
void TXMLWriter::FinishAttributes(bool hasChildren) {
	if (!hasChildren) {
		_buffer << "/>\r\n";
		return;
	}

	_buffer << ">\r\n";
	_indent++;
}

// Confirmed (asm lines 521143-521248)
void TXMLWriter::FinishTag(int name) {
	std::string text((const char *)TXMLNames::GetString(name).mb_str());
	FinishTagS(text.c_str());
}

// Confirmed (asm lines 522049-522101): drops the indentation level, then
// writes the closing tag. (FinishTag() above is the same with the name looked
// up first; both carry the same assert, at line 247.)
void TXMLWriter::FinishTagS(const char *name) {
	_indent--;
	x_assert((unsigned)_indent < kMaxIndentationLevels, kIndentAssert, kSourceFile, 0xF7);
	_buffer << _indents[_indent] << "</" << name << ">\r\n";
}

// Confirmed (asm lines 521909-521956): never called. The disassembly writes
// into `this` where the other methods use the buffer at +0x80 (so, run as is,
// it would scribble over the object); the buffer is what is meant.
void TXMLWriter::FinishWithContent(int name, const wxString &content) {
	_buffer << '>' << content << "</" << TXMLNames::GetString(name) << ">\r\n";
}

// Confirmed (asm lines 521957-522001): never called; same remark as above.
void TXMLWriter::FinishWithContent(const wxString &name, const wxString &content) {
	_buffer << '>' << content << "</" << name << ">\r\n";
}

// Confirmed (asm lines 522002-522048)
void TXMLWriter::AddContent(const wxString &content) {
	x_assert((unsigned)_indent < kMaxIndentationLevels, kIndentAssert, kSourceFile, 0xE9);
	_buffer << _indents[_indent] << content << "\r\n";
}

// Confirmed (asm lines 521526-521542)
void TXMLWriter::WriteXMLHeader() {
	_buffer << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n";
}

// Confirmed (asm lines 522102-522191): writes the path as an attribute only
// (no tag of its own).
void TXMLWriter::Serialize(int name, const wxFileName &value) {
	AddAttribute(name, TFieldValue(value));
}

// Confirmed (asm lines 519916-519955)
void TXMLWriter::Serialize(int name, const wxPoint &value) {
	StartTag(name);
	AddAttribute(kX, value.x);
	AddAttribute(kY, value.y);
	FinishAttributes(false);
}

// Confirmed (asm lines 520274-520335): top, left, bottom, right.
void TXMLWriter::Serialize(int name, const wxRect &value) {
	StartTag(name);
	AddAttribute(kT, value.GetTop());
	AddAttribute(kLeft, value.GetLeft());
	AddAttribute(kBottom, value.GetBottom());
	AddAttribute(kRight, value.GetRight());
	FinishAttributes(false);
}

// Confirmed (asm lines 521684-521812)
void TXMLWriter::Serialize(int name, const TSprite &value) {
	StartTag(name);

	TSprite nameCopy(value);
	AddAttributeS(kName, nameCopy.GetNameNonConst());
	AddAttribute(kPause, value.GetPause());

	TSprite pathCopy(value);
	AddAttributeS(kPath, pathCopy.GetPathNonConst());
	AddAttribute(kTransparency, (int)value.GetTransparency());
	if ((int)value.GetTransparency() == 1)
		AddAttribute(kTranspColor, (unsigned int)value.GetTransparentColor());

	FinishAttributes(true);
	Serialize(kPosition, value.GetPosition());
	FinishTag(name);
}

// Confirmed (asm lines 519956-520015)
void TXMLWriter::Serialize(int name, const std::vector<wxRect> &values) {
	StartTagWithoutAttributes(name);
	for (const wxRect &rect : values)
		Serialize(kRect, rect);
	FinishTag(name);
}

// Confirmed (asm lines 520016-520083)
void TXMLWriter::Serialize(int name, const std::vector<TSprite> &values) {
	StartTagWithoutAttributes(name);
	for (const TSprite &sprite : values)
		Serialize(kSprite, sprite);
	FinishTag(name);
}

// Confirmed (asm lines 521335-521439): each string is written as a `path`
// element.
void TXMLWriter::Serialize(int name, const std::vector<TCharHolder> &values) {
	StartTagWithoutAttributes(name);
	for (const TCharHolder &value : values) {
		StartTag(kPath);
		Serialize(kPath, (wxFileName)value);
		FinishAttributes(false);
	}
	FinishTag(name);
}

// Confirmed (asm lines 520084-520141)
void TXMLWriter::Serialize(int name, const std::vector<wxPoint> &values) {
	StartTagWithoutAttributes(name);
	for (const wxPoint &point : values)
		Serialize(kPoint, point);
	FinishTag(name);
}

// Confirmed (asm lines 520142-520207)
void TXMLWriter::Serialize(int name, const std::vector<int> &values) {
	StartTagWithoutAttributes(name);
	for (int value : values) {
		StartTag(kInt);
		AddAttribute(kInt, value);
		FinishAttributes(false);
	}
	FinishTag(name);
}

// Confirmed (asm lines 520208-520273)
void TXMLWriter::Serialize(int name, const std::vector<float> &values) {
	StartTagWithoutAttributes(name);
	for (float value : values) {
		StartTag(kFloat);
		AddAttribute(kFloat, value);
		FinishAttributes(false);
	}
	FinishTag(name);
}

// Confirmed (asm lines 520753-520858): an empty list writes nothing at all,
// not even the enclosing tag.
void TXMLWriter::Serialize(int name, const std::vector<TTextLanguage> &values) {
	if (values.empty())
		return;

	StartTagWithoutAttributes(name);
	for (const TTextLanguage &text : values) {
		StartTag(kText);
		AddAttributeS(kText_0x3A, text.text);
		AddAttributeS(kPath, text.audioFile);
		AddAttribute(kInt, text.languageId);
		FinishAttributes(false);
	}
	FinishTag(name);
}

// Confirmed (asm lines 539950-539993): compresses the text if the content
// flags ask for it, then encrypts it with the file's password.
bool TXMLStringWriter::FinishWrite() {
	if (IsContentCompressed() && !_buffer.Compress())
		return false;
	if (!IsContentEncrypted())
		return true;
	return _buffer.Encrypt(_composedName, nullptr, 0);
}
