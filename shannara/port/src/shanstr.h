// SHANSTR.DAT: Huffman + phrase-dictionary compressed message strings.
// Layout and decoder verified against the retail file (see analysis/retail_game_layer.md sections 16, 20).
#pragma once
#include "common.h"

namespace shannara {

class ShanStr {
public:
	explicit ShanStr(Bytes data);
	int blockCount() const { return (int)_blocks.size(); }
	int stringCount(int block) const { return _blocks[block].count; }
	// Decoded bytes of string `index` in `block` (message id = block << 16 | index). Latin-1, "\v" (0x0B) ends a message.
	std::string string(int block, int index) const;
	std::string string(uint32_t id) const { return string((int)(id >> 16), (int)(id & 0xFFFF)); }
	size_t computedEnd() const { return _end; }
	size_t fileSize() const { return _data.size(); }
	size_t phraseCount() const { return _phrases.size(); }

private:
	struct Block {
		uint16_t count;
		uint32_t bytes;
		size_t dataOffset;
		std::vector<uint16_t> lengths;
		std::vector<uint32_t> prefix; // cumulative length before each string
	};
	std::string decode(const uint8_t *src, size_t n) const;

	Bytes _data;
	std::vector<Block> _blocks;
	std::vector<int16_t> _tree; // pairs; negative = leaf (symbol = -v - 1); root = last pair
	std::vector<std::string> _phrases;
	size_t _end = 0;
};

} // namespace shannara
