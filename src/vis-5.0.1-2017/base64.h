// Confirmed names and signatures (Deponia_Linux.asm: `base64_decode(std::string const&)` called at lines 395935,
// 433078; `base64_encode(unsigned char const*, unsigned int)` at 432451): the usual third-party base64 functions
// (alphabet A-Z a-z 0-9 + /, padded with `=`).
#pragma once

#include <string>

/** The bytes of a base64 text (it ends at the first `=` or character that is not base64). */
std::string base64_decode(const std::string &text);
/** The base64 text of the bytes. */
std::string base64_encode(const unsigned char *data, unsigned int length);
