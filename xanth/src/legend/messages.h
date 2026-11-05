/* XANTHSTR.DAT / SHANSTR.DAT: all the game's text, Huffman coded in blocks.
 *
 * Layout: u16 blockCount; per block {u16 stringCount; u32 dataBytes}; u16 byte length of every string; the Huffman tree (u16 n, n words; negative = leaf -(symbol+1));
 * the phrase table (u16 k, k offsets, u16 poolSize, pool of NUL-terminated strings; symbols 0x80 and up expand to a phrase); then the blocks' bit-packed strings
 * (least significant bit first, a NUL symbol ends a string).
 * The object code refers to a string by message id 0xF0BBIIII: block BB, index IIII.
 */
#ifndef LEGEND_MESSAGES_H
#define LEGEND_MESSAGES_H

#include "common/types.h"

namespace Legend {

class MessageFile {
public:
	bool load(const Common::Bytes &data);
	size_t blockCount() const { return _blocks.size(); }
	size_t stringCount(size_t block) const { return block < _blocks.size() ? _blocks[block].lengths.size() : 0; }
	/** Decoded string (latin-1, with embedded control codes such as 0x0B and '\n'); empty if the id does not exist. */
	std::string string(size_t block, size_t index) const;
	std::string string(uint32 messageId) const { return string((messageId >> 16) & 0xFF, messageId & 0xFFFF); }
	/** True for ids of the form 0xF0BBIIII that exist in the file. */
	bool has(uint32 messageId) const {
		return (messageId >> 24) == 0xF0 && ((messageId >> 16) & 0xFF) < _blocks.size() && (messageId & 0xFFFF) < stringCount((messageId >> 16) & 0xFF);
	}

private:
	struct Block {
		uint32 dataOffset;
		std::vector<uint16> lengths;
		std::vector<uint32> offsets;
	};
	Common::Bytes _d;
	std::vector<Block> _blocks;
	std::vector<int16> _tree;
	std::vector<std::string> _phrases;
};

} // namespace Legend

#endif
