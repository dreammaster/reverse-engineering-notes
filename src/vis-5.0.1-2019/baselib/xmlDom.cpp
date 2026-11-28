#include "baselib/xmlDom.h"

#include <cstdint>
#include <cstring>

class XmlParser {
public:
	XmlParser(XmlDocument &document, const char *text) : _document(document), _begin(text), _text(text) {
	}

	void Parse();

private:
	XmlDocument &_document;
	const char *_begin;
	const char *_text;

	[[noreturn]] void fail(const char *what) const {
		throw XmlParseError{what, (size_t)(_text - _begin)};
	}
	static bool isWhitespace(char c) {
		return c == ' ' || c == '\n' || c == '\r' || c == '\t';
	}
	static bool isNameEnd(char c) {
		return c == 0 || isWhitespace(c) || c == '>' || c == '/';
	}
	static bool isAttributeNameEnd(char c) {
		return c == 0 || isWhitespace(c) || c == '/' || c == '<' || c == '>' || c == '=';
	}
	void skipWhitespace() {
		while (isWhitespace(*_text))
			_text++;
	}
	XmlNode *newNode();
	XmlNode *parseNode(XmlNode *parent);
	void parseContents(XmlNode *node);
	void parseAttributes(XmlNode *node);
	std::string parseValue(char quote);
	static void appendUtf8(std::string &out, unsigned long code);
	void skipUntil(const char *end);
};

XmlNode *XmlParser::newNode() {
	_document._nodes.push_back(std::unique_ptr<XmlNode>(new XmlNode()));
	return _document._nodes.back().get();
}

void XmlParser::appendUtf8(std::string &out, unsigned long code) {
	if (code < 0x80) {
		out += (char)code;
	} else if (code < 0x800) {
		out += (char)(0xC0 | (code >> 6));
		out += (char)(0x80 | (code & 0x3F));
	} else if (code < 0x10000) {
		out += (char)(0xE0 | (code >> 12));
		out += (char)(0x80 | ((code >> 6) & 0x3F));
		out += (char)(0x80 | (code & 0x3F));
	} else {
		out += (char)(0xF0 | (code >> 18));
		out += (char)(0x80 | ((code >> 12) & 0x3F));
		out += (char)(0x80 | ((code >> 6) & 0x3F));
		out += (char)(0x80 | (code & 0x3F));
	}
}

void XmlParser::skipUntil(const char *end) {
	size_t length = strlen(end);

	while (*_text && strncmp(_text, end, length) != 0)
		_text++;
	if (!*_text)
		fail("unexpected end of data");
	_text += length;
}

// The value of an attribute up to the closing quote, with the character references replaced.
std::string XmlParser::parseValue(char quote) {
	std::string value;

	while (*_text && *_text != quote) {
		if (*_text != '&') {
			value += *_text++;
			continue;
		}

		const char *at = _text + 1;

		if (strncmp(at, "amp;", 4) == 0) {
			value += '&';
			_text += 5;
		} else if (strncmp(at, "apos;", 5) == 0) {
			value += '\'';
			_text += 6;
		} else if (strncmp(at, "quot;", 5) == 0) {
			value += '"';
			_text += 6;
		} else if (strncmp(at, "lt;", 3) == 0) {
			value += '<';
			_text += 4;
		} else if (strncmp(at, "gt;", 3) == 0) {
			value += '>';
			_text += 4;
		} else if (*at == '#') {
			unsigned long code = 0;
			bool hex = at[1] == 'x';
			const char *digits = at + (hex ? 2 : 1);
			const char *p = digits;

			for (;; p++) {
				int digit;

				if (*p >= '0' && *p <= '9')
					digit = *p - '0';
				else if (hex && *p >= 'a' && *p <= 'f')
					digit = *p - 'a' + 10;
				else if (hex && *p >= 'A' && *p <= 'F')
					digit = *p - 'A' + 10;
				else
					break;
				code = code * (hex ? 16 : 10) + digit;
			}
			if (p == digits || *p != ';')
				fail("invalid numeric character entity");
			appendUtf8(value, code);
			_text = p + 1;
		} else {
			value += *_text++;
		}
	}
	return value;
}

