#include "base64.h"

static const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64Value(char c) {
	if (c >= 'A' && c <= 'Z')
		return c - 'A';

	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;

	if (c >= '0' && c <= '9')
		return c - '0' + 52;

	if (c == '+')
		return 62;

	if (c == '/')
		return 63;

	return -1;
}

std::string base64_decode(const std::string &text) {
	std::string result;
	unsigned int accumulator = 0;
	int bits = 0;

	for (char c : text) {
		int value = base64Value(c);

		if (value < 0)
			break;

		accumulator = (accumulator << 6) | value;
		bits += 6;

		if (bits >= 8) {
			bits -= 8;
			result += static_cast<char>((accumulator >> bits) & 0xFF);
		}
	}

	return result;
}

std::string base64_encode(const unsigned char *data, unsigned int length) {
	std::string result;

	for (unsigned int i = 0; i < length; i += 3) {
		unsigned int chunk = data[i] << 16;
		unsigned int count = length - i;

		if (count > 1)
			chunk |= data[i + 1] << 8;

		if (count > 2)
			chunk |= data[i + 2];

		result += kAlphabet[(chunk >> 18) & 0x3F];
		result += kAlphabet[(chunk >> 12) & 0x3F];
		result += (count > 1) ? kAlphabet[(chunk >> 6) & 0x3F] : '=';
		result += (count > 2) ? kAlphabet[chunk & 0x3F] : '=';
	}

	return result;
}
