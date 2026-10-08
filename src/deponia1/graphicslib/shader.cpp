#include "graphicslib/shader.h"

std::list<TShader *> shader_list;
std::vector<std::vector<TRenderPass> > shader_renderpasses;
int shader_buffers = 0;
int shader_transition = -1;

TShader *CreateShader() {
	return nullptr;
}

// Confirmed (asm lines 404590-404612, and the table `CSWTCH_1542` at 3141883): the blend modes of the engine for
// the numbers 0 to 10 of the scripts.
int CompositeEnums(int value) {
	static const int kModes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 11};

	if (value < 0 || value > 10)
		return 2;

	return kModes[value];
}

std::string base64_decode(const std::string &text) {
	std::string result;
	unsigned int accumulator = 0;
	int bits = 0;

	for (char c : text) {
		int value;

		if (c >= 'A' && c <= 'Z')
			value = c - 'A';
		else if (c >= 'a' && c <= 'z')
			value = c - 'a' + 26;
		else if (c >= '0' && c <= '9')
			value = c - '0' + 52;
		else if (c == '+')
			value = 62;
		else if (c == '/')
			value = 63;
		else
			continue;

		accumulator = (accumulator << 6) | value;
		bits += 6;

		if (bits >= 8) {
			bits -= 8;
			result += static_cast<char>((accumulator >> bits) & 0xFF);
		}
	}

	return result;
}