void XmlParser::parseAttributes(XmlNode *node) {
	XmlAttribute *last = nullptr;

	skipWhitespace();
	while (!isAttributeNameEnd(*_text)) {
		const char *start = _text;

		while (!isAttributeNameEnd(*_text))
			_text++;
		node->attributes.push_back(std::unique_ptr<XmlAttribute>(new XmlAttribute()));

		XmlAttribute *attribute = node->attributes.back().get();

		attribute->name.assign(start, _text - start);
		if (last)
			last->next = attribute;
		else
			node->firstAttribute = attribute;
		last = attribute;

		skipWhitespace();
		if (*_text != '=')
			fail("expected =");
		_text++;
		skipWhitespace();

		char quote = *_text;

		if (quote != '\'' && quote != '"')
			fail("expected ' or \"");
		_text++;
		attribute->value = parseValue(quote);
		if (*_text != quote)
			fail("expected ' or \"");
		_text++;
		skipWhitespace();
	}
}

// What follows "<": an element, or something that has no node (declaration, comment, doctype), or a CDATA section.
XmlNode *XmlParser::parseNode(XmlNode *parent) {
	if (*_text == '?') {
		skipUntil("?>");
		return nullptr;
	}

	if (*_text == '!') {
		if (strncmp(_text, "!--", 3) == 0) {
			skipUntil("-->");
			return nullptr;
		}
		if (strncmp(_text, "![CDATA[", 8) == 0) {
			_text += 8;

			const char *start = _text;

			skipUntil("]]>");

			XmlNode *data = newNode();

			data->isData = true;
			data->namePos = start - _begin;
			data->value.assign(start, _text - 3 - start);
			return data;
		}

		// a doctype: up to the matching ">" (it can have a bracketed part with ">" in it)
		int depth = 1;

		_text++;
		while (depth > 0) {
			if (!*_text)
				fail("unexpected end of data");
			if (*_text == '[')
				depth++;
			else if (*_text == ']')
				depth--;
			else if (*_text == '>' && depth == 1)
				break;
			_text++;
		}
		_text++;
		return nullptr;
	}

	XmlNode *node = newNode();
	const char *start = _text;

	(void)parent;
	while (!isNameEnd(*_text))
		_text++;
	if (_text == start)
		fail("expected element name");
	node->name.assign(start, _text - start);
	node->namePos = start - _begin;

	parseAttributes(node);

	if (*_text == '>') {
		_text++;
		parseContents(node);
	} else if (*_text == '/') {
		_text++;
		if (*_text != '>')
			fail("expected >");
		_text++;
	} else {
		fail("expected >");
	}
	return node;
}

void XmlParser::parseContents(XmlNode *node) {
	XmlNode *last = nullptr;

	for (;;) {
		const char *contentsStart = _text;

		skipWhitespace();

		if (*_text == '<') {
			if (_text[1] == '/') {
				_text += 2;

				const char *start = _text;

				while (!isNameEnd(*_text))
					_text++;
				if (strlen(node->name.c_str()) != (size_t)(_text - start) || strncmp(node->name.c_str(), start, _text - start) != 0)
					fail("invalid closing tag name");
				skipWhitespace();
				if (*_text != '>')
					fail("expected >");
				_text++;
				return;
			}

			_text++;

			XmlNode *child = parseNode(node);

			if (child) {
				if (last)
					last->next = child;
				else
					node->firstChild = child;
				last = child;
			}
		} else if (!*_text) {
			fail("unexpected end of data");
		} else {
			// text: a data node with no name (the leading white space is part of it)
			const char *start = contentsStart;

			while (*_text && *_text != '<')
				_text++;

			XmlNode *data = newNode();

			data->isData = true;
			data->namePos = start - _begin;
			data->value.assign(start, _text - start);
			if (last)
				last->next = data;
			else
				node->firstChild = data;
			last = data;
		}
	}
}

void XmlParser::Parse() {
	// (a byte order mark)
	if ((uint8_t)_text[0] == 0xEF && (uint8_t)_text[1] == 0xBB && (uint8_t)_text[2] == 0xBF)
		_text += 3;

	XmlNode *last = nullptr;

	for (;;) {
		skipWhitespace();
		if (!*_text)
			break;
		if (*_text != '<')
			fail("expected <");
		_text++;

		XmlNode *node = parseNode(nullptr);

		if (node) {
			if (last)
				last->next = node;
			else
				_document._firstNode = node;
			last = node;
		}
	}
}

void XmlDocument::Parse(const char *text) {
	Clear();
	XmlParser parser(*this, text);

	try {
		parser.Parse();
	} catch (...) {
		Clear();
		throw;
	}
}

void XmlDocument::Clear() {
	_firstNode = nullptr;
	_nodes.clear();
}
