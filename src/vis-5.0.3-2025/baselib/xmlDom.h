// A small XML document, standing in for rapidxml (the third-party library the original's TXMLProjectReader
// parses with, rapidxml::xml_document<char>::parse<0>). It parses the way rapidxml does with no flags: elements
// with attributes and their entity references, no nodes for declarations, comments and doctypes, a data node (with
// no name) for text that is not only white space. A malformed document throws XmlParseError.
#pragma once

#include <memory>
#include <string>
#include <vector>

struct XmlAttribute {
	std::string name;
	std::string value;
	XmlAttribute *next = nullptr;
};

struct XmlNode {
	std::string name;       ///< empty for a data node
	std::string value;      ///< the text of a data node
	size_t namePos = 0;     ///< where the name starts in the text that was parsed (for the progress of the load)
	bool isData = false;
	XmlNode *firstChild = nullptr;
	XmlNode *next = nullptr;
	XmlAttribute *firstAttribute = nullptr;

	std::vector<std::unique_ptr<XmlAttribute>> attributes;
};

struct XmlParseError {
	const char *what;
	size_t position;
};

class XmlDocument {
public:
	/** Parses the NUL terminated text; replaces the document. Throws XmlParseError. */
	void Parse(const char *text);
	/** The first element of the document (null if it has none). */
	XmlNode *GetFirstNode() const {
		return _firstNode;
	}
	void Clear();

private:
	friend class XmlParser;

	std::vector<std::unique_ptr<XmlNode>> _nodes;
	XmlNode *_firstNode = nullptr;
};
